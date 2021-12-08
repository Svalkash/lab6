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

int digits_only(char *str)
{
    char *ptr = str;
    while (*ptr != '\0')
    {
        if (*ptr < '0' || *ptr > '9')
            return 0;
        ++ptr;
    }
    return 1;
}

int decode(char *cmdstr, int sock)
{
    char *cmd, *arg, *trash;
    cmd_t dec_cmd;
    int player;

    //decode command
    cmd = strtok(cmdstr, " \t");
    arg = strtok(NULL, " \t");
    trash = strtok(NULL, " \t");

    w_logwrite("A: Decoding command:", V_DEBUG);
    w_logwrite(cmdstr, V_DEBUG);
    //common errors
    if (!cmd)
    {
        w_logwrite("A: [ERROR] Empty command!", V_ALL);
        send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Empty command!\n");
        return 0;
    }
    //finally decode the command
    dec_cmd = !strcmp(cmd, "start")  ? CMD_START
              : !strcmp(cmd, "join") ? CMD_JOIN
              : !strcmp(cmd, "shot") ? CMD_SHOT
              : !strcmp(cmd, "save") ? CMD_SAVE
                                     : CMD_UNKN;
    if (dec_cmd == CMD_UNKN)
    {
        w_logwrite("A: [ERROR] Unknown command!", V_ALL);
        send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Unknown command!\n");
        return 0;
    }
    if (!arg)
    {
        w_logwrite("A: [ERROR] No arguments for the command!", V_ALL);
        send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] No arguments for the command!\n");
        return 0;
    }
    if (trash)
    {
        w_logwrite("A: [ERROR] Trash at the end of the command!", V_ALL);
        send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Trash at the end of the command!\n");
        return 0;
    }
    if ((dec_cmd == CMD_START || dec_cmd == CMD_JOIN) && strlen(arg) > NAMELEN - 1)
    {
        w_logwrite("A: [ERROR] Password is too long!", V_ALL);
        send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Password is too long!\n");
        return 0;
    }
    if ((dec_cmd == CMD_SHOT || dec_cmd == CMD_SAVE) && (!digits_only(arg) || atoi(arg) < 0 || atoi(arg) > 10))
        {
            w_logwrite("A: [ERROR] Invalid zone number!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Invalid zone number!\n");
            return 0;
        }
    //here we look at the current state, so lock it
    semop(shm_sem, sop_lock, 2);
    player = (sock == state->p1_sock ? 1 : sock == state->p2_sock ? 2
                                                                  : 0);
    switch (state->g_st)
    {
    GS_NO:
        if (dec_cmd == CMD_START)
        {
            w_logwrite("A: Received 'start' command.", V_ALL);
            state->g_st = GS_CONNECT;  //waiting for connections now
            state->p1_sock = sock;     //mark current player as first
            state->p1_st = PS_INIT; //he won't send anything though
            strcpy(state->pass, arg);
            w_logwrite("A: New password:", V_ALL);
            w_logwrite(arg, V_ALL);
            w_logwrite("A: Waiting for player 2...", V_ALL);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Game successfully created, waiting for player 2...\n"); //notify player 1
            return 1;
        }
        else
        {
            w_logwrite("A: [ERROR] Game is not created yet!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Game is not created yet!\n");
            return 0;
        }
    GS_CONNECT:
        if (dec_cmd == CMD_START)
        {
            w_logwrite("A: [DENIED] Game is already created, but you can join it.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] Game is already created, but you can join it.\n");
            return 0;
        }
        else if (dec_cmd == CMD_JOIN)
        {
            w_logwrite("A: Received 'join' command.", V_ALL);
            if (strcmp(state->pass, arg))
            {
                w_logwrite("A: [DENIED] Password mismatch!", V_ALL);
                send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] Password mismatch!\n");
                return 0;
            }
            //password is good
            state->g_st = GS_GEN;      //waiting for connections now
            state->p2_sock = sock;     //mark current player as first
            state->p2_st = PS_INIT; //he will WAIT
            w_logwrite("A: Player 2 joined, waiting for G...", V_ALL);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Player 2 joined.\n");                   //notify player 1
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Клиент: Синхронизация... (loading)\n"); //notify player 1
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "Клиент: Синхронизация... (loading)\n"); //notify player 2
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_GEN, "");                                      //start generator
        }
        else
        {
            w_logwrite("A: [ERROR] Game is not started yet!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Game is not started yet!\n");
            return 0;
        }
    GS_GEN:
        if (dec_cmd == CMD_START || dec_cmd == CMD_JOIN)
        {
            w_logwrite("A: [DENIED] Game is already started, please wait.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] Game is already started, please wait.\n");
            return 0;
        }
        else
        {
            w_logwrite("A: [ERROR] Please wait.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[ERROR] Please wait.\n");
            return 0;
        }
    GS_GAME:
        if (dec_cmd == CMD_START || dec_cmd == CMD_JOIN)
        {
            w_logwrite("A: [DENIED] Game is already started, please wait.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] Game is already started, please wait.\n");
            return 0;
        }
        //game command - shot/save
        if (player == 1 && state->p1_st != PS_INIT || player == 2 && state->p2_st != PS_INIT)
        {
            w_logwrite("A: [DENIED] You have already made your turn!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] You have already made your turn!\n");
            return 0;
        }
        //can send commands
        if (dec_cmd == CMD_SHOT && state->turn != player)
        {
            w_logwrite("A: [DENIED] You are attacking!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] You are attacking!\n");
            return 0;
        }
        if (dec_cmd == CMD_SAVE && state->turn == player)
        {
            w_logwrite("A: [DENIED] You are defending!", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SEND, "[DENIED] You are defending!\n");
            return 0;
        }
        //command is ok, now do it and send it
        //now FOR NO REASON we send it to the executor...
        if (dec_cmd == CMD_SHOT)
        {
            w_logwrite("A: Received 'shot' command.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SHOT, arg);
        }
        if (dec_cmd == CMD_SAVE)
        {
            w_logwrite("A: Received 'save' command.", V_ALL);
            send_msg(msq_m, sock, sock, CMD_SAVE, arg);
        }
        if (player == 1) //mark player as EXEC so no new commands will be accepted
            state->p1_st = PS_EXEC;
        else
            state->p2_st = PS_EXEC;
        return 1;
    default:
        w_logwrite("A: [ERROR] Incorrect state!", V_ALL);
        return 0;
    }
    //We're done here, now unlock
    semop(shm_sem, sop_unlock, 1);
}

