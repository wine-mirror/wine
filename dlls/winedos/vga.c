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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wincon.h"
#include "dosexe.h"
#include "vga.h"
#include "ddraw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static IDirectDraw *lpddraw = NULL;
static IDirectDrawSurface *lpddsurf;
static IDirectDrawPalette *lpddpal;
static DDSURFACEDESC sdesc;

static BOOL vga_retrace_vertical;
static BOOL vga_retrace_horizontal;

/*
 * Size and location of VGA controller window to framebuffer.
 *
 * Note: We support only single window even though some
 *       controllers support two. This should not be changed unless
 *       there are programs that depend on having two windows.
 */
#define VGA_WINDOW_SIZE  (64 * 1024)
#define VGA_WINDOW_START ((char *)0xa0000)

/*
 * VGA controller memory is emulated using linear framebuffer.
 * This frambuffer also acts as an interface
 * between VGA controller emulation and DirectDraw.
 *
 * vga_fb_width: Display width in pixels. Can be modified when
 *               display mode is changed.
 * vga_fb_height: Display height in pixels. Can be modified when
 *                display mode is changed.
 * vga_fb_depth: Number of bits used to store single pixel color information.
 *               Each pixel uses (vga_fb_depth+7)/8 bytes because
 *               1-16 color modes are mapped to 256 color mode.
 *               Can be modified when display mode is changed.
 * vga_fb_pitch: How many bytes to add to pointer in order to move
 *               from one row to another. This is fixed in VGA modes,
 *               but can be modified in SVGA modes.
 * vga_fb_offset: Offset added to framebuffer start address in order
 *                to find the display origin. Programs use this to do
 *                double buffering and to scroll display. The value can
 *                be modified in VGA and SVGA modes.
 * vga_fb_size: How many bytes are allocated to framebuffer.
 *              VGA framebuffers are always larger than display size and
 *              SVGA framebuffers may also be.
 * vga_fb_data: Pointer to framebuffer start.
 * vga_fb_window: Offset of 64k window 0xa0000 in bytes from framebuffer start.
 *                This value is >= 0, if mode uses linear framebuffer and
 *                -1, if mode uses color planes. This value is fixed
 *                in all modes except 0x13 (256 color VGA) where
 *                0 means normal mode and -1 means Mode-X (unchained mode).
 */
static int   vga_fb_width;
static int   vga_fb_height;
static int   vga_fb_depth;
static int   vga_fb_pitch;
static int   vga_fb_offset;
static int   vga_fb_size = 0;
static char *vga_fb_data = 0;
static int   vga_fb_window = 0;

/*
 * VGA text mode data.
 *
 * vga_text_attr: Current active attribute.
 * vga_text_old: Last data sent to console. 
 *               This is used to optimize console updates.
 * vga_text_width:  Width of the text display in characters.
 * vga_text_height: Height of the text display in characters.
 * vga_text_x: Current cursor X-position. Starts from zero.
 * vga_text_y: Current cursor Y-position. Starts from zero.
 * vga_text_console: TRUE if stdout is console, 
 *                   FALSE if it is regular file.
 */
static BYTE  vga_text_attr;
static char *vga_text_old = NULL;
static BYTE  vga_text_width;
static BYTE  vga_text_height;
static BYTE  vga_text_x;
static BYTE  vga_text_y;
static BOOL  vga_text_console;

/*
 * VGA controller ports 0x3c0, 0x3c4, 0x3ce and 0x3d4 are
 * indexed registers. These ports are used to select VGA controller
 * subregister that can be written to or read from using ports 0x3c1,
 * 0x3c5, 0x3cf or 0x3d5. Selected subregister indexes are
 * stored in variables vga_index_*.
 *
 * Port 0x3c0 is special because it is both index and
 * data-write register. Flip-flop vga_address_3c0 tells whether
 * the port acts currently as an address register. Reading from port
 * 0x3da resets the flip-flop to address mode.
 */
