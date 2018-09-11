//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_COMMANDEXECUTOR_H
#define MONDIS_COMMANDEXECUTOR_H

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#define PUT(TYPE) map[#TYPE] = CommandType::TYPE;

#define MAP(TYPE) typeToStr[TYPE] = #TYPE;

#define INSERT(TYPE) serverCommand.insert(CommandType::TYPE);

#define CHECK_PARAM_NUM(x) if(command->params.size()!=x) {\
                             res.res = "arguments num error";\
                             return res;\
                             }

#define OK_AND_RETURN res.type = OK;\
                       return res;

#define INVALID_AND_RETURN res.res = "Invalid command";\
                             return res;

#define CHECK_INT_LEGAL(DATA, NAME) if(!util::toInteger(DATA,NAME)) {\
                                    res.res = "argument can not be transformed to int";\
                                    return res;\
                                    }

#define CHECK_INT_START_LEGAL(INDEX) int start;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,start)

#define CHECK_INT_END_LEGAL(INDEX) int end;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,end)

#define CHECK_AND_DEFINE_INT_LEGAL(INDEX,NAME) int NAME;\
                                      CHECK_INT_LEGAL((*command)[INDEX].content,NAME)

#define CHECK_START if(start<0) {\
                     res.res = "start under zero!";\
                     return res;\
                     }

#define CHECK_END(size) if(end>(int)size) {\
                     res.res = "end over flow!";\
                     return res;\
                     }

#define CHECK_START_AND_DEFINE(INDEX) CHECK_INT_START_LEGAL(INDEX)\
                             CHECK_START

#define CHECK_END_AND_DEFINE(INDEX,SIZE) CHECK_INT_END_LEGAL(INDEX)\
                             CHECK_END(SIZE)

#define CHECK_PARAM_TYPE(INDEX, TYPE) if((*command)[INDEX].type!=Command::ParamType::TYPE) {\
                                        res.res = "Invalid param";\
                                        return res;\
                                        }

#define KEY(INDEX) Key key((*command)[INDEX].content);

#define CHECK_PARAM_LENGTH(INDEX, LENGTH) if((*command)[INDEX].content.size()!=LENGTH) { \
                                            res.res = "argument length error";\
                                            return res;\
                                            }

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
    SIZE,
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
    SET_POSITION,
    READ,
    WRITE,

    //key space command
//    KEYSPACE_SET,//set
//    KEYSPACE_DEL,//del
//    KEYSPACE_DUMP,//dump
//    KEYSPACE_EXISTS,//exists
//    KEYSPACE_RENAME,//rename
//    KEYSPACE_TYPE,//type
//    KEYSPACE_MOVE,//move

    //list command
//    LIST_PUSHFRONT,//pushfront
//    LIST_PUSHBACK,//pushback
//    LIST_POPFRONT,//popfront
//    LIST_POPBACK,//popback
//    LIST_SET,
//    LIST_GET,
//    LIST_GETRANGE,//getrange

    //BIND
//    SET_ADD,//add
//    SET_REMOVE,//remove
//    SET_EXISTS,
//    SET_SIZE,

    //ZSET
//    ZSET_INSERT,
//    ZSET_REMOVE_BY_RANK,
//    ZSET_REMOVE_BY_SCORE,
//    ZSET_REMOVE_RANGE_BY_RANK,
//    ZSET_REMOVE_RANGE_BY_SCORE,
//    ZSET_EXISTS,
//    ZSET_SIZE,
//    ZSET_COUNT_RANGE_BY_SCORE,
//    ZSET_GET_BY_RANK,
//    ZSET_GET_BY_SCORE,
//    ZSET_GETRANGE_BY_RANK,
//    ZSET_GETRANGE_BY_SCORE,

    //hash
//    HASH_GET,
//    HASH_SET,
//    HASH_DEL,
//    HASH_EXISTS,
//    HASH_SIZE,

    //locate
    LOCATE,

    //persistence
    SAVE,

    //control
    EXIT,
    LOGIN,
    SELECT,


    VACANT,
    ERROR,
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
    Command* next;
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
