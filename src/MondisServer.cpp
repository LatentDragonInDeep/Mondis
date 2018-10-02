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
#include <netinet.h>
#include <stdlib>
#include <stdio>
#include <sys/epoll.h>
#endif

#include "Command.h"
#include "MondisServer.h"

#define ADD(TYPE) modifyCommands.insert(TYPE);

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
                FD_CLR(client->sock, &fds);
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
            this->masterSock = masterSock;
#elif defined(linux)
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in serAddr;
            serAddr.sin_family = AF_INET;
            serAddr.sin_port = htons(atoi(port.c_str()));
            serAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
            if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0) {
                res.res = "can not connect to master" + PARAM(0);
                closesocket(sclient);
                LOGIC_ERROR_AND_RETURN
            }
#endif
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
            JSONParser temp(json);
            temp.parse(curKeySpace);
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
#ifdef WIN32
            socketToClient.erase(socketToClient.find(&client->sock));
#elif defined(linux)
            fdToClient.erase(fdToClient.find(client->fd));
#endif
            delete client;
            OK_AND_RETURN
        }
        case PING: {
            client->sendResult("PONG");
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

ExecutionResult MondisServer::locateExecute(Command *command) {
    ExecutionResult res;
    MondisObject *curObj = curKeySpace->locate(command);
    Command *curCommand = command->next;
    while (true) {
        if(curObj == nullptr) {
            res.type = LOGIC_ERROR;
            res.res = "Error on locating";
            return res;
        }
        if(curCommand->type!=LOCATE) {
            break;
        }
        curObj = curObj->locate(curCommand);
        curCommand = curCommand->next;
    }

    return curObj->execute(curCommand);

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
    struct rlimit rl;
    pid = fork();
    if(pid < 0)
        ERROR_EXIT("First fork failed!");

     if(pid > 0)
         exit(EXIT_SUCCESS);// father exit

     if(setsid() == -1)
         ERROR_EXIT("setsid failed!");

     pid = fork();
     if(pid < 0)
         ERROR_EXIT("Second fork failed!");

     if(pid > 0)// father exit
         exit(EXIT_SUCCESS);
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
     if(fd = open("/dev/null", O_RDWR) < 0)
         ERROR_EXIT("open /dev/null failed!");
     for(i=0; i<3; i++)
         dup2(fd, i);
     if(fd > 2) close(fd);
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
    startEventLoop();
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
        unique_lock lck(mtx);
        cv.wait(lck);
    }
    if (!hasLogin) {
        ExecutionResult e;
        e.res = "you haven't login,please login";
        return e;
    }
    Command *c = interpreter->getCommand(commandStr);
    Command *last = c;
    while (last->next != nullptr) {
        last = last->next;
    }
    bool isModifyCommand = modifyCommands.find(last->type) != modifyCommands.end();
    if (isSlave && isModifyCommand) {
        ExecutionResult res;
        res.res = "the current server is a slave,can not execute command which will modify database state";
        LOGIC_ERROR_AND_RETURN
    }
    ExecutionResult res = executor->execute(c, nullptr);
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
    if (isModifyCommand) {
        singleCommandPropagate();
        if (replicaCommandBuffer->size() == MAX_COMMAND_BUFFER_SIZE) {
            replicaCommandBuffer->pop_front();
            replicaCommandBuffer->push_back(commandStr);
        }
        replicaOffset++;
    }
    curCommand = commandStr;
    cv2.notify_all();

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
        } else if (kv.first == "slaveOf") {
            slaveof = kv.second;
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
    if (slaveof != "") {
        isReplicatingFromMaster = true;
        cout << "is Replicating from master..." << endl;
        string sync = "SLAVE_OF ";
        sync += slaveof;
        execute(sync, nullptr);
    }
    std::thread accept(&MondisServer::acceptClient, this);
    std::thread eventLoop(&MondisServer::startEventLoop, this);
    std::thread propagateIO(&MondisServer::singleCommandPropagate, this);
    selectAndHandle();
}

void MondisServer::acceptClient() {
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
        if (socketToClient.size() > MAX_SOCK_NUM) {
            ExecutionResult res;
            res.type = LOGIC_ERROR;
            res.res = "can not build connection because has up to max sockets number!";
            send(clientSock, res.toString());
        }
        FD_SET(clientSock, &fds);
        MondisClient *client = new MondisClient(clientSock);
        socketToClient[&client->sock] = client;
    }
#elif defined(linux)
    int socket_fd;
    int connect_fd;
    struct sockaddr_in servaddr;
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    sockAddr.sin_port = htons(port);
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    setnoblocking(socket_fd);
    bind(socket_fd,&servaddr, sizeof(servaddr));
    listen(socket_fd,10);
    while (true) {
        connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        if(fdToClient.size()>MAX_SOCK_NUM) {
            ExecutionResult res;
            res.type = LOGIC_ERROR;
            res.res = "can not build connection because has up to max sockets number!"
            send(connect_fd,res.toString());
        }
        setnoblocking(connect_fd);
        MondisClient* client = new MondisClient(connect_fd);
        fdToClient[connect_fd] = client;
        epoll_event event;
        event.events = EPOLLIN|EPOLLET;
        epoll_ctl(efd, EPOLL_CTL_ADD, connect_fd, &event);
    }
