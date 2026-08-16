/*
 * xboxkrnl.exe - Xc* crypto: SHA-1 (inline FIPS 180-1), RC4 (inline),
 * HMAC-SHA1, DES/3DES via BCrypt, RSA stubs.
 *
 * Xbox SHA_CTX (92 bytes): we store BCRYPT_HASH_HANDLE in first 4 bytes.
 * Xbox DES key table: we store BCRYPT_KEY_HANDLE in first 4 bytes.
 * Callers allocate these structs; they are opaque to callers (not inspected).
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "bcrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

/* ======================================================================
 * Inline SHA-1 (FIPS 180-1, portable C)
 * ====================================================================== */
typedef struct {
    ULONG state[5];
    ULONG count[2];
    BYTE  buf[64];
} SHA1_CTX_INLINE;

#define ROL32(v,n) (((v)<<(n))|((v)>>(32-(n))))

static void sha1_transform(ULONG state[5], const BYTE *block)
{
    ULONG a, b, c, d, e, tmp, W[80];
    int i;
    for (i = 0; i < 16; i++)
        W[i] = ((ULONG)block[i*4]<<24)|((ULONG)block[i*4+1]<<16)|
               ((ULONG)block[i*4+2]<<8)|(ULONG)block[i*4+3];
    for (i = 16; i < 80; i++)
        W[i] = ROL32(W[i-3]^W[i-8]^W[i-14]^W[i-16],1);
    a=state[0]; b=state[1]; c=state[2]; d=state[3]; e=state[4];
#define SHA1_ROUND(f,k) tmp=ROL32(a,5)+(f)+e+(k)+W[i]; e=d; d=c; c=ROL32(b,30); b=a; a=tmp
    for (i=0;i<20;i++) SHA1_ROUND((b&c)|(~b&d), 0x5A827999u);
    for (i=20;i<40;i++) SHA1_ROUND(b^c^d, 0x6ED9EBA1u);
    for (i=40;i<60;i++) SHA1_ROUND((b&c)|(b&d)|(c&d), 0x8F1BBCDCu);
    for (i=60;i<80;i++) SHA1_ROUND(b^c^d, 0xCA62C1D6u);
#undef SHA1_ROUND
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d; state[4]+=e;
}

static void sha1_init(SHA1_CTX_INLINE *ctx)
{
    ctx->state[0]=0x67452301u; ctx->state[1]=0xEFCDAB89u;
    ctx->state[2]=0x98BADCFEu; ctx->state[3]=0x10325476u;
    ctx->state[4]=0xC3D2E1F0u;
    ctx->count[0]=ctx->count[1]=0;
}

static void sha1_update(SHA1_CTX_INLINE *ctx, const BYTE *data, ULONG len)
{
    ULONG j = (ctx->count[0]>>3) & 63;
    if ((ctx->count[0] += len<<3) < (len<<3)) ctx->count[1]++;
    ctx->count[1] += len>>29;
    if (j + len < 64) { memcpy(&ctx->buf[j], data, len); return; }
    memcpy(&ctx->buf[j], data, 64-j);
    sha1_transform(ctx->state, ctx->buf);
    data += 64-j; len -= 64-j;
    while (len >= 64) { sha1_transform(ctx->state, data); data+=64; len-=64; }
    memcpy(ctx->buf, data, len);
}

static void sha1_final(SHA1_CTX_INLINE *ctx, BYTE digest[20])
{
    BYTE pad[64] = {0}; ULONG i, n;
    pad[0]=0x80;
    n = ((ctx->count[0]>>3) & 63);
    if (n < 56) { sha1_update(ctx, pad, 56-n); }
    else { sha1_update(ctx, pad, 120-n); }
    pad[0]=(BYTE)(ctx->count[1]>>24); pad[1]=(BYTE)(ctx->count[1]>>16);
    pad[2]=(BYTE)(ctx->count[1]>>8);  pad[3]=(BYTE)(ctx->count[1]);
    pad[4]=(BYTE)(ctx->count[0]>>24); pad[5]=(BYTE)(ctx->count[0]>>16);
    pad[6]=(BYTE)(ctx->count[0]>>8);  pad[7]=(BYTE)(ctx->count[0]);
    sha1_update(ctx, pad, 8);
    for (i=0;i<5;i++) {
        digest[i*4]=(BYTE)(ctx->state[i]>>24); digest[i*4+1]=(BYTE)(ctx->state[i]>>16);
        digest[i*4+2]=(BYTE)(ctx->state[i]>>8); digest[i*4+3]=(BYTE)(ctx->state[i]);
    }
    SecureZeroMemory(ctx, sizeof(*ctx));
}

