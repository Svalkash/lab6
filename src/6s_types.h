#ifndef __6S_TYPES_H__
#define __6S_TYPES_H__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define BUFSIZE 1024
#define NAMELEN 80
#define MAXLISTEN 256
#define MAXCONS 256

#define T_PRINT 1

#define CHECK(fun, errval, msg) \
    {                           \
        if ((fun) == errval)    \
        {                       \
            perror(msg);        \
            exit(errno);        \
        }                       \
    }

#define CHECK_SPECIFIC(msg, errval) \
    {                               \
        if (errno != errval)        \
        {                           \
            perror(msg);            \
            exit(errno);            \
        }                           \
    }

#define BLOCKRUN(FUN, SIGNAL)                \
    {                                        \
        int is_blocked = check_mask(SIGNAL); \
        if (!is_blocked)                     \
            block_signal(SIGNAL);            \
        FUN;                                 \
        if (!is_blocked)                     \
            unblock_signal(SIGNAL);          \
    }

typedef enum verb_e
{
    V_MAIN,  //only main events
    V_GAME,  //game events
    V_ALL,   //all events
    V_DEBUG, //even trash
    V_SCREEN //WOW, SO MUCH LETTERS
} verb_t;

typedef enum cmd_e
{
    CMD_PRINT, //M - print to log
    CMD_SEND,  //M - send message to socket
    CMD_SHOT,  //E
    CMD_SAVE,  //E
    CMD_START, //A - create game - internal
    CMD_JOIN,  //A - join game - internal
    CMD_A,     //A - interpret command
    CMD_GEN,   //G - select first attacker and clear game score
    CMD_INIT,  //G - reset structure to 0
    CMD_STOP,  //A - stop
    CMD_UNKN
} cmd_t;

typedef enum gstate_e
{
    GS_NO,
    GS_CONNECT,
    GS_GEN,
    GS_GAME
} gstate_t; //game state for each turn

typedef enum pstate_s
{
    PS_NO,
    PS_INIT,
    PS_EXEC,
    PS_DONE
} pstate_t; //player state for each turn

typedef struct con_s
{
    int sock;
    struct sockaddr_in addr;
    int pid_a;
} con_t;

typedef struct msg_s
{
    long mtype;
    int sock;
    cmd_t cmd;
    char mtext[BUFSIZE];
} msg_t;

typedef struct shmstr_s
{
    int p1_sock, p2_sock; //player 1 & 2 sockets
    char pass[NAMELEN];
    int first_turn;
    int min_rounds;
    gstate_t g_st;
    pstate_t p1_st, p2_st;
    int p1_zone, p2_zone;
    int p1_score, p2_score;
    int round, turn;
} shmstr_t;

struct sembuf sop_lock[2] = {0, 0, 0, 0, 1, 0};
struct sembuf sop_unlock[1] = {0, -1, 0};

#endif