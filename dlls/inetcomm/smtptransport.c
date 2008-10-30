/*
 * SMTP Transport
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "objbase.h"
#include "mimeole.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct
{
    InternetTransport InetTransport;
    ULONG refs;
} SMTPTransport;

static HRESULT WINAPI SMTPTransport_QueryInterface(ISMTPTransport2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInternetTransport) ||
        IsEqualIID(riid, &IID_ISMTPTransport) ||
        IsEqualIID(riid, &IID_ISMTPTransport2))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    FIXME("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SMTPTransport_AddRef(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    return InterlockedIncrement((LONG *)&This->refs);
}

static ULONG WINAPI SMTPTransport_Release(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    ULONG refs = InterlockedDecrement((LONG *)&This->refs);
    if (!refs)
    {
        TRACE("destroying %p\n", This);
        if (This->InetTransport.Status != IXP_DISCONNECTED)
            InternetTransport_DropConnection(&This->InetTransport);

        if (This->InetTransport.pCallback) ITransportCallback_Release(This->InetTransport.pCallback);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI SMTPTransport_GetServerInfo(ISMTPTransport2 *iface,
    LPINETSERVER pInetServer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%p)\n", pInetServer);
    return InternetTransport_GetServerInfo(&This->InetTransport, pInetServer);
}

static IXPTYPE WINAPI SMTPTransport_GetIXPType(ISMTPTransport2 *iface)
{
    TRACE("()\n");
    return IXP_SMTP;
}

static HRESULT WINAPI SMTPTransport_IsState(ISMTPTransport2 *iface,
    IXPISSTATE isstate)
{
    FIXME("(%d): stub\n", isstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_InetServerFromAccount(
    ISMTPTransport2 *iface, IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%p, %p)\n", pAccount, pInetServer);
    return InternetTransport_InetServerFromAccount(&This->InetTransport, pAccount, pInetServer);
}

static HRESULT WINAPI SMTPTransport_Connect(ISMTPTransport2 *iface,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    SMTPTransport *This = (SMTPTransport *)iface;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pInetServer, fAuthenticate ? "TRUE" : "FALSE", fCommandLogging ? "TRUE" : "FALSE");

    hr = InternetTransport_Connect(&This->InetTransport, pInetServer, fAuthenticate, fCommandLogging);

    FIXME("continue state machine here\n");
    return hr;
}

static HRESULT WINAPI SMTPTransport_HandsOffCallback(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_HandsOffCallback(&This->InetTransport);
}

static HRESULT WINAPI SMTPTransport_Disconnect(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_DropConnection(ISMTPTransport2 *iface)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_DropConnection(&This->InetTransport);
}

static HRESULT WINAPI SMTPTransport_GetStatus(ISMTPTransport2 *iface,
    IXPSTATUS *pCurrentStatus)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_GetStatus(&This->InetTransport, pCurrentStatus);
}

static HRESULT WINAPI SMTPTransport_InitNew(ISMTPTransport2 *iface,
    LPSTR pszLogFilePath, ISMTPCallback *pCallback)
{
    SMTPTransport *This = (SMTPTransport *)iface;

    TRACE("(%s, %p)\n", debugstr_a(pszLogFilePath), pCallback);

    if (!pCallback)
        return E_INVALIDARG;

    if (pszLogFilePath)
        FIXME("not using log file of %s, use Wine debug logging instead\n",
            debugstr_a(pszLogFilePath));

    ISMTPCallback_AddRef(pCallback);
    This->InetTransport.pCallback = (ITransportCallback *)pCallback;
    This->InetTransport.fInitialised = TRUE;

    return S_OK;
}

static HRESULT WINAPI SMTPTransport_SendMessage(ISMTPTransport2 *iface,
    LPSMTPMESSAGE pMessage)
{
    FIXME("(%p)\n", pMessage);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandMAIL(ISMTPTransport2 *iface, LPSTR pszEmailFrom)
{
    FIXME("(%s)\n", pszEmailFrom);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandRCPT(ISMTPTransport2 *iface, LPSTR pszEmailTo)
{
    FIXME("(%s)\n", pszEmailTo);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandEHLO(ISMTPTransport2 *iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandHELO(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandAUTH(ISMTPTransport2 *iface,
    LPSTR pszAuthType)
{
    FIXME("(%s)\n", pszAuthType);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandQUIT(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandRSET(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandDATA(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandDOT(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SendDataStream(ISMTPTransport2 *iface,
    IStream *pStream, ULONG cbSize)
{
    FIXME("(%p, %d)\n", pStream, cbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SetWindow(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_ResetWindow(ISMTPTransport2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_SendMessage2(ISMTPTransport2 *iface, LPSMTPMESSAGE2 pMessage)
{
    FIXME("(%p)\n", pMessage);
    return E_NOTIMPL;
}

static HRESULT WINAPI SMTPTransport_CommandRCPT2(ISMTPTransport2 *iface, LPSTR pszEmailTo,
    INETADDRTYPE atDSN)
{
    FIXME("(%s, %u)\n", pszEmailTo, atDSN);
    return E_NOTIMPL;
}

static const ISMTPTransport2Vtbl SMTPTransport2Vtbl =
{
    SMTPTransport_QueryInterface,
    SMTPTransport_AddRef,
    SMTPTransport_Release,
    SMTPTransport_GetServerInfo,
    SMTPTransport_GetIXPType,
    SMTPTransport_IsState,
    SMTPTransport_InetServerFromAccount,
    SMTPTransport_Connect,
    SMTPTransport_HandsOffCallback,
    SMTPTransport_Disconnect,
    SMTPTransport_DropConnection,
    SMTPTransport_GetStatus,
    SMTPTransport_InitNew,
    SMTPTransport_SendMessage,
    SMTPTransport_CommandMAIL,
    SMTPTransport_CommandRCPT,
    SMTPTransport_CommandEHLO,
    SMTPTransport_CommandHELO,
    SMTPTransport_CommandAUTH,
    SMTPTransport_CommandQUIT,
    SMTPTransport_CommandRSET,
    SMTPTransport_CommandDATA,
    SMTPTransport_CommandDOT,
    SMTPTransport_SendDataStream,
    SMTPTransport_SetWindow,
    SMTPTransport_ResetWindow,
    SMTPTransport_SendMessage2,
    SMTPTransport_CommandRCPT2
};

HRESULT WINAPI CreateSMTPTransport(ISMTPTransport **ppTransport)
{
    HRESULT hr;
    SMTPTransport *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->InetTransport.u.vtblSMTP2 = &SMTPTransport2Vtbl;
    This->refs = 0;
    hr = InternetTransport_Init(&This->InetTransport);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    *ppTransport = (ISMTPTransport *)&This->InetTransport.u.vtblSMTP2;
    ISMTPTransport_AddRef(*ppTransport);

    return S_OK;
}


static HRESULT WINAPI SMTPTransportCF_QueryInterface(LPCLASSFACTORY iface,
    REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI SMTPTransportCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI SMTPTransportCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI SMTPTransportCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    ISMTPTransport *pSmtpTransport;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateSMTPTransport(&pSmtpTransport);
    if (FAILED(hr))
        return hr;

    hr = ISMTPTransport_QueryInterface(pSmtpTransport, riid, ppv);
    ISMTPTransport_Release(pSmtpTransport);

    return hr;
}

static HRESULT WINAPI SMTPTransportCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d)\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl SMTPTransportCFVtbl =
{
    SMTPTransportCF_QueryInterface,
    SMTPTransportCF_AddRef,
    SMTPTransportCF_Release,
    SMTPTransportCF_CreateInstance,
    SMTPTransportCF_LockServer
};
static const IClassFactoryVtbl *SMTPTransportCF = &SMTPTransportCFVtbl;

HRESULT SMTPTransportCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&SMTPTransportCF, riid, ppv);
}
