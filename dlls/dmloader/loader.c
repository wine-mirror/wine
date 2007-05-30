/* IDirectMusicLoaderImpl
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

static HRESULT WINAPI DMUSIC_SetLoaderSettings (LPDIRECTMUSICLOADER8 iface, REFGUID pClassID, WCHAR* wszSearchPath, LPBOOL pbCache);

/*****************************************************************************
 * IDirectMusicLoaderImpl implementation
 */
/* IUnknown/IDirectMusicLoader(8) part: */
static ULONG WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_AddRef (LPDIRECTMUSICLOADER8 iface);

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);

	TRACE("(%p, %s, %p)\n",This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicLoader) ||
	    IsEqualIID (riid, &IID_IDirectMusicLoader8)) {
		IDirectMusicLoaderImpl_IDirectMusicLoader_AddRef (iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN(": not found\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_AddRef (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p): AddRef from %d\n", This, This->dwRef);
	return InterlockedIncrement (&This->dwRef);
}

static ULONG WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_Release (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	
	DWORD dwRef = InterlockedDecrement (&This->dwRef);
	TRACE("(%p): ReleaseRef to %d\n", This, This->dwRef);
	if (dwRef == 0) {
		DMUSIC_DestroyDirectMusicLoaderImpl (iface);
		HeapFree (GetProcessHeap(), 0, This);
	}
	
	return dwRef;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID* ppv) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	HRESULT result = S_OK;
	HRESULT ret = S_OK; /* used at the end of function, to determine whether everything went OK */
	
	struct list *pEntry;
	LPWINE_LOADER_ENTRY pObjectEntry = NULL;
	LPSTREAM pStream;
	IPersistStream* pPersistStream = NULL;

	LPDIRECTMUSICOBJECT pObject;
	DMUS_OBJECTDESC GotDesc;
	BOOL bCache;

	TRACE("(%p, %p, %s, %p): pDesc:\n%s\n", This, pDesc, debugstr_dmguid(riid), ppv, debugstr_DMUS_OBJECTDESC(pDesc));
	
	/* sometimes it happens that guidClass is missing... which is a BadThingTM */
	if (!(pDesc->dwValidData & DMUS_OBJ_CLASS)) {
	  ERR(": guidClass not valid but needed\n");
	  *ppv = NULL;
	  return DMUS_E_LOADER_NOCLASSID;
	}
	
	/* OK, first we iterate thru the list of objects we know about; these are either loaded (GetObject, LoadObjectFromFile)
	   or set via SetObject; */
	TRACE(": looking if we have object in the cache or if it can be found via alias\n");
	LIST_FOR_EACH(pEntry, This->pObjects) {
		LPWINE_LOADER_ENTRY pExistingEntry = LIST_ENTRY(pEntry, WINE_LOADER_ENTRY, entry);
		if ((pDesc->dwValidData & DMUS_OBJ_OBJECT) &&
			(pExistingEntry->Desc.dwValidData & DMUS_OBJ_OBJECT) &&
			IsEqualGUID (&pDesc->guidObject, &pExistingEntry->Desc.guidObject)) {
			TRACE(": found it by object GUID\n");
			/* I suppose such stuff can happen only when GUID for object is given (GUID_DefaultGMCollection) */
			if (pExistingEntry->bInvalidDefaultDLS) {
				TRACE(": found faulty default DLS collection... enabling M$ compliant behaviour\n");
				return DMUS_E_LOADER_NOFILENAME;	
			}
			if (pExistingEntry->Desc.dwValidData & DMUS_OBJ_LOADED) {
				TRACE(": already loaded\n");
				return IDirectMusicObject_QueryInterface (pExistingEntry->pObject, riid, ppv);
			} else {
				TRACE(": not loaded yet\n");
				pObjectEntry = pExistingEntry;
			}
		}
		else if ((pDesc->dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH)) &&
				(pExistingEntry->Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH)) &&
				!strncmpW (pDesc->wszFileName, pExistingEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by fullpath filename\n");
			if (pExistingEntry->Desc.dwValidData & DMUS_OBJ_LOADED) {
				TRACE(": already loaded\n");
				return IDirectMusicObject_QueryInterface (pExistingEntry->pObject, riid, ppv);
			} else {
				TRACE(": not loaded yet\n");
				pObjectEntry = pExistingEntry;
			}
		}
		else if ((pDesc->dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY)) &&
				(pExistingEntry->Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY)) &&
				!strncmpW (pDesc->wszName, pExistingEntry->Desc.wszName, DMUS_MAX_NAME) &&
				!strncmpW (pDesc->wszCategory, pExistingEntry->Desc.wszCategory, DMUS_MAX_CATEGORY)) {
			TRACE(": found it by name and category\n");
			if (pExistingEntry->Desc.dwValidData & DMUS_OBJ_LOADED) {
				TRACE(": already loaded\n");
				return IDirectMusicObject_QueryInterface (pExistingEntry->pObject, riid, ppv);
			} else {
				TRACE(": not loaded yet\n");
				pObjectEntry = pExistingEntry;
			}
		}
		else if ((pDesc->dwValidData & DMUS_OBJ_NAME) &&
				(pExistingEntry->Desc.dwValidData & DMUS_OBJ_NAME) &&
				!strncmpW (pDesc->wszName, pExistingEntry->Desc.wszName, DMUS_MAX_NAME)) {
			TRACE(": found it by name\n");
			if (pExistingEntry->Desc.dwValidData & DMUS_OBJ_LOADED) {
				TRACE(": already loaded\n");
				return IDirectMusicObject_QueryInterface (pExistingEntry->pObject, riid, ppv);
			} else {
				TRACE(": not loaded yet\n");
				pObjectEntry = pExistingEntry;
			}
		}
		else if ((pDesc->dwValidData & DMUS_OBJ_FILENAME) &&
				(pExistingEntry->Desc.dwValidData & DMUS_OBJ_FILENAME) &&
				!strncmpW (pDesc->wszFileName, pExistingEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by filename\n");				
			if (pExistingEntry->Desc.dwValidData & DMUS_OBJ_LOADED) {
				TRACE(": already loaded\n");
				return IDirectMusicObject_QueryInterface (pExistingEntry->pObject, riid, ppv);
			} else {
				TRACE(": not loaded yet\n");
				pObjectEntry = pExistingEntry;
			}
		}
	}
	
	/* basically, if we found alias, we use its descriptor to load...
	   else we use info we were given */
	if (pObjectEntry) {
		TRACE(": found alias entry for requested object... using stored info\n");
		/* I think in certain cases it can happen that entry's descriptor lacks info
		   where to load from (e.g.: if we loaded from stream and then released object
		   from cache; then only it's CLSID, GUID and perhaps name are left); so just
		   overwrite info entry has (since it ought to be 100% correct) */
		DMUSIC_CopyDescriptor (pDesc, &pObjectEntry->Desc);
		/*pDesc = &pObjectEntry->Desc; */ /* FIXME: is this OK? */
	} else {
		TRACE(": no cache/alias entry found for requested object\n");
	}
	
	if (pDesc->dwValidData & DMUS_OBJ_URL) {
		TRACE(": loading from URLs not supported yet\n");
		return DMUS_E_LOADER_FORMATNOTSUPPORTED;
	}
	else if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* load object from file */
		/* generate filename; if it's full path, don't add search 
		   directory path, otherwise do */
		WCHAR wszFileName[MAX_PATH];

		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW(wszFileName, pDesc->wszFileName);
		} else {
			WCHAR *p, wszSearchPath[MAX_PATH];
			DMUSIC_GetLoaderSettings (iface, &pDesc->guidClass, wszSearchPath, NULL);
			lstrcpyW(wszFileName, wszSearchPath);
			p = wszFileName + lstrlenW(wszFileName);
			if (p > wszFileName && p[-1] != '\\') *p++ = '\\';
			strcpyW(p, pDesc->wszFileName);
		}
		TRACE(": loading from file (%s)\n", debugstr_w(wszFileName));
		/* create stream and associate it with file */			
		result = DMUSIC_CreateDirectMusicLoaderFileStream ((LPVOID*)&pStream);
		if (FAILED(result)) {
			ERR(": could not create file stream\n");
			return result;
		}
		result = IDirectMusicLoaderFileStream_Attach (pStream, wszFileName, iface);
		if (FAILED(result)) {
			ERR(": could not attach stream to file\n");
			IStream_Release (pStream);
			return result;
		}
	}
	else if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		/* load object from resource */
		TRACE(": loading from resource\n");
		/* create stream and associate it with given resource */			
		result = DMUSIC_CreateDirectMusicLoaderResourceStream ((LPVOID*)&pStream);
		if (FAILED(result)) {
			ERR(": could not create resource stream\n");
			return result;
		}
		result = IDirectMusicLoaderResourceStream_Attach (pStream, pDesc->pbMemData, pDesc->llMemLength, 0, iface);
		if (FAILED(result)) {
			ERR(": could not attach stream to resource\n");
			IStream_Release (pStream);
			return result;
		}
	}
	else if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* load object from stream */
		TRACE(": loading from stream\n");
		/* create universal stream and associate it with given one */			
		result = DMUSIC_CreateDirectMusicLoaderGenericStream ((LPVOID*)&pStream);
		if (FAILED(result)) {
			ERR(": could not create generic stream\n");
			return result;
		}
		result = IDirectMusicLoaderGenericStream_Attach (pStream, pDesc->pStream, iface);
		if (FAILED(result)) {
			ERR(": failed to attach stream\n");
			IStream_Release (pStream);
			return result;
		}
	} else {
		/* nowhere to load from */
		FIXME(": unknown/unsupported way of loading\n");
		return DMUS_E_LOADER_NOFILENAME; /* test shows this is returned */
	}

	/* create object */
	result = CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);
	if (FAILED(result)) {
		ERR(": could not create object\n");
		return result;
	}
	/* acquire PersistStream interface */
	result = IDirectMusicObject_QueryInterface (pObject, &IID_IPersistStream, (LPVOID*)&pPersistStream);
	if (FAILED(result)) {
		ERR("failed to Query\n");
		return result;
	}
	/* load */
	result = IPersistStream_Load (pPersistStream, pStream);
	if (result != S_OK) {
		WARN(": failed to (completely) load object (%s)\n", debugstr_dmreturn(result));
		ret = DMUS_S_PARTIALLOAD /*result*/;
	}
	/* get descriptor */
	DM_STRUCT_INIT(&GotDesc);
	result = IDirectMusicObject_GetDescriptor (pObject, &GotDesc);
	/* set filename (if we loaded via filename) */
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		GotDesc.dwValidData |= (pDesc->dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH));
		strcpyW (GotDesc.wszFileName, pDesc->wszFileName);
	}
	if (FAILED(result)) {
		ERR(": failed to get descriptor\n");
		return result;
	}
	/* release all loading related stuff */
	IStream_Release (pStream);
	IPersistStream_Release (pPersistStream);	
		
	/* add object to cache/overwrite existing info (if cache is enabled) */
	DMUSIC_GetLoaderSettings (iface, &pDesc->guidClass, NULL, &bCache);
	if (bCache) {
		if (!pObjectEntry) {
			pObjectEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(WINE_LOADER_ENTRY));
			DM_STRUCT_INIT(&pObjectEntry->Desc);
			if (pObject) {
				DMUSIC_CopyDescriptor (&pObjectEntry->Desc, &GotDesc);
				pObjectEntry->pObject = pObject;
				pObjectEntry->bInvalidDefaultDLS = FALSE;
			}
			list_add_head (This->pObjects, &pObjectEntry->entry);
		} else {
			if (pObject) {
				DMUSIC_CopyDescriptor (&pObjectEntry->Desc, &GotDesc);
				pObjectEntry->pObject = pObject;
				pObjectEntry->bInvalidDefaultDLS = FALSE;
			}		
		}
		TRACE(": filled in cache entry\n");
	} else TRACE(": caching disabled\n");

