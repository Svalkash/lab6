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
#define RESPLEN 5


typedef struct msgbuf {
    long mtype;
    char mtext[BUFSIZE];
} msgbuf;

int send_msg(int msqid, int mtype, char *str) {
    msgbuf msg;

    msg.mtype = mtype;
    strncpy(msg.mtext, str, BUFSIZE-1);

    if (msgsnd(msqid, &msg, strlen(str) + 1, 0) == -1) {
        perror("Error while sending: ");
        return errno;
    }
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, char *str) {
    msgbuf msg;
    int rec;

    msg.mtype = msgtyp;

    if ((rec = msgrcv(msqid, &msg, msgsz, msgtyp, MSG_NOERROR)) == -1) {
        perror("Error while receiving: ");
        return errno;
    }
    printf("%d bytes received\n", rec);
    printf("Full message (type %d): %s\n", msg.mtype, msg.mtext);
    strncpy(str, msg.mtext, BUFSIZE - 1);
    str[BUFSIZE] = '\0';
    return 0;
}

int rm_msq(int rmsqid) {
    printf("Removing queue...\n");
    if (msgctl(rmsqid, IPC_RMID, NULL) == -1) {
        perror("Error while removing: ");
        return errno; 
    }
    printf("Queue removed.\n");
    return 0;
}

main(int argc, char *argv[])
{
    int msqid, respid;
    struct msqid_ds msqinfo;
    char msg[BUFSIZE], str[BUFSIZE], resp[BUFSIZE];
    int msgtype, resptype;
    int ret, rm_ret;

    if (argc < 3) {
        printf("Usage: %s <key> <task>\n", argv[0]);
        return -1;
    }

/*
    printf("Creating queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | O_EXCL | 0666)) == -1) {
        if (errno == EEXIST)
            printf("ERROR: Queue already exists.\n");
        else
            perror("Error while creating: ");
        return errno;
    }
    */

    printf("Opened, waiting for client messages... (send '$' to stop)\n");

    while (1) {
        msgtype = 0;
        if (ret = rcv_msg(msqid, BUFSIZE, msgtype, MSG_NOERROR, msg)) {
            //always fails when interrupted
            if (ret == EINTR)
                printf("Signal caught, stopping...");
            break;
        }
        sscanf(msg, "%d %s", &respid, str);
        if (strlen(msg) == 1 && msg[0] == '$') {
            ret = 0;
            break;
        }
        printf("Received message: %s\n", str);
        printf("Responding to %d...\n", respid);
        strcpy(resp, "response for '");
        strncat(resp, str, RESPLEN);
        if (strlen(str) > RESPLEN)
            strcat(resp, "...");
        strcat(resp, "'"); 
        resptype = 1;
        if (ret = send_msg(respid, resptype, resp))
            break;
        printf("Response sent.\n");
    }
    if (rm_ret = rm_msq(msqid))
        return rm_ret;
    return ret;
}