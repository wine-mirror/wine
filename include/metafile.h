/*
 * Metafile definitions
 *
 * Copyright  David W. Metcalfe, 1994
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_METAFILE_H
#define __WINE_METAFILE_H

#include "gdi.h"
#include "windef.h"
#include "wingdi.h"

  /* GDI32 metafile object */
typedef struct
{
    GDIOBJHDR   header;
    METAHEADER  *mh;
} METAFILEOBJ;

#define META_EOF 0x0000

/* values of mtType in METAHEADER.  Note however that the disk image of a disk
   based metafile has mtType == 1 */
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh);
extern HMETAFILE16 MF_Create_HMETAFILE16(METAHEADER *mh);
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCSTR filename);

#endif   /* __WINE_METAFILE_H */