int main(int argc, char *argv[])
{
    int stop_flag = 0;
    //receiving
    int sock;
    cmd_t msg_cmd;
    char msg[BUFSIZE];
    //decoding
    char *cmdstr = NULL;
    char *cmdend = NULL, *cmdstart = NULL; //marks for particular command token

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <msq_m> <msq_w> <shm> <sem>\n", argv[0]);
        return -1;
    }
    //init
    msq_m = atoi(argv[1]);
    msq_w = atoi(argv[2]);
    shm = atoi(argv[3]);
    CHECK(state = (shmstr_t *)shmat(shm, NULL, 0), NULL, "A: Error attaching memory")
    shm_sem = atoi(argv[4]);
    w_logwrite("A: Ready, ShM attached", V_ALL);

    //start
    while (!stop_flag)
    {
        while (!cmdend)
        {
            int startpos;

            w_logwrite("Can't found \\n (or start)", V_DEBUG);
            w_logwrite("A: Waiting for command", V_DEBUG);
            CHECK(rcv_msg(msq_w, BUFSIZE - 1, getpid(), MSG_NOERROR, &sock, &msg_cmd, msg), -1, "A: Error receiving message")
            if (msg_cmd == CMD_STOP)
            {
                stop_flag = 0;
                break;
            }
            else if (msg_cmd != CMD_A)
            {
                w_logwrite_int("A: Error command type:", (int)msg_cmd, V_MAIN);
                continue;
            }
            //CMD_A
            w_logwrite("A: Got CMD_A:", V_ALL);
            w_logwrite(msg, V_ALL);
            if (!cmdstr || *cmdstart == '\0')
            {
                w_logwrite("A: str empty, free-malloc", V_DEBUG);
                free(cmdstr); //safe for NULL
                cmdstr = malloc(strlen(msg) + 1);
                strcpy(cmdstr, msg);
                cmdstart = cmdstr;
                w_logwrite("A: cmdstr = cmdstart = ", V_DEBUG);
                w_logwrite(cmdstr, V_DEBUG);
            }
            else
            {
                w_logwrite("A: str not empty, realloc", V_DEBUG);
                startpos = cmdstart - cmdstr; //remember startpos (realloc changes ptrs)
                cmdstr = realloc(cmdstr, strlen(cmdstr) + strlen(msg) + 1);
                w_logwrite("A: cmdstr = ", V_DEBUG);
                w_logwrite(cmdstr, V_DEBUG);
                strcat(cmdstr, msg);
                cmdstart = cmdstr + startpos; //correct startpos
                w_logwrite("A: cmdstart = ", V_DEBUG);
                w_logwrite(cmdstart, V_DEBUG);
            }
            cmdend = strchr(cmdstart, '\n');
        }
        w_logwrite("Found \\n", V_DEBUG);
        if (stop_flag)
            break;
        //Got full command, mark its end
        *cmdend = '\0';
        //interpret
        decode(cmdstart, sock);
        //unmark
        *cmdend = '\n';
        //next cmdstr of \0
        cmdstart = cmdend + 1;
        cmdend = strchr(cmdstart, '\n'); //get FULL command
    }
    w_logwrite("A: Got CMD_STOP, stopping..", V_ALL);
    free(cmdstr);
    shmdt(state);
    w_logwrite("A: Stopped.", V_ALL);
    return 0;
}