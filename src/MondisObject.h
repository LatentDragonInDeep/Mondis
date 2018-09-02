//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>
#include <stdint-gcc.h>

#include "MondisList.h"

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
    void* objectData;
private:
    MondisObject * mp_nullObj = new MondisObject;
public:
    static MondisObject* getNullObject() {
        return mp_nullObj;
    }


};


#endif //MONDIS_MONDISOBJECT_H
