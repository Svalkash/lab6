#ifndef __6S_TYPES_H__
#define __6S_TYPES_H__
#include <sys/socket.h>
#include <netinet/ip.h>

#define BUFSIZE 1024
#define NAMELEN 80
#define MAXLISTEN 256
#define MAXCONS 256
#define MAXEX 16

#define CHECK(fun, errval, msg) \
    {                           \
        if ((fun) == errval)    \
        {                       \
            perror(msg);        \
            exit(errno);        \
        }                       \
    }

typedef enum verb_e
{
    V_MAIN, //only main events
    V_GAME, //game events
    V_ALL, //all events
    V_DEBUG, //even trash
    V_SCREEN //WOW, SO MUCH LETTERS
} verb_t;

typedef enum cmd_e
{
    CMD_SEND,
    CMD_HIT,
    CMD_SAVE,
    CMD_CREATE
} cmd_t;

typedef struct con_s {
    int sock;
    struct sockaddr_in addr;
    int pid_a;
    int pid_e_size;
    int pid_e[MAXEX];
} con_t;

typedef struct msgbuf {
    long mtype;
    int sock;
    cmd_t cmd;
    char mtext[BUFSIZE];
} msgbuf;

struct sembuf sop_lock[2] = {0, 0, 0, 0, 1, 0};
struct sembuf sop_unlock[1] = {0, -1, 0};

#endif