void WINAPI XBOXKRNL_XcSHAInit(SHA1_CTX_INLINE *ctx)
{
    sha1_init(ctx);
}

void WINAPI XBOXKRNL_XcSHAUpdate(SHA1_CTX_INLINE *ctx, void *data, ULONG len)
{
    sha1_update(ctx, (const BYTE *)data, len);
}

void WINAPI XBOXKRNL_XcSHAFinal(SHA1_CTX_INLINE *ctx, BYTE *digest)
{
    sha1_final(ctx, digest);
}

/* ======================================================================
 * Inline RC4
 * ====================================================================== */
typedef struct { BYTE S[256]; BYTE i, j; } RC4_CTX;

void WINAPI XBOXKRNL_XcRC4Key(RC4_CTX *ctx, ULONG len, const BYTE *key)
{
    ULONG i, j=0;
    for (i=0;i<256;i++) ctx->S[i]=(BYTE)i;
    for (i=0;i<256;i++) {
        j=(j+ctx->S[i]+key[i%len])&255;
        BYTE t=ctx->S[i]; ctx->S[i]=ctx->S[j]; ctx->S[j]=t;
    }
    ctx->i=ctx->j=0;
}

void WINAPI XBOXKRNL_XcRC4Crypt(RC4_CTX *ctx, ULONG len, BYTE *data)
{
    BYTE i=ctx->i, j=ctx->j, t;
    while (len--) {
        i=(i+1)&255; j=(j+ctx->S[i])&255;
        t=ctx->S[i]; ctx->S[i]=ctx->S[j]; ctx->S[j]=t;
        *data++ ^= ctx->S[(ctx->S[i]+ctx->S[j])&255];
    }
    ctx->i=i; ctx->j=j;
}

/* ======================================================================
 * HMAC-SHA1
 * XcHMAC(Key,KeyLen,Data,DataLen,Data2,Data2Len,OutDigest[20])
 * ====================================================================== */
void WINAPI XBOXKRNL_XcHMAC(const BYTE *Key, ULONG KeyLen,
                              const BYTE *Data, ULONG DataLen,
                              const BYTE *Data2, ULONG Data2Len,
                              BYTE *Digest)
{
    BYTE kpad[64], ipad[64], opad[64], inner[20];
    SHA1_CTX_INLINE ctx;
    ULONG i;

    memset(kpad, 0, 64);
    if (KeyLen > 64) { sha1_init(&ctx); sha1_update(&ctx,(const BYTE*)Key,KeyLen); sha1_final(&ctx,kpad); }
    else memcpy(kpad, Key, KeyLen);

    for (i=0;i<64;i++) { ipad[i]=kpad[i]^0x36; opad[i]=kpad[i]^0x5c; }
    sha1_init(&ctx);
    sha1_update(&ctx, ipad, 64);
    if (Data && DataLen)   sha1_update(&ctx, (const BYTE*)Data,  DataLen);
    if (Data2 && Data2Len) sha1_update(&ctx, (const BYTE*)Data2, Data2Len);
    sha1_final(&ctx, inner);

    sha1_init(&ctx);
    sha1_update(&ctx, opad, 64);
    sha1_update(&ctx, inner, 20);
    sha1_final(&ctx, Digest);
}

/* ======================================================================
 * DES/3DES via BCrypt
 *
 * Xbox XcKeyTable stores the key schedule in a caller-provided buffer.
 * We store a BCRYPT_KEY_HANDLE in the first 4 bytes of that buffer.
 * ====================================================================== */
static BCRYPT_ALG_HANDLE bh_des   = NULL;
static BCRYPT_ALG_HANDLE bh_3des  = NULL;
static LONG bh_once = 0;

static void bcrypt_init(void)
{
    if (InterlockedCompareExchange(&bh_once, 1, 0)) return;
    BCryptOpenAlgorithmProvider(&bh_des,  BCRYPT_DES_ALGORITHM,  NULL, 0);
    BCryptOpenAlgorithmProvider(&bh_3des, BCRYPT_3DES_ALGORITHM, NULL, 0);
}

static inline BCRYPT_ALG_HANDLE alg_for_type(ULONG type)
{
    return (type == 0) ? bh_des : bh_3des;
}

