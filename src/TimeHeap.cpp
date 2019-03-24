//
// Created by 11956 on 2019/3/24.
//

#include "TimeHeap.h"

void TimeHeap::start() {
    while (true) {
        if (!ttlQueue.empty()) {
            TTLStruct ts = ttlQueue.pop();
            ts.
        }
    }
}

void TimeHeap::put(TTLStruct& ts) {
    ttlQueue.push(ts);
}
