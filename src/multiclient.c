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
#include <complex.h>
#include <math.h>
#include "./voicechanger.c"
#include "../rust_apis/rust_apis.h"
#include "../include/parameter.h"
// gcc ./src/multiclient.c -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o ./build/multiclient

#define ENC_PTYPE_ID 0
#define DEC_PTYPE_ID 1


void child_process(CEncryptionKey *ek, CDecryptionKey *dk, int enc_pipefd[2],  const char *ip_address, const char *port, const long f) {
    // make a socket
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                      // IPv4
    int check = inet_aton(ip_address, &addr.sin_addr); // IPアドレスの設定
    if (check == -1) {
        perror("check");
        exit(1);
    }
    if (check == 0) {
        printf(
            "the address was not parseable in the specified address family\n");
        exit(1);
    }
    addr.sin_port = htons(atoi(port)); //ポートの設定
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("connect");
        exit(1);
    }

    // Enc Process
    short data_plaintext[M*NUM_CHUNK];
    
    while(1){
        read(enc_pipefd[0], data_plaintext, N*NUM_CHUNK);
        /*voicechanger。第一引数は一度に変換するshortデータの数。第二引数はシフトする周波数。第三引数はシフトされるデータ。*/
        /*第二引数のシフトする周波数fをGUIの入力から受け付ける必要がある。*/
        //printf("f: %ld\n",f);
        freqshift((long)(M*NUM_CHUNK),f,data_plaintext);
        const CCiphertext *data_enc = batch_encrypt(ek, data_plaintext);
        int send_str_lens[NUM_CHUNK];
        char send_str[TRANSFER_BYTE_SIZE*NUM_CHUNK] = {'\0'};
        for(int i=0;i<NUM_CHUNK;i++) {
            send_str_lens[i] = (int) strlen(data_enc[i].ptr);
            strcat(send_str,data_enc[i].ptr);
        }
        int cs1 = send(s, send_str_lens, sizeof(int)*NUM_CHUNK, 0);
        if (cs1 == -1) {
            perror("send");
            exit(1);
        }
        send(s, send_str, strlen(send_str)+1, 0);
        /*if (cs2 == -1) {
            perror("send");
            exit(1);
        }*/
        batch_free_ciphertext(data_enc);

        int recv_str_lens[NUM_CHUNK];
        int nr1 = recv(s, recv_str_lens, sizeof(int)*NUM_CHUNK, 0);
        if (nr1 == -1) {
            perror("recv");
            exit(1);
        }
        int size_sum = 0;
        for(int i=0;i<NUM_CHUNK;i++) {
            size_sum += recv_str_lens[i];
        }
        char recv_str[size_sum+1];
        ssize_t offset = 0;
        while (1) {
            int nr2 = recv(s, recv_str+offset,size_sum+1, 0);
            if(nr2 == -1) {
                printf("recv error\n");
                perror("recv");
                exit(1);
            }
            offset += nr2;
            if(offset==size_sum+1) break;
            else exit(1);
        }

        CCiphertext ct_recv[NUM_CHUNK];
        for(int i=0; i<NUM_CHUNK; i++) {
            ct_recv[i].ptr = (char*) malloc(TRANSFER_BYTE_SIZE);
        }
        char *last_src_ptr = recv_str;
        for(int i=0;i<NUM_CHUNK;i++) {
            strncpy(ct_recv[i].ptr, last_src_ptr, recv_str_lens[i]);
            ct_recv[i].ptr[recv_str_lens[i]] = '\0';
            last_src_ptr += recv_str_lens[i];
        }
        short *decrypted = batch_decrypt(dk, ct_recv);
        short *subed = batch_sub_plaintexts(decrypted, data_plaintext);
        free(decrypted);
        write(1,subed,N*NUM_CHUNK);
        for(int i=0;i<NUM_CHUNK; i++) {
            free(ct_recv[i].ptr);
        }
        free(subed);
    }
}

void main_process(CEncryptionKey *ek, CDecryptionKey *dk, int enc_pipefd[2], const char *ip_address, const char *port) {
    short data_record[M*NUM_CHUNK];
    short data_plaintext[M*NUM_CHUNK];
    short data_decrypted[M*NUM_CHUNK];
    for(int i=0;i<M*NUM_CHUNK;i++) {
        data_record[i] = 0;
    }
    int is_enc_first = 1;
    while (1) {
        int ns = read(0, data_record, N*NUM_CHUNK);
        //printf(" ----new data---- \n");
        if (ns == -1) {
            perror("read");
            exit(1);
        }
        if (ns == 0) {
            // EOF
            exit(1);
        }
        for(int i=0;i<M*NUM_CHUNK;i++){
            data_plaintext[i] = data_record[i];
        }
        /*if(is_enc_first==1) {
            is_enc_first = 0;
        } else {
            read(enc_pipefd[0], data_decrypted, N*NUM_CHUNK);
            write(1, data_decrypted, N*NUM_CHUNK);
        }*/
        //新しい録音データをEnc Processに渡す。
        write(enc_pipefd[1], data_plaintext, N*NUM_CHUNK);
        /*int cs = send(s, ct.ptr, TRANSFER_BYTE_SIZE, 0);
        if (cs == -1) {
            perror("send");
            exit(1);
        }
        int nr = recv(s, data_r, TRANSFER_BYTE_SIZE, 0);
        if (nr == -1) {
            perror("recv");
            exit(1);
        }
        if (nr == 0) {
            // EOF
            perror("EOF");
            exit(1);
        }
        CCiphertext *rec_ct = (CCiphertext*) malloc(sizeof(CCiphertext));
        rec_ct -> ptr = data_r;
            short *decrypted = decrypt(dk, rec_ct);*/
            //write(1, dummy_decrypted, N*NUM_CHUNK);
            //free(dummy_decrypted);
            /*if (cr == -1) {
                perror("write");
                exit(1);
            }*/
    }

	close(enc_pipefd[0]);
	close(enc_pipefd[1]);
}


int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: ./multiclient <IP address> <port> <f [Hz]>\n");
        exit(1);
    }

    long f = strtol(argv[3], NULL, 10);//GUIの操作からシフト周波数fをとってきて定義

    FILE *ek_fp = fopen(encryption_key_file,"r");
    CEncryptionKey *ek = (CEncryptionKey*) malloc(sizeof(CEncryptionKey));
    char *ek_str = (char*) malloc(sizeof(char)*TRANSFER_BYTE_SIZE);
    fscanf(ek_fp,"%s",ek_str);
    ek->ptr = ek_str;
    fclose(ek_fp);
    FILE *dk_fp = fopen(decryption_key_file,"r");
    CDecryptionKey *dk = (CDecryptionKey*) malloc(sizeof(CDecryptionKey));
    char *dk_str = (char*) malloc(sizeof(char)*TRANSFER_BYTE_SIZE);
    fscanf(dk_fp,"%s",dk_str);
    dk->ptr = dk_str;
    fclose(dk_fp);

    int enc_pipefd[2];
	if (pipe(enc_pipefd) < 0) {
		perror("enc pipe");
		exit(-1);
	}
    pid_t enc_pid = fork();
	if (enc_pid < 0) {
		perror("fork");
		exit(-1);
	} else if (enc_pid == 0) {
		// Child Process
        child_process(ek, dk, enc_pipefd, argv[1], argv[2], f);
	} else {
		// Parent Process
         main_process(ek, dk, enc_pipefd, argv[1], argv[2]);
        //main_process(ek, dk, enc_pipefd, dec_pipefd, argv[1], argv[2]);
	}
    return 0;
}