#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char *argv[]) {
    int frk;
    int status;
    int desc_in, desc_out;
    int ret;
    int comlen = 0;
    char *com;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s command params...", argv[0]);
        return 3;
    }

    printf("MASTER: Printing arguments:\n");
    for (char **arg = argv; *arg != 0; ++arg)
        printf("%s\n", *arg);

    comlen = strlen(argv[1]);
    for (int i = 1; i < argc; ++i)
        comlen += 1 + strlen(argv[i]);
    ++comlen; //for \0
    com = malloc(comlen * sizeof(char));
    strcpy(com, argv[1]);
    for (int i = 2; i < argc; ++i) {
        strcat(com, " ");
        strcat(com, argv[i]);
    }
        printf("Executing command: '%s'\n", com);
        system(com);
/*
    frk = fork();
    if (frk) {
        wait(&status);
        if (WIFEXITED(status))
            printf("exited, status=%d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("killed by signal %d\n", WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("stopped by signal %d\n", WSTOPSIG(status));
        else if (WIFCONTINUED(status))
            printf("continued\n");
        return 0;
    }
    else {
        printf("Executing command: '%s'\n", com);
        system(com);
    }
    */
}