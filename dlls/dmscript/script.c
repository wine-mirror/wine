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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

/* IDirectMusicScript IUnknown part: */
HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicScriptImpl,iface);

	if (IsEqualIID(riid, &IID_IUnknown) || 
	    IsEqualIID(riid, &IID_IDirectMusicScript)) {
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
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicScript IDirectMusicScript part: */
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
	IDirectMusicScriptImpl* dmscript;
	
	TRACE("(%p,%p,%p)\n",lpcGUID, ppDMScript, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicScript)) {
		dmscript = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptImpl));
		if (NULL == dmscript) {
			*ppDMScript = (LPDIRECTMUSICSCRIPT) NULL;
			return E_OUTOFMEMORY;
		}
		dmscript->lpVtbl = &DirectMusicScript_Vtbl;
		dmscript->ref = 1;
		*ppDMScript = (LPDIRECTMUSICSCRIPT) dmscript;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;	
}

/*****************************************************************************
 * IDirectMusicScriptObject implementation
 */
/* IDirectMusicScriptObject IUnknown part: */
HRESULT WINAPI IDirectMusicScriptObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID(riid, &IID_IDirectMusicObject)) {
		IDirectMusicScriptObject_AddRef(iface);
		*ppobj = This;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		IPersistStream_AddRef ((LPPERSISTSTREAM)This->pStream);
		*ppobj = (LPPERSISTSTREAM)This->pStream;
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicScript)) {
		IDirectMusicScript_AddRef ((LPDIRECTMUSICSCRIPT)This->pScript);
		*ppobj = (LPDIRECTMUSICSCRIPT)This->pScript;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicScriptObject_AddRef (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicScriptObject_Release (LPDIRECTMUSICOBJECT iface)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicScriptObject IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicScriptObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	pDesc = This->pDesc;
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);

	TRACE("(%p, %p)\n", This, pDesc);
	This->pDesc = pDesc;

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicScriptObject,iface);

	FIXME("(%p, %p, %p): stub\n", This, pStream, pDesc);

	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicScriptObject_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptObject_QueryInterface,
	IDirectMusicScriptObject_AddRef,
	IDirectMusicScriptObject_Release,
	IDirectMusicScriptObject_GetDescriptor,
	IDirectMusicScriptObject_SetDescriptor,
	IDirectMusicScriptObject_ParseDescriptor
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicScriptObject (LPCGUID lpcGUID, LPDIRECTMUSICOBJECT* ppObject, LPUNKNOWN pUnkOuter)
{
	IDirectMusicScriptObject *obj;
	
	TRACE("(%p,%p,%p)\n", lpcGUID, ppObject, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicObject)) {
		obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptObject));
		if (NULL == obj) {
			*ppObject = (LPDIRECTMUSICOBJECT) NULL;
			return E_OUTOFMEMORY;
		}
		obj->lpVtbl = &DirectMusicScriptObject_Vtbl;
		obj->ref = 1;
		/* prepare IPersistStream */
		obj->pStream = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptObjectStream));
		obj->pStream->lpVtbl = &DirectMusicScriptObjectStream_Vtbl;
		obj->pStream->ref = 1;	
		obj->pStream->pParentObject = obj;
		/* prepare IDirectMusicScript */
		DMUSIC_CreateDirectMusicScript (&IID_IDirectMusicScript, (LPDIRECTMUSICSCRIPT*)&obj->pScript, NULL);
		obj->pScript->pObject = obj;
		*ppObject = (LPDIRECTMUSICOBJECT) obj;
		return S_OK;
	}
	WARN("No interface found\n");
	
	return E_NOINTERFACE;
}
	
/*****************************************************************************
 * IDirectMusicScriptObjectStream implementation
 */
/* IDirectMusicScriptObjectStream IUnknown part: */
HRESULT WINAPI IDirectMusicScriptObjectStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicScriptObjectStream,iface);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IPersistStream)) {
		IDirectMusicScriptObjectStream_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicScriptObjectStream_AddRef (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicScriptObjectStream,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicScriptObjectStream_Release (LPPERSISTSTREAM iface)
{
	ICOM_THIS(IDirectMusicScriptObjectStream,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicScriptObjectStream IPersist part: */
HRESULT WINAPI IDirectMusicScriptObjectStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID)
{
        *pClassID = CLSID_DirectMusicScript;
	return S_OK;
}

/* IDirectMusicScriptObjectStream IPersistStream part: */
HRESULT WINAPI IDirectMusicScriptObjectStream_IsDirty (LPPERSISTSTREAM iface)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicScriptObjectStream_Load (LPPERSISTSTREAM iface, IStream* pStm)
{
	FIXME(": Loading not implemented yet\n");
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptObjectStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicScriptObjectStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize)
{
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicScriptObjectStream_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptObjectStream_QueryInterface,
	IDirectMusicScriptObjectStream_AddRef,
	IDirectMusicScriptObjectStream_Release,
	IDirectMusicScriptObjectStream_GetClassID,
	IDirectMusicScriptObjectStream_IsDirty,
	IDirectMusicScriptObjectStream_Load,
	IDirectMusicScriptObjectStream_Save,
	IDirectMusicScriptObjectStream_GetSizeMax
};
