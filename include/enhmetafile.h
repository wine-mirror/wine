/*
 * Enhanced Metafile definitions
 *
 */

#ifndef __WINE_ENHMETAFILE_H
#define __WINE_ENHMETAFILE_H

#include "gdi.h"
#include "windef.h"
#include "wingdi.h"

  /* GDI32 enhanced metafile object */
typedef struct
{
    GDIOBJHDR      header;
    ENHMETAHEADER  *emh;
    HFILE          hFile;      /* File handle if EMF is disk-based */
    HANDLE         hMapping;   /* Mapping handle if EMF is disk-based */
} ENHMETAFILEOBJ;


extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, HFILE hFile,
					    HANDLE hMapping);


#endif   /* __WINE_ENHMETAFILE_H */

