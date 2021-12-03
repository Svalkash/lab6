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

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
    char input[80] = "ararstart 1\n     boastop 2\n     .sta";
    char new_cmd[80] = "rt 3\n st!op !4  \n";
    int zone;
    char *cmd;
    char *cmdend, *cmdstart;

    cmd = calloc(strlen(input) + 1, sizeof(char));
    strcpy(cmd, input);

    cmdstart = cmd;

    while (*cmdstart != '\0') {
        cmdend = strchr(cmdstart, '\n');
        while (!cmdend) {
            int startpos = cmdstart - cmd; //remember startpos (realloc changes ptrs)

            ; //lock until new message arrives and then get new_cmd (new command)
            cmd = realloc(cmd, strlen(cmd) + strlen(new_cmd) + 1);
            strcat(cmd, new_cmd);
            cmdstart = cmd + startpos; //correct startpos
            cmdend = strchr(cmdstart, '\n');
        }
        //mark the end of the command
        *cmdend = '\0';
        //interpret
        printf("Command: '%s'\n", cmdstart);
        if (sscanf(cmdstart, "start %d", &zone) == 1)
            printf("good start, zone %d\n", zone);
        else if (sscanf(cmdstart, "stop %d", &zone) == 1)
            printf("good stop, zone %d\n", zone);
        else
            printf("nope\n");
        //unmark
        *cmdend = '\n';
        //next cmd of \0
        cmdstart = cmdend + 1;
        while (*cmdstart != '\0' && (*cmdstart == ' ' || *cmdstart == '\t')) //skip spaces at the beginning/end of commands
            ++cmdstart;
    }
    printf("done\n");
}