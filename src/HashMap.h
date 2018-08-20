//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_HASHMAP_H
#define MONDIS_HASHMAP_H

#include "MondisObject.h"
typedef union {
    string* str;
    int intValue;
} HashKey;

class Entry{
    HashKey* key;
    MondisObject* object;
    bool isString;
public:
    bool compare(Entry& other) {
        if(isString) {
            return key->str->compare(other.key->str);
        }
        return key->intValue>other.key->intValue;
    }
};
class HashMap
{

};


#endif //MONDIS_HASHMAP_H
