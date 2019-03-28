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

    virtual ExecRes execute(Command *command);

    virtual MondisObject *locate(Command *command);

    bool hasModified();

};

class MondisObject {
public:
    MondisObjectType type = MondisObjectType::EMPTY;
    void *objData;
    ~MondisObject();
private:
    bool hasSerialized = false;
    string json;
    static MondisObject *nullObj;
public:
    static string typeStrs[];

    string getTypeStr();

    static MondisObject *getNullObject();

    string getJson();

    ExecRes execute(Command *command);

private:
    ExecRes executeInteger(Command *command);

    ExecRes executeString(Command *command);
public:
    MondisObject *locate(Command *command);

    bool modified();
};


#endif //MONDIS_MONDISOBJECT_H
