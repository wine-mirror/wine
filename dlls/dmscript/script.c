/* IDirectMusicScript
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicScript IUnknown parts follow: */
HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	if (IsEqualGUID(riid, &IID_IUnknown) || 
	    IsEqualGUID(riid, &IID_IDirectMusicScript))
	{
		IDirectMusicScriptImpl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicScriptImpl_AddRef (LPDIRECTMUSICSCRIPT iface)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicScriptImpl_Release (LPDIRECTMUSICSCRIPT iface)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0)
	{
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicScript Interface follow: */
HRESULT WINAPI IDirectMusicScriptImpl_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %p): stub\n", This, pPerformance, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszRoutineName), pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, FIXME, %d, %p): stub\n", This, pwszVariableName,/* varValue,*/ fSetRef, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, pvarValue, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %li, %p): stub\n", This, pwszVariableName, lValue, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, plValue, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, punkValue, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %p, %s, %p, %p): stub\n", This, pwszVariableName, debugstr_guid(riid), ppv, pErrorInfo);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_QueryInterface,
	IDirectMusicScriptImpl_AddRef,
	IDirectMusicScriptImpl_Release,
	IDirectMusicScriptImpl_Init,
	IDirectMusicScriptImpl_CallRoutine,
	IDirectMusicScriptImpl_SetVariableVariant,
	IDirectMusicScriptImpl_GetVariableVariant,
	IDirectMusicScriptImpl_SetVariableNumber,
	IDirectMusicScriptImpl_GetVariableNumber,
	IDirectMusicScriptImpl_SetVariableObject,
	IDirectMusicScriptImpl_GetVariableObject,
	IDirectMusicScriptImpl_EnumRoutine,
	IDirectMusicScriptImpl_EnumVariable
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicScript (LPCGUID lpcGUID, LPDIRECTMUSICSCRIPT* ppDMScript, LPUNKNOWN pUnkOuter)
{
	if (IsEqualGUID (lpcGUID, &IID_IDirectMusicScript))
	{
		FIXME("Not yet\n");
		return E_NOINTERFACE;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;	
}
