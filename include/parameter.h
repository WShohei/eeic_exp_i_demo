#pragma once
#include "../rust_apis/rust_apis.h"

#define N MSG_BYTE_SIZE
#define M (N/2)

#define TRANSFER_BYTE_SIZE CT_BYTE_SIZE

const char *keys_dir = "./keys";
const char *encryption_key_file = "./keys/encryption_key.txt";
const char *decryption_key_file = "./keys/decryption_key.txt";