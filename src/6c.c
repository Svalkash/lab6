#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

//Я сделал красивый макрос для вывода ошибок, но мне лень сразу засорять этим код. Я потом добавлю, если необходимо.
#define CHECK(fun, errval, msg) \
    {                           \
        if ((fun) == errval)    \
        {                       \
            perror(msg);        \
            exit(errno);        \
        }                       \
    }

struct in_addr lookup_host(const char *host)
{
    struct addrinfo hints, *res;
    int errcode;
    void *ptr;
    char buf[BUFSIZE];
    struct in_addr retval;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    CHECK(getaddrinfo(host, NULL, &hints, &res), -1, "Can't retrieve address info")


    printf("Host: %s\n", host);
    retval = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    //here it should be usable for sockets
    inet_ntop(res->ai_family, &retval, buf, BUFSIZE - 1);
    printf("IPv4 address: %s\n", buf);
    freeaddrinfo(res);
    return retval;
}
/*
int is_num(char *str)
{
    while (*str)
    {
        if (*str < '0' || *str > '9')
            return 0;
        ++str;
    }
    return 1;
}

int check_ip(char *ip)
{ //check whether the IP is valid or not
    int i, num, dots = 0;
    char *ptr;

    if (ip == NULL)
        return 0;
    ptr = strtok(ip, ".");
    if (ptr == NULL)
        return 0;
    while (ptr)
    {
        if (!validate_number(ptr))
            return 0;
        num = atoi(ptr); //convert substring to number
        if (num >= 0 && num <= 255)
        {
            ptr = strtok(NULL, ".");
            if (ptr != NULL)
                dots++;
        }
        else
            return 0;
    }
    if (dots != 3)
        return 0;
    return 1;
}
*/

int check_disconnect(char *str) {
    char *t = strtok(str, " \t");

    return !t || strcmp(t, "disconnect\n") ? 0 : 1;
}

int main(int argc, char *argv[])
{
    int ret;
    int sock;
    struct sockaddr_in sa_srv;
    int rcv_sz;
    char rcv_msg[BUFSIZE];

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_address/hostname> <server_port>\n", argv[0]);
        return -1;
    }

    sa_srv.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &sa_srv.sin_addr))
        printf("Address is a valid IP\n");
    else {
        printf("Address is a hostname... maybe\n");
        sa_srv.sin_addr = lookup_host(argv[1]);
    }
    sa_srv.sin_port = htons(atoi(argv[2]));
    printf("Connecting...\n");
    sock = socket(PF_INET, SOCK_STREAM, 0);
    sa_srv.sin_family = AF_INET;
        //check error <=0
    CHECK(connect(sock, (struct sockaddr *)&sa_srv, sizeof(sa_srv)), -1, "Error while connecting")
    printf("Connection estabilished.\n");
    if (fork()) {
        //reader
        int ret;

        while ((ret = recv(sock, rcv_msg, BUFSIZE - 1, 0)) > 0) {
            rcv_msg[ret] = '\0';
            printf("[SERVER] %s", rcv_msg);
        }
        if (ret == -1) {
            perror("Error while receiving data");
            kill(0, SIGTERM);
            exit(-1);
        }
        else {
            printf("Connection closed.\n");
            kill(0, SIGTERM);
            shutdown(sock, SHUT_RDWR);
            close(sock);
        }
    }
    else {
        //writer
        char snd_msg[BUFSIZE];
        int ret;

        do {
            scanf("%[^\n]%*c", snd_msg);
            //gets(snd_msg);
            strcat(snd_msg, "\n");
            printf(snd_msg);
            printf("size %d\n", strlen(snd_msg));
            if (check_disconnect(snd_msg))
            {
                printf("Disconnecting...\n");
                shutdown(sock, SHUT_WR);
                break;
            }
            //ret = send(sock, snd_msg, strlen(snd_msg), 0);
            ret = send(sock, "a b \n", 5, 0);
            //bruh wtf it works that way
            printf("sent %d\n", ret);
        } while (ret > 0);
        if (ret == -1) {
            perror("Error while sending data");
            exit(-1);
        }
        return 0;
    }
}