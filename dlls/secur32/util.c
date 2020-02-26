/*
 * Copyright 2006 Kai Blin
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
 *
 * This file contains various helper functions needed for NTLM and maybe others
 */
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "sspi.h"
#include "secur32_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

static const char client_to_server_sign_constant[] = "session key to client-to-server signing key magic constant";
static const char client_to_server_seal_constant[] = "session key to client-to-server sealing key magic constant";
static const char server_to_client_sign_constant[] = "session key to server-to-client signing key magic constant";
static const char server_to_client_seal_constant[] = "session key to server-to-client sealing key magic constant";

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

/* And now the same with a different memory layout. */
typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

VOID WINAPI MD4Init( MD4_CTX *ctx );
VOID WINAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD4Final( MD4_CTX *ctx );
VOID WINAPI MD5Init( MD5_CTX *ctx );
VOID WINAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD5Final( MD5_CTX *ctx );

SECURITY_STATUS SECUR32_CreateNTLM1SessionKey(PBYTE password, int len, PBYTE session_key)
{
    MD4_CTX ctx;
    BYTE ntlm_hash[16];

    TRACE("(%p, %p)\n", password, session_key);

    MD4Init(&ctx);
    MD4Update(&ctx, password, len);
    MD4Final(&ctx);

    memcpy(ntlm_hash, ctx.digest, 0x10);

    MD4Init(&ctx);
    MD4Update(&ctx, ntlm_hash, 0x10u);
    MD4Final(&ctx);

    memcpy(session_key, ctx.digest, 0x10);

    return SEC_E_OK;
}

static void SECUR32_CalcNTLM2Subkey(const BYTE *session_key, const char *magic, PBYTE subkey)
{
    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, session_key, 16);
    MD5Update(&ctx, (const unsigned char*)magic, lstrlenA(magic)+1);
    MD5Final(&ctx);
    memcpy(subkey, ctx.digest, 16);
}

/* This assumes we do have a valid NTLM2 user session key */
SECURITY_STATUS SECUR32_CreateNTLM2SubKeys(PNegoHelper helper)
{
    helper->crypt.ntlm2.send_sign_key = heap_alloc(16);
    helper->crypt.ntlm2.send_seal_key = heap_alloc(16);
    helper->crypt.ntlm2.recv_sign_key = heap_alloc(16);
    helper->crypt.ntlm2.recv_seal_key = heap_alloc(16);

    if(helper->mode == NTLM_CLIENT)
    {
        SECUR32_CalcNTLM2Subkey(helper->session_key, client_to_server_sign_constant,
                helper->crypt.ntlm2.send_sign_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, client_to_server_seal_constant,
                helper->crypt.ntlm2.send_seal_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, server_to_client_sign_constant,
                helper->crypt.ntlm2.recv_sign_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, server_to_client_seal_constant,
                helper->crypt.ntlm2.recv_seal_key);
    }
    else
    {
        SECUR32_CalcNTLM2Subkey(helper->session_key, server_to_client_sign_constant,
                helper->crypt.ntlm2.send_sign_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, server_to_client_seal_constant,
                helper->crypt.ntlm2.send_seal_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, client_to_server_sign_constant,
                helper->crypt.ntlm2.recv_sign_key);
        SECUR32_CalcNTLM2Subkey(helper->session_key, client_to_server_seal_constant,
                helper->crypt.ntlm2.recv_seal_key);
    }

    return SEC_E_OK;
}

arc4_info *SECUR32_arc4Alloc(void)
{
    arc4_info *a4i = heap_alloc(sizeof(arc4_info));
    return a4i;
}

/*
 * The arc4 code is based on dlls/advapi32/crypt_arc4.c by Mike McCormack,
 * which in turn is based on public domain code by Wei Dai
 */
void SECUR32_arc4Init(arc4_info *a4i, const BYTE *key, unsigned int keyLen)
{
    unsigned int keyIndex = 0, stateIndex = 0;
    unsigned int i, a;

    TRACE("(%p, %p, %d)\n", a4i, key, keyLen);

        a4i->x = a4i->y = 0;

    for (i=0; i<256; i++)
        a4i->state[i] = i;

    for (i=0; i<256; i++)
    {
        a = a4i->state[i];
        stateIndex += key[keyIndex] + a;
        stateIndex &= 0xff;
        a4i->state[i] = a4i->state[stateIndex];
        a4i->state[stateIndex] = a;
        if (++keyIndex >= keyLen)
            keyIndex = 0;
    }

}

void SECUR32_arc4Process(arc4_info *a4i, BYTE *inoutString, unsigned int length)
{
    BYTE *const s=a4i->state;
    unsigned int x = a4i->x;
    unsigned int y = a4i->y;
    unsigned int a, b;

    while(length--)
    {
        x = (x+1) & 0xff;
        a = s[x];
        y = (y+a) & 0xff;
        b = s[y];
        s[x] = b;
        s[y] = a;
        *inoutString++ ^= s[(a+b) & 0xff];
    }

    a4i->x = x;
    a4i->y = y;
}

void SECUR32_arc4Cleanup(arc4_info *a4i)
{
    heap_free(a4i);
}
