//
// Created by 11956 on 2018/9/5.
//

#include "Command.h"

std::unordered_map<CommandType, std::string> Command::typeToStr;

std::unordered_map<std::string, CommandType> CommandInterpreter::map;

CommandInterpreter::CommandInterpreter() {
}

Command *CommandInterpreter::getCommand(std::string &raw) {
    LexicalParser parser(raw);
    Command* res = new Command;
    Command* cur = res;
    while (true) {
        Token next = parser.nextToken();
        std::vector<Token> vt;
        if (next.type == END) {
            return res;
        }
        while(next.type!=END&&next.type!=VERTICAL) {
            vt.push_back(next);
            next = parser.nextToken();
        }
        pre = next;
        for (int i = 0; i <vt.size(); ++i) {
            if (i == 0) {
                if(vt[i].type!=PLAIN_PARAM) {
                    cur->type = M_ERROR;
                    return res;
                }
                util::toUpperCase(vt[i].content);
                if(map.find(vt[i].content)==map.end()) {
                    cur->type = M_ERROR;
                    return res;
                }
                cur->type = map[vt[i].content];
                continue;
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
            cur->params.push_back(param);
        }
        cur->next = new Command;
        cur = cur->next;
    }
}

void CommandInterpreter::init() {
    map["SET"] = CommandType::BIND;
    PUT(DEL)
    PUT(EXISTS)
    PUT(RENAME)
    PUT(TYPE)
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
    map["size"] = M_SIZE;
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
    PUT(PERSIST)
    PUT(READ_FROM)
    PUT(READ_CHAR)
    PUT(READ_SHORT)
    PUT(READ_INT)
    PUT(READ_LONG)
    PUT(READ_LONG_LONG)
    PUT(READ)
    PUT(WRITE)
    PUT(BACK)
    PUT(FORWARD)
    PUT(SELECT)
    PUT(LOCATE)
    PUT(TO_STRING)
    PUT(TO_INTEGER)
    PUT(CHECK_POS)
    PUT(CHANGE_SCORE)
    PUT(SET_POS)
    PUT(COUNT)
    PUT(SET_NAME)
    PUT(SLAVE_OF)
    PUT(SYNC)
    PUT(SYNC_FINISHED)
    PUT(DISCONNECT_SLAVE)
    PUT(DISCONNECT_CLIENT)
    PUT(PING)
    PUT(PONG)
    PUT(UNDO)
    PUT(MULTI)
    PUT(EXEC)
    PUT(DISCARD)
    PUT(WATCH)
    PUT(UNWATCH)
    PUT(SAVE_ALL)
}

void CommandInterpreter::LexicalParser::skip() {
    if (curIndex >= raw.size()) {
        isEnd = true;
        return;
    }
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
    if (curIndex >= raw.size()) {
        return res;
    }
    skip();
    if (isEnd) {
        return res;
    }
    int start;
    if (raw[curIndex] == '"') {
        if (raw[raw.size() - 1] == '"') {
            res.type = STRING_PARAM;
            res.content = raw.substr(curIndex + 1, (raw.size() - curIndex - 2));
            isEnd = true;
            return res;
        }
        res.type = END;
        return res;
    } else if (raw[curIndex] == '|') {
        res.type = VERTICAL;
        curIndex++;
        return res;
    }
    start = curIndex;
    while (true) {
        if (curIndex >= raw.size()) {
            res.type = PLAIN_PARAM;
            res.content = raw.substr(start);
            return res;
        }
        if (raw[curIndex] == ' ') {
            res.type = PLAIN_PARAM;
            res.content = raw.substr(start, curIndex - start);
            curIndex++;
            return res;
        } else if (raw[curIndex] == '|') {
            res.type = PLAIN_PARAM;
            res.content = raw.substr(start, curIndex - start);
            return res;
        }
        curIndex++;
    }
}


bool util::toInteger(std::string &data, int &res) {
    try {
        res = std::stoi(data, 0, 10);
    } catch (std::invalid_argument &) {
        return false;
    }
    catch (std::out_of_range &) {
        return false;
    }
    return true;
}

std::string util::to_string(bool data) {
    if (data) {
        return "true";
    }
    return "false";
}

void util::toUpperCase(std::string &data) {
    for (auto &ch:data) {
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
    }
}

std::string ExecutionResult::typeToStr[] = {"OK", "SYNTAX_ERROR", "INTERNAL_ERROR", "LOGIC_ERROR"};

void Command::init() {
    MAP(BIND)
    MAP(DEL)
    MAP(EXISTS)
    MAP(RENAME)
    MAP(TYPE)
    MAP(GET)
    MAP(GET_RANGE)
    MAP(SET_RANGE)
    MAP(REMOVE_RANGE)
    MAP(STRLEN)
    MAP(INCR)
    MAP(DECR)
    MAP(INCR_BY)
    MAP(DECR_BY)
    MAP(APPEND)
    MAP(PUSH_FRONT)
    MAP(PUSH_BACK)
    MAP(POP_FRONT)
    MAP(POP_BACK)
    MAP(ADD)
    MAP(REMOVE)
    MAP(M_SIZE)
    MAP(REMOVE_BY_RANK)
    MAP(REMOVE_BY_SCORE)
    MAP(REMOVE_RANGE_BY_RANK)
    MAP(REMOVE_RANGE_BY_SCORE)
    MAP(COUNT_RANGE)
    MAP(GET_BY_RANK)
    MAP(GET_BY_SCORE)
    MAP(GET_RANGE_BY_RANK)
    MAP(GET_RANGE_BY_SCORE)
    MAP(READ_FROM)
    MAP(READ_CHAR)
    MAP(READ_SHORT)
    MAP(READ_INT)
    MAP(READ_LONG)
    MAP(READ_LONG_LONG)
    MAP(BACK)
    MAP(FORWARD)
    MAP(SET_POS)
    MAP(READ)
    MAP(WRITE)
    MAP(LOCATE)
    MAP(SAVE)
    MAP(EXIT)
    MAP(LOGIN)
    MAP(DISCONNECT_SLAVE)
    MAP(PING)
    MAP(PONG)
    MAP(UNDO)
    MAP(MULTI)
    MAP(EXEC)
    MAP(DISCARD)
    MAP(WATCH)
    MAP(UNWATCH)
}
