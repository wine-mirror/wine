/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_BCRYPT_H
#define __WINE_BCRYPT_H

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif

#define BCRYPT_ALGORITHM_NAME (const WCHAR []){'A','l','g','o','r','i','t','h','m','N','a','m','e',0}
#define BCRYPT_AUTH_TAG_LENGTH (const WCHAR []){'A','u','t','h','T','a','g','L','e','n','g','t','h',0}
#define BCRYPT_BLOCK_LENGTH (const WCHAR []){'B','l','o','c','k','L','e','n','g','t','h',0}
#define BCRYPT_BLOCK_SIZE_LIST (const WCHAR []){'B','l','o','c','k','S','i','z','e','L','i','s','t',0}
#define BCRYPT_CHAINING_MODE (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e',0}
#define BCRYPT_EFFECTIVE_KEY_LENGTH (const WCHAR []){'E','f','f','e','c','t','i','v','e','K','e','y','L','e','n','g','t','h',0}
#define BCRYPT_HASH_BLOCK_LENGTH (const WCHAR []){'H','a','s','h','B','l','o','c','k','L','e','n','g','t','h',0}
#define BCRYPT_HASH_LENGTH (const WCHAR []){'H','a','s','h','D','i','g','e','s','t','L','e','n','g','t','h',0}
#define BCRYPT_HASH_OID_LIST (const WCHAR []){'H','a','s','h','O','I','D','L','i','s','t',0}
#define BCRYPT_KEY_LENGTH (const WCHAR []){'K','e','y','L','e','n','g','t','h',0}
#define BCRYPT_KEY_LENGTHS (const WCHAR []){'K','e','y','L','e','n','g','t','h','s',0}
#define BCRYPT_KEY_OBJECT_LENGTH (const WCHAR []){'K','e','y','O','b','j','e','c','t','L','e','n','g','t','h',0}
#define BCRYPT_KEY_STRENGTH (const WCHAR []){'K','e','y','S','t','r','e','n','g','t','h',0}
#define BCRYPT_OBJECT_LENGTH (const WCHAR []){'O','b','j','e','c','t','L','e','n','g','t','h',0}
#define BCRYPT_PADDING_SCHEMES (const WCHAR []){'P','a','d','d','i','n','g','S','c','h','e','m','e','s',0}
#define BCRYPT_PROVIDER_HANDLE (const WCHAR []){'P','r','o','v','i','d','e','r','H','a','n','d','l','e',0}
#define BCRYPT_SIGNATURE_LENGTH (const WCHAR []){'S','i','g','n','a','t','u','r','e','L','e','n','g','t','h',0}

#define BCRYPT_OPAQUE_KEY_BLOB   (const WCHAR []){'O','p','a','q','u','e','K','e','y','B','l','o','b',0}
#define BCRYPT_KEY_DATA_BLOB     (const WCHAR []){'K','e','y','D','a','t','a','B','l','o','b',0}
#define BCRYPT_AES_WRAP_KEY_BLOB (const WCHAR []){'R','f','c','3','5','6','5','K','e','y','W','r','a','p','B','l','o','b',0}
#define BCRYPT_ECCPUBLIC_BLOB    (const WCHAR []){'E','C','C','P','U','B','L','I','C','B','L','O','B',0}
#define BCRYPT_ECCPRIVATE_BLOB   (const WCHAR []){'E','C','C','P','R','I','V','A','T','E','B','L','O','B',0}
#define BCRYPT_RSAPUBLIC_BLOB    (const WCHAR []){'R','S','A','P','U','B','L','I','C','B','L','O','B',0}
#define BCRYPT_RSAPRIVATE_BLOB   (const WCHAR []){'R','S','A','P','R','I','V','A','T','E','B','L','O','B',0}

#define MS_PRIMITIVE_PROVIDER (const WCHAR [])\
    {'M','i','c','r','o','s','o','f','t',' ','P','r','i','m','i','t','i','v','e',' ','P','r','o','v','i','d','e','r',0}
#define MS_PLATFORM_CRYPTO_PROVIDER (const WCHAR [])\
    {'M','i','c','r','o','s','o','f','t',' ','P','l','a','t','f','o','r','m',' ','C','r','y','p','t','o',' ','P','r','o','v','i','d','e','r',0}

