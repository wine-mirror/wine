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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;
static LONG vga_refresh;
static HANDLE poll_timer;

static int vga_width;
static int vga_height;
static int vga_depth;
static BYTE vga_text_attr;

static BOOL vga_mode_initialized = FALSE;

static CRITICAL_SECTION vga_lock = CRITICAL_SECTION_INIT("VGA");

typedef HRESULT (WINAPI *DirectDrawCreateProc)(LPGUID,LPDIRECTDRAW *,LPUNKNOWN);
static DirectDrawCreateProc pDirectDrawCreate;

static void CALLBACK VGA_Poll( LPVOID arg, DWORD low, DWORD high );

static HWND vga_hwnd = (HWND) NULL;

/*
 * For simplicity, I'm creating a second palette.
 * 16 color accesses will use these pointers and insert
 * entries from the 64-color palette into the default
 * palette.   --Robert 'Admiral' Coeyman
 */

static char vga_16_palette[17]={
  0x00,  /* 0 - Black         */
  0x01,  /* 1 - Blue          */
  0x02,  /* 2 - Green         */
  0x03,  /* 3 - Cyan          */
  0x04,  /* 4 - Red           */
  0x05,  /* 5 - Magenta       */
  0x14,  /* 6 - Brown         */
  0x07,  /* 7 - Light gray    */
  0x38,  /* 8 - Dark gray     */
  0x39,  /* 9 - Light blue    */
  0x3a,  /* A - Light green   */
  0x3b,  /* B - Light cyan    */
  0x3c,  /* C - Light red     */
  0x3d,  /* D - Light magenta */
  0x3e,  /* E - Yellow        */
  0x3f,  /* F - White         */
  0x00   /* Border Color      */
};

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

/*
 *   This palette is the dos default, converted from 18 bit color to 24.
 *      It contains only 64 entries of colors--all others are zeros.
 *          --Robert 'Admiral' Coeyman
 */
