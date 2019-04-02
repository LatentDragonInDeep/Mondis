//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_COMMANDEXECUTOR_H
#define MONDIS_COMMANDEXECUTOR_H

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#define LOGIC_ERROR_AND_RETURN res.type = LOGIC_ERROR;\
                                return res;

#define CHECK_PARAM_NUM(x) if(command->params.size()!=x) {\
                             res.type = SYNTAX_ERROR;\
                             res.desc = "arguments num error";\
                             LOGIC_ERROR_AND_RETURN\
                             }

#define OK_AND_RETURN res.type = OK;\
                       return res;

#define INVALID_AND_RETURN res.desc = "Invalid command";\
                             return res;

#define CHECK_INT_LEGAL(DATA, NAME) if(!util::toInteger(DATA,NAME)) {\
                                    res.desc = "argument can not be transformed to int";\
                                    LOGIC_ERROR_AND_RETURN\
                                    }

#define CHECK_INT_START_LEGAL(INDEX) int start;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,start)

#define CHECK_INT_END_LEGAL(INDEX) int end;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,end)

#define CHECK_AND_DEFINE_INT_LEGAL(INDEX,NAME) int NAME;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,NAME)

#define CHECK_START if(start<0) {\
                     res.desc = "start under zero!";\
                     LOGIC_ERROR_AND_RETURN\
                     }

#define CHECK_END(size) if(end>(int)size) {\
                     res.desc = "end over flow!";\
                     LOGIC_ERROR_AND_RETURN\
                     }

#define CHECK_START_AND_DEFINE(INDEX) CHECK_INT_START_LEGAL(INDEX)\
                             CHECK_START

#define CHECK_END_AND_DEFINE(INDEX,SIZE) CHECK_INT_END_LEGAL(INDEX)\
                             CHECK_END(SIZE)

#define CHECK_PARAM_TYPE(INDEX, TYPE) if((*command)[INDEX].type!=Command::ParamType::TYPE) {\
                                        res.desc = "Invalid param";\
                                        res.type = SYNTAX_ERROR;\
                                        return res;\
                                        }

#define KEY(INDEX) string key = (*command)[INDEX].content;

#define TOKEY(COMMAND, INDEX) string key = (*COMMAND)[INDEX].content;

#define CHECK_PARAM_LENGTH(INDEX, LENGTH) if((*command)[INDEX].content.size()!=LENGTH) { \
                                            res.res = "argument length error";\
                                            LOGIC_ERROR_AND_RETURN\
                                            }
#define PARAM(INDEX) (*command)[INDEX].content

#define RAW_PARAM(COMMAND, INDEX) (*COMMAND)[INDEX]

namespace util {
    bool toInteger(std::string &data, int &res);

    bool toInteger(std::string &data, long long &res);

    std::string to_string(bool data);

    void toUpperCase(std::string &data);
};
enum CommandType {
    BIND,
    DEL,
    SET_TTL,
    EXISTS,
    RENAME,
    TYPE,
    GET,
    GET_RANGE,
    SET_RANGE,
    REMOVE_RANGE,
    STRLEN,
    INCR,
    DECR,
    INCR_BY,
    DECR_BY,
    INSERT,
    FRONT,
    BACK,
    PUSH_FRONT,
    PUSH_BACK,
    POP_FRONT,
    POP_BACK,
    ADD,
    REMOVE,
    M_SIZE,
    COUNT,
    REMOVE_BY_RANK,
    REMOVE_BY_SCORE,
    REMOVE_RANGE_BY_RANK,
    REMOVE_RANGE_BY_SCORE,
    COUNT_RANGE,
    RANK_TO_SCORE,
    SCORE_TO_RANK,
    GET_BY_RANK,
    GET_BY_SCORE,
    GET_RANGE_BY_RANK,
    GET_RANGE_BY_SCORE,
    TO_STRING,
    TO_INTEGER,
    CHANGE_SCORE,
    LOCATE,
    SAVE,
    SAVE_ALL,
    EXIT,
    LOGIN,
    SELECT,
    VACANT,
    M_ERROR,
    SLAVE_OF,
    SYNC,
    CLIENT_KILL,
    MULTI,
    EXEC,
    DISCARD,
    WATCH,
    UNWATCH,
    MASTER_INFO,
    NEW_PEER,
    NEW_CLIENT,
    ASK_FOR_VOTE,
    VOTE,
    UNVOTE,
    UPDATE_OFFSET,
    MASTER_INVITE,
    MASTER_DEAD,
    I_AM_NEW_MASTER,
    CLIENT_LIST,
};

enum ExecResType {
    OK,
    SYNTAX_ERROR,
    INTERNAL_ERROR,
    LOGIC_ERROR,
};

class ExecRes {
public:
    static std::string typeToStr[];
    static std::unordered_map<std::string, ExecResType> strToType;
    ExecResType type = OK;
    std::string desc;
    bool needReturn = true;
    ExecRes():type(LOGIC_ERROR){};
    std::string toString() const {
        std::string res;
        res+=typeToStr[type];
        res+=" ";
        res+=this->desc;
        return res;
    }

    std::string getTypeStr() {
        return typeToStr[type];
    }

    static ExecRes stringToResult(const std::string &data) {
        ExecRes value;
        int divider = data.find_first_of(" ");
        std::string typeStr = data.substr(0, divider);
        std::string resStr = data.substr(divider + 1);
        value.type = strToType[typeStr];
        value.desc = resStr;
        return value;
    }
};

class Command{
public:
    enum ParamType {
        PLAIN,
        STRING,
        EMPTY,
    };
    class Param {
    public:
        std::string content;
        ParamType type;
    };
    CommandType type = VACANT;
    std::vector<Param> params;
    Command *next = nullptr;
    Command() {
    }

    Param& operator[](int index) {
        return params[index];
    }

    std::string toString() {
        std::string res;
        res+=typeToStr[type];
        res+=" ";
        for(Param& param:params) {
            res+=param.content;
            res+=" ";
        }
        return res;
    }
    static std::unordered_map<CommandType,std::string> typeToStr;

    void addParam(const std::string &param, ParamType t) {
        Param p;
        p.type = t;
        p.content = param;
        params.push_back(p);
    }

    void addParam(Param &p) {
        params.push_back(p);
    }

    static void destroyCommand(Command *command) {
        Command *cur = command;
        while (cur != nullptr) {
            Command *temp = cur;
            cur = cur->next;
            delete temp;
        }
    }
};

class CommandInterpreter {
private:
    enum TokenType {
        PLAIN_PARAM,
        STRING_PARAM,
        VERTICAL,
        END,
    };

    class Token{
    public:
        TokenType type;
        std::string content;
        Token():type(END) {};
    };
    class LexicalParser {
    private:
        std::string raw;
        int curIndex = 0;
        bool isEnd = false;
        void skip();
    public:
        explicit LexicalParser(std::string& raw):raw(raw){

        }
        Token nextToken();

    };
    static std::unordered_map<std::string,CommandType> strToType;
public:
    CommandInterpreter();
    Command* getCommand(std::string& raw);
};

class MultiCommand {
public:
    Command *locateCommand = nullptr;
    std::vector<Command *> operations;

    ~MultiCommand() {
        Command::destroyCommand(locateCommand);
        for (auto c:operations) {
            delete c;
        }
    }
};


#endif //MONDIS_COMMANDEXECUTOR_H
