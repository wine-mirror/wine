/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_IOSERVICE_CLASSNAME "nsIOService"
#define NS_IOSERVICE_CONTRACTID "@mozilla.org/network/io-service;1"

static IID NS_IOSERVICE_CID =
    {0x9ac9e770, 0x18bc, 0x11d3, {0x93, 0x37, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40}};

static nsIIOService *nsio = NULL;

typedef struct {
    const nsIWineURIVtbl *lpWineURIVtbl;

    LONG ref;

    nsIURI *uri;
    NSContainer *container;
} nsURI;

#define NSURI(x)         ((nsIURI*)            &(x)->lpWineURIVtbl)

#define NSURI_THIS(iface) DEFINE_THIS(nsURI, WineURI, iface)

static nsresult NSAPI nsURI_QueryInterface(nsIWineURI *iface, nsIIDRef riid, nsQIResult result)
{
    nsURI *This = NSURI_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIURI, riid)) {
        TRACE("(%p)->(IID_nsIURI %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIWineURI, riid)) {
        TRACE("(%p)->(IID_nsIWineURI %p)\n", This, result);
        *result = NSURI(This);
    }

    if(*result) {
        nsIURI_AddRef(NSURI(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return nsIURI_QueryInterface(This->uri, riid, result);
}

static nsrefcnt NSAPI nsURI_AddRef(nsIWineURI *iface)
{
    nsURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsURI_Release(nsIWineURI *iface)
{
    nsURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        nsIURI_Release(This->uri);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static nsresult NSAPI nsURI_GetSpec(nsIWineURI *iface, nsACString *aSpec)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aSpec);
    return nsIURI_GetSpec(This->uri, aSpec);
}

static nsresult NSAPI nsURI_SetSpec(nsIWineURI *iface, const nsACString *aSpec)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aSpec);
    return nsIURI_SetSpec(This->uri, aSpec);
}

static nsresult NSAPI nsURI_GetPrePath(nsIWineURI *iface, nsACString *aPrePath)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPrePath);
    return nsIURI_GetPrePath(This->uri, aPrePath);
}

static nsresult NSAPI nsURI_GetScheme(nsIWineURI *iface, nsACString *aScheme)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aScheme);
    return nsIURI_GetScheme(This->uri, aScheme);
}

static nsresult NSAPI nsURI_SetScheme(nsIWineURI *iface, const nsACString *aScheme)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aScheme);
    return nsIURI_SetScheme(This->uri, aScheme);
}

static nsresult NSAPI nsURI_GetUserPass(nsIWineURI *iface, nsACString *aUserPass)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aUserPass);
    return nsIURI_GetUserPass(This->uri, aUserPass);
}

static nsresult NSAPI nsURI_SetUserPass(nsIWineURI *iface, const nsACString *aUserPass)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aUserPass);
    return nsIURI_SetUserPass(This->uri, aUserPass);
}

static nsresult NSAPI nsURI_GetUsername(nsIWineURI *iface, nsACString *aUsername)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aUsername);
    return nsIURI_GetUsername(This->uri, aUsername);
}

static nsresult NSAPI nsURI_SetUsername(nsIWineURI *iface, const nsACString *aUsername)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aUsername);
    return nsIURI_SetUsername(This->uri, aUsername);
}

static nsresult NSAPI nsURI_GetPassword(nsIWineURI *iface, nsACString *aPassword)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPassword);
    return nsIURI_GetPassword(This->uri, aPassword);
}

static nsresult NSAPI nsURI_SetPassword(nsIWineURI *iface, const nsACString *aPassword)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPassword);
    return nsIURI_SetPassword(This->uri, aPassword);
}

static nsresult NSAPI nsURI_GetHostPort(nsIWineURI *iface, nsACString *aHostPort)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aHostPort);
    return nsIURI_GetHostPort(This->uri, aHostPort);
}

static nsresult NSAPI nsURI_SetHostPort(nsIWineURI *iface, const nsACString *aHostPort)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aHostPort);
    return nsIURI_SetHostPort(This->uri, aHostPort);
}

static nsresult NSAPI nsURI_GetHost(nsIWineURI *iface, nsACString *aHost)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aHost);
    return nsIURI_GetHost(This->uri, aHost);
}

static nsresult NSAPI nsURI_SetHost(nsIWineURI *iface, const nsACString *aHost)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aHost);
    return nsIURI_SetHost(This->uri, aHost);
}

