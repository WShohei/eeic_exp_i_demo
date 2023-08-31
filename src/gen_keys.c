#include<stdio.h>
#include<stdlib.h>
#include "../rust_apis/rust_apis.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "../include/parameter.h"
// gcc ./src/gen_keys.c -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o ./build/gen_keys

int main() {
    CKeys keys = key_gen();
    mkdir(keys_dir, S_IRWXU);
    FILE *ek_fp = fopen(encryption_key_file,"w");
    fprintf(ek_fp, "%s", keys.ek.ptr);
    fclose(ek_fp);
    FILE *dk_fp = fopen(decryption_key_file,"w");
    fprintf(dk_fp, "%s", keys.dk.ptr);
    fclose(dk_fp);
}