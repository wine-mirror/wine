/* IDirectMusicLoader8 Implementation
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

#include "dmloader_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmloader);

HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]);

/* IDirectMusicLoader8 IUnknown part: */
HRESULT WINAPI IDirectMusicLoader8Impl_QueryInterface (LPDIRECTMUSICLOADER8 iface, REFIID riid, LPVOID *ppobj) {
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

ULONG WINAPI IDirectMusicLoader8Impl_AddRef (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	TRACE("(%p) : AddRef from %ld\n", This, This->ref);
	return ++(This->ref);
}

ULONG WINAPI IDirectMusicLoader8Impl_Release (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	ULONG ref = --This->ref;
	TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
	if (ref == 0) {
		HeapFree(GetProcessHeap(), 0, This);
	}
	return ref;
}

/* IDirectMusicLoader8 IDirectMusicLoader part: */
HRESULT WINAPI IDirectMusicLoader8Impl_GetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc, REFIID riid, LPVOID* ppv) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	HRESULT result = 0;
	struct list *listEntry;
	LPDMUS_PRIVATE_ALIAS_ENTRY aliasEntry;
	DMUS_PRIVATE_CACHE_ENTRY *cacheEntry;
	DMUS_OBJECTDESC CacheDesc;
	IDirectMusicObject* pObject;
	LPDMUS_PRIVATE_CACHE_ENTRY newEntry;

	TRACE("(%p, %p, %s, %p): pDesc:\n", This, pDesc, debugstr_guid(riid), ppv);
	if (TRACE_ON(dmloader))
		DMUSIC_dump_DMUS_OBJECTDESC(pDesc);
	
	/* if I understand correctly, SetObject makes sort of aliases for entries in cache;
		therefore I created alias list, which is similiar to cache list, and is used as resolver
		(it maps let's say GUID to filename) */
	TRACE(": looking for alias\n");
	LIST_FOR_EACH (listEntry, &This->AliasList) {
		aliasEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_ALIAS_ENTRY, entry);
		/* for the time being, we support only GUID/name mapping */
		if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_OBJECT) && (pDesc->dwValidData & DMUS_OBJ_OBJECT)
			&& IsEqualGUID (&aliasEntry->pDesc->guidObject, &pDesc->guidObject)) {
			TRACE(": found alias by GUID (%s)... mapping:\n", debugstr_guid(&aliasEntry->pDesc->guidObject));
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_FILENAME) && !(pDesc->dwValidData & DMUS_OBJ_FILENAME)) {
				TRACE(":     - to filename (%s)\n", debugstr_w(aliasEntry->pDesc->wszFileName));
				pDesc->dwValidData |= DMUS_OBJ_FILENAME;
				pDesc->dwValidData |= (aliasEntry->pDesc->dwValidData & DMUS_OBJ_FULLPATH);
				strncpyW (pDesc->wszFileName, aliasEntry->pDesc->wszFileName, DMUS_MAX_FILENAME);
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_NAME) && !(pDesc->dwValidData & DMUS_OBJ_NAME)) {
				TRACE(":     - to name (%s)\n", debugstr_w(aliasEntry->pDesc->wszName));
				pDesc->dwValidData |= DMUS_OBJ_NAME;
				strncpyW (pDesc->wszName, aliasEntry->pDesc->wszName, DMUS_MAX_NAME);
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_MEMORY) && !(pDesc->dwValidData & DMUS_OBJ_MEMORY)) {
				TRACE(":     - to memory location\n");
				pDesc->dwValidData |= DMUS_OBJ_MEMORY;
				/* FIXME: is this correct? */
				pDesc->pbMemData = aliasEntry->pDesc->pbMemData;
				pDesc->llMemLength = aliasEntry->pDesc->llMemLength;
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_STREAM) && !(pDesc->dwValidData & DMUS_OBJ_STREAM)) {
				TRACE(":     - to stream\n");
				pDesc->dwValidData |= DMUS_OBJ_STREAM;
				IStream_Clone (aliasEntry->pDesc->pStream, &pDesc->pStream);	
			}					
		}
		else if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_NAME) && (pDesc->dwValidData & DMUS_OBJ_NAME)	
			&& !strncmpW (aliasEntry->pDesc->wszName, pDesc->wszName, DMUS_MAX_NAME)) {
			TRACE(": found alias by name (%s)... mapping:\n", debugstr_w(aliasEntry->pDesc->wszName));	
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_FILENAME) && !(pDesc->dwValidData & DMUS_OBJ_FILENAME)) {
				TRACE(":     - to filename (%s)\n", debugstr_w(aliasEntry->pDesc->wszFileName));
				pDesc->dwValidData |= DMUS_OBJ_FILENAME;
				pDesc->dwValidData |= (aliasEntry->pDesc->dwValidData & DMUS_OBJ_FULLPATH);
				strncpyW (pDesc->wszFileName, aliasEntry->pDesc->wszFileName, DMUS_MAX_FILENAME);
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_OBJECT) && !(pDesc->dwValidData & DMUS_OBJ_OBJECT)) {
				TRACE(":     - to object GUID (%s)\n", debugstr_guid(&aliasEntry->pDesc->guidObject));
				pDesc->dwValidData |= DMUS_OBJ_OBJECT;
				memcpy (&pDesc->guidObject, &aliasEntry->pDesc->guidObject, sizeof(GUID));
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_MEMORY) && !(pDesc->dwValidData & DMUS_OBJ_MEMORY)) {
				TRACE(":     - to memory location\n");
				pDesc->dwValidData |= DMUS_OBJ_MEMORY;
				/* FIXME: is this correct? */
				pDesc->pbMemData = aliasEntry->pDesc->pbMemData;
				pDesc->llMemLength = aliasEntry->pDesc->llMemLength;
			}
			if ((aliasEntry->pDesc->dwValidData & DMUS_OBJ_STREAM) && !(pDesc->dwValidData & DMUS_OBJ_STREAM)) {
				TRACE(":     - to stream\n");
				pDesc->dwValidData |= DMUS_OBJ_STREAM;
				IStream_Clone (aliasEntry->pDesc->pStream, &pDesc->pStream);	
			}				
		}
		/*else FIXME(": implement other types of mapping\n"); */
	}
	
	/* iterate through cache and check whether object has already been loaded */
	TRACE(": looking up cache...\n");
	DM_STRUCT_INIT(&CacheDesc);
	LIST_FOR_EACH (listEntry, &This->CacheList) {
		cacheEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_CACHE_ENTRY, entry);
		/* first check whether cached object is "faulty" default dls collection;
		 *  I don't think it's recongised by object descriptor, since it contains no
		 *  data; it's not very elegant way, but it works :)
		 */
		if (cacheEntry->bIsFaultyDLS == TRUE) {
			if ((pDesc->dwValidData & DMUS_OBJ_OBJECT) && IsEqualGUID (&GUID_DefaultGMCollection, &pDesc->guidObject)) {
				TRACE(": found faulty default DLS collection... enabling M$ compliant behaviour\n");
				return DMUS_E_LOADER_NOFILENAME;			
			}
		}
		/* I think it shouldn't happen that pObject is NULL, but better be safe */
		if (cacheEntry->pObject) {
			DM_STRUCT_INIT(&CacheDesc); /* prepare desc for reuse */
			IDirectMusicObject_GetDescriptor (cacheEntry->pObject, &CacheDesc);
			/* according to MSDN, search order is:
				   1. DMUS_OBJ_OBJECT
				   2. DMUS_OBJ_MEMORY (FIXME)
				   3. DMUS_OBJ_FILENAME & DMUS_OBJ_FULLPATH
				   4. DMUS_OBJ_NAME & DMUS_OBJ_CATEGORY
				   5. DMUS_OBJ_NAME
				   6. DMUS_OBJ_FILENAME */
			if ((pDesc->dwValidData & DMUS_OBJ_OBJECT) && (CacheDesc.dwValidData & DMUS_OBJ_OBJECT)
				&& IsEqualGUID (&pDesc->guidObject, &CacheDesc.guidObject)) {
					TRACE(": found it by object GUID\n");
					return IDirectMusicObject_QueryInterface (cacheEntry->pObject, riid, ppv);
			}
			if ((pDesc->dwValidData & DMUS_OBJ_MEMORY) && (CacheDesc.dwValidData & DMUS_OBJ_MEMORY)) {
					FIXME(": DMUS_OBJ_MEMORY not supported yet\n");
			}
			if ((pDesc->dwValidData & DMUS_OBJ_FILENAME) && (pDesc->dwValidData & DMUS_OBJ_FULLPATH)
				&& (CacheDesc.dwValidData & DMUS_OBJ_FILENAME) && (CacheDesc.dwValidData & DMUS_OBJ_FULLPATH)
				&& !strncmpW (pDesc->wszFileName, CacheDesc.wszFileName, DMUS_MAX_FILENAME)) {
					TRACE(": found it by fullpath filename\n");
					return IDirectMusicObject_QueryInterface (cacheEntry->pObject, riid, ppv);
			}
			if ((pDesc->dwValidData & DMUS_OBJ_NAME) && (pDesc->dwValidData & DMUS_OBJ_CATEGORY)
				&& (CacheDesc.dwValidData & DMUS_OBJ_NAME) && (CacheDesc.dwValidData & DMUS_OBJ_CATEGORY)
				&& !strncmpW (pDesc->wszName, CacheDesc.wszName, DMUS_MAX_NAME)
				&& !strncmpW (pDesc->wszCategory, CacheDesc.wszCategory, DMUS_MAX_CATEGORY)) {
					TRACE(": found it by name and category\n");
					return IDirectMusicObject_QueryInterface (cacheEntry->pObject, riid, ppv);
			}
			if ((pDesc->dwValidData & DMUS_OBJ_NAME) && (CacheDesc.dwValidData & DMUS_OBJ_NAME)
				&& !strncmpW (pDesc->wszName, CacheDesc.wszName, DMUS_MAX_NAME)) {
					TRACE(": found it by name\n");
					return IDirectMusicObject_QueryInterface (cacheEntry->pObject, riid, ppv);
			}
			if ((pDesc->dwValidData & DMUS_OBJ_FILENAME) && (CacheDesc.dwValidData & DMUS_OBJ_FILENAME)
				&& !strncmpW (pDesc->wszFileName, CacheDesc.wszFileName, DMUS_MAX_FILENAME)) {
					TRACE(": found it by filename\n");
					return IDirectMusicObject_QueryInterface (cacheEntry->pObject, riid, ppv);
			}
		}
	}
	
	/* object doesn't exist in cache... guess we'll have to load it */
	TRACE(": object does not exist in cache\n");
	
	/* sometimes it happens that guidClass is missing */
	if (!(pDesc->dwValidData & DMUS_OBJ_CLASS)) {
	  ERR(": guidClass not valid but needed\n");
	  *ppv = NULL;
	  return DMUS_E_LOADER_NOCLASSID;
	}

	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* load object from file */
		/* generate filename; if it's full path, don't add search 
		   directory path, otherwise do */
		WCHAR wzFileName[MAX_PATH];
		DMUS_OBJECTDESC GotDesc;
		LPSTREAM pStream;
		IPersistStream* pPersistStream = NULL;

		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW(wzFileName, pDesc->wszFileName);
		} else {
			WCHAR *p;
			lstrcpyW(wzFileName, This->wzSearchPath);
			p = wzFileName + lstrlenW(wzFileName);
			if (p > wzFileName && p[-1] != '\\') *p++ = '\\';
			strcpyW(p, pDesc->wszFileName);
		}
		TRACE(": loading from file (%s)\n", debugstr_w(wzFileName));
		/* create stream and associate it with dls collection file */			
		result = DMUSIC_CreateLoaderStream ((LPVOID*)&pStream);
		if (FAILED(result)) {
				ERR(": could not create loader stream\n");
			return result;
		}
		result = ILoaderStream_Attach (pStream, wzFileName, (LPDIRECTMUSICLOADER)iface);
		if (FAILED(result)) {
			ERR(": could not attach stream to file\n");			
			return result;
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
		if (FAILED(result)) {
			ERR(": failed to load object\n");
			return result;
		}
		/* get descriptor */
		DM_STRUCT_INIT(&GotDesc);
		result = IDirectMusicObject_GetDescriptor (pObject, &GotDesc);
		if (FAILED(result)) {
			ERR(": failed to get descriptor\n");
			return result;
		}
		/* now set the "missing" info (check comment at "Loading default DLS collection") */
		GotDesc.dwValidData |= (DMUS_OBJ_FILENAME | DMUS_OBJ_LOADED); /* this time only these are missing */
		strncpyW (GotDesc.wszFileName, pDesc->wszFileName, DMUS_MAX_FILENAME); /* set wszFileName, even if futile */
		/* set descriptor */			
		IDirectMusicObject_SetDescriptor (pObject, &GotDesc);		
		/* release all loading related stuff */
		IStream_Release (pStream);
		IPersistStream_Release (pPersistStream);
	}
	else if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		LPSTREAM pClonedStream = NULL;
		IPersistStream* pPersistStream = NULL;
		DMUS_OBJECTDESC GotDesc;
		/* load object from stream */
		TRACE(": loading from stream\n");
		/* clone stream, given in pDesc */
		result = IStream_Clone (pDesc->pStream, &pClonedStream);
		if (FAILED(result)) {
			ERR(": failed to clone stream\n");
			return result;
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
			ERR(": could not acquire IPersistStream\n");
			return result;
		}
		/* load */
		result = IPersistStream_Load (pPersistStream, pClonedStream);
		if (FAILED(result)) {
			ERR(": failed to load object\n");
			return result;
		}
		/* get descriptor */
		DM_STRUCT_INIT(&GotDesc);
		result = IDirectMusicObject_GetDescriptor (pObject, &GotDesc);
		if (FAILED(result)) {
			ERR(": failed to get descriptor\n");
			return result;
		}
		/* now set the "missing" info */
		GotDesc.dwValidData |= DMUS_OBJ_LOADED; /* only missing data with streams */
		/* set descriptor */			
		IDirectMusicObject_SetDescriptor (pObject, &GotDesc);		
		/* release all loading-related stuff */
		IPersistStream_Release (pPersistStream);
		IStream_Release (pClonedStream);
	}
	else if (pDesc->dwValidData & DMUS_OBJ_OBJECT) {
		/* load object by GUID */
		TRACE(": loading by GUID (only default DLS supported)\n");
		if (IsEqualGUID (&pDesc->guidObject, &GUID_DefaultGMCollection)) {
			/* Loading default DLS collection: *dirty* secret (TM)
            *  By mixing native and builtin loader and collection and 
			 *   various .dls files, I found out following undocumented 
			 *   behaviour:
			 *    - loader creates two instances of collection object
			 *    - it calls ParseDescriptor on first, then releases it
			 *    - then it checks returned descriptor; I'm not sure, but 
			 *       it seems that DMUS_OBJ_OBJECT is not present if DLS
			 *       collection is indeed *real* one (gm.dls)
            *    - if above mentioned flag is not set, it creates another 
			 *      instance and loads it; it also gets descriptor and adds
			 *      guidObject and wszFileName (even though this one cannot be
			 *      set on native collection, or so it seems)
            *    => it seems to be sort of check whether given 'default
			 *       DLS collection' is really one shipped with DX before
			 *       actually loading it
			 * -cheers, Rok
			 */			
			WCHAR wzFileName[DMUS_MAX_FILENAME];
			LPSTREAM pStream;
			LPSTREAM pProbeStream;
			IDirectMusicObject *pProbeObject;
			DMUS_OBJECTDESC ProbeDesc;
			DMUS_OBJECTDESC GotDesc;
                        IPersistStream *pPersistStream = NULL;

			/* get the path for default collection */
			TRACE(": getting default DLS collection path...\n");
			if (FAILED(DMUSIC_GetDefaultGMPath (wzFileName))) {
				ERR(": could not get default collection path\n");
				return E_FAIL;
			}
			/* create stream and associate it with dls collection file */
			TRACE(": creating stream...\n");
			result = DMUSIC_CreateLoaderStream ((LPVOID*) &pStream);
			if (FAILED(result)) {
				ERR(": could not create loader stream\n");
				return result;
			}
			TRACE(": attaching stream...\n");
			result = ILoaderStream_Attach (pStream, wzFileName, (LPDIRECTMUSICLOADER)iface);
			if (FAILED(result)) {
				ERR(": could not attach stream to file\n");
				return result;
			}
			/* now create a clone of stream for "probe" */
			TRACE(": cloning stream (for probing)...\n");
			result = IStream_Clone (pStream, &pProbeStream);
			if (FAILED(result)) {
				ERR(": could not clone stream\n");
				return result;
			}
			/* create object for "probing" */
			TRACE(": creating IDirectMusicObject (for probing)...\n");
			result = CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*) &pProbeObject);
			if (FAILED(result)) {
				ERR(": could not create object (for probing)\n");
				return result;
			}
			/* get descriptor from stream */
			TRACE(": parsing descriptor on probe stream...\n");
			DM_STRUCT_INIT(&ProbeDesc);
			result = IDirectMusicObject_ParseDescriptor (pProbeObject, pProbeStream, &ProbeDesc);
                        if (FAILED(result)) {
				ERR(": could not parse descriptor\n");
				return result;
			}
			/* release all probing-related stuff */
			TRACE(": releasing probing-related stuff...\n");
                        IStream_Release (pProbeStream);
			IDirectMusicObject_Release (pProbeObject);
			/* now, if it happens by any chance that dls collection isn't *the one* 
			 *  TODO: - check if the way below is the appropriate one
			 */
			if (ProbeDesc.dwValidData & DMUS_OBJ_OBJECT) {
				LPDMUS_PRIVATE_CACHE_ENTRY newEntry;
				WARN(": the default DLS collection is not the one shipped with DX\n");
				/* my tests show that we return pointer to something or NULL, depending on + how 
				 * input object was defined (therefore we probably don't return anything for object)
				 * and DMUS_E_LOADER_NOFILENAME as error code
				 * (I'd personally rather return DMUS_S_PARTIALLOAD, but I don't set rules)
				 */
				newEntry = (LPDMUS_PRIVATE_CACHE_ENTRY) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_CACHE_ENTRY));
				newEntry->pObject = NULL;
				newEntry->bIsFaultyDLS = TRUE; /* so that cache won't try to get descriptor */
				list_add_tail (&This->CacheList, &newEntry->entry);
				TRACE(": filled in cache entry\n");
				return DMUS_E_LOADER_NOFILENAME;
			}
			/* now the real loading... create object */
			TRACE(": creating IDirectMusicObject (for loading)\n");
			result = CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*) &pObject);
			if (FAILED(result)) {
				ERR(": could not create object (for loading)\n");
				return result;
			}
			/* acquire PersistStream interface */
			TRACE(": getting IPersistStream on object...\n");
			result = IDirectMusicObject_QueryInterface (pObject, &IID_IPersistStream, (LPVOID*) &pPersistStream);
			if (FAILED(result)) {
				ERR(": could not acquire IPersistStream\n");
				return result;
			}
			/* load */
			TRACE(": loading object..\n");
			result = IPersistStream_Load (pPersistStream, pStream);
			if (FAILED(result)) {
				ERR(": failed to load object\n");
				return result;
			}
			/* get descriptor */
			TRACE(": getting descriptor of loaded object...\n");
			DM_STRUCT_INIT(&GotDesc);
			result = IDirectMusicObject_GetDescriptor (pObject, &GotDesc);
			if (FAILED(result)) {
				ERR(": failed to get descriptor\n");
				return result;
			}
			/* now set the "missing" info */
			TRACE(": adding \"missing\" info...\n");
			GotDesc.dwValidData |= (DMUS_OBJ_OBJECT | DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH | DMUS_OBJ_LOADED);
			memcpy (&GotDesc.guidObject, &pDesc->guidObject, sizeof(GUID)); /* set guidObject */
			strncpyW (GotDesc.wszFileName, wzFileName, DMUS_MAX_FILENAME); /* set wszFileName, even if futile */
			/* set descriptor */
			TRACE(": setting descriptor\n");
			IDirectMusicObject_SetDescriptor (pObject, &GotDesc);
			/* release all loading related stuff */
			TRACE(": releasing all loading-related stuff\n");
            IStream_Release (pStream);
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
	if (pObject) {
		newEntry->pObject = pObject;
		newEntry->bIsFaultyDLS = FALSE;
	}
	list_add_tail (&This->CacheList, &newEntry->entry);
	TRACE(": filled in cache entry\n");

