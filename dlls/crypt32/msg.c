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

typedef BOOL (*CryptMsgControlFunc)(HCRYPTMSG hCryptMsg, DWORD dwFlags,
 DWORD dwCtrlType, const void *pvCtrlPara);

static BOOL extract_hash(HCRYPTHASH hash, BYTE **data, DWORD *size)
{
    DWORD sz;

    *data = NULL;
    sz = sizeof(*size);
    if (!CryptGetHashParam(hash, HP_HASHSIZE, (BYTE *)size, &sz, 0)) return FALSE;
    if (!(*data = CryptMemAlloc(*size)))
    {
        ERR("No memory.\n");
        return FALSE;
    }
    if (CryptGetHashParam(hash, HP_HASHVAL, *data, size, 0)) return TRUE;
    CryptMemFree(*data);
    *data = NULL;
    return FALSE;
}

static BOOL CRYPT_DefaultMsgControl(HCRYPTMSG hCryptMsg, DWORD dwFlags,
 DWORD dwCtrlType, const void *pvCtrlPara)
{
    TRACE("(%p, %08lx, %ld, %p)\n", hCryptMsg, dwFlags, dwCtrlType, pvCtrlPara);
    SetLastError(E_INVALIDARG);
    return FALSE;
}

typedef enum _CryptMsgState {
    MsgStateInit,
    MsgStateUpdated,
    MsgStateDataFinalized,
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
    CryptMsgControlFunc  control;
} CryptMsgBase;

static inline void CryptMsgBase_Init(CryptMsgBase *msg, DWORD dwFlags,
 PCMSG_STREAM_INFO pStreamInfo, CryptMsgCloseFunc close,
 CryptMsgGetParamFunc get_param, CryptMsgUpdateFunc update,
 CryptMsgControlFunc control)
{
    msg->ref = 1;
    msg->open_flags = dwFlags;
    if (pStreamInfo)
    {
        msg->streamed = TRUE;
        msg->stream_info = *pStreamInfo;
    }
    else
    {
        msg->streamed = FALSE;
        memset(&msg->stream_info, 0, sizeof(msg->stream_info));
    }
    msg->close = close;
    msg->get_param = get_param;
    msg->update = update;
    msg->control = control;
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
    CDataEncodeMsg *msg = hCryptMsg;

    if (msg->bare_content != empty_data_content)
        LocalFree(msg->bare_content);
}

static BOOL WINAPI CRYPT_EncodeContentLength(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD dataLen = *(DWORD *)pvStructInfo;
    DWORD lenBytes;
    BOOL ret = TRUE;

    /* Trick:  report bytes needed based on total message length, even though
     * the message isn't available yet.  The caller will use the length
     * reported here to encode its length.
     */
    CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
    if (!pbEncoded)
        *pcbEncoded = 1 + lenBytes + dataLen;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
         pcbEncoded, 1 + lenBytes)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_OCTETSTRING;
            CRYPT_EncodeLen(dataLen, pbEncoded,
             &lenBytes);
        }
    }
    return ret;
}

static BOOL CRYPT_EncodeDataContentInfoHeader(const CDataEncodeMsg *msg,
 CRYPT_DATA_BLOB *header)
{
    BOOL ret;

    if (msg->base.streamed && msg->base.stream_info.cbContent == 0xffffffff)
    {
        static const BYTE headerValue[] = { 0x30,0x80,0x06,0x09,0x2a,0x86,0x48,
         0x86,0xf7,0x0d,0x01,0x07,0x01,0xa0,0x80,0x24,0x80 };

        header->pbData = LocalAlloc(0, sizeof(headerValue));
        if (header->pbData)
        {
            header->cbData = sizeof(headerValue);
            memcpy(header->pbData, headerValue, sizeof(headerValue));
            ret = TRUE;
        }
        else
            ret = FALSE;
    }
    else
    {
        struct AsnConstructedItem constructed = { 0,
         &msg->base.stream_info.cbContent, CRYPT_EncodeContentLength };
        struct AsnEncodeSequenceItem items[2] = {
         { szOID_RSA_data, CRYPT_AsnEncodeOid, 0 },
         { &constructed,   CRYPT_AsnEncodeConstructed, 0 },
        };

        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items,
         ARRAY_SIZE(items), CRYPT_ENCODE_ALLOC_FLAG, NULL,
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
    CDataEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    if (msg->base.state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else if (msg->base.streamed)
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
            /* Curiously, every indefinite-length streamed update appears to
             * get its own tag and length, regardless of fFinal.
             */
            if (msg->base.stream_info.cbContent == 0xffffffff)
            {
                BYTE *header;
                DWORD headerLen;

                ret = CRYPT_EncodeContentLength(X509_ASN_ENCODING, NULL,
                 &cbData, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&header,
                 &headerLen);
                if (ret)
                {
                    ret = msg->base.stream_info.pfnStreamOutput(
                     msg->base.stream_info.pvArg, header, headerLen,
                     FALSE);
                    LocalFree(header);
                }
            }
            if (!fFinal)
            {
                ret = msg->base.stream_info.pfnStreamOutput(
                 msg->base.stream_info.pvArg, (BYTE *)pbData, cbData,
                 FALSE);
                msg->base.state = MsgStateUpdated;
            }
            else
            {
                msg->base.state = MsgStateFinalized;
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
            ret = FALSE;
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
            CRYPT_DATA_BLOB blob = { cbData, (LPBYTE)pbData };

            msg->base.state = MsgStateFinalized;
            /* non-streamed data messages don't allow non-final updates,
             * don't bother checking whether data already exist, they can't.
             */
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
             &blob, CRYPT_ENCODE_ALLOC_FLAG, NULL, &msg->bare_content,
             &msg->bare_content_len);
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
    CDataEncodeMsg *msg = hCryptMsg;
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
         CDataEncodeMsg_Close, CDataEncodeMsg_GetParam, CDataEncodeMsg_Update,
         CRYPT_DefaultMsgControl);
        msg->bare_content_len = sizeof(empty_data_content);
        msg->bare_content = (LPBYTE)empty_data_content;
    }
    return msg;
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
    CHashEncodeMsg *msg = hCryptMsg;

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
            ret = extract_hash(msg->hash, &digestedData.hash.pbData, &digestedData.hash.cbData);
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
    CHashEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %ld, %p, %p)\n", hCryptMsg, dwParamType, dwIndex,
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
        ret = CryptGetHashParam(msg->hash, HP_HASHVAL, pvData, pcbData, 0);
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
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CHashEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CHashEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %ld, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    if (msg->base.state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else if (msg->base.streamed || (msg->base.open_flags & CMSG_DETACHED_FLAG))
    {
        /* Doesn't do much, as stream output is never called, and you
         * can't get the content.
         */
        ret = CryptHashData(msg->hash, pbData, cbData, 0);
        msg->base.state = fFinal ? MsgStateFinalized : MsgStateUpdated;
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
            msg->base.state = MsgStateFinalized;
        }
    }
    return ret;
}

static HCRYPTMSG CHashEncodeMsg_Open(DWORD dwFlags, const void *pvMsgEncodeInfo,
 LPSTR pszInnerContentObjID, PCMSG_STREAM_INFO pStreamInfo)
{
    CHashEncodeMsg *msg;
    const CMSG_HASHED_ENCODE_INFO *info = pvMsgEncodeInfo;
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
        prov = I_CryptGetDefaultCryptProv(algID);
        if (!prov)
        {
            SetLastError(E_INVALIDARG);
            return NULL;
        }
        dwFlags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
    }
    msg = CryptMemAlloc(sizeof(CHashEncodeMsg));
    if (msg)
    {
        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CHashEncodeMsg_Close, CHashEncodeMsg_GetParam, CHashEncodeMsg_Update,
         CRYPT_DefaultMsgControl);
        msg->prov = prov;
        msg->data.cbData = 0;
        msg->data.pbData = NULL;
        if (!CryptCreateHash(prov, algID, 0, 0, &msg->hash))
        {
            CryptMsgClose(msg);
            msg = NULL;
        }
    }
    return msg;
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
} CMSG_SIGNER_ENCODE_INFO_WITH_CMS;

typedef struct _CMSG_SIGNED_ENCODE_INFO_WITH_CMS
{
    DWORD                             cbSize;
    DWORD                             cSigners;
    CMSG_SIGNER_ENCODE_INFO_WITH_CMS *rgSigners;
    DWORD                             cCertEncoded;
    PCERT_BLOB                        rgCertEncoded;
    DWORD                             cCrlEncoded;
    PCRL_BLOB                         rgCrlEncoded;
    DWORD                             cAttrCertEncoded;
    PCERT_BLOB                        rgAttrCertEncoded;
} CMSG_SIGNED_ENCODE_INFO_WITH_CMS;

