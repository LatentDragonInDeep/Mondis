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
    pid_t pid;
    std::string configfile;
    std::string executable;
    int port = 6379;
    int databaseNum = 16;
    bool aof = true;
    string aofSyncStrategy = "osDefault";
    bool json = true;
    int jsonDuration = 10;
    string slaveof = "127.0.0.1";
    string workDir;
    string logFile = "D:\\MondisWork\\log.txt";
    HashMap* curKeySpace;
    vector<HashMap *> dbs;
    int curDbIndex = 0;
    int daemonize = false;
    static JSONParser parser;
    ofstream logFileOut;
    ofstream aofFileOut;
    ofstream jsonFileOut;
    unordered_map<string,string> conf;
    Executor* executor;
    CommandInterpreter* interpreter;
    string username = "root";
    string password = "admin";

    string aofFile;
    string jsonFile;
    bool isLoading;
    bool isRecovering;

    bool hasLogin = true;
public:
    int start(string& confFile);
    int runAsDaemon();

    void init();

    int save(string &jsonFile);
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