#if 0
	/* for debug purposes (e.g. to check if all files are cached) */
	TRACE("*** Loader's cache ***\n");
	int i = 0;
	LIST_FOR_EACH (pEntry, This->pObjects) {
		i++;
		pObjectEntry = LIST_ENTRY(pEntry, WINE_LOADER_ENTRY, entry);
		TRACE(": entry nr. %i:\n%s\n  - bInvalidDefaultDLS = %i\n  - pObject = %p\n", i, debugstr_DMUS_OBJECTDESC(&pObjectEntry->Desc), pObjectEntry->bInvalidDefaultDLS, pObjectEntry->pObject);
	}
#endif
	
	result = IDirectMusicObject_QueryInterface (pObject, riid, ppv);
	if (!bCache) IDirectMusicObject_Release (pObject); /* since loader's reference is not needed */
	/* if there was trouble with loading, and if no other error occurred,
	   we should return DMUS_S_PARTIALLOAD; else, error is returned */
	if (result == S_OK)
		return ret;
	else
		return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	LPSTREAM pStream;
	LPDIRECTMUSICOBJECT pObject;
	DMUS_OBJECTDESC Desc;
	struct list *pEntry;
	LPWINE_LOADER_ENTRY pObjectEntry, pNewEntry;
	HRESULT hr;

	TRACE("(%p, %p): pDesc:\n%s\n", This, pDesc, debugstr_DMUS_OBJECTDESC(pDesc));

	/* create stream and load additional info from it */
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* generate filename; if it's full path, don't add search 
		   directory path, otherwise do */
		WCHAR wszFileName[MAX_PATH];

		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW(wszFileName, pDesc->wszFileName);
		} else {
			WCHAR *p;
			WCHAR wszSearchPath[MAX_PATH];
			DMUSIC_GetLoaderSettings (iface, &pDesc->guidClass, wszSearchPath, NULL);
			lstrcpyW(wszFileName, wszSearchPath);
			p = wszFileName + lstrlenW(wszFileName);
			if (p > wszFileName && p[-1] != '\\') *p++ = '\\';
			strcpyW(p, pDesc->wszFileName);
		}
		/* create stream */
		hr = DMUSIC_CreateDirectMusicLoaderFileStream ((LPVOID*)&pStream);
		if (FAILED(hr)) {
			ERR(": could not create file stream\n");
			return DMUS_E_LOADER_FAILEDOPEN;
		}
		/* attach stream */
		hr = IDirectMusicLoaderFileStream_Attach (pStream, wszFileName, iface);
		if (FAILED(hr)) {
			ERR(": could not attach stream to file\n");
			IStream_Release (pStream);
			return DMUS_E_LOADER_FAILEDOPEN;
		}
	}
	else if (pDesc->dwValidData & DMUS_OBJ_STREAM) {	
		/* create stream */
		hr = DMUSIC_CreateDirectMusicLoaderGenericStream ((LPVOID*)&pStream);
		if (FAILED(hr)) {
			ERR(": could not create generic stream\n");
			return DMUS_E_LOADER_FAILEDOPEN;
		}
		/* attach stream */
		hr = IDirectMusicLoaderGenericStream_Attach (pStream, pDesc->pStream, iface);
		if (FAILED(hr)) {
			ERR(": could not attach stream\n");
			IStream_Release (pStream);
			return DMUS_E_LOADER_FAILEDOPEN;
		}
	}
	else if (pDesc->dwValidData & DMUS_OBJ_MEMORY) {
		/* create stream */
		hr = DMUSIC_CreateDirectMusicLoaderResourceStream ((LPVOID*)&pStream);
		if (FAILED(hr)) {
			ERR(": could not create resource stream\n");
			return DMUS_E_LOADER_FAILEDOPEN;
		}
		/* attach stream */
		hr = IDirectMusicLoaderResourceStream_Attach (pStream, pDesc->pbMemData, pDesc->llMemLength, 0, iface);
		if (FAILED(hr)) {
			ERR(": could not attach stream to resource\n");
			IStream_Release (pStream);
			return DMUS_E_LOADER_FAILEDOPEN;
		}
	}
	else {
		ERR(": no way to get additional info\n");
		return DMUS_E_LOADER_FAILEDOPEN;
	}

	/* create object */
	CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);

	/* *sigh*... some ms objects have lousy implementation of ParseDescriptor that clears input descriptor :( */
