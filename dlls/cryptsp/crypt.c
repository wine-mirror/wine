/*
 * Copyright 1999 Ian Schmidt
 * Copyright 2001 Travis Michielsen
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef struct tagMD4_CTX
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

void WINAPI MD4Init(MD4_CTX *ctx);
void WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len);
void WINAPI MD4Final(MD4_CTX *ctx);

/******************************************************************************
 *     SystemFunction007   (cryptsp.@)
 *
 * MD4 hash a unicode string
 *
 * PARAMS
 *   string  [I] the string to hash
 *   output  [O] the md4 hash of the string (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction007( const UNICODE_STRING *string, BYTE *hash )
{
    MD4_CTX ctx;

    MD4Init( &ctx );
    MD4Update( &ctx, (const BYTE *)string->Buffer, string->Length );
    MD4Final( &ctx );
    memcpy( hash, ctx.digest, 0x10 );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction010   (cryptsp.@)
 *     SystemFunction011   (cryptsp.@)
 *
 * MD4 hashes 16 bytes of data
 *
 * PARAMS
 *   unknown []  seems to have no effect on the output
 *   data    [I] pointer to data to hash (16 bytes)
 *   output  [O] the md4 hash of the data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS WINAPI SystemFunction010( void *unknown, const BYTE *data, BYTE *hash )
{
    MD4_CTX ctx;

    MD4Init( &ctx );
    MD4Update( &ctx, data, 0x10 );
    MD4Final( &ctx );
    memcpy( hash, ctx.digest, 0x10 );

    return STATUS_SUCCESS;
}

/******************************************************************************
 *     SystemFunction030   (cryptsp.@)
 *     SystemFunction031   (cryptsp.@)
 *
 * Tests if two blocks of 16 bytes are equal
 *
 * PARAMS
 *  b1,b2   [I] block of 16 bytes
 *
 * RETURNS
 *  TRUE  if blocks are the same
 *  FALSE if blocks are different
 */
BOOL WINAPI SystemFunction030( const void *b1, const void *b2 )
{
    return !memcmp( b1, b2, 0x10 );
}

/******************************************************************************
 *     CheckSignatureInFile   (cryptsp.@)
 *     SystemFunction035   (cryptsp.@)
 */
BOOL WINAPI CheckSignatureInFile( const char *file )
{
    FIXME( "%s: stub\n", debugstr_a(file) );
    return TRUE;
}
