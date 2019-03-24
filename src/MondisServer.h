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

class Log{
private:
    string currentTime;
    string &command;
    ExecutionResult& res;
public:
    Log(string &command, ExecutionResult &res) : command(command), res(res) {
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
            res += this->res.res;
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
    int curDbIndex = 0;
    HashMap *keySpace = nullptr;
    string name;
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

    void send(const string &res);

    void updateHeartBeatTime();

    void startTransaction();

    void closeTransaction();

    ExecutionResult commitTransaction(MondisServer *server);

    string read();
};

class CommandStruct {
public:
    Command *locate = nullptr;
    Command *operation = nullptr;
    MondisObject *obj = nullptr;
    bool isModify = false;
    bool isLocate = false;

    CommandStruct() {};

    CommandStruct(CommandStruct &other) {
        locate = other.locate;
        operation = other.operation;
        obj = other.obj;
        other.locate = nullptr;
        other.operation = nullptr;
        other.obj = nullptr;
        isModify = other.isModify;
        isLocate = other.isLocate;
    }
};

class MondisServer {
private:
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
    bool isLoading = false;
    bool isRecovering = false;
    bool isReplicatingFromMaster = false;
    bool isMaster = false;
    bool isSlave = false;
    unordered_map<unsigned, MondisClient *> idToPeers;
    unordered_map<string, MondisClient *> nameToClients;

    long long replicaOffset = 0;
    deque<string> *replicaCommandBuffer;
    static unordered_set<CommandType> modifyCommands;
    static unordered_set<CommandType> transactionAboutCommands;

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
    
    bool slaveOf=false;
    bool hasVoteFor = false;

    unordered_set<MondisClient *> maxOffsetClients;

    MondisClient *master = nullptr;
#ifdef WIN32
    fd_set clientFds;
    fd_set peerFds;
    unordered_map<SOCKET, MondisClient *> socketToClient;

    void send(SOCKET &sock, const string &res) {
        char buffer[4096];
        int ret;
        const char *data = res.data();
        int hasWrite = 0;
        while (hasWrite < res.size()) {
            ret = ::send(sock, data + hasWrite, res.size() - hasWrite, 0);
            hasWrite += ret;
        }
    };

    string read(SOCKET &sock) {
        string res;
        char buffer[4096];
        int ret;
        while (true) {
            ret = recv(sock, buffer, sizeof(buffer), 0);
            if (ret == SOCKET_ERROR) {
                return res;
            }
            res += string(buffer, ret);
        }
    };
#elif defined(linux)
    int clientsEpollFd;
    int peersEpollFd;
    epoll_event* clientEvents;
    epoll_event* peerEvents;
    struct epoll_event listenEvent;
    unordered_map<int,MondisClient*> fdToClient;
    void send(int fd, const string& data) {
        char buffer[4096];
        int ret;
        const char *toWrite = data.data();
        int hasWrite = 0;
        while (hasWrite<data.size()) {
            ret = write(fd,toWrite+hasWrite,data.size()-hasWrite);
            hasWrite+=ret;
        }
    };
    string read(int fd) {
        string res;
        char buffer[4096];
        int ret;
        while ((ret = ::read(fd, buffer, sizeof(buffer))) != 0) {
            res += string(buffer, ret);
        }
        return res;
    };
#endif

    void selectAndHandle(bool isClient);
public:
    unsigned id;
    CommandInterpreter *interpreter;

    MondisServer();

    ~MondisServer();

    int start(string& confFile);

    ExecutionResult execute(Command *command, MondisClient *client);

    static JSONParser *getJSONParser();

    bool handleWatchedKey(const string &key);

    bool putToPropagateBuffer(const string &curCommand);

    void incrReplicaOffset();

    void appendLog(string &commandStr, ExecutionResult &res);

    void appendAof(const string &command);

    MondisObject *chainLocate(Command *command, MondisClient *client);

    static bool isModifyCommand(Command *command);;

    MultiCommand *getUndoCommand(CommandStruct &cstruct, MondisClient *client);

    void undoExecute(MultiCommand *command, MondisClient *client);

    ExecutionResult transactionExecute(CommandStruct &cstruct, MondisClient *client);

    CommandStruct getCommandStruct(Command *command, MondisClient *client);
private:
    void runAsDaemon();

    void init();

    void save(string &jsonFile, int dbIndex);

    void startEventLoop();
    void applyConf();

    void parseConfFile(string& confFile);

    ExecutionResult execute(string &commandStr, MondisClient *client);

    void acceptSocket();

    void sendToMaster(const string &res);

    string readFromMaster(bool isBlocking);

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

    thread *recvFromMaster = nullptr;
    thread *sendHeartBeatToSlaves = nullptr;
    thread *sendHeartBeatToClients = nullptr;
    thread *recvFromSlaves = nullptr;
    thread *propagateIO = nullptr;
    bool autoMoveCommandToMaster = true;

    void saveAll(const string &jsonFile);

    bool forbidOtherModifyInTransaction = false;

    static unordered_set<CommandType> controlCommands;
    static unordered_set<CommandType> clientControlCommands;

    ExecutionResult beSlaveOf(Command *command, MondisClient *client, string &masterUsername, string &masterPassword);

    MondisClient *buildConnection(const string &ip, int port);

    void getJson(string *res);

    static unsigned curClientId;

    unsigned nextPeerId();

    static string nextDefaultClientName();

    unsigned voteNum = 0;

    thread *forVote = nullptr;

    void askForVote();

    const unsigned maxVoteIdle = 10000;

    bool isVoting = false;
};




#endif //MONDIS_MONDISSERVER_H
