/*
 *  Copyright 2004 Hans Leidekker
 *  Copyright 2006 Mike McCormack
 *
 *  Based on DES.c from libcifs
 *
 *  Copyright (C) 2003, 2004 by Christopher R. Hertel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

struct ustring
{
    DWORD Length;
    DWORD MaximumLength;
    unsigned char *Buffer;
};

static const unsigned char InitialPermuteMap[64] =
{
    57, 49, 41, 33, 25, 17,  9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7,
    56, 48, 40, 32, 24, 16,  8, 0,
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6
 };

static const unsigned char KeyPermuteMap[56] =
{
    49, 42, 35, 28, 21, 14,  7,  0,
    50, 43, 36, 29, 22, 15,  8,  1,
    51, 44, 37, 30, 23, 16,  9,  2,
    52, 45, 38, 31, 55, 48, 41, 34,
    27, 20, 13,  6, 54, 47, 40, 33,
    26, 19, 12,  5, 53, 46, 39, 32,
    25, 18, 11,  4, 24, 17, 10,  3,
};

static const unsigned char KeyRotation[16] =
    { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

static const unsigned char KeyCompression[48] =
{
    13, 16, 10, 23,  0,  4,  2, 27,
    14,  5, 20,  9, 22, 18, 11,  3,
    25,  7, 15,  6, 26, 19, 12,  1,
    40, 51, 30, 36, 46, 54, 29, 39,
    50, 44, 32, 47, 43, 48, 38, 55,
    33, 52, 45, 41, 49, 35, 28, 31
};

static const unsigned char DataExpansion[48] =
{
    31,  0,  1,  2,  3,  4,  3,  4,
     5,  6,  7,  8,  7,  8,  9, 10,
    11, 12, 11, 12, 13, 14, 15, 16,
    15, 16, 17, 18, 19, 20, 19, 20,
    21, 22, 23, 24, 23, 24, 25, 26,
    27, 28, 27, 28, 29, 30, 31,  0
};

static const unsigned char SBox[8][64] =
{
    {  /* S0 */
        14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
         3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
         4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
        15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13
    },
    {  /* S1 */
        15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
         9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
         0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
         5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9
    },
    {  /* S2 */
        10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
         1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
        13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
        11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12
    },
    {  /* S3 */
         7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
         1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
        10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
        15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14
    },
    {  /* S4 */
         2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
         8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
         4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
        15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3
    },
    {  /* S5 */
        12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
         0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
         9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
         7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13
    },
    {  /* S6 */
         4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
         3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
         1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
        10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12
    },
    {  /* S7 */
        13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
        10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
         7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
         0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11
    }
};

static const unsigned char PBox[32] =
{
    15,  6, 19, 20, 28, 11, 27, 16,
     0, 14, 22, 25,  4, 17, 30,  9,
     1,  7, 23, 13, 31, 26,  2,  8,
    18, 12, 29,  5, 21, 10,  3, 24
};

static const unsigned char FinalPermuteMap[64] =
{
     7, 39, 15, 47, 23, 55, 31, 63,
     6, 38, 14, 46, 22, 54, 30, 62,
     5, 37, 13, 45, 21, 53, 29, 61,
     4, 36, 12, 44, 20, 52, 28, 60,
     3, 35, 11, 43, 19, 51, 27, 59,
     2, 34, 10, 42, 18, 50, 26, 58,
     1, 33,  9, 41, 17, 49, 25, 57,
     0, 32,  8, 40, 16, 48, 24, 56
};

#define CLRBIT( STR, IDX ) ( (STR)[(IDX)/8] &= ~(0x01 << (7 - ((IDX)%8))) )
#define SETBIT( STR, IDX ) ( (STR)[(IDX)/8] |= (0x01 << (7 - ((IDX)%8))) )
#define GETBIT( STR, IDX ) (( ((STR)[(IDX)/8]) >> (7 - ((IDX)%8)) ) & 0x01)

static void Permute( unsigned char *dst, const unsigned char *src, const unsigned char *map, const int mapsize )
{
    int bitcount, i;

    for (i = 0; i < mapsize; i++)
        dst[i] = 0;

    bitcount = mapsize * 8;

    for (i = 0; i < bitcount; i++)
    {
        if (GETBIT( src, map[i] ))
            SETBIT( dst, i );
    }
}

