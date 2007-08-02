/*
 * Copyright 2007 Juan Lang
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
#include "wincrypt.h"
#include "snmp.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

/* Called when a message's ref count reaches zero.  Free any message-specific
 * data here.
 */
typedef void (*CryptMsgCloseFunc)(HCRYPTMSG msg);

typedef BOOL (*CryptMsgGetParamFunc)(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData);

typedef BOOL (*CryptMsgUpdateFunc)(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal);

typedef enum _CryptMsgState {
    MsgStateInit,
    MsgStateUpdated,
    MsgStateFinalized
} CryptMsgState;

typedef struct _CryptMsgBase
{
    LONG                 ref;
    DWORD                open_flags;
    BOOL                 streamed;
    CMSG_STREAM_INFO     stream_info;
    CryptMsgState        state;
    CryptMsgCloseFunc    close;
    CryptMsgUpdateFunc   update;
    CryptMsgGetParamFunc get_param;
} CryptMsgBase;

static inline void CryptMsgBase_Init(CryptMsgBase *msg, DWORD dwFlags,
 PCMSG_STREAM_INFO pStreamInfo, CryptMsgCloseFunc close,
 CryptMsgGetParamFunc get_param, CryptMsgUpdateFunc update)
{
    msg->ref = 1;
    msg->open_flags = dwFlags;
    if (pStreamInfo)
    {
        msg->streamed = TRUE;
        memcpy(&msg->stream_info, pStreamInfo, sizeof(msg->stream_info));
    }
    else
    {
        msg->streamed = FALSE;
        memset(&msg->stream_info, 0, sizeof(msg->stream_info));
    }
    msg->close = close;
    msg->get_param = get_param;
    msg->update = update;
    msg->state = MsgStateInit;
}

typedef struct _CDataEncodeMsg
{
    CryptMsgBase base;
    DWORD        bare_content_len;
    LPBYTE       bare_content;
} CDataEncodeMsg;

static const BYTE empty_data_content[] = { 0x04,0x00 };

static void CDataEncodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CDataEncodeMsg *msg = (CDataEncodeMsg *)hCryptMsg;

    if (msg->bare_content != empty_data_content)
        LocalFree(msg->bare_content);
}

static WINAPI BOOL CRYPT_EncodeContentLength(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CDataEncodeMsg *msg = (const CDataEncodeMsg *)pvStructInfo;
    DWORD lenBytes;
    BOOL ret = TRUE;

    /* Trick:  report bytes needed based on total message length, even though
     * the message isn't available yet.  The caller will use the length
     * reported here to encode its length.
     */
    CRYPT_EncodeLen(msg->base.stream_info.cbContent, NULL, &lenBytes);
    if (!pbEncoded)
        *pcbEncoded = 1 + lenBytes + msg->base.stream_info.cbContent;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
         pcbEncoded, 1 + lenBytes)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_OCTETSTRING;
            CRYPT_EncodeLen(msg->base.stream_info.cbContent, pbEncoded,
             &lenBytes);
        }
    }
    return ret;
}

static BOOL CRYPT_EncodeDataContentInfoHeader(CDataEncodeMsg *msg,
 CRYPT_DATA_BLOB *header)
{
    BOOL ret;

    if (msg->base.streamed && msg->base.stream_info.cbContent == 0xffffffff)
    {
        FIXME("unimplemented for indefinite-length encoding\n");
        header->cbData = 0;
        header->pbData = NULL;
        ret = TRUE;
    }
    else
    {
        struct AsnConstructedItem constructed = { 0, msg,
         CRYPT_EncodeContentLength };
        struct AsnEncodeSequenceItem items[2] = {
         { szOID_RSA_data, CRYPT_AsnEncodeOid, 0 },
         { &constructed,   CRYPT_AsnEncodeConstructed, 0 },
        };

        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items,
         sizeof(items) / sizeof(items[0]), CRYPT_ENCODE_ALLOC_FLAG, NULL,
         (LPBYTE)&header->pbData, &header->cbData);
        if (ret)
        {
            /* Trick:  subtract the content length from the reported length,
             * as the actual content hasn't come yet.
             */
            header->cbData -= msg->base.stream_info.cbContent;
        }
    }
    return ret;
}

static BOOL CDataEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CDataEncodeMsg *msg = (CDataEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    if (msg->base.streamed)
    {
        __TRY
        {
            if (msg->base.state != MsgStateUpdated)
            {
                CRYPT_DATA_BLOB header;

                ret = CRYPT_EncodeDataContentInfoHeader(msg, &header);
                if (ret)
                {
                    ret = msg->base.stream_info.pfnStreamOutput(
                     msg->base.stream_info.pvArg, header.pbData, header.cbData,
                     FALSE);
                    LocalFree(header.pbData);
                }
            }
            if (!fFinal)
                ret = msg->base.stream_info.pfnStreamOutput(
                 msg->base.stream_info.pvArg, (BYTE *)pbData, cbData,
                 FALSE);
            else
            {
                if (msg->base.stream_info.cbContent == 0xffffffff)
                {
                    BYTE indefinite_trailer[6] = { 0 };

                    ret = msg->base.stream_info.pfnStreamOutput(
                     msg->base.stream_info.pvArg, (BYTE *)pbData, cbData,
                     FALSE);
                    if (ret)
                        ret = msg->base.stream_info.pfnStreamOutput(
                         msg->base.stream_info.pvArg, indefinite_trailer,
                         sizeof(indefinite_trailer), TRUE);
                }
                else
                    ret = msg->base.stream_info.pfnStreamOutput(
                     msg->base.stream_info.pvArg, (BYTE *)pbData, cbData, TRUE);
            }
        }
        __EXCEPT_PAGE_FAULT
        {
            SetLastError(STATUS_ACCESS_VIOLATION);
        }
        __ENDTRY;
    }
    else
    {
        if (!fFinal)
        {
            if (msg->base.open_flags & CMSG_DETACHED_FLAG)
                SetLastError(E_INVALIDARG);
            else
                SetLastError(CRYPT_E_MSG_ERROR);
        }
        else
        {
            if (!cbData)
                SetLastError(E_INVALIDARG);
            else
            {
                CRYPT_DATA_BLOB blob = { cbData, (LPBYTE)pbData };

                /* non-streamed data messages don't allow non-final updates,
                 * don't bother checking whether data already exist, they can't.
                 */
                ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
                 &blob, CRYPT_ENCODE_ALLOC_FLAG, NULL, &msg->bare_content,
                 &msg->bare_content_len);
            }
        }
    }
    return ret;
}

static BOOL CRYPT_CopyParam(void *pvData, DWORD *pcbData, const void *src,
 DWORD len)
{
    BOOL ret = TRUE;

    if (!pvData)
        *pcbData = len;
    else if (*pcbData < len)
    {
        *pcbData = len;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pcbData = len;
        memcpy(pvData, src, len);
    }
    return ret;
}

static BOOL CDataEncodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CDataEncodeMsg *msg = (CDataEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_CONTENT_PARAM:
        if (msg->base.streamed)
            SetLastError(E_INVALIDARG);
        else
        {
            CRYPT_CONTENT_INFO info;
            char rsa_data[] = "1.2.840.113549.1.7.1";

            info.pszObjId = rsa_data;
            info.Content.cbData = msg->bare_content_len;
            info.Content.pbData = msg->bare_content;
            ret = CryptEncodeObject(X509_ASN_ENCODING, PKCS_CONTENT_INFO, &info,
             pvData, pcbData);
        }
        break;
    case CMSG_BARE_CONTENT_PARAM:
        if (msg->base.streamed)
            SetLastError(E_INVALIDARG);
        else
            ret = CRYPT_CopyParam(pvData, pcbData, msg->bare_content,
             msg->bare_content_len);
        break;
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static HCRYPTMSG CDataEncodeMsg_Open(DWORD dwFlags, const void *pvMsgEncodeInfo,
 LPSTR pszInnerContentObjID, PCMSG_STREAM_INFO pStreamInfo)
{
    CDataEncodeMsg *msg;

    if (pvMsgEncodeInfo)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    msg = CryptMemAlloc(sizeof(CDataEncodeMsg));
    if (msg)
    {
        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CDataEncodeMsg_Close, CDataEncodeMsg_GetParam, CDataEncodeMsg_Update);
        msg->bare_content_len = sizeof(empty_data_content);
        msg->bare_content = (LPBYTE)empty_data_content;
    }
    return (HCRYPTMSG)msg;
}

