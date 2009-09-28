/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HOSTSECMGR_THIS(iface) DEFINE_THIS(HTMLDocumentNode, IInternetHostSecurityManager, iface)

static HRESULT WINAPI InternetHostSecurityManager_QueryInterface(IInternetHostSecurityManager *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->node), riid, ppv);
}

static ULONG WINAPI InternetHostSecurityManager_AddRef(IInternetHostSecurityManager *iface)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->node));
}

static ULONG WINAPI InternetHostSecurityManager_Release(IInternetHostSecurityManager *iface)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->node));
}

static HRESULT WINAPI InternetHostSecurityManager_GetSecurityId(IInternetHostSecurityManager *iface,  BYTE *pbSecurityId,
        DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    FIXME("(%p)->(%p %p %lx)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetHostSecurityManager_ProcessUrlAction(IInternetHostSecurityManager *iface, DWORD dwAction,
        BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    FIXME("%p)->(%d %p %d %p %d %x %x)\n", This, dwAction, pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetHostSecurityManager_QueryCustomPolicy(IInternetHostSecurityManager *iface, REFGUID guidKey,
        BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    FIXME("(%p)->(%s %p %p %p %d %x)\n", This, debugstr_guid(guidKey), ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);
    return E_NOTIMPL;
}

#undef HOSTSECMGR_THIS

static const IInternetHostSecurityManagerVtbl InternetHostSecurityManagerVtbl = {
    InternetHostSecurityManager_QueryInterface,
    InternetHostSecurityManager_AddRef,
    InternetHostSecurityManager_Release,
    InternetHostSecurityManager_GetSecurityId,
    InternetHostSecurityManager_ProcessUrlAction,
    InternetHostSecurityManager_QueryCustomPolicy
};

void HTMLDocumentNode_SecMgr_Init(HTMLDocumentNode *This)
{
    This->lpIInternetHostSecurityManagerVtbl = &InternetHostSecurityManagerVtbl;
}
