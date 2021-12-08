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

int digits_only(char *str) {
    char *ptr = str;
    while (*ptr != '\0') {
        printf("%c", *ptr);
        if (*ptr < '0' || *ptr > '9')
            return 0;
        ++ptr;
    }
    return 1;
}

int decode(char *cmdstr) {
    char *cmd, *arg, *trash;

    //decode command
    cmd = strtok(cmdstr, " \t");
    arg = strtok(NULL, " \t");
    trash = strtok(NULL, " \t");

    printf("Decoding command:");
    printf(cmdstr);
    if (!cmd)
    {
        printf("[ERROR] Empty command!");
        return 0;
    }
    if (!arg) {
        printf("[ERROR] No arguments for the command!");
        return 0;
    }
    if (trash) {
        printf("[ERROR] Trash at the end of the command!");
        return 0;
    }
    if ((!strcmp(cmd, "shot") || !strcmp(cmd, "save")) && !digits_only(arg)) {
        printf("[ERROR] Invalid zone number!");
        return 0;
    }
    if (!strcmp(cmd, "start")) {
        printf("Received start");
    return 1;
    }
    if (!strcmp(cmd, "join")) {
        printf("Received join");
    return 1;
        
    }
    if (!strcmp(cmd, "shot")) {
        printf("Received shot");
    return 1;
        
    }
    if (!strcmp(cmd, "save")) {
        printf("Received save");
    return 1;
        
    }
    printf("unknown command");
    return 0;
}

int main(int argc, char *argv[])
{
    char input[80] = "save 45  \n";
    char new_cmd[80] = "rt 3\n st!op !4  \n";
    int zone;
    char buf[80];
    char *cmd;
    char *cmdend, *cmdstart;
    int scand = 0;

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
        decode(cmdstart);
        //unmark
        *cmdend = '\n';
        //next cmd of \0
        cmdstart = cmdend + 1;
        while (*cmdstart != '\0' && (*cmdstart == ' ' || *cmdstart == '\t')) //skip spaces at the beginning/end of commands
            ++cmdstart;
    }
    printf("done\n");
}