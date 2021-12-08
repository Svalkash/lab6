#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
    int frk;
    int status;
    int desc_in, desc_out;
    int ret;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file> args...", argv[0]);
        return 3;
    }

    printf("MASTER: Printing arguments:\n");
    for (char **arg = argv; *arg != 0; arg++)
        printf("%s\n", *arg);
    printf("________________________________________\n");
    printf("MASTER: Printing environment variables:\n");
    for (char **env = envp; *env != 0; env++)
        printf("%s\n", *env);
    printf("________________________________________\n");

    printf("Added new var:\n");
    char name[80] = "MYVAR";
    char val[80] = "MYVAL";
    printf("________________________________________\n");

    setenv(name, val, 1);
    printf("%s = %s\n", name, getenv(name));

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
        printf(argv[1]);
        execv(argv[1], &argv[1]);
        perror("Exec: ");
    }
}