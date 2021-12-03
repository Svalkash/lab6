#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int copy(int desc_in, int desc_out) {
    int ret;
    char tmp[80];
    while ((ret = read(desc_in, tmp, 80)) > 0)
        write(desc_out, tmp, ret);
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
        desc_out = creat(argv[2], 0777);
        if (desc_out == -1) {
            printf("argv[2]: errno = %d\n", errno);
            perror(sys_errlist[errno]);
            return 2;
        }
    }
    else
        desc_out = 1;
    if (argc > 1) {
        desc_in = open(argv[1], O_RDONLY);
        if (desc_in == -1) {
            printf("argv[1]: errno = %d\n", errno);
            perror(sys_errlist[errno]);
            return 1;
        }
    }
    else
        desc_in = 0;
    ret = copy(desc_in, desc_out);
    if (ret == 0)
        printf("\nCopied successfully\n");
    else
        printf("\nError while copying\n");
    close(desc_in);
    close(desc_out);
}