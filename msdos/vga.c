/*
 * VGA hardware emulation
 * 
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
 *
 */

#include <string.h>
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wincon.h"
#include "miscemu.h"
#include "vga.h"
#include "ddraw.h"
#include "services.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;
static LONG vga_polling,vga_refresh;
static HANDLE poll_timer;

typedef HRESULT WINAPI (*DirectDrawCreateProc)(LPGUID,LPDIRECTDRAW *,LPUNKNOWN);
static DirectDrawCreateProc pDirectDrawCreate;

typedef HWND WINAPI (*CreateWindowExAProc)(DWORD,LPCSTR,LPCSTR,DWORD,INT,INT, INT,INT,HWND,HMENU,HINSTANCE,LPVOID);
static CreateWindowExAProc pCreateWindowExA;

static void VGA_DeinstallTimer(void)
{
    if (poll_timer) {
        SERVICE_Delete( poll_timer );
        poll_timer = 0;
    }
}

static void VGA_InstallTimer(unsigned Rate)
{
    VGA_DeinstallTimer();
    if (!poll_timer)
        poll_timer = SERVICE_AddTimer( Rate, VGA_Poll, 0 );
}

HANDLE VGA_AlphaConsole(void)
{
    /* this assumes that no Win32 redirection has taken place, but then again,
     * only 16-bit apps are likely to use this part of Wine... */
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

char*VGA_AlphaBuffer(void)
{
    return DOSMEM_MapDosToLinear(0xb8000);
}

/*** GRAPHICS MODE ***/

int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth)
{
    LRESULT	res;
    HWND	hwnd;

    if (lpddraw) VGA_Exit();
    if (!lpddraw) {
        if (!pDirectDrawCreate)
        {
            HMODULE hmod = LoadLibraryA( "ddraw.dll" );
            if (hmod) pDirectDrawCreate = (DirectDrawCreateProc)GetProcAddress( hmod, "DirectDrawCreate" );
	    if (!pDirectDrawCreate) {
		ERR("Can't lookup DirectDrawCreate from ddraw.dll.\n");
		return 1;
	    }
        }
        if (!pCreateWindowExA)
	{
            HMODULE hmod = LoadLibraryA( "user32.dll" );
	    if (!hmod) {
		ERR("Can't load user32.dll.\n");
		return 1;
	    }
            if (hmod) pCreateWindowExA = (CreateWindowExAProc)GetProcAddress( hmod, "CreateWindowExA" );
	    if (!pCreateWindowExA) {
		ERR("Can't lookup CreateWindowExA from user32.dll.\n");
		return 1;
	    }
	}
        res = pDirectDrawCreate(NULL,&lpddraw,NULL);
        if (!lpddraw) {
            ERR("DirectDraw is not available (res = %lx)\n",res);
            return 1;
        }
	hwnd = pCreateWindowExA(0,"STATIC","WINEDOS VGA",WS_POPUP|WS_BORDER|WS_CAPTION|WS_SYSMENU,0,0,Xres,Yres,0,0,0,NULL);
	if (!hwnd) {
	    ERR("Failed to create user window.\n");
	}
        if ((res=IDirectDraw_SetCooperativeLevel(lpddraw,hwnd,DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE))) {
	    ERR("Could not set cooperative level to exclusive (%lx)\n",res);
	}

        if ((res=IDirectDraw_SetDisplayMode(lpddraw,Xres,Yres,Depth))) {
            ERR("DirectDraw does not support requested display mode (%dx%dx%d), res = %lx!\n",Xres,Yres,Depth,res);
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return 1;
        }
        res=IDirectDraw_CreatePalette(lpddraw,DDPCAPS_8BIT,NULL,&lpddpal,NULL);
	if (res) {
	    ERR("Could not create palette (res = %lx)\n",res);
	}
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
        FIXME("no default palette entries\n");
        IDirectDrawSurface_SetPalette(lpddsurf,lpddpal);
        vga_refresh=0;
        /* poll every 20ms (50fps should provide adequate responsiveness) */
        VGA_InstallTimer(20);
    }
    return 0;
}

int VGA_GetMode(unsigned*Height,unsigned*Width,unsigned*Depth)
{
    if (!lpddraw) return 1;
    if (!lpddsurf) return 1;
    if (Height) *Height=sdesc.dwHeight;
    if (Width) *Width=sdesc.dwWidth;
    if (Depth) *Depth=sdesc.ddpfPixelFormat.u1.dwRGBBitCount;
    return 0;
}

void VGA_Exit(void)
{
    if (lpddraw) {
        VGA_DeinstallTimer();
        IDirectDrawSurface_SetPalette(lpddsurf,NULL);
        IDirectDrawSurface_Release(lpddsurf);
        lpddsurf=NULL;
        IDirectDrawPalette_Release(lpddpal);
        lpddpal=NULL;
        IDirectDraw_Release(lpddraw);
        lpddraw=NULL;
    }
}

