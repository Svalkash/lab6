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

int no_more_info = 0;

void sig_handler(int signum){
    no_more_info = 1;
}

int readprint(int desc) {
    char tmp[80];
    int ret;

    ret = read(desc, tmp, 80);
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
    while (!readprint(descrd)) //waiting for response
        ;
    while (ret = readprint(descrd) > 0) //reading it all
        ;
    if (ret == -1 && errno != EAGAIN)
        perror("Error reading: ");
}

void waitreadwrite(int descwr, char *str, int descrd) {
    int ret;

    while (!readprint(descrd)) //waiting for response
        ;
    while (ret = readprint(descrd) > 0) //reading it all
        ;
    if (ret == -1 && errno != EAGAIN)
        perror("Error reading: ");
    printwrite(descwr, str);
}

int main(int argc, char *argv[]) {
    char str[80];
    int pipes[2][2];
    int fid;
    int desc_out;

    if (argc < 2) {
        printf("Usage: %s 1/2 (directions) <log_file>", argv[0]);
        return -1;
    }

    close(1); //wow, reopening it moves file into this place? cool
    if (argc >= 3)
        desc_out = creat(argv[2], 0777);
    else
        desc_out = creat("4-1_log.txt", 0777);
    if (desc_out == -1) {
        printf("argv[2]: errno = %d\n", errno);
        perror(sys_errlist[errno]);
        return 2;
    }

    if (atoi(argv[1]) == 1) {
        signal(SIGUSR1, sig_handler);
        pipe(pipes[0]);
        if (fid = fork()) {
            close(pipes[0][1]);
            while (!no_more_info || readprint(pipes[0][0]))
                ;
            close(pipes[0][0]);
            printf("Parent: SIGUSR1 received, stopped reading.\n");

        }
        else {
            close(pipes[0][0]);
            printwrite(pipes[0][1], "NEVER GONNA GIVE YOU UP\n");
            printwrite(pipes[0][1], "NEVER GONNA LET YOU DOWN\n");
            printwrite(pipes[0][1], "NEVER GONNA RUN AROUND\n");
            printwrite(pipes[0][1], "AND DESERT YOU\n");
            close(pipes[0][1]);
            printf("Child: Stopped writing, sending SIGUSR1.\n");
            kill(getppid(), SIGUSR1);

        }
    }
    else if (atoi(argv[1]) == 2) {
        pipe2(pipes[0], O_NONBLOCK);
        pipe2(pipes[1], O_NONBLOCK);
        if (fid = fork()) {
            close(pipes[0][1]);
            close(pipes[1][0]);
            waitreadwrite(pipes[1][1], "I'm Pupa. And you?", pipes[0][0]);
            waitreadwrite(pipes[1][1], "I got a big salary", pipes[0][0]);
            waitreadwrite(pipes[1][1], "Nope", pipes[0][0]);
            close(pipes[0][0]);
            close(pipes[1][1]);
            printf("Parent: Stopped.\n");
            printf("Child: Stopped.\n");
        }
        else {
            close(pipes[0][0]);
            close(pipes[1][1]);
            writewaitread(pipes[0][1], "Who are you?", pipes[1][0]);
            writewaitread(pipes[0][1], "I'm Lupa.", pipes[1][0]);
            writewaitread(pipes[0][1], "THAT'S MY! GIVE IT BACK", pipes[1][0]);
            close(pipes[0][1]);
            close(pipes[1][0]);
            printf("Parent: Stopped.\n");
            printf("Child: Stopped.\n");
        }
    }
    return 0;
}