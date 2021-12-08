#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

void sig_handler(int signum){
    printf("Signal %d, ", signum);
    switch(signum) {
    case SIGTERM:
        printf("SIGTERM received\n");
        break;
    case SIGALRM:
        printf("SIGALRM received\n");
        break;
    case SIGUSR1:
        printf("SIGUSR1 received\n");
        break;
    case SIGUSR2:
        printf("SIGUSR2 received\n");
        break;
    default: break;
    }
    sleep(5);
    printf("Handler done: %d\n", signum);
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    int fid;
    int status;

    printf("Start\n");
    //SIGUSR1,2 < SIGALRM < SIGTERM
    sa.sa_handler = sig_handler;
    sigemptyset((struct sigset_t*)(&sa.sa_mask));
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL)== -1) {
        perror(0);
        return 1;
    }
    if (sigaction(SIGUSR2, &sa, NULL)== -1) {
        perror(0);
        return 1;
    }
    sigaddset((struct sigset_t*)(&sa.sa_mask), SIGUSR1);
    sigaddset((struct sigset_t*)(&sa.sa_mask), SIGUSR2);
    if (sigaction(SIGALRM, &sa, NULL)== -1) {
        perror(0);
        return 1;
    }
    sigaddset((struct sigset_t*)(&sa.sa_mask), SIGALRM);
    if (sigaction(SIGTERM, &sa, NULL)== -1) {
        perror(0);
        return 1;
    }
    if (fid = fork()) {
        if (argc == 1 || atoi(argv[1]) == 0) {
            printf("Parent continues...\n");
            sleep(1);
            printf("Sending SIGUSR1\n");
            kill(fid, SIGUSR1);
            sleep(1);
            printf("Sending SIGUSR1\n");
            kill(fid, SIGUSR1);
            sleep(1);
            printf("Sending SIGUSR2\n");
            kill(fid, SIGUSR2);
            sleep(3);
            printf("Sending SIGTERM\n");
            kill(fid, SIGTERM);
        }
        else if (atoi(argv[1]) == 1) {
            printf("Parent continues...\n");
            sleep(5);
            printf("Sending SIGUSR1\n");
            kill(fid, SIGUSR1);
            sleep(1);
            printf("Sending SIGUSR1\n");
            kill(fid, SIGUSR1);
            sleep(1);
            printf("Sending SIGTERM\n");
            kill(fid, SIGTERM);
            sleep(1);
            printf("Sending SIGUSR2\n");
            kill(fid, SIGUSR2);
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
        printf("Son lives\n");
        alarm(4);
        sleep(20);
        printf("Son dead\n");
    }
    return 0;
}