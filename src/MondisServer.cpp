//
// Created by 11956 on 2018/9/5.
//
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>

#include "Command.h"
#include "MondisServer.h"

unordered_set<CommandType> Executor::serverCommand;

JSONParser MondisServer::parser;

ExecutionResult MondisServer::execute(Command *command, MondisClient *client) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, STRING)
            Key *key = new Key((*command)[0].content);
            if(client== nullptr) {
                curKeySpace->put(key, parser.parseObject((*command)[1].content));
            } else {
                client->keySpace->put(key, parser.parseObject((*command)[1].content));
            }
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject* data;
            if(client== nullptr) {
                data = curKeySpace->get(key);
            } else{
                data = client->keySpace->get(key);
            }
            if (data == nullptr) {
                res.res = "the key does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getJson();
            OK_AND_RETURN
        }
        case DEL: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            if(client == nullptr) {
                curKeySpace->remove(key);
            } else {
                client->keySpace->remove(key);
            }
            OK_AND_RETURN
        }
        case EXISTS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            bool r;
            if(client == nullptr) {
                r = curKeySpace->containsKey(key);
            } else {
                r = client->keySpace->containsKey(key);
            }
            res.res = util::to_string(curKeySpace->containsKey(key));
            OK_AND_RETURN
        }
        case TYPE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            KEY(0)
            MondisObject* data;
            if(client == nullptr) {
                data = curKeySpace->get(key);
            } else {
                data = client->keySpace->get(key);
            }
            if (data == nullptr) {
                res.res = "the key does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = data->getTypeStr();
            OK_AND_RETURN;
        }
        case EXIT: {
            CHECK_PARAM_NUM(0)
            if(client == nullptr) {
                system("exit");
            } else {
                fdToClient.erase(fdToClient.find(client->fd));
                delete client;
            }
        }
        case SAVE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            string jsonFile = (*command)[0].content;
            save(jsonFile);
            OK_AND_RETURN
        }
        case LOGIN: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            string userName = (*command)[0].content;
            string pwd = (*command)[1].content;
            if(client == nullptr) {
                if (userName == username && pwd == password) {
                    hasLogin = true;
                    OK_AND_RETURN
                }
                res.res = "username or password error";
                return res;
            } else {
                if (userName == username && pwd == password) {
                    client->hasLogin = true;
                    OK_AND_RETURN
                }
                res.res = "username or password error";
                return res;
            }
        }
        case SELECT: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, index);
            if (index < 0 || index >= dbs.size()) {
                res.res = "Invalid database id";
                LOGIC_ERROR_AND_RETURN
            }
            if(client == nullptr) {
                curDbIndex = index;
                curKeySpace = dbs[curDbIndex];
                OK_AND_RETURN
            } else {
                client->curDbIndex = index;
                client->keySpace = dbs[index];
                OK_AND_RETURN
            }
        }
        case RENAME: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_PARAM_TYPE(1, PLAIN)
            Key *key1 = new Key(PARAM(0));
            Key *key2 = new Key(PARAM(1));
            MondisObject *data = curKeySpace->get(*key1);
            if (data == nullptr) {
                res.res = "the object whose key is" + PARAM(0) + "does not exists";
                LOGIC_ERROR_AND_RETURN
            }
            curKeySpace->put(key1, nullptr);
            curKeySpace->remove(*key1);
            curKeySpace->put(key2, data);
            OK_AND_RETURN

        }
        case SIZE: {
            CHECK_PARAM_NUM(0);
            if(client == nullptr) {
                res.res = to_string(curKeySpace->size());
            } else {
                res.res = to_string(client->keySpace->size());
            }
            OK_AND_RETURN
        }
        case SET_NAME: {
            CHECK_PARAM_NUM(1);
            CHECK_PARAM_TYPE(0,PLAIN)
            if(client == nullptr) {
                res.res = "no known client!";
                LOGIC_ERROR_AND_RETURN;
            }
            client->name = PARAM(0);
            OK_AND_RETURN
        }
    }
    INVALID_AND_RETURN
}

