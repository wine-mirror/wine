/*
 * dlls/rsaenh/implglue.h
 * Glueing the RSAENH specific code to the crypto library
 *
 * Copyright (c) 2004 Michael Jung
 *
 * based on code by Mike McCormack and David Hammerton
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

#ifndef __WINE_IMPLGLUE_H
#define __WINE_IMPLGLUE_H

#include "tomcrypt.h"

#define RSAENH_MAX_HASH_SIZE        104

typedef union tagKEY_CONTEXT {
    symmetric_key key;
    prng_state prng;
    rsa_key rsa;
} KEY_CONTEXT;

extern prng_state prng;
extern int wprng;

struct hash
{
    const struct ltc_hash_descriptor *desc;
    hash_state state;
};

BOOL init_hash_impl(ALG_ID algid, struct hash *hash);
BOOL update_hash_impl(struct hash *hash, const BYTE *data, DWORD len);
BOOL finalize_hash_impl(struct hash *hash, BYTE *hash_value, DWORD hash_size);

BOOL new_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen);
BOOL free_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext);
BOOL setup_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                    DWORD dwEffectiveKeyLen, DWORD dwSaltLen, BYTE *abKeyValue);
BOOL duplicate_key_impl(ALG_ID aiAlgid, const KEY_CONTEXT *pSrcKeyContext,
                        KEY_CONTEXT *pDestKeyContext);

/* dwKeySpec is optional for symmetric key algorithms */
BOOL encrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, const BYTE *pbIn,
                        BYTE *pbOut);
BOOL decrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, const BYTE *pbIn,
                        BYTE *pbOut);
BOOL encrypt_stream_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, BYTE *pbInOut, DWORD dwLen);

BOOL export_public_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                            DWORD *pdwPubExp);
BOOL import_public_key_impl(const BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                            DWORD dwPubExp);
BOOL export_private_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD *pdwPubExp);
BOOL import_private_key_impl(const BYTE* pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD dwDataLen, DWORD dwPubExp);

#endif /* __WINE_IMPLGLUE_H */
