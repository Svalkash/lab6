#ifndef __6S_LIB_H__
#define __6S_LIB_H__

#include "6s_types.h"

int block_signal(int signal)
{
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigaddset(&mask, signal);
    return sigprocmask(SIG_SETMASK, &mask, NULL);
}

int unblock_signal(int signal)
{
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigdelset(&mask, signal);
    return sigprocmask(SIG_SETMASK, &mask, NULL);
}

int send_msg(int msqid, int mtype, int sock, cmd_t &cmd, char *str) {
    msgbuf msg;

    msg.mtype = mtype;
    msg.sock = sock;
    msg.cmd = cmd;
    strncpy(msg.mtext, str, BUFSIZE - 1);
    str[BUFSIZE - 1] = '\0';
    CHECK(msgsnd(msqid, &msg, strlen(str) + 1, 0), -1, "Error while sending message")
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, int &sock, msg_t &cmd, char *str) {
    msgbuf msg;
    int rec;

    msg.mtype = msgtyp;
    CHECK(rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg), -1, "Error while receiving message")
    *sock = msg.sock;
    *cmd = msg.cmd;
    strncpy(str, msg.mtext, rec);
    str[rec < BUFSIZE-1 ? rec : BUFSIZE-1] = '\0';
    return 0;
}

#endif