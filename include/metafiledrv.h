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


/* Metafile driver functions */

extern BOOL32 MFDRV_BitBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                            INT32 width, INT32 height, struct tagDC *dcSrc,
                            INT32 xSrc, INT32 ySrc, DWORD rop );
extern BOOL32 MFDRV_PatBlt( struct tagDC *dc, INT32 left, INT32 top,
                            INT32 width, INT32 height, DWORD rop );
extern BOOL32 MFDRV_StretchBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                                INT32 widthDst, INT32 heightDst,
                                struct tagDC *dcSrc, INT32 xSrc, INT32 ySrc,
                                INT32 widthSrc, INT32 heightSrc, DWORD rop );

#endif  /* __WINE_METAFILEDRV_H */
