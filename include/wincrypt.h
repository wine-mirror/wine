/*
 * Copyright (C) 2002 Travis Michielsen
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINCRYPT_H
#define __WINE_WINCRYPT_H

#ifdef __cplusplus
extern "C" {
#endif

/* some typedefs for function parameters */
typedef unsigned int ALG_ID;
typedef unsigned long HCRYPTPROV;
typedef unsigned long HCRYPTKEY;
typedef unsigned long HCRYPTHASH;
typedef void *HCERTSTORE;

/* CSP Structs */

typedef struct _CRYPTOAPI_BLOB {
  DWORD    cbData;
  BYTE*    pbData;
} CRYPT_INTEGER_BLOB,  *PCRYPT_INTEGER_BLOB,
  CRYPT_UINT_BLOB,     *PCRYPT_UINT_BLOB,
  CRYPT_OBJID_BLOB,    *PCRYPT_OBJID_BLOB,
  CERT_NAME_BLOB,      *PCERT_NAME_BLOB,
  CERT_RDN_VALUE_BLOB, *PCERT_RDN_VALUE_BLOB,
  CERT_BLOB,           *PCERT_BLOB,
  CRL_BLOB,            *PCRL_BLOB,
  DATA_BLOB,           *PDATA_BLOB,
  CRYPT_DATA_BLOB,     *PCRYPT_DATA_BLOB,
  CRYPT_HASH_BLOB,     *PCRYPT_HASH_BLOB,
  CRYPT_DIGEST_BLOB,   *PCRYPT_DIGEST_BLOB,
  CRYPT_DER_BLOB,      *PCRYPT_DER_BLOB,
  CRYPT_ATTR_BLOB,     *PCRYPT_ATTR_BLOB;