static BYTE vga_index_3c0;
static BYTE vga_index_3c4;
static BYTE vga_index_3ce;
static BYTE vga_index_3d4;
static BOOL vga_address_3c0 = TRUE;

/*
 * This mutex is used to protect VGA state during asynchronous
 * screen updates (see VGA_Poll). It makes sure that VGA state changes
 * are atomic and the user interface is protected from flicker and
 * corruption.
 *
 * The mutex actually serializes VGA operations and the screen update. 
 * Which means that whenever VGA_Poll occurs, application stalls if it 
 * tries to modify VGA state. This is not how real VGA adapters work,
 * but it makes timing and correctness issues much easier to deal with.
 */
static CRITICAL_SECTION vga_lock;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &vga_lock,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": vga_lock") }
};
static CRITICAL_SECTION vga_lock = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef HRESULT (WINAPI *DirectDrawCreateProc)(LPGUID,LPDIRECTDRAW *,LPUNKNOWN);
static DirectDrawCreateProc pDirectDrawCreate;

static void CALLBACK VGA_Poll( LPVOID arg, DWORD low, DWORD high );

static HWND vga_hwnd = NULL;

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

    when.u.LowPart = when.u.HighPart = 0;
    SetWaitableTimer( VGA_timer, &when, arg, VGA_Poll, 0, FALSE );
}

static DWORD CALLBACK VGA_TimerThread( void *dummy )
{
    for (;;) SleepEx( INFINITE, TRUE );
}

