//
// Created by 11956 on 2018/9/5.
//

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
            //TODO
        case SAVE:
        case LOGIN:
    }
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
