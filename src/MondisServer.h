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
};

class MondisClient {
public:
    unsigned id;            /* Client incremental unique ID. */
#ifdef WIN32
    SOCKET sock;
#elif defined(linux)
    int fd;/* Client socket. */
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
};

enum RunStatus {
    LOADING,
    RECOVERING,
    REPLACTING,
    RUNNING,
};

typedef ExecRes (MondisServer::*CommandHandler)(Command*,MondisClient*);

class MondisServer {
private:
    static MondisServer* server;
    friend class MondisClient;
    int maxClientNum = 1024;
    int maxCommandReplicaBufferSize = 1024 * 1024;
    int maxCommandPropagateBufferSize = 1024;
    int maxSlaveNum = 1024;
    int maxSlaveIdle = 10000;
    int maxMasterIdle = 10000;
    int maxClientIdle = 10000;
    int toSlaveHeartBeatDuration = 3000;
    int toClientHeartBeatDuration = 3000;
    deque<string> * replicaCommandBuffer = new deque<string>();
    bool isSlaveOf = false;
    pid_t pid;
    std::string configfile;
    int port = 6379;
    int databaseNum = 16;
    bool aof = true;
    int aofSyncStrategy = 1;
    bool json = true;
    int jsonDuration = 10;
    string workDir;
    string logFile;
    vector<HashMap *> dbs;
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
    BlockingQueue<mondis::Message*> readQueue;
    BlockingQueue<mondis::Message*> writeQueue;
    void writeToClient();
public:
    void putToReadQueue(mondis::Message *msg);
    void putToWriteQueue(mondis::Message* msg);
    void putCommandMsgToWriteQueue(const string &cmdStr, unsigned int clientId, mondis::CommandType commandType,
                                       mondis::SendToType sendToType);
    void putExecResMsgToWriteQueue(const ExecRes &res, unsigned int clientId, mondis::SendToType sendToType);
private:
    long long replicaOffset = 0;
    static unordered_set<CommandType> modifyCommands;
    static unordered_set<CommandType> transactionAboutCommands;
    static unordered_map<CommandType,CommandHandler> commandHandlers;
    static void initStaticMember();

    bool isPropagating = false;
    condition_variable redirectCV;
    mutex redirectMtx;

    shared_mutex allModifyMtx;
    shared_mutex clientModifyMtx;
    shared_mutex peersModifyMtx;
    shared_mutex watchedKeyMtx;

    bool isRedirecting = false;

    queue<string> *commandPropagateBuffer;
    
    string masterUsername;
    string masterPassword;
    string masterIP;
    int masterPort;
    bool hasVoteFor = false;
    unordered_set<MondisClient *> maxOffsetClients;
    MondisClient *master = nullptr;
#ifdef WIN32
    fd_set clientFds;
    fd_set peerFds;
    unordered_map<SOCKET, MondisClient *> socketToClient;

#elif defined(linux)
    int clientsEpollFd;
    int peersEpollFd;
    epoll_event* clientEvents;
    epoll_event* peerEvents;
    struct epoll_event listenEvent;
    unordered_map<int,MondisClient*> fdToClient;
#endif

    void selectAndHandle(bool isClient);
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

    bool putToPropagateBuffer(const string &curCommand);

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

    void startEventLoop();
    void applyConf();

    void parseConfFile(string& confFile);

    ExecRes execute(const string &commandStr, MondisClient *client);

    void acceptSocket();

    void replicaToSlave(MondisClient *client, long long slaveReplicaOffset);

    void singleCommandPropagate();

    void replicaCommandPropagate(vector<string> &commands, MondisClient *client);

    string takeFromPropagateBuffer();

    condition_variable notEmpty;
    mutex notEmptyMtx;

    unordered_map<string, unordered_set<MondisClient *>> keyToWatchedClients;

    MondisClient *self = nullptr;

    void checkAndHandleIdleConnection();

    void closeClient(MondisClient *c);

    thread *sendHeartBeatToSlaves = nullptr;
    thread *propagateIO = nullptr;
    thread *msgHandler = nullptr;
    thread *msgWriter = nullptr;
    bool autoMoveCommandToMaster = true;

    void saveAll(const string &jsonFile);

    bool forbidOtherModifyInTransaction = false;

    static unordered_set<CommandType> controlCommands;
    static unordered_set<CommandType> clientControlCommands;

    MondisClient *buildConnection(const string &ip, int port);

    void getJson(string *res);

    static unsigned curClientId;

    unsigned nextClientId();

    static string nextDefaultClientName();

    unsigned voteNum = 0;

    const unsigned maxVoteIdle = 10000;

    bool isVoting = false;

    deque<ExecRes> resQueue;
    ExecRes bindKey(Command *, MondisClient *);
    ExecRes get(Command*,MondisClient*);
    ExecRes del(Command*,MondisClient*);
    ExecRes exsits(Command*,MondisClient*);
    ExecRes login(Command*,MondisClient*);
    ExecRes type(Command*,MondisClient*);
    ExecRes selectDb(Command *, MondisClient *);
    ExecRes save(Command*,MondisClient*);
    ExecRes saveAll(Command*,MondisClient*);
    ExecRes renameKey(Command *, MondisClient *);
    ExecRes size(Command*,MondisClient*);
    ExecRes beSlaveOf(Command *, MondisClient *);
    ExecRes sync(Command*,MondisClient*);
    ExecRes disconnectClient(Command*,MondisClient*);
    ExecRes disconnectSlave(Command*,MondisClient*);
    ExecRes ping(Command*,MondisClient*);
    ExecRes pong(Command*,MondisClient*);
    ExecRes multi(Command*,MondisClient*);
    ExecRes exec(Command*,MondisClient*);
    ExecRes discard(Command*,MondisClient*);
    ExecRes watch(Command*,MondisClient*);
    ExecRes unwatch(Command*,MondisClient*);
    ExecRes getMasterInfo(Command *, MondisClient *);
    ExecRes newClient(Command *command, MondisClient *client);
    ExecRes askForVote(Command*,MondisClient*);
    ExecRes vote(Command*,MondisClient*);
    ExecRes unvote(Command*,MondisClient*);
    ExecRes masterDead(Command*,MondisClient*);
    ExecRes iAmNewMaster(Command*,MondisClient*);
    ExecRes updateOffset(Command*,MondisClient*);
    ExecRes clientInfo(Command*,MondisClient*);
    ExecRes clientList(Command*,MondisClient*);
    ExecRes slaveInfo(Command*,MondisClient*);
    ExecRes slaveList(Command*,MondisClient*);
    ExecRes newPeer(Command*,MondisClient*);
    ExecRes exit(Command*,MondisClient*);

};




#endif //MONDIS_MONDISSERVER_H
