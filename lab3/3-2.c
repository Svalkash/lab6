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
    struct sigaction sa;

    printf("Handler called, changing itself\n");
    sa.sa_handler = SIG_DFL;
    //sa_sigaction - можно передать больше информации, в том числе ЗДОРОВЕННУЮ siginfo_t
    sigemptyset((struct sigset_t*)(&sa.sa_mask));
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    printf("Changed\n");
}

int main(int argc, char *argv[]) {
    struct sigaction sa;

    printf("Start\n");
    sa.sa_handler = sig_handler;
    //sa_sigaction - можно передать больше информации, в том числе ЗДОРОВЕННУЮ siginfo_t
    sigemptyset((struct sigset_t*)(&sa.sa_mask)); /*a mask of signals which should be blocked
       (i.e., added to the signal mask of the thread in which the signal
       handler is invoked) during execution of the signal handler.*/
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    /* Oldact - for saving OLD structure (we give non-null pointer, it gets overridden) */
    printf("Set signal. Raising...\n");
    raise(SIGTERM);
    printf("Raised and survived. Raising one more time...\n");
    raise(SIGTERM);
    printf("Wow.\n");
    return 0;
}