#ifndef CS_VKEY_ENCRYPT_H_
#define CS_VKEY_ENCRYPT_H_

#include <stdint.h>

int encrypt_init();


int encrypt_random(uint8_t *random);

int encrypt_makeDHPublic(const uint8_t *secret, uint8_t *public);

int encrypt_makeSignPublic(const uint8_t *secret, uint8_t *public);

int encrypt_enHash(const uint64_t *in,uint64_t *out);
int encrypt_hash( uint8_t *out,  const uint8_t *in, int len);

int encrypt_hmac(const uint8_t *secrect, const uint8_t *msg, int len, uint8_t *out);


int encrypt_encrypt(uint8_t *cipherText, const uint8_t *plainText, int textLength,
                    const uint8_t *key, const uint8_t *iv, const uint8_t *add, int add_len, uint8_t *tag);

int encrypt_decrypt(uint8_t *plainText, const uint8_t *cipherText, int textLength,
                    const uint8_t *key, const uint8_t *iv, const uint8_t *add, int add_len, const uint8_t *tag);

#endif