//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>
#include <stdint-gcc.h>

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


class MondisObject
{
public:
    MondisObjectType type = MondisObjectType::EMPTY;
    void * objectData;
private:
    MondisObject * mp_nullObj = new MondisObject;
    bool hasSerialized = false;
    string& json;
public:
    static MondisObject* getNullObject() {
        return mp_nullObj;
    }
    string& getJson() {
        if(hasSerialized) {
            return json;
        }
        switch (type){
            case MondisObjectType ::RAW_STRING:
                json="\""+(*(string*)objectData)+"\"";
                break;
            case MondisObjectType ::RAW_INT:
                json="\""+std::to_string(*((int*)objectData))+"\"";
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
    }
};


#endif //MONDIS_MONDISOBJECT_H
