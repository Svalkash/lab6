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

int gotstop = 0;

int readprint(int desc) {
    char tmp[BUF_SIZE];
    int ret;

    ret = read(desc, tmp, BUF_SIZE);
    tmp[ret] = '\0';
    if (strlen(tmp) == 1 && tmp[0] == '$')
        gotstop = 1;
    if (ret > 0) {
        printf("______________________\n");
        printf("%d: Received message:\n", getpid());
        printf("%s\n", tmp);
        printf("______________________\n");
    }
    return ret;
}

int main(int argc, char *argv[]) {
    int fdesc;
    char str[BUF_SIZE];

    if (argc < 2) {
        printf("Usage: %s <filename> <use O_NDELAY>", argv[0]);
        return -1;
    }

    printf("Opening the file...\n");
    if (argc == 3 && atoi(argv[2]))
        fdesc = open(argv[1], O_RDONLY | O_NDELAY);
    else
        fdesc = open(argv[1], O_RDONLY);
    if (fdesc == -1) {
        perror("Error while opening file: ");
        return errno;
    }
    printf("File opened\n");

    printf("Received messages:\n");
    while (!gotstop)
        readprint(fdesc);
    printf("Got $, stopping...:\n");
    close(fdesc);
    printf("File closed\n");

    return 0;
}