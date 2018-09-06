//
// Created by 11956 on 2018/9/5.
//

#include "Command.h"

ExecutionResult Executor::execute(Command *command) {
    if(serverCommand.find(command->type)!=serverCommand.end()) {
        if(command->next == nullptr) {
            return server->execute(command);
        }
        ExecutionResult res;
        res.type = LOGIC_ERROR;
        res.res = "invalid pipeline command";
        return res;
    } else if(command->type = LOCATE) {
        return server->locateExecute(command);
    }
    ExecutionResult res;
    res.type = LOGIC_ERROR;
    res.res = "unsuitable command";
    return res;
}

void CommandInterpreter::init() {
    PUT(SET)
    PUT(GET)
    PUT(DUMP)
    PUT(EXISTS)
    PUT(RENAME)
    PUT(TYPE)
    PUT(MOVE)
    PUT(GET)
    PUT(GET_RANGE)
    PUT(SET_RANGE)
    PUT(REMOVE_RANGE)
    PUT(STRLEN)
    PUT(INCR)
    PUT(DECR)
    PUT(INCR_BY)
    PUT(DECR_BY)
    PUT(APPEND)
    PUT(PUSH_FRONT)
    PUT(PUSH_BACK)
    PUT(POP_FRONT)
    PUT(POP_BACK)
    PUT(ADD)
    PUT(REMOVE)
    PUT(SIZE)
    PUT(REMOVE_BY_RANK)
    PUT(REMOVE_BY_SCORE)
    PUT(REMOVE_RANGE_BY_RANK)
    PUT(REMOVE_RANGE_BY_SCORE)
    PUT(COUNT_RANGE)
    PUT(GET_BY_RANK)
    PUT(GET_BY_SCORE)
    PUT(GET_RANGE_BY_RANK)
    PUT(GET_RANGE_BY_SCORE)
    PUT(LOGIN)
    PUT(EXIT)
    PUT(SAVE)
    PUT(BGSAVE)
}

CommandInterpreter::CommandInterpreter() {
    if(!hasInit) {
        init();
    }
}

Command *CommandInterpreter::getCommand(string &raw) {
    parser = new LexicalParser(raw);
    Command* res = new Command;
    Command* cur = res;
    while (true) {
        Token next = parser->nextToken();
        if(next.type = END) {
            break;
        }
        vector<Token> vt;
        while(next.type!=END&&next.type!=VERTICAL) {
            vt.push_back(next);
        }
        pre = next;
        for (int i = 0; i <vt.size(); ++i) {
            if(i == 1) {
                if(vt[i].type!=PLAIN_PARAM) {
                    cur->type = ERROR;
                    return res;
                }
                if(map.find(vt[i].content)==map.end()) {
                    cur->type = ERROR;
                    return res;
                }
                cur->type = map[vt[i].content];
                continue;
            }
            else if(i>=5) {
                cur->type = ERROR;
                return res;
            }
            Command::Param param;
            switch (vt[i].type) {
                case PLAIN_PARAM:
                    param.type = Command::ParamType::PLAIN;
                    break;
                case STRING_PARAM:
                    param.type = Command::ParamType::STRING;
                    break;
            }
            param.content = vt[i].content;
            res->params.push_back(param);
        }
        cur->next = new Command;
        cur = cur->next;
    }
    delete parser;
}

void CommandInterpreter::LexicalParser::skip() {
    while (raw[curIndex] ==' '||raw[curIndex]=='\n'||raw[curIndex] == '\r'||raw[curIndex] == '\t') {
        curIndex++;
        if(curIndex>=raw.size()) {
            isEnd = true;
            break;
        }
    }
}

CommandInterpreter::Token CommandInterpreter::LexicalParser::nextToken() {
    Token res;
    if(curIndex>=raw.size()) {
        return res;
    }
    skip();
    if(isEnd) {
        return res;
    }
    int start;
    if(raw[curIndex] == '"') {
        start = curIndex;
        curIndex++;
        while (true) {
            if(raw[curIndex]=='"'&&raw[curIndex-1]!='\\') {
                res.type = STRING_PARAM;
                res.content = raw.substr(start+1,curIndex);
                curIndex++;
                return res;
            }
            curIndex++;
        }
    } else if(raw[curIndex] == '|') {
        res.type = VERTICAL;
        curIndex++;
        return res;
    }
    start = curIndex;
    while (true) {
        if(raw[curIndex]==' ') {
            res.type = PLAIN_PARAM;
            res.content = raw.substr(start,curIndex);
            curIndex++;
            return res;
        }
        curIndex++;
    }

}

Executor::Executor() {
    if(!hasInit) {
        init();
    }
}

void Executor::init() {
    INSERT(SET)
    INSERT(GET)
    INSERT(DUMP)
    INSERT(EXISTS)
    INSERT(RENAME)
    INSERT(TYPE)
    INSERT(MOVE)

    INSERT(SAVE)
    INSERT(BGSAVE)
    INSERT(EXIT)
    INSERT(LOGIN)
}