#endif
}

void MondisServer::handleCommand(MondisClient *client) {
    string commandStr;
    while ((commandStr = client->readCommand()) != "") {
        ExecutionResult res = execute(commandStr, nullptr);
        client->sendResult(res.toString());
    }
}

void MondisServer::selectAndHandle() {
#ifdef WIN32
    while (true) {
        int ret = select(0, &fds, nullptr, nullptr, nullptr);
        if (ret == 0) {
            continue;
        }
        for (auto &pair:socketToClient) {
            if (FD_ISSET(*pair.first, &fds)) {
                MondisClient *client = pair.second;
                handleCommand(client);
            }
        }
    }
#elif defined(linux)
    while (true) {
        int nfds = epoll_wait(epollFd, events, MAX_SOCK_NUM, -1);
        for(int i=0;i<nfds;i++) {
            MondisClient* client = fdToClient[events[i].data.fd];
            handleCommand(client);
        }
    }
#endif
}

MondisServer::MondisServer() {
#ifdef WIN32
    FD_ZERO(&fds);
#elif defined(linux)
    epollFd = epoll_create(1024);
    events = new epoll_events[1024];
#endif
    replicaCommandBuffer = new deque<string>;
}

void MondisServer::sendToMaster(const string &res) {
#ifdef WIN32
    send(masterSock, res);
#elif defined(linux)
    send(masterFd,res);
#endif
}

MondisServer::~MondisServer() {
    delete replicaCommandBuffer;
}

void MondisServer::replicaToSlave(MondisClient *client, unsigned dbIndex, unsigned long long slaveReplicaOffset) {
    if (replicaOffset - slaveReplicaOffset > MAX_COMMAND_BUFFER_SIZE) {
        const unsigned long long start = replicaOffset;
        const string &json = dbs[dbIndex]->getJson();
        client->sendResult(json);
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - start), replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
        isPropagating = false;
        cv.notify_all();
    } else {
        isPropagating = true;
        vector<string> commands(replicaCommandBuffer->begin() + (replicaOffset - slaveReplicaOffset),
                                replicaCommandBuffer->end());
        replicaCommandPropagate(commands, client);
        isPropagating = false;
        cv.notify_all();
    }
}

void MondisServer::singleCommandPropagate() {
    std::unique_lock lck(mtx2);
    cv2.wait(lck);
    while (true) {
        for (MondisClient *slave:slaves) {
            slave->sendResult(curCommand);
        }
        cv2.wait(lck);
    }
}

void MondisServer::replicaCommandPropagate(vector<string> &commands, MondisClient *client) {
    for (auto &c:commands) {
        client->sendResult(c);
    }
}

void MondisServer::initModifyCommands() {
    ADD(BIND)
    ADD(DEL)
    ADD(RENAME)
    ADD(SET_RANGE)
    ADD(REMOVE_RANGE)
    ADD(INCR)
    ADD(DECR)
    ADD(INCR_BY)
    ADD(DECR_BY)
    ADD(APPEND)
    ADD(PUSH_FRONT)
    ADD(PUSH_BACK)
    ADD(POP_FRONT)
    ADD(POP_BACK)
    ADD(ADD)
    ADD(REMOVE)
    ADD(REMOVE_BY_RANK)
    ADD(REMOVE_BY_SCORE)
    ADD(REMOVE_RANGE_BY_RANK)
    ADD(REMOVE_RANGE_BY_SCORE)
    ADD(WRITE)
    ADD(TO_STRING)
    ADD(CHANGE_SCORE)
}

string MondisServer::readFromMaster() {
#ifdef WIN32
    return read(masterSock);
#elif defined(linux)
    return read(masterFd);
#endif
}

ExecutionResult Executor::execute(Command *command, MondisClient *client) {
    if(serverCommand.find(command->type)!=serverCommand.end()) {
        if (command->next == nullptr || command->next->type == VACANT) {
            ExecutionResult res = server->execute(command, client);
            destroyCommand(command);
            return res;
        }
        destroyCommand(command);
        ExecutionResult res;
        res.type = LOGIC_ERROR;
        res.res = "invalid pipeline command";
        return res;
    } else if (command->type == LOCATE) {
        ExecutionResult res = server->locateExecute(command);
        destroyCommand(command);
        return res;
    }
    destroyCommand(command);
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
}

void Executor::destroyCommand(Command *command) {
    if(command == nullptr) {
        return;
    }
    Command* cur = command;
    while (cur!= nullptr) {
        Command* temp = cur;
        cur = cur->next;
        delete temp;
    }

}

void Executor::bindServer(MondisServer *sv) {
    server = sv;
}

Executor *Executor::executor = new Executor;

unordered_set<CommandType> MondisServer::modifyCommands;