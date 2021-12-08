#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h> 
#include <errno.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/ip.h>

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

int main(int argc, char *argv[]) {
    int ret;

    if (fork()) {
        int sock, sock_r;
        struct sockaddr_in sa, sa_r;
        int sa_r_len;
        int rcv_sz;
        char rcv_msg[BUFSIZE];
        char snd_msg[BUFSIZE] = "answer\n";

        sock = socket(PF_INET, SOCK_STREAM, 0);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(1234);
        //inet_aton("63.161.169.137", &sa.sin_addr.s_addr);
        sa.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (struct sockaddr *)&sa, sizeof(sa));
        listen(sock, 10);
        sock_r = accept(sock, (struct sockaddr *)&sa_r, (socklen_t *)&sa_r_len);
        rcv_sz = recv(sock_r, rcv_msg, BUFSIZE - 1, 0); //-1 so \0 will be conserved
        scv_msg[rcv_sz] = '\0';
        //MSG_DONTWAIT - EAGAIN
        //strtok only 1 command
        printf("received %s", rcv_msg);
        send(sock_r, snd_msg, strlen(snd_msg), 0);
        printf("Hello message sent\n");
        shutdown(sock, SHUT_RD);
        while ((ret = recv(sock_r, rcv_msg, BUFSIZE - 1, MSG_DONTWAIT)) == -1 && errno == EAGAIN)
            ;
        if (ret)
            perror("close broken");
        close(sock);
    }
    else {
        int sock;
        struct sockaddr_in sa_srv;
        int rcv_sz;
        char rcv_msg[BUFSIZE];
        char snd_msg[BUFSIZE] = "hello\naboba\n";
        
        sock = socket(PF_INET, SOCK_STREAM, 0);
        sa_srv.sin_family = AF_INET;
        sa_srv.sin_port = htons(1234);
        ret = inet_pton(AF_INET, "127.0.0.1", &sa_srv.sin_addr);
        //check error <=0
        connect(sock, (struct sockaddr *)&sa_srv, sizeof(sa_srv));
        send(sock, snd_msg, strlen(snd_msg), 0);
        printf("Hello message sent\n");
        rcv_sz = recv(sock, rcv_msg, BUFSIZE - 1, 0); //-1 so \0 will be conserved
        printf("received %s", rcv_msg);
        shutdown(sock, SHUT_RD);
    }
}