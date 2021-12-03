#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int copy(int desc_in, int desc_out) {
    int ret;
    char tmp[80];
    while ((ret = read(desc_in, tmp, 80)) > 0)
        write(desc_out, tmp, ret);
    return ret;
}

int main(int argc, char *argv[]) {
    int frk;
    int pid;
    int status;
    int desc_in, desc_out;
    int ret;
    char name[80];

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: <filename (in)> <out_file>", argv[0]);
        return 3;
    }
    desc_in = open(argv[1], O_RDONLY);
    if (desc_in == -1) {
        printf("argv[1]: errno = %d\n", errno);
        perror(sys_errlist[errno]);
        return 1;
    }

    frk = fork();
    pid = getpid();

    if (argc == 3) {
        strcpy(name, argv[2]);
        if (frk)
            strcat(name, "_p");
        else 
            strcat(name, "_s");
        desc_out = creat(name, 0777);
        if (desc_out == -1) {
            printf("argv[2]: errno = %d\n", errno);
            perror(sys_errlist[errno]);
            return 2;
        }
    }
    else
        desc_out = 1;
    ret = copy(desc_in, desc_out);
    if (ret == 0)
        printf("\nCopied successfully\n");
    else
        printf("\nError while copying\n");
    close(desc_out);
    close(desc_in);
}