static BOOL CRYPT_IsValidSigner(const CMSG_SIGNER_ENCODE_INFO_WITH_CMS *signer)
{
    if (signer->cbSize != sizeof(CMSG_SIGNER_ENCODE_INFO) &&
     signer->cbSize != sizeof(CMSG_SIGNER_ENCODE_INFO_WITH_CMS))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (signer->cbSize == sizeof(CMSG_SIGNER_ENCODE_INFO))
    {
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
    }
    else if (signer->cbSize == sizeof(CMSG_SIGNER_ENCODE_INFO_WITH_CMS))
    {
        switch (signer->SignerId.dwIdChoice)
        {
        case 0:
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
            break;
        case CERT_ID_ISSUER_SERIAL_NUMBER:
            if (!signer->SignerId.IssuerSerialNumber.SerialNumber.cbData)
            {
                SetLastError(E_INVALIDARG);
                return FALSE;
            }
            if (!signer->SignerId.IssuerSerialNumber.Issuer.cbData)
            {
                SetLastError(E_INVALIDARG);
                return FALSE;
            }
            break;
        case CERT_ID_KEY_IDENTIFIER:
            if (!signer->SignerId.KeyId.cbData)
            {
                SetLastError(E_INVALIDARG);
                return FALSE;
            }
            break;
        default:
            SetLastError(E_INVALIDARG);
        }
        if (signer->HashEncryptionAlgorithm.pszObjId)
        {
            FIXME("CMSG_SIGNER_ENCODE_INFO with CMS fields unsupported\n");
            return FALSE;
        }
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

static BOOL CRYPT_ConstructBlobArray(DWORD *outCBlobs,
 PCRYPT_DATA_BLOB *outPBlobs, DWORD cBlobs, const CRYPT_DATA_BLOB *pBlobs)
{
    BOOL ret = TRUE;

    *outCBlobs = cBlobs;
    if (cBlobs)
    {
        *outPBlobs = CryptMemAlloc(cBlobs * sizeof(CRYPT_DATA_BLOB));
        if (*outPBlobs)
        {
            DWORD i;

            memset(*outPBlobs, 0, cBlobs * sizeof(CRYPT_DATA_BLOB));
            for (i = 0; ret && i < cBlobs; i++)
                ret = CRYPT_ConstructBlob(&(*outPBlobs)[i], &pBlobs[i]);
        }
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_AddBlobArray(DWORD *outCBlobs,
 PCRYPT_DATA_BLOB *outPBlobs, DWORD cBlobs, const CRYPT_DATA_BLOB *pBlobs)
{
    BOOL ret = TRUE;

    if (cBlobs)
    {
        CRYPT_DATA_BLOB *new_blobs = CryptMemRealloc(*outPBlobs, (*outCBlobs + cBlobs) * sizeof(CRYPT_DATA_BLOB));
        if (new_blobs)
        {
            DWORD i;

            *outPBlobs = new_blobs;

            memset(*outPBlobs + *outCBlobs, 0, cBlobs * sizeof(CRYPT_DATA_BLOB));
            for (i = *outCBlobs; ret && i < *outCBlobs + cBlobs; i++)
                ret = CRYPT_ConstructBlob(&(*outPBlobs)[i], &pBlobs[i]);

            if (ret)
                *outCBlobs += cBlobs;
        }
        else
            ret = FALSE;
    }
    return ret;
}

static void CRYPT_FreeBlobArray(DWORD cBlobs, PCRYPT_DATA_BLOB blobs)
{
    DWORD i;

    for (i = 0; i < cBlobs; i++)
        CryptMemFree(blobs[i].pbData);
    CryptMemFree(blobs);
}

static BOOL CRYPT_ConstructAttribute(CRYPT_ATTRIBUTE *out,
 const CRYPT_ATTRIBUTE *in)
{
    BOOL ret;

    out->pszObjId = CryptMemAlloc(strlen(in->pszObjId) + 1);
    if (out->pszObjId)
    {
        strcpy(out->pszObjId, in->pszObjId);
        ret = CRYPT_ConstructBlobArray(&out->cValue, &out->rgValue,
         in->cValue, in->rgValue);
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

static BOOL CRYPT_AddAttribute(CRYPT_ATTRIBUTES *out, const CRYPT_ATTRIBUTE *in)
{
    BOOL ret;
    CRYPT_ATTRIBUTE *new_attrs;

    new_attrs = CryptMemRealloc(out->rgAttr, (out->cAttr + 1) * sizeof(CRYPT_ATTRIBUTE));
    if (new_attrs)
    {
        out->rgAttr = new_attrs;
        memset(&out->rgAttr[out->cAttr], 0, sizeof(CRYPT_ATTRIBUTE));
        ret = CRYPT_ConstructAttribute(&new_attrs[out->cAttr], in);
        if (ret)
            out->cAttr++;
    }
    else
        ret = FALSE;
    return ret;
}

/* Constructs a CMSG_CMS_SIGNER_INFO from a CMSG_SIGNER_ENCODE_INFO_WITH_CMS. */
static BOOL CSignerInfo_Construct(CMSG_CMS_SIGNER_INFO *info,
 const CMSG_SIGNER_ENCODE_INFO_WITH_CMS *in)
{
    BOOL ret;

    if (in->cbSize == sizeof(CMSG_SIGNER_ENCODE_INFO))
    {
        info->dwVersion = CMSG_SIGNER_INFO_V1;
        ret = CRYPT_ConstructBlob(&info->SignerId.IssuerSerialNumber.Issuer,
         &in->pCertInfo->Issuer);
        if (ret)
            ret = CRYPT_ConstructBlob(
             &info->SignerId.IssuerSerialNumber.SerialNumber,
             &in->pCertInfo->SerialNumber);
        info->SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
        info->HashEncryptionAlgorithm.pszObjId =
         in->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;
        if (ret)
            ret = CRYPT_ConstructBlob(&info->HashEncryptionAlgorithm.Parameters,
             &in->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters);
    }
    else
    {
        const CRYPT_ALGORITHM_IDENTIFIER *pEncrAlg;

        /* Implicitly in->cbSize == sizeof(CMSG_SIGNER_ENCODE_INFO_WITH_CMS).
         * See CRYPT_IsValidSigner.
         */
        if (!in->SignerId.dwIdChoice)
        {
            info->dwVersion = CMSG_SIGNER_INFO_V1;
            ret = CRYPT_ConstructBlob(&info->SignerId.IssuerSerialNumber.Issuer,
             &in->pCertInfo->Issuer);
            if (ret)
                ret = CRYPT_ConstructBlob(
                 &info->SignerId.IssuerSerialNumber.SerialNumber,
                 &in->pCertInfo->SerialNumber);
            info->SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
        }
        else if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
        {
            info->dwVersion = CMSG_SIGNER_INFO_V1;
            info->SignerId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
            ret = CRYPT_ConstructBlob(&info->SignerId.IssuerSerialNumber.Issuer,
             &in->SignerId.IssuerSerialNumber.Issuer);
            if (ret)
                ret = CRYPT_ConstructBlob(
                 &info->SignerId.IssuerSerialNumber.SerialNumber,
                 &in->SignerId.IssuerSerialNumber.SerialNumber);
        }
        else
        {
            /* Implicitly dwIdChoice == CERT_ID_KEY_IDENTIFIER */
            info->dwVersion = CMSG_SIGNER_INFO_V3;
            info->SignerId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
            ret = CRYPT_ConstructBlob(&info->SignerId.KeyId,
             &in->SignerId.KeyId);
        }
        pEncrAlg = in->HashEncryptionAlgorithm.pszObjId ?
         &in->HashEncryptionAlgorithm :
         &in->pCertInfo->SubjectPublicKeyInfo.Algorithm;
        info->HashEncryptionAlgorithm.pszObjId = pEncrAlg->pszObjId;
        if (ret)
            ret = CRYPT_ConstructBlob(&info->HashEncryptionAlgorithm.Parameters,
             &pEncrAlg->Parameters);
    }
    /* Assumption:  algorithm IDs will point to static strings, not
     * stack-based ones, so copying the pointer values is safe.
     */
    info->HashAlgorithm.pszObjId = in->HashAlgorithm.pszObjId;
    if (ret)
        ret = CRYPT_ConstructBlob(&info->HashAlgorithm.Parameters,
         &in->HashAlgorithm.Parameters);
    if (ret)
        ret = CRYPT_ConstructAttributes(&info->AuthAttrs,
         (CRYPT_ATTRIBUTES *)&in->cAuthAttr);
    if (ret)
        ret = CRYPT_ConstructAttributes(&info->UnauthAttrs,
         (CRYPT_ATTRIBUTES *)&in->cUnauthAttr);
    return ret;
}

static void CSignerInfo_Free(CMSG_CMS_SIGNER_INFO *info)
{
    DWORD i, j;

    if (info->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
    {
        CryptMemFree(info->SignerId.IssuerSerialNumber.Issuer.pbData);
        CryptMemFree(info->SignerId.IssuerSerialNumber.SerialNumber.pbData);
    }
    else
        CryptMemFree(info->SignerId.KeyId.pbData);
    CryptMemFree(info->HashAlgorithm.Parameters.pbData);
    CryptMemFree(info->HashEncryptionAlgorithm.Parameters.pbData);
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

typedef struct _CSignerHandles
{
    HCRYPTHASH contentHash;
    HCRYPTHASH authAttrHash;
} CSignerHandles;

typedef struct _CSignedMsgData
{
    CRYPT_SIGNED_INFO *info;
    DWORD              cSignerHandle;
    CSignerHandles    *signerHandles;
} CSignedMsgData;

/* Constructs the signer handles for the signerIndex'th signer of msg_data.
 * Assumes signerIndex is a valid index, and that msg_data's info has already
 * been constructed.
 */
static BOOL CSignedMsgData_ConstructSignerHandles(CSignedMsgData *msg_data,
 DWORD signerIndex, HCRYPTPROV *crypt_prov, DWORD *flags)
{
    ALG_ID algID;
    BOOL ret;

    algID = CertOIDToAlgId(
     msg_data->info->rgSignerInfo[signerIndex].HashAlgorithm.pszObjId);

    if (!*crypt_prov)
    {
        *crypt_prov = I_CryptGetDefaultCryptProv(algID);
        if (!*crypt_prov) return FALSE;
        *flags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
    }

    ret = CryptCreateHash(*crypt_prov, algID, 0, 0,
     &msg_data->signerHandles->contentHash);
    if (ret && msg_data->info->rgSignerInfo[signerIndex].AuthAttrs.cAttr > 0)
        ret = CryptCreateHash(*crypt_prov, algID, 0, 0,
         &msg_data->signerHandles->authAttrHash);
    return ret;
}

/* Allocates a CSignedMsgData's handles.  Assumes its info has already been
 * constructed.
 */
static BOOL CSignedMsgData_AllocateHandles(CSignedMsgData *msg_data)
{
    BOOL ret = TRUE;

    if (msg_data->info->cSignerInfo)
    {
        msg_data->signerHandles =
         CryptMemAlloc(msg_data->info->cSignerInfo * sizeof(CSignerHandles));
        if (msg_data->signerHandles)
        {
            msg_data->cSignerHandle = msg_data->info->cSignerInfo;
            memset(msg_data->signerHandles, 0,
             msg_data->info->cSignerInfo * sizeof(CSignerHandles));
        }
        else
        {
            msg_data->cSignerHandle = 0;
            ret = FALSE;
        }
    }
    else
    {
        msg_data->cSignerHandle = 0;
        msg_data->signerHandles = NULL;
    }
    return ret;
}

static void CSignedMsgData_CloseHandles(CSignedMsgData *msg_data)
{
    DWORD i;

    for (i = 0; i < msg_data->cSignerHandle; i++)
    {
        if (msg_data->signerHandles[i].contentHash)
            CryptDestroyHash(msg_data->signerHandles[i].contentHash);
        if (msg_data->signerHandles[i].authAttrHash)
            CryptDestroyHash(msg_data->signerHandles[i].authAttrHash);
    }
    CryptMemFree(msg_data->signerHandles);
    msg_data->signerHandles = NULL;
    msg_data->cSignerHandle = 0;
}

static BOOL CSignedMsgData_UpdateHash(CSignedMsgData *msg_data,
 const BYTE *pbData, DWORD cbData)
{
    DWORD i;
    BOOL ret = TRUE;

    for (i = 0; ret && i < msg_data->cSignerHandle; i++)
        ret = CryptHashData(msg_data->signerHandles[i].contentHash, pbData,
         cbData, 0);
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

static BOOL CSignedMsgData_AppendMessageDigestAttribute(
 CSignedMsgData *msg_data, DWORD signerIndex)
{
    BOOL ret;
    CRYPT_HASH_BLOB hash = { 0, NULL }, encodedHash = { 0, NULL };
    char messageDigest[] = szOID_RSA_messageDigest;
    CRYPT_ATTRIBUTE messageDigestAttr = { messageDigest, 1, &encodedHash };

    if (!(ret = extract_hash(msg_data->signerHandles[signerIndex].contentHash, &hash.pbData, &hash.cbData)))
        return FALSE;

    ret = CRYPT_AsnEncodeOctets(0, NULL, &hash, CRYPT_ENCODE_ALLOC_FLAG, NULL, (LPBYTE)&encodedHash.pbData,
            &encodedHash.cbData);
    if (ret)
    {
        ret = CRYPT_AppendAttribute(
         &msg_data->info->rgSignerInfo[signerIndex].AuthAttrs,
         &messageDigestAttr);
        LocalFree(encodedHash.pbData);
    }
    CryptMemFree(hash.pbData);
    return ret;
}

typedef enum {
    Sign,
    Verify
} SignOrVerify;

static BOOL CSignedMsgData_UpdateAuthenticatedAttributes(
 CSignedMsgData *msg_data, SignOrVerify flag)
{
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%p)\n", msg_data);

    for (i = 0; ret && i < msg_data->info->cSignerInfo; i++)
    {
        if (msg_data->info->rgSignerInfo[i].AuthAttrs.cAttr)
        {
            if (flag == Sign)
            {
                BYTE oid_rsa_data_encoded[] = { 0x06,0x09,0x2a,0x86,0x48,0x86,
                 0xf7,0x0d,0x01,0x07,0x01 };
                CRYPT_DATA_BLOB content = { sizeof(oid_rsa_data_encoded),
                 oid_rsa_data_encoded };
                char contentType[] = szOID_RSA_contentType;
                CRYPT_ATTRIBUTE contentTypeAttr = { contentType, 1, &content };

                /* FIXME: does this depend on inner OID? */
                ret = CRYPT_AppendAttribute(
                 &msg_data->info->rgSignerInfo[i].AuthAttrs, &contentTypeAttr);
                if (ret)
                    ret = CSignedMsgData_AppendMessageDigestAttribute(msg_data,
                     i);
            }
            if (ret)
            {
                LPBYTE encodedAttrs;
                DWORD size;

                ret = CryptEncodeObjectEx(X509_ASN_ENCODING, PKCS_ATTRIBUTES,
                 &msg_data->info->rgSignerInfo[i].AuthAttrs,
                 CRYPT_ENCODE_ALLOC_FLAG, NULL, &encodedAttrs, &size);
                if (ret)
                {
                    ret = CryptHashData(
                     msg_data->signerHandles[i].authAttrHash, encodedAttrs,
                     size, 0);
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

static BOOL CSignedMsgData_Sign(CSignedMsgData *msg_data)
{
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%p)\n", msg_data);

    for (i = 0; ret && i < msg_data->info->cSignerInfo; i++)
    {
        HCRYPTHASH hash;
        DWORD keySpec = msg_data->info->signerKeySpec[i];

        if (!keySpec)
            keySpec = AT_SIGNATURE;
        if (msg_data->info->rgSignerInfo[i].AuthAttrs.cAttr)
            hash = msg_data->signerHandles[i].authAttrHash;
        else
            hash = msg_data->signerHandles[i].contentHash;
        ret = CryptSignHashW(hash, keySpec, NULL, 0, NULL,
         &msg_data->info->rgSignerInfo[i].EncryptedHash.cbData);
        if (ret)
        {
            msg_data->info->rgSignerInfo[i].EncryptedHash.pbData =
             CryptMemAlloc(
             msg_data->info->rgSignerInfo[i].EncryptedHash.cbData);
            if (msg_data->info->rgSignerInfo[i].EncryptedHash.pbData)
            {
                ret = CryptSignHashW(hash, keySpec, NULL, 0,
                 msg_data->info->rgSignerInfo[i].EncryptedHash.pbData,
                 &msg_data->info->rgSignerInfo[i].EncryptedHash.cbData);
                if (ret)
                    CRYPT_ReverseBytes(
                     &msg_data->info->rgSignerInfo[i].EncryptedHash);
            }
            else
                ret = FALSE;
        }
    }
    return ret;
}

static BOOL CSignedMsgData_Update(CSignedMsgData *msg_data,
 const BYTE *pbData, DWORD cbData, BOOL fFinal, SignOrVerify flag)
{
    BOOL ret = CSignedMsgData_UpdateHash(msg_data, pbData, cbData);

    if (ret && fFinal)
    {
        ret = CSignedMsgData_UpdateAuthenticatedAttributes(msg_data, flag);
        if (ret && flag == Sign)
            ret = CSignedMsgData_Sign(msg_data);
    }
    return ret;
}

typedef struct _CSignedEncodeMsg
{
    CryptMsgBase    base;
    LPSTR           innerOID;
    CRYPT_DATA_BLOB data;
    CSignedMsgData  msg_data;
} CSignedEncodeMsg;

static void CSignedEncodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CSignedEncodeMsg *msg = hCryptMsg;
    DWORD i;

    CryptMemFree(msg->innerOID);
    CryptMemFree(msg->data.pbData);
    CRYPT_FreeBlobArray(msg->msg_data.info->cCertEncoded,
     msg->msg_data.info->rgCertEncoded);
    CRYPT_FreeBlobArray(msg->msg_data.info->cCrlEncoded,
     msg->msg_data.info->rgCrlEncoded);
    for (i = 0; i < msg->msg_data.info->cSignerInfo; i++)
        CSignerInfo_Free(&msg->msg_data.info->rgSignerInfo[i]);
    CSignedMsgData_CloseHandles(&msg->msg_data);
    CryptMemFree(msg->msg_data.info->signerKeySpec);
    CryptMemFree(msg->msg_data.info->rgSignerInfo);
    CryptMemFree(msg->msg_data.info);
}

static BOOL CSignedEncodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CSignedEncodeMsg *msg = hCryptMsg;
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
        BOOL freeContent = FALSE;
        char oid_rsa_data[] = szOID_RSA_data;

        info = *msg->msg_data.info;
        if (!msg->innerOID || !strcmp(msg->innerOID, szOID_RSA_data))
        {
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
                freeContent = TRUE;
            }
            else
            {
                info.content.Content.cbData = 0;
                info.content.Content.pbData = NULL;
                ret = TRUE;
            }
        }
        else
        {
            info.content.pszObjId = msg->innerOID;
            info.content.Content.cbData = msg->data.cbData;
            info.content.Content.pbData = msg->data.pbData;
            ret = TRUE;
        }
        if (ret)
        {
            ret = CRYPT_AsnEncodeCMSSignedInfo(&info, pvData, pcbData);
            if (freeContent)
                LocalFree(info.content.Content.pbData);
        }
        break;
    }
    case CMSG_COMPUTED_HASH_PARAM:
        if (dwIndex >= msg->msg_data.cSignerHandle)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            ret = CryptGetHashParam(
             msg->msg_data.signerHandles[dwIndex].contentHash, HP_HASHVAL,
             pvData, pcbData, 0);
        break;
    case CMSG_ENCODED_SIGNER:
        if (dwIndex >= msg->msg_data.info->cSignerInfo)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            ret = CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
             CMS_SIGNER_INFO, &msg->msg_data.info->rgSignerInfo[dwIndex], 0,
             NULL, pvData, pcbData);
        break;
    case CMSG_VERSION_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, &msg->msg_data.info->version,
         sizeof(msg->msg_data.info->version));
        break;
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CSignedEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CSignedEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    if (msg->base.state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else if (msg->base.streamed || (msg->base.open_flags & CMSG_DETACHED_FLAG))
    {
        ret = CSignedMsgData_Update(&msg->msg_data, pbData, cbData, fFinal,
         Sign);
        if (msg->base.streamed)
            FIXME("streamed partial stub\n");
        msg->base.state = fFinal ? MsgStateFinalized : MsgStateUpdated;
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
                ret = CSignedMsgData_Update(&msg->msg_data, pbData, cbData,
                 fFinal, Sign);
            msg->base.state = MsgStateFinalized;
        }
    }
    return ret;
}

static HCRYPTMSG CSignedEncodeMsg_Open(DWORD dwFlags,
 const void *pvMsgEncodeInfo, LPCSTR pszInnerContentObjID,
 PCMSG_STREAM_INFO pStreamInfo)
{
    const CMSG_SIGNED_ENCODE_INFO_WITH_CMS *info = pvMsgEncodeInfo;
    DWORD i;
    CSignedEncodeMsg *msg;

    if (info->cbSize != sizeof(CMSG_SIGNED_ENCODE_INFO) &&
     info->cbSize != sizeof(CMSG_SIGNED_ENCODE_INFO_WITH_CMS))
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (info->cbSize == sizeof(CMSG_SIGNED_ENCODE_INFO_WITH_CMS) &&
     info->cAttrCertEncoded)
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
         CSignedEncodeMsg_Update, CRYPT_DefaultMsgControl);
        if (pszInnerContentObjID)
        {
            msg->innerOID = CryptMemAlloc(strlen(pszInnerContentObjID) + 1);
            if (msg->innerOID)
                strcpy(msg->innerOID, pszInnerContentObjID);
            else
                ret = FALSE;
        }
        else
            msg->innerOID = NULL;
        msg->data.cbData = 0;
        msg->data.pbData = NULL;
        if (ret)
            msg->msg_data.info = CryptMemAlloc(sizeof(CRYPT_SIGNED_INFO));
        else
            msg->msg_data.info = NULL;
        if (msg->msg_data.info)
        {
            memset(msg->msg_data.info, 0, sizeof(CRYPT_SIGNED_INFO));
            msg->msg_data.info->version = CMSG_SIGNED_DATA_V1;
        }
        else
            ret = FALSE;
        if (ret)
        {
            if (info->cSigners)
            {
                msg->msg_data.info->rgSignerInfo =
                 CryptMemAlloc(info->cSigners * sizeof(CMSG_CMS_SIGNER_INFO));
                if (msg->msg_data.info->rgSignerInfo)
                {
                    msg->msg_data.info->cSignerInfo = info->cSigners;
                    memset(msg->msg_data.info->rgSignerInfo, 0,
                     msg->msg_data.info->cSignerInfo *
                     sizeof(CMSG_CMS_SIGNER_INFO));
                    ret = CSignedMsgData_AllocateHandles(&msg->msg_data);
                    msg->msg_data.info->signerKeySpec = CryptMemAlloc(info->cSigners * sizeof(DWORD));
                    if (!msg->msg_data.info->signerKeySpec)
                        ret = FALSE;
                    for (i = 0; ret && i < msg->msg_data.info->cSignerInfo; i++)
                    {
                        if (info->rgSigners[i].SignerId.dwIdChoice ==
                         CERT_ID_KEY_IDENTIFIER)
                            msg->msg_data.info->version = CMSG_SIGNED_DATA_V3;
                        ret = CSignerInfo_Construct(
                         &msg->msg_data.info->rgSignerInfo[i],
                         &info->rgSigners[i]);
                        if (ret)
                        {
                            ret = CSignedMsgData_ConstructSignerHandles(
                             &msg->msg_data, i, &info->rgSigners[i].hCryptProv, &dwFlags);
                            if (dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
                                CryptReleaseContext(info->rgSigners[i].hCryptProv,
                                 0);
                        }
                        msg->msg_data.info->signerKeySpec[i] =
                         info->rgSigners[i].dwKeySpec;
                    }
                }
                else
                    ret = FALSE;
            }
            else
            {
                msg->msg_data.info->cSignerInfo = 0;
                msg->msg_data.signerHandles = NULL;
                msg->msg_data.cSignerHandle = 0;
            }
        }
        if (ret)
            ret = CRYPT_ConstructBlobArray(&msg->msg_data.info->cCertEncoded,
             &msg->msg_data.info->rgCertEncoded, info->cCertEncoded,
             info->rgCertEncoded);
        if (ret)
            ret = CRYPT_ConstructBlobArray(&msg->msg_data.info->cCrlEncoded,
             &msg->msg_data.info->rgCrlEncoded, info->cCrlEncoded,
             info->rgCrlEncoded);
        if (!ret)
        {
            CSignedEncodeMsg_Close(msg);
            CryptMemFree(msg);
            msg = NULL;
        }
    }
    return msg;
}

typedef struct _CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS
{
    DWORD                       cbSize;
    HCRYPTPROV_LEGACY           hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER  ContentEncryptionAlgorithm;
    void                       *pvEncryptionAuxInfo;
    DWORD                       cRecipients;
    PCERT_INFO                 *rgpRecipientCert;
    PCMSG_RECIPIENT_ENCODE_INFO rgCmsRecipients;
    DWORD                       cCertEncoded;
    PCERT_BLOB                  rgCertEncoded;
    DWORD                       cCrlEncoded;
    PCRL_BLOB                   rgCrlEncoded;
    DWORD                       cAttrCertEncoded;
    PCERT_BLOB                  rgAttrCertEncoded;
    DWORD                       cUnprotectedAttr;
    PCRYPT_ATTRIBUTE            rgUnprotectedAttr;
} CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS;

typedef struct _CEnvelopedEncodeMsg
{
    CryptMsgBase                   base;
    CRYPT_ALGORITHM_IDENTIFIER     algo;
    HCRYPTPROV                     prov;
    HCRYPTKEY                      key;
    DWORD                          cRecipientInfo;
    CMSG_KEY_TRANS_RECIPIENT_INFO *recipientInfo;
    CRYPT_DATA_BLOB                data;
} CEnvelopedEncodeMsg;

static BOOL CRYPT_ConstructAlgorithmId(CRYPT_ALGORITHM_IDENTIFIER *out,
 const CRYPT_ALGORITHM_IDENTIFIER *in)
{
    out->pszObjId = CryptMemAlloc(strlen(in->pszObjId) + 1);
    if (out->pszObjId)
    {
        strcpy(out->pszObjId, in->pszObjId);
        return CRYPT_ConstructBlob(&out->Parameters, &in->Parameters);
    }
    else
        return FALSE;
}

static BOOL CRYPT_ConstructBitBlob(CRYPT_BIT_BLOB *out, const CRYPT_BIT_BLOB *in)
{
    out->cbData = in->cbData;
    out->cUnusedBits = in->cUnusedBits;
    if (out->cbData)
    {
        out->pbData = CryptMemAlloc(out->cbData);
        if (out->pbData)
            memcpy(out->pbData, in->pbData, out->cbData);
        else
            return FALSE;
    }
    else
        out->pbData = NULL;
    return TRUE;
}

static BOOL CRYPT_GenKey(CMSG_CONTENT_ENCRYPT_INFO *info, ALG_ID algID)
{
    static HCRYPTOIDFUNCSET set = NULL;
    PFN_CMSG_GEN_CONTENT_ENCRYPT_KEY genKeyFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc;
    BOOL ret;

    if (!set)
        set = CryptInitOIDFunctionSet(CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC, 0);
    CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING,
     info->ContentEncryptionAlgorithm.pszObjId, 0, (void **)&genKeyFunc, &hFunc);
    if (genKeyFunc)
    {
        ret = genKeyFunc(info, 0, NULL);
        CryptFreeOIDFunctionAddress(hFunc, 0);
    }
    else
        ret = CryptGenKey(info->hCryptProv, algID, CRYPT_EXPORTABLE,
         &info->hContentEncryptKey);
    return ret;
}

static BOOL WINAPI CRYPT_ExportKeyTrans(
 PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
 PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
 PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
 DWORD dwFlags, void *pvReserved)
{
    CERT_PUBLIC_KEY_INFO keyInfo;
    HCRYPTKEY expKey;
    BOOL ret;

    ret = CRYPT_ConstructAlgorithmId(&keyInfo.Algorithm,
        &pKeyTransEncodeInfo->KeyEncryptionAlgorithm);
    if (ret)
        ret = CRYPT_ConstructBitBlob(&keyInfo.PublicKey,
         &pKeyTransEncodeInfo->RecipientPublicKey);
    if (ret)
        ret = CryptImportPublicKeyInfo(pKeyTransEncodeInfo->hCryptProv,
         X509_ASN_ENCODING, &keyInfo, &expKey);
    if (ret)
    {
        DWORD size;

        ret = CryptExportKey(pContentEncryptInfo->hContentEncryptKey, expKey,
         SIMPLEBLOB, 0, NULL, &size);
        if (ret)
        {
            BYTE *keyBlob;

            keyBlob = CryptMemAlloc(size);
            if (keyBlob)
            {
                ret = CryptExportKey(pContentEncryptInfo->hContentEncryptKey,
                 expKey, SIMPLEBLOB, 0, keyBlob, &size);
                if (ret)
                {
                    DWORD head = sizeof(BLOBHEADER) + sizeof(ALG_ID);

                    pKeyTransEncryptInfo->EncryptedKey.pbData =
                     CryptMemAlloc(size - head);
                    if (pKeyTransEncryptInfo->EncryptedKey.pbData)
                    {
                        DWORD i, k = 0;

                        pKeyTransEncryptInfo->EncryptedKey.cbData = size - head;
                        for (i = size - 1; i >= head; --i, ++k)
                            pKeyTransEncryptInfo->EncryptedKey.pbData[k] =
                             keyBlob[i];
                    }
                    else
                        ret = FALSE;
                }
                CryptMemFree(keyBlob);
            }
            else
                ret = FALSE;
        }
        CryptDestroyKey(expKey);
    }

    CryptMemFree(keyInfo.PublicKey.pbData);
    CryptMemFree(keyInfo.Algorithm.pszObjId);
    CryptMemFree(keyInfo.Algorithm.Parameters.pbData);
    return ret;
}

static BOOL CRYPT_ExportEncryptedKey(CMSG_CONTENT_ENCRYPT_INFO *info, DWORD i,
 CRYPT_DATA_BLOB *key)
{
    static HCRYPTOIDFUNCSET set = NULL;
    PFN_CMSG_EXPORT_KEY_TRANS exportKeyFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO *encodeInfo =
     info->rgCmsRecipients[i].pKeyTrans;
    CMSG_KEY_TRANS_ENCRYPT_INFO encryptInfo;
    BOOL ret;

    memset(&encryptInfo, 0, sizeof(encryptInfo));
    encryptInfo.cbSize = sizeof(encryptInfo);
    encryptInfo.dwRecipientIndex = i;
    ret = CRYPT_ConstructAlgorithmId(&encryptInfo.KeyEncryptionAlgorithm,
     &encodeInfo->KeyEncryptionAlgorithm);

    if (!set)
        set = CryptInitOIDFunctionSet(CMSG_OID_EXPORT_KEY_TRANS_FUNC, 0);
    CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING,
     encryptInfo.KeyEncryptionAlgorithm.pszObjId, 0, (void **)&exportKeyFunc,
     &hFunc);
    if (!exportKeyFunc)
        exportKeyFunc = CRYPT_ExportKeyTrans;
    if (ret)
    {
        ret = exportKeyFunc(info, encodeInfo, &encryptInfo, 0, NULL);
        if (ret)
        {
            key->cbData = encryptInfo.EncryptedKey.cbData;
            key->pbData = encryptInfo.EncryptedKey.pbData;
        }
    }
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);

    CryptMemFree(encryptInfo.KeyEncryptionAlgorithm.pszObjId);
    CryptMemFree(encryptInfo.KeyEncryptionAlgorithm.Parameters.pbData);
    return ret;
}

static LPVOID WINAPI mem_alloc(size_t size)
{
    return CryptMemAlloc(size);
}

static VOID WINAPI mem_free(LPVOID pv)
{
    CryptMemFree(pv);
}


static BOOL CContentEncryptInfo_Construct(CMSG_CONTENT_ENCRYPT_INFO *info,
 const CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS *in, HCRYPTPROV prov)
{
    BOOL ret;

    info->cbSize = sizeof(CMSG_CONTENT_ENCRYPT_INFO);
    info->hCryptProv = prov;
    ret = CRYPT_ConstructAlgorithmId(&info->ContentEncryptionAlgorithm,
     &in->ContentEncryptionAlgorithm);
    info->pvEncryptionAuxInfo = in->pvEncryptionAuxInfo;
    info->cRecipients = in->cRecipients;
    if (ret)
    {
        info->rgCmsRecipients = CryptMemAlloc(in->cRecipients *
         sizeof(CMSG_RECIPIENT_ENCODE_INFO));
        if (info->rgCmsRecipients)
        {
            DWORD i;

            for (i = 0; ret && i < in->cRecipients; ++i)
            {
                CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO *encodeInfo;
                CERT_INFO *cert = in->rgpRecipientCert[i];

                info->rgCmsRecipients[i].dwRecipientChoice =
                 CMSG_KEY_TRANS_RECIPIENT;
                encodeInfo = CryptMemAlloc(sizeof(*encodeInfo));
                info->rgCmsRecipients[i].pKeyTrans = encodeInfo;
                if (encodeInfo)
                {
                    encodeInfo->cbSize = sizeof(*encodeInfo);
                    ret = CRYPT_ConstructAlgorithmId(
                     &encodeInfo->KeyEncryptionAlgorithm,
                     &cert->SubjectPublicKeyInfo.Algorithm);
                    encodeInfo->pvKeyEncryptionAuxInfo = NULL;
                    encodeInfo->hCryptProv = prov;
                    if (ret)
                        ret = CRYPT_ConstructBitBlob(
                         &encodeInfo->RecipientPublicKey,
                         &cert->SubjectPublicKeyInfo.PublicKey);
                    if (ret)
                        ret = CRYPT_ConstructBlob(
                         &encodeInfo->RecipientId.IssuerSerialNumber.Issuer,
                         &cert->Issuer);
                    if (ret)
                        ret = CRYPT_ConstructBlob(
                         &encodeInfo->RecipientId.IssuerSerialNumber.SerialNumber,
                         &cert->SerialNumber);
                }
                else
                    ret = FALSE;
            }
        }
        else
            ret = FALSE;
    }
    info->pfnAlloc = mem_alloc;
    info->pfnFree = mem_free;
    return ret;
}

static void CContentEncryptInfo_Free(CMSG_CONTENT_ENCRYPT_INFO *info)
{
    CryptMemFree(info->ContentEncryptionAlgorithm.pszObjId);
    CryptMemFree(info->ContentEncryptionAlgorithm.Parameters.pbData);
    if (info->rgCmsRecipients)
    {
        DWORD i;

        for (i = 0; i < info->cRecipients; ++i)
        {
            CMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO *encodeInfo =
             info->rgCmsRecipients[i].pKeyTrans;

            CryptMemFree(encodeInfo->KeyEncryptionAlgorithm.pszObjId);
            CryptMemFree(encodeInfo->KeyEncryptionAlgorithm.Parameters.pbData);
            CryptMemFree(encodeInfo->RecipientPublicKey.pbData);
            CryptMemFree(
             encodeInfo->RecipientId.IssuerSerialNumber.Issuer.pbData);
            CryptMemFree(
             encodeInfo->RecipientId.IssuerSerialNumber.SerialNumber.pbData);
            CryptMemFree(encodeInfo);
        }
        CryptMemFree(info->rgCmsRecipients);
    }
}

static BOOL CRecipientInfo_Construct(CMSG_KEY_TRANS_RECIPIENT_INFO *info,
 const CERT_INFO *cert, CRYPT_DATA_BLOB *key)
{
    BOOL ret;

    info->dwVersion = CMSG_KEY_TRANS_PKCS_1_5_VERSION;
    info->RecipientId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
    ret = CRYPT_ConstructBlob(&info->RecipientId.IssuerSerialNumber.Issuer,
     &cert->Issuer);
    if (ret)
        ret = CRYPT_ConstructBlob(
         &info->RecipientId.IssuerSerialNumber.SerialNumber,
         &cert->SerialNumber);
    if (ret)
        ret = CRYPT_ConstructAlgorithmId(&info->KeyEncryptionAlgorithm,
         &cert->SubjectPublicKeyInfo.Algorithm);
    info->EncryptedKey.cbData = key->cbData;
    info->EncryptedKey.pbData = key->pbData;
    return ret;
}

static void CRecipientInfo_Free(CMSG_KEY_TRANS_RECIPIENT_INFO *info)
{
    CryptMemFree(info->RecipientId.IssuerSerialNumber.Issuer.pbData);
    CryptMemFree(info->RecipientId.IssuerSerialNumber.SerialNumber.pbData);
    CryptMemFree(info->KeyEncryptionAlgorithm.pszObjId);
    CryptMemFree(info->KeyEncryptionAlgorithm.Parameters.pbData);
    CryptMemFree(info->EncryptedKey.pbData);
}

static void CEnvelopedEncodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CEnvelopedEncodeMsg *msg = hCryptMsg;

    CryptMemFree(msg->algo.pszObjId);
    CryptMemFree(msg->algo.Parameters.pbData);
    if (msg->base.open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        CryptReleaseContext(msg->prov, 0);
    CryptDestroyKey(msg->key);
    if (msg->recipientInfo)
    {
        DWORD i;

        for (i = 0; i < msg->cRecipientInfo; ++i)
            CRecipientInfo_Free(&msg->recipientInfo[i]);
        CryptMemFree(msg->recipientInfo);
    }
    CryptMemFree(msg->data.pbData);
}

static BOOL CEnvelopedEncodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CEnvelopedEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_BARE_CONTENT_PARAM:
        if (msg->base.streamed)
            SetLastError(E_INVALIDARG);
        else
        {
            char oid_rsa_data[] = szOID_RSA_data;
            CRYPT_ENVELOPED_DATA envelopedData = {
             CMSG_ENVELOPED_DATA_PKCS_1_5_VERSION, msg->cRecipientInfo,
             msg->recipientInfo, { oid_rsa_data, {
               msg->algo.pszObjId,
               { msg->algo.Parameters.cbData, msg->algo.Parameters.pbData }
              },
              { msg->data.cbData, msg->data.pbData }
             }
            };

            ret = CRYPT_AsnEncodePKCSEnvelopedData(&envelopedData, pvData,
             pcbData);
        }
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
                    char oid_rsa_enveloped[] = szOID_RSA_envelopedData;

                    info.pszObjId = oid_rsa_enveloped;
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
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CEnvelopedEncodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CEnvelopedEncodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    if (msg->base.state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else if (msg->base.streamed)
    {
        FIXME("streamed stub\n");
        msg->base.state = fFinal ? MsgStateFinalized : MsgStateUpdated;
        ret = TRUE;
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
            if (cbData)
            {
                DWORD dataLen = cbData;

                msg->data.cbData = cbData;
                msg->data.pbData = CryptMemAlloc(cbData);
                if (msg->data.pbData)
                {
                    memcpy(msg->data.pbData, pbData, cbData);
                    ret = CryptEncrypt(msg->key, 0, TRUE, 0, msg->data.pbData,
                     &dataLen, msg->data.cbData);
                    msg->data.cbData = dataLen;
                    if (dataLen > cbData)
                    {
                        msg->data.pbData = CryptMemRealloc(msg->data.pbData,
                         dataLen);
                        if (msg->data.pbData)
                        {
                            dataLen = cbData;
                            ret = CryptEncrypt(msg->key, 0, TRUE, 0,
                             msg->data.pbData, &dataLen, msg->data.cbData);
                        }
                        else
                            ret = FALSE;
                    }
                    if (!ret)
                        CryptMemFree(msg->data.pbData);
                }
                else
                    ret = FALSE;
                if (!ret)
                {
                    msg->data.cbData = 0;
                    msg->data.pbData = NULL;
                }
            }
            else
                ret = TRUE;
            msg->base.state = MsgStateFinalized;
        }
    }
    return ret;
}

static HCRYPTMSG CEnvelopedEncodeMsg_Open(DWORD dwFlags,
 const void *pvMsgEncodeInfo, LPCSTR pszInnerContentObjID,
 PCMSG_STREAM_INFO pStreamInfo)
{
    CEnvelopedEncodeMsg *msg;
    const CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS *info = pvMsgEncodeInfo;
    HCRYPTPROV prov;
    ALG_ID algID;

    if (info->cbSize != sizeof(CMSG_ENVELOPED_ENCODE_INFO) &&
     info->cbSize != sizeof(CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS))
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (info->cbSize == sizeof(CMSG_ENVELOPED_ENCODE_INFO_WITH_CMS))
        FIXME("CMS fields unsupported\n");
    if (!(algID = CertOIDToAlgId(info->ContentEncryptionAlgorithm.pszObjId)))
    {
        SetLastError(CRYPT_E_UNKNOWN_ALGO);
        return NULL;
    }
    if (info->cRecipients && !info->rgpRecipientCert)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (info->hCryptProv)
        prov = info->hCryptProv;
    else
    {
        prov = I_CryptGetDefaultCryptProv(0);
        dwFlags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
    }
    msg = CryptMemAlloc(sizeof(CEnvelopedEncodeMsg));
    if (msg)
    {
        CRYPT_DATA_BLOB encryptedKey = { 0, NULL };
        CMSG_CONTENT_ENCRYPT_INFO encryptInfo;
        BOOL ret;
        DWORD i;

        CryptMsgBase_Init((CryptMsgBase *)msg, dwFlags, pStreamInfo,
         CEnvelopedEncodeMsg_Close, CEnvelopedEncodeMsg_GetParam,
         CEnvelopedEncodeMsg_Update, CRYPT_DefaultMsgControl);
        ret = CRYPT_ConstructAlgorithmId(&msg->algo,
         &info->ContentEncryptionAlgorithm);
        msg->prov = prov;
        msg->data.cbData = 0;
        msg->data.pbData = NULL;
        msg->cRecipientInfo = info->cRecipients;
        msg->recipientInfo = CryptMemAlloc(info->cRecipients *
         sizeof(CMSG_KEY_TRANS_RECIPIENT_INFO));
        if (!msg->recipientInfo)
            ret = FALSE;
        memset(&encryptInfo, 0, sizeof(encryptInfo));
        if (ret)
        {
            ret = CContentEncryptInfo_Construct(&encryptInfo, info, prov);
            if (ret)
            {
                ret = CRYPT_GenKey(&encryptInfo, algID);
                if (ret)
                    msg->key = encryptInfo.hContentEncryptKey;
            }
        }
        for (i = 0; ret && i < msg->cRecipientInfo; ++i)
        {
            ret = CRYPT_ExportEncryptedKey(&encryptInfo, i, &encryptedKey);
            if (ret)
                ret = CRecipientInfo_Construct(&msg->recipientInfo[i],
                 info->rgpRecipientCert[i], &encryptedKey);
        }
        CContentEncryptInfo_Free(&encryptInfo);
        if (!ret)
        {
            CryptMsgClose(msg);
            msg = NULL;
        }
    }
    if (!msg && (dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG))
        CryptReleaseContext(prov, 0);
    return msg;
}

HCRYPTMSG WINAPI CryptMsgOpenToEncode(DWORD dwMsgEncodingType, DWORD dwFlags,
 DWORD dwMsgType, const void *pvMsgEncodeInfo, LPSTR pszInnerContentObjID,
 PCMSG_STREAM_INFO pStreamInfo)
{
    HCRYPTMSG msg = NULL;

    TRACE("(%08lx, %08lx, %08lx, %p, %s, %p)\n", dwMsgEncodingType, dwFlags,
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
        msg = CEnvelopedEncodeMsg_Open(dwFlags, pvMsgEncodeInfo,
         pszInnerContentObjID, pStreamInfo);
        break;
    case CMSG_SIGNED_AND_ENVELOPED:
    case CMSG_ENCRYPTED:
        /* defined but invalid, fall through */
    default:
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return msg;
}

typedef struct _CEnvelopedDecodeMsg
{
    CRYPT_ENVELOPED_DATA *data;
    HCRYPTPROV            crypt_prov;
    CRYPT_DATA_BLOB       content;
    BOOL                  decrypted;
} CEnvelopedDecodeMsg;

typedef struct _CDecodeMsg
{
    CryptMsgBase           base;
    DWORD                  type;
    HCRYPTPROV             crypt_prov;
    union {
        HCRYPTHASH          hash;
        CSignedMsgData      signed_data;
        CEnvelopedDecodeMsg enveloped_data;
    } u;
    CRYPT_DATA_BLOB        msg_data;
    CRYPT_DATA_BLOB        detached_data;
    CONTEXT_PROPERTY_LIST *properties;
} CDecodeMsg;

static void CDecodeMsg_Close(HCRYPTMSG hCryptMsg)
{
    CDecodeMsg *msg = hCryptMsg;

    if (msg->crypt_prov && msg->base.open_flags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG)
        CryptReleaseContext(msg->crypt_prov, 0);
    switch (msg->type)
    {
    case CMSG_HASHED:
        if (msg->u.hash)
            CryptDestroyHash(msg->u.hash);
        break;
    case CMSG_ENVELOPED:
        if (msg->u.enveloped_data.crypt_prov)
            CryptReleaseContext(msg->u.enveloped_data.crypt_prov, 0);
        LocalFree(msg->u.enveloped_data.data);
        CryptMemFree(msg->u.enveloped_data.content.pbData);
        break;
    case CMSG_SIGNED:
        if (msg->u.signed_data.info)
        {
            LocalFree(msg->u.signed_data.info);
            CSignedMsgData_CloseHandles(&msg->u.signed_data);
        }
        break;
    }
    CryptMemFree(msg->msg_data.pbData);
    CryptMemFree(msg->detached_data.pbData);
    ContextPropertyList_Free(msg->properties);
}

static BOOL CDecodeMsg_CopyData(CRYPT_DATA_BLOB *blob, const BYTE *pbData,
 DWORD cbData)
{
    BOOL ret = TRUE;

    if (cbData)
    {
        if (blob->cbData)
            blob->pbData = CryptMemRealloc(blob->pbData,
             blob->cbData + cbData);
        else
            blob->pbData = CryptMemAlloc(cbData);
        if (blob->pbData)
        {
            memcpy(blob->pbData + blob->cbData, pbData, cbData);
            blob->cbData += cbData;
        }
        else
            ret = FALSE;
    }
    return ret;
}

static BOOL CDecodeMsg_DecodeDataContent(CDecodeMsg *msg, const CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_DATA_BLOB *data;
    DWORD size;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
     blob->pbData, blob->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &data, &size);
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
 const CRYPT_DER_BLOB *blob)
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
        if (!(msg->base.open_flags & CMSG_DETACHED_FLAG))
        {
            if (digestedData->ContentInfo.Content.cbData)
                CDecodeMsg_DecodeDataContent(msg,
                 &digestedData->ContentInfo.Content);
            else
                ContextPropertyList_SetProperty(msg->properties,
                 CMSG_CONTENT_PARAM, NULL, 0);
        }
        ContextPropertyList_SetProperty(msg->properties, CMSG_HASH_DATA_PARAM,
         digestedData->hash.pbData, digestedData->hash.cbData);
        LocalFree(digestedData);
    }
    return ret;
}