static void KeyShiftLeft( unsigned char *key, const int numbits )
{
    int i;
    unsigned char keep = key[0];

    for (i = 0; i < numbits; i++)
    {
        int j;

        for (j = 0; j < 7; j++)
        {
            if (j && (key[j] & 0x80))
                key[j-1] |=  0x01;
            key[j] <<= 1;
        }

        if (GETBIT( key, 27 ))
        {
            CLRBIT( key, 27 );
            SETBIT( key, 55 );
        }

        if (keep & 0x80)
            SETBIT( key, 27 );

        keep <<= 1;
    }
}

static void KeyShiftRight( unsigned char *key, const int numbits )
{
    int i;
    unsigned char keep = key[6];

    for (i = 0; i < numbits; i++)
    {
        int j;

        for (j = 6; j >= 0; j--)
        {
            if (j!=6 && (key[j] & 0x01))
                key[j+1] |=  0x80;
            key[j] >>= 1;
        }

        if (GETBIT( key, 28 ))
        {
            CLRBIT( key, 28 );
            SETBIT( key, 0 );
        }

        if (keep & 0x01)
            SETBIT( key, 28 );

        keep >>= 1;
    }
}

static void sbox( unsigned char *dst, const unsigned char *src )
{
    int i;

    for (i = 0; i < 4; i++)
        dst[i] = 0;

    for (i = 0; i < 8; i++)
    {
        int j, Snum, bitnum;

        for (Snum = j = 0, bitnum = (i * 6); j < 6; j++, bitnum++)
        {
            Snum <<= 1;
            Snum |= GETBIT( src, bitnum );
        }

        if (0 == (i%2))
            dst[i/2] |= ((SBox[i][Snum]) << 4);
        else
            dst[i/2] |= SBox[i][Snum];
    }
}

static void xor( unsigned char *dst, const unsigned char *a, const unsigned char *b, const int count )
{
    int i;

    for (i = 0; i < count; i++)
        dst[i] = a[i] ^ b[i];
}

static unsigned char *DEShash( unsigned char *dst, const unsigned char *key, const unsigned char *src )
{
    int i;
    unsigned char K[7];
    unsigned char D[8];

    Permute( K, key, KeyPermuteMap, 7 );
    Permute( D, src, InitialPermuteMap, 8 );

    for (i = 0; i < 16; i++)
    {
        int j;
        unsigned char *L = D;
        unsigned char *R = &(D[4]);
        unsigned char  Rexp[6];
        unsigned char  Rn[4];
        unsigned char  SubK[6];

        KeyShiftLeft( K, KeyRotation[i] );
        Permute( SubK, K, KeyCompression, 6 );

        Permute( Rexp, R, DataExpansion, 6 );
        xor( Rexp, Rexp, SubK, 6 );

        sbox( Rn, Rexp );
        Permute( Rexp, Rn, PBox, 4 );
        xor( Rn, L, Rexp, 4 );

        for (j = 0; j < 4; j++)
        {
            L[j] = R[j];
            R[j] = Rn[j];
        }
    }

    Permute( dst, D, FinalPermuteMap, 8 );

    return dst;
}

static unsigned char *DESunhash( unsigned char *dst, const unsigned char *key, const unsigned char *src )
{
    int i;
    unsigned char K[7];
    unsigned char D[8];

    Permute( K, key, KeyPermuteMap, 7 );
    Permute( D, src, InitialPermuteMap, 8 );

    for (i = 0; i < 16; i++)
    {
        int j;
        unsigned char *L = D;
        unsigned char *R = &(D[4]);
        unsigned char  Rexp[6];
        unsigned char  Rn[4];
        unsigned char  SubK[6];

        Permute( SubK, K, KeyCompression, 6 );

        Permute( Rexp, R, DataExpansion, 6 );
        xor( Rexp, Rexp, SubK, 6 );

        sbox( Rn, Rexp );
        Permute( Rexp, Rn, PBox, 4 );
        xor( Rn, L, Rexp, 4 );

        for (j = 0; j < 4; j++)
        {
            L[j] = R[j];
            R[j] = Rn[j];
        }

        KeyShiftRight( K, KeyRotation[15 - i] );
    }

    Permute( dst, D, FinalPermuteMap, 8 );

    return dst;
}


