/*
 * IDirectMusicLoaderImpl
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

static const GUID *classes[] = {
    &GUID_DirectMusicAllTypes,  /* Keep as first */
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

/* cache/alias entry */
struct cache_entry {
    struct list entry;
    DMUS_OBJECTDESC Desc;
    IDirectMusicObject *pObject;
    BOOL bInvalidDefaultDLS; /* workaround for enabling caching of "faulty" default dls collection */
};

typedef struct IDirectMusicLoaderImpl {
    IDirectMusicLoader8 IDirectMusicLoader8_iface;
    LONG ref;
    WCHAR *search_paths[ARRAY_SIZE(classes)];
    unsigned int cache_class;
    struct list cache;
} IDirectMusicLoaderImpl;


static inline IDirectMusicLoaderImpl* impl_from_IDirectMusicLoader8(IDirectMusicLoader8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectMusicLoaderImpl, IDirectMusicLoader8_iface);
}

static int index_from_class(REFCLSID class)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(classes); i++)
        if (IsEqualGUID(class, classes[i]))
            return i;

    return -1;
}

static inline BOOL is_cache_enabled(IDirectMusicLoaderImpl *This, REFCLSID class)
{
    return !!(This->cache_class & 1 << index_from_class(class));
}

static void get_search_path(IDirectMusicLoaderImpl *This, REFGUID class, WCHAR *path)
{
    int index = index_from_class(class);

    if (index < 0 || !This->search_paths[index])
        index = 0; /* Default to GUID_DirectMusicAllTypes */

    if (This->search_paths[index])
        lstrcpynW(path, This->search_paths[index], MAX_PATH);
    else
        path[0] = 0;
}

