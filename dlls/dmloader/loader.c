/* IDirectMusicLoader8 Implementation
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
#include "wine/unicode.h"
#include "winreg.h"

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]);

/* IDirectMusicLoader8 IUnknown part: */
HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicLoader) ||
	    IsEqualIID (riid, &IID_IDirectMusicLoader8)) {
		IDirectMusicLoader8Impl_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

ULONG WINAPI IDirectMusicLoader8Impl_AddRef (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicLoader8Impl_Release (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicLoader8 IDirectMusicLoader part: */
HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID* ppv)
{
	IDirectMusicObject* pObject;
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	HRESULT result;
	struct list *listEntry;
	DMUS_PRIVATE_CACHE_ENTRY *cacheEntry;
	LPDMUS_PRIVATE_CACHE_ENTRY newEntry;

	TRACE("(%p, %p, %s, %p)\n", This, pDesc, debugstr_guid(riid), ppv);

	TRACE("looking up cache...\n");

	LIST_FOR_EACH (listEntry, &This->CacheList) {
		cacheEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_CACHE_ENTRY, entry );
		
		if ((pDesc->dwValidData & DMUS_OBJ_OBJECT) || (pDesc->dwValidData & DMUS_OBJ_FILENAME)) {
			if (pDesc->dwValidData & DMUS_OBJ_OBJECT) {
				if (IsEqualGUID (&cacheEntry->guidObject, &pDesc->guidObject)) {
					TRACE(": found it by GUID\n");
					if (cacheEntry->pObject)
						return IDirectMusicObject_QueryInterface ((LPDIRECTMUSICOBJECT)cacheEntry->pObject, riid, ppv);
				}
			}
			if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
				if (cacheEntry->wzFileName && !strncmpW (pDesc->wszFileName, cacheEntry->wzFileName, MAX_PATH)) {
					TRACE(": found it by FileName\n");
					if (cacheEntry->pObject)
						return IDirectMusicObject_QueryInterface ((LPDIRECTMUSICOBJECT)cacheEntry->pObject, riid, ppv);
				}
			}
		}
	}
	
	/* object doesn't exist in cache... guess we'll have to load it */
	TRACE(": object does not exist in cache\n");
	result = CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);
	if (FAILED(result)) return result;
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* load object from file */
		WCHAR wzFileName[MAX_PATH];
		ILoaderStream* pStream;
                IPersistStream *pPersistStream = NULL;
		/* if it's full path, don't add search directory path, otherwise do */
		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW( wzFileName, pDesc->wszFileName );
		} else {
			WCHAR *p;
			lstrcpyW( wzFileName, This->wzSearchPath );
			p = wzFileName + lstrlenW(wzFileName);
			if (p > wzFileName && p[-1] != '\\') *p++ = '\\';
			strcpyW( p, pDesc->wszFileName );
		}
		TRACE(": loading from file (%s)\n", debugstr_w(wzFileName));
         
		result = DMUSIC_CreateLoaderStream ((LPSTREAM*)&pStream);
		if (FAILED(result)) return result;
		
		result = ILoaderStream_Attach (pStream, wzFileName, (LPDIRECTMUSICLOADER)iface);
		if (FAILED(result)) return result;
			
		result = IDirectMusicObject_QueryInterface (pObject, &IID_IPersistStream, (LPVOID*)&pPersistStream);
                if (FAILED(result)) return result;
			
		result = IPersistStream_Load (pPersistStream, (LPSTREAM)pStream);
                if (FAILED(result)) return result;
			
                ILoaderStream_IStream_Release ((LPSTREAM)pStream);
                IPersistStream_Release (pPersistStream);
	} else if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* load object from stream */
		IStream* pClonedStream = NULL;
		IPersistStream* pPersistStream = NULL;

		TRACE(": loading from stream\n");
		result = IDirectMusicObject_QueryInterface (pObject, &IID_IPersistStream, (LPVOID*)&pPersistStream);
                if (FAILED(result)) return result;
			
                result = IStream_Clone (pDesc->pStream, &pClonedStream);
                if (FAILED(result)) return result;

                result = IPersistStream_Load (pPersistStream, pClonedStream);
                if (FAILED(result))	return result;

		IPersistStream_Release (pPersistStream);
		IStream_Release (pClonedStream);
	} else if (pDesc->dwValidData & DMUS_OBJ_OBJECT) {
		/* load object by GUID */
		TRACE(": loading by GUID (only default DLS supported)\n");
		if (IsEqualGUID (&pDesc->guidObject, &GUID_DefaultGMCollection)) {
			WCHAR wzFileName[MAX_PATH];
                        IPersistStream *pPersistStream = NULL;
			ILoaderStream* pStream;
			if (FAILED(DMUSIC_GetDefaultGMPath (wzFileName)))
				return E_FAIL;
			/* load object from file */
			result = DMUSIC_CreateLoaderStream ((LPSTREAM*)&pStream);
			if (FAILED(result)) return result;

			result = ILoaderStream_Attach (pStream, wzFileName, (LPDIRECTMUSICLOADER)iface);
			if (FAILED(result)) return result;

			result = IDirectMusicObject_QueryInterface (pObject, &IID_IPersistStream, (LPVOID*)&pPersistStream);
                        if (FAILED(result)) return result;

			result = IPersistStream_Load (pPersistStream, (LPSTREAM)pStream);
                        if (FAILED(result)) return result;

                        ILoaderStream_IStream_Release ((LPSTREAM)pStream);
			IPersistStream_Release (pPersistStream);
		} else {
			return E_FAIL;
		}
	} else {
		/* nowhere to load from */
		FIXME(": unknown/unsupported way of loading\n");
		return E_FAIL;
	}
	/* add object to cache */
	newEntry = (LPDMUS_PRIVATE_CACHE_ENTRY) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_CACHE_ENTRY));
	if (pDesc->dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&newEntry->guidObject, &pDesc->guidObject, sizeof (pDesc->guidObject));
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
		strncpyW (newEntry->wzFileName, pDesc->wszFileName, MAX_PATH);
	if (pObject)
		newEntry->pObject = pObject;
	list_add_tail (&This->CacheList, &newEntry->entry);
	TRACE(": filled in cache entry\n");
	
	/* for debug purposes (e.g. to check if all files are cached) */
