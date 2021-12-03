#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (fork()) {
        printf("father1\n");
        printf("father2\n");
        printf("father3\n");
    }
    else {
        printf("son1\n");
        printf("son2\n");
        printf("son3\n");
    }
}