void VGA_SetPalette(PALETTEENTRY*pal,int start,int len)
{
    if (!lpddraw) return;
    IDirectDrawPalette_SetEntries(lpddpal,0,start,len,pal);
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
}

LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth)
{
    if (!lpddraw) return NULL;
    if (!lpddsurf) return NULL;
    if (IDirectDrawSurface_Lock(lpddsurf,NULL,&sdesc,0,0)) {
        ERR("could not lock surface!\n");
        return NULL;
    }
    if (Pitch) *Pitch=sdesc.u1.lPitch;
    if (Height) *Height=sdesc.dwHeight;
    if (Width) *Width=sdesc.dwWidth;
    if (Depth) *Depth=sdesc.ddpfPixelFormat.u1.dwRGBBitCount;
    return sdesc.lpSurface;
}

void VGA_Unlock(void)
{
    IDirectDrawSurface_Unlock(lpddsurf,sdesc.lpSurface);
}

/*** TEXT MODE ***/

int VGA_SetAlphaMode(unsigned Xres,unsigned Yres)
{
    COORD siz;

    if (lpddraw) VGA_Exit();

    /* the xterm is slow, so refresh only every 200ms (5fps) */
    VGA_InstallTimer(200);

    siz.X = Xres;
    siz.Y = Yres;
    SetConsoleScreenBufferSize(VGA_AlphaConsole(),siz);
    return 0;
}

void VGA_GetAlphaMode(unsigned*Xres,unsigned*Yres)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(VGA_AlphaConsole(),&info);
    if (Xres) *Xres=info.dwSize.X;
    if (Yres) *Yres=info.dwSize.Y;
}

void VGA_SetCursorPos(unsigned X,unsigned Y)
{
    COORD pos;
    
    pos.X = X;
    pos.Y = Y;
    SetConsoleCursorPosition(VGA_AlphaConsole(),pos);
}

void VGA_GetCursorPos(unsigned*X,unsigned*Y)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(VGA_AlphaConsole(),&info);
    if (X) *X=info.dwCursorPosition.X;
    if (Y) *Y=info.dwCursorPosition.Y;
}

void VGA_WriteChars(unsigned X,unsigned Y,unsigned ch,int attr,int count)
{
    unsigned XR, YR;
    char*dat;

    VGA_GetAlphaMode(&XR, &YR);
    dat = VGA_AlphaBuffer() + ((XR*Y + X) * 2);
    /* FIXME: also call WriteConsoleOutputA, for better responsiveness */
    while (count--) {
        *dat++ = ch;
        if (attr>=0) *dat = attr;
        dat++;
    }
}

/*** CONTROL ***/

void CALLBACK VGA_Poll( ULONG_PTR arg )
{
    char *dat;
    unsigned int Pitch,Height,Width,Y,X;
    char *surf;

    if (!InterlockedExchangeAdd(&vga_polling, 1)) {
        /* FIXME: optimize by doing this only if the data has actually changed
         *        (in a way similar to DIBSection, perhaps) */
        if (lpddraw) {
          /* graphics mode */
          surf = VGA_Lock(&Pitch,&Height,&Width,NULL);
          if (!surf) return;
          dat = DOSMEM_MapDosToLinear(0xa0000);
          /* copy from virtual VGA frame buffer to DirectDraw surface */
          for (Y=0; Y<Height; Y++,surf+=Pitch,dat+=Width) {
              memcpy(surf,dat,Width);
              /*for (X=0; X<Width; X++) if (dat[X]) TRACE(ddraw,"data(%d) at (%d,%d)\n",dat[X],X,Y);*/
          }
          VGA_Unlock();
        } else {
          /* text mode */
          CHAR_INFO ch[80];
          COORD siz, off;
          SMALL_RECT dest;
          HANDLE con = VGA_AlphaConsole();

          VGA_GetAlphaMode(&Width,&Height);
          dat = VGA_AlphaBuffer();
          siz.X = 80; siz.Y = 1;
          off.X = 0; off.Y = 0;
          /* copy from virtual VGA frame buffer to console */
          for (Y=0; Y<Height; Y++) {
              dest.Top=Y; dest.Bottom=Y;
              for (X=0; X<Width; X++) {
                  ch[X].Char.AsciiChar = *dat++;
		  /* WriteConsoleOutputA doesn't like "dead" chars */
		  if (ch[X].Char.AsciiChar == '\0')
		      ch[X].Char.AsciiChar = ' ';
                  ch[X].Attributes = *dat++;
              }
              dest.Left=0; dest.Right=Width+1;
              WriteConsoleOutputA(con, ch, siz, off, &dest);
          }
        }
        vga_refresh=1;
    }
    InterlockedDecrement(&vga_polling);
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
            ret=vga_refresh?0x00:0x08;
            vga_refresh=0;
            break;
        default:
            ret=0xff;
    }
    return ret;
}

void VGA_Clean(void)
{
    VGA_Exit();
    VGA_DeinstallTimer();
}
