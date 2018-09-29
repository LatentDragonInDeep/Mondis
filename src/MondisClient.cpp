//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisClient.h"
#include <boost/algorithm/string.hpp>

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
    boost::split(commandBuffer, commandStr, boost::is_any_of("\r\n\r\n\r\n"));
    return commandBuffer[curCommandIndex];
}

void MondisClient::sendResult(const string &res) {
    char buffer[4096];
    int ret;
    char *data = res.data();
    int hasWrite = 0;
#ifdef WIN32
    while (hasWrite < res.size()) {
        ret = send(sock, data + hasWrite, res.size() - hasWrite);
        hasWrite += ret;
    }
#elif defined(linux)
    while (hasWrite<res.size()) {
        ret = write(fd,data+hasWrite,res.size()-hasWrite);
        hasWrite+=ret;
    }
#endif
}
