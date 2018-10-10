//
// Created by 11956 on 2018/9/5.
//
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <thread>

#ifdef WIN32

#include <winsock2.h>
#include <inaddr.h>
#include <stdio.h>

#elif defined(linux)
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#endif

#include "Command.h"
#include "MondisServer.h"

#define ADD(SET, TYPE) SET.insert(TYPE);

#define ERROR_EXIT(MESSAGE) cout<<MESSAGE;\
                            exit(1);

#define ADD_AND_RETURN(RES, UNDO) RES->modifies.push_back(UNDO);\
                                return RES;

unordered_set<CommandType> Executor::serverCommand;

JSONParser MondisServer::parser;

ExecutionResult MondisServer::execute(Command *command, MondisClient *client) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            Key *key = new Key((*command)[0].content);
            if (client == nullptr) {
                curKeySpace->put(key, parser.parseObject((*command)[1].content));
            } else {
                client->keySpace->put(key, parser.parseObject((*command)[1].content));
            }
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject *data;
            if (client == nullptr) {
                data = curKeySpace->get(key);
            } else {
                data = client->keySpace->get(key);
            }
            if (data == nullptr) {
                res.res = "the key does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getJson();
            OK_AND_RETURN
        }
        case DEL: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            if (client == nullptr) {
                curKeySpace->remove(key);
            } else {
                client->keySpace->remove(key);
            }
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            bool r;
            if (client == nullptr) {
                r = curKeySpace->containsKey(key);
            } else {
                r = client->keySpace->containsKey(key);
            }
            res.res = util::to_string(curKeySpace->containsKey(key));
            OK_AND_RETURN
        }
        case TYPE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject *data;
            if (client == nullptr) {
                data = curKeySpace->get(key);
            } else {
                data = client->keySpace->get(key);
            }
            if (data == nullptr) {
                res.res = "the key does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getTypeStr();
            OK_AND_RETURN;
        }
        case EXIT: {
            CHECK_PARAM_NUM(0)
            if (client == nullptr) {
                system("exit");
            } else {
#ifdef WIN32
                socketToClient.erase(socketToClient.find(&client->sock));
                delete client;
                FD_CLR(client->sock, &clientFds);
#elif defined(linux)
                fdToClient.erase(fdToClient.find(client->fd));
                delete client;
#endif
            }
        }
        case SAVE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            string jsonFile = (*command)[0].content;
            save(jsonFile);
            OK_AND_RETURN
        }
        case LOGIN: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            string userName = (*command)[0].content;
            string pwd = (*command)[1].content;
            if (client == nullptr) {
                if (userName == username && pwd == password) {
                    hasLogin = true;
                    OK_AND_RETURN
                }
                res.res = "username or password error";
                return res;
            } else {
                if (userName == username && pwd == password) {
                    client->hasLogin = true;
                    OK_AND_RETURN
                }
                res.res = "username or password error";
                return res;
            }
        }
        case SELECT: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, index);
            if (index < 0 || index >= dbs.size()) {
                res.res = "Invalid database id";
                LOGIC_ERROR_AND_RETURN
            }
            if (client == nullptr) {
                curDbIndex = index;
                curKeySpace = dbs[curDbIndex];
                OK_AND_RETURN
            } else {
                client->curDbIndex = index;
                client->keySpace = dbs[index];
                OK_AND_RETURN
            }
        }
        case RENAME: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            Key *key1 = new Key(PARAM(0));
            Key *key2 = new Key(PARAM(1));
            MondisObject *data = curKeySpace->get(*key1);
            if (data == nullptr) {
                res.res = "the object whose key is" + PARAM(0) + "does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            curKeySpace->put(key1, nullptr);
            curKeySpace->remove(*key1);
            curKeySpace->put(key2, data);
            OK_AND_RETURN

        }
        case M_SIZE: {
            CHECK_PARAM_NUM(0);
            if (client == nullptr) {
                res.res = to_string(curKeySpace->size());
            } else {
                res.res = to_string(client->keySpace->size());
            }
            OK_AND_RETURN
        }
        case SET_NAME: {
            CHECK_PARAM_NUM(1);
            CHECK_PARAM_TYPE(0, PLAIN)
            if (client == nullptr) {
                res.res = "no known client!";
                LOGIC_ERROR_AND_RETURN;
            }
            client->name = PARAM(0);
            OK_AND_RETURN
        }
        case SLAVE_OF: {
            if (isSlave) {
                sendToMaster(string("DISCONNECT"));
            }
            CHECK_PARAM_NUM(4);
            CHECK_PARAM_TYPE(0, PLAIN)
            isSlave = true;
#ifdef WIN32
            WORD sockVersion = MAKEWORD(2, 2);
            WSADATA data;
            WSAStartup(sockVersion, &data);
            SOCKET masterSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in serAddr;
            serAddr.sin_family = AF_INET;
            serAddr.sin_port = htons(atoi(PARAM(0).c_str()));
            serAddr.sin_addr.S_un.S_addr = inet_addr(PARAM(1).c_str());
            if (connect(masterSock, (sockaddr *) &serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
                res.res = "can not connect to master" + PARAM(0);
                closesocket(masterSock);
                LOGIC_ERROR_AND_RETURN
            }
            master = new MondisClient(masterSock);
            master->type = MASTER;
#elif defined(linux)
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in serAddr;
            serAddr.sin_family = AF_INET;
            serAddr.sin_port = htons((u_int16_t)atoi(PARAM(1).c_str()));
            serAddr.sin_addr.s_addr = htonl(atoi(PARAM(0).c_str()));
            int masterFd;
            if((masterFd = connect(sockfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))<0) {
                res.res = "can not connect to master" + PARAM(0);
                close(clientFd);
                LOGIC_ERROR_AND_RETURN
            }
            master = new MondisClient(masterFd);
            master->type = MASTER;
#endif
            sendToMaster(string("IS_SLAVE"));
            sendToMaster(string("PING"));
            string reply = readFromMaster();
            if (reply != "PONG") {
                res.res = "the socket to master is unavailable";
                res.type = INTERNAL_ERROR;
                return res;
            }
            sendToMaster(string("LOGIN ") + PARAM(2) + " " + PARAM(3));
            string loginRes = readFromMaster();
            if (loginRes.substr(0, 2) != "OK") {
                res.res = "username or password error";
                LOGIC_ERROR_AND_RETURN
            }
            sendToMaster(string("SYNC ") + to_string(replicaOffset) + " " + to_string(curDbIndex));
            string &&json = readFromMaster();
            curKeySpace->clear();
            JSONParser temp(json);
            temp.parse(curKeySpace);
            recvFromMaster = new thread([&]() {
                while (true) {
                    string next = readFromMaster();
                    execute(interpreter->getCommand(next), nullptr);
                    putToPropagateBuffer(next);
                }
            });
            if (client != nullptr) {
                OK_AND_RETURN;
            }
        }
        case SYNC: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, dbIndex);
            CHECK_AND_DEFINE_INT_LEGAL(1, offset);
            slaves.insert(client);
            client->type = SLAVE;
            std::thread t(&MondisServer::replicaToSlave, this, client, dbIndex, offset);
            OK_AND_RETURN
        }
        case SYNC_FINISHED: {
            CHECK_PARAM_NUM(0)
            isReplicatingFromMaster = false;
            res.res = "sync has finished";
            OK_AND_RETURN
        }
        case DISCONNECT: {
            closeClient(client);
            OK_AND_RETURN
        }
        case PING: {
            client->send("PONG");
            OK_AND_RETURN
        }
        case HEART_BEAT_TO: {
            client->updateHeartBeatTime();
            client->send("HEART_BEAT_REPLY");
            OK_AND_RETURN
        }
        case HEART_BEAT_REPLY: {
            client->updateHeartBeatTime();
            OK_AND_RETURN
        }
        case MULTI: {
            isInTransaction = true;
            transactionCommands = new queue<string>;
            OK_AND_RETURN
        }
        case EXEC: {
            if (!isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            if (watchedKeysHasModified) {
                res.res = "can not execute the transaction,because the following keys has been modified.\n";
                for (auto &key:modifiedKeys) {
                    res.res += key;
                    res.res += " ";
                }
                LOGIC_ERROR_AND_RETURN
            }
            while (!transactionCommands->empty()) {
                string next = transactionCommands->front();
                transactionCommands->pop();
                ExecutionResult res = execute(interpreter->getCommand(next), client);
                putToPropagateBuffer(next);
                if (res.type != OK) {
                    string undo = "UNDO";
                    while (hasExecutedCommandNumInTransaction > 0) {
                        execute(interpreter->getCommand(undo), client);
                        putToPropagateBuffer(undo);
                        hasExecutedCommandNumInTransaction--;
                    }
                    res.res = "error in executing the command ";
                    res.res += next;
                    LOGIC_ERROR_AND_RETURN
                }
                hasExecutedCommandNumInTransaction++;
            }
            closeTransaction();
            OK_AND_RETURN
        }
        case DISCARD: {
            closeTransaction();
        }
        case WATCH: {
            if (!isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            watchedKeys.insert(PARAM(0));
            OK_AND_RETURN
        }
        case UNWATCH: {
            if (!isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            watchedKeys.erase(watchedKeys.find(PARAM(0)));
            OK_AND_RETURN
        }
        case UNDO: {
            if (undoCommands.empty()) {
                res.res = "has reached max undo command number!";
                LOGIC_ERROR_AND_RETURN
            }
            MultiCommand *undo = undoCommands.back();
            undoCommands.pop_back();
            execute(undo, client);
            delete undo;
            replicaOffset--;
            replicaCommandBuffer->pop_back();
            OK_AND_RETURN;
        }
    }
    INVALID_AND_RETURN
}

MultiCommand *MondisServer::getUndoCommand(Command *locate, Command *modify, MondisObject *obj) {
    //TODO
    MultiCommand *res = new MultiCommand;
    res->locateCommand = locate;
    if (obj == nullptr) {
        Command *undo = new Command;
        switch (modify->type) {
            case BIND: {
                TOKEY(modify, 0);
                MondisObject *original = curKeySpace->get(key);
                if (original != nullptr) {
                    undo->type = BIND;
                    undo->addParam(RAW_PARAM(modify, 0));
                    undo->addParam(original->getJson(), Command::ParamType::STRING);
                } else {
                    undo->type = DEL;
                    undo->addParam(RAW_PARAM(modify, 0));
                }
                ADD_AND_RETURN(res, undo)
            }
            case DEL: {

            }
        }
    } else {

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

int MondisServer::runAsDaemon() {
#ifdef WIN32
    return 0;
#elif defined(linux)
    pid_t pid;
    int fd, i, nfiles;
    pid = fork();
    if(pid < 0) {
        ERROR_EXIT("First fork failed!");
    }

     if(pid > 0) {
         exit(EXIT_SUCCESS);// father exit
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
     return 0;
#endif
}

int MondisServer::appendLog(Log &log) {
    logFileOut<<log.toString();
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

int MondisServer::startEventLoop() {
    cout << "start event loop,now you can input command" << endl;
    while (true) {
        cout << username + "@Mondis>";
        string nextCommand;
        getline(std::cin, nextCommand);
        ExecutionResult res = execute(nextCommand, nullptr);
        cout << res.toString();
        cout << endl;
    }
}

//client表示执行命令的客户端，如果为nullptr则为Mondisserver自身
ExecutionResult MondisServer::execute(string &commandStr, MondisClient *client) {
    if (isPropagating) {
        unique_lock lck(propagateMtx);
        propagateCV.wait(lck);
    }
    if (!hasLogin) {
        ExecutionResult e;
        e.res = "you haven't login,please login";
        return e;
    }
    ExecutionResult res;
    bool isModifyCommand;
    Command *c = interpreter->getCommand(commandStr);
    Command *modify = c;
    bool isLocate = false;
    MondisObject *obj = nullptr;
    if (c->type == LOCATE) {
        Command *last = c;
        while (last->next->type == LOCATE) {
            last = last->next;
        }
        modify = last->next;
        isLocate = true;
    }
    isModifyCommand = modifyCommands.find(modify->type) != modifyCommands.end();
    if (isSlave && isModifyCommand) {
        res.res = "the current server is a slave,can not execute command which will modify database state";
        LOGIC_ERROR_AND_RETURN
    }
    if (isInTransaction && transactionAboutCommands.find(c->type) == transactionAboutCommands.end()) {
        transactionCommands->push(commandStr);
        OK_AND_RETURN
    }
    if (isLocate) {
        obj = chainLocate(c);
        res = obj->execute(modify);
    } else {
        res = executor->execute(modify, nullptr);
    }
    if (isModifyCommand && (isInTransaction || canUndoNotInTransaction)) {
        MultiCommand *undo = nullptr;
        if (isLocate) {
            undo = getUndoCommand(nullptr, modify, nullptr);
        } else {
            undo = getUndoCommand(c, modify, obj);
        }
        undoCommands.push_back(undo);
    }
    if (res.type == OK) {
        if (aof) {
            aofFileOut << commandStr + "\n";
            if (aofSyncStrategy == 1) {
                clock_t cur = clock();
                if (cur - preSync >= 1000) {
                    aofFileOut.flush();
                    preSync = cur;
                }
            } else if (aofSyncStrategy == 2) {
                aofFileOut.flush();
            }
        }
    }
    Log log(commandStr, res);
    logFileOut << log.toString();
    logFileOut.flush();
    if (isModifyCommand && res.type == OK) {
        if (replicaCommandBuffer->size() == maxCommandReplicaBufferSize) {
            replicaCommandBuffer->pop_front();
            replicaCommandBuffer->push_back(commandStr);
        }
        replicaOffset++;
        while (!putToPropagateBuffer(commandStr));
        if (isInTransaction) {
            switch (c->type) {
                case DEL:
                case RENAME:
                case BIND:
                case LOCATE: {
                    if (watchedKeys.find((*c)[0].content) != watchedKeys.end()) {
                        watchedKeysHasModified = true;
                        watchedKeys.erase(watchedKeys.find((*c)[0].content));
                        modifiedKeys.insert((*c)[0].content);
                    }
                    break;
                }
            }
        }
    } else {
        destroyCommand(c);
    }

    return res;
}

int MondisServer::save(string &jsonFile) {
#ifdef WIN32
    ofstream out(jsonFile + "2");
    out << curKeySpace->getJson();
    out.flush();
    remove(jsonFile.c_str());
    out.close();
    rename((jsonFile + "2").c_str(), jsonFile.c_str());
#elif defined(linux)
    int pid = fork();
    if(pid == 0) {
        ofstream out(jsonFile + "2");
        out << curKeySpace->getJson();
        out.flush();
        remove(jsonFile.c_str());
        out.close();
        rename((jsonFile + "2").c_str(), jsonFile.c_str());
        exit(0);
    }
#endif
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
            util::toInteger(kv.second, curDbIndex);
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
            maxCilentNum=atoi(kv.second.c_str());
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
            masterPort = kv.second;
        } else if(kv.first == "slaveOf") {
            if(kv.second == "true") {
                slaveOf = true;
            } else if(kv.second == "false"){
                slaveOf = false;
            }
        } else if (kv.first == "maxSlaveNum") {
            maxSlaveNum = atoi(kv.second.c_str());
        } else if (kv.first == "maxUndoCommandBufferSize") {
            maxUndoCommandBufferSize = min(maxCommandReplicaBufferSize, atoi(kv.second.c_str()));
        } else if (kv.first == "maxSlaveIdle") {
            maxSlaveIdle = atoi(kv.second.c_str());
        } else if (kv.first == "maxMasterIdle") {
            maxMasterIdle = atoi(kv.second.c_str());
        } else if (kv.first == "maxClientIdle") {
            maxClientIdle = atoi(kv.second.c_str());
        } else if (kv.first == "toSlaveHeartBeatDuration") {
            toSlaveHeartBeatDuration = atoi(kv.second.c_str());
        } else if (kv.first == "toClientHeartBeatDuration") {
            toClientHeartBeatDuration = atoi(kv.second.c_str());
        } else if (kv.first == "canUndoNotInTransaction") {
            if (kv.second == "true") {
                canUndoNotInTransaction = true;
            } else if (kv.second == "false") {
                canUndoNotInTransaction = false;
            }
        }
    }
}

void MondisServer::init() {
    cout << "is Initializing..." << endl;
    isLoading = true;
    Executor::init();
    Executor::bindServer(this);
    executor = Executor::getExecutor();
    JSONParser::LexicalParser::init();
    Command::init();
    CommandInterpreter::init();
    interpreter = new CommandInterpreter;
    for (int i = 0; i < databaseNum; ++i) {
        dbs.push_back(new HashMap(16, 0.75f, false, false));
    }
    curKeySpace = dbs[curDbIndex];
    if (aof) {
        aofFileOut.open(aofFile, ios::app);
    }
    if (json) {
        jsonFileOut.open(jsonFile, ios::app);
    }
    isRecovering = true;
    cout << "is Recovering..." << endl;
    if (recoveryStrategy == "json") {
        JSONParser temp(recoveryFile.c_str());
        temp.parse(curKeySpace);
    } else if (recoveryStrategy == "aof") {
        recoveryFileIn.open(recoveryFile);
        string command;
        while (getline(recoveryFileIn, command)) {
            execute(command, nullptr);
        }
    }
    isRecovering = false;
    if (daemonize) {
        runAsDaemon();
    }
    logFileOut.open(logFile, ios::app);
    if (slaveOf) {
        isReplicatingFromMaster = true;
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
    //命令传播
    std::thread propagateIO(&MondisServer::singleCommandPropagate, this);
    //检查超时的客户端，从服务器，主服务器并清理
    std::thread checkAndHandle(&MondisServer::checkAndHandleIdleClient, this);
    //向客户端发心跳包
    sendHeartBeatToClients = new std::thread([&]() {
        while (true) {
            for (auto &client:clients) {
                client->send("HEART_BEAT_TO");
            }
            std::this_thread::sleep_for(chrono::milliseconds(toClientHeartBeatDuration));
        }
    });
    //向从服务器发心跳包
    sendHeartBeatToSlaves = new std::thread([&]() {
        while (true) {
            for (auto &slave:slaves) {
                slave->send("HEART_BEAT_TO");
            }
            std::this_thread::sleep_for(chrono::milliseconds(toSlaveHeartBeatDuration));
        }
    });
#ifdef WIN32
    selectAndHandle(clientFds);
    std::thread handleSlaves(&MondisServer::selectAndHandle, this, slaveFds);
#elif defined(linux)
    selectAndHandle(clientsEpollFd,clientEvents);
    std::thread handleSlaves(&MondisServer::selectAndHandle,this,slavesEpollFd,slaveEvents);
#endif
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
        SOCKET clientSock = accept(servSock, (SOCKADDR *) &remoteAddr, &len);
        string auth = read(clientSock);
        MondisClient *client = new MondisClient(clientSock);
        if (auth == "IS_CLIENT") {
            if (clients.size() > maxCilentNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max client number!";
                send(clientSock, res.toString());
                delete client;
                continue;
            }
            client->type = CLIENT;
            FD_SET(clientSock, &clientFds);
            clients.insert(client);

        } else if (auth == "IS_SLAVE") {
            if (clients.size() > maxCilentNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max slave number!";
                send(clientSock, res.toString());
                continue;
            }
            client->type = SLAVE;
            FD_SET(clientSock, &slaveFds);
            slaves.insert(client);
        }
        socketToClient[&client->sock] = client;
    }
#elif defined(linux)
    int socket_fd;
    int connect_fd;
    struct sockaddr_in servaddr;
    servaddr.sin_family = PF_INET;
    servaddr.sin_addr.s_addr = htonl(atoi("127.0.0.1"));
    servaddr.sin_port = htons(port);
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(socket_fd,F_SETFL,O_NONBLOCK);
    bind(socket_fd,(sockaddr*)&servaddr, sizeof(servaddr));
    listen(socket_fd,10);
    while (true) {
        connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        string auth = read(clientSock);
        MondisClient *client = new MondisClient(clientSock);
        if(auth == "IS_CLIENT") {
            if (clients.size() > maxCilentNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max client number!";
                send(connect_fd, res.toString());
                delete client;
                continue;
            }
            epoll_event event;
            event.events = EPOLLIN|EPOLLET;
            client->type = CLIENT;
            epoll_ctl(clientsEpollFd, EPOLL_CTL_ADD, connect_fd, &event);
            clients.insert(client);

        } else if(auth == "IS_SLAVE") {
            if (clients.size() > maxCilentNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max slave number!";
                send(clientSock, res.toString());
                delete client;
                continue;
            }
            client->type = SLAVE;
            epoll_event event;
            event.events = EPOLLIN|EPOLLET;
            epoll_ctl(slavesEpollFd, EPOLL_CTL_ADD, connect_fd, &event);
            slaves.insert(client);
        }
        fcntl(connect_fd,F_SETFL,O_NONBLOCK);
        fdToClient[connect_fd] = client;
    }
#endif
}

void MondisServer::handleCommand(MondisClient *client) {
    string commandStr;
    while ((commandStr = client->readCommand()) != "") {
        ExecutionResult res = execute(commandStr, nullptr);
        client->send(res.toString());
    }
}

MondisServer::MondisServer() {
#ifdef WIN32
    FD_ZERO(&clientFds);
    FD_ZERO(&slaveFds);
#elif defined(linux)
    clientsEpollFd = epoll_create(1024);
    clientEvents = new epoll_event[1024];
    slavesEpollFd = epoll_create(1024);
    slaveEvents = new epoll_event[1024];
#endif
    replicaCommandBuffer = new deque<string>;
    commandPropagateBuffer = new queue<string>;
}

void MondisServer::sendToMaster(const string &res) {
#ifdef WIN32
    send(master->sock, res);
#elif defined(linux)
    send(master->fd,res);
#endif
}

MondisServer::~MondisServer() {
#ifdef linux
    delete[] clientEvents;
    delete[] slaveEvents;
#endif
    delete replicaCommandBuffer;
    delete commandPropagateBuffer;
    delete recvFromMaster;
    delete sendHeartBeatToClients;
    delete sendHeartBeatToSlaves;
}

void MondisServer::replicaToSlave(MondisClient *client, unsigned dbIndex, unsigned long long slaveReplicaOffset) {
    if (replicaOffset - slaveReplicaOffset > maxCommandReplicaBufferSize) {
        const unsigned long long start = replicaOffset;
        const string &json = dbs[dbIndex]->getJson();
        client->send(json);
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - start), replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
        isPropagating = false;
        propagateCV.notify_all();
    } else {
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - slaveReplicaOffset),
                                replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
        isPropagating = false;
        propagateCV.notify_all();
    }
}

void MondisServer::singleCommandPropagate() {
    while (true) {
        const string& cur = takeFromPropagateBuffer();
        for (MondisClient *slave:slaves) {
            slave->send(cur);
        }
    }
}

void MondisServer::replicaCommandPropagate(vector<string> &commands, MondisClient *client) {
    for (auto &c:commands) {
        client->send(c);
    }
}

void MondisServer::initStaticMember() {
    ADD(modifyCommands, BIND)
    ADD(modifyCommands, DEL)
    ADD(modifyCommands, RENAME)
    ADD(modifyCommands, SET_RANGE)
    ADD(modifyCommands, REMOVE_RANGE)
    ADD(modifyCommands, INCR)
    ADD(modifyCommands, DECR)
    ADD(modifyCommands, INCR_BY)
    ADD(modifyCommands, DECR_BY)
    ADD(modifyCommands, APPEND)
    ADD(modifyCommands, PUSH_FRONT)
    ADD(modifyCommands, PUSH_BACK)
    ADD(modifyCommands, POP_FRONT)
    ADD(modifyCommands, POP_BACK)
    ADD(modifyCommands, ADD)
    ADD(modifyCommands, REMOVE)
    ADD(modifyCommands, REMOVE_BY_RANK)
    ADD(modifyCommands, REMOVE_BY_SCORE)
    ADD(modifyCommands, REMOVE_RANGE_BY_RANK)
    ADD(modifyCommands, REMOVE_RANGE_BY_SCORE)
    ADD(modifyCommands, WRITE)
    ADD(modifyCommands, TO_STRING)
    ADD(modifyCommands, CHANGE_SCORE)
    ADD(modifyCommands, UNDO)
    ADD(transactionAboutCommands, DISCARD)
    ADD(transactionAboutCommands, EXEC)
    ADD(transactionAboutCommands, WATCH)
    ADD(transactionAboutCommands, UNWATCH)
}

string MondisServer::readFromMaster() {
#ifdef WIN32
    return read(master->sock);
#elif defined(linux)
    return read(master->fd);
#endif
}

string MondisServer::takeFromPropagateBuffer() {
    if(commandPropagateBuffer->empty()) {
        unique_lock lck(notEmptyMtx);
        notEmpty.wait(lck);
    }
    string& res = commandPropagateBuffer->front();
    commandPropagateBuffer->pop();
    return res;
}

bool MondisServer::putToPropagateBuffer(const string &curCommand) {
    if(commandPropagateBuffer->size() == maxCommandPropagateBufferSize) {
        return false;
    }
    commandPropagateBuffer->push(curCommand);
    notEmpty.notify_all();
    return true;
}

void MondisServer::closeTransaction() {
    isInTransaction = false;
    watchedKeys.clear();
    modifiedKeys.clear();
    delete transactionCommands;
    watchedKeysHasModified = false;
    hasExecutedCommandNumInTransaction = 0;
}

ExecutionResult Executor::execute(Command *command, MondisClient *client) {
    if(serverCommand.find(command->type)!=serverCommand.end()) {
        if (command->next == nullptr || command->next->type == VACANT) {
            ExecutionResult res = server->execute(command, client);
            return res;
        }
        ExecutionResult res;
        res.type = LOGIC_ERROR;
        res.res = "invalid pipeline command";
        return res;
    }
    ExecutionResult res;
    res.type = LOGIC_ERROR;
    res.res = "Invalid command";
    return res;
}

Executor::Executor() {
}

MondisServer *Executor::server = nullptr;

Executor *Executor::getExecutor() {
    return executor;
}

void Executor::init() {
    INSERT(BIND)
    INSERT(GET)
    INSERT(LOGIN)
    INSERT(EXISTS)
    INSERT(RENAME)
    INSERT(TYPE)
    INSERT(SAVE)
    INSERT(EXIT)
    INSERT(SELECT)
    INSERT(DEL)
    INSERT(SLAVE_OF)
    INSERT(SYNC)
    INSERT(SET_NAME)
    INSERT(SYNC_FINISHED)
    INSERT(DISCONNECT)
    INSERT(HEART_BEAT_TO)
    INSERT(HEART_BEAT_REPLY)
    INSERT(UNDO)
    INSERT(MULTI)
    INSERT(EXEC)
    INSERT(DISCARD)
    INSERT(WATCH)
    INSERT(UNWATCH)
}

void MondisServer::destroyCommand(Command *command) {
    Command* cur = command;
    while (cur!= nullptr) {
        Command* temp = cur;
        cur = cur->next;
        delete temp;
    }
}

void MondisServer::checkAndHandleIdleClient() {
    while (true) {
        clock_t current = clock();
#ifdef WIN32
        for (auto &kv:socketToClient) {
            MondisClient *c = kv.second;
            if (c->type == CLIENT) {
                if (current - c->preInteraction > toClientHeartBeatDuration) {
                    closeClient(c);
                }
            } else if (c->type == SLAVE) {
                if (current - c->preInteraction > toSlaveHeartBeatDuration) {
                    closeClient(c);
                }
            }
        }
#elif defined(linux)
        for (auto &kv:fdToClient) {
            MondisClient *c = kv.second;
            if (c->type == CLIENT) {
                if (current - c->preInteraction > toClientHeartBeatDuration) {
                    closeClient(c);
                }
            } else if (c->type == SLAVE) {
                if (current - c->preInteraction > toSlaveHeartBeatDuration) {
                    closeClient(c);
                }
            }
        }
#endif
        if (current - master->preInteraction > maxMasterIdle) {
            delete master;
            master = nullptr;
            delete recvFromMaster;
            recvFromMaster = nullptr;
        }
    }
}

void MondisServer::closeClient(MondisClient *client) {
#ifdef WIN32
    if (client->type == CLIENT) {
        FD_CLR(client->sock, &clientFds);
        clients.erase(clients.find(client));
    } else if (client->type == SLAVE) {
        FD_CLR(client->sock, &slaveFds);
        slaves.erase(slaves.find(client));
    }
    socketToClient.erase(socketToClient.find(&client->sock));
#elif defined(linux)
    if(client->type == CLIENT) {
        epoll_ctl(clientsEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        clients.erase(clients.find(client));
    } else if(client->type == SLAVE) {
        epoll_ctl(slavesEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        slaves.erase(slaves.find(client));
    }
    fdToClient.erase(fdToClient.find(client->fd));
#endif
    delete client;
}

void MondisServer::execute(MultiCommand *command, MondisClient *client) {
    if (command->locateCommand == nullptr) {
        for (auto m:command->modifies) {
            execute(m, client);
        }
    } else {
        MondisObject *obj = chainLocate(command->locateCommand);
        for (auto m:command->modifies) {
            obj->execute(m);
        }
    }
}

MondisObject *MondisServer::chainLocate(Command *command) {
    MondisObject *curObj = curKeySpace->locate(command);
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

void Executor::bindServer(MondisServer *sv) {
    server = sv;
}

Executor *Executor::executor = new Executor;

unordered_set<CommandType> MondisServer::modifyCommands;
unordered_set<CommandType> MondisServer::transactionAboutCommands;