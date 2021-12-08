#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int readprint(int desc) {
    char tmp[10];
    int ret;

    ret = read(desc, tmp, 10);
    tmp[ret] = '\0';
    printf("%s", tmp);
    return ret;
}

int main(int argc, char *argv[]) {
    int desc;
    char name[80];
    int mod;
    char tmp[81];
    int ret;
    printf("name: ");
    scanf("%s", name);
    printf("mode: ");
    scanf("%o", &mod);
    //NEVER
    printf("\nCreating the file... \n");
    desc = creat(name, mod);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        desc = creat(name, mod);
        lseek(desc, 0, SEEK_SET);
        write(desc, "NEVER GONNA GIVE YOU UP\n", 24);
        write(desc, "NEVER GONNA LET YOU DOWN\n", 25);
        write(desc, "NEVER GONNA RUN AROUND\n", 23);
        write(desc, "AND DESERT YOU", 14);
        //GONNA
        close(desc);
    }

    printf("Opening to read..\n");
    desc = open(name, O_RDONLY);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        printf("File contents (random): \n");
        lseek(desc, 5, SEEK_SET);
        readprint(desc);
        lseek(desc, 15, SEEK_CUR);
        readprint(desc);
        lseek(desc, -20, SEEK_END);
        readprint(desc);
        close(desc);
    }

    printf("\nOpening to write... \n");
    desc = open(name, O_RDWR);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        lseek(desc, 1000000, SEEK_END);
        write(desc, "\n\nNEVER GONNA LET YOU CRY\n", 26);
        write(desc, "NEVER GONNA SAY GOODBUY\n", 24);
        write(desc, "NEVER GONNA TELL A LIE\n", 23);
        write(desc, "AND HURT YOU", 12);
        close(desc);
    }

    printf("Opening to read..\n");
    desc = open(name, O_RDONLY);
    if (desc == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        printf("File contents: \n");
        desc = open(name, O_RDONLY);
        while (readprint(desc) > 0)
            ;
        close(desc);
    }

}