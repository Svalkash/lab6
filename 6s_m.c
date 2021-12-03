#include "6s_types.h"
#include "6s_lib.c"

//-----------------------------------------------------------------

/*
Текущая проблема:
Сделай наконец память с семафорами!!!
*/

int port;
int port_changed = 0;

int msq_w, msq_m; //workers' and master's msq
//msq_w types - master and worker's PIDs
//msq_m types - socket IDs

int shm, shm_sem;
shmstr_t *state; //shared structure

#include "6s_m_file.c"
#include "6s_m_sock.c"

void server_init() {
    //create basic structures and MQs
    logwrite("Creating basic structures...", V_ALL);
    CHECK(msq_m = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_m")
    logwrite("MQ_m created", V_ALL);
    CHECK(msq_w = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_w")
    logwrite("MQ_w created", V_ALL);
    CHECK(shm = shmget(IPC_PRIVATE, sizeof(shmstr_t), IPC_CREAT | 0666), -1, "Error while creating shmemory")
    logwrite("ShM created", V_ALL);
    CHECK(shm_sem = semget(IPC_PRIVATE, 1, IPC_CREAT), -1, "Error while creating semaphore")
    logwrite("Semaphore created", V_ALL);
}

void server_stop() {
    logwrite("Stopping server...", V_MAIN);
    msgctl(msq_m, IPC_RMID, NULL);
    msgctl(msq_w, IPC_RMID, NULL);
    logwrite("MQs removed", V_ALL);
    shmctl(shm, IPC_RMID, NULL);
    logwrite("ShM removed", V_ALL);
    semctl(sem, 0, IPC_RMID);
    logwrite("Semaphore removed", V_ALL);
}

int check_msg(int msgtype, int send_only)
{
    int msglen;
    char msg[BUFSIZE];
    int sock;
    cmd_t cmd; //dummy for receiving
    con_t *sock_con;
    int pid_f;

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    msglen = rcv_msg(msq_m, BUFSIZE - 1, msgtype, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, buf);
    if (msglen == -1)
    {
        CHECK_EAGAIN("Error while receiving message")
        if (!is_blocked)
            unblock_signal(SIGHUP);
        return 0;
    }
    //work with message - check its type first
    if (send_only && cmd != CMD_SEND)
        return 1; //skip commands
    switch (cmd)
    {
    case CMD_SEND:
        sock_send(sock, msg, 0);
        break;
    case CMD_SHOT:
    case CMD_SAVE:
    case CMD_CREATE:
    case CMD_JOIN:
        //create E and send command
        if (!(sock_con = sock_findcon(sock))) {
            logwrite_int("ERROR: Tried work with closed socket:", sock_r, V_MAIN); //sanity check
            return -1;
        }
        if (!(pid_f = fork()))
        {
            //TODO: exec E
        }
        send_msg(msq_w, pid_f, sock, cmd, msg);
        break;
    case CMD_GEN:
        if (!(pid_f = fork()))
        {
            //TODO: exec G
        }
        break;
    }
    if (!is_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

void handler_cfg(int signum)
{
    int old_port = port;

    cfgread();
    //reopen log
    logopen();
    logwrite("Log re-opened.", V_MAIN);
    //TODO: change ports.
    port_changed = (old_port != port);
    if (port_changed) {
        sock_closeall(0, "Server disconnected."); //sockets
        logwrite("Socket changed.", V_MAIN);
        sock_l = sock_listen(port);
    }
}

void handler_stop(int signum)
{
    //ending phase: delete all created stuff.
    sock_closeall(0, "Server stopped."); //sockets
    server_stop(); //all MQ and trash
    //closing log
    logwrite("Server stopped. Closing the log...", V_MAIN);
    logclose();
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <filecfg> <verbosity>\n", argv[0]);
        return -1;
    }
    if (argc == 3)
    {
        int vint = atoi(argv[3]);
        if (vint < (int)V_MAIN || vint > (int)V_SCREEN)
        {
            fprintf(stderr, "Wrong verbosity value, supported - from %d to %d\n", (int)V_MAIN, (int)V_SCREEN);
            return -2;
        }
        else
            verb = (verbosity_t)vint;
    }
    else
        verb = V_ALL;
    //TODO: well...
    //read config file, create log
    cfgread(argv[1]);
    logopen();
    logwrite("Log opened.", V_MAIN);
    //init basic structures
    server_init();
    //open connect socket
    sock_l = sock_listen(port);
    //prepare interrupt handlers
    //TODO: SIGTERM handler (SIGUSR1 for now)
    signal(SIGHUP, handler_cfg);
    signal(SIGUSR1, handler_stop);
    //TODO: start generator ONCE and wait for it.
    while (1)
    {
        //accept new players, create A
        while (!port_changed && sock_accept(sock_l) == 1)
            ;
        //get new messages, answer to FINs
        for (int i = 0; !port_changed && i < cons_size; ++i)
        {
            if (sock_rcv(cons[i].sock, cons[i].pid_a) == -2) //"socket closed" here
                --i;
        }
        //send pending messages & execute commands
        //if port changed while receiving, msgs will be cleared anyway
        while (check_msg(0, 0))
            ;
        if (port_changed)
            port_changed = 0;
    }
    //ending phase: delete all created stuff.
    sock_closeall(0, "Server stopped."); //sockets
    server_stop(); //all MQ and trash
    //closing log
    logwrite("Server stopped. Closing the log...", V_MAIN);
    logclose();
    return 0;
}