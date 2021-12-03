#ifndef __6S_M_FILE_C__
#define __6S_M_FILE_C__

verb_t verb; //how many we want to see in the log

char logname[NAMELEN];
int logfile = -1; //log file desc

int cfgread(char *cfgfilename)
{
    char buf[BUFSIZE];
    int cfgfile; //cfg file desc

    printf("Opening cfg file... \n");
    CHECK(cfgfile = open(cfgfilename, O_RDONLY), -1, "Error while opening config file")
    buf[read(cfgfile, buf, BUFSIZE - 1)] = '\0';
    sscanf(buf, "%d %s", &port, logname);
    printf("Port = %d\n", port);
    printf("Log file name = %s\n", logname);
    printf("Closing cfg file... \n");
    close(cfgfile);
}

int logopen()
{
    char *logfilename;
    time_t t;
    char *timestr;

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
        block_signal(SIGHUP);
    if (logfile != -1)
    {
        logwrite("Closing log file to reopen");
        close(logfile);
    }

    printf("Creating log file... \n");
    time(&t);
    timestr = ctime(&t);
    logfilename = malloc(strlen(logname) + 3 + strlen(timestr) + 4 + 1));
    strcpy(logfilename, logname);
    strcat(logfilename, " - ");
    strcat(logfilename, timestr);
    strcat(logfilename, ".txt");
    CHECK(logfile = creat(logfilename, 0666), -1, "Error while opening log file")
    if (!is_blocked)
        unblock_signal(SIGHUP);
    free(logfilename);
}

int logwrite(char *str, verb_t print_v)
{
    time_t t;
    char *timestr;

    if (print_v > verb)
        return 0;

    int is_blocked = check_mask(SIGHUP);
    if (!is_blocked)
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
    if (!is_blocked)
        unblock_signal(SIGHUP);
    return 1;
}

int logwrite_int(char *str, long num, verb_t print_v)
{
    char outstr[BUFSIZE];

    sprintf(outstr, "%s %d", str, num);
    logwrite(outstr, print_v);
}

int logclose() { close(logfile); }

#endif