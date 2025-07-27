/*
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

#ifndef __CRYPTXML_H__
#define __CRYPTXML_H__

#include <wincrypt.h>
#include <bcrypt.h>
#include <ncrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *HCRYPTXML;

#define CRYPT_XML_E_BASE                 _HRESULT_TYPEDEF_(0x80092100)
#define CRYPT_XML_E_LARGE                _HRESULT_TYPEDEF_(0x80092101)
#define CRYPT_XML_E_TOO_MANY_TRANSFORMS  _HRESULT_TYPEDEF_(0x80092102)
#define CRYPT_XML_E_ENCODING             _HRESULT_TYPEDEF_(0x80092103)
#define CRYPT_XML_E_ALGORITHM            _HRESULT_TYPEDEF_(0x80092104)
#define CRYPT_XML_E_TRANSFORM            _HRESULT_TYPEDEF_(0x80092105)
#define CRYPT_XML_E_HANDLE               _HRESULT_TYPEDEF_(0x80092106)
#define CRYPT_XML_E_OPERATION            _HRESULT_TYPEDEF_(0x80092107)
#define CRYPT_XML_E_UNRESOLVED_REFERENCE _HRESULT_TYPEDEF_(0x80092108)
#define CRYPT_XML_E_INVALID_DIGEST       _HRESULT_TYPEDEF_(0x80092109)
#define CRYPT_XML_E_INVALID_SIGNATURE    _HRESULT_TYPEDEF_(0x8009210A)
#define CRYPT_XML_E_HASH_FAILED          _HRESULT_TYPEDEF_(0x8009210B)
#define CRYPT_XML_E_SIGN_FAILED          _HRESULT_TYPEDEF_(0x8009210C)
#define CRYPT_XML_E_VERIFY_FAILED        _HRESULT_TYPEDEF_(0x8009210D)
#define CRYPT_XML_E_TOO_MANY_SIGNATURES  _HRESULT_TYPEDEF_(0x8009210E)
#define CRYPT_XML_E_INVALID_KEYVALUE     _HRESULT_TYPEDEF_(0x8009210F)
#define CRYPT_XML_E_UNEXPECTED_XML       _HRESULT_TYPEDEF_(0x80092110)
#define CRYPT_XML_E_SIGNER               _HRESULT_TYPEDEF_(0x80092111)
#define CRYPT_XML_E_NON_UNIQUE_ID        _HRESULT_TYPEDEF_(0x80092112)
#define CRYPT_XML_E_LAST                 _HRESULT_TYPEDEF_(0x80092112)

#define CRYPT_XML_FLAG_NO_SERIALIZE                 0x80000000
#define CRYPT_XML_FLAG_ALWAYS_RETURN_ENCODED_OBJECT 0x40000000
#define CRYPT_XML_FLAG_ENFORCE_ID_NCNAME_FORMAT     0x20000000
#define CRYPT_XML_FLAG_DISABLE_EXTENSIONS           0x10000000
#define CRYPT_XML_FLAG_ENFORCE_ID_NAME_FORMAT       0x08000000
#define CRYPT_XML_FLAG_ECDSA_DSIG11                 0x04000000

typedef HRESULT (CALLBACK *PFN_CRYPT_XML_DATA_PROVIDER_READ)(void *, BYTE *, ULONG, ULONG *);
typedef HRESULT (CALLBACK *PFN_CRYPT_XML_DATA_PROVIDER_CLOSE)(void *);

typedef enum _CRYPT_XML_CHARSET
{
    CRYPT_XML_CHARSET_AUTO    = 0,
    CRYPT_XML_CHARSET_UTF8    = 1,
    CRYPT_XML_CHARSET_UTF16LE = 2,
    CRYPT_XML_CHARSET_UTF16BE = 3,
} CRYPT_XML_CHARSET;

typedef struct _CRYPT_XML_BLOB
{
    CRYPT_XML_CHARSET dwCharset;
    ULONG             cbData;
    BYTE             *pbData;
} CRYPT_XML_BLOB, *PCRYPT_XML_BLOB;

typedef struct _CRYPT_XML_DATA_BLOB
{
    ULONG             cbData;
    BYTE             *pbData;
} CRYPT_XML_DATA_BLOB, *PCRYPT_XML_DATA_BLOB;

typedef struct _CRYPT_XML_DATA_PROVIDER
{
    void                             *pvCallbackState;
    ULONG                             cbBufferSize;
    PFN_CRYPT_XML_DATA_PROVIDER_READ  pfnRead;
    PFN_CRYPT_XML_DATA_PROVIDER_CLOSE pfnClose;
} CRYPT_XML_DATA_PROVIDER, *PCRYPT_XML_DATA_PROVIDER;

typedef struct _CRYPT_XML_ALGORITHM
{
    ULONG          cbSize;
    LPCWSTR        wszAlgorithm;
    CRYPT_XML_BLOB Encoded;
} CRYPT_XML_ALGORITHM, *PCRYPT_XML_ALGORITHM;

typedef HRESULT (CALLBACK *PFN_CRYPT_XML_CREATE_TRANSFORM)(const CRYPT_XML_ALGORITHM *, CRYPT_XML_DATA_PROVIDER *, CRYPT_XML_DATA_PROVIDER *);

typedef struct _CRYPT_XML_TRANSFORM_INFO
{
    ULONG                          cbSize;
    LPCWSTR                        wszAlgorithm;
    ULONG                          cbBufferSize;
    DWORD                          dwFlags;
    PFN_CRYPT_XML_CREATE_TRANSFORM pfnCreateTransform;
} CRYPT_XML_TRANSFORM_INFO, *PCRYPT_XML_TRANSFORM_INFO;

typedef struct _CRYPT_XML_TRANSFORM_CHAIN_CONFIG
{
    ULONG                      cbSize;
    ULONG                      cTransformInfo;
    PCRYPT_XML_TRANSFORM_INFO *rgpTransformInfo;
} CRYPT_XML_TRANSFORM_CHAIN_CONFIG, *PCRYPT_XML_TRANSFORM_CHAIN_CONFIG;

typedef enum _CRYPT_XML_PROPERTY_ID
{
    CRYPT_XML_PROPERTY_MAX_HEAP_SIZE      = 1,
    CRYPT_XML_PROPERTY_SIGNATURE_LOCATION = 2,
    CRYPT_XML_PROPERTY_MAX_SIGNATURES     = 3,
    CRYPT_XML_PROPERTY_DOC_DECLARATION    = 4,
    CRYPT_XML_PROPERTY_XML_OUTPUT_CHARSET = 5,
} CRYPT_XML_PROPERTY_ID;

typedef struct _CRYPT_XML_PROPERTY
{
    CRYPT_XML_PROPERTY_ID dwPropId;
    const void           *pvValue;
    ULONG                 cbValue;
} CRYPT_XML_PROPERTY, *PCRYPT_XML_PROPERTY;

typedef struct _CRYPT_XML_ISSUER_SERIAL
{
    LPCWSTR wszIssuer;
    LPCWSTR wszSerial;
} CRYPT_XML_ISSUER_SERIAL;

typedef struct _CRYPT_XML_X509DATA_ITEM
{
    DWORD dwType;
    union
    {
        CRYPT_XML_ISSUER_SERIAL IssuerSerial;
        CRYPT_XML_DATA_BLOB     SKI;
        LPCWSTR                 wszSubjectName;
        CRYPT_XML_DATA_BLOB     Certificate;
        CRYPT_XML_DATA_BLOB     CRL;
        CRYPT_XML_BLOB          Custom;
    };
} CRYPT_XML_X509DATA_ITEM;

#define CRYPT_XML_X509DATA_TYPE_ISSUER_SERIAL 0x00000001
#define CRYPT_XML_X509DATA_TYPE_SKI           0x00000002
#define CRYPT_XML_X509DATA_TYPE_SUBJECT_NAME  0x00000003
#define CRYPT_XML_X509DATA_TYPE_CERTIFICATE   0x00000004
#define CRYPT_XML_X509DATA_TYPE_CRL           0x00000005
#define CRYPT_XML_X509DATA_TYPE_CUSTOM        0x00000006

typedef struct _CRYPT_XML_X509DATA
{
    UINT                     cX509Data;
    CRYPT_XML_X509DATA_ITEM *rgX509Data;
} CRYPT_XML_X509DATA;

typedef struct _CRYPT_XML_KEY_DSA_KEY_VALUE
{
    CRYPT_XML_DATA_BLOB    P;
    CRYPT_XML_DATA_BLOB    Q;
    CRYPT_XML_DATA_BLOB    G;
    CRYPT_XML_DATA_BLOB    Y;
    CRYPT_XML_DATA_BLOB    J;
    CRYPT_XML_DATA_BLOB    Seed;
    CRYPT_XML_DATA_BLOB    Counter;
} CRYPT_XML_KEY_DSA_KEY_VALUE;

typedef struct _CRYPT_XML_KEY_RSA_KEY_VALUE{
    CRYPT_XML_DATA_BLOB    Modulus;
    CRYPT_XML_DATA_BLOB    Exponent;
} CRYPT_XML_KEY_RSA_KEY_VALUE;

typedef struct _CRYPT_XML_KEY_ECDSA_KEY_VALUE
{
    LPCWSTR                wszNamedCurve;
    CRYPT_XML_DATA_BLOB    X;
    CRYPT_XML_DATA_BLOB    Y;
    CRYPT_XML_BLOB         ExplicitPara;
} CRYPT_XML_KEY_ECDSA_KEY_VALUE;

typedef struct _CRYPT_XML_KEY_VALUE
{
    DWORD dwType;
    union
    {
        CRYPT_XML_KEY_DSA_KEY_VALUE   DSAKeyValue;
        CRYPT_XML_KEY_RSA_KEY_VALUE   RSAKeyValue;
        CRYPT_XML_KEY_ECDSA_KEY_VALUE ECDSAKeyValue;
        CRYPT_XML_BLOB                Custom;
    };
} CRYPT_XML_KEY_VALUE;

#define CRYPT_XML_KEY_VALUE_TYPE_DSA    0x00000001
#define CRYPT_XML_KEY_VALUE_TYPE_RSA    0x00000002
#define CRYPT_XML_KEY_VALUE_TYPE_ECDSA  0x00000003
#define CRYPT_XML_KEY_VALUE_TYPE_CUSTOM 0x00000004

typedef struct _CRYPT_XML_KEY_INFO_ITEM
{
    DWORD dwType;
    union
    {
        LPCWSTR             wszKeyName;
        CRYPT_XML_KEY_VALUE KeyValue;
        CRYPT_XML_BLOB      RetrievalMethod;
        CRYPT_XML_X509DATA  X509Data;
        CRYPT_XML_BLOB      Custom;
    };
} CRYPT_XML_KEY_INFO_ITEM;

#define CRYPT_XML_KEYINFO_TYPE_KEYNAME   0x00000001
#define CRYPT_XML_KEYINFO_TYPE_KEYVALUE  0x00000002
#define CRYPT_XML_KEYINFO_TYPE_RETRIEVAL 0x00000003
#define CRYPT_XML_KEYINFO_TYPE_X509DATA  0x00000004
#define CRYPT_XML_KEYINFO_TYPE_CUSTOM    0x00000005

typedef struct _CRYPT_XML_KEY_INFO
{
    ULONG                    cbSize;
    LPCWSTR                  wszId;
    UINT                     cKeyInfo;
    CRYPT_XML_KEY_INFO_ITEM *rgKeyInfo;
    BCRYPT_KEY_HANDLE        hVerifyKey;
} CRYPT_XML_KEY_INFO, *PCRYPT_XML_KEY_INFO;

typedef struct _CRYPT_XML_REFERENCE
{
    ULONG                cbSize;
    HCRYPTXML            hReference;
    LPCWSTR              wszId;
    LPCWSTR              wszUri;
    LPCWSTR              wszType;
    CRYPT_XML_ALGORITHM  DigestMethod;
    CRYPT_DATA_BLOB      DigestValue;
    ULONG                cTransform;
    CRYPT_XML_ALGORITHM *rgTransform;
} CRYPT_XML_REFERENCE, *PCRYPT_XML_REFERENCE;

typedef struct _CRYPT_XML_REFERENCES
{
    ULONG                 cReference;
    PCRYPT_XML_REFERENCE *rgpReference;
} CRYPT_XML_REFERENCES, *PCRYPT_XML_REFERENCES;

typedef struct _CRYPT_XML_OBJECT
{
    ULONG                cbSize;
    HCRYPTXML            hObject;
    LPCWSTR              wszId;
    LPCWSTR              wszMimeType;
    LPCWSTR              wszEncoding;
    CRYPT_XML_REFERENCES Manifest;
    CRYPT_XML_BLOB       Encoded;
} CRYPT_XML_OBJECT, *PCRYPT_XML_OBJECT;

typedef struct _CRYPT_XML_SIGNED_INFO
{
    ULONG                 cbSize;
    LPCWSTR               wszId;
    CRYPT_XML_ALGORITHM   Canonicalization;
    CRYPT_XML_ALGORITHM   SignatureMethod;
    ULONG                 cReference;
    PCRYPT_XML_REFERENCE *rgpReference;
    CRYPT_XML_BLOB        Encoded;
} CRYPT_XML_SIGNED_INFO, *PCRYPT_XML_SIGNED_INFO;

typedef struct _CRYPT_XML_SIGNATURE
{
    ULONG                 cbSize;
    HCRYPTXML             hSignature;
    LPCWSTR               wszId;
    CRYPT_XML_SIGNED_INFO SignedInfo;
    CRYPT_DATA_BLOB       SignatureValue;
    CRYPT_XML_KEY_INFO    *pKeyInfo;
    ULONG                 cObject;
    PCRYPT_XML_OBJECT     *rgpObject;
} CRYPT_XML_SIGNATURE, *PCRYPT_XML_SIGNATURE;

typedef struct _CRYPT_XML_DOC_CTXT
{
    ULONG                             cbSize;
    HCRYPTXML                         hDocCtxt;
    CRYPT_XML_TRANSFORM_CHAIN_CONFIG *pTransformsConfig;
    ULONG                             cSignature;
    PCRYPT_XML_SIGNATURE             *rgpSignature;
} CRYPT_XML_DOC_CTXT, *PCRYPT_XML_DOC_CTXT;

typedef struct _CRYPT_XML_STATUS
{
    ULONG cbSize;
    DWORD dwErrorStatus;
    DWORD dwInfoStatus;
} CRYPT_XML_STATUS, *PCRYPT_XML_STATUS;

#define CRYPT_XML_STATUS_NO_ERROR                      0x00000000
#define CRYPT_XML_STATUS_ERROR_NOT_RESOLVED            0x00000001
#define CRYPT_XML_STATUS_ERROR_DIGEST_INVALID          0x00000002
#define CRYPT_XML_STATUS_ERROR_NOT_SUPPORTED_ALGORITHM 0x00000004
#define CRYPT_XML_STATUS_ERROR_NOT_SUPPORTED_TRANSFORM 0x00000008
#define CRYPT_XML_STATUS_ERROR_SIGNATURE_INVALID       0x00010000
#define CRYPT_XML_STATUS_ERROR_KEYINFO_NOT_PARSED      0x00020000

#define CRYPT_XML_STATUS_INTERNAL_REFERENCE            0x00000001
#define CRYPT_XML_STATUS_KEY_AVAILABLE                 0x00000002
#define CRYPT_XML_STATUS_DIGESTING                     0x00000004
#define CRYPT_XML_STATUS_DIGEST_VALID                  0x00000008
#define CRYPT_XML_STATUS_SIGNATURE_VALID               0x00010000
#define CRYPT_XML_STATUS_OPENED_TO_ENCODE              0x80000000

HRESULT WINAPI CryptXmlClose(HCRYPTXML handle);
HRESULT WINAPI CryptXmlGetDocContext(HCRYPTXML handle, const CRYPT_XML_DOC_CTXT **context);
HRESULT WINAPI CryptXmlGetSignature(HCRYPTXML handle, const CRYPT_XML_SIGNATURE **signature);
HRESULT WINAPI CryptXmlGetStatus(HCRYPTXML handle, CRYPT_XML_STATUS *status);
HRESULT WINAPI CryptXmlOpenToDecode(const CRYPT_XML_TRANSFORM_CHAIN_CONFIG *config, DWORD flags, const CRYPT_XML_PROPERTY *property,
                                    ULONG property_count, const CRYPT_XML_BLOB *blob, HCRYPTXML *handle);
HRESULT WINAPI CryptXmlVerifySignature(HCRYPTXML handle, BCRYPT_KEY_HANDLE key, DWORD flags);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTXML_H */
