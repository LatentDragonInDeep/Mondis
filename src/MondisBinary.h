//
// Created by 11956 on 2018/9/2.
//

#ifndef MONDIS_MONDISBINARY_H
#define MONDIS_MONDISBINARY_H


#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

#include "MondisData.h"

class MondisBinary: public MondisData{
private:
    char* heapBuffer;
    const unsigned offset;//offset以前无法读
    const unsigned capacity;//char数组的容量
    const unsigned limit;//limit以后无法读
    unsigned position;//下一次读的起始位置
    unsigned m_mark;//标记
    const bool isWrapped;
public:
    MondisBinary(int mark, int pos, int lim, int cap,
                 char hb[], int offset, bool isWrapped);
    MondisBinary(int mark, int pos, int lim, int cap);

    ~MondisBinary();

    static MondisBinary* allocate(int cap);

    static MondisBinary* wrap(char hb[],int cap,int offset,int length);

    char get(unsigned i);

    void put(unsigned i, char data);

    unsigned read(unsigned length, char buffer[]);

    unsigned write(unsigned length,char buffer[]);

    bool setPosition(unsigned pos);

    bool back(unsigned off);

    bool forward(unsigned off);

    void mark();

    void reset();

    template <typename T>
    T readType() {
        if(position+ sizeof(T)>=limit) {
            throw new std::invalid_argument("read out of range");
        }
        T res =  *((T*)(heapBuffer+position));
        position+=sizeof(T);

        return res;
    }

    void persist(std::string& filePath,int start,int end);
    void persist(std::string& filePath);

    void toJson();
};


#endif //MONDIS_MONDISBINARY_H
