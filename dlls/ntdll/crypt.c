/*
 * Copyright 2001 Nikos Mavroyanopoulos
 * Copyright 2004 Hans Leidekker
 * Copyright 2004 Filip Navara
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"

/* SHA1 algorithm
 *
 * Based on public domain SHA code by Steve Reid <steve@edmweb.com>
 */

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
/* FIXME: This definition of DWORD2BE is little endian specific! */
#define DWORD2BE(x) (((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | (((x) << 24) & 0xff000000);
/* FIXME: This definition of blk0 is little endian specific! */
#define blk0(i) (Block[i] = (rol(Block[i],24)&0xFF00FF00)|(rol(Block[i],8)&0x00FF00FF))
#define blk1(i) (Block[i&15] = rol(Block[(i+13)&15]^Block[(i+8)&15]^Block[(i+2)&15]^Block[i&15],1))
#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)
/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

/* Hash a single 512-bit block. This is the core of the algorithm. */
static void SHA1Transform(ULONG State[5], UCHAR Buffer[64])
{
   ULONG a, b, c, d, e;
   ULONG *Block;

   Block = (ULONG*)Buffer;

   /* Copy Context->State[] to working variables */
   a = State[0];
   b = State[1];
   c = State[2];
   d = State[3];
   e = State[4];

   /* 4 rounds of 20 operations each. Loop unrolled. */
   R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
   R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
   R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
   R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
   R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
   R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
   R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
   R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
   R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
   R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
   R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
   R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
   R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
   R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
   R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
   R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
   R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
   R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
   R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
   R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

   /* Add the working variables back into Context->State[] */
   State[0] += a;
   State[1] += b;
   State[2] += c;
   State[3] += d;
   State[4] += e;
}


/******************************************************************************
 * A_SHAInit (ntdll.@)
 *
 * Initialize a SHA context structure.
 *
 * PARAMS
 *  Context [O] SHA context
 *
 * RETURNS
 *  Nothing
 */
void WINAPI A_SHAInit(PSHA_CTX Context)
{
   /* SHA1 initialization constants */
   Context->State[0] = 0x67452301;
   Context->State[1] = 0xEFCDAB89;
   Context->State[2] = 0x98BADCFE;
   Context->State[3] = 0x10325476;
   Context->State[4] = 0xC3D2E1F0;
   Context->Count[0] =
   Context->Count[1] = 0;
}

/******************************************************************************
 * A_SHAUpdate (ntdll.@)
 *
 * Update a SHA context with a hashed data from supplied buffer.
 *
 * PARAMS
 *  Context    [O] SHA context
 *  Buffer     [I] hashed data
 *  BufferSize [I] hashed data size
 *
 * RETURNS
 *  Nothing
 */
void WINAPI A_SHAUpdate(PSHA_CTX Context, const unsigned char *Buffer, UINT BufferSize)
{
   ULONG BufferContentSize;

   BufferContentSize = Context->Count[1] & 63;
   Context->Count[1] += BufferSize;
   if (Context->Count[1] < BufferSize)
      Context->Count[0]++;
   Context->Count[0] += (BufferSize >> 29);

   if (BufferContentSize + BufferSize < 64)
   {
      RtlCopyMemory(&Context->Buffer[BufferContentSize], Buffer,
                    BufferSize);
   }
   else
   {
      while (BufferContentSize + BufferSize >= 64)
      {
         RtlCopyMemory(Context->Buffer + BufferContentSize, Buffer,
                       64 - BufferContentSize);
         Buffer += 64 - BufferContentSize;
         BufferSize -= 64 - BufferContentSize;
         SHA1Transform(Context->State, Context->Buffer);
         BufferContentSize = 0;
      }
      RtlCopyMemory(Context->Buffer + BufferContentSize, Buffer, BufferSize);
   }
}

/******************************************************************************
 * A_SHAFinal (ntdll.@)
 *
 * Finalize SHA context and return the resulting hash.
 *
 * PARAMS
 *  Context [I/O] SHA context
 *  Result  [O] resulting hash
 *
 * RETURNS
 *  Nothing
 */
