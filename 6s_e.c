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
    //ex
    int player;
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
    CHECK(state = (shmstr_t *)shmat(shm, NULL, 0), NULL, "E: Error attaching memory")
    shm_sem = atoi(argv[4]);
    w_logwrite("E: Ready, ShM attached", V_ALL);

    //start
    CHECK(rcv_msg(msq_w, BUFSIZE - 1, getpid(), MSG_NOERROR, &sock, &msg_cmd, msg), -1, "E: Error receiving message")
    if (msg_cmd != CMD_SHOT && msg_cmd != CMD_SAVE)
        w_logwrite("E: [ERROR] Received unexpected command", V_MAIN);
    //execute them all
    semop(shm_sem, sop_lock, 2); //lock the struct
    if (sock == state->p1_sock)
    {
        state->p1_zone = atoi(msg);
        state->p1_st = PS_DONE;
        w_logwrite_int("E: P1 selected zone: ", state->p1_zone, V_GAME);
    }
    else if (sock == state->p2_sock)
    {
        state->p2_zone = atoi(msg);
        state->p2_st = PS_DONE;
        w_logwrite_int("E: P2 selected zone: ", state->p2_zone, V_GAME);
    }
    else
        w_logwrite("E: [ERROR] Command from a non-player!", V_MAIN);
    //now check - maybe both players did their turns?
    if (state->p1_st == PS_DONE && state->p2_st == PS_DONE)
    {
        int atk_zone = state->turn == 1 ? state->p1_zone : state->p2_zone;
        int def_zone = state->turn == 1 ? state->p2_zone : state->p1_zone;
        int p_hit = 100, p_save = 0;
        int miss = 0, save = 0, goal = 0;
        int rand_hit, rand_save;

        int gameover = 0;

        w_logwrite("E: both players ready, calculating..", V_ALL);
        //calculate result
        if (atk_zone == def_zone)
            p_save = 95;
        //special cases and misses
        switch (atk_zone)
        {
        case 0:
            p_hit = 100;
            break;
        case 2:
        case 3:
            p_hit = 100;
            if (def_zone == 0)
                p_save = 75;
            break;
        case 1:
        case 4:
            p_hit = 80;
            if (def_zone == 0)
                p_save = 50;
            break;
        case 7:
        case 8:
            p_hit = 70;
            break;
        case 6:
        case 9:
            p_hit = 60;
            break;
        case 5:
        case 10:
            p_hit = 50;
            break;
        default:
            w_logwrite("E: [ERROR] Invalid attack zone!", V_MAIN);
            break;
        }
        w_logwrite_int("E: Hit chance: ", p_hit, V_GAME);
        w_logwrite_int("E: Save chance: ", p_save, V_GAME);
        //rand
        rand_hit = randint() % 100;
        rand_save = randint() % 100;
        w_logwrite_int("E: rand-hit (< chance): ", rand_hit, V_GAME);
        w_logwrite_int("E: rand-save (< chance): ", rand_save, V_GAME);
        if (rand_hit < p_hit)
        {
            if (rand_save < p_save) {
                w_logwrite("E: SAVE!", V_GAME);
                save = 1;
            }
            else {
                w_logwrite("E: GOAL!", V_GAME);
                goal = 1;
                if (state->turn == 1)
                    ++state->p1_score;
                else
                    ++state->p2_score;
            }
        }
        else {
            w_logwrite("E: MISS!", V_GAME);
            miss = 1;
        }
        //send messages about results
        print_res(sndbuf, miss ? 0 : save ? 1 : 2, state->p1_score, state->p2_score);
        send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, sndbuf);
        send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, sndbuf);
        //next turn or next round?
        if (state->turn == state->first_turn) //next turn
            state->turn = state->first_turn == 1 ? 2 : 1;
        else if (state->round < state->min_rounds || state->p1_score == state->p2_score) {//next round
            ++state->round;
            state->turn = state->first_turn;
        }
        else
            gameover = 1; //well, gameover.
        if (gameover) {
            w_logwrite_int("E: Gameover, cleaning mess...", state->turn, V_GAME);
            print_gameover(sndbuf, 1, 1, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, sndbuf);
            print_gameover(sndbuf, 2, 2, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, sndbuf);
            //clean state
            state->p1_sock = -1;
            state->p2_sock = -1;
            state->pass[0] = '\0';
            state->g_st = GS_NO;
            state->p1_st = PS_NO;
            state->p2_st = PS_NO;
        }
        else {
            w_logwrite_int("E: Round: ", state->round, V_GAME);
            w_logwrite_int("E: Turn: ", state->turn, V_GAME);
            print_turn(sndbuf, state->round, state->turn, state->turn == 1);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, sndbuf);
            print_turn(sndbuf, state->round, state->turn, state->turn == 2);
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, sndbuf);
            state->p1_st = PS_INIT; //ready for commands
            state->p2_st = PS_INIT; //ready for commands
        }
    }
    //all good
    semop(shm_sem, sop_unlock, 1); //unlock the struct
    w_logwrite("E: Done, stopping..", V_ALL);
    shmdt(state);
    w_logwrite("E: Stopped.", V_ALL);
    return 0;
}