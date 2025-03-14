/*
 *  Copyright 2004 Hans Leidekker
 *
 *  Based on LMHash.c from libcifs
 *
 *  Copyright (C) 2004 by Christopher R. Hertel
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

#include "crypt.h"

static const unsigned char CRYPT_LMhash_Magic[8] =
    { 'K', 'G', 'S', '!', '@', '#', '$', '%' };

/******************************************************************************
 * SystemFunction001  [ADVAPI32.@]
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
NTSTATUS WINAPI SystemFunction001(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, data);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction002  [ADVAPI32.@]
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
NTSTATUS WINAPI SystemFunction002(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DESunhash(output, key, data);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction003  [ADVAPI32.@]
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
NTSTATUS WINAPI SystemFunction003(const BYTE *key, LPBYTE output)
{
    if (!output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, CRYPT_LMhash_Magic);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction004  [ADVAPI32.@]
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
NTSTATUS WINAPI SystemFunction004(const struct ustring *in,
                                  const struct ustring *key,
                                  struct ustring *out)
{
    union {
         unsigned char uc[8];
         unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int crypt_len, ofs;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    crypt_len = ((in->Length+7)&~7);
    if (out->MaximumLength < (crypt_len+8))
        return STATUS_BUFFER_TOO_SMALL;

    data.ui[0] = in->Length;
    data.ui[1] = 1;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DEShash(out->Buffer, deskey, data.uc);

    for(ofs=0; ofs<(crypt_len-8); ofs+=8)
        CRYPT_DEShash(out->Buffer+8+ofs, deskey, in->Buffer+ofs);

    memset(data.uc, 0, sizeof data.uc);
    memcpy(data.uc, in->Buffer+ofs, in->Length +8-crypt_len);
    CRYPT_DEShash(out->Buffer+8+ofs, deskey, data.uc);

    out->Length = crypt_len+8;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction005  [ADVAPI32.@]
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
NTSTATUS WINAPI SystemFunction005(const struct ustring *in,
                                  const struct ustring *key,
                                  struct ustring *out)
{
    union {
         unsigned char uc[8];
         unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int ofs, crypt_len;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DESunhash(data.uc, deskey, in->Buffer);

    if (data.ui[1] != 1)
        return STATUS_UNKNOWN_REVISION;

    crypt_len = data.ui[0];
    if (crypt_len > out->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    for (ofs=0; (ofs+8)<crypt_len; ofs+=8)
        CRYPT_DESunhash(out->Buffer+ofs, deskey, in->Buffer+ofs+8);

    if (ofs<crypt_len)
    {
        CRYPT_DESunhash(data.uc, deskey, in->Buffer+ofs+8);
        memcpy(out->Buffer+ofs, data.uc, crypt_len-ofs);
    }

    out->Length = crypt_len;

    return STATUS_SUCCESS;
}
