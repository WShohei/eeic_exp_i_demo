#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define N 32
#define M 16

#define NUM_THREAD 100
#define NUM_CLIENT 4

pthread_t t[NUM_THREAD];

typedef struct Play_thread_data {
    int s;
    int i;
} play_thread_data;

void *play_thread(void *arg) {
    play_thread_data *a = (play_thread_data *)arg;
    int i = a->i;
    int s = a->s;
    FILE *f = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
    if (f == NULL) {
        perror("popen");
        exit(1);
    }

    for (;;) {
        short data[M];
        int n = recv(s, data, N, 0);
        if (n == -1) {
            perror("recv");
            exit(1);
        }
        n = write(f->_file, data, N);
        if (n == -1) {
            perror("write");
            exit(1);
        }
    }
}

void *rec_thread(void *arg) {
    int s = *(int *)arg;
    for (;;) {
        short data[M];
        int n = read(0, data, N);
        if (n == -1) {
            perror("read");
            exit(1);
        }
        n = send(s, data, N, 0);
        if (n == -1) {
            perror("send");
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: ./multiclient <IP address> <port>\n");
        exit(1);
    }

    for (int i = 0; i < NUM_CLIENT; i++) {
        int s = socket(PF_INET, SOCK_STREAM, 0);
        if (s == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        int check = inet_aton(argv[1], &addr.sin_addr);
        if (check == -1) {
            perror("check");
            exit(1);
        }
        if (check == 0) {
            printf("the address was not parseable in the specified address "
                   "family\n");
            exit(1);
        }
        addr.sin_port = htons(atoi(argv[2]));
        int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            perror("connect");
            exit(1);
        }
        if (i == 0) {
            pthread_create(&t[0], NULL, rec_thread, &s);
            printf("rec_thread created\n");
        } else {
            play_thread_data d;
            d.i = i;
            d.s = s;
            pthread_create(&t[i], NULL, play_thread, &d);
            printf("play_thread created\n");
        }
        usleep(100);
    }
    pthread_join(t[0], NULL);
    return 0;
}
