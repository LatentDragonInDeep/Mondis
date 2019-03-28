//
// Created by 11956 on 2019/3/24.
//

#include "TimeHeap.h"
#include "mondis.pb.h"
#include "MondisServer.h"
#include <thread>
#include <condition_variable>
using namespace std;

void TimeHeap::start() {
    while (true) {
        if (!ttlQueue.empty()) {
            Timer timer = ttlQueue.top();
            ttlQueue.pop();
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
                ttlQueue.push(next);
            }
        } else{
            unique_lock<mutex> lck(mtx);
            notEmptyCV.wait(lck);
        }
    }
}

void TimeHeap::put(Timer& ts) {
    ttlQueue.push(ts);
    notEmptyCV.notify_all();
}
