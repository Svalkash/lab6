#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "6s_types.h"

#define BUFSIZE 1024
#define NAMELEN 80

#define CHECK(fun, errval, msg) \
    {                           \
        if ((fun) == errval)    \
        {                       \
            perror(msg);        \
            exit(errno);        \
        }                       \
    }

int port;
char logfilename[NAMELEN];

int main(int argc, char *argv[])
{
    char cfgfilename[NAMELEN] = "cfg.txt";
    char buf[BUFSIZE];

    int cfgfile; //cfg file desc

    printf("Opening cfg file... \n");
    CHECK(cfgfile = open(cfgfilename, O_RDONLY), -1, "Error while opening config file")
    lseek(cfgfile, 0, SEEK_SET);
    buf[read(cfgfile, buf, BUFSIZE - 1)] = '\0';
    sscanf(buf, "%d %s", &port, logfilename);
    printf("Port = %d\n", port);
    printf("Log file name = %s\n", logfilename);
    printf("Closing cfg file... \n");
}