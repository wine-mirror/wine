/*
 * VGA hardware emulation
 * 
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wincon.h"
#include "miscemu.h"
#include "dosexe.h"
#include "vga.h"
#include "ddraw.h"
#include "services.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;
static LONG vga_polling,vga_refresh;
static HANDLE poll_timer;

static int vga_width;
static int vga_height;
static int vga_depth;

typedef HRESULT (WINAPI *DirectDrawCreateProc)(LPGUID,LPDIRECTDRAW *,LPUNKNOWN);
static DirectDrawCreateProc pDirectDrawCreate;

static PALETTEENTRY vga_def_palette[256]={
/* red  green  blue */
  {0x00, 0x00, 0x00}, /* 0 - Black */
  {0x00, 0x00, 0x80}, /* 1 - Blue */
  {0x00, 0x80, 0x00}, /* 2 - Green */
  {0x00, 0x80, 0x80}, /* 3 - Cyan */
  {0x80, 0x00, 0x00}, /* 4 - Red */
  {0x80, 0x00, 0x80}, /* 5 - Magenta */
  {0x80, 0x80, 0x00}, /* 6 - Brown */
  {0xC0, 0xC0, 0xC0}, /* 7 - Light gray */
  {0x80, 0x80, 0x80}, /* 8 - Dark gray */
  {0x00, 0x00, 0xFF}, /* 9 - Light blue */
  {0x00, 0xFF, 0x00}, /* A - Light green */
  {0x00, 0xFF, 0xFF}, /* B - Light cyan */
  {0xFF, 0x00, 0x00}, /* C - Light red */
  {0xFF, 0x00, 0xFF}, /* D - Light magenta */
  {0xFF, 0xFF, 0x00}, /* E - Yellow */
  {0xFF, 0xFF, 0xFF}, /* F - White */
  {0,0,0} /* FIXME: a series of continuous rainbow hues should follow */
};

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

typedef struct {
  unsigned Xres, Yres, Depth;
  int ret;
} ModeSet;

static void WINAPI VGA_DoSetMode(ULONG_PTR arg)
{
    LRESULT	res;
    HWND	hwnd;
    ModeSet *par = (ModeSet *)arg;
    par->ret=1;

    if (lpddraw) VGA_Exit();
    if (!lpddraw) {
        if (!pDirectDrawCreate)
        {
            HMODULE hmod = LoadLibraryA( "ddraw.dll" );
            if (hmod) pDirectDrawCreate = (DirectDrawCreateProc)GetProcAddress( hmod, "DirectDrawCreate" );
	    if (!pDirectDrawCreate) {
		ERR("Can't lookup DirectDrawCreate from ddraw.dll.\n");
		return;
	    }
        }
        res = pDirectDrawCreate(NULL,&lpddraw,NULL);
        if (!lpddraw) {
            ERR("DirectDraw is not available (res = %lx)\n",res);
            return;
        }
	hwnd = CreateWindowExA(0,"STATIC","WINEDOS VGA",WS_POPUP|WS_BORDER|WS_CAPTION|WS_SYSMENU,0,0,par->Xres,par->Yres,0,0,0,NULL);
	if (!hwnd) {
	    ERR("Failed to create user window.\n");
	}
        if ((res=IDirectDraw_SetCooperativeLevel(lpddraw,hwnd,DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE))) {
	    ERR("Could not set cooperative level to exclusive (%lx)\n",res);
	}

        if ((res=IDirectDraw_SetDisplayMode(lpddraw,par->Xres,par->Yres,par->Depth))) {
            ERR("DirectDraw does not support requested display mode (%dx%dx%d), res = %lx!\n",par->Xres,par->Yres,par->Depth,res);
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return;
        }

        res=IDirectDraw_CreatePalette(lpddraw,DDPCAPS_8BIT,NULL,&lpddpal,NULL);
        if (res) {
	    ERR("Could not create palette (res = %lx)\n",res);
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return;
        }
        if ((res=IDirectDrawPalette_SetEntries(lpddpal,0,0,256,vga_def_palette))) {
            ERR("Could not set default palette entries (res = %lx)\n", res);
        }

        memset(&sdesc,0,sizeof(sdesc));
        sdesc.dwSize=sizeof(sdesc);
	sdesc.dwFlags = DDSD_CAPS;
	sdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw_CreateSurface(lpddraw,&sdesc,&lpddsurf,NULL)||(!lpddsurf)) {
            ERR("DirectDraw surface is not available\n");
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return;
        }
        IDirectDrawSurface_SetPalette(lpddsurf,lpddpal);
        vga_refresh=0;
        /* poll every 20ms (50fps should provide adequate responsiveness) */
        VGA_InstallTimer(20);
    }
    par->ret=0;
    return;
}