static BOOL CDecodeMsg_DecodeEnvelopedContent(CDecodeMsg *msg,
 const CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_ENVELOPED_DATA *envelopedData;
    DWORD size;

    ret = CRYPT_AsnDecodePKCSEnvelopedData(blob->pbData, blob->cbData,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (CRYPT_ENVELOPED_DATA *)&envelopedData,
     &size);
    if (ret)
        msg->u.enveloped_data.data = envelopedData;
    return ret;
}

static BOOL CDecodeMsg_DecodeSignedContent(CDecodeMsg *msg,
 const CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    CRYPT_SIGNED_INFO *signedInfo;
    DWORD size;

    ret = CRYPT_AsnDecodeCMSSignedInfo(blob->pbData, blob->cbData,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (CRYPT_SIGNED_INFO *)&signedInfo,
     &size);
    if (ret)
        msg->u.signed_data.info = signedInfo;
    return ret;
}

/* Decodes the content in blob as the type given, and updates the value
 * (type, parameters, etc.) of msg based on what blob contains.
 * It doesn't just use msg's type, to allow a recursive call from an implicitly
 * typed message once the outer content info has been decoded.
 */
static BOOL CDecodeMsg_DecodeContent(CDecodeMsg *msg, const CRYPT_DER_BLOB *blob,
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
        if ((ret = CDecodeMsg_DecodeEnvelopedContent(msg, blob)))
            msg->type = CMSG_ENVELOPED;
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
         NULL, &info, &size);
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

