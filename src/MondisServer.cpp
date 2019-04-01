//
// Created by 11956 on 2018/9/5.
//
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <thread>
#include <random>
#include <memory>

#ifdef WIN32

#include <winsock2.h>
#include <inaddr.h>
#include <stdio.h>

#elif defined(linux)
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>

#endif

#include "Command.h"
#include "MondisServer.h"

#define ERROR_EXIT(MESSAGE) cout<<MESSAGE;\
                            exit(1);

#define ADD_AND_RETURN(RES, UNDO) RES->operations.push_back(UNDO);\
                                return RES;
#define TO_FULL_KEY_NAME(DBINDEX, KEY) to_string(DBINDEX)+"_"+KEY

#define PARAM_TYPE_STRING Command::ParamType::STRING
#define PARAM_TYPE_PLAIN Command::ParamType::PLAIN

#define CHECK_COMMAND_FROM_SLAVE if (serverStatus!=ServerStatus::SV_STAT_MASTER||client->type!=ClientType::SLAVE) { \
INVALID_AND_RETURN\
}

#define CHECK_COMMAND_FROM_CLIENT if (serverStatus!=ServerStatus::SV_STAT_MASTER||client->type!=ClientType::CLIENT) { \
INVALID_AND_RETURN \
}

JSONParser MondisServer::parser;

ExecRes MondisServer::execute(Command *command, MondisClient *client) {
    return (this->*commandHandlers[command->type])(command,client);
}

