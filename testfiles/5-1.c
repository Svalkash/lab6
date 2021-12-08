#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>

#define BUFSIZE 80
/*
typedef struct msgbuf {
    long mtype;
    int a;
    char mtext[BUFSIZE];
} msgbuf;*/

typedef enum cmd_e
{
    CMD_PRINT, //M - print to log
    CMD_SEND, //M - send message to socket
    CMD_SHOT,   //E
    CMD_SAVE,   //E
    CMD_START, //A - create game - internal
    CMD_JOIN,   //A - join game - internal
    CMD_A,      //A - interpret command
    CMD_GEN,    //G - select first attacker and clear game score
    CMD_INIT,   //G - reset structure to 0
    CMD_STOP,    //A - stop
    CMD_UNKN
} cmd_t;

typedef struct msg_s
{
    long mtype;
    int sock;
    cmd_t cmd;
    char mtext[BUFSIZE];
} msg_t;

size_t msg_size(msg_t msg) { //requires null-terminated string inside, else fails
    return sizeof(int) + sizeof(cmd_t) + strlen(msg.mtext) + 1; //type ignored
}

int send_msg(int msqid, int mtype, char *str) {
    msg_t msg;

    msg.mtype = mtype;
    msg.sock = 1234;
    msg.cmd = CMD_GEN;
    strncpy(msg.mtext, str, BUFSIZE);

    if (msgsnd(msqid, &msg, msg_size(msg), 0) == -1) {
        perror("Error: ");
        return errno;
    }
    return 0;
}

main(int argc, char *argv[])
{
    int msqid;
    struct msqid_ds msqinfo;

    if (argc < 4) {
        printf("Usage: %s <key> <type> <msg>\n", argv[0]);
        return -1;
    }
    if (atoi(argv[3]) > BUFSIZE - 1) {
        printf("Too big msg size, max = %d!\n", BUFSIZE);
        return -2;
    }
    
    printf("Opening queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | 0666)) == -1) {
        perror("Error: ");
        return errno;
    }

    printf("Sending message...\n");
    send_msg(msqid, atoi(argv[2]), argv[3]);
    printf("Done.\n");
    return 0;
}