typedef struct _CHashEncodeMsg
{
    CryptMsgBase    base;
    HCRYPTPROV      prov;
    HCRYPTHASH      hash;
    CRYPT_DATA_BLOB data;
} CHashEncodeMsg;

static void CHashEncodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CHashEncodeMsg *msg = (CHashEncodeMsg *)hCryptMsg;

    CryptMemFree(msg->data.pbData);
    CryptDestroyHash(msg->hash);
    if (msg->base.open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        CryptReleaseContext(msg->prov, 0);
}

static BOOL CRYPT_EncodePKCSDigestedData(CHashEncodeMsg *msg, void *pvData,
 DWORD *pcbData)
{
    BOOL ret;
    ALG_ID algID;
    DWORD size = sizeof(algID);

    ret = CryptGetHashParam(msg->hash, HP_ALGID, (BYTE *)&algID, &size, 0);
    if (ret)
    {
        CRYPT_DIGESTED_DATA digestedData = { 0 };
        char oid_rsa_data[] = szOID_RSA_data;

        digestedData.version = CMSG_HASHED_DATA_PKCS_1_5_VERSION;
        digestedData.DigestAlgorithm.pszObjId = (LPSTR)CertAlgIdToOID(algID);
        /* FIXME: what about digestedData.DigestAlgorithm.Parameters? */
        /* Quirk:  OID is only encoded messages if an update has happened */
        if (msg->base.state != MsgStateInit)
            digestedData.ContentInfo.pszObjId = oid_rsa_data;
        if (!(msg->base.open_flags & CMSG_DETACHED_FLAG) && msg->data.cbData)
        {
            ret = CRYPT_AsnEncodeOctets(0, NULL, &msg->data,
             CRYPT_ENCODE_ALLOC_FLAG, NULL,
             (LPBYTE)&digestedData.ContentInfo.Content.pbData,
             &digestedData.ContentInfo.Content.cbData);
        }
        if (msg->base.state == MsgStateFinalized)
        {
            size = sizeof(DWORD);
            ret = CryptGetHashParam(msg->hash, HP_HASHSIZE,
             (LPBYTE)&digestedData.hash.cbData, &size, 0);
            if (ret)
            {
                digestedData.hash.pbData = CryptMemAlloc(
                 digestedData.hash.cbData);
                ret = CryptGetHashParam(msg->hash, HP_HASHVAL,
                 digestedData.hash.pbData, &digestedData.hash.cbData, 0);
            }
        }
        if (ret)
            ret = CRYPT_AsnEncodePKCSDigestedData(&digestedData, pvData,
             pcbData);
        CryptMemFree(digestedData.hash.pbData);
        LocalFree(digestedData.ContentInfo.Content.pbData);
    }
    return ret;
}

static BOOL CHashEncodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CHashEncodeMsg *msg = (CHashEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %d, %d, %p, %p)\n", hCryptMsg, dwParamType, dwIndex,
     pvData, pcbData);

    switch (dwParamType)
    {
    case CMSG_BARE_CONTENT_PARAM:
        if (msg->base.streamed)
            SetLastError(E_INVALIDARG);
        else
            ret = CRYPT_EncodePKCSDigestedData(msg, pvData, pcbData);
        break;
    case CMSG_CONTENT_PARAM:
    {
        CRYPT_CONTENT_INFO info;

        ret = CryptMsgGetParam(hCryptMsg, CMSG_BARE_CONTENT_PARAM, 0, NULL,
         &info.Content.cbData);
        if (ret)
        {
            info.Content.pbData = CryptMemAlloc(info.Content.cbData);
            if (info.Content.pbData)
            {
                ret = CryptMsgGetParam(hCryptMsg, CMSG_BARE_CONTENT_PARAM, 0,
                 info.Content.pbData, &info.Content.cbData);
                if (ret)
                {
                    char oid_rsa_hashed[] = szOID_RSA_hashedData;

                    info.pszObjId = oid_rsa_hashed;
                    ret = CryptEncodeObjectEx(X509_ASN_ENCODING,
                     PKCS_CONTENT_INFO, &info, 0, NULL, pvData, pcbData);
                }
                CryptMemFree(info.Content.pbData);
            }
            else
                ret = FALSE;
        }
        break;
    }
    case CMSG_COMPUTED_HASH_PARAM:
        ret = CryptGetHashParam(msg->hash, HP_HASHVAL, (BYTE *)pvData, pcbData,
         0);
        break;
    case CMSG_VERSION_PARAM:
        if (msg->base.state != MsgStateFinalized)
            SetLastError(CRYPT_E_MSG_ERROR);
        else
        {
            DWORD version = CMSG_HASHED_DATA_PKCS_1_5_VERSION;

            /* Since the data are always encoded as octets, the version is
             * always 0 (see rfc3852, section 7)
             */
            ret = CRYPT_CopyParam(pvData, pcbData, &version, sizeof(version));
        }
        break;
    default:
        ret = FALSE;
    }
    return ret;
}

static BOOL CHashEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CHashEncodeMsg *msg = (CHashEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %d, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    if (msg->base.streamed || (msg->base.open_flags & CMSG_DETACHED_FLAG))
    {
        /* Doesn't do much, as stream output is never called, and you
         * can't get the content.
         */
        ret = CryptHashData(msg->hash, pbData, cbData, 0);
    }
    else
    {
        if (!fFinal)
            SetLastError(CRYPT_E_MSG_ERROR);
        else
        {
            ret = CryptHashData(msg->hash, pbData, cbData, 0);
            if (ret)
            {
                msg->data.pbData = CryptMemAlloc(cbData);
                if (msg->data.pbData)
                {
                    memcpy(msg->data.pbData + msg->data.cbData, pbData, cbData);
                    msg->data.cbData += cbData;
                }
                else
                    ret = FALSE;
            }
        }
    }
    return ret;
}

static HCRYPTMSG CHashEncodeMsg_Open(DWORD dwFlags, const void *pvMsgEncodeInfo,
 LPSTR pszInnerContentObjID, PCMSG_STREAM_INFO pStreamInfo)
{
    CHashEncodeMsg *msg;
    const CMSG_HASHED_ENCODE_INFO *info =
     (const CMSG_HASHED_ENCODE_INFO *)pvMsgEncodeInfo;
    HCRYPTPROV prov;
    ALG_ID algID;

    if (info->cbSize != sizeof(CMSG_HASHED_ENCODE_INFO))
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (!(algID = CertOIDToAlgId(info->HashAlgorithm.pszObjId)))
    {
        SetLastError(CRYPT_E_UNKNOWN_ALGO);
        return NULL;
    }
    if (info->hCryptProv)
        prov = info->hCryptProv;
    else
    {
        prov = CRYPT_GetDefaultProvider();
        dwFlags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
    }
    msg = CryptMemAlloc(sizeof(CHashEncodeMsg));
    if (msg)
    {
        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CHashEncodeMsg_Close, CHashEncodeMsg_GetParam, CHashEncodeMsg_Update);
        msg->prov = prov;
        msg->data.cbData = 0;
        msg->data.pbData = NULL;
        if (!CryptCreateHash(prov, algID, 0, 0, &msg->hash))
        {
            CryptMsgClose(msg);
            msg = NULL;
        }
    }
    return (HCRYPTMSG)msg;
}

typedef struct _CMSG_SIGNER_ENCODE_INFO_WITH_CMS
{
    DWORD                      cbSize;
    PCERT_INFO                 pCertInfo;
    HCRYPTPROV                 hCryptProv;
    DWORD                      dwKeySpec;
    CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
    void                      *pvHashAuxInfo;
    DWORD                      cAuthAttr;
    PCRYPT_ATTRIBUTE           rgAuthAttr;
    DWORD                      cUnauthAttr;
    PCRYPT_ATTRIBUTE           rgUnauthAttr;
    CERT_ID                    SignerId;
    CRYPT_ALGORITHM_IDENTIFIER HashEncryptionAlgorithm;
    void                      *pvHashEncryptionAuxInfo;
} CMSG_SIGNER_ENCODE_INFO_WITH_CMS, *PCMSG_SIGNER_ENCODE_INFO_WITH_CMS;

