//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>
#include <stdint-gcc.h>
#include <unordered_set>
#include <sstream>

#include "MondisList.h"
#include "HashMap.h"
#include "MondisBinary.h"
#include "SkipList.h"
#include "Command.h"

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

void handleEscapeChar(string& raw) {
    int modCount = 0;
    for (int i = 0;i<raw.size()+modCount; ++i) {
        if(raw[i]=='"') {
            raw.insert(i,'\\');
            ++i;
            modCount++;
        }
    }
}

class MondisObject
{
public:
    MondisObjectType type = MondisObjectType::EMPTY;
    void * objectData;
    long lastAccessTime;//上次修改时间
    long ttl;//单位秒

    ~MondisObject(){
        switch (type){
            case MondisObjectType ::RAW_STRING:
                delete reinterpret_cast<string*>(objectData);
                break;
            case MondisObjectType ::RAW_INT:
                delete reinterpret_cast<int*>(objectData);
                break
            case RAW_BIN:
            case LIST:
            case SET:
            case ZSET:
            case HASH:
                MondisData* data = dynamic_cast<MondisData*>(objectData);
                delete data;
                break;
        }
        delete json;
    }
private:
    MondisObject * nullObj = new MondisObject;
    bool hasSerialized = false;
    string* json = new string("");
public:
    static string typeStrs[];
    string getTypeStr() {
        return typeStrs[type];
    }
    static MondisObject* getNullObject() {
        return nullObj;
    };
    string* getJson() {
        if(hasSerialized) {
            return json;
        }
        switch (type){
            case MondisObjectType ::RAW_STRING:
                reinterpret_cast<string*>(objectData);
                handleEscapeChar(*objectData);
                *json+=("\""+*objectData+"\"");
                break;
            case MondisObjectType ::RAW_INT:
                reinterpret_cast<int*>(objectData);
                *json+=("\""+std::to_string(*((int*)objectData))+"\"");
                break
            case RAW_BIN:
            case LIST:
            case SET:
            case ZSET:
            case HASH:
                MondisData* data = static_cast<MondisData*>(objectData);
                json = data->getJson();
        }

        hasSerialized = true;
        return json;
    };

    ExecutionResult execute(Command& command) {
        if(type = RAW_STRING) {
            return executeString();
        } else if(type = RAW_INT) {
            return executeInteger(command);
        } else  {
            MondisData* data = (MondisData*)objectData;
            return data->execute(command);
        }
    }

private:
    ExecutionResult executeInteger(Command& command) {
        int * data = (int*)objectData;
        ExecutionResult res;
        switch (command.type) {
            case INCR:
                CHECK_PARAM_NUM(0)
                (*data)++;
                OK_AND_RETURN
            case DECR:
                CHECK_PARAM_NUM(0)
                (*data)--;
                OK_AND_RETURN
            case INCR_BY:
                CHECK_PARAM_NUM(1)
                CHECK_AND_DEFINE_INT_LEGAL(0,delta)
                (*data)+=delta;
                OK_AND_RETURN
            case DECR_BY:
                CHECK_PARAM_NUM(1)
                CHECK_AND_DEFINE_INT_LEGAL(0,delta)
                (*data)-=delta;
                OK_AND_RETURN
        }
        INVALID_AND_RETURN
    };

    ExecutionResult executeString(Command& command) {
        string* data = (string*)objectData;
        ExecutionResult res;
        switch (type) {
            case SET:
            case GET:
                CHECK_PARAM_NUM(2)
                CHECK_AND_DEFINE_INT_LEGAL(0,index)
                if(command.second.size()!=1) {
                    res.res = "error data";
                }
                if(command.type = SET) {
                    (*data)[index] = command[1].content[0];
                } else{
                    res.res = data->substr(index,1);
                }
                OK_AND_RETURN
            case SET_RANGE:
                CHECK_PARAM_NUM(3)
                CHECK_START(0)
                CHECK_END(1,data->size())
                if(command.third.size()!=end-start) {
                    res.res = "data length error!";
                    return res;
                }
                for (int i = start; i < end; ++i) {
                    (*data)[i] = command.third[i-start];
                }
                OK_AND_RETURN
            case GET_RANGE:
                CHECK_PARAM_NUM(2)
                CHECK_START(0)
                CHECK_END(1,data->size())

                res.res = data->substr(start,end-start);

                OK_AND_RETURN
            case STRLEN:
                CHECK_PARAM_NUM(0)
                res.res = std::to_string(data->size());

                OK_AND_RETURN
            case APPEND:
                CHECK_PARAM_NUM(1)
                (*data)+=command.first;

                OK_AND_RETURN
        }
        res.res = "Invalid command";

        return res
    };
public:
    MondisObject* locate(Command* command) {
        if(type == RAW_INT||type == RAW_STRING||type == RAW_BIN) {
            return nullptr;
        }
        MondisData* data = (MondisData*)objectData;
        return data->locate(command);
    }
};

string MondisObject::typeStrs[] = {"RAW_STRING", "RAW_INT", "RAW_BIN", "LIST", "SET", "ZSET", "HASH", "EMPTY"};
#endif //MONDIS_MONDISOBJECT_H