#if 0
	/* for debug purposes (e.g. to check if all files are cached) */
	TRACE("*** Loader's cache ***\n");
	int i = 0;
	LIST_FOR_EACH (listEntry, &This->CacheList) {
		i++;
		TRACE("Entry nr. %i:\n", i);
		cacheEntry = LIST_ENTRY(listEntry, DMUS_PRIVATE_CACHE_ENTRY, entry);
		if (cacheEntry->bIsFaultyDLS == FALSE) {
			DM_STRUCT_INIT(&CacheDesc); /* prepare desc for reuse */
			IDirectMusicObject_GetDescriptor (cacheEntry->pObject, &CacheDesc);
			DMUSIC_dump_DMUS_OBJECTDESC(&CacheDesc);
		} else {
			DPRINTF("faulty DLS collection\n");
		}
	}
#endif
	
	return IDirectMusicObject_QueryInterface (pObject, riid, ppv);
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetObject (LPDIRECTMUSICLOADER8 iface, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	DMUS_PRIVATE_ALIAS_ENTRY *newEntry;
	DMUS_OBJECTDESC Desc;
	
	TRACE("(%p, %p): pDesc:\n", This, pDesc);
	if (TRACE_ON(dmloader))
		DMUSIC_dump_DMUS_OBJECTDESC(pDesc);
	
	/* create stream and load additional info from it */
	if (pDesc->dwValidData & DMUS_OBJ_FILENAME) {
		/* generate filename; if it's full path, don't add search 
		   directory path, otherwise do */
		WCHAR wzFileName[MAX_PATH];
		LPSTREAM pStream;
		IDirectMusicObject* pObject;

		if (pDesc->dwValidData & DMUS_OBJ_FULLPATH) {
			lstrcpyW(wzFileName, pDesc->wszFileName);
		} else {
			WCHAR *p;
			lstrcpyW(wzFileName, This->wzSearchPath);
			p = wzFileName + lstrlenW(wzFileName);
			if (p > wzFileName && p[-1] != '\\') *p++ = '\\';
			strcpyW(p, pDesc->wszFileName);
		}
		/* create stream */
		DMUSIC_CreateLoaderStream ((LPVOID*) &pStream);
		/* attach stream */
		ILoaderStream_Attach (pStream, wzFileName, (LPDIRECTMUSICLOADER)iface);
		/* create object */
		CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);
		/* parse descriptor */
		DM_STRUCT_INIT(&Desc);
		IDirectMusicObject_ParseDescriptor (pObject, pStream, &Desc);
		/* release everything */
		IDirectMusicObject_Release (pObject);
		IStream_Release (pStream);
	}
	else if (pDesc->dwValidData & DMUS_OBJ_STREAM) {
		/* clone stream */
		LPSTREAM pStream = NULL;
		IDirectMusicObject* pObject;

		IStream_Clone (pDesc->pStream, &pStream);
		/* create object */
		CoCreateInstance (&pDesc->guidClass, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectMusicObject, (LPVOID*)&pObject);
		/* parse descriptor */
		DM_STRUCT_INIT(&Desc);
		IDirectMusicObject_ParseDescriptor (pObject, pStream, &Desc);
		/* release everything */
		IDirectMusicObject_Release (pObject);
		IStream_Release (pStream);
	}
	else {
		WARN(": no way to get additional info\n");
	}
	
	/* now set additional info... my tests show that existing fields should be overwritten */
	if (Desc.dwValidData & DMUS_OBJ_OBJECT)
		memcpy (&pDesc->guidObject, &Desc.guidObject, sizeof(Desc.guidObject));
	if (Desc.dwValidData & DMUS_OBJ_CLASS)
		memcpy (&pDesc->guidClass, &Desc.guidClass, sizeof(Desc.guidClass));		
	if (Desc.dwValidData & DMUS_OBJ_NAME)
		strncpyW (pDesc->wszName, Desc.wszName, DMUS_MAX_NAME);
	if (Desc.dwValidData & DMUS_OBJ_CATEGORY)
		strncpyW (pDesc->wszCategory, Desc.wszCategory, DMUS_MAX_CATEGORY);		
	if (Desc.dwValidData & DMUS_OBJ_FILENAME)
		strncpyW (pDesc->wszFileName, Desc.wszFileName, DMUS_MAX_FILENAME);		
	if (Desc.dwValidData & DMUS_OBJ_VERSION)
		memcpy (&pDesc->vVersion, &Desc.vVersion, sizeof(Desc.vVersion));				
	if (Desc.dwValidData & DMUS_OBJ_DATE)
		memcpy (&pDesc->ftDate, &Desc.ftDate, sizeof(Desc.ftDate));
	pDesc->dwValidData |= Desc.dwValidData; /* add new flags */
	
	/* add new entry */
	TRACE(": adding alias entry with following info:\n");
	if (TRACE_ON(dmloader))
		DMUSIC_dump_DMUS_OBJECTDESC(pDesc);
	newEntry = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_PRIVATE_ALIAS_ENTRY));
	newEntry->pDesc = HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof(DMUS_OBJECTDESC));
	memcpy (newEntry->pDesc, pDesc, sizeof(DMUS_OBJECTDESC));
	list_add_tail (&This->AliasList, &newEntry->entry);
	
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_SetSearchDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzPath, BOOL fClear) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	TRACE("(%p, %s, %s, %d)\n", This, debugstr_guid(rguidClass), debugstr_w(pwzPath), fClear);
	if (0 == strncmpW(This->wzSearchPath, pwzPath, MAX_PATH)) {
	  return S_FALSE;
	} 
	strncpyW(This->wzSearchPath, pwzPath, MAX_PATH);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ScanDirectory (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, WCHAR* pwzFileExtension, WCHAR* pwzScanFileName) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_guid(rguidClass), pwzFileExtension, pwzScanFileName);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_CacheObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %p): stub\n", This, pObject);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObject (LPDIRECTMUSICLOADER8 iface, IDirectMusicObject* pObject) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %p): stub\n", This, pObject);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_ClearCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %s): stub\n", This, debugstr_guid(rguidClass));
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnableCache (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, BOOL fEnable) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %s, %d): stub\n", This, debugstr_guid(rguidClass), fEnable);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_EnumObject (LPDIRECTMUSICLOADER8 iface, REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %s, %ld, %p): stub\n", This, debugstr_guid(rguidClass), dwIndex, pDesc);
	return S_OK;
}

