//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_MONDISSERVER_H
#define MONDIS_MONDISSERVER_H


#include <sys/types.h>
#include <string>
#include <time.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <shared_mutex>

#ifdef WIN32

#include <winsock2.h>
#include <winsock.h>
#include <inaddr.h>
#include <stdio.h>

#elif defined(linux)
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#endif

#include "HashMap.h"
#include "Command.h"
#include "JSONParser.h"
#include "SplayTree.h"
#include "BlockingQueue.h"
#include "mondis.pb.h"
#include "TimerHeap.h"

using namespace std;
class Log{
private:
    string currentTime;
    string &command;
    ExecRes& res;
public:
    Log(string &command, ExecRes &res) : command(command), res(res) {
        time_t t = time(0);
        char ch[64];
        strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S", localtime(&t));
        currentTime = ch;
    }
    string toString() {
        string res;
        res+=currentTime;
        res+='\t';
        res += command;
        res+='\t';
        res += this->res.getTypeStr();
        if (this->res.type != OK) {
            res += '\t';
            res += this->res.desc;
        }
        res += '\n';

        return res;
    }

};

class MondisServer;

enum ClientType {
    MASTER,
    PEER,
    CLIENT,
    SERVER_SELF,
    SLAVE,
    TIMER,
};

class MondisClient {
public:
    friend class MondisServer;
    unsigned id;
#ifdef WIN32
    SOCKET sock;
#elif defined(linux)
    int fd;
#endif
    ClientType type = CLIENT;
    int dBIndex = 0;
    string ip;
    int port;
    long long preInteraction = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
    unordered_set<string> watchedKeys;
    unordered_set<string> modifiedKeys;

    queue<string> *transactionCommands = nullptr;

    deque<MultiCommand *> *undoCommands = nullptr;

    bool isInTransaction = false;
    bool watchedKeysHasModified = false;

    int hasExecutedCommandNumInTransaction = 0;
    bool hasAuthenticate = true;
private:
    queue<mondis::Message*> recvMsgs;
    char * halfPacketBuffer = nullptr;
    unsigned nextMessageLen = 0;
    mondis::Message* nextMsg = nullptr;
    unsigned nextMsgHasRecv = 0;
    unsigned nextDataLenHasRecv = 0;
    char * nextDataLenBuffer = new char[4];
#ifdef WIN32

    MondisClient(MondisServer *server, SOCKET sock);

#elif defined(linux)
    MondisClient(MondisServer* server,int fd);

#endif

    ~MondisClient();

    void updateHeartBeatTime();

    void startTransaction();

    void closeTransaction();

    ExecRes commitTransaction(MondisServer *server);

private:

    void readMessage();

public:
    mondis::Message* nextMessage();

    void writeMessage(mondis::Message* msg);

private:
};

class CommandStruct {
public:
    Command *locate = nullptr;
    Command *operation = nullptr;
    MondisObject *obj = nullptr;
    bool isModify = false;
    bool isLocate = false;
};

enum ServerStatus {
    SV_STAT_MASTER,
    SV_STAT_SLAVE,
    SV_STAT_UNDETERMINED,
};

enum RunStatus {
    LOADING,
    RECOVERING,
    REPLACTING,
    RUNNING,
};

typedef ExecRes (MondisServer::*CommandHandler)(Command*,MondisClient*);

enum SendToType {
    ALL_CLIENTS,
    ALL_PEERS,
    SPECIFY_CLIENT,
    SPECIFY_PEER,
    NO_ONE,
};

class Action {
public:
    mondis::Message * msg = nullptr;
    MondisClient* client = nullptr;
};

class ActionResult {
public:
    mondis::Message * msg = nullptr;
    SendToType sendToType = SendToType::SPECIFY_CLIENT;
    MondisClient* client = nullptr;
};

class MondisServer {
private:
    static MondisServer* server;
    friend class MondisClient;
    int maxClientNum = 1024;
    int maxCommandReplicaBufferSize = 1024 * 1024;
    int maxCommandPropagateBufferSize = 1024;
    int maxSlaveNum = 1024;
    deque<string> * replicaCommandBuffer = new deque<string>();
    bool isSlaveOf = false;
    pid_t pid;
    std::string configfile;
    int port = 56379;
    int databaseNum = 16;
    bool aof = true;
    int aofSyncStrategy = 1;
    bool json = true;
    int jsonDuration = 10;
    string workDir;
    string logFile;
    vector<HashMap*> dbs;
    int daemonize = false;
    static JSONParser parser;
    ofstream logFileOut;
    ofstream aofFileOut;
    ofstream jsonFileOut;
    ifstream recoveryFileIn;
    unordered_map<string,string> conf;
    string username = "root";
    string password = "admin";
    long long preSync = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();

