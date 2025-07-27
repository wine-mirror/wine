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
