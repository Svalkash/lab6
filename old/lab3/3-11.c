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

//*****************************************************
/*
Дочерний процесс пишет время в файл. Основной процесс периодически открывает тот же файл и пишет туда всякую ерунду.
Сигнал 1 показывает  процессу, что надо остановить запись,
сигнал 2 - перейти в конец и продолжить запись.

Аналогично, ответ на сигнал 1:
сигнал 2 - всё остановлено, можно писать.
*/
//*****************************************************

int block_signal(int signal) {
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigaddset(&mask, signal);
    return sigprocmask(SIG_SETMASK, &mask, NULL);
}

int unblock_signal(int signal) {
    sigset_t mask;

    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigdelset(&mask, signal);
    return sigprocmask(SIG_SETMASK, &mask, NULL);
}

int wait_signal(int signal) {
    sigset_t mask;
    
    sigprocmask(SIG_SETMASK, NULL, &mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigdelset(&mask, signal);
    return sigsuspend(&mask);
}

int can_write = 0;
int desc = -2;
const char name[] = "file11";

void sig_handler_p(int signum){
    //printf("Signal %d, ", signum);
    switch(signum) {
    case SIGUSR2:
        //printf("Parent: SIGUSR2 received\n");
        lseek(desc, 0, SEEK_END);
        can_write = 1;
        break;
    default: break;
    }
    //printf("Parent: Handler done: %d\n", signum);
}

void sig_handler_s(int sig, siginfo_t *info, void *ucontext){
    sigset_t mask;

    printf("Signal %d, ", sig);
    switch(sig) {
    case SIGUSR1:
        //disable can_write
        //printf("Son: SIGUSR1 received\n");
        can_write = 0;
        //printf("Son: SIGUSR2 (response) sent to PID: %d\n", info->si_pid);
        kill(info->si_pid, SIGUSR2); //send response
        break;
    case SIGUSR2:
        //open file, enable can-write
        //printf("Son: SIGUSR2 received\n");
        if (desc == -2) {
            //printf("Son: Opening the file... \n");
            desc = open(name, O_WRONLY);
            if (desc == -1) {
                //printf("errno is %d\n", errno);
                //perror("error opening file: ");
                return 0;
            }
        }
        lseek(desc, 0, SEEK_END);
        can_write = 1;
        break;
    default: break;
    }
    //printf("Son: Handler done: %d\n", sig);
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    int fid;
    int status;
    sigset_t mask;
    int cnt = 0;
    char tstr[80];

    printf("Start\n");
    if (fid = fork()) {
        //set handler for response
        sa.sa_handler = sig_handler_p;
        sigemptyset((struct sigset_t*)(&sa.sa_mask));
        sa.sa_flags = 0;
        if (sigaction(SIGUSR2, &sa, NULL)== -1) {
            perror(0);
            return 1;
        }
        printf("Parent: Creating the file... \n");
        desc = creat(name, 0777);
        if (desc == -1) {
            printf("errno is %d\n", errno);
            perror("error creating file: ");
            return 0;
        }

        sleep(1);
        printf("Parent: sending first SIGUSR2 - 'file is created'...\n");
        kill(fid, SIGUSR2); //ask for sleep

        for (cnt = 0; cnt < 4; ++cnt) {
            printf("Parent: iteration %d, sleeping...\n", cnt);
            sleep(5 + rand()%5);
            printf("Parent: sending SIGUSR1 and waiting for response...\n");
            kill(fid, SIGUSR1); //ask for sleep
            wait_signal(SIGUSR2); //wait for response
            can_write = 1;
            printf("Parent: response received, writing...\n");
            //write to file
            write(desc, "________________________________________\n", 41);
            write(desc, "PARENT WRITING\n", 15);
            write(desc, "________________________________________\n", 41);
            sleep(1);
            write(desc, "NEVER GONNA GIVE YOU UP\n", 24);
            sleep(1);
            write(desc, "NEVER GONNA LET YOU DOWN\n", 25);
            sleep(1);
            write(desc, "NEVER GONNA RUN AROUND\n", 23);
            sleep(1);
            write(desc, "AND DESERT YOU\n", 14);
            printf("Parent: write finished, sending SIGUSR2...\n");
            kill(fid, SIGUSR2); //allow to continue
        }
        close(desc);
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
        //setpgrp();
        //set handlers
        sa.sa_handler = sig_handler_s;
        sigemptyset((struct sigset_t*)(&sa.sa_mask));
        sigaddset((struct sigset_t*)(&sa.sa_mask), SIGUSR2);
        sa.sa_flags = SA_SIGINFO;
        if (sigaction(SIGUSR1, &sa, NULL)== -1) {
            perror(0);
            return 1;
        }
        sigemptyset((struct sigset_t*)(&sa.sa_mask));
        sigaddset((struct sigset_t*)(&sa.sa_mask), SIGUSR1);
        if (sigaction(SIGUSR2, &sa, NULL)== -1) {
            perror(0);
            return 1;
        }
        
        for (cnt = 0; cnt < 20;)
            if (can_write) {
                //lock SIGUSR1 signals (critical part)
                printf("Son: Entering the critical part, disabling signals...\n");
                block_signal(SIGUSR1);
                block_signal(SIGUSR2);
                printf("Son: Writing, iter %d...\n", cnt);
                write(desc, "________________________________________\n", 41);
                write(desc, "SON WRITING\n", 12);
                write(desc, "________________________________________\n", 41);
                for (int i = 0; i < 3; ++i) {
                    sleep(1);
                    snprintf(tstr, 80, "%d\n", time(NULL));
                    write(desc, tstr, strlen(tstr));
                }
                printf("Son: Exiting the critical part...\n");
                unblock_signal(SIGUSR2);
                unblock_signal(SIGUSR1);
                ++cnt;
            }
            else {
                printf("Son: Waiting for a permission to continue...\n");                
                wait_signal(SIGUSR2); //wait for permission
                printf("Son: Got permission\n");
            }
        close(desc);
        printf("Son dead\n");
        return 0;
    }
}