/*
 * sha512 - Implementation of SHA256 hash function.
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

#define ROR64( value, bits ) (((value) >> (bits)) | ((value) << (64 - (bits))))

#define MIN( x, y ) ( ((x)<(y))?(x):(y) )

#define LOAD64H( x, y )                                                      \
   { x = (((uint64_t)((y)[0] & 255))<<56)|(((uint64_t)((y)[1] & 255))<<48) | \
         (((uint64_t)((y)[2] & 255))<<40)|(((uint64_t)((y)[3] & 255))<<32) | \
         (((uint64_t)((y)[4] & 255))<<24)|(((uint64_t)((y)[5] & 255))<<16) | \
         (((uint64_t)((y)[6] & 255))<<8)|(((uint64_t)((y)[7] & 255))); }

#define STORE64H( x, y )                                                                     \
   { (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);     \
     (y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);     \
     (y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);     \
     (y)[6] = (uint8_t)(((x)>>8)&255); (y)[7] = (uint8_t)((x)&255); }

/*
 * CONSTANTS
 */

// The K array
static const uint64_t K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

#define BLOCK_SIZE          128

/*
 * INTERNAL FUNCTIONS
 */

// Various logical functions
#define Ch( x, y, z )     (z ^ (x & (y ^ z)))
#define Maj(x, y, z )     (((x | y) & z) | (x & y))
#define S( x, n )         ROR64( x, n )
#define R( x, n )         (((x)&0xFFFFFFFFFFFFFFFFULL)>>((uint64_t)n))
#define Sigma0( x )       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1( x )       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0( x )       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1( x )       (S(x, 19) ^ S(x, 61) ^ R(x, 6))

#define Sha512Round( a, b, c, d, e, f, g, h, i )       \
     t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                    \
     d += t0;                                          \
     h  = t0 + t1;

// TransformFunction: Compress 1024-bits
static void
TransformFunction (pcutils_sha512_ctxt* ctxt, uint8_t const* buff)
{
    uint64_t    S[8];
    uint64_t    W[80];
    uint64_t    t0;
    uint64_t    t1;
    int         i;

    // Copy state into S
    for( i=0; i<8; i++ )
    {
        S[i] = ctxt->state[i];
    }

    // Copy the state into 1024-bits into W[0..15]
    for( i=0; i<16; i++ )
    {
        LOAD64H(W[i], buff + (8*i));
    }

    // Fill W[16..79]
    for( i=16; i<80; i++ )
    {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    // Compress
     for( i=0; i<80; i+=8 )
     {
         Sha512Round(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
         Sha512Round(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
         Sha512Round(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
         Sha512Round(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
         Sha512Round(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
         Sha512Round(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
         Sha512Round(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
         Sha512Round(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
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
pcutils_sha512_begin(pcutils_sha512_ctxt* ctxt)
{
    ctxt->curlen = 0;
    ctxt->length = 0;
    ctxt->state[0] = 0x6a09e667f3bcc908ULL;
    ctxt->state[1] = 0xbb67ae8584caa73bULL;
    ctxt->state[2] = 0x3c6ef372fe94f82bULL;
    ctxt->state[3] = 0xa54ff53a5f1d36f1ULL;
    ctxt->state[4] = 0x510e527fade682d1ULL;
    ctxt->state[5] = 0x9b05688c2b3e6c1fULL;
    ctxt->state[6] = 0x1f83d9abfb41bd6bULL;
    ctxt->state[7] = 0x5be0cd19137e2179ULL;
}

void
pcutils_sha512_hash(pcutils_sha512_ctxt* ctxt, const void *buff, size_t buff_sz)
{
    size_t    n;

    if( ctxt->curlen > sizeof(ctxt->buf) )
    {
       return;
    }

    while( buff_sz > 0 )
    {
        if( ctxt->curlen == 0 && buff_sz >= BLOCK_SIZE )
        {
           TransformFunction( ctxt, (uint8_t *)buff );
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
pcutils_sha512_end(pcutils_sha512_ctxt* ctxt, unsigned char *digest)
{
    int i;

    if( ctxt->curlen >= sizeof(ctxt->buf) )
    {
       return;
    }

    // Increase the length of the message
    ctxt->length += ctxt->curlen * 8ULL;

    // Append the '1' bit
    ctxt->buf[ctxt->curlen++] = (uint8_t)0x80;

    // If the length is currently above 112 bytes we append zeros
    // then compress.  Then we can fall back to padding zeros and length
    // encoding like normal.
    if( ctxt->curlen > 112 )
    {
        while( ctxt->curlen < 128 )
        {
            ctxt->buf[ctxt->curlen++] = (uint8_t)0;
        }
        TransformFunction( ctxt, ctxt->buf );
        ctxt->curlen = 0;
    }

    // Pad up to 120 bytes of zeroes
    // note: that from 112 to 120 is the 64 MSB of the length.
    // We assume that you won't hash > 2^64 bits of data... :-)
    while( ctxt->curlen < 120 )
    {
        ctxt->buf[ctxt->curlen++] = (uint8_t)0;
    }

    // Store length
    STORE64H( ctxt->length, ctxt->buf+120 );
    TransformFunction( ctxt, ctxt->buf );

    // Copy output
    for( i=0; i<8; i++ )
    {
        STORE64H( ctxt->state[i], digest+(8*i) );
    }
}

void
pcutils_sha512_calc_digest(const void *data, size_t data_len,
        unsigned char *digest)
{
    pcutils_sha512_ctxt context;

    pcutils_sha512_begin(&context);
    pcutils_sha512_hash(&context, data, data_len);
    pcutils_sha512_end(&context, digest);
}

