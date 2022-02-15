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

#include "bcrypt.h"
#include "tomcrypt.h"

#define RSAENH_MAX_HASH_SIZE        104

typedef union tagKEY_CONTEXT {
    rc2_key rc2;
    des_key des;
    des3_key des3;
    aes_key aes;
    prng_state rc4;
    rsa_key rsa;
} KEY_CONTEXT;

BOOL init_hash_impl(ALG_ID aiAlgid, BCRYPT_HASH_HANDLE *hash_handle) DECLSPEC_HIDDEN;
BOOL update_hash_impl(BCRYPT_HASH_HANDLE hash_handle, const BYTE *pbData,
                      DWORD dwDataLen) DECLSPEC_HIDDEN;
BOOL finalize_hash_impl(BCRYPT_HASH_HANDLE hash_handle, BYTE *pbHashValue) DECLSPEC_HIDDEN;
BOOL duplicate_hash_impl(BCRYPT_HASH_HANDLE src_hash_handle,
                         BCRYPT_HASH_HANDLE *dest_hash_handle) DECLSPEC_HIDDEN;

BOOL new_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen) DECLSPEC_HIDDEN;
BOOL free_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext) DECLSPEC_HIDDEN;
BOOL setup_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                    DWORD dwEffectiveKeyLen, DWORD dwSaltLen, BYTE *abKeyValue) DECLSPEC_HIDDEN;
BOOL duplicate_key_impl(ALG_ID aiAlgid, const KEY_CONTEXT *pSrcKeyContext,
                        KEY_CONTEXT *pDestKeyContext) DECLSPEC_HIDDEN;

/* dwKeySpec is optional for symmetric key algorithms */
BOOL encrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, const BYTE *pbIn,
                        BYTE *pbOut, DWORD enc) DECLSPEC_HIDDEN;
BOOL encrypt_stream_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, BYTE *pbInOut, DWORD dwLen) DECLSPEC_HIDDEN;

BOOL export_public_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                            DWORD *pdwPubExp) DECLSPEC_HIDDEN;
BOOL import_public_key_impl(const BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                            DWORD dwPubExp) DECLSPEC_HIDDEN;
BOOL export_private_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD *pdwPubExp) DECLSPEC_HIDDEN;
BOOL import_private_key_impl(const BYTE* pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD dwDataLen, DWORD dwPubExp) DECLSPEC_HIDDEN;

BOOL gen_rand_impl(BYTE *pbBuffer, DWORD dwLen) DECLSPEC_HIDDEN;

#endif /* __WINE_IMPLGLUE_H */