typedef struct _CMSG_SIGNED_ENCODE_INFO_WITH_CMS
{
    DWORD                             cbSize;
    DWORD                             cSigners;
    PCMSG_SIGNER_ENCODE_INFO_WITH_CMS rgSigners;
    DWORD                             cCertEncoded;
    PCERT_BLOB                        rgCertEncoded;
    DWORD                             cCrlEncoded;
    PCRL_BLOB                         rgCrlEncoded;
    DWORD                             cAttrCertEncoded;
    PCERT_BLOB                        rgAttrCertEncoded;
} CMSG_SIGNED_ENCODE_INFO_WITH_CMS, *PCMSG_SIGNED_ENCODE_INFO_WITH_CMS;

static BOOL CRYPT_IsValidSigner(CMSG_SIGNER_ENCODE_INFO_WITH_CMS *signer)
{
    if (signer->cbSize != sizeof(CMSG_SIGNER_ENCODE_INFO) &&
     signer->cbSize != sizeof(CMSG_SIGNER_ENCODE_INFO_WITH_CMS))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (signer->cbSize == sizeof(CMSG_SIGNER_ENCODE_INFO_WITH_CMS))
    {
        FIXME("CMSG_SIGNER_ENCODE_INFO with CMS fields unsupported\n");
        return FALSE;
    }
    if (!signer->pCertInfo->SerialNumber.cbData)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!signer->pCertInfo->Issuer.cbData)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!signer->hCryptProv)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!CertOIDToAlgId(signer->HashAlgorithm.pszObjId))
    {
        SetLastError(CRYPT_E_UNKNOWN_ALGO);
        return FALSE;
    }
    return TRUE;
}

typedef struct _CSignerHandles
{
    HCRYPTPROV       prov;
    HCRYPTHASH       contentHash;
    HCRYPTHASH       authAttrHash;
    HCRYPTKEY        key;
} CSignerHandles;

static BOOL CRYPT_ConstructBlob(CRYPT_DATA_BLOB *out, const CRYPT_DATA_BLOB *in)
{
    BOOL ret = TRUE;

    out->cbData = in->cbData;
    if (out->cbData)
    {
        out->pbData = CryptMemAlloc(out->cbData);
        if (out->pbData)
            memcpy(out->pbData, in->pbData, out->cbData);
        else
            ret = FALSE;
    }
    else
        out->pbData = NULL;
    return ret;
}

typedef struct _BlobArray
{
    DWORD            cBlobs;
    PCRYPT_DATA_BLOB blobs;
} BlobArray;

static BOOL CRYPT_ConstructBlobArray(BlobArray *out, const BlobArray *in)
{
    BOOL ret = TRUE;

    out->cBlobs = in->cBlobs;
    if (out->cBlobs)
    {
        out->blobs = CryptMemAlloc(out->cBlobs * sizeof(CRYPT_DATA_BLOB));
        if (out->blobs)
        {
            DWORD i;

            memset(out->blobs, 0, out->cBlobs * sizeof(CRYPT_DATA_BLOB));
            for (i = 0; ret && i < out->cBlobs; i++)
                ret = CRYPT_ConstructBlob(&out->blobs[i], &in->blobs[i]);
        }
        else
            ret = FALSE;
    }
    return ret;
}

static void CRYPT_FreeBlobArray(BlobArray *array)
{
    DWORD i;

    for (i = 0; i < array->cBlobs; i++)
        CryptMemFree(array->blobs[i].pbData);
    CryptMemFree(array->blobs);
}

static BOOL CRYPT_ConstructAttribute(CRYPT_ATTRIBUTE *out,
 const CRYPT_ATTRIBUTE *in)
{
    BOOL ret;

    out->pszObjId = CryptMemAlloc(strlen(in->pszObjId) + 1);
    if (out->pszObjId)
    {
        strcpy(out->pszObjId, in->pszObjId);
        ret = CRYPT_ConstructBlobArray((BlobArray *)&out->cValue,
         (const BlobArray *)&in->cValue);
    }
    else
        ret = FALSE;
    return ret;
}

static BOOL CRYPT_ConstructAttributes(CRYPT_ATTRIBUTES *out,
 const CRYPT_ATTRIBUTES *in)
{
    BOOL ret = TRUE;

    out->cAttr = in->cAttr;
    if (out->cAttr)
    {
        out->rgAttr = CryptMemAlloc(out->cAttr * sizeof(CRYPT_ATTRIBUTE));
        if (out->rgAttr)
        {
            DWORD i;

            memset(out->rgAttr, 0, out->cAttr * sizeof(CRYPT_ATTRIBUTE));
            for (i = 0; ret && i < out->cAttr; i++)
                ret = CRYPT_ConstructAttribute(&out->rgAttr[i], &in->rgAttr[i]);
        }
        else
            ret = FALSE;
    }
    else
        out->rgAttr = NULL;
    return ret;
}

/* Constructs both a CSignerHandles and a CMSG_SIGNER_INFO from a
 * CMSG_SIGNER_ENCODE_INFO_WITH_CMS.
 */
static BOOL CSignerInfo_Construct(CSignerHandles *handles,
 CMSG_SIGNER_INFO *info, CMSG_SIGNER_ENCODE_INFO_WITH_CMS *in, DWORD open_flags)
{
    ALG_ID algID;
    BOOL ret;

    handles->prov = in->hCryptProv;
    if (!(open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG))
        CryptContextAddRef(handles->prov, NULL, 0);
    algID = CertOIDToAlgId(in->HashAlgorithm.pszObjId);
    ret = CryptCreateHash(handles->prov, algID, 0, 0, &handles->contentHash);
    if (ret && in->cAuthAttr)
        ret = CryptCreateHash(handles->prov, algID, 0, 0,
         &handles->authAttrHash);
    if (ret)
    {
        /* Note: needs to change if CMS fields are supported */
        info->dwVersion = CMSG_SIGNER_INFO_V1;
        ret = CRYPT_ConstructBlob(&info->Issuer, &in->pCertInfo->Issuer);
        if (ret)
            ret = CRYPT_ConstructBlob(&info->SerialNumber,
             &in->pCertInfo->SerialNumber);
        /* Assumption:  algorithm IDs will point to static strings, not
         * stack-based ones, so copying the pointer values is safe.
         */
        info->HashAlgorithm.pszObjId = in->HashAlgorithm.pszObjId;
        if (ret)
            ret = CRYPT_ConstructBlob(&info->HashAlgorithm.Parameters,
             &in->HashAlgorithm.Parameters);
        memset(&info->HashEncryptionAlgorithm, 0,
         sizeof(info->HashEncryptionAlgorithm));
        if (ret)
            ret = CRYPT_ConstructAttributes(&info->AuthAttrs,
             (CRYPT_ATTRIBUTES *)&in->cAuthAttr);
        if (ret)
            ret = CRYPT_ConstructAttributes(&info->UnauthAttrs,
             (CRYPT_ATTRIBUTES *)&in->cUnauthAttr);
    }
    return ret;
}

static void CSignerInfo_Free(CMSG_SIGNER_INFO *info)
{
    DWORD i, j;

    CryptMemFree(info->Issuer.pbData);
    CryptMemFree(info->SerialNumber.pbData);
    CryptMemFree(info->HashAlgorithm.Parameters.pbData);
    CryptMemFree(info->EncryptedHash.pbData);
    for (i = 0; i < info->AuthAttrs.cAttr; i++)
    {
        for (j = 0; j < info->AuthAttrs.rgAttr[i].cValue; j++)
            CryptMemFree(info->AuthAttrs.rgAttr[i].rgValue[j].pbData);
        CryptMemFree(info->AuthAttrs.rgAttr[i].rgValue);
        CryptMemFree(info->AuthAttrs.rgAttr[i].pszObjId);
    }
    CryptMemFree(info->AuthAttrs.rgAttr);
    for (i = 0; i < info->UnauthAttrs.cAttr; i++)
    {
        for (j = 0; j < info->UnauthAttrs.rgAttr[i].cValue; j++)
            CryptMemFree(info->UnauthAttrs.rgAttr[i].rgValue[j].pbData);
        CryptMemFree(info->UnauthAttrs.rgAttr[i].rgValue);
        CryptMemFree(info->UnauthAttrs.rgAttr[i].pszObjId);
    }
    CryptMemFree(info->UnauthAttrs.rgAttr);
}

