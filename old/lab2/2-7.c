#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int frk;
    int pid;
    int status;

    frk = fork();
    pid = getpid();
    if (frk) {
        printf("Father pid = %d\n", pid);
        printf("Father ppid = %d\n", getppid());
        printf("Father pgid = %d\n", getpgid(pid));
        printf("Father son pid = %d\n", frk);
        printf("Father paused\n");
        pause();
        printf("Father continued\n");
    }
    else {
        printf("Son pid = %d\n", pid);
        printf("Son ppid = %d\n", getppid());
        printf("Son pgid = %d\n", getpgid(pid));
        printf("Son paused\n");
        setpgrp();
        pause();
        printf("Son continued\n");
    }
}