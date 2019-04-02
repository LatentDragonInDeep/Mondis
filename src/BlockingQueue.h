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
            unique_lock<mutex> lck(notEmptyMtx);
            notEmptyCV.wait(lck);
        }
        T t = innerQueue.back();
        innerQueue.pop();
        return t;
    };
    T put(T t) {
        lock_guard lck(mtx);
        innerQueue.push(t);
        notEmptyCV.notify_all();
    };
private:
    condition_variable notEmptyCV;
    mutex notEmptyMtx;
    queue<T> innerQueue;
    mutex mtx;
};

#endif //MONDIS_BLOCKINGQUEUE_H
