/* CryptXML Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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
#include "winbase.h"
#include "cryptxml.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptxml);

#define DOC_MAGIC 0x584d4c44
#define SIG_MAGIC 0x5349474e

struct object
{
    ULONG magic;
    CRYPT_XML_STATUS status;
};

struct xmldoc
{
    struct object hdr;
    CRYPT_XML_DOC_CTXT ctx;
};

struct signature
{
    struct object hdr;
    CRYPT_XML_SIGNATURE sig;
    CRYPT_XML_KEY_INFO key_info;
    struct xmldoc *doc;
};

struct xmldoc *alloc_doc( void )
{
    struct xmldoc *doc;

    if (!(doc = calloc( 1, sizeof( *doc ) + sizeof( *doc->ctx.rgpSignature ) ))) return NULL;

    doc->hdr.magic                = DOC_MAGIC;
    doc->hdr.status.cbSize        = sizeof( doc->hdr.status );
    doc->hdr.status.dwErrorStatus = CRYPT_XML_STATUS_ERROR_NOT_RESOLVED;

    doc->ctx.cbSize               = sizeof( doc->ctx );
    doc->ctx.hDocCtxt             = (HCRYPTXML)doc;
    doc->ctx.rgpSignature         = (CRYPT_XML_SIGNATURE **)(doc + 1);
    return doc;
}

struct signature *alloc_sig( const CRYPT_XML_BLOB *blob )
{
    struct signature *sig;

    if (!(sig = calloc( 1, sizeof( *sig ) + blob->cbData ))) return NULL;

    sig->hdr.magic                 = SIG_MAGIC;
    sig->hdr.status.cbSize         = sizeof( sig->hdr.status );
    sig->hdr.status.dwErrorStatus  = CRYPT_XML_STATUS_ERROR_NOT_RESOLVED;

    sig->sig.cbSize                = sizeof( sig->sig );
    sig->sig.hSignature            = (HCRYPTXML)sig;
    sig->sig.SignatureValue.pbData = (BYTE *)(sig + 1);
    memcpy( sig->sig.SignatureValue.pbData, blob->pbData, blob->cbData );
    sig->sig.SignatureValue.cbData = blob->cbData;

    sig->sig.pKeyInfo              = &sig->key_info;
    return sig;
}

HRESULT WINAPI CryptXmlOpenToDecode( const CRYPT_XML_TRANSFORM_CHAIN_CONFIG *config, DWORD flags,
                                     const CRYPT_XML_PROPERTY *property, ULONG property_count,
                                     const CRYPT_XML_BLOB *blob, HCRYPTXML *handle )
{
    struct xmldoc *doc;
    struct signature *sig;

    FIXME( "config %p, flags %lx, property %p, property_count %lu, blob %p, handle %p stub!\n",
            config, flags, property, property_count, blob, handle );

    if (!blob || !handle) return E_INVALIDARG;

    if (!(sig = alloc_sig( blob ))) return E_OUTOFMEMORY;
    if (!(doc = alloc_doc()))
    {
        free( sig );
        return E_OUTOFMEMORY;
    }

    doc->ctx.cSignature      = 1;
    doc->ctx.rgpSignature[0] = &sig->sig;
    sig->doc = doc;

    *handle = (HCRYPTXML)doc;
    return S_OK;
}

HRESULT WINAPI CryptXmlClose( HCRYPTXML handle )
{
    struct object *obj = (struct object *)handle;

    TRACE( "handle %p\n", handle );

    if (!obj) return E_INVALIDARG;

    switch (obj->magic)
    {
    case DOC_MAGIC:
    {
        struct xmldoc *doc = (struct xmldoc *)obj;
        const CRYPT_XML_SIGNATURE *sig = doc->ctx.rgpSignature[0];
        struct signature *sig_obj = CONTAINING_RECORD( sig, struct signature, sig );

        free( sig_obj );
        break;
    }
    case SIG_MAGIC:
        return S_FALSE;
    default:
        FIXME( "unhandled object magic %lx\n", obj->magic );
        return E_NOTIMPL;
    }

    free( obj );
    return S_OK;
}

HRESULT WINAPI CryptXmlGetDocContext( HCRYPTXML handle, const CRYPT_XML_DOC_CTXT **ctx )
{
    struct xmldoc *doc = (struct xmldoc *)handle;

    TRACE( "handle %p, ctx %p\n", handle, ctx );

    if (!doc || !ctx) return E_INVALIDARG;
    if (doc->hdr.magic != DOC_MAGIC)
    {
        *ctx = NULL;
        return CRYPT_XML_E_HANDLE;
    }

    *ctx = &doc->ctx;
    return S_OK;
}

HRESULT WINAPI CryptXmlGetSignature( HCRYPTXML handle, const CRYPT_XML_SIGNATURE **ret_sig )
{
    struct signature *sig = (struct signature *)handle;

    TRACE( "handle %p, ret_sig %p\n", handle, ret_sig );

    if (!sig || !ret_sig) return E_INVALIDARG;
    if (sig->hdr.magic != SIG_MAGIC)
    {
        *ret_sig = NULL;
        return CRYPT_XML_E_HANDLE;
    }

    *ret_sig = &sig->sig;
    return S_OK;
}

HRESULT WINAPI CryptXmlVerifySignature( HCRYPTXML handle, BCRYPT_KEY_HANDLE key, DWORD flags )
{
    struct signature *sig = (struct signature *)handle;

    FIXME( "handle %p, key %p, flags %lx stub!\n", handle, key, flags );

    if (!sig) return E_INVALIDARG;
    if (sig->hdr.magic != SIG_MAGIC) return CRYPT_XML_E_HANDLE;

    sig->hdr.status.dwErrorStatus = sig->doc->hdr.status.dwErrorStatus = CRYPT_XML_STATUS_NO_ERROR;
    sig->hdr.status.dwInfoStatus  = sig->doc->hdr.status.dwInfoStatus  = CRYPT_XML_STATUS_SIGNATURE_VALID;
    return S_OK;
}
