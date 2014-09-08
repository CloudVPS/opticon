#ifndef _AES_H
#define _AES_H 1

#include <stdlib.h>
#include <datatypes.h>
#include <ioport.h>
#include <auth.h>

/* =============================== TYPES =============================== */

typedef struct {
    uint8_t key[32]; 
    uint8_t enckey[32]; 
    uint8_t deckey[32];
} aes256_context; 

/* ============================= FUNCTIONS ============================= */

void aes256_init(aes256_context *, uint8_t * /* key */);
void aes256_done(aes256_context *);
void aes256_encrypt_ecb(aes256_context *, uint8_t * /* plaintext */);
void aes256_decrypt_ecb(aes256_context *, uint8_t * /* cipertext */);

int ioport_encrypt (aeskey *k, ioport *in, ioport *out, time_t ts);
int ioport_decrypt (aeskey *k, ioport *in, ioport *out, time_t ts);

#endif
