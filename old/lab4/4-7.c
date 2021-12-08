#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 80

int readprint(int desc) {
    char tmp[BUF_SIZE];
    int ret;

    ret = read(desc, tmp, BUF_SIZE);
    tmp[ret] = '\0';
    if (ret > 0)
        printf("%s", tmp);
    return ret;
}

int smart_lock(int desc, short type) {
    struct flock fl, flg;

    fl.l_type = type;/* Type of lock: F_RDLCK, F_WRLCK, F_UNLCK */
    fl.l_whence = SEEK_SET;/* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start = 0; /* Starting offset for lock */
    fl.l_len = 0; /* Number of bytes to lock, 0 - all */
    fl.l_pid = getpid(); /* PID of process blocking our lock (set by F_GETLK and F_OFD_GETLK) */
    flg = fl;

    printf("\n%d: Testing for lock... \n", getpid());
    fcntl(desc, F_GETLK, &flg);
    if (flg.l_type == F_UNLCK)
        printf("\n%d: Lock is free, taking it!\n", getpid());
    else
        printf("\n%d: Process is locked by %d, waiting...\n", flg.l_pid, getpid());
    fcntl(desc, F_SETLKW, &fl);
    printf("\n%d: Got lock! \n", getpid());
}

int smart_unlock(int desc) {
    struct flock fl, flg;

    fl.l_type = F_UNLCK;/* Type of lock: F_RDLCK, F_WRLCK, F_UNLCK */
    fl.l_whence = SEEK_SET;/* How to interpret l_start: SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start = 0; /* Starting offset for lock */
    fl.l_len = 0; /* Number of bytes to lock, 0 - all */
    fl.l_pid = getpid(); /* PID of process blocking our lock (set by F_GETLK and F_OFD_GETLK) */

    if (fcntl(desc, F_SETLK, &fl) == -1)
        perror("Couldn't unlock: ");
    printf("\n%d: Unlocked!\n", getpid());
}


int main(int argc, char *argv[]) {
    int desc;
    char str[BUF_SIZE];

    if (argc < 3) {
        printf("Usage: %s <filename> <7/8/9 task> <print: 0 - no, 1 - 0/1, 2 - all>", argv[0]);
        return -1;
    }
    
    printf("\nOpening output file...\n");
    close(1);
    desc = creat("4-7_out.txt", 0666);
    if (desc != 1) {
        perror("Error opening file: ");
        return errno;
    }

    if (fork()) {
        printf("\nParent: Creating the file... \n");
        desc = creat(argv[1], 0666);
        if (desc == -1) {
            perror("Error opening file: ");
            return errno;
        }
        //locking
        if (atoi(argv[2]) > 7)
            smart_lock(desc, F_WRLCK);

        printf("\nParent: Opened. \n");
        for (int i = 0; i < 1000000; ++i) {
            sprintf(str, "%d|", i);
            write(desc, str, strlen(str));
            if (argc == 4 && atoi(argv[3]) == 1)
                printf("1");
        }
        if (atoi(argv[2]) > 7)
            smart_unlock(desc);
        close(desc);
        printf("\nParent: Closed. \n");
            
    }
    else {
        sleep(1);
        printf("\nChild: Opening the file... \n");
        desc = open(argv[1], O_RDONLY);
        if (desc == -1) {
            perror("Error opening file: ");
            return errno;
        }
        //locking
        if (atoi(argv[2]) > 8)
            smart_lock(desc, F_RDLCK);
        printf("\nChild: Opened. \n");
        if (argc < 4 || atoi(argv[3]) == 2)
            while (readprint(desc))
                ;
        else
            while (read(desc, str, BUF_SIZE))
                if (argc == 4 && atoi(argv[3]) == 1)
                    printf("0");
        if (atoi(argv[2]) > 8)
            smart_unlock(desc);
        close(desc);
        printf("\nChild: Closed. \n");
    }
}