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
    if(argc!=3) {
        printf("invalid number of arguments.\n");
        exit(1);
    }
    const char *ip_addr = argv[1];
    errno = 0;
    char* end;
    long int port = strtol(argv[2], &end, 10);
    if(errno == ERANGE || strlen(end)!=0) {
        printf("port is invalid.\n");
        exit(1);
    }

    int s = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    FILE *pfp = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -","r");
    char r_data[16];
    char s_data[16];
    int is_r_mode = 1;
    while(1) {
        if(is_r_mode==1) {
            int n_re = recv(s, r_data, 16, 0);
            if(n_re==0) break;
            if(n_re==-1) {
                printf("fail to receive data from the socket.\n");
                exit(1);
            }
            write(1, r_data, n_re);
            is_r_mode = 0;
        }
        else {
            int n_p = fread(s_data, sizeof(char), 16, pfp);
            if (n_p==-1) exit(1);
            int n_se = send(s, s_data, n_p, 0);
            is_r_mode = 1;
        }
    }
    close(s);
}