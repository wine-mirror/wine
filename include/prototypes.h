/* $Id: prototypes.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef _WINE_PROTOTYPES_H
#define _WINE_PROTOTYPES_H

#include <sys/types.h>

#include "windows.h"

#ifndef WINELIB

/* loader/resource.c */

extern HBITMAP ConvertCoreBitmap( HDC hdc, BITMAPCOREHEADER * image );
extern HBITMAP ConvertInfoBitmap( HDC hdc, BITMAPINFO * image );

/* loader/signal.c */

extern void init_wine_signals(void);

/* loader/wine.c */

extern int _WinMain(int argc, char **argv);

/* misc/spy.c */

extern void SpyInit(void);

#endif /* WINELIB */
#endif /* _WINE_PROTOTYPES_H */
