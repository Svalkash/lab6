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
#include "6s_lib.h"

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
    for (int i = 0; i < 10; ++i)
        printf("%d ", randint());
}