ExecutionResult MondisServer::locateExecute(Command *command) {
    ExecutionResult res;
    MondisObject *curObj = curKeySpace->locate(command);
    Command *curCommand = command->next;
    while (true) {
        if(curObj == nullptr) {
            res.type = LOGIC_ERROR;
            res.res = "Error on locating";
            return res;
        }
        if(curCommand->type!=LOCATE) {
            break;
        }
        curObj = curObj->locate(curCommand);
        curCommand = curCommand->next;
    }

    return curObj->execute(curCommand);

}

JSONParser *MondisServer::getJSONParser() {
    return &parser;
}

void MondisServer::parseConfFile(string &confFile) {
    ifstream configFile;
    configFile.open(confFile.c_str());
    string strLine;
    string filepath;
    if(configFile.is_open())
    {
        while (!configFile.eof())
        {
            getline(configFile, strLine);
            if (strLine[0] == '#' || strLine == "") {
                continue;
            }
            size_t pos = strLine.find('=');
            string key = strLine.substr(0, pos);
            string value = strLine.substr(pos+1);
            conf[key] = value;
        }
    }
}

int MondisServer::runAsDaemon() {
    //TODO
    return 0;
}

int MondisServer::appendLog(Log &log) {
    logFileOut<<log.toString();
}

int MondisServer::start(string &confFile) {
    configfile = confFile;
    parseConfFile(confFile);
    applyConf();
    init();
    startEventLoop();
}

int MondisServer::startEventLoop() {
    while (true) {
        cout << username + "@Mondis>";
        string nextCommand;
        getline(std::cin, nextCommand);
        ExecutionResult res = execute(nextCommand, nullptr);
        cout << res.toString();
        cout << endl;
    }
}

ExecutionResult MondisServer::execute(string &commandStr, MondisClient *client) {
    if (!hasLogin) {
        ExecutionResult e;
        e.res = "you haven't login,please login";
        return e;
    }
    ExecutionResult res = executor->execute(interpreter->getCommand(commandStr),client);
    if (res.type == OK) {
        if (aof) {
            aofFileOut << commandStr + "\n";
            if (aofSyncStrategy == 1) {
                clock_t cur = clock();
                if (cur - preSync >= 1000) {
                    aofFileOut.flush();
                    preSync = cur;
                }
            } else if (aofSyncStrategy == 2) {
                aofFileOut.flush();
            }
        }
    }
    Log log(commandStr, res);
    logFileOut << log.toString();
    logFileOut.flush();

    return res;
}

int MondisServer::save(string &jsonFile) {
    ofstream out(jsonFile + "2");
    out << curKeySpace->getJson();
    out.flush();
    remove(jsonFile.c_str());
    out.close();
    rename((jsonFile + "2").c_str(), jsonFile.c_str());
}

void MondisServer::applyConf() {
    for (auto &kv:conf) {
        if (kv.first == "deamonize") {
            if (kv.second == "true") {
                daemonize = true;
            } else if (kv.second == "false") {
                daemonize = false;
            }
        } else if (kv.first == "port") {
            util::toInteger(kv.second, port);
        } else if (kv.first == "username") {
            username = kv.second;
        } else if (kv.first == "password") {
            password = kv.second;
        } else if (kv.first == "databaseNum") {
            util::toInteger(kv.second, databaseNum);
        } else if (kv.first == "aof") {
            if (kv.second == "true") {
                aof = true;
            } else if (kv.second == "false") {
                aof = false;
            }
        } else if (kv.first == "databaseID") {
            util::toInteger(kv.second, curDbIndex);
        } else if (kv.first == "aofSyncStrategy") {
            aofSyncStrategy = atoi(kv.second.c_str());
        } else if (kv.first == "json") {
            if (kv.second == "true") {
                json = true;
            } else if (kv.second == "false") {
                json = false;
            }
        } else if (kv.first == "jsonDuration") {
            util::toInteger(kv.second, jsonDuration);
        } else if (kv.first == "slaveOf") {
            slaveof = kv.second;
        } else if (kv.first == "workDir") {
            workDir = kv.second;
        } else if (kv.first == "logFile") {
            logFile = kv.second;
        } else if (kv.first == "aofFile") {
            aofFile = kv.second;
        } else if (kv.first == "jsonFile") {
            jsonFile = kv.second;
        } else if (kv.first == "recovery") {
            recoveryStrategy = kv.second;
        } else if (kv.first == "recoveryFile") {
            recoveryFile = kv.second;
        }
    }
}

