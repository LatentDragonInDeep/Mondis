//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_MONDISSERVER_H
#define MONDIS_MONDISSERVER_H


#include <sys/types.h>
#include <string>
#include <time.h>
#include <fstream>
#include "HashMap.h"
#include "MondisClient.h"
#include "Command.h"
#include "JSONParser.h"

class Log{
private:
    string currentTime;
    Command& command;
    ExecutionResult& res;
public:
    Log(Command& command,ExecutionResult& res):command(command),res(res) {
        time_t t = time(0);
        char ch[64];
        strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S", localtime(&t));
        currentTime = ch;
    }
    string toString() {
        string res;
        res+=currentTime;
        res+='\t';
        res+=command.toString();
        res+='\t';
        res+=this->res.toString();

        return res;
    }

};
class Executor;
class MondisServer {
private:
    /* General */
    pid_t pid;                  /* Main process pid. */
    std::string configfile;           /* Absolute config file path, or NULL */
    std::string executable;           /* Absolute executable file path. */
    std::string logFile;
    char **execArgs;           /* Executable argv vector (copy). */

    HashMap* curKeySpace;
    HashMap** allDbs;
    int curDbIndex = 0;

    /* Networking */
    int port;                   /* TCP listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
    char *unixsocket;           /* UNIX socket path */
    mode_t unixsocketperm;      /* UNIX socket permission */
    vector<int> tcpFds; /* TCP socket file descriptors */
    int ipfd_count;             /* Used slots in ipfd[] */
    int sofd;                   /* Unix socket file descriptor */

    vector<MondisClient*> *clients;              /* List of active clients */
    vector<MondisClient*> *clients_to_close;     /* Clients to close asynchronously */
    vector<MondisClient*> *clients_pending_write; /* There is to write or install handler. */

    MondisClient *current_client; /* Current client, only used on crash report */

    /* RDB / AOF loading information */
    bool loading;                /* We are loading data from disk if true */
    bool isJSONSerialization = false;
    bool isAOFSerialization  = false;

    int verbosity;                  /* Loglevel in redis.conf */
    int maxidletime;                /* Client timeout in seconds */
    bool tcpkeepalive;               /* Set SO_KEEPALIVE if non-zero. */

    int daemonize;                  /* True if running as a daemon */

    JSONParser parser;
    ofstream logFileOut;
    ofstream aofFileOut;
    unordered_map<string,string> conf;
    Executor* executor;
    CommandInterpreter* interpreter;

public:
    int start(string& confFile);
    int runAsDaemon();
    int save();
    int startEventLoop();
    void applyConf();
    int appendLog(Log& log);
    void appendOnly(Command& command);
    void parseConfFile(string& confFile);
    ExecutionResult execute(string& commandStr);

    ExecutionResult execute(Command *command);

    ExecutionResult locateExecute(Command *command);
    static JSONParser* getJSONParser();
};

class Executor {
public:

    ExecutionResult execute(Command* command);
    static Executor* getExecutor();
    static void destroyCommand(Command* command);
    void bindServer(MondisServer* server);
private:
    Executor();
    Executor(Executor&) = default;
    Executor(Executor&&) = default;
    Executor&operator=(Executor&) = default;
    Executor&operator=(Executor&&) = default;
    static Executor* executor;
    MondisServer* server;
    static unordered_set<CommandType> serverCommand;
    static void init();
};




#endif //MONDIS_MONDISSERVER_H
