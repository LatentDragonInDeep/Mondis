//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_COMMANDEXECUTOR_H
#define MONDIS_COMMANDEXECUTOR_H

#include <string>

#include "MondisServer.h"

enum CommandType {
    //key space command
    KEYSPACE_SET,
    KEYSPACE_DEL,
    KEYSPACE_DUMP,
    KEYSPACE_EXISTS,
    KEYSPACE_RENAME,
    KEYSPACE_TYPE,
    KEYSPACE_MOVE,

    //string command
    STRING_SET,
    STRING_GET,
    STRING_GETRANGE,
    STRING_GETSET,
    STRING_STRLEN,
    STRING_INCR,
    STRING_DECR,
    STRING_INCR_BY,
    STRING_DECR_BY,

    //list command
    LIST_PUSHFRONT,
    LIST_PUSHBACK,
    LIST_POPFRONT,
    LIST_POPBACK,
    LIST_SET,
    LIST_GET,
    LIST_GETRANGE,

    //SET
    SET_ADD,
    SET_REMOVE,
    SET_EXISTS,
    SET_SIZE,

    //ZSET
    ZSET_ADD,
    ZSET_REMOVE_BY_RANK,
    ZSET_REMOVE_BY_SCORE,
    ZSET_REMOVE_RANGE_BY_RANK,
    ZSET_REMOVE_RANGE_BY_SCORE,
    ZSET_EXISTS,
    ZSET_SIZE,
    ZSET_COUNT_RANGE_BY_RANK,
    ZSET_COUNT_RANGE_BY_SCORE,
    ZSET_GET_BY_RANK,
    ZSET_GET_BY_SCORE,
    ZSET_GETRANGE_BY_RANK,
    ZSET_GETRANGE_BY_SCORE,

    //hash
    HASH_GET,
    HASH_SET,
    HASH_DEL,
    HASH_EXISTS,
    HASH_SIZE,

    //locate
    LOCATE,

    //persistence
    SAVE,
    BGSAVE,
    EXIT,
    MULTI,
    DISCARD,
    EXEC,
    WATCH,

};

enum ExecutionResultType {
    OK,
    SYNTAX_ERROR,
    INTERNAL_ERROR,
};

class ExecutionResult {
public:
    ExecutionResultType type;
    std::string* res;
};

class CommandExecutor;

class Command{
public:
    CommandType type;
    std::string raw;
    ExecutionResult execute(CommandExecutor * executor) {
        return executor->execute(this);
    }
};

class CommandExecutor {
public:

    ExecutionResult execute(Command* command);
    static CommandExecutor* getExecutor();

private:
    CommandExecutor();
    CommandExecutor(CommandExecutor&) = default;
    CommandExecutor(CommandExecutor&&) = default;
    CommandExecutor&operator=(CommandExecutor&) = default;
    CommandExecutor&operator=(CommandExecutor&&) = default;
    static CommandExecutor* executor;
    MondisServer* server;

};

CommandExecutor* CommandExecutor::executor = new CommandExecutor;


#endif //MONDIS_COMMANDEXECUTOR_H