int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth)
{
    ModeSet par;

    vga_width = Xres;
    vga_height = Yres;
    vga_depth = Depth;

    if(Xres >= 640 || Yres >= 480) {
      par.Xres = Xres;
      par.Yres = Yres;
    } else {
      par.Xres = 640;
      par.Yres = 480;
    }

    par.Depth = (Depth < 8) ? 8 : Depth;

    MZ_RunInThread(VGA_DoSetMode, (ULONG_PTR)&par);
    return par.ret;
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

static void WINAPI VGA_DoExit(ULONG_PTR arg)
{
    VGA_DeinstallTimer();
    IDirectDrawSurface_SetPalette(lpddsurf,NULL);
    IDirectDrawSurface_Release(lpddsurf);
    lpddsurf=NULL;
    IDirectDrawPalette_Release(lpddpal);
    lpddpal=NULL;
    IDirectDraw_Release(lpddraw);
    lpddraw=NULL;
}

void VGA_Exit(void)
{
    if (lpddraw) MZ_RunInThread(VGA_DoExit, 0);
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

    if (!poll_timer) VGA_SetAlphaMode(80, 25);
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

static void VGA_Poll_Graphics(void)
{
  unsigned int Pitch, Height, Width, X, Y;
  char *surf;
  char *dat = DOSMEM_MapDosToLinear(0xa0000);

  surf = VGA_Lock(&Pitch,&Height,&Width,NULL);
  if (!surf) return;

  if(vga_width == 320 && vga_depth <= 4)
    for (Y=0; Y<vga_height; Y++,surf+=Pitch*2,dat+=vga_width/8) {
      for(X=0; X<vga_width; X+=8) {
       int offset = X/8;
       int Z;
       for(Z=0; Z<8; Z++) {
         int b0 =  (dat[offset] >> Z) & 0x1;
         int index = 7-Z;
         surf[(X+index)*2] = b0;
         surf[(X+index)*2+1] = b0;
         surf[(X+index)*2+Pitch] = b0;
         surf[(X+index)*2+Pitch+1] = b0;
       }
      }
    }

  if(vga_width == 320 && vga_depth == 8)
    for (Y=0; Y<vga_height; Y++,surf+=Pitch*2,dat+=vga_width) {
      for(X=0; X<vga_width; X++) {
       int b0 = dat[X];
       surf[X*2] = b0;
       surf[X*2+1] = b0;
       surf[X*2+Pitch] = b0;
       surf[X*2+Pitch+1] = b0;
      }
    }

  if(vga_depth <= 4)
    for (Y=0; Y<vga_height; Y++,surf+=Pitch,dat+=vga_width/8) {
      for(X=0; X<vga_width; X+=8) {
       int offset = X/8;
       int Z;
       for(Z=0; Z<8; Z++) {
         int b0 =  (dat[offset] >> Z) & 0x1;
         int index = 7-Z;
         surf[X+index] = b0;
       }
      }
    }

  VGA_Unlock();
}


void CALLBACK VGA_Poll( ULONG_PTR arg )
{
    char *dat;
    unsigned int Height,Width,Y,X;

    if (!InterlockedExchangeAdd(&vga_polling, 1)) {
        /* FIXME: optimize by doing this only if the data has actually changed
         *        (in a way similar to DIBSection, perhaps) */
        if (lpddraw) {
         VGA_Poll_Graphics();
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
