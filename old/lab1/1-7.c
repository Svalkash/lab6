#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int desc;
    char name[80];
    struct stat st;
    int ret;
    printf("Name: ");
    scanf("%s", name);
    ret = stat(name, &st);
    if (ret == -1) {
        printf("errno is %d\n", errno);
        perror(sys_errlist[errno]);
    }
    else {
        printf("ID of device containing file    : %x\n", st.st_dev);
        printf("Inode number                    : %x\n", st.st_ino);
        printf("File type and mode              : %o\n", st.st_mode);
        printf("Number of hard links            : %d\n", st.st_nlink);
        printf("User ID of owner                : %d\n", st.st_uid);
        printf("Group ID of owner               : %d\n", st.st_gid);
        printf("Device ID (if special file)     : %x\n", st.st_rdev);
        printf("Total size, in bytes            : %d\n", st.st_size);
        printf("Block size for filesystem I/O   : %d\n", st.st_blksize);
        printf("Number of 512B blocks allocated : %d\n", st.st_blocks);
        printf("Time of last access             : %d\n", st.st_atim.tv_sec);
        printf("Time of last modification       : %d\n", st.st_mtim.tv_sec);
        printf("Time of last status change      : %d\n", st.st_ctim.tv_sec);
    }
}