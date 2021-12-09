#include "6s_types.h"
#include "6s_lib.h"

//------------------------------------------------------------------------------------------------------

int port;
int port_changed = 0;

int msq_m, msq_w; //workers' and master's msq
//msq_w types - master and worker's PIDs
//msq_m types - socket IDs, BUT print = 1
//in PRINT msgs we send 1(T_PRINT) as type, verbosity as port
//in SHOT/SAVE - argument (zone) in text field

int shm;
int shm_sem;
shmstr_t *state; //shared structure

char str_msq_m[NAMELEN], str_msq_w[NAMELEN];
char str_shm[NAMELEN], str_shm_sem[NAMELEN];

//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
//prototypes
//file
int cfgread();
int logwrite(const char *str, verb_t print_v);
int logwrite_int(const char *str, long num, verb_t print_v);
int logopen();
int logclose();
//sock
int sock_send(int sock_r, const char *msg);
int sock_rcv(int sock_r, int pid_a);
int sock_listen(int port);
int sock_accept(int sock_l);
con_t *sock_findcon(int sock_r);
void sock_closeall(int hard, const char *msg);
int sock_fin(int sock);
//server
void server_init();
void server_stop();
int check_msg(int msgtype, int send_only, int print_only);
void handler_cfg(int signum);
void handler_stop(int signum);

//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
//FILE FUNCTIONS

verb_t verb; //how many we want to see in the log

char cfgfilename[NAMELEN];
char logname[NAMELEN];
int logfile = -1; //log file desc

int cfgread()
{
    char buf[BUFSIZE];
    int cfgfile; //cfg file desc

    if (verb == V_SCREEN)
        printf("Opening cfg file... \n");
    CHECK(cfgfile = open(cfgfilename, O_RDONLY), -1, "Error while opening config file")
    buf[read(cfgfile, buf, BUFSIZE - 1)] = '\0';
    sscanf(buf, "%d %s", &port, logname);
    if (verb == V_SCREEN)
        printf("Port = %d\n", port);
    if (verb == V_SCREEN)
        printf("Log file name = %s\n", logname);
    if (verb == V_SCREEN)
        printf("Closing cfg file... \n");
    close(cfgfile);
}

