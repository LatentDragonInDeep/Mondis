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

#define ADD(SET, TYPE) SET.insert(TYPE);

#define ERROR_EXIT(MESSAGE) cout<<MESSAGE;\
                            exit(1);

#define ADD_AND_RETURN(RES, UNDO) RES->operations.push_back(UNDO);\
                                return RES;
#define TO_FULL_KEY_NAME(DBINDEX, KEY) to_string(DBINDEX)+"_"+KEY

#define PARAM_TYPE_STRING Command::ParamType::STRING
#define PARAM_TYPE_PLAIN Command::ParamType::PLAIN

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
            clientModifyMtx.lock();
            nameToClients.erase(nameToClients.find(client->name));
            nameToClients[PARAM(0)] = client;
            clientModifyMtx.unlock();
            client->name = PARAM(0);
            OK_AND_RETURN
        }
        case SLAVE_OF: {
            CHECK_PARAM_NUM(4)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            CHECK_PARAM_TYPE(2, PLAIN)
            CHECK_PARAM_TYPE(3, PLAIN)
            if (recvFromMaster != nullptr) {
                delete recvFromMaster;
            }
            hasVoteFor = true;
            res = beSlaveOf(command, client, PARAM(2), PARAM(3));
            if (res.type == OK) {
                masterIP = PARAM(0);
                masterPort = atoi(PARAM(1).c_str());
            }
            return res;
        }
        case SYNC: {
            peersModifyMtx.lock_shared();
            if (idToPeers.size() > maxSlaveNum) {
                ExecutionResult res;
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max slave number!";
                client->send(res.toString());
#ifdef WIN32
                FD_CLR(client->sock, &clientFds);
                allModifyMtx.lock();
                socketToClient.erase(socketToClient.find(client->sock));
                allModifyMtx.unlock();
#elif defined(linux)
                epoll_ctl(clientsEpollFd,EPOLL_CTL_DEL,client->fd, nullptr);
                allModifyMtx.lock();
                fdToClient.erase(fdToClient.find(client->fd));
                allModifyMtx.unlock();
#endif
                delete client;
            }
            peersModifyMtx.unlock_shared();
            client->type = PEER;
#ifdef WIN32
            FD_CLR(client->sock, &clientFds);
            FD_SET(client->sock, &peerFds);
#elif defined(linux)
            epoll_ctl(clientsEpollFd,EPOLL_CTL_DEL,client->fd, nullptr);
            epoll_ctl(peersEpollFd,EPOLL_CTL_ADD,client->fd, nullptr);

#endif
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
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
            peersModifyMtx.lock();
            idToPeers[nextPeerId()] = client;
            peersModifyMtx.unlock();
            client->type = PEER;
            std::thread t(&MondisServer::replicaToSlave, this, client, offset);
            OK_AND_RETURN
        }
        case SYNC_FINISHED: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            isReplicatingFromMaster = false;
            res.needSend = false;
            id = atoi(PARAM(0).c_str());
            recvFromMaster = new thread([&]() {
                while (true) {
                    if (isRedirecting) {
                        unique_lock lck(redirectMtx);
                        redirectCV.wait(lck);
                    }
                    string next = readFromMaster(true);
                    if (next == "") {
                        continue;
                    }
                    execute(interpreter->getCommand(next), master);
                    putToPropagateBuffer(next);
                }
            });
            cout << "sync finished!";
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
                if (client->type != PEER) {
                    res.res = "the sender is not a slave!";
                    LOGIC_ERROR_AND_RETURN
                }
                closeClient(client);
                OK_AND_RETURN
            } else {
                sendToMaster("DISCONNECT_SLAVE");
                string resStr = readFromMaster(true);
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
            KEY(0)
            if (client->keySpace->get(key) == nullptr) {
                res.res = "the key ";
                res.res += PARAM(0);
                res.res += " does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            watchedKeyMtx.lock();
            keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].insert(client);
            client->watchedKeys.insert(TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0)));
            watchedKeyMtx.unlock();
            OK_AND_RETURN
        }
        case UNWATCH: {
            if (!client->isInTransaction) {
                res.res = "please start a transaction!";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            watchedKeyMtx.lock();
            keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].erase(
                    keyToWatchedClients[PARAM(0)].find(client));
            if (keyToWatchedClients[TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))].size() == 0) {
                keyToWatchedClients.erase(keyToWatchedClients.find(TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))));
            }
            client->watchedKeys.erase(client->watchedKeys.find(TO_FULL_KEY_NAME(client->curDbIndex, PARAM(0))));
            watchedKeyMtx.unlock();
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
            peersModifyMtx.lock();
            idToPeers[atoi(PARAM(2).c_str())] = client;
            peersModifyMtx.unlock();
            string c = "MASTER_INVITE ";
            c += masterIP;
            c += " ";
            c += masterPort;
            peer->send(c);
        }
        case IS_CLIENT: {
            clientModifyMtx.lock();
            if (nameToClients.size() > maxClientNum) {
                res.type = LOGIC_ERROR;
                res.res = "can not build connection because has up to max client number!";
                client->send(res.toString());
#ifdef WIN32
                FD_CLR(client->sock, &clientFds);
                allModifyMtx.lock();
                socketToClient.erase(socketToClient.find(client->sock));
                allModifyMtx.unlock();
#elif defined(linux)
                epoll_ctl(clientsEpollFd,EPOLL_CTL_DEL,client->fd, nullptr);
                allModifyMtx.lock();
                fdToClient.erase(fdToClient.find(client->fd));
                allModifyMtx.unlock();
#endif
                nameToClients.erase(nameToClients.find(client->name));
                delete client;
            }
            clientModifyMtx.unlock();
            client->type = CLIENT;
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
                epoll_ctl(clientsEpollFd,EPOLL_CTL_DEL,client->fd, nullptr);
                epoll_ctl(peersEpollFd,EPOLL_CTL_ADD,client->fd,&listenEvent);
#endif
                peersModifyMtx.lock();
                idToPeers[atoi(PARAM(2).c_str())] = client;
                peersModifyMtx.unlock();
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
            peersModifyMtx.lock_shared();
            if (voteNum > idToPeers.size() / 2) {
                delete forVote;
                forVote = nullptr;
                isVoting = false;
                peersModifyMtx.lock_shared();
                for (auto &kv:idToPeers) {
                    kv.second->send("I_AM_NEW_MASTER");
                }
                peersModifyMtx.unlock_shared();
            }
            peersModifyMtx.unlock_shared();
            res.needSend = false;
            OK_AND_RETURN
        }
        case UNVOTE: {
            res.needSend = false;
            OK_AND_RETURN
        }
        case MASTER_DEAD: {
            isVoting = true;
            delete master;
            delete recvFromMaster;
            recvFromMaster = nullptr;
            master = nullptr;
            res.needSend = false;
            forVote = new thread(&MondisServer::askForVote, this);
            OK_AND_RETURN;
        }
        case I_AM_NEW_MASTER: {
#ifdef WIN32
            FD_CLR(client->sock, &peerFds);
#elif defined(linux)
            epoll_ctl(clientsEpollFd,EPOLL_CTL_DEL,client->fd, nullptr);
#endif
            isVoting = false;
            hasVoteFor = true;
            delete forVote;
            delete master;
            master = client;
            string sync = "SYNC ";
            sync += to_string(replicaOffset);
            sendToMaster(sync);
        }
        case UPDATE_OFFSET: {
            client->send(to_string(replicaOffset));
            res.needSend = false;
            OK_AND_RETURN
        }
        case CLIENT_INFO: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            clientModifyMtx.lock_shared();
            if (nameToClients.find(PARAM(0)) == nameToClients.end()) {
                res.res = "client whose name is ";
                res.res += PARAM(0);
                res.res += " does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            MondisClient *c = nameToClients[PARAM(0)];
            clientModifyMtx.unlock_shared();
            res.res += "name:";
            res.res += PARAM(0);
            res.res += "\nip:";
            res.res += c->ip;
            res.res += "\nport:";
            res.res += to_string(c->port);
            res.res += "\nhasAuthenticated:";
            res.res += util::to_string(c->hasAuthenticate);
            res.res += "\ndbIndex:";
            res.res += to_string(c->curDbIndex);
            res.res += "\nisIntransaction:";
            res.res += util::to_string(c->isInTransaction);
            res.res += "\n";
            OK_AND_RETURN
        }
        case CLIENT_LIST: {
            CHECK_PARAM_NUM(0)
            res.res = "current server has ";
            clientModifyMtx.lock_shared();
            res.res += to_string(nameToClients.size());
            clientModifyMtx.unlock_shared();
            res.res += " clients,and the following are the list:\n";
            clientModifyMtx.lock_shared();
            for (auto &kv:nameToClients) {
                res.res += kv.first;
                res.res += ",\n";
            }
            clientModifyMtx.unlock_shared();
            OK_AND_RETURN
        }
        case SLAVE_INFO: {
            if (isSlave) {
                res.res = "current server is a slave,can not has slaves";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            peersModifyMtx.lock_shared();
            if (idToPeers.find(atoi(PARAM(0).c_str())) == idToPeers.end()) {
                res.res = "slave whose name is ";
                res.res += PARAM(0);
                res.res += " does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            MondisClient *c = idToPeers[atoi(PARAM(0).c_str())];
            peersModifyMtx.unlock_shared();
            res.res += "id:";
            res.res += PARAM(0);
            res.res += "\nip:";
            res.res += c->ip;
            res.res += "\nport:";
            res.res += to_string(c->port);
            res.res += "\n";
            OK_AND_RETURN
        }
        case SLAVE_LIST: {
            if (isSlave) {
                res.res = "current server is a slave,can not has slaves";
                LOGIC_ERROR_AND_RETURN
            }
            CHECK_PARAM_NUM(0)
            res.res = "current server has ";
            clientModifyMtx.lock_shared();
            res.res += nameToClients.size();
            clientModifyMtx.unlock_shared();
            res.res += " slaves,and the following are the list:\n";
            peersModifyMtx.lock_shared();
            for (auto &kv:idToPeers) {
                res.res += kv.first;
                res.res += ",\n";
            }
            peersModifyMtx.unlock_shared();
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

MultiCommand *MondisServer::getUndoCommand(CommandStruct &cstruct, MondisClient *client) {
    MultiCommand *res = new MultiCommand;
    res->locateCommand = cstruct.locate;

    Command *undo = new Command;
    if (cstruct.obj == nullptr) {
        switch (cstruct.operation->type) {
            case BIND: {
                TOKEY(cstruct.operation, 0);
                MondisObject *original = client->keySpace->get(key);
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
                MondisObject *original = client->keySpace->get(key);
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
                undo->addParam(to_string(client->curDbIndex), PARAM_TYPE_PLAIN);
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
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->addParam(temp.res, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo);
                    }
                    case SET_RANGE: {
                        undo->type = SET_RANGE;
                        assist->type = GET_RANGE;
                        if (cstruct.operation->params.size() == 1) {
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            ExecutionResult temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo)
                        } else if (cstruct.operation->params.size() == 2) {
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(RAW_PARAM(cstruct.operation, 1));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 1));
                            ExecutionResult temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
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
                            ExecutionResult temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
                            ADD_AND_RETURN(res, undo);
                        } else if (cstruct.operation->params.size() == 2) {
                            undo->type = INSERT;
                            assist->type = GET_RANGE;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 0));
                            assist->addParam(RAW_PARAM(cstruct.operation, 1));
                            ExecutionResult temp = cstruct.obj->execute(assist.get());
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
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
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->type = PUSH_FRONT;
                        undo->addParam(temp.res, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case POP_BACK: {
                        assist->type = BACK;
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->type = PUSH_BACK;
                        undo->addParam(temp.res, PARAM_TYPE_STRING);
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
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->type = BIND;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        if (temp.type = OK) {
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
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
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        if (temp.res == "true") {
                            undo->type = ADD;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                }
            }
            case RAW_BIN: {
                switch (cstruct.operation->type) {
                    case BIND: {
                        undo->type = BIND;
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->addParam(temp.res, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo);
                    }
                    case WRITE: {
                        assist->type = GET_POS;
                        ExecutionResult temp1 = cstruct.obj->execute(assist.get());
                        undo->addParam(temp1.res, PARAM_TYPE_PLAIN);
                        res->operations.push_back(undo);
                        assist->type = READ;
                        ExecutionResult temp2 = cstruct.obj->execute(assist.get());
                        int length = atoi(RAW_PARAM(cstruct.operation, 0).content.c_str());
                        Command *undo2 = new Command;
                        undo2->type = WRITE;
                        undo2->addParam(to_string(length < temp2.res.size() ? length : temp2.res.size()),
                                        PARAM_TYPE_PLAIN);
                        undo2->addParam(temp2.res, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo2);
                    }
                    case SET_POS:
                    case READ_CHAR:
                    case READ_SHORT:
                    case READ_INT:
                    case READ_LONG:
                    case READ_LONG_LONG:
                    case FORWARD:
                    case BACKWARD:
                    case READ: {
                        assist->type = GET_POS;
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        undo->type = SET_POS;
                        undo->addParam(temp.res, PARAM_TYPE_PLAIN);
                        ADD_AND_RETURN(res, undo)
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
                        ExecutionResult temp1 = cstruct.obj->execute(assist.get());
                        if (temp1.type != OK) {
                            return res;
                        }
                        assist->type = RANK_TO_SCORE;
                        ExecutionResult temp2 = cstruct.obj->execute(assist.get());
                        undo->type = ADD;
                        undo->addParam(temp2.res, PARAM_TYPE_PLAIN);
                        undo->addParam(temp1.res, PARAM_TYPE_STRING);
                        ADD_AND_RETURN(res, undo)
                    }
                    case REMOVE_BY_SCORE: {
                        assist->type = GET_BY_SCORE;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecutionResult temp1 = cstruct.obj->execute(assist.get());
                        if (temp1.type != OK) {
                            return res;
                        }
                        undo->type = ADD;
                        undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        undo->addParam(temp1.res, PARAM_TYPE_STRING);
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
                        ExecutionResult temp1 = cstruct.obj->execute(assist.get());
                        int rankStart = atoi(temp1.res.c_str());
                        assist->params.clear();
                        SplayTreeNode *lastNode = tree->getLowerBound(scoreEnd, true);
                        assist->type = SCORE_TO_RANK;
                        assist->addParam(to_string(lastNode->score), PARAM_TYPE_PLAIN);
                        ExecutionResult temp2 = cstruct.obj->execute(assist.get());
                        int rankEnd = atoi(temp2.res.c_str());

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
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        if (temp.type == OK) {
                            undo->type = BIND;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
                        } else {
                            undo->type = DEL;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                        }
                        ADD_AND_RETURN(res, undo);
                    }
                    case DEL: {
                        assist->type = GET;
                        assist->addParam(RAW_PARAM(cstruct.operation, 0));
                        ExecutionResult temp = cstruct.obj->execute(assist.get());
                        if (temp.type == OK) {
                            undo->type = BIND;
                            undo->addParam(RAW_PARAM(cstruct.operation, 0));
                            undo->addParam(temp.res, PARAM_TYPE_STRING);
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
    Command *command = interpreter->getCommand(commandStr);
    CommandStruct cstruct = getCommandStruct(command, client);
    ExecutionResult res;
    if (cstruct.operation->type == PONG) {
        res = execute(cstruct.operation, client);
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        return res;
    }
    if (isPropagating) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.res = "is propagating command to a slave,please try later on";
        res.type = INTERNAL_ERROR;
        return res;
    }
    if (isVoting && client->type == CLIENT) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.res = "master is dead,the cluster is voting for new master";
        res.type = INTERNAL_ERROR;
        return res;
    }
    if ((!client->hasAuthenticate) && (cstruct.operation->type != LOGIN)) {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        res.res = "you haven't login,please login";
        LOGIC_ERROR_AND_RETURN
    }
    if (isSlave && cstruct.isModify) {
        if (autoMoveCommandToMaster) {
            Command::destroyCommand(command);
            isRedirecting = true;
            sendToMaster(commandStr);
            string resStr = readFromMaster(true);
            client->send(resStr);
            redirectCV.notify_all();
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
        Command::destroyCommand(cstruct.operation);
        Command::destroyCommand(cstruct.locate);
        client->transactionCommands->push(commandStr);
        OK_AND_RETURN
    }
    if (controlCommands.find(command->type) != controlCommands.end() || cstruct.isLocate) {
        res = transactionExecute(cstruct, client);
    } else if (command->type != LOCATE) {
        res.res = "Invalid command";
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
        out << dbs[dbIndex]->getJson();
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
        } else if(kv.first == "slaveOf") {
            if(kv.second == "true") {
                slaveOf = true;
            } else if(kv.second == "false"){
                slaveOf = false;
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
#ifdef WIN32
    self = new MondisClient(this, (SOCKET) 0);
#elif defined(linux)
    self = new MondisClient(this,0);
    clientsEpollFd = epoll_create(maxClientNum);
    clientEvents = new epoll_event[maxClientNum];
    peersEpollFd = epoll_create(maxSlaveNum);
    peerEvents = new epoll_event[maxSlaveNum];
#endif
    self->type = SERVER_SELF;
    self->hasAuthenticate = true;
    if (aof) {
        aofFileOut.open(workDir + "/" + aofFile, ios::app);
    }
    if (json) {
        jsonFileOut.open(workDir + "/" + jsonFile, ios::app);
    }
    isRecovering = true;
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
    isRecovering = false;
    if (daemonize) {
        runAsDaemon();
    }
    logFileOut.open(workDir + "/" + logFile, ios::app);
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
    //检查超时的客户端，从服务器，主服务器并清理
    std::thread checkAndHandle(&MondisServer::checkAndHandleIdleConnection, this);
    //向客户端发心跳包
    sendHeartBeatToClients = new std::thread([&]() {
        while (true) {
            clientModifyMtx.lock_shared();
            for (auto &kv:nameToClients) {
                kv.second->send("PING");
            }
            clientModifyMtx.unlock_shared();
            std::this_thread::sleep_for(chrono::milliseconds(toClientHeartBeatDuration));
        }
    });
    selectAndHandle(true);
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
        client->name = nextDefaultClientName();
        FD_SET(clientSock, &clientFds);
        allModifyMtx.lock();
        socketToClient[client->sock] = client;
        allModifyMtx.unlock();
        clientModifyMtx.lock();
        nameToClients[client->name] = client;
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
        client->name = nextDefaultClientName();
        int flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd,F_SETFL,flags|O_NONBLOCK);
        int noDelay = 1;
        setsockopt(clientFd, IPPROTO_TCP, FNDELAY, &noDelay, sizeof(noDelay));
        allModifyMtx.lock();
        fdToClient[clientFd] = client;
        allModifyMtx.unlock();
        clientModifyMtx.lock();
        nameToClients[client->name]=client;
        clientModifyMtx.unlock();
        epoll_ctl(clientFd,EPOLL_CTL_ADD,clientFd,&listenEvent);
    }
#endif
}

MondisServer::MondisServer() {
#ifdef WIN32
    FD_ZERO(&clientFds);
    FD_ZERO(&peerFds);
#elif defined(linux)
    listenEvent.events = EPOLLET|EPOLLIN;
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
    delete[] peerEvents;
#endif
    delete replicaCommandBuffer;
    delete commandPropagateBuffer;
    delete recvFromMaster;
    delete sendHeartBeatToClients;
    delete sendHeartBeatToSlaves;
    delete self;
}

void MondisServer::replicaToSlave(MondisClient *client, long long slaveReplicaOffset) {
    if (replicaOffset - slaveReplicaOffset > maxCommandReplicaBufferSize) {
        const long long start = replicaOffset;
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

    peersModifyMtx.lock_shared();
    for (auto &kv:idToPeers) {
        if (kv.second != client) {
            kv.second->send(newPeer);
        }
    }
    peersModifyMtx.unlock_shared();
    isPropagating = false;
    if (sendHeartBeatToSlaves == nullptr) {
        sendHeartBeatToSlaves = new std::thread([&]() {
            while (true) {
                peersModifyMtx.lock_shared();
                for (auto &kv:idToPeers) {
                    kv.second->send("PING");
                }
                peersModifyMtx.unlock_shared();
                std::this_thread::sleep_for(chrono::milliseconds(toSlaveHeartBeatDuration));
            }
        });
    }
    if (recvFromSlaves == nullptr) {
        recvFromSlaves = new std::thread(&MondisServer::selectAndHandle,this,false);
    }
    if (propagateIO == nullptr) {
        propagateIO = new std::thread(&MondisServer::singleCommandPropagate, this);
    }
}

void MondisServer::singleCommandPropagate() {
    while (true) {
        const string& cur = takeFromPropagateBuffer();
        peersModifyMtx.lock_shared();
        for (auto &kv:idToPeers) {
            kv.second->send(cur);
        }
        peersModifyMtx.unlock_shared();
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
    ADD(modifyCommands, INSERT)
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
    ADD(modifyCommands, SET_POS)
    ADD(modifyCommands, READ)
    ADD(modifyCommands, READ_CHAR)
    ADD(modifyCommands, READ_SHORT)
    ADD(modifyCommands, READ_INT)
    ADD(modifyCommands, READ_LONG)
    ADD(modifyCommands, READ_LONG_LONG)
    ADD(modifyCommands, FORWARD)
    ADD(modifyCommands, BACKWARD)
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
    ADD(controlCommands, VOTE)
    ADD(controlCommands, UNVOTE)
    ADD(controlCommands, ASK_FOR_VOTE)
    ADD(controlCommands, MASTER_DEAD)
    ADD(controlCommands, I_AM_NEW_MASTER)
    ADD(controlCommands, UPDATE_OFFSET)
    ADD(controlCommands, CLIENT_INFO)
    ADD(controlCommands, CLIENT_LIST)
    ADD(controlCommands, SLAVE_INFO)
    ADD(controlCommands, SLAVE_LIST)
    ADD(controlCommands, IS_CLIENT)
}

string MondisServer::readFromMaster(bool isBlocking) {
    if (isBlocking) {
        string res;
#ifdef WIN32
        while ((res = read(master->sock)) == "");
#elif defined(linux)
        while((res = read(master->fd))=="");
#endif
        return res;
    }
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

void MondisServer::checkAndHandleIdleConnection() {
    vector<MondisClient *> needDeleted;
    while (true) {
        long long current = chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()).count();
        allModifyMtx.lock_shared();
#ifdef WIN32
        for (auto &kv:socketToClient) {
            MondisClient *c = kv.second;
            if (c->type == CLIENT) {
                if (current - c->preInteraction > maxClientIdle) {
                    needDeleted.push_back(c);
                }
            } else if (c->type == PEER) {
                if (current - c->preInteraction > maxSlaveIdle) {
                    needDeleted.push_back(c);
                }
            }
        }
#elif defined(linux)
        for (auto &kv:fdToClient) {
            MondisClient *c = kv.second;
            if (c->type == CLIENT) {
                if (current - c->preInteraction > toClientHeartBeatDuration) {
                    needDeleted.push_back(c);
                }
            } else if (c->type == PEER) {
                if (current - c->preInteraction > toSlaveHeartBeatDuration) {
                    needDeleted.push_back(c);
                }
            }
        }
#endif
        allModifyMtx.unlock_shared();
        if (isSlave && master != nullptr && current - master->preInteraction > maxMasterIdle) {
            delete master;
            master = nullptr;
            delete recvFromMaster;
            recvFromMaster = nullptr;
            peersModifyMtx.lock_shared();
            for (auto &kv:idToPeers) {
                kv.second->send("MASTER_DEAD");
            }
            peersModifyMtx.unlock_shared();
            Command *command = new Command;
            command->type = MASTER_DEAD;
            execute(command, self);
        }
        for (auto c :needDeleted) {
            closeClient(c);
        }
        needDeleted.clear();
    }
}

void MondisServer::closeClient(MondisClient *client) {
#ifdef WIN32
    if (client->type == CLIENT) {
        FD_CLR(client->sock, &clientFds);
        watchedKeyMtx.lock();
        for (auto &key:client->watchedKeys) {
            keyToWatchedClients[key].erase(keyToWatchedClients[key].find(client));
        }
        watchedKeyMtx.unlock();
        clientModifyMtx.lock();
        nameToClients.erase(nameToClients.find(client->name));
        clientModifyMtx.unlock();
    } else if (client->type == PEER) {
        FD_CLR(client->sock, &peerFds);
        peersModifyMtx.lock();
        idToPeers.erase(idToPeers.find(client->id));
        peersModifyMtx.unlock();
    }
    allModifyMtx.lock();
    socketToClient.erase(socketToClient.find(client->sock));
    allModifyMtx.unlock();
#elif defined(linux)
    if(client->type == CLIENT) {
        epoll_ctl(clientsEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        clientModifyMtx.lock();
        nameToClients.erase(nameToClients.find(client->name));
        clientModifyMtx.unlock();
    } else if(client->type == PEER) {
        epoll_ctl(peersEpollFd, EPOLL_CTL_DEL, client->fd, nullptr);
        peerModifyMtx.lock();
        idToPeers.erase(idToPeers.find(client->id));
        peerModifyMtx.unlock();
    }
    allModifyMtx.lock();
    fdToClient.erase(fdToClient.find(client->fd));
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
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
        return res;
    } else {
        Command::destroyCommand(cstruct.locate);
        Command::destroyCommand(cstruct.operation);
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
    } else {
        res.operation = command;
        res.isLocate = false;
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
    string reply = readFromMaster(true);
    if (reply != "PONG") {
        res.res = "the socket to master is unavailable";
        res.type = INTERNAL_ERROR;
        return res;
    }
    sendToMaster(login);
    ExecutionResult loginRes = ExecutionResult::stringToResult(readFromMaster(true));
    if (loginRes.type != OK) {
        res.res = "username or password error";
        LOGIC_ERROR_AND_RETURN
    }
    sendToMaster(string("SYNC ") + to_string(replicaOffset));
    string &&json = readFromMaster(true);
    for (auto db:dbs) {
        db->clear();
    }
    JSONParser temp(json);
    temp.parseAll(dbs);
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


unordered_set<CommandType> MondisServer::modifyCommands;
unordered_set<CommandType> MondisServer::transactionAboutCommands;
unordered_set<CommandType> MondisServer::clientControlCommands;

unsigned MondisServer::curClientId = 0;

unsigned MondisServer::nextPeerId() {
    static std::uniform_int_distribution<int> dis(0, numeric_limits<int>::max());
    static default_random_engine engine;
    int id = 0;
    peersModifyMtx.lock_shared();
    do {
        id = dis(engine);
    } while (idToPeers.find(id) != idToPeers.end());
    peersModifyMtx.unlock_shared();
    return id;
}

string MondisServer::nextDefaultClientName() {
    string prefix = "client_";
    return prefix + to_string(++curClientId);
}

void MondisServer::askForVote() {
    while (nullptr == master) {
        long long maxOffset = 0;
        peersModifyMtx.lock_shared();
        for (auto &kv:idToPeers) {
            kv.second->send("UPDATE_OFFSET");
            long long cur = atoll(kv.second->read().c_str());
            if (cur > maxOffset) {
                maxOffsetClients.clear();
                maxOffsetClients.insert(kv.second);
            } else if (cur == maxOffset) {
                maxOffsetClients.insert(kv.second);
            }
        }
        this_thread::sleep_for(chrono::milliseconds(abs((int) (rand() % maxVoteIdle))));
        for (auto &kv:idToPeers) {
            kv.second->send("ASK_FOR_VOTE");
        }
        peersModifyMtx.unlock_shared();
    }
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

void MondisServer::selectAndHandle(bool isClient) {
#ifdef WIN32
    timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        fd_set *fds = nullptr;
#elif defined(linux)
    int epollFd = 0;
    epoll_event* events = nullptr;
    if(isClient){
        epollFd = clientsEpollFd;
        events = clientEvents;
    } else{
        epollFd = peersEpollFd;
        events = peerEvents;
    }
#endif
    while (true) {
#ifdef WIN32
        if (isClient) {
            FD_ZERO(&clientFds);
            clientModifyMtx.lock_shared();
            for (auto &kv:nameToClients) {
                FD_SET(kv.second->sock, &clientFds);
            }
            clientModifyMtx.unlock_shared();
            fds = &clientFds;
        } else {
            FD_ZERO(&peerFds);
            peersModifyMtx.lock_shared();
            for (auto &kv:idToPeers) {
                FD_SET(kv.second->sock, &clientFds);
            }
            peersModifyMtx.unlock_shared();
            fds = &peerFds;
        }
        int ret = select(0, fds, nullptr, nullptr, &timeout);
        if (ret <= 0) {
            continue;
        }
        allModifyMtx.lock_shared();
        for (auto pair:socketToClient) {
            if (FD_ISSET(pair.first, fds)) {
                MondisClient *client = pair.second;
                string commandStr = client->read();
                if (commandStr == "CLOSED" || commandStr == "") {
                    continue;
                }
                ExecutionResult res = execute(commandStr, client);
                if (res.needSend) {
                    client->send(res.toString());
                }
            }
        }
        allModifyMtx.unlock_shared();
#elif defined(linux)
        int nfds = epoll_wait(epollFd, events, maxClientNum, 500);
        if(nfds == 0) {
            continue;
        }
        for(int i=0;i<nfds;i++) {
            MondisClient* client = fdToClient[events[i].data.fd];
            if(events[i].events&EPOLLRDHUP){
                needDeleted.push_back(fdToClient[client->fd]);
            } else {
                string commandStr = client->read();
                if (commandStr == "CLOSED" || commandStr == "") {
                    continue;
                }
                ExecutionResult res = execute(commandStr, client);
                if (res.needSend) {
                    client->send(res.toString());
                }
            }
        }
#endif
    }
}
