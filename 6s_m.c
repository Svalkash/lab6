#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> 
#include <errno.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "6s_types.h"
#include "6s_lib.h"

//-----------------------------------------------------------------

/*
Текущая проблема:
Сделай наконец память с семафорами!!!
*/

int port;
int port_changed = 0;

int cons_size = 0;
con_t cons[MAXCONS]; //array of connection structures

int msq_w, msq_m; //workers' and master's msq
//msq_w types - master and worker's PIDs
//msq_m types - socket IDs


#include "6s_m_file.h"

void handler_cfg(int signum) {
    int old_port = port;

    cfgread();
    //reopen log
    logopen();
    logwrite("Log re-opened.", V_MAIN);
    //TODO: change ports.
    port_changed = (old_port != port);
}

void handler_stop(int signum) {
    //TODO: code
    //TODO: use 'shutdown' in error situations
}

int sock_listen(int port) {
    int sock;
    struct sockaddr_in sa;

    CHECK(sock = socket(PF_INET, SOCK_STREAM, 0), -1, "Error while creating socket")
    fcntl(sock, F_SETFL, O_NONBLOCK | fcntl(sock_l, F_GETFL));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    CHECK(bind(sock, (struct sockaddr *)&sa, sizeof(sa)), -1, "Error while binding socket")
    listen(sock, MAXLISTEN);
    logwrite("Connect socket opened.", V_ALL);
    return sock;
}

int sock_accept(int sock_l) {
    int sock_r;
    struct sockaddr_in sa_r;
    int sa_r_len;
    const char hello_msg[] = "Welcome! Please enter your commands:\n";

    if (cons_size == MAXCONS)
        return -1; //too much connections
    block_signal(SIGHUP);
    if ((sock_r = accept(sock_l, (struct sockaddr *)&sa_r, (socklen_t *)&sa_r_len)) == -1)
    {
        unblock_signal(SIGHUP);
        if (errno == EAGAIN) //no pending connections
            return 0;
        else
        {
            perror("Error while accepting connection");
            exit(errno);
        }
    }
    logwrite_int("Connection accepted:", sock_r, V_ALL);
    //connection accepted, all good
    fcntl(sock_r, F_SETFL, O_NONBLOCK | fcntl(sock_l, F_GETFL));
    ++con_size;
    cons[con_size - 1].sock = sock_r;
    cons[con_size - 1].addr = sa_r;
    cons[con_size - 1].pid_e_dize = 0;
    if (!(cons[con_size - 1].pid_a = fork())) {
        //TODO: exec A
    }
    logwrite_int("Created A:", cons[con_size - 1].pid_a, V_DEBUG);
    send(sock_r, hello_msg, strlen(hello_msg), 0); //send hello msg
    unblock_signal(SIGHUP);
    return 1;
}

int sock_rcv(int sock_r, int msg_type, int pid_a) {
    int rcv_sz;
    char rcv_msg[BUFSIZE];

    block_signal(SIGHUP);
    //receive message
    rcv_sz = recv(sock_r, rcv_msg, BUFSIZE - 1, 0); //-1 so \0 will be conserved
    if (rcv_sz == -1) {
        unblock_signal(SIGHUP);
        if (errno == EAGAIN) //no messages
            return 0;
        else
        {
            perror("Error while receiving message");
            exit(errno);
        }
    }
    else if (rcv_sz == 0)
        return -1; //FIN accepted
    rcv_msg[rcv_sz] = '\0';
    //send command to A
    send_msg(msq_w, pid_a, rcv_msg);
    unblock_signal(SIGHUP);
    return rcv_sz;
}

int sock_fin(int index) {
    int sock;
    cmd_t cmd;
    char buf[BUFSIZE]

        block_signal(SIGHUP);
    if (cons[index].sock == PLAYER) {
        //TODO section
    }
    //TODO: lock shm
    shutdown(cons[index].sock, SHUT_RD); //say bye-bye to the socket and now close it (it's ALREADY FIN'd by client)
    close(cons[index].sock);
    //remove messages and kill forks associated with this process
    while (rcv_msg(msq_w, BUFSIZE, cons[index].pid_a, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, buf) != -1)
        ;
    if (errno != EAGAIN) {
        perror("Error while receiving message");
        exit(errno);
    }
    kill(cons[index].pid_a, SIGTERM); //kill A
    //kill E
    for (int i = 0; i < cons[index].pid_e_size; ++i) {
        while (rcv_msg(msq_w, BUFSIZE, cons[index].pid_e, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, buf) != -1)
            ;
        if (errno != EAGAIN) {
            perror("Error while receiving message");
            exit(errno);
        }
        kill(cons[index].pid_e[i], SIGTERM); //kill Es
    }
    //now remove master's messages
    while (rcv_msg(msq_m, BUFSIZE, cons[index].sock, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, buf) != -1)
        ;
    //all trash deleted, now remove the record
    memcpy(cons[index + 1], cons[index], (cons_size - 1 - index) * sizeof(con_t));
    --cons_size; //reduce size
    unblock_signal(SIGHUP);
}

int main(int argc, char *argv[]) {
    int sock_l; //listening

    if (argc < 2 || argc > 3){
        fprintf(stderr, "Usage: %s <filecfg> <verbosity>\n", argv[0]);
        return -1;
    }
    if (argc == 3) {
        int vint = atoi(argv[3]);
        if (vint < (int)V_MAIN || vint > (int)V_SCREEN) {
            fprintf(stderr, "Wrong verbosity value, supported - from %d to %d\n", (int)V_MAIN, (int)V_SCREEN);
            return -2;
        }
        else
            verb = (verbosity_t)vint;
    }
    else
        verb = V_ALL;
    //create basic structures and MQs
    logwrite("Creating MQ...\n", V_ALL);
    CHECK(msq_m = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_m")
    CHECK(msq_w = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_w")
    //TODO: well...
    //read config file, create log
    cfgread(argv[1]);
    logopen();
    logwrite("Log opened.", V_MAIN);
    //open connect socket
    sock_l = sock_listen(port);
    //prepare interrupt handlers
    //TODO: SIGTERM handler (SIGUSR1 for now)
    signal(SIGHUP, handler_cfg);
    while (1)
    {
        //accept new players, create A
        while (!port_changed && sock_accept(sock_l) == 1)
            ;
        //get new messages, answer to FINs
        for (int i = 0; !port_changed && i < cons_size; ++i) {
            if (sock_rcv(cons[i].sock, cons[i].pid_a) == -1) //"socket closed" here
                sock_fin(i--);
        }
        //send pending messages & execute commands
    }
}