    string recoveryFile;
    string recoveryStrategy;
    string aofFile;
    string jsonFile;
    ServerStatus serverStatus;
    RunStatus runStatus;
    unordered_map<unsigned, MondisClient *> idToPeers;
    unordered_map<unsigned, MondisClient *> idToClients;
    unordered_map<unsigned,MondisClient*> idToPeersAndClients;
    BlockingQueue<Action> readQueue;
    BlockingQueue<ActionResult> writeQueue;
    void writeToClient();
public:
    void putToReadQueue(Action &action);
    void putToWriteQueue(ActionResult &actionResult);
    void putCommandMsgToWriteQueue(const string &cmdStr, unsigned int clientId, mondis::CommandType commandType,
                                       SendToType sendToType);
    void putExecResMsgToWriteQueue(const ExecRes &res, unsigned int clientId, SendToType sendToType);
private:
    unsigned long long replicaOffset = 0;
    unsigned long long maxOtherReplicaOffset = 0;
    static unordered_set<CommandType> modifyCommands;
    static unordered_set<CommandType> transactionAboutCommands;
    static unordered_map<CommandType,CommandHandler> commandHandlers;
    bool isPropagating = false;
    shared_mutex allModifyMtx;
    shared_mutex clientModifyMtx;
    shared_mutex peersModifyMtx;
    shared_mutex watchedKeyMtx;
    string masterUsername;
    string masterPassword;
    string masterIP;
    int masterPort;
    bool hasVoteFor = false;
    unordered_set<MondisClient*> maxOffsetClients;
    MondisClient *master = nullptr;
#ifdef WIN32
    fd_set fds;
    unordered_map<SOCKET, MondisClient *> socketToClient;

#elif defined(linux)
    int epollFd;
    epoll_event* epollEvents;
    struct epoll_event listenEvent;
    unordered_map<int,MondisClient*> fdToClients;
#endif
    void selectAndHandle();
public:
    static MondisServer* getInstance();
    unsigned id;
    CommandInterpreter *interpreter;

private:
    MondisServer();
public:
    ~MondisServer();

    int start(string& confFile);

    ExecRes execute(Command *command, MondisClient *client);

    static JSONParser *getJSONParser();

    bool handleWatchedKey(const string &key);

    void incrReplicaOffset();

    void appendLog(const string &commandStr, ExecRes &res);

    void appendAof(const string &command);

    MondisObject *chainLocate(Command *command, MondisClient *client);

    static bool isModifyCommand(Command *command);;

    MultiCommand *getUndoCommand(CommandStruct &cstruct, MondisClient *client);

    void undoExecute(MultiCommand *command, MondisClient *client);

    ExecRes transactionExecute(CommandStruct &cstruct, MondisClient *client);

    CommandStruct getCommandStruct(Command *command, MondisClient *client);
private:
    void runAsDaemon();

    void init();

    void msgHandle();

    void serverEventLoop();
    void applyConf();

    void parseConfFile(string& confFile);

    ExecRes execute(const string &commandStr, MondisClient *client);

    void acceptSocket();

    void replicaToSlave(MondisClient *client, long long slaveReplicaOffset);

    void replicaCommandPropagate(vector<string> commands,MondisClient* client);

    unordered_map<string, unordered_set<MondisClient *>> keyToWatchedClients;

    MondisClient *self = nullptr;

    void closeClient(MondisClient *c);

    thread *msgHandler = nullptr;
    thread *msgWriter = nullptr;
    thread *timer = nullptr;
    bool autoMoveCommandToMaster = true;

    void saveAll(const string &jsonFile);

    bool forbidOtherModifyInTransaction = false;

    static unordered_set<CommandType> controlCommands;
    static unordered_set<CommandType> clientControlCommands;

    MondisClient *buildConnection(const string &ip, int port);

    void getJson(string *res);

    unsigned nextClientId();

    unsigned voteNum = 0;

    const unsigned maxVoteIdle = 10000;

    bool isVoting = false;

    chrono::time_point<chrono::system_clock> preHeartBeat = chrono::system_clock::now();
    TimerHeap timeHeap;
    condition_variable syncFin;
    mutex syncFinMtx;
    BlockingQueue<ExecRes> resQueue;
    ExecRes bindKey(Command *, MondisClient *);
    ExecRes get(Command*,MondisClient*);
    ExecRes del(Command*,MondisClient*);
    ExecRes exsits(Command*,MondisClient*);
    ExecRes login(Command*,MondisClient*);
    ExecRes type(Command*,MondisClient*);
    ExecRes setTTl(Command*,MondisClient*);
    ExecRes selectDb(Command *, MondisClient *);
    ExecRes save(Command*,MondisClient*);
    ExecRes saveAll(Command*,MondisClient*);
    ExecRes renameKey(Command *, MondisClient *);
    ExecRes size(Command*,MondisClient*);
    ExecRes beSlaveOf(Command *, MondisClient *);
    ExecRes sync(Command*,MondisClient*);
    ExecRes disconnectClient(Command*,MondisClient*);
    ExecRes deletePeer(Command *, MondisClient *);
    ExecRes multi(Command*,MondisClient*);
    ExecRes exec(Command*,MondisClient*);
    ExecRes discard(Command*,MondisClient*);
    ExecRes watch(Command*,MondisClient*);
    ExecRes unwatch(Command*,MondisClient*);
    ExecRes getMasterInfo(Command *, MondisClient *);
    ExecRes ensureIdentity(Command *command, MondisClient *client);
    ExecRes askForVote(Command*,MondisClient*);
    ExecRes vote(Command*,MondisClient*);
    ExecRes unvote(Command*,MondisClient*);
    ExecRes masterDead(Command*,MondisClient*);
    ExecRes iAmNewMaster(Command*,MondisClient*);
    ExecRes updateOffset(Command*,MondisClient*);
    ExecRes clientList(Command*,MondisClient*);
    ExecRes newPeer(Command*,MondisClient*);
    ExecRes mondisExit(Command *, MondisClient *);
};




#endif //MONDIS_MONDISSERVER_H
