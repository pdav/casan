#ifndef BLA_H_
#define BLA_H_

#include <stdint.h>

/**
 * \brief encrypt with 128 bit key.
 *
 * This function encrypts one block with the AES algorithm under control of
 * a keyschedule produced from a 128 bit key.
 * \param buffer pointer to the block to encrypt
 * \param ctx    pointer to the key schedule
 */

/**
 * \brief decrypt with 128 bit key.
 *
 * This function decrypts one block with the AES algorithm under control of
 * a keyschedule produced from a 128 bit key.
 * \param buffer pointer to the block to decrypt
 * \param ctx    pointer to the key schedule
 */

/**
 * \brief initialize the keyschedule
 *
 * This function computes the keyschedule from a given key with a given length
 * and stores it in the context variable
 * \param key       pointer to the key material
 * \param keysize_b length of the key in bits (valid are 128, 192 and 256)
 * \param ctx       pointer to the context where the keyschedule should be stored
 */

/**
 * \brief initialize the keyschedule for 128 bit key
 *
 * This function computes the keyschedule from a given 128 bit key
 * and stores it in the context variable
 * \param key       pointer to the key material
 * \param ctx       pointer to the context where the keyschedule should be stored
 */


#if 0
typedef struct{
	uint8_t ks[16];
} aes_roundkey_t;

typedef struct{
	aes_roundkey_t key[10+1];
} aes128_ctx_t;

typedef struct{
	aes_roundkey_t key[1]; /* just to avoid the warning */
} aes_genctx_t;

typedef struct{
	uint8_t s[16];
} aes_cipher_state_t;

#else
struct aes_roundkey {
    unsigned char ks[16];
};

typedef struct aes_roundkey aes_roundkey_t;

typedef struct {
    uint8_t key[10+1];
} aes128_ctx_t;
#endif

#if 0
void aes_encrypt_core(aes_cipher_state_t *state, const aes_genctx_t *ks,
        uint8_t rounds);
void aes_decrypt_core(aes_cipher_state_t *state,const aes_genctx_t *ks, uint8_t rounds);
extern const uint8_t aes_invsbox[];
extern const uint8_t aes_sbox[];
void aes_init(const void *key, uint16_t keysize_b, aes_genctx_t *ctx);
#else
void aes128_enc(void *buffer, aes128_ctx_t *ctx);
void aes128_dec(void *buffer, aes128_ctx_t *ctx);
void aes128_init(const void *key, aes128_ctx_t *ctx);
#endif

#endif
