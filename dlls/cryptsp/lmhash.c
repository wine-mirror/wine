/*
 * Copyright 2004 Hans Leidekker
 *
 * Based on LMHash.c from libcifs
 *
 * Copyright (C) 2004 by Christopher R. Hertel
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

NTSTATUS WINAPI SystemFunction001(const BYTE *data, const BYTE *key, BYTE *output);
NTSTATUS WINAPI SystemFunction002(const BYTE *data, const BYTE *key, BYTE *output);

static void DEShash( BYTE *output, const BYTE *key, const BYTE *data )
{
    SystemFunction001( data, key, output );
}

static void DESunhash( BYTE *output, const BYTE *key, const BYTE *data )
{
    SystemFunction002( data, key, output );
}

static const unsigned char LMhash_Magic[8] = {'K','G','S','!','@','#','$','%'};

static void LMhash( unsigned char *dst, const unsigned char *pwd, const int len )
{
    int i, max = 14;
    unsigned char tmp_pwd[14] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    max = len > max ? max : len;

    for (i = 0; i < max; i++)
        tmp_pwd[i] = pwd[i];

    DEShash( dst, tmp_pwd, LMhash_Magic );
    DEShash( &dst[8], &tmp_pwd[7], LMhash_Magic );
}

/******************************************************************************
 *     SystemFunction006   (cryptsp.@)
 */
NTSTATUS WINAPI SystemFunction006( const char *password, char *hash )
{
    LMhash( (unsigned char *)hash, (const unsigned char *)password, strlen( password ) );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction008   (cryptsp.@)
 *     SystemFunction009   (cryptsp.@)
 *
 * Creates a LM response from a challenge and a password hash
 *
 * PARAMS
 *   challenge  [I] Challenge from authentication server
 *   hash       [I] NTLM hash (from SystemFunction006)
 *   response   [O] response to send back to the server
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 * NOTES
 *  see http://davenport.sourceforge.net/ntlm.html#theLmResponse
 *
 */
NTSTATUS WINAPI SystemFunction008( const BYTE *challenge, const BYTE *hash, BYTE *response )
{
    BYTE key[7 * 3];

    if (!challenge || !response)
        return STATUS_UNSUCCESSFUL;

    memset( key, 0, sizeof(key) );
    memcpy( key, hash, 0x10 );

    DEShash( response, key, challenge );
    DEShash( response + 8, key + 7, challenge );
    DEShash( response + 16, key + 14, challenge );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction012   (cryptsp.@)
 *     SystemFunction014   (cryptsp.@)
 *     SystemFunction016   (cryptsp.@)
 *     SystemFunction018   (cryptsp.@)
 *     SystemFunction020   (cryptsp.@)
 *     SystemFunction022   (cryptsp.@)
 *
 * Encrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS WINAPI SystemFunction012( const BYTE *in, const BYTE *key, BYTE *out )
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;

    DEShash( out, key, in );
    DEShash( out + 8, key + 7, in + 8 );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction013   (cryptsp.@)
 *     SystemFunction015   (cryptsp.@)
 *     SystemFunction021   (cryptsp.@)
 *     SystemFunction023   (cryptsp.@)
 *
 * Decrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to decrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive decrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS WINAPI SystemFunction013( const BYTE *in, const BYTE *key, BYTE *out )
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;

    DESunhash( out, key, in );
    DESunhash( out + 8, key + 7, in + 8 );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction024   (cryptsp.@)
 *     SystemFunction026   (cryptsp.@)
 *
 * Encrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS WINAPI SystemFunction024( const BYTE *in, const BYTE *key, BYTE *out )
{
    BYTE deskey[0x10];

    memcpy( deskey, key, 4 );
    memcpy( deskey + 4, key, 4 );
    memcpy( deskey + 8, key, 4 );
    memcpy( deskey + 12, key, 4 );

    DEShash( out, deskey, in );
    DEShash( out + 8, deskey + 7, in + 8 );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction025   (cryptsp.@)
 *     SystemFunction027   (cryptsp.@)
 *
 * Decrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS WINAPI SystemFunction025( const BYTE *in, const BYTE *key, BYTE *out )
{
    BYTE deskey[0x10];

    memcpy( deskey, key, 4 );
    memcpy( deskey + 4, key, 4 );
    memcpy( deskey + 8, key, 4 );
    memcpy( deskey + 12, key, 4 );

    DESunhash( out, deskey, in );
    DESunhash( out + 8, deskey + 7, in + 8 );

    return STATUS_SUCCESS;
}
