#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "../rust_apis/rust_apis.h"
#include "../include/parameter.h"
// gcc ./src/multiserver.c -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o ./build/multiserver

/*最大スレッド数*/
#define NUM_THREAD 3
/*最大クライアント数*/
#define NUM_CLIENT 12

/*スレッド配列の現在のpositions*/
int num_thread = 0;

/*ソケット配列の現在のposition*/
int num_client = 0;

/*ソケット配列*/
int s[NUM_CLIENT];


pthread_t t[NUM_THREAD];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*managerから受け取るデータ*/
CEncryptionKey *ek;
int nc;            // client number
short p[NUM_CLIENT]; // percentage

struct data_accept_in {
    int ss;
};

void mix(CEncryptionKey *ek, int num_client, CCiphertext cts[num_client][NUM_CHUNK], short scalars[num_client], CCiphertext *mixed_cts[num_client]) {
    pthread_mutex_lock(&mutex);
    /*for (int i=1;i<num_client;i++) {
        for(int j=0;j<NUM_CHUNK;j++) {
            mixed_cts[i][j] = zero_ct;
        }
    }*/
    CCiphertext *scalared[num_client];
    for(int i=1; i<num_client;i++) {
        scalared[i] = (CCiphertext*) batch_scalar_ciphertext_percent(ek, cts[i], scalars[i]);
    }

    uint16_t zero = 0;
    short data_plaintext[M*NUM_CHUNK];
    for(int i=0;i<M*NUM_CHUNK;i++) {
        data_plaintext[i] = 0;
    }
    CCiphertext *sum_ct = (CCiphertext*) batch_encrypt(ek, data_plaintext);
    for(int i=1;i<num_client;i++) {
        CCiphertext *pre_ct = sum_ct;
        sum_ct = (CCiphertext*) batch_add_ciphertext(ek, sum_ct, scalared[i]);
        batch_free_ciphertext(pre_ct);
    }
    for(int i=1;i<num_client;i++) {
        mixed_cts[i] = sum_ct;//(CCiphertext*) batch_sub_ciphertext(ek, sum_ct, scalared[i]);
    }
    for(int i=1;i<num_client;i++) {
        batch_free_ciphertext(scalared[i]);
    }
    //batch_free_ciphertext(sum_ct);
    pthread_mutex_unlock(&mutex);
}

/*void mix(CEncryptionKey *ek, CCiphertext *cts[num_client], short scalars[num_client], CCiphertext mixed_cts[num_client]) {
    pthread_mutex_lock(&mutex);
    uint16_t zero = 0;
    CCiphertext zero_ct = encrypt(ek, &zero);
    for (int i=1;i<num_client;i++) {
        mixed_cts[i] = zero_ct;
    }
    //printf("mixed ct initialized \n");
    for (int i = 1; i<num_client; i++) {
        //printf("ct %p\n%s\n",cts[i],cts[i]->ptr);
        CCiphertext scalared = scalar_ciphertext_percent(ek, cts[i], scalars[i]);
        //printf("scalared");
        for(int j=1; j<num_client; j++) {
            if(i!=j) {
                mixed_cts[j] = add_ciphertext(ek, &mixed_cts[j], &scalared);
                //printf("added");
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}*/



void *manager_thread(void *arg) {
    int ms = s[0];
    char *ek_str = (char*) malloc(sizeof(char)*TRANSFER_BYTE_SIZE);
    recv(ms, ek_str, TRANSFER_BYTE_SIZE, 0);
    ek->ptr = ek_str;
    printf("ek:\n%s\n",ek->ptr);
    while (1) {
        recv(ms, &nc, 4, 0);
        recv(ms, &p[nc], 4, 0);
        printf("Set the volume of client No.%d to %d percent\n", nc, p[nc]);
    }
}

