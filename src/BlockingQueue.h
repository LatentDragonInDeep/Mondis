//
// Created by chenshaojie on 2019/3/24.
//

#ifndef MONDIS_BLOCKINGQUEUE_H
#define MONDIS_BLOCKINGQUEUE_H

#include <queue>
#include <condition_variable>
#include <mutex>
#include <list>

using namespace std;
template <typename T>
class BlockingQueue {
public:
    T take() {
        mtx.lock();
        bool isEmpty = innerQueue.empty();
        mtx.unlock();
        if (isEmpty) {
            unique_lock<mutex> lck(notEmptyMtx);
            notEmptyCV.wait(lck);
        }
        mtx.lock();
        T t = innerQueue.front();
        innerQueue.pop();
        mtx.unlock();
        return t;
    };
    T put(T t) {
        mtx.lock();
        innerQueue.push(t);
        mtx.unlock();
        notEmptyCV.notify_all();
    };
private:
    condition_variable notEmptyCV;
    mutex notEmptyMtx;
    queue<T,std::list<T>> innerQueue;
    mutex mtx;
};

#endif //MONDIS_BLOCKINGQUEUE_H
