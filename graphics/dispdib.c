/*
 * DISPDIB.dll
 * 
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
 *
 */

#include <string.h>
#include "windows.h"
#include "dispdib.h"
#include "compobj.h"
#include "interfaces.h"
#include "ddraw.h"
#include "debug.h"

static int dispdib_multi = 0;
static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;

static WORD DISPDIB_Begin(WORD wFlags)
{
    unsigned Xres,Yres,Depth;

    switch(wFlags&DISPLAYDIB_MODE) {
        case DISPLAYDIB_MODE_DEFAULT:
            /* FIXME: is this supposed to autodetect? */
        case DISPLAYDIB_MODE_320x200x8:
            Xres=320; Yres=200; Depth=8; break;
        case DISPLAYDIB_MODE_320x240x8:
            Xres=320; Yres=240; Depth=8; break;
        default:
            return DISPLAYDIB_NOTSUPPORTED;
    }
    if (!lpddraw) {
        DirectDrawCreate(NULL,&lpddraw,NULL);
        if (!lpddraw) {
            ERR(ddraw,"DirectDraw is not available\n");
            return DISPLAYDIB_NOTSUPPORTED;
        }
        if (lpddraw->lpvtbl->fnSetDisplayMode(lpddraw,Xres,Yres,Depth)) {
            ERR(ddraw,"DirectDraw does not support requested display mode\n");
            lpddraw->lpvtbl->fnRelease(lpddraw);
            return DISPLAYDIB_NOTSUPPORTED;
        }
        lpddraw->lpvtbl->fnCreatePalette(lpddraw,0,NULL,&lpddpal,NULL);
        memset(&sdesc,0,sizeof(sdesc));
        sdesc.dwSize=sizeof(sdesc);
        lpddraw->lpvtbl->fnCreateSurface(lpddraw,&sdesc,&lpddsurf,NULL);
    }
    return DISPLAYDIB_NOERROR;
}

static void DISPDIB_End(void)
{
    if (lpddraw) {
        lpddsurf->lpvtbl->fnRelease(lpddsurf);
        lpddraw->lpvtbl->fnRelease(lpddraw);
        lpddraw=NULL;
    }
}

static void DISPDIB_Palette(LPBITMAPINFO lpbi)
{
    PALETTEENTRY pal[256];
    int c;

    for (c=0; c<256; c++) {
        pal[c].peRed  =lpbi->bmiColors[c].rgbRed;
        pal[c].peGreen=lpbi->bmiColors[c].rgbGreen;
        pal[c].peBlue =lpbi->bmiColors[c].rgbBlue;
        pal[c].peFlags=0;
    }
    lpddpal->lpvtbl->fnSetEntries(lpddpal,0,0,256,pal);
    lpddsurf->lpvtbl->fnSetPalette(lpddsurf,lpddpal);
}

static void DISPDIB_Show(LPBITMAPINFOHEADER lpbi,LPSTR lpBits,WORD uFlags)
{
    int Xofs,Yofs,Width=lpbi->biWidth,Height=lpbi->biHeight,Delta;
    unsigned Pitch=(Width+3)&~3;
    LPSTR surf;

    if (lpddsurf->lpvtbl->fnLock(lpddsurf,NULL,&sdesc,0,0)) {
        ERR(ddraw,"could not lock surface!\n");
        return;
    }
    /* size in sdesc.dwHeight, sdesc.dwWidth, pitch in sdesc.lPitch, ptr in sdesc.y.lpSurface */

    Delta=(Height<0)*2-1;
    Height*=-Delta; Pitch*=Delta;

    if (uFlags&DISPLAYDIB_NOCENTER) {
        Xofs=0; Yofs=0;
    } else {
        Xofs=(sdesc.dwWidth-Width)/2;
        Yofs=(sdesc.dwHeight-Height)/2;
    }
    surf=(LPSTR)sdesc.y.lpSurface + (Yofs*sdesc.lPitch)+Xofs;
    if (Pitch<0) lpBits-=Pitch*(Height-1);
    for (; Height; Height--,lpBits+=Pitch,surf+=sdesc.lPitch) {
        memcpy(surf,lpBits,Width);
    }

    lpddsurf->lpvtbl->fnUnlock(lpddsurf,sdesc.y.lpSurface);
}

/*********************************************************************
 *	DisplayDib	(DISPDIB.1)
 *
 *  Disables GDI and takes over the VGA screen to show DIBs in full screen.
 *
 * FLAGS
 *
 *  DISPLAYDIB_NOPALETTE: don't change palette
 *  DISPLAYDIB_NOCENTER: don't center bitmap
 *  DISPLAYDIB_NOWAIT: don't wait (for keypress) before returning
 *  DISPLAYDIB_BEGIN: start of multiple calls (does not restore the screen)
 *  DISPLAYDIB_END: end of multiple calls (restores the screen)
 *  DISPLAYDIB_MODE_DEFAULT: default display mode
 *  DISPLAYDIB_MODE_320x200x8: Standard VGA 320x200 256 colors
 *  DISPLAYDIB_MODE_320x240x8: Tweaked VGA 320x240 256 colors
 *
 * RETURNS
 *
 *  DISPLAYDIB_NOERROR: success
 *  DISPLAYDIB_NOTSUPPORTED: function not supported
 *  DISPLAYDIB_INVALIDDIB: null or invalid DIB header
 *  DISPLAYDIB_INVALIDFORMAT: invalid DIB format
 *  DISPLAYDIB_INVALIDTASK: not called from current task
 *
 * BUGS
 *
 *  Waiting for keypresses is not implemented.
 */
WORD WINAPI DisplayDib(
		LPBITMAPINFO lpbi, /* DIB header with resolution and palette */
		LPSTR lpBits, /* Bitmap bits to show */
		WORD wFlags
	)
{
    WORD ret;

    if (wFlags&DISPLAYDIB_END) {
        if (dispdib_multi) DISPDIB_End();
        dispdib_multi = 0;
        return DISPLAYDIB_NOERROR;
    }
    if (!dispdib_multi) {
        ret=DISPDIB_Begin(wFlags);
        if (ret) return ret;
    }
    if (wFlags&DISPLAYDIB_BEGIN) dispdib_multi = 1;
    if (!(wFlags&DISPLAYDIB_NOPALETTE)) {
        DISPDIB_Palette(lpbi);
    }
    /* FIXME: not sure if it's valid to draw images in DISPLAYDIB_BEGIN, so... */
    if (lpBits) {
        DISPDIB_Show(&(lpbi->bmiHeader),lpBits,wFlags);
    }
    if (!(wFlags&DISPLAYDIB_NOWAIT)) {
        FIXME(ddraw,"wait not implemented\n");
    }
    if (!dispdib_multi) DISPDIB_End();
    return DISPLAYDIB_NOERROR;
}
