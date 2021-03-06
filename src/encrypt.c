
#include <string.h>
#include "encrypt.h"
#include "libsodium/include/sodium.h"
#include "gcm/gcm.h"

int encrypt_init()
{
    if(0!=sodium_init())
    {
        printf("Sodium init failed.");
        return -1;
    }
    gcm_initialize();

    //{{test code
//    unsigned char seed[32]="1234567890123456789012345678901";
//    unsigned char pk[32];
//    unsigned char sk[64];
//    crypto_sign_seed_keypair(pk,sk,seed);
//    printf("public key: %s\n",pk);
//    printf("secret key: %s\n",sk);
    //}}

}


int encrypt_random(uint8_t *random)
{
    randombytes(random,32);
    return 0;
}

int encrypt_hmac(const uint8_t *secrect, const uint8_t *msg, int len, uint8_t *out)
{
    crypto_auth_hmacsha256(out,msg,len,secrect);
    return 0;
}

int encrypt_makeDHPublic(const uint8_t *secret, uint8_t *public)
{
    crypto_scalarmult_base( public, secret );

    return 0;
}

int encrypt_makeDHShareKey(const uint8_t *secret,const uint8_t *public,const uint8_t *to_public,uint8_t* share)
{
    crypto_generichash_state	h;
    unsigned	char	scalarmult_q[32];

    /*	The	client	derives	a	shared	key	from	its	secret	key	and	the	server's	public	key	*/
    /*	shared	key	=	h(q	||	client_publickey	||	server_publickey)	*/
    if	(crypto_scalarmult(scalarmult_q, secret, public) !=	0 )
    {
        return -1;
    }
    crypto_generichash_init(&h,	NULL,	0U,	crypto_generichash_BYTES);
    crypto_generichash_update(&h,	scalarmult_q,	sizeof	scalarmult_q);
    crypto_generichash_update(&h,	public,	sizeof	public);
    crypto_generichash_update(&h,	to_public,	sizeof	to_public);
    crypto_generichash_final(&h,	share,	sizeof	share);
    return 0;
}

int encrypt_makeSignPublic2(const uint8_t *secret, uint8_t *public)
{
    uint8_t sk[crypto_sign_SECRETKEYBYTES];
    sodium_mlock( sk, crypto_sign_SECRETKEYBYTES );
    crypto_sign_seed_keypair( public, sk, secret );
    sodium_munlock( sk, crypto_sign_SECRETKEYBYTES );

    return 0;
}


int encrypt_makeSignPublic(const uint8_t *seed, uint8_t *secret, uint8_t *public)
{
    //uint8_t sk[crypto_sign_SECRETKEYBYTES];
    //sodium_mlock( sk, crypto_sign_SECRETKEYBYTES );
    return crypto_sign_seed_keypair( public, secret, seed );
    //sodium_munlock( sk, crypto_sign_SECRETKEYBYTES );

    //return 0;
}


int encrypt_hash(uint8_t *out, const uint8_t *in, int len)
{
    return crypto_generichash(out, 32, in, len, NULL, 0);
}


int encrypt_enHash(const uint64_t *in,uint64_t *out) {
    uint64_t trans[4];
    uint64_t tmp[4];
    memset( out, 0, 32 );
    memcpy( tmp, in, 32 );
    int i;
    sodium_mlock( trans, 32 );
    sodium_mlock( tmp, 32 );
    for( i = 0; i < 16; i++ ) {
        crypto_hash_sha256( (unsigned char*)trans, (unsigned char*)tmp, 32 );
        out[0] ^= trans[0];
        out[1] ^= trans[1];
        out[2] ^= trans[2];
        out[3] ^= trans[3];
        memcpy( tmp, trans, 32 );
    }
    sodium_munlock( trans, 32 );
    sodium_munlock( tmp, 32 );
    return 0;
}

int encrypt_enScrypt(uint8_t *out, const uint8_t *in, int len,const char* s_salt)
{
    //todo:enscypt
    return encrypt_hash(out,in,len);
}

/*
int encrypt_enHash(const uint8_t *in, uint8_t *out)
{
    uint64_t trans[4];
    uint64_t tmp[4];
    memset( out, 0, 32 );
    memcpy( tmp, in, 32 );
    int i;
    sodium_mlock( trans, 32 );
    sodium_mlock( tmp, 32 );
    for( i = 0; i < 16; i++ ) {
        crypto_hash_sha256( (uint8_t*)trans, (uint8_t*)tmp, 32 );
        out[0] ^= trans[0];
        out[1] ^= trans[1];
        out[2] ^= trans[2];
        out[3] ^= trans[3];
        memcpy( tmp, trans, 32 );
    }
    sodium_munlock( trans, 32 );
    sodium_munlock( tmp, 32 );
    return 0;
}*/

int encrypt_encrypt(uint8_t *cipherText, const uint8_t *plainText, int textLength,
                 const uint8_t *key, const uint8_t *iv, const uint8_t *add, int add_len, uint8_t *tag) {
    gcm_context ctx;
    uint8_t miv[12] = {0};
    size_t iv_len = 0;
    size_t tag_len = 0;
    int retVal;

    if( iv ) memcpy( miv, iv, iv_len );
    if( iv ) iv_len = 12;
    if( tag ) tag_len = 16;
    if( !add ) add_len = 0;

    gcm_setkey( &ctx, (uint8_t*)key, 32 );
    retVal = gcm_crypt_and_tag(
            &ctx, ENCRYPT,
            miv, iv_len,
            add, add_len,
            plainText, cipherText, textLength,
            tag, tag_len );
    gcm_zero_ctx( &ctx );
    return retVal;
}

int encrypt_decrypt(uint8_t *plainText, const uint8_t *cipherText, int textLength,
                 const uint8_t *key, const uint8_t *iv, const uint8_t *add, int add_len, const uint8_t *tag) {
    gcm_context ctx;
    uint8_t miv[12] = {0};
    size_t iv_len = 0;
    size_t tag_len = 0;
    int retVal;

    if( iv ) memcpy( miv, iv, iv_len );
    if( iv ) iv_len = 12;
    if( tag ) tag_len = 16;
    if( !add ) add_len = 0;

    gcm_setkey( &ctx, (uint8_t*)key, 32 );
    retVal = gcm_auth_decrypt(
            &ctx, miv, iv_len,
            add, add_len,
            cipherText, plainText, textLength,
            tag, tag_len );
    gcm_zero_ctx( &ctx );
    return retVal;

}

int encrypt_sign(const uint8_t* msg,int n_msg,const uint8_t * sk, uint8_t* sig)
{
    return crypto_sign_detached(sig, NULL, msg, n_msg, sk);
}

int encrypt_recover(const uint8_t *msg, int n_msg, const uint8_t *pk, uint8_t *sig)
{
    return crypto_sign_verify_detached(sig, msg, n_msg, pk);
}