#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

//*****************************************************
//START IN BACKGROUND
//*****************************************************

void sig_handler(int signum){
    printf("Yay, child dead\n");
}

int main(int argc, char *argv[]) {
    struct sigaction sa;

    printf("Start\n");
    sa.sa_handler = sig_handler;
    //sa_sigaction - можно передать больше информации, в том числе ЗДОРОВЕННУЮ siginfo_t
    sigemptyset((struct sigset_t*)(&sa.sa_mask)); /*a mask of signals which should be blocked
       (i.e., added to the signal mask of the thread in which the signal
       handler is invoked) during execution of the signal handler.*/
    //sa.sa_flags = 0;
    sa.sa_flags = SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &sa, NULL)== -1) {
        perror(0);
        return 1;
    }
    /* Oldact - for saving OLD structure (we give non-null pointer, it gets overridden) */
    if (fork()) {
        sleep(3600);
    }
    else {
        printf("Son lives\n");
        sleep(1);
        printf("Son dead\n");
    }
    return 0;
}