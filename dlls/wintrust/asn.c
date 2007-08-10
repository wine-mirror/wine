/* wintrust asn functions
 *
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
 *
 */
#include <stdarg.h>
#include <assert.h>
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "snmp.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptasn);

#ifdef WORDS_BIGENDIAN

#define hton16(x) (x)
#define n16toh(x) (x)

#else

#define hton16(x) RtlUshortByteSwap(x)
#define n16toh(x) RtlUshortByteSwap(x)

#endif

static BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded, significantBytes = 0;

    if (len <= 0x7f)
        bytesNeeded = 1;
    else
    {
        DWORD temp;

        for (temp = len, significantBytes = sizeof(temp); !(temp & 0xff000000);
         temp <<= 8, significantBytes--)
            ;
        bytesNeeded = significantBytes + 1;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (*pcbEncoded < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    if (len <= 0x7f)
        *pbEncoded = (BYTE)len;
    else
    {
        DWORD i;

        *pbEncoded++ = significantBytes | 0x80;
        for (i = 0; i < significantBytes; i++)
        {
            *(pbEncoded + significantBytes - i - 1) = (BYTE)(len & 0xff);
            len >>= 8;
        }
    }
    *pcbEncoded = bytesNeeded;
    return TRUE;
}

static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvStructInfo;
    DWORD bytesNeeded, lenBytes;

    TRACE("(%d, %p), %p, %d\n", blob->cbData, blob->pbData, pbEncoded,
     *pcbEncoded);

    CRYPT_EncodeLen(blob->cbData, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + blob->cbData;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else if (*pcbEncoded < bytesNeeded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pbEncoded++ = ASN_OCTETSTRING;
        CRYPT_EncodeLen(blob->cbData, pbEncoded, &lenBytes);
        pbEncoded += lenBytes;
        if (blob->cbData)
            memcpy(pbEncoded, blob->pbData, blob->cbData);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcLinkEncode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret = FALSE;

    TRACE("(0x%08x, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    __TRY
    {
        const SPC_LINK *link = (const SPC_LINK *)pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        switch (link->dwLinkChoice)
        {
        case SPC_FILE_LINK_CHOICE:
        {
            DWORD fileNameLen, fileNameLenBytes;
            LPWSTR ptr;

            fileNameLen = lstrlenW(link->u.pwszFile) * sizeof(WCHAR);
            CRYPT_EncodeLen(fileNameLen, NULL, &fileNameLenBytes);
            CRYPT_EncodeLen(1 + fileNameLenBytes + fileNameLen, NULL,
             &lenBytes);
            bytesNeeded = 2 + lenBytes + fileNameLenBytes + fileNameLen;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else if (*pcbEncoded < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbEncoded = bytesNeeded;
            }
            else
            {
                *pcbEncoded = bytesNeeded;
                *pbEncoded++ = ASN_CONSTRUCTOR | ASN_CONTEXT | 2;
                CRYPT_EncodeLen(1 + fileNameLenBytes + fileNameLen, pbEncoded,
                 &lenBytes);
                pbEncoded += lenBytes;
                *pbEncoded++ = ASN_CONTEXT;
                CRYPT_EncodeLen(fileNameLen, pbEncoded, &fileNameLenBytes);
                pbEncoded += fileNameLenBytes;
                for (ptr = link->u.pwszFile; ptr && *ptr; ptr++)
                {
                    *(WCHAR *)pbEncoded = hton16(*ptr);
                    pbEncoded += sizeof(WCHAR);
                }
                ret = TRUE;
            }
            break;
        }
        case SPC_MONIKER_LINK_CHOICE:
        {
            DWORD classIdLenBytes, dataLenBytes, dataLen;
            CRYPT_DATA_BLOB classId = { sizeof(link->u.Moniker.ClassId),
             (BYTE *)&link->u.Moniker.ClassId };

            CRYPT_EncodeLen(classId.cbData, NULL, &classIdLenBytes);
            CRYPT_EncodeLen(link->u.Moniker.SerializedData.cbData, NULL,
             &dataLenBytes);
            dataLen = 2 + classIdLenBytes + classId.cbData +
             dataLenBytes + link->u.Moniker.SerializedData.cbData;
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + dataLen + lenBytes;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else if (*pcbEncoded < bytesNeeded)
            {
                SetLastError(ERROR_MORE_DATA);
                *pcbEncoded = bytesNeeded;
            }
            else
            {
                DWORD size;

                *pcbEncoded = bytesNeeded;
                *pbEncoded++ = ASN_CONSTRUCTOR | ASN_CONTEXT | 1;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                size = 1 + classIdLenBytes + classId.cbData;
                CRYPT_AsnEncodeOctets(X509_ASN_ENCODING, NULL, &classId,
                 pbEncoded, &size);
                pbEncoded += size;
                size = 1 + dataLenBytes + link->u.Moniker.SerializedData.cbData;
                CRYPT_AsnEncodeOctets(X509_ASN_ENCODING, NULL,
                 &link->u.Moniker.SerializedData, pbEncoded, &size);
                pbEncoded += size;
                ret = TRUE;
            }
            break;
        }
        case SPC_URL_LINK_CHOICE:
        {
            LPWSTR ptr;
            DWORD urlLen;

            /* Check for invalid characters in URL */
            ret = TRUE;
            urlLen = 0;
            for (ptr = link->u.pwszUrl; ptr && *ptr && ret; ptr++)
                if (*ptr > 0x7f)
                {
                    *pcbEncoded = 0;
                    SetLastError(CRYPT_E_INVALID_IA5_STRING);
                    ret = FALSE;
                }
                else
                    urlLen++;
            if (ret)
            {
                CRYPT_EncodeLen(urlLen, NULL, &lenBytes);
                bytesNeeded = 1 + lenBytes + urlLen;
                if (!pbEncoded)
                    *pcbEncoded = bytesNeeded;
                else if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    *pcbEncoded = bytesNeeded;
                    ret = FALSE;
                }
                else
                {
                    *pcbEncoded = bytesNeeded;
                    *pbEncoded++ = ASN_CONTEXT;
                    CRYPT_EncodeLen(urlLen, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    for (ptr = link->u.pwszUrl; ptr && *ptr; ptr++)
                        *pbEncoded++ = (BYTE)*ptr;
                }
            }
            break;
        }
        default:
            SetLastError(E_INVALIDARG);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcPeImageDataEncode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    FIXME("(0x%08x, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);
    return FALSE;
}

/* Gets the number of length bytes from the given (leading) length byte */
#define GET_LEN_BYTES(b) ((b) <= 0x7f ? 1 : 1 + ((b) & 0x7f))

/* Helper function to get the encoded length of the data starting at pbEncoded,
 * where pbEncoded[0] is the tag.  If the data are too short to contain a
 * length or if the length is too large for cbEncoded, sets an appropriate
 * error code and returns FALSE.
 */
static BOOL CRYPT_GetLen(const BYTE *pbEncoded, DWORD cbEncoded, DWORD *len)
{
    BOOL ret;

    if (cbEncoded <= 1)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    else if (pbEncoded[1] <= 0x7f)
    {
        if (pbEncoded[1] + 1 > cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_EOD);
            ret = FALSE;
        }
        else
        {
            *len = pbEncoded[1];
            ret = TRUE;
        }
    }
    else if (pbEncoded[1] == 0x80)
    {
        FIXME("unimplemented for indefinite-length encoding\n");
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    else
    {
        BYTE lenLen = GET_LEN_BYTES(pbEncoded[1]);

        if (lenLen > sizeof(DWORD) + 1)
        {
            SetLastError(CRYPT_E_ASN1_LARGE);
            ret = FALSE;
        }
        else if (lenLen + 2 > cbEncoded)
        {
            SetLastError(CRYPT_E_ASN1_CORRUPT);
            ret = FALSE;
        }
        else
        {
            DWORD out = 0;

            pbEncoded += 2;
            while (--lenLen)
            {
                out <<= 8;
                out |= *pbEncoded++;
            }
            if (out + lenLen + 1 > cbEncoded)
            {
                SetLastError(CRYPT_E_ASN1_EOD);
                ret = FALSE;
            }
            else
            {
                *len = out;
                ret = TRUE;
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret;
    DWORD bytesNeeded, dataLen;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if (!cbEncoded)
    {
        SetLastError(CRYPT_E_ASN1_CORRUPT);
        ret = FALSE;
    }
    else if (pbEncoded[0] != ASN_OCTETSTRING)
    {
        SetLastError(CRYPT_E_ASN1_BADTAG);
        ret = FALSE;
    }
    else if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
            bytesNeeded = sizeof(CRYPT_DATA_BLOB);
        else
            bytesNeeded = dataLen + sizeof(CRYPT_DATA_BLOB);
        if (!pvStructInfo)
            *pcbStructInfo = bytesNeeded;
        else if (*pcbStructInfo < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbStructInfo = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            CRYPT_DATA_BLOB *blob;
            BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);

            blob = (CRYPT_DATA_BLOB *)pvStructInfo;
            blob->cbData = dataLen;
            if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG)
                blob->pbData = (BYTE *)pbEncoded + 1 + lenBytes;
            else
            {
                assert(blob->pbData);
                if (blob->cbData)
                    memcpy(blob->pbData, pbEncoded + 1 + lenBytes,
                     blob->cbData);
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnDecodeSPCLinkInternal(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;
    DWORD bytesNeeded = sizeof(SPC_LINK), dataLen;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    if ((ret = CRYPT_GetLen(pbEncoded, cbEncoded, &dataLen)))
    {
        BYTE lenBytes = GET_LEN_BYTES(pbEncoded[1]);
        DWORD realDataLen;

        switch (pbEncoded[0])
        {
        case ASN_CONTEXT:
            bytesNeeded += (dataLen + 1) * sizeof(WCHAR);
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                PSPC_LINK link = (PSPC_LINK)pvStructInfo;
                DWORD i;

                link->dwLinkChoice = SPC_URL_LINK_CHOICE;
                for (i = 0; i < dataLen; i++)
                    link->u.pwszUrl[i] =
                     *(pbEncoded + 1 + lenBytes + i);
                link->u.pwszUrl[i] = '\0';
                TRACE("returning url %s\n", debugstr_w(link->u.pwszUrl));
            }
            break;
        case ASN_CONSTRUCTOR | ASN_CONTEXT | 1:
        {
            CRYPT_DATA_BLOB classId;
            DWORD size = sizeof(classId);

            if ((ret = CRYPT_AsnDecodeOctets(dwCertEncodingType, NULL,
             pbEncoded + 1 + lenBytes, cbEncoded - 1 - lenBytes,
             CRYPT_DECODE_NOCOPY_FLAG, &classId, &size)))
            {
                if (classId.cbData != sizeof(SPC_UUID))
                {
                    SetLastError(CRYPT_E_BAD_ENCODE);
                    ret = FALSE;
                }
                else
                {
                    CRYPT_DATA_BLOB data;

                    /* The tag length for the classId must be 1 since the
                     * length is correct.
                     */
                    size = sizeof(data);
                    if ((ret = CRYPT_AsnDecodeOctets(dwCertEncodingType, NULL,
                     pbEncoded + 3 + lenBytes + classId.cbData,
                     cbEncoded - 3 - lenBytes - classId.cbData,
                     CRYPT_DECODE_NOCOPY_FLAG, &data, &size)))
                    {
                        bytesNeeded += data.cbData;
                        if (!pvStructInfo)
                            *pcbStructInfo = bytesNeeded;
                        else if (*pcbStructInfo < bytesNeeded)
                        {
                            *pcbStructInfo = bytesNeeded;
                            SetLastError(ERROR_MORE_DATA);
                            ret = FALSE;
                        }
                        else
                        {
                            PSPC_LINK link = (PSPC_LINK)pvStructInfo;

                            link->dwLinkChoice = SPC_MONIKER_LINK_CHOICE;
                            /* pwszFile pointer was set by caller, copy it
                             * before overwriting it
                             */
                            link->u.Moniker.SerializedData.pbData =
                             (BYTE *)link->u.pwszFile;
                            memcpy(&link->u.Moniker.ClassId, classId.pbData,
                             classId.cbData);
                            memcpy(link->u.Moniker.SerializedData.pbData,
                             data.pbData, data.cbData);
                            link->u.Moniker.SerializedData.cbData = data.cbData;
                        }
                    }
                }
            }
            break;
        }
        case ASN_CONSTRUCTOR | ASN_CONTEXT | 2:
            if (dataLen && pbEncoded[1 + lenBytes] != ASN_CONTEXT)
                SetLastError(CRYPT_E_ASN1_BADTAG);
            else if ((ret = CRYPT_GetLen(pbEncoded + 1 + lenBytes, dataLen,
             &realDataLen)))
            {
                BYTE realLenBytes = GET_LEN_BYTES(pbEncoded[2 + lenBytes]);

                bytesNeeded += realDataLen + sizeof(WCHAR);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if (*pcbStructInfo < bytesNeeded)
                {
                    *pcbStructInfo = bytesNeeded;
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    PSPC_LINK link = (PSPC_LINK)pvStructInfo;
                    DWORD i;
                    const BYTE *ptr = pbEncoded + 2 + lenBytes + realLenBytes;

                    link->dwLinkChoice = SPC_FILE_LINK_CHOICE;
                    for (i = 0; i < dataLen / sizeof(WCHAR); i++)
                        link->u.pwszFile[i] =
                         hton16(*(WORD *)(ptr + i * sizeof(WCHAR)));
                    link->u.pwszFile[realDataLen / sizeof(WCHAR)] = '\0';
                    TRACE("returning file %s\n", debugstr_w(link->u.pwszFile));
                }
            }
            else
            {
                bytesNeeded += sizeof(WCHAR);
                if (!pvStructInfo)
                    *pcbStructInfo = bytesNeeded;
                else if (*pcbStructInfo < bytesNeeded)
                {
                    *pcbStructInfo = bytesNeeded;
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    PSPC_LINK link = (PSPC_LINK)pvStructInfo;

                    link->dwLinkChoice = SPC_FILE_LINK_CHOICE;
                    link->u.pwszFile[0] = '\0';
                    ret = TRUE;
                }
            }
            break;
        default:
            SetLastError(CRYPT_E_ASN1_BADTAG);
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcLinkDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    BOOL ret = FALSE;

    TRACE("%p, %d, %08x, %p, %d\n", pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, *pcbStructInfo);

    __TRY
    {
        DWORD bytesNeeded;

        ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType,
         lpszStructType, pbEncoded, cbEncoded, dwFlags, NULL, &bytesNeeded);
        if (ret)
        {
            if (!pvStructInfo)
                *pcbStructInfo = bytesNeeded;
            else if (*pcbStructInfo < bytesNeeded)
            {
                *pcbStructInfo = bytesNeeded;
                SetLastError(ERROR_MORE_DATA);
                ret = FALSE;
            }
            else
            {
                SPC_LINK *link = (SPC_LINK *)pvStructInfo;

                link->u.pwszFile =
                 (LPWSTR)((BYTE *)pvStructInfo + sizeof(SPC_LINK));
                ret = CRYPT_AsnDecodeSPCLinkInternal(dwCertEncodingType,
                 lpszStructType, pbEncoded, cbEncoded, dwFlags, pvStructInfo,
                 pcbStructInfo);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
    }
    __ENDTRY
    TRACE("returning %d\n", ret);
    return ret;
}

BOOL WINAPI WVTAsn1SpcPeImageDataDecode(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
 void *pvStructInfo, DWORD *pcbStructInfo)
{
    FIXME("(0x%08x, %s, %p, %d, 0x%08x, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pbEncoded, cbEncoded, dwFlags,
     pvStructInfo, pcbStructInfo);
    return FALSE;
}
