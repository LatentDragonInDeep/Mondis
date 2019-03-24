//
// Created by 11956 on 2019/3/24.
//

#ifndef MONDIS_TIMEHEAP_H
#define MONDIS_TIMEHEAP_H

#include <queue>
#include "MondisObject.h"
#include <string>
#include <condition_variable>
#include <mutex>

using namespace std;

class TTLStruct {
public:
    string key;
    time_t ttl;
    int dbIndex;
    bool operator<(TTLStruct& other) {
        return ttl>other.ttl;
    }
};

class TimeHeap {
    priority_queue<TTLStruct> ttlQueue;
    mutex mtx;
    condition_variable notEmptyCV;
    void put(TTLStruct& ts);
    void start();
};


#endif //MONDIS_TIMEHEAP_H
