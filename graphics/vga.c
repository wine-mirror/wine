/*
 * VGA hardware emulation
 * 
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
 *
 */

#include <string.h>
#include "winbase.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "miscemu.h"
#include "vga.h"
#include "ddraw.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;
static WORD poll_timer;
static CRITICAL_SECTION vga_crit;
static int vga_polling,vga_refresh;

int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth)
{
    if (lpddraw) VGA_Exit();
    if (!lpddraw) {
        DirectDrawCreate(NULL,&lpddraw,NULL);
        if (!lpddraw) {
            ERR("DirectDraw is not available\n");
            return 1;
        }
        if (IDirectDraw_SetDisplayMode(lpddraw,Xres,Yres,Depth)) {
            ERR("DirectDraw does not support requested display mode\n");
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return 1;
        }
        IDirectDraw_CreatePalette(lpddraw,DDPCAPS_8BIT,NULL,&lpddpal,NULL);
        memset(&sdesc,0,sizeof(sdesc));
        sdesc.dwSize=sizeof(sdesc);
	sdesc.dwFlags = DDSD_CAPS;
	sdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw_CreateSurface(lpddraw,&sdesc,&lpddsurf,NULL)||(!lpddsurf)) {
            ERR("DirectDraw surface is not available\n");
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return 1;
        }
        vga_refresh=0;
        InitializeCriticalSection(&vga_crit);
        MakeCriticalSectionGlobal(&vga_crit);
        /* poll every 20ms (50fps should provide adequate responsiveness) */
        poll_timer = CreateSystemTimer( 20, VGA_Poll );
    }
    return 0;
}

int VGA_GetMode(unsigned*Height,unsigned*Width,unsigned*Depth)
{
    if (!lpddraw) return 1;
    if (!lpddsurf) return 1;
    if (Height) *Height=sdesc.dwHeight;
    if (Width) *Width=sdesc.dwWidth;
    if (Depth) *Depth=sdesc.ddpfPixelFormat.x.dwRGBBitCount;
    return 0;
}

void VGA_Exit(void)
{
    if (lpddraw) {
        SYSTEM_KillSystemTimer(poll_timer);
        DeleteCriticalSection(&vga_crit);
        IDirectDrawSurface_Release(lpddsurf);
        lpddsurf=NULL;
        IDirectDraw_Release(lpddraw);
        lpddraw=NULL;
    }
}

void VGA_SetPalette(PALETTEENTRY*pal,int start,int len)
{
    if (!lpddraw) return;
    IDirectDrawPalette_SetEntries(lpddpal,0,start,len,pal);
    IDirectDrawSurface_SetPalette(lpddsurf,lpddpal);
}

void VGA_SetQuadPalette(RGBQUAD*color,int start,int len)
{
    PALETTEENTRY pal[256];
    int c;

    if (!lpddraw) return;
    for (c=0; c<len; c++) {
        pal[c].peRed  =color[c].rgbRed;
        pal[c].peGreen=color[c].rgbGreen;
        pal[c].peBlue =color[c].rgbBlue;
        pal[c].peFlags=0;
    }
    IDirectDrawPalette_SetEntries(lpddpal,0,start,len,pal);
    IDirectDrawSurface_SetPalette(lpddsurf,lpddpal);
}

LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth)
{
    if (!lpddraw) return NULL;
    if (!lpddsurf) return NULL;
    if (IDirectDrawSurface_Lock(lpddsurf,NULL,&sdesc,0,0)) {
        ERR("could not lock surface!\n");
        return NULL;
    }
    if (Pitch) *Pitch=sdesc.lPitch;
    if (Height) *Height=sdesc.dwHeight;
    if (Width) *Width=sdesc.dwWidth;
    if (Depth) *Depth=sdesc.ddpfPixelFormat.x.dwRGBBitCount;
    return sdesc.y.lpSurface;
}

void VGA_Unlock(void)
{
    IDirectDrawSurface_Unlock(lpddsurf,sdesc.y.lpSurface);
}

/* We are called from SIGALRM, aren't we? We should _NOT_ do synchronization
 * stuff!
 */
void VGA_Poll( WORD timer )
{
    char *dat;
    unsigned Pitch,Height,Width;
    char *surf;
    int Y;
    /* int X; */

    EnterCriticalSection(&vga_crit);
    if (!vga_polling) {
        vga_polling++;
        LeaveCriticalSection(&vga_crit);
        /* FIXME: optimize by doing this only if the data has actually changed
         *        (in a way similar to DIBSection, perhaps) */
        surf = VGA_Lock(&Pitch,&Height,&Width,NULL);
        if (!surf) return;
        dat = DOSMEM_MapDosToLinear(0xa0000);
        /* copy from virtual VGA frame buffer to DirectDraw surface */
        for (Y=0; Y<Height; Y++,surf+=Pitch,dat+=Width) {
            memcpy(surf,dat,Width);
            /*for (X=0; X<Width; X++) if (dat[X]) TRACE(ddraw,"data(%d) at (%d,%d)\n",dat[X],X,Y);*/
        }
        VGA_Unlock();
        vga_refresh=1;
        EnterCriticalSection(&vga_crit);
        vga_polling--;
    }
    LeaveCriticalSection(&vga_crit);
}

static BYTE palreg,palcnt;
static PALETTEENTRY paldat;

void VGA_ioport_out( WORD port, BYTE val )
{
    switch (port) {
        case 0x3c8:
            palreg=val; palcnt=0; break;
        case 0x3c9:
            ((BYTE*)&paldat)[palcnt++]=val << 2;
            if (palcnt==3) {
                VGA_SetPalette(&paldat,palreg++,1);
                palcnt=0;
            }
            break;
    }
}

BYTE VGA_ioport_in( WORD port )
{
    BYTE ret;

    switch (port) {
        case 0x3da:
            /* since we don't (yet?) serve DOS VM requests while VGA_Poll is running,
               we need to fake the occurrence of the vertical refresh */
            if (lpddraw) {
                ret=vga_refresh?0x00:0x08;
                vga_refresh=0;
            } else ret=0x08;
            break;
        default:
            ret=0xff;
    }
    return ret;
}
