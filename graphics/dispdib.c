/*
 * DISPDIB.dll
 * 
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
 *
 */

#include <string.h>
#include "miscemu.h"
#include "dispdib.h"
#include "vga.h"
#include "debug.h"

static int dispdib_multi = 0;

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
    /* more or less dummy calls to Death/Resurrection, for completeness */
    /* FIXME: what arguments should they get? */
    Death16(0);
    if (VGA_SetMode(Xres,Yres,Depth)) {
         Resurrection16(0,0,0,0,0,0,0);
         return DISPLAYDIB_NOTSUPPORTED;
    }
    return DISPLAYDIB_NOERROR;
}

static void DISPDIB_End(void)
{
    Resurrection16(0,0,0,0,0,0,0); /* FIXME: arguments */
    VGA_Exit();
}

static void DISPDIB_Palette(LPBITMAPINFO lpbi)
{
    VGA_SetQuadPalette(lpbi->bmiColors,0,256);
}

static void DISPDIB_Show(LPBITMAPINFOHEADER lpbi,LPSTR lpBits,WORD uFlags)
{
    int Xofs,Yofs,Width=lpbi->biWidth,Height=lpbi->biHeight,Delta;
    unsigned Pitch=(Width+3)&~3,sPitch,sWidth,sHeight;
    LPSTR surf = DOSMEM_MapDosToLinear(0xa0000);

    if (VGA_GetMode(&sHeight,&sWidth,NULL)) return;
    sPitch=320;

    Delta=(Height<0)*2-1;
    Height*=-Delta; Pitch*=Delta;

    if (uFlags&DISPLAYDIB_NOCENTER) {
        Xofs=0; Yofs=0;
    } else {
        Xofs=(sWidth-Width)/2;
        Yofs=(sHeight-Height)/2;
    }
    surf += (Yofs*sPitch)+Xofs;
    if (Pitch<0) lpBits-=Pitch*(Height-1);
    for (; Height; Height--,lpBits+=Pitch,surf+=sPitch) {
        memcpy(surf,lpBits,Width);
    }

    VGA_Poll(0);
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