#define BCRYPT_AES_ALGORITHM        (const WCHAR []){'A','E','S',0}
#define BCRYPT_MD2_ALGORITHM        (const WCHAR []){'M','D','2',0}
#define BCRYPT_MD4_ALGORITHM        (const WCHAR []){'M','D','4',0}
#define BCRYPT_MD5_ALGORITHM        (const WCHAR []){'M','D','5',0}
#define BCRYPT_RNG_ALGORITHM        (const WCHAR []){'R','N','G',0}
#define BCRYPT_RSA_ALGORITHM        (const WCHAR []){'R','S','A',0}
#define BCRYPT_SHA1_ALGORITHM       (const WCHAR []){'S','H','A','1',0}
#define BCRYPT_SHA256_ALGORITHM     (const WCHAR []){'S','H','A','2','5','6',0}
#define BCRYPT_SHA384_ALGORITHM     (const WCHAR []){'S','H','A','3','8','4',0}
#define BCRYPT_SHA512_ALGORITHM     (const WCHAR []){'S','H','A','5','1','2',0}
#define BCRYPT_ECDH_P256_ALGORITHM  (const WCHAR []){'E','C','D','H','_','P','2','5','6',0}
#define BCRYPT_ECDSA_P256_ALGORITHM (const WCHAR []){'E','C','D','S','A','_','P','2','5','6',0}
#define BCRYPT_ECDSA_P384_ALGORITHM (const WCHAR []){'E','C','D','S','A','_','P','3','8','4',0}
#define BCRYPT_ECDSA_P521_ALGORITHM (const WCHAR []){'E','C','D','S','A','_','P','5','2','1',0}

#define BCRYPT_CHAIN_MODE_NA        (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','N','/','A',0}
#define BCRYPT_CHAIN_MODE_CBC       (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','C','B','C',0}
#define BCRYPT_CHAIN_MODE_ECB       (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','E','C','B',0}
#define BCRYPT_CHAIN_MODE_CFB       (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','C','F','B',0}
#define BCRYPT_CHAIN_MODE_CCM       (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','C','C','M',0}
#define BCRYPT_CHAIN_MODE_GCM       (const WCHAR []){'C','h','a','i','n','i','n','g','M','o','d','e','G','C','M',0}

#define BCRYPT_ECDSA_PUBLIC_P256_MAGIC  0x31534345
#define BCRYPT_ECDSA_PRIVATE_P256_MAGIC 0x32534345
#define BCRYPT_ECDSA_PUBLIC_P384_MAGIC  0x33534345
#define BCRYPT_ECDSA_PRIVATE_P384_MAGIC 0x34534345
#define BCRYPT_ECDSA_PUBLIC_P521_MAGIC  0x35534345
#define BCRYPT_ECDSA_PRIVATE_P521_MAGIC 0x36534345

#define BCRYPT_ECDH_PUBLIC_P256_MAGIC  0x314b4345
#define BCRYPT_ECDH_PRIVATE_P256_MAGIC 0x324b4345
#define BCRYPT_ECDH_PUBLIC_P384_MAGIC  0x334b4345
#define BCRYPT_ECDH_PRIVATE_P384_MAGIC 0x344b4345
#define BCRYPT_ECDH_PUBLIC_P521_MAGIC  0x354b4345
#define BCRYPT_ECDH_PRIVATE_P521_MAGIC 0x364b4345

typedef struct _BCRYPT_ALGORITHM_IDENTIFIER
{
    LPWSTR pszName;
    ULONG  dwClass;
    ULONG  dwFlags;
} BCRYPT_ALGORITHM_IDENTIFIER;

typedef struct __BCRYPT_KEY_LENGTHS_STRUCT
{
    ULONG dwMinLength;
    ULONG dwMaxLength;
    ULONG dwIncrement;
} BCRYPT_KEY_LENGTHS_STRUCT, BCRYPT_AUTH_TAG_LENGTHS_STRUCT;

typedef struct _BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO
{
    ULONG cbSize;
    ULONG dwInfoVersion;
    UCHAR *pbNonce;
    ULONG cbNonce;
    UCHAR *pbAuthData;
    ULONG cbAuthData;
    UCHAR *pbTag;
    ULONG cbTag;
    UCHAR *pbMacContext;
    ULONG cbMacContext;
    ULONG cbAAD;
    ULONGLONG cbData;
    ULONG dwFlags;
} BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO, *PBCRYPT_AUTHENTICATED_CIPHER_MODE_INFO;

typedef struct _BCRYPT_ECCKEY_BLOB
{
    ULONG dwMagic;
    ULONG cbKey;
} BCRYPT_ECCKEY_BLOB, *PBCRYPT_ECCKEY_BLOB;

#define BCRYPT_RSAPUBLIC_MAGIC      0x31415352
#define BCRYPT_RSAPRIVATE_MAGIC     0x32415352
#define BCRYPT_RSAFULLPRIVATE_MAGIC 0x33415352

