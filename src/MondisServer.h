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

#ifdef WIN32

#include <winsock2.h>

#endif

#include "HashMap.h"
#include "MondisClient.h"
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

class Executor;
class MondisServer {
private:
    pid_t pid;
    std::string configfile;
    std::string executable;
    int port = 6379;
    int databaseNum = 16;
    bool aof = true;
    int aofSyncStrategy = 1;
    bool json = true;
    int jsonDuration = 10;
    string slaveof = "127.0.0.1";
    string workDir;
    string logFile;
    HashMap* curKeySpace;
    vector<HashMap *> dbs;
    int curDbIndex = 0;
    int daemonize = false;
    static JSONParser parser;
    ofstream logFileOut;
    ofstream aofFileOut;
    ofstream jsonFileOut;
    ifstream recoveryFileIn;
    unordered_map<string,string> conf;
    Executor* executor;
    CommandInterpreter* interpreter;
    string username = "root";
    string password = "admin";
    clock_t preSync = clock();

    string recoveryFile;
    string recoveryStrategy;
    string aofFile;
    string jsonFile;
    bool isLoading;
    bool isRecovering;
#ifdef WIN32
    fd_set fds;
    unordered_map<SOCKET *, MondisClient *> socketToClient;
#elif defined(linux)
    unordered_map<int,MondisClient*> fdToClient;
#endif

    bool hasLogin = true;
public:
    MondisServer();
    int start(string& confFile);
    int runAsDaemon();

    void init();

    int save(string &jsonFile);
    int startEventLoop();
    void applyConf();
    int appendLog(Log& log);
    void parseConfFile(string& confFile);

    ExecutionResult execute(string &commandStr, MondisClient *client);

    ExecutionResult execute(Command *command, MondisClient *client);

    void handleCommand(MondisClient *client);

    void selectAndHandle();
    ExecutionResult locateExecute(Command *command);
    static JSONParser* getJSONParser();

    void acceptClient();
};

class Executor {
public:

    ExecutionResult execute(Command* command);
    static Executor* getExecutor();
    static void destroyCommand(Command* command);

    static void bindServer(MondisServer *server);
private:
    Executor();
    Executor(Executor&) = default;
    Executor(Executor&&) = default;
    Executor&operator=(Executor&) = default;
    Executor&operator=(Executor&&) = default;
    static Executor* executor;
    static MondisServer *server;
    static unordered_set<CommandType> serverCommand;
public:
    static void init();
};




#endif //MONDIS_MONDISSERVER_H