#if 0
	int i = 0;
	LIST_FOR_EACH (listEntry, &This->CacheList) {
		i++;
		cacheEntry = LIST_ENTRY( listEntry, DMUS_PRIVATE_CACHE_ENTRY, entry );
		TRACE("Entry nr. %i: GUID = %s, FileName = %s\n", i, debugstr_guid (&cacheEntry->guidObject), debugstr_w (cacheEntry->wzFileName));
	}
#endif
	
	return IDirectMusicObject_QueryInterface (pObject, riid, ppv);
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pDesc);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	TRACE("(%p, %s, %s, %d)\n", This, debugstr_guid(rguidClass), debugstr_w(pwzPath), fClear);
	if (0 == strncmpW(This->wzSearchPath, pwzPath, MAX_PATH)) {
	  return S_FALSE;
	} 
	strncpyW(This->wzSearchPath, pwzPath, MAX_PATH);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_guid(rguidClass), pwzFileExtension, pwzScanFileName);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidClass));

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %d): stub\n", This, debugstr_guid(rguidClass), fEnable);

	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidClass), dwIndex, pDesc);

	return S_OK;
}

/* IDirectMusicLoader8 Interface part follow: */
void WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p): stub\n", This);
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);

	FIXME("(%p, %p): stub\n", This, pObject);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, 
							   REFGUID rguidClassID, 
							   REFIID iidInterfaceID, 
							   WCHAR* pwzFilePath, 
							   void** ppObject)
{
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	DMUS_OBJECTDESC ObjDesc;
	
	TRACE("(%p, %s, %s, %s, %p): wrapping to IDirectMusicLoader8Impl_GetObject\n", This, debugstr_guid(rguidClassID), debugstr_guid(iidInterfaceID), debugstr_w(pwzFilePath), ppObject);
	
	ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);
	ObjDesc.dwValidData = DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_CLASS; /* I believe I've read somewhere in MSDN that this function requires either full path or relative path */
	ObjDesc.guidClass = *rguidClassID;
	/* OK, MSDN says that search order is the following:
	    - current directory (DONE)
	    - windows search path (FIXME: how do I get that?)
	    - loader's search path (DONE)
	*/
        /* search in current directory */
        if (!SearchPathW (NULL, pwzFilePath, NULL,
                          sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL) &&
            /* search in loader's search path */
            !SearchPathW (This->wzSearchPath, pwzFilePath, NULL,
                          sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL))
        {
		/* cannot find file */
		TRACE("cannot find file\n");
		return DMUS_E_LOADER_FAILEDOPEN;
	}
	
	TRACE("full file path = %s\n", debugstr_w (ObjDesc.wszFileName));
	
	return IDirectMusicLoader8Impl_GetObject (iface, &ObjDesc, iidInterfaceID, ppObject);
}

ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectMusicLoader8Impl_QueryInterface,
	IDirectMusicLoader8Impl_AddRef,
	IDirectMusicLoader8Impl_Release,
	IDirectMusicLoader8Impl_GetObject,
	IDirectMusicLoader8Impl_SetObject,
	IDirectMusicLoader8Impl_SetSearchDirectory,
	IDirectMusicLoader8Impl_ScanDirectory,
	IDirectMusicLoader8Impl_CacheObject,
	IDirectMusicLoader8Impl_ReleaseObject,
	IDirectMusicLoader8Impl_ClearCache,
	IDirectMusicLoader8Impl_EnableCache,
	IDirectMusicLoader8Impl_EnumObject,
	IDirectMusicLoader8Impl_CollectGarbage,
	IDirectMusicLoader8Impl_ReleaseObjectByUnknown,
	IDirectMusicLoader8Impl_LoadObjectFromFile
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicLoader (LPCGUID lpcGUID, LPDIRECTMUSICLOADER8 *ppDMLoad, LPUNKNOWN pUnkOuter)
{
	IDirectMusicLoader8Impl *dmloader;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppDMLoad, pUnkOuter);
	if (IsEqualIID (lpcGUID, &IID_IDirectMusicLoader) || 
	    IsEqualIID (lpcGUID, &IID_IDirectMusicLoader8)) {
		dmloader = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoader8Impl));
		if (NULL == dmloader) {
			*ppDMLoad = (LPDIRECTMUSICLOADER8)NULL;
			return E_OUTOFMEMORY;
		}
		dmloader->lpVtbl = &DirectMusicLoader8_Vtbl;
		dmloader->ref = 1;
		MultiByteToWideChar (CP_ACP, 0, ".\\", -1, dmloader->wzSearchPath, MAX_PATH);
		list_init (&dmloader->CacheList);
		*ppDMLoad = (LPDIRECTMUSICLOADER8)dmloader;
		return S_OK;
	}
	
	WARN("No interface found\n");
	return E_NOINTERFACE;
}

/* help function for IDirectMusicLoader8Impl_GetObject */
HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH])
{
	HKEY hkDM;
	DWORD returnType, sizeOfReturnBuffer = MAX_PATH;
	char szPath[MAX_PATH];

	if ((RegOpenKeyExA (HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic" , 0, KEY_READ, &hkDM) != ERROR_SUCCESS) || 
	    (RegQueryValueExA (hkDM, "GMFilePath", NULL, &returnType, szPath, &sizeOfReturnBuffer) != ERROR_SUCCESS)) {
		WARN(": registry entry missing\n" );
		return E_FAIL;
	}
	/* FIXME: Check return types to ensure we're interpreting data right */
	MultiByteToWideChar (CP_ACP, 0, szPath, -1, wszPath, MAX_PATH);
	
	return S_OK;
}
