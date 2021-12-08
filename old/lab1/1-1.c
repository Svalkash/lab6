#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char *argv[]) {
    int ret;
    char *str;

    str = readline("Enter string: ");
    printf("%s\n", str);
    /*

    ret = open("file1.txt", O_RDONLY);
    if (ret == -1) {
        printf("errno is %d\n", errno);
        perror();
    }
    else {
        printf("good");
        close(ret);
    }
    */
}