void MondisServer::init() {
    isLoading = true;
    Executor::init();
    Executor::bindServer(this);
    executor = Executor::getExecutor();
    JSONParser::LexicalParser::init();
    Command::init();
    CommandInterpreter::init();
    interpreter = new CommandInterpreter;
    for (int i = 0; i < databaseNum; ++i) {
        dbs.push_back(new HashMap(16, 0.75f, false, false));
    }
    curKeySpace = dbs[curDbIndex];
    if (aof) {
        aofFileOut.open(aofFile, ios::app);
    }
    if (json) {
        jsonFileOut.open(jsonFile, ios::app);
    }
    isLoading = false;
    isRecovering = true;
    if (recoveryStrategy == "json") {
        JSONParser temp(recoveryFile);
        temp.parse(curKeySpace);
    } else if (recoveryStrategy == "aof") {
        recoveryFileIn.open(recoveryFile);
        string command;
        while (getline(recoveryFileIn, command)) {
            execute(command, nullptr);
        }
    }
    isRecovering = false;
    if (daemonize) {
        runAsDaemon();
    }
    logFileOut.open(logFile, ios::app);
    std::thread t(&MondisServer::acceptClient, this);
}

void MondisServer::acceptClient() {
    int socket_fd;
    int connect_fd;
    struct sockaddr_in servaddr;
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    bind(socket_fd,&servaddr, sizeof(servaddr));
    listen(socket_fd,10);
    while (true) {
        connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL));
        MondisClient* client = new MondisClient(connect_fd);
        fdToClient[connect_fd] = client;
    }
}

void MondisServer::handle(MondisClient *client) {
    string commandStr = client->readCommand();
    ExecutionResult res = execute(commandStr, nullptr);
    client->sendResult(res.toString());
}

ExecutionResult Executor::execute(Command *command) {
    if(serverCommand.find(command->type)!=serverCommand.end()) {
        if (command->next == nullptr || command->next->type == VACANT) {
            ExecutionResult res = server->execute(command, nullptr);
            destroyCommand(command);
            return res;
        }
        destroyCommand(command);
        ExecutionResult res;
        res.type = LOGIC_ERROR;
        res.res = "invalid pipeline command";
        return res;
    } else if (command->type == LOCATE) {
        ExecutionResult res = server->locateExecute(command);
        destroyCommand(command);
        return res;
    }
    destroyCommand(command);
    ExecutionResult res;
    res.type = LOGIC_ERROR;
    res.res = "Invalid command";
    return res;
}

Executor::Executor() {
}

MondisServer *Executor::server = nullptr;

Executor *Executor::getExecutor() {
    return executor;
}

void Executor::init() {
    INSERT(BIND)
    INSERT(GET)
    INSERT(LOGIN)
    INSERT(EXISTS)
    INSERT(RENAME)
    INSERT(TYPE)
    INSERT(SAVE)
    INSERT(EXIT)
    INSERT(SELECT)
    INSERT(DEL)
}

void Executor::destroyCommand(Command *command) {
    if(command == nullptr) {
        return;
    }
    Command* cur = command;
    while (cur!= nullptr) {
        Command* temp = cur;
        cur = cur->next;
        delete temp;
    }

}

void Executor::bindServer(MondisServer *sv) {
    server = sv;
}

Executor *Executor::executor = new Executor;