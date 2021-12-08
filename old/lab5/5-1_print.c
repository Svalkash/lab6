#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>

#define BUFSIZE 80

main(int argc, char *argv[])
{
    int msqid;
    struct msqid_ds msqinfo;

    if (argc < 2) {
        printf("Usage: %s <key>\n", argv[0]);
        return -1;
    }
    
    printf("\nOpening queue...\n");
    if ((msqid = msgget(atoi(argv[1]), IPC_CREAT | 0666)) == -1) {
        perror("Error: ");
        return errno;
    }

    printf("\nPrinting info...\n");
    if (msgctl(msqid, IPC_STAT, &msqinfo) == -1) {
        perror("Error: ");
        return errno;
    }
    printf("________________________________________\n");
    printf("Ownership and permissions:\n");
    printf("Key supplied to msgget = %d\n", msqinfo.msg_perm.__key);
    printf("Effective UID of owner = %d\n", msqinfo.msg_perm.uid);
    printf("Effective GID of owner = %d\n", msqinfo.msg_perm.gid);
    printf("Effective UID of creator = %d\n", msqinfo.msg_perm.cuid);
    printf("Effective GID of creator = %d\n", msqinfo.msg_perm.cgid);
    printf("Permissions = %o\n", msqinfo.msg_perm.mode);
    printf("Sequence number = %d\n", msqinfo.msg_perm.__seq);
    printf("________________________________________\n");
    printf("Time of last msgsnd = %d\n", msqinfo.msg_stime);
    printf("Time of last msgrcv = %d\n", msqinfo.msg_rtime);
    printf("Time of creation or last modification by msgctl = %d\n", msqinfo.msg_ctime);
    printf("# of bytes in queue = %d\n", msqinfo.msg_cbytes);
    printf("# number of messages in queue = %d\n", msqinfo.msg_qnum);
    printf("Maximum # of bytes in queue = %d\n", msqinfo.msg_qbytes);
    printf("PID of last msgsnd = %d\n", msqinfo.msg_lspid);
    printf("PID of last msgrcv = %d\n", msqinfo.msg_lrpid);
    printf("________________________________________\n");
    return 0;
}