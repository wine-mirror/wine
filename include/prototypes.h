/* $Id: prototypes.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef _WINE_PROTOTYPES_H
#define _WINE_PROTOTYPES_H

#include <sys/types.h>

#include "neexe.h"
#include "segmem.h"
#include "heap.h"
#include "msdos.h"
#include "windows.h"

#ifndef WINELIB

/* loader/ldtlib.c */

struct segment_descriptor *
make_sd(unsigned base, unsigned limit, int contents, int read_exec_only, int seg32, int inpgs);
int get_ldt(void *buffer);
int set_ldt_entry(int entry, unsigned long base, unsigned int limit,
	      int seg_32bit_flag, int contents, int read_only_flag,
	      int limit_in_pages_flag);

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

extern int KERNEL_LockSegment(int segment);
extern int KERNEL_UnlockSegment(int segment);
extern void KERNEL_InitTask(void);
extern int KERNEL_WaitEvent(int task);

/* misc/spy.c */

extern void SpyInit(void);

/* controls/widget.c */

extern BOOL WIDGETS_Init(void);

/* windows/dce.c */

extern void DCE_Init(void);

#endif /* WINELIB */
#endif /* _WINE_PROTOTYPES_H */