void WINAPI A_SHAFinal(PSHA_CTX Context, PULONG Result)
{
   INT Pad, Index;
   UCHAR Buffer[72];
   ULONG *Count;
   ULONG BufferContentSize, LengthHi, LengthLo;

   BufferContentSize = Context->Count[1] & 63;
   if (BufferContentSize >= 56)
      Pad = 56 + 64 - BufferContentSize;
   else
      Pad = 56 - BufferContentSize;

   LengthHi = (Context->Count[0] << 3) | (Context->Count[1] >> (32 - 3));
   LengthLo = (Context->Count[1] << 3);

   RtlZeroMemory(Buffer + 1, Pad - 1);
   Buffer[0] = 0x80;
   Count = (ULONG*)(Buffer + Pad);
   Count[0] = DWORD2BE(LengthHi);
   Count[1] = DWORD2BE(LengthLo);
   A_SHAUpdate(Context, Buffer, Pad + 8);

   for (Index = 0; Index < 5; Index++)
      Result[Index] = DWORD2BE(Context->State[Index]);

   A_SHAInit(Context);
}


/* MD4 algorithm
 *
 * This code implements the MD4 message-digest algorithm.
 * It is based on code in the public domain written by Colin
 * Plumb in 1993. The algorithm is due to Ron Rivest.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;


#define F( x, y, z ) (((x) & (y)) | ((~x) & (z)))
#define G( x, y, z ) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H( x, y, z ) ((x) ^ (y) ^ (z))

#define FF( a, b, c, d, x, s ) { \
    (a) += F( (b), (c), (d) ) + (x); \
    (a) = rol( (a), (s) ); \
  }
#define GG( a, b, c, d, x, s ) { \
    (a) += G( (b), (c), (d) ) + (x) + (unsigned int)0x5a827999; \
    (a) = rol( (a), (s) ); \
  }
#define HH( a, b, c, d, x, s ) { \
    (a) += H( (b), (c), (d) ) + (x) + (unsigned int)0x6ed9eba1; \
    (a) = rol( (a), (s) ); \
  }

static void MD4Transform( unsigned int buf[4], const unsigned int in[16] )
{
    unsigned int a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    FF( a, b, c, d, in[0], 3 );
    FF( d, a, b, c, in[1], 7 );
    FF( c, d, a, b, in[2], 11 );
    FF( b, c, d, a, in[3], 19 );
    FF( a, b, c, d, in[4], 3 );
    FF( d, a, b, c, in[5], 7 );
    FF( c, d, a, b, in[6], 11 );
    FF( b, c, d, a, in[7], 19 );
    FF( a, b, c, d, in[8], 3 );
    FF( d, a, b, c, in[9], 7 );
    FF( c, d, a, b, in[10], 11 );
    FF( b, c, d, a, in[11], 19 );
    FF( a, b, c, d, in[12], 3 );
    FF( d, a, b, c, in[13], 7 );
    FF( c, d, a, b, in[14], 11 );
    FF( b, c, d, a, in[15], 19 );

    GG( a, b, c, d, in[0], 3 );
    GG( d, a, b, c, in[4], 5 );
    GG( c, d, a, b, in[8], 9 );
    GG( b, c, d, a, in[12], 13 );
    GG( a, b, c, d, in[1], 3 );
    GG( d, a, b, c, in[5], 5 );
    GG( c, d, a, b, in[9], 9 );
    GG( b, c, d, a, in[13], 13 );
    GG( a, b, c, d, in[2], 3 );
    GG( d, a, b, c, in[6], 5 );
    GG( c, d, a, b, in[10], 9 );
    GG( b, c, d, a, in[14], 13 );
    GG( a, b, c, d, in[3], 3 );
    GG( d, a, b, c, in[7], 5 );
    GG( c, d, a, b, in[11], 9 );
    GG( b, c, d, a, in[15], 13 );

    HH( a, b, c, d, in[0], 3 );
    HH( d, a, b, c, in[8], 9 );
    HH( c, d, a, b, in[4], 11 );
    HH( b, c, d, a, in[12], 15 );
    HH( a, b, c, d, in[2], 3 );
    HH( d, a, b, c, in[10], 9 );
    HH( c, d, a, b, in[6], 11 );
    HH( b, c, d, a, in[14], 15 );
    HH( a, b, c, d, in[1], 3 );
    HH( d, a, b, c, in[9], 9 );
    HH( c, d, a, b, in[5], 11 );
    HH( b, c, d, a, in[13], 15 );
    HH( a, b, c, d, in[3], 3 );
    HH( d, a, b, c, in[11], 9 );
    HH( c, d, a, b, in[7], 11 );
    HH( b, c, d, a, in[15], 15 );

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/*
 * Note: this code is harmless on little-endian machines.
 */