int logwrite(const char *str, verb_t print_v)
{
    int sighup_blocked = check_mask(SIGHUP);
    time_t t;
    char *timestr;

    if (print_v > verb)
        return 0;

    if (!sighup_blocked)
        block_signal(SIGHUP);
    //write to log
    time(&t);
    timestr = ctime(&t);
    write(logfile, timestr, strlen(timestr));
    write(logfile, " | ", 3);
    write(logfile, str, strlen(str));
    write(logfile, "\n", 1);
    if (verb == V_SCREEN)
        printf("[%d] %s\n", print_v, str); //print this trash on the screen
    //unblock SIGHUP
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

int logwrite_int(const char *str, long num, verb_t print_v)
{
    char outstr[BUFSIZE];

    sprintf(outstr, "%s %ld", str, num);
    logwrite(outstr, print_v);
}

int logopen()
{
    int sighup_blocked = check_mask(SIGHUP);
    char *logfilename;
    time_t t;
    char *timestr;

    if (!sighup_blocked)
        block_signal(SIGHUP);
    if (logfile != -1)
    {
        logwrite("Closing log file to reopen", V_MAIN);
        close(logfile);
    }

    printf("Creating log file... \n");
    time(&t);
    timestr = ctime(&t);
    logfilename = malloc(strlen(logname) + 3 + strlen(timestr) + 4 + 1);
    strcpy(logfilename, logname);
    strcat(logfilename, " - ");
    strcat(logfilename, timestr);
    strcat(logfilename, ".txt");
    CHECK(logfile = creat(logfilename, 0666), -1, "Error while opening log file")
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    free(logfilename);
}

int logclose() { close(logfile); }

//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
//SOCKET FUNCTIONS

int sock_l; //listening socket

int cons_size = 0;
con_t cons[MAXCONS]; //array of connection structures

int sock_send(int sock_r, const char *msg)
{
    int sighup_blocked = check_mask(SIGHUP);
    int ret;

    if (!sighup_blocked)
        block_signal(SIGHUP);
    //sanity check
    if (!sock_findcon(sock_r))
    {
        logwrite_int("ERROR: Tried to send message to closed socket:", sock_r, V_MAIN);
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return -1;
    }
    logwrite_int("Sending to socket ", sock_r, V_DEBUG);
    logwrite(msg, V_DEBUG);
    ret = send(sock_r, msg, strlen(msg), 0);
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    return ret;
}

int sock_rcv(int sock_r, int pid_a)
{
    int rcv_sz;
    char msg[BUFSIZE];

    int sighup_blocked = check_mask(SIGHUP);
    if (!sighup_blocked)
        block_signal(SIGHUP);
    //receive message
    rcv_sz = recv(sock_r, msg, BUFSIZE - 1, 0); //-1 so \0 will be conserved
    if (rcv_sz == -1)
    {
        CHECK_SPECIFIC("Error while receiving data from socket", EAGAIN)
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return 0;
    }
    else if (rcv_sz == 0)
    {
        logwrite_int("Socket down: ", sock_r, V_DEBUG);
        sock_fin(sock_r);
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return -2;
    }
    msg[rcv_sz] = '\0';
    logwrite_int("Received from socket: ", sock_r, V_DEBUG);
    logwrite(msg, V_DEBUG);
    logwrite_int("size: ", rcv_sz, V_DEBUG);
    //send command to A
    send_msg(msq_w, pid_a, sock_r, CMD_A, msg);
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    return rcv_sz;
}

int sock_listen(int port)
{
    int sock;
    struct sockaddr_in sa;

    CHECK(sock = socket(PF_INET, SOCK_STREAM, 0), -1, "Error while creating socket")
    fcntl(sock, F_SETFL, O_NONBLOCK | fcntl(sock_l, F_GETFL));
    sa.sin_family = AF_INET;
    if (port < 1024)
        logwrite("WARNING: Trying to use root-only socket", V_ALL);
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    CHECK(bind(sock, (struct sockaddr *)&sa, sizeof(sa)), -1, "Error while binding socket")
    listen(sock, MAXLISTEN);
    logwrite("Connect socket opened.", V_ALL);
    return sock;
}

int sock_accept(int sock_l)
{
    int sighup_blocked = check_mask(SIGHUP);
    int sock_r;
    struct sockaddr_in sa_r;
    int sa_r_len;
    const char hello_msg[] = "Welcome! Please enter your commands:\n";

    if (cons_size == MAXCONS)
        return -1; //too much connections

    if (!sighup_blocked)
        block_signal(SIGHUP);
    if ((sock_r = accept(sock_l, (struct sockaddr *)&sa_r, (socklen_t *)&sa_r_len)) == -1)
    {
        CHECK_SPECIFIC("Error while accepting connection", EAGAIN)
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return 0;
    }
    logwrite_int("Connection accepted:", sock_r, V_ALL);
    //connection accepted, all good
    fcntl(sock_r, F_SETFL, O_NONBLOCK | fcntl(sock_l, F_GETFL));
    ++cons_size;
    cons[cons_size - 1].sock = sock_r;
    cons[cons_size - 1].addr = sa_r;
    if (!(cons[cons_size - 1].pid_a = fork()))
        execl("6s_a.exe", "6s_a.exe", str_msq_m, str_msq_w, str_shm, str_shm_sem, NULL);
    logwrite_int("Created new fork (A):", cons[cons_size - 1].pid_a, V_DEBUG);
    sock_send(sock_r, hello_msg); //send hello msg
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

con_t *sock_findcon(int sock_r)
{ //find connection for socket
    for (int i = 0; i < cons_size; ++i)
        if (cons[i].sock == sock_r)
            return &cons[i];
    return NULL;
}

void sock_closeall(int hard, const char *msg)
{ //if hard, we don't send anything to clients
    int sighup_blocked = check_mask(SIGHUP);
    int sigterm_blocked = check_mask(SIGTERM);
    char gameover[BUFSIZE];
    struct sigaction act, oldact;

    act.sa_handler = SIG_IGN;
    sigemptyset((sigset_t*)(&act.sa_mask));
    act.sa_flags = 0;

    if (!sighup_blocked)
        block_signal(SIGHUP);
    logwrite("Closing all connections...", V_MAIN);
    sigaction(SIGTERM, &act, &oldact);//ignore SIGTERM - 0 kills itself, lol
    kill(0, SIGTERM); //stop ALL processes hard and rough
    sigaction(SIGTERM, &act, &oldact); //un-ignore SIGTERM
    semfix(shm_sem, 0); //fix my semaphore
    logwrite("Workers brutally killed.", V_ALL);
    //don't need to lock the structure now - I'm alone!
    eat_msgs(msq_w, 0); //delete ALL messages TO workers
    logwrite("Workers' MQ freed", V_ALL);
    //check gameover - new messages here
    if (!hard)
    {
        if (state->g_st == GS_GAME)
        {
            //gameovers
            print_gameover(gameover, 0, 1, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, gameover);
            print_gameover(gameover, 0, 2, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, gameover);
            logwrite("Gameovers queued", V_ALL);
        }
        else
            logwrite("No game - no gameovers", V_ALL);
        //send last words to everyone
        if (msg)
            for (int i = 0; i < cons_size; ++i)
                send_msg(msq_m, cons[i].sock, cons[i].sock, CMD_SEND, msg);
        logwrite("Last messages queued", V_ALL);
    }
    //free receiver MQ and send last messages
    if (!hard) {
        logwrite("Sending last messages to sockets...", V_ALL);
        while (check_msg(0, 1, 1)) //send all remaining stuff,
            ;
    }
    else
        while (check_msg(0, 0, 1)) //don't send anything, only print to log
            ;
    CHECK_SPECIFIC("Error receiving message", ENOMSG)
    logwrite("Master's MQ freed", V_ALL);
    //close r-sockets
    for (int i = 0; i < cons_size; ++i)
    {
        shutdown(cons[i].sock, SHUT_RDWR);
        close(cons[i].sock); //because this IS an emergency
    }
    cons_size = 0;
    logwrite("Client sockets closed", V_ALL);
    //finally close l-socket
    shutdown(sock_l, SHUT_RDWR); //no, server will NOT wait for clients now. Shutdown is shutdown.
    close(sock_l);
    logwrite("Listen-socket closed", V_ALL);
    //wipe state
    state->p1_sock = -1;
    state->p2_sock = -1;
    state->pass[0] = '\0';
    state->p1_st = PS_NO;
    state->p2_st = PS_NO;
    state->g_st = GS_NO;
    logwrite("Game state wiped.", V_ALL);
    //now can unlock SIGHUP
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    logwrite("All sockets closed.", V_MAIN);
}

int sock_fin(int sock)
{
    int sighup_blocked = check_mask(SIGHUP);
    char gameover[BUFSIZE];
    con_t *scon = sock_findcon(sock); //socket 'connection'

    if (!sighup_blocked)
        block_signal(SIGHUP);
    //check if THIS client was a player and do a gameover
    semop(shm_sem, sop_lock, 2);
    if (scon->sock == state->p1_sock || scon->sock == state->p2_sock)
    {
        logwrite("sock_fin: Socket was a player, endgame..", V_DEBUG);
        //basic messages
        if (scon->sock == state->p1_sock) // || scon->sock == state->p2_sock)
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "You are disconnected.\n");
        else
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "You are disconnected.\n");
        if (state->g_st == GS_GEN || state->g_st == GS_GAME)
        {                                     //send gameover only in the game
            if (scon->sock == state->p1_sock) //don't send this in connect phase
                send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, "Player 1 was disconnected.\n");
            else
                send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, "Player 2 was disconnected.\n");
        }
        //gameovers
        if (state->g_st == GS_GAME)
        {
            print_gameover(gameover, 0, 1, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p1_sock, state->p1_sock, CMD_SEND, gameover);
            print_gameover(gameover, 0, 2, state->p1_score, state->p2_score);
            send_msg(msq_m, state->p2_sock, state->p2_sock, CMD_SEND, gameover);
        }
        logwrite("Last messages to leaving player queued.", V_ALL);
        //wipe state
        state->p1_sock = -1;
        state->p2_sock = -1;
        state->pass[0] = '\0';
        state->p1_st = PS_NO;
        state->p2_st = PS_NO;
        state->g_st = GS_NO;
        logwrite("Game state wiped.", V_ALL);
    }

    semop(shm_sem, sop_unlock, 1);

    //clear A's messages
    eat_msgs(msq_w, scon->pid_a);
    send_msg(msq_w, scon->pid_a, sock, CMD_STOP, ""); //So A will be 'stopped' with messages
    waitpid(scon->pid_a, NULL, 0);                    //wait for A to stop
    //send all pending messages to the client before closing the socket
    while (check_msg(scon->sock, 1, 1))
        ;
    logwrite("sock_fin: Messages cleared.", V_ALL);
    shutdown(scon->sock, SHUT_RDWR); //say bye-bye to the socket and now close it (it's ALREADY FIN'd by client)
    close(scon->sock);
    logwrite("sock_fin: Socket closed.", V_ALL);
    //Forks are not killed - or else we'll need to deal with broken semaphores.
    //E are not killed at all: info about them is lost, we hope they'll shutdown without help.
    //We can try to remember every E's PID, keep it in con structure and track their completion...
    //But that's horrible, please no.
    //all trash deleted, now remove the record amd decrease size
    memcpy(scon, scon + 1, (cons + cons_size-- - 1 - scon) * sizeof(con_t));
    //yes, last line was impossible to read. I know ;)
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
}

