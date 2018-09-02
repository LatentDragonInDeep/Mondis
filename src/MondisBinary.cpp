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