static void byteReverse( unsigned char *buf, unsigned longs )
{
    unsigned int t;

    do {
        t = ((unsigned)buf[3] << 8 | buf[2]) << 16 |
            ((unsigned)buf[1] << 8 | buf[0]);
        *(unsigned int *)buf = t;
        buf += 4;
    } while (--longs);
}

/******************************************************************************
 * MD4Init (ntdll.@)
 *
 * Start MD4 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void WINAPI MD4Init( MD4_CTX *ctx )
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->i[0] = ctx->i[1] = 0;
}

/******************************************************************************
 * MD4Update (ntdll.@)
 *
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void WINAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len )
{
    unsigned int t;

    /* Update bitcount */
    t = ctx->i[0];

    if ((ctx->i[0] = t + (len << 3)) < t)
        ctx->i[1]++;        /* Carry from low to high */

    ctx->i[1] += len >> 29;
    t = (t >> 3) & 0x3f;

    /* Handle any leading odd-sized chunks */
    if (t)
    {
        unsigned char *p = (unsigned char *)ctx->in + t;
        t = 64 - t;

        if (len < t)
        {
            memcpy( p, buf, len );
            return;
        }

        memcpy( p, buf, t );
        byteReverse( ctx->in, 16 );

        MD4Transform( ctx->buf, (unsigned int *)ctx->in );

        buf += t;
        len -= t;
    }

    /* Process data in 64-byte chunks */
    while (len >= 64)
    {
        memcpy( ctx->in, buf, 64 );
        byteReverse( ctx->in, 16 );

        MD4Transform( ctx->buf, (unsigned int *)ctx->in );

        buf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */
    memcpy( ctx->in, buf, len );
}

/******************************************************************************
 * MD4Final (ntdll.@)
 *
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void WINAPI MD4Final( MD4_CTX *ctx )
{
    unsigned int count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->i[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8)
    {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset( p, 0, count );
        byteReverse( ctx->in, 16 );
        MD4Transform( ctx->buf, (unsigned int *)ctx->in );

        /* Now fill the next block with 56 bytes */
        memset( ctx->in, 0, 56 );
    }
    else
    {
        /* Pad block to 56 bytes */
        memset( p, 0, count - 8 );
    }

    byteReverse( ctx->in, 14 );

    /* Append length in bits and transform */
    ((unsigned int *)ctx->in)[14] = ctx->i[0];
    ((unsigned int *)ctx->in)[15] = ctx->i[1];

    MD4Transform( ctx->buf, (unsigned int *)ctx->in );
    byteReverse( (unsigned char *)ctx->buf, 4 );
    memcpy( ctx->digest, ctx->buf, 16 );
}


