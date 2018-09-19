/**
 * @cond internal
 * @file shake.c
 * @copyright
 *   Uses public domain code by Mathias Panzenböck \n
 *   Uses CC0 code by David Leon Gil, 2015 \n
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Copyright (c) 2018 the libgoldilocks contributors.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 * @author Mike Hamburg
 * @brief SHA-3-n and SHAKE-n instances.
 * @warning EXPERIMENTAL!  The names, parameter orders etc are likely to change.
 */

#define __STDC_WANT_LIB_EXT1__ 1 /* for memset_s */
#define _BSD_SOURCE 1 /* for endian */
#define _DEFAULT_SOURCE 1 /* for endian with glibc 2.20 */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "portable_endian.h"
#include "keccak_internal.h"
#include <goldilocks/shake.h>

#define FLAG_ABSORBING 'A'
#define FLAG_SQUEEZING 'Z'

/** Constants. **/
static const uint8_t pi[24] = {
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
};

#define RC_B(x,n) ((((x##ull)>>n)&1)<<((1<<n)-1))
#define RC_X(x) (RC_B(x,0)|RC_B(x,1)|RC_B(x,2)|RC_B(x,3)|RC_B(x,4)|RC_B(x,5)|RC_B(x,6))
static const uint64_t RC[24] = {
    RC_X(0x01), RC_X(0x1a), RC_X(0x5e), RC_X(0x70), RC_X(0x1f), RC_X(0x21),
    RC_X(0x79), RC_X(0x55), RC_X(0x0e), RC_X(0x0c), RC_X(0x35), RC_X(0x26),
    RC_X(0x3f), RC_X(0x4f), RC_X(0x5d), RC_X(0x53), RC_X(0x52), RC_X(0x48),
    RC_X(0x16), RC_X(0x66), RC_X(0x79), RC_X(0x58), RC_X(0x21), RC_X(0x74)
};

static inline uint64_t rol(uint64_t x, int s) {
    return (x << s) | (x >> (64 - s));
}

/* Helper macros to unroll the permutation. */
#define REPEAT5(e) e e e e e
#define FOR51(v, e) v = 0; REPEAT5(e; v += 1;)
#ifndef SHAKE_NO_UNROLL_LOOPS
#    define FOR55(v, e) v = 0; REPEAT5(e; v += 5;)
#    define REPEAT24(e) e e e e e e e e e e e e e e e e e e e e e e e e
#else
#    define FOR55(v, e) for (v=0; v<25; v+= 5) { e; }
#    define REPEAT24(e) {int _j=0; for (_j=0; _j<24; _j++) { e }}
#endif

/*** The Keccak-f[1600] permutation ***/
void keccakf(kdomain_u state, uint8_t start_round) {
    uint64_t* a = state->w;
    uint64_t b[5] = {0}, t, u;
    uint8_t x, y, i;

    for (i=0; i<25; i++) a[i] = le64toh(a[i]);

    for (i = start_round; i < 24; i++) {
        FOR51(x, b[x] = 0; )
        FOR55(y, FOR51(x, b[x] ^= a[x + y]; ))
        FOR55(y, FOR51(x,
            a[y + x] ^= b[(x + 4) % 5] ^ rol(b[(x + 1) % 5], 1);
        ))
        // Rho and pi
        t = a[1];
        x = y = 0;
        REPEAT24(u = a[pi[x]]; y += x+1; a[pi[x]] = rol(t, y % 64); t = u; x++; )
        // Chi
        FOR55(y,
             FOR51(x, b[x] = a[y + x];)
             FOR51(x, a[y + x] = b[x] ^ ((~b[(x + 1) % 5]) & b[(x + 2) % 5]);)
        )
        // Iota
        a[0] ^= RC[i];
    }

    for (i=0; i<25; i++) a[i] = htole64(a[i]);
}

goldilocks_error_t goldilocks_sha3_update (
    struct goldilocks_keccak_sponge_s * __restrict__ goldilocks_sponge,
    const uint8_t *in,
    size_t len
) {
    assert(goldilocks_sponge->params->position < goldilocks_sponge->params->rate);
    assert(goldilocks_sponge->params->rate < sizeof(goldilocks_sponge->state));
    assert(goldilocks_sponge->params->flags == FLAG_ABSORBING);
    while (len) {
        size_t cando = goldilocks_sponge->params->rate - goldilocks_sponge->params->position, i;
        uint8_t* state = &goldilocks_sponge->state->b[goldilocks_sponge->params->position];
        if (cando > len) {
            for (i = 0; i < len; i += 1) state[i] ^= in[i];
            goldilocks_sponge->params->position += len;
            break;
        } else {
            for (i = 0; i < cando; i += 1) state[i] ^= in[i];
            dokeccak(goldilocks_sponge);
            len -= cando;
            in += cando;
        }
    }
    return (goldilocks_sponge->params->flags == FLAG_ABSORBING) ? GOLDILOCKS_SUCCESS : GOLDILOCKS_FAILURE;
}

