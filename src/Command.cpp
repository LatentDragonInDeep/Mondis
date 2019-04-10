//
// Created by 11956 on 2018/9/5.
//

#include "Command.h"

std::unordered_map<CommandType, std::string> Command::typeToStr = {
        {CommandType::BIND,"BIND"},
        {CommandType::DEL,"DEL"},
        {CommandType::EXISTS,"EXISTS"},
        {CommandType::RENAME,"RENAME"},
        {CommandType::TYPE,"TYPE"},
        {CommandType::GET,"GET"},
        {CommandType::GET_RANGE,"GET_RANGE"},
        {CommandType::SET_RANGE,"SET_RANGE"},
        {CommandType::REMOVE_RANGE,"REMOVE_RANGE"},
        {CommandType::STRLEN,"STRLEN"},
        {CommandType::INCR,"INCR"},
        {CommandType::DECR,"DECR"},
        {CommandType::INCR_BY,"INCR_BY"},
        {CommandType::DECR_BY,"DECR_BY"},
        {CommandType::INSERT,"INSERT"},
        {CommandType::PUSH_FRONT,"PUSH_FRONT"},
        {CommandType::PUSH_BACK,"PUSH_BACK"},
        {CommandType::POP_FRONT,"POP_FRONT"},
        {CommandType::POP_BACK,"POP_BACK"},
        {CommandType::ADD,"ADD"},
        {CommandType::REMOVE,"REMOVE"},
        {CommandType::M_SIZE,"M_SIZE"},
        {CommandType::REMOVE_BY_RANK,"REMOVE_BY_RANK"},
        {CommandType::REMOVE_BY_SCORE,"REMOVE_BY_SCORE"},
        {CommandType::REMOVE_RANGE_BY_RANK,"REMOVE_RANGE_BY_RANK"},
        {CommandType::REMOVE_RANGE_BY_SCORE,"REMOVE_RANGE_BY_SCORE"},
        {CommandType::COUNT_RANGE,"COUNT_RANGE"},
        {CommandType::GET_BY_RANK,"GET_BY_RANK"},
        {CommandType::GET_BY_SCORE,"GET_BY_SCORE"},
        {CommandType::GET_RANGE_BY_RANK,"GET_RANGE_BY_RANK"},
        {CommandType::GET_RANGE_BY_SCORE,"GET_RANGE_BY_SCORE"},
        {CommandType::LOGIN,"LOGIN"},
        {CommandType::EXIT,"EXIT"},
        {CommandType::SAVE,"SAVE"},
        {CommandType::SELECT,"SELECT"},
        {CommandType::LOCATE,"LOCATE"},
        {CommandType::TO_STRING,"TO_STRING"},
        {CommandType::TO_INTEGER,"TO_INTEGER"},
        {CommandType::CHANGE_SCORE,"CHANGE_SCORE"},
        {CommandType::COUNT,"COUNT"},
        {CommandType::SLAVE_OF,"SLAVE_OF"},
        {CommandType::SYNC,"SYNC"},
        {CommandType::CLIENT_KILL,"CLIENT_KILL"},
        {CommandType::MULTI,"MULTI"},
        {CommandType::EXEC,"EXEC"},
        {CommandType::DISCARD,"DISCARD"},
        {CommandType::WATCH,"WATCH"},
        {CommandType::UNWATCH,"UNWATCH"},
        {CommandType::SAVE_ALL,"SAVE_ALL"},
        {CommandType::MASTER_INFO,"MASTER_INFO"},
        {CommandType::MY_IDENTITY,"MY_IDENTITY"},
        {CommandType::ASK_FOR_VOTE,"ASK_FOR_VOTE"},
        {CommandType::VOTE,"VOTE"},
        {CommandType::UNVOTE,"UNVOTE"},
        {CommandType::MASTER_DEAD,"MASTER_DEAD"},
        {CommandType::I_AM_NEW_MASTER,"I_AM_NEW_MASTER"},
        {CommandType::CLIENT_LIST,"CLIENT_LIST"},
        {CommandType::SET_TTL,"SET_TTL"},
        {CommandType::UPDATE_OFFSET,"UPDATE_OFFSET"},
        {CommandType::NEW_PEER,"NEW_PEER"},
        {CommandType::DEL_PEER,"DEL_PEER"}
};