/* MD5 algorithm
 *
 * This code implements the MD5 message-digest algorithm.
 * It is based on code in the public domain written by Colin
 * Plumb in 1993. The algorithm is due to Ron Rivest.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

/* #define F1( x, y, z ) (x & y | ~x & z) */
#define F1( x, y, z ) (z ^ (x & (y ^ z)))
#define F2( x, y, z ) F1( z, x, y )
#define F3( x, y, z ) (x ^ y ^ z)
#define F4( x, y, z ) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP( f, w, x, y, z, data, s ) \
        ( w += f( x, y, z ) + data,  w = w << s | w >> (32 - s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void MD5Transform( unsigned int buf[4], const unsigned int in[16] )
{
    unsigned int a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP( F1, a, b, c, d, in[0] + 0xd76aa478, 7 );
    MD5STEP( F1, d, a, b, c, in[1] + 0xe8c7b756, 12 );
    MD5STEP( F1, c, d, a, b, in[2] + 0x242070db, 17 );
    MD5STEP( F1, b, c, d, a, in[3] + 0xc1bdceee, 22 );
    MD5STEP( F1, a, b, c, d, in[4] + 0xf57c0faf, 7 );
    MD5STEP( F1, d, a, b, c, in[5] + 0x4787c62a, 12 );
    MD5STEP( F1, c, d, a, b, in[6] + 0xa8304613, 17 );
    MD5STEP( F1, b, c, d, a, in[7] + 0xfd469501, 22 );
    MD5STEP( F1, a, b, c, d, in[8] + 0x698098d8, 7 );
    MD5STEP( F1, d, a, b, c, in[9] + 0x8b44f7af, 12 );
    MD5STEP( F1, c, d, a, b, in[10] + 0xffff5bb1, 17 );
    MD5STEP( F1, b, c, d, a, in[11] + 0x895cd7be, 22 );
    MD5STEP( F1, a, b, c, d, in[12] + 0x6b901122, 7 );
    MD5STEP( F1, d, a, b, c, in[13] + 0xfd987193, 12 );
    MD5STEP( F1, c, d, a, b, in[14] + 0xa679438e, 17 );
    MD5STEP( F1, b, c, d, a, in[15] + 0x49b40821, 22 );

    MD5STEP( F2, a, b, c, d, in[1] + 0xf61e2562, 5 );
    MD5STEP( F2, d, a, b, c, in[6] + 0xc040b340, 9 );
    MD5STEP( F2, c, d, a, b, in[11] + 0x265e5a51, 14 );
    MD5STEP( F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20 );
    MD5STEP( F2, a, b, c, d, in[5] + 0xd62f105d, 5 );
    MD5STEP( F2, d, a, b, c, in[10] + 0x02441453, 9 );
    MD5STEP( F2, c, d, a, b, in[15] + 0xd8a1e681, 14 );
    MD5STEP( F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20 );
    MD5STEP( F2, a, b, c, d, in[9] + 0x21e1cde6, 5 );
    MD5STEP( F2, d, a, b, c, in[14] + 0xc33707d6, 9 );
    MD5STEP( F2, c, d, a, b, in[3] + 0xf4d50d87, 14 );
    MD5STEP( F2, b, c, d, a, in[8] + 0x455a14ed, 20 );
    MD5STEP( F2, a, b, c, d, in[13] + 0xa9e3e905, 5 );
    MD5STEP( F2, d, a, b, c, in[2] + 0xfcefa3f8, 9 );
    MD5STEP( F2, c, d, a, b, in[7] + 0x676f02d9, 14 );
    MD5STEP( F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20 );

    MD5STEP( F3, a, b, c, d, in[5] + 0xfffa3942, 4 );
    MD5STEP( F3, d, a, b, c, in[8] + 0x8771f681, 11 );
    MD5STEP( F3, c, d, a, b, in[11] + 0x6d9d6122, 16 );
    MD5STEP( F3, b, c, d, a, in[14] + 0xfde5380c, 23 );
    MD5STEP( F3, a, b, c, d, in[1] + 0xa4beea44, 4 );
    MD5STEP( F3, d, a, b, c, in[4] + 0x4bdecfa9, 11 );
    MD5STEP( F3, c, d, a, b, in[7] + 0xf6bb4b60, 16 );
    MD5STEP( F3, b, c, d, a, in[10] + 0xbebfbc70, 23 );
    MD5STEP( F3, a, b, c, d, in[13] + 0x289b7ec6, 4 );
    MD5STEP( F3, d, a, b, c, in[0] + 0xeaa127fa, 11 );
    MD5STEP( F3, c, d, a, b, in[3] + 0xd4ef3085, 16 );
    MD5STEP( F3, b, c, d, a, in[6] + 0x04881d05, 23 );
    MD5STEP( F3, a, b, c, d, in[9] + 0xd9d4d039, 4 );
    MD5STEP( F3, d, a, b, c, in[12] + 0xe6db99e5, 11 );
    MD5STEP( F3, c, d, a, b, in[15] + 0x1fa27cf8, 16 );
    MD5STEP( F3, b, c, d, a, in[2] + 0xc4ac5665, 23 );

    MD5STEP( F4, a, b, c, d, in[0] + 0xf4292244, 6 );
    MD5STEP( F4, d, a, b, c, in[7] + 0x432aff97, 10 );
    MD5STEP( F4, c, d, a, b, in[14] + 0xab9423a7, 15 );
    MD5STEP( F4, b, c, d, a, in[5] + 0xfc93a039, 21 );
    MD5STEP( F4, a, b, c, d, in[12] + 0x655b59c3, 6 );
    MD5STEP( F4, d, a, b, c, in[3] + 0x8f0ccc92, 10 );
    MD5STEP( F4, c, d, a, b, in[10] + 0xffeff47d, 15 );
    MD5STEP( F4, b, c, d, a, in[1] + 0x85845dd1, 21 );
    MD5STEP( F4, a, b, c, d, in[8] + 0x6fa87e4f, 6 );
    MD5STEP( F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10 );
    MD5STEP( F4, c, d, a, b, in[6] + 0xa3014314, 15 );
    MD5STEP( F4, b, c, d, a, in[13] + 0x4e0811a1, 21 );
    MD5STEP( F4, a, b, c, d, in[4] + 0xf7537e82, 6 );
    MD5STEP( F4, d, a, b, c, in[11] + 0xbd3af235, 10 );
    MD5STEP( F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15 );
    MD5STEP( F4, b, c, d, a, in[9] + 0xeb86d391, 21 );

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/******************************************************************************
 * MD5Init (ntdll.@)
 *
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void WINAPI MD5Init( MD5_CTX *ctx )
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->i[0] = ctx->i[1] = 0;
}

/******************************************************************************
 * MD5Update (ntdll.@)
 *
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void WINAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len )
{
    register unsigned int t;

    /* Update bitcount */
    t = ctx->i[0];

    if ((ctx->i[0] = t + (len << 3)) < t)
        ctx->i[1]++;        /* Carry from low to high */

    ctx->i[1] += len >> 29;
    t = (t >> 3) & 0x3f;

    /* Handle any leading odd-sized chunks */
    if (t)
    {
        unsigned char *p = (unsigned char *)ctx->in + t;
        t = 64 - t;

        if (len < t)
        {
            memcpy( p, buf, len );
            return;
        }

        memcpy( p, buf, t );
        byteReverse( ctx->in, 16 );

        MD5Transform( ctx->buf, (unsigned int *)ctx->in );

        buf += t;
        len -= t;
    }

    /* Process data in 64-byte chunks */
    while (len >= 64)
    {
        memcpy( ctx->in, buf, 64 );
        byteReverse( ctx->in, 16 );

        MD5Transform( ctx->buf, (unsigned int *)ctx->in );

        buf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */
    memcpy( ctx->in, buf, len );
}

/******************************************************************************
 * MD5Final (ntdll.@)
 *
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void WINAPI MD5Final( MD5_CTX *ctx )
{
    unsigned int count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->i[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8)
    {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset( p, 0, count );
        byteReverse( ctx->in, 16 );
        MD5Transform( ctx->buf, (unsigned int *)ctx->in );

        /* Now fill the next block with 56 bytes */
        memset( ctx->in, 0, 56 );
    }
    else
    {
        /* Pad block to 56 bytes */
        memset( p, 0, count - 8 );
    }

    byteReverse( ctx->in, 14 );

    /* Append length in bits and transform */
    ((unsigned int *)ctx->in)[14] = ctx->i[0];
    ((unsigned int *)ctx->in)[15] = ctx->i[1];

    MD5Transform( ctx->buf, (unsigned int *)ctx->in );
    byteReverse( (unsigned char *)ctx->buf, 4 );
    memcpy( ctx->digest, ctx->buf, 16 );
}
