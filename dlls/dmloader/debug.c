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

/* returns name of given error code */
const char *debugstr_dmreturn (DWORD code) {
	static const flag_info codes[] = {
		FE(S_OK),
		FE(S_FALSE),
		FE(DMUS_S_PARTIALLOAD),
		FE(DMUS_S_PARTIALDOWNLOAD),
		FE(DMUS_S_REQUEUE),
		FE(DMUS_S_FREE),
		FE(DMUS_S_END),
		FE(DMUS_S_STRING_TRUNCATED),
		FE(DMUS_S_LAST_TOOL),
		FE(DMUS_S_OVER_CHORD),
		FE(DMUS_S_UP_OCTAVE),
		FE(DMUS_S_DOWN_OCTAVE),
		FE(DMUS_S_NOBUFFERCONTROL),
		FE(DMUS_S_GARBAGE_COLLECTED),
		FE(E_NOTIMPL),
		FE(E_NOINTERFACE),
		FE(E_POINTER),
		FE(CLASS_E_NOAGGREGATION),
		FE(CLASS_E_CLASSNOTAVAILABLE),
		FE(REGDB_E_CLASSNOTREG),
		FE(E_OUTOFMEMORY),
		FE(E_FAIL),
		FE(E_INVALIDARG),
		FE(DMUS_E_DRIVER_FAILED),
		FE(DMUS_E_PORTS_OPEN),
		FE(DMUS_E_DEVICE_IN_USE),
		FE(DMUS_E_INSUFFICIENTBUFFER),
		FE(DMUS_E_BUFFERNOTSET),
		FE(DMUS_E_BUFFERNOTAVAILABLE),
		FE(DMUS_E_NOTADLSCOL),
		FE(DMUS_E_INVALIDOFFSET),
		FE(DMUS_E_ALREADY_LOADED),
		FE(DMUS_E_INVALIDPOS),
		FE(DMUS_E_INVALIDPATCH),
		FE(DMUS_E_CANNOTSEEK),
		FE(DMUS_E_CANNOTWRITE),
		FE(DMUS_E_CHUNKNOTFOUND),
		FE(DMUS_E_INVALID_DOWNLOADID),
		FE(DMUS_E_NOT_DOWNLOADED_TO_PORT),
		FE(DMUS_E_ALREADY_DOWNLOADED),
		FE(DMUS_E_UNKNOWN_PROPERTY),
		FE(DMUS_E_SET_UNSUPPORTED),
		FE(DMUS_E_GET_UNSUPPORTED),
		FE(DMUS_E_NOTMONO),
		FE(DMUS_E_BADARTICULATION),
		FE(DMUS_E_BADINSTRUMENT),
		FE(DMUS_E_BADWAVELINK),
		FE(DMUS_E_NOARTICULATION),
		FE(DMUS_E_NOTPCM),
		FE(DMUS_E_BADWAVE),
		FE(DMUS_E_BADOFFSETTABLE),
		FE(DMUS_E_UNKNOWNDOWNLOAD),
		FE(DMUS_E_NOSYNTHSINK),
		FE(DMUS_E_ALREADYOPEN),
		FE(DMUS_E_ALREADYCLOSED),
		FE(DMUS_E_SYNTHNOTCONFIGURED),
		FE(DMUS_E_SYNTHACTIVE),
		FE(DMUS_E_CANNOTREAD),
		FE(DMUS_E_DMUSIC_RELEASED),
		FE(DMUS_E_BUFFER_EMPTY),
		FE(DMUS_E_BUFFER_FULL),
		FE(DMUS_E_PORT_NOT_CAPTURE),
		FE(DMUS_E_PORT_NOT_RENDER),
		FE(DMUS_E_DSOUND_NOT_SET),
		FE(DMUS_E_ALREADY_ACTIVATED),
		FE(DMUS_E_INVALIDBUFFER),
		FE(DMUS_E_WAVEFORMATNOTSUPPORTED),
		FE(DMUS_E_SYNTHINACTIVE),
		FE(DMUS_E_DSOUND_ALREADY_SET),
		FE(DMUS_E_INVALID_EVENT),
		FE(DMUS_E_UNSUPPORTED_STREAM),
		FE(DMUS_E_ALREADY_INITED),
		FE(DMUS_E_INVALID_BAND),
		FE(DMUS_E_TRACK_HDR_NOT_FIRST_CK),
		FE(DMUS_E_TOOL_HDR_NOT_FIRST_CK),
		FE(DMUS_E_INVALID_TRACK_HDR),
		FE(DMUS_E_INVALID_TOOL_HDR),
		FE(DMUS_E_ALL_TOOLS_FAILED),
		FE(DMUS_E_ALL_TRACKS_FAILED),
		FE(DMUS_E_NOT_FOUND),
		FE(DMUS_E_NOT_INIT),
		FE(DMUS_E_TYPE_DISABLED),
		FE(DMUS_E_TYPE_UNSUPPORTED),
		FE(DMUS_E_TIME_PAST),
		FE(DMUS_E_TRACK_NOT_FOUND),
		FE(DMUS_E_TRACK_NO_CLOCKTIME_SUPPORT),
		FE(DMUS_E_NO_MASTER_CLOCK),
		FE(DMUS_E_LOADER_NOCLASSID),
		FE(DMUS_E_LOADER_BADPATH),
		FE(DMUS_E_LOADER_FAILEDOPEN),
		FE(DMUS_E_LOADER_FORMATNOTSUPPORTED),
		FE(DMUS_E_LOADER_FAILEDCREATE),
		FE(DMUS_E_LOADER_OBJECTNOTFOUND),
		FE(DMUS_E_LOADER_NOFILENAME),
		FE(DMUS_E_INVALIDFILE),
		FE(DMUS_E_ALREADY_EXISTS),
		FE(DMUS_E_OUT_OF_RANGE),
		FE(DMUS_E_SEGMENT_INIT_FAILED),
		FE(DMUS_E_ALREADY_SENT),
		FE(DMUS_E_CANNOT_FREE),
		FE(DMUS_E_CANNOT_OPEN_PORT),
		FE(DMUS_E_CANNOT_CONVERT),
		FE(DMUS_E_DESCEND_CHUNK_FAIL),
		FE(DMUS_E_NOT_LOADED),
		FE(DMUS_E_SCRIPT_LANGUAGE_INCOMPATIBLE),
		FE(DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE),
		FE(DMUS_E_SCRIPT_ERROR_IN_SCRIPT),
		FE(DMUS_E_SCRIPT_CANTLOAD_OLEAUT32),
		FE(DMUS_E_SCRIPT_LOADSCRIPT_ERROR),
		FE(DMUS_E_SCRIPT_INVALID_FILE),
		FE(DMUS_E_INVALID_SCRIPTTRACK),
		FE(DMUS_E_SCRIPT_VARIABLE_NOT_FOUND),
		FE(DMUS_E_SCRIPT_ROUTINE_NOT_FOUND),
		FE(DMUS_E_SCRIPT_CONTENT_READONLY),
		FE(DMUS_E_SCRIPT_NOT_A_REFERENCE),
		FE(DMUS_E_SCRIPT_VALUE_NOT_SUPPORTED),
		FE(DMUS_E_INVALID_SEGMENTTRIGGERTRACK),
		FE(DMUS_E_INVALID_LYRICSTRACK),
		FE(DMUS_E_INVALID_PARAMCONTROLTRACK),
		FE(DMUS_E_AUDIOVBSCRIPT_SYNTAXERROR),
		FE(DMUS_E_AUDIOVBSCRIPT_RUNTIMEERROR),
		FE(DMUS_E_AUDIOVBSCRIPT_OPERATIONFAILURE),
		FE(DMUS_E_AUDIOPATHS_NOT_VALID),
		FE(DMUS_E_AUDIOPATHS_IN_USE),
		FE(DMUS_E_NO_AUDIOPATH_CONFIG),
		FE(DMUS_E_AUDIOPATH_INACTIVE),
		FE(DMUS_E_AUDIOPATH_NOBUFFER),
		FE(DMUS_E_AUDIOPATH_NOPORT),
		FE(DMUS_E_NO_AUDIOPATH),
		FE(DMUS_E_INVALIDCHUNK),
		FE(DMUS_E_AUDIOPATH_NOGLOBALFXBUFFER),
		FE(DMUS_E_INVALID_CONTAINER_OBJECT)
	};
	
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(codes); i++) {
		if (code == codes[i].val)
			return codes[i].name;
	}
	
	/* if we didn't find it, return value */
	return wine_dbg_sprintf("0x%08X", code);
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

/* dump DMUS_CONTAINER flags */
static const char *debugstr_DMUS_CONTAINER_FLAGS (DWORD flagmask) {
    static const flag_info flags[] = {
	    FE(DMUS_CONTAINER_NOLOADS)
	};
    return debugstr_flags(flagmask, flags, ARRAY_SIZE(flags));
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
		ptr += sprintf(ptr, "\n - dwFlags = %s", debugstr_DMUS_CONTAINER_FLAGS(pHeader->dwFlags));
		
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
