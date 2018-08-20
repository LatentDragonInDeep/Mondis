//
// Created by caesarschen on 2018/8/20.
//

#ifndef MONDIS_MONDISOBJECT_H
#define MONDIS_MONDISOBJECT_H

#include <string>

#include "MondisList.h"

using namespace std;
enum MondisObjectType {
    RAW_STRING,
    RAW_INT,
    LIST,
    SET,
    ZSET,
    HASH,
    NULL
};


class MondisObject
{
public:
    MondisObjectType type;
    union {
        int intValue;
        string* strValue;
        MondisList* list;

    } objectData;
    static MondisObject* getNullObject() {
        MondisObject* pObj = new MondisObject;
        pObj->type = MondisObjectType ::NULL;

        return pObj;
    }

    unsigned int hashCode() {
        if(type == MondisObjectType::RAW_INT) {
            return intHash(objectData.intValue);
        }
        else if(type = MondisObjectType::RAW_STRING) {
            return strHash(*objectData.strValue);
        }
        else{
            return -1;
        }
    }

private:
    unsigned int intHash(int key) {
        key += ~(key << 15);
        key ^=  (key >> 10);
        key +=  (key << 3);
        key ^=  (key >> 6);
        key += ~(key << 11);
        key ^=  (key >> 16);
        return key;
    }

    unsigned int strHash(string& str) {
        /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
        //m和r这两个值用于计算哈希值，只是因为效果好。
        int len = str.size();
        const char * key = str.c_str();
        uint32_t seed = 2017;
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        /* Initialize the hash to a 'random' value */
        uint32_t h = seed ^ len;    //初始化

        /* Mix 4 bytes at a time into the hash */
        const unsigned char *data = (const unsigned char *)key;

        //将字符串key每四个一组看成uint32_t类型，进行运算的到h
        while(len >= 4) {
            uint32_t k = *(uint32_t*)data;

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            data += 4;
            len -= 4;
        }

        /* Handle the last few bytes of the input array  */
        switch(len) {
            case 3: h ^= data[2] << 16;
            case 2: h ^= data[1] << 8;
            case 1: h ^= data[0]; h *= m;
        };

        /* Do a few final mixes of the hash to ensure the last few
         * bytes are well-incorporated. */
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return (unsigned int)h;
    }

};


#endif //MONDIS_MONDISOBJECT_H