static HRESULT DMUSIC_CopyDescriptor(DMUS_OBJECTDESC *pDst, DMUS_OBJECTDESC *pSrc)
{
	if (TRACE_ON(dmloader))
		dump_DMUS_OBJECTDESC(pSrc);

	/* copy field by field */
	if (pSrc->dwValidData & DMUS_OBJ_CLASS) pDst->guidClass = pSrc->guidClass;
	if (pSrc->dwValidData & DMUS_OBJ_OBJECT) pDst->guidObject = pSrc->guidObject;
	if (pSrc->dwValidData & DMUS_OBJ_DATE) pDst->ftDate = pSrc->ftDate;
	if (pSrc->dwValidData & DMUS_OBJ_VERSION) pDst->vVersion = pSrc->vVersion;
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

/*****************************************************************************
 * IDirectMusicLoaderImpl implementation
 */
/* IUnknown/IDirectMusicLoader(8) part: */

static HRESULT WINAPI IDirectMusicLoaderImpl_QueryInterface(IDirectMusicLoader8 *iface, REFIID riid, void **ppobj)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);

	TRACE("(%p, %s, %p)\n",This, debugstr_dmguid(riid), ppobj);
	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IDirectMusicLoader) ||
	    IsEqualIID (riid, &IID_IDirectMusicLoader8)) {
		IDirectMusicLoader_AddRef (iface);
		*ppobj = This;
		return S_OK;
	}
	
	WARN(": not found\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicLoaderImpl_AddRef(IDirectMusicLoader8 *iface)
{
    IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicLoaderImpl_Release(IDirectMusicLoader8 *iface)
{
    IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref) {
        unsigned int i;

        IDirectMusicLoader8_ClearCache(iface, &GUID_DirectMusicAllTypes);
        for (i = 0; i < ARRAY_SIZE(classes); i++)
            HeapFree(GetProcessHeap(), 0, This->search_paths[i]);
        HeapFree(GetProcessHeap(), 0, This);
        unlock_module();
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_GetObject(IDirectMusicLoader8 *iface, DMUS_OBJECTDESC *pDesc, REFIID riid, void **ppv)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	HRESULT result = S_OK;
	HRESULT ret = S_OK; /* used at the end of function, to determine whether everything went OK */
        struct cache_entry *pExistingEntry, *pObjectEntry = NULL;
	LPSTREAM pStream;
	IPersistStream* pPersistStream = NULL;

	LPDIRECTMUSICOBJECT pObject;
	DMUS_OBJECTDESC GotDesc;
	BOOL bCache;

	TRACE("(%p)->(%p, %s, %p)\n", This, pDesc, debugstr_dmguid(riid), ppv);

	if (TRACE_ON(dmloader))
	  dump_DMUS_OBJECTDESC(pDesc);
	
	/* sometimes it happens that guidClass is missing... which is a BadThingTM */
	if (!(pDesc->dwValidData & DMUS_OBJ_CLASS)) {
	  ERR(": guidClass not valid but needed\n");
	  *ppv = NULL;
	  return DMUS_E_LOADER_NOCLASSID;
	}
	
	/* OK, first we iterate through the list of objects we know about; these are either loaded (GetObject, LoadObjectFromFile)
	   or set via SetObject; */
	TRACE(": looking if we have object in the cache or if it can be found via alias\n");
        LIST_FOR_EACH_ENTRY(pExistingEntry, &This->cache, struct cache_entry, entry) {
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
		/* I think in certain cases it can happen that entry's descriptor lacks info about
		   where to load from (e.g.: if we loaded from stream and then released object
		   from cache; then only its CLSID, GUID and perhaps name are left); so just
		   overwrite whatever info the entry has (since it ought to be 100% correct) */
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
                        WCHAR *p;
                        get_search_path(This, &pDesc->guidClass, wszFileName);
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
		return result;
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
        bCache = is_cache_enabled(This, &pDesc->guidClass);
	if (bCache) {
		if (!pObjectEntry) {
                        pObjectEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(*pObjectEntry));
			DM_STRUCT_INIT(&pObjectEntry->Desc);
			DMUSIC_CopyDescriptor (&pObjectEntry->Desc, &GotDesc);
			pObjectEntry->pObject = pObject;
			pObjectEntry->bInvalidDefaultDLS = FALSE;
                        list_add_head(&This->cache, &pObjectEntry->entry);
		} else {
			DMUSIC_CopyDescriptor (&pObjectEntry->Desc, &GotDesc);
			pObjectEntry->pObject = pObject;
			pObjectEntry->bInvalidDefaultDLS = FALSE;
		}
		TRACE(": filled in cache entry\n");
	} else TRACE(": caching disabled\n");

	result = IDirectMusicObject_QueryInterface (pObject, riid, ppv);
	if (!bCache) IDirectMusicObject_Release (pObject); /* since loader's reference is not needed */
	/* if there was trouble with loading, and if no other error occurred,
	   we should return DMUS_S_PARTIALLOAD; else, error is returned */
	if (result == S_OK)
		return ret;
	else
		return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_SetObject(IDirectMusicLoader8 *iface, DMUS_OBJECTDESC *pDesc)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	LPSTREAM pStream;
	LPDIRECTMUSICOBJECT pObject;
	DMUS_OBJECTDESC Desc;
        struct cache_entry *pObjectEntry, *pNewEntry;
	HRESULT hr;

	TRACE("(%p)->(%p)\n", This, pDesc);

	if (TRACE_ON(dmloader))
		dump_DMUS_OBJECTDESC(pDesc);

	/* create stream and load additional info from it */
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* generate filename; if it's full path, don't add search 
		   directory path, otherwise do */
		WCHAR wszFileName[MAX_PATH];

		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW(wszFileName, pDesc->wszFileName);
		} else {
			WCHAR *p;
                        get_search_path(This, &pDesc->guidClass, wszFileName);
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
			ERR(": could not attach stream to file %s, make sure it exists\n", debugstr_w(wszFileName));
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
	hr = CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);
        if (FAILED(hr)) {
		ERR("Object creation of %s failed 0x%08x\n", debugstr_guid(&pDesc->guidClass),hr);
		return DMUS_E_LOADER_FAILEDOPEN;
        }

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
        LIST_FOR_EACH_ENTRY(pObjectEntry, &This->cache, struct cache_entry, entry) {
		if (!memcmp (&pObjectEntry->Desc, pDesc, sizeof(DMUS_OBJECTDESC))) {
			TRACE(": exactly same entry already exists\n");
			return S_OK;
		}
	}		
	
	/* add new entry */
	TRACE(": adding alias entry with following info:\n");
	if (TRACE_ON(dmloader))
		dump_DMUS_OBJECTDESC(pDesc);
        pNewEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(*pNewEntry));
	/* use this function instead of pure memcpy due to streams (memcpy just copies pointer), 
	   which is basically used further by app that called SetDescriptor... better safety than exception */
	DMUSIC_CopyDescriptor (&pNewEntry->Desc, pDesc);
        list_add_head(&This->cache, &pNewEntry->entry);

	return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_SetSearchDirectory(IDirectMusicLoader8 *iface,
        REFGUID class, WCHAR *path, BOOL clear)
{
    IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
    int index = index_from_class(class);
    DWORD attr;

    TRACE("(%p, %s, %s, %d)\n", This, debugstr_dmguid(class), debugstr_w(path), clear);

    if (!path)
        return E_POINTER;

    if (path[0]) {
        attr = GetFileAttributesW(path);
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
            return DMUS_E_LOADER_BADPATH;
    }

    if (clear)
        FIXME("clear flag ignored\n");

    /* Ignore invalid GUIDs */
    if (index < 0)
        return S_OK;

    if (!This->search_paths[index])
        This->search_paths[index] = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    else if (!strncmpW(This->search_paths[index], path, MAX_PATH))
        return S_FALSE;

    lstrcpynW(This->search_paths[index], path, MAX_PATH);

    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_ScanDirectory(IDirectMusicLoader8 *iface, REFGUID rguidClass, WCHAR *pwzFileExtension, WCHAR *pwzScanFileName)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	static const WCHAR wszAny[] = {'*',0};
	WIN32_FIND_DATAW FileData;
	HANDLE hSearch;
	WCHAR wszSearchString[MAX_PATH];
	WCHAR *p;
	HRESULT result;
        TRACE("(%p, %s, %s, %s)\n", This, debugstr_dmguid(rguidClass), debugstr_w(pwzFileExtension),
                        debugstr_w(pwzScanFileName));
        if (index_from_class(rguidClass) <= 0) {
		ERR(": rguidClass invalid CLSID\n");
		return REGDB_E_CLASSNOTREG;
	}

        if (!pwzFileExtension)
                return S_FALSE;

	/* get search path for given class */
        get_search_path(This, rguidClass, wszSearchString);

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
		Desc.guidClass = *rguidClass;
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

static HRESULT WINAPI IDirectMusicLoaderImpl_CacheObject(IDirectMusicLoader8 *iface, IDirectMusicObject *pObject)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	DMUS_OBJECTDESC Desc;
	HRESULT result = DMUS_E_LOADER_OBJECTNOTFOUND;
        struct cache_entry *pObjectEntry;

	TRACE("(%p, %p)\n", This, pObject);
	
	/* get descriptor */
	DM_STRUCT_INIT(&Desc);
	IDirectMusicObject_GetDescriptor (pObject, &Desc);
	
	/* now iterate through the list and check if we have an alias (without object), corresponding
	   to the descriptor of the input object */
        LIST_FOR_EACH_ENTRY(pObjectEntry, &This->cache, struct cache_entry, entry) {
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

static HRESULT WINAPI IDirectMusicLoaderImpl_ReleaseObject(IDirectMusicLoader8 *iface, IDirectMusicObject *pObject)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	DMUS_OBJECTDESC Desc;
        struct cache_entry *pObjectEntry;
	HRESULT result = S_FALSE;

	TRACE("(%p, %p)\n", This, pObject);
	
	if(!pObject) return E_POINTER;

	/* get descriptor */
	DM_STRUCT_INIT(&Desc);
	IDirectMusicObject_GetDescriptor (pObject, &Desc);
	
	/* iterate through the list of objects we know about; check only those with DMUS_OBJ_LOADED */
	TRACE(": looking for the object in cache\n");
        LIST_FOR_EACH_ENTRY(pObjectEntry, &This->cache, struct cache_entry, entry) {
		if ((Desc.dwValidData & DMUS_OBJ_OBJECT) &&
			(pObjectEntry->Desc.dwValidData & (DMUS_OBJ_OBJECT | DMUS_OBJ_LOADED)) &&
			IsEqualGUID (&Desc.guidObject, &pObjectEntry->Desc.guidObject)) {
			TRACE(": found it by object GUID\n");
			if (TRACE_ON(dmloader))
				dump_DMUS_OBJECTDESC(&pObjectEntry->Desc);
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
		/*TRACE(": releasing:\n%s  - bInvalidDefaultDLS = %i\n  - pObject = %p\n", debugstr_DMUS_OBJECTDESC(&pObjectEntry->Desc), pObjectEntry->bInvalidDefaultDLS, pObjectEntry->pObject); */
		IDirectMusicObject_Release (pObjectEntry->pObject);
		pObjectEntry->pObject = NULL;
		pObjectEntry->Desc.dwValidData &= ~DMUS_OBJ_LOADED;
	} 
	return result;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_ClearCache(IDirectMusicLoader8 *iface, REFGUID class)
{
    IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
    struct cache_entry *obj, *obj2;

    TRACE("(%p, %s)\n", This, debugstr_dmguid(class));

    LIST_FOR_EACH_ENTRY_SAFE(obj, obj2, &This->cache, struct cache_entry, entry) {
        if ((IsEqualGUID(class, &GUID_DirectMusicAllTypes) || IsEqualGUID(class, &obj->Desc.guidClass)) &&
            (obj->Desc.dwValidData & DMUS_OBJ_LOADED)) {
            /* basically, wrap to ReleaseObject for each object found */
            IDirectMusicLoader8_ReleaseObject(iface, obj->pObject);
            list_remove(&obj->entry);
            HeapFree(GetProcessHeap(), 0, obj);
        }
    }

    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_EnableCache(IDirectMusicLoader8 *iface, REFGUID class,
        BOOL enable)
{
    IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
    BOOL current;

    TRACE("(%p, %s, %d)\n", This, debugstr_dmguid(class), enable);

    current = is_cache_enabled(This, class);

    if (IsEqualGUID(class, &GUID_DirectMusicAllTypes))
        This->cache_class = enable ? ~0 : 0;
    else {
        if (enable)
            This->cache_class |= 1 << index_from_class(class);
        else
            This->cache_class &= ~(1 << index_from_class(class));
    }

    if (!enable)
        IDirectMusicLoader8_ClearCache(iface, class);

    if (current == enable)
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI IDirectMusicLoaderImpl_EnumObject(IDirectMusicLoader8 *iface, REFGUID rguidClass, DWORD dwIndex, DMUS_OBJECTDESC *pDesc)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	DWORD dwCount = 0;
        struct cache_entry *pObjectEntry;
	TRACE("(%p, %s, %d, %p)\n", This, debugstr_dmguid(rguidClass), dwIndex, pDesc);

	DM_STRUCT_INIT(pDesc);

        LIST_FOR_EACH_ENTRY(pObjectEntry, &This->cache, struct cache_entry, entry) {
		if (IsEqualGUID (rguidClass, &GUID_DirectMusicAllTypes) || IsEqualGUID (rguidClass, &pObjectEntry->Desc.guidClass)) {
			if (dwCount == dwIndex) {
				*pDesc = pObjectEntry->Desc;
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

static void WINAPI IDirectMusicLoaderImpl_CollectGarbage(IDirectMusicLoader8 *iface)
{
    FIXME("(%p)->(): stub\n", iface);
}

static HRESULT WINAPI IDirectMusicLoaderImpl_ReleaseObjectByUnknown(IDirectMusicLoader8 *iface, IUnknown *pObject)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	HRESULT result;
	LPDIRECTMUSICOBJECT pObjectInterface;
	
	TRACE("(%p, %p)\n", This, pObject);
	
	if (IsBadReadPtr (pObject, sizeof(*pObject))) {
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

static HRESULT WINAPI IDirectMusicLoaderImpl_LoadObjectFromFile(IDirectMusicLoader8 *iface, REFGUID rguidClassID, REFIID iidInterfaceID, WCHAR *pwzFilePath, void **ppObject)
{
	IDirectMusicLoaderImpl *This = impl_from_IDirectMusicLoader8(iface);
	DMUS_OBJECTDESC ObjDesc;
	WCHAR wszLoaderSearchPath[MAX_PATH];

	TRACE("(%p, %s, %s, %s, %p): wrapping to IDirectMusicLoaderImpl_GetObject\n", This, debugstr_dmguid(rguidClassID), debugstr_dmguid(iidInterfaceID), debugstr_w(pwzFilePath), ppObject);

	DM_STRUCT_INIT(&ObjDesc);	
	ObjDesc.dwValidData = DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_CLASS; /* I believe I've read somewhere in MSDN that this function requires either full path or relative path */
	ObjDesc.guidClass = *rguidClassID;
	/* OK, MSDN says that search order is the following:
	    - current directory (DONE)
	    - windows search path (FIXME: how do I get that?)
	    - loader's search path (DONE)
	*/
        get_search_path(This, rguidClassID, wszLoaderSearchPath);
    /* search in current directory */
	if (!SearchPathW (NULL, pwzFilePath, NULL, sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL) &&
	/* search in loader's search path */
		!SearchPathW (wszLoaderSearchPath, pwzFilePath, NULL, sizeof(ObjDesc.wszFileName)/sizeof(WCHAR), ObjDesc.wszFileName, NULL)) {
		/* cannot find file */
		TRACE(": cannot find file\n");
		return DMUS_E_LOADER_FAILEDOPEN;
	}
	
	TRACE(": full file path = %s\n", debugstr_w (ObjDesc.wszFileName));
	
	return IDirectMusicLoader_GetObject(iface, &ObjDesc, iidInterfaceID, ppObject);
}

static const IDirectMusicLoader8Vtbl DirectMusicLoader_Loader_Vtbl = {
    IDirectMusicLoaderImpl_QueryInterface,
    IDirectMusicLoaderImpl_AddRef,
    IDirectMusicLoaderImpl_Release,
    IDirectMusicLoaderImpl_GetObject,
    IDirectMusicLoaderImpl_SetObject,
    IDirectMusicLoaderImpl_SetSearchDirectory,
    IDirectMusicLoaderImpl_ScanDirectory,
    IDirectMusicLoaderImpl_CacheObject,
    IDirectMusicLoaderImpl_ReleaseObject,
    IDirectMusicLoaderImpl_ClearCache,
    IDirectMusicLoaderImpl_EnableCache,
    IDirectMusicLoaderImpl_EnumObject,
    IDirectMusicLoaderImpl_CollectGarbage,
    IDirectMusicLoaderImpl_ReleaseObjectByUnknown,
    IDirectMusicLoaderImpl_LoadObjectFromFile
};

/* help function for DMUSIC_SetDefaultDLS */
static HRESULT DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]) {
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

/* for ClassFactory */
HRESULT WINAPI create_dmloader(REFIID lpcGUID, void **ppobj)
{
	IDirectMusicLoaderImpl *obj;
	DMUS_OBJECTDESC Desc;
        struct cache_entry *dls;
	struct list *pEntry;

        TRACE("(%s, %p)\n", debugstr_dmguid(lpcGUID), ppobj);
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoaderImpl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->IDirectMusicLoader8_iface.lpVtbl = &DirectMusicLoader_Loader_Vtbl;
	obj->ref = 0; /* Will be inited with QueryInterface */
        list_init(&obj->cache);
        /* Caching is enabled by default for all classes */
        obj->cache_class = ~0;

	/* set default DLS collection (via SetObject... so that loading via DMUS_OBJ_OBJECT is possible) */
	DM_STRUCT_INIT(&Desc);
	Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_OBJECT;
	Desc.guidClass = CLSID_DirectMusicCollection;
	Desc.guidObject = GUID_DefaultGMCollection;
	DMUSIC_GetDefaultGMPath (Desc.wszFileName);
	IDirectMusicLoader_SetObject(&obj->IDirectMusicLoader8_iface, &Desc);
	/* and now the workaroundTM for "invalid" default DLS; basically, 
	   my tests showed that if GUID chunk is present in default DLS 
	   collection, loader treats it as "invalid" and returns 
	   DMUS_E_LOADER_NOFILENAME for all requests for it; basically, we check 
	   if out input guidObject was overwritten */
        pEntry = list_head(&obj->cache);
        dls = LIST_ENTRY(pEntry, struct cache_entry, entry);
	if (!IsEqualGUID(&Desc.guidObject, &GUID_DefaultGMCollection)) {
                dls->bInvalidDefaultDLS = TRUE;
	}

        lock_module();

	return IDirectMusicLoader_QueryInterface(&obj->IDirectMusicLoader8_iface, lpcGUID, ppobj);
}
