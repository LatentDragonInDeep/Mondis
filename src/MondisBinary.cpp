//
// Created by 11956 on 2018/9/2.
//

#include "MondisBinary.h"

#define READ_TYPE(TYPE) CHECK_PARAM_NUM(0)\
                        string r = readType<TYPE>();\
                         if(r.size() == 0) {\
                         res.res = "read out of range!";\
                         LOGIC_ERROR_AND_RETURN\
                         }\
                         res.res = r;\
                           OK_AND_RETURN

MondisBinary::MondisBinary(int pos, int cap, char *hb) :
        position(pos), capacity(cap), heapBuffer(hb) {}

MondisBinary::MondisBinary(int pos, int cap) : MondisBinary(pos, cap, new char[capacity]) {}

MondisBinary *MondisBinary::allocate(int cap) {
    return new MondisBinary(0, cap);
}

char MondisBinary::get(unsigned i) {
    return heapBuffer[i];
}

void MondisBinary::put(unsigned i, char data) {
    heapBuffer[i] = data;
    modified();
}

unsigned MondisBinary::read(unsigned length, char *buffer) {
    if(position>=capacity) {
        return 0;
    }
    else if(position+length<=capacity) {
        memcpy(buffer,heapBuffer+position,length);
        position+=length;
        return length;
    }
    int readable = capacity-position;
    memcpy(buffer,heapBuffer+position,readable);
    position = capacity;

    return readable;
}

unsigned MondisBinary::write(unsigned length, char *buffer) {
    if(position>=capacity) {
        return 0;
    }
    else if(position+length<=capacity) {
        memcpy(heapBuffer+position,buffer,length);
        position+=length;
        return length;
    }
    int writable = capacity-position;
    memcpy(heapBuffer+position,buffer,writable);
    position = capacity;
    modified();

    return writable;
}

bool MondisBinary::setPosition(unsigned pos) {
    if(pos>=capacity) {
        throw std::invalid_argument("position out of range");
    }
    position = pos;
}

bool MondisBinary::back(unsigned off) {
    if(position-off<0) {
        position = 0;
        return true;
    }
    position-=off;
    return true;
}

bool MondisBinary::forward(unsigned off) {
    if(position+off>+capacity) {
        position = capacity;
        return true;
    }
    position+=off;
    return true;
}

MondisBinary::~MondisBinary() {
    delete[] heapBuffer;
}

void MondisBinary::persist(std::string &filePath) {
    persist(filePath,0,capacity);
}

void MondisBinary::persist(std::string &filePath, int start, int end) {
    std::ofstream out(filePath.c_str(),std::ios::binary);
    out.write(heapBuffer+start,end-start);
    out.flush();
}

void MondisBinary::toJson() {
    json = "";
    json += "\"";
    json += string(heapBuffer, capacity);
    json += "\"";
}

ExecutionResult MondisBinary::execute(Command *command) {
    ExecutionResult res;
    switch (command->type) {
        case BIND: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            CHECK_PARAM_TYPE(1, STRING)
            CHECK_PARAM_LENGTH(1, 1)
            if (pos < 0 || pos >=capacity) {
                res.res = "read or write out of range";
                LOGIC_ERROR_AND_RETURN
            }
            heapBuffer[pos] = (*command)[1].content[0];
            modified();
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            if (pos < 0 || pos >=capacity) {
                res.res = "read or write out of range";
                LOGIC_ERROR_AND_RETURN
            }
            res.res += heapBuffer[pos];
            OK_AND_RETURN
        }
        case SET_RANGE: {
            CHECK_PARAM_NUM(3)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_PARAM_TYPE(1, PLAIN);
            CHECK_PARAM_TYPE(2, STRING);
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            if (start < 0 || end > capacity || start >= end) {
                res.res = "read or write out of range";
                LOGIC_ERROR_AND_RETURN
            }
            if (PARAM(2).size() < end - start) {
                res.res = "data length too shrot!";
                LOGIC_ERROR_AND_RETURN
            }
            memcpy(heapBuffer + start, (*command)[2].content.data(), end - start);
            modified();
            OK_AND_RETURN
        }
        case GET_RANGE: {
            if (command->params.size() == 1) {
                CHECK_PARAM_TYPE(0, PLAIN);
                CHECK_AND_DEFINE_INT_LEGAL(0, start)
                if (start < 0) {
                    res.res = "read or write out of range";
                    LOGIC_ERROR_AND_RETURN
                }
                res.res = string(heapBuffer + start, capacity - start);
                OK_AND_RETURN
            }
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_PARAM_TYPE(1, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            if (start < 0 || end > capacity || start > end) {
                res.res = "read or write out of range";
                LOGIC_ERROR_AND_RETURN
            }
            res.res = string(heapBuffer + start, end - start);
            OK_AND_RETURN
        }
        case READ_CHAR: {
            READ_TYPE(char)
        }
        case READ_SHORT: {
            READ_TYPE(short)
        }
        case READ_INT: {
            READ_TYPE(int)
        }
        case READ_LONG: {
            READ_TYPE(long)
        }
        case READ_LONG_LONG: {
            READ_TYPE(long long)
        }
        case BACK: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, off)
            back(off);
            OK_AND_RETURN
        }
        case FORWARD: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, off)
            forward(off);
            OK_AND_RETURN
        }
        case READ: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN)
            CHECK_AND_DEFINE_INT_LEGAL(0, readable)
            char *buffer = new char[readable];
            readable = read(readable, buffer);
            res.res = string(buffer, readable);
            delete[] buffer;
            OK_AND_RETURN
        }
        case WRITE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, STRING)
            write((*command)[0].content.size(), (*command)[0].content.data());
            modified();
            OK_AND_RETURN
        }
        case SET_POS: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            setPosition(pos);
            OK_AND_RETURN
        }
        case CHECK_POS: {
            CHECK_PARAM_NUM(0)
            res.res = to_string(position);
            OK_AND_RETURN
        }

    }
    INVALID_AND_RETURN
}

MondisObject *MondisBinary::locate(Command *command) {
    return nullptr;
}

