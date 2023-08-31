#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


#define CT_BYTE_SIZE 1276

#define DK_BYTE_SIZE 633

#define EK_BYTE_SIZE 625

#define MSG_BYTE_SIZE 64

#define MSG_SHORT_SIZE (MSG_BYTE_SIZE / 2)

#define NUM_CHUNK 32

typedef struct CCiphertext {
  char *ptr;
} CCiphertext;

typedef struct CEncryptionKey {
  char *ptr;
} CEncryptionKey;

typedef struct CDecryptionKey {
  char *ptr;
} CDecryptionKey;

typedef struct CKeys {
  struct CEncryptionKey ek;
  struct CDecryptionKey dk;
} CKeys;

struct CCiphertext add_ciphertext(const struct CEncryptionKey *cek,
                                  const struct CCiphertext *c1,
                                  const struct CCiphertext *c2);

const struct CCiphertext *batch_add_ciphertext(const struct CEncryptionKey *cek,
                                               const struct CCiphertext *c1s,
                                               const struct CCiphertext *c2s);

uint16_t *batch_decrypt(const struct CDecryptionKey *cdk, const struct CCiphertext *cts);

const struct CCiphertext *batch_encrypt(const struct CEncryptionKey *cek, uint16_t *plaintext_ptr);

void batch_free_ciphertext(const struct CCiphertext *cs);

const struct CCiphertext *batch_scalar_ciphertext(const struct CEncryptionKey *cek,
                                                  const struct CCiphertext *cs,
                                                  uint16_t scalar);

const struct CCiphertext *batch_scalar_ciphertext_percent(const struct CEncryptionKey *cek,
                                                          const struct CCiphertext *cs,
                                                          uint16_t scalar);

const struct CCiphertext *batch_sub_ciphertext(const struct CEncryptionKey *cek,
                                               const struct CCiphertext *c1s,
                                               const struct CCiphertext *c2s);

uint16_t *batch_sub_plaintexts(uint16_t *plaintext1_ptr, uint16_t *plaintext2_ptr);

uint16_t *decrypt(const struct CDecryptionKey *cdk, const struct CCiphertext *c);

struct CCiphertext encrypt(const struct CEncryptionKey *cek, uint16_t *plaintext_ptr);

struct CKeys key_gen(void);

struct CCiphertext scalar_ciphertext(const struct CEncryptionKey *cek,
                                     const struct CCiphertext *c,
                                     uint16_t scalar);

struct CCiphertext scalar_ciphertext_percent(const struct CEncryptionKey *cek,
                                             const struct CCiphertext *c,
                                             uint16_t scalar);

struct CCiphertext sub_ciphertext(const struct CEncryptionKey *cek,
                                  const struct CCiphertext *c1,
                                  const struct CCiphertext *c2);
