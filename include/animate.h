/*
 * Animation class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_ANIMATE_H
#define __WINE_ANIMATE_H

#include "windef.h"
#include "winbase.h"
#include "vfw.h"

typedef struct tagANIMATE_INFO
{
   /* pointer to msvideo functions. it's easier to put them here.
    * to be correct, they should be defined on a per process basis, but
    * this would required a per process storage. We're using a per object
    * storage instead, which is not efficient on memory usage, but
    * will lead to less bugs in the future
    */
   HIC	  	WINAPI  (*fnICOpen)(DWORD, DWORD, UINT);
   LRESULT	WINAPI  (*fnICClose)(HIC);
   LRESULT	WINAPI  (*fnICSendMessage)(HIC, UINT, DWORD, DWORD);
   DWORD	WINAPIV (*fnICDecompress)(HIC,DWORD,LPBITMAPINFOHEADER,LPVOID,LPBITMAPINFOHEADER,LPVOID);

    HMMIO WINAPI (*fnmmioOpenA)(LPSTR,MMIOINFO*,DWORD);
    MMRESULT WINAPI (*fnmmioClose)(HMMIO,UINT);
    UINT  WINAPI (*fnmmioAscend)(HMMIO,MMCKINFO*,UINT);
    UINT  WINAPI (*fnmmioDescend)(HMMIO,MMCKINFO*,const MMCKINFO*,UINT);
    LONG  WINAPI (*fnmmioSeek)(HMMIO,LONG,INT);
    LONG  WINAPI (*fnmmioRead)(HMMIO,HPSTR,LONG);

   /* reference to input stream (file or resource) */
   HGLOBAL 		hRes;
   HMMIO			hMMio;	/* handle to mmio stream */
   HWND			hWnd;
   /* information on the loaded AVI file */
   MainAVIHeader	mah;
   AVIStreamHeader	ash;
   LPBITMAPINFOHEADER	inbih;
   LPDWORD		lpIndex;
   /* data for the decompressor */
   HIC			hic;
   LPBITMAPINFOHEADER	outbih;
   LPVOID		indata;
   LPVOID		outdata;
   /* data for the background mechanism */
   CRITICAL_SECTION	cs;
   HANDLE		hService;
   UINT			uTimer;
   /* data for playing the file */
   int			nFromFrame;
   int			nToFrame;
   int			nLoop;
   int			currFrame;
} ANIMATE_INFO;


extern VOID ANIMATE_Register (VOID);
extern VOID ANIMATE_Unregister (VOID);

#endif  /* __WINE_ANIMATE_H */
