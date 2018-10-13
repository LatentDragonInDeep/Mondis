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
#define TO_FULL_KEY_NAME(DBINDEX, KEY) to_string(DBINDEX)+"_"+KEY

unordered_set<CommandType> MondisServer::controlCommands;

JSONParser MondisServer::parser;

ExecutionResult MondisServer::execute(Command *command, MondisClient *client) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            Key *key = new Key((*command)[0].content);
            client->keySpace->put(key, parser.parseObject((*command)[1].content));

            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject *data;
            data = client->keySpace->get(key);
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
            client->keySpace->remove(key);
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            bool r = client->keySpace->containsKey(key);
            res.res = util::to_string(r);
            OK_AND_RETURN
        }
        case TYPE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject *data;
            data = client->keySpace->get(key);
            if (data == nullptr) {
                res.res = "the key does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getTypeStr();
            OK_AND_RETURN;
        }
        case EXIT: {
            CHECK_PARAM_NUM(0)
            system("exit");
        }
        case SAVE: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(1, index)
            if (index > databaseNum - 1) {
                res.res = "database index out of range!";
                LOGIC_ERROR_AND_RETURN
            }
            string jsonFile = (*command)[0].content;
            save(jsonFile, index);
            OK_AND_RETURN
        }
        case SAVE_ALL: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            string jsonFile = (*command)[0].content;
            saveAll(jsonFile);
            OK_AND_RETURN
        }
        case LOGIN: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            string userName = (*command)[0].content;
            string pwd = (*command)[1].content;
            if (userName == username && pwd == password) {
                client->hasAuthenticate = true;
                OK_AND_RETURN
            }
            res.res = "username or password error";
            LOGIC_ERROR_AND_RETURN
        }
        case SELECT: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, index);
            if (index < 0 || index >= dbs.size()) {
                res.res = "Invalid database id";
                LOGIC_ERROR_AND_RETURN
            }
            client->curDbIndex = index;
            client->keySpace = dbs[index];
            OK_AND_RETURN
        }
        case RENAME: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            Key *key1 = new Key(PARAM(0));
            Key *key2 = new Key(PARAM(1));
            MondisObject *data = client->keySpace->get(*key1);
            if (data == nullptr) {
                res.res = "the object whose key is" + PARAM(0) + "does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            client->keySpace->put(key1, nullptr);
            client->keySpace->remove(*key1);
            client->keySpace->put(key2, data);
            OK_AND_RETURN

        }
        case M_SIZE: {
            CHECK_PARAM_NUM(0);
            res.res = to_string(client->keySpace->size());
            OK_AND_RETURN
        }
        case SET_CLIENT_NAME: {
            CHECK_PARAM_NUM(1);
            CHECK_PARAM_TYPE(0, PLAIN)
            nameToClients.erase(nameToClients.find(client->name));
            nameToClients[PARAM(0)] = client;
            client->name = PARAM(0);
            OK_AND_RETURN
        }
        case SLAVE_OF: {
            res = beSlaveOf(command, client, PARAM(2), PARAM(3));
            if (res.type == OK) {
                masterIP = PARAM(0);
                masterPort = atoi(PARAM(1).c_str());
            }
            return res;
        }
        case SYNC: {
            if (idToPeers.size() > maxSlaveNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max slave number!";
                client->send(res.toString());
#ifdef WIN32
                FD_CLR(client->sock, &clientFds);
                socketToClient.erase(socketToClient.find(&client->sock));
#elif defined(linux)
                epoll_ctl(clientFd,EPOLL_CTL_DEL,client->fd);
                fdToClient.erase(fdToClient.find(client->fd));
#endif
                delete client;
            }
            client->type = SLAVE;
#ifdef WIN32
            FD_CLR(client->sock, &clientFds);
            FD_SET(client->sock, &peerFds);
#elif defined(linux)
            epoll_ctl(clientFd,EPOLL_CTL_DEL,client->fd);
            epoll_ctl(slaveFd,EPOLL_CTL_ADD,client->fd);

#endif
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(1, offset);
            if (isSlave) {
                Command *temp = new Command;
                temp->type = GET_MASTER;
                ExecutionResult realRes = execute(temp, nullptr);
                Command::destroyCommand(temp);
                res.res = "the target server is a slave,if you want continue,please input correct master "
                          "ip and port.the target server's master ip and port is ";
                res.res += realRes.res;
                delete client;
                LOGIC_ERROR_AND_RETURN
            }
            idToPeers[nextPeerId()] = client;
            client->type = SLAVE;
            std::thread t(&MondisServer::replicaToSlave, this, client, offset);
            OK_AND_RETURN
        }
        case SYNC_FINISHED: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            isReplicatingFromMaster = false;
            res.needSend = false;
            id = atoi(PARAM(0).c_str());
            OK_AND_RETURN
        }
        case DISCONNECT_CLIENT: {
            if (client->type != CLIENT) {
                res.res = "the sender is not a client!";
                LOGIC_ERROR_AND_RETURN
            }
            closeClient(client);
            OK_AND_RETURN
        }
        case DISCONNECT_SLAVE: {
            if (isMaster) {
                if (client->type != SLAVE) {
                    res.res = "the sender is not a slave!";
                    LOGIC_ERROR_AND_RETURN
                }
                closeClient(client);
                OK_AND_RETURN
            } else {
                sendToMaster("DISCONNECT_SLAVE");
                string resStr = readFromMaster();
                res = ExecutionResult::stringToResult(resStr);
                return res;
            }
        }
        case PING: {
            client->send("PONG");
            client->updateHeartBeatTime();
            res.needSend = false;
            OK_AND_RETURN
        }
        case PONG: {
            client->updateHeartBeatTime();
            res.needSend = false;
            OK_AND_RETURN
        }
        case MULTI: {
            client->startTransaction();
            OK_AND_RETURN
        }
        case EXEC: {
            return client->commitTransaction(this);
        }
        case DISCARD: {
            client->closeTransaction();
        }
        case WATCH: {
            if (!client->isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].insert(client);
            client->watchedKeys.insert(PARAM(0));
            OK_AND_RETURN
        }
        case UNWATCH: {
            if (!client->isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].erase(
                    keyToWatchedClients[PARAM(0)].find(client));
            if (keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].size() == 0) {
                keyToWatchedClients.erase(keyToWatchedClients.find(TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))));
            }
            client->watchedKeys.erase(client->watchedKeys.find(TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))));
            OK_AND_RETURN
        }
        case GET_MASTER: {
            CHECK_PARAM_NUM(0)
            if (isMaster) {
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
                string ip;
                unsigned int ipInt = ntohl(((struct in_addr*)hent->h_addr)->s_addr);
                int first = (ipInt>>24)&0xff;
                int second = (ipInt>>16)&0xff;
                int third = (ipInt>>8)&0xff;
                int four = ipInt&0xff;
                ip+=first;
                ip+=".";
                ip+=second;
                ip+=".";
                ip+=third;
                ip+=".";
                ip+="four";
#endif
                res.res += "master ip:";
                res.res += ip;
                res.res += " master port:";
                res.res += to_string(port);
                OK_AND_RETURN
            } else if (isSlave) {
                res.res += "master ip:";
                res.res += masterIP;
                res.res += " master port:";
                res.res += masterPort;
                OK_AND_RETURN
            } else {
                res.res = "current server has no master";
                LOGIC_ERROR_AND_RETURN
            }
        }
        case NEW_PEER: {
            if (client->type != MASTER) {
                res.res = "the command is not from master!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(3)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_PARAM_TYPE(2, PLAIN)
            MondisClient *peer = buildConnection(PARAM(0), atoi(PARAM(1).c_str()));
            if (peer == nullptr) {
                res.res = "can not connect to peer which ip is ";
                res.res += PARAM(0);
                res.res += " port is ";
                res.res += PARAM(1);
            }
            idToPeers[atoi(PARAM(2).c_str())] = client;
            string c = "MASTER_INVITE ";
            c += masterIP;
            c += " ";
            c += masterPort;
            peer->send(c);
        }
        case IS_CLIENT: {
            if (nameToClients.size() > maxCilentNum) {
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max client number!";
                client->send(res.toString());
#ifdef WIN32
                FD_CLR(client->sock, &clientFds);
                socketToClient.erase(socketToClient.find(&client->sock));
#elif defined(linux)
                epoll_ctl(clientFd,EPOLL_CTL_DEL,client->fd);
                fdToClient.erase(fdToClient.find(client->fd));
#endif
                delete client;
            }
            client->type = CLIENT;
            nameToClients[nextDefaultClientName()] = client;
            OK_AND_RETURN
        }
        case MASTER_INVITE: {
            CHECK_PARAM_NUM(3);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_PARAM_TYPE(2, PLAIN)
            if (PARAM(0) == masterIP && atoi(PARAM(1).c_str()) == masterPort) {
                client->hasAuthenticate = true;
#ifdef WIN32
                FD_CLR(client->sock, &clientFds);
                FD_SET(client->sock, &peerFds);
#elif defined(linux)
                epoll_ctl(clientFd,EPOLL_CTL_DEL,client->fd);
                epoll_ctl(peerFd,EPOLL_CTL_ADD,client->fd);
#endif
                idToPeers[atoi(PARAM(2).c_str())] = client;
                res.needSend = false;
                OK_AND_RETURN
            }
            res.res = "wrong master ip or port";
            LOGIC_ERROR_AND_RETURN
        }
        case ASK_FOR_VOTE: {
            if (hasVoteFor) {
                client->send("UNVOTE");
            } else if (maxOffsetClients.find(client) != maxOffsetClients.end()) {
                client->send("VOTE");
                hasVoteFor = true;
            }
            res.needSend = false;
            OK_AND_RETURN
        }
        case VOTE: {
            voteNum++;
            if (voteNum > idToPeers.size() / 2) {
                delete forVote;
                forVote = nullptr;
                for (auto &kv:idToPeers) {
                    kv.second->send("I_AM_NEW_MASTER");
                }
                //TODO 更新所有slave的offset最新
                //TODO 处理双leader的情况
            }
            res.needSend = false;
            OK_AND_RETURN
        }
        case UNVOTE: {
            res.needSend = false;
            OK_AND_RETURN
        }
        case MASTER_DEAD: {
            unsigned long long maxOffset = 0;
            for (auto &kv:idToPeers) {
                kv.second->send("UPDATE_OFFSET");
                unsigned long long cur = atoll(kv.second->read().c_str());
                if (cur > maxOffset) {
                    maxOffsetClients.clear();
                    maxOffsetClients.insert(kv.second);
                } else if (cur == maxOffset) {
                    maxOffsetClients.insert(kv.second);
                }
            }
            res.needSend = false;
            forVote = new thread(&MondisServer::askForVote, this);
            OK_AND_RETURN;
        }
        case I_AM_NEW_MASTER: {
            //TODO
        }
        case UPDATE_OFFSET: {
            client->send(to_string(replicaOffset));
            res.needSend = false;
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

MultiCommand *MondisServer::getUndoCommand(CommandStruct &cstruct, MondisClient *client) {
    //TODO
    MultiCommand *res = new MultiCommand;
    res->locateCommand = cstruct.locate;
    if (cstruct.obj == nullptr) {
        Command *undo = new Command;
        switch (cstruct.operation->type) {
            case BIND: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = client->keySpace->get(key);
                if (original != nullptr) {
                    undo->type = BIND;
                    undo->addParam(RAW_PARAM(cstruct.operation, 0));
                    undo->addParam(original->getJson(), Command::ParamType::STRING);
                } else {
                    undo->type = DEL;
                    undo->addParam(RAW_PARAM(cstruct.operation, 0));
                }
                ADD_AND_RETURN(res, undo)
            }
            case DEL: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = client->keySpace->get(key);
                if (original == nullptr) {
                    return res;
                }
                undo->type = BIND;
                undo->addParam(RAW_PARAM(cstruct.operation, 0));
                undo->addParam(original->getJson(), Command::ParamType::STRING);
                ADD_AND_RETURN(res, undo)
            }
            case RENAME: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = client->keySpace->get(key);
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
                undo->addParam(to_string(client->curDbIndex), Command::ParamType::PLAIN);
                ADD_AND_RETURN(res, undo);
            }
        }
    } else {
        switch (cstruct.obj->type) {
            case RAW_STRING: {
                switch (cstruct.operation->type) {
                    case BIND: {

                    }
                    case SET_RANGE: {

                    }
                    case REMOVE_RANGE: {

                    }
                    case TO_INTEGER: {

                    }
                    case APPEND: {

                    }
                }
            }
            case RAW_INT: {
                switch (cstruct.operation->type) {
                    case INCR: {

                    }
                    case INCR_BY: {

                    }
                    case DECR: {

                    }
                    case DECR_BY: {

                    }
                    case TO_STRING: {

                    }
                }
            }
            case LIST: {
                switch (cstruct.operation->type) {
                    case POP_FRONT: {

                    }
                    case POP_BACK: {

                    }
                    case PUSH_FRONT: {

                    }
                    case PUSH_BACK: {

                    }
                    case BIND: {

                    }
                }
            }
            case SET: {
                switch (cstruct.operation->type) {
                    case ADD: {

                    }
                    case REMOVE: {

                    }
                }
            }
            case RAW_BIN: {
                switch (cstruct.operation->type) {
                    case BIND: {

                    }
                    case SET_RANGE: {

                    }
                    case WRITE: {

                    }
                }

            }
            case ZSET: {
                switch (cstruct.operation->type) {
                    case ADD: {

                    }
                    case REMOVE_BY_RANK: {

                    }
                    case REMOVE_BY_SCORE: {

                    }
                    case REMOVE_RANGE_BY_RANK: {

                    }
                    case REMOVE_RANGE_BY_SCORE: {

                    }
                    case CHANGE_SCORE: {

                    }
                }
            }
            case HASH: {
                switch (cstruct.operation->type) {
                    case BIND: {

                    }
                    case DEL: {

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

void MondisServer::appendLog(string &commandStr, ExecutionResult &res) {
    Log log(commandStr, res);
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
        ExecutionResult res = execute(nextCommand, self);
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
    ExecutionResult res;
    if (!client->hasAuthenticate) {
        res.res = "you haven't login,please login";
        LOGIC_ERROR_AND_RETURN
    }
    Command *command = interpreter->getCommand(commandStr);
    if (controlCommands.find(command->type) != controlCommands.end()) {
        if (command->next == nullptr || command->next->type == VACANT) {
            res = execute(command, client);
            return res;
        }
    } else if (command->type != LOCATE) {
        res.res = "Invalid command";
        LOGIC_ERROR_AND_RETURN
    }
    CommandStruct cstruct = getCommandStruct(command, client);
    if (isSlave && cstruct.isModify) {
        if (autoMoveCommandToMaster) {
            Command::destroyCommand(command);
            sendToMaster(commandStr);
            string resStr = readFromMaster();
            client->send(resStr);
            return res;
        } else {
            res.res = "the current server is a slave,can not undoExecute command which will modify database state";
            LOGIC_ERROR_AND_RETURN
        }
    }
    if (client->type == CLIENT && clientControlCommands.find(cstruct.operation->type) == clientControlCommands.end()) {
        res.res = "this command can not execute from a client";
        LOGIC_ERROR_AND_RETURN
    }
    if (client->isInTransaction && transactionAboutCommands.find(command->type) == transactionAboutCommands.end()) {
        client->transactionCommands->push(commandStr);
        OK_AND_RETURN
    }
    res = transactionExecute(cstruct, client);
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
    } else {
        Command::destroyCommand(command);
    }

    return res;
}

void MondisServer::save(string &jsonFile, int dbIndex) {
#ifdef WIN32
    ofstream out(jsonFile + "2");
    out << dbs[dbIndex]->getJson();
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
            util::toInteger(kv.second, self->curDbIndex);
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
            masterPort = atoi(kv.second.c_str());
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
    isLoading = true;
    JSONParser::LexicalParser::init();
    Command::init();
    CommandInterpreter::init();
    ExecutionResult::init();
    initStaticMember();
    interpreter = new CommandInterpreter;
    for (int i = 0; i < databaseNum; ++i) {
        dbs.push_back(new HashMap(16, 0.75f, false, false));
    }
    self->keySpace = dbs[self->curDbIndex];
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
        temp.parseAll(dbs);
    } else if (recoveryStrategy == "aof") {
        recoveryFileIn.open(recoveryFile);
        string command;
        while (getline(recoveryFileIn, command)) {
            execute(interpreter->getCommand(command), self);
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
            for (auto &kv:nameToClients) {
                kv.second->send("HEART_BEAT_TO");
            }
            std::this_thread::sleep_for(chrono::milliseconds(toClientHeartBeatDuration));
        }
    });
    //向从服务器发心跳包
    sendHeartBeatToSlaves = new std::thread([&]() {
        while (true) {
            for (auto &kv:idToPeers) {
                kv.second->send("HEART_BEAT_TO");
            }
            std::this_thread::sleep_for(chrono::milliseconds(toSlaveHeartBeatDuration));
        }
    });
#ifdef WIN32
    selectAndHandle(clientFds);
    std::thread handleSlaves(&MondisServer::selectAndHandle, this, peerFds);
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
        MondisClient *client = new MondisClient(clientSock);
        socketToClient[&client->sock] = client;
        getpeername(clientSock, (sockaddr *) &remoteAddr, &len);
        client->ip = inet_ntoa(remoteAddr.sin_addr);
        client->port = ntohs(remoteAddr.sin_port);
        socketToClient[&client->sock] = client;
        FD_SET(clientSock, &clientFds);
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
        sockaddr_in clientAddr;
        int len = sizeof(clientAddr);
        connect_fd = accept(socket_fd, (struct sockaddr*)clientAddr, &len);
        string auth = read(clientSock);
        MondisClient *client = new MondisClient(clientSock);
        getsockname(connect_fd,(sockaddr*)&clientAddr,&len);
        client->ip = inet_ntoa(clientAddr.sin_addr);
        client->port=ntohs(clientAddr.sin_port);
        fcntl(connect_fd,F_SETFL,O_NONBLOCK);
        fdToClient[connect_fd] = client;
        epoll_ctl(clientFd,EPOLL_CTL_ADD,connect_fd);
    }
#endif
}

void MondisServer::handleCommand(MondisClient *client) {
    string commandStr;
    while ((commandStr = client->readCommand()) != "") {
        ExecutionResult res = execute(commandStr, client);
        if (res.needSend) {
            client->send(res.toString());
        }
    }
}

MondisServer::MondisServer() {
#ifdef WIN32
    FD_ZERO(&clientFds);
    FD_ZERO(&peerFds);
#elif defined(linux)
    clientsEpollFd = epoll_create(1024);
    clientEvents = new epoll_event[1024];
    slavesEpollFd = epoll_create(1024);
    slaveEvents = new epoll_event[1024];
#endif
    replicaCommandBuffer = new deque<string>;
    commandPropagateBuffer = new queue<string>;
    self = new MondisClient(SERVER_SELF);
    self->hasAuthenticate = true;
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
    delete self;
}

void MondisServer::replicaToSlave(MondisClient *client, unsigned long long slaveReplicaOffset) {
    if (replicaOffset - slaveReplicaOffset > maxCommandReplicaBufferSize) {
        const unsigned long long start = replicaOffset;
        string *temp = new string;
        getJson(temp);
        client->send(*temp);
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
    string finished = "SYNC_FINISHED ";
    finished += to_string(client->id);
    client->send(finished);

    string newPeer = "NEW_PEER ";
    newPeer += client->ip;
    newPeer += " ";
    newPeer += to_string(client->port);

    for (auto &kv:idToPeers) {
        if (kv.second != client) {
            kv.second->send(newPeer);
        }
    }
    isPropagating = false;
    propagateCV.notify_all();
}

void MondisServer::singleCommandPropagate() {
    while (true) {
        const string& cur = takeFromPropagateBuffer();
        for (auto &kv:idToPeers) {
            kv.second->send(cur);
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
    ADD(modifyCommands, TO_INTEGER)
    ADD(modifyCommands, CHANGE_SCORE)
    ADD(modifyCommands, SELECT)
    ADD(transactionAboutCommands, DISCARD)
    ADD(transactionAboutCommands, EXEC)
    ADD(transactionAboutCommands, WATCH)
    ADD(transactionAboutCommands, UNWATCH)
    ADD(clientControlCommands, BIND)
    ADD(clientControlCommands, GET)
    ADD(clientControlCommands, LOGIN)
    ADD(clientControlCommands, EXISTS)
    ADD(clientControlCommands, RENAME)
    ADD(clientControlCommands, TYPE)
    ADD(clientControlCommands, SAVE)
    ADD(clientControlCommands, SAVE_ALL)
    ADD(clientControlCommands, EXIT)
    ADD(clientControlCommands, SELECT)
    ADD(clientControlCommands, DEL)
    ADD(clientControlCommands, SLAVE_OF)
    ADD(clientControlCommands, SET_CLIENT_NAME)
    ADD(clientControlCommands, MULTI)
    ADD(clientControlCommands, EXEC)
    ADD(clientControlCommands, DISCARD)
    ADD(clientControlCommands, WATCH)
    ADD(clientControlCommands, UNWATCH)
    ADD(clientControlCommands, GET_MASTER)
    ADD(clientControlCommands, PING)
    ADD(clientControlCommands, PONG)
    ADD(controlCommands, BIND)
    ADD(controlCommands, GET)
    ADD(controlCommands, LOGIN)
    ADD(controlCommands, EXISTS)
    ADD(controlCommands, RENAME)
    ADD(controlCommands, TYPE)
    ADD(controlCommands, SAVE)
    ADD(controlCommands, SAVE_ALL)
    ADD(controlCommands, EXIT)
    ADD(controlCommands, SELECT)
    ADD(controlCommands, DEL)
    ADD(controlCommands, SLAVE_OF)
    ADD(controlCommands, SYNC)
    ADD(controlCommands, SET_CLIENT_NAME)
    ADD(controlCommands, SYNC_FINISHED)
    ADD(controlCommands, DISCONNECT_SLAVE)
    ADD(controlCommands, DISCONNECT_CLIENT)
    ADD(controlCommands, PING)
    ADD(controlCommands, PONG)
    ADD(controlCommands, MULTI)
    ADD(controlCommands, EXEC)
    ADD(controlCommands, DISCARD)
    ADD(controlCommands, WATCH)
    ADD(controlCommands, UNWATCH)
    ADD(controlCommands, GET_MASTER)
    ADD(controlCommands, NEW_PEER)
    ADD(controlCommands, IS_CLIENT)
    ADD(controlCommands, VOTE)
    ADD(controlCommands, UNVOTE)
    ADD(controlCommands, ASK_FOR_VOTE)
    ADD(controlCommands, MASTER_DEAD)
    ADD(controlCommands, I_AM_NEW_MASTER)
    ADD(controlCommands, UPDATE_OFFSET)
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
        nameToClients.erase(nameToClients.find(client->name));
    } else if (client->type == SLAVE) {
        FD_CLR(client->sock, &peerFds);
        idToPeers.erase(idToPeers.find(client->id));
    }
    socketToClient.erase(socketToClient.find(&client->sock));
#elif defined(linux)
    if(client->type == CLIENT) {
        epoll_ctl(clientsEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        nameToClients.erase(nameToClients.find(client->name));
    } else if(client->type == SLAVE) {
        epoll_ctl(slavesEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        idToPeers.erase(idToPeers.find(client->id));
    }
    fdToClient.erase(fdToClient.find(client->fd));
#endif
    delete client;
}

void MondisServer::undoExecute(MultiCommand *command, MondisClient *client) {
    if (command == nullptr) {
        return;
    }
    if (command->locateCommand == nullptr) {
        for (auto m:command->modifies) {
            execute(m, client);
        }
    } else {
        MondisObject *obj = chainLocate(command->locateCommand, client);
        for (auto m:command->modifies) {
            obj->execute(m);
        }
    }
}

MondisObject *MondisServer::chainLocate(Command *command, MondisClient *client) {
    MondisObject *curObj = client->keySpace->locate(command);
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

void MondisServer::saveAll(const string &jsonFile) {
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
}

bool MondisServer::handleWatchedKey(const string &key) {
    if (keyToWatchedClients.find(key) == keyToWatchedClients.end()) {
        return true;
    }
    if (forbidOtherModifyInTransaction) {
        return false;
    }
    unordered_set<MondisClient *> &vc = keyToWatchedClients[key];
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

ExecutionResult MondisServer::transactionExecute(CommandStruct &cstruct, MondisClient *client) {
    ExecutionResult res;
    bool canContinue = true;
    if (cstruct.isModify) {
        canContinue = handleWatchedKey(TO_FULL_KEY_NAME(client->curDbIndex, (*cstruct.operation)[0].content));
    }
    if (canContinue) {
        if (cstruct.isLocate) {
            cstruct.obj->execute(cstruct.operation);
        } else {
            res = execute(cstruct.operation, client);
        }
        return res;
    } else {
        res.res = "other transaction is watching the key ";
        res.res += TO_FULL_KEY_NAME(client->curDbIndex, (*cstruct.operation)[0].content);
        res.res += ",so can undoExecute the command";
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
    }
    res.isModify = isModifyCommand(res.operation);

    return res;
}

ExecutionResult
MondisServer::beSlaveOf(Command *command, MondisClient *client, string &masterUsername, string &masterPassword) {
    ExecutionResult res;
    if (isSlave) {
        sendToMaster(string("DISCONNECT_SLAVE"));
    }
    CHECK_PARAM_NUM(4);
    CHECK_PARAM_TYPE(0, PLAIN)
    string login = "LOGIN ";
    login += masterUsername;
    login += " ";
    login += masterPassword;
    isMaster = false;
    MondisClient *c = buildConnection(PARAM(2), atoi(PARAM(3).c_str()));
    if (c == nullptr) {
        res.res = "can not connect to the master!";
        res.type = INTERNAL_ERROR;
        return res;
    }
    master = c;
    sendToMaster(string("PING"));
    string reply = readFromMaster();
    if (reply != "PONG") {
        res.res = "the socket to master is unavailable";
        res.type = INTERNAL_ERROR;
        return res;
    }
    sendToMaster(login);
    ExecutionResult loginRes = ExecutionResult::stringToResult(readFromMaster());
    if (loginRes.type != OK) {
        res.res = "username or password error";
        LOGIC_ERROR_AND_RETURN
    }
    sendToMaster(string("SYNC ") + to_string(replicaOffset));
    string &&json = readFromMaster();
    for (auto db:dbs) {
        db->clear();
    }
    JSONParser temp(json);
    temp.parseAll(dbs);
    recvFromMaster = new thread([&]() {
        while (true) {
            string next = readFromMaster();
            execute(interpreter->getCommand(next), nullptr);
            putToPropagateBuffer(next);
        }
    });
    if (command != nullptr) {
        OK_AND_RETURN;
    }
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
    res = new MondisClient(masterSock);
    return res;
#elif defined(linux)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons((u_int16_t)port);
    inet_pton(AF_INET,ip.c_str(),&servAddr.sin_addr);
    int masterFd;
    if((masterFd = connect(sockfd, (struct sockaddr*)&serAddr, sizeof(serAddr)))<0) {
        return res;
    }
    master = new MondisClient(masterFd);
    return res;
#endif
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


unordered_set<CommandType> MondisServer::modifyCommands;
unordered_set<CommandType> MondisServer::transactionAboutCommands;
unordered_set<CommandType> MondisServer::clientControlCommands;

unsigned MondisServer::curPeerId = 0;

unsigned MondisServer::curClientId = 0;

unsigned MondisServer::nextPeerId() {
    return ++curPeerId;
}

string MondisServer::nextDefaultClientName() {
    string prefix = "client_";
    return prefix + to_string(++curClientId);
}

void MondisServer::askForVote() {
    this_thread::sleep_for(chrono::milliseconds(abs(rand() % maxVoteIdle)));
    for (auto &kv:idToPeers) {
        kv.second->send("ASK_FOR_VOTE");
    }
}
