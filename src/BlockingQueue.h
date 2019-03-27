//
// Created by chenshaojie on 2019/3/24.
//

#ifndef MONDIS_BLOCKINGQUEUE_H
#define MONDIS_BLOCKINGQUEUE_H

#include <queue>
#include <condition_variable>
#include <mutex>

using namespace std;
template <typename T>
class BlockingQueue {
public:
    T take() {
        if (innerQueue.empty()) {
            unique_lock<mutex> lck(mtx);
            notEmptyCV.wait(lck);
        }
        return innerQueue.pop();
    };
    T put(T t) {
        innerQueue.push(t);
        notEmptyCV.notify_all();
    };
private:
    condition_variable notEmptyCV;
    mutex mtx;
    queue<T> innerQueue;
};

#endif //MONDIS_BLOCKINGQUEUE_H
