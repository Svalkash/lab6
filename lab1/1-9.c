#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int copy() {
    int ret;
    char tmp[80];
    while ((ret = read(0, tmp, 80)) > 0)
        write(1, tmp, ret);
    return ret;
}

int main(int argc, char *argv[]) {
    int desc_in, desc_out;
    int ret;
    
    if (argc > 3){
        fprintf(stderr, "Usage: %s filein fileout\n", argv[0]);
        return 3;
    }
    if (argc > 2) {
        close(1); //wow, reopening it moves file into this place? cool
        desc_out = creat(argv[2], 0777);
        if (desc_out == -1) {
            printf("argv[2]: errno = %d\n", errno);
            perror(sys_errlist[errno]);
            return 2;
        }
    }
    if (argc > 1) {
        close(0);
        desc_in = open(argv[1], O_RDONLY);
        if (desc_in == -1) {
            printf("argv[1]: errno = %d\n", errno);
            perror(sys_errlist[errno]);
            return 1;
        }
    }
    ret = copy();
    // if (ret == 0)
    //     printf("\nCopied successfully\n");
    // else
    //     printf("\nError while copying\n");

    //hehe, printing doesn't work either
    close(desc_in);
    close(desc_out);
}