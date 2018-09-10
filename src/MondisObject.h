//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>
#include <stdint-gcc.h>
#include <unordered_set>
#include <sstream>

#include "Command.h"
#include "JSONParser.h"

using namespace std;
enum MondisObjectType {
    RAW_STRING = 0,
    RAW_INT,
    RAW_BIN,
    LIST,
    SET,
    ZSET,
    HASH,
    EMPTY,
};

void handleEscapeChar(string &raw);

class MondisObject;

class MondisData {
protected:
    string json;
    bool hasSerialized = false;

    virtual void toJson() = 0;

public:
    string getJson();

    virtual ~MondisData();;

    virtual ExecutionResult execute(Command *command);;

    virtual MondisObject *locate(Command *command);;

};

class MondisObject {
public:
    MondisObjectType type = MondisObjectType::EMPTY;
    void * objectData;

    ~MondisObject();
private:
    bool hasSerialized = false;
    string json;
    static MondisObject *nullObj;
public:
    static string typeStrs[];

    string getTypeStr();

    static MondisObject *getNullObject();;

    string getJson();;

    ExecutionResult execute(Command *command);

private:
    ExecutionResult executeInteger(Command *command);;

    ExecutionResult executeString(Command *command);;
public:
    MondisObject *locate(Command *command);
};


#endif //MONDIS_MONDISOBJECT_H
