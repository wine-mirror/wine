/* IDirectMusicScript
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/*****************************************************************************
 * IDirectMusicScriptImpl implementation
 */
/* IDirectMusicScriptImpl IUnknown part: */
HRESULT WINAPI IDirectMusicScriptImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, UnknownVtbl, iface);
	
	if (IsEqualIID (riid, &IID_IUnknown)) {
		*ppobj = (LPVOID)&This->UnknownVtbl;
		IDirectMusicScriptImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
		return S_OK;	
	} else if (IsEqualIID (riid, &IID_IDirectMusicScript)) {
		*ppobj = (LPVOID)&This->ScriptVtbl;
		IDirectMusicScriptImpl_IDirectMusicScript_AddRef ((LPDIRECTMUSICSCRIPT)&This->ScriptVtbl);
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IDirectMusicObject)) {
		*ppobj = (LPVOID)&This->ObjectVtbl;
		IDirectMusicScriptImpl_IDirectMusicObject_AddRef ((LPDIRECTMUSICOBJECT)&This->ObjectVtbl);		
		return S_OK;
	} else if (IsEqualIID (riid, &IID_IPersistStream)) {
		*ppobj = (LPVOID)&This->PersistStreamVtbl;
		IDirectMusicScriptImpl_IPersistStream_AddRef ((LPPERSISTSTREAM)&This->PersistStreamVtbl);		
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicScriptImpl_IUnknown_AddRef (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, UnknownVtbl, iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicScriptImpl_IUnknown_Release (LPUNKNOWN iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, UnknownVtbl, iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

ICOM_VTABLE(IUnknown) DirectMusicScript_Unknown_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_IUnknown_QueryInterface,
	IDirectMusicScriptImpl_IUnknown_AddRef,
	IDirectMusicScriptImpl_IUnknown_Release
};

/* IDirectMusicScriptImpl IDirectMusicScript part: */
HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicScript_AddRef (LPDIRECTMUSICSCRIPT iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicScript_Release (LPDIRECTMUSICSCRIPT iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %p): stub\n", This, pPerformance, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszRoutineName), pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, FIXME, %d, %p): stub\n", This, pwszVariableName,/* varValue,*/ fSetRef, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, pvarValue, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %li, %p): stub\n", This, pwszVariableName, lValue, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, plValue, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %p, %p): stub\n", This, pwszVariableName, punkValue, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %p, %s, %p, %p): stub\n", This, pwszVariableName, debugstr_guid(riid), ppv, pErrorInfo);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicScript_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ScriptVtbl, iface);
	FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
	return S_OK;
}

ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Script_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_IDirectMusicScript_QueryInterface,
	IDirectMusicScriptImpl_IDirectMusicScript_AddRef,
	IDirectMusicScriptImpl_IDirectMusicScript_Release,
	IDirectMusicScriptImpl_IDirectMusicScript_Init,
	IDirectMusicScriptImpl_IDirectMusicScript_CallRoutine,
	IDirectMusicScriptImpl_IDirectMusicScript_SetVariableVariant,
	IDirectMusicScriptImpl_IDirectMusicScript_GetVariableVariant,
	IDirectMusicScriptImpl_IDirectMusicScript_SetVariableNumber,
	IDirectMusicScriptImpl_IDirectMusicScript_GetVariableNumber,
	IDirectMusicScriptImpl_IDirectMusicScript_SetVariableObject,
	IDirectMusicScriptImpl_IDirectMusicScript_GetVariableObject,
	IDirectMusicScriptImpl_IDirectMusicScript_EnumRoutine,
	IDirectMusicScriptImpl_IDirectMusicScript_EnumVariable
};