static void VGA_DeinstallTimer(void)
{
    if (VGA_timer_thread)
    {
        /*
         * Make sure the update thread is not holding
         * system resources when we kill it.
         *
         * Now, we only need to worry about update thread
         * getting terminated while in EnterCriticalSection 
         * or WaitForMultipleObjectsEx.
         *
         * FIXME: Is this a problem?
         */
        EnterCriticalSection(&vga_lock);

        CancelWaitableTimer( VGA_timer );
        CloseHandle( VGA_timer );
        TerminateThread( VGA_timer_thread, 0 );
        CloseHandle( VGA_timer_thread );
        VGA_timer_thread = 0;

        LeaveCriticalSection(&vga_lock);

        /*
         * Synchronize display. This makes sure that
         * changes to display become visible even if program 
         * terminates before update thread had time to run.
         */
        VGA_Poll( 0, 0, 0 );
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

static BOOL VGA_IsTimerRunning(void)
{
    return VGA_timer_thread ? TRUE : FALSE;
}

static HANDLE VGA_AlphaConsole(void)
{
    /* this assumes that no Win32 redirection has taken place, but then again,
     * only 16-bit apps are likely to use this part of Wine... */
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static char*VGA_AlphaBuffer(void)
{
    return (char *)0xb8000;
}

/*** GRAPHICS MODE ***/

typedef struct {
  unsigned Xres, Yres, Depth;
  int ret;
} ModeSet;


/**********************************************************************
 *         VGA_SyncWindow
 *
 * Copy VGA window into framebuffer (if argument is TRUE) or
 * part of framebuffer into VGA window (if argument is FALSE).
 */
static void VGA_SyncWindow( BOOL target_is_fb )
{
    int size = VGA_WINDOW_SIZE;

    /* Window does not overlap framebuffer. */
    if (vga_fb_window >= vga_fb_size)
        return;

    /* Check if window overlaps framebuffer only partially. */
    if (vga_fb_size - vga_fb_window < VGA_WINDOW_SIZE)
        size = vga_fb_size - vga_fb_window;

    if (target_is_fb)
        memmove( vga_fb_data + vga_fb_window, VGA_WINDOW_START, size );
    else
        memmove( VGA_WINDOW_START, vga_fb_data + vga_fb_window, size );
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
            vga_hwnd = CreateWindowExA(0,"STATIC","WINEDOS VGA",
                                       WS_POPUP|WS_VISIBLE|SS_NOTIFY,0,0,
                                       par->Xres,par->Yres,0,0,0,NULL);
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
        vga_retrace_vertical = vga_retrace_horizontal = FALSE;
        /* poll every 20ms (50fps should provide adequate responsiveness) */
        VGA_InstallTimer(20);
    }
    par->ret=0;
    return;
}

int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth)
{
    ModeSet par;
    int     newSize;

    vga_fb_width = Xres;
    vga_fb_height = Yres;
    vga_fb_depth = Depth;
    vga_fb_offset = 0;
    vga_fb_pitch = Xres * ((Depth + 7) / 8);

    newSize = Xres * Yres * ((Depth + 7) / 8);
    if(newSize < 256 * 1024)
      newSize = 256 * 1024;

    if(vga_fb_size < newSize) {
      HeapFree(GetProcessHeap(), 0, vga_fb_data);
      vga_fb_data = HeapAlloc(GetProcessHeap(), 0, newSize);
      vga_fb_size = newSize;
    }

    if(Xres >= 640 || Yres >= 480) {
      par.Xres = Xres;
      par.Yres = Yres;
    } else {
      par.Xres = 640;
      par.Yres = 480;
    }

    VGA_SetWindowStart((Depth < 8) ? -1 : 0);

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

static void VGA_Exit(void)
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
    memcpy( Table, &vga_16_palette, 17 ); /* copy the entries into the table */

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
    memcpy( &vga_16_palette, Table, 17 ); /* copy the entries into the table */
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

static LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth)
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

static void VGA_Unlock(void)
{
    IDirectDrawSurface_Unlock(lpddsurf,sdesc.lpSurface);
}

/*
 * Set start of 64k window at 0xa0000 in bytes.
 * If value is -1, initialize color plane support.
 * If value is >= 0, window contains direct copy of framebuffer.
 */
void VGA_SetWindowStart(int start)
{
    if(start == vga_fb_window)
        return;

    EnterCriticalSection(&vga_lock);

    if(vga_fb_window == -1)
        FIXME("Remove VGA memory emulation.\n");
    else
        VGA_SyncWindow( TRUE );

    vga_fb_window = start;

    if(vga_fb_window == -1)
        FIXME("Install VGA memory emulation.\n");
    else
        VGA_SyncWindow( FALSE );

    LeaveCriticalSection(&vga_lock);
}

/*
 * Get start of 64k window at 0xa0000 in bytes.
 * Value is -1 in color plane modes.
 */
int VGA_GetWindowStart(void)
{
    return vga_fb_window;
}


/**********************************************************************
 *         VGA_DoShowMouse
 *
 * Callback for VGA_ShowMouse.
 */
static WINAPI void VGA_DoShowMouse( ULONG_PTR show )
{
    INT rv;

    do
    {
        rv = ShowCursor( show );
    }
    while( show ? (rv < 0) : (rv >= 0) );
}


/**********************************************************************
 *         VGA_ShowMouse
 *
 * If argument is TRUE, unconditionally show mouse cursor.
 * If argument is FALSE, unconditionally hide mouse cursor.
 * This only works in graphics mode.
 */
void VGA_ShowMouse( BOOL show )
{
    if (lpddraw)
        MZ_RunInThread( VGA_DoShowMouse, (ULONG_PTR)show );
}


/*** TEXT MODE ***/

/* prepare the text mode video memory copy that is used to only
 * update the video memory line that did get updated. */
static void VGA_PrepareVideoMemCopy(unsigned Xres, unsigned Yres)
{
    char *p, *p2;
    unsigned int i;

    /*
     * Allocate space for char + attr.
     */

    if (vga_text_old)
        vga_text_old = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                vga_text_old, Xres * Yres * 2 );
    else
        vga_text_old = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                 Xres * Yres * 2 );
    p = VGA_AlphaBuffer();
    p2 = vga_text_old;

    /* make sure the video mem copy contains the exact opposite of our
     * actual text mode memory area to make sure the screen
     * does get updated fully initially */
    for (i=0; i < Xres*Yres*2; i++)
	*p2++ = *p++ ^ 0xff; /* XOR it */
}