//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------

void server_init()
{
    int pid_f;

    //create basic structures and MQs
    logwrite("Creating basic structures...", V_ALL);
    CHECK(msq_m = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_m")
    logwrite("MQ_m created", V_ALL);
    CHECK(msq_w = msgget(IPC_PRIVATE, IPC_CREAT | 0666), -1, "Error while creating MQ_w")
    logwrite("MQ_w created", V_ALL);
    CHECK(shm = shmget(IPC_PRIVATE, sizeof(shmstr_t), IPC_CREAT | 0666), -1, "Error while creating shmemory")
    CHECK(state = (shmstr_t *)shmat(shm, NULL, 0), NULL, "Error attaching memory")
    logwrite("ShM created and attached", V_ALL);
    CHECK(shm_sem = semget(IPC_PRIVATE, 1, IPC_CREAT), -1, "Error while creating semaphore")
    logwrite("Semaphore created", V_ALL);
    //now make 'string' versions of IDs
    //now make 'string' versions of IDs
    sprintf(str_msq_m, "%d", msq_m);
    sprintf(str_msq_w, "%d", msq_w);
    sprintf(str_shm, "%d", shm);
    sprintf(str_shm_sem, "%d", shm_sem);
    //reset game structure
    if (!(pid_f = fork()))
        execl("6s_g.exe", "6s_g.exe", str_msq_m, str_msq_w, str_shm, str_shm_sem, NULL);
    logwrite_int("Created new fork (G):", pid_f, V_DEBUG);
    send_msg(msq_w, pid_f, state->p1_sock, CMD_INIT, "");
    waitpid(pid_f, NULL, 0); //wait for gen to init structure
}

void server_stop()
{
    logwrite("Stopping server...", V_MAIN);
    msgctl(msq_m, IPC_RMID, NULL);
    msgctl(msq_w, IPC_RMID, NULL);
    logwrite("MQs removed", V_ALL);
    shmdt(state);
    shmctl(shm, IPC_RMID, NULL);
    logwrite("ShM detached and removed", V_ALL);
    semctl(shm_sem, 0, IPC_RMID);
    logwrite("Semaphore removed", V_ALL);
}

int check_msg(int msgtype, int send_only, int print_only)
{
    int sighup_blocked = check_mask(SIGHUP);
    int msglen;
    char msg[BUFSIZE];
    int sock;
    cmd_t cmd; //dummy for receiving
    con_t *scon;
    int pid_f;

    if (!sighup_blocked)
        block_signal(SIGHUP);
    msglen = rcv_msg(msq_m, BUFSIZE - 1, msgtype, IPC_NOWAIT | MSG_NOERROR, &sock, &cmd, msg);
    if (msglen == -1)
    {
        CHECK_SPECIFIC("Error while receiving message", ENOMSG)
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return 0;
    }
    //work with message - check its type first
    if ((send_only || print_only) && !(send_only && cmd == CMD_SEND) && !(print_only && cmd == CMD_PRINT)) {
        if (!sighup_blocked)
            unblock_signal(SIGHUP);
        return 1; //skip commands
    }
    switch (cmd)
    {
    case CMD_PRINT:
        logwrite(msg, sock);
        break;
    case CMD_SEND:
        //create E and send command
        if (!(scon = sock_findcon(sock)))
        {
            logwrite_int("ERROR: Tried to work with closed socket:", sock, V_MAIN); //sanity check
            break;
        }
        sock_send(sock, msg);
        break;
    case CMD_SHOT:
    case CMD_SAVE:
    case CMD_START:
    case CMD_JOIN:
        //create E and send command
        if (!(scon = sock_findcon(sock)))
        {
            logwrite_int("ERROR: Tried to work with closed socket:", sock, V_MAIN); //sanity check
            break;
        }
        if (!(pid_f = fork()))
            execl("6s_e.exe", "6s_e.exe", str_msq_m, str_msq_w, str_shm, str_shm_sem, NULL);
        logwrite_int("Created new fork (E):", pid_f, V_DEBUG);
        send_msg(msq_w, pid_f, sock, cmd, msg);
        break;
    case CMD_GEN:
        if (!(pid_f = fork()))
            execl("6s_g.exe", "6s_g.exe", str_msq_m, str_msq_w, str_shm, str_shm_sem, NULL);
        logwrite_int("Created new fork (G):", pid_f, V_DEBUG);
        send_msg(msq_w, pid_f, state->p1_sock, CMD_GEN, "");
        //waitpid(pid_f, NULL, 0); //wait for gen to complete
        break;
    default:
        logwrite("Error command:", V_MAIN);
        logwrite(msg, sock);
        break;
    }
    if (!sighup_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

void handler_cfg(int signum)
{
    int old_port = port;

    logwrite("SIGHUP received.", V_MAIN);
    cfgread();
    //reopen log
    logopen();
    logwrite("Log re-opened.", V_MAIN);
    //TODO: change ports.
    port_changed = (old_port != port);
    if (port_changed)
    {
        sock_closeall(0, "Server disconnected.\n"); //sockets
        logwrite("Socket changed.", V_MAIN);
        sock_l = sock_listen(port);
    }
}

void handler_stop(int signum)
{
    logwrite("SIGTERM (SIGINT?) received.", V_MAIN);
    //ending phase: delete all created stuff.
    sock_closeall(0, "Server stopped."); //sockets
    server_stop();                       //all MQ and trash
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
    strcpy(cfgfilename, argv[1]);
    if (argc == 3)
    {
        int vint = atoi(argv[2]);
        if (vint < (int)V_MAIN || vint > (int)V_SCREEN)
        {
            fprintf(stderr, "Wrong verbosity value, supported - from %d to %d\n", (int)V_MAIN, (int)V_SCREEN);
            return -2;
        }
        else
            verb = (verb_t)vint;
    }
    else
        verb = V_ALL;
    //setsid();
    //please no, it's hard to debug after sid()
    //read config file, create log
    cfgread(argv[1]);
    logopen();
    logwrite("Log opened.", V_MAIN);
    //init basic structures
    server_init();
    //open connect socket
    sock_l = sock_listen(port);
    //prepare interrupt handlers
    signal(SIGHUP, handler_cfg);
    signal(SIGTERM, handler_stop);
    signal(SIGINT, handler_stop);
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
        while (check_msg(0, 0, 0))
            ;
        if (port_changed)
            port_changed = 0;
    }
    //ending phase: delete all created stuff.
    sock_closeall(0, "Server stopped."); //sockets
    server_stop();                       //all MQ and trash
    //closing log
    logwrite("Server stopped. Closing the log...", V_MAIN);
    logclose();
    return 0;
}