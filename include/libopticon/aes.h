#ifndef _AES_H
#define _AES_H 1

#include <stdlib.h>
#include <sys/types.h>
#include <libopticon/datatypes.h>
#include <libopticon/ioport.h>

/* =============================== TYPES =============================== */

typedef struct {
    uint8_t key[32]; 
    uint8_t enckey[32]; 
    uint8_t deckey[32];
} aes256_context; 

/* ============================= FUNCTIONS ============================= */

void     aes256_init(aes256_context *, uint8_t * /* key */);
void     aes256_done(aes256_context *);
void     aes256_encrypt_ecb(aes256_context *, uint8_t * /* plaintext */);
void     aes256_decrypt_ecb(aes256_context *, uint8_t * /* ciphertext */);

aeskey   aeskey_create (void);
aeskey   aeskey_from_base64 (const char *);
char    *aeskey_to_base64 (aeskey);
int      ioport_encrypt (aeskey *k, ioport *in, 
                         ioport *out, time_t, uint32_t);
int      ioport_decrypt (aeskey *k, ioport *in, 
                         ioport *out, time_t, uint32_t);

#endif
