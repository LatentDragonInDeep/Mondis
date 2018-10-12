//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisServer.h"
#include <unistd.h>

#ifdef WIN32

MondisClient::MondisClient(SOCKET sock) : sock(sock) {
    nextId++;
    ctime = time(nullptr);
}
#elif defined(linux)
MondisClient::MondisClient(int fd) : fd(fd), id(nextId) {
    nextId++;
    ctime = time(nullptr);
}

#endif

string MondisClient::readCommand() {
    if (curCommandIndex + 1 < commandBuffer.size()) {
        return commandBuffer[++curCommandIndex];
    }
    string commandStr;
    char buffer[4096];
    int ret;
#ifdef WIN32
    while ((ret = recv(sock, buffer, sizeof(buffer), 0)) != 0) {
        commandStr += string(buffer, ret);
    }
#elif defined(linux)
    while ((ret = read(fd, buffer, sizeof(buffer))) != 0) {
        commandStr += string(buffer, ret);
    }
#endif
    commandBuffer.clear();
    curCommandIndex = 0;
    int searchStart = 0;
    int occurrence = 0;

    while ((occurrence=commandStr.find("\r\n\r\n\r\n",searchStart))!=string::npos) {
        commandBuffer.push_back(commandStr.substr(searchStart,occurrence-searchStart));
        searchStart = occurrence+6;
    }
    commandBuffer.push_back(commandStr.substr(searchStart,commandStr.size()-searchStart));
    return commandBuffer[curCommandIndex];
}

void MondisClient::send(const string &res) {
    char buffer[4096];
    int ret;
    const char *data = res.data();
    int hasWrite = 0;
#ifdef WIN32
    while (hasWrite < res.size()) {
        ret = ::send(sock, data + hasWrite, res.size() - hasWrite, 0);
        hasWrite += ret;
    }
#elif defined(linux)
    while (hasWrite<res.size()) {
        ret = write(fd,data+hasWrite,res.size()-hasWrite);
        hasWrite+=ret;
    }
#endif
}

MondisClient::~MondisClient() {
#ifdef WIN32
    closesocket(sock);
#elif defined(linux)
    close(fd);
#endif
}

int MondisClient::nextId = 1;

void MondisClient::updateHeartBeatTime() {
    preInteraction = clock();
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
    if (isInTransaction) {
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
        ExecutionResult res;
        string next = transactionCommands->front();
        transactionCommands->pop();
        Command *c = server->interpreter->getCommand(next);
        Command *modify = nullptr;
        MondisObject *obj = nullptr;
        if (c->type == LOCATE) {
            Command *last = c;
            while (last->next->type == LOCATE) {
                last = last->next;
            }
            modify = last->next;
            obj = server->chainLocate(c);
            obj->execute(modify);
        } else {
            res = server->execute(c, this);
        }
        bool isModify = MondisServer::isModifyCommand(modify);
        server->appendLog(next, res);
        if (res.type != OK) {
            string undo = "UNDO";
            while (hasExecutedCommandNumInTransaction > 0) {
                server->execute(server->interpreter->getCommand(undo), this);
            }
            res.res = "error in executing the command ";
            res.res += next;
            LOGIC_ERROR_AND_RETURN
        }
        if (isModify) {
            aofBuffer.push_back(next);
        }
        send(res.toString());
        server->incrReplicaOffset();
        server->putToPropagateBuffer(next);
        hasExecutedCommandNumInTransaction++;
        server->handleWatchedKey((*c)[0].content);
    }
    for (auto &c:aofBuffer) {
        server->appendAof(c);
    }
    closeTransaction();
}
