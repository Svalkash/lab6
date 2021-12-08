#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>


int main(int argc, char *argv[]) {
    remove("pipe6");
    mknod("pipe6", 0666, 0);
    return 0;
}