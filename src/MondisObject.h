//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>
#include <stdint-gcc.h>
#include <unordered_set>

#include "MondisList.h"
#include "HashMap.h"
#include "MondisBinary.h"
#include "SkipList.h"

using namespace std;
enum MondisObjectType {
    RAW_STRING,
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
        delete nullObj;
    }
private:
    MondisObject * nullObj = new MondisObject;
    bool hasSerialized = false;
    string* json = new string("");
public:
    static MondisObject* getNullObject() {
        return nullObj;
    }
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
                *json = data->getJson();
        }

        hasSerialized = true;
        return json;
    }
};


#endif //MONDIS_MONDISOBJECT_H