static BOOL CDecodeMsg_FinalizeHashedContent(CDecodeMsg *msg,
 CRYPT_DER_BLOB *blob)
{
    CRYPT_ALGORITHM_IDENTIFIER *hashAlgoID = NULL;
    DWORD size = 0;
    ALG_ID algID = 0;
    BOOL ret;

    CryptMsgGetParam(msg, CMSG_HASH_ALGORITHM_PARAM, 0, NULL, &size);
    hashAlgoID = CryptMemAlloc(size);
    ret = CryptMsgGetParam(msg, CMSG_HASH_ALGORITHM_PARAM, 0, hashAlgoID,
     &size);
    if (ret)
        algID = CertOIDToAlgId(hashAlgoID->pszObjId);

    if (!msg->crypt_prov)
    {
        msg->crypt_prov = I_CryptGetDefaultCryptProv(algID);
        if (msg->crypt_prov)
            msg->base.open_flags &= ~CMSG_CRYPT_RELEASE_CONTEXT_FLAG;
    }

    ret = CryptCreateHash(msg->crypt_prov, algID, 0, 0, &msg->u.hash);
    if (ret)
    {
        CRYPT_DATA_BLOB content;

        if (msg->base.open_flags & CMSG_DETACHED_FLAG)
        {
            /* Unlike for non-detached messages, the data were never stored as
             * the content param, but were saved in msg->detached_data instead.
             */
            content.pbData = msg->detached_data.pbData;
            content.cbData = msg->detached_data.cbData;
        }
        else
            ret = ContextPropertyList_FindProperty(msg->properties,
             CMSG_CONTENT_PARAM, &content);
        if (ret)
            ret = CryptHashData(msg->u.hash, content.pbData, content.cbData, 0);
    }
    CryptMemFree(hashAlgoID);
    return ret;
}

