#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
    int frk;
    int status;
    int desc_in, desc_out;
    int ret;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file> args...", argv[0]);
        return 3;
    }

    printf("SLAVE: Printing arguments:\n");
    for (char **arg = argv; *arg != 0; arg++)
        printf("%s\n", *arg);
    printf("________________________________________\n");
    printf("SLAVE: Printing environment variables:\n");
    for (char **env = envp; *env != 0; env++)
        printf("%s\n", *env);
    printf("________________________________________\n");

    return 12;
}