#ifdef NOW_WE_ARE_FREE
	/* parse descriptor: we actually use input descriptor; fields that aren't available from stream remain,
	   otherwise real info is set */
	 IDirectMusicObject_ParseDescriptor (pObject, pStream, pDesc);
#endif
	/* hmph... due to some trouble I had with certain tests, we store current position and then set it back */
	DM_STRUCT_INIT(&Desc);
	if (FAILED(IDirectMusicObject_ParseDescriptor (pObject, pStream, &Desc))) {
		ERR(": couldn't parse descriptor\n");
		return DMUS_E_LOADER_FORMATNOTSUPPORTED;
	}

	/* copy elements from parsed descriptor into input descriptor; this sets new info, overwriting if necessary,
	   but leaves info that's provided by input and not available from stream */	
	DMUSIC_CopyDescriptor (pDesc, &Desc);
	
	/* release everything */
	IDirectMusicObject_Release (pObject);
	IStream_Release (pStream);	
	
	/* sometimes it happens that twisted programs call SetObject for same object twice...
	   in such cases, native loader returns S_OK and does nothing... a sound plan */
	LIST_FOR_EACH (pEntry, This->pObjects) {
		pObjectEntry = LIST_ENTRY (pEntry, WINE_LOADER_ENTRY, entry);
		if (!memcmp (&pObjectEntry->Desc, pDesc, sizeof(DMUS_OBJECTDESC))) {
			TRACE(": exacly same entry already exists\n");
			return S_OK;
		}
	}		
	
	/* add new entry */
	TRACE(": adding alias entry with following info: \n%s\n", debugstr_DMUS_OBJECTDESC(pDesc));
	pNewEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(WINE_LOADER_ENTRY));
	/* use this function instead of pure memcpy due to streams (memcpy just copies pointer), 
	   which is basically used further by app that called SetDescriptor... better safety than exception */
	DMUSIC_CopyDescriptor (&pNewEntry->Desc, pDesc);
	list_add_head (This->pObjects, &pNewEntry->entry);

	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear) {
	WCHAR wszCurrentPath[MAX_PATH];
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %s, %s, %d)\n", This, debugstr_dmguid(rguidClass), debugstr_w(pwzPath), fClear);
	FIXME(": fClear ignored\n");
	DMUSIC_GetLoaderSettings (iface, rguidClass, wszCurrentPath, NULL);
	if (!strncmpW(wszCurrentPath, pwzPath, MAX_PATH)) {
	  return S_FALSE;
	}
	/* FIXME: check if path is valid; else return DMUS_E_LOADER_BADPATH */
	return DMUSIC_SetLoaderSettings (iface, rguidClass, pwzPath, NULL);
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName) {
	static const WCHAR wszAny[] = {'*',0};
	WIN32_FIND_DATAW FileData;
	HANDLE hSearch;
	WCHAR wszSearchString[MAX_PATH];
	WCHAR *p;
	HRESULT result;
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %s, %p, %p)\n", This, debugstr_dmguid(rguidClass), pwzFileExtension, pwzScanFileName);
	if (IsEqualGUID (rguidClass, &GUID_DirectMusicAllTypes) || !DMUSIC_IsValidLoadableClass(rguidClass)) {
		ERR(": rguidClass invalid CLSID\n");
		return REGDB_E_CLASSNOTREG;
	}
	
	/* get search path for given class */
	DMUSIC_GetLoaderSettings (iface, rguidClass, wszSearchString, NULL);
	
	p = wszSearchString + lstrlenW(wszSearchString);
	if (p > wszSearchString && p[-1] != '\\') *p++ = '\\';
	*p++ = '*'; /* any file */
	if (strcmpW (pwzFileExtension, wszAny)) *p++ = '.'; /* if we have actual extension, put a dot */
	strcpyW (p, pwzFileExtension);
	
	TRACE(": search string: %s\n", debugstr_w(wszSearchString));
	
	hSearch = FindFirstFileW (wszSearchString, &FileData);
	if (hSearch == INVALID_HANDLE_VALUE) {
		TRACE(": no files found\n");
		return S_FALSE;
	}
	
	do {
		DMUS_OBJECTDESC Desc;
		DM_STRUCT_INIT(&Desc);
		Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_DATE;
		memcpy (&Desc.guidClass, rguidClass, sizeof(GUID));
		strcpyW (Desc.wszFileName, FileData.cFileName);
		FileTimeToLocalFileTime (&FileData.ftCreationTime, &Desc.ftDate);
		IDirectMusicLoader8_SetObject (iface, &Desc);
		
		if (!FindNextFileW (hSearch, &FileData)) {
			if (GetLastError () == ERROR_NO_MORE_FILES) {
				TRACE(": search completed\n");
				result = S_OK;
			} else {
				ERR(": could not get next file\n");
				result = E_FAIL;
			}
			FindClose (hSearch);
			return result;
		}
	} while (1);
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject) {
	DMUS_OBJECTDESC Desc;
	HRESULT result = DMUS_E_LOADER_OBJECTNOTFOUND;
	struct list *pEntry;
	LPWINE_LOADER_ENTRY  pObjectEntry = NULL;

	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %p)\n", This, pObject);
	
	/* get descriptor */
	DM_STRUCT_INIT(&Desc);
	IDirectMusicObject_GetDescriptor (pObject, &Desc);
	
	/* now iterate thru list and check if we have alias (without object), corresponding
	   to descriptor of input object */
	LIST_FOR_EACH(pEntry, This->pObjects) {
		pObjectEntry = LIST_ENTRY(pEntry, WINE_LOADER_ENTRY, entry);
		if ((Desc.dwValidData & DMUS_OBJ_OBJECT) &&
			(pObjectEntry->Desc.dwValidData & DMUS_OBJ_OBJECT) &&
			IsEqualGUID (&Desc.guidObject, &pObjectEntry->Desc.guidObject)) {
			TRACE(": found it by object GUID\n");
			if ((pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED) && pObjectEntry->pObject)
				result = S_FALSE;
			else
				result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH)) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH)) &&
				!strncmpW (Desc.wszFileName, pObjectEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by fullpath filename\n");
			if ((pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED) && pObjectEntry->pObject)
				result = S_FALSE;
			else
				result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY)) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY)) &&
				!strncmpW (Desc.wszName, pObjectEntry->Desc.wszName, DMUS_MAX_NAME) &&
				!strncmpW (Desc.wszCategory, pObjectEntry->Desc.wszCategory, DMUS_MAX_CATEGORY)) {
			TRACE(": found it by name and category\n");
			if ((pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED) && pObjectEntry->pObject)
				result = S_FALSE;
			else
				result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & DMUS_OBJ_NAME) &&
				(pObjectEntry->Desc.dwValidData & DMUS_OBJ_NAME) &&
				!strncmpW (Desc.wszName, pObjectEntry->Desc.wszName, DMUS_MAX_NAME)) {
			TRACE(": found it by name\n");
			if ((pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED) && pObjectEntry->pObject)
				result = S_FALSE;
			else
				result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & DMUS_OBJ_FILENAME) &&
				(pObjectEntry->Desc.dwValidData & DMUS_OBJ_FILENAME) &&
				!strncmpW (Desc.wszFileName, pObjectEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by filename\n");				
			if ((pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED) && pObjectEntry->pObject)
				result = S_FALSE;
			else
				result = S_OK;
			break;
		}
	}
	
	/* if we found such alias, then set everything */
	if (result == S_OK) {
		pObjectEntry->Desc.dwValidData &= DMUS_OBJ_LOADED;
		pObjectEntry->pObject = pObject;
		IDirectMusicObject_AddRef (pObjectEntry->pObject);
	}
	
	return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject) {
	DMUS_OBJECTDESC Desc;
	struct list *pEntry;
	LPWINE_LOADER_ENTRY pObjectEntry = NULL;
	HRESULT result = S_FALSE;

	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %p)\n", This, pObject);
	
	/* get descriptor */
	DM_STRUCT_INIT(&Desc);
	IDirectMusicObject_GetDescriptor (pObject, &Desc);
	
	/* iterate thru the list of objects we know about; check only those with DMUS_OBJ_LOADED */
	TRACE(": looking for the object in cache\n");
	LIST_FOR_EACH(pEntry, This->pObjects) {
		pObjectEntry = LIST_ENTRY(pEntry, WINE_LOADER_ENTRY, entry);
		if ((Desc.dwValidData & DMUS_OBJ_OBJECT) &&
			(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_OBJECT | DMUS_OBJ_LOADED)) &&
			IsEqualGUID (&Desc.guidObject, &pObjectEntry->Desc.guidObject)) {
			TRACE(": found it by object GUID\n%s\n", debugstr_DMUS_OBJECTDESC(&pObjectEntry->Desc));
			result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH)) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_LOADED)) &&
				!strncmpW (Desc.wszFileName, pObjectEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by fullpath filename\n");
			result = S_OK;			
			break;
		}
		else if ((Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY)) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_CATEGORY | DMUS_OBJ_LOADED)) &&
				!strncmpW (Desc.wszName, pObjectEntry->Desc.wszName, DMUS_MAX_NAME) &&
				!strncmpW (Desc.wszCategory, pObjectEntry->Desc.wszCategory, DMUS_MAX_CATEGORY)) {
			TRACE(": found it by name and category\n");
			result = S_OK;			
			break;
		}
		else if ((Desc.dwValidData & DMUS_OBJ_NAME) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_NAME | DMUS_OBJ_LOADED)) &&
				!strncmpW (Desc.wszName, pObjectEntry->Desc.wszName, DMUS_MAX_NAME)) {
			TRACE(": found it by name\n");
			result = S_OK;
			break;
		}
		else if ((Desc.dwValidData & DMUS_OBJ_FILENAME) &&
				(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_LOADED)) &&
				!strncmpW (Desc.wszFileName, pObjectEntry->Desc.wszFileName, DMUS_MAX_FILENAME)) {
			TRACE(": found it by filename\n");
			result = S_OK;			
			break;
		}
	}
	if (result == S_OK) {
		/*TRACE(": releasing: \n%s  - bInvalidDefaultDLS = %i\n  - pObject = %p\n", debugstr_DMUS_OBJECTDESC(&pObjectEntry->Desc), pObjectEntry->bInvalidDefaultDLS, pObjectEntry->pObject); */
		IDirectMusicObject_Release (pObjectEntry->pObject);
		pObjectEntry->pObject = NULL;
		pObjectEntry->Desc.dwValidData &= ~DMUS_OBJ_LOADED;
	} 
	return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass) {
	struct list *pEntry;
	LPWINE_LOADER_ENTRY pObjectEntry;
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %s)\n", This, debugstr_dmguid(rguidClass));
	
	LIST_FOR_EACH (pEntry, This->pObjects) {
		pObjectEntry = LIST_ENTRY (pEntry, WINE_LOADER_ENTRY, entry);
		
		if ((IsEqualGUID (rguidClass, &GUID_DirectMusicAllTypes) || IsEqualGUID (rguidClass, &pObjectEntry->Desc.guidClass)) &&
			(pObjectEntry->Desc.dwValidData & DMUS_OBJ_LOADED)) {
			/* basically, wrap to ReleaseObject for each object found */
			IDirectMusicLoader8_ReleaseObject (iface, pObjectEntry->pObject);
		}
	}
	
	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	BOOL bCurrent;
	TRACE("(%p, %s, %d)\n", This, debugstr_dmguid(rguidClass), fEnable);
	DMUSIC_GetLoaderSettings (iface, rguidClass, NULL, &bCurrent);
	if (bCurrent == fEnable)
		return S_FALSE;
	else
		return DMUSIC_SetLoaderSettings (iface, rguidClass, NULL, &fEnable);
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc) {
	DWORD dwCount = 0;
	struct list *pEntry;
	LPWINE_LOADER_ENTRY pObjectEntry;
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	TRACE("(%p, %s, %d, %p)\n", This, debugstr_dmguid(rguidClass), dwIndex, pDesc);
	
	DM_STRUCT_INIT(pDesc);
	
	LIST_FOR_EACH (pEntry, This->pObjects) {
		pObjectEntry = LIST_ENTRY (pEntry, WINE_LOADER_ENTRY, entry);
			
		if (IsEqualGUID (rguidClass, &GUID_DirectMusicAllTypes) || IsEqualGUID (rguidClass, &pObjectEntry->Desc.guidClass)) {
			if (dwCount == dwIndex) {
				memcpy (pDesc, &pObjectEntry->Desc, sizeof(DMUS_OBJECTDESC));
				/* we aren't supposed to reveal this info */
				pDesc->dwValidData &= ~(DMUS_OBJ_MEMORY | DMUS_OBJ_STREAM);
				pDesc->pbMemData = NULL;
				pDesc->llMemLength = 0;
				pDesc->pStream = NULL;
				return S_OK;
			}
			dwCount++;
		}
	}
	
	TRACE(": not found\n");	
	return S_FALSE;
}

