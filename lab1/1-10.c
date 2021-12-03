#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    int desc;
    int pos;
    char name[80];
    char buf;
    printf("name: ");
    scanf("%s", name);
    
    printf("Opening to read..\n");
    desc = open(name, O_RDONLY);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        printf("File contents (reversed): \n");
        pos = lseek(desc, -1, SEEK_END);
        while (pos > 0) {
            read(desc, &buf, 1);
            write(1, &buf, 1);
            pos = lseek(desc, -2, SEEK_CUR);
        }
        printf("\n");
    }
}
