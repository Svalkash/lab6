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

//check all queues with ipcs

main(int argc, char *argv[])
{
    int msqid;

    if (argc < 2) {
        printf("Usage: %s <queue id>\n", argv[0]);
        return -1;
    }
    
    printf("Removing queue...\n");
    if (msgctl(atoi(argv[1]), IPC_RMID, NULL) == -1)
    switch(errno) {
    case EIDRM:
        printf("ERROR: Queue already removed.\n");
        return errno;
    case EINVAL:
        printf("ERROR: Invalid queue ID.\n");
        return errno;
    case EPERM:
        printf("ERROR: Permission denied\n");
        return errno;
    default:
        perror("Error: ");
        return errno;
    }
    else
        printf("Queue removed.\n");
    return 0;
}