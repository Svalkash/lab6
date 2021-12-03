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
    int pipe_in, pipe_out;
    char str[BUF_SIZE*2];

    if (argc < 3) {
        printf("Usage: %s pipe_in pipe_out", argv[0]);
        return -1;
    }

    pipe_in = atoi(argv[1]);
    pipe_out = atoi(argv[2]);
    while (1) {
        str[0] = '\0';
        scanf("%s", str);
        if (strlen(str) == 1 && str[0] =='$')
            break;
        writewaitread(pipe_out, str, pipe_in);
    }
    close(pipe_out);
    close(pipe_in);
    printf("Child: Stopped.\n");
    return 0;
}