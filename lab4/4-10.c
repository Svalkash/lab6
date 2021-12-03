#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 800

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
    int fid;

    if (argc < 3) {
        printf("Usage: %s <mode=0-print,1-scan> <lock>", argv[0]);
        return -1;
    }
    
    printf("\nOpening output file...\n");
    if (fid = fork())
        desc = creat("4-10_p.txt", 0666);
    else
        desc = creat("4-10_c.txt", 0666);
    if (desc == -1) {
        perror("Error opening file: ");
        return errno;
    }

    if (atoi(argv[1]))
        for (int i = 0; i < 100; ++i) {
            if (atoi(argv[2]))
                smart_lock(1, F_WRLCK);
            printf("\n__________________________\n");
            printf("\n%d: Writing big thing\n", getpid());
            for (int p = 0; p <= strlen(str); ++p) {
                for (int c = 'a'; c <= 'z'; ++c) {
                    for (int i = 0; i < 100; ++i)
                        if (fid)
                            printf("%c", c);
                        else
                            printf(" "); //yay, i noticed it
                    printf("\n", str[p]);
                }
            }
            if (atoi(argv[2]))
                smart_unlock(1);
        }
    else
        for (int i = 0; i < 10; ++i) {
            if (atoi(argv[2]))
                smart_lock(0, F_WRLCK);
            printf("\n__________________________\n");
            printf("\n__________________________\n");
            printf("\n%d: Reading big thing\n", getpid());
            scanf("%s", str);
            printf("\n__________________________\n");
            write(desc, str, strlen(str));
            if (atoi(argv[2]))
                smart_unlock(0);
        }

    close(desc);
    printf("\n%d: Closed. \n", getpid());
}