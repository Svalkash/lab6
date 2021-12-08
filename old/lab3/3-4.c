#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define CNT_MAX 100
int cnt = 0;

void sig_handler(int signum){
    printf("Handler called\n");
    cnt = CNT_MAX;
}

int main(int argc, char *argv[]) {
    int status, fid;

    if (argc < 4) {
        printf("Usage: %s task(4-6/9) pause(1)/cycles(0) handler(1)/no(0)", argv[0]);
        return -1;
    }


    printf("Start\n");
    if (fid = fork()) {
        if (atoi(argv[1]) == 5) {
            for (int i = 0; i < CNT_MAX/2; ++i)
                for (int i1 = 0; i1 < 10000; ++i1)
                    for (int i2 = 0; i2 < 1000; ++i2)
                        ;
            kill(fid, SIGUSR1);
        }
        else if (atoi(argv[1]) == 6) {
            for (int i = 0; i < CNT_MAX/2; ++i)
                for (int i1 = 0; i1 < 10000; ++i1)
                    for (int i2 = 0; i2 < 1000; ++i2)
                        ;
            kill(fid, SIGTERM);
        }

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
        if (atoi(argv[3]) == 1) {
            signal(SIGUSR1, sig_handler);
            signal(SIGTERM, sig_handler);
            signal(SIGALRM, sig_handler);
        }
        if (atoi(argv[1]) == 9)
            alarm(2);
        printf("Son lives\n");
        if (atoi(argv[2]) == 0) {
            for (; cnt < CNT_MAX; ++cnt) {
                for (int i1 = 0; i1 < 10000; ++i1)
                    for (int i2 = 0; i2 < 1000; ++i2)
                        ;
                printf("%d\n", cnt);
            }
        }
        else {
            pause();
        }
        printf("Son dead\n");
        return 12;
    }
}