/**********************************************************************
 *         VGA_SetAlphaMode
 *
 * Set VGA emulation to text mode.
 */
void VGA_SetAlphaMode(unsigned Xres,unsigned Yres)
{
    VGA_Exit();
    VGA_DeinstallTimer();
    
    VGA_PrepareVideoMemCopy(Xres, Yres);
    vga_text_width = Xres;
    vga_text_height = Yres;

    if (vga_text_x >= vga_text_width || vga_text_y >= vga_text_height)
        VGA_SetCursorPos(0,0);

    if(vga_text_console) {
        COORD size;
        size.X = Xres;
        size.Y = Yres;
        SetConsoleScreenBufferSize( VGA_AlphaConsole(), size );

        /* poll every 30ms (33fps should provide adequate responsiveness) */
        VGA_InstallTimer(30);
    }
}

/**********************************************************************
 *         VGA_InitAlphaMode
 *
 * Initialize VGA text mode handling and return default text mode.
 * This function does not set VGA emulation to text mode.
 */
void VGA_InitAlphaMode(unsigned*Xres,unsigned*Yres)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    if(GetConsoleScreenBufferInfo( VGA_AlphaConsole(), &info ))
    {
        vga_text_console = TRUE;
        vga_text_x = info.dwCursorPosition.X;
        vga_text_y = info.dwCursorPosition.Y;
        vga_text_attr = info.wAttributes;
        *Xres = info.dwSize.X;
        *Yres = info.dwSize.Y;
    } 
    else
    {
        vga_text_console = FALSE;
        vga_text_x = 0;
        vga_text_y = 0;
        vga_text_attr = 0x0f;
        *Xres = 80;
        *Yres = 25;
    }
}

/**********************************************************************
 *         VGA_GetAlphaMode
 *
 * Get current text mode. Returns TRUE and sets resolution if
 * any VGA text mode has been initialized.
 */
BOOL VGA_GetAlphaMode(unsigned*Xres,unsigned*Yres)
{
    if (vga_text_width != 0 && vga_text_height != 0) {
        *Xres = vga_text_width;
        *Yres = vga_text_height;
        return TRUE;
    } else
        return FALSE;
}

void VGA_SetCursorShape(unsigned char start_options, unsigned char end)
{
    CONSOLE_CURSOR_INFO cci;

    /* standard cursor settings:
     * 0x0607 == CGA, 0x0b0c == monochrome, 0x0d0e == EGA/VGA */

    /* calculate percentage from bottom - assuming VGA (bottom 0x0e) */
    cci.dwSize = ((end & 0x1f) - (start_options & 0x1f))/0x0e * 100;
    if (!cci.dwSize) cci.dwSize++; /* NULL cursor would make SCCI() fail ! */
    cci.bVisible = ((start_options & 0x60) != 0x20); /* invisible ? */

    SetConsoleCursorInfo(VGA_AlphaConsole(),&cci);
}

void VGA_SetCursorPos(unsigned X,unsigned Y)
{
    vga_text_x = X;
    vga_text_y = Y;
}

void VGA_GetCursorPos(unsigned*X,unsigned*Y)
{
    if (X) *X = vga_text_x;
    if (Y) *Y = vga_text_y;
}

static void VGA_PutCharAt(unsigned x, unsigned y, BYTE ascii, int attr)
{
    char *dat = VGA_AlphaBuffer() + ((vga_text_width * y + x) * 2);
    dat[0] = ascii;
    if (attr>=0)
        dat[1] = attr;
}

void VGA_WriteChars(unsigned X,unsigned Y,unsigned ch,int attr,int count)
{
    EnterCriticalSection(&vga_lock);

    while (count--) 
        VGA_PutCharAt(X + count, Y, ch, attr);

    LeaveCriticalSection(&vga_lock);
}

