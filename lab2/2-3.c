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
        wait(&status);
        if (WIFEXITED(status))
            printf("exited, status=%d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("killed by signal %d\n", WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("stopped by signal %d\n", WSTOPSIG(status));
        else if (WIFCONTINUED(status))
            printf("continued\n");
    }
    else {
        printf("Son pid = %d\n", pid);
        printf("Son ppid = %d\n", getppid());
        printf("Son pgid = %d\n", getpgid(pid));
        return (7);
    }
}