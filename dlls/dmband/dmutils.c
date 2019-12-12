/* Debug and Helper Functions
 *
 * Copyright (C) 2004 Rok Mandeljc
 * Copyright (C) 2004 Raphael Junqueira
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

#define COBJMACROS


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

#include "dmutils.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);
WINE_DECLARE_DEBUG_CHANNEL(dmfile);

/* generic flag-dumping function */
static const char* debugstr_flags (DWORD flags, const flag_info* names, size_t num_names){
	static char buffer[128] = "", *ptr = &buffer[0];
	unsigned int i;
	int size = sizeof(buffer);

	for (i=0; i < num_names; i++) {
		if ((flags & names[i].val)) {
			int cnt = snprintf(ptr, size, "%s ", names[i].name);
			if (cnt < 0 || cnt >= size) break;
			size -= cnt;
			ptr += cnt;
		}
	}

	ptr = &buffer[0];
	return ptr;
}

/* dump DMUS_OBJ flags */
static const char *debugstr_DMUS_OBJ_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_OBJ_OBJECT),
	    FE(DMUS_OBJ_CLASS),
	    FE(DMUS_OBJ_NAME),
	    FE(DMUS_OBJ_CATEGORY),
	    FE(DMUS_OBJ_FILENAME),
	    FE(DMUS_OBJ_FULLPATH),
	    FE(DMUS_OBJ_URL),
	    FE(DMUS_OBJ_VERSION),
	    FE(DMUS_OBJ_DATE),
	    FE(DMUS_OBJ_LOADED),
	    FE(DMUS_OBJ_MEMORY),
	    FE(DMUS_OBJ_STREAM)
	};
    return debugstr_flags(flagmask, flags, ARRAY_SIZE(flags));
}

/* month number into month name (for debugstr_filetime) */
static const char *debugstr_month (DWORD dwMonth) {
	switch (dwMonth) {
		case 1: return "January";
		case 2: return "February";
		case 3: return "March";
		case 4: return "April";
		case 5: return "May";
		case 6: return "June";
		case 7: return "July";
		case 8: return "August";
		case 9: return "September";
		case 10: return "October";
		case 11: return "November";
		case 12: return "December";
		default: return "Invalid";
	}
}

/* FILETIME struct to string conversion for debug messages */
static const char *debugstr_filetime (const FILETIME *time) {
	SYSTEMTIME sysTime;

	if (!time) return "'null'";

	FileTimeToSystemTime (time, &sysTime);

	return wine_dbg_sprintf ("\'%02i. %s %04i %02i:%02i:%02i\'",
		sysTime.wDay, debugstr_month(sysTime.wMonth), sysTime.wYear,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
}

/* DMUS_VERSION struct to string conversion for debug messages */
static const char *debugstr_dmversion (const DMUS_VERSION *version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		HIWORD(version->dwVersionMS),LOWORD(version->dwVersionMS),
		HIWORD(version->dwVersionLS), LOWORD(version->dwVersionLS));
}

