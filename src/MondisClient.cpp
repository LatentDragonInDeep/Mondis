//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisClient.h"

#ifdef WIN32

MondisClient::MondisClient(SOCKET sock) : sock(sock) {

}
#elif defined(linux)
MondisClient::MondisClient(int fd) : fd(fd), id(nextId) {
    nextId++;
    ctime = time(nullptr);
}

#endif

string MondisClient::readCommand() {
    return std::__cxx11::string();
}

void MondisClient::sendResult(const string &res) {

}