void VGA_PutChar(BYTE ascii)
{
    DWORD w;

    EnterCriticalSection(&vga_lock);

    switch(ascii) {
    case '\b':
        if (vga_text_x)
        {
            vga_text_x--;
            VGA_PutCharAt(vga_text_x, vga_text_y, ' ', 0);
        }
        break;

    case '\t':
        vga_text_x += ((vga_text_x + 8) & ~7) - vga_text_x;
        break;

    case '\n':
        vga_text_y++;
        vga_text_x = 0;
        break;

    case '\a':
        break;

    case '\r':
        vga_text_x = 0;
        break;

    default:
        VGA_PutCharAt(vga_text_x, vga_text_y, ascii, vga_text_attr);
        vga_text_x++;
    }

    if (vga_text_x >= vga_text_width)
    {
        vga_text_x = 0;
        vga_text_y++;
    }

    if (vga_text_y >= vga_text_height)
    {
        vga_text_y = vga_text_height - 1;
        VGA_ScrollUpText( 0, 0, 
                          vga_text_height - 1, vga_text_width - 1, 
                          1, vga_text_attr );
    }

    /*
     * If we don't have a console, write directly to standard output.
     */
    if(!vga_text_console)
        WriteFile(VGA_AlphaConsole(), &ascii, 1, &w, NULL);

    LeaveCriticalSection(&vga_lock);
}

void VGA_SetTextAttribute(BYTE attr)
{
    vga_text_attr = attr;
}

void VGA_ClearText(unsigned row1, unsigned col1,
                   unsigned row2, unsigned col2,
                   BYTE attr)
{
    unsigned x, y;

    EnterCriticalSection(&vga_lock);

    for(y=row1; y<=row2; y++)
        for(x=col1; x<=col2; x++)
            VGA_PutCharAt(x, y, 0x20, attr);

    LeaveCriticalSection(&vga_lock);
}

void VGA_ScrollUpText(unsigned row1,  unsigned col1,
                      unsigned row2,  unsigned col2,
                      unsigned lines, BYTE attr)
{
    char    *buffer = VGA_AlphaBuffer();
    unsigned y;

    EnterCriticalSection(&vga_lock);

    /*
     * Scroll buffer.
     */
    for (y = row1; y <= row2 - lines; y++)
        memmove( buffer + col1 + y * vga_text_width * 2,
                 buffer + col1 + (y + lines) * vga_text_width * 2,
                 (col2 - col1 + 1) * 2 );

    /*
     * Fill exposed lines.
     */
    for (y = max(row1, row2 - lines + 1); y <= row2; y++)
        VGA_WriteChars( col1, y, ' ', attr, col2 - col1 + 1 );

    LeaveCriticalSection(&vga_lock);
}

void VGA_ScrollDownText(unsigned row1,  unsigned col1,
                        unsigned row2,  unsigned col2,
                        unsigned lines, BYTE attr)
{
    char    *buffer = VGA_AlphaBuffer();
    unsigned y;

    EnterCriticalSection(&vga_lock);

    /*
     * Scroll buffer.
     */
    for (y = row2; y >= row1 + lines; y--)
        memmove( buffer + col1 + y * vga_text_width * 2,
                 buffer + col1 + (y - lines) * vga_text_width * 2,
                 (col2 - col1 + 1) * 2 );

    /*
     * Fill exposed lines.
     */
    for (y = row1; y <= min(row1 + lines - 1, row2); y++)
        VGA_WriteChars( col1, y, ' ', attr, col2 - col1 + 1 );

    LeaveCriticalSection(&vga_lock);
}

void VGA_GetCharacterAtCursor(BYTE *ascii, BYTE *attr)
{
    char *dat;

    dat = VGA_AlphaBuffer() + ((vga_text_width * vga_text_y + vga_text_x) * 2);

    *ascii = dat[0];
    *attr = dat[1];
}


/*** CONTROL ***/

