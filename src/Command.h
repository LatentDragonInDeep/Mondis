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

#define PUT(TYPE) map[#TYPE] = CommandType::TYPE;

#define MAP(TYPE) typeToStr[TYPE] = #TYPE;

#define INSERT(TYPE) serverCommand.insert(CommandType::TYPE);

#define CHECK_PARAM_NUM(x) if(command->params.size()!=x) {\
                             res.type = SYNTAX_ERROR;\
                             res.res = "arguments num error";\
                             LOGIC_ERROR_AND_RETURN\
                             }

#define OK_AND_RETURN res.type = OK;\
                       return res;

#define INVALID_AND_RETURN res.res = "Invalid command";\
                             return res;

#define CHECK_INT_LEGAL(DATA, NAME) if(!util::toInteger(DATA,NAME)) {\
                                    res.res = "argument can not be transformed to int";\
                                    LOGIC_ERROR_AND_RETURN\
                                    }

#define CHECK_INT_START_LEGAL(INDEX) int start;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,start)

#define CHECK_INT_END_LEGAL(INDEX) int end;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,end)

#define CHECK_AND_DEFINE_INT_LEGAL(INDEX,NAME) int NAME;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,NAME)

#define CHECK_START if(start<0) {\
                     res.res = "start under zero!";\
                     LOGIC_ERROR_AND_RETURN\
                     }

#define CHECK_END(size) if(end>(int)size) {\
                     res.res = "end over flow!";\
                     LOGIC_ERROR_AND_RETURN\
                     }

#define CHECK_START_AND_DEFINE(INDEX) CHECK_INT_START_LEGAL(INDEX)\
                             CHECK_START

#define CHECK_END_AND_DEFINE(INDEX,SIZE) CHECK_INT_END_LEGAL(INDEX)\
                             CHECK_END(SIZE)

#define CHECK_PARAM_TYPE(INDEX, TYPE) if((*command)[INDEX].type!=Command::ParamType::TYPE) {\
                                        res.res = "Invalid param";\
                                        res.type = SYNTAX_ERROR;\
                                        return res;\
                                        }

#define KEY(INDEX) Key key((*command)[INDEX].content);

#define TOKEY(COMMAND, INDEX) Key key((*COMMAND)[INDEX].content);

#define CHECK_PARAM_LENGTH(INDEX, LENGTH) if((*command)[INDEX].content.size()!=LENGTH) { \
                                            res.res = "argument length error";\
                                            LOGIC_ERROR_AND_RETURN\
                                            }
#define PARAM(INDEX) (*command)[INDEX].content

#define RAW_PARAM(COMMAND, INDEX) (*COMMAND)[INDEX]

namespace util {
    bool toInteger(std::string &data, int &res);

    std::string to_string(bool data);

    void toUpperCase(std::string &data);
};
enum CommandType {
    BIND,
    DEL,
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
    APPEND,
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
    GET_BY_RANK,
    GET_BY_SCORE,
    GET_RANGE_BY_RANK,
    GET_RANGE_BY_SCORE,
    PERSIST,
    READ_FROM,
    READ_CHAR,
    READ_SHORT,
    READ_INT,
    READ_LONG,
    READ_LONG_LONG,
    BACK,
    FORWARD,
    SET_POS,
    READ,
    WRITE,
    TO_STRING,
    CHECK_POS,
    CHANGE_SCORE,
    LOCATE,
    SAVE,
    SAVE_ALL,
    EXIT,
    LOGIN,
    SELECT,
    VACANT,
    M_ERROR,
    SET_NAME,
    SLAVE_OF,
    SYNC,
    SYNC_FINISHED,//通知从服务器同步完成
    DISCONNECT_SLAVE,
    DISCONNECT_CLIENT,
    PING,
    PONG,
    UNDO,
    MULTI,
    EXEC,
    DISCARD,
    WATCH,
    UNWATCH,
};

enum ExecutionResultType {
    OK,
    SYNTAX_ERROR,
    INTERNAL_ERROR,
    LOGIC_ERROR,
};

class ExecutionResult {
public:
    static std::string typeToStr[];
    ExecutionResultType type;
    std::string res;
    ExecutionResult():type(LOGIC_ERROR){};
    std::string toString() {
        std::string res;
        res+=typeToStr[type];
        res+=" ";
        res+=this->res;
        return res;
    }

    std::string getTypeStr() {
        return typeToStr[type];
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

    static void init();

    void addParam(const std::string &param, ParamType t) {
        Param p;
        p.type = t;
        p.content = param;
        params.push_back(p);
    }

    void addParam(Param &p) {
        params.push_back(p);
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

    Token pre;
    LexicalParser* parser;
    static std::unordered_map<std::string,CommandType> map;
public:
    static void init();
public:
    CommandInterpreter();
    Command* getCommand(std::string& raw);
};


#endif //MONDIS_COMMANDEXECUTOR_H
