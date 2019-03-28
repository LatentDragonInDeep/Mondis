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
#include <chrono>
#include <ratio>
#include <functional>

using namespace std;

class Timer {
public:
    std::function<void()> task = nullptr;
    decltype(chrono::system_clock::now()) expireTime;
    bool isLoop = false;
    chrono::duration<int> period;
    bool operator<(const Timer& other) const {
        return expireTime>other.expireTime;
    }
    Timer(std::function<void()> t, decltype(chrono::system_clock::now()) et,bool l = false,chrono::duration<int> p = chrono::duration<int>(1)):task(t),expireTime(et),isLoop(l),period(p){};
};

class TimeHeap {
    priority_queue<Timer> ttlQueue;
    mutex mtx;
    condition_variable notEmptyCV;
public:
    void put(Timer& ts);
    void start();
};


#endif //MONDIS_TIMEHEAP_H
