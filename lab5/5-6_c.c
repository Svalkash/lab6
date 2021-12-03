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
        if (!(errno == EIDRM || errno == EINVAL))
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

int timetostop = 0;

void sig_handler(int signum){
    timetostop = 1;
}  

main(int argc, char *argv[])
{
    int msqid, servid;
    struct msqid_ds msqinfo;
    char msg[BUFSIZE], str[BUFSIZE], resp[BUFSIZE];
    int msgtype = 1, resptype = 0;
    int ret = 0, rm_ret = 0;
    int fid;

    if (argc < 4) {
        printf("Usage: %s <key/type> <keyserver> <task>\n", argv[0]);
        return -1;
    }

    printf("Opening server queue...\n");
    if ((servid = msgget(atoi(argv[2]), 0)) == -1) {
        perror("Error while opening: ");
        return errno;
    }

    if (atoi(argv[3]) == 9) {
        printf("Opening response queue...\n");
        if ((msqid = msgget(atoi(argv[2]) + 1, 0)) == -1) {
            perror("Error while opening: ");
            return errno;
        }
    }
    else if (atoi(argv[3]) == 10)
        msqid = servid;
    else {
        printf("Creating client queue...\n");
        if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | O_EXCL | 0666)) == -1) {
            if (errno == EEXIST)
                printf("ERROR: Queue already exists.\n");
            else
                perror("Error while creating: ");
            return errno;
        }
        printf("Queue ID: %d\n", msqid);
    }
    
    //set up filter
    if (atoi(argv[3]) == 9 || atoi(argv[3]) == 10) {
        msgtype = 1;
        resptype = atoi(argv[1]);
    }

    //set handlers
    signal(SIGTERM, sig_handler);
    if (atoi(argv[3]) == 8 || atoi(argv[3]) == 9 || atoi(argv[3]) == 10)
        signal(SIGINT, sig_handler);
    
    printf("Opened, enter messages... (send '$' to stop)\n");
    if (fid = fork())
    {
        while (1) {
            if (atoi(argv[3]) == 7 || atoi(argv[3]) == 71) {
                printf("Enter receiver's type/key: ");
                scanf("%d", &msgtype);
            }
            else
                msgtype = atoi(argv[2]);
            printf("Enter your message: ");
            scanf("%s", str);
            if (strlen(str) == 1 && str[0] == '$') {
                printf("Sendind '$' to server...\n");
                ret = 0;
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
            if (atoi(argv[2]) == 9 || atoi(argv[2]) == 10)
                sprintf(msg, "%d %s", resptype, str);
            else
                sprintf(msg, "%d %s", msqid, str);

            if (ret = send_msg(servid, msgtype, msg)) {
                if (ret == EIDRM || ret == EINVAL)
                    printf("Server queue is removed.\n");
                break;
            }
            printf("Sent.\n");
        }
        //kill child
        kill(fid, SIGTERM);
        return ret;
    }
    else {
        while (!timetostop) {
            if (atoi(argv[3]) == 7)
                resptype = atoi(argv[1]);
            else
                resptype = 0;
            if (ret = rcv_msg(msqid, BUFSIZE, resptype, MSG_NOERROR, resp)) {
                //always fails when interrupted
                if (ret == EINTR)
                    printf("Signal caught, stopping...");
                break;
            }
            printf("Received message: %s\n", resp);
        }
        if ((atoi(argv[2]) == 9 || atoi(argv[2]) == 10) && (rm_ret = rm_msq(msqid)))
            ret = rm_ret;
        return ret;
    }
    
}