static nsresult NSAPI nsURI_GetPort(nsIWineURI *iface, PRInt32 *aPort)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPort);
    return nsIURI_GetPort(This->uri, aPort);
}

static nsresult NSAPI nsURI_SetPort(nsIWineURI *iface, PRInt32 aPort)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%ld)\n", This, aPort);
    return nsIURI_SetPort(This->uri, aPort);
}

static nsresult NSAPI nsURI_GetPath(nsIWineURI *iface, nsACString *aPath)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPath);
    return nsIURI_GetPath(This->uri, aPath);
}

static nsresult NSAPI nsURI_SetPath(nsIWineURI *iface, const nsACString *aPath)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aPath);
    return nsIURI_SetPath(This->uri, aPath);
}

static nsresult NSAPI nsURI_Equals(nsIWineURI *iface, nsIURI *other, PRBool *_retval)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p %p)\n", This, other, _retval);
    return nsIURI_Equals(This->uri, other, _retval);
}

static nsresult NSAPI nsURI_SchemeIs(nsIWineURI *iface, const char *scheme, PRBool *_retval)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_a(scheme), _retval);
    return nsIURI_SchemeIs(This->uri, scheme, _retval);
}

static nsresult NSAPI nsURI_Clone(nsIWineURI *iface, nsIURI **_retval)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, _retval);
    return nsIURI_Clone(This->uri, _retval);
}

static nsresult NSAPI nsURI_Resolve(nsIWineURI *iface, const nsACString *arelativePath,
        nsACString *_retval)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p %p)\n", This, arelativePath, _retval);
    return nsIURI_Resolve(This->uri, arelativePath, _retval);
}

static nsresult NSAPI nsURI_GetAsciiSpec(nsIWineURI *iface, nsACString *aAsciiSpec)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aAsciiSpec);
    return nsIURI_GetAsciiSpec(This->uri, aAsciiSpec);
}

static nsresult NSAPI nsURI_GetAsciiHost(nsIWineURI *iface, nsACString *aAsciiHost)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aAsciiHost);
    return nsIURI_GetAsciiHost(This->uri, aAsciiHost);
}

static nsresult NSAPI nsURI_GetOriginCharset(nsIWineURI *iface, nsACString *aOriginCharset)
{
    nsURI *This = NSURI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, aOriginCharset);
    return nsIURI_GetOriginCharset(This->uri, aOriginCharset);
}

static nsresult NSAPI nsURI_GetNSContainer(nsIWineURI *iface, NSContainer **aContainer)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContainer);

    if(This->container)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(This->container));
    *aContainer = This->container;

    return NS_OK;
}

static nsresult NSAPI nsURI_SetNSContainer(nsIWineURI *iface, NSContainer *aContainer)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContainer);

    if(This->container) {
        WARN("Container already set: %p\n", This->container);
        nsIWebBrowserChrome_Release(NSWBCHROME(This->container));
    }

    if(aContainer)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(aContainer));
    This->container = aContainer;

    return NS_OK;
}

#undef NSURI_THIS

static const nsIWineURIVtbl nsWineURIVtbl = {
    nsURI_QueryInterface,
    nsURI_AddRef,
    nsURI_Release,
    nsURI_GetSpec,
    nsURI_SetSpec,
    nsURI_GetPrePath,
    nsURI_GetScheme,
    nsURI_SetScheme,
    nsURI_GetUserPass,
    nsURI_SetUserPass,
    nsURI_GetUsername,
    nsURI_SetUsername,
    nsURI_GetPassword,
    nsURI_SetPassword,
    nsURI_GetHostPort,
    nsURI_SetHostPort,
    nsURI_GetHost,
    nsURI_SetHost,
    nsURI_GetPort,
    nsURI_SetPort,
    nsURI_GetPath,
    nsURI_SetPath,
    nsURI_Equals,
    nsURI_SchemeIs,
    nsURI_Clone,
    nsURI_Resolve,
    nsURI_GetAsciiSpec,
    nsURI_GetAsciiHost,
    nsURI_GetOriginCharset,
    nsURI_GetNSContainer,
    nsURI_SetNSContainer,
};

