/*
 *	AutoComplete interfaces implementation.
 *
 *	Copyright 2004	Maxime Bellengé <maxime.bellenge@laposte.net>
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "pidl.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shldisp.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
    ICOM_VFIELD(IAutoComplete);
    ICOM_VTABLE (IAutoComplete2) * lpvtblAutoComplete2;
    DWORD ref;
    BOOL  enable;
    AUTOCOMPLETEOPTIONS options;
} IAutoCompleteImpl;

static struct ICOM_VTABLE(IAutoComplete) acvt;
static struct ICOM_VTABLE(IAutoComplete2) ac2vt;

#define _IAutoComplete2_Offset ((int)(&(((IAutoCompleteImpl*)0)->lpvtblAutoComplete2)))
#define _ICOM_THIS_From_IAutoComplete2(class, name) class* This = (class*)(((char*)name)-_IAutoComplete2_Offset);

/*
  converts This to a interface pointer
*/
#define _IUnknown_(This) (IUnknown*)&(This->lpVtbl)
#define _IAutoComplete2_(This)  (IAutoComplete2*)&(This->lpvtblAutoComplete2) 

/**************************************************************************
 *  IAutoComplete_Constructor
 */
HRESULT WINAPI IAutoComplete_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IAutoCompleteImpl *lpac;

    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
	return CLASS_E_NOAGGREGATION;

    lpac = (IAutoCompleteImpl*)HeapAlloc(GetProcessHeap(),
      HEAP_ZERO_MEMORY, sizeof(IAutoCompleteImpl));
    if (!lpac) 
	return E_OUTOFMEMORY;

    lpac->ref = 1;
    lpac->lpVtbl = &acvt;
    lpac->lpvtblAutoComplete2 = &ac2vt;
    lpac->enable = TRUE;
    
    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (lpac), riid, ppv))) {
	IUnknown_Release (_IUnknown_ (lpac));
	return E_NOINTERFACE;
    }
    
    TRACE("-- (%p)->\n",lpac);

    return S_OK;
}

/**************************************************************************
 *  AutoComplete_QueryInterface
 */
