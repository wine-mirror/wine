/* Debug and Helper Functions
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __WINE_DMLOADER_DEBUG_H
#define __WINE_DMLOADER_DEBUG_H
 
/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

typedef struct {
    const GUID *guid;
    const char* name;
} guid_info;

/* used for initialising structs */
#define DM_STRUCT_INIT(x) 				\
	do {								\
		memset((x), 0, sizeof(*(x)));	\
		(x)->dwSize = sizeof(*x);		\
	} while (0)

#define FE(x) { x, #x }	
#define GE(x) { &x, #x }

/* check whether the given DWORD is even (return 0) or odd (return 1) */
extern int even_or_odd (DWORD number);
/* check whether chunkID is valid dmobject form chunk */
extern BOOL IS_VALID_DMFORM (FOURCC chunkID);
/* translate STREAM_SEEK flag to string */
extern const char *resolve_STREAM_SEEK (DWORD flag);
/* FOURCC to string conversion for debug messages */
extern const char *debugstr_fourcc (DWORD fourcc);
/* DMUS_VERSION struct to string conversion for debug messages */
extern const char *debugstr_dmversion (const DMUS_VERSION *version);
/* FILETIME struct to string conversion for debug messages */
extern const char *debugstr_filetime (const FILETIME *time);
/* returns name of given GUID */
extern const char *debugstr_dmguid (const GUID *id);
/* returns name of given error code */
extern const char *debugstr_dmreturn (DWORD code);

/* dump whole DMUS_OBJECTDESC struct */
extern const char *debugstr_DMUS_OBJECTDESC (LPDMUS_OBJECTDESC pDesc);
extern const char *debugstr_DMUS_IO_CONTAINER_HEADER (LPDMUS_IO_CONTAINER_HEADER pHeader);
extern const char *debugstr_DMUS_IO_CONTAINED_OBJECT_HEADER (LPDMUS_IO_CONTAINED_OBJECT_HEADER pHeader);

#endif /* __WINE_DMLOADER_DEBUG_H */