static BOOL CDecodeMsg_FinalizeEnvelopedContent(CDecodeMsg *msg,
 CRYPT_DER_BLOB *blob)
{
    CRYPT_DATA_BLOB *content;

    if (msg->base.open_flags & CMSG_DETACHED_FLAG)
        content = &msg->detached_data;
    else
        content =
         &msg->u.enveloped_data.data->encryptedContentInfo.encryptedContent;

    return CRYPT_ConstructBlob(&msg->u.enveloped_data.content, content);
}

static BOOL CDecodeMsg_FinalizeSignedContent(CDecodeMsg *msg,
 CRYPT_DER_BLOB *blob)
{
    BOOL ret;
    DWORD i, size;

    ret = CSignedMsgData_AllocateHandles(&msg->u.signed_data);
    for (i = 0; ret && i < msg->u.signed_data.info->cSignerInfo; i++)
        ret = CSignedMsgData_ConstructSignerHandles(&msg->u.signed_data, i,
         &msg->crypt_prov, &msg->base.open_flags);
    if (ret)
    {
        CRYPT_DATA_BLOB *content;

        /* Now that we have all the content, update the hash handles with
         * it.  If the message is a detached message, the content is stored
         * in msg->detached_data rather than in the signed message's
         * content.
         */
        if (msg->base.open_flags & CMSG_DETACHED_FLAG)
            content = &msg->detached_data;
        else
            content = &msg->u.signed_data.info->content.Content;
        if (content->cbData)
        {
            /* If the message is not detached, have to decode the message's
             * content if the type is szOID_RSA_data.
             */
            if (!(msg->base.open_flags & CMSG_DETACHED_FLAG) &&
             !strcmp(msg->u.signed_data.info->content.pszObjId,
             szOID_RSA_data))
            {
                CRYPT_DATA_BLOB *rsa_blob;

                ret = CryptDecodeObjectEx(X509_ASN_ENCODING,
                 X509_OCTET_STRING, content->pbData, content->cbData,
                 CRYPT_DECODE_ALLOC_FLAG, NULL, &rsa_blob, &size);
                if (ret)
                {
                    ret = CSignedMsgData_Update(&msg->u.signed_data,
                     rsa_blob->pbData, rsa_blob->cbData, TRUE, Verify);
                    LocalFree(rsa_blob);
                }
            }
            else
                ret = CSignedMsgData_Update(&msg->u.signed_data,
                 content->pbData, content->cbData, TRUE, Verify);
        }
    }
    return ret;
}

static BOOL CDecodeMsg_FinalizeContent(CDecodeMsg *msg, CRYPT_DER_BLOB *blob)
{
    BOOL ret = FALSE;

    switch (msg->type)
    {
    case CMSG_HASHED:
        ret = CDecodeMsg_FinalizeHashedContent(msg, blob);
        break;
    case CMSG_ENVELOPED:
        ret = CDecodeMsg_FinalizeEnvelopedContent(msg, blob);
        break;
    case CMSG_SIGNED:
        ret = CDecodeMsg_FinalizeSignedContent(msg, blob);
        break;
    default:
        ret = TRUE;
    }
    return ret;
}

static BOOL CDecodeMsg_Update(HCRYPTMSG hCryptMsg, const BYTE *pbData,
 DWORD cbData, BOOL fFinal)
{
    CDecodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %ld, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    if (msg->base.state == MsgStateFinalized)
        SetLastError(CRYPT_E_MSG_ERROR);
    else if (msg->base.streamed)
    {
        FIXME("(%p, %p, %ld, %d): streamed update stub\n", hCryptMsg, pbData,
         cbData, fFinal);
        switch (msg->base.state)
        {
        case MsgStateInit:
            ret = CDecodeMsg_CopyData(&msg->msg_data, pbData, cbData);
            if (fFinal)
            {
                if (msg->base.open_flags & CMSG_DETACHED_FLAG)
                    msg->base.state = MsgStateDataFinalized;
                else
                    msg->base.state = MsgStateFinalized;
            }
            else
                msg->base.state = MsgStateUpdated;
            break;
        case MsgStateUpdated:
            ret = CDecodeMsg_CopyData(&msg->msg_data, pbData, cbData);
            if (fFinal)
            {
                if (msg->base.open_flags & CMSG_DETACHED_FLAG)
                    msg->base.state = MsgStateDataFinalized;
                else
                    msg->base.state = MsgStateFinalized;
            }
            break;
        case MsgStateDataFinalized:
            ret = CDecodeMsg_CopyData(&msg->detached_data, pbData, cbData);
            if (fFinal)
                msg->base.state = MsgStateFinalized;
            break;
        default:
            SetLastError(CRYPT_E_MSG_ERROR);
            break;
        }
    }
    else
    {
        if (!fFinal)
            SetLastError(CRYPT_E_MSG_ERROR);
        else
        {
            switch (msg->base.state)
            {
            case MsgStateInit:
                ret = CDecodeMsg_CopyData(&msg->msg_data, pbData, cbData);
                if (msg->base.open_flags & CMSG_DETACHED_FLAG)
                    msg->base.state = MsgStateDataFinalized;
                else
                    msg->base.state = MsgStateFinalized;
                break;
            case MsgStateDataFinalized:
                ret = CDecodeMsg_CopyData(&msg->detached_data, pbData, cbData);
                msg->base.state = MsgStateFinalized;
                break;
            default:
                SetLastError(CRYPT_E_MSG_ERROR);
            }
        }
    }
    if (ret && fFinal &&
     ((msg->base.open_flags & CMSG_DETACHED_FLAG && msg->base.state ==
     MsgStateDataFinalized) ||
     (!(msg->base.open_flags & CMSG_DETACHED_FLAG) && msg->base.state ==
     MsgStateFinalized)))
        ret = CDecodeMsg_DecodeContent(msg, &msg->msg_data, msg->type);
    if (ret && msg->base.state == MsgStateFinalized)
        ret = CDecodeMsg_FinalizeContent(msg, &msg->msg_data);
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
                CRYPT_FixUpAlgorithmID(pvData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    }
    case CMSG_COMPUTED_HASH_PARAM:
        ret = CryptGetHashParam(msg->u.hash, HP_HASHVAL, pvData, pcbData, 0);
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

        *nextData = POINTER_ALIGN_DWORD_PTR(*nextData);
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
                *nextData = POINTER_ALIGN_DWORD_PTR(*nextData);
                out->rgAttr[i].rgValue = (PCRYPT_DATA_BLOB)*nextData;
                *nextData += in->rgAttr[i].cValue * sizeof(CRYPT_DATA_BLOB);
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
        size = ALIGN_DWORD_PTR(size);
        size += attr->rgAttr[i].cValue * sizeof(CRYPT_DATA_BLOB);
        for (j = 0; j < attr->rgAttr[i].cValue; j++)
            size += attr->rgAttr[i].rgValue[j].cbData;
    }
    /* align pointer again to be conservative */
    size = ALIGN_DWORD_PTR(size);
    return size;
}

static DWORD CRYPT_SizeOfKeyIdAsIssuerAndSerial(const CRYPT_DATA_BLOB *keyId)
{
    static char oid_key_rdn[] = szOID_KEYID_RDN;
    DWORD size = 0;
    CERT_RDN_ATTR attr;
    CERT_RDN rdn = { 1, &attr };
    CERT_NAME_INFO name = { 1, &rdn };

    attr.pszObjId = oid_key_rdn;
    attr.dwValueType = CERT_RDN_OCTET_STRING;
    attr.Value.cbData = keyId->cbData;
    attr.Value.pbData = keyId->pbData;
    if (CryptEncodeObject(X509_ASN_ENCODING, X509_NAME, &name, NULL, &size))
        size++; /* Only include size of special zero serial number on success */
    return size;
}

static BOOL CRYPT_CopyKeyIdAsIssuerAndSerial(CERT_NAME_BLOB *issuer,
 CRYPT_INTEGER_BLOB *serialNumber, const CRYPT_DATA_BLOB *keyId, DWORD encodedLen,
 LPBYTE *nextData)
{
    static char oid_key_rdn[] = szOID_KEYID_RDN;
    CERT_RDN_ATTR attr;
    CERT_RDN rdn = { 1, &attr };
    CERT_NAME_INFO name = { 1, &rdn };
    BOOL ret;

    /* Encode special zero serial number */
    serialNumber->cbData = 1;
    serialNumber->pbData = *nextData;
    **nextData = 0;
    (*nextData)++;
    /* Encode issuer */
    issuer->pbData = *nextData;
    attr.pszObjId = oid_key_rdn;
    attr.dwValueType = CERT_RDN_OCTET_STRING;
    attr.Value.cbData = keyId->cbData;
    attr.Value.pbData = keyId->pbData;
    ret = CryptEncodeObject(X509_ASN_ENCODING, X509_NAME, &name, *nextData,
     &encodedLen);
    if (ret)
    {
        *nextData += encodedLen;
        issuer->cbData = encodedLen;
    }
    return ret;
}

