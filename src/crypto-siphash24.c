/*
   SipHash reference C implementation

   Written in 2012 by
   Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
   Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along
   with this software. If not, see
   <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "crypto-siphash24.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

#define ROTL(x, b) (u64)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)        \
    (p)[0] = (u8) ((v));       \
    (p)[1] = (u8) ((v) >> 8);  \
    (p)[2] = (u8) ((v) >> 16); \
    (p)[3] = (u8) ((v) >> 24);

#define U64TO8_LE(p, v)          \
    U32TO8_LE((p), (u32) ((v))); \
    U32TO8_LE((p) + 4, (u32) ((v) >> 32));

#define U8TO64_LE(p)                                                                              \
    (((u64) ((p)[0])) | ((u64) ((p)[1]) << 8) | ((u64) ((p)[2]) << 16) | ((u64) ((p)[3]) << 24) | \
     ((u64) ((p)[4]) << 32) | ((u64) ((p)[5]) << 40) | ((u64) ((p)[6]) << 48) |                   \
     ((u64) ((p)[7]) << 56))

#define SIPROUND           \
    {                      \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    }

/* SipHash-2-4 */
static int crypto_auth(unsigned char* out, const unsigned char* in, unsigned long long inlen,
                       const unsigned char* k)
{
    /* "somepseudorandomlygeneratedbytes" */
    u64 v0 = 0x736f6d6570736575ULL;
    u64 v1 = 0x646f72616e646f6dULL;
    u64 v2 = 0x6c7967656e657261ULL;
    u64 v3 = 0x7465646279746573ULL;
    u64 b;
    u64 k0 = U8TO64_LE(k);
    u64 k1 = U8TO64_LE(k + 8);
    const u8* end = in + inlen - (inlen % sizeof(u64));
    const int left = inlen & 7;
    b = ((u64) inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8)
    {
        u64 m;
        m = U8TO64_LE(in);
#ifdef xxxDEBUG
        printf("(%3d) v0 %08x %08x\n", (int) inlen, (u32) (v0 >> 32), (u32) v0);
        printf("(%3d) v1 %08x %08x\n", (int) inlen, (u32) (v1 >> 32), (u32) v1);
        printf("(%3d) v2 %08x %08x\n", (int) inlen, (u32) (v2 >> 32), (u32) v2);
        printf("(%3d) v3 %08x %08x\n", (int) inlen, (u32) (v3 >> 32), (u32) v3);
        printf("(%3d) compress %08x %08x\n", (int) inlen, (u32) (m >> 32), (u32) m);
#endif
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    switch (left)
    {
        case 7:
            b |= ((u64) in[6]) << 48;
        case 6:
            b |= ((u64) in[5]) << 40;
        case 5:
            b |= ((u64) in[4]) << 32;
        case 4:
            b |= ((u64) in[3]) << 24;
        case 3:
            b |= ((u64) in[2]) << 16;
        case 2:
            b |= ((u64) in[1]) << 8;
        case 1:
            b |= ((u64) in[0]);
            break;
        case 0:
            break;
    }

#ifdef xxxDEBUG
    printf("(%3d) v0 %08x %08x\n", (int) inlen, (u32) (v0 >> 32), (u32) v0);
    printf("(%3d) v1 %08x %08x\n", (int) inlen, (u32) (v1 >> 32), (u32) v1);
    printf("(%3d) v2 %08x %08x\n", (int) inlen, (u32) (v2 >> 32), (u32) v2);
    printf("(%3d) v3 %08x %08x\n", (int) inlen, (u32) (v3 >> 32), (u32) v3);
    printf("(%3d) padding   %08x %08x\n", (int) inlen, (u32) (b >> 32), (u32) b);
#endif
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
#ifdef xxxDEBUG
    printf("(%3d) v0 %08x %08x\n", (int) inlen, (u32) (v0 >> 32), (u32) v0);
    printf("(%3d) v1 %08x %08x\n", (int) inlen, (u32) (v1 >> 32), (u32) v1);
    printf("(%3d) v2 %08x %08x\n", (int) inlen, (u32) (v2 >> 32), (u32) v2);
    printf("(%3d) v3 %08x %08x\n", (int) inlen, (u32) (v3 >> 32), (u32) v3);
#endif
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);
    return 0;
}

