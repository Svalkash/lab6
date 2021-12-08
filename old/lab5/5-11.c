#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>

#define BUFSIZE 80

#define MAX_BIG 100
#define MAX_SMALL 20

//https://intuit.ru/studies/courses/553/409/lecture/17867

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

long powg(long b, long e, long p) {
    long ret = 1;

    for (int i = 0; i < e; ++i)
        ret = (ret * b) % p;
    return ret;
}

int wait_signal(int signal) {
    sigset_t mask;
    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigdelset(&mask, signal);
    return sigsuspend(&mask);
}

void sig_handler(int signum){
}  

main(int argc, char *argv[])
{
    int msqid;
    char sndstr[BUFSIZE], rcvstr[BUFSIZE];
    int sndtype = 1, rcvtype = 0;
    int ret = 0;
    int fid;
    int active = 0;
    sigset_t mask;

    long g, p, a, A, B, K;
    int id;

    if (argc < 4) {
        printf("Usage: %s <key> <p> <g>\n", argv[0]);
        return -1;
    }

    srand(time(NULL));
    signal(SIGUSR1, sig_handler);

    printf("Opening queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | 0666)) == -1) {
        perror("Error while opening/creating: ");
        return errno;
    }
    printf("Queue ID: %d\n", msqid);

    printf("Trying to read anyone's PID...\n", msqid);

    /*
    Handshake:
    first prints PID to broadcast (type=1)
    second prints PID to type=pid_first
    first does the same

    (assuming two processes CAN'T enter queue and read nothing at the same time)
    */

    printf("---HANDSHAKE---\n");
    if (rcv_msg(msqid, BUFSIZE, rcvtype, IPC_NOWAIT, rcvstr) == ENOMSG) {
        active = 1;
        sprintf(sndstr, "%d", getpid());
        printf("Sending own PID (%d)...\n", getpid());
        send_msg(msqid, sndtype, sndstr);
        printf("Waiting for someone to answer directly...\n");
        rcvtype = getpid();
        printf("[Now listening to type %d]\n", rcvtype);
        rcv_msg(msqid, BUFSIZE, rcvtype, 0, rcvstr);
        printf("Got direct answer from PID (%d), finishing a handshake...\n", atoi(rcvstr));
        sndtype = atoi(rcvstr); //got someone's PID
        printf("[Now sending to type %d]\n", sndtype);
        sprintf(sndstr, "%d", getpid());
        send_msg(msqid, sndtype, sndstr);
        printf("Handshake complete!\n");

        a = 6;
    }
    else {
        active = 0;
        printf("Got broadcast from PID (%d), starting a handshake and sending own PID (%d)...\n", atoi(rcvstr), getpid());
        sndtype = atoi(rcvstr); //got someone's PID
        printf("[Now sending to type %d]\n", sndtype);
        sprintf(sndstr, "%ld", getpid());
        send_msg(msqid, sndtype, sndstr);
        printf("Waiting for a finishing answer...\n");
        rcvtype = getpid();
        printf("[Now listening to type %d]\n", rcvtype);
        rcv_msg(msqid, BUFSIZE, rcvtype, 0, rcvstr);
        if (atoi(rcvstr) != sndtype) {
            printf("PID mismatch!\n");
            return 0;
        }
        printf("Handshake complete!\n");
        
    a = 15;
    }

    printf("---DHALG---\n");
    p = atoi(argv[2]);
    g = atoi(argv[3]);
    //a = rand() % MAX_SMALL + 1;
    printf("Selected a = %ld.\n", a);
    A = powg(g, a, p);
    printf("Calculated A = %ld. Sending...\n", A);
    sprintf(sndstr, "%ld", A);
    send_msg(msqid, sndtype, sndstr);
    printf("Done. Waiting for B...\n");
    rcv_msg(msqid, BUFSIZE, rcvtype, 0, rcvstr);
    sscanf(rcvstr, "%ld", &B);
    printf("Received B = %ld.\n", B);
    K = powg(B, a, p);
    printf("Calculated K = %ld.\n", K);
    printf("---DONE---\n");
    if (active) {
        printf("Waiting for passive...\n");
        wait_signal(SIGUSR1);
        printf("Done, removing...\n");
        rm_msq(msqid);
    }
    else {
        printf("Done, sending signal...\n");
        kill(sndtype, SIGUSR1);
        printf("Signal sent.\n");
    }
    return 0;    
}