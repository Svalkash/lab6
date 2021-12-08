#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int desc;
    char tmp[81];
    int ret;

    if (argc != 2) {
        printf("Wrong use: 2-8_write <filename>");
        return -1;
    }

    printf("\nCreating the file... \n");
    desc = creat(argv[1], 0777);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror("");
    }
    else {
        for (char c = 'a'; c <= 'z'; ++c) {
            for (int i = 0; i < 1000; ++i) 
                write(desc, &c, 1);
            write(desc, "\n", 1);
        }
        close(desc);
    }
}