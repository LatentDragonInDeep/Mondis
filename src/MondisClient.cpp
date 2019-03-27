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

void MondisClient::updateHeartBeatTime() {
    preInteraction = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
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
        res.desc = "can not execute the transaction,because the following keys has been modified.\n";
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
        send(res.toString());
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
#if defined(linux)
    msg->SerializePartialToFileDescriptor(fd);
#elif defined(WIN32)
#endif
}

mondis::Message *MondisClient::nextMessage() {
#if defined(linux)
    mondis::Message* msg = new mondis::Message;
    msg->ParseFromFileDescriptor(fd);
    return msg;
#elif defined(WIN32)
#endif
}
