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

static BOOL CRYPT_CopyParam(void *pvData, DWORD *pcbData, const BYTE *src,
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
        CRYPT_ALGORITHM_IDENTIFIER algoId = { 0 };
        DWORD version = CMSG_HASHED_DATA_PKCS_1_5_VERSION;
        struct AsnEncodeSequenceItem items[7] = { { 0 } };
        DWORD cItem = 0;
        CRYPT_DATA_BLOB hash = { 0, NULL };
        CRYPT_CONTENT_INFO contentInfo = { NULL, { 0, NULL } };
        char oid_rsa_data[] = szOID_RSA_data;

        items[cItem].pvStructInfo = &version;
        items[cItem].encodeFunc = CRYPT_AsnEncodeInt;
        cItem++;
        algoId.pszObjId = (LPSTR)CertAlgIdToOID(algID);
        /* FIXME: what about algoId.Parameters? */
        items[cItem].pvStructInfo = &algoId;
        items[cItem].encodeFunc = CRYPT_AsnEncodeAlgorithmIdWithNullParams;
        cItem++;
        /* Quirk:  OID is only encoded messages if an update has happened */
        if (msg->base.state != MsgStateInit)
            contentInfo.pszObjId = oid_rsa_data;
        if (!(msg->base.open_flags & CMSG_DETACHED_FLAG) && msg->data.cbData)
        {
            ret = CRYPT_AsnEncodeOctets(0, NULL, &msg->data,
             CRYPT_ENCODE_ALLOC_FLAG, NULL,
             (LPBYTE)&contentInfo.Content.pbData,
             &contentInfo.Content.cbData);
        }
        items[cItem].pvStructInfo = &contentInfo;
        items[cItem].encodeFunc =
         CRYPT_AsnEncodePKCSContentInfoInternal;
        cItem++;
        if (msg->base.state == MsgStateFinalized)
        {
            size = sizeof(DWORD);
            ret = CryptGetHashParam(msg->hash, HP_HASHSIZE,
             (LPBYTE)&hash.cbData, &size, 0);
            if (ret)
            {
                hash.pbData = CryptMemAlloc(hash.cbData);
                ret = CryptGetHashParam(msg->hash, HP_HASHVAL, hash.pbData,
                 &hash.cbData, 0);
            }
        }
        items[cItem].pvStructInfo = &hash;
        items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
        cItem++;
        if (ret)
            ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem,
             0, NULL, pvData, pcbData);
        CryptMemFree(hash.pbData);
        LocalFree(contentInfo.Content.pbData);
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
            ret = CRYPT_CopyParam(pvData, pcbData, (const BYTE *)&version,
             sizeof(version));
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
    CryptMsgBase     base;
    DWORD            type;
    HCRYPTPROV       crypt_prov;
    CRYPT_DATA_BLOB  msg_data;
    PCRYPT_DATA_BLOB data; /* for type == CMSG_DATA */
} CDecodeMsg;

static void CDecodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CDecodeMsg *msg = (CDecodeMsg *)hCryptMsg;

    if (msg->base.open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        CryptReleaseContext(msg->crypt_prov, 0);
    CryptMemFree(msg->msg_data.pbData);
    LocalFree(msg->data);
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

/* Decodes the content in blob as the type given, and updates the value
 * (type, parameters, etc.) of msg based on what blob contains.
 * It doesn't just use msg's type, to allow a recursive call from an implicitly
 * typed message once the outer content info has been decoded.
 */
static BOOL CDecodeMsg_DecodeContent(CDecodeMsg *msg, CRYPT_DER_BLOB *blob,
 DWORD type)
{
    BOOL ret;
    DWORD size;

    switch (type)
    {
    case CMSG_DATA:
        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
         blob->pbData, blob->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (LPBYTE)&msg->data, &size);
        if (ret)
            msg->type = CMSG_DATA;
        break;
    case CMSG_HASHED:
    case CMSG_ENVELOPED:
    case CMSG_SIGNED:
        FIXME("unimplemented for type %s\n", MSG_TYPE_STR(type));
        ret = TRUE;
        break;
    default:
    {
        CRYPT_CONTENT_INFO *info;

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

static BOOL CDecodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CDecodeMsg *msg = (CDecodeMsg *)hCryptMsg;
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_TYPE_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, (const BYTE *)&msg->type,
         sizeof(msg->type));
        break;
    default:
        FIXME("unimplemented for parameter %d\n", dwParamType);
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

HCRYPTMSG WINAPI CryptMsgOpenToDecode(DWORD dwMsgEncodingType, DWORD dwFlags,
 DWORD dwMsgType, HCRYPTPROV hCryptProv, PCERT_INFO pRecipientInfo,
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
        msg->msg_data.cbData = 0;
        msg->msg_data.pbData = NULL;
        msg->data = NULL;
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
