/* prng.h -- Pseudo Random Numbers
 *
 * Copyright (C) 2010--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the library tinydtls. Please see
 * README for terms of use. 
 */

/** 
 * @file prng.h
 * @brief Pseudo Random Numbers
 */

#ifndef _DTLS_PRNG_H_
#define _DTLS_PRNG_H_

#include "tinydtls.h"

/** 
 * @defgroup prng Pseudo Random Numbers
 * @{
 */

#ifdef WITH_CONTIKI
// FIXME maybe we should put this 2 lines below the HAVE_PRNG condition
#include <string.h>
#include "random.h"

#  ifdef HAVE_PRNG
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
    return contiki_prng_impl(buf, len);
}

#  else

/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
    unsigned short v = random_rand();
    while (len > sizeof(v)) {
        memcpy(buf, &v, sizeof(v));
        len -= sizeof(v);
        buf += sizeof(v);
        v = random_rand();
    }

    memcpy(buf, &v, len);
    return 1;
}
#  endif /* HAVE_PRNG */

static inline void
dtls_prng_init(unsigned short seed) {
    random_init(seed);
}

#elif defined WITH_ARDUINO

unsigned long (*get_rand)(unsigned int max);

/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
    while (len--)
        *buf++ = (*get_rand)(256) & 0xFF;
    return 1;
}

static inline void
dtls_prng_init(unsigned long (*get_r)(unsigned int max)) {
    get_rand = get_r;
}

#else

#include <stdlib.h>

/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
    while (len--)
        *buf++ = rand() & 0xFF;
    return 1;
}

static inline void
dtls_prng_init(unsigned short seed) {
    srand(seed);
}

/** @} */
#endif /* WITH_CONTIKI */


#endif /* _DTLS_PRNG_H_ */