static void WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_CollectGarbage (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	FIXME("(%p): stub\n", This);
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	HRESULT result;
	LPDIRECTMUSICOBJECT pObjectInterface;
	
	TRACE("(%p, %p)\n", This, pObject);
	
	if (IsBadReadPtr (pObject, sizeof(LPUNKNOWN))) {
		ERR(": pObject bad write pointer\n");
		return E_POINTER;
	}
	/* we simply get IDirectMusicObject interface */
	result = IUnknown_QueryInterface (pObject, &IID_IDirectMusicObject, (LPVOID*)&pObjectInterface);
	if (FAILED(result)) return result;
	/* and release it in old-fashioned way */
	result = IDirectMusicLoader8_ReleaseObject (iface, pObjectInterface);
	IDirectMusicObject_Release (pObjectInterface);
	
	return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_IDirectMusicLoader_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR* pwzFilePath, void** ppObject) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	DMUS_OBJECTDESC ObjDesc;
	WCHAR wszLoaderSearchPath[MAX_PATH];
	
	TRACE("(%p, %s, %s, %s, %p): wrapping to IDirectMusicLoaderImpl_GetObject\n", This, debugstr_dmguid(rguidClassID), debugstr_dmguid(iidInterfaceID), debugstr_w(pwzFilePath), ppObject);
	
	DM_STRUCT_INIT(&ObjDesc);	
	ObjDesc.dwValidData = DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_CLASS; /* I believe I've read somewhere in MSDN that this function requires either full path or relative path */
	memcpy (&ObjDesc.guidClass, rguidClassID, sizeof(CLSID));
	/* OK, MSDN says that search order is the following:
	    - current directory (DONE)
	    - windows search path (FIXME: how do I get that?)
	    - loader's search path (DONE)
	*/
	DMUSIC_GetLoaderSettings (iface, rguidClassID, wszLoaderSearchPath, NULL);
    /* search in current directory */
	if (!SearchPathW (NULL, pwzFilePath, NULL, sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL) &&
	/* search in loader's search path */
		!SearchPathW (wszLoaderSearchPath, pwzFilePath, NULL, sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL)) {
		/* cannot find file */
		TRACE(": cannot find file\n");
		return DMUS_E_LOADER_FAILEDOPEN;
	}
	
	TRACE(": full file path = %s\n", debugstr_w (ObjDesc.wszFileName));
	
	return IDirectMusicLoaderImpl_IDirectMusicLoader_GetObject (iface, &ObjDesc, iidInterfaceID, ppObject);
}

