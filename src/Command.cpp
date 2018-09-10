//
// Created by 11956 on 2018/9/5.
//

#include "Command.h"

std::unordered_map<CommandType, std::string> Command::typeToStr;

std::unordered_map<std::string, CommandType> CommandInterpreter::map;

CommandInterpreter::CommandInterpreter() {
}

Command *CommandInterpreter::getCommand(std::string &raw) {
    parser = new LexicalParser(raw);
    Command* res = new Command;
    Command* cur = res;
    while (true) {
        Token next = parser->nextToken();
        if(next.type = END) {
            break;
        }
        std::vector<Token> vt;
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
    if (curIndex >= raw.size()) {
        return res;
    }
    skip();
    if (isEnd) {
        return res;
    }
    int start;
    if (raw[curIndex] == '"') {
        if (*raw.end() == '"') {
            start = curIndex;
            curIndex = raw.size();
            res.type = STRING_PARAM;
            res.content = raw.substr(start);
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
        if (raw[curIndex] == ' ') {
            res.type = PLAIN_PARAM;
            res.content = raw.substr(start, curIndex);
            curIndex++;
            return res;
        }
        curIndex++;
    }
}


bool util::toInteger(std::string &data, int &res) {
    static std::stringstream ss;
    ss << data;
    ss >> res;
    if (!ss.fail()) {
        ss.clear();
        return true;
    }
    return false;
}

std::string util::to_string(bool data) {
    if (data) {
        return "true";
    }
    return "false";
}

std::string ExecutionResult::typeToStr[] = {"OK", "SYNTAX_ERROR,INTERNAL_ERROR,LOGIC_ERROR"};
