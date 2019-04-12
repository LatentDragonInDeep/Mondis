//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisServer.h"
#include <unistd.h>

#ifdef linux
#include <netinet/in.h>
#endif

MondisClient::~MondisClient() {
#ifdef WIN32
    closesocket(sock);
#elif defined(linux)
    close(fd);
#endif
}

void MondisClient::closeTransaction() {
    isInTransaction = false;
    watchedKeys.clear();
    modifiedKeys.clear();
    delete transactionCommands;
    delete undoCommands;
    transactionCommands = nullptr;
    undoCommands = nullptr;
    watchedKeysHasModified = false;
    hasExecutedCommandNumInTransaction = 0;
}

void MondisClient::startTransaction() {
    isInTransaction = true;
    transactionCommands = new queue<string>;
    undoCommands = new deque<MultiCommand *>;
}

ExecRes MondisClient::commitTransaction(MondisServer *server) {
    ExecRes res;
    if (!isInTransaction) {
        res.desc = "please start a transaction!";
        LOGIC_ERROR_AND_RETURN
    }
    if (watchedKeysHasModified) {
        res.desc = "can not execute the transaction,because the following keys has been hasModified.\n";
        for (auto &key:modifiedKeys) {
            res.desc += key;
            res.desc += " ";
        }
        LOGIC_ERROR_AND_RETURN
    }
    vector<string> aofBuffer;
    while (!transactionCommands->empty()) {
        string next = transactionCommands->front();
        transactionCommands->pop();
        Command *command = server->interpreter->getCommand(next);
        CommandStruct cstruct = server->getCommandStruct(command, this);
        MultiCommand *undo = server->getUndoCommand(cstruct, this);
        ExecRes res = server->transactionExecute(cstruct, this);
        if (res.type != OK) {
            while (hasExecutedCommandNumInTransaction > 0) {
                MultiCommand *un = undoCommands->back();
                undoCommands->pop_back();
                server->undoExecute(un, this);
            }
            res.desc = "error in executing the command ";
            res.desc += next;
            LOGIC_ERROR_AND_RETURN
        }
        if (cstruct.isModify) {
            aofBuffer.push_back(next);
        }
        server->putExecResMsgToWriteQueue(res,id,SendToType::SPECIFY_CLIENT);
        server->incrReplicaOffset();
        server->putCommandMsgToWriteQueue(next, 0, mondis::CommandFrom::MASTER_COMMAND, SendToType::ALL_PEERS, 0);
        undoCommands->push_back(undo);
        hasExecutedCommandNumInTransaction++;
    }
    for (auto &c:aofBuffer) {
        server->appendAof(c);
    }
    closeTransaction();
    OK_AND_RETURN
}

#ifdef WIN32

MondisClient::MondisClient(MondisServer *server, SOCKET sock) {
    this->sock = sock;
}

#elif defined(linux)
MondisClient::MondisClient(MondisServer* server,int fd){
    this->fd = fd;
}
#endif
void MondisClient::writeMessage(mondis::Message *msg) {
    string * data = new string;
    msg->SerializeToString(data);
    unsigned len = data->length();
    char * dataLenPtr = (char*)&len;
    char dataLenBuf[4];
    for (int i = 0; i < 4; ++i) {
        dataLenBuf[i] = dataLenPtr[3-i];
    }
    string lenStr(dataLenBuf, sizeof(unsigned));
    data->insert(0,lenStr);
    unsigned hasWrite = 0;
    unsigned writed = 0;
    unsigned totalLen = data->length();
#ifdef WIN32
    while (hasWrite < totalLen){
        writed = send(sock,data->data()+hasWrite,totalLen,0);
        hasWrite+=writed;
    }
#elif defined(linux)
    while ((writed = send(fd,data->data()+hasWrite,data->length(),0)!=0)) {
        hasWrite+=writed;
    }
#endif
    delete data;
}

void MondisClient::readMessage() {
    while (true) {
        int recved = 0;
#ifdef WIN32
        while (nextDataLenHasRecv < 4) {
            recved = recv(sock, nextDataLenBuffer + nextDataLenHasRecv, 4 - nextDataLenHasRecv, 0);
            if (recved == SOCKET_ERROR) {
                return;
            }
            nextDataLenHasRecv += recved;
        }
        for (int i = 0; i <4; ++i) {
            nextMessageLen |= (((unsigned)nextDataLenBuffer[i])<<(24-i*8));
        }
        if (halfPacketBuffer == nullptr) {
            halfPacketBuffer = new char[nextMessageLen];
            nextMsgHasRecv = 0;
        }
        while (nextMsgHasRecv < nextMessageLen) {
            recved = recv(sock, halfPacketBuffer + nextMsgHasRecv, nextMessageLen - nextMsgHasRecv, 0);
            if (recved == SOCKET_ERROR) {
                return;
            }
            nextMsgHasRecv += recved;
        }
#elif defined(linux)
        if (nextDataLenBuffer != nullptr) {
            while (nextDataLenHasRecv < 4) {
                recved = recv(fd, nextDataLenBuffer + nextDataLenHasRecv, 4 - nextDataLenHasRecv, 0);
                if (recved == 0) {
                    return;
                }
                nextDataLenHasRecv += recved;
            }
            nextMessageLen = *((unsigned *) nextDataLenBuffer);
            nextMsgHasRecv = 0;
        }
        while (nextMsgHasRecv < nextMessageLen) {
            recved = recv(fd, halfPacketBuffer + nextMsgHasRecv, nextMessageLen - nextMsgHasRecv, 0);
            if (recved == 0) {
                return;
            }
            nextMsgHasRecv += recved;
        }
#endif
        mondis::Message *nextMsg = new mondis::Message;
        nextMsg->ParseFromArray(halfPacketBuffer, nextMessageLen);
        if (nextMsg->IsInitialized()) {
            recvMsgs.push(nextMsg);
        }
        nextMessageLen = 0;
        nextMsgHasRecv = 0;
        nextDataLenHasRecv = 0;
        delete [] halfPacketBuffer;
        halfPacketBuffer = nullptr;
    }
}

mondis::Message *MondisClient::nextMessage() {
    readMessage();
    if (recvMsgs.empty()) {
        return nullptr;
    }
    mondis::Message * next = recvMsgs.front();
    recvMsgs.pop();
    return next;
}