static const IDirectMusicLoader8Vtbl DirectMusicLoader_Loader_Vtbl = {
	IDirectMusicLoaderImpl_IDirectMusicLoader_QueryInterface,
	IDirectMusicLoaderImpl_IDirectMusicLoader_AddRef,
	IDirectMusicLoaderImpl_IDirectMusicLoader_Release,
	IDirectMusicLoaderImpl_IDirectMusicLoader_GetObject,
	IDirectMusicLoaderImpl_IDirectMusicLoader_SetObject,
	IDirectMusicLoaderImpl_IDirectMusicLoader_SetSearchDirectory,
	IDirectMusicLoaderImpl_IDirectMusicLoader_ScanDirectory,
	IDirectMusicLoaderImpl_IDirectMusicLoader_CacheObject,
	IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObject,
	IDirectMusicLoaderImpl_IDirectMusicLoader_ClearCache,
	IDirectMusicLoaderImpl_IDirectMusicLoader_EnableCache,
	IDirectMusicLoaderImpl_IDirectMusicLoader_EnumObject,
	IDirectMusicLoaderImpl_IDirectMusicLoader_CollectGarbage,
	IDirectMusicLoaderImpl_IDirectMusicLoader_ReleaseObjectByUnknown,
	IDirectMusicLoaderImpl_IDirectMusicLoader_LoadObjectFromFile
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicLoaderImpl *obj;
	DMUS_OBJECTDESC Desc;
	LPWINE_LOADER_ENTRY pDefaultDLSEntry;
	struct list *pEntry;

	TRACE("(%s, %p, %p)\n", debugstr_dmguid(lpcGUID), ppobj, pUnkOuter);
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoaderImpl));
	if (NULL == obj) {
		*ppobj = (LPDIRECTMUSICLOADER8)NULL;
		return E_OUTOFMEMORY;
	}
	obj->LoaderVtbl = &DirectMusicLoader_Loader_Vtbl;
	obj->dwRef = 0; /* will be inited with QueryInterface */
	/* init critical section */
	/* init cache/alias list */
	/*InitializeCriticalSection (&obj->CritSect);
	obj->CritSect.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IDirectMusicLoaderImpl.CritSect"); */
	obj->pObjects = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(struct list));
	list_init (obj->pObjects);
	/* init settings */
	obj->pClassSettings = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(struct list));
	list_init (obj->pClassSettings);
	DMUSIC_InitLoaderSettings ((LPDIRECTMUSICLOADER8)obj);

	/* set default DLS collection (via SetObject... so that loading via DMUS_OBJ_OBJECT is possible) */
	DM_STRUCT_INIT(&Desc);
	Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_OBJECT;
	memcpy (&Desc.guidClass, &CLSID_DirectMusicCollection, sizeof(CLSID));
	memcpy (&Desc.guidObject, &GUID_DefaultGMCollection, sizeof(GUID));
	DMUSIC_GetDefaultGMPath (Desc.wszFileName);
	IDirectMusicLoader_SetObject ((LPDIRECTMUSICLOADER8)obj, &Desc);
	/* and now the workaroundTM for "invalid" default DLS; basically, 
	   my tests showed that if GUID chunk is present in default DLS 
	   collection, loader treats it as "invalid" and returns 
	   DMUS_E_LOADER_NOFILENAME for all requests for it; basically, we check 
	   if out input guidObject was overwritten */
	pEntry = list_head (obj->pObjects);
	pDefaultDLSEntry = LIST_ENTRY (pEntry, WINE_LOADER_ENTRY, entry);
	if (!IsEqualGUID(&Desc.guidObject, &GUID_DefaultGMCollection)) {
		pDefaultDLSEntry->bInvalidDefaultDLS = TRUE;
	}

	/* increase number of instances */
	InterlockedIncrement (&dwDirectMusicLoader);
	
	return IDirectMusicLoaderImpl_IDirectMusicLoader_QueryInterface ((LPDIRECTMUSICLOADER8)obj, lpcGUID, ppobj);
}

