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
    printf("Handler called, changing itself\n");
    signal(SIGTERM, SIG_DFL);
    printf("Changed\n");
}

int main(int argc, char *argv[]) {
    printf("Start\n");
    signal(SIGTERM, sig_handler);
    printf("Set signal. Raising...\n");
    raise(SIGTERM);
    printf("Raised and survived. Raising one more time...\n");
    raise(SIGTERM);
    printf("Wow.\n");
    return 0;
}