static HRESULT WINAPI IAutoComplete_fnQueryInterface(
    IAutoComplete * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS(IAutoCompleteImpl, iface);
    
    TRACE("(%p)->(\n\tIID:\t%s,%p)\n", This, shdebugstr_guid(riid), ppvObj);
    *ppvObj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown)) 
    {
	*ppvObj = This;
    } 
    else if(IsEqualIID(riid, &IID_IAutoComplete))
    {
	*ppvObj = (IAutoComplete*)This;
    }
    else if(IsEqualIID(riid, &IID_IAutoComplete2))
    {
	*ppvObj = _IAutoComplete2_ (This);
    }

    if (*ppvObj)
    {
	IAutoComplete_AddRef((IAutoComplete*)*ppvObj);
	TRACE("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;	
}

/******************************************************************************
 * IAutoComplete_fnAddRef
 */
static ULONG WINAPI IAutoComplete_fnAddRef(
	IAutoComplete * iface)
{
    ICOM_THIS(IAutoCompleteImpl,iface);
    
    TRACE("(%p)->(%lu)\n",This,This->ref);
    return ++(This->ref);
}

/******************************************************************************
 * IAutoComplete_fnRelease
 */
static ULONG WINAPI IAutoComplete_fnRelease(
	IAutoComplete * iface)
{
    ICOM_THIS(IAutoCompleteImpl,iface);
    
    TRACE("(%p)->(%lu)\n",This,This->ref);

    if (!--(This->ref)) {
	TRACE(" destroying IAutoComplete(%p)\n",This);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

/******************************************************************************
 * IAutoComplete_fnEnable
 */
static HRESULT WINAPI IAutoComplete_fnEnable(
    IAutoComplete * iface,
    BOOL fEnable)
{
    ICOM_THIS(IAutoCompleteImpl, iface);

    HRESULT hr = S_OK;

    TRACE("(%p)->(%s)\n", This, (fEnable)?"true":"false");

    This->enable = fEnable;

    return hr;
}

/******************************************************************************
 * IAutoComplete_fnInit
 */
static HRESULT WINAPI IAutoComplete_fnInit(
    IAutoComplete * iface,
    HWND hwndEdit,
    IUnknown *punkACL,
    LPCOLESTR pwzsRegKeyPath,
    LPCOLESTR pwszQuickComplete)
{
    ICOM_THIS(IAutoCompleteImpl, iface);
    
    HRESULT hr = E_NOTIMPL;

    FIXME("(%p)->(0x%08lx, %p, %s, %s) not implemented\n", 
	  This, (long)hwndEdit, punkACL, debugstr_w(pwzsRegKeyPath), debugstr_w(pwszQuickComplete));

    return hr;
}

/**************************************************************************
 *  IAutoComplete_fnVTable
 */
static ICOM_VTABLE (IAutoComplete) acvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IAutoComplete_fnQueryInterface,
    IAutoComplete_fnAddRef,
    IAutoComplete_fnRelease,
    IAutoComplete_fnInit,
    IAutoComplete_fnEnable,
};

/**************************************************************************
 *  AutoComplete2_QueryInterface
 */
static HRESULT WINAPI IAutoComplete2_fnQueryInterface(
    IAutoComplete2 * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl, iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    return IAutoComplete_QueryInterface((IAutoComplete*)This, riid, ppvObj);
}

/******************************************************************************
 * IAutoComplete2_fnAddRef
 */
static ULONG WINAPI IAutoComplete2_fnAddRef(
	IAutoComplete2 * iface)
{
    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl,iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IAutoComplete2_AddRef((IAutoComplete*)This);
}

/******************************************************************************
 * IAutoComplete2_fnRelease
 */
static ULONG WINAPI IAutoComplete2_fnRelease(
	IAutoComplete2 * iface)
{
    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl,iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IAutoComplete_Release((IAutoComplete*)This);
}

/******************************************************************************
 * IAutoComplete2_fnEnable
 */
static HRESULT WINAPI IAutoComplete2_fnEnable(
    IAutoComplete2 * iface,
    BOOL fEnable)
{
    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl, iface);

    TRACE ("(%p)->(%s)\n", This, (fEnable)?"true":"false");

    return IAutoComplete_Enable((IAutoComplete*)This, fEnable);
}

/******************************************************************************
 * IAutoComplete2_fnInit
 */
static HRESULT WINAPI IAutoComplete2_fnInit(
    IAutoComplete2 * iface,
    HWND hwndEdit,
    IUnknown *punkACL,
    LPCOLESTR pwzsRegKeyPath,
    LPCOLESTR pwszQuickComplete)
{
    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl, iface);

    TRACE("(%p)\n", This);

    return IAutoComplete_Init((IAutoComplete*)This, hwndEdit, punkACL, pwzsRegKeyPath, pwszQuickComplete);
}

/**************************************************************************
 *  IAutoComplete_fnGetOptions
 */
static HRESULT WINAPI IAutoComplete2_fnGetOptions(
    IAutoComplete2 * iface,
    DWORD *pdwFlag)
{
    HRESULT hr = S_OK;

    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl, iface);

    TRACE("(%p) -> (%p)\n", This, pdwFlag);

    *pdwFlag = This->options;

    return hr;
}

/**************************************************************************
 *  IAutoComplete_fnSetOptions
 */
static HRESULT WINAPI IAutoComplete2_fnSetOptions(
    IAutoComplete2 * iface,
    DWORD dwFlag)
{
    HRESULT hr = S_OK;

    _ICOM_THIS_From_IAutoComplete2(IAutoCompleteImpl, iface);

    FIXME("(%p) -> (0x%lx) not implemented\n", This, dwFlag);

    return hr;
}

/**************************************************************************
 *  IAutoComplete2_fnVTable
 */
static ICOM_VTABLE (IAutoComplete2) ac2vt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IAutoComplete2_fnQueryInterface,
    IAutoComplete2_fnAddRef,
    IAutoComplete2_fnRelease,
    IAutoComplete2_fnInit,
    IAutoComplete2_fnEnable,
    /* IAutoComplete2 */
    IAutoComplete2_fnSetOptions,
    IAutoComplete2_fnGetOptions,
};