static BOOL CRYPT_CopySignerInfo(void *pvData, DWORD *pcbData,
 const CMSG_CMS_SIGNER_INFO *in)
{
    DWORD size = sizeof(CMSG_SIGNER_INFO), rdnSize = 0;
    BOOL ret;

    TRACE("(%p, %ld, %p)\n", pvData, pvData ? *pcbData : 0, in);

    if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
    {
        size += in->SignerId.IssuerSerialNumber.Issuer.cbData;
        size += in->SignerId.IssuerSerialNumber.SerialNumber.cbData;
    }
    else
    {
        rdnSize = CRYPT_SizeOfKeyIdAsIssuerAndSerial(&in->SignerId.KeyId);
        size += rdnSize;
    }
    if (in->HashAlgorithm.pszObjId)
        size += strlen(in->HashAlgorithm.pszObjId) + 1;
    size += in->HashAlgorithm.Parameters.cbData;
    if (in->HashEncryptionAlgorithm.pszObjId)
        size += strlen(in->HashEncryptionAlgorithm.pszObjId) + 1;
    size += in->HashEncryptionAlgorithm.Parameters.cbData;
    size += in->EncryptedHash.cbData;
    /* align pointer */
    size = ALIGN_DWORD_PTR(size);
    size += CRYPT_SizeOfAttributes(&in->AuthAttrs);
    size += CRYPT_SizeOfAttributes(&in->UnauthAttrs);
    if (!pvData)
    {
        ret = TRUE;
    }
    else if (*pcbData < size)
    {
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        LPBYTE nextData = (BYTE *)pvData + sizeof(CMSG_SIGNER_INFO);
        CMSG_SIGNER_INFO *out = pvData;

        ret = TRUE;
        out->dwVersion = in->dwVersion;
        if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
        {
            CRYPT_CopyBlob(&out->Issuer,
             &in->SignerId.IssuerSerialNumber.Issuer, &nextData);
            CRYPT_CopyBlob(&out->SerialNumber,
             &in->SignerId.IssuerSerialNumber.SerialNumber, &nextData);
        }
        else
            ret = CRYPT_CopyKeyIdAsIssuerAndSerial(&out->Issuer, &out->SerialNumber,
             &in->SignerId.KeyId, rdnSize, &nextData);
        if (ret)
        {
            CRYPT_CopyAlgorithmId(&out->HashAlgorithm, &in->HashAlgorithm,
             &nextData);
            CRYPT_CopyAlgorithmId(&out->HashEncryptionAlgorithm,
             &in->HashEncryptionAlgorithm, &nextData);
            CRYPT_CopyBlob(&out->EncryptedHash, &in->EncryptedHash, &nextData);
            nextData = POINTER_ALIGN_DWORD_PTR(nextData);
            CRYPT_CopyAttributes(&out->AuthAttrs, &in->AuthAttrs, &nextData);
            CRYPT_CopyAttributes(&out->UnauthAttrs, &in->UnauthAttrs, &nextData);
        }
    }
    *pcbData = size;
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_CopyCMSSignerInfo(void *pvData, DWORD *pcbData,
 const CMSG_CMS_SIGNER_INFO *in)
{
    DWORD size = sizeof(CMSG_CMS_SIGNER_INFO);
    BOOL ret;

    TRACE("(%p, %ld, %p)\n", pvData, pvData ? *pcbData : 0, in);

    if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
    {
        size += in->SignerId.IssuerSerialNumber.Issuer.cbData;
        size += in->SignerId.IssuerSerialNumber.SerialNumber.cbData;
    }
    else
        size += in->SignerId.KeyId.cbData;
    if (in->HashAlgorithm.pszObjId)
        size += strlen(in->HashAlgorithm.pszObjId) + 1;
    size += in->HashAlgorithm.Parameters.cbData;
    if (in->HashEncryptionAlgorithm.pszObjId)
        size += strlen(in->HashEncryptionAlgorithm.pszObjId) + 1;
    size += in->HashEncryptionAlgorithm.Parameters.cbData;
    size += in->EncryptedHash.cbData;
    /* align pointer */
    size = ALIGN_DWORD_PTR(size);
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
        LPBYTE nextData = (BYTE *)pvData + sizeof(CMSG_CMS_SIGNER_INFO);
        CMSG_CMS_SIGNER_INFO *out = pvData;

        out->dwVersion = in->dwVersion;
        out->SignerId.dwIdChoice = in->SignerId.dwIdChoice;
        if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
        {
            CRYPT_CopyBlob(&out->SignerId.IssuerSerialNumber.Issuer,
             &in->SignerId.IssuerSerialNumber.Issuer, &nextData);
            CRYPT_CopyBlob(&out->SignerId.IssuerSerialNumber.SerialNumber,
             &in->SignerId.IssuerSerialNumber.SerialNumber, &nextData);
        }
        else
            CRYPT_CopyBlob(&out->SignerId.KeyId, &in->SignerId.KeyId, &nextData);
        CRYPT_CopyAlgorithmId(&out->HashAlgorithm, &in->HashAlgorithm,
         &nextData);
        CRYPT_CopyAlgorithmId(&out->HashEncryptionAlgorithm,
         &in->HashEncryptionAlgorithm, &nextData);
        CRYPT_CopyBlob(&out->EncryptedHash, &in->EncryptedHash, &nextData);
        nextData = POINTER_ALIGN_DWORD_PTR(nextData);
        CRYPT_CopyAttributes(&out->AuthAttrs, &in->AuthAttrs, &nextData);
        CRYPT_CopyAttributes(&out->UnauthAttrs, &in->UnauthAttrs, &nextData);
        ret = TRUE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_CopySignerCertInfo(void *pvData, DWORD *pcbData,
 const CMSG_CMS_SIGNER_INFO *in)
{
    DWORD size = sizeof(CERT_INFO), rdnSize = 0;
    BOOL ret;

    TRACE("(%p, %ld, %p)\n", pvData, pvData ? *pcbData : 0, in);

    if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
    {
        size += in->SignerId.IssuerSerialNumber.Issuer.cbData;
        size += in->SignerId.IssuerSerialNumber.SerialNumber.cbData;
    }
    else
    {
        rdnSize = CRYPT_SizeOfKeyIdAsIssuerAndSerial(&in->SignerId.KeyId);
        size += rdnSize;
    }
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
        CERT_INFO *out = pvData;

        memset(out, 0, sizeof(CERT_INFO));
        if (in->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
        {
            CRYPT_CopyBlob(&out->Issuer,
             &in->SignerId.IssuerSerialNumber.Issuer, &nextData);
            CRYPT_CopyBlob(&out->SerialNumber,
             &in->SignerId.IssuerSerialNumber.SerialNumber, &nextData);
            ret = TRUE;
        }
        else
            ret = CRYPT_CopyKeyIdAsIssuerAndSerial(&out->Issuer, &out->SerialNumber,
             &in->SignerId.KeyId, rdnSize, &nextData);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CRYPT_CopyRecipientInfo(void *pvData, DWORD *pcbData,
 const CERT_ISSUER_SERIAL_NUMBER *in)
{
    DWORD size = sizeof(CERT_INFO);
    BOOL ret;

    TRACE("(%p, %ld, %p)\n", pvData, pvData ? *pcbData : 0, in);

    size += in->SerialNumber.cbData;
    size += in->Issuer.cbData;
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
        CERT_INFO *out = pvData;

        CRYPT_CopyBlob(&out->SerialNumber, &in->SerialNumber, &nextData);
        CRYPT_CopyBlob(&out->Issuer, &in->Issuer, &nextData);
        ret = TRUE;
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL CDecodeEnvelopedMsg_GetParam(CDecodeMsg *msg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    BOOL ret = FALSE;

    switch (dwParamType)
    {
    case CMSG_TYPE_PARAM:
        ret = CRYPT_CopyParam(pvData, pcbData, &msg->type, sizeof(msg->type));
        break;
    case CMSG_CONTENT_PARAM:
        if (msg->u.enveloped_data.data)
            ret = CRYPT_CopyParam(pvData, pcbData,
             msg->u.enveloped_data.content.pbData,
             msg->u.enveloped_data.content.cbData);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_RECIPIENT_COUNT_PARAM:
        if (msg->u.enveloped_data.data)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.enveloped_data.data->cRecipientInfo, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_RECIPIENT_INFO_PARAM:
        if (msg->u.enveloped_data.data)
        {
            if (dwIndex < msg->u.enveloped_data.data->cRecipientInfo)
            {
                PCMSG_KEY_TRANS_RECIPIENT_INFO recipientInfo =
                 &msg->u.enveloped_data.data->rgRecipientInfo[dwIndex];

                ret = CRYPT_CopyRecipientInfo(pvData, pcbData,
                 &recipientInfo->RecipientId.IssuerSerialNumber);
            }
            else
                SetLastError(CRYPT_E_INVALID_INDEX);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    default:
        FIXME("unimplemented for %ld\n", dwParamType);
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CRYPT_CopyAttr(void *pvData, DWORD *pcbData, const CRYPT_ATTRIBUTES *attr)
{
    DWORD size;
    BOOL ret;

    TRACE("(%p, %ld, %p)\n", pvData, pvData ? *pcbData : 0, attr);

    size = CRYPT_SizeOfAttributes(attr);
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
        ret = CRYPT_ConstructAttributes(pvData, attr);

    TRACE("returning %d\n", ret);
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
        if (msg->u.signed_data.info)
        {
            if (!strcmp(msg->u.signed_data.info->content.pszObjId,
             szOID_RSA_data))
            {
                CRYPT_DATA_BLOB *blob;
                DWORD size;

                ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_OCTET_STRING,
                 msg->u.signed_data.info->content.Content.pbData,
                 msg->u.signed_data.info->content.Content.cbData,
                 CRYPT_DECODE_ALLOC_FLAG, NULL, &blob, &size);
                if (ret)
                {
                    ret = CRYPT_CopyParam(pvData, pcbData, blob->pbData,
                     blob->cbData);
                    LocalFree(blob);
                }
            }
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 msg->u.signed_data.info->content.Content.pbData,
                 msg->u.signed_data.info->content.Content.cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_INNER_CONTENT_TYPE_PARAM:
        if (msg->u.signed_data.info)
            ret = CRYPT_CopyParam(pvData, pcbData,
             msg->u.signed_data.info->content.pszObjId,
             strlen(msg->u.signed_data.info->content.pszObjId) + 1);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_COUNT_PARAM:
        if (msg->u.signed_data.info)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signed_data.info->cSignerInfo, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_INFO_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopySignerInfo(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex]);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_CERT_INFO_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopySignerCertInfo(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex]);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CERT_COUNT_PARAM:
        if (msg->u.signed_data.info)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signed_data.info->cCertEncoded, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CERT_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cCertEncoded)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 msg->u.signed_data.info->rgCertEncoded[dwIndex].pbData,
                 msg->u.signed_data.info->rgCertEncoded[dwIndex].cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CRL_COUNT_PARAM:
        if (msg->u.signed_data.info)
            ret = CRYPT_CopyParam(pvData, pcbData,
             &msg->u.signed_data.info->cCrlEncoded, sizeof(DWORD));
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CRL_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cCrlEncoded)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 msg->u.signed_data.info->rgCrlEncoded[dwIndex].pbData,
                 msg->u.signed_data.info->rgCrlEncoded[dwIndex].cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_COMPUTED_HASH_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.cSignerHandle)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CryptGetHashParam(
                 msg->u.signed_data.signerHandles[dwIndex].contentHash,
                 HP_HASHVAL, pvData, pcbData, 0);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ENCODED_MESSAGE:
        if (msg->msg_data.pbData)
            ret = CRYPT_CopyParam(pvData, pcbData, msg->msg_data.pbData, msg->msg_data.cbData);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ENCODED_SIGNER:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CryptEncodeObjectEx(
                 X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CMS_SIGNER_INFO,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex], 0, NULL,
                 pvData, pcbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ATTR_CERT_COUNT_PARAM:
        if (msg->u.signed_data.info)
        {
            DWORD attrCertCount = 0;

            ret = CRYPT_CopyParam(pvData, pcbData,
             &attrCertCount, sizeof(DWORD));
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ATTR_CERT_PARAM:
        if (msg->u.signed_data.info)
            SetLastError(CRYPT_E_INVALID_INDEX);
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_CMS_SIGNER_INFO_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyCMSSignerInfo(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex]);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_AUTH_ATTR_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyAttr(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex].AuthAttrs);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_SIGNER_UNAUTH_ATTR_PARAM:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyAttr(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex].UnauthAttrs);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    case CMSG_ENCRYPTED_DIGEST:
        if (msg->u.signed_data.info)
        {
            if (dwIndex >= msg->u.signed_data.info->cSignerInfo)
                SetLastError(CRYPT_E_INVALID_INDEX);
            else
                ret = CRYPT_CopyParam(pvData, pcbData,
                 &msg->u.signed_data.info->rgSignerInfo[dwIndex].EncryptedHash.pbData,
                 msg->u.signed_data.info->rgSignerInfo[dwIndex].EncryptedHash.cbData);
        }
        else
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        break;
    default:
        FIXME("unimplemented for %ld\n", dwParamType);
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    }
    return ret;
}

