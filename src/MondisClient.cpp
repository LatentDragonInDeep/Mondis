//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisClient.h"
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
