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
            if (now< timer.expireTime) {
                this_thread::sleep_for(timer.expireTime - now);
            }
            now = chrono::system_clock::now();
            chrono::duration<int,ratio<1,1000000000>> diff;
            if (now > timer.expireTime) {
                diff = now - timer.expireTime;
            } else {
                diff = timer.expireTime - now;
            }

            if (diff > chrono::duration<int,ratio<1,1000000000>>(5000000)) {
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
