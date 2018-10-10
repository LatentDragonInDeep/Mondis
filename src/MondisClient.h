//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_MONDISCLIENT_H
#define MONDIS_MONDISCLIENT_H


#include <stdint-gcc.h>
#include <vector>
#include <time.h>

#ifdef WIN32

#include <winsock2.h>
#include <inaddr.h>

#elif defined(linux)
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "HashMap.h"
#include "Command.h"

enum ClientType {
    MASTER,
    SLAVE,
    CLIENT,
    PEER,
};
class MondisClient {
public:
    uint64_t id;            /* Client incremental unique ID. */
#ifdef WIN32
    SOCKET sock;
#elif defined(linux)
    int fd;/* Client socket. */
#endif
    ClientType type = CLIENT;
    int curDbIndex = 0;
    HashMap *keySpace = nullptr;            /* Pointer to currently SELECTed DB. */
    string name;             /* As set by CLIENT SETNAME. */
    vector<string> commandBuffer;         /* Buffer we use to accumulate client queries. */
    int curCommandIndex = 0;
    size_t querybuf_peak;   /* Recent (100ms or more) peak of querybuf size. */
    Command *curCommand;          /* Arguments of current command. */
    Command *lastcmd;  /* Last command executed. */
    int reqtype;            /* Request protocol type: PROTO_REQ_* */
    int multibulklen;       /* Number of multi bulk arguments left to read. */
    long bulklen;           /* Length of bulk argument in multi bulk request. */
    vector<ExecutionResult> *reply;            /* List of reply objects to sendToMaster to the client. */
    unsigned long long reply_bytes; /* Tot bytes of objects in reply list. */
    size_t sentlen;         /* Amount of bytes already sent in the current
                               buffer or object being sent. */
    time_t ctime;           /* Client creation time. */
    time_t lastinteraction; /* Time of the last interaction, used for timeout */

    string ip;
    string port;

    clock_t preInteraction = clock();
public:
    bool hasLogin = false;
private:
    static int nextId;
public:
#ifdef WIN32

    MondisClient(SOCKET sock);

#elif defined(linux)
    MondisClient(int fd);

#endif

    ~MondisClient();
    string readCommand();

    void send(const string &res);

    void updateHeartBeatTime();
};


#endif //MONDIS_MONDISCLIENT_H
