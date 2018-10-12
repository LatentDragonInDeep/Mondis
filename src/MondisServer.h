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

#ifdef WIN32

#include <winsock2.h>
#include <inaddr.h>
#include <stdio.h>

#elif defined(linux)
#include <sys/epoll.h>
#include <unistd.h>
#endif

#include "HashMap.h"
#include "Command.h"
#include "JSONParser.h"

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

class MultiCommand;

class MondisClient;

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
    int maxCilentNum = 1024;
    int maxCommandReplicaBufferSize = 1024 * 1024;
    int maxCommandPropagateBufferSize = 1024;
    int maxUndoCommandBufferSize = 1024;
    int maxSlaveNum = 1024;
    int maxSlaveIdle = 10000;
    int maxMasterIdle = 10000;
    int maxClientIdle = 10000;
    int toSlaveHeartBeatDuration = 1000;
    int toClientHeartBeatDuration = 1000;

    pid_t pid;
    std::string configfile;
    std::string executable;
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
    clock_t preSync = clock();

    string recoveryFile;
    string recoveryStrategy;
    string aofFile;
    string jsonFile;
    bool isLoading = false;
    bool isRecovering = false;
    bool isReplicatingFromMaster = false;
    bool isMaster = false;
    unordered_set<MondisClient *> slaves;
    unordered_set<MondisClient *> clients;
    unordered_set<MondisClient *> peers;

    unsigned long long replicaOffset = 0;
    deque<string> *replicaCommandBuffer;
    static unordered_set<CommandType> modifyCommands;
    static unordered_set<CommandType> transactionAboutCommands;

    static void initStaticMember();

    bool isPropagating = false;
    condition_variable propagateCV;
    mutex propagateMtx;

    queue<string> *commandPropagateBuffer;
    
    string masterUsername;
    string masterPassword;
    string masterIP;
    string masterPort;
    
    bool slaveOf=false;

    MondisClient *master = nullptr;
#ifdef WIN32
    fd_set clientFds;
    fd_set slaveFds;
    unordered_map<SOCKET *, MondisClient *> socketToClient;

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
        while ((ret = recv(sock, buffer, sizeof(buffer), 0)) != 0) {
            res += string(buffer, ret);
        }
        return res;
    };

    void selectAndHandle(fd_set &fds) {
        while (true) {
            int ret = select(0, &fds, nullptr, nullptr, nullptr);
            if (ret == 0) {
                continue;
            }
            for (auto &pair:socketToClient) {
                if (FD_ISSET(*pair.first, &clientFds)) {
                    MondisClient *client = pair.second;
                    handleCommand(client);
                }
            }
        }
    }
#elif defined(linux)
    int clientsEpollFd;
    int slavesEpollFd;
    epoll_event* clientEvents;
    epoll_event* slaveEvents;
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
    void selectAndHandle(int epollFd,epoll_event* events) {
        while (true) {
            int nfds = epoll_wait(epollFd, events, maxCilentNum, -1);
            for(int i=0;i<nfds;i++) {
                MondisClient* client = fdToClient[events[i].data.fd];
                handleCommand(client);
            }
        }
    }
#endif

    bool hasLogin = true;
public:
    CommandInterpreter *interpreter;

    MondisServer();

    ~MondisServer();

    int start(string& confFile);

    ExecutionResult execute(Command *command, MondisClient *client);

    static JSONParser *getJSONParser();

    bool handleWatchedKey(const string &key);

    bool putToPropagateBuffer(const string &curCommand);

    void incrReplicaOffset() {
        replicaOffset++;
    };

    void decrReplicaOffset() {
        replicaOffset--;
    };

    void appendLog(string &commandStr, ExecutionResult &res);

    void appendAof(const string &command);

    MondisObject *chainLocate(Command *command, MondisClient *client);

    static bool isModifyCommand(Command *command) {
        if (command->type == LOCATE) {
            while (command->next->type == LOCATE) {
                command = command->next;
            }
        }
        return modifyCommands.find(command->type) != modifyCommands.end();
    };

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

    void handleCommand(MondisClient *client);

    void acceptSocket();

    void sendToMaster(const string &res);

    string readFromMaster();

    void replicaToSlave(MondisClient *client, unsigned dbIndex, unsigned long long slaveReplicaOffset);

    void singleCommandPropagate();

    void replicaCommandPropagate(vector<string> &commands, MondisClient *client);

    string takeFromPropagateBuffer();

    condition_variable notEmpty;
    mutex notEmptyMtx;

    unordered_map<string, unordered_set<MondisClient *>> keyToWatchedClients;

    MondisClient *self = nullptr;

    void checkAndHandleIdleClient();

    void closeClient(MondisClient *c);

    thread *recvFromMaster = nullptr;
    thread *sendHeartBeatToSlaves = nullptr;
    thread *sendHeartBeatToClients = nullptr;

    bool autoMoveCommandToMaster = true;

    void saveAll(const string &jsonFile);

    bool forbidOtherModifyInTransaction = false;

    static unordered_set<CommandType> serverCommand;
};

enum ClientType {
    MASTER,
    SLAVE,
    CLIENT,
    PEER,
    SERVER_SELF,
};

class MondisClient {
public:
    uint64_t id;            /* Client incremental unique ID. */
#ifdef WIN32
    SOCKET sock;
#elif defined(linux)
    int fd;/* Client socket. */
#endif
    ClientType type = CLIENT;
    int curDbIndex = 0;
    HashMap *keySpace = nullptr;            /* Pointer to currently SELECTed DB. */
    string name;             /* As set by CLIENT SETNAME. */
    vector<string> commandBuffer;         /* Buffer we use to accumulate client queries. */
    int curCommandIndex = 0;
    vector<ExecutionResult> *reply;            /* List of reply objects to sendToMaster to the client. */
    time_t ctime;           /* Client creation time. */
    time_t lastinteraction; /* Time of the last interaction, used for timeout */

    string ip;
    string port;

    clock_t preInteraction = clock();

    unordered_set<string> watchedKeys;
    unordered_set<string> modifiedKeys;

    queue<string> *transactionCommands = nullptr;

    deque<MultiCommand *> *undoCommands = nullptr;

    bool isInTransaction = false;
    bool watchedKeysHasModified = false;

    int hasExecutedCommandNumInTransaction = 0;
public:
    bool hasLogin = false;
private:
    static int nextId;
public:
#ifdef WIN32

    MondisClient(SOCKET sock);

#elif defined(linux)
    MondisClient(int fd);

#endif

    MondisClient(ClientType t) : type(t) {};

    ~MondisClient();

    string readCommand();

    void send(const string &res);

    void updateHeartBeatTime();

    void startTransaction();

    void closeTransaction();

    ExecutionResult commitTransaction(MondisServer *server);
};




#endif //MONDIS_MONDISSERVER_H
