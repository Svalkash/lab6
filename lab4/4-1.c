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

#define BUF_SIZE 80

int no_more_info = 0;

/*
void sig_handler(int signum){
    no_more_info = 1;
}
*/

int readprint(int desc) {
    char tmp[BUF_SIZE];
    int ret;

    ret = read(desc, tmp, BUF_SIZE);
    tmp[ret] = '\0';
    if (ret > 0) {
        printf("______________________\n");
        printf("%d: Received data:\n", getpid());
        printf("%s\n", tmp);
        printf("______________________\n");
    }
    return ret;
}

int printwrite(int desc, char *str) {
    if (strlen(str) > 0) {
        printf("______________________\n");
        printf("%d: Sent data:\n", getpid());
        printf(str);
        printf("\n");
        printf("______________________\n");
    }
    return write(desc, str, strlen(str));
}

void writewaitread(int descwr, char *str, int descrd) {
    int ret;

    printwrite(descwr, str);
    while (readprint(descrd) == BUF_SIZE) //waiting for response
        ;
}

void waitreadwrite(int descwr, char *str, int descrd) {
    int ret;

    while (readprint(descrd) == BUF_SIZE) //waiting for response
        ;
    printwrite(descwr, str);
}

int main(int argc, char *argv[]) {
    char str[BUF_SIZE];
    int pipes[2][2];
    int fid;
    int desc_out;

    if (argc < 2) {
        printf("Usage: %s 1/2 (directions)", argv[0]);
        return -1;
    }

    if (atoi(argv[1]) == 1) {
        //signal(SIGUSR1, sig_handler);
        pipe(pipes[0]);
        if (fid = fork()) {
            close(pipes[0][1]);
            //while (!no_more_info || readprint(pipes[0][0]))
            while (readprint(pipes[0][0]))
                ;
            close(pipes[0][0]);
            printf("Parent: Stopped reading.\n");

        }
        else {
            close(pipes[0][0]);
            printwrite(pipes[0][1], "NEVER GONNA GIVE YOU UP\n");
            printwrite(pipes[0][1], "NEVER GONNA LET YOU DOWN\n");
            printwrite(pipes[0][1], "NEVER GONNA RUN AROUND\n");
            printwrite(pipes[0][1], "AND DESERT YOU\n");
            close(pipes[0][1]);
            printf("Child: Stopped writing.\n");
            //kill(getppid(), SIGUSR1);

        }
    }
    else if (atoi(argv[1]) == 2) {
        pipe(pipes[0]);
        pipe(pipes[1]);
        if (fid = fork()) {
            close(pipes[0][1]);
            close(pipes[1][0]);
            waitreadwrite(pipes[1][1], "I'm Pupa. And you?", pipes[0][0]);
            waitreadwrite(pipes[1][1], "I got a big salary", pipes[0][0]);
            waitreadwrite(pipes[1][1], "Nope", pipes[0][0]);
            close(pipes[0][0]);
            close(pipes[1][1]);
            printf("Parent: Stopped.\n");
        }
        else {
            close(pipes[0][0]);
            close(pipes[1][1]);
            writewaitread(pipes[0][1], "Who are you?", pipes[1][0]);
            writewaitread(pipes[0][1], "I'm Lupa.", pipes[1][0]);
            writewaitread(pipes[0][1], "THAT'S MY! GIVE IT BACK", pipes[1][0]);
            close(pipes[0][1]);
            close(pipes[1][0]);
            printf("Child: Stopped.\n");
        }
    }
    return 0;
}