/* $Id: prototypes.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef _WINE_PROTOTYPES_H
#define _WINE_PROTOTYPES_H

#include <sys/types.h>

#include "neexe.h"
#include "msdos.h"
#include "windows.h"

#ifndef WINELIB

/* loader/resource.c */

extern HBITMAP ConvertCoreBitmap( HDC hdc, BITMAPCOREHEADER * image );
extern HBITMAP ConvertInfoBitmap( HDC hdc, BITMAPINFO * image );

/* loader/signal.c */

extern void init_wine_signals(void);
extern void wine_debug(int signal, int * regs);

/* loader/wine.c */

extern void myerror(const char *s);

extern char *GetFilenameFromInstance(unsigned short instance);
extern HINSTANCE LoadImage(char *modulename, int filetype, int change_dir);
extern int _WinMain(int argc, char **argv);
extern void InitializeLoadedDLLs();

/* misc/spy.c */

extern void SpyInit(void);

/* controls/widget.c */

extern BOOL WIDGETS_Init(void);

#endif /* WINELIB */
#endif /* _WINE_PROTOTYPES_H */
