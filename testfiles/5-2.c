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

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, char *str) {
    msg_t msg;
    int rec;

    msg.mtype = msgtyp;

    if ((rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg)) == -1) {
        perror("Error: ");
        return errno;
    }
    printf("%d bytes received\n", rec);
    printf("Received message (type %d): %s\n", msg.mtype, msg.mtext);
    printf("sock %d\n", msg.sock);
    printf("cmd %d\n", msg.cmd);
    strncpy(str, msg.mtext, BUFSIZE - 1);
    str[BUFSIZE] = '\0';
    return 0;
}

main(int argc, char *argv[])
{
    int msqid;
    struct msqid_ds msqinfo;
    int msgflg = 0;
    char str[BUFSIZE];

    if (argc < 4) {
        printf("Usage: %s <key> <type> <bytes> <nowait> <noerror>\n", argv[0]);
        return -1;
    }
    if (atoi(argv[3]) > BUFSIZE - 1) {
        printf("Too big msg size, max = %d!\n", BUFSIZE);
        return -2;
    }
    if (argc >= 5 && atoi(argv[4]))
        msgflg |= IPC_NOWAIT;
    if (argc >= 6 && atoi(argv[5]))
        msgflg |= MSG_NOERROR;

    printf("\nOpening queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | 0666)) == -1) {
        perror("Error: ");
        return errno;
    }

    printf("Receiving message...\n");
    return rcv_msg(msqid, atoi(argv[3]), atoi(argv[2]), msgflg, str);
}