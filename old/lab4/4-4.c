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

#define BUF_SIZE 40

int no_more_info = 0;

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

int writewaitread(int descwr, char *str, int descrd) {
    int ret;

    printwrite(descwr, str);
    while ((ret = readprint(descrd)) == BUF_SIZE) //waiting for response
        ;
    return ret;
}

int waitreadwrite(int descwr, char *str, int descrd) {
    int ret;

    while ((ret = readprint(descrd)) == BUF_SIZE) //waiting for response
        ;
    if (ret != 0)
        printwrite(descwr, str);
    return ret;
}

int main(int argc, char *argv[]) {
    int pipes[2][2];
    int fid;
    char str[BUF_SIZE*2];
    char descstr_in[BUF_SIZE];
    char descstr_out[BUF_SIZE];

    pipe(pipes[0]);
    pipe(pipes[1]);
    if (fid = fork()) {
            close(pipes[0][1]);
            close(pipes[1][0]);
            while (waitreadwrite(pipes[1][1], "response", pipes[0][0]));
                ;
        close(pipes[0][0]);
        close(pipes[1][1]);
        printf("Parent: Stopped.\n");
    }
    else {
        close(pipes[0][0]);
        close(pipes[1][1]);
        sprintf(descstr_in,"%d",pipes[1][0]);
        sprintf(descstr_out,"%d",pipes[0][1]);
        execl("4-4_1", "4-4_1", descstr_in, descstr_out, NULL);
        perror("not exec: ");

    }
    return 0;
}