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
        mondis::Message *msg = new mondis::Message;
        msg->set_msg_type(mondis::MsgType::EXEC_RES);
        msg->set_res_type(mondis::ExecResType(res.type));
        msg->set_content(res.toString());
        writeMessage(msg);
        delete msg;
        server->incrReplicaOffset();
        server->putToPropagateBuffer(next);
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
    char * dataLenPtr = (char*)(&len);
    string lenStr(dataLenPtr, sizeof(unsigned));
    data->insert(0,lenStr);
    unsigned hasWrite = 0;
    unsigned writed = 0;
#ifdef WIN32
    while ((writed = send(sock,data->data()+hasWrite,data->length(),0)!=0)) {
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
#ifdef WIN32
        if (nextDataLenBuffer!= nullptr) {
            unsigned recved = 0;
            while ( nextDataLenHasRecv< 4) {
                recved = recv(sock, nextDataLenBuffer+nextDataLenHasRecv, 4 - nextDataLenHasRecv, 0);
                if (recved == 0) {
                    return;
                }
                nextMsgHasRecv += recved;
            }
            nextMessageLen = *((unsigned*)nextDataLenBuffer);
            while ( nextMsgHasRecv< nextMessageLen) {
                recved = recv(sock, halfPacketBuffer+nextMsgHasRecv, nextMessageLen - nextMsgHasRecv, 0);
                if (recved == 0) {
                    return;
                }
                nextMsgHasRecv += recved;
            }
            nextMsg->ParseFromArray(halfPacketBuffer,nextMessageLen);
            recvMsgs.push(nextMsg);
            nextMessageLen = 0;
            nextMsgHasRecv = 0;
            nextDataLenHasRecv = 0;
            delete halfPacketBuffer;
            delete nextDataLenBuffer;
            halfPacketBuffer = nullptr;
            nextDataLenBuffer = nullptr;
            nextMsg = nullptr;
            continue;
        }
        unsigned dataLen;
        char *dataLenPtr = (char *) &dataLen;
        unsigned hasRecv = 0;
        unsigned recved = 0;
        mondis::Message *msg = new mondis::Message;
        while (hasRecv < 4) {
            recved = recv(sock, dataLenPtr, sizeof(unsigned) - hasRecv, 0);
            if (recved == 0) {
                delete msg;
                return;
            }
            hasRecv += recved;
        }
        char *readBuffer = new char[dataLen];
        hasRecv = 0;
        while (hasRecv < dataLen) {
            recved = recv(sock, dataLenPtr, dataLen - hasRecv, 0);
            if (recved == 0) {
                halfPacketBuffer = readBuffer;
                nextMessageLen = dataLen;
                nextMsg = msg;
                nextMsgHasRecv = hasRecv;
                return;
            }
            hasRecv += recved;
        }
#elif defined(linux)
        if (halfPacketBuffer!= nullptr) {
            unsigned hasRecv = 0;
            unsigned recved = 0;
            while ( hasRecv< nextMessageLen) {
                recved = recv(fd, halfPacketBuffer+hasRecv, nextMessageLen - hasRecv, 0);
                hasRecv += recved;
            }
            nextMsg->ParseFromArray(halfPacketBuffer,nextMessageLen);
            recvMsgs.push(nextMsg);
            nextMessageLen = 0;
            delete halfPacketBuffer;
            halfPacketBuffer = nullptr;
            nextMsg = nullptr;
            continue;
        }
        unsigned dataLen;
        char *dataLenPtr = (char *) &dataLen;
        unsigned hasRecv = 0;
        unsigned recved = 0;
        mondis::Message *msg = new mondis::Message;
        while (hasRecv < 4) {
            recved = recv(fd, dataLenPtr, sizeof(unsigned) - hasRecv, 0);
            if (recved == 0&&hasRecv == 0) {
                delete msg;
                return;
            }
            hasRecv += recved;
        }
        char *readBuffer = new char[dataLen];
        hasRecv = 0;
        while (hasRecv < dataLen) {
            recved = recv(fd, dataLenPtr, dataLen - hasRecv, 0);
            if (recved == 0) {
                halfPacketBuffer = readBuffer;
                nextMessageLen = dataLen;
                nextMsg = msg;
                return;
            }
            hasRecv += recved;
        }
#endif
        msg->ParseFromArray(readBuffer, dataLen);
        delete[] readBuffer;
        recvMsgs.push(msg);
    }
}

mondis::Message *MondisClient::nextMessage() {
    readMessage();
    if (recvMsgs.empty()) {
        return nullptr;
    }
    mondis::Message * next = recvMsgs.back();
    recvMsgs.pop();
    return next;
}