typedef struct _BCRYPT_RSAKEY_BLOB
{
    ULONG Magic;
    ULONG BitLength;
    ULONG cbPublicExp;
    ULONG cbModulus;
    ULONG cbPrime1;
    ULONG cbPrime2;
} BCRYPT_RSAKEY_BLOB;

typedef struct _BCRYPT_PKCS1_PADDING_INFO
{
    LPCWSTR pszAlgId;
} BCRYPT_PKCS1_PADDING_INFO;

#define BCRYPT_PAD_NONE                     0x00000001
#define BCRYPT_PAD_PKCS1                    0x00000002
#define BCRYPT_PAD_OAEP                     0x00000004
#define BCRYPT_PAD_PSS                      0x00000008
#define BCRYPT_PAD_PKCS1_OPTIONAL_HASH_OID  0x00000010

#define BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO_VERSION 1

#define BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG 0x00000001
#define BCRYPT_AUTH_MODE_IN_PROGRESS_FLAG 0x00000002

typedef struct _CRYPT_INTERFACE_REG
{
    ULONG dwInterface;
    ULONG dwFlags;
    ULONG cFunctions;
    PWSTR *rgpszFunctions;
} CRYPT_INTERFACE_REG, *PCRYPT_INTERFACE_REG;

typedef struct _CRYPT_IMAGE_REG
{
    PWSTR pszImage;
    ULONG cInterfaces;
    PCRYPT_INTERFACE_REG *rgpInterfaces;
} CRYPT_IMAGE_REG, *PCRYPT_IMAGE_REG;

typedef struct _CRYPT_PROVIDER_REG
{
    ULONG cAliases;
    PWSTR *rgpszAliases;
    PCRYPT_IMAGE_REG pUM;
    PCRYPT_IMAGE_REG pKM;
} CRYPT_PROVIDER_REG, *PCRYPT_PROVIDER_REG;

typedef struct _BCRYPT_KEY_DATA_BLOB_HEADER
{
    ULONG dwMagic;
    ULONG dwVersion;
    ULONG cbKeyData;
} BCRYPT_KEY_DATA_BLOB_HEADER, *PBCRYPT_KEY_DATA_BLOB_HEADER;

#define BCRYPT_KEY_DATA_BLOB_MAGIC    0x4d42444b
#define BCRYPT_KEY_DATA_BLOB_VERSION1 1

typedef PVOID BCRYPT_ALG_HANDLE;
typedef PVOID BCRYPT_KEY_HANDLE;
typedef PVOID BCRYPT_HANDLE;
typedef PVOID BCRYPT_HASH_HANDLE;

/* Flags for BCryptGenRandom */
#define BCRYPT_RNG_USE_ENTROPY_IN_BUFFER 0x00000001
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG  0x00000002

/* Flags for BCryptOpenAlgorithmProvider */
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008

/* Flags for BCryptEncrypt/BCryptDecrypt */
#define BCRYPT_BLOCK_PADDING        0x00000001

/* Flags for BCryptCreateHash */
#define BCRYPT_HASH_REUSABLE_FLAG   0x00000020

NTSTATUS WINAPI BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, ULONG);
NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDecrypt(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptDeriveKeyPBKDF2(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, ULONGLONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDestroyHash(BCRYPT_HASH_HANDLE);
NTSTATUS WINAPI BCryptDestroyKey(BCRYPT_KEY_HANDLE);
NTSTATUS WINAPI BCryptDuplicateHash(BCRYPT_HASH_HANDLE, BCRYPT_HASH_HANDLE *, UCHAR *, ULONG, ULONG);
NTSTATUS WINAPI BCryptEncrypt(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG, ULONG *, BCRYPT_ALGORITHM_IDENTIFIER **, ULONG);
NTSTATUS WINAPI BCryptFinalizeKeyPair(BCRYPT_KEY_HANDLE, ULONG);
NTSTATUS WINAPI BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenerateKeyPair(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenerateSymmetricKey(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *);
NTSTATUS WINAPI BCryptGetProperty(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptHash(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, PUCHAR, ULONG);
NTSTATUS WINAPI BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptImportKeyPair(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, UCHAR *, ULONG, ULONG);
NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);
NTSTATUS WINAPI BCryptSetProperty(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptVerifySignature(BCRYPT_KEY_HANDLE, void *, UCHAR *, ULONG, UCHAR *, ULONG, ULONG);

#endif  /* __WINE_BCRYPT_H */
