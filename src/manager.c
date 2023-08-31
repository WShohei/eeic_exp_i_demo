#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../rust_apis/rust_apis.h"
#include "../include/parameter.h"
// gcc ./src/manager.c -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o ./build/manager

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: ./manager <IP address> <port> \n");
        exit(1);
    }

    FILE *ek_fp = fopen(encryption_key_file,"r");
    if(ek_fp==NULL) {
        perror("open ek error");
        exit(1);
    }
    CEncryptionKey *ek = (CEncryptionKey*) malloc(sizeof(CEncryptionKey));
    char ek_str[TRANSFER_BYTE_SIZE];
    fscanf(ek_fp,"%s",ek_str);
    ek->ptr = ek_str;
    printf("ek:\n%s\n",ek->ptr);
    fclose(ek_fp);
    // make a socket
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                      // IPv4
    int check = inet_aton(argv[1], &addr.sin_addr); // IPアドレスの設定
    if (check == -1) {
        perror("check");
        exit(1);
    }
    if (check == 0) {
        printf(
            "the address was not parseable in the specified address family\n");
        exit(1);
    }
    addr.sin_port = htons(atoi(argv[2])); //ポートの設定
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("connect");
        exit(1);
    }

    char a[20];
    int cn;
    int p;
    send(s, ek->ptr, TRANSFER_BYTE_SIZE, 0);
    while (1) {
        printf("<client number (int)> <percentage (int)>\n");
        scanf("%[^\n]%*c", a);
        if (sscanf(a, "%d %d", &cn, &p) != 2) {
            printf("ignored\n");
            continue;
        }
        send(s, &cn, 4, 0);
        send(s, &p, 4, 0);
        sleep(1);
        printf("successfully sent cd = %d, p = %d\n", cn, p);
    }
    close(s);
    return 0;
}