std::unordered_map<std::string, CommandType> CommandInterpreter::strToType = {
        {"SET",CommandType:BIND},
        {"DEL",CommandType::DEL},
        {"EXISTS",CommandType::EXISTS},
        {"RENAME",CommandType::RENAME},
        {"TYPE",CommandType::TYPE},
        {"GET",CommandType::GET},
        {"GET_RANGE",CommandType::GET_RANGE},
        {"SET_RANGE",CommandType::SET_RANGE},
        {"REMOVE_RANGE",CommandType::REMOVE_RANGE},
        {"STRLEN",CommandType::STRLEN},
        {"INCR",CommandType::INCR},
        {"DECR",CommandType::DECR},
        {"INCR_BY",CommandType::INCR_BY},
        {"DECR_BY",CommandType::DECR_BY},
        {"INSERT",CommandType::INSERT},
        {"PUSH_FRONT",CommandType::PUSH_FRONT},
        {"PUSH_BACK",CommandType::PUSH_BACK},
        {"POP_FRONT",CommandType::POP_FRONT},
        {"POP_BACK",CommandType::POP_BACK},
        {"ADD",CommandType::ADD},
        {"REMOVE",CommandType::REMOVE},
        {"SIZE",CommandType::M_SIZE},
        {"REMOVE_BY_RANK",CommandType::REMOVE_BY_RANK},
        {"REMOVE_BY_SCORE",CommandType::REMOVE_BY_SCORE},
        {"REMOVE_RANGE_BY_RANK",CommandType::REMOVE_RANGE_BY_RANK},
        {"REMOVE_RANGE_BY_SCORE",CommandType::REMOVE_RANGE_BY_SCORE},
        {"COUNT_RANGE",CommandType::COUNT_RANGE},
        {"GET_BY_RANK",CommandType::GET_BY_RANK},
        {"GET_BY_SCORE",CommandType::GET_BY_SCORE},
        {"GET_RANGE_BY_RANK",CommandType::GET_RANGE_BY_RANK},
        {"GET_RANGE_BY_SCORE",CommandType::GET_RANGE_BY_SCORE},
        {"LOGIN",CommandType::LOGIN},
        {"EXIT",CommandType::EXIT},
        {"SAVE",CommandType::SAVE},
        {"SELECT",CommandType::SELECT},
        {"LOCATE",CommandType::LOCATE},
        {"TO_STRING",CommandType::TO_STRING},
        {"TO_INTEGER",CommandType::TO_INTEGER},
        {"CHANGE_SCORE",CommandType::CHANGE_SCORE},
        {"COUNT",CommandType::COUNT},
        {"SLAVE_OF",CommandType::SLAVE_OF},
        {"SYNC",CommandType::SYNC},
        {"CLIENT_KILL",CommandType::CLIENT_KILL},
        {"MULTI",CommandType::MULTI},
        {"EXEC",CommandType::EXEC},
        {"DISCARD",CommandType::DISCARD},
        {"WATCH",CommandType::WATCH},
        {"UNWATCH",CommandType::UNWATCH},
        {"SAVE_ALL",CommandType::SAVE_ALL},
        {"MASTER_INFO",CommandType::MASTER_INFO},
        {"MY_IDENTITY",CommandType::MY_IDENTITY},
        {"ASK_FOR_VOTE",CommandType::ASK_FOR_VOTE},
        {"VOTE",CommandType::VOTE},
        {"UNVOTE",CommandType::UNVOTE},
        {"MASTER_DEAD",CommandType::MASTER_DEAD},
        {"I_AM_NEW_MASTER",CommandType::I_AM_NEW_MASTER},
        {"CLIENT_LIST",CommandType::CLIENT_LIST},
        {"SET_TTL",CommandType::SET_TTL},
        {"UPDATE_OFFSET",CommandType::UPDATE_OFFSET},
        {"NEW_PEER",CommandType::NEW_PEER},
        {"DEL_PEER",CommandType::DEL_PEER},
};

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
        for (int i = 0; i <vt.size(); ++i) {
            if (i == 0) {
                if(vt[i].type!=PLAIN_PARAM) {
                    cur->type = M_ERROR;
                    return res;
                }
                util::toUpperCase(vt[i].content);
                if(strToType.find(vt[i].content)==strToType.end()) {
                    cur->type = M_ERROR;
                    return res;
                }
                cur->type = strToType[vt[i].content];
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

bool util::toInteger(std::string &data, long long &res) {
    try {
        res = std::stoll(data, 0, 10);
    } catch (std::invalid_argument &) {
        return false;
    }
    catch (std::out_of_range &) {
        return false;
    }
    return true;
}

std::string ExecRes::typeToStr[] = {"OK", "SYNTAX_ERROR", "INTERNAL_ERROR", "LOGIC_ERROR"};

std::unordered_map<std::string, ExecResType> ExecRes::strToType = {
        {"OK",ExecResType ::OK},
        {"SYNTAX_ERROR",ExecResType ::SYNTAX_ERROR},
        {"INTERNAL_ERROR",ExecResType ::INTERNAL_ERROR},
        {"LOGIC_ERROR",ExecResType ::LOGIC_ERROR}
};