HRESULT WINAPI DMUSIC_DestroyDirectMusicLoaderImpl (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);

	TRACE("(%p)\n", This);
	
	/* firstly, release the cache */
	IDirectMusicLoader8_ClearCache (iface, &GUID_DirectMusicAllTypes);
	/* FIXME: release all allocated entries */
	/* destroy critical section */
	/*This->CritSect.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection (&This->CritSect); */
	
	/* decrease number of instances */
	InterlockedDecrement (&dwDirectMusicLoader);
	
	return S_OK;
}

/* help function for DMUSIC_SetDefaultDLS */
HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]) {
	HKEY hkDM;
	DWORD returnType, sizeOfReturnBuffer = MAX_PATH;
	char szPath[MAX_PATH];

	if ((RegOpenKeyExA (HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic" , 0, KEY_READ, &hkDM) != ERROR_SUCCESS) || 
	    (RegQueryValueExA (hkDM, "GMFilePath", NULL, &returnType, (LPBYTE) szPath, &sizeOfReturnBuffer) != ERROR_SUCCESS)) {
		WARN(": registry entry missing\n" );
		return E_FAIL;
	}
	/* FIXME: Check return types to ensure we're interpreting data right */
	MultiByteToWideChar (CP_ACP, 0, szPath, -1, wszPath, MAX_PATH);
	
	return S_OK;
}

/* help function for retrieval of search path and caching option for certain class */
HRESULT WINAPI DMUSIC_GetLoaderSettings (LPDIRECTMUSICLOADER8 iface, REFGUID pClassID, WCHAR* wszSearchPath, LPBOOL pbCache) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	struct list *pEntry;
	TRACE(": (%p, %s, %p, %p)\n", This, debugstr_dmguid(pClassID), wszSearchPath, pbCache);
	
	LIST_FOR_EACH(pEntry, This->pClassSettings) {
		LPWINE_LOADER_OPTION pOptionEntry = LIST_ENTRY(pEntry, WINE_LOADER_OPTION, entry);
		if (IsEqualCLSID (pClassID, &pOptionEntry->guidClass)) {
			if (wszSearchPath)
				strcpyW(wszSearchPath, pOptionEntry->wszSearchPath);
			if (pbCache)
				*pbCache = pOptionEntry->bCache;
			return S_OK;
		}
	}
	return S_FALSE;
}

/* help function for setting search path and caching option for certain class */
static HRESULT WINAPI DMUSIC_SetLoaderSettings (LPDIRECTMUSICLOADER8 iface, REFGUID pClassID, WCHAR* wszSearchPath, LPBOOL pbCache) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	struct list *pEntry;
	HRESULT result = S_FALSE; /* in case pClassID != GUID_DirectMusicAllTypes and not a valid CLSID */
	TRACE(": (%p, %s, %p, %p)\n", This, debugstr_dmguid(pClassID), wszSearchPath, pbCache);
	
	LIST_FOR_EACH(pEntry, This->pClassSettings) {
		LPWINE_LOADER_OPTION pOptionEntry = LIST_ENTRY(pEntry, WINE_LOADER_OPTION, entry);
		/* well, either we have GUID_DirectMusicAllTypes and need to set it to all,
		   or specific CLSID is given and we set it only to it */
		if (IsEqualGUID (pClassID, &GUID_DirectMusicAllTypes) ||
			IsEqualCLSID (pClassID, &pOptionEntry->guidClass)) {
			if (wszSearchPath)
				strcpyW(pOptionEntry->wszSearchPath, wszSearchPath);
			if (pbCache)
				pOptionEntry->bCache = *pbCache;
			result = S_OK;
		}
	}

	return result;
}