HRESULT IDirectMusicUtils_IPersistStream_ParseDescGeneric (DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, LPDMUS_OBJECTDESC pDesc) {

  switch (pChunk->fccID) {
  case DMUS_FOURCC_GUID_CHUNK: {
    TRACE_(dmfile)(": GUID chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_OBJECT;
    IStream_Read (pStm, &pDesc->guidObject, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_DATE_CHUNK: {
    TRACE_(dmfile)(": file date chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_DATE;
    IStream_Read (pStm, &pDesc->ftDate, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_NAME_CHUNK: {
    TRACE_(dmfile)(": name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_NAME;
    IStream_Read (pStm, pDesc->wszName, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_FILE_CHUNK: {
    TRACE_(dmfile)(": file name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_FILENAME;
    IStream_Read (pStm, pDesc->wszFileName, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_VERSION_CHUNK: {
    TRACE_(dmfile)(": version chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_VERSION;
    IStream_Read (pStm, &pDesc->vVersion, pChunk->dwSize, NULL);
    break;
  }
  case DMUS_FOURCC_CATEGORY_CHUNK: {
    TRACE_(dmfile)(": category chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
    IStream_Read (pStm, pDesc->wszCategory, pChunk->dwSize, NULL);
    break;
  }
  default:
    /* not handled */
    return S_FALSE;
  }

  return S_OK;
}

HRESULT IDirectMusicUtils_IPersistStream_ParseUNFOGeneric (DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, LPDMUS_OBJECTDESC pDesc) {

  LARGE_INTEGER liMove; /* used when skipping chunks */

  /**
   * don't ask me why, but M$ puts INFO elements in UNFO list sometimes
   * (though strings seem to be valid unicode) 
   */
  switch (pChunk->fccID) {

  case mmioFOURCC('I','N','A','M'):
  case DMUS_FOURCC_UNAM_CHUNK: {
    TRACE_(dmfile)(": name chunk\n");
    pDesc->dwValidData |= DMUS_OBJ_NAME;
    IStream_Read (pStm, pDesc->wszName, pChunk->dwSize, NULL);
    TRACE_(dmfile)(" - wszName: %s\n", debugstr_w(pDesc->wszName));
    break;
  }

  case mmioFOURCC('I','A','R','T'):
  case DMUS_FOURCC_UART_CHUNK: {
    TRACE_(dmfile)(": artist chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','C','O','P'):
  case DMUS_FOURCC_UCOP_CHUNK: {
    TRACE_(dmfile)(": copyright chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','S','B','J'):
  case DMUS_FOURCC_USBJ_CHUNK: {
    TRACE_(dmfile)(": subject chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  case mmioFOURCC('I','C','M','T'):
  case DMUS_FOURCC_UCMT_CHUNK: {
    TRACE_(dmfile)(": comment chunk (ignored)\n");
    liMove.QuadPart = pChunk->dwSize;
    IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
    break;
  }
  default:
    /* not handled */
    return S_FALSE;
  }

  return S_OK;
}

HRESULT IDirectMusicUtils_IPersistStream_ParseReference (LPPERSISTSTREAM iface, DMUS_PRIVATE_CHUNK* pChunk, IStream* pStm, IDirectMusicObject** ppObject) {
  DMUS_PRIVATE_CHUNK Chunk;
  DWORD ListSize[3], ListCount[3];
  LARGE_INTEGER liMove; /* used when skipping chunks */
  HRESULT hr;

  DMUS_IO_REFERENCE ref;
  DMUS_OBJECTDESC ref_desc;

  memset(&ref, 0, sizeof(ref));
  memset(&ref_desc, 0, sizeof(ref_desc));

  if (pChunk->fccID != DMUS_FOURCC_REF_LIST) {
    ERR_(dmfile)(": %s chunk should be a REF list\n", debugstr_fourcc (pChunk->fccID));
    return E_FAIL;
  }

  ListSize[0] = pChunk->dwSize - sizeof(FOURCC);
  ListCount[0] = 0;

  do {
    IStream_Read (pStm, &Chunk, sizeof(FOURCC)+sizeof(DWORD), NULL);
    ListCount[0] += sizeof(FOURCC) + sizeof(DWORD) + Chunk.dwSize;
    TRACE_(dmfile)(": %s chunk (size = %d)", debugstr_fourcc (Chunk.fccID), Chunk.dwSize);

    hr = IDirectMusicUtils_IPersistStream_ParseDescGeneric(&Chunk, pStm, &ref_desc);
    if (FAILED(hr)) return hr;
    
    if (hr == S_FALSE) {
      switch (Chunk.fccID) { 
      case DMUS_FOURCC_REF_CHUNK: {
	TRACE_(dmfile)(": Reference chunk\n");
	if (Chunk.dwSize != sizeof(DMUS_IO_REFERENCE)) return E_FAIL;
	IStream_Read (pStm, &ref, sizeof(DMUS_IO_REFERENCE), NULL);
	TRACE_(dmfile)(" - guidClassID: %s\n", debugstr_dmguid(&ref.guidClassID));
	TRACE_(dmfile)(" - dwValidData: %u\n", ref.dwValidData);
	break;
      } 
      default: {
	TRACE_(dmfile)(": unknown chunk (irrelevant & skipping)\n");
	liMove.QuadPart = Chunk.dwSize;
	IStream_Seek (pStm, liMove, STREAM_SEEK_CUR, NULL);
	break;						
      }
      }
    }
    TRACE_(dmfile)(": ListCount[0] = %d < ListSize[0] = %d\n", ListCount[0], ListSize[0]);
  } while (ListCount[0] < ListSize[0]);

  ref_desc.dwValidData |= DMUS_OBJ_CLASS;
  ref_desc.guidClass = ref.guidClassID;

  TRACE_(dmfile)("** DM Reference Begin of Load ***\n");
  TRACE_(dmfile)("With Desc:\n");
  debug_DMUS_OBJECTDESC(&ref_desc);

  {
    LPDIRECTMUSICGETLOADER pGetLoader = NULL;
    LPDIRECTMUSICLOADER pLoader = NULL;

    IStream_QueryInterface (pStm, &IID_IDirectMusicGetLoader, (LPVOID*)&pGetLoader);
    IDirectMusicGetLoader_GetLoader (pGetLoader, &pLoader);
    IDirectMusicGetLoader_Release (pGetLoader);
  
    hr = IDirectMusicLoader_GetObject (pLoader, &ref_desc, &IID_IDirectMusicObject, (LPVOID*)ppObject);
    IDirectMusicLoader_Release (pLoader); /* release loader */
  }
  TRACE_(dmfile)("** DM Reference End of Load ***\n");

  return hr;
}

void debug_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		TRACE("DMUS_OBJECTDESC (%p):\n", pDesc);
                TRACE(" - dwSize = %d\n", pDesc->dwSize);
                TRACE(" - dwValidData = %s\n", debugstr_DMUS_OBJ_FLAGS (pDesc->dwValidData));
                if (pDesc->dwValidData & DMUS_OBJ_NAME)     TRACE(" - wszName = %s\n", debugstr_w(pDesc->wszName));
                if (pDesc->dwValidData & DMUS_OBJ_CLASS)    TRACE(" - guidClass = %s\n", debugstr_dmguid(&pDesc->guidClass));
                if (pDesc->dwValidData & DMUS_OBJ_OBJECT)   TRACE(" - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
                if (pDesc->dwValidData & DMUS_OBJ_DATE)     TRACE(" - ftDate = %s\n", debugstr_filetime(&pDesc->ftDate));
                if (pDesc->dwValidData & DMUS_OBJ_VERSION)  TRACE(" - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
                if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) TRACE(" - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
                if (pDesc->dwValidData & DMUS_OBJ_FILENAME) TRACE(" - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
                if (pDesc->dwValidData & DMUS_OBJ_MEMORY)   TRACE(" - llMemLength = 0x%s - pbMemData = %p\n",
		                                                wine_dbgstr_longlong(pDesc->llMemLength), pDesc->pbMemData);
                if (pDesc->dwValidData & DMUS_OBJ_STREAM)   TRACE(" - pStream = %p\n", pDesc->pStream);
	} else {
		TRACE("(NULL)\n");
	}
}