MultiCommand *MondisServer::getUndoCommand(CommandStruct &cstruct, MondisClient *client) {
    MultiCommand *res = new MultiCommand;
    res->locateCommand = cstruct.locate;

    Command *undo = new Command;
    if (cstruct.obj == nullptr) {
        switch (cstruct.operation->type) {
            case BIND: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = dbs[client->dBIndex]->get(key);
                if (original != nullptr) {
                    undo->type = BIND;
                    undo->addParam(RAW_PARAM(cstruct.operation, 0));
                    undo->addParam(original->getJson(), PARAM_TYPE_STRING);
                } else {
                    undo->type = DEL;
                    undo->addParam(RAW_PARAM(cstruct.operation, 0));
                }
                ADD_AND_RETURN(res, undo)
            }
            case DEL: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = dbs[client->dBIndex]->get(key);
                if (original == nullptr) {
                    return res;
                }
                undo->type = BIND;
                undo->addParam(RAW_PARAM(cstruct.operation, 0));
                undo->addParam(original->getJson(), PARAM_TYPE_STRING);
                ADD_AND_RETURN(res, undo)
            }
            case RENAME: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = dbs[client->dBIndex]->get(key);
                if (original == nullptr) {
                    return res;
                }
                undo->type = RENAME;
                undo->addParam(RAW_PARAM(cstruct.operation, 1));
                undo->addParam(RAW_PARAM(cstruct.operation, 0));
                ADD_AND_RETURN(res, undo)
            }
            case SELECT: {
                undo->type = SELECT;
                undo->addParam(to_string(client->dBIndex), PARAM_TYPE_PLAIN);
                ADD_AND_RETURN(res, undo);
            }
        }
    } else {
        unique_ptr<Command> assist(new Command);
        switch (cstruct.obj->type) {
            case RAW_STRING: {
                switch (cstruct.operation->type) {
                    case BIND: {
                        undo->type = BIND;
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo);
                    }
                    case SET_RANGE: {
                        undo->type = SET_RANGE;
                        assist->type = GET_RANGE;
                        if (cstruct.operation->params.size() == 1) {
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            ExecRes temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo)
                        } else if (cstruct.operation->params.size() == 2) {
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(RAW_PARAM(cstruct.operation, 1));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 1));
                            ExecRes temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo)
                        }
                        return res;
                    }
                    case REMOVE_RANGE: {
                        if (cstruct.operation->params.size() == 1) {
                            undo->type = INSERT;
                            assist->type = GET_RANGE;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            ExecRes temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo);
                        } else if (cstruct.operation->params.size() == 2) {
                            undo->type = INSERT;
                            assist->type = GET_RANGE;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 1));
                            ExecRes temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo);
                        }
                        return res;
                    }
                    case TO_INTEGER: {
                        undo->type = TO_STRING;
                        ADD_AND_RETURN(res, undo);
                    }
                    case INSERT: {
                        undo->type = REMOVE_RANGE;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        undo->addParam(to_string(atoi(RAW_PARAM(cstruct.operation, 0).content.c_str())
                                                 + RAW_PARAM(cstruct.operation, 1).content.size()), PARAM_TYPE_PLAIN);
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case RAW_INT: {
                switch (cstruct.operation->type) {
                    case INCR: {
                        undo->type = DECR;
                        ADD_AND_RETURN(res, undo);
                    }
                    case INCR_BY: {
                        undo->type = DECR_BY;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ADD_AND_RETURN(res, undo);
                    }
                    case DECR: {
                        undo->type = INCR;
                        ADD_AND_RETURN(res, undo);
                    }
                    case DECR_BY: {
                        undo->type = INCR_BY;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ADD_AND_RETURN(res, undo);
                    }
                    case TO_STRING: {
                        undo->type = TO_INTEGER;
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case LIST: {
                switch (cstruct.operation->type) {
                    case POP_FRONT: {
                        assist->type = FRONT;
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        undo->type = PUSH_FRONT;
                        undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case POP_BACK: {
                        assist->type = BACK;
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        undo->type = PUSH_BACK;
                        undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case PUSH_FRONT: {
                        undo->type = POP_FRONT;
                        ADD_AND_RETURN(res, undo)
                    }
                    case PUSH_BACK: {
                        undo->type = POP_BACK;
                        ADD_AND_RETURN(res, undo)
                    }
                    case BIND: {
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        undo->type = BIND;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        if (temp.type = OK) {
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case SET: {
                switch (cstruct.operation->type) {
                    case ADD: {
                        undo->type = REMOVE;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ADD_AND_RETURN(res, undo);
                    }
                    case REMOVE: {
                        assist->type = EXISTS;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        if (temp.desc == "true") {
                            undo->type = ADD;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case ZSET: {
                switch (cstruct.operation->type) {
                    case ADD: {
                        undo->type = REMOVE_BY_SCORE;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ADD_AND_RETURN(res, undo);
                    }
                    case REMOVE_BY_RANK: {
                        assist->type = GET_BY_RANK;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp1 = cstruct.obj->execute(assist.get());
                        if (temp1.type != OK) {
                            return res;
                        }
                        assist->type = RANK_TO_SCORE;
                        ExecRes temp2 = cstruct.obj->execute(assist.get());
                        undo->type = ADD;
                        undo->addParam(temp2.desc, PARAM_TYPE_PLAIN);
                        undo->addParam(temp1.desc, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case REMOVE_BY_SCORE: {
                        assist->type = GET_BY_SCORE;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp1 = cstruct.obj->execute(assist.get());
                        if (temp1.type != OK) {
                            return res;
                        }
                        undo->type = ADD;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        undo->addParam(temp1.desc, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case REMOVE_RANGE_BY_RANK: {
                        SplayTree *tree = (SplayTree *) cstruct.obj->objData;
                        int rankStart = atoi(RAW_PARAM(cstruct.operation, 0).content.c_str());
                        int rankEnd = atoi(RAW_PARAM(cstruct.operation, 1).content.c_str());
                        for (int i = rankStart; i < rankEnd; ++i) {
                            Command *un = new Command;
                            SplayTreeNode *node = tree->getNodeByRank(i);
                            un->type = ADD;
                            un->addParam(to_string(node->score), PARAM_TYPE_PLAIN);
                            un->addParam(node->data->getJson(), PARAM_TYPE_STRING);
                            res->operations.push_back(un);
                        }
                        return res;
                    }
                    case REMOVE_RANGE_BY_SCORE: {
                        SplayTree *tree = (SplayTree *) cstruct.obj->objData;
                        int scoreStart = atoi(RAW_PARAM(cstruct.operation, 0).content.c_str());
                        int scoreEnd = atoi(RAW_PARAM(cstruct.operation, 1).content.c_str());
                        SplayTreeNode *firstNode = tree->getUpperBound(scoreStart, true);
                        assist->type = SCORE_TO_RANK;
                        assist->addParam(to_string(firstNode->score), PARAM_TYPE_PLAIN);
                        ExecRes temp1 = cstruct.obj->execute(assist.get());
                        int rankStart = atoi(temp1.desc.c_str());
                        assist->params.clear();
                        SplayTreeNode *lastNode = tree->getLowerBound(scoreEnd, true);
                        assist->type = SCORE_TO_RANK;
                        assist->addParam(to_string(lastNode->score), PARAM_TYPE_PLAIN);
                        ExecRes temp2 = cstruct.obj->execute(assist.get());
                        int rankEnd = atoi(temp2.desc.c_str());

                        for (int i = rankStart; i < rankEnd; ++i) {
                            Command *un = new Command;
                            SplayTreeNode *node = tree->getNodeByScore(i);
                            un->type = ADD;
                            un->addParam(to_string(node->score), PARAM_TYPE_PLAIN);
                            un->addParam(node->data->getJson(), PARAM_TYPE_STRING);
                            res->operations.push_back(un);
                        }
                        return res;
                    }
                    case CHANGE_SCORE: {
                        undo->type = CHANGE_SCORE;
                        undo->addParam(RAW_PARAM(cstruct.operation, 1));
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case HASH: {
                switch (cstruct.operation->type) {
                    case BIND: {
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        if (temp.type == OK) {
                            undo->type = BIND;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        } else {
                            undo->type = DEL;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                    case DEL: {
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecRes temp = cstruct.obj->execute(assist.get());
                        if (temp.type == OK) {
                            undo->type = BIND;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(temp.desc, PARAM_TYPE_STRING);
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
        }
    }
}

JSONParser *MondisServer::getJSONParser() {
    return &parser;
}

void MondisServer::parseConfFile(string &confFile) {
    ifstream configFile;
    configFile.open(confFile.c_str());
    string strLine;
    string filepath;
    if(configFile.is_open())
    {
        while (!configFile.eof())
        {
            getline(configFile, strLine);
            if (strLine[0] == '#' || strLine == "") {
                continue;
            }
            size_t pos = strLine.find('=');
            string key = strLine.substr(0, pos);
            string value = strLine.substr(pos+1);
            conf[key] = value;
        }
    }
}

void MondisServer::runAsDaemon() {
#ifdef linux
    pid_t pid;
    int fd, i, nfiles;
    pid = fork();
    if(pid < 0) {
        ERROR_EXIT("First fork failed!");
    }

     if(pid > 0) {
         exit(EXIT_SUCCESS);// father mondisExit
     }

     if(setsid() == -1) {
         ERROR_EXIT("setsid failed!");
     }

     pid = fork();
     if(pid < 0) {
         ERROR_EXIT("Second fork failed!");
     }

     if(pid > 0) {
         exit(EXIT_SUCCESS);
     }
#ifdef RLIMIT_NOFILE
     /* 关闭从父进程继承来的文件描述符 */
     if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
         ERROR_EXIT("getrlimit failed!");
     nfiles = rl.rlim_cur = rl.rlim_max;
     setrlimit(RLIMIT_NOFILE, &rl);
     for(i=3; i<nfiles; i++)
         close(i);
#endif
     /* 重定向标准的3个文件描述符 */
     if(fd = open("/dev/null", O_RDWR) < 0) {
         ERROR_EXIT("open /dev/null failed!");
     }
     for(i=0; i<3; i++) {
         dup2(fd, i);
     }
     if(fd > 2) {
         close(fd);
     }
     /* 改变工作目录和文件掩码常量 */
     chdir("/");
     umask(0);
#endif
}

void MondisServer::appendLog(const string &commandStr, ExecRes &res) {
    Log log(const_cast<string&>(commandStr), res);
    logFileOut << log.toString();
}

int MondisServer::start(string &confFile) {
    cout << "Mondis 1.0" << endl;
    configfile = confFile;
    if (configfile != "") {
        cout << "is apply configuration..." << endl;
        parseConfFile(confFile);
        applyConf();
    }
    init();
}

void MondisServer::startEventLoop() {
    cout << "start event loop,now you can input command" << endl;
    while (true) {
        cout << username + "@Mondis>";
        string nextCommand;
        getline(std::cin, nextCommand);
        ExecRes res = execute(nextCommand, self);
        cout << res.toString();
        cout << endl;
    }
}

//client表示执行命令的客户端，如果为nullptr则为Mondisserver自身
ExecRes MondisServer::execute(const string &commandStr, MondisClient *client) {
    Command *command = interpreter->getCommand(const_cast<string&>(commandStr));
    CommandStruct cstruct = getCommandStruct(command, client);
    ExecRes res;
    if (isPropagating) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.desc = "is propagating command to a slave,please try later on";
        res.type = INTERNAL_ERROR;
        return res;
    }
    if (isVoting && client->type == CLIENT) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.desc = "master is dead,the cluster is voting for new master";
        res.type = INTERNAL_ERROR;
        return res;
    }
    if ((!client->hasAuthenticate) && (cstruct.operation->type != LOGIN)) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.desc = "you haven't login,please login";
        LOGIC_ERROR_AND_RETURN
    }
    if (serverStatus == SLAVE && cstruct.isModify) {
        if (autoMoveCommandToMaster) {
            Command::destroyCommand(command);
            mondis::Message * msg = new mondis::Message;
            msg->set_msg_type(mondis::MsgType::COMMAND);
            msg->set_command_type(mondis::CommandType::SLAVE_FORWARD);
            msg->set_content(commandStr);
            putToWriteQueue(msg);
            return res;
        } else {
            res.desc = "the current server is a slave,can not undoExecute command which will modify database state";
            LOGIC_ERROR_AND_RETURN
        }
    }
    if (client->type == CLIENT && clientControlCommands.find(cstruct.operation->type) == clientControlCommands.end()) {
        res.desc = "this command can not execute from a client";
        LOGIC_ERROR_AND_RETURN
    }
    if (client->isInTransaction && transactionAboutCommands.find(command->type) == transactionAboutCommands.end()) {
        Command::destroyCommand(cstruct.operation);
        Command::destroyCommand(cstruct.locate);
        client->transactionCommands->push(commandStr);
        OK_AND_RETURN
    }
    if (controlCommands.find(command->type) != controlCommands.end() || cstruct.isLocate) {
        res = transactionExecute(cstruct, client);
    } else if (command->type != LOCATE) {
        res.desc = "Invalid command";
        Command::destroyCommand(command);
        LOGIC_ERROR_AND_RETURN
    }
    if (res.type != OK) {
        return res;
    }
    appendLog(commandStr, res);
    if (isModifyCommand && res.type == OK) {
        appendAof(commandStr);
        if (replicaCommandBuffer->size() == maxCommandReplicaBufferSize) {
            replicaCommandBuffer->pop_front();
            replicaCommandBuffer->push_back(commandStr);
        }
        replicaOffset++;
        while (!putToPropagateBuffer(commandStr));
    }

    return res;
}

void MondisServer::applyConf() {
    for (auto &kv:conf) {
        if (kv.first == "deamonize") {
            if (kv.second == "true") {
                daemonize = true;
            } else if (kv.second == "false") {
                daemonize = false;
            }
        } else if (kv.first == "port") {
            util::toInteger(kv.second, port);
        } else if (kv.first == "username") {
            username = kv.second;
        } else if (kv.first == "password") {
            password = kv.second;
        } else if (kv.first == "databaseNum") {
            util::toInteger(kv.second, databaseNum);
        } else if (kv.first == "aof") {
            if (kv.second == "true") {
                aof = true;
            } else if (kv.second == "false") {
                aof = false;
            }
        } else if (kv.first == "databaseID") {
            util::toInteger(kv.second, self->dBIndex);
        } else if (kv.first == "aofSyncStrategy") {
            aofSyncStrategy = atoi(kv.second.c_str());
        } else if (kv.first == "json") {
            if (kv.second == "true") {
                json = true;
            } else if (kv.second == "false") {
                json = false;
            }
        } else if (kv.first == "jsonDuration") {
            util::toInteger(kv.second, jsonDuration);
        } else if (kv.first == "workDir") {
            workDir = kv.second;
        } else if (kv.first == "logFile") {
            logFile = kv.second;
        } else if (kv.first == "aofFile") {
            aofFile = kv.second;
        } else if (kv.first == "jsonFile") {
            jsonFile = kv.second;
        } else if (kv.first == "recovery") {
            recoveryStrategy = kv.second;
        } else if (kv.first == "recoveryFile") {
            recoveryFile = kv.second;
        } else if(kv.first == "maxClientNum") {
            maxClientNum = atoi(kv.second.c_str());
        } else if(kv.first == "maxCommandReplicaBufferSize") {
            maxCommandPropagateBufferSize = atoi(kv.second.c_str());
        } else if(kv.first == "maxCommandPropagateBufferSize" ) {
            maxCommandPropagateBufferSize=atoi(kv.second.c_str());
        } else if(kv.first == "masterUsername") {
            masterUsername = kv.second;
        } else if(kv.first == "masterPassword") {
            masterPassword = kv.second;
        } else if(kv.first == "masterIP" ) {
            masterIP = kv.second;
        } else if(kv.first == "masterPort") {
            masterPort = atoi(kv.second.c_str());
        } else if(kv.first == "beSlaveOf") {
            if(kv.second == "true") {
                isSlaveOf = true;
            } else if(kv.second == "false"){
                isSlaveOf = false;
            }
        } else if (kv.first == "maxSlaveNum") {
            maxSlaveNum = atoi(kv.second.c_str());
        } else if (kv.first == "autoMoveCommandToMaster") {
            if (kv.second == "true") {
                autoMoveCommandToMaster = true;
            } else if (kv.second == "false") {
                autoMoveCommandToMaster = false;
            }
        } else if (kv.first == "forbidOtherModifyInTransaction") {
            if (kv.second == "true") {
                forbidOtherModifyInTransaction = true;
            } else if (kv.second == "false") {
                forbidOtherModifyInTransaction = false;
            }
        }
    }
}

void MondisServer::init() {
    cout << "is Initializing..." << endl;
    runStatus = LOADING;
    interpreter = new CommandInterpreter;
    for (int i = 0; i < databaseNum; ++i) {
        dbs.push_back(new HashMap(16, 0.75f));
    }
#ifdef WIN32
    self = new MondisClient(this, (SOCKET) 0);
#elif defined(linux)
    self = new MondisClient(this,0);
    epollFd = epoll_create(maxClientNum+maxSlaveNum);
    epollEvents = new epoll_event[maxClientNum+maxSlaveNum];
#endif
    self->type = SERVER_SELF;
    self->hasAuthenticate = true;
    if (aof) {
        aofFileOut.open(workDir + "/" + aofFile, ios::app);
    }
    if (json) {
        jsonFileOut.open(workDir + "/" + jsonFile, ios::app);
    }
    runStatus = RECOVERING;
    cout << "is Recovering..." << endl;
    if (recoveryStrategy == "json") {
        JSONParser temp((workDir + "/" + recoveryFile).c_str());
        temp.parseAll(dbs);
    } else if (recoveryStrategy == "aof") {
        recoveryFileIn.open(workDir + "/" + recoveryFile);
        string command;
        while (getline(recoveryFileIn, command)) {
            execute(interpreter->getCommand(command), self);
        }
    }
    if (daemonize) {
        runAsDaemon();
    }
    logFileOut.open(workDir + "/" + logFile, ios::app);
    if (isSlaveOf) {
        runStatus = REPLACTING;
        cout << "is Replicating from master..." << endl;
        string sync = "SLAVE_OF ";
        sync += masterIP;
        sync += " ";
        sync += masterPort;
        sync +=" ";
        sync+= masterUsername;
        sync+=" ";
        sync+=masterPassword;
        execute(interpreter->getCommand(sync), nullptr);
    }
    //接收连接
    std::thread accept(&MondisServer::acceptSocket, this);
    //控制台事件循环
    std::thread eventLoop(&MondisServer::startEventLoop, this);

    selectAndHandle();
    //读线程
    msgHandler = new std::thread(&MondisServer::msgHandle, this);
    //写线程
    msgWriter = new std::thread(&MondisServer::writeToClient,this);
    runStatus = RunStatus ::RUNNING;
}

void MondisServer::acceptSocket() {
#ifdef WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    SOCKET servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    sockAddr.sin_port = htons(port);
    bind(servSock, (SOCKADDR *) &sockAddr, sizeof(SOCKADDR));
    listen(servSock, 10);
    while (true) {
        sockaddr_in remoteAddr;
        int len = sizeof(remoteAddr);
        SOCKET clientSock = accept(servSock, (SOCKADDR *)nullptr, nullptr);
        MondisClient *client = new MondisClient(this, clientSock);
        unsigned long iMode = 1;
        ioctlsocket(clientSock, FIONBIO, &iMode);
        BOOL bNoDelay = TRUE;
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char FAR *) &bNoDelay, sizeof(BOOL));
        getpeername(clientSock, (sockaddr *) &remoteAddr, &len);
        client->ip = inet_ntoa(remoteAddr.sin_addr);
        client->port = ntohs(remoteAddr.sin_port);
        client->id = nextClientId();
        FD_SET(clientSock, &fds);
        allModifyMtx.lock();
        socketToClient[client->sock] = client;
        allModifyMtx.unlock();
        clientModifyMtx.lock();
        idToClients[client->id] = client;
        clientModifyMtx.unlock();
    }
#elif defined(linux)
    int socket_fd;
    int clientFd;
    sockaddr_in servaddr;
    servaddr.sin_family = PF_INET;
    servaddr.sin_addr.s_addr = htonl(atoi("127.0.0.1"));
    servaddr.sin_port = htons(port);
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    listen(socket_fd,10);
    while (true) {
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        clientFd = accept(socket_fd, (sockaddr*)&clientAddr,&len);
        MondisClient *client = new MondisClient(this,clientFd);
        ::getsockname(clientFd,(sockaddr*)&clientAddr,&len);
        client->ip = inet_ntoa(clientAddr.sin_addr);
        client->port=ntohs(clientAddr.sin_port);
        client->id = nextClientId();
        int flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd,F_SETFL,flags|O_NONBLOCK);
        int noDelay = 1;
        setsockopt(clientFd, IPPROTO_TCP, FNDELAY, &noDelay, sizeof(noDelay));
        allModifyMtx.lock();
        fdToClients[clientFd] = client;
        allModifyMtx.unlock();
        clientModifyMtx.lock();
        idToClients[client->id]=client;
        clientModifyMtx.unlock();
        epoll_ctl(clientFd,EPOLL_CTL_ADD,clientFd,&listenEvent);
    }
#endif
}

MondisServer::MondisServer() {
#ifdef WIN32
    FD_ZERO(&fds);
#elif defined(linux)
    listenEvent.events = EPOLLET|EPOLLIN;
#endif
    replicaCommandBuffer = new deque<string>;
    commandPropagateBuffer = new queue<string>;
}

MondisServer::~MondisServer() {
#ifdef linux
    delete[] epollEvents;
#endif
    delete replicaCommandBuffer;
    delete commandPropagateBuffer;
    delete msgHandler;
    delete msgWriter;
}

void MondisServer::replicaToSlave(MondisClient *client, long long slaveReplicaOffset) {
    if (replicaOffset - slaveReplicaOffset > maxCommandReplicaBufferSize) {
        const long long start = replicaOffset;
        string *temp = new string;
        getJson(temp);
        mondis::Message* msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::DATA);
        msg->set_data_type(mondis::DataType::SYNC_DATA);
        msg->set_content(*temp);
        msg->set_client_id(client->id);
        putToWriteQueue(msg);
        delete temp;
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - start), replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
    } else {
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - slaveReplicaOffset),
                                replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
    }
    string newPeer = "NEW_PEER ";
    newPeer += client->ip;
    newPeer += " ";
    newPeer += to_string(client->port);

    putCommandMsgToWriteQueue(newPeer,0,mondis::CommandType::MASTER_COMMAND,mondis::SendToType::ALL_PEERS);
    isPropagating = false;
}

bool MondisServer::putToPropagateBuffer(const string &curCommand) {
    if(commandPropagateBuffer->size() == maxCommandPropagateBufferSize) {
        return false;
    }
    commandPropagateBuffer->push(curCommand);
    notEmpty.notify_all();
    return true;
}

void MondisServer::closeClient(MondisClient *client) {
#ifdef WIN32
    if (client->type == CLIENT) {
        watchedKeyMtx.lock();
        for (auto &key:client->watchedKeys) {
            keyToWatchedClients[key].erase(keyToWatchedClients[key].find(client));
        }
        watchedKeyMtx.unlock();
        clientModifyMtx.lock();
        idToClients.erase(idToClients.find(client->id));
        clientModifyMtx.unlock();
    } else if (client->type == PEER) {
        peersModifyMtx.lock();
        idToPeers.erase(idToPeers.find(client->id));
        peersModifyMtx.unlock();
    }
    FD_CLR(client->sock, &fds);
    allModifyMtx.lock();
    socketToClient.erase(socketToClient.find(client->sock));
    allModifyMtx.unlock();
    idToPeersAndClients.erase(client->id);
#elif defined(linux)
    watchedKeyMtx.lock();
    for (auto &key:client->watchedKeys) {
        keyToWatchedClients[key].erase(keyToWatchedClients[key].find(client));
    }
    watchedKeyMtx.unlock();
    epoll_ctl(epollFd, EPOLL_CTL_DEL, client->fd, nullptr);
    if(client->type == CLIENT) {
        clientModifyMtx.lock();
        idToClients.erase(idToClients.find(client->id));
        clientModifyMtx.unlock();
    } else if(client->type == PEER) {
        peersModifyMtx.lock();
        idToPeers.erase(idToPeers.find(client->id));
        peersModifyMtx.unlock();
    }
    idToPeersAndClients.erase(client->id);
    allModifyMtx.lock();
    fdToClients.erase(fdToClients.find(client->fd));
    allModifyMtx.unlock();
#endif
    delete client;
}

void MondisServer::undoExecute(MultiCommand *command, MondisClient *client) {
    if (command == nullptr) {
        return;
    }
    if (command->locateCommand == nullptr) {
        for (auto m:command->operations) {
            execute(m, client);
            Command::destroyCommand(m);
        }
    } else {
        MondisObject *obj = chainLocate(command->locateCommand, client);
        for (auto m:command->operations) {
            obj->execute(m);
            Command::destroyCommand(m);
        }
    }
}

MondisObject *MondisServer::chainLocate(Command *command, MondisClient *client) {
    MondisObject *curObj = dbs[client->dBIndex]->locate(command);
    Command *curCommand = command->next;
    while (true) {
        if (curCommand == nullptr || curCommand->type != LOCATE) {
            return curObj;
        }
        if (curObj == nullptr) {
            return nullptr;
        }
        curObj = curObj->locate(curCommand);
        curCommand = curCommand->next;
    }
}

bool MondisServer::handleWatchedKey(const string &key) {
    watchedKeyMtx.lock_shared();
    if (keyToWatchedClients.find(key) == keyToWatchedClients.end()) {
        watchedKeyMtx.unlock_shared();
        return true;
    }
    watchedKeyMtx.unlock_shared();
    if (forbidOtherModifyInTransaction) {
        return false;
    }
    watchedKeyMtx.lock_shared();
    unordered_set<MondisClient *> &vc = keyToWatchedClients[key];
    watchedKeyMtx.unlock_shared();
    for (MondisClient *client:vc) {
        client->watchedKeysHasModified = true;
        client->modifiedKeys.insert(key);
    }
    return true;
}

void MondisServer::appendAof(const string &command) {
    if (aof) {
        aofFileOut << command + "\n";
        if (aofSyncStrategy == 1) {
            auto cur = chrono::duration_cast<chrono::milliseconds>(
                    chrono::system_clock::now().time_since_epoch()).count();
            if (cur - preSync >= 1000) {
                aofFileOut.flush();
                preSync = cur;
            }
        } else if (aofSyncStrategy == 2) {
            aofFileOut.flush();
        }
    }
}

ExecRes MondisServer::transactionExecute(CommandStruct &cstruct, MondisClient *client) {
    ExecRes res;
    bool canContinue = true;
    if (cstruct.isModify) {
        canContinue = handleWatchedKey(TO_FULL_KEY_NAME(client->dBIndex, (*cstruct.operation)[0].content));
    }
    if (canContinue) {
        if (cstruct.isLocate) {
            cstruct.obj->execute(cstruct.operation);
        } else {
            res = execute(cstruct.operation, client);
        }
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        return res;
    } else {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.desc = "other transaction is watching the key ";
        res.desc += TO_FULL_KEY_NAME(client->dBIndex, (*cstruct.operation)[0].content);
        res.desc += ",so can undoExecute the command";
        LOGIC_ERROR_AND_RETURN;
    }
}

CommandStruct MondisServer::getCommandStruct(Command *command, MondisClient *client) {
    CommandStruct res;
    if (command->type == LOCATE) {
        Command *last = command;
        while (last->next->type == LOCATE) {
            last = last->next;
        }
        res.operation = last->next;
        last->next = nullptr;
        res.locate = command;
        res.isLocate = true;
        res.obj = chainLocate(res.locate, client);
    } else {
        res.operation = command;
        res.isLocate = false;
    }
    res.isModify = isModifyCommand(res.operation);

    return res;
}

MondisClient *MondisServer::buildConnection(const string &ip, int port) {
    MondisClient *res = nullptr;
#ifdef WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(sockVersion, &data);
    SOCKET masterSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(port);
    serAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    if (connect(masterSock, (sockaddr *) &serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
        return res;
    }
    unsigned long iMode = 0;
    ioctlsocket(masterSock, FIONBIO, &iMode);
    res = new MondisClient(this, masterSock);
#elif defined(linux)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons((u_int16_t)port);
    inet_pton(AF_INET,ip.c_str(),&serAddr.sin_addr);
    int masterFd;
    if((masterFd = connect(sockfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))<0) {
        return res;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    master = new MondisClient(this,masterFd);
#endif
    res->hasAuthenticate = true;
    return res;
}

void MondisServer::getJson(string *res) {
    (*res) += "{\n";
    for (int i = 0; i < databaseNum; ++i) {
        (*res) += "\"";
        (*res) += i;
        (*res) += "\"";
        (*res) += " : ";
        (*res) += dbs[i]->getJson();
        (*res) += "\n";
    }
    (*res) += "}";
}


unordered_set<CommandType> MondisServer::modifyCommands = {
        CommandType:: BIND,
        CommandType:: DEL,
        CommandType:: RENAME,
        CommandType:: SET_RANGE,
        CommandType:: REMOVE_RANGE,
        CommandType:: INCR,
        CommandType:: DECR,
        CommandType:: INCR_BY,
        CommandType:: DECR_BY,
        CommandType:: INSERT,
        CommandType:: PUSH_FRONT,
        CommandType:: PUSH_BACK,
        CommandType:: POP_FRONT,
        CommandType:: POP_BACK,
        CommandType:: ADD,
        CommandType:: REMOVE,
        CommandType:: REMOVE_BY_RANK,
        CommandType:: REMOVE_BY_SCORE,
        CommandType:: REMOVE_RANGE_BY_RANK,
        CommandType:: REMOVE_RANGE_BY_SCORE,
        CommandType:: TO_STRING,
        CommandType:: TO_INTEGER,
        CommandType:: CHANGE_SCORE,
        CommandType:: SELECT
};
unordered_set<CommandType> MondisServer::transactionAboutCommands = {
        CommandType ::MULTI,
        CommandType ::EXEC,
        CommandType ::DISCARD,
        CommandType ::WATCH,
        CommandType ::UNWATCH
};
unordered_set<CommandType> MondisServer::clientControlCommands = {
        CommandType:: BIND,
        CommandType:: GET,
        CommandType:: LOGIN,
        CommandType:: EXISTS,
        CommandType:: RENAME,
        CommandType:: TYPE,
        CommandType:: SAVE,
        CommandType:: SAVE_ALL,
        CommandType:: EXIT,
        CommandType:: SELECT,
        CommandType:: DEL,
        CommandType:: SLAVE_OF,
        CommandType:: MULTI,
        CommandType:: EXEC,
        CommandType:: DISCARD,
        CommandType:: WATCH,
        CommandType:: UNWATCH,
        CommandType:: MASTER_INFO
};

unordered_set<CommandType> MondisServer::controlCommands = {
        CommandType:: BIND,
        CommandType:: GET,
        CommandType:: LOGIN,
        CommandType:: EXISTS,
        CommandType:: RENAME,
        CommandType:: TYPE,
        CommandType:: SAVE,
        CommandType:: SAVE_ALL,
        CommandType:: EXIT,
        CommandType:: SELECT,
        CommandType:: DEL,
        CommandType:: SLAVE_OF,
        CommandType:: SYNC,
        CommandType:: CLIENT_KILL,
        CommandType:: MULTI,
        CommandType:: EXEC,
        CommandType:: DISCARD,
        CommandType:: WATCH,
        CommandType:: UNWATCH,
        CommandType:: MASTER_INFO,
        CommandType:: NEW_PEER,
        CommandType:: VOTE,
        CommandType:: UNVOTE,
        CommandType:: ASK_FOR_VOTE,
        CommandType:: MASTER_DEAD,
        CommandType:: I_AM_NEW_MASTER,
        CommandType:: CLIENT_LIST,
        CommandType:: NEW_CLIENT,
};
MondisServer* MondisServer::server = nullptr;

unsigned MondisServer::nextClientId() {
    static std::uniform_int_distribution<int> dis(1, numeric_limits<int>::max());
    static default_random_engine engine;
    int id = 1;
    peersModifyMtx.lock_shared();
    do {
        id = dis(engine);
    } while (idToPeers.find(id) != idToPeers.end());
    peersModifyMtx.unlock_shared();
    return id;
}

void MondisServer::incrReplicaOffset() {
    replicaOffset++;
}

bool MondisServer::isModifyCommand(Command *command) {
    if (command->type == LOCATE) {
        while (command->next->type == LOCATE) {
            command = command->next;
        }
    }
    return modifyCommands.find(command->type) != modifyCommands.end();
}

void MondisServer::selectAndHandle() {
#ifdef WIN32
    timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        fd_set *fds = nullptr;
#elif defined(linux)
    int epollFd = 0;
    epoll_event* events = nullptr;
#endif
    while (true) {
#ifdef WIN32
        FD_ZERO(&fds);
        clientModifyMtx.lock_shared();
        for (auto &kv:idToClients) {
            FD_SET(kv.second->sock, &fds);
        }
        clientModifyMtx.unlock_shared();
        int ret = select(0, fds, nullptr, nullptr, &timeout);
        if (ret <= 0) {
            continue;
        }
        allModifyMtx.lock_shared();
        for (auto pair:socketToClient) {
            if (FD_ISSET(pair.first, fds)) {
                MondisClient *client = pair.second;
                putToReadQueue(client->nextMessage());
            }
        }
        allModifyMtx.unlock_shared();
#elif defined(linux)
        int nfds = epoll_wait(epollFd, events, maxClientNum, 500);
        if(nfds == 0) {
            continue;
        }
        for(int i=0;i<nfds;i++) {
            MondisClient* client = fdToClients[events[i].data.fd];
            if(events[i].events&EPOLLRDHUP){
                closeClient(fdToClients[client->fd]);
            } else {
                putToReadQueue(client->nextMessage());
            }
        }
#endif
    }
}

MondisServer *MondisServer::getInstance() {
    if (server == nullptr){
        server = new MondisServer;
    }
    return server;
}

void MondisServer::msgHandle() {
    while (true) {
        mondis::Message *msg = readQueue.take();
        switch (msg->msg_type()) {
            case mondis::MsgType::COMMAND:
                putExecResMsgToWriteQueue(execute(msg->content(), idToPeersAndClients[msg->client_id()]),
                                          msg->client_id(), mondis::SendToType::SPECIFY_CLIENT);
            break;
            case mondis::MsgType::DATA:
                switch (msg->data_type()) {
                    case mondis::DataType::SYNC_DATA: {
                        for (auto db:dbs) {
                            db->clear();
                        }
                        JSONParser temp(msg->content());
                        temp.parseAll(dbs);
                    }
                    case mondis::DataType::CONTROL_MSG:{
                        cout<<msg->content();
                    }
                    case mondis::DataType ::HEART_BEAT: {
                        if (msg->content() == "PING" && serverStatus == ServerStatus ::SV_STAT_MASTER) {
                            mondis::Message * reply = new mondis::Message;
                            reply->set_msg_type(mondis::MsgType::DATA);
                            reply->set_data_type(mondis::DataType::HEART_BEAT);
                            reply->set_content("PONG");
                            reply->set_client_id(msg->client_id());
                            putToWriteQueue(msg);
                        }
                    }
                }
            break;
            case mondis::MsgType ::EXEC_RES:
                ExecRes res;
                res.type = (ExecResType)msg->res_type();
                res.desc = msg->content();
                resQueue.push_front(res);
            break;
        }
        delete msg;
    }
}

void MondisServer::putToReadQueue(mondis::Message *msg) {
    readQueue.put(msg);
}
unordered_map<CommandType,CommandHandler> MondisServer::commandHandlers = {
        {CommandType::BIND, &MondisServer::bindKey},
        {CommandType::DEL,&MondisServer::del},
        {CommandType::TYPE,&MondisServer::type},
        {CommandType::SELECT, &MondisServer::selectDb},
        {CommandType::RENAME, &MondisServer::renameKey},
        {CommandType::GET,&MondisServer::get},
        {CommandType::SET_TTL,&MondisServer::setTTl},
        {CommandType::EXISTS,&MondisServer::exsits},
        {CommandType::EXIT, &MondisServer::mondisExit},
        {CommandType::SAVE,&MondisServer::save},
        {CommandType::SAVE_ALL,&MondisServer::saveAll},
        {CommandType::LOGIN,&MondisServer::login},
        {CommandType::M_SIZE,&MondisServer::size},
        {CommandType::SLAVE_OF, &MondisServer::beSlaveOf},
        {CommandType::SYNC,&MondisServer::sync},
        {CommandType::CLIENT_KILL,&MondisServer::disconnectClient},
        {CommandType::MULTI,&MondisServer::multi},
        {CommandType::EXEC,&MondisServer::exec},
        {CommandType::DISCARD,&MondisServer::discard},
        {CommandType::WATCH,&MondisServer::watch},
        {CommandType::UNWATCH,&MondisServer::unwatch},
        {CommandType::MASTER_INFO, &MondisServer::getMasterInfo},
        {CommandType::NEW_PEER,&MondisServer::newPeer},
        {CommandType::NEW_CLIENT, &MondisServer::newClient},
        {CommandType::ASK_FOR_VOTE,&MondisServer::askForVote},
        {CommandType::VOTE,&MondisServer::vote},
        {CommandType::UNVOTE,&MondisServer::unvote},
        {CommandType::MASTER_DEAD,&MondisServer::masterDead},
        {CommandType::I_AM_NEW_MASTER,&MondisServer::iAmNewMaster},
        {CommandType::CLIENT_LIST,&MondisServer::clientList},
        {CommandType::MASTER_INVITE,&MondisServer::masterInvite},
        {CommandType::UPDATE_OFFSET,&MondisServer::updateOffset}
};

ExecRes MondisServer::bindKey(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(2);
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, STRING)
    KEY(0)
    MondisObject * obj = parser.parseObject((*command)[1].content);
    dbs[client->dBIndex]->put(key, obj);
    OK_AND_RETURN
};

ExecRes MondisServer::get(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    KEY(0)
    MondisObject *data;
    data = dbs[client->dBIndex]->get(key);
    if (data == nullptr) {
        res.desc = "the key does not exists";
        LOGIC_ERROR_AND_RETURN
    }
    res.desc = data->getJson();
    OK_AND_RETURN
}

ExecRes MondisServer::del(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    KEY(0)
    dbs[client->dBIndex]->remove(key);
    OK_AND_RETURN
}

ExecRes MondisServer::exsits(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    KEY(0)
    bool r = dbs[client->dBIndex]->containsKey(key);
    res.desc = util::to_string(r);
    OK_AND_RETURN
}

ExecRes MondisServer::login(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(2)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, PLAIN)
    string userName = (*command)[0].content;
    string pwd = (*command)[1].content;
    if (userName == username && pwd == password) {
        client->hasAuthenticate = true;
        OK_AND_RETURN
    }
    res.desc = "username or password error";
    LOGIC_ERROR_AND_RETURN
}

ExecRes MondisServer::type(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    KEY(0)
    MondisObject *data;
    data = dbs[client->dBIndex]->get(key);
    if (data == nullptr) {
        res.desc = "the key does not exists";
        LOGIC_ERROR_AND_RETURN
    }
    res.desc = data->getTypeStr();
    OK_AND_RETURN;
}

ExecRes MondisServer::selectDb(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_AND_DEFINE_INT_LEGAL(0, index);
    if (index < 0 || index >= dbs.size()) {
        res.desc = "Invalid database id";
        LOGIC_ERROR_AND_RETURN
    }
    client->dBIndex = index;
    OK_AND_RETURN
}

ExecRes MondisServer::save(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(2)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, PLAIN)
    CHECK_AND_DEFINE_INT_LEGAL(1, index)
    if (index > databaseNum - 1) {
        res.desc = "database index out of range!";
        LOGIC_ERROR_AND_RETURN
    }
    string jsonFile = (*command)[0].content;
#ifdef WIN32
    ofstream out(jsonFile + "2");
    out << dbs[client->dBIndex]->getJson();
    out.flush();
    remove(jsonFile.c_str());
    out.close();
    rename((jsonFile + "2").c_str(), jsonFile.c_str());
#elif defined(linux)
    int pid = fork();
    if(pid == 0) {
        ofstream out(jsonFile + "2");
        out << dbs[client->dBIndex]->getJson();
        out.flush();
        remove(jsonFile.c_str());
        out.close();
        rename((jsonFile + "2").c_str(), jsonFile.c_str());
        exit(0);
    }
#endif
    OK_AND_RETURN
}

ExecRes MondisServer::saveAll(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    string jsonFile = (*command)[0].content;
#ifdef WIN32
    string *temp = new string;
    getJson(temp);
    ofstream out(jsonFile + "2");
    out << temp;
    out.flush();
    remove(jsonFile.c_str());
    out.close();
    delete temp;
    rename((jsonFile + "2").c_str(), jsonFile.c_str());
#elif defined(linux)
    int pid = fork();
        if(pid == 0) {
            string* temp = new string;
            getJson(temp);
            ofstream out(jsonFile + "2");
            out << temp;
            out.flush();
            remove(jsonFile.c_str());
            out.close();
            delete temp;
            rename((jsonFile + "2").c_str(), jsonFile.c_str());
            exit(0);
        }
#endif
     OK_AND_RETURN
}

ExecRes MondisServer::renameKey(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(2)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, PLAIN)
    MondisObject *data = dbs[client->dBIndex]->get(PARAM(0));
    if (data == nullptr) {
        res.desc = "the object whose key is " + PARAM(0) + " does not exists";
        LOGIC_ERROR_AND_RETURN
    }
    bool containsNew = dbs[client->dBIndex]->containsKey(PARAM(1));
    if (containsNew) {
        res.desc = "the object whose key is " + PARAM(0) + " already exists";
        LOGIC_ERROR_AND_RETURN
    }
    dbs[client->dBIndex]->put(PARAM(0), nullptr);
    dbs[client->dBIndex]->remove(PARAM(0));
    dbs[client->dBIndex]->put(PARAM(1), data);
    OK_AND_RETURN
}

ExecRes MondisServer::size(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(0);
    res.desc = to_string(dbs[client->dBIndex]->size());
    OK_AND_RETURN
}

ExecRes MondisServer::beSlaveOf(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(4)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, PLAIN)
    CHECK_PARAM_TYPE(2, PLAIN)
    CHECK_PARAM_TYPE(3, PLAIN)
    hasVoteFor = true;
    if (res.type == OK) {
        masterIP = PARAM(0);
        masterPort = atoi(PARAM(1).c_str());
    }
    CHECK_PARAM_NUM(4);
    CHECK_PARAM_TYPE(0, PLAIN)
    string login = "LOGIN ";
    login += masterUsername;
    login += " ";
    login += masterPassword;
    MondisClient *c = buildConnection(PARAM(2), atoi(PARAM(3).c_str()));
    if (c == nullptr) {
        res.desc = "can not connect to the master!";
        res.type = INTERNAL_ERROR;
        return res;
    }
    master = c;
    idToPeers[0] = c;
    idToPeersAndClients[0] = c;
    c->id = 0;
    putCommandMsgToWriteQueue(login, c->id, mondis::CommandType::CLIENT_COMMAND, mondis::SendToType::SPECIFY_PEER);
    ExecRes loginRes = resQueue.back();
    if (loginRes.type != OK) {
        res.desc = "username or password error";
        closeClient(c);
        LOGIC_ERROR_AND_RETURN
    }
    putCommandMsgToWriteQueue(string("SYNC ") + to_string(replicaOffset), 0, mondis::CommandType::PEER_COMMAND,
                              mondis::SendToType::SPECIFY_PEER);
    ExecRes syncRes = resQueue.back();
    if (syncRes.type != OK) {
        res.desc = "sync failed!";
        closeClient(c);
        LOGIC_ERROR_AND_RETURN
    }
    Timer timer(std::bind([=]{
        mondis::Message* msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::DATA);
        msg->set_data_type(mondis::DataType::HEART_BEAT);
        msg->set_content("PING");
        msg->set_client_id(0);
        putToWriteQueue(msg);
    }),chrono::system_clock::now(),true,chrono::duration<int>(5));
    timeHeap.put(timer);
}

ExecRes MondisServer::sync(Command *command, MondisClient *client) {
    ExecRes res;
    peersModifyMtx.lock_shared();
    if (idToPeers.size() > maxSlaveNum) {
        ExecRes res;
        res.type = LOGIC_ERROR;
        res.desc = "can not build connection because has up to max slave number!";
#ifdef WIN32
        FD_CLR(client->sock, &fds);
        allModifyMtx.lock();
        socketToClient.erase(socketToClient.find(client->sock));
        allModifyMtx.unlock();
#elif defined(linux)
        epoll_ctl(epollFd,EPOLL_CTL_DEL,client->fd, nullptr);
        allModifyMtx.lock();
        fdToClients.erase(fdToClients.find(client->fd));
        allModifyMtx.unlock();
#endif
        delete client;
        return res;
    }
    peersModifyMtx.unlock_shared();
    client->type = PEER;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_AND_DEFINE_INT_LEGAL(1, offset);
    if (serverStatus == ServerStatus::SV_STAT_SLAVE) {
        Command *temp = new Command;
        temp->type = MASTER_INFO;
        ExecRes realRes = execute(temp, nullptr);
        Command::destroyCommand(temp);
        res.desc = "the target server is a slave,if you want continue,please input correct master "
                  "ip and port.the target server's master ip and port is ";
        res.desc += realRes.desc;
        delete client;
        LOGIC_ERROR_AND_RETURN
    }
    peersModifyMtx.lock();
    idToPeers[nextClientId()] = client;
    peersModifyMtx.unlock();
    client->type = PEER;
    std::thread t(&MondisServer::replicaToSlave, this, client, offset);
    OK_AND_RETURN
}

ExecRes MondisServer::disconnectClient(Command *command, MondisClient *client) {
    ExecRes res;
    if (client->type != CLIENT) {
        res.desc = "the sender is not a client!";
        LOGIC_ERROR_AND_RETURN
    }
    closeClient(client);
    OK_AND_RETURN
}

ExecRes MondisServer::disconnectSlave(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    closeClient(client);
    OK_AND_RETURN
}

ExecRes MondisServer::multi(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    client->startTransaction();
    OK_AND_RETURN
}

ExecRes MondisServer::exec(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    return client->commitTransaction(this);
}

ExecRes MondisServer::discard(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    client->closeTransaction();
}

ExecRes MondisServer::watch(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    if (!client->isInTransaction) {
        res.desc = "please start a transaction!";
        LOGIC_ERROR_AND_RETURN
    }
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    KEY(0)
    if (dbs[client->dBIndex]->get(key) == nullptr) {
        res.desc = "the key ";
        res.desc += PARAM(0);
        res.desc += " does not exists";
        LOGIC_ERROR_AND_RETURN
    }
    watchedKeyMtx.lock();
    keyToWatchedClients[TO_FULL_KEY_NAME(client->dBIndex, PARAM(0))].insert(client);
    client->watchedKeys.insert(TO_FULL_KEY_NAME(client->dBIndex, PARAM(0)));
    watchedKeyMtx.unlock();
    OK_AND_RETURN
}

ExecRes MondisServer::unwatch(Command *command, MondisClient *client){
    ExecRes res;
    CHECK_COMMAND_FROM_CLIENT
    if (!client->isInTransaction) {
        res.desc = "please start a transaction!";
        LOGIC_ERROR_AND_RETURN
    }
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0, PLAIN)
    watchedKeyMtx.lock();
    keyToWatchedClients[TO_FULL_KEY_NAME(client->dBIndex, PARAM(0))].erase(
            keyToWatchedClients[PARAM(0)].find(client));
    if (keyToWatchedClients[TO_FULL_KEY_NAME(client->dBIndex, PARAM(0))].size() == 0) {
        keyToWatchedClients.erase(keyToWatchedClients.find(TO_FULL_KEY_NAME(client->dBIndex, PARAM(0))));
    }
    client->watchedKeys.erase(client->watchedKeys.find(TO_FULL_KEY_NAME(client->dBIndex, PARAM(0))));
    watchedKeyMtx.unlock();
    OK_AND_RETURN
}

ExecRes MondisServer::getMasterInfo(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(0)
    if (serverStatus == ServerStatus::SV_STAT_MASTER) {
#ifdef WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        HOSTENT *host = gethostbyname(hostname);
        string ip(inet_ntoa(*(in_addr *) *host->h_addr_list));
#elif defined(linux)
        char hostname[128];
        gethostname(hostname, sizeof(hostname));
        struct hostent *hent;
        hent = gethostbyname(hostname);
        unsigned int ipInt = ntohl(((struct in_addr*)hent->h_addr)->s_addr);
        in_addr addr;
        addr.s_addr = htonl(ipInt);
        string ip(inet_ntoa(addr));
#endif
        res.desc += "master ip:";
        res.desc += ip;
        res.desc += " master port:";
        res.desc += to_string(port);
        OK_AND_RETURN
    } else if (serverStatus == ServerStatus::SV_STAT_SLAVE) {
        res.desc += "master ip:";
        res.desc += masterIP;
        res.desc += " master port:";
        res.desc += masterPort;
        OK_AND_RETURN
    } else {
        res.desc = "current server has no master";
        LOGIC_ERROR_AND_RETURN
    }
}

ExecRes MondisServer::newClient(Command *command, MondisClient *client) {
    ExecRes res;
    res.needSend = false;
    clientModifyMtx.lock();
    if (idToClients.size() > maxClientNum) {
        res.type = LOGIC_ERROR;
        res.desc = "can not build connection because has up to max client number!";
        mondis::Message *msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::EXEC_RES);
        msg->set_res_type(mondis::ExecResType::LOGIC_ERROR);
        msg->set_content(res.toString());
        client->writeMessage(msg);
#ifdef WIN32
        FD_CLR(client->sock, &fds);
        allModifyMtx.lock();
        socketToClient.erase(socketToClient.find(client->sock));
        allModifyMtx.unlock();
#elif defined(linux)
        epoll_ctl(epollFd,EPOLL_CTL_DEL,client->fd, nullptr);
                allModifyMtx.lock();
                fdToClients.erase(fdToClients.find(client->fd));
                allModifyMtx.unlock();
#endif
        idToClients.erase(idToClients.find(client->id));
        delete client;
    }
    client->type = ClientType ::CLIENT;
    clientModifyMtx.unlock();
    client->type = CLIENT;
    OK_AND_RETURN
}

ExecRes MondisServer::askForVote(Command *command, MondisClient *client) {
    ExecRes res;
    if (hasVoteFor) {
        putCommandMsgToWriteQueue("UNVOTE", client->id, mondis::CommandType::PEER_COMMAND, mondis::SendToType::SPECIFY_PEER);
    } else if (maxOffsetClients.find(client) != maxOffsetClients.end()) {
        putCommandMsgToWriteQueue("VOTE", client->id, mondis::CommandType::PEER_COMMAND, mondis::SendToType::SPECIFY_PEER);
        hasVoteFor = true;
    }
    res.needSend = false;
    OK_AND_RETURN
}

ExecRes MondisServer::vote(Command *command, MondisClient *client) {
    ExecRes res;
    voteNum++;
    peersModifyMtx.lock_shared();
    if (voteNum > idToPeers.size() / 2) {
        isVoting = false;
        peersModifyMtx.lock_shared();
        mondis::Message * msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::COMMAND);
        msg->set_content("I_AM_A_NEW_MASTER");
        msg->set_send_to_type(mondis::SendToType::ALL_PEERS);
        putToWriteQueue(msg);
        peersModifyMtx.unlock_shared();
    }
    peersModifyMtx.unlock_shared();
    res.needSend = false;
    OK_AND_RETURN
}

ExecRes MondisServer::unvote(Command *command, MondisClient *client) {
    ExecRes res;
    res.needSend = false;
    OK_AND_RETURN
}

ExecRes MondisServer::masterDead(Command *command, MondisClient *client) {
    ExecRes res;
    isVoting = true;
    delete master;
    master = nullptr;
    res.needSend = false;
#ifdef WIN32
    FD_CLR(client->sock, &fds);
#elif defined(linux)
    epoll_ctl(epollFd,EPOLL_CTL_DEL,client->fd, nullptr);
#endif
    string updateOffset = "UPDATE_OFFSET ";
    updateOffset += to_string(replicaOffset);
    putCommandMsgToWriteQueue(updateOffset,0,mondis::CommandType::PEER_COMMAND,mondis::SendToType::ALL_PEERS);
    OK_AND_RETURN;
}

ExecRes MondisServer::iAmNewMaster(Command *command, MondisClient *client) {
    ExecRes res;
#ifdef WIN32
    FD_CLR(client->sock, &fds);
#elif defined(linux)
    epoll_ctl(epollFd,EPOLL_CTL_DEL,client->fd, nullptr);
#endif
    client->type = ClientType::MASTER;
    isVoting = false;
    hasVoteFor = true;
    delete master;
    master = client;
    string sync = "SYNC ";
    sync += to_string(replicaOffset);
    putCommandMsgToWriteQueue(sync, client->id, mondis::CommandType::SLAVE_FORWARD, mondis::SendToType::ALL_PEERS);
    OK_AND_RETURN
}

ExecRes MondisServer::clientList(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(0)
    res.desc = "current server has ";
    clientModifyMtx.lock_shared();
    res.desc += to_string(idToClients.size());
    clientModifyMtx.unlock_shared();
    res.desc += " clients,and the following are the list:\n";
    clientModifyMtx.lock_shared();
    for (auto &kv:idToClients) {
        res.desc += kv.first;
        res.desc += ",\n";
        res.desc += "id:";
        res.desc += PARAM(0);
        res.desc += "\nip:";
        res.desc += kv.second->ip;
        res.desc += "\nport:";
        res.desc += to_string(kv.second->port);
        res.desc += "\nhasAuthenticated:";
        res.desc += util::to_string(kv.second->hasAuthenticate);
        res.desc += "\ndbIndex:";
        res.desc += to_string(kv.second->dBIndex);
        res.desc += "\nisIntransaction:";
        res.desc += util::to_string(kv.second->isInTransaction);
        res.desc += "\n";
    }
    clientModifyMtx.unlock_shared();
    OK_AND_RETURN
}

ExecRes MondisServer::newPeer(Command *command, MondisClient *client) {
    ExecRes res;
    if (client->type != MASTER) {
        res.desc = "the command is not from master!";
        LOGIC_ERROR_AND_RETURN
    }
    CHECK_PARAM_NUM(3)
    CHECK_PARAM_TYPE(0, PLAIN)
    CHECK_PARAM_TYPE(1, PLAIN)
    CHECK_PARAM_TYPE(2, PLAIN)
    MondisClient *peer = buildConnection(PARAM(0), atoi(PARAM(1).c_str()));
    if (peer == nullptr) {
        res.desc = "can not connect to peer which ip is ";
        res.desc += PARAM(0);
        res.desc += " port is ";
        res.desc += PARAM(1);
    }
    peersModifyMtx.lock();
    idToPeers[atoi(PARAM(2).c_str())] = client;
    idToPeersAndClients[atoi(PARAM(2).c_str())] = client;
    peersModifyMtx.unlock();
    putCommandMsgToWriteQueue("MASTER_INVITE",client->id,mondis::CommandType::PEER_COMMAND,mondis::SendToType::SPECIFY_PEER);
    OK_AND_RETURN
}

ExecRes MondisServer::mondisExit(Command *command, MondisClient *client) {
    ExecRes res;
    CHECK_PARAM_NUM(0)
    system("mondisExit");
}

void MondisServer::writeToClient() {
    while (true) {
        mondis::Message * msg = writeQueue.take();
        if (msg->msg_type() == mondis::MsgType::DATA && msg->data_type() == mondis::DataType::HEART_BEAT) {
            if (chrono::system_clock::now() - preHeartBeat > chrono::duration<int>(10)) {
                mondis::Message* masterDead = new mondis::Message;
                masterDead->set_msg_type(mondis::MsgType::COMMAND);
                masterDead->set_command_type(mondis::CommandType::PEER_COMMAND);
                masterDead->set_content("MASTER_DEAD");
                for(auto kv:idToPeers) {
                    kv.second->writeMessage(msg);
                }
                continue;
            }
        }
        switch (msg->send_to_type()) {
            case mondis::SendToType ::ALL_PEERS:
                for(auto kv:idToPeers) {
                    kv.second->writeMessage(msg);
                }
                break;
            case mondis::SendToType ::ALL_CLIENTS:
                for(auto kv:idToClients) {
                    kv.second->writeMessage(msg);
                }
                break;
            case mondis::SendToType::SPECIFY_CLIENT:
                idToClients[msg->client_id()]->writeMessage(msg);
                break;
            case mondis::SendToType ::SPECIFY_PEER:
                idToPeers[msg->client_id()]->writeMessage(msg);
                break;
        }
        delete msg;
    }
}

void MondisServer::putToWriteQueue(mondis::Message *msg) {
    writeQueue.put(msg);
}

void MondisServer::putCommandMsgToWriteQueue(const string &cmdStr, unsigned int clientId, mondis::CommandType commandType,
                                        mondis::SendToType sendToType) {
    mondis::Message* msg = new mondis::Message;
    msg->set_msg_type(mondis::MsgType::COMMAND);
    msg->set_command_type(commandType);
    msg->set_content(cmdStr);
    msg->set_client_id(clientId);
    msg->set_send_to_type(sendToType);
    putToWriteQueue(msg);
}

void MondisServer::putExecResMsgToWriteQueue(const ExecRes &res, unsigned int clientId, mondis::SendToType sendToType) {
    mondis::Message* msg = new mondis::Message;
    msg->set_msg_type(mondis::MsgType::EXEC_RES);
    msg->set_res_type((mondis::ExecResType)(res.type));
    msg->set_content(res.desc);
    msg->set_client_id(clientId);
    msg->set_send_to_type(sendToType);
    putToWriteQueue(msg);
}

void MondisServer::replicaCommandPropagate(vector<string> commands, MondisClient *client) {
    for (auto c:commands){
        putCommandMsgToWriteQueue(c,client->id,mondis::CommandType::MASTER_COMMAND,mondis::SendToType::SPECIFY_CLIENT);
    }
}

ExecRes MondisServer::masterInvite(Command *command, MondisClient *client) {
    ExecRes res;
    client->type = ClientType ::PEER;
    idToPeers[client->id] = client;
    res.needSend = false;
    OK_AND_RETURN
}

ExecRes MondisServer::setTTl(Command* command,MondisClient* client) {
    ExecRes res;
    CHECK_PARAM_NUM(2)
    CHECK_PARAM_TYPE(0,PLAIN)
    CHECK_PARAM_TYPE(1,PLAIN)
    if (!dbs[client->dBIndex]->containsKey(PARAM(0))) {
        res.desc = "target key doesn't exists!";
        INVALID_AND_RETURN
    }
    Timer timer(std::bind([=]() {
        mondis::Message* msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::COMMAND);
        msg->set_command_type(mondis::CommandType::TIMER_COMMAND);
        msg->set_content(string("DEL ")+PARAM(0));
        putToReadQueue(msg);
    }),chrono::system_clock::now()+chrono::duration<int,std::ratio<1,1>>(atoi(PARAM(1).c_str())));
    timeHeap.put(timer);
    OK_AND_RETURN
}

ExecRes MondisServer::updateOffset(Command *command, MondisClient * client) {
    ExecRes res;
    CHECK_PARAM_NUM(1)
    CHECK_PARAM_TYPE(0,PLAIN)
    unsigned long long  otherOffset = std::stoull(PARAM(0),0,10);
    if (otherOffset>=replicaOffset) {
        if (otherOffset>maxOtherReplicaOffset) {
            maxOffsetClients.clear();
            maxOffsetClients.insert(client);
            maxOtherReplicaOffset = otherOffset;
        } else if (otherOffset == maxOtherReplicaOffset) {
            maxOffsetClients.insert(client);
        }
    }
}

