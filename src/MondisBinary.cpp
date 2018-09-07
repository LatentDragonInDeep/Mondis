//
// Created by 11956 on 2018/9/2.
//

#include "MondisBinary.h"
#include <fstream>

MondisBinary::MondisBinary(int mark, int pos, int lim, int cap, char *hb, int offset,bool isWrapped) :
        m_mark(mark),position(pos),limit(lim),capacity(cap),heapBuffer(hb),offset(offset),isWrapped(isWrapped){}

MondisBinary::MondisBinary(int mark, int pos, int lim, int cap) :MondisBinary(mark,pos,lim,cap,new char[capacity],0,false){}

MondisBinary *MondisBinary::allocate(int cap) {
    return new MondisBinary(0,0,cap,cap);
}

MondisBinary *MondisBinary::wrap(char *hb, int cap, int offset, int length) {
    return new MondisBinary(0,offset,offset+length,cap,hb,offset, true);
}

char MondisBinary::get(unsigned i) {
    if(i<offset||i>limit) {
        throw std::invalid_argument("read out of range");
    }
    return heapBuffer[i];
}

void MondisBinary::put(unsigned i, char data) {
    if(i<offset||i>limit) {
        throw std::invalid_argument("write out of range");
    }
    heapBuffer[i] = data;
}

unsigned MondisBinary::read(unsigned length, char *buffer) {
    if(position>=limit) {
        return 0;
    }
    else if(position+length<=limit) {
        memcpy(buffer,heapBuffer+position,length);
        position+=length;
        return length;
    }
    int readable = limit-position;
    memcpy(buffer,heapBuffer+position,readable);
    position = limit;

    return readable;
}

unsigned MondisBinary::write(unsigned length, char *buffer) {
    if(position>=limit) {
        return 0;
    }
    else if(position+length<=limit) {
        memcpy(heapBuffer+position,buffer,length);
        position+=length;
        return length;
    }
    int writable = limit-position;
    memcpy(heapBuffer+position,buffer,writable);
    position = limit;

    return writable;
}

bool MondisBinary::setPosition(unsigned pos) {
    if(pos<offset||pos>limit) {
        throw std::invalid_argument("position out of range");
    }
    position = pos;
}

bool MondisBinary::back(unsigned off) {
    if(position-off<offset) {
        return false;
    }
    position-=offset;
    return true;
}

bool MondisBinary::forward(unsigned off) {
    if(position+off>limit) {
        return false;
    }
    position+=offset;
    return true;
}

void MondisBinary::mark() {
    m_mark = position;
}

void MondisBinary::reset() {
    position = m_mark;
}

MondisBinary::~MondisBinary() {
    if(!isWrapped) {
        delete[] heapBuffer;
    }
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
    *json+="\"";
    *json+="LatentDragon";
    *json+= *new std::string(heapBuffer,capacity);
    *json+="\"";
}

ExecutionResult MondisBinary::execute(Command &command) {
    ExecutionResult res;
    switch (command.type) {
        case SET: {
            CHECK_PARAM_NUM(2)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            CHECK_PARAM_TYPE(1, STRING)
            CHECK_PARAM_LENGTH(1, 1)
            if (pos < offset || pos > limit) {
                res.res = "read or write out of range";
                return res;
            }
            heapBuffer[pos] = command[1][0];
            OK_AND_RETURN
        }
        case GET: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            if (pos < offset || pos > limit) {
                res.res = "read or write out of range";
                return res;
            }
            res.res = std::to_string(heapBuffer[pos]);
            OK_AND_RETURN
        }
        case SET_RANGE: {
            CHECK_PARAM_NUM(3)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_PARAM_TYPE(1, PLAIN);
            CHECK_PARAM_TYPE(2, STRING);
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            CHECK_PARAM_LENGTH(2, end - start);
            if (start < offset || end > limit || start > end) {
                res.res = "read or write out of range";
                return res;
            }
            memcpy(heapBuffer + start, command[0].content.data(), end - start);
            OK_AND_RETURN
        }
        case GET_RANGE: {
            CHECK_PARAM_NUM(3)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_PARAM_TYPE(1, PLAIN);
            CHECK_PARAM_TYPE(2, STRING);
            CHECK_AND_DEFINE_INT_LEGAL(0, start)
            CHECK_AND_DEFINE_INT_LEGAL(1, end)
            CHECK_PARAM_LENGTH(2, end - start);
            if (start < offset || end > limit || start > end) {
                res.res = "read or write out of range";
                return res;
            }
            res.res = string(heapBuffer + start, end - start);
            OK_AND_RETURN
        }
        case READ_CHAR: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(readType<char>());
            OK_AND_RETURN
        }
        case READ_SHORT: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(readType<short>());
            OK_AND_RETURN
        }
        case READ_INT: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(readType<int>());
            OK_AND_RETURN
        }
        case READ_LONG: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(readType<long>());
            OK_AND_RETURN
        }
        case READ_LONG_LONG: {
            CHECK_PARAM_NUM(0)
            res.res = std::to_string(readType<long long>());
            OK_AND_RETURN
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
            read(readable, buffer);
            res.res = string(buffer, readable);
            delete[] buffer;
            OK_AND_RETURN
        }
        case WRITE: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, STRING)
            write(command[0].content.size(), command[0].content.data());
            OK_AND_RETURN
        }
        case SET_POSITION: {
            CHECK_PARAM_NUM(1)
            CHECK_PARAM_TYPE(0, PLAIN);
            CHECK_AND_DEFINE_INT_LEGAL(0, pos)
            setPosition(pos);
            OK_AND_RETURN
        }

    }
    INVALID_AND_RETURN
}

MondisObject *MondisBinary::locate(Command &command) {
    return nullptr;
}

