//
// Created by 11956 on 2018/9/5.
//

#ifndef MONDIS_MONDISSERVER_H
#define MONDIS_MONDISSERVER_H


#include <sys/types.h>
#include <string>

#include "HashMap.h"
#include "MondisClient.h"
#include "Command.h"
#include "JSONParser.h"

class MondisServer {
private:
    /* General */
    pid_t pid;                  /* Main process pid. */
    std::string configfile;           /* Absolute config file path, or NULL */
    std::string executable;           /* Absolute executable file path. */
    std::string logFile;
    char **exec_argv;           /* Executable argv vector (copy). */
              /* serverCron() calls frequency in hertz */
    HashMap* curKeySpace;
    HashMap** allDbs;
    int curDbIndex = 0;

    HashMap *commands;             /* Command table */

    size_t initial_memory_usage; /* Bytes used after initialization. */
    int always_show_logo;       /* Show logo even for non-stdout logging. */

    /* Networking */
    int port;                   /* TCP listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
    char *unixsocket;           /* UNIX socket path */
    mode_t unixsocketperm;      /* UNIX socket permission */
    vector<int> tcpFds; /* TCP socket file descriptors */
    int ipfd_count;             /* Used slots in ipfd[] */
    int sofd;                   /* Unix socket file descriptor */

    vector<MondisClient*> *clients;              /* List of active clients */
    vector<MondisClient*> *clients_to_close;     /* Clients to close asynchronously */
    vector<MondisClient*> *clients_pending_write; /* There is to write or install handler. */

    MondisClient *current_client; /* Current client, only used on crash report */

    /* RDB / AOF loading information */
    bool loading;                /* We are loading data from disk if true */

    off_t loading_total_bytes;
    off_t loading_loaded_bytes;
    time_t loading_start_time;
    off_t loading_process_events_interval_bytes;
    /* Fast pointers to often looked up command */


    /* Configuration */
    int verbosity;                  /* Loglevel in redis.conf */
    int maxidletime;                /* Client timeout in seconds */
    bool tcpkeepalive;               /* Set SO_KEEPALIVE if non-zero. */

    int daemonize;                  /* True if running as a daemon */

    JSONParser parser;

public:
    int start(string& confFile);
    int runAsDeamon();
    int save();
    int startEventLoop();
    int appendLog();
    ExecutionResult execute(Command& command);
    ExecutionResult locateExecute(Command& command);
};


#endif //MONDIS_MONDISSERVER_H
