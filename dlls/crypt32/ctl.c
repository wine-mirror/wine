/*
 * Copyright 2008 Juan Lang
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

#include <assert.h>
#include <stdarg.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

PCCTL_CONTEXT WINAPI CertCreateCTLContext(DWORD dwMsgAndCertEncodingType,
 const BYTE *pbCtlEncoded, DWORD cbCtlEncoded)
{
    PCTL_CONTEXT ctl = NULL;
    HCRYPTMSG msg;
    BOOL ret;
    BYTE *content = NULL;
    DWORD contentSize = 0, size;
    PCTL_INFO ctlInfo = NULL;

    TRACE("(%08x, %p, %d)\n", dwMsgAndCertEncodingType, pbCtlEncoded,
     cbCtlEncoded);

    if (GET_CERT_ENCODING_TYPE(dwMsgAndCertEncodingType) != X509_ASN_ENCODING)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }
    if (!pbCtlEncoded || !cbCtlEncoded)
    {
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 0, 0,
     0, NULL, NULL);
    if (!msg)
        return NULL;
    ret = CryptMsgUpdate(msg, pbCtlEncoded, cbCtlEncoded, TRUE);
    if (!ret)
    {
        SetLastError(ERROR_INVALID_DATA);
        goto end;
    }
    /* Check that it's really a CTL */
    ret = CryptMsgGetParam(msg, CMSG_INNER_CONTENT_TYPE_PARAM, 0, NULL, &size);
    if (ret)
    {
        char *innerContent = CryptMemAlloc(size);

        if (innerContent)
        {
            ret = CryptMsgGetParam(msg, CMSG_INNER_CONTENT_TYPE_PARAM, 0,
             innerContent, &size);
            if (ret)
            {
                if (strcmp(innerContent, szOID_CTL))
                {
                    SetLastError(ERROR_INVALID_DATA);
                    ret = FALSE;
                }
            }
            CryptMemFree(innerContent);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    if (!ret)
        goto end;
    ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, NULL, &contentSize);
    if (!ret)
        goto end;
    content = CryptMemAlloc(contentSize);
    if (content)
    {
        ret = CryptMsgGetParam(msg, CMSG_CONTENT_PARAM, 0, content,
         &contentSize);
        if (ret)
        {
            ret = CryptDecodeObjectEx(dwMsgAndCertEncodingType, PKCS_CTL,
             content, contentSize, CRYPT_DECODE_ALLOC_FLAG, NULL,
             (BYTE *)&ctlInfo, &size);
            if (ret)
            {
                ctl = (PCTL_CONTEXT)Context_CreateDataContext(
                 sizeof(CTL_CONTEXT));
                if (ctl)
                {
                    BYTE *data = CryptMemAlloc(cbCtlEncoded);

                    if (data)
                    {
                        memcpy(data, pbCtlEncoded, cbCtlEncoded);
                        ctl->dwMsgAndCertEncodingType =
                         X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
                        ctl->pbCtlEncoded             = data;
                        ctl->cbCtlEncoded             = cbCtlEncoded;
                        ctl->pCtlInfo                 = ctlInfo;
                        ctl->hCertStore               = NULL;
                        ctl->hCryptMsg                = msg;
                        ctl->pbCtlContext             = content;
                        ctl->cbCtlContext             = contentSize;
                    }
                    else
                    {
                        SetLastError(ERROR_OUTOFMEMORY);
                        ret = FALSE;
                    }
                }
                else
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    ret = FALSE;
                }
            }
        }
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ret = FALSE;
    }

end:
    if (!ret)
    {
        CryptMemFree(ctl);
        ctl = NULL;
        LocalFree(ctlInfo);
        CryptMemFree(content);
        CryptMsgClose(msg);
    }
    return (PCCTL_CONTEXT)ctl;
}

static void CTLDataContext_Free(void *context)
{
    PCTL_CONTEXT ctlContext = (PCTL_CONTEXT)context;

    CryptMsgClose(ctlContext->hCryptMsg);
    CryptMemFree(ctlContext->pbCtlEncoded);
    CryptMemFree(ctlContext->pbCtlContext);
    LocalFree(ctlContext->pCtlInfo);
}

BOOL WINAPI CertFreeCTLContext(PCCTL_CONTEXT pCTLContext)
{
    TRACE("(%p)\n", pCTLContext);

    if (pCTLContext)
        Context_Release((void *)pCTLContext, sizeof(CTL_CONTEXT),
         CTLDataContext_Free);
    return TRUE;
}
