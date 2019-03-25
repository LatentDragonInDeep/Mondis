//
// Created by 11956 on 2019/3/24.
//

#include "TimeHeap.h"
#include "mondis.pb.h"
#include <thread>
#include <condition_variable>

using namespace mondis;

void TimeHeap::start() {
    while (true) {
        if (!ttlQueue.empty()) {
            TTLStruct ts = ttlQueue.pop();
            Message * msg = new Message;
            msg.
        } else{
            notEmptyCV.;
        }
    }
}

void TimeHeap::put(TTLStruct& ts) {
    ttlQueue.push(ts);
}
