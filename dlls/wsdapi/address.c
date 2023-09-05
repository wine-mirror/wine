/*
 * Web Services on Devices
 *
 * Copyright 2017 Owen Rudge for CodeWeavers
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

#define COBJMACROS

#include "wsdapi_internal.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

typedef struct IWSDUdpAddressImpl {
    IWSDUdpAddress IWSDUdpAddress_iface;
    LONG           ref;
    SOCKADDR_STORAGE sockAddr;
    WCHAR            ipv4Address[25];
    WCHAR            ipv6Address[64];
    WORD             port;
    WSDUdpMessageType messageType;
} IWSDUdpAddressImpl;

static inline IWSDUdpAddressImpl *impl_from_IWSDUdpAddress(IWSDUdpAddress *iface)
{
    return CONTAINING_RECORD(iface, IWSDUdpAddressImpl, IWSDUdpAddress_iface);
}

static HRESULT WINAPI IWSDUdpAddressImpl_QueryInterface(IWSDUdpAddress *iface, REFIID riid, void **ppv)
{
    IWSDUdpAddressImpl *This = impl_from_IWSDUdpAddress(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
    {
        WARN("Invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDUdpAddress) ||
        IsEqualIID(riid, &IID_IWSDTransportAddress) ||
        IsEqualIID(riid, &IID_IWSDAddress))
    {
        *ppv = &This->IWSDUdpAddress_iface;
    }
    else
    {
        WARN("Unknown IID %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDUdpAddressImpl_AddRef(IWSDUdpAddress *iface)
{
    IWSDUdpAddressImpl *This = impl_from_IWSDUdpAddress(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDUdpAddressImpl_Release(IWSDUdpAddress *iface)
{
    IWSDUdpAddressImpl *This = impl_from_IWSDUdpAddress(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IWSDUdpAddressImpl_Serialize(IWSDUdpAddress *This, LPWSTR pszBuffer, DWORD cchLength, BOOL fSafe)
{
    FIXME("(%p, %p, %ld, %d)\n", This, pszBuffer, cchLength, fSafe);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_Deserialize(IWSDUdpAddress *This, LPCWSTR pszBuffer)
{
    FIXME("(%p, %s)\n", This, debugstr_w(pszBuffer));
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetPort(IWSDUdpAddress *This, WORD *pwPort)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);

    TRACE("(%p, %p)\n", This, pwPort);

    if (pwPort == NULL)
    {
        return E_POINTER;
    }

    *pwPort = impl->port;
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetPort(IWSDUdpAddress *This, WORD wPort)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);

    TRACE("(%p, %d)\n", This, wPort);

    impl->port = wPort;
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetTransportAddressEx(IWSDUdpAddress *This, BOOL fSafe, LPCWSTR *ppszAddress)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);
    SOCKADDR_STORAGE storage;
    DWORD size;

    TRACE("(%p, %d, %p)\n", This, fSafe, ppszAddress);

    if (ppszAddress == NULL)
        return E_POINTER;

    *ppszAddress = NULL;

    switch (((SOCKADDR_IN *) &impl->sockAddr)->sin_family)
    {
        case AF_INET:
            size = ARRAY_SIZE(impl->ipv4Address);

            if (WSAAddressToStringW((LPSOCKADDR) &impl->sockAddr, sizeof(SOCKADDR_IN), NULL, impl->ipv4Address, &size) == 0)
            {
                *ppszAddress = impl->ipv4Address;
                return S_OK;
            }

            break;

        case AF_INET6:
            size = ARRAY_SIZE(impl->ipv6Address);

            /* Copy the SOCKADDR structure so we can remove the scope ID if not required */
            memcpy(&storage, &impl->sockAddr, sizeof(SOCKADDR_IN6));

            if (!fSafe)
                ((SOCKADDR_IN6 *) &storage)->sin6_scope_id = 0;

            if (WSAAddressToStringW((LPSOCKADDR) &storage, sizeof(SOCKADDR_IN6), NULL, impl->ipv6Address, &size) == 0)
            {
                *ppszAddress = impl->ipv6Address;
                return S_OK;
            }

            break;

        default:
            return S_OK;
    }

    return HRESULT_FROM_WIN32(WSAGetLastError());
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetTransportAddress(IWSDUdpAddress *This, LPCWSTR *ppszAddress)
{
    return IWSDUdpAddressImpl_GetTransportAddressEx(This, FALSE, ppszAddress);
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetTransportAddress(IWSDUdpAddress *This, LPCWSTR pszAddress)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);
    ADDRINFOW *addrInfo = NULL;
    ADDRINFOW hints;
    int ret;

    TRACE("(%p, %s)\n", impl, debugstr_w(pszAddress));

    if (pszAddress == NULL)
        return E_INVALIDARG;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    ret = GetAddrInfoW(pszAddress, NULL, &hints, &addrInfo);

    if (ret == 0)
    {
        ZeroMemory(&impl->sockAddr, sizeof(SOCKADDR_STORAGE));
        memcpy(&impl->sockAddr, addrInfo->ai_addr, addrInfo->ai_addrlen);
    }

    if (addrInfo != NULL)
        FreeAddrInfoW(addrInfo);

    return HRESULT_FROM_WIN32(ret);
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetSockaddr(IWSDUdpAddress *This, const SOCKADDR_STORAGE *pSockAddr)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);

    TRACE("(%p, %p)\n", This, pSockAddr);

    if (pSockAddr == NULL)
    {
        return E_POINTER;
    }

    memcpy(&impl->sockAddr, pSockAddr, sizeof(SOCKADDR_STORAGE));
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetSockaddr(IWSDUdpAddress *This, SOCKADDR_STORAGE *pSockAddr)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);
    SOCKADDR_IN *sockAddr = (SOCKADDR_IN *) &impl->sockAddr;

    TRACE("(%p, %p)\n", This, pSockAddr);

    if (pSockAddr == NULL)
    {
        return E_POINTER;
    }

    /* Ensure the sockaddr is initialised correctly */
    if ((sockAddr->sin_family != AF_INET) && (sockAddr->sin_family != AF_INET6))
    {
        return E_FAIL;
    }

    memcpy(pSockAddr, &impl->sockAddr, sizeof(SOCKADDR_STORAGE));
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetExclusive(IWSDUdpAddress *This, BOOL fExclusive)
{
    FIXME("(%p, %d)\n", This, fExclusive);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetExclusive(IWSDUdpAddress *This)
{
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetMessageType(IWSDUdpAddress *This, WSDUdpMessageType messageType)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);

    TRACE("(%p, %d)\n", This, messageType);

    impl->messageType = messageType;
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetMessageType(IWSDUdpAddress *This, WSDUdpMessageType *pMessageType)
{
    IWSDUdpAddressImpl *impl = impl_from_IWSDUdpAddress(This);

    TRACE("(%p, %p)\n", This, pMessageType);

    if (pMessageType == NULL)
    {
        return E_POINTER;
    }

    *pMessageType = impl->messageType;
    return S_OK;
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetTTL(IWSDUdpAddress *This, DWORD dwTTL)
{
    FIXME("(%p, %ld)\n", This, dwTTL);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetTTL(IWSDUdpAddress *This, DWORD *pdwTTL)
{
    FIXME("(%p, %p)\n", This, pdwTTL);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_SetAlias(IWSDUdpAddress *This, const GUID *pAlias)
{
    FIXME("(%p, %s)\n", This, debugstr_guid(pAlias));
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpAddressImpl_GetAlias(IWSDUdpAddress *This, GUID *pAlias)
{
    FIXME("(%p, %p)\n", This, pAlias);
    return E_NOTIMPL;
}

static const IWSDUdpAddressVtbl udpAddressVtbl =
{
    IWSDUdpAddressImpl_QueryInterface,
    IWSDUdpAddressImpl_AddRef,
    IWSDUdpAddressImpl_Release,
    IWSDUdpAddressImpl_Serialize,
    IWSDUdpAddressImpl_Deserialize,
    IWSDUdpAddressImpl_GetPort,
    IWSDUdpAddressImpl_SetPort,
    IWSDUdpAddressImpl_GetTransportAddress,
    IWSDUdpAddressImpl_GetTransportAddressEx,
    IWSDUdpAddressImpl_SetTransportAddress,
    IWSDUdpAddressImpl_SetSockaddr,
    IWSDUdpAddressImpl_GetSockaddr,
    IWSDUdpAddressImpl_SetExclusive,
    IWSDUdpAddressImpl_GetExclusive,
    IWSDUdpAddressImpl_SetMessageType,
    IWSDUdpAddressImpl_GetMessageType,
    IWSDUdpAddressImpl_SetTTL,
    IWSDUdpAddressImpl_GetTTL,
    IWSDUdpAddressImpl_SetAlias,
    IWSDUdpAddressImpl_GetAlias
};

HRESULT WINAPI WSDCreateUdpAddress(IWSDUdpAddress **ppAddress)
{
    IWSDUdpAddressImpl *obj;

    TRACE("(%p)\n", ppAddress);

    if (ppAddress == NULL)
    {
        WARN("Invalid parameter: ppAddress == NULL\n");
        return E_POINTER;
    }

    *ppAddress = NULL;

    obj = calloc(1, sizeof(*obj));

    if (!obj)
    {
        WARN("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    obj->IWSDUdpAddress_iface.lpVtbl = &udpAddressVtbl;
    obj->ref = 1;

    *ppAddress = &obj->IWSDUdpAddress_iface;
    TRACE("Returning iface %p\n", *ppAddress);

    return S_OK;
}