uint64_t siphash24(const void* in, size_t inlen, const uint64_t key[2])
{
    uint64_t result;

    crypto_auth((unsigned char*) &result, (const unsigned char*) in, inlen,
                (const unsigned char*) &key[0]);

    return result;
}

/*
   SipHash-2-4 output with
   k = 00 01 02 ...
   and
   in = (empty string)
   in = 00 (1 byte)
   in = 00 01 (2 bytes)
   in = 00 01 02 (3 bytes)
   ...
   in = 00 01 02 ... 3e (63 bytes)
*/
static u8 vectors[64][8] = {{
                                0x31,
                                0x0e,
                                0x0e,
                                0xdd,
                                0x47,
                                0xdb,
                                0x6f,
                                0x72,
                            },
                            {
                                0xfd,
                                0x67,
                                0xdc,
                                0x93,
                                0xc5,
                                0x39,
                                0xf8,
                                0x74,
                            },
                            {
                                0x5a,
                                0x4f,
                                0xa9,
                                0xd9,
                                0x09,
                                0x80,
                                0x6c,
                                0x0d,
                            },
                            {
                                0x2d,
                                0x7e,
                                0xfb,
                                0xd7,
                                0x96,
                                0x66,
                                0x67,
                                0x85,
                            },
                            {
                                0xb7,
                                0x87,
                                0x71,
                                0x27,
                                0xe0,
                                0x94,
                                0x27,
                                0xcf,
                            },
                            {
                                0x8d,
                                0xa6,
                                0x99,
                                0xcd,
                                0x64,
                                0x55,
                                0x76,
                                0x18,
                            },
                            {
                                0xce,
                                0xe3,
                                0xfe,
                                0x58,
                                0x6e,
                                0x46,
                                0xc9,
                                0xcb,
                            },
                            {
                                0x37,
                                0xd1,
                                0x01,
                                0x8b,
                                0xf5,
                                0x00,
                                0x02,
                                0xab,
                            },
                            {
                                0x62,
                                0x24,
                                0x93,
                                0x9a,
                                0x79,
                                0xf5,
                                0xf5,
                                0x93,
                            },
                            {
                                0xb0,
                                0xe4,
                                0xa9,
                                0x0b,
                                0xdf,
                                0x82,
                                0x00,
                                0x9e,
                            },
                            {
                                0xf3,
                                0xb9,
                                0xdd,
                                0x94,
                                0xc5,
                                0xbb,
                                0x5d,
                                0x7a,
                            },
                            {
                                0xa7,
                                0xad,
                                0x6b,
                                0x22,
                                0x46,
                                0x2f,
                                0xb3,
                                0xf4,
                            },
                            {
                                0xfb,
                                0xe5,
                                0x0e,
                                0x86,
                                0xbc,
                                0x8f,
                                0x1e,
                                0x75,
                            },
                            {
                                0x90,
                                0x3d,
                                0x84,
                                0xc0,
                                0x27,
                                0x56,
                                0xea,
                                0x14,
                            },
                            {
                                0xee,
                                0xf2,
                                0x7a,
                                0x8e,
                                0x90,
                                0xca,
                                0x23,
                                0xf7,
                            },
                            {
                                0xe5,
                                0x45,
                                0xbe,
                                0x49,
                                0x61,
                                0xca,
                                0x29,
                                0xa1,
                            },
                            {
                                0xdb,
                                0x9b,
                                0xc2,
                                0x57,
                                0x7f,
                                0xcc,
                                0x2a,
                                0x3f,
                            },
                            {
                                0x94,
                                0x47,
                                0xbe,
                                0x2c,
                                0xf5,
                                0xe9,
                                0x9a,
                                0x69,
                            },
                            {
                                0x9c,
                                0xd3,
                                0x8d,
                                0x96,
                                0xf0,
                                0xb3,
                                0xc1,
                                0x4b,
                            },
                            {
                                0xbd,
                                0x61,
                                0x79,
                                0xa7,
                                0x1d,
                                0xc9,
                                0x6d,
                                0xbb,
                            },
                            {
                                0x98,
                                0xee,
                                0xa2,
                                0x1a,
                                0xf2,
                                0x5c,
                                0xd6,
                                0xbe,
                            },
                            {
                                0xc7,
                                0x67,
                                0x3b,
                                0x2e,
                                0xb0,
                                0xcb,
                                0xf2,
                                0xd0,
                            },
                            {
                                0x88,
                                0x3e,
                                0xa3,
                                0xe3,
                                0x95,
                                0x67,
                                0x53,
                                0x93,
                            },
                            {
                                0xc8,
                                0xce,
                                0x5c,
                                0xcd,
                                0x8c,
                                0x03,
                                0x0c,
                                0xa8,
                            },
                            {
                                0x94,
                                0xaf,
                                0x49,
                                0xf6,
                                0xc6,
                                0x50,
                                0xad,
                                0xb8,
                            },
                            {
                                0xea,
                                0xb8,
                                0x85,
                                0x8a,
                                0xde,
                                0x92,
                                0xe1,
                                0xbc,
                            },
                            {
                                0xf3,
                                0x15,
                                0xbb,
                                0x5b,
                                0xb8,
                                0x35,
                                0xd8,
                                0x17,
                            },
                            {
                                0xad,
                                0xcf,
                                0x6b,
                                0x07,
                                0x63,
                                0x61,
                                0x2e,
                                0x2f,
                            },
                            {
                                0xa5,
                                0xc9,
                                0x1d,
                                0xa7,
                                0xac,
                                0xaa,
                                0x4d,
                                0xde,
                            },
                            {
                                0x71,
                                0x65,
                                0x95,
                                0x87,
                                0x66,
                                0x50,
                                0xa2,
                                0xa6,
                            },
                            {
                                0x28,
                                0xef,
                                0x49,
                                0x5c,
                                0x53,
                                0xa3,
                                0x87,
                                0xad,
                            },
                            {
                                0x42,
                                0xc3,
                                0x41,
                                0xd8,
                                0xfa,
                                0x92,
                                0xd8,
                                0x32,
                            },
                            {
                                0xce,
                                0x7c,
                                0xf2,
                                0x72,
                                0x2f,
                                0x51,
                                0x27,
                                0x71,
                            },
                            {
                                0xe3,
                                0x78,
                                0x59,
                                0xf9,
                                0x46,
                                0x23,
                                0xf3,
                                0xa7,
                            },
                            {
                                0x38,
                                0x12,
                                0x05,
                                0xbb,
                                0x1a,
                                0xb0,
                                0xe0,
                                0x12,
                            },
                            {
                                0xae,
                                0x97,
                                0xa1,
                                0x0f,
                                0xd4,
                                0x34,
                                0xe0,
                                0x15,
                            },
                            {
                                0xb4,
                                0xa3,
                                0x15,
                                0x08,
                                0xbe,
                                0xff,
                                0x4d,
                                0x31,
                            },
                            {
                                0x81,
                                0x39,
                                0x62,
                                0x29,
                                0xf0,
                                0x90,
                                0x79,
                                0x02,
                            },
                            {
                                0x4d,
                                0x0c,
                                0xf4,
                                0x9e,
                                0xe5,
                                0xd4,
                                0xdc,
                                0xca,
                            },
                            {
                                0x5c,
                                0x73,
                                0x33,
                                0x6a,
                                0x76,
                                0xd8,
                                0xbf,
                                0x9a,
                            },
                            {
                                0xd0,
                                0xa7,
                                0x04,
                                0x53,
                                0x6b,
                                0xa9,
                                0x3e,
                                0x0e,
                            },
                            {
                                0x92,
                                0x59,
                                0x58,
                                0xfc,
                                0xd6,
                                0x42,
                                0x0c,
                                0xad,
                            },
                            {
                                0xa9,
                                0x15,
                                0xc2,
                                0x9b,
                                0xc8,
                                0x06,
                                0x73,
                                0x18,
                            },
                            {
                                0x95,
                                0x2b,
                                0x79,
                                0xf3,
                                0xbc,
                                0x0a,
                                0xa6,
                                0xd4,
                            },
                            {
                                0xf2,
                                0x1d,
                                0xf2,
                                0xe4,
                                0x1d,
                                0x45,
                                0x35,
                                0xf9,
                            },
                            {
                                0x87,
                                0x57,
                                0x75,
                                0x19,
                                0x04,
                                0x8f,
                                0x53,
                                0xa9,
                            },
                            {
                                0x10,
                                0xa5,
                                0x6c,
                                0xf5,
                                0xdf,
                                0xcd,
                                0x9a,
                                0xdb,
                            },
                            {
                                0xeb,
                                0x75,
                                0x09,
                                0x5c,
                                0xcd,
                                0x98,
                                0x6c,
                                0xd0,
                            },
                            {
                                0x51,
                                0xa9,
                                0xcb,
                                0x9e,
                                0xcb,
                                0xa3,
                                0x12,
                                0xe6,
                            },
                            {
                                0x96,
                                0xaf,
                                0xad,
                                0xfc,
                                0x2c,
                                0xe6,
                                0x66,
                                0xc7,
                            },
                            {
                                0x72,
                                0xfe,
                                0x52,
                                0x97,
                                0x5a,
                                0x43,
                                0x64,
                                0xee,
                            },
                            {
                                0x5a,
                                0x16,
                                0x45,
                                0xb2,
                                0x76,
                                0xd5,
                                0x92,
                                0xa1,
                            },
                            {
                                0xb2,
                                0x74,
                                0xcb,
                                0x8e,
                                0xbf,
                                0x87,
                                0x87,
                                0x0a,
                            },
                            {
                                0x6f,
                                0x9b,
                                0xb4,
                                0x20,
                                0x3d,
                                0xe7,
                                0xb3,
                                0x81,
                            },
                            {
                                0xea,
                                0xec,
                                0xb2,
                                0xa3,
                                0x0b,
                                0x22,
                                0xa8,
                                0x7f,
                            },
                            {
                                0x99,
                                0x24,
                                0xa4,
                                0x3c,
                                0xc1,
                                0x31,
                                0x57,
                                0x24,
                            },
                            {
                                0xbd,
                                0x83,
                                0x8d,
                                0x3a,
                                0xaf,
                                0xbf,
                                0x8d,
                                0xb7,
                            },
                            {
                                0x0b,
                                0x1a,
                                0x2a,
                                0x32,
                                0x65,
                                0xd5,
                                0x1a,
                                0xea,
                            },
                            {
                                0x13,
                                0x50,
                                0x79,
                                0xa3,
                                0x23,
                                0x1c,
                                0xe6,
                                0x60,
                            },
                            {
                                0x93,
                                0x2b,
                                0x28,
                                0x46,
                                0xe4,
                                0xd7,
                                0x06,
                                0x66,
                            },
                            {
                                0xe1,
                                0x91,
                                0x5f,
                                0x5c,
                                0xb1,
                                0xec,
                                0xa4,
                                0x6c,
                            },
                            {
                                0xf3,
                                0x25,
                                0x96,
                                0x5c,
                                0xa1,
                                0x6d,
                                0x62,
                                0x9f,
                            },
                            {
                                0x57,
                                0x5f,
                                0xf2,
                                0x8e,
                                0x60,
                                0x38,
                                0x1b,
                                0xe5,
                            },
                            {
                                0x72,
                                0x45,
                                0x06,
                                0xeb,
                                0x4c,
                                0x32,
                                0x8a,
                                0x95,
                            }};

static int test_vectors()
{
#define MAXLEN 64
    u8 in[MAXLEN], out[8], k[16];
    int i;
    int ok = 1;

    for (i = 0; i < 16; ++i) k[i] = (u8) i;

    for (i = 0; i < MAXLEN; ++i)
    {
        in[i] = (u8) i;
        crypto_auth(out, in, i, k);

        if (memcmp(out, vectors[i], 8))
        {
            printf("test vector failed for %d bytes\n", i);
            ok = 0;
        }
    }

    return ok;
}

int siphash24_selftest(void)
{
    if (test_vectors())
        return 0;
    else
        return 1;
}
