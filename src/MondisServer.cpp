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
            curKeySpace->put(*new Key(command[0].content),*parser.parseObject(command[1].content));
            OK_AND_RETURN
        case :

    }
}

ExecutionResult MondisServer::locateExecute(Command& command) {
    return ExecutionResult();
}
