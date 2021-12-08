#include "6s_types.h"
#include "6s_lib.h"

int msq_m, msq_w;
int shm;
int shm_sem;
shmstr_t *state; //shared structure

int w_logwrite(char *str, verb_t print_v)
{
    return send_msg(msq_m, T_PRINT, print_v, CMD_PRINT, str);
}

int w_logwrite_int(char *str, long num, verb_t print_v)
{
    char outstr[BUFSIZE];

    sprintf(outstr, "%s %ld", str, num);
    w_logwrite(outstr, print_v);
}

int main(int argc, char *argv[])
{
    int stop_flag = 0;
    //receiving
    int sock;
    cmd_t msg_cmd;
    char msg[BUFSIZE];
    char sndbuf[BUFSIZE];

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <msq_m> <msq_w> <shm> <sem>\n", argv[0]);
        return -1;
    }
    //init
    msq_m = atoi(argv[1]);
    msq_w = atoi(argv[2]);
    shm = atoi(argv[3]);
    CHECK(state = (shmstr_t *)shmat(shm, NULL, 0), NULL, "G: Error attaching memory")
    shm_sem = atoi(argv[4]);
    w_logwrite("G: Ready, ShM attached", V_ALL);

    //start
    CHECK(rcv_msg(msq_w, BUFSIZE - 1, getpid(), MSG_NOERROR, &sock, &msg_cmd, msg), -1, "G: Error receiving message")
    if (msg_cmd != CMD_GEN)
        w_logwrite("G: [ERROR] Received non-GEN command", V_MAIN);
    else
    {
        //lock and init memory
        semop(shm_sem, sop_lock, 2);
        state->first_turn = randint() % 2 ? 1 : 2;
        w_logwrite_int("G: First turn belongs to player: ", state->first_turn, V_GAME);
        state->min_rounds = 5;
        state->g_st = GS_GAME;
        state->p1_st = PS_INIT; //redundant
        state->p2_st = PS_INIT; //redundant
        state->p1_score = 0;
        state->p2_score = 0;
        state->p1_zone = 0;
        state->p2_zone = 0;
        state->round = 1;
        state->turn = state->first_turn;
        send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Game has started!\n");
        send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "Game has started!\n");
        w_logwrite_int("G: Game started, round: ", state->round, V_GAME);
        w_logwrite_int("G: turn: ", state->turn, V_GAME);
        print_turn(sndbuf, state->round, state->turn, state->turn == 1);
        send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, sndbuf);
        print_turn(sndbuf, state->round, state->turn, state->turn == 2);
        send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, sndbuf);
        semop(shm_sem, sop_unlock, 1);
    }

    w_logwrite("G: Done, stopping..", V_ALL);
    shmdt(state);
    w_logwrite("G: Stopped.", V_ALL);
    return 0;
}