/* FIXME: optimize by doing this only if the data has actually changed
 *        (in a way similar to DIBSection, perhaps) */
static void VGA_Poll_Graphics(void)
{
  unsigned int Pitch, Height, Width, X, Y;
  char *surf;
  char *dat = vga_fb_data + vga_fb_offset;
  int   bpp = (vga_fb_depth + 7) / 8;

  surf = VGA_Lock(&Pitch,&Height,&Width,NULL);
  if (!surf) return;

  /*
   * Synchronize framebuffer contents.
   */
  if (vga_fb_window != -1)
      VGA_SyncWindow( TRUE );

  /*
   * Double VGA framebuffer (320x200 -> 640x400), if needed.
   */
  if(Height >= 2 * vga_fb_height && Width >= 2 * vga_fb_width && bpp == 1)
    for (Y=0; Y<vga_fb_height; Y++,surf+=Pitch*2,dat+=vga_fb_pitch)
      for (X=0; X<vga_fb_width; X++) {
       BYTE value = dat[X];
       surf[X*2] = value;
       surf[X*2+1] = value;
       surf[X*2+Pitch] = value;
       surf[X*2+Pitch+1] = value;
      }
  else
    for (Y=0; Y<vga_fb_height; Y++,surf+=Pitch,dat+=vga_fb_pitch)
      memcpy(surf, dat, vga_fb_width * bpp);

  VGA_Unlock();
}

static void VGA_Poll_Text(void)
{
    char *dat, *old, *p_line;
    unsigned int X, Y;
    CHAR_INFO ch[256]; /* that should suffice for the largest text width */
    COORD siz, off;
    SMALL_RECT dest;
    HANDLE con = VGA_AlphaConsole();
    BOOL linechanged = FALSE; /* video memory area differs from stored copy? */

    /* Synchronize cursor position. */
    off.X = vga_text_x;
    off.Y = vga_text_y;
    SetConsoleCursorPosition(con,off);

    dat = VGA_AlphaBuffer();
    old = vga_text_old; /* pointer to stored video mem copy */
    siz.X = vga_text_width; siz.Y = 1;
    off.X = 0; off.Y = 0;

    /* copy from virtual VGA frame buffer to console */
    for (Y=0; Y<vga_text_height; Y++) {
	linechanged = memcmp(dat, old, vga_text_width*2);
	if (linechanged)
	{
	    /*TRACE("line %d changed\n", Y);*/
	    p_line = dat;
            for (X=0; X<vga_text_width; X++) {
                ch[X].Char.AsciiChar = *p_line++;
                /* WriteConsoleOutputA doesn't like "dead" chars */
                if (ch[X].Char.AsciiChar == '\0')
                    ch[X].Char.AsciiChar = ' ';
                ch[X].Attributes = *p_line++;
            }
            dest.Top=Y; dest.Bottom=Y;
            dest.Left=0; dest.Right=vga_text_width+1;
            WriteConsoleOutputA(con, ch, siz, off, &dest);
	    memcpy(old, dat, vga_text_width*2);
	}
	/* advance to next text line */
	dat += vga_text_width*2;
	old += vga_text_width*2;
    }
}

static void CALLBACK VGA_Poll( LPVOID arg, DWORD low, DWORD high )
{
    EnterCriticalSection(&vga_lock);

    if (lpddraw)
        VGA_Poll_Graphics();
    else
        VGA_Poll_Text();

    /*
     * Fake start of retrace.
     */
    vga_retrace_vertical = TRUE;

    LeaveCriticalSection(&vga_lock);
}

static BYTE palreg,palcnt;
static PALETTEENTRY paldat;