/* IDirectMusicScriptImpl IDirectMusicObject part: */
HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	TRACE("(%p, %p)\n", This, pDesc);
	/* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
	memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	TRACE("(%p, %p): setting descriptor:\n", This, pDesc);
	if (TRACE_ON(dmscript)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	/* According to MSDN, we should copy only given values, not whole struct */	
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&This->pDesc->guidObject, &pDesc->guidObject, sizeof (pDesc->guidObject));
	if (pDesc->dwValidData & DMUS_OBJ_CLASS)
		memcpy (&This->pDesc->guidClass, &pDesc->guidClass, sizeof (pDesc->guidClass));		
	if (pDesc->dwValidData & DMUS_OBJ_NAME)
		strncpyW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
	if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
		strncpyW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);		
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		strncpyW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);		
	if (pDesc->dwValidData & DMUS_OBJ_VERSION)
		memcpy (&This->pDesc->vVersion, &pDesc->vVersion, sizeof (pDesc->vVersion));				
	if (pDesc->dwValidData & DMUS_OBJ_DATE)
		memcpy (&This->pDesc->ftDate, &pDesc->ftDate, sizeof (pDesc->ftDate));				
	if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		memcpy (&This->pDesc->llMemLength, &pDesc->llMemLength, sizeof (pDesc->llMemLength));				
		memcpy (This->pDesc->pbMemData, pDesc->pbMemData, sizeof (pDesc->pbMemData));
	}
	if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* according to MSDN, we copy the stream */
		IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
	}
	
	/* add new flags */
	This->pDesc->dwValidData |= pDesc->dwValidData;

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);
	
	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&pDesc->guidClass, &CLSID_DirectMusicScript, sizeof(CLSID));
	
	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			if (Chunk.fccID == DMUS_FOURCC_SCRIPT_FORM) {
				TRACE_(dmfile)(": script form\n");
				do {
					IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
					StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
					TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
					switch (Chunk.fccID) {
						case DMUS_FOURCC_GUID_CHUNK: {
							TRACE_(dmfile)(": GUID chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_OBJECT;
							IStream_Read (pStream, &pDesc->guidObject, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_VERSION_CHUNK: {
							TRACE_(dmfile)(": version chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_VERSION;
							IStream_Read (pStream, &pDesc->vVersion, Chunk.dwSize, NULL);
							break;
						}
						case DMUS_FOURCC_CATEGORY_CHUNK: {
							TRACE_(dmfile)(": category chunk\n");
							pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
							IStream_Read (pStream, pDesc->wszCategory, Chunk.dwSize, NULL);
							break;
						}
						case FOURCC_LIST: {
							IStream_Read (pStream, &Chunk.fccID, sizeof(FOURCC), NULL);				
							TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
							ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
							ListCount[0] = 0;
							switch (Chunk.fccID) {
								/* evil M$ UNFO list, which can (!?) contain INFO elements */
								case DMUS_FOURCC_UNFO_LIST: {
									TRACE_(dmfile)(": UNFO list\n");
									do {
										IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
										ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
										TRACE_(dmfile)(": %s chunk (size = 0x%04lx)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
										switch (Chunk.fccID) {
											/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                             (though strings seem to be valid unicode) */
											case mmioFOURCC('I','N','A','M'):
											case DMUS_FOURCC_UNAM_CHUNK: {
												TRACE_(dmfile)(": name chunk\n");
												pDesc->dwValidData |= DMUS_OBJ_NAME;
												IStream_Read (pStream, pDesc->wszName, Chunk.dwSize, NULL);
												break;
											}
											case mmioFOURCC('I','A','R','T'):
											case DMUS_FOURCC_UART_CHUNK: {
												TRACE_(dmfile)(": artist chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','O','P'):
											case DMUS_FOURCC_UCOP_CHUNK: {
												TRACE_(dmfile)(": copyright chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','S','B','J'):
											case DMUS_FOURCC_USBJ_CHUNK: {
												TRACE_(dmfile)(": subject chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											case mmioFOURCC('I','C','M','T'):
											case DMUS_FOURCC_UCMT_CHUNK: {
												TRACE_(dmfile)(": comment chunk (ignored)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;
											}
											default: {
												TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;						
											}
										}
										TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
									} while (ListCount[0] < ListSize[0]);
									break;
								}
								default: {
									TRACE_(dmfile)(": unknown (skipping)\n");
									liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
									IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
									break;						
								}
							}
							break;
						}	
						default: {
							TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
				break;
			} else {
				TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
				liMove.QuadPart = StreamSize;
				IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
				return E_FAIL;
			}
		
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return DMUS_E_INVALIDFILE;
		}
	}	
	
	TRACE(": returning descriptor:\n");
	if (TRACE_ON(dmscript)) {
		DMUSIC_dump_DMUS_OBJECTDESC (pDesc);
	}
	
	return S_OK;
}

ICOM_VTABLE(IDirectMusicObject) DirectMusicScript_Object_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_IDirectMusicObject_QueryInterface,
	IDirectMusicScriptImpl_IDirectMusicObject_AddRef,
	IDirectMusicScriptImpl_IDirectMusicObject_Release,
	IDirectMusicScriptImpl_IDirectMusicObject_GetDescriptor,
	IDirectMusicScriptImpl_IDirectMusicObject_SetDescriptor,
	IDirectMusicScriptImpl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicScriptImpl IPersistStream part: */
HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_QueryInterface ((LPUNKNOWN)&This->UnknownVtbl, riid, ppobj);
}

ULONG WINAPI IDirectMusicScriptImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_AddRef ((LPUNKNOWN)&This->UnknownVtbl);
}

ULONG WINAPI IDirectMusicScriptImpl_IPersistStream_Release (LPPERSISTSTREAM iface) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
	return IDirectMusicScriptImpl_IUnknown_Release ((LPUNKNOWN)&This->UnknownVtbl);
}

HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);

	FOURCC chunkID;
	DWORD chunkSize, StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */

	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
	IStream_Read (pStm, &chunkSize, sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
	switch (chunkID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(chunkID));
			StreamSize = chunkSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (chunkID) {
				case DMUS_FOURCC_SCRIPT_FORM: {
					TRACE_(dmfile)(": script form\n");
					do {
						IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
						IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
						TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
						switch (chunkID) {
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, chunkSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->pDesc->wszCategory, chunkSize, NULL);
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(chunkID));
								ListSize[0] = chunkSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (chunkID) {
									case DMUS_FOURCC_UNFO_LIST: {
										TRACE_(dmfile)(": UNFO list\n");
										do {
											IStream_Read (pStm, &chunkID, sizeof(FOURCC), NULL);
											IStream_Read (pStm, &chunkSize, sizeof(FOURCC), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + chunkSize;
											TRACE_(dmfile)(": %s chunk (size = %ld)", debugstr_fourcc (chunkID), chunkSize);
											switch (chunkID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->pDesc->wszName, chunkSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
													liMove.QuadPart = chunkSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %ld < ListSize[0] = %ld\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = chunkSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrevelant & skipping)\n");
								liMove.QuadPart = chunkSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount[0] = %ld < StreamSize[0] = %ld\n", StreamCount, StreamSize);
					} while (StreamCount < StreamSize);
					break;
				}
				default: {
					TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
					liMove.QuadPart = StreamSize;
					IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
					return E_FAIL;
				}
			}
			TRACE_(dmfile)(": reading finished\n");
			break;
		}
		default: {
			TRACE_(dmfile)(": unexpected chunk; loading failed)\n");
			liMove.QuadPart = chunkSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
	return E_NOTIMPL;
}

HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
	return E_NOTIMPL;
}

ICOM_VTABLE(IPersistStream) DirectMusicScript_PersistStream_Vtbl = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicScriptImpl_IPersistStream_QueryInterface,
	IDirectMusicScriptImpl_IPersistStream_AddRef,
	IDirectMusicScriptImpl_IPersistStream_Release,
	IDirectMusicScriptImpl_IPersistStream_GetClassID,
	IDirectMusicScriptImpl_IPersistStream_IsDirty,
	IDirectMusicScriptImpl_IPersistStream_Load,
	IDirectMusicScriptImpl_IPersistStream_Save,
	IDirectMusicScriptImpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicScriptImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicScriptImpl* obj;
	
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptImpl));
	if (NULL == obj) {
		*ppobj = (LPVOID) NULL;
		return E_OUTOFMEMORY;
	}
	obj->UnknownVtbl = &DirectMusicScript_Unknown_Vtbl;
	obj->ScriptVtbl = &DirectMusicScript_Script_Vtbl;
	obj->ObjectVtbl = &DirectMusicScript_Object_Vtbl;
	obj->PersistStreamVtbl = &DirectMusicScript_PersistStream_Vtbl;
	obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	DM_STRUCT_INIT(obj->pDesc);
	obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
	memcpy (&obj->pDesc->guidClass, &CLSID_DirectMusicScript, sizeof (CLSID));
	obj->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicScriptImpl_IUnknown_QueryInterface ((LPUNKNOWN)&obj->UnknownVtbl, lpcGUID, ppobj);
}
