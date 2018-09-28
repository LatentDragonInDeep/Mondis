//
// Created by 11956 on 2018/9/10.
//
#include "MondisServer.h"
int main() {
    MondisServer *server = new MondisServer;
    string confFile = "mondis.conf";
    server->start(confFile);
}