static nsresult NSAPI nsIOService_QueryInterface(nsIIOService *iface, nsIIDRef riid,
                                                 nsQIResult result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIIOService, riid)) {
        TRACE("(IID_nsIIOService %p)\n", result);
        *result = iface;
    }

    if(*result) {
        nsIIOService_AddRef(iface);
        return S_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsIOService_AddRef(nsIIOService *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsIOService_Release(nsIIOService *iface)
{
    return 1;
}

static nsresult NSAPI nsIOService_GetProtocolHandler(nsIIOService *iface, const char *aScheme,
                                                     nsIProtocolHandler **_retval)
{
    TRACE("(%s %p)\n", debugstr_a(aScheme), _retval);
    return nsIIOService_GetProtocolHandler(nsio, aScheme, _retval);
}

static nsresult NSAPI nsIOService_GetProtocolFlags(nsIIOService *iface, const char *aScheme,
                                                    PRUint32 *_retval)
{
    TRACE("(%s %p)\n", debugstr_a(aScheme), _retval);
    return nsIIOService_GetProtocolFlags(nsio, aScheme, _retval);
}

static nsresult NSAPI nsIOService_NewURI(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    const char *spec = NULL;
    nsIURI *uri;
    nsURI *ret;
    PRBool is_javascript = FALSE;
    nsresult nsres;

    nsACString_GetData(aSpec, &spec, NULL);

    TRACE("(%p(%s) %s %p %p)\n", aSpec, debugstr_a(spec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    if(aBaseURI) {
        nsACString *base_uri_str = nsACString_Create();
        const char *base_uri = NULL;

        nsres = nsIURI_GetSpec(aBaseURI, base_uri_str);
        if(NS_SUCCEEDED(nsres)) {
            nsACString_GetData(base_uri_str, &base_uri, NULL);
            TRACE("uri=%s\n", debugstr_a(base_uri));
        }else {
            ERR("GetSpec failed: %08lx\n", nsres);
        }

        nsACString_Destroy(base_uri_str);
    }

    nsres = nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, &uri);
    if(NS_FAILED(nsres)) {
        WARN("NewURI failed: %08lx\n", nsres);
        return nsres;
    }

    nsIURI_SchemeIs(uri, "javascript", &is_javascript);
    if(is_javascript) {
        TRACE("returning javascript uri: %p\n", uri);
        *_retval = uri;
        return NS_OK;
    }

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(nsURI));

    ret->lpWineURIVtbl = &nsWineURIVtbl;
    ret->ref = 1;
    ret->uri = uri;
    ret->container = NULL;

    TRACE("_retval = %p\n", ret);
    *_retval = NSURI(ret);
    return NS_OK;
}

static nsresult NSAPI nsIOService_NewFileURI(nsIIOService *iface, nsIFile *aFile,
                                             nsIURI **_retval)
{
    TRACE("(%p %p)\n", aFile, _retval);
    return nsIIOService_NewFileURI(nsio, aFile, _retval);
}

static nsresult NSAPI nsIOService_NewChannelFromURI(nsIIOService *iface, nsIURI *aURI,
                                                     nsIChannel **_retval)
{
    TRACE("(%p %p)\n", aURI, _retval);
    return nsIIOService_NewChannelFromURI(nsio, aURI, _retval);
}

static nsresult NSAPI nsIOService_NewChannel(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIChannel **_retval)
{
    TRACE("(%p %s %p %p)\n", aSpec, debugstr_a(aOriginCharset), aBaseURI, _retval);
    return nsIIOService_NewChannel(nsio, aSpec, aOriginCharset, aBaseURI, _retval);
}

static nsresult NSAPI nsIOService_GetOffline(nsIIOService *iface, PRBool *aOffline)
{
    TRACE("(%p)\n", aOffline);
    return nsIIOService_GetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_SetOffline(nsIIOService *iface, PRBool aOffline)
{
    TRACE("(%x)\n", aOffline);
    return nsIIOService_SetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_AllowPort(nsIIOService *iface, PRInt32 aPort,
                                             const char *aScheme, PRBool *_retval)
{
    TRACE("(%ld %s %p)\n", aPort, debugstr_a(aScheme), _retval);
    return nsIIOService_AllowPort(nsio, aPort, debugstr_a(aScheme), _retval);
}

static nsresult NSAPI nsIOService_ExtractScheme(nsIIOService *iface, const nsACString *urlString,
                                                 nsACString * _retval)
{
    TRACE("(%p %p)\n", urlString, _retval);
    return nsIIOService_ExtractScheme(nsio, urlString, _retval);
}

static const nsIIOServiceVtbl nsIOServiceVtbl = {
    nsIOService_QueryInterface,
    nsIOService_AddRef,
    nsIOService_Release,
    nsIOService_GetProtocolHandler,
    nsIOService_GetProtocolFlags,
    nsIOService_NewURI,
    nsIOService_NewFileURI,
    nsIOService_NewChannelFromURI,
    nsIOService_NewChannel,
    nsIOService_GetOffline,
    nsIOService_SetOffline,
    nsIOService_AllowPort,
    nsIOService_ExtractScheme
};

static nsIIOService nsIOService = { &nsIOServiceVtbl };

static nsresult NSAPI nsIOServiceFactory_QueryInterface(nsIFactory *iface, nsIIDRef riid,
                                                        nsQIResult result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupoprts %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIFactory, riid)) {
        TRACE("(IID_nsIFactory %p)\n", result);
        *result = iface;
    }

    if(*result) {
        nsIFactory_AddRef(iface);
        return NS_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsIOServiceFactory_AddRef(nsIFactory *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsIOServiceFactory_Release(nsIFactory *iface)
{
    return 1;
}

static nsresult NSAPI nsIOServiceFactory_CreateInstance(nsIFactory *iface,
        nsISupports *aOuter, const nsIID *iid, void **result)
{
    return nsIIOService_QueryInterface(&nsIOService, iid, result);
}

static nsresult NSAPI nsIOServiceFactory_LockFactory(nsIFactory *iface, PRBool lock)
{
    WARN("(%x)\n", lock);
    return NS_OK;
}

static const nsIFactoryVtbl nsIOServiceFactoryVtbl = {
    nsIOServiceFactory_QueryInterface,
    nsIOServiceFactory_AddRef,
    nsIOServiceFactory_Release,
    nsIOServiceFactory_CreateInstance,
    nsIOServiceFactory_LockFactory
};

static nsIFactory nsIOServiceFactory = { &nsIOServiceFactoryVtbl };

void init_nsio(nsIComponentManager *component_manager, nsIComponentRegistrar *registrar)
{
    nsIFactory *old_factory = NULL;
    nsresult nsres;

    nsres = nsIComponentManager_GetClassObject(component_manager, &NS_IOSERVICE_CID,
                                               &IID_nsIFactory, (void**)&old_factory);
    if(NS_FAILED(nsres)) {
        ERR("Could not get factory: %08lx\n", nsres);
        nsIFactory_Release(old_factory);
        return;
    }

    nsres = nsIFactory_CreateInstance(old_factory, NULL, &IID_nsIIOService, (void**)&nsio);
    if(NS_FAILED(nsres)) {
        ERR("Couldn not create nsIOService instance %08lx\n", nsres);
        nsIFactory_Release(old_factory);
        return;
    }

    nsres = nsIComponentRegistrar_UnregisterFactory(registrar, &NS_IOSERVICE_CID, old_factory);
    nsIFactory_Release(old_factory);
    if(NS_FAILED(nsres))
        ERR("UnregisterFactory failed: %08lx\n", nsres);

    nsres = nsIComponentRegistrar_RegisterFactory(registrar, &NS_IOSERVICE_CID,
            NS_IOSERVICE_CLASSNAME, NS_IOSERVICE_CONTRACTID, &nsIOServiceFactory);
    if(NS_FAILED(nsres))
        ERR("RegisterFactory failed: %08lx\n", nsres);
}

nsIURI *get_nsIURI(LPCWSTR url)
{
    nsIURI *ret;
    nsACString *acstr;
    nsresult nsres;
    char *urla;
    int len;

    len = WideCharToMultiByte(CP_ACP, 0, url, -1, NULL, -1, NULL, NULL);
    urla = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_ACP, 0, url, -1, urla, -1, NULL, NULL);

    acstr = nsACString_Create();
    nsACString_SetData(acstr, urla);

    nsres = nsIIOService_NewURI(nsio, acstr, NULL, NULL, &ret);
    if(NS_FAILED(nsres))
        FIXME("NewURI failed: %08lx\n", nsres);

    nsACString_Destroy(acstr);
    HeapFree(GetProcessHeap(), 0, urla);

    return ret;
}
