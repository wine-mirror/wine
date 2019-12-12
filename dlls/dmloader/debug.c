/* Debug and Helper Functions
 *
 * Copyright (C) 2004 Rok Mandeljc
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

/* figures out whether given FOURCC is valid DirectMusic form ID */
BOOL IS_VALID_DMFORM (FOURCC chunkID) {
	if ((chunkID == DMUS_FOURCC_AUDIOPATH_FORM) || (chunkID == DMUS_FOURCC_BAND_FORM) || (chunkID == DMUS_FOURCC_CHORDMAP_FORM)
		|| (chunkID == DMUS_FOURCC_CONTAINER_FORM) || (chunkID == FOURCC_DLS) || (chunkID == DMUS_FOURCC_SCRIPT_FORM)
		|| (chunkID == DMUS_FOURCC_SEGMENT_FORM) || (chunkID == DMUS_FOURCC_STYLE_FORM) || (chunkID == DMUS_FOURCC_TOOLGRAPH_FORM)
		|| (chunkID == DMUS_FOURCC_TRACK_FORM) || (chunkID == mmioFOURCC('W','A','V','E')))  return TRUE;
	else return FALSE;
}

/* translate STREAM_SEEK flag to string */
const char *resolve_STREAM_SEEK (DWORD flag) {
	switch (flag) {
		case STREAM_SEEK_SET:
			return wine_dbg_sprintf ("STREAM_SEEK_SET");
		case STREAM_SEEK_CUR:
			return wine_dbg_sprintf ("STREAM_SEEK_CUR");
		case STREAM_SEEK_END:
			return wine_dbg_sprintf ("STREAM_SEEK_END");
		default:
			return wine_dbg_sprintf ("()");			
	}
}

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

/* dump DMUS_CONTAINED_OBJF flags */
static const char *debugstr_DMUS_CONTAINED_OBJF_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_CONTAINED_OBJF_KEEP)
	};
    return debugstr_flags(flagmask, flags, ARRAY_SIZE(flags));
}

const char *debugstr_DMUS_IO_CONTAINER_HEADER (LPDMUS_IO_CONTAINER_HEADER pHeader) {
	if (pHeader) {
		char buffer[1024], *ptr = buffer;

		ptr += sprintf(ptr, "DMUS_IO_CONTAINER_HEADER (%p):", pHeader);
		ptr += sprintf(ptr, "\n - dwFlags = %#x%s", pHeader->dwFlags,
                        pHeader->dwFlags & DMUS_CONTAINER_NOLOADS ? " (DMUS_CONTAINER_NOLOADS)" : "");

		return wine_dbg_sprintf("%s", buffer);
	} else {
		return wine_dbg_sprintf("(NULL)");
	}
}

const char *debugstr_DMUS_IO_CONTAINED_OBJECT_HEADER (LPDMUS_IO_CONTAINED_OBJECT_HEADER pHeader) {
	if (pHeader) {
		char buffer[1024], *ptr = buffer;
		
		ptr += sprintf(ptr, "DMUS_IO_CONTAINED_OBJECT_HEADER (%p):", pHeader);
		ptr += sprintf(ptr, "\n - guidClassID = %s", debugstr_dmguid(&pHeader->guidClassID));
		ptr += sprintf(ptr, "\n - dwFlags = %s", debugstr_DMUS_CONTAINED_OBJF_FLAGS (pHeader->dwFlags));
		ptr += sprintf(ptr, "\n - ckid = %s", debugstr_fourcc (pHeader->ckid));
		ptr += sprintf(ptr, "\n - fccType = %s", debugstr_fourcc (pHeader->fccType));

		return wine_dbg_sprintf("%s", buffer);
	} else {
		return wine_dbg_sprintf("(NULL)");
	}
}
