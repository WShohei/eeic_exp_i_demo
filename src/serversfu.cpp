#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define Ringlen 8192
#define N 32
#define M 16
#define NUM_CLIENT 4
#define NUM_THREAD 100

class Ring {
  public:
    Ring();
    int Rp[NUM_CLIENT];
    int Wp;
    short buf[Ringlen];
    short pop(int sendto);
    void push(short arg);
};

short Ring::pop(int sendto) {
    // printf("Rp[%d]=%d\n", sendto, Rp[sendto]);
    short tmp = buf[Rp[sendto]];
    int next = (Rp[sendto] + 1) % Ringlen;
    while (1) {
        if (next != Wp) {
            break;
        }
    }
    Rp[sendto] = next;
    return tmp;
}

void Ring::push(short arg) {
    buf[Wp] = arg;
    Wp = (Wp + 1) % Ringlen;
    return;
}

Ring::Ring() {
    Wp = 0;
    for (int i = 0; i < NUM_CLIENT; i++) {
        Rp[i] = 0;
    }
    for (int i = 0; i < Ringlen; i++) {
        buf[i] = (short)0;
    }
}

Ring *make_data() {
    Ring *data = (Ring *)malloc(sizeof(Ring) * NUM_CLIENT);
    for (int i = 0; i < NUM_CLIENT; i++) {
        data[i] = Ring();
    }
    return data;
}

Ring *data;
pthread_t t[NUM_THREAD];
int num_thread = 0;
int num_client = 0;
pthread_mutex_t mutex;
int s[NUM_CLIENT][NUM_CLIENT];

typedef struct {
    int me;
} recv_client_data;

typedef struct {
    int me;
    int sendto;
} send_client_data;

void *recv_thread(void *a) {
    recv_client_data *arg = (recv_client_data *)a;
    int me = arg->me;
    while (1) {
        short data_r[M];
        int n = recv(s[me][0], data_r, N, 0);
        if (n == 0) {
            continue;
        }
        if (n == -1) {
            perror("recv");
            exit(1);
        }
        for (int i = 0; i < M; i++) {
            data[me].push(data_r[i]);
            // printf("data_sentfrom%d[%d]=%d\n", me, i, data_r[i]);
        }
    }
}

void *send_thread(void *a) {
    send_client_data *arg = (send_client_data *)a;
    int me = arg->me;
    int sendto = arg->sendto;
    int idx = (me < sendto) ? me + 1 : me;
    while (1) {
        if (sendto <= num_client - 1) {
            break;
        }
        usleep(100 * 1000);
    }
    printf("start sending from %d to %d (use s[%d][%d])\n", me, sendto, sendto,
           idx);
    while (1) {
        short data_s[M];
        for (int i = 0; i < M; i++) {
            data_s[i] = data[me].pop(sendto);
            // printf("data_sendto%d[%d]=%d\n", sendto, i, data_s[i]);
        }
        int n = send(s[sendto][idx], data_s, N, 0);
        if (n == -1) {
            perror("send");
            exit(1);
        }
    }
}

void *accept_thread(void *a) {
    pthread_mutex_init(&mutex, NULL);
    int ss = *(int *)a;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    while (num_client < NUM_CLIENT) {
        pthread_mutex_lock(&mutex);
        int me = num_client;
        pthread_mutex_unlock(&mutex);
        int sendto = 0;
        for (int i = 0; i < NUM_CLIENT; i++) {
            s[me][i] = accept(ss, (struct sockaddr *)&client_addr, &len);
            if (sendto == me) {
                sendto++;
            }
            if (i == 0) {
                recv_client_data arg;
                arg.me = me;
                pthread_mutex_lock(&mutex);
                pthread_create(&t[num_thread], NULL, recv_thread, &arg);
                printf("created recv_thread %d\n", me);
                num_thread++;
                pthread_mutex_unlock(&mutex);
            } else {
                send_client_data arg;
                arg.me = me;
                arg.sendto = sendto;
                pthread_mutex_lock(&mutex);
                pthread_create(&t[num_thread], NULL, send_thread, &arg);
                printf("created send_thread (data of %d to %d)\n", me, sendto);
                num_thread++;
                pthread_mutex_unlock(&mutex);
                sendto++;
            }
        }
        usleep(100);
        pthread_mutex_lock(&mutex);
        num_client++;
        pthread_mutex_unlock(&mutex);
    }
    printf("full!\n");
    for (;;) {
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: ./serversfu <port>\n");
        exit(1);
    }
    data = make_data();

    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));
    int check = bind(ss, (struct sockaddr *)&addr, sizeof(addr));
    if (check == -1) {
        perror("bind");
        exit(1);
    }

    check = listen(ss, 10);
    if (check == -1) {
        perror("listen");
        exit(1);
    }

    pthread_create(&t[0], NULL, accept_thread, &ss);
    num_thread++;
    pthread_join(t[0], NULL);
}