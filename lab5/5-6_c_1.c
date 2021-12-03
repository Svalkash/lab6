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
    strncpy(msg.mtext, str, BUFSIZE);

    if (msgsnd(msqid, &msg, strlen(str) + 1, 0) == -1) {
        if (!(errno == EIDRM || errno == EINVAL))
            perror("Error while sending: ");
        return errno;
    }
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, char *str) {
    msgbuf msg;
    int rec;

    msg.mtype = msgtyp;

    if ((rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg)) == -1) {
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

void sig_handler(int signum){
}    

main(int argc, char *argv[])
{
    int msqid, servid;
    struct msqid_ds msqinfo;
    char msg[BUFSIZE], str[BUFSIZE], resp[BUFSIZE];
    int msgtype, resptype;
    int ret, rm_ret;

    if (argc < 4) {
        printf("Usage: %s <key> <keyserver> <task>\n", argv[0]);
        return -1;
    }

    signal(SIGALRM, sig_handler);
    printf("Creating client queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | O_EXCL | 0666)) == -1) {
        if (errno == EEXIST)
            printf("ERROR: Queue already exists.\n");
        else
            perror("Error while creating: ");
        return errno;
    }

    printf("Opening server queue...\n");
    if ((servid = msgget(atoi(argv[2]), 0)) == -1) {
        perror("Error while opening: ");
        return errno;
    }

    printf("Opened, enter messages... (send '$' to stop)\n");

    while (1) {
        printf("Enter your message: ");
        scanf("%s", str);
        if (strlen(str) == 1 && str[0] == '$') {
            msgtype = 1;
            ret = 0;
            printf("Sendind '$' to server...\n");
            if (ret = send_msg(servid, msgtype, str)) {
                if (ret == EIDRM || ret == EINVAL) {
                    printf("Server queue is removed.\n");
                    ret = 0;
                }
            }
            else
                printf("Done.\n");
            break;
        }
        sprintf(msg, "%d %s", msqid, str);

        msgtype = 1;
        if (ret = send_msg(servid, msgtype, msg)) {
            if (ret == EIDRM || ret == EINVAL)
                printf("Server queue is removed.\n");
            break;
        }
        printf("Sent, waiting for response...\n");

        alarm(1); //set alarm for timeout
        resptype = 0;
        if (ret = rcv_msg(msqid, BUFSIZE, resptype, MSG_NOERROR, resp)) {
            if (ret == EINTR)
                printf("Timeout.\n");
            break;
        }
        printf("Received response: %s\n", resp);
    }
    if (rm_ret = rm_msq(msqid))
        return rm_ret;
    return ret;
}