static PALETTEENTRY vga_def64_palette[256]={
/* red  green  blue */
  {0x00, 0x00, 0x00}, /* 0x00      Black      */
  {0x00, 0x00, 0xaa}, /* 0x01      Blue       */
  {0x00, 0xaa, 0x00}, /* 0x02      Green      */
  {0x00, 0xaa, 0xaa}, /* 0x03      Cyan       */
  {0xaa, 0x00, 0x00}, /* 0x04      Red        */
  {0xaa, 0x00, 0xaa}, /* 0x05      Magenta    */
  {0xaa, 0xaa, 0x00}, /* 0x06      */
  {0xaa, 0xaa, 0xaa}, /* 0x07      Light Gray */
  {0x00, 0x00, 0x55}, /* 0x08      */
  {0x00, 0x00, 0xff}, /* 0x09      */
  {0x00, 0xaa, 0x55}, /* 0x0a      */
  {0x00, 0xaa, 0xff}, /* 0x0b      */
  {0xaa, 0x00, 0x55}, /* 0x0c      */
  {0xaa, 0x00, 0xff}, /* 0x0d      */
  {0xaa, 0xaa, 0x55}, /* 0x0e      */
  {0xaa, 0xaa, 0xff}, /* 0x0f      */
  {0x00, 0x55, 0x00}, /* 0x10      */
  {0x00, 0x55, 0xaa}, /* 0x11      */
  {0x00, 0xff, 0x00}, /* 0x12      */
  {0x00, 0xff, 0xaa}, /* 0x13      */
  {0xaa, 0x55, 0x00}, /* 0x14      Brown      */
  {0xaa, 0x55, 0xaa}, /* 0x15      */
  {0xaa, 0xff, 0x00}, /* 0x16      */
  {0xaa, 0xff, 0xaa}, /* 0x17      */
  {0x00, 0x55, 0x55}, /* 0x18      */
  {0x00, 0x55, 0xff}, /* 0x19      */
  {0x00, 0xff, 0x55}, /* 0x1a      */
  {0x00, 0xff, 0xff}, /* 0x1b      */
  {0xaa, 0x55, 0x55}, /* 0x1c      */
  {0xaa, 0x55, 0xff}, /* 0x1d      */
  {0xaa, 0xff, 0x55}, /* 0x1e      */
  {0xaa, 0xff, 0xff}, /* 0x1f      */
  {0x55, 0x00, 0x00}, /* 0x20      */
  {0x55, 0x00, 0xaa}, /* 0x21      */
  {0x55, 0xaa, 0x00}, /* 0x22      */
  {0x55, 0xaa, 0xaa}, /* 0x23      */
  {0xff, 0x00, 0x00}, /* 0x24      */
  {0xff, 0x00, 0xaa}, /* 0x25      */
  {0xff, 0xaa, 0x00}, /* 0x26      */
  {0xff, 0xaa, 0xaa}, /* 0x27      */
  {0x55, 0x00, 0x55}, /* 0x28      */
  {0x55, 0x00, 0xff}, /* 0x29      */
  {0x55, 0xaa, 0x55}, /* 0x2a      */
  {0x55, 0xaa, 0xff}, /* 0x2b      */
  {0xff, 0x00, 0x55}, /* 0x2c      */
  {0xff, 0x00, 0xff}, /* 0x2d      */
  {0xff, 0xaa, 0x55}, /* 0x2e      */
  {0xff, 0xaa, 0xff}, /* 0x2f      */
  {0x55, 0x55, 0x00}, /* 0x30      */
  {0x55, 0x55, 0xaa}, /* 0x31      */
  {0x55, 0xff, 0x00}, /* 0x32      */
  {0x55, 0xff, 0xaa}, /* 0x33      */
  {0xff, 0x55, 0x00}, /* 0x34      */
  {0xff, 0x55, 0xaa}, /* 0x35      */
  {0xff, 0xff, 0x00}, /* 0x36      */
  {0xff, 0xff, 0xaa}, /* 0x37      */
  {0x55, 0x55, 0x55}, /* 0x38      Dark Gray     */
  {0x55, 0x55, 0xff}, /* 0x39      Light Blue    */
  {0x55, 0xff, 0x55}, /* 0x3a      Light Green   */
  {0x55, 0xff, 0xff}, /* 0x3b      Light Cyan    */
  {0xff, 0x55, 0x55}, /* 0x3c      Light Red     */
  {0xff, 0x55, 0xff}, /* 0x3d      Light Magenta */
  {0xff, 0xff, 0x55}, /* 0x3e      Yellow        */
  {0xff, 0xff, 0xff}, /* 0x3f      White         */
  {0,0,0} /* The next 192 entries are all zeros  */
};

static HANDLE VGA_timer;
static HANDLE VGA_timer_thread;

/* set the timer rate; called in the polling thread context */
static void CALLBACK set_timer_rate( ULONG_PTR arg )
{
    LARGE_INTEGER when;

    when.s.LowPart = when.s.HighPart = 0;
    SetWaitableTimer( VGA_timer, &when, arg, VGA_Poll, 0, FALSE );
}

static DWORD CALLBACK VGA_TimerThread( void *dummy )
{
    for (;;) WaitForMultipleObjectsEx( 0, NULL, FALSE, INFINITE, TRUE );
}

static void VGA_DeinstallTimer(void)
{
    if (VGA_timer_thread)
    {
        CancelWaitableTimer( VGA_timer );
        CloseHandle( VGA_timer );
        TerminateThread( VGA_timer_thread, 0 );
        CloseHandle( VGA_timer_thread );
        VGA_timer_thread = 0;
    }
}

