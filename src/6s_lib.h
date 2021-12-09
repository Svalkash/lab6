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

size_t msg_size(msg_t msg)
{                                                               //requires null-terminated string inside, else fails
    return sizeof(int) + sizeof(cmd_t) + strlen(msg.mtext) + 1; //type ignored
}

int send_msg(int msqid, int mtype, int sock, cmd_t cmd, const char *str)
{
    msg_t msg;

    msg.mtype = mtype;
    msg.sock = sock;
    msg.cmd = cmd;
    strncpy(msg.mtext, str, BUFSIZE - 1);
    msg.mtext[BUFSIZE - 1] = '\0';
    CHECK(msgsnd(msqid, &msg, msg_size(msg), 0), -1, "Error while sending message")
    return 0;
}

int rcv_msg(int msqid, int msgsz, long msgtyp, int msgflg, int *sock, cmd_t *cmd, char *str)
{
    msg_t msg;
    int rec;

    msg.mtype = msgtyp;
    rec = msgrcv(msqid, &msg, msgsz, msgtyp, msgflg);
    if (rec == -1)
        return -1;
    *sock = msg.sock;
    *cmd = msg.cmd;
    if (rec >= BUFSIZE - 1)
        rec = BUFSIZE - 1;
    strncpy(str, msg.mtext, rec);
    str[rec] = '\0';
    return rec;
}

int eat_msgs(int msq, int type)
{
    char msg[BUFSIZE]; //dummy for receiving
    int sock;          //dummy for receiving
    cmd_t cmd;         //dummy for receiving

    while (rcv_msg(msq, BUFSIZE - 1, type, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, msg) != -1)
        ;
    CHECK_SPECIFIC("Error while receiving message", ENOMSG)
}

void semfix(int sem, int val) { semctl(sem, 0, SETVAL, val); }

uint32_t randint()
{
    //YES, I KNOW THAT SIZEOF(INT) = 4!
    uint32_t rdesc;
    char buf[sizeof(uint32_t)];
    uint32_t rint = 0;

    rdesc = open("/dev/random", O_RDONLY);
    read(rdesc, buf, sizeof(uint32_t));
    close(rdesc);
    for (int i = 0; i < sizeof(uint32_t); ++i)
        rint += (uint32_t)buf[i] << 8 * i;
    return rint;
}

char *print_gameover(char *buf, int gameover, int player, int p1_score, int p2_score)
{
    char scorestr[BUFSIZE];
    int score_y, score_e;

    switch (player)
    {
    case 1:
        score_y = p1_score;
        score_e = p2_score;
        break;
    case 2:
        score_y = p2_score;
        score_e = p1_score;
        break;
    default:
        printf("Error: trying to send game state to non-player");
        break;
    }
    buf[0] = '\n';
    buf[1] = '\0';
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
    if (gameover)
    {
        if (score_y > score_e)
            strcat(buf, "      YOU  WIN!     \n");
        else
            strcat(buf, "      YOU LOSE!     \n");
        strcat(buf, "--------------------\n");
    }
    return buf;
}

char *print_turn(char *buf, int round, int turn, int attack)
{
    char str[BUFSIZE];

    buf[0] = '\n';
    buf[1] = '\0';
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

char *print_res(char *buf, int result, int p1_score, int p2_score)
{
    char str[BUFSIZE];

    buf[0] = '\n';
    buf[1] = '\0';
    strcat(buf, "--------------------\n");
    if (result == 0)
        strcat(buf, "Накрыл        (MISS)\n");
    else if (result == 1)
        strcat(buf, "Не пробил     (SAVE)\n");
    else
        strcat(buf, "Есть пробитие (GOAL)\n");
    strcat(buf, "--------------------\n");
    sprintf(str, "SCORE | P1: %d  P2: %d\n", p1_score, p2_score);
    strcat(buf, str);
    strcat(buf, "--------------------\n");
    return buf;
}

#endif