void VGA_ioport_out( WORD port, BYTE val )
{
    switch (port) {
        case 0x3c0:
           if (vga_address_3c0)
               vga_index_3c0 = val;
           else
               FIXME("Unsupported index, register 0x3c0: 0x%02x (value 0x%02x)\n",
                     vga_index_3c0, val);
           vga_address_3c0 = !vga_address_3c0;
           break;
        case 0x3c4:
           vga_index_3c4 = val;
           break;
        case 0x3c5:
          switch(vga_index_3c4) {
               case 0x04: /* Sequencer: Memory Mode Register */
                  if(vga_fb_depth == 8)
                      VGA_SetWindowStart((val & 8) ? 0 : -1);
                  else
                      FIXME("Memory Mode Register not supported in this mode.\n");
               break;
               default:
                  FIXME("Unsupported index, register 0x3c4: 0x%02x (value 0x%02x)\n",
                        vga_index_3c4, val);
           }
           break;
        case 0x3c8:
            palreg=val; palcnt=0; break;
        case 0x3c9:
            ((BYTE*)&paldat)[palcnt++]=val << 2;
            if (palcnt==3) {
                VGA_SetPalette(&paldat,palreg++,1);
                palcnt=0;
            }
            break;
        case 0x3ce:
            vga_index_3ce = val;
           break;
        case 0x3cf:
           FIXME("Unsupported index, register 0x3ce: 0x%02x (value 0x%02x)\n",
                 vga_index_3ce, val);
           break;
        case 0x3d4:
           vga_index_3d4 = val;
           break;
        case 0x3d5:
           FIXME("Unsupported index, register 0x3d4: 0x%02x (value 0x%02x)\n",
                 vga_index_3d4, val);
           break;
        default:
            FIXME("Unsupported VGA register: 0x%04x (value 0x%02x)\n", port, val);
    }
}

BYTE VGA_ioport_in( WORD port )
{
    BYTE ret;

    switch (port) {
        case 0x3c1:
           FIXME("Unsupported index, register 0x3c0: 0x%02x\n",
                 vga_index_3c0);
           return 0xff;
        case 0x3c5:
           switch(vga_index_3c4) {
               case 0x04: /* Sequencer: Memory Mode Register */
                    return (VGA_GetWindowStart() == -1) ? 0xf7 : 0xff;
               default:
                   FIXME("Unsupported index, register 0x3c4: 0x%02x\n",
                         vga_index_3c4);
                   return 0xff;
           }
        case 0x3cf:
           FIXME("Unsupported index, register 0x3ce: 0x%02x\n",
                 vga_index_3ce);
           return 0xff;
        case 0x3d5:
           FIXME("Unsupported index, register 0x3d4: 0x%02x\n",
                 vga_index_3d4);
           return 0xff;

        case 0x3da:
            /*
             * Read from this register resets register 0x3c0 address flip-flop.
             */
            vga_address_3c0 = TRUE;

            /*
             * Read from this register returns following bits:
             *   xxxx1xxx = Vertical retrace in progress if set.
             *   xxxxx1xx = Light pen switched on.
             *   xxxxxx1x = Light pen trigger set.
             *   xxxxxxx1 = Either vertical or horizontal retrace 
             *              in progress if set.
             */
            ret = 0;
            if (vga_retrace_vertical)
                ret |= 9;
            if (vga_retrace_horizontal)
                ret |= 3;
            
            /*
             * If VGA mode has been set, vertical retrace is
             * turned on once a frame and cleared after each read.
             * This might cause applications that synchronize with
             * vertical retrace to actually skip one frame but that
             * is probably not a problem.
             * 
             * If no VGA mode has been set, vertical retrace is faked
             * by toggling the value after every read.
             */
            if (VGA_IsTimerRunning())
                vga_retrace_vertical = FALSE;
            else
                vga_retrace_vertical = !vga_retrace_vertical;

            /*
             * Toggle horizontal retrace.
             */
            vga_retrace_horizontal = !vga_retrace_horizontal;
            break;

        default:
            ret=0xff;
            FIXME("Unsupported VGA register: 0x%04x\n", port);
    }
    return ret;
}

void VGA_Clean(void)
{
    VGA_Exit();
    VGA_DeinstallTimer();
}
