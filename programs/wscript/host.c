/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <ole2.h>

#include "wscript.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wscript);

static HRESULT WINAPI Host_QueryInterface(IHost *iface, REFIID riid, void **ppv)
{
    WINE_TRACE("(%s %p)\n", wine_dbgstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)
       || IsEqualGUID(&IID_IDispatch, riid)
       || IsEqualGUID(&IID_IHost, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Host_AddRef(IHost *iface)
{
    return 2;
}

static ULONG WINAPI Host_Release(IHost *iface)
{
    return 1;
}

static HRESULT WINAPI Host_GetTypeInfoCount(IHost *iface, UINT *pctinfo)
{
    WINE_TRACE("(%p)\n", pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI Host_GetTypeInfo(IHost *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    WINE_TRACE("(%x %x %p\n", iTInfo, lcid, ppTInfo);

    ITypeInfo_AddRef(host_ti);
    *ppTInfo = host_ti;
    return S_OK;
}

static HRESULT WINAPI Host_GetIDsOfNames(IHost *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WINE_TRACE("(%s %p %d %x %p)\n", wine_dbgstr_guid(riid), rgszNames,
        cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(host_ti, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI Host_Invoke(IHost *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WINE_TRACE("(%d %p %p)\n", dispIdMember, pDispParams, pVarResult);

    return ITypeInfo_Invoke(host_ti, iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI Host_get_Name(IHost *iface, BSTR *out_Name)
{
    WINE_FIXME("(%p)\n", out_Name);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Application(IHost *iface, IDispatch **out_Dispatch)
{
    WINE_FIXME("(%p)\n", out_Dispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_FullName(IHost *iface, BSTR *out_Path)
{
    WINE_FIXME("(%p)\n", out_Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Path(IHost *iface, BSTR *out_Path)
{
    WINE_FIXME("(%p)\n", out_Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Interactive(IHost *iface, VARIANT_BOOL *out_Interactive)
{
    WINE_FIXME("(%p)\n", out_Interactive);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_put_Interactive(IHost *iface, VARIANT_BOOL v)
{
    WINE_FIXME("(%x)\n", v);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_Quit(IHost *iface, int ExitCode)
{
    WINE_FIXME("(%d)\n", ExitCode);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_ScriptName(IHost *iface, BSTR *out_ScriptName)
{
    WINE_FIXME("(%p)\n", out_ScriptName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_ScriptFullName(IHost *iface, BSTR *out_ScriptFullName)
{
    WINE_FIXME("(%p)\n", out_ScriptFullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Arguments(IHost *iface, IArguments2 **out_Arguments)
{
    WINE_FIXME("(%p)\n", out_Arguments);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Version(IHost *iface, BSTR *out_Version)
{
    WINE_FIXME("(%p)\n", out_Version);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_BuildVersion(IHost *iface, int *out_Build)
{
    WINE_FIXME("(%p)\n", out_Build);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_Timeout(IHost *iface, LONG *out_Timeout)
{
    WINE_FIXME("(%p)\n", out_Timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_put_Timeout(IHost *iface, LONG v)
{
    WINE_FIXME("(%d)\n", v);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_CreateObject(IHost *iface, BSTR ProgID, BSTR Prefix,
        IDispatch **out_Dispatch)
{
    WINE_FIXME("(%s %s %p)\n", wine_dbgstr_w(ProgID), wine_dbgstr_w(Prefix), out_Dispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_Echo(IHost *iface, SAFEARRAY *args)
{
    WINE_FIXME("(%p)\n", args);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_GetObject(IHost *iface, BSTR Pathname, BSTR ProgID,
        BSTR Prefix, IDispatch **out_Dispatch)
{
    WINE_FIXME("(%s %s %s %p)\n", wine_dbgstr_w(Pathname), wine_dbgstr_w(ProgID),
        wine_dbgstr_w(Prefix), out_Dispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_DisconnectObject(IHost *iface, IDispatch *Object)
{
    WINE_FIXME("(%p)\n", Object);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_Sleep(IHost *iface, LONG Time)
{
    WINE_FIXME("(%d)\n", Time);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_ConnectObject(IHost *iface, IDispatch *Object, BSTR Prefix)
{
    WINE_FIXME("(%p %s)\n", Object, wine_dbgstr_w(Prefix));
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdIn(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdOut(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static HRESULT WINAPI Host_get_StdErr(IHost *iface, ITextStream **ppts)
{
    WINE_FIXME("(%p)\n", ppts);
    return E_NOTIMPL;
}

static const IHostVtbl HostVtbl = {
    Host_QueryInterface,
    Host_AddRef,
    Host_Release,
    Host_GetTypeInfoCount,
    Host_GetTypeInfo,
    Host_GetIDsOfNames,
    Host_Invoke,
    Host_get_Name,
    Host_get_Application,
    Host_get_FullName,
    Host_get_Path,
    Host_get_Interactive,
    Host_put_Interactive,
    Host_Quit,
    Host_get_ScriptName,
    Host_get_ScriptFullName,
    Host_get_Arguments,
    Host_get_Version,
    Host_get_BuildVersion,
    Host_get_Timeout,
    Host_put_Timeout,
    Host_CreateObject,
    Host_Echo,
    Host_GetObject,
    Host_DisconnectObject,
    Host_Sleep,
    Host_ConnectObject,
    Host_get_StdIn,
    Host_get_StdOut,
    Host_get_StdErr
};

IHost host_obj = { &HostVtbl };
