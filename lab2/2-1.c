#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char name[80] = "MYVAR";
    char val[80] = "MYVAL";

    setenv(name, val, 1);
    printf("%s = %s\n", name, getenv(name));
}