typedef struct _CSignedEncodeMsg
{
    CryptMsgBase      base;
    CRYPT_DATA_BLOB   data;
    CRYPT_SIGNED_INFO info;
    CSignerHandles   *signerHandles;
} CSignedEncodeMsg;

static void CSignedEncodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CSignedEncodeMsg *msg = (CSignedEncodeMsg *)hCryptMsg;
    DWORD i;

    CryptMemFree(msg->data.pbData);
    CRYPT_FreeBlobArray((BlobArray *)&msg->info.cCertEncoded);
    CRYPT_FreeBlobArray((BlobArray *)&msg->info.cCrlEncoded);
    for (i = 0; i < msg->info.cSignerInfo; i++)
    {
        CSignerInfo_Free(&msg->info.rgSignerInfo[i]);
        CryptDestroyKey(msg->signerHandles[i].key);
        CryptDestroyHash(msg->signerHandles[i].contentHash);
        CryptDestroyHash(msg->signerHandles[i].authAttrHash);
        CryptReleaseContext(msg->signerHandles[i].prov, 0);
    }
    CryptMemFree(msg->signerHandles);
    CryptMemFree(msg->info.rgSignerInfo);
}

static BOOL CSignedEncodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CSignedEncodeMsg *msg = (CSignedEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_CONTENT_PARAM:
    {
        CRYPT_CONTENT_INFO info;

        ret = CryptMsgGetParam(hCryptMsg, CMSG_BARE_CONTENT_PARAM, 0, NULL,
         &info.Content.cbData);
        if (ret)
        {
            info.Content.pbData = CryptMemAlloc(info.Content.cbData);
            if (info.Content.pbData)
            {
                ret = CryptMsgGetParam(hCryptMsg, CMSG_BARE_CONTENT_PARAM, 0,
                 info.Content.pbData, &info.Content.cbData);
                if (ret)
                {
                    char oid_rsa_signed[] = szOID_RSA_signedData;

                    info.pszObjId = oid_rsa_signed;
                    ret = CryptEncodeObjectEx(X509_ASN_ENCODING,
                     PKCS_CONTENT_INFO, &info, 0, NULL, pvData, pcbData);
                }
                CryptMemFree(info.Content.pbData);
            }
            else
                ret = FALSE;
        }
        break;
    }
    case CMSG_BARE_CONTENT_PARAM:
    {
        CRYPT_SIGNED_INFO info;
        char oid_rsa_data[] = szOID_RSA_data;

        memcpy(&info, &msg->info, sizeof(info));
        /* Quirk:  OID is only encoded messages if an update has happened */
        if (msg->base.state != MsgStateInit)
            info.content.pszObjId = oid_rsa_data;
        else
            info.content.pszObjId = NULL;
        if (msg->data.cbData)
        {
            CRYPT_DATA_BLOB blob = { msg->data.cbData, msg->data.pbData };

            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
             &blob, CRYPT_ENCODE_ALLOC_FLAG, NULL,
             &info.content.Content.pbData, &info.content.Content.cbData);
        }
        else
        {
            info.content.Content.cbData = 0;
            info.content.Content.pbData = NULL;
            ret = TRUE;
        }
        if (ret)
        {
            ret = CRYPT_AsnEncodePKCSSignedInfo(&info, pvData, pcbData);
            LocalFree(info.content.Content.pbData);
        }
        break;
    }
    case CMSG_COMPUTED_HASH_PARAM:
        if (dwIndex >= msg->info.cSignerInfo)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            ret = CryptGetHashParam(msg->signerHandles[dwIndex].contentHash,
             HP_HASHVAL, pvData, pcbData, 0);
        break;
    case CMSG_ENCODED_SIGNER:
        if (dwIndex >= msg->info.cSignerInfo)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
             PKCS7_SIGNER_INFO, &msg->info.rgSignerInfo[dwIndex], 0, NULL,
             pvData, pcbData);
        break;
    case CMSG_VERSION_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, &msg->info.version,
         sizeof(msg->info.version));
        break;
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CSignedEncodeMsg_UpdateHash(CSignedEncodeMsg *msg,
 const BYTE *pbData, DWORD cbData)
{
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%p, %p, %d)\n", msg, pbData, cbData);

    for (i = 0; ret && i < msg->info.cSignerInfo; i++)
        ret = CryptHashData(msg->signerHandles[i].contentHash, pbData, cbData,
         0);
    return ret;
}

static BOOL CRYPT_AppendAttribute(CRYPT_ATTRIBUTES *out,
 const CRYPT_ATTRIBUTE *in)
{
    BOOL ret = FALSE;

    out->rgAttr = CryptMemRealloc(out->rgAttr,
     (out->cAttr + 1) * sizeof(CRYPT_ATTRIBUTE));
    if (out->rgAttr)
    {
        ret = CRYPT_ConstructAttribute(&out->rgAttr[out->cAttr], in);
        if (ret)
            out->cAttr++;
    }
    return ret;
}

static BOOL CSignedEncodeMsg_AppendMessageDigestAttribute(CSignedEncodeMsg *msg,
 DWORD signerIndex)
{
    BOOL ret;
    DWORD size;
    CRYPT_HASH_BLOB hash = { 0, NULL }, encodedHash = { 0, NULL };
    char messageDigest[] = szOID_RSA_messageDigest;
    CRYPT_ATTRIBUTE messageDigestAttr = { messageDigest, 1, &encodedHash };

    size = sizeof(DWORD);
    ret = CryptGetHashParam(msg->signerHandles[signerIndex].contentHash,
     HP_HASHSIZE, (LPBYTE)&hash.cbData, &size, 0);
    if (ret)
    {
        hash.pbData = CryptMemAlloc(hash.cbData);
        ret = CryptGetHashParam(msg->signerHandles[signerIndex].contentHash,
         HP_HASHVAL, hash.pbData, &hash.cbData, 0);
        if (ret)
        {
            ret = CRYPT_AsnEncodeOctets(0, NULL, &hash, CRYPT_ENCODE_ALLOC_FLAG,
             NULL, (LPBYTE)&encodedHash.pbData, &encodedHash.cbData);
            if (ret)
            {
                ret = CRYPT_AppendAttribute(
                 &msg->info.rgSignerInfo[signerIndex].AuthAttrs,
                 &messageDigestAttr);
                LocalFree(encodedHash.pbData);
            }
        }
        CryptMemFree(hash.pbData);
    }
    return ret;
}

