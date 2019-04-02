//
// Created by 11956 on 2019/3/24.
//

#include "TimerHeap.h"
#include <thread>
#include <condition_variable>
using namespace std;

void TimerHeap::start() {
    while (true) {
        if (!ttlQueue.empty()) {
            mtx.lock();
            Timer timer = ttlQueue.top();
            ttlQueue.pop();
            mtx.unlock();
            auto now = chrono::system_clock::now();
            if (timer.expireTime<now) {
                continue;
            }
            auto sleepTime = timer.expireTime-now;
            this_thread::sleep_for(sleepTime);
            timer.task();
            if (timer.isLoop) {
                Timer next = timer;
                next.expireTime = timer.expireTime;
                next.expireTime += timer.period;
                mtx.lock();
                ttlQueue.push(next);
                mtx.unlock();
            }
        } else{
            unique_lock<mutex> lck(notEmptyMtx);
            notEmptyCV.wait(lck);
        }
    }
}

void TimerHeap::put(Timer& ts) {
    mtx.lock();
    ttlQueue.push(ts);
    mtx.unlock();
    notEmptyCV.notify_all();
}