/* IDirectMusicLoader8 Interface part follow: */
void WINAPI IDirectMusicLoader8Impl_CollectGarbage (LPDIRECTMUSICLOADER8 iface) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p): stub\n", This);
}

HRESULT WINAPI IDirectMusicLoader8Impl_ReleaseObjectByUnknown (LPDIRECTMUSICLOADER8 iface, IUnknown* pObject) {
	ICOM_THIS(IDirectMusicLoader8Impl,iface);
	FIXME("(%p, %p): stub\n", This, pObject);
	return S_OK;
}

HRESULT WINAPI IDirectMusicLoader8Impl_LoadObjectFromFile (LPDIRECTMUSICLOADER8 iface,  
							   REFGUID rguidClassID, 
							   REFIID iidInterfaceID, 
							   WCHAR* pwzFilePath, 
							   void** ppObject) {
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

ICOM_VTABLE(IDirectMusicLoader8) DirectMusicLoader8_Vtbl = {
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
HRESULT WINAPI DMUSIC_CreateDirectMusicLoaderImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicLoader8Impl *obj;

	TRACE("(%p,%p,%p)\n",lpcGUID, ppobj, pUnkOuter);
	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicLoader8Impl));
	if (NULL == obj) {
		*ppobj = (LPDIRECTMUSICLOADER8)NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicLoader8_Vtbl;
	obj->ref = 0; /* will be inited with QueryInterface */
	MultiByteToWideChar (CP_ACP, 0, ".\\", -1, obj->wzSearchPath, MAX_PATH);
	list_init (&obj->CacheList);
	list_init (&obj->AliasList);

	return IDirectMusicLoader8Impl_QueryInterface ((LPDIRECTMUSICLOADER8)obj, lpcGUID, ppobj);
}

/* help function for IDirectMusicLoader8Impl_GetObject */
HRESULT WINAPI DMUSIC_GetDefaultGMPath (WCHAR wszPath[MAX_PATH]) {
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
