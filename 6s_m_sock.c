#ifndef __6S_M_SOCK_C__
#define __6S_M_SOCK_C__

int sock_l; //listening socket

int cons_size = 0;
con_t cons[MAXCONS]; //array of connection structures

int sock_listen(int port)
{
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

int sock_accept(int sock_l)
{
    int sock_r;
    struct sockaddr_in sa_r;
    int sa_r_len;
    const char hello_msg[] = "Welcome! Please enter your commands:\n";

    if (cons_size == MAXCONS)
        return -1; //too much connections

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    if ((sock_r = accept(sock_l, (struct sockaddr *)&sa_r, (socklen_t *)&sa_r_len)) == -1)
    {
        CHECK_EAGAIN("Error while accepting connection")
    if (!is_blocked)
        unblock_signal(SIGHUP);
        return 0;
    }
    logwrite_int("Connection accepted:", sock_r, V_ALL);
    //connection accepted, all good
    fcntl(sock_r, F_SETFL, O_NONBLOCK | fcntl(sock_l, F_GETFL));
    ++con_size;
    cons[con_size - 1].sock = sock_r;
    cons[con_size - 1].addr = sa_r;
    cons[con_size - 1].pid_e_dize = 0;
    if (!(cons[con_size - 1].pid_a = fork()))
    {
        //TODO: exec A
    }
    logwrite_int("Created A:", cons[con_size - 1].pid_a, V_DEBUG);
    sock_send(sock_r, hello_msg, 0); //send hello msg
    if (!is_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

con_t *sock_con(int sock_r)
{ //find connection for socket
    for (int i = 0; i < cons_size; ++i)
        if (cons[i].addr == sock_r)
            return &cons[i];
    return NULL;
}

int sock_send(int sock_r, char *msg)
{
    int san_check = 0;

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    //sanity check
    if (!sock_con(sock_r))
    {
        logwrite_int("ERROR: Tried to send message to closed socket:", sock_r, V_MAIN);
        return -1;
    }
    logwrite_int("Sending to socket ", sock_r, V_DEBUG);
    logwrite(msg, V_DEBUG);
    send(sock_r, msg, strlen(msg), 0);
    if (!is_blocked)
        unblock_signal(SIGHUP);
}

int sock_rcv(int sock_r, int pid_a)
{
    int rcv_sz;
    char msg[BUFSIZE];

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    //receive message
    rcv_sz = recv(sock_r, msg, BUFSIZE - 1, 0); //-1 so \0 will be conserved
    if (rcv_sz == -1)
    {
        CHECK_EAGAIN("Error while receiving data from socket")
        if (!is_blocked)
            unblock_signal(SIGHUP);
        return 0;
    }
    else if (rcv_sz == 0) {
        sock_fin(sock_r);
        return -2;
    }
    msg[rcv_sz] = '\0';
    logwrite_int("Received from socket ", sock_r, V_DEBUG);
    logwrite(msg, V_DEBUG);
    //send command to A
    send_msg(msq_w, pid_a, sock_r, CMD_A, msg);
    if (!is_blocked)
        unblock_signal(SIGHUP);
    return rcv_sz;
}

void sock_closeall(int hard, char *msg) { //if hard, we don't send anything to clients
    char gameover[BUFSIZE];

    logwrite("Closing all connections...", V_MAIN);
    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    kill(0, SIGTERM); //stop ALL processes
    semfix(shm_sem, 0); //fix my semaphore
    //don't need to lock the structure now - I'm alone!
    eat_msgs(msq_w, 0); //delete ALL messages TO workers
    //free MQ
    if (!hard)
        while (check_msg(0, 1)) //send all remaining stuff,
            ;
    else
        eat_msgs(msq_m, 0); //eat ALL messages FROM workers
    if (!hard) {
        //send last words to everyone
        if (!hard && msg)
            for (int i = 0; i < cons_size; ++i)
                sock_send(cons[i].sock, msg, 0);
        logwrite("Last messages sent", V_ALL);
        //gameovers
        print_state(1, state, msg, state->g_st == GS_FINISH);
        send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, msg);
        print_state(2, state, msg, state->g_st == GS_FINISH);
        send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, msg);
        logwrite("Gameovers sent", V_ALL);
    }
    //close r-sockets
    for (int i = 0; i < cons_size; ++i) {
        shutdown(cons[i].sock, SHUT_RDWR);
        close(cons[i].sock); //because this IS an emergency
    }
    cons_size = 0;
    logwrite("Client sockets closed", V_ALL);
    //finally close l-socket
    shutdown(sock, SHUT_RDWR); //no, server will NOT wait for clients now. Shutdown is shutdown.
    close(sock);
    logwrite("Listen-socket closed", V_ALL);
    //now can unlock SIGHUP
    if (!is_blocked)
        unblock_signal(SIGHUP);
    logwrite("Listen-socket closed", V_ALL);
    logwrite("All sockets closed.", V_MAIN);

}

int sock_fin(int sock)
{
    char gameover[BUFSIZE];
    con_t *scon = sock_con(sock); //socket 'connection'

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    //check if THIS client was a player and do a gameover
    semop(shm_sem, sop_lock, 2);
    if (scon->sock == state->p1_sock) || scon->sock == state->p2_sock)
        {
            //basic messages
            if (scon->sock == state->p1_sock) // || scon->sock == state->p2_sock)
                send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "You are disconnected.");
            else
                send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "You are disconnected.");
            if (state->g_st == GS_GAME || state->g_st == GS_FINISH) {  //send gameover only in the game
                if (scon->sock == state->p1_sock) //don't send this in connect phase
                    send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "Player 1 was disconnected.");
                else
                    send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Player 2 was disconnected.");
                //gameovers
                print_state(1, state, msg, state->g_st == GS_FINISH);
                send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, msg);
                print_state(2, state, msg, state->g_st == GS_FINISH);
                send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, msg);
                //wipe state
                state->p1_sock  = -1;
                state->p2_sock  = -1;
                state->p1_st    = PS_NO;
                state->p2_st    = PS_NO;
                state->g_st     = GS_NO;
            }
        }
    semop(shm_sem, sop_unlock, 1);

    //clear A's messages
    eat_msgs(msq_w, scon->pid_a);
    send_msg(msq_w, scon->pid_a, sock_r, CMD_A, ""); //So A will be 'stopped' with messages
    waitpid(scon->pid_a, NULL, 0);                   //wait for A to stop
    //send all pending messages to the client before closing the socket
    while (check_msg(scon->sock, 1))
        ;
    shutdown(scon->sock, SHUT_RDWR); //say bye-bye to the socket and now close it (it's ALREADY FIN'd by client)
    close(scon->sock);
    //Forks are not killed - or else we'll need to deal with broken semaphores.
    //E are not killed at all: info about them is lost, we hope they'll shutdown without help.
    //We can try to remember every E's PID, keep it in con structure and track their completion...
    //But that's horrible, please no.
    //all trash deleted, now remove the record amd decrease size
    memcpy(scon, scon + 1, (cons + cons_size-- - 1 - scon) * sizeof(con_t));
    //yes, last line was impossible to read. I know ;)
    if (!is_blocked)
        unblock_signal(SIGHUP);
}

#endif