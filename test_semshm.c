#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define BUFSIZE 1024

typedef struct aboba {
    int a;
    char boba[7];
};

int
main(int argc, char *argv[])
{
    int sem;
    struct sembuf sop_lock[2] = { 0, 0, 0, 0, 1, 0 };
    struct sembuf sop_unlock[1] = { 0, -1, 0 };
    int shm;
    struct aboba *state;

    sem = semget(IPC_PRIVATE, 1, IPC_CREAT);
    shm = shmget(IPC_PRIVATE, sizeof(struct aboba), IPC_CREAT | 0666);

    state = (struct aboba*)shmat(shm, NULL, 0);

    semop(sem, sop_lock, 2);
    state->a = 6;
    semop(sem, sop_unlock, 1);
    semop(sem, sop_lock, 2);
    printf("%d", state->a);
    semop(sem, sop_unlock, 1);

    shmdt(state);

    semctl(sem, 0, IPC_RMID);
    shmctl(shm, IPC_RMID, NULL);
}