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

int printwrite(int desc, char *str) {
    if (strlen(str) > 0) {
        printf("______________________\n");
        printf("%d: Sent message:\n", getpid());
        printf(str);
        printf("\n");
        printf("______________________\n");
    }
    return write(desc, str, strlen(str));
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
        fdesc = open(argv[1], O_WRONLY | O_TRUNC | O_NDELAY);
    else
        fdesc = open(argv[1], O_WRONLY | O_TRUNC);
    if (fdesc == -1) {
        perror("Error while opening file: ");
        return errno;
    }
    printf("File opened\n");

    printf("Input messages here, '$' to stop:\n");
    do {
        scanf("%s", str);
        printwrite(fdesc, str);
    }
    while(!(strlen(str) == 1 && str[0] == '$'));

    close(fdesc);
    printf("File closed\n");

    return 0;
}