HRESULT WINAPI DMUSIC_InitLoaderSettings (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS_MULTI(IDirectMusicLoaderImpl, LoaderVtbl, iface);
	
	/* hard-coded list of classes */
	static REFCLSID classes[] = {
		&CLSID_DirectMusicAudioPathConfig,
		&CLSID_DirectMusicBand,
		&CLSID_DirectMusicContainer,
		&CLSID_DirectMusicCollection,
		&CLSID_DirectMusicChordMap,
		&CLSID_DirectMusicSegment,
		&CLSID_DirectMusicScript,
		&CLSID_DirectMusicSong,
		&CLSID_DirectMusicStyle,
		&CLSID_DirectMusicGraph,
		&CLSID_DirectSoundWave
	};
	
	unsigned int i;
	WCHAR wszCurrent[MAX_PATH];

	TRACE(": (%p)\n", This);
	GetCurrentDirectoryW (MAX_PATH, wszCurrent);
	
	for (i = 0; i < sizeof(classes)/sizeof(REFCLSID); i++) {
		LPWINE_LOADER_OPTION pNewSetting = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(WINE_LOADER_OPTION));
		memcpy (&pNewSetting->guidClass, classes[i], sizeof(CLSID));
		strcpyW (pNewSetting->wszSearchPath, wszCurrent);
		pNewSetting->bCache = TRUE;
		list_add_tail (This->pClassSettings, &pNewSetting->entry);
	}

	return S_OK;
}

