#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>

#define BUFSIZE 80

typedef struct msgbuf {
    long mtype;
    int a;
    char mtext[BUFSIZE];
} msgbuf;

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, char *str) {
    msgbuf msg;
    int rec;

    msg.mtype = msgtyp;

    if ((rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg)) == -1) {
        perror("Error: ");
        return errno;
    }
    printf("%d bytes received\n", rec);
    printf("Received message (type %d): %s\n", msg.mtype, msg.mtext);
    printf("%d", msg.a);
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

    printf("\Receiving message...\n");
    return rcv_msg(msqid, atoi(argv[3]), atoi(argv[2]), msgflg, str);
}