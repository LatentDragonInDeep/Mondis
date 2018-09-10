//
// Created by 11956 on 2018/9/5.
//

#include "Command.h"
#include <iostream>
#include "MondisServer.h"
#include "JSONParser.h"

ExecutionResult MondisServer::execute(Command& command) {
    ExecutionResult res;
    switch (command.type) {
        case SET:
            CHECK_PARAM_NUM(2);
            CHECK_PARAM_TYPE(0,PLAIN)
            CHECK_PARAM_TYPE(1,STRING)
            curKeySpace->put(*new KEY(0),*parser.parseObject(command[1].content));
            OK_AND_RETURN
        case GET:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            res.res = *curKeySpace->get(KEY(0))->getJson();
            OK_AND_RETURN
        case DEL:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            curKeySpace->remove(KEY(0));
            OK_AND_RETURN
        case EXISTS:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            res.res = curKeySpace->containsKey(KEY(0));
            OK_AND_RETURN
        case TYPE:
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0,PLAIN)
            res.res = curKeySpace->get(KEY(0))->getTypeStr();
            OK_AND_RETURN;
        case EXIT:
            CHECK_PARAM_NUM(0)
            system("exit");
        case SAVE:
        case LOGIN:
    }
    INVALID_AND_RETURN
}

ExecutionResult MondisServer::locateExecute(Command& command) {
    ExecutionResult res;
    MondisObject* curObj = curKeySpace->locate(&command);
    Command* curCommand = command.next;
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

    return curObj->execute(*curCommand);

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
            size_t pos = strLine.find('=');
            string key = strLine.substr(0, pos);
            string value = strLine.substr(pos+1);
            conf[key] = value;
        }
    }
}

int MondisServer::runAsDaemon() {
    return 0;
}

int MondisServer::appendLog(Log &log) {
    logFileOut<<log.toString();
}

int MondisServer::start(string &confFile) {
    //parseConfFile(confFile);
    //applyConf();
    executor = Executor::getExecutor();
    executor->bindServer(this);
    interpreter = new CommandInterpreter;
    startEventLoop();
}

int MondisServer::startEventLoop() {
    while (true) {
        string nextCommand;
        std::cin>>nextCommand;
        execute(nextCommand);
        cout<<endl<<"Mondis>";
    }
}

ExecutionResult MondisServer::execute(string &commandStr) {
    return executor->execute(interpreter->getCommand(commandStr));
}

void MondisServer::appendOnly(Command &command) {
    aofFileOut<<command.toString();
}

ExecutionResult Executor::execute(Command *command) {
    if(serverCommand.find(command->type)!=serverCommand.end()) {
        if(command->next == nullptr) {
            ExecutionResult res = server->execute(command);
            destroyCommand(command);
            return res;
        }
        destroyCommand(command);
        ExecutionResult res;
        res.type = LOGIC_ERROR;
        res.res = "invalid pipeline command";
        return res;
    } else if(command->type = LOCATE) {
        ExecutionResult res = server->locateExecute(command);
        destroyCommand(command);
        return res;
    }
    destroyCommand(command);
    ExecutionResult res;
    res.type = LOGIC_ERROR;
    res.res = "unsuitable command";
    return res;
}

Executor::Executor() {
    if(!hasInit) {
        init();
    }
}

Executor *Executor::getExecutor() {
    return executor;
}

void Executor::init() {
    INSERT(SET)
    INSERT(GET)

    INSERT(EXISTS)
    INSERT(RENAME)
    INSERT(TYPE)

    INSERT(SAVE)
    INSERT(EXIT)
    INSERT(LOGIN)
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