/* DirectMusic Wave Private Include
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

#ifndef __WINE_DSWAVE_PRIVATE_H
#define __WINE_DSWAVE_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "winreg.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicWaveImpl IDirectMusicWaveImpl;


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IUnknown)               DirectMusicWave_Unknown_Vtbl;
extern ICOM_VTABLE(IDirectMusicObject)     DirectMusicWave_Object_Vtbl;
extern ICOM_VTABLE(IPersistStream)         DirectMusicWave_PersistStream_Vtbl;


/*****************************************************************************
 * ClassFactory
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicWaveImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter);


/*****************************************************************************
 * IDirectMusicWaveImpl implementation structure
 */
struct IDirectMusicWaveImpl {
  /* IUnknown fields */
  ICOM_VTABLE(IUnknown) *UnknownVtbl;
  ICOM_VTABLE(IDirectMusicObject) *ObjectVtbl;
  ICOM_VTABLE(IPersistStream) *PersistStreamVtbl;
  DWORD          ref;

  /* IDirectMusicWaveImpl fields */
  LPDMUS_OBJECTDESC pDesc;

};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IUnknown_QueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveImpl_IUnknown_AddRef (LPUNKNOWN iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IUnknown_Release (LPUNKNOWN iface);
/* IDirectMusicObject: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_QueryInterface (LPDIRECTMUSICOBJECT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicObject_AddRef (LPDIRECTMUSICOBJECT iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IDirectMusicObject_Release (LPDIRECTMUSICOBJECT iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_GetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_SetDescriptor (LPDIRECTMUSICOBJECT iface, LPDMUS_OBJECTDESC pDesc);
extern HRESULT WINAPI IDirectMusicWaveImpl_IDirectMusicObject_ParseDescriptor (LPDIRECTMUSICOBJECT iface, LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);
/* IPersistStream: */
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_QueryInterface (LPPERSISTSTREAM iface, REFIID riid, void** ppvObject);
extern ULONG WINAPI   IDirectMusicWaveImpl_IPersistStream_AddRef (LPPERSISTSTREAM iface);
extern ULONG WINAPI   IDirectMusicWaveImpl_IPersistStream_Release (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_GetClassID (LPPERSISTSTREAM iface, CLSID* pClassID);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_IsDirty (LPPERSISTSTREAM iface);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_Load (LPPERSISTSTREAM iface, IStream* pStm);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_Save (LPPERSISTSTREAM iface, IStream* pStm, BOOL fClearDirty);
extern HRESULT WINAPI IDirectMusicWaveImpl_IPersistStream_GetSizeMax (LPPERSISTSTREAM iface, ULARGE_INTEGER* pcbSize);


/*****************************************************************************
 * Misc.
 */
/* for simpler reading */
typedef struct _DMUS_PRIVATE_CHUNK {
	FOURCC fccID; /* FOURCC ID of the chunk */
	DWORD dwSize; /* size of the chunk */
} DMUS_PRIVATE_CHUNK, *LPDMUS_PRIVATE_CHUNK;

/* check whether the given DWORD is even (return 0) or odd (return 1) */
static inline int even_or_odd (DWORD number) {
	return (number & 0x1); /* basically, check if bit 0 is set ;) */
}

/* FOURCC to string conversion for debug messages */
static inline const char *debugstr_fourcc (DWORD fourcc) {
    if (!fourcc) return "'null'";
    return wine_dbg_sprintf ("\'%c%c%c%c\'",
		(char)(fourcc), (char)(fourcc >> 8),
        (char)(fourcc >> 16), (char)(fourcc >> 24));
}

/* DMUS_VERSION struct to string conversion for debug messages */
static inline const char *debugstr_dmversion (LPDMUS_VERSION version) {
	if (!version) return "'null'";
	return wine_dbg_sprintf ("\'%i,%i,%i,%i\'",
		(int)((version->dwVersionMS && 0xFFFF0000) >> 8), (int)(version->dwVersionMS && 0x0000FFFF), 
		(int)((version->dwVersionLS && 0xFFFF0000) >> 8), (int)(version->dwVersionLS && 0x0000FFFF));
}

/* used for initialising structs (primarily for DMUS_OBJECTDESC) */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)


/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }
#define DMUSIC_dump_flags(flags,names,num_names) DMUSIC_dump_flags_(flags, names, num_names, 1)

/* generic dump function */
static inline void DMUSIC_dump_flags_ (DWORD flags, const flag_info* names, size_t num_names, int newline) {
	unsigned int i;
	
	for (i=0; i < num_names; i++) {
		if ((flags & names[i].val) ||      /* standard flag value */
		((!flags) && (!names[i].val))) /* zero value only */
	    	DPRINTF("%s ", names[i].name);
	}
	
    if (newline) DPRINTF("\n");
}

static inline void DMUSIC_dump_DMUS_OBJ_FLAGS (DWORD flagmask) {
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
    DMUSIC_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

static inline void DMUSIC_dump_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc) {
	if (pDesc) {
		DPRINTF("DMUS_OBJECTDESC (%p)\n", pDesc);
		DPRINTF("  - dwSize = %ld\n", pDesc->dwSize);
		DPRINTF("  - dwValidData = ");
		DMUSIC_dump_DMUS_OBJ_FLAGS (pDesc->dwValidData);
		if (pDesc->dwValidData & DMUS_OBJ_CLASS) DPRINTF("  - guidClass = %s\n", debugstr_guid(&pDesc->guidClass));
		if (pDesc->dwValidData & DMUS_OBJ_OBJECT) DPRINTF("  - guidObject = %s\n", debugstr_guid(&pDesc->guidObject));
		if (pDesc->dwValidData & DMUS_OBJ_DATE) DPRINTF("  - ftDate = FIXME\n");
		if (pDesc->dwValidData & DMUS_OBJ_VERSION) DPRINTF("  - vVersion = %s\n", debugstr_dmversion(&pDesc->vVersion));
		if (pDesc->dwValidData & DMUS_OBJ_NAME) DPRINTF("  - wszName = %s\n", debugstr_w(pDesc->wszName));
		if (pDesc->dwValidData & DMUS_OBJ_CATEGORY) DPRINTF("  - wszCategory = %s\n", debugstr_w(pDesc->wszCategory));
		if (pDesc->dwValidData & DMUS_OBJ_FILENAME) DPRINTF("  - wszFileName = %s\n", debugstr_w(pDesc->wszFileName));
		if (pDesc->dwValidData & DMUS_OBJ_MEMORY) DPRINTF("  - llMemLength = %lli\n  - pbMemData = %p\n", pDesc->llMemLength, pDesc->pbMemData);
		if (pDesc->dwValidData & DMUS_OBJ_STREAM) DPRINTF("  - pStream = %p\n", pDesc->pStream);		
	} else {
		DPRINTF("(NULL)\n");
	}
}

#endif	/* __WINE_DSWAVE_PRIVATE_H */