static BOOL CSignedEncodeMsg_UpdateAuthenticatedAttributes(
 CSignedEncodeMsg *msg)
{
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%p)\n", msg);

    for (i = 0; ret && i < msg->info.cSignerInfo; i++)
    {
        if (msg->info.rgSignerInfo[i].AuthAttrs.cAttr)
        {
            BYTE oid_rsa_data_encoded[] = { 0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,
             0x0d,0x01,0x07,0x01 };
            CRYPT_DATA_BLOB content = { sizeof(oid_rsa_data_encoded),
             oid_rsa_data_encoded };
            char contentType[] = szOID_RSA_contentType;
            CRYPT_ATTRIBUTE contentTypeAttr = { contentType, 1, &content };

            /* FIXME: does this depend on inner OID? */
            ret = CRYPT_AppendAttribute(&msg->info.rgSignerInfo[i].AuthAttrs,
             &contentTypeAttr);
            if (ret)
                ret = CSignedEncodeMsg_AppendMessageDigestAttribute(msg, i);
            if (ret)
            {
                LPBYTE encodedAttrs;
                DWORD size;

                ret = CryptEncodeObjectEx(X509_ASN_ENCODING, PKCS_ATTRIBUTES,
                 &msg->info.rgSignerInfo[i].AuthAttrs, CRYPT_ENCODE_ALLOC_FLAG,
                 NULL, (LPBYTE)&encodedAttrs, &size);
                if (ret)
                {
                    ret = CryptHashData(msg->signerHandles[i].authAttrHash,
                     encodedAttrs, size, 0);
                    LocalFree(encodedAttrs);
                }
            }
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static void CRYPT_ReverseBytes(CRYPT_HASH_BLOB *hash)
{
    DWORD i;
    BYTE tmp;

    for (i = 0; i < hash->cbData / 2; i++)
    {
        tmp = hash->pbData[hash->cbData - i - 1];
        hash->pbData[hash->cbData - i - 1] = hash->pbData[i];
        hash->pbData[i] = tmp;
    }
}

static BOOL CSignedEncodeMsg_Sign(CSignedEncodeMsg *msg)
{
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%p)\n", msg);

    for (i = 0; ret && i < msg->info.cSignerInfo; i++)
    {
        HCRYPTHASH hash;

        if (msg->info.rgSignerInfo[i].AuthAttrs.cAttr)
            hash = msg->signerHandles[i].authAttrHash;
        else
            hash = msg->signerHandles[i].contentHash;
        ret = CryptSignHashW(hash, AT_SIGNATURE, NULL, 0, NULL,
         &msg->info.rgSignerInfo[i].EncryptedHash.cbData);
        if (ret)
        {
            msg->info.rgSignerInfo[i].EncryptedHash.pbData =
             CryptMemAlloc(msg->info.rgSignerInfo[i].EncryptedHash.cbData);
            if (msg->info.rgSignerInfo[i].EncryptedHash.pbData)
            {
                ret = CryptSignHashW(hash, AT_SIGNATURE, NULL, 0,
                 msg->info.rgSignerInfo[i].EncryptedHash.pbData,
                 &msg->info.rgSignerInfo[i].EncryptedHash.cbData);
                if (ret)
                    CRYPT_ReverseBytes(&msg->info.rgSignerInfo[i].EncryptedHash);
            }
            else
                ret = FALSE;
        }
    }
    return ret;
}

static BOOL CSignedEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CSignedEncodeMsg *msg = (CSignedEncodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    if (msg->base.streamed || (msg->base.open_flags & CMSG_DETACHED_FLAG))
    {
        ret = CSignedEncodeMsg_UpdateHash(msg, pbData, cbData);
        if (ret && fFinal)
        {
            ret = CSignedEncodeMsg_UpdateAuthenticatedAttributes(msg);
            if (ret)
                ret = CSignedEncodeMsg_Sign(msg);
        }
        if (msg->base.streamed)
            FIXME("streamed partial stub\n");
    }
    else
    {
        if (!fFinal)
            SetLastError(CRYPT_E_MSG_ERROR);
        else
        {
            if (cbData)
            {
                msg->data.pbData = CryptMemAlloc(cbData);
                if (msg->data.pbData)
                {
                    memcpy(msg->data.pbData, pbData, cbData);
                    msg->data.cbData = cbData;
                    ret = TRUE;
                }
            }
            else
                ret = TRUE;
            if (ret)
                ret = CSignedEncodeMsg_UpdateHash(msg, pbData, cbData);
            if (ret)
                ret = CSignedEncodeMsg_UpdateAuthenticatedAttributes(msg);
            if (ret)
                ret = CSignedEncodeMsg_Sign(msg);
        }
    }
    return ret;
}

static HCRYPTMSG CSignedEncodeMsg_Open(DWORD dwFlags,
 const void *pvMsgEncodeInfo, LPSTR pszInnerContentObjID,
 PCMSG_STREAM_INFO pStreamInfo)
{
    const CMSG_SIGNED_ENCODE_INFO_WITH_CMS *info =
     (const CMSG_SIGNED_ENCODE_INFO_WITH_CMS *)pvMsgEncodeInfo;
    DWORD i;
    CSignedEncodeMsg *msg;

    if (info->cbSize != sizeof(CMSG_SIGNED_ENCODE_INFO) &&
     info->cbSize != sizeof(CMSG_SIGNED_ENCODE_INFO_WITH_CMS))
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (info->cbSize == sizeof(CMSG_SIGNED_ENCODE_INFO_WITH_CMS))
    {
        FIXME("CMSG_SIGNED_ENCODE_INFO with CMS fields unsupported\n");
        return NULL;
    }
    for (i = 0; i < info->cSigners; i++)
        if (!CRYPT_IsValidSigner(&info->rgSigners[i]))
            return NULL;
    msg = CryptMemAlloc(sizeof(CSignedEncodeMsg));
    if (msg)
    {
        BOOL ret = TRUE;

        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CSignedEncodeMsg_Close, CSignedEncodeMsg_GetParam,
         CSignedEncodeMsg_Update);
        msg->data.cbData = 0;
        msg->data.pbData = NULL;
        memset(&msg->info, 0, sizeof(msg->info));
        msg->info.version = CMSG_SIGNED_DATA_V1;
        if (info->cSigners)
        {
            msg->signerHandles =
             CryptMemAlloc(info->cSigners * sizeof(CSignerHandles));
            if (msg->signerHandles)
                msg->info.rgSignerInfo =
                 CryptMemAlloc(info->cSigners * sizeof(CMSG_SIGNER_INFO));
            else
            {
                ret = FALSE;
                msg->info.rgSignerInfo = NULL;
            }
            if (msg->info.rgSignerInfo)
            {
                msg->info.cSignerInfo = info->cSigners;
                memset(msg->signerHandles, 0,
                 msg->info.cSignerInfo * sizeof(CSignerHandles));
                memset(msg->info.rgSignerInfo, 0,
                 msg->info.cSignerInfo * sizeof(CMSG_SIGNER_INFO));
                for (i = 0; ret && i < msg->info.cSignerInfo; i++)
                    ret = CSignerInfo_Construct(&msg->signerHandles[i],
                     &msg->info.rgSignerInfo[i], &info->rgSigners[i], dwFlags);
            }
            else
                ret = FALSE;
        }
        if (ret)
            ret = CRYPT_ConstructBlobArray((BlobArray *)&msg->info.cCertEncoded,
             (const BlobArray *)&info->cCertEncoded);
        if (ret)
            ret = CRYPT_ConstructBlobArray((BlobArray *)&msg->info.cCrlEncoded,
             (const BlobArray *)&info->cCrlEncoded);
        if (!ret)
        {
            CSignedEncodeMsg_Close(msg);
            msg = NULL;
        }
    }
    return msg;
}

static inline const char *MSG_TYPE_STR(DWORD type)
{
    switch (type)
    {
#define _x(x) case (x): return #x
        _x(CMSG_DATA);
        _x(CMSG_SIGNED);
        _x(CMSG_ENVELOPED);
        _x(CMSG_SIGNED_AND_ENVELOPED);
        _x(CMSG_HASHED);
        _x(CMSG_ENCRYPTED);
#undef _x
        default:
            return wine_dbg_sprintf("unknown (%d)", type);
    }
}

