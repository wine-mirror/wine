/* IDirectMusicScript
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "dmscript_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);


/*****************************************************************************
 * IDirectMusicScriptImpl implementation
 */
typedef struct IDirectMusicScriptImpl {
    IDirectMusicScript IDirectMusicScript_iface;
    const IDirectMusicObjectVtbl *ObjectVtbl;
    const IPersistStreamVtbl *PersistStreamVtbl;
    LONG ref;
    IDirectMusicPerformance *pPerformance;
    DMUS_OBJECTDESC *pDesc;
    DMUS_IO_SCRIPT_HEADER *pHeader;
    DMUS_IO_VERSION *pVersion;
    WCHAR *pwzLanguage;
    WCHAR *pwzSource;
} IDirectMusicScriptImpl;

static inline IDirectMusicScriptImpl *impl_from_IDirectMusicScript(IDirectMusicScript *iface)
{
  return CONTAINING_RECORD(iface, IDirectMusicScriptImpl, IDirectMusicScript_iface);
}

static HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface(IDirectMusicScript *iface, REFIID riid,
        void **ret_iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicScript))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->ObjectVtbl;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->PersistStreamVtbl;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicScriptImpl_AddRef(IDirectMusicScript *iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicScriptImpl_Release(IDirectMusicScript *iface)
{
    IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This->pHeader);
        HeapFree(GetProcessHeap(), 0, This->pVersion);
        HeapFree(GetProcessHeap(), 0, This->pwzLanguage);
        HeapFree(GetProcessHeap(), 0, This->pwzSource);
        HeapFree(GetProcessHeap(), 0, This->pDesc);
        HeapFree(GetProcessHeap(), 0, This);
        DMSCRIPT_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicScriptImpl_Init(IDirectMusicScript *iface,
        IDirectMusicPerformance *pPerformance, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %p, %p): stub\n", This, pPerformance, pErrorInfo);
  This->pPerformance = pPerformance;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine(IDirectMusicScript *iface,
        WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszRoutineName), pErrorInfo);
  /*return E_NOTIMPL;*/
  return S_OK;
  /*return E_FAIL;*/
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, FIXME, %d, %p): stub\n", This, debugstr_w(pwszVariableName),/* varValue,*/ fSetRef, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), pvarValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %i, %p): stub\n", This, debugstr_w(pwszVariableName), lValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG *plValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), plValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, IUnknown *punkValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), punkValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, REFIID riid, void **ppv, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), debugstr_dmguid(riid), ppv, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %d, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  IDirectMusicScriptImpl *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %d, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static const IDirectMusicScriptVtbl dmscript_vtbl = {
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

/* IDirectMusicScriptImpl IDirectMusicObject part: */
static HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  return IDirectMusicScript_QueryInterface(&This->IDirectMusicScript_iface, riid, ppobj);
}

static ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  return IDirectMusicScript_AddRef (&This->IDirectMusicScript_iface);
}

static ULONG WINAPI IDirectMusicScriptImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  return IDirectMusicScript_Release (&This->IDirectMusicScript_iface);
}

static HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  TRACE("(%p, %p)\n", This, pDesc);
  /* I think we shouldn't return pointer here since then values can be changed; it'd be a mess */
  memcpy (pDesc, This->pDesc, This->pDesc->dwSize);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  TRACE("(%p, %p): setting descriptor:\n%s\n", This, pDesc, debugstr_DMUS_OBJECTDESC (pDesc));

  /* According to MSDN, we should copy only given values, not whole struct */	
  if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
    This->pDesc->guidObject = pDesc->guidObject;
  if (pDesc->dwValidData & DMUS_OBJ_CLASS)
    This->pDesc->guidClass = pDesc->guidClass;
  if (pDesc->dwValidData & DMUS_OBJ_NAME)
    lstrcpynW (This->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME);
  if (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
    lstrcpynW (This->pDesc->wszCategory, pDesc->wszCategory, DMUS_MAX_CATEGORY);
  if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
    lstrcpynW (This->pDesc->wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME);
  if (pDesc->dwValidData & DMUS_OBJ_VERSION)
    This->pDesc->vVersion = pDesc->vVersion;
  if (pDesc->dwValidData & DMUS_OBJ_DATE)
    This->pDesc->ftDate = pDesc->ftDate;
  if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
    This->pDesc->llMemLength = pDesc->llMemLength;
    memcpy (This->pDesc->pbMemData, pDesc->pbMemData, pDesc->llMemLength);
  }
  if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
    /* according to MSDN, we copy the stream */
    IStream_Clone (pDesc->pStream, &This->pDesc->pStream);	
  }
  
  /* add new flags */
  This->pDesc->dwValidData |= pDesc->dwValidData;
  
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, ObjectVtbl, iface);
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD StreamSize, StreamCount, ListSize[1], ListCount[1];
  LARGE_INTEGER liMove; /* used when skipping chunks */

	TRACE("(%p, %p, %p)\n", This, pStream, pDesc);

	/* FIXME: should this be determined from stream? */
	pDesc->dwValidData |= DMUS_OBJ_CLASS;
	pDesc->guidClass = CLSID_DirectMusicScript;

	IStream_Read (pStream, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
					TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
										TRACE_(dmfile)(": %s chunk (size = 0x%04x)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
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
												TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
												liMove.QuadPart = Chunk.dwSize;
												IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
												break;						
											}
										}
										TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
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
							TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
							liMove.QuadPart = Chunk.dwSize;
							IStream_Seek (pStream, liMove, STREAM_SEEK_CUR, NULL);
							break;						
						}
					}
					TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
				} while (StreamCount < StreamSize);
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
	
	TRACE(": returning descriptor:\n%s\n", debugstr_DMUS_OBJECTDESC (pDesc));
	
	return S_OK;
}

static const IDirectMusicObjectVtbl DirectMusicScript_Object_Vtbl = {
  IDirectMusicScriptImpl_IDirectMusicObject_QueryInterface,
  IDirectMusicScriptImpl_IDirectMusicObject_AddRef,
  IDirectMusicScriptImpl_IDirectMusicObject_Release,
  IDirectMusicScriptImpl_IDirectMusicObject_GetDescriptor,
  IDirectMusicScriptImpl_IDirectMusicObject_SetDescriptor,
  IDirectMusicScriptImpl_IDirectMusicObject_ParseDescriptor
};

/* IDirectMusicScriptImpl IPersistStream part: */
static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, LPVOID *ppobj) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
  return IDirectMusicScript_QueryInterface(&This->IDirectMusicScript_iface, riid, ppobj);
}

static ULONG WINAPI IDirectMusicScriptImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
  return IDirectMusicScript_AddRef(&This->IDirectMusicScript_iface);
}

static ULONG WINAPI IDirectMusicScriptImpl_IPersistStream_Release (LPPERSISTSTREAM iface) {
  ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);
  return IDirectMusicScript_Release(&This->IDirectMusicScript_iface);
}

