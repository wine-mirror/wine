/*
 * X11 display driver definitions
 */

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "windows.h"

  /* X physical pen */
typedef struct
{
    int          style;
    int          pixel;
    int          width;
    char *       dashes;
    int          dash_len;
} X_PHYSPEN;

  /* X physical brush */
typedef struct
{
    int          style;
    int          fillStyle;
    int          pixel;
    Pixmap       pixmap;
} X_PHYSBRUSH;

  /* X physical font */
typedef struct
{
    XFontStruct * fstruct;
    TEXTMETRIC16  metrics;
} X_PHYSFONT;

  /* X physical device */
typedef struct
{
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    X_PHYSFONT    font;
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
} X11DRV_PDEVICE;

/* Wine driver X11 functions */

struct tagDC;

extern BOOL32 X11DRV_BitBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                             INT32 width, INT32 height, struct tagDC *dcSrc,
                             INT32 xSrc, INT32 ySrc, DWORD rop );
extern BOOL32 X11DRV_GetTextExtentPoint( struct tagDC *dc, LPCSTR str,
                                         INT32 count, LPSIZE32 size );
extern BOOL32 X11DRV_PatBlt( struct tagDC *dc, INT32 left, INT32 top,
                             INT32 width, INT32 height, DWORD rop );
extern VOID X11DRV_SetDeviceClipping(struct tagDC *dc);
extern BOOL32 X11DRV_StretchBlt( struct tagDC *dcDst, INT32 xDst, INT32 yDst,
                                 INT32 widthDst, INT32 heightDst,
                                 struct tagDC *dcSrc, INT32 xSrc, INT32 ySrc,
                                 INT32 widthSrc, INT32 heightSrc, DWORD rop );

#endif  /* __WINE_X11DRV_H */
