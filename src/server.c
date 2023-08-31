#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    if(argc!=2) {
        printf("invalid number of arguments.\n");
        exit(1);
    }
    errno = 0;
    char* end;
    long int port = strtol(argv[1], &end, 10);
    if(errno == ERANGE || strlen(end)!=0) {
        printf("port is invalid.\n");
        exit(1);
    }

    int ss = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(ss, (struct sockaddr *)&addr, sizeof(addr));
    listen(ss,10);
    
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr *)&client_addr, &len);
    FILE *pfp = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -","r");
    char s_data[16];
    char r_data[16];
    int is_w_mode = 1;
    while(1) {
        if(is_w_mode==1) {
            int n_p = fread(s_data, sizeof(char), 16, pfp);
            if (n_p==-1) exit(1);
            int n_se = send(s, s_data, n_p, 0);
            is_w_mode = 0;
        }
        else {
            int n_re = recv(s, r_data, 16, 0);
            if(n_re==0) break;
            if(n_re==-1) {
                printf("fail to receive data from the socket.\n");
                exit(1);
            }
            write(1, r_data, n_re);
            is_w_mode = 1;
        }
    }
    pclose(pfp);
    close(s);
}