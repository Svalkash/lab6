#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>


int main(int argc, char *argv[]) {
    int pipes[2];
    int status;


    pipe(pipes);
    if (fork()) {
        close(pipes[1]);
        if (dup2(pipes[0], 0) == -1) //replace std with pipe
            perror("Error during dup: ");
        close(pipes[0]);
        printf("Waiting for the child...");
        wait(&status);
        printf("Done!...");
        execlp("wc", "wc", "-l", NULL);
        close(0);
    }
    else {
        close(pipes[0]);
        if (dup2(pipes[1], 1) == -1) //replace std with pipe
            perror("Error during dup: ");
        close(pipes[1]);
        execlp("who", "who", NULL);
        close(1);
    }
    return 0;
}