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
	DMUS_OBJECTDESC desc;
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	int i;
	HRESULT result;

	TRACE("(%p, %p, %s, %p)\n", This, pDesc, debugstr_guid(riid), ppv);
	TRACE(": looking up cache");
	/* first, check if requested object is already in cache (check by name and GUID) */
	for (i = 0; i < This->dwCacheSize; i++) {
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) {
			if (IsEqualGUID (&pDesc->guidObject, &This->pCache[i].guidObject)) {
				TRACE(": object already exist in cache (found by GUID)\n");
				if (This->pCache[i].pObject) {
					return IDirectMusicObject_QueryInterface (This->pCache[i].pObject, riid, ppv);
				}
			}
		} else if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
			if (This->pCache[i].pwzFileName && !strncmpW(pDesc->wszFileName, This->pCache[i].pwzFileName, MAX_PATH)) {
				TRACE(": object already exist in cache (found by file name)\n");
				if (This->pCache[i].pObject) {
					return IDirectMusicObject_QueryInterface (This->pCache[i].pObject, riid, ppv);
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
                    lstrcpyW( p, pDesc->wszFileName );
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
        if (FAILED(result)) {
			TRACE("couln\'t get IPersistStream\n");
			return result;
		}
			
        result = IStream_Clone (pDesc->pStream, &pClonedStream);
        if (FAILED(result)) {
			TRACE("failed to clone\n");
			return result;
		}

        result = IPersistStream_Load (pPersistStream, pClonedStream);
        if (FAILED(result)) {
			TRACE("failed to load\n");
			return result;
		}

		IPersistStream_Release (pPersistStream);
		IStream_Release (pClonedStream);
	} else if (pDesc->dwValidData & DMUS_OBJ_OBJECT) {
		/* load object by GUID */
		TRACE(": loading by GUID (only default DLS supported)\n");
		if (IsEqualGUID (&pDesc->guidObject, &GUID_DefaultGMCollection)) {
			/* great idea: let's just change dwValid and wszFileName fields and then call ourselves again :D */
			pDesc->dwValidData = DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
			if (FAILED(DMUSIC_GetDefaultGMPath (pDesc->wszFileName)))
				return E_FAIL;
			return IDirectMusicLoader8Impl_GetObject (iface, pDesc, riid, ppv);
		} else {
			return E_FAIL;
		}
	} else {
		/* nowhere to load from */
		FIXME(": unknown/unsupported way of loading\n");
		return E_FAIL;
	}

	memset((LPVOID)&desc, 0, sizeof(desc));
	desc.dwSize = sizeof (DMUS_OBJECTDESC); 
	IDirectMusicObject_GetDescriptor (pObject, &desc);
	
	/* tests with native dlls show that descriptor, which is received by GetDescriptor doesn't contain filepath
	   therefore we must copy it from input description	*/	
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME || desc.dwValidData & DMUS_OBJ_OBJECT) {
		DMUS_PRIVATE_CACHE_ENTRY CacheEntry;
		This->dwCacheSize++; /* increase size of cache for one entry */
		This->pCache = HeapReAlloc (GetProcessHeap (), 0, This->pCache, sizeof(DMUS_PRIVATE_CACHE_ENTRY) * This->dwCacheSize);
		if (desc.dwValidData & DMUS_OBJ_OBJECT)
			CacheEntry.guidObject = desc.guidObject;
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME)
			strncpyW (CacheEntry.pwzFileName, pDesc->wszFileName, MAX_PATH);
		CacheEntry.pObject = pObject;
		IDirectMusicObject_AddRef (pObject); /* MSDN says that we should */
		This->pCache[This->dwCacheSize - 1] = CacheEntry; /* fill in one backward, as list is zero based */
		TRACE(": filled in cache entry\n");
	}
	
	TRACE(": nr. of entries = %ld\n", This->dwCacheSize);	
	for (i = 0; i < This->dwCacheSize; i++) {
		TRACE(": cache entry [%i]: GUID = %s, file name = %s, object = %p\n", i, debugstr_guid(&This->pCache[i].guidObject), debugstr_w(This->pCache[i].pwzFileName), This->pCache[i].pObject);
	}

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

	TRACE("(%p, %s, %p, %d)\n", This, debugstr_guid(rguidClass), pwzPath, fClear);
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
	strncpyW (ObjDesc.wszFileName, pwzFilePath, MAX_PATH);

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
