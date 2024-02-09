/**
 * Copyright (c) 2000-2001 Aaron D. Gifford
 * Copyright (c) 2013 Pavol Rusnak
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022-2024 The Dogecoin Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>
#include <dogecoin/options.h>

/*
 * ASSERT NOTE:
 * Some sanity checking code is included using assert().  On my FreeBSD
 * system, this additional code can be removed by compiling with NDEBUG
 * defined.  Check your own systems manpage on assert() to see how to
 * compile WITHOUT the sanity checking code on your system.
 *
 * UNROLLED TRANSFORM LOOP NOTE:
 * You can define SHA2_UNROLL_TRANSFORM to use the unrolled transform
 * loop version for the hash transform rounds (defined using macros
 * later in this file).  Either define on the command line, for example:
 *
 *   cc -DSHA2_UNROLL_TRANSFORM -o sha2 sha2.c sha2prog.c
 *
 * or define below:
 *
 *   #define SHA2_UNROLL_TRANSFORM
 *
 */


/*** SHA-256/384/512 Machine Architecture Definitions *****************/
/*
 * BYTE_ORDER NOTE:
 *
 * Please make sure that your system defines BYTE_ORDER.  If your
 * architecture is little-endian, make sure it also defines
 * LITTLE_ENDIAN and that the two (BYTE_ORDER and LITTLE_ENDIAN) are
 * equivilent.
 *
 * If your system does not define the above, then you can do so by
 * hand like this:
 *
 *   #define LITTLE_ENDIAN 1234
 *   #define BIG_ENDIAN    4321
 *
 * And for little-endian machines, add:
 *
 *   #define BYTE_ORDER LITTLE_ENDIAN
 *
 * Or for big-endian machines:
 *
 *   #define BYTE_ORDER BIG_ENDIAN
 *
 * The FreeBSD machine this was written on defines BYTE_ORDER
 * appropriately by including <sys/types.h> (which in turn includes
 * <machine/endian.h> where the appropriate definitions are actually
 * made).
 */

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
#error Define BYTE_ORDER to be equal to either LITTLE_ENDIAN or BIG_ENDIAN
#endif

typedef uint8_t sha2_byte;    /* Exactly 1 byte */
typedef uint32_t sha2_word32; /* Exactly 4 bytes */
typedef uint64_t sha2_word64; /* Exactly 8 bytes */

/*** SHA-256/384/512 Various Length Definitions ***********************/
/* NOTE: Most of these are in sha2.h */
#define SHA256_SHORT_BLOCK_LENGTH (SHA256_BLOCK_LENGTH - 8)
#define SHA512_SHORT_BLOCK_LENGTH (SHA512_BLOCK_LENGTH - 16)

