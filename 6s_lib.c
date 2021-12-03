#ifndef __6S_LIB_C__
#define __6S_LIB_C__

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

int check_mask(int signal)
{
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    return mask | signal;
}

int send_msg(int msqid, int mtype, int sock, cmd_t cmd, char str)
{
    msgbuf_t msg;

    msg.mtype = mtype;
    msg.sock = sock;
    msg.cmd = cmd;
    strncpy(msg.mtext, str, BUFSIZE - 1);
    str[BUFSIZE - 1] = '\0';
    CHECK(msgsnd(msqid, &msg, strlen(str) + 1, 0), -1, "Error while sending message")
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, int &sock, msg_t *cmd, char *str)
{
    msgbuf_t msg;
    int rec;

    msg.mtype = msgtyp;
    CHECK(rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg), -1, "Error while receiving message")
    *sock = msg.sock;
    *cmd = msg.cmd;
    if (rec >= BUFSIZE - 1)
        rec = BUFSIZE - 1;
    strncpy(str, msg.mtext, rec);
    str[rec] = '\0';
    return rec;
}

int eat_msgs(int msq, int type) {
    char msg[BUFSIZE]; //dummy for receiving
    int sock; //dummy for receiving
    cmd_t cmd; //dummy for receiving

    while (rcv_msg(msq, BUFSIZE - 1, type, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, msg) != -1)
        ;
    CHECK_EAGAIN("Error while receiving message")
}

char *print_state(int player, shmstr_t *state, char *buf, int gameover) {
    char scorestr[BUFSIZE];
    int score_y, score_e;

    switch(player) {
    case 1:
        score_y = state->p1_score;
        score_e = state->p2_score;
        break;
    case 2:
        score_y = state->p2_score;
        score_e = state->p1_score;
        break;
    default:
        printf("Error: trying to send game state to non-player");
        break;
    }
    strcat(buf, "--------------------\n");
    if (gameover)
        strcat(buf, " GAME     OVER      \n");
    else
        strcat(buf, " GAME  INTERRUPTED  \n");
    strcat(buf, "--------------------\n");
    sprintf(scorestr, "  Your  score:  %d  \n", score_y);
    strcat(buf, scorestr);
    sprintf(scorestr, "  Enemy score:  %d  \n", score_e);
    strcat(buf, scorestr);
    strcat(buf, "--------------------\n");
    if (gameover) {
        if (score_y > score_e)
            strcat(buf, "      YOU  WIN!     ");
        else
            strcat(buf, "      YOU LOSE!     ");
        strcat(buf, "--------------------\n");
    }
    return buf;
}

inline void semfix(int sem, int val) { semctl(sem, 0, SETVAL, val); }


#endif