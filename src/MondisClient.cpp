//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisServer.h"
#include <unistd.h>

#ifdef linux
#include <netinet/in.h>
#endif

void MondisClient::send(const string &res) {
    char buffer[4096];
    int ret;
    const char *data = res.data();
    int hasWrite = 0;
    while (hasWrite < res.size()) {
#ifdef WIN32
        ret = ::send(sock, data + hasWrite, res.size() - hasWrite, 0);
#elif defined(linux)
        ret = ::send(fd,data+hasWrite,res.size()-hasWrite,0);
#endif
        hasWrite += ret;
    }

}

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

ExecutionResult MondisClient::commitTransaction(MondisServer *server) {
    ExecutionResult res;
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
    vector<string> aofBuffer;
    while (!transactionCommands->empty()) {
        string next = transactionCommands->front();
        transactionCommands->pop();
        Command *command = server->interpreter->getCommand(next);
        CommandStruct cstruct = server->getCommandStruct(command, this);
        MultiCommand *undo = server->getUndoCommand(cstruct, this);
        ExecutionResult res = server->transactionExecute(cstruct, this);
        if (res.type != OK) {
            while (hasExecutedCommandNumInTransaction > 0) {
                MultiCommand *un = undoCommands->back();
                undoCommands->pop_back();
                server->undoExecute(un, this);
            }
            res.res = "error in executing the command ";
            res.res += next;
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

string MondisClient::read() {
    string res;
    char buffer[4096];
    int ret;
    while (true) {
#ifdef WIN32
        ret = ::recv(sock, buffer, sizeof(buffer), 0);
#elif defined(linux)
        ret = ::recv(fd,buffer, sizeof(buffer),0);
#endif
        if (ret == -1) {
            return res;
        }
        if (ret == 0) {
            return "CLOSED";
        }
        res += string(buffer, ret);
    }
}

#ifdef WIN32

MondisClient::MondisClient(MondisServer *server, SOCKET sock) {
    this->sock = sock;
    keySpace = server->dbs[curDbIndex];
}

#elif defined(linux)
MondisClient::MondisClient(MondisServer* server,int fd){
    this->fd = fd;
    keySpace = server->dbs[curDbIndex];
}
#endif