goldilocks_error_t goldilocks_sha3_output (
    goldilocks_keccak_sponge_p goldilocks_sponge,
    uint8_t * __restrict__ out,
    size_t len
) {
    goldilocks_error_t ret = GOLDILOCKS_SUCCESS;
    assert(goldilocks_sponge->params->position < goldilocks_sponge->params->rate);
    assert(goldilocks_sponge->params->rate < sizeof(goldilocks_sponge->state));

    if (goldilocks_sponge->params->max_out != 0xFF) {
        if (goldilocks_sponge->params->remaining >= len) {
            goldilocks_sponge->params->remaining -= len;
        } else {
            goldilocks_sponge->params->remaining = 0;
            ret = GOLDILOCKS_FAILURE;
        }
    }

    switch (goldilocks_sponge->params->flags) {
    case FLAG_SQUEEZING: break;
    case FLAG_ABSORBING:
        {
            uint8_t* state = goldilocks_sponge->state->b;
            state[goldilocks_sponge->params->position] ^= goldilocks_sponge->params->pad;
            state[goldilocks_sponge->params->rate - 1] ^= goldilocks_sponge->params->rate_pad;
            dokeccak(goldilocks_sponge);
            goldilocks_sponge->params->flags = FLAG_SQUEEZING;
            break;
        }
    default:
        assert(0);
    }

    while (len) {
        size_t cando = goldilocks_sponge->params->rate - goldilocks_sponge->params->position;
        uint8_t* state = &goldilocks_sponge->state->b[goldilocks_sponge->params->position];
        if (cando > len) {
            memcpy(out, state, len);
            goldilocks_sponge->params->position += len;
            return ret;
        } else {
            memcpy(out, state, cando);
            dokeccak(goldilocks_sponge);
            len -= cando;
            out += cando;
        }
    }
    return ret;
}

goldilocks_error_t goldilocks_sha3_final (
    goldilocks_keccak_sponge_p goldilocks_sponge,
    uint8_t * __restrict__ out,
    size_t len
) {
    goldilocks_error_t ret = goldilocks_sha3_output(goldilocks_sponge,out,len);
    goldilocks_sha3_reset(goldilocks_sponge);
    return ret;
}

void goldilocks_sha3_reset (
    goldilocks_keccak_sponge_p goldilocks_sponge
) {
    goldilocks_sha3_init(goldilocks_sponge, goldilocks_sponge->params);
    goldilocks_sponge->params->flags = FLAG_ABSORBING;
    goldilocks_sponge->params->remaining = goldilocks_sponge->params->max_out;
}

void goldilocks_sha3_destroy (goldilocks_keccak_sponge_p goldilocks_sponge) {
    goldilocks_bzero(goldilocks_sponge, sizeof(goldilocks_keccak_sponge_p));
}

void goldilocks_sha3_init (
    goldilocks_keccak_sponge_p goldilocks_sponge,
    const struct goldilocks_kparams_s *params
) {
    memset(goldilocks_sponge->state, 0, sizeof(goldilocks_sponge->state));
    goldilocks_sponge->params[0] = params[0];
    goldilocks_sponge->params->position = 0;
}

goldilocks_error_t goldilocks_sha3_hash (
    uint8_t *out,
    size_t outlen,
    const uint8_t *in,
    size_t inlen,
    const struct goldilocks_kparams_s *params
) {
    goldilocks_keccak_sponge_p goldilocks_sponge;
    goldilocks_error_t ret;
    goldilocks_sha3_init(goldilocks_sponge, params);
    goldilocks_sha3_update(goldilocks_sponge, in, inlen);
    ret = goldilocks_sha3_output(goldilocks_sponge, out, outlen);
    goldilocks_sha3_destroy(goldilocks_sponge);
    return ret;
}

#define DEFSHAKE(n) \
    const struct goldilocks_kparams_s GOLDILOCKS_SHAKE##n##_params_s = \
        { 0, FLAG_ABSORBING, 200-n/4, 0, 0x1f, 0x80, 0xFF, 0xFF };

#define DEFSHA3(n) \
    const struct goldilocks_kparams_s GOLDILOCKS_SHA3_##n##_params_s = \
        { 0, FLAG_ABSORBING, 200-n/4, 0, 0x06, 0x80, n/8, n/8 };

size_t goldilocks_sha3_default_output_bytes (
    const goldilocks_keccak_sponge_p s
) {
    return (s->params->max_out == 0xFF)
        ? (200-s->params->rate)
        : ((200-s->params->rate)/2);
}

size_t goldilocks_sha3_max_output_bytes (
    const goldilocks_keccak_sponge_p s
) {
    return (s->params->max_out == 0xFF)
        ? SIZE_MAX
        : (size_t)((200-s->params->rate)/2);
}

DEFSHAKE(128)
DEFSHAKE(256)
DEFSHA3(224)
DEFSHA3(256)
DEFSHA3(384)
DEFSHA3(512)

/* FUTURE: Keyak instances, etc */