static void VGA_InstallTimer(unsigned Rate)
{
    if (!VGA_timer_thread)
    {
        VGA_timer = CreateWaitableTimerA( NULL, FALSE, NULL );
        VGA_timer_thread = CreateThread( NULL, 0, VGA_TimerThread, NULL, 0, NULL );
    }
    QueueUserAPC( set_timer_rate, VGA_timer_thread, (ULONG_PTR)Rate );
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

static void WINAPI VGA_DoSetMode(ULONG_PTR arg)
{
    LRESULT	res;
    ModeSet *par = (ModeSet *)arg;
    par->ret=1;

    if (lpddraw) VGA_DoExit(0);
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
        if (!vga_hwnd) {
            vga_hwnd = CreateWindowExA(0,"STATIC","WINEDOS VGA",WS_POPUP|WS_BORDER|WS_CAPTION|WS_SYSMENU,0,0,par->Xres,par->Yres,0,0,0,NULL);
            if (!vga_hwnd) {
                ERR("Failed to create user window.\n");
                IDirectDraw_Release(lpddraw);
                lpddraw=NULL;
                return;
            }
        }
        else
            SetWindowPos(vga_hwnd,0,0,0,par->Xres,par->Yres,SWP_NOMOVE|SWP_NOZORDER);

        if ((res=IDirectDraw_SetCooperativeLevel(lpddraw,vga_hwnd,DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE))) {
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

    vga_mode_initialized = TRUE;

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

void VGA_Exit(void)
{
    if (lpddraw) MZ_RunInThread(VGA_DoExit, 0);
}

void VGA_SetPalette(PALETTEENTRY*pal,int start,int len)
{
    if (!lpddraw) return;
    IDirectDrawPalette_SetEntries(lpddpal,0,start,len,pal);
}

/* set a single [char wide] color in 16 color mode. */
void VGA_SetColor16(int reg,int color)
{
	PALETTEENTRY *pal;

    if (!lpddraw) return;
	pal= &vga_def64_palette[color];
        IDirectDrawPalette_SetEntries(lpddpal,0,reg,1,pal);
	vga_16_palette[reg]=(char)color;
}

/* Get a single [char wide] color in 16 color mode. */
char VGA_GetColor16(int reg)
{

    if (!lpddraw) return 0;
	return (char)vga_16_palette[reg];
}

/* set all 17 [char wide] colors at once in 16 color mode. */
void VGA_Set16Palette(char *Table)
{
	PALETTEENTRY *pal;
	int c;

    if (!lpddraw) return;         /* return if we're in text only mode */
	bcopy((void *)&vga_16_palette,(void *)Table,17);
		                    /* copy the entries into the table */
    for (c=0; c<17; c++) {                                /* 17 entries */
	pal= &vga_def64_palette[(int)vga_16_palette[c]];  /* get color  */
        IDirectDrawPalette_SetEntries(lpddpal,0,c,1,pal); /* set entry  */
	TRACE("Palette register %d set to %d\n",c,(int)vga_16_palette[c]);
   } /* end of the counting loop */
}

/* Get all 17 [ char wide ] colors at once in 16 color mode. */
void VGA_Get16Palette(char *Table)
{

    if (!lpddraw) return;         /* return if we're in text only mode */
	bcopy((void *)Table,(void *)&vga_16_palette,17);
		                    /* copy the entries into the table */
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

    /* FIXME: Where to initialize text attributes? */
    VGA_SetTextAttribute(0xf);

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
    CHAR_INFO info;
    COORD siz, off;
    SMALL_RECT dest;
    unsigned XR, YR;
    char *dat;

    EnterCriticalSection(&vga_lock);

    info.Char.AsciiChar = ch;
    info.Attributes = (WORD)attr;
    siz.X = 1;
    siz.Y = 1;
    off.X = 0;
    off.Y = 0;
    dest.Top=Y;
    dest.Bottom=Y;

    VGA_GetAlphaMode(&XR, &YR);
    dat = VGA_AlphaBuffer() + ((XR*Y + X) * 2);
    while (count--) {
        dest.Left = X + count;
       dest.Right = X + count;

        *dat++ = ch;
        if (attr>=0)
         *dat = attr;
       else
         info.Attributes = *dat;
        dat++;

       WriteConsoleOutputA(VGA_AlphaConsole(), &info, siz, off, &dest);
    }

    LeaveCriticalSection(&vga_lock);
}

static void VGA_PutCharAt(BYTE ascii, unsigned x, unsigned y)
{
    unsigned width, height;
    char *dat;

    VGA_GetAlphaMode(&width, &height);
    dat = VGA_AlphaBuffer() + ((width*y + x) * 2);
    dat[0] = ascii;
    dat[1] = vga_text_attr;
}

void VGA_PutChar(BYTE ascii)
{
    unsigned width, height, x, y, nx, ny;

    EnterCriticalSection(&vga_lock);

    VGA_GetAlphaMode(&width, &height);
    VGA_GetCursorPos(&x, &y);

    switch(ascii) {
    case '\b':
        VGA_PutCharAt(' ', x, y);
       x--;
       break;

    case '\t':
       x += ((x + 8) & ~7) - x;
       break;

    case '\n':
        y++;
       x = 0;
       break;

    case '\a':
        break;

    case '\r':
        x = 0;
       break;

    default:
        VGA_PutCharAt(ascii, x, y);
       x++;
    }

    /*
     * FIXME: add line wrapping and scrolling
     */

    WriteFile(VGA_AlphaConsole(), &ascii, 1, NULL, NULL);

    /*
     * The following is just a sanity check.
     */
    VGA_GetCursorPos(&nx, &ny);
    if(nx != x || ny != y)
      WARN("VGA emulator and text console have become unsynchronized.\n");

    LeaveCriticalSection(&vga_lock);
}

void VGA_SetTextAttribute(BYTE attr)
{
    vga_text_attr = attr;
    SetConsoleTextAttribute(VGA_AlphaConsole(), attr);
}

void VGA_ClearText(unsigned row1, unsigned col1,
                  unsigned row2, unsigned col2,
                  BYTE attr)
{
    unsigned width, height, x, y;
    COORD off;
    char *dat = VGA_AlphaBuffer();
    HANDLE con = VGA_AlphaConsole();
    VGA_GetAlphaMode(&width, &height);

    EnterCriticalSection(&vga_lock);

    for(y=row1; y<=row2; y++) {
        off.X = col1;
       off.Y = y;
       FillConsoleOutputCharacterA(con, ' ', col2-col1+1, off, NULL);
       FillConsoleOutputAttribute(con, attr, col2-col1+1, off, NULL);

       for(x=col1; x<=col2; x++) {
           char *ptr = dat + ((width*y + x) * 2);
           ptr[0] = ' ';
           ptr[1] = attr;
       }
    }

    LeaveCriticalSection(&vga_lock);
}

void VGA_ScrollUpText(unsigned row1, unsigned col1,
                     unsigned row2, unsigned col2,
                     unsigned lines, BYTE attr)
{
    FIXME("not implemented\n");
}

void VGA_ScrollDownText(unsigned row1, unsigned col1,
                       unsigned row2, unsigned col2,
                       unsigned lines, BYTE attr)
{
    FIXME("not implemented\n");
}

void VGA_GetCharacterAtCursor(BYTE *ascii, BYTE *attr)
{
    unsigned width, height, x, y;
    char *dat;

    VGA_GetAlphaMode(&width, &height);
    VGA_GetCursorPos(&x, &y);
    dat = VGA_AlphaBuffer() + ((width*y + x) * 2);

    *ascii = dat[0];
    *attr = dat[1];
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

static void VGA_Poll_Text(void)
{
    char *dat;
    unsigned int Height,Width,Y,X;
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

static void CALLBACK VGA_Poll( LPVOID arg, DWORD low, DWORD high )
{
    if(!TryEnterCriticalSection(&vga_lock))
        return;

    /* FIXME: optimize by doing this only if the data has actually changed
     *        (in a way similar to DIBSection, perhaps) */
    if (lpddraw) {
        VGA_Poll_Graphics();
    } else {
        VGA_Poll_Text();
    }

    vga_refresh=1;
    LeaveCriticalSection(&vga_lock);
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
            if (vga_mode_initialized)
                vga_refresh=0;
            else
                /* Also fake the occurence of the vertical refresh when no graphic
                   mode has been set */
                vga_refresh=!vga_refresh;
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