static BOOL CDecodeMsg_GetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CDecodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    switch (msg->type)
    {
    case CMSG_HASHED:
        ret = CDecodeHashMsg_GetParam(msg, dwParamType, dwIndex, pvData,
         pcbData);
        break;
    case CMSG_ENVELOPED:
        ret = CDecodeEnvelopedMsg_GetParam(msg, dwParamType, dwIndex, pvData,
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

static BOOL CDecodeHashMsg_VerifyHash(CDecodeMsg *msg)
{
    BOOL ret;
    CRYPT_DATA_BLOB hashBlob;

    ret = ContextPropertyList_FindProperty(msg->properties,
     CMSG_HASH_DATA_PARAM, &hashBlob);
    if (ret)
    {
        DWORD computedHashSize = 0;

        ret = CDecodeHashMsg_GetParam(msg, CMSG_COMPUTED_HASH_PARAM, 0, NULL,
         &computedHashSize);
        if (hashBlob.cbData == computedHashSize)
        {
            LPBYTE computedHash = CryptMemAlloc(computedHashSize);

            if (computedHash)
            {
                ret = CDecodeHashMsg_GetParam(msg, CMSG_COMPUTED_HASH_PARAM, 0,
                 computedHash, &computedHashSize);
                if (ret)
                {
                    if (memcmp(hashBlob.pbData, computedHash, hashBlob.cbData))
                    {
                        SetLastError(CRYPT_E_HASH_VALUE);
                        ret = FALSE;
                    }
                }
                CryptMemFree(computedHash);
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                ret = FALSE;
            }
        }
        else
        {
            SetLastError(CRYPT_E_HASH_VALUE);
            ret = FALSE;
        }
    }
    return ret;
}

static BOOL cng_verify_msg_signature(CMSG_CMS_SIGNER_INFO *signer, HCRYPTHASH hash, CERT_PUBLIC_KEY_INFO *key_info)
{
    BYTE *hash_value, *sig_value = NULL;
    DWORD hash_len, sig_len;
    BCRYPT_KEY_HANDLE key;
    BOOL ret = FALSE;
    NTSTATUS status;

    if (!CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, key_info, 0, NULL, &key)) return FALSE;
    if (!extract_hash(hash, &hash_value, &hash_len)) goto done;
    if (!cng_prepare_signature(key_info->Algorithm.pszObjId, signer->EncryptedHash.pbData,
            signer->EncryptedHash.cbData, &sig_value, &sig_len)) goto done;
    status = BCryptVerifySignature(key, NULL, hash_value, hash_len, sig_value, sig_len, 0);
    if (status)
    {
        FIXME("Failed to verify signature: %08lx.\n", status);
        SetLastError(RtlNtStatusToDosError(status));
    }
    ret = !status;
done:
    CryptMemFree(sig_value);
    CryptMemFree(hash_value);
    BCryptDestroyKey(key);
    return ret;
}

static BOOL CDecodeSignedMsg_VerifySignatureWithKey(CDecodeMsg *msg,
 HCRYPTPROV prov, DWORD signerIndex, PCERT_PUBLIC_KEY_INFO keyInfo)
{
    HCRYPTHASH hash;
    HCRYPTKEY key;
    BOOL ret;
    ALG_ID alg_id = 0;

    if (msg->u.signed_data.info->rgSignerInfo[signerIndex].AuthAttrs.cAttr)
        hash = msg->u.signed_data.signerHandles[signerIndex].authAttrHash;
    else
        hash = msg->u.signed_data.signerHandles[signerIndex].contentHash;

    if (keyInfo->Algorithm.pszObjId) alg_id = CertOIDToAlgId(keyInfo->Algorithm.pszObjId);
    if (alg_id == CALG_OID_INFO_PARAMETERS || alg_id == CALG_OID_INFO_CNG_ONLY)
        return cng_verify_msg_signature(&msg->u.signed_data.info->rgSignerInfo[signerIndex], hash, keyInfo);

    if (!prov)
        prov = msg->crypt_prov;
    ret = CryptImportPublicKeyInfo(prov, X509_ASN_ENCODING, keyInfo, &key);
    if (ret)
    {
        CRYPT_HASH_BLOB reversedHash;

        ret = CRYPT_ConstructBlob(&reversedHash,
         &msg->u.signed_data.info->rgSignerInfo[signerIndex].EncryptedHash);
        if (ret)
        {
            CRYPT_ReverseBytes(&reversedHash);
            ret = CryptVerifySignatureW(hash, reversedHash.pbData,
             reversedHash.cbData, key, NULL, 0);
            CryptMemFree(reversedHash.pbData);
        }
        CryptDestroyKey(key);
    }
    return ret;
}

static BOOL CDecodeSignedMsg_VerifySignature(CDecodeMsg *msg, PCERT_INFO info)
{
    BOOL ret = FALSE;
    DWORD i;

    if (!msg->u.signed_data.signerHandles)
    {
        SetLastError(NTE_BAD_SIGNATURE);
        return FALSE;
    }
    for (i = 0; !ret && i < msg->u.signed_data.info->cSignerInfo; i++)
    {
        PCMSG_CMS_SIGNER_INFO signerInfo =
         &msg->u.signed_data.info->rgSignerInfo[i];

        if (signerInfo->SignerId.dwIdChoice == CERT_ID_ISSUER_SERIAL_NUMBER)
        {
            ret = CertCompareCertificateName(X509_ASN_ENCODING,
             &signerInfo->SignerId.IssuerSerialNumber.Issuer,
             &info->Issuer);
            if (ret)
            {
                ret = CertCompareIntegerBlob(
                 &signerInfo->SignerId.IssuerSerialNumber.SerialNumber,
                 &info->SerialNumber);
                if (ret)
                    break;
            }
        }
        else
        {
            FIXME("signer %ld: unimplemented for key id\n", i);
        }
    }
    if (ret)
        ret = CDecodeSignedMsg_VerifySignatureWithKey(msg, 0, i,
         &info->SubjectPublicKeyInfo);
    else
        SetLastError(CRYPT_E_SIGNER_NOT_FOUND);

    return ret;
}

static BOOL CDecodeSignedMsg_VerifySignatureEx(CDecodeMsg *msg,
 PCMSG_CTRL_VERIFY_SIGNATURE_EX_PARA para)
{
    BOOL ret = FALSE;

    if (para->cbSize != sizeof(CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (para->dwSignerIndex >= msg->u.signed_data.info->cSignerInfo)
        SetLastError(CRYPT_E_SIGNER_NOT_FOUND);
    else if (!msg->u.signed_data.signerHandles)
        SetLastError(NTE_BAD_SIGNATURE);
    else
    {
        switch (para->dwSignerType)
        {
        case CMSG_VERIFY_SIGNER_PUBKEY:
            ret = CDecodeSignedMsg_VerifySignatureWithKey(msg,
             para->hCryptProv, para->dwSignerIndex, para->pvSigner);
            break;
        case CMSG_VERIFY_SIGNER_CERT:
        {
            PCCERT_CONTEXT cert = para->pvSigner;

            ret = CDecodeSignedMsg_VerifySignatureWithKey(msg, para->hCryptProv,
             para->dwSignerIndex, &cert->pCertInfo->SubjectPublicKeyInfo);
            break;
        }
        default:
            FIXME("unimplemented for signer type %ld\n", para->dwSignerType);
            SetLastError(CRYPT_E_SIGNER_NOT_FOUND);
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_ImportKeyTrans(
 PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
 PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara, DWORD dwFlags,
 void *pvReserved, HCRYPTKEY *phContentEncryptKey)
{
    BOOL ret;
    HCRYPTKEY key;

    ret = CryptGetUserKey(pKeyTransDecryptPara->hCryptProv,
     pKeyTransDecryptPara->dwKeySpec ? pKeyTransDecryptPara->dwKeySpec :
     AT_KEYEXCHANGE, &key);
    if (ret)
    {
        CMSG_KEY_TRANS_RECIPIENT_INFO *info =
         &pKeyTransDecryptPara->pKeyTrans[pKeyTransDecryptPara->dwRecipientIndex];
        CRYPT_DATA_BLOB *encryptedKey = &info->EncryptedKey;
        DWORD size = encryptedKey->cbData + sizeof(BLOBHEADER) + sizeof(ALG_ID);
        BYTE *keyBlob = CryptMemAlloc(size);

        if (keyBlob)
        {
            DWORD i, k = size - 1;
            BLOBHEADER *blobHeader = (BLOBHEADER *)keyBlob;
            ALG_ID *algID = (ALG_ID *)(keyBlob + sizeof(BLOBHEADER));

            blobHeader->bType = SIMPLEBLOB;
            blobHeader->bVersion = CUR_BLOB_VERSION;
            blobHeader->reserved = 0;
            blobHeader->aiKeyAlg = CertOIDToAlgId(
             pContentEncryptionAlgorithm->pszObjId);
            *algID = CertOIDToAlgId(info->KeyEncryptionAlgorithm.pszObjId);
            for (i = 0; i < encryptedKey->cbData; ++i, --k)
                keyBlob[k] = encryptedKey->pbData[i];

            ret = CryptImportKey(pKeyTransDecryptPara->hCryptProv, keyBlob,
             size, key, 0, phContentEncryptKey);
            CryptMemFree(keyBlob);
        }
        else
            ret = FALSE;
        CryptDestroyKey(key);
    }
    return ret;
}

static BOOL CRYPT_ImportEncryptedKey(PCRYPT_ALGORITHM_IDENTIFIER contEncrAlg,
 PCMSG_CTRL_DECRYPT_PARA para, PCMSG_KEY_TRANS_RECIPIENT_INFO info,
 HCRYPTKEY *key)
{
    static HCRYPTOIDFUNCSET set = NULL;
    PFN_CMSG_IMPORT_KEY_TRANS importKeyFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;
    CMSG_CTRL_KEY_TRANS_DECRYPT_PARA decryptPara;
    BOOL ret;

    memset(&decryptPara, 0, sizeof(decryptPara));
    decryptPara.cbSize = sizeof(decryptPara);
    decryptPara.hCryptProv = para->hCryptProv;
    decryptPara.dwKeySpec = para->dwKeySpec;
    decryptPara.pKeyTrans = info;
    decryptPara.dwRecipientIndex = para->dwRecipientIndex;

    if (!set)
        set = CryptInitOIDFunctionSet(CMSG_OID_IMPORT_KEY_TRANS_FUNC, 0);
    CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, contEncrAlg->pszObjId, 0,
     (void **)&importKeyFunc, &hFunc);
    if (!importKeyFunc)
        importKeyFunc = CRYPT_ImportKeyTrans;
    ret = importKeyFunc(contEncrAlg, &decryptPara, 0, NULL, key);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

static BOOL CDecodeEnvelopedMsg_CrtlDecrypt(CDecodeMsg *msg,
 PCMSG_CTRL_DECRYPT_PARA para)
{
    BOOL ret = FALSE;
    CEnvelopedDecodeMsg *enveloped_data = &msg->u.enveloped_data;
    CRYPT_ENVELOPED_DATA *data = enveloped_data->data;

    if (para->cbSize != sizeof(CMSG_CTRL_DECRYPT_PARA))
        SetLastError(E_INVALIDARG);
    else if (!data)
        SetLastError(CRYPT_E_INVALID_MSG_TYPE);
    else if (para->dwRecipientIndex >= data->cRecipientInfo)
        SetLastError(CRYPT_E_INVALID_INDEX);
    else if (enveloped_data->decrypted)
        SetLastError(CRYPT_E_ALREADY_DECRYPTED);
    else if (!para->hCryptProv)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (enveloped_data->content.cbData)
    {
        HCRYPTKEY key;

        ret = CRYPT_ImportEncryptedKey(
         &data->encryptedContentInfo.contentEncryptionAlgorithm, para,
         data->rgRecipientInfo, &key);
        if (ret)
        {
            ret = CryptDecrypt(key, 0, TRUE, 0, enveloped_data->content.pbData,
             &enveloped_data->content.cbData);
            CryptDestroyKey(key);
        }
    }
    else
        ret = TRUE;
    if (ret)
        enveloped_data->decrypted = TRUE;
    return ret;
}

static BOOL CDecodeMsg_Control(HCRYPTMSG hCryptMsg, DWORD dwFlags,
 DWORD dwCtrlType, const void *pvCtrlPara)
{
    CDecodeMsg *msg = hCryptMsg;
    BOOL ret = FALSE;

    switch (dwCtrlType)
    {
    case CMSG_CTRL_VERIFY_SIGNATURE:
        switch (msg->type)
        {
        case CMSG_SIGNED:
            ret = CDecodeSignedMsg_VerifySignature(msg, (PCERT_INFO)pvCtrlPara);
            break;
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        }
        break;
    case CMSG_CTRL_DECRYPT:
        switch (msg->type)
        {
        case CMSG_ENVELOPED:
            ret = CDecodeEnvelopedMsg_CrtlDecrypt(msg,
             (PCMSG_CTRL_DECRYPT_PARA)pvCtrlPara);
            if (ret && (dwFlags & CMSG_CRYPT_RELEASE_CONTEXT_FLAG))
                msg->u.enveloped_data.crypt_prov =
                 ((PCMSG_CTRL_DECRYPT_PARA)pvCtrlPara)->hCryptProv;
            break;
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        }
        break;
    case CMSG_CTRL_VERIFY_HASH:
        switch (msg->type)
        {
        case CMSG_HASHED:
            ret = CDecodeHashMsg_VerifyHash(msg);
            break;
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        }
        break;
    case CMSG_CTRL_VERIFY_SIGNATURE_EX:
        switch (msg->type)
        {
        case CMSG_SIGNED:
            ret = CDecodeSignedMsg_VerifySignatureEx(msg,
             (PCMSG_CTRL_VERIFY_SIGNATURE_EX_PARA)pvCtrlPara);
            break;
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
        }
        break;
    case CMSG_CTRL_ADD_CERT:
        switch (msg->type)
        {
        case CMSG_SIGNED:
            if (!msg->u.signed_data.info)
            {
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
                break;
            }
            ret = CRYPT_AddBlobArray(&msg->u.signed_data.info->cCertEncoded,
                 &msg->u.signed_data.info->rgCertEncoded, 1, (const CRYPT_DATA_BLOB *)pvCtrlPara);
            break;
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
            break;
        }
        break;
    case CMSG_CTRL_DEL_CERT:
        switch (msg->type)
        {
        case CMSG_SIGNED:
        {
            DWORD index = *(const DWORD *)pvCtrlPara;

            if (!msg->u.signed_data.info)
            {
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
                break;
            }
            if (index >= msg->u.signed_data.info->cCertEncoded)
            {
                SetLastError(CRYPT_E_INVALID_INDEX);
                break;
            }
            TRACE("CMSG_CTRL_DEL_CERT: index %lu of %lu\n", index, msg->u.signed_data.info->cCertEncoded);
            CryptMemFree(msg->u.signed_data.info->rgCertEncoded[index].pbData);
            memmove(&msg->u.signed_data.info->rgCertEncoded[index], &msg->u.signed_data.info->rgCertEncoded[index + 1],
                    (msg->u.signed_data.info->cCertEncoded - index - 1) * sizeof(msg->u.signed_data.info->rgCertEncoded));
            msg->u.signed_data.info->cCertEncoded--;
            ret = TRUE;
            break;
        }
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
            break;
        }
        break;
    case CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR:
        switch (msg->type)
        {
        case CMSG_SIGNED:
        {
            CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA *param = (CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA *)pvCtrlPara;
            CRYPT_ATTRIBUTE *attr;
            DWORD size;

            if (!msg->u.signed_data.info)
            {
                SetLastError(CRYPT_E_INVALID_MSG_TYPE);
                break;
            }

            if (param->dwSignerIndex >= msg->u.signed_data.info->cSignerInfo)
            {
                SetLastError(CRYPT_E_INVALID_INDEX);
                break;
            }

            ret = CryptDecodeObjectEx(PKCS_7_ASN_ENCODING, PKCS_ATTRIBUTE, param->blob.pbData, param->blob.cbData,
                                      CRYPT_DECODE_ALLOC_FLAG, NULL, &attr, &size);
            if (!ret)
            {
                WARN("CryptDecodeObjectEx(PKCS_ATTRIBUTE) error %#lx\n", GetLastError());
                break;
            }

            TRACE("existing CRYPT_ATTRIBUTES: cAttr = %lu\n", msg->u.signed_data.info->rgSignerInfo[param->dwSignerIndex].UnauthAttrs.cAttr);
            TRACE("CRYPT_ATTRIBUTE: %p: pszObjId %s, cValue %lu, rgValue %p\n", attr, debugstr_a(attr->pszObjId), attr->cValue, attr->rgValue);
            ret = CRYPT_AddAttribute(&msg->u.signed_data.info->rgSignerInfo[param->dwSignerIndex].UnauthAttrs, attr);
            LocalFree(attr);
            break;
        }
        default:
            SetLastError(CRYPT_E_INVALID_MSG_TYPE);
            break;
        }
        break;

    default:
        FIXME("unimplemented for %ld\n", dwCtrlType);
        SetLastError(CRYPT_E_CONTROL_TYPE);
        break;
    }
    return ret;
}

HCRYPTMSG WINAPI CryptMsgOpenToDecode(DWORD dwMsgEncodingType, DWORD dwFlags,
 DWORD dwMsgType, HCRYPTPROV_LEGACY hCryptProv, PCERT_INFO pRecipientInfo,
 PCMSG_STREAM_INFO pStreamInfo)
{
    CDecodeMsg *msg;

    TRACE("(%08lx, %08lx, %08lx, %08Ix, %p, %p)\n", dwMsgEncodingType,
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
         CDecodeMsg_Close, CDecodeMsg_GetParam, CDecodeMsg_Update,
         CDecodeMsg_Control);
        msg->type = dwMsgType;
        msg->crypt_prov = hCryptProv;
        memset(&msg->u, 0, sizeof(msg->u));
        msg->msg_data.cbData = 0;
        msg->msg_data.pbData = NULL;
        msg->detached_data.cbData = 0;
        msg->detached_data.pbData = NULL;
        msg->properties = ContextPropertyList_Create();
    }
    return msg;
}

HCRYPTMSG WINAPI CryptMsgDuplicate(HCRYPTMSG hCryptMsg)
{
    TRACE("(%p)\n", hCryptMsg);

    if (hCryptMsg)
    {
        CryptMsgBase *msg = hCryptMsg;

        InterlockedIncrement(&msg->ref);
    }
    return hCryptMsg;
}

BOOL WINAPI CryptMsgClose(HCRYPTMSG hCryptMsg)
{
    TRACE("(%p)\n", hCryptMsg);

    if (hCryptMsg)
    {
        CryptMsgBase *msg = hCryptMsg;

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
    CryptMsgBase *msg = hCryptMsg;

    TRACE("(%p, %p, %ld, %d)\n", hCryptMsg, pbData, cbData, fFinal);

    return msg->update(hCryptMsg, pbData, cbData, fFinal);
}

BOOL WINAPI CryptMsgGetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType,
 DWORD dwIndex, void *pvData, DWORD *pcbData)
{
    CryptMsgBase *msg = hCryptMsg;

    TRACE("(%p, %ld, %ld, %p, %p)\n", hCryptMsg, dwParamType, dwIndex,
     pvData, pcbData);
    return msg->get_param(hCryptMsg, dwParamType, dwIndex, pvData, pcbData);
}

BOOL WINAPI CryptMsgControl(HCRYPTMSG hCryptMsg, DWORD dwFlags,
 DWORD dwCtrlType, const void *pvCtrlPara)
{
    CryptMsgBase *msg = hCryptMsg;

    TRACE("(%p, %08lx, %ld, %p)\n", hCryptMsg, dwFlags, dwCtrlType,
     pvCtrlPara);
    return msg->control(hCryptMsg, dwFlags, dwCtrlType, pvCtrlPara);
}

static CERT_INFO *CRYPT_GetSignerCertInfoFromMsg(HCRYPTMSG msg,
 DWORD dwSignerIndex)
{
    CERT_INFO *certInfo = NULL;
    DWORD size;

    if (CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM, dwSignerIndex, NULL,
     &size))
    {
        certInfo = CryptMemAlloc(size);
        if (certInfo)
        {
            if (!CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM,
             dwSignerIndex, certInfo, &size))
            {
                CryptMemFree(certInfo);
                certInfo = NULL;
            }
        }
    }
    return certInfo;
}

BOOL WINAPI CryptMsgGetAndVerifySigner(HCRYPTMSG hCryptMsg, DWORD cSignerStore,
 HCERTSTORE *rghSignerStore, DWORD dwFlags, PCCERT_CONTEXT *ppSigner,
 DWORD *pdwSignerIndex)
{
    HCERTSTORE store;
    DWORD i, signerIndex = 0;
    PCCERT_CONTEXT signerCert = NULL;
    BOOL ret = FALSE;

    TRACE("(%p, %ld, %p, %08lx, %p, %p)\n", hCryptMsg, cSignerStore,
     rghSignerStore, dwFlags, ppSigner, pdwSignerIndex);

    /* Clear output parameters */
    if (ppSigner)
        *ppSigner = NULL;
    if (pdwSignerIndex && !(dwFlags & CMSG_USE_SIGNER_INDEX_FLAG))
        *pdwSignerIndex = 0;

    /* Create store to search for signer certificates */
    store = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (!(dwFlags & CMSG_TRUSTED_SIGNER_FLAG))
    {
        HCERTSTORE msgStore = CertOpenStore(CERT_STORE_PROV_MSG, 0, 0, 0,
         hCryptMsg);

        CertAddStoreToCollection(store, msgStore, 0, 0);
        CertCloseStore(msgStore, 0);
    }
    for (i = 0; i < cSignerStore; i++)
        CertAddStoreToCollection(store, rghSignerStore[i], 0, 0);

    /* Find signer cert */
    if (dwFlags & CMSG_USE_SIGNER_INDEX_FLAG)
    {
        CERT_INFO *signer = CRYPT_GetSignerCertInfoFromMsg(hCryptMsg,
         *pdwSignerIndex);

        if (signer)
        {
            signerIndex = *pdwSignerIndex;
            signerCert = CertFindCertificateInStore(store, X509_ASN_ENCODING,
             0, CERT_FIND_SUBJECT_CERT, signer, NULL);
            CryptMemFree(signer);
        }
    }
    else
    {
        DWORD count, size = sizeof(count);

        if (CryptMsgGetParam(hCryptMsg, CMSG_SIGNER_COUNT_PARAM, 0, &count,
         &size))
        {
            for (i = 0; !signerCert && i < count; i++)
            {
                CERT_INFO *signer = CRYPT_GetSignerCertInfoFromMsg(hCryptMsg,
                 i);

                if (signer)
                {
                    signerCert = CertFindCertificateInStore(store,
                     X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, signer,
                     NULL);
                    if (signerCert)
                        signerIndex = i;
                    CryptMemFree(signer);
                }
            }
        }
        if (!signerCert)
            SetLastError(CRYPT_E_NO_TRUSTED_SIGNER);
    }
    if (signerCert)
    {
        if (!(dwFlags & CMSG_SIGNER_ONLY_FLAG))
            ret = CryptMsgControl(hCryptMsg, 0, CMSG_CTRL_VERIFY_SIGNATURE,
             signerCert->pCertInfo);
        else
            ret = TRUE;
        if (ret)
        {
            if (ppSigner)
                *ppSigner = CertDuplicateCertificateContext(signerCert);
            if (pdwSignerIndex)
                *pdwSignerIndex = signerIndex;
        }
        CertFreeCertificateContext(signerCert);
    }

    CertCloseStore(store, 0);
    return ret;
}

BOOL WINAPI CryptMsgVerifyCountersignatureEncoded(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwEncodingType, BYTE *pbSignerInfo, DWORD cbSignerInfo,
 PBYTE pbSignerInfoCountersignature, DWORD cbSignerInfoCountersignature,
 CERT_INFO *pciCountersigner)
{
    FIXME("(%08Ix, %08lx, %p, %ld, %p, %ld, %p): stub\n", hCryptProv,
     dwEncodingType, pbSignerInfo, cbSignerInfo, pbSignerInfoCountersignature,
     cbSignerInfoCountersignature, pciCountersigner);
    return FALSE;
}

BOOL WINAPI CryptMsgVerifyCountersignatureEncodedEx(HCRYPTPROV_LEGACY hCryptProv,
 DWORD dwEncodingType, PBYTE pbSignerInfo, DWORD cbSignerInfo,
 PBYTE pbSignerInfoCountersignature, DWORD cbSignerInfoCountersignature,
 DWORD dwSignerType, void *pvSigner, DWORD dwFlags, void *pvReserved)
{
    FIXME("(%08Ix, %08lx, %p, %ld, %p, %ld, %ld, %p, %08lx, %p): stub\n", hCryptProv,
     dwEncodingType, pbSignerInfo, cbSignerInfo, pbSignerInfoCountersignature,
     cbSignerInfoCountersignature, dwSignerType, pvSigner, dwFlags, pvReserved);
    return FALSE;
}

BOOL WINAPI CryptMsgEncodeAndSignCTL(DWORD dwMsgEncodingType,
 PCTL_INFO pCtlInfo, PCMSG_SIGNED_ENCODE_INFO pSignInfo, DWORD dwFlags,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    BYTE *pbCtlContent;
    DWORD cbCtlContent;

    TRACE("(%08lx, %p, %p, %08lx, %p, %p)\n", dwMsgEncodingType, pCtlInfo,
     pSignInfo, dwFlags, pbEncoded, pcbEncoded);

    if (dwFlags)
    {
        FIXME("unimplemented for flags %08lx\n", dwFlags);
        return FALSE;
    }
    if ((ret = CryptEncodeObjectEx(dwMsgEncodingType, PKCS_CTL, pCtlInfo,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &pbCtlContent, &cbCtlContent)))
    {
        ret = CryptMsgSignCTL(dwMsgEncodingType, pbCtlContent, cbCtlContent,
         pSignInfo, dwFlags, pbEncoded, pcbEncoded);
        LocalFree(pbCtlContent);
    }
    return ret;
}

BOOL WINAPI CryptMsgSignCTL(DWORD dwMsgEncodingType, BYTE *pbCtlContent,
 DWORD cbCtlContent, PCMSG_SIGNED_ENCODE_INFO pSignInfo, DWORD dwFlags,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    static char oid_ctl[] = szOID_CTL;
    BOOL ret;
    HCRYPTMSG msg;

    TRACE("(%08lx, %p, %ld, %p, %08lx, %p, %p)\n", dwMsgEncodingType,
     pbCtlContent, cbCtlContent, pSignInfo, dwFlags, pbEncoded, pcbEncoded);

    if (dwFlags)
    {
        FIXME("unimplemented for flags %08lx\n", dwFlags);
        return FALSE;
    }
    msg = CryptMsgOpenToEncode(dwMsgEncodingType, 0, CMSG_SIGNED, pSignInfo,
     oid_ctl, NULL);
    if (msg)
    {
        ret = CryptMsgUpdate(msg, pbCtlContent, cbCtlContent, TRUE);
        if (ret)
            ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, pbEncoded,
             pcbEncoded);
        CryptMsgClose(msg);
    }
    else
        ret = FALSE;
    return ret;
}
