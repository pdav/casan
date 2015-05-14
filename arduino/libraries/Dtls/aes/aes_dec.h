#ifndef AES_DEC_H_
#define AES_DEC_H_
#include "aes_types.h"
#include <stdint.h>

void aes_decrypt_core(aes_cipher_state_t *state,const aes_genctx_t *ks, uint8_t rounds);

#endif
