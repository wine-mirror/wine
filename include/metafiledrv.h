/*
 * Metafile driver definitions
 */

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include "windows.h"
#include "gdi.h"

/* FIXME: SDK docs says these should be 1 and 2 */
#define METAFILE_MEMORY 0
#define METAFILE_DISK   1

/* Metafile driver physical DC */
typedef struct
{
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT32       nextHandle;   /* Next handle number */
} METAFILEDRV_PDEVICE;

#endif  /* __WINE_METAFILEDRV_H */
