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
            TTLStruct ts = ttlQueue.top();
            ttlQueue.pop();
            auto now = system_clock::now();
            if (ts.ttl<now) {
                continue;
            }
            auto sleepTime = ts.ttl-now;
            this_thread::sleep_for(sleepTime);
            mondis::Message * msg = new mondis::Message;
            msg->set_msg_type(mondis::MsgType::COMMAND);
            msg->set_command_type(mondis::CommandType::TIMER_COMMAND)
            msg->set_client_name(ts.client_name);
            string command = "DEL ";
            command+=ts.key;
            msg->set_content(command);
            MondisServer::getInstance()->putToReadQueue(msg);
        } else{
            unique_lock<mutex> lck(mtx);
            notEmptyCV.wait(lck);
        }
    }
}

void TimeHeap::put(TTLStruct& ts) {
    ttlQueue.push(ts);
}