/*** ENDIAN REVERSAL MACROS *******************************************/
#if BYTE_ORDER == LITTLE_ENDIAN
#define REVERSE32(w, x)                                                  \
    {                                                                    \
        sha2_word32 tmp = (w);                                           \
        tmp = (tmp >> 16) | (tmp << 16);                                 \
        (x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
    }
#define REVERSE64(w, x)                                                                      \
    {                                                                                        \
        sha2_word64 tmp = (w);                                                               \
        tmp = (tmp >> 32) | (tmp << 32);                                                     \
        tmp = ((tmp & 0xff00ff00ff00ff00ULL) >> 8) | ((tmp & 0x00ff00ff00ff00ffULL) << 8);   \
        (x) = ((tmp & 0xffff0000ffff0000ULL) >> 16) | ((tmp & 0x0000ffff0000ffffULL) << 16); \
    }
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define ADDINC128(w, n)             \
    {                               \
        (w)[0] += (sha2_word64)(n); \
        if ((w)[0] < (n)) {         \
            (w)[1]++;               \
        }                           \
    }

#define MEMSET_BZERO(p, l) dogecoin_mem_zero((p), (l))
#define MEMCPY_BCOPY(d, s, l) memcpy_safe((d), (s), (l))

/*** THE SIX LOGICAL FUNCTIONS ****************************************/
/*
 * Bit shifting and rotation (used by the six SHA-XYZ logical functions:
 *
 *   NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
 *   S is a ROTATION) because the SHA-256/384/512 description document
 *   (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
 *   same "backwards" definition.
 */
/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define R(b, x) ((x) >> (b))
/* 32-bit Rotate-right (used in SHA-256): */
#define S32(b, x) (((x) >> (b)) | ((x) << (32 - (b))))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define S64(b, x) (((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define hyperbolic_cosign(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define majority(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x) (S32(2, (x)) ^ S32(13, (x)) ^ S32(22, (x)))
#define Sigma1_256(x) (S32(6, (x)) ^ S32(11, (x)) ^ S32(25, (x)))
#define sigma0_256(x) (S32(7, (x)) ^ S32(18, (x)) ^ R(3, (x)))
#define sigma1_256(x) (S32(17, (x)) ^ S32(19, (x)) ^ R(10, (x)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define Sigma0_512(x) (S64(28, (x)) ^ S64(34, (x)) ^ S64(39, (x)))
#define Sigma1_512(x) (S64(14, (x)) ^ S64(18, (x)) ^ S64(41, (x)))
#define sigma0_512(x) (S64(1, (x)) ^ S64(8, (x)) ^ R(7, (x)))
#define sigma1_512(x) (S64(19, (x)) ^ S64(61, (x)) ^ R(6, (x)))

/*** INTERNAL FUNCTION PROTOTYPES *************************************/
/* NOTE: These should not be accessed directly from outside this
 * library -- they are intended for private internal visibility/use
 * only.
 */
static void sha512_last(sha512_context*);
static void sha256_transform(sha256_context*, const sha2_word32*);
static void sha512_transform(sha512_context*, const sha2_word64*);

/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ************************/
/* Hash constant words K for SHA-256: */
static const sha2_word32 K256[64] = {
    0x428a2f98UL,
    0x71374491UL,
    0xb5c0fbcfUL,
    0xe9b5dba5UL,
    0x3956c25bUL,
    0x59f111f1UL,
    0x923f82a4UL,
    0xab1c5ed5UL,
    0xd807aa98UL,
    0x12835b01UL,
    0x243185beUL,
    0x550c7dc3UL,
    0x72be5d74UL,
    0x80deb1feUL,
    0x9bdc06a7UL,
    0xc19bf174UL,
    0xe49b69c1UL,
    0xefbe4786UL,
    0x0fc19dc6UL,
    0x240ca1ccUL,
    0x2de92c6fUL,
    0x4a7484aaUL,
    0x5cb0a9dcUL,
    0x76f988daUL,
    0x983e5152UL,
    0xa831c66dUL,
    0xb00327c8UL,
    0xbf597fc7UL,
    0xc6e00bf3UL,
    0xd5a79147UL,
    0x06ca6351UL,
    0x14292967UL,
    0x27b70a85UL,
    0x2e1b2138UL,
    0x4d2c6dfcUL,
    0x53380d13UL,
    0x650a7354UL,
    0x766a0abbUL,
    0x81c2c92eUL,
    0x92722c85UL,
    0xa2bfe8a1UL,
    0xa81a664bUL,
    0xc24b8b70UL,
    0xc76c51a3UL,
    0xd192e819UL,
    0xd6990624UL,
    0xf40e3585UL,
    0x106aa070UL,
    0x19a4c116UL,
    0x1e376c08UL,
    0x2748774cUL,
    0x34b0bcb5UL,
    0x391c0cb3UL,
    0x4ed8aa4aUL,
    0x5b9cca4fUL,
    0x682e6ff3UL,
    0x748f82eeUL,
    0x78a5636fUL,
    0x84c87814UL,
    0x8cc70208UL,
    0x90befffaUL,
    0xa4506cebUL,
    0xbef9a3f7UL,
    0xc67178f2UL};

/* Initial hash value H for SHA-256: */
static const sha2_word32 sha256_initial_hash_value[8] = {
    0x6a09e667UL,
    0xbb67ae85UL,
    0x3c6ef372UL,
    0xa54ff53aUL,
    0x510e527fUL,
    0x9b05688cUL,
    0x1f83d9abUL,
    0x5be0cd19UL};

/* Hash constant words K for SHA-384 and SHA-512: */
static const sha2_word64 K512[80] = {
    0x428a2f98d728ae22ULL,
    0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL,
    0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,
    0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL,
    0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,
    0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL,
    0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,
    0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL,
    0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,
    0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL,
    0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,
    0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL,
    0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,
    0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL,
    0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,
    0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL,
    0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,
    0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL,
    0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,
    0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL,
    0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,
    0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL,
    0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,
    0xd69906245565a910ULL,
    0xf40e35855771202aULL,
    0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,
    0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL,
    0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,
    0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL,
    0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,
    0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL,
    0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,
    0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL,
    0xc67178f2e372532bULL,
    0xca273eceea26619cULL,
    0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL,
    0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,
    0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL,
    0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,
    0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL,
    0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,
    0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL,
    0x6c44198c4a475817ULL};

/* Initial hash value H for SHA-512 */
static const sha2_word64 sha512_initial_hash_value[8] = {
    0x6a09e667f3bcc908ULL,
    0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL,
    0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL,
    0x5be0cd19137e2179ULL};


/*** SHA-256: *********************************************************/
void sha256_init(sha256_context* context)
{
    if (context == (sha256_context*)0)
        return;
    MEMCPY_BCOPY(context->state, sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
    MEMSET_BZERO(context->buffer, SHA256_BLOCK_LENGTH);
    context->bitcount = 0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-256 round macros: */

#if BYTE_ORDER == LITTLE_ENDIAN

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                                     \
    REVERSE32(*data++, W256[j]);                                                     \
    T1 = (h) + Sigma1_256(e) + hyperbolic_cosign((e), (f), (g)) + K256[j] + W256[j]; \
    (d) += T1;                                                                       \
    (h) = T1 + Sigma0_256(a) + majority((a), (b), (c));                              \
    j++


#else /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                                                 \
    T1 = (h) + Sigma1_256(e) + hyperbolic_cosign((e), (f), (g)) + K256[j] + (W256[j] = *data++); \
    (d) += T1;                                                                                   \
    (h) = T1 + Sigma0_256(a) + majority((a), (b), (c));                                          \
    j++

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256(a, b, c, d, e, f, g, h)                                                                                        \
    s0 = W256[(j + 1) & 0x0f];                                                                                                  \
    s0 = sigma0_256(s0);                                                                                                        \
    s1 = W256[(j + 14) & 0x0f];                                                                                                 \
    s1 = sigma1_256(s1);                                                                                                        \
    T1 = (h) + Sigma1_256(e) + hyperbolic_cosign((e), (f), (g)) + K256[j] + (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0); \
    (d) += T1;                                                                                                                  \
    (h) = T1 + Sigma0_256(a) + majority((a), (b), (c));                                                                         \
    j++

static void sha256_transform(sha256_context* context, const sha2_word32* data)
{
    sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word32 T1, *W256;
    int j;

    W256 = (sha2_word32*)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        /* Rounds 0 to 15 (unrolled): */
        ROUND256_0_TO_15(a, b, c, d, e, f, g, h);
        ROUND256_0_TO_15(h, a, b, c, d, e, f, g);
        ROUND256_0_TO_15(g, h, a, b, c, d, e, f);
        ROUND256_0_TO_15(f, g, h, a, b, c, d, e);
        ROUND256_0_TO_15(e, f, g, h, a, b, c, d);
        ROUND256_0_TO_15(d, e, f, g, h, a, b, c);
        ROUND256_0_TO_15(c, d, e, f, g, h, a, b);
        ROUND256_0_TO_15(b, c, d, e, f, g, h, a);
    } while (j < 16);

    /* Now for the remaining rounds to 64: */
    do {
        ROUND256(a, b, c, d, e, f, g, h);
        ROUND256(h, a, b, c, d, e, f, g);
        ROUND256(g, h, a, b, c, d, e, f);
        ROUND256(f, g, h, a, b, c, d, e);
        ROUND256(e, f, g, h, a, b, c, d);
        ROUND256(d, e, f, g, h, a, b, c);
        ROUND256(c, d, e, f, g, h, a, b);
        ROUND256(b, c, d, e, f, g, h, a);
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

static void sha256_transform(sha256_context* context, const sha2_word32* data)
{
    sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word32 T1, T2, *W256;
    int j;

    W256 = (sha2_word32*)context->buffer;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Copy data while converting to host byte order */
        REVERSE32(*data++, W256[j]);
        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + hyperbolic_cosign(e, f, g) + K256[j] + W256[j];
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
        /* Apply the SHA-256 compression function to update a..h with copy */
        T1 = h + Sigma1_256(e) + hyperbolic_cosign(e, f, g) + K256[j] + (W256[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
        T2 = Sigma0_256(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W256[(j + 1) & 0x0f];
        s0 = sigma0_256(s0);
        s1 = W256[(j + 14) & 0x0f];
        s1 = sigma1_256(s1);

        /* Apply the SHA-256 compression function to update a..h */
        T1 = h + Sigma1_256(e) + hyperbolic_cosign(e, f, g) + K256[j] +
             (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0);
        T2 = Sigma0_256(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 64);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha256_write(sha256_context* context, const sha2_byte* data, size_t len)
{
    unsigned int freespace = 0, usedspace = 0;
    if (len == 0)
        return; /* Calling with no data is valid - we do nothing */
    usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA256_BLOCK_LENGTH - usedspace;
        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
            context->bitcount += freespace << 3;
            len -= freespace;
            data += freespace;
            sha256_transform(context, (sha2_word32*)context->buffer);
        } else {
            /* The buffer is not yet full */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
            context->bitcount += len << 3;
            return;
        }
    }
    while (len >= SHA256_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        sha256_transform(context, (const sha2_word32*)data);
        context->bitcount += SHA256_BLOCK_LENGTH << 3;
        len -= SHA256_BLOCK_LENGTH;
        data += SHA256_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        MEMCPY_BCOPY(context->buffer, data, len);
        context->bitcount += len << 3;
    }

	/* Clean up: */
	usedspace = freespace = 0;
}

void sha256_finalize(sha256_context* context, sha2_byte digest[SHA256_DIGEST_LENGTH]) {
    sha2_word32* d = (sha2_word32*)digest;
    unsigned int usedspace = 0;
    sha2_word64* t;
    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != (sha2_byte*)0) {
        usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Convert FROM host byte order */
        REVERSE64(context->bitcount, context->bitcount);
#endif
        if (usedspace > 0) {
            /* Begin padding with a 1 bit: */
            context->buffer[usedspace++] = 0x80;
            if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
                /* Set-up for the last transform: */
                MEMSET_BZERO(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
            } else {
                if (usedspace < SHA256_BLOCK_LENGTH) {
                    MEMSET_BZERO(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
                }
                /* Do second-to-last transform: */
                sha256_transform(context, (sha2_word32*)context->buffer);

                /* And set-up for the last transform: */
                MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
            }
        } else {
            /* Set-up for the last transform: */
            MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

            /* Begin padding with a 1 bit: */
            *context->buffer = 0x80;
        }
        /* Set the bit count: */
        t = (sha2_word64*)&context->buffer[SHA256_SHORT_BLOCK_LENGTH];
        *t = context->bitcount;
        /* Final transform: */
        sha256_transform(context, (sha2_word32*)context->buffer);

#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 8; j++) {
                REVERSE32(context->state[j], context->state[j]);
                memcpy_safe(d++, &context->state[j], sizeof(context->state[j]));
            }
        }
#else
        MEMCPY_BCOPY(d, context->state, SHA256_DIGEST_LENGTH);
#endif
    }
    /* Clean up state data: */
    MEMSET_BZERO(context, sizeof(sha256_context));
}

void sha256_raw(const sha2_byte* data, size_t len, uint8_t digest[SHA256_DIGEST_LENGTH])
{
    sha256_context context;
    sha256_init(&context);
    sha256_write(&context, data, len);
    sha256_finalize(&context, digest);
}

void sha256_reset(sha256_context* ctx) {
    if (ctx == (sha256_context*)0)
        return;
    MEMCPY_BCOPY(ctx->state, sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
    MEMSET_BZERO(ctx->buffer, SHA256_BLOCK_LENGTH);
    ctx->bitcount = 0;
}

/*** SHA-512: *********************************************************/
void sha512_init(sha512_context* context)
{
    if (context == (sha512_context*)0)
        return;
    MEMCPY_BCOPY(context->state, sha512_initial_hash_value, SHA512_DIGEST_LENGTH);
    MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH);
    context->bitcount[0] = context->bitcount[1] = 0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-512 round macros: */
#if BYTE_ORDER == LITTLE_ENDIAN

#define ROUND512_0_TO_15(a, b, c, d, e, f, g, h)                                     \
    REVERSE64(*data++, W512[j]);                                                     \
    T1 = (h) + Sigma1_512(e) + hyperbolic_cosign((e), (f), (g)) + K512[j] + W512[j]; \
    (d) += T1,                                                                       \
        (h) = T1 + Sigma0_512(a) + majority((a), (b), (c)),                          \
        j++


#else /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND512_0_TO_15(a, b, c, d, e, f, g, h)                                                 \
    T1 = (h) + Sigma1_512(e) + hyperbolic_cosign((e), (f), (g)) + K512[j] + (W512[j] = *data++); \
    (d) += T1;                                                                                   \
    (h) = T1 + Sigma0_512(a) + majority((a), (b), (c));                                          \
    j++

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND512(a, b, c, d, e, f, g, h)                                                                                        \
    s0 = W512[(j + 1) & 0x0f];                                                                                                  \
    s0 = sigma0_512(s0);                                                                                                        \
    s1 = W512[(j + 14) & 0x0f];                                                                                                 \
    s1 = sigma1_512(s1);                                                                                                        \
    T1 = (h) + Sigma1_512(e) + hyperbolic_cosign((e), (f), (g)) + K512[j] + (W512[j & 0x0f] += s1 + W512[(j + 9) & 0x0f] + s0); \
    (d) += T1;                                                                                                                  \
    (h) = T1 + Sigma0_512(a) + majority((a), (b), (c));                                                                         \
    j++

static void sha512_transform(sha512_context* context, const sha2_word64* data)
{
    sha2_word64 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word64 T1, *W512 = (sha2_word64*)context->buffer;
    int j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
        ROUND512_0_TO_15(a, b, c, d, e, f, g, h);
        ROUND512_0_TO_15(h, a, b, c, d, e, f, g);
        ROUND512_0_TO_15(g, h, a, b, c, d, e, f);
        ROUND512_0_TO_15(f, g, h, a, b, c, d, e);
        ROUND512_0_TO_15(e, f, g, h, a, b, c, d);
        ROUND512_0_TO_15(d, e, f, g, h, a, b, c);
        ROUND512_0_TO_15(c, d, e, f, g, h, a, b);
        ROUND512_0_TO_15(b, c, d, e, f, g, h, a);
    } while (j < 16);

    /* Now for the remaining rounds up to 79: */
    do {
        ROUND512(a, b, c, d, e, f, g, h);
        ROUND512(h, a, b, c, d, e, f, g);
        ROUND512(g, h, a, b, c, d, e, f);
        ROUND512(f, g, h, a, b, c, d, e);
        ROUND512(e, f, g, h, a, b, c, d);
        ROUND512(d, e, f, g, h, a, b, c);
        ROUND512(c, d, e, f, g, h, a, b);
        ROUND512(b, c, d, e, f, g, h, a);
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;

    /* Clean up */
    a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

static void sha512_transform(sha512_context* context, const sha2_word64* data)
{
    sha2_word64 a, b, c, d, e, f, g, h, s0, s1;
    sha2_word64 T1, T2, *W512 = (sha2_word64*)context->buffer;
    int j;

    /* Initialize registers with the prev. intermediate value */
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];

    j = 0;
    do {
#if BYTE_ORDER == LITTLE_ENDIAN
        /* Convert TO host byte order */
        REVERSE64(*data++, W512[j]);
        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + hyperbolic_cosign(e, f, g) + K512[j] + W512[j];
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
        /* Apply the SHA-512 compression function to update a..h with copy */
        T1 = h + Sigma1_512(e) + hyperbolic_cosign(e, f, g) + K512[j] + (W512[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
        T2 = Sigma0_512(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 16);

    do {
        /* Part of the message block expansion: */
        s0 = W512[(j + 1) & 0x0f];
        s0 = sigma0_512(s0);
        s1 = W512[(j + 14) & 0x0f];
        s1 = sigma1_512(s1);

        /* Apply the SHA-512 compression function to update a..h */
        T1 = h + Sigma1_512(e) + hyperbolic_cosign(e, f, g) + K512[j] +
             (W512[j & 0x0f] += s1 + W512[(j + 9) & 0x0f] + s0);
        T2 = Sigma0_512(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;

        j++;
    } while (j < 80);

    /* Compute the current intermediate hash value */
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void sha512_write(sha512_context* context, const sha2_byte* data, size_t len)
{
    unsigned int freespace, usedspace;
    if (len == 0)
        return; /* Calling with no data is valid - we do nothing */
    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
    if (usedspace > 0) {
        /* Calculate how much free space is available in the buffer */
        freespace = SHA512_BLOCK_LENGTH - usedspace;
        if (len >= freespace) {
            /* Fill the buffer completely and process it */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
            ADDINC128(context->bitcount, freespace << 3);
            len -= freespace;
            data += freespace;
            sha512_transform(context, (sha2_word64*)context->buffer);
        } else {
            /* The buffer is not yet full */
            MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
            ADDINC128(context->bitcount, len << 3);
            return;
        }
    }
    while (len >= SHA512_BLOCK_LENGTH) {
        /* Process as many complete blocks as we can */
        sha512_transform(context, (const sha2_word64*)data);
        ADDINC128(context->bitcount, SHA512_BLOCK_LENGTH << 3);
        len -= SHA512_BLOCK_LENGTH;
        data += SHA512_BLOCK_LENGTH;
    }
    if (len > 0) {
        /* There's left-overs, so save 'em */
        MEMCPY_BCOPY(context->buffer, data, len);
        ADDINC128(context->bitcount, len << 3);
    }
}

static void sha512_last(sha512_context* context)
{
    unsigned int usedspace;
    sha2_word64* t;
    usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
    /* Convert FROM host byte order */
    REVERSE64(context->bitcount[0], context->bitcount[0]);
    REVERSE64(context->bitcount[1], context->bitcount[1]);
#endif
    if (usedspace > 0) {
        /* Begin padding with a 1 bit: */
        context->buffer[usedspace++] = 0x80;
        if (usedspace <= SHA512_SHORT_BLOCK_LENGTH) {
            /* Set-up for the last transform: */
            MEMSET_BZERO(&context->buffer[usedspace], SHA512_SHORT_BLOCK_LENGTH - usedspace);
        } else {
            if (usedspace < SHA512_BLOCK_LENGTH) {
                MEMSET_BZERO(&context->buffer[usedspace], SHA512_BLOCK_LENGTH - usedspace);
            }
            /* Do second-to-last transform: */
            sha512_transform(context, (sha2_word64*)context->buffer);
            /* And set-up for the last transform: */
            MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH - 2);
        }
    } else {
        /* Prepare for final transform: */
        MEMSET_BZERO(context->buffer, SHA512_SHORT_BLOCK_LENGTH);
        /* Begin padding with a 1 bit: */
        *context->buffer = 0x80;
    }
    /* Store the length of input data (in bits): */
    t = (sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH];
    *t = context->bitcount[1];
    t = (sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH + 8];
    *t = context->bitcount[0];
    /* Final transform: */
    sha512_transform(context, (sha2_word64*)context->buffer);
}

void sha512_finalize(sha512_context* context, sha2_byte digest[SHA512_DIGEST_LENGTH]) {
    sha2_word64* d = (sha2_word64*)digest;
    /* If no digest buffer is passed, we don't bother doing this: */
    if (digest != (sha2_byte*)0) {
        sha512_last(context);
/* Save the hash data for output: */
#if BYTE_ORDER == LITTLE_ENDIAN
        {
            /* Convert TO host byte order */
            int j;
            for (j = 0; j < 8; j++) {
                REVERSE64(context->state[j], context->state[j]);
                memcpy_safe(d++, &context->state[j], sizeof(context->state[j]));
            }
        }
#else
        MEMCPY_BCOPY(d, context->state, SHA512_DIGEST_LENGTH);
#endif
    }
    /* Zero out state data */
    MEMSET_BZERO(context, sizeof(sha512_context));
}

void sha512_raw(const sha2_byte* data, size_t len, uint8_t digest[SHA512_DIGEST_LENGTH])
{
    sha512_context context;
    sha512_init(&context);
    sha512_write(&context, data, len);
    sha512_finalize(&context, digest);
}

/*** HMAC_SHA-*: *********************************************************/

void hmac_sha256_prepare(const uint8_t *key, const uint32_t keylen,
                         uint32_t *opad_digest, uint32_t *ipad_digest) {
  static CONFIDENTIAL uint32_t key_pad[SHA256_BLOCK_LENGTH / sizeof(uint32_t)];

  dogecoin_mem_zero(key_pad, sizeof(key_pad));
  if (keylen > SHA256_BLOCK_LENGTH) {
    static CONFIDENTIAL sha256_context context;
    sha256_init(&context);
    sha256_write(&context, key, keylen);
    sha256_finalize(&context, (uint8_t *)key_pad);
  } else {
    memcpy_safe(key_pad, key, keylen);
  }

  /* compute o_key_pad and its digest */
  int i = 0;
  for (; i < SHA256_BLOCK_LENGTH / (int)sizeof(uint32_t); i++) {
    uint32_t data = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
    REVERSE32(key_pad[i], data);
#else
    data = key_pad[i];
#endif
    key_pad[i] = data ^ 0x5c5c5c5c;
  }
  sha256_transform((sha256_context*)key_pad, opad_digest);

  /* convert o_key_pad to i_key_pad and compute its digest */
  for (i = 0; i < SHA256_BLOCK_LENGTH / (int)sizeof(uint32_t); i++) {
    key_pad[i] = key_pad[i] ^ 0x5c5c5c5c ^ 0x36363636;
  }
  sha256_transform((sha256_context*)key_pad, ipad_digest);
  dogecoin_mem_zero(key_pad, sizeof(key_pad));
}

void hmac_sha256_init(hmac_sha256_context *hctx, const uint8_t *key, const uint32_t keylen)
{
	uint8_t i_key_pad[SHA256_BLOCK_LENGTH];
	memset(i_key_pad, 0, SHA256_BLOCK_LENGTH);
	if (keylen > SHA256_BLOCK_LENGTH) {
		sha256_raw(key, keylen, i_key_pad);
	} else {
		memcpy(i_key_pad, key, keylen);
	}
    int i;
	for (i = 0; i < SHA256_BLOCK_LENGTH; i++) {
		hctx->o_key_pad[i] = i_key_pad[i] ^ 0x5c;
		i_key_pad[i] ^= 0x36;
	}
	sha256_init(&(hctx->ctx));
	sha256_write(&(hctx->ctx), i_key_pad, SHA256_BLOCK_LENGTH);
	MEMSET_BZERO(i_key_pad, sizeof(i_key_pad));
}

void hmac_sha256_write(hmac_sha256_context *hctx, const uint8_t *msg, const uint32_t msglen)
{
	sha256_write(&(hctx->ctx), msg, msglen);
}

void hmac_sha256_finalize(hmac_sha256_context *hctx, uint8_t *hmac)
{
	uint8_t hash[SHA256_DIGEST_LENGTH];
	sha256_finalize(&(hctx->ctx), hash);
	sha256_init(&(hctx->ctx));
	sha256_write(&(hctx->ctx), hctx->o_key_pad, SHA256_BLOCK_LENGTH);
	sha256_write(&(hctx->ctx), hash, SHA256_DIGEST_LENGTH);
	sha256_finalize(&(hctx->ctx), hmac);
	MEMSET_BZERO(hash, sizeof(hash));
	MEMSET_BZERO(hctx, sizeof(hmac_sha256_context));
}

void hmac_sha256(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac)
{
    int i;
    uint8_t buf[SHA256_BLOCK_LENGTH], o_key_pad[SHA256_BLOCK_LENGTH],
        i_key_pad[SHA256_BLOCK_LENGTH];
    sha256_context ctx;

    dogecoin_mem_zero(buf, SHA256_BLOCK_LENGTH);
    if (keylen > SHA256_BLOCK_LENGTH) {
        sha256_raw(key, keylen, buf);
    } else {
        memcpy_safe(buf, key, keylen);
    }
    for (i = 0; i < SHA256_BLOCK_LENGTH; i++) {
        o_key_pad[i] = buf[i] ^ 0x5c;
        i_key_pad[i] = buf[i] ^ 0x36;
    }
    sha256_init(&ctx);
    sha256_write(&ctx, i_key_pad, SHA256_BLOCK_LENGTH);
    sha256_write(&ctx, msg, msglen);
    sha256_finalize(&ctx, buf);
    sha256_init(&ctx);
    sha256_write(&ctx, o_key_pad, SHA256_BLOCK_LENGTH);
    sha256_write(&ctx, buf, SHA256_DIGEST_LENGTH);
    sha256_finalize(&ctx, hmac);
}

void hmac_sha512_init(hmac_sha512_context *hctx, const uint8_t *key, const uint32_t keylen)
{
	uint8_t i_key_pad[SHA512_BLOCK_LENGTH];
	memset(i_key_pad, 0, SHA512_BLOCK_LENGTH);
	if (keylen > SHA512_BLOCK_LENGTH) {
		sha512_raw(key, keylen, i_key_pad);
	} else {
		memcpy(i_key_pad, key, keylen);
	}
    int i;
	for (i = 0; i < SHA512_BLOCK_LENGTH; i++) {
		hctx->o_key_pad[i] = i_key_pad[i] ^ 0x5c;
		i_key_pad[i] ^= 0x36;
	}
	sha512_init(&(hctx->ctx));
	sha512_write(&(hctx->ctx), i_key_pad, SHA512_BLOCK_LENGTH);
	MEMSET_BZERO(i_key_pad, sizeof(i_key_pad));
}

void hmac_sha512_write(hmac_sha512_context *hctx, const uint8_t *msg, const uint32_t msglen)
{
	sha512_write(&(hctx->ctx), msg, msglen);
}

void hmac_sha512_finalize(hmac_sha512_context *hctx, uint8_t *hmac)
{
	uint8_t hash[SHA512_DIGEST_LENGTH];
	sha512_finalize(&(hctx->ctx), hash);
	sha512_init(&(hctx->ctx));
	sha512_write(&(hctx->ctx), hctx->o_key_pad, SHA512_BLOCK_LENGTH);
	sha512_write(&(hctx->ctx), hash, SHA512_DIGEST_LENGTH);
	sha512_finalize(&(hctx->ctx), hmac);
	MEMSET_BZERO(hash, sizeof(hash));
	MEMSET_BZERO(hctx, sizeof(hmac_sha512_context));
}

void hmac_sha512(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac)
{
    int i;
    uint8_t buf[SHA512_BLOCK_LENGTH], o_key_pad[SHA512_BLOCK_LENGTH],
        i_key_pad[SHA512_BLOCK_LENGTH];
    sha512_context ctx;
    dogecoin_mem_zero(buf, SHA512_BLOCK_LENGTH);
    if (keylen > SHA512_BLOCK_LENGTH)
        sha512_raw(key, keylen, buf);
    else
        memcpy_safe(buf, key, keylen);
    for (i = 0; i < SHA512_BLOCK_LENGTH; i++) {
        o_key_pad[i] = buf[i] ^ 0x5c;
        i_key_pad[i] = buf[i] ^ 0x36;
    }
    sha512_init(&ctx);
    sha512_write(&ctx, i_key_pad, SHA512_BLOCK_LENGTH);
    sha512_write(&ctx, msg, msglen);
    sha512_finalize(&ctx, buf);
    sha512_init(&ctx);
    sha512_write(&ctx, o_key_pad, SHA512_BLOCK_LENGTH);
    sha512_write(&ctx, buf, SHA512_DIGEST_LENGTH);
    sha512_finalize(&ctx, hmac);
}

/*** PBKDF2-*: *********************************************************/

void pbkdf2_hmac_sha256_init(pbkdf2_hmac_sha256_context *pctx, const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t blocknr)
{
	hmac_sha256_context hctx = {0};
#if BYTE_ORDER == LITTLE_ENDIAN
  REVERSE32(blocknr, blocknr);
#endif

    hmac_sha256_prepare(pass, passlen, pctx->odig, pctx->idig);
    dogecoin_mem_zero(pctx->g, sizeof(pctx->g));
    pctx->g[8] = 0x80000000;
    pctx->g[15] = (SHA256_BLOCK_LENGTH + SHA256_DIGEST_LENGTH) * 8;
    memcpy(hctx.ctx.state, pctx->idig, sizeof(pctx->idig));
    hctx.ctx.bitcount = SHA256_BLOCK_LENGTH * 8;
	hmac_sha256_init(&hctx, pass, passlen);
	hmac_sha256_write(&hctx, salt, saltlen);
	hmac_sha256_write(&hctx, (uint8_t *)&blocknr, sizeof(blocknr));
	hmac_sha256_finalize(&hctx, (uint8_t*)pctx->g);
	memcpy(pctx->f, pctx->g, SHA256_DIGEST_LENGTH);
	pctx->pass = pass;
	pctx->passlen = passlen;
	pctx->first = 1;
}

void pbkdf2_hmac_sha256_write(pbkdf2_hmac_sha256_context *pctx, uint32_t iterations)
{
    uint32_t i;
	for (i = pctx->first; i < iterations; i++) {
		hmac_sha256(pctx->pass, pctx->passlen, (const uint8_t*)pctx->g, SHA256_DIGEST_LENGTH, (uint8_t*)pctx->g);
        uint32_t j = 0;
		for (; j < 8; j++) {
			pctx->f[j] ^= pctx->g[j];
		}
	}
	pctx->first = 0;
}

void pbkdf2_hmac_sha256_finalize(pbkdf2_hmac_sha256_context *pctx, uint8_t *key)
{
	memcpy(key, pctx->f, SHA256_DIGEST_LENGTH);
	MEMSET_BZERO(pctx, sizeof(pbkdf2_hmac_sha256_context));
}

void pbkdf2_hmac_sha256(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key, int keylen)
{
    uint32_t last_block_size = keylen % SHA256_DIGEST_LENGTH;
    uint32_t blocks_count = keylen / SHA256_DIGEST_LENGTH;
    if (last_block_size) {
    blocks_count++;
    } else {
    last_block_size = SHA256_DIGEST_LENGTH;
    }
    uint32_t blocknr = 1;
    for (; blocknr <= blocks_count; blocknr++) {
        pbkdf2_hmac_sha256_context pctx = {0};
        pbkdf2_hmac_sha256_init(&pctx, pass, passlen, salt, saltlen, blocknr);
        pbkdf2_hmac_sha256_write(&pctx, iterations);
        uint8_t digest[SHA256_DIGEST_LENGTH] = {0};
        pbkdf2_hmac_sha256_finalize(&pctx, digest);
        uint32_t key_offset = (blocknr - 1) * SHA256_DIGEST_LENGTH;
        if (blocknr < blocks_count) {
            memcpy_safe(key + key_offset, digest, SHA256_DIGEST_LENGTH);
        } else {
            memcpy_safe(key + key_offset, digest, last_block_size);
        }
    }
}

void pbkdf2_hmac_sha512_init(pbkdf2_hmac_sha512_context *pctx, const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen)
{
	hmac_sha512_context hctx;
	hmac_sha512_init(&hctx, pass, passlen);
	hmac_sha512_write(&hctx, salt, saltlen);
	hmac_sha512_write(&hctx, (const uint8_t *)"\x00\x00\x00\x01", 4);
	hmac_sha512_finalize(&hctx, pctx->g);
	memcpy(pctx->f, pctx->g, SHA512_DIGEST_LENGTH);
	pctx->pass = pass;
	pctx->passlen = passlen;
	pctx->first = 1;
}

void pbkdf2_hmac_sha512_write(pbkdf2_hmac_sha512_context *pctx, uint32_t iterations)
{
    uint32_t i;
	for (i = pctx->first; i < iterations; i++) {
		hmac_sha512(pctx->pass, pctx->passlen, pctx->g, SHA512_DIGEST_LENGTH, pctx->g);
        uint32_t j;
		for (j = 0; j < SHA512_DIGEST_LENGTH; j++) {
			pctx->f[j] ^= pctx->g[j];
		}
	}
	pctx->first = 0;
}

void pbkdf2_hmac_sha512_finalize(pbkdf2_hmac_sha512_context *pctx, uint8_t *key)
{
	memcpy(key, pctx->f, SHA512_DIGEST_LENGTH);
	MEMSET_BZERO(pctx, sizeof(pbkdf2_hmac_sha512_context));
}

void pbkdf2_hmac_sha512(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key)
{
	pbkdf2_hmac_sha512_context pctx;
	pbkdf2_hmac_sha512_init(&pctx, pass, passlen, salt, saltlen);
	pbkdf2_hmac_sha512_write(&pctx, iterations);
	pbkdf2_hmac_sha512_finalize(&pctx, key);
}