static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID) {
  TRACE("(%p, %p) method not implemented\n", iface, pClassID);

  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface) {
  return S_FALSE;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm) {
	ICOM_THIS_MULTI(IDirectMusicScriptImpl, PersistStreamVtbl, iface);

	DMUS_PRIVATE_CHUNK Chunk;
	DWORD StreamSize, StreamCount, ListSize[3], ListCount[3];
	LARGE_INTEGER liMove; /* used when skipping chunks */
	LPDIRECTMUSICGETLOADER pGetLoader = NULL;
	LPDIRECTMUSICLOADER pLoader = NULL;

	FIXME("(%p, %p): Loading not implemented yet\n", This, pStm);
	IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
	TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
	switch (Chunk.fccID) {	
		case FOURCC_RIFF: {
			IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
			TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
			StreamSize = Chunk.dwSize - sizeof(FOURCC);
			StreamCount = 0;
			switch (Chunk.fccID) {
				case DMUS_FOURCC_SCRIPT_FORM: {
					TRACE_(dmfile)(": script form\n");
					do {
						IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
						StreamCount += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
						TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
						switch (Chunk.fccID) { 
						        case DMUS_FOURCC_SCRIPT_CHUNK: {
							        TRACE_(dmfile)(": script header chunk\n");
								This->pHeader = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pHeader, Chunk.dwSize, NULL);
								break;
						        }
						        case DMUS_FOURCC_SCRIPTVERSION_CHUNK: {
							        TRACE_(dmfile)(": script version chunk\n");
								This->pVersion = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pVersion, Chunk.dwSize, NULL); 
								TRACE_(dmfile)("version: 0x%08x.0x%08x\n", This->pVersion->dwVersionMS, This->pVersion->dwVersionLS);
								break;
						        }
						        case DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK: {
							        TRACE_(dmfile)(": script language chunk\n");
								This->pwzLanguage = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pwzLanguage, Chunk.dwSize, NULL); 
								TRACE_(dmfile)("using language: %s\n", debugstr_w(This->pwzLanguage));
								break;
						        }
						        case DMUS_FOURCC_SCRIPTSOURCE_CHUNK: {
							        TRACE_(dmfile)(": script source chunk\n");
								This->pwzSource = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, Chunk.dwSize);
								IStream_Read (pStm, This->pwzSource, Chunk.dwSize, NULL); 
								if (TRACE_ON(dmscript)) {
								    int count = WideCharToMultiByte(CP_ACP, 0, This->pwzSource, -1, NULL, 0, NULL, NULL);
								    LPSTR str = HeapAlloc(GetProcessHeap (), 0, count);
								    WideCharToMultiByte(CP_ACP, 0, This->pwzSource, -1, str, count, NULL, NULL);
								    str[count-1] = '\n';
								    TRACE("source:\n");
								    write( 2, str, count );
								    HeapFree(GetProcessHeap(), 0, str);
								}
								break;
						        }
							case DMUS_FOURCC_GUID_CHUNK: {
								TRACE_(dmfile)(": GUID chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_OBJECT;
								IStream_Read (pStm, &This->pDesc->guidObject, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_VERSION_CHUNK: {
								TRACE_(dmfile)(": version chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_VERSION;
								IStream_Read (pStm, &This->pDesc->vVersion, Chunk.dwSize, NULL);
								break;
							}
							case DMUS_FOURCC_CATEGORY_CHUNK: {
								TRACE_(dmfile)(": category chunk\n");
								This->pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
								IStream_Read (pStm, This->pDesc->wszCategory, Chunk.dwSize, NULL);
								break;
							}
						        case FOURCC_RIFF: {
								IDirectMusicObject* pObject = NULL;
								DMUS_OBJECTDESC desc;

								ZeroMemory (&desc, sizeof(DMUS_OBJECTDESC));
								desc.dwSize = sizeof(DMUS_OBJECTDESC);
								desc.dwValidData = DMUS_OBJ_STREAM | DMUS_OBJ_CLASS;
								desc.guidClass = CLSID_DirectMusicContainer;
								desc.pStream = NULL;
								IStream_Clone (pStm, &desc.pStream);

								liMove.QuadPart = 0;
								liMove.QuadPart -= (sizeof(FOURCC) + sizeof(DWORD));
								IStream_Seek (desc.pStream, liMove, STREAM_SEEK_CUR, NULL);

								IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader);
								IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
								IDirectMusicGetLoader_Release (pGetLoader);

								if (SUCCEEDED(IDirectMusicLoader_GetObject (pLoader, &desc, &IID_IDirectMusicObject, (LPVOID*) &pObject))) {
								  IDirectMusicObject_Release (pObject);
								} else {
								  ERR_(dmfile)("Error on GetObject while trying to load Scrip SubContainer\n");
								}

								IDirectMusicLoader_Release (pLoader); pLoader = NULL; /* release loader */
								IStream_Release(desc.pStream); desc.pStream = NULL; /* release cloned stream */

								liMove.QuadPart = Chunk.dwSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								/*
							        IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": RIFF chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[0] = 0;

								switch (Chunk.fccID) {
								        default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								*/
								break;
							}
							case FOURCC_LIST: {
								IStream_Read (pStm, &Chunk.fccID, sizeof(FOURCC), NULL);				
								TRACE_(dmfile)(": LIST chunk of type %s", debugstr_fourcc(Chunk.fccID));
								ListSize[0] = Chunk.dwSize - sizeof(FOURCC);
								ListCount[0] = 0;
								switch (Chunk.fccID) {
									case DMUS_FOURCC_UNFO_LIST: {
										TRACE_(dmfile)(": UNFO list\n");
										do {
											IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
											ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
											TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);
											switch (Chunk.fccID) {
												/* don't ask me why, but M$ puts INFO elements in UNFO list sometimes
                                              (though strings seem to be valid unicode) */
												case mmioFOURCC('I','N','A','M'):
												case DMUS_FOURCC_UNAM_CHUNK: {
													TRACE_(dmfile)(": name chunk\n");
													This->pDesc->dwValidData |= DMUS_OBJ_NAME;
													IStream_Read (pStm, This->pDesc->wszName, Chunk.dwSize, NULL);
													break;
												}
												case mmioFOURCC('I','A','R','T'):
												case DMUS_FOURCC_UART_CHUNK: {
													TRACE_(dmfile)(": artist chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','O','P'):
												case DMUS_FOURCC_UCOP_CHUNK: {
													TRACE_(dmfile)(": copyright chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','S','B','J'):
												case DMUS_FOURCC_USBJ_CHUNK: {
													TRACE_(dmfile)(": subject chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												case mmioFOURCC('I','C','M','T'):
												case DMUS_FOURCC_UCMT_CHUNK: {
													TRACE_(dmfile)(": comment chunk (ignored)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;
												}
												default: {
													TRACE_(dmfile)(": unknown sub-chunk (irrelevant & skipping)\n");
													liMove.QuadPart = Chunk.dwSize;
													IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
													break;						
												}
											}
											TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
										} while (ListCount[0] < ListSize[0]);
										break;
									}
									default: {
										TRACE_(dmfile)(": unknown (skipping)\n");
										liMove.QuadPart = Chunk.dwSize - sizeof(FOURCC);
										IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
										break;						
									}
								}
								break;
							}	
							default: {
								TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
								liMove.QuadPart = Chunk.dwSize;
								IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
								break;						
							}
						}
						TRACE_(dmfile)(": StreamCount[0] = %d < StreamSize[0] = %d\n", StreamCount, StreamSize);
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
			liMove.QuadPart = Chunk.dwSize;
			IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL); /* skip the rest of the chunk */
			return E_FAIL;
		}
	}

	return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty) {
  return E_NOTIMPL;
}

static HRESULT WINAPI IDirectMusicScriptImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize) {
  return E_NOTIMPL;
}

static const IPersistStreamVtbl DirectMusicScript_PersistStream_Vtbl = {
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
HRESULT WINAPI DMUSIC_CreateDirectMusicScriptImpl(REFIID lpcGUID, void **ppobj, IUnknown *pUnkOuter)
{
  IDirectMusicScriptImpl *obj;
  HRESULT hr;

  *ppobj = NULL;

  if (pUnkOuter)
    return CLASS_E_NOAGGREGATION;

  obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicScriptImpl));
  if (!obj)
    return E_OUTOFMEMORY;

  obj->IDirectMusicScript_iface.lpVtbl = &dmscript_vtbl;
  obj->ObjectVtbl = &DirectMusicScript_Object_Vtbl;
  obj->PersistStreamVtbl = &DirectMusicScript_PersistStream_Vtbl;
  obj->pDesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
  DM_STRUCT_INIT(obj->pDesc);
  obj->pDesc->dwValidData |= DMUS_OBJ_CLASS;
  obj->pDesc->guidClass = CLSID_DirectMusicScript;
  obj->ref = 1;

  DMSCRIPT_LockModule();
  hr = IDirectMusicScript_QueryInterface(&obj->IDirectMusicScript_iface, lpcGUID, ppobj);
  IDirectMusicScript_Release(&obj->IDirectMusicScript_iface);

  return hr;
}
