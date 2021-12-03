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

int check_mask(int signal)
{
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    return sigismember(&mask, signal);
}

int send_msg(int msqid, int mtype, int sock, cmd_t cmd, char *str)
{
    msg_t msg;

    msg.mtype = mtype;
    msg.sock = sock;
    msg.cmd = cmd;
    strncpy(msg.mtext, str, BUFSIZE - 1);
    str[BUFSIZE - 1] = '\0';
    CHECK(msgsnd(msqid, &msg, strlen(str) + 1, 0), -1, "Error while sending message")
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, int *sock, cmd_t *cmd, char *str)
{
    msg_t msg;
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
            strcat(buf, "      YOU  WIN!     \n");
        else
            strcat(buf, "      YOU LOSE!     \n");
        strcat(buf, "--------------------\n");
    }
    return buf;
}

inline void semfix(int sem, int val) { semctl(sem, 0, SETVAL, val); }

int randint() {
    //YES, I KNOW THAT SIZEOF(INT) = 4!
    int rdesc;
    char buf[sizeof(int)];
    int rint = 0;

    rdesc = open("/dev/random", O_RDONLY);
    read(rdesc, buf, sizeof(int));
    close(rdesc);
    for (int i = 0; i < sizeof(int); ++i)
        rint += (int)buf[i] << 8 * i;
    return rint;
}

char *turnmsg(char *buf, int round, int turn, int attack) {
    char str[BUFSIZE];

    strcat(buf, "--------------------\n");
    sprintf(str, "      ROUND  %d      \n", round);
    strcat(buf, str);
    strcat(buf, "--------------------\n");
    sprintf(str, "       TURN %d       \n", turn);
    strcat(buf, str);
    if (attack)
        strcat(buf, " You are attacking! \n");
    else
        strcat(buf, " You are defending! \n");
    strcat(buf, "--------------------\n");
    strcat(buf, "Choose your zone:   \n");
    return buf;
}

#endif