HRESULT WINAPI DMUSIC_CopyDescriptor (LPDMUS_OBJECTDESC pDst, LPDMUS_OBJECTDESC pSrc) {
	TRACE(": copy \n%s", debugstr_DMUS_OBJECTDESC(pSrc));
	/* copy field by field */
	if (pSrc->dwValidData & DMUS_OBJ_CLASS) memcpy (&pDst->guidClass, &pSrc->guidClass, sizeof(CLSID));
	if (pSrc->dwValidData & DMUS_OBJ_OBJECT) memcpy (&pDst->guidObject, &pSrc->guidObject, sizeof(GUID));
	if (pSrc->dwValidData & DMUS_OBJ_DATE) memcpy (&pDst->ftDate, &pSrc->ftDate, sizeof(FILETIME));
	if (pSrc->dwValidData & DMUS_OBJ_VERSION) memcpy (&pDst->vVersion, &pSrc->vVersion, sizeof(DMUS_VERSION));
	if (pSrc->dwValidData & DMUS_OBJ_NAME) strcpyW (pDst->wszName, pSrc->wszName);
	if (pSrc->dwValidData & DMUS_OBJ_CATEGORY) strcpyW (pDst->wszCategory, pSrc->wszCategory);
	if (pSrc->dwValidData & DMUS_OBJ_FILENAME) strcpyW (pDst->wszFileName, pSrc->wszFileName);
	if (pSrc->dwValidData & DMUS_OBJ_STREAM) IStream_Clone (pSrc->pStream, &pDst->pStream);
	if (pSrc->dwValidData & DMUS_OBJ_MEMORY) {
		pDst->pbMemData = pSrc->pbMemData;
		pDst->llMemLength = pSrc->llMemLength;
	}
	/* set flags */
	pDst->dwValidData |= pSrc->dwValidData;
	return S_OK;
}

BOOL WINAPI DMUSIC_IsValidLoadableClass (REFCLSID pClassID) {
	if (IsEqualCLSID(pClassID, &CLSID_DirectMusicAudioPathConfig) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicBand) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicContainer) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicCollection) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicChordMap) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicSegment) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicScript) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicSong) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicStyle) ||
		IsEqualCLSID(pClassID, &CLSID_DirectMusicGraph) ||
		IsEqualCLSID(pClassID, &CLSID_DirectSoundWave) ||
		IsEqualCLSID(pClassID, &GUID_DirectMusicAllTypes))
		return TRUE;
	else
		return FALSE;
}
