/*
 * sha256 - Implementation of SHA256 hash function.
 *
 * Original author: Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 * Modified by WaterJuice retaining Public Domain license.
 *
 * This is free and unencumbered software released into the public domain
 *  - June 2013 waterjuice.org
 *
 * Modify the source code with Unix C style by Vincent Wei
 *  - Nov. 2020
 *
 * Copyright (C) 2020 FMSoft <https://www.fmsoft.cn>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "purc-utils.h"

/*
 * MACROS
 */

#define ror(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))

#define MIN(x, y) ( ((x)<(y))?(x):(y) )

#define STORE32H(x, y)                                                          \
     { (y)[0] = (uint8_t)(((x)>>24)&255); (y)[1] = (uint8_t)(((x)>>16)&255);    \
       (y)[2] = (uint8_t)(((x)>>8)&255); (y)[3] = (uint8_t)((x)&255); }

#define LOAD32H(x, y)                               \
     { x = ((uint32_t)((y)[0] & 255)<<24) |         \
           ((uint32_t)((y)[1] & 255)<<16) |         \
           ((uint32_t)((y)[2] & 255)<<8)  |         \
           ((uint32_t)((y)[3] & 255)); }

#define STORE64H(x, y)                                                          \
   { (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);      \
     (y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);      \
     (y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);      \
     (y)[6] = (uint8_t)(((x)>>8)&255); (y)[7] = (uint8_t)((x)&255); }

/*
 * CONSTANTS
 */

// The K array
static const uint32_t K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

#define BLOCK_SIZE          64

/*
 * INTERNAL FUNCTIONS
 */

// Various logical functions
#define Ch( x, y, z )     (z ^ (x & (y ^ z)))
#define Maj( x, y, z )    (((x | y) & z) | (x & y))
#define S( x, n )         ror((x),(n))
#define R( x, n )         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0( x )       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1( x )       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0( x )       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1( x )       (S(x, 17) ^ S(x, 19) ^ R(x, 10))

#define Sha256Round( a, b, c, d, e, f, g, h, i )       \
     t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                    \
     d += t0;                                          \
     h  = t0 + t1;

// TransformFunction: Compress 512-bits
static void
TransformFunction (pcutils_sha256_ctxt* ctxt, uint8_t const* buff)
{
    uint32_t    S[8];
    uint32_t    W[64];
    uint32_t    t0;
    uint32_t    t1;
    uint32_t    t;
    int         i;

    // Copy state into S
    for( i=0; i<8; i++ )
    {
        S[i] = ctxt->state[i];
    }

    // Copy the state into 512-bits into W[0..15]
    for( i=0; i<16; i++ )
    {
        LOAD32H( W[i], buff + (4*i) );
    }

    // Fill W[16..63]
    for( i=16; i<64; i++ )
    {
        W[i] = Gamma1( W[i-2]) + W[i-7] + Gamma0( W[i-15] ) + W[i-16];
    }

    // Compress
    for( i=0; i<64; i++ )
    {
        Sha256Round( S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i );
        t = S[7];
        S[7] = S[6];
        S[6] = S[5];
        S[5] = S[4];
        S[4] = S[3];
        S[3] = S[2];
        S[2] = S[1];
        S[1] = S[0];
        S[0] = t;
    }

    // Feedback
    for( i=0; i<8; i++ )
    {
        ctxt->state[i] = ctxt->state[i] + S[i];
    }
}

/*
 * PUBLIC FUNCTIONS
 */
void
pcutils_sha256_begin(pcutils_sha256_ctxt *ctxt)
{
    ctxt->curlen = 0;
    ctxt->length = 0;
    ctxt->state[0] = 0x6A09E667UL;
    ctxt->state[1] = 0xBB67AE85UL;
    ctxt->state[2] = 0x3C6EF372UL;
    ctxt->state[3] = 0xA54FF53AUL;
    ctxt->state[4] = 0x510E527FUL;
    ctxt->state[5] = 0x9B05688CUL;
    ctxt->state[6] = 0x1F83D9ABUL;
    ctxt->state[7] = 0x5BE0CD19UL;
}

void
pcutils_sha256_hash(pcutils_sha256_ctxt *ctxt, const void *buff, size_t buff_sz)
{
    size_t n;

    if( ctxt->curlen > sizeof(ctxt->buf) )
    {
       return;
    }

    while( buff_sz > 0 )
    {
        if( ctxt->curlen == 0 && buff_sz >= BLOCK_SIZE )
        {
           TransformFunction( ctxt, (uint8_t*)buff );
           ctxt->length += BLOCK_SIZE * 8;
           buff = (uint8_t*)buff + BLOCK_SIZE;
           buff_sz -= BLOCK_SIZE;
        }
        else
        {
           n = MIN( buff_sz, (BLOCK_SIZE - ctxt->curlen) );
           memcpy( ctxt->buf + ctxt->curlen, buff, (size_t)n );
           ctxt->curlen += n;
           buff = (uint8_t*)buff + n;
           buff_sz -= n;
           if( ctxt->curlen == BLOCK_SIZE )
           {
              TransformFunction( ctxt, ctxt->buf );
              ctxt->length += 8*BLOCK_SIZE;
              ctxt->curlen = 0;
           }
       }
    }
}

void
pcutils_sha256_end(pcutils_sha256_ctxt *ctxt, unsigned char *digest)
{
    int i;

    if (ctxt->curlen >= sizeof(ctxt->buf)) {
       return;
    }

    // Increase the length of the message
    ctxt->length += ctxt->curlen * 8;

    // Append the '1' bit
    ctxt->buf[ctxt->curlen++] = (uint8_t)0x80;

    // if the length is currently above 56 bytes we append zeros
    // then compress.  Then we can fall back to padding zeros and length
    // encoding like normal.
    if (ctxt->curlen > 56) {
        while (ctxt->curlen < 64) {
            ctxt->buf[ctxt->curlen++] = (uint8_t)0;
        }
        TransformFunction (ctxt, ctxt->buf);
        ctxt->curlen = 0;
    }

    // Pad up to 56 bytes of zeroes
    while (ctxt->curlen < 56) {
        ctxt->buf[ctxt->curlen++] = (uint8_t)0;
    }

    // Store length
    STORE64H (ctxt->length, ctxt->buf+56);
    TransformFunction (ctxt, ctxt->buf);

    // Copy output
    for (i=0; i<8; i++) {
        STORE32H (ctxt->state[i], digest+(4*i));
    }
}

void
sha256_calc_digest(const void* data, size_t data_len, unsigned char *digest)
{
    pcutils_sha256_ctxt context;

    pcutils_sha256_begin(&context);
    pcutils_sha256_hash(&context, data, data_len);
    pcutils_sha256_end(&context, digest);
}

