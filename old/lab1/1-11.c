#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    int desc;
    char name[80];
    char buf;
    int my_d = 0;
    int len = -1;
    
    if (argc < 2){
        printf("Usage: %s file1 file2 ...\n", argv[0]);
        return 3;
    }

    for (int i = 1; i < argc; ++i) {
        printf("Opening %s to read..\n", argv[i]);
        desc = open(argv[i], O_RDONLY);
        if (desc == -1) {
            printf("Can't open file %s: errno is %d\n", argv[i], errno);
            perror(sys_errlist[errno]);
        }
        else {
            int pos = lseek(desc, 0, SEEK_END);
            if (pos > len) {
                my_d = i;
                len = pos;
            }
            close(desc);
        }
    }
    printf("Biggest file: %s, length = %d\n", argv[my_d], len);
}
