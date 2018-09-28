//
// Created by 11956 on 2018/9/5.
//

#include <ctime>
#include "MondisClient.h"

MondisClient::MondisClient(int fd) : fd(fd), id(nextId) {
    nextId++;
    ctime = time(nullptr);
}
