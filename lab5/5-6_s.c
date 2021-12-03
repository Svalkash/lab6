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

//ipcs to check

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

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, char *str, int *retype) {
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
    if (retype != NULL)
        *retype = msg.mtype;
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

int timetostop = 0;

void sig_handler(int signum) {
    timetostop = 1;
}  

main(int argc, char *argv[])
{
    int msqid, respid, fwdid;
    struct msqid_ds msqinfo;
    char msg[BUFSIZE], str[BUFSIZE], resp[BUFSIZE];
    int msgtype = 0, resptype = 1, retype = 0;
    int ret = 0, rm_ret = 0;

    if (argc < 3) {
        printf("Usage: %s <key> <task>\n", argv[0]);
        return -1;
    }

    printf("Creating queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | O_EXCL | 0666)) == -1) {
        if (errno == EEXIST)
            printf("ERROR: Queue already exists.\n");
        else
            perror("Error while creating: ");
        return errno;
    }
    printf("Queue ID: %d\n", msqid);

    if (atoi(argv[2]) == 9) {
        printf("Creating response queue...\n");
        if ((respid = msgget(atoi(argv[1]) + 1, IPC_CREAT | O_EXCL | 0666)) == -1) {
            if (errno == EEXIST)
                printf("ERROR: Queue already exists.\n");
            else
                perror("Error while creating resp: ");
            return errno;
        }
        printf("Response queue ID: %d\n", msqid);
    }
    //set respid
    if (atoi(argv[2]) == 10)
        respid = msqid;
        
    //set up filter
    if (atoi(argv[2]) == 9 || atoi(argv[2]) == 10)
        msgtype = 1;

    //set handlers
    if (atoi(argv[2]) == 8 || atoi(argv[2]) == 9 || atoi(argv[2]) == 10)
        signal(SIGINT, sig_handler);

    printf("Opened, waiting for client messages... (send '$' to stop)\n");

    while (!timetostop) {
        if (ret = rcv_msg(msqid, BUFSIZE, msgtype, MSG_NOERROR, msg, &retype)) {
            //always fails when interrupted
            if (ret == EINTR)
                printf("Signal caught, stopping...");
            break;
        }
        if (atoi(argv[2]) == 9 || atoi(argv[2]) == 10)
            sscanf(msg, "%d %s", &resptype, str);
        else
            sscanf(msg, "%d %s", &respid, str);
        if (strlen(msg) == 1 && msg[0] == '$') {
            ret = 0;
            break;
        }
        printf("Received message: %s\n", str);
        if (atoi(argv[2]) == 7 || atoi(argv[2]) == 71) {
            printf("Resolving receiver...\n", str);
            fwdid = msgget(retype, 0);
            printf("Receiver ID: %d\n", fwdid);
            if (ret = send_msg(fwdid, retype, str))
                break;
            printf("Message forwarded.\n");
        }
        else {
            printf("Responding to %d...\n", respid);
            strcpy(resp, "response for '");
            strncat(resp, str, RESPLEN);
            if (strlen(str) > RESPLEN)
                strcat(resp, "...");
            strcat(resp, "'");
            if (ret = send_msg(respid, resptype, resp))
                break;
            printf("Response sent.\n");
        }
    }
    
    if (rm_ret = rm_msq(msqid))
        ret = rm_ret;
    if (atoi(argv[2]) == 9 && (rm_ret = rm_msq(respid)))
        ret = rm_ret;
    return ret;
}