/******************************************************************************
 *     SystemFunction001   (cryptbase.@)
 *
 * Encrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to encrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the encrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction001( const BYTE *data, const BYTE *key, BYTE *output )
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    DEShash( output, key, data );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction002   (cryptbase.@)
 *
 * Decrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to decrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the decrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction002( const BYTE *data, const BYTE *key, BYTE *output )
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    DESunhash( output, key, data );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction003   (cryptbase.@)
 *
 * Hashes a key using DES and a fixed datablock
 *
 * PARAMS
 *   key     [I] key data    (7 bytes)
 *   output  [O] hashed key  (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction003( const BYTE *key, BYTE *output )
{
    static const unsigned char LMhash_Magic[8] = {'K','G','S','!','@','#','$','%'};

    if (!output)
        return STATUS_UNSUCCESSFUL;
    DEShash( output, key, LMhash_Magic );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction004   (cryptbase.@)
 *
 * Encrypts a block of data with DES in ECB mode, preserving the length
 *
 * PARAMS
 *   data    [I] data to encrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive encrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 * NOTES
 *  Encrypt buffer size should be input size rounded up to 8 bytes
 *   plus an extra 8 bytes.
 */
NTSTATUS WINAPI SystemFunction004( const struct ustring *in, const struct ustring *key,
                                   struct ustring *out )
{
    union
    {
        unsigned char uc[8];
        unsigned int ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int crypt_len, ofs;

    if (key->Length <= 0)
        return STATUS_INVALID_PARAMETER_2;

    crypt_len = ((in->Length + 7) & ~7);
    if (out->MaximumLength < (crypt_len + 8))
        return STATUS_BUFFER_TOO_SMALL;

    data.ui[0] = in->Length;
    data.ui[1] = 1;

    if (key->Length < sizeof(deskey))
    {
        memset( deskey, 0, sizeof(deskey) );
        memcpy( deskey, key->Buffer, key->Length );
    }
    else
        memcpy( deskey, key->Buffer, sizeof(deskey) );

    DEShash( out->Buffer, deskey, data.uc );

    for (ofs = 0; ofs < (crypt_len - 8); ofs += 8)
        DEShash( out->Buffer + 8 + ofs, deskey, in->Buffer + ofs );

    memset( data.uc, 0, sizeof(data.uc) );
    memcpy( data.uc, in->Buffer + ofs, in->Length + 8 - crypt_len );
    DEShash( out->Buffer + 8 + ofs, deskey, data.uc );

    out->Length = crypt_len + 8;

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction005   (cryptbase.@)
 *
 * Decrypts a block of data with DES in ECB mode
 *
 * PARAMS
 *   data    [I] data to decrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive decrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 */
NTSTATUS WINAPI SystemFunction005( const struct ustring *in, const struct ustring *key,
                                   struct ustring *out )
{
    union
    {
        unsigned char uc[8];
        unsigned int ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int ofs, crypt_len;

    if (key->Length <= 0)
        return STATUS_INVALID_PARAMETER_2;

    if (key->Length < sizeof(deskey))
    {
        memset( deskey, 0, sizeof(deskey) );
        memcpy( deskey, key->Buffer, key->Length );
    }
    else
        memcpy( deskey, key->Buffer, sizeof(deskey) );

    DESunhash(data.uc, deskey, in->Buffer);

    if (data.ui[1] != 1)
        return STATUS_UNKNOWN_REVISION;

    crypt_len = data.ui[0];
    if (crypt_len > out->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    for (ofs = 0; (ofs + 8) < crypt_len; ofs += 8)
        DESunhash( out->Buffer+ofs, deskey, in->Buffer + ofs + 8 );

    if (ofs < crypt_len)
    {
        DESunhash( data.uc, deskey, in->Buffer + ofs + 8 );
        memcpy( out->Buffer + ofs, data.uc, crypt_len - ofs );
    }

    out->Length = crypt_len;

    return STATUS_SUCCESS;
}
