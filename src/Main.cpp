//
// Created by 11956 on 2018/9/10.
//
#include "MondisServer.h"

int main(int argc, char **argv) {
    MondisServer *server = new MondisServer;
    string confFile;
    if (argc > 1) {
        confFile = argv[1];
    }
    server->start(confFile);
}

