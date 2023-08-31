#include<stdio.h>
#include<stdlib.h>
#include "../rust_apis/rust_apis.h"
#include <string.h>
// gcc ./test/rust_api_test.c -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o ./test/rust_api_test

int main() {
    CKeys keys = key_gen();
    short *msg1 = (short int*) malloc(sizeof(short int)*MSG_SHORT_SIZE);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        msg1[i] = (short) i;
    }
    CCiphertext ct1 = encrypt(&keys.ek, msg1);
    printf("ct 1 %s, ct 1 size %lu\n",ct1.ptr,strlen(ct1.ptr));
    short int *decrypted = decrypt(&keys.dk, &ct1);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        if(msg1[i]!=decrypted[i]) {
            printf("decryption test: %d-th value is invalid. msg1:%hd, decrypted: %hd\n",i,msg1[i],decrypted[i]);
            exit(1);
        }
    }

    short *msg2 = (short int*) malloc(sizeof(short int)*MSG_SHORT_SIZE);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        msg2[i] = (short) 3*i;
    }
    CCiphertext ct2 = encrypt(&keys.ek, msg2);
    //printf("ct 2 %s, ct 2 size %lu\n",ct2.ptr,strlen(ct2.ptr));
    CCiphertext ct_add = add_ciphertext(&keys.ek, &ct1, &ct2);
    short int *decrypted_add = decrypt(&keys.dk, &ct_add);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        if(msg1[i]+msg2[i]!=decrypted_add[i]) {
            printf("add test: %d-th value is invalid. msg1:%hd, msg2:%hd, decrypted: %hd\n",i,msg1[i],msg2[i],decrypted_add[i]);
            exit(1);
        }
    }

    CCiphertext ct_scalar = scalar_ciphertext(&keys.ek, &ct1, 10);
    short int *decrypted_scalar = decrypt(&keys.dk, &ct_scalar);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        if(10*msg1[i]!=decrypted_scalar[i]) {
            printf("scalar test: %d-th value is invalid. msg1:%hd,  decrypted: %hd\n",i,msg1[i],decrypted_scalar[i]);
            exit(1);
        }
    }

    CCiphertext ct_scalar_100a = scalar_ciphertext_percent(&keys.ek, &ct1, 30);
    CCiphertext ct_scalar_100b = scalar_ciphertext_percent(&keys.ek, &ct1, 70);
    CCiphertext ct_scalar_add = add_ciphertext(&keys.ek, &ct_scalar_100a, &ct_scalar_100b);
    short int *decrypted_scalar_add = decrypt(&keys.dk, &ct_scalar_add);
    for(int i=0;i<MSG_SHORT_SIZE;++i) {
        if(msg1[i]!=decrypted_scalar_add[i]) {
            printf("scalar 100 test: %d-th value is invalid. msg:%hd, decrypted: %hd\n",i,msg1[i],decrypted_scalar_add[i]);
            exit(1);
        }
    }
}