typedef struct _CRYPTPROTECT_PROMPTSTRUCT{
  DWORD   cbSize;
  DWORD   dwPromptFlags;
  HWND    hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT, *PCRYPTPROTECT_PROMPTSTRUCT;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
  LPSTR            pszObjId;
  CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;

typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE {
  LPSTR               pszObjId;
  CRYPT_OBJID_BLOB    Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE, *PCRYPT_ATTRIBUTE_TYPE_VALUE;

typedef struct _PUBLICKEYSTRUC {
    BYTE   bType;
    BYTE   bVersion;
    WORD   reserved;
    ALG_ID aiKeyAlg;
} BLOBHEADER, PUBLICKEYSTRUC;

typedef struct _CRYPT_BIT_BLOB {
    DWORD cbData;
    BYTE  *pbData;
    DWORD cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;

typedef struct _CERT_PUBLIC_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER Algorithm;
    CRYPT_BIT_BLOB             PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;

typedef struct _CERT_EXTENSION {
    LPSTR               pszObjId;
    BOOL                fCritical;
    CRYPT_OBJID_BLOB    Value;
} CERT_EXTENSION, *PCERT_EXTENSION;

typedef struct _CERT_INFO {
    DWORD                      dwVersion;
    CRYPT_INTEGER_BLOB         SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB             Issuer;
    FILETIME                   NotBefore;
    FILETIME                   NotAfter;
    CERT_NAME_BLOB             Subject;
    CERT_PUBLIC_KEY_INFO       SubjectPublicKeyInfo;
    CRYPT_BIT_BLOB             IssuerUniqueId;
    CRYPT_BIT_BLOB             SubjectUniqueId;
    DWORD                      cExtension;
    PCERT_EXTENSION            rgExtension;
} CERT_INFO, *PCERT_INFO;

typedef struct _CERT_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE       *pbCertEncoded;
    DWORD      cbCertEncoded;
    PCERT_INFO pCertInfo;
    HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

typedef struct _CRL_ENTRY {
    CRYPT_INTEGER_BLOB SerialNumber;
    FILETIME           RevocationDate;
    DWORD              cExtension;
    PCERT_EXTENSION    rgExtension;
} CRL_ENTRY, *PCRL_ENTRY;

typedef struct _CRL_INFO {
    DWORD           dwVersion;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB  Issuer;
    FILETIME        ThisUpdate;
    FILETIME        NextUpdate;
    DWORD           cCRLEntry;
    PCRL_ENTRY      rgCRLEntry;
    DWORD           cExtension;
    PCERT_EXTENSION rgExtension;
} CRL_INFO, *PCRL_INFO;

typedef struct _CRL_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE      *pbCrlEncoded;
    DWORD      cbCrlEncoded;
    PCRL_INFO  pCrlInfo;
    HCERTSTORE hCertStore;
} CRL_CONTEXT, *PCRL_CONTEXT;
typedef const CRL_CONTEXT *PCCRL_CONTEXT;

typedef struct _VTableProvStruc {
    DWORD    Version;
    FARPROC  pFuncVerifyImage;
    FARPROC  pFuncReturnhWnd;
    DWORD    dwProvType;
    BYTE    *pbContextInfo;
    DWORD    cbContextInfo;
    LPSTR    pszProvName;
} VTableProvStruc, *PVTableProvStruc;

/* Algorithm IDs */

#define GET_ALG_CLASS(x)                (x & (7 << 13))
#define GET_ALG_TYPE(x)                 (x & (15 << 9))
#define GET_ALG_SID(x)                  (x & (511))

/* Algorithm Classes */
#define ALG_CLASS_ANY                   (0)
#define ALG_CLASS_SIGNATURE             (1 << 13)
#define ALG_CLASS_MSG_ENCRYPT           (2 << 13)
#define ALG_CLASS_DATA_ENCRYPT          (3 << 13)
#define ALG_CLASS_HASH                  (4 << 13)
#define ALG_CLASS_KEY_EXCHANGE          (5 << 13)
/* Algorithm types */
#define ALG_TYPE_ANY                    (0)
#define ALG_TYPE_DSS                    (1 << 9)
#define ALG_TYPE_RSA                    (2 << 9)
#define ALG_TYPE_BLOCK                  (3 << 9)
#define ALG_TYPE_STREAM                 (4 << 9)

/* SIDs */
#define ALG_SID_ANY                     (0)
/* RSA SIDs */
#define ALG_SID_RSA_ANY                 0
#define ALG_SID_RSA_PKCS                1
#define ALG_SID_RSA_MSATWORK            2
#define ALG_SID_RSA_ENTRUST             3
#define ALG_SID_RSA_PGP                 4
/* DSS SIDs */
#define ALG_SID_DSS_ANY                 0
#define ALG_SID_DSS_PKCS                1
#define ALG_SID_DSS_DMS                 2

/* DES SIDs */
#define ALG_SID_DES                     1
#define ALG_SID_3DES			3
#define ALG_SID_DESX			4
#define ALG_SID_IDEA			5
#define ALG_SID_CAST			6
#define ALG_SID_SAFERSK64		7
#define ALD_SID_SAFERSK128		8
/* RC2 SIDs */
#define ALG_SID_RC4                     1
#define ALG_SID_RC2                     2
#define ALG_SID_SEAL                    2
/* Hash SIDs */
#define ALG_SID_MD2                     1
#define ALG_SID_MD4                     2
#define ALG_SID_MD5                     3
#define ALG_SID_SHA                     4
#define ALG_SID_MAC                     5
#define ALG_SID_RIPEMD			6
#define ALG_SID_RIPEMD160		7
#define ALG_SID_SSL3SHAMD5		8

/* Algorithm Definitions */
#define CALG_MD2        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD2)
#define CALG_MD4        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD4)
#define CALG_MD5        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD5)
#define CALG_SHA        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_SHA)
#define CALG_MAC        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MAC)
#define CALG_RSA_SIGN   (ALG_CLASS_SIGNATURE    | ALG_TYPE_RSA    | ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN   (ALG_CLASS_SIGNATURE    | ALG_TYPE_DSS    | ALG_SID_DSS_ANY)
#define CALG_RSA_KEYX   (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA    | ALG_SID_RSA_ANY)
#define CALG_DES        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK  | ALG_SID_DES)
#define CALG_RC2        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK  | ALG_SID_RC2)
#define CALG_RC4        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_RC4)
#define CALG_SEAL       (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_SEAL)

/* Provider names */
#define MS_DEF_PROV_A                            "Microsoft Base Cryptographic Provider v1.0"
#if defined(__GNUC__)
# define MS_DEF_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 }
#elif defined(_MSC_VER)
# define MS_DEF_PROV_W      L"Microsoft Base Cryptographic Provider v1.0"
#else
static const WCHAR MS_DEF_PROV_W[] =             { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 };
#endif
#define MS_DEF_PROV                              WINELIB_NAME_AW(MS_DEF_PROV_)

#define MS_ENHANCED_PROV_A                       "Microsoft Enhanced Cryptographic Provider v1.0"
#if defined(__GNUC__)
# define MS_ENHANCED_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'E','n','h','a','n','c','e','d',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 }
#elif defined(_MSC_VER)
# define MS_ENHANCED_PROV_W     L"Microsoft Enhanced Cryptographic Provider v1.0"
#else
static const WCHAR MS_ENHANCED_PROV_W[] =        { 'M','i','c','r','o','s','o','f','t',' ',
	'E','n','h','a','n','c','e','d',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',' ','v','1','.','0',0 };
#endif
#define MS_ENHANCED_PROV                         WINELIB_NAME_AW(MS_ENHANCED_PROV_)

#define MS_STRONG_PROV_A                         "Microsoft Strong Cryptographic Provider"
#if defined(__GNUC__)
# define MS_STRONG_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'S','t','r','o','n','g',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ', \
	'P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_STRONG_PROV_W     L"Microsoft Strong Cryptographic Provider"
#else
static const WCHAR MS_STRONG_PROV_W[] =          { 'M','i','c','r','o','s','o','f','t',' ',
	'S','t','r','o','n','g',' ','C','r','y','p','t','o','g','r','a','p','h','i','c',' ',
	'P','r','o','v','i','d','e','r',0 };
#endif
#define MS_STRONG_PROV                           WINELIB_NAME_AW(MS_STRONG_PROV_)

#define MS_DEF_RSA_SIG_PROV_A                    "Microsoft RSA Signature Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_RSA_SIG_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'R','S','A',' ','S','i','g','n','a','t','u','r','e',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_RSA_SIG_PROV_W      L"Microsoft RSA Signature Cryptographic Provider"
#else
static const WCHAR MS_DEF_RSA_SIG_PROV_W[] =     { 'M','i','c','r','o','s','o','f','t',' ',
	'R','S','A',' ','S','i','g','n','a','t','u','r','e',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_RSA_SIG_PROV                      WINELIB_NAME_AW(MS_DEF_RSA_SIG_PROV_)

#define MS_DEF_RSA_SCHANNEL_PROV_A               "Microsoft RSA SChannel Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_RSA_SCHANNEL_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'R','S','A',' ','S','C','h','a','n','n','e','l',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_RSA_SCHANNEL_PROV_W     L"Microsoft RSA SChannel Cryptographic Provider"
#else
static const WCHAR MS_DEF_RSA_SCHANNEL_PROV_W[] = { 'M','i','c','r','o','s','o','f','t',' ',
	'R','S','A',' ','S','C','h','a','n','n','e','l',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_RSA_SCHANNEL_PROV                 WINELIB_NAME_AW(MS_DEF_RSA_SCHANNEL_PROV_)

#define MS_DEF_DSS_PROV_A                        "Microsoft Base DSS Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DSS_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','D','S','S',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DSS_PROV_W     L"Microsoft Base DSS Cryptographic Provider"
#else
static const WCHAR MS_DEF_DSS_PROV_W[] =         { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','D','S','S',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DSS_PROV                          WINELIB_NAME_AW(MS_DEF_DSS_PROV_)

#define MS_DEF_DSS_DH_PROV_A                     "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DSS_DH_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','D','S','S',' ','a','n','d',' ', \
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DSS_DH_PROV_W     L"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#else
static const WCHAR MS_DEF_DSS_DH_PROV_W[] =      { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','D','S','S',' ','a','n','d',' ',
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DSS_DH_PROV                       WINELIB_NAME_AW(MS_DEF_DSS_DH_PROV_)

#define MS_ENH_DSS_DH_PROV_A                     "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#if defined(__GNUC__)
# define MS_ENH_DSS_DH_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'E','n','h','a','n','c','e','d',' ','D','S','S',' ','a','n','d',' ', \
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_ENH_DSS_DH_PROV_W     L"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#else
static const WCHAR MS_ENH_DSS_DH_PROV_W[] =      { 'M','i','c','r','o','s','o','f','t',' ',
	'E','n','h','a','n','c','e','d',' ','D','S','S',' ','a','n','d',' ',
	'D','i','f','f','i','e','-','H','e','l','l','m','a','n',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_ENH_DSS_DH_PROV                       WINELIB_NAME_AW(MS_ENH_DSS_DH_PROV_)

#define MS_DEF_DH_SCHANNEL_PROV_A                "Microsoft DH SChannel Cryptographic Provider"
#if defined(__GNUC__)
# define MS_DEF_DH_SCHANNEL_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'D','H',' ','S','C','h','a','n','n','e','l',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_DEF_DH_SCHANNEL_PROV_W     L"Microsoft DH SChannel Cryptographic Provider"
#else
static const WCHAR MS_DEF_DH_SCHANNEL_PROV_W[] = { 'M','i','c','r','o','s','o','f','t',' ',
	'D','H',' ','S','C','h','a','n','n','e','l',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_DEF_DH_SCHANNEL_PROV                  WINELIB_NAME_AW(MS_DEF_DH_SCHANNEL_PROV_)

#define MS_SCARD_PROV_A                          "Microsoft Base Smart Card Cryptographic Provider"
#if defined(__GNUC__)
# define MS_SCARD_PROV_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',' ', \
	'B','a','s','e',' ','S','m','a','r','t',' ','C','a','r','d',' ', \
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
# define MS_SCARD_PROV_W     L"Microsoft Base Smart Card Cryptographic Provider"
#else
static const WCHAR MS_SCARD_PROV_W[] =           { 'M','i','c','r','o','s','o','f','t',' ',
	'B','a','s','e',' ','S','m','a','r','t',' ','C','a','r','d',' ',
	'C','r','y','p','t','o','g','r','a','p','h','i','c',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define MS_SCARD_PROV                            WINELIB_NAME_AW(MS_SCARD_PROV_)

/* Key Specs*/
#define AT_KEYEXCHANGE          1
#define AT_SIGNATURE            2

/* Provider Types */
#define PROV_RSA_FULL             1
#define PROV_RSA_SIG              2
#define PROV_DSS                  3
#define PROV_FORTEZZA             4
#define PROV_MS_EXCHANGE          5
#define PROV_SSL                  6
#define PROV_RSA_SCHANNEL         12
#define PROV_DSS_DH               13
#define PROV_EC_ECDSA_SIG         14
#define PROV_EC_ECNRA_SIG         15
#define PROV_EC_ECDSA_FULL        16
#define PROV_EC_ECNRA_FULL        17
#define PROV_DH_SCHANNEL          18
#define PROV_SPYRUS_LYNKS         20
#define PROV_RNG                  21
#define PROV_INTEL_SEC            22
#define PROV_REPLACE_OWF          23
#define PROV_RSA_AES              24

/* FLAGS Section */

/* Provider Parameters */
#define PP_ENUMALGS             1
#define PP_ENUMCONTAINERS       2
#define PP_IMPTYPE              3
#define PP_NAME                 4
#define PP_VERSION              5
#define PP_CONTAINER            6

#define CRYPT_FIRST             1
#define CRYPT_NEXT              2

#define CRYPT_IMPL_HARDWARE     1
#define CRYPT_IMPL_SOFTWARE     2
#define CRYPT_IMPL_MIXED        3
#define CRYPT_IMPL_UNKNOWN      4

/* CryptAcquireContext */
#define CRYPT_VERIFYCONTEXT       0xF0000000
#define CRYPT_NEWKEYSET           0x00000008
#define CRYPT_MACHINE_KEYSET      0x00000000
#define CRYPT_DELETEKEYSET        0x00000010
#define CRYPT_SILENT              0x00000000

/* Crypt{Get|Set}Provider */
#define CRYPT_MACHINE_DEFAULT     0x00000001
#define CRYPT_USER_DEFAULT        0x00000002
#define CRYPT_DELETE_DEFAULT      0x00000004

/* Crypt{Get/Set}ProvParam */
#define PP_CLIENT_HWND          1
#define PP_ENUMALGS             1
#define PP_ENUMCONTAINERS       2
#define PP_IMPTYPE              3
#define PP_NAME                 4
#define PP_VERSION              5
#define PP_CONTAINER            6
#define PP_CHANGE_PASSWORD      7
#define PP_KEYSET_SEC_DESCR     8
#define PP_KEY_TYPE_SUBTYPE     10
#define PP_CONTEXT_INFO         11
#define PP_KEYEXCHANGE_KEYSIZE  12
#define PP_SIGNATURE_KEYSIZE    13
#define PP_KEYEXCHANGE_ALG      14
#define PP_SIGNATURE_ALG        15
#define PP_PROVTYPE             16
#define PP_KEYSTORAGE           17
#define PP_SYM_KEYSIZE          19
#define PP_SESSION_KEYSIZE      20
#define PP_UI_PROMPT            21
#define PP_ENUMALGS_EX          22
#define PP_DELETEKEY            24
#define PP_ENUMMANDROOTS        25
#define PP_ENUMELECTROOTS       26
#define PP_KEYSET_TYPE          27
#define PP_ADMIN_PIN            31
#define PP_KEYEXCHANGE_PIN      32
#define PP_SIGNATURE_PIN        33
#define PP_SIG_KEYSIZE_INC      34
#define PP_KEYX_KEYSIZE_INC     35
#define PP_UNIQUE_CONTAINER     36
#define PP_SGC_INFO             37
#define PP_USE_HARDWARE_RNG     38
#define PP_KEYSPEC              39
#define PP_ENUMEX_SIGNING_PROT  40

/* CryptSignHash/CryptVerifySignature */
#define CRYPT_NOHASHOID         0x00000001
#define CRYPT_TYPE2_FORMAT      0x00000002
#define CRYPT_X931_FORMAT       0x00000004

/* Crypt*Key */
#define CRYPT_EXPORTABLE        0x00000001
#define CRYPT_USER_PROTECTED    0x00000002
#define CRYPT_CREATE_SALT       0x00000004
#define CRYPT_UPDATE_KEY        0x00000008
#define CRYPT_NO_SALT           0x00000010
#define CRYPT_PREGEN            0x00000040
#define CRYPT_ARCHIVABLE        0x00004000
#define CRYPT_SSL2_FALLBACK     0x00000002
#define CRYPT_DESTROYKEY        0x00000004
#define CRYPT_OAEP              0x00000040

/* Blob Types */
#define SIMPLEBLOB              0x1
#define PUBLICKEYBLOB           0x6
#define PRIVATEKEYBLOB          0x7
#define PLAINTEXTKEYBLOB        0x8
#define OPAQUEKEYBLOB           0x9
#define PUBLICKEYBLOBEX         0xA
#define SYMMETRICWRAPKEYBLOB    0xB

/* function declarations */
/* advapi32.dll */
BOOL WINAPI CryptAcquireContextA(HCRYPTPROV *phProv, LPCSTR pszContainer,
				   LPCSTR pszProvider, DWORD dwProvType,
				   DWORD dwFlags);
BOOL WINAPI CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
#define CryptAcquireContext WINELIB_NAME_AW(CryptAcquireContext)
BOOL WINAPI CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer);
BOOL WINAPI CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags);
BOOL WINAPI CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash);
BOOL WINAPI CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
BOOL WINAPI CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey);
BOOL WINAPI CryptDestroyHash (HCRYPTHASH hHash);
BOOL WINAPI CryptDestroyKey (HCRYPTKEY hKey);
BOOL WINAPI CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey);
BOOL WINAPI CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash);
BOOL WINAPI CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen);
BOOL WINAPI CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName);
BOOL WINAPI CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName);
#define CryptEnumProviders WINELIB_NAME_AW(CryptEnumProviders)
BOOL WINAPI CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName);
BOOL WINAPI CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName);
#define CryptEnumProviderTypes WINELIB_NAME_AW(CryptEnumProviderTypes)
BOOL WINAPI CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
BOOL WINAPI CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey);
BOOL WINAPI CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
BOOL WINAPI CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
BOOL WINAPI CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
BOOL WINAPI CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName);
BOOL WINAPI CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName);
#define CryptGetDefaultProvider WINELIB_NAME_AW(CryptGetDefaultProvider)
BOOL WINAPI CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey);
BOOL WINAPI CryptHashData (HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags);
BOOL WINAPI CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags);
BOOL WINAPI CryptImportKey (HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey);
BOOL WINAPI CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags);
BOOL WINAPI CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
BOOL WINAPI CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
#define CryptSignHash WINELIB_NAME_AW(CryptSignHash)
BOOL WINAPI CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
BOOL WINAPI CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
BOOL WINAPI CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType);
BOOL WINAPI CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType);
#define CryptSetProvider WINELIB_NAME_AW(CryptSetProvider)
BOOL WINAPI CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags);
BOOL WINAPI CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags);
#define CryptSetProviderEx WINELIB_NAME_AW(CryptSetProviderEx)
BOOL WINAPI CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
BOOL WINAPI CryptVerifySignatureA (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags);
BOOL WINAPI CryptVerifySignatureW (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags);
#define CryptVerifySignature WINELIB_NAME_AW(CryptVerifySignature)

#ifdef __cplusplus
}
#endif

#endif
