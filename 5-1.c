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

int send_msg(int msqid, int mtype, char *str) {
    msgbuf msg;

    msg.mtype = mtype;
    msg.a = 7;
    strncpy(msg.mtext, str, BUFSIZE);

    if (msgsnd(msqid, &msg, strlen(str) + 1, 0) == -1) {
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