/*送受信スレッド:データの中継のみを行い録音再生はクライアントが各自行う*/
void *main_thread() {
    printf("Hello from main thread\n");
    /*読み込みデータ配列*/
    //char data_r[NUM_CLIENT];
    /*CCiphertext *cts[NUM_CLIENT][NUM_CHUNK];
    for(int i=0; i<NUM_CLIENT; i++) {
        for(int j=0; j<NUM_CHUNK; j++) {
            cts[i][j] = (CCiphertext*) malloc(sizeof(CCiphertext));
            cts[i][j]->ptr = (char*) malloc(TRANSFER_BYTE_SIZE);
        }
    }*/
    printf("Init in main thread\n");
    for (;;) {
        /*num_clientのアクセスの安全性確保が必要*/
        int num_client_tmp = num_client;
        
        CCiphertext cts[num_client_tmp][NUM_CHUNK];
        for(int i=0; i<num_client_tmp; i++) {
            for(int j=0; j<NUM_CHUNK; j++) {
                cts[i][j].ptr = (char*) malloc(TRANSFER_BYTE_SIZE);
            }
        }
        for (int i = 1; i < num_client_tmp; i++) {
            int recv_str_lens[NUM_CHUNK];
            int nr1 = recv(s[i], recv_str_lens, sizeof(int)*NUM_CHUNK, 0);
            if (nr1 == -1) {
                perror("recv");
                exit(1);
            }
            int size_sum = 0;
            for(int j=0;j<NUM_CHUNK;j++) {
                size_sum += recv_str_lens[j];
            }

            char recv_str[size_sum+1];
            ssize_t offset = 0;
            while (1) {
                int nr2 = recv(s[i], recv_str+offset,size_sum+1, 0);
                if(nr2 == -1) {
                    printf("recv error\n");
                    perror("recv");
                    exit(1);
                }
                offset += nr2;
                if(offset==size_sum+1) break;
                else exit(1);
            }

            char *last_src_ptr = recv_str;
            for(int j=0;j<NUM_CHUNK;j++) {
                strncpy(cts[i][j].ptr, last_src_ptr, recv_str_lens[j]);
                cts[i][j].ptr[recv_str_lens[j]] = '\0';
                last_src_ptr += recv_str_lens[j];
            }
            
        }
        
        CCiphertext *mixed_cts[num_client_tmp];
        mix(ek, num_client_tmp, cts, p, mixed_cts);
        //printf("mixed\n");
        for(int i=1;i<num_client_tmp;i++) {
            int send_str_lens[NUM_CHUNK];
            char send_str[TRANSFER_BYTE_SIZE*NUM_CHUNK] = {'\0'};
            for(int j=0;j<NUM_CHUNK;j++) {
                send_str_lens[j] = (int) strlen(mixed_cts[i][j].ptr);
                //printf("j:%d len:%d\n",j,send_str_lens[j]);
                strcat(send_str,mixed_cts[i][j].ptr);
            }
            //printf("send_str: %s",send_str);
            int cs1 = send(s[i], send_str_lens, sizeof(int)*NUM_CHUNK, 0);
            if (cs1 == -1) {
                perror("send");
                exit(1);
            }
            send(s[i], send_str, strlen(send_str)+1, 0);
            //printf("cs2 %d\n",cs2);
            /*if (cs2 == -1) {
                perror("send");
                exit(1);
            }*/
        }
        //printf("sent all\n");
        for(int i=0; i<num_client_tmp; i++) {
            for(int j=0; j<NUM_CHUNK; j++) {
                free(cts[i][j].ptr);
            }
        }
        batch_free_ciphertext(mixed_cts[1]);
    }
    return NULL;
}

void *accept_thread(void *data_in) {

    struct data_accept_in *data = (struct data_accept_in *)data_in;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        pthread_mutex_lock(&mutex);
        int tmp = num_client;
        pthread_mutex_unlock(&mutex);
        if (tmp == NUM_CLIENT) {
            printf("FULL\n");
            while (1) {
                printf("q to quit\n");
                int in = getc(stdin);
                if ((char)in == 'q') {
                    close(data->ss);
                    exit(1);
                }
            }
        }

        s[tmp] = accept(data->ss, (struct sockaddr *)&client_addr, &len);
        if (s[tmp] == -1) {
            perror("accept");
            exit(1);
        }
        pthread_mutex_lock(&mutex);
        if (num_client == 0) {
            printf("connected! manager\n");
        } else {
            printf("connected! client No.%d\n", num_client);
        }
        num_client++;
        pthread_mutex_unlock(&mutex);

        if (num_client == 1) {
            /*managerスレッドの作成*/
            for (int i = 1; i < NUM_CLIENT; i++) {
                p[i] = 100;
            }
            pthread_mutex_lock(&mutex);
            num_thread++;
            int c = pthread_create(&t[num_thread], NULL, manager_thread, NULL);
            pthread_mutex_unlock(&mutex);
            if (c != 0) {
                perror("pthread_create");
                exit(1);
            }
            printf("manager_thread created\n");
        }

        if (num_client == 2) {
            /*送受信スレッドの作成*/
            pthread_mutex_lock(&mutex);
            num_thread++;
            int c = pthread_create(&t[num_thread], NULL, main_thread, NULL);
            pthread_mutex_unlock(&mutex);
            if (c != 0) {
                perror("pthread_create");
                exit(1);
            }
        }
    }
    pthread_mutex_lock(&mutex);
    num_thread--;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: ./multiserver2 <port>\n");
        exit(1);
    }

    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket");
        exit(1);
    }
    ek = (CEncryptionKey*) malloc(sizeof(CEncryptionKey));

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

    struct data_accept_in *data =
        (struct data_accept_in *)malloc(sizeof(struct data_accept_in));
    data->ss = ss;
    check = pthread_create(&t[0], NULL, accept_thread, data);
    if (check != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_join(t[0], NULL);
    return 0;
}