/* XcDESKeyParity: set odd parity bit (bit 0) on each DES key byte */
void WINAPI XBOXKRNL_XcDESKeyParity(BYTE *Key, ULONG len)
{
    ULONG i, cnt;
    BYTE b, v;
    for (i = 0; i < len; i++) {
        b = Key[i] & 0xFE;
        cnt = 0; v = b >> 1;
        while (v) { cnt += v & 1; v >>= 1; }
        Key[i] = b | (cnt & 1 ? 0 : 1);
    }
}

/* XcKeyTable: import DES/3DES key, store BCRYPT_KEY_HANDLE at table[0] */
void WINAPI XBOXKRNL_XcKeyTable(ULONG Type, void *KeyTable, void *Key)
{
    BCRYPT_KEY_HANDLE *hkey = (BCRYPT_KEY_HANDLE *)KeyTable;
    ULONG keylen = (Type == 0) ? 8 : 24;
    bcrypt_init();
    if (*hkey) { BCryptDestroyKey(*hkey); *hkey = NULL; }
    BCryptGenerateSymmetricKey(alg_for_type(Type), hkey, NULL, 0,
                               (BYTE *)Key, keylen, 0);
}

/* XcBlockCrypt: ECB single block, Enc=1 encrypt, Enc=0 decrypt */
void WINAPI XBOXKRNL_XcBlockCrypt(ULONG Type, void *Out, void *In,
                                   void *KeyTable, ULONG Enc)
{
    BCRYPT_KEY_HANDLE hkey = *(BCRYPT_KEY_HANDLE *)KeyTable;
    ULONG res;
    if (!hkey) return;
    if (Enc)
        BCryptEncrypt(hkey, (BYTE *)In, 8, NULL, NULL, 0, (BYTE *)Out, 8, &res, 0);
    else
        BCryptDecrypt(hkey, (BYTE *)In, 8, NULL, NULL, 0, (BYTE *)Out, 8, &res, 0);
}

/* XcBlockCryptCBC: CBC mode, Feedback is IV (modified in-place) */
void WINAPI XBOXKRNL_XcBlockCryptCBC(ULONG Type, ULONG Len, void *Out,
                                      void *In, void *KeyTable, ULONG Enc,
                                      void *Feedback)
{
    BCRYPT_KEY_HANDLE hkey = *(BCRYPT_KEY_HANDLE *)KeyTable;
    ULONG res;
    if (!hkey) return;
    if (Enc)
        BCryptEncrypt(hkey, (BYTE *)In, Len, NULL,
                      (BYTE *)Feedback, 8, (BYTE *)Out, Len, &res, 0);
    else
        BCryptDecrypt(hkey, (BYTE *)In, Len, NULL,
                      (BYTE *)Feedback, 8, (BYTE *)Out, Len, &res, 0);
}

/* ======================================================================
 * RSA / big-number stubs (complex; return success with zero output)
 * ====================================================================== */
LONG WINAPI XBOXKRNL_XcModExp(void *Result, void *Base, void *Exp,
                               void *Modulus, ULONG Len)
{
    FIXME("XcModExp: stub\n");
    if (Result) memset(Result, 0, Len);
    return 0;
}

LONG WINAPI XBOXKRNL_XcPKEncPublic(void *Key, void *Input, void *Output)
{
    FIXME("XcPKEncPublic: stub\n");
    return 0;
}

LONG WINAPI XBOXKRNL_XcPKDecPrivate(void *Key, void *Input, void *Output)
{
    FIXME("XcPKDecPrivate: stub\n");
    return 0;
}

LONG WINAPI XBOXKRNL_XcPKGetKeyLen(void *Key)
{
    return 0;
}

LONG WINAPI XBOXKRNL_XcVerifyPKCS1Signature(void *Hash, void *PublicKey, void *Signature)
{
    FIXME("XcVerifyPKCS1Signature: returning success\n");
    return 1; /* TRUE = verified */
}

/* ======================================================================
 * XcCryptService - dispatcher; most services implemented above
 * ====================================================================== */
LONG WINAPI XBOXKRNL_XcCryptService(ULONG ServiceCode, void *ServiceData)
{
    FIXME("XcCryptService(%lu): stub\n", (ULONG)ServiceCode);
    return 0;
}

/* XcUpdateCrypto - update internal crypto state (e.g. after EEPROM update) */
void WINAPI XBOXKRNL_XcUpdateCrypto(void *CryptoData, void *UpdateFlags) { }