HCRYPTMSG WINAPI CryptMsgOpenToEncode(DWORD dwMsgEncodingType, DWORD dwFlags,
 DWORD dwMsgType, const void *pvMsgEncodeInfo, LPSTR pszInnerContentObjID,
 PCMSG_STREAM_INFO pStreamInfo)
{
    HCRYPTMSG msg = NULL;

    TRACE("(%08x, %08x, %08x, %p, %s, %p)\n", dwMsgEncodingType, dwFlags,
     dwMsgType, pvMsgEncodeInfo, debugstr_a(pszInnerContentObjID), pStreamInfo);

    if (GET_CMSG_ENCODING_TYPE(dwMsgEncodingType) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    switch (dwMsgType)
    {
    case CMSG_DATA:
        msg = CDataEncodeMsg_Open(dwFlags, pvMsgEncodeInfo,
         pszInnerContentObjID, pStreamInfo);
        break;
    case CMSG_HASHED:
        msg = CHashEncodeMsg_Open(dwFlags, pvMsgEncodeInfo,
         pszInnerContentObjID, pStreamInfo);
        break;
    case CMSG_SIGNED:
        msg = CSignedEncodeMsg_Open(dwFlags, pvMsgEncodeInfo,
         pszInnerContentObjID, pStreamInfo);
        break;
    case CMSG_ENVELOPED:
        FIXME("unimplemented for type %s\n", MSG_TYPE_STR(dwMsgType));
        break;
    case CMSG_SIGNED_AND_ENVELOPED:
    case CMSG_ENCRYPTED:
        /* defined but invalid, fall through */
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return msg;
}

typedef struct _CDecodeMsg
{
    CryptMsgBase           base;
    DWORD                  type;
    HCRYPTPROV             crypt_prov;
    union {
        HCRYPTHASH             hash;
        CRYPT_SIGNED_INFO     *signedInfo;
    } u;
    CRYPT_DATA_BLOB        msg_data;
    PCONTEXT_PROPERTY_LIST properties;
} CDecodeMsg;

static void CDecodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CDecodeMsg *msg = (CDecodeMsg *)hCryptMsg;

    if (msg->base.open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        CryptReleaseContext(msg->crypt_prov, 0);
    switch (msg->type)
    {
    case CMSG_HASHED:
        if (msg->u.hash)
            CryptDestroyHash(msg->u.hash);
        break;
    case CMSG_SIGNED:
        LocalFree(msg->u.signedInfo);
        break;
    }
    CryptMemFree(msg->msg_data.pbData);
    ContextPropertyList_Free(msg->properties);
}

static BOOL CDecodeMsg_CopyData(CDecodeMsg *msg, const BYTE *pbData,
 DWORD cbData)
{
    BOOL ret = TRUE;

    if (cbData)
    {
        if (msg->msg_data.cbData)
            msg->msg_data.pbData = CryptMemRealloc(msg->msg_data.pbData,
             msg->msg_data.cbData + cbData);
        else
            msg->msg_data.pbData = CryptMemAlloc(cbData);
        if (msg->msg_data.pbData)
        {
            memcpy(msg->msg_data.pbData + msg->msg_data.cbData, pbData, cbData);
            msg->msg_data.cbData += cbData;
        }
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL CDecodeMsg_DecodeDataContent(CDecodeMsg *msg, CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_DATA_BLOB *data;
    DWORD size;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
     blob->pbData, blob->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, (LPBYTE)&data,
     &size);
    if (ret)
    {
        ret = ContextPropertyList_SetProperty(msg->properties,
         CMSG_CONTENT_PARAM, data->pbData, data->cbData);
        LocalFree(data);
    }
    return ret;
}

static void CDecodeMsg_SaveAlgorithmID(CDecodeMsg *msg, DWORD param,
 const CRYPT_ALGORITHM_IDENTIFIER *id)
{
    static const BYTE nullParams[] = { ASN_NULL, 0 };
    CRYPT_ALGORITHM_IDENTIFIER *copy;
    DWORD len = sizeof(CRYPT_ALGORITHM_IDENTIFIER);

    /* Linearize algorithm id */
    len += strlen(id->pszObjId) + 1;
    len += id->Parameters.cbData;
    copy = CryptMemAlloc(len);
    if (copy)
    {
        copy->pszObjId =
         (LPSTR)((BYTE *)copy + sizeof(CRYPT_ALGORITHM_IDENTIFIER));
        strcpy(copy->pszObjId, id->pszObjId);
        copy->Parameters.pbData = (BYTE *)copy->pszObjId + strlen(id->pszObjId)
         + 1;
        /* Trick:  omit NULL parameters */
        if (id->Parameters.cbData == sizeof(nullParams) &&
         !memcmp(id->Parameters.pbData, nullParams, sizeof(nullParams)))
        {
            copy->Parameters.cbData = 0;
            len -= sizeof(nullParams);
        }
        else
            copy->Parameters.cbData = id->Parameters.cbData;
        if (copy->Parameters.cbData)
            memcpy(copy->Parameters.pbData, id->Parameters.pbData,
             id->Parameters.cbData);
        ContextPropertyList_SetProperty(msg->properties, param, (BYTE *)copy,
         len);
        CryptMemFree(copy);
    }
}

static inline void CRYPT_FixUpAlgorithmID(CRYPT_ALGORITHM_IDENTIFIER *id)
{
    id->pszObjId = (LPSTR)((BYTE *)id + sizeof(CRYPT_ALGORITHM_IDENTIFIER));
    id->Parameters.pbData = (BYTE *)id->pszObjId + strlen(id->pszObjId) + 1;
}

static BOOL CDecodeMsg_DecodeHashedContent(CDecodeMsg *msg,
 CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_DIGESTED_DATA *digestedData;
    DWORD size;

    ret = CRYPT_AsnDecodePKCSDigestedData(blob->pbData, blob->cbData,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (CRYPT_DIGESTED_DATA *)&digestedData,
     &size);
    if (ret)
    {
        ContextPropertyList_SetProperty(msg->properties, CMSG_VERSION_PARAM,
         (const BYTE *)&digestedData->version, sizeof(digestedData->version));
        CDecodeMsg_SaveAlgorithmID(msg, CMSG_HASH_ALGORITHM_PARAM,
         &digestedData->DigestAlgorithm);
        ContextPropertyList_SetProperty(msg->properties,
         CMSG_INNER_CONTENT_TYPE_PARAM,
         (const BYTE *)digestedData->ContentInfo.pszObjId,
         digestedData->ContentInfo.pszObjId ?
         strlen(digestedData->ContentInfo.pszObjId) + 1 : 0);
        if (digestedData->ContentInfo.Content.cbData)
            CDecodeMsg_DecodeDataContent(msg,
             &digestedData->ContentInfo.Content);
        else
            ContextPropertyList_SetProperty(msg->properties,
             CMSG_CONTENT_PARAM, NULL, 0);
        ContextPropertyList_SetProperty(msg->properties, CMSG_HASH_DATA_PARAM,
         digestedData->hash.pbData, digestedData->hash.cbData);
        LocalFree(digestedData);
    }
    return ret;
}

static BOOL CDecodeMsg_DecodeSignedContent(CDecodeMsg *msg,
 CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_SIGNED_INFO *signedInfo;
    DWORD size;

    ret = CRYPT_AsnDecodePKCSSignedInfo(blob->pbData, blob->cbData,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (CRYPT_SIGNED_INFO *)&signedInfo,
     &size);
    if (ret)
        msg->u.signedInfo = signedInfo;
    return ret;
}
/* Decodes the content in blob as the type given, and updates the value
 * (type, parameters, etc.) of msg based on what blob contains.
 * It doesn't just use msg's type, to allow a recursive call from an implicitly
 * typed message once the outer content info has been decoded.
 */
static BOOL CDecodeMsg_DecodeContent(CDecodeMsg *msg, CRYPT_DER_BLOB *blob,
 DWORD type)
{
    BOOL ret;

    switch (type)
    {
    case CMSG_DATA:
        if ((ret = CDecodeMsg_DecodeDataContent(msg, blob)))
            msg->type = CMSG_DATA;
        break;
    case CMSG_HASHED:
        if ((ret = CDecodeMsg_DecodeHashedContent(msg, blob)))
            msg->type = CMSG_HASHED;
        break;
    case CMSG_ENVELOPED:
        FIXME("unimplemented for type %s\n", MSG_TYPE_STR(type));
        ret = TRUE;
        break;
    case CMSG_SIGNED:
        if ((ret = CDecodeMsg_DecodeSignedContent(msg, blob)))
            msg->type = CMSG_SIGNED;
        break;
    default:
    {
        CRYPT_CONTENT_INFO *info;
        DWORD size;

        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, PKCS_CONTENT_INFO,
         msg->msg_data.pbData, msg->msg_data.cbData, CRYPT_DECODE_ALLOC_FLAG,
         NULL, (LPBYTE)&info, &size);
        if (ret)
        {
            if (!strcmp(info->pszObjId, szOID_RSA_data))
                ret = CDecodeMsg_DecodeContent(msg, &info->Content, CMSG_DATA);
            else if (!strcmp(info->pszObjId, szOID_RSA_digestedData))
                ret = CDecodeMsg_DecodeContent(msg, &info->Content,
                 CMSG_HASHED);
            else if (!strcmp(info->pszObjId, szOID_RSA_envelopedData))
                ret = CDecodeMsg_DecodeContent(msg, &info->Content,
                 CMSG_ENVELOPED);
            else if (!strcmp(info->pszObjId, szOID_RSA_signedData))
                ret = CDecodeMsg_DecodeContent(msg, &info->Content,
                 CMSG_SIGNED);
            else
            {
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
                ret = FALSE;
            }
            LocalFree(info);
        }
    }
    }
    return ret;
}

static BOOL CDecodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CDecodeMsg *msg = (CDecodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %d, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    if (msg->base.streamed)
    {
        ret = CDecodeMsg_CopyData(msg, pbData, cbData);
        FIXME("(%p, %p, %d, %d): streamed update stub\n", hCryptMsg, pbData,
         cbData, fFinal);
    }
    else
    {
        if (!fFinal)
            SetLastError(CRYPT_E_MSG_ERROR);
        else
        {
            ret = CDecodeMsg_CopyData(msg, pbData, cbData);
            if (ret)
                ret = CDecodeMsg_DecodeContent(msg, &msg->msg_data, msg->type);

        }
    }
    return ret;
}

static BOOL CDecodeHashMsg_GetParam(CDecodeMsg *msg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_TYPE_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, &msg->type, sizeof(msg->type));
        break;
    case CMSG_HASH_ALGORITHM_PARAM:
    {
        CRYPT_DATA_BLOB blob;

        ret = ContextPropertyList_FindProperty(msg->properties, dwParamType,
         &blob);
        if (ret)
        {
            ret = CRYPT_CopyParam(pvData, pcbData, blob.pbData, blob.cbData);
            if (ret && pvData)
                CRYPT_FixUpAlgorithmID((CRYPT_ALGORITHM_IDENTIFIER *)pvData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    }
    case CMSG_COMPUTED_HASH_PARAM:
        if (!msg->u.hash)
        {
            CRYPT_ALGORITHM_IDENTIFIER *hashAlgoID = NULL;
            DWORD size = 0;
            ALG_ID algID = 0;

            CryptMsgGetParam(msg, CMSG_HASH_ALGORITHM_PARAM, 0, NULL, &size);
            hashAlgoID = CryptMemAlloc(size);
            ret = CryptMsgGetParam(msg, CMSG_HASH_ALGORITHM_PARAM, 0,
             hashAlgoID, &size);
            if (ret)
                algID = CertOIDToAlgId(hashAlgoID->pszObjId);
            ret = CryptCreateHash(msg->crypt_prov, algID, 0, 0, &msg->u.hash);
            if (ret)
            {
                CRYPT_DATA_BLOB content;

                ret = ContextPropertyList_FindProperty(msg->properties,
                 CMSG_CONTENT_PARAM, &content);
                if (ret)
                    ret = CryptHashData(msg->u.hash, content.pbData,
                     content.cbData, 0);
            }
            CryptMemFree(hashAlgoID);
        }
        else
            ret = TRUE;
        if (ret)
            ret = CryptGetHashParam(msg->u.hash, HP_HASHVAL, pvData, pcbData,
             0);
        break;
    default:
    {
        CRYPT_DATA_BLOB blob;

        ret = ContextPropertyList_FindProperty(msg->properties, dwParamType,
         &blob);
        if (ret)
            ret = CRYPT_CopyParam(pvData, pcbData, blob.pbData, blob.cbData);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    }
    return ret;
}

/* nextData is an in/out parameter - on input it's the memory location in
 * which a copy of in's data should be made, and on output it's the memory
 * location immediately after out's copy of in's data.
 */
static inline void CRYPT_CopyBlob(CRYPT_DATA_BLOB *out,
 const CRYPT_DATA_BLOB *in, LPBYTE *nextData)
{
    out->cbData = in->cbData;
    if (in->cbData)
    {
        out->pbData = *nextData;
        memcpy(out->pbData, in->pbData, in->cbData);
        *nextData += in->cbData;
    }
}

static inline void CRYPT_CopyAlgorithmId(CRYPT_ALGORITHM_IDENTIFIER *out,
 const CRYPT_ALGORITHM_IDENTIFIER *in, LPBYTE *nextData)
{
    if (in->pszObjId)
    {
        out->pszObjId = (LPSTR)*nextData;
        strcpy(out->pszObjId, in->pszObjId);
        *nextData += strlen(out->pszObjId) + 1;
    }
    CRYPT_CopyBlob(&out->Parameters, &in->Parameters, nextData);
}

static inline void CRYPT_CopyAttributes(CRYPT_ATTRIBUTES *out,
 const CRYPT_ATTRIBUTES *in, LPBYTE *nextData)
{
    out->cAttr = in->cAttr;
    if (in->cAttr)
    {
        DWORD i;

        if ((*nextData - (LPBYTE)0) % sizeof(DWORD))
            *nextData += (*nextData - (LPBYTE)0) % sizeof(DWORD);
        out->rgAttr = (CRYPT_ATTRIBUTE *)*nextData;
        *nextData += in->cAttr * sizeof(CRYPT_ATTRIBUTE);
        for (i = 0; i < in->cAttr; i++)
        {
            if (in->rgAttr[i].pszObjId)
            {
                out->rgAttr[i].pszObjId = (LPSTR)*nextData;
                strcpy(out->rgAttr[i].pszObjId, in->rgAttr[i].pszObjId);
                *nextData += strlen(in->rgAttr[i].pszObjId) + 1;
            }
            if (in->rgAttr[i].cValue)
            {
                DWORD j;

                out->rgAttr[i].cValue = in->rgAttr[i].cValue;
                if ((*nextData - (LPBYTE)0) % sizeof(DWORD))
                    *nextData += (*nextData - (LPBYTE)0) % sizeof(DWORD);
                out->rgAttr[i].rgValue = (PCRYPT_DATA_BLOB)*nextData;
                for (j = 0; j < in->rgAttr[i].cValue; j++)
                    CRYPT_CopyBlob(&out->rgAttr[i].rgValue[j],
                     &in->rgAttr[i].rgValue[j], nextData);
            }
        }
    }
}

static DWORD CRYPT_SizeOfAttributes(const CRYPT_ATTRIBUTES *attr)
{
    DWORD size = attr->cAttr * sizeof(CRYPT_ATTRIBUTE), i, j;

    for (i = 0; i < attr->cAttr; i++)
    {
        if (attr->rgAttr[i].pszObjId)
            size += strlen(attr->rgAttr[i].pszObjId) + 1;
        /* align pointer */
        if (size % sizeof(DWORD))
            size += size % sizeof(DWORD);
        size += attr->rgAttr[i].cValue * sizeof(CRYPT_DATA_BLOB);
        for (j = 0; j < attr->rgAttr[i].cValue; j++)
            size += attr->rgAttr[i].rgValue[j].cbData;
    }
    return size;
}

static BOOL CRYPT_CopySignerInfo(void *pvData, DWORD *pcbData,
 const CMSG_SIGNER_INFO *in)
{
    DWORD size = sizeof(CMSG_SIGNER_INFO);
    BOOL ret;

    size += in->Issuer.cbData;
    size += in->SerialNumber.cbData;
    if (in->HashAlgorithm.pszObjId)
        size += strlen(in->HashAlgorithm.pszObjId) + 1;
    size += in->HashAlgorithm.Parameters.cbData;
    if (in->HashEncryptionAlgorithm.pszObjId)
        size += strlen(in->HashEncryptionAlgorithm.pszObjId) + 1;
    size += in->HashEncryptionAlgorithm.Parameters.cbData;
    size += in->EncryptedHash.cbData;
    /* align pointer */
    if (size % sizeof(DWORD))
        size += size % sizeof(DWORD);
    size += CRYPT_SizeOfAttributes(&in->AuthAttrs);
    size += CRYPT_SizeOfAttributes(&in->UnauthAttrs);
    if (!pvData)
    {
        *pcbData = size;
        ret = TRUE;
    }
    else if (*pcbData < size)
    {
        *pcbData = size;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        LPBYTE nextData = (BYTE *)pvData + sizeof(CMSG_SIGNER_INFO);
        CMSG_SIGNER_INFO *out = (CMSG_SIGNER_INFO *)pvData;

        out->dwVersion = in->dwVersion;
        CRYPT_CopyBlob(&out->Issuer, &in->Issuer, &nextData);
        CRYPT_CopyBlob(&out->SerialNumber, &in->SerialNumber, &nextData);
        CRYPT_CopyAlgorithmId(&out->HashAlgorithm, &in->HashAlgorithm,
         &nextData);
        CRYPT_CopyAlgorithmId(&out->HashEncryptionAlgorithm,
         &in->HashEncryptionAlgorithm, &nextData);
        CRYPT_CopyBlob(&out->EncryptedHash, &in->EncryptedHash, &nextData);
        /* align pointer */
        if ((nextData - (LPBYTE)0) % sizeof(DWORD))
            nextData += (nextData - (LPBYTE)0) % sizeof(DWORD);
        CRYPT_CopyAttributes(&out->AuthAttrs, &in->AuthAttrs, &nextData);
        CRYPT_CopyAttributes(&out->UnauthAttrs, &in->UnauthAttrs, &nextData);
        ret = TRUE;
    }
    return ret;
}

static BOOL CRYPT_CopySignerCertInfo(void *pvData, DWORD *pcbData,
 const CMSG_SIGNER_INFO *in)
{
    DWORD size = sizeof(CERT_INFO);
    BOOL ret;

    size += in->Issuer.cbData;
    size += in->SerialNumber.cbData;
    if (!pvData)
    {
        *pcbData = size;
        ret = TRUE;
    }
    else if (*pcbData < size)
    {
        *pcbData = size;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        LPBYTE nextData = (BYTE *)pvData + sizeof(CERT_INFO);
        CERT_INFO *out = (CERT_INFO *)pvData;

        memset(out, 0, sizeof(CERT_INFO));
        CRYPT_CopyBlob(&out->Issuer, &in->Issuer, &nextData);
        CRYPT_CopyBlob(&out->SerialNumber, &in->SerialNumber, &nextData);
        ret = TRUE;
    }
    return ret;
}

static BOOL CDecodeSignedMsg_GetParam(CDecodeMsg *msg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_TYPE_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, &msg->type, sizeof(msg->type));
        break;
    case CMSG_CONTENT_PARAM:
        if (msg->u.signedInfo)
        {
            CRYPT_DATA_BLOB *blob;
            DWORD size;

            /* FIXME: does this depend on inner content type? */
            ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
             msg->u.signedInfo->content.Content.pbData,
             msg->u.signedInfo->content.Content.cbData, CRYPT_DECODE_ALLOC_FLAG,
             NULL, (LPBYTE)&blob, &size);
            if (ret)
            {
                ret = CRYPT_CopyParam(pvData, pcbData, blob->pbData,
                 blob->cbData);
                LocalFree(blob);
            }
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_INNER_CONTENT_TYPE_PARAM:
        if (msg->u.signedInfo)
            ret = CRYPT_CopyParam(pvData, pcbData,
             msg->u.signedInfo->content.pszObjId,
             strlen(msg->u.signedInfo->content.pszObjId) + 1);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_COUNT_PARAM:
        if (msg->u.signedInfo)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signedInfo->cSignerInfo, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_INFO_PARAM:
        if (msg->u.signedInfo)
        {
            if (dwIndex >= msg->u.signedInfo->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopySignerInfo(pvData, pcbData,
                 &msg->u.signedInfo->rgSignerInfo[dwIndex]);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_CERT_INFO_PARAM:
        if (msg->u.signedInfo)
        {
            if (dwIndex >= msg->u.signedInfo->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopySignerCertInfo(pvData, pcbData,
                 &msg->u.signedInfo->rgSignerInfo[dwIndex]);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CERT_COUNT_PARAM:
        if (msg->u.signedInfo)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signedInfo->cCertEncoded, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CERT_PARAM:
        if (msg->u.signedInfo)
        {
            if (dwIndex >= msg->u.signedInfo->cCertEncoded)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 msg->u.signedInfo->rgCertEncoded[dwIndex].pbData,
                 msg->u.signedInfo->rgCertEncoded[dwIndex].cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CRL_COUNT_PARAM:
        if (msg->u.signedInfo)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signedInfo->cCrlEncoded, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CRL_PARAM:
        if (msg->u.signedInfo)
        {
            if (dwIndex >= msg->u.signedInfo->cCrlEncoded)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 msg->u.signedInfo->rgCrlEncoded[dwIndex].pbData,
                 msg->u.signedInfo->rgCrlEncoded[dwIndex].cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ATTR_CERT_COUNT_PARAM:
        if (msg->u.signedInfo)
        {
            DWORD attrCertCount = 0;

            ret = CRYPT_CopyParam(pvData, pcbData,
             &attrCertCount, sizeof(DWORD));
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ATTR_CERT_PARAM:
        if (msg->u.signedInfo)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    default:
        FIXME("unimplemented for %d\n", dwParamType);
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CDecodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CDecodeMsg *msg = (CDecodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    switch (msg->type)
    {
    case CMSG_HASHED:
        ret = CDecodeHashMsg_GetParam(msg, dwParamType, dwIndex, pvData,
         pcbData);
        break;
    case CMSG_SIGNED:
        ret = CDecodeSignedMsg_GetParam(msg, dwParamType, dwIndex, pvData,
         pcbData);
        break;
    default:
        switch (dwParamType)
        {
        case CMSG_TYPE_PARAM:
            ret = CRYPT_CopyParam(pvData, pcbData, &msg->type,
             sizeof(msg->type));
            break;
        default:
        {
            CRYPT_DATA_BLOB blob;

            ret = ContextPropertyList_FindProperty(msg->properties, dwParamType,
             &blob);
            if (ret)
                ret = CRYPT_CopyParam(pvData, pcbData, blob.pbData,
                 blob.cbData);
            else
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        }
        }
    }
    return ret;
}

HCRYPTMSG WINAPI CryptMsgOpenToDecode(DWORD dwMsgEncodingType, DWORD dwFlags,
 DWORD dwMsgType, HCRYPTPROV_LEGACY hCryptProv, PCERT_INFO pRecipientInfo,
 PCMSG_STREAM_INFO pStreamInfo)
{
    CDecodeMsg *msg;

    TRACE("(%08x, %08x, %08x, %08lx, %p, %p)\n", dwMsgEncodingType,
     dwFlags, dwMsgType, hCryptProv, pRecipientInfo, pStreamInfo);

    if (GET_CMSG_ENCODING_TYPE(dwMsgEncodingType) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    msg = CryptMemAlloc(sizeof(CDecodeMsg));
    if (msg)
    {
        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CDecodeMsg_Close, CDecodeMsg_GetParam, CDecodeMsg_Update);
        msg->type = dwMsgType;
        if (hCryptProv)
            msg->crypt_prov = hCryptProv;
        else
        {
            msg->crypt_prov = CRYPT_GetDefaultProvider();
            msg->base.open_flags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
        }
        memset(&msg->u, 0, sizeof(msg->u));
        msg->msg_data.cbData = 0;
        msg->msg_data.pbData = NULL;
        msg->properties = ContextPropertyList_Create();
    }
    return msg;
}

HCRYPTMSG WINAPI CryptMsgDuplicate(HCRYPTMSG hCryptMsg)
{
    TRACE("(%p)\n", hCryptMsg);

    if (hCryptMsg)
    {
        CryptMsgBase *msg = (CryptMsgBase *)hCryptMsg;

        InterlockedIncrement(&msg->ref);
    }
    return hCryptMsg;
}

BOOL WINAPI CryptMsgClose(HCRYPTMSG hCryptMsg)
{
    TRACE("(%p)\n", hCryptMsg);

    if (hCryptMsg)
    {
        CryptMsgBase *msg = (CryptMsgBase *)hCryptMsg;

        if (InterlockedDecrement(&msg->ref) == 0)
        {
            TRACE("freeing %p\n", msg);
            if (msg->close)
                msg->close(msg);
            CryptMemFree(msg);
        }
    }
    return TRUE;
}

BOOL WINAPI CryptMsgUpdate(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CryptMsgBase *msg = (CryptMsgBase *)hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %d, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    if (msg->state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else
    {
        ret = msg->update(hCryptMsg, pbData, cbData, fFinal);
        msg->state = MsgStateUpdated;
        if (fFinal)
            msg->state = MsgStateFinalized;
    }
    return ret;
}

BOOL WINAPI CryptMsgGetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CryptMsgBase *msg = (CryptMsgBase *)hCryptMsg;

    TRACE("(%p, %d, %d, %p, %p)\n", hCryptMsg, dwParamType, dwIndex,
     pvData, pcbData);
    return msg->get_param(hCryptMsg, dwParamType, dwIndex, pvData, pcbData);
}

BOOL WINAPI CryptMsgControl(HCRYPTMSG hCryptMsg, DWORD dwFlags,
 DWORD dwCtrlType, const void *pvCtrlPara)
{
    FIXME("(%p, %08x, %d, %p): stub\n", hCryptMsg, dwFlags, dwCtrlType,
     pvCtrlPara);
    return TRUE;
}
