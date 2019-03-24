//
// Created by 11956 on 2018/9/2.
//

#ifndef MONDIS_MONDISBINARY_H
#define MONDIS_MONDISBINARY_H


#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>

#include "MondisObject.h"
#include "Command.h"

class MondisBinary: public MondisData{
private:
    char* heapBuffer;
    const unsigned capacity;//char数组的容量
    unsigned position;//下一次读的起始位置
public:
    MondisBinary(int pos, int cap,
                 char hb[]);

    MondisBinary(int pos, int cap);

    ~MondisBinary();

    static MondisBinary* allocate(int cap);

    static MondisBinary* wrap(char hb[],int cap,int length);

    char get(unsigned i);

    unsigned read(unsigned length, char *buffer);

    unsigned write(unsigned length,char *buffer);

    bool setPosition(unsigned pos);

    bool back(unsigned off);

    bool forward(unsigned off);

    template <typename T>
    string readType() {
        if(position+ sizeof(T)>=capacity) {
            return "";
        }
        string res(heapBuffer + position, sizeof(T));
        position+=sizeof(T);

        return res;
    }

    void persist(std::string& filePath,int start,int end);
    void persist(std::string& filePath);

    void toJson();

    ExecutionResult execute(Command *command);

    MondisObject *locate(Command *command);
};


#endif //MONDIS_MONDISBINARY_H
