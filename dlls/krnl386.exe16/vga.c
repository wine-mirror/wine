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
 * Size and location of CGA controller window to framebuffer.
 */
#define CGA_WINDOW_SIZE  (32 * 1024)
#define CGA_WINDOW_START ((char *)0xb8000)

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
static int   vga_fb_window_size;
static char *vga_fb_window_data;
static PALETTEENTRY *vga_fb_palette;
static unsigned vga_fb_palette_index;
static unsigned vga_fb_palette_size;
static BOOL  vga_fb_bright;
static BOOL  vga_fb_enabled;

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
 * List of supported video modes.
 *
 * should be expanded to contain most or all information
 * as required for SVGA VESA mode info block for VESA BIOS subsystem 1 (see _ModeInfoBlock)
 * this will allow proper support for FIXME items in VGA/VESA mode configuration
 *
 * FIXME - verify and define support for VESA modes
 * FIXME - add # bit planes, # video memory banks, & memory model
 */
static WORD VGA_CurrentMode;
static BOOL CGA_ColorComposite = FALSE;  /* behave like composite monitor */

const VGA_MODE VGA_modelist[] =
{
    /* Mode, ModeType, TextCols, TextRows, CharWidth, CharHeight, Width, Height, Depth, Colors, ScreenPages, Supported*/
    /* VGA modes */
    {0x0000,    TEXT, 40, 25,  9, 16,  360,  400,  0,  16, 8, TRUE},   /* VGA/CGA text mode 0 */
    {0x0001,    TEXT, 40, 25,  9, 16,  360,  400,  0,  16, 8, TRUE},   /* VGA/CGA text mode 1 */
    {0x0002,    TEXT, 80, 25,  9, 16,  360,  400,  0,  16, 8, TRUE},   /* VGA/CGA text mode 2 */
    {0x0003,    TEXT, 80, 25,  9, 16,  360,  400,  0,  16, 8, TRUE},   /* VGA/CGA text mode 3 */
    {0x0004, GRAPHIC, 40, 25,  8,  8,  320,  200,  2,   4, 1, TRUE},   /* VGA/CGA graphics mode 4 */
    {0x0005, GRAPHIC, 40, 25,  8,  8,  320,  200,  2,   4, 1, TRUE},   /* VGA/CGA graphics mode 5 */
    {0x0006, GRAPHIC, 80, 25,  8,  8,  640,  200,  1,   2, 1, TRUE},   /* VGA/CGA graphics mode 6 */
    {0x0007,    TEXT, 80, 25,  9, 16,  720,  400,  0,   0, 8, FALSE},   /* VGA text mode 7 - FIXME bad default address */
    {0x000d, GRAPHIC, 40, 25,  8,  8,  320,  200,  4,  16, 8, FALSE},   /* VGA graphics mode 13 */
    {0x000e, GRAPHIC, 80, 25,  8,  8,  640,  200,  4,  16, 4, FALSE},   /* VGA graphics mode 14 */
    {0x000f, GRAPHIC, 80, 25,  8, 14,  640,  350,  0,   0, 2, FALSE},   /* VGA graphics mode 15 */
    {0x0010, GRAPHIC, 80, 25,  8, 14,  640,  350,  4,  16, 2, FALSE},   /* VGA graphics mode 16 */
    {0x0011, GRAPHIC, 80, 30,  8, 16,  640,  480,  1,   2, 1, FALSE},   /* VGA graphics mode 17 */
    {0x0012, GRAPHIC, 80, 30,  8, 16,  640,  480,  4,  16, 1, FALSE},   /* VGA graphics mode 18 */
    {0x0013, GRAPHIC, 40, 25,  8,  8,  320,  200,  8, 256, 1, TRUE},   /* VGA graphics mode 19 */
    /* VESA 7-bit modes */
    {0x006a, GRAPHIC,  0,  0,  0,  0,  800,  600,  4,  16, 1, TRUE},   /* VESA graphics mode, same as 0x102 */
    /* VESA 15-bit modes */
    {0x0100, GRAPHIC,  0,  0,  0,  0,  640,  400,  8, 256, 1, TRUE},   /* VESA graphics mode */
    {0x0101, GRAPHIC,  0,  0,  0,  0,  640,  480,  8, 256, 1, TRUE},   /* VESA graphics mode */
    {0x0102, GRAPHIC,  0,  0,  0,  0,  800,  600,  4,  16, 1, TRUE},   /* VESA graphics mode */
    {0x0103, GRAPHIC,  0,  0,  0,  0,  800,  600,  8, 256, 1, TRUE},   /* VESA graphics mode */
    {0x0104, GRAPHIC,  0,  0,  0,  0, 1024,  768,  4,  16, 1, TRUE},   /* VESA graphics mode */
    {0x0105, GRAPHIC,  0,  0,  0,  0, 1024,  768,  8, 256, 1, TRUE},   /* VESA graphics mode */
    {0x0106, GRAPHIC,  0,  0,  0,  0, 1280, 1024,  4,  16, 1, TRUE},   /* VESA graphics mode */
    {0x0107, GRAPHIC,  0,  0,  0,  0, 1280, 1024,  8, 256, 1, TRUE},   /* VESA graphics mode */
    {0x0108,    TEXT,  0,  0,  0,  0,   80,   60,  0,   0, 1, TRUE},   /* VESA text mode */
    {0x0109,    TEXT,  0,  0,  0,  0,  132,   25,  0,   0, 1, TRUE},   /* VESA text mode */
    {0x010a,    TEXT,  0,  0,  0,  0,  132,   43,  0,   0, 1, TRUE},   /* VESA text mode */
    {0x010b,    TEXT,  0,  0,  0,  0,  132,   50,  0,   0, 1, TRUE},   /* VESA text mode */
    {0x010c,    TEXT,  0,  0,  0,  0,  132,   60,  0,   0, 1, TRUE},   /* VESA text mode */
    /* VESA 1.2 modes */
    {0x010d, GRAPHIC,  0,  0,  0,  0,  320,  200, 15,   0, 1, TRUE},   /* VESA graphics mode, 32K colors */
    {0x010e, GRAPHIC,  0,  0,  0,  0,  320,  200, 16,   0, 1, TRUE},   /* VESA graphics mode, 64K colors */
    {0x010f, GRAPHIC,  0,  0,  0,  0,  320,  200, 24,   0, 1, TRUE},   /* VESA graphics mode, 16.8 Million colors */
    {0x0110, GRAPHIC,  0,  0,  0,  0,  640,  480, 15,   0, 1, TRUE},   /* VESA graphics mode, 32K colors */
    {0x0111, GRAPHIC,  0,  0,  0,  0,  640,  480, 16,   0, 1, TRUE},   /* VESA graphics mode, 64K colors */
    {0x0112, GRAPHIC,  0,  0,  0,  0,  640,  480, 24,   0, 1, TRUE},   /* VESA graphics mode, 16.8 Million colors */
    {0x0113, GRAPHIC,  0,  0,  0,  0,  800,  600, 15,   0, 1, TRUE},   /* VESA graphics mode, 32K colors */
    {0x0114, GRAPHIC,  0,  0,  0,  0,  800,  600, 16,   0, 1, TRUE},   /* VESA graphics mode, 64K colors */
    {0x0115, GRAPHIC,  0,  0,  0,  0,  800,  600, 24,   0, 1, TRUE},   /* VESA graphics mode, 16.8 Million colors */
    {0x0116, GRAPHIC,  0,  0,  0,  0, 1024,  768, 15,   0, 1, TRUE},   /* VESA graphics mode, 32K colors */
    {0x0117, GRAPHIC,  0,  0,  0,  0, 1024,  768, 16,   0, 1, TRUE},   /* VESA graphics mode, 64K colors */
    {0x0118, GRAPHIC,  0,  0,  0,  0, 1024,  768, 24,   0, 1, TRUE},   /* VESA graphics mode, 16.8 Million colors */
    {0x0119, GRAPHIC,  0,  0,  0,  0, 1280, 1024, 15,   0, 1, TRUE},   /* VESA graphics mode, 32K colors */
    {0x011a, GRAPHIC,  0,  0,  0,  0, 1280, 1024, 16,   0, 1, TRUE},   /* VESA graphics mode, 64K colors */
    {0x011b, GRAPHIC,  0,  0,  0,  0, 1280, 1024, 24,   0, 1, TRUE},   /* VESA graphics mode, 16.8 Million colors */
    {0xffff,    TEXT,  0,  0,  0,  0,    0,    0,  0,   0, 1, FALSE}
};

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

static void CALLBACK VGA_Poll( LPVOID arg, DWORD low, DWORD high );

static HWND vga_hwnd = NULL;

/*
 * CGA palette 1
 */
static PALETTEENTRY cga_palette1[] = {
  {0x00, 0x00, 0x00}, /* 0 - Black */
  {0x00, 0xAA, 0xAA}, /* 1 - Cyan */
  {0xAA, 0x00, 0xAA}, /* 2 - Magenta */
  {0xAA, 0xAA, 0xAA}  /* 3 - White */
};

/*
 * CGA palette 1 in the bright variant
 * intensities, when signalled to do so
 * in register 0x3d9
 */
static PALETTEENTRY cga_palette1_bright[] = {
  {0x00, 0x00, 0x00}, /* 0 - Black */
  {0x55, 0xFF, 0xFF}, /* 1 - Light cyan */
  {0xFF, 0x55, 0xFF}, /* 2 - Light magenta */
  {0xFF, 0xFF, 0xFF}, /* 3 - Bright White */
};

/*
 * CGA palette 2
 */
static PALETTEENTRY cga_palette2[] = {
  {0x00, 0x00, 0x00}, /* 0 - Black */
  {0x00, 0xAA, 0x00}, /* 1 - Green */
  {0xAA, 0x00, 0x00}, /* 2 - Red */
  {0xAA, 0x55, 0x00}  /* 3 - Brown */
};

/*
 * CGA palette 2 in the bright variant
 * intensities, when signalled to do so
 * in register 0x3d9
 */
static PALETTEENTRY cga_palette2_bright[] = {
  {0x00, 0x00, 0x00}, /* 0 - Black */
  {0x55, 0xFF, 0x55}, /* 1 - Light green */
  {0xFF, 0x55, 0x55}, /* 2 - Light red */
  {0xFF, 0xFF, 0x55}, /* 3 - Yellow */
};

/*
 *  VGA Palette Registers, in actual 16 bit color
 *  port 3C0H - 6 bit rgbRGB format
 *
 *  16 color accesses will use these pointers and insert
 *  entries from the 64-color palette (mode 18) into the default
 *  palette.   --Robert 'Admiral' Coeyman
 */
static char vga_16_palette[17]={
  0x00,  /* 0 - Black         */
  0x01,  /* 1 - Blue          */
  0x02,  /* 2 - Green         */
  0x03,  /* 3 - Cyan          */
  0x04,  /* 4 - Red           */
  0x05,  /* 5 - Magenta       */
  0x14,  /* 6 - Brown         */
  0x07,  /* 7 - White (Light gray)        */
  0x38,  /* 8 - Dark gray     */
  0x39,  /* 9 - Light blue    */
  0x3a,  /* A - Light green   */
  0x3b,  /* B - Light cyan    */
  0x3c,  /* C - Light red     */
  0x3d,  /* D - Light magenta */
  0x3e,  /* E - Yellow        */
  0x3f,  /* F - Bright White (White) */
  0x00   /* Border Color      */
};

/*
 *  Mode 19 Default Color Register Setting
 *  DAC palette registers, converted from actual 18 bit color to 24.
 */
static PALETTEENTRY vga_def_palette[256]={
/* red  green  blue */
  /* 16 colors in IRGB values */
  {0x00, 0x00, 0x00}, /* 0 (0) - Black */
  {0x00, 0x00, 0xAA}, /* 1 (1) - Blue */
  {0x00, 0xAA, 0x00}, /* 2 (2) - Green */
  {0x00, 0xAA, 0xAA}, /* 3 (3) - Cyan */
  {0xAA, 0x00, 0x00}, /* 4 (4) - Red */
  {0xAA, 0x00, 0xAA}, /* 5 (5) - Magenta */
  {0xAA, 0x55, 0x00}, /* 6 (6) - Brown */
  {0xAA, 0xAA, 0xAA}, /* 7 (7) - White (Light gray) */
  {0x55, 0x55, 0x55}, /* 8 (8) - Dark gray */
  {0x55, 0x55, 0xFF}, /* 9 (9) - Light blue */
  {0x55, 0xFF, 0x55}, /* 10 (A) - Light green */
  {0x55, 0xFF, 0xFF}, /* 11 (B) - Light cyan */
  {0xFF, 0x55, 0x55}, /* 12 (C) - Light red */
  {0xFF, 0x55, 0xFF}, /* 13 (D) - Light magenta */
  {0xFF, 0xFF, 0x55}, /* 14 (E) -  Yellow */
  {0xFF, 0xFF, 0xFF}, /* 15 (F) - Bright White (White) */
  /* 16 shades of gray */
  {0x00, 0x00, 0x00}, /* 16 (10) */
  {0x10, 0x10, 0x10}, /* 17 (11) */
  {0x20, 0x20, 0x20}, /* 18 (12) */
  {0x35, 0x35, 0x35}, /* 19 (13) */
  {0x45, 0x45, 0x45}, /* 20 (14) */
  {0x55, 0x55, 0x55}, /* 21 (15) */
  {0x65, 0x65, 0x65}, /* 22 (16) */
  {0x75, 0x75, 0x75}, /* 23 (17) */
  {0x8A, 0x8A, 0x8A}, /* 24 (18) */
  {0x9A, 0x9A, 0x9A}, /* 25 (19) */
  {0xAA, 0xAA, 0xAA}, /* 26 (1A) */
  {0xBA, 0xBA, 0xBA}, /* 27 (1B) */
  {0xCA, 0xCA, 0xCA}, /* 28 (1C) */
  {0xDF, 0xDF, 0xDF}, /* 29 (1D) */
  {0xEF, 0xEF, 0xEF}, /* 30 (1E) */
  {0xFF, 0xFF, 0xFF}, /* 31 (1F) */
  /* High Intensity group - 72 colors in 1/3 saturation groups (20H-37H high) */
  {0x00, 0x00, 0xFF}, /* 32 (20) */
  {0x41, 0x00, 0xFF}, /* 33 (21) */
  {0x82, 0x00, 0xFF}, /* 34 (22) */
  {0xBE, 0x00, 0xFF}, /* 35 (23) */
  {0xFF, 0x00, 0xFF}, /* 36 (24) */
  {0xFF, 0x00, 0xBE}, /* 37 (25) */
  {0xFF, 0x00, 0x82}, /* 38 (26) */
  {0xFF, 0x00, 0x41}, /* 39 (27) */
  {0xFF, 0x00, 0x00}, /* 40 (28) */
  {0xFF, 0x41, 0x00}, /* 41 (29) */
  {0xFF, 0x82, 0x00}, /* 42 (2A) */
  {0xFF, 0xBE, 0x00}, /* 43 (2B) */
  {0xFF, 0xFF, 0x00}, /* 44 (2C) */
  {0xBE, 0xFF, 0x00}, /* 45 (2D) */
  {0x82, 0xFF, 0x00}, /* 46 (2E) */
  {0x41, 0xFF, 0x00}, /* 47 (2F) */
  {0x00, 0xFF, 0x00}, /* 48 (30) */
  {0x00, 0xFF, 0x41}, /* 49 (31) */
  {0x00, 0xFF, 0x82}, /* 50 (32) */
  {0x00, 0xFF, 0xBE}, /* 51 (33) */
  {0x00, 0xFF, 0xFF}, /* 52 (34) */
  {0x00, 0xBE, 0xFF}, /* 53 (35) */
  {0x00, 0x82, 0xFF}, /* 54 (36) */
  {0x00, 0x41, 0xFF}, /* 55 (37) */
  /* High Intensity group - 72 colors in 2/3 saturation groups (38H-4FH moderate) */
  {0x82, 0x82, 0xFF}, /* 56 (38) */
  {0x9E, 0x82, 0xFF}, /* 57 (39) */
  {0xBE, 0x82, 0xFF}, /* 58 (3A) */
  {0xDF, 0x82, 0xFF}, /* 59 (3B) */
  {0xFF, 0x82, 0xFF}, /* 60 (3C) */
  {0xFF, 0x82, 0xDF}, /* 61 (3D) */
  {0xFF, 0x82, 0xBE}, /* 62 (3E) */
  {0xFF, 0x82, 0x9E}, /* 63 (3F) */
  {0xFF, 0x82, 0x82}, /* 64 (40) */
  {0xFF, 0x9E, 0x82}, /* 65 (41) */
  {0xFF, 0xBE, 0x82}, /* 66 (42) */
  {0xFF, 0xDF, 0x82}, /* 67 (43) */
  {0xFF, 0xFF, 0x82}, /* 68 (44) */
  {0xDF, 0xFF, 0x82}, /* 69 (45) */
  {0xBE, 0xFF, 0x82}, /* 70 (46) */
  {0x9E, 0xFF, 0x82}, /* 71 (47) */
  {0x82, 0xFF, 0x82}, /* 72 (48) */
  {0x82, 0xFF, 0x9E}, /* 73 (49) */
  {0x82, 0xFF, 0xBE}, /* 74 (4A) */
  {0x82, 0xFF, 0xDF}, /* 75 (4B) */
  {0x82, 0xFF, 0xFF}, /* 76 (4C) */
  {0x82, 0xDF, 0xFF}, /* 77 (4D) */
  {0x82, 0xBE, 0xFF}, /* 78 (4E) */
  {0x82, 0x9E, 0xFF}, /* 79 (4F) */
  /* High Intensity group - 72 colors in 3/3 saturation groups (50H-67H low) */
  {0xBA, 0xBA, 0xFF}, /* 80 (50) */
  {0xCA, 0xBA, 0xFF}, /* 81 (51) */
  {0xDF, 0xBA, 0xFF}, /* 82 (52) */
  {0xEF, 0xBA, 0xFF}, /* 83 (53) */
  {0xFF, 0xBA, 0xFF}, /* 84 (54) */
  {0xFF, 0xBA, 0xEF}, /* 85 (55) */
  {0xFF, 0xBA, 0xDF}, /* 86 (56) */
  {0xFF, 0xBA, 0xCA}, /* 87 (57) */
  {0xFF, 0xBA, 0xBA}, /* 88 (58) */
  {0xFF, 0xCA, 0xBA}, /* 89 (59) */
  {0xFF, 0xDF, 0xBA}, /* 90 (5A) */
  {0xFF, 0xEF, 0xBA}, /* 91 (5B) */
  {0xFF, 0xFF, 0xBA}, /* 92 (5C) */
  {0xEF, 0xFF, 0xBA}, /* 93 (5D) */
  {0xDF, 0xFF, 0xBA}, /* 94 (5E) */
  {0xCA, 0xFF, 0xBA}, /* 95 (5F) */
  {0xBA, 0xFF, 0xBA}, /* 96 (60) */
  {0xBA, 0xFF, 0xCA}, /* 97 (61) */
  {0xBA, 0xFF, 0xDF}, /* 98 (62) */
  {0xBA, 0xFF, 0xEF}, /* 99 (63) */
  {0xBA, 0xFF, 0xFF}, /* 100 (64) */
  {0xBA, 0xEF, 0xFF}, /* 101 (65) */
  {0xBA, 0xDF, 0xFF}, /* 102 (66) */
  {0xBA, 0xCA, 0xFF}, /* 103 (67) */
  /* Medium Intensity group - 72 colors in 1/3 saturation groups (68H-7FH high) */
  {0x00, 0x00, 0x71}, /* 104 (68) */
  {0x1C, 0x00, 0x71}, /* 105 (69) */
  {0x39, 0x00, 0x71}, /* 106 (6A) */
  {0x55, 0x00, 0x71}, /* 107 (6B) */
  {0x71, 0x00, 0x71}, /* 108 (6C) */
  {0x71, 0x00, 0x55}, /* 109 (6D) */
  {0x71, 0x00, 0x39}, /* 110 (6E) */
  {0x71, 0x00, 0x1C}, /* 111 (6F) */
  {0x71, 0x00, 0x00}, /* 112 (70) */
  {0x71, 0x1C, 0x00}, /* 113 (71) */
  {0x71, 0x39, 0x00}, /* 114 (72) */
  {0x71, 0x55, 0x00}, /* 115 (73) */
  {0x71, 0x71, 0x00}, /* 116 (74) */
  {0x55, 0x71, 0x00}, /* 117 (75) */
  {0x39, 0x71, 0x00}, /* 118 (76) */
  {0x1C, 0x71, 0x00}, /* 119 (77) */
  {0x00, 0x71, 0x00}, /* 120 (78) */
  {0x00, 0x71, 0x1C}, /* 121 (79) */
  {0x00, 0x71, 0x39}, /* 122 (7A) */
  {0x00, 0x71, 0x55}, /* 123 (7B) */
  {0x00, 0x71, 0x71}, /* 124 (7C) */
  {0x00, 0x55, 0x71}, /* 125 (7D) */
  {0x00, 0x39, 0x71}, /* 126 (7E) */
  {0x00, 0x1C, 0x71}, /* 127 (7F) */
  /* Medium Intensity group - 72 colors in 2/3 saturation groups (80H-97H moderate) */
  {0x39, 0x39, 0x71}, /* 128 (80) */
  {0x45, 0x39, 0x71}, /* 129 (81) */
  {0x55, 0x39, 0x71}, /* 130 (82) */
  {0x61, 0x39, 0x71}, /* 131 (83) */
  {0x71, 0x39, 0x71}, /* 132 (84) */
  {0x71, 0x39, 0x61}, /* 133 (85) */
  {0x71, 0x39, 0x55}, /* 134 (86) */
  {0x71, 0x39, 0x45}, /* 135 (87) */
  {0x71, 0x39, 0x39}, /* 136 (88) */
  {0x71, 0x45, 0x39}, /* 137 (89) */
  {0x71, 0x55, 0x39}, /* 138 (8A) */
  {0x71, 0x61, 0x39}, /* 139 (8B) */
  {0x71, 0x71, 0x39}, /* 140 (8C) */
  {0x61, 0x71, 0x39}, /* 141 (8D) */
  {0x55, 0x71, 0x39}, /* 142 (8E) */
  {0x45, 0x71, 0x39}, /* 143 (8F) */
  {0x39, 0x71, 0x39}, /* 144 (90) */
  {0x39, 0x71, 0x45}, /* 145 (91) */
  {0x39, 0x71, 0x55}, /* 146 (92) */
  {0x39, 0x71, 0x61}, /* 147 (93) */
  {0x39, 0x71, 0x71}, /* 148 (94) */
  {0x39, 0x61, 0x71}, /* 149 (95) */
  {0x39, 0x55, 0x71}, /* 150 (96) */
  {0x39, 0x45, 0x71}, /* 151 (97) */
  /* Medium Intensity group - 72 colors in 3/3 saturation groups (98H-AFH low) */
  {0x51, 0x51, 0x71}, /* 152 (98) */
  {0x59, 0x51, 0x71}, /* 153 (99) */
  {0x61, 0x51, 0x71}, /* 154 (9A) */
  {0x69, 0x51, 0x71}, /* 155 (9B) */
  {0x71, 0x51, 0x71}, /* 156 (9C) */
  {0x71, 0x51, 0x69}, /* 157 (9D) */
  {0x71, 0x51, 0x61}, /* 158 (9E) */
  {0x71, 0x51, 0x59}, /* 159 (9F) */
  {0x71, 0x51, 0x51}, /* 160 (A0) */
  {0x71, 0x59, 0x51}, /* 161 (A1) */
  {0x71, 0x61, 0x51}, /* 162 (A2) */
  {0x71, 0x69, 0x51}, /* 163 (A3) */
  {0x71, 0x71, 0x51}, /* 164 (A4) */
  {0x69, 0x71, 0x51}, /* 165 (A5) */
  {0x61, 0x71, 0x51}, /* 166 (A6) */
  {0x59, 0x71, 0x51}, /* 167 (A7) */
  {0x51, 0x71, 0x51}, /* 168 (A8) */
  {0x51, 0x71, 0x59}, /* 169 (A9) */
  {0x51, 0x71, 0x61}, /* 170 (AA) */
  {0x51, 0x71, 0x69}, /* 171 (AB) */
  {0x51, 0x71, 0x71}, /* 172 (AC) */
  {0x51, 0x69, 0x71}, /* 173 (AD) */
  {0x51, 0x61, 0x71}, /* 174 (AE) */
  {0x51, 0x59, 0x71}, /* 175 (AF) */
  /* Low Intensity group - 72 colors in 1/3 saturation groups (B0H-C7H high) */
  {0x00, 0x00, 0x41}, /* 176 (B0) */
  {0x10, 0x00, 0x41}, /* 177 (B1) */
  {0x20, 0x00, 0x41}, /* 178 (B2) */
  {0x31, 0x00, 0x41}, /* 179 (B3) */
  {0x41, 0x00, 0x41}, /* 180 (B4) */
  {0x41, 0x00, 0x31}, /* 181 (B5) */
  {0x41, 0x00, 0x20}, /* 182 (B6) */
  {0x41, 0x00, 0x10}, /* 183 (B7) */
  {0x41, 0x00, 0x00}, /* 184 (B8) */
  {0x41, 0x10, 0x00}, /* 185 (B9) */
  {0x41, 0x20, 0x00}, /* 186 (BA) */
  {0x41, 0x31, 0x00}, /* 187 (BB) */
  {0x41, 0x41, 0x00}, /* 188 (BC) */
  {0x31, 0x41, 0x00}, /* 189 (BD) */
  {0x20, 0x41, 0x00}, /* 190 (BE) */
  {0x10, 0x41, 0x00}, /* 191 (BF) */
  {0x00, 0x41, 0x00}, /* 192 (C0) */
  {0x00, 0x41, 0x10}, /* 193 (C1) */
  {0x00, 0x41, 0x20}, /* 194 (C2) */
  {0x00, 0x41, 0x31}, /* 195 (C3) */
  {0x00, 0x41, 0x41}, /* 196 (C4) */
  {0x00, 0x31, 0x41}, /* 197 (C5) */
  {0x00, 0x20, 0x41}, /* 198 (C6) */
  {0x00, 0x10, 0x41}, /* 199 (C7) */
  /* Low Intensity group - 72 colors in 2/3 saturation groups (C8H-DFH moderate) */
  {0x20, 0x20, 0x41}, /* 200 (C8) */
  {0x28, 0x20, 0x41}, /* 201 (C9) */
  {0x31, 0x20, 0x41}, /* 202 (CA) */
  {0x39, 0x20, 0x41}, /* 203 (CB) */
  {0x41, 0x20, 0x41}, /* 204 (CC) */
  {0x41, 0x20, 0x39}, /* 205 (CD) */
  {0x41, 0x20, 0x31}, /* 206 (CE) */
  {0x41, 0x20, 0x28}, /* 207 (CF) */
  {0x41, 0x20, 0x20}, /* 208 (D0) */
  {0x41, 0x28, 0x20}, /* 209 (D1) */
  {0x41, 0x31, 0x20}, /* 210 (D2) */
  {0x41, 0x39, 0x20}, /* 211 (D3) */
  {0x41, 0x41, 0x20}, /* 212 (D4) */
  {0x39, 0x41, 0x20}, /* 213 (D5) */
  {0x31, 0x41, 0x20}, /* 214 (D6) */
  {0x28, 0x41, 0x20}, /* 215 (D7) */
  {0x20, 0x41, 0x20}, /* 216 (D8) */
  {0x20, 0x41, 0x28}, /* 217 (D9) */
  {0x20, 0x41, 0x31}, /* 218 (DA) */
  {0x20, 0x41, 0x39}, /* 219 (DB) */
  {0x20, 0x41, 0x41}, /* 220 (DC) */
  {0x20, 0x39, 0x41}, /* 221 (DD) */
  {0x20, 0x31, 0x41}, /* 222 (DE) */
  {0x20, 0x28, 0x41}, /* 223 (DF) */
  /* Low Intensity group - 72 colors in 3/3 saturation groups (E0H-F7H low) */
  {0x2D, 0x2D, 0x41}, /* 223 (E0) */
  {0x31, 0x2D, 0x41}, /* 224 (E1) */
  {0x35, 0x2D, 0x41}, /* 225 (E2) */
  {0x3D, 0x2D, 0x41}, /* 226 (E3) */
  {0x41, 0x2D, 0x41}, /* 227 (E4) */
  {0x41, 0x2D, 0x3D}, /* 228 (E5) */
  {0x41, 0x2D, 0x35}, /* 229 (E6) */
  {0x41, 0x2D, 0x31}, /* 230 (E7) */
  {0x41, 0x2D, 0x2D}, /* 231 (E8) */
  {0x41, 0x31, 0x2D}, /* 232 (E9) */
  {0x41, 0x35, 0x2D}, /* 233 (EA) */
  {0x41, 0x3D, 0x2D}, /* 234 (EB) */
  {0x41, 0x41, 0x2D}, /* 235 (EC) */
  {0x3D, 0x41, 0x2D}, /* 236 (ED) */
  {0x35, 0x41, 0x2D}, /* 237 (EE) */
  {0x31, 0x41, 0x2D}, /* 238 (EF) */
  {0x2D, 0x41, 0x2D}, /* 239 (F0) */
  {0x2D, 0x41, 0x31}, /* 240 (F1) */
  {0x2D, 0x41, 0x35}, /* 241 (F2) */
  {0x2D, 0x41, 0x3D}, /* 242 (F3) */
  {0x2D, 0x41, 0x41}, /* 243 (F4) */
  {0x2D, 0x3D, 0x41}, /* 244 (F5) */
  {0x2D, 0x35, 0x41}, /* 245 (F6) */
  {0x2D, 0x31, 0x41}, /* 246 (F7) */
  /* Fill up remainder of palettes with black */
  {0,0,0}
};

/*
 *  Mode 18 Default Color Register Setting
 *  DAC palette registers, converted from actual 18 bit color to 24.
 *
 *  This palette is the dos default, converted from 18 bit color to 24.
 *  It contains only 64 entries of colors--all others are zeros.
 *      --Robert 'Admiral' Coeyman
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
  {0xaa, 0xaa, 0xaa}, /* 0x07      White (Light Gray) */
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
    return 0;
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
    return VGA_timer_thread != 0;
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
  BOOL ret;
} ModeSet;


/**********************************************************************
 *         VGA_SyncWindow
 *
 * Copy VGA window into framebuffer (if argument is TRUE) or
 * part of framebuffer into VGA window (if argument is FALSE).
 */
static void VGA_SyncWindow( BOOL target_is_fb )
{
    int size = vga_fb_window_size;

    /* Window does not overlap framebuffer. */
    if (vga_fb_window >= vga_fb_size)
        return;

    /* Check if window overlaps framebuffer only partially. */
    if (vga_fb_size - vga_fb_window < vga_fb_window_size)
        size = vga_fb_size - vga_fb_window;

    if (target_is_fb)
        memmove( vga_fb_data + vga_fb_window, vga_fb_window_data, size );
    else
        memmove( vga_fb_window_data, vga_fb_data + vga_fb_window, size );
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
    HRESULT	res;
    ModeSet *par = (ModeSet *)arg;
    par->ret = FALSE;

    if (lpddraw) VGA_DoExit(0);
    if (!lpddraw) {
        res = DirectDrawCreate(NULL,&lpddraw,NULL);
        if (!lpddraw) {
            ERR("DirectDraw is not available (res = 0x%x)\n",res);
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

        res=IDirectDraw_SetCooperativeLevel(lpddraw,vga_hwnd,DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE);
        if (res != S_OK) {
	    ERR("Could not set cooperative level to exclusive (0x%x)\n",res);
	}

        res=IDirectDraw_SetDisplayMode(lpddraw,par->Xres,par->Yres,par->Depth);
        if (res != S_OK) {
            ERR("DirectDraw does not support requested display mode (%dx%dx%d), res = 0x%x!\n",par->Xres,par->Yres,par->Depth,res);
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return;
        }

        res=IDirectDraw_CreatePalette(lpddraw,DDPCAPS_8BIT,NULL,&lpddpal,NULL);
        if (res != S_OK) {
	    ERR("Could not create palette (res = 0x%x)\n",res);
            IDirectDraw_Release(lpddraw);
            lpddraw=NULL;
            return;
        }

        res=IDirectDrawPalette_SetEntries(lpddpal,0,0,vga_fb_palette_size,vga_fb_palette);
        if (res != S_OK) {
           ERR("Could not set default palette entries (res = 0x%x)\n", res);
        }

        memset(&sdesc,0,sizeof(sdesc));
        sdesc.dwSize=sizeof(sdesc);
	sdesc.dwFlags = DDSD_CAPS;
	sdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        res=IDirectDraw_CreateSurface(lpddraw,&sdesc,&lpddsurf,NULL);
        if (res != S_OK || !lpddsurf) {
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
    par->ret = TRUE;
    return;
}

/**********************************************************************\
* VGA_GetModeInfo
*
* Returns mode information, given mode number
\**********************************************************************/
const VGA_MODE *VGA_GetModeInfo(WORD mode)
{
    const VGA_MODE *ModeInfo = VGA_modelist;

    /*
     * Filter out flags.
     */
    mode &= 0x17f;

    while (ModeInfo->Mode != 0xffff)
    {
        if (ModeInfo->Mode == mode)
            return ModeInfo;
        ModeInfo++;
    }
    return NULL;
}

static BOOL VGA_SetGraphicMode(WORD mode)
{
    ModeSet par;
    int     newSize;

    /* get info on VGA mode & set appropriately */
    const VGA_MODE *ModeInfo = VGA_GetModeInfo(VGA_CurrentMode);
    /* check if we're assuming composite display */
    if ((mode == 6) && (CGA_ColorComposite))
    {
       vga_fb_width = (ModeInfo->Width / 4);
       vga_fb_height = ModeInfo->Height;
       vga_fb_depth = (ModeInfo->Depth * 4);
    }
    else
    {
       vga_fb_width = ModeInfo->Width;
       vga_fb_height = ModeInfo->Height;
       vga_fb_depth = ModeInfo->Depth;
    }
    vga_fb_offset = 0;
    vga_fb_pitch = vga_fb_width * ((vga_fb_depth + 7) / 8);

    newSize = vga_fb_width * vga_fb_height * ((vga_fb_depth + 7) / 8);
    if(newSize < 256 * 1024)
      newSize = 256 * 1024;

    if(vga_fb_size < newSize) {
      HeapFree(GetProcessHeap(), 0, vga_fb_data);
      vga_fb_data = HeapAlloc(GetProcessHeap(), 0, newSize);
      vga_fb_size = newSize;
    }

    if(vga_fb_width >= 640 || vga_fb_height >= 480) {
      par.Xres = vga_fb_width;
      par.Yres = vga_fb_height;
    } else {
      par.Xres = 640;
      par.Yres = 480;
    }

    /* Setup window */
    if(vga_fb_depth >= 8)
    {
      vga_fb_window_data = VGA_WINDOW_START;
      vga_fb_window_size = VGA_WINDOW_SIZE;
      vga_fb_palette = vga_def_palette;
      vga_fb_palette_size = 256;
    }
    else
    {
      vga_fb_window_data = CGA_WINDOW_START;
      vga_fb_window_size = CGA_WINDOW_SIZE;
      if(vga_fb_depth == 2)
      {
        /* Select default 2 bit CGA palette */
        vga_fb_palette = cga_palette1;
        vga_fb_palette_size = 4;
      }
      else
      {
        /* Top of VGA palette is same as 4 bit CGA palette */
        vga_fb_palette = vga_def_palette;
        vga_fb_palette_size = 16;
      }

      vga_fb_palette_index = 0;
      vga_fb_bright = 0;
    }

    /* Clean the HW buffer */
    memset(vga_fb_window_data, 0x00, vga_fb_window_size);

    /* Reset window start */
    VGA_SetWindowStart(0);

    par.Depth = (vga_fb_depth < 8) ? 8 : vga_fb_depth;

    MZ_RunInThread(VGA_DoSetMode, (ULONG_PTR)&par);
    return par.ret;
}

BOOL VGA_SetMode(WORD mode)
{
    const VGA_MODE *ModeInfo;
    /* get info on VGA mode & set appropriately */
    VGA_CurrentMode = mode;
    ModeInfo = VGA_GetModeInfo(VGA_CurrentMode);

    /* check if mode is supported */
    if (ModeInfo->Supported)
    {
       FIXME("Setting VGA mode %i - Supported mode - Improve reporting of missing capabilities for modes & modetypes.\n", mode);
    }
    else
    {
       FIXME("Setting VGA mode %i - Unsupported mode - Will doubtfully work at all, but we'll try anyways.\n", mode);
    }

    /* set up graphic or text display */
    if (ModeInfo->ModeType == TEXT)
    {
       VGA_SetAlphaMode(ModeInfo->TextCols, ModeInfo->TextRows);
    }
    else
    {
       return VGA_SetGraphicMode(mode);
    }
    return TRUE; /* assume all good & return TRUE */
}

BOOL VGA_GetMode(unsigned *Height, unsigned *Width, unsigned *Depth)
{
    if (!lpddraw) return FALSE;
    if (!lpddsurf) return FALSE;
    if (Height) *Height=sdesc.dwHeight;
    if (Width) *Width=sdesc.dwWidth;
    if (Depth) *Depth=sdesc.ddpfPixelFormat.u1.dwRGBBitCount;
    return TRUE;
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
    return vga_16_palette[reg];
}

/* set all 17 [char wide] colors at once in 16 color mode. */
void VGA_Set16Palette(char *Table)
{
	PALETTEENTRY *pal;
	int c;

    if (!lpddraw) return;         /* return if we're in text only mode */
    memcpy( Table, vga_16_palette, 17 ); /* copy the entries into the table */

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
    memcpy( vga_16_palette, Table, 17 ); /* copy the entries into the table */
}

static LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth)
{
    if (!lpddraw) return NULL;
    if (!lpddsurf) return NULL;
    if (IDirectDrawSurface_Lock(lpddsurf,NULL,&sdesc,0,0) != S_OK) {
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
static void WINAPI VGA_DoShowMouse( ULONG_PTR show )
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


/**********************************************************************
 *         VGA_UpdatePalette
 *
 * Update the current palette
 *
 * Note: When updating the current CGA palette, palette index 0
 * refers to palette2, and index 1 is palette1 (default palette)
 */
void VGA_UpdatePalette(void)
{
  /* Figure out which palette is used now */
  if(vga_fb_bright)
  {
    if(vga_fb_palette_index == 0)
    {
      vga_fb_palette = cga_palette2_bright;
    }
    else if(vga_fb_palette_index == 1)
    {
      vga_fb_palette = cga_palette1_bright;
    }
  }
  else
  {
    if(vga_fb_palette_index == 0)
    {
      vga_fb_palette = cga_palette2;
    }
    else if(vga_fb_palette_index == 1)
    {
      vga_fb_palette = cga_palette1;
    }
  }

  /* Now update the palette */
  VGA_SetPalette(vga_fb_palette,0,4);
}

/**********************************************************************
 *         VGA_SetBright
 *
 * Select if using a "bright" palette or not.
 * This is a property of the CGA controller
 */
void VGA_SetBright(BOOL bright)
{
  TRACE("%i\n", bright);

  /* Remember the "bright" value used by the CGA controller */
  vga_fb_bright = bright;
}

/**********************************************************************
 *         VGA_SetEnabled
 *
 * Select if output is enabled or disabled
 * This is a property of the CGA controller
 */
static void VGA_SetEnabled(BOOL enabled)
{
  TRACE("%i\n", enabled);

  /* Check if going from enabled to disabled */
  if(vga_fb_enabled && !enabled)
  {
    /* Clear frame buffer */
    memset(vga_fb_window_data, 0x00, vga_fb_window_size);
  }

  /* Remember the "enabled" value */
  vga_fb_enabled = enabled;
}

/**********************************************************************
 *         VGA_SetPaletteIndex
 *
 * Select the index of the palette which is currently in use
 * This is a property of the CGA controller
 */
void VGA_SetPaletteIndex(unsigned index)
{
  TRACE("%i\n", index);

  /* Remember the palette index, which is only used by CGA for now */
  vga_fb_palette_index = index;
}

/**********************************************************************
 *         VGA_WritePixel
 *
 * Write data to the framebuffer
 * This is a property of the CGA controller, but might be supported
 * later by other framebuffer types
 */
void VGA_WritePixel(unsigned color, unsigned page, unsigned col, unsigned row)
{
  int off;
  int bits;
  int pos;

  /* Calculate CGA byte offset */
  char *data = vga_fb_window_data;
  off = row & 1 ? (8 * 1024) : 0;
  off += (80 * (row/2));
  off += col/4;

  /* Calculate bits offset */
  pos = 6 - (col%4 * 2);

  /* Clear current data */
  bits = 0x03 << pos;
  data[off] &= ~bits;

  /* Set new data */
  bits = color << pos;
  data[off] |= bits;
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
    const VGA_MODE *ModeInfo = VGA_GetModeInfo(VGA_CurrentMode);
    if ( ModeInfo->ModeType == TEXT )
    {
       char *dat = VGA_AlphaBuffer() + ((vga_text_width * y + x) * 2);
       dat[0] = ascii;
       if (attr>=0)
          dat[1] = attr;
    }
    else
    {
       FIXME("Write %c at (%i,%i) - not yet supported in graphic modes.\n", (char)ascii, x, y);
    }
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
   * CGA framebuffer (160x200) - CGA_ColorComposite, special subtype of mode 6
   * This buffer is encoded as following:
   * - 4 bit pr. pixel, 2 pixels per byte
   * - 80 bytes per row
   * - Every second line has an offset of 8096
   */
  if(vga_fb_depth == 4 && vga_fb_width == 160 && vga_fb_height == 200){
    WORD off = 0;
    BYTE bits = 4;
    BYTE value;
    for(Y=0; Y<vga_fb_height; Y++, surf+=(Pitch*2)){
      for(X=0; X<vga_fb_width; X++){
        off = Y & 1 ? (8 * 1024) : 0;
        off += (80 * (Y/2));
        off += X/2;
        value = (dat[off] >> bits) & 0xF;
        surf[(X*4)+0] = value;
        surf[(X*4)+1] = value;
        surf[(X*4)+2] = value;
        surf[(X*4)+3] = value;
        surf[(X*4)+Pitch+0] = value;
        surf[(X*4)+Pitch+1] = value;
        surf[(X*4)+Pitch+2] = value;
        surf[(X*4)+Pitch+3] = value;
        bits -= 4;
        bits &= 7;
      }
    }
  }

  /*
   * CGA framebuffer (320x200)
   * This buffer is encoded as following:
   * - 2 bits per color, 4 pixels per byte
   * - 80 bytes per row
   * - Every second line has an offset of 8096
   */
  else if(vga_fb_depth == 2 && vga_fb_width == 320 && vga_fb_height == 200){
    WORD off = 0;
    BYTE bits = 6;
    BYTE value;
    /* Iterate over the rows */
    for(Y=0; Y<vga_fb_height; Y++, surf+=(Pitch*2)){
      for(X=0; X<vga_fb_width; X++){
        off = Y & 1 ? (8 * 1024) : 0;
        off += (80 * (Y/2));
        off += X/4;
        value = (dat[off] >> bits) & 0x3;
        surf[(X*2)] = value;
        surf[(X*2)+1] = value;
        surf[(X*2)+Pitch] = value;
        surf[(X*2)+Pitch+1] = value;
        bits -= 2;
        bits &= 7;
      }
    }
  }

  /*
   * Double VGA framebuffer (320x200 -> 640x400), if needed.
   */
  else if(Height >= 2 * vga_fb_height && Width >= 2 * vga_fb_width && bpp == 1)
    for (Y=0; Y<vga_fb_height; Y++,surf+=Pitch*2,dat+=vga_fb_pitch)
      for (X=0; X<vga_fb_width; X++) {
       BYTE value = dat[X];
       surf[X*2] = value;
       surf[X*2+1] = value;
       surf[X*2+Pitch] = value;
       surf[X*2+Pitch+1] = value;
      }
  /*
   * Linear Buffer, including mode 19
   */
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
        /* General Register - Feature Control */
        case 0x3ba:
           FIXME("Unsupported VGA register: general register - feature control 0x%04x (value 0x%02x)\n", port, val);
           break;
        /* Attribute Controller - Address/Other */
        case 0x3c0:
           if (vga_address_3c0)
               vga_index_3c0 = val;
           else
               FIXME("Unsupported index, VGA attribute controller register 0x3c0: 0x%02x (value 0x%02x)\n",
                     vga_index_3c0, val);
           vga_address_3c0 = !vga_address_3c0;
           break;
        /* General Register - Misc output */
        case 0x3c2:
           FIXME("Unsupported VGA register: general register - misc output 0x%04x (value 0x%02x)\n", port, val);
           break;
        /* General Register - Video subsystem enable */
        case 0x3c3:
           FIXME("Unsupported VGA register: general register - video subsystem enable 0x%04x (value 0x%02x)\n", port, val);
           break;
        /* Sequencer Register - Address */
        case 0x3c4:
           vga_index_3c4 = val;
           break;
        /* Sequencer Register - Other */
        case 0x3c5:
          switch(vga_index_3c4) {
               case 0x04: /* Sequencer: Memory Mode Register */
                  if(vga_fb_depth == 8)
                      VGA_SetWindowStart((val & 8) ? 0 : -1);
                  else
                      FIXME("Memory Mode Register not supported in this mode.\n");
                  break;
               default:
                  FIXME("Unsupported index, VGA sequencer register 0x3c4: 0x%02x (value 0x%02x)\n",
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
        /* Graphics Controller Register - Address */
        case 0x3ce:
            vga_index_3ce = val;
           break;
        /* Graphics Controller Register - Other */
        case 0x3cf:
           FIXME("Unsupported index, VGA graphics controller register - other 0x3ce: 0x%02x (value 0x%02x)\n",
                 vga_index_3ce, val);
           break;
        /* CRT Controller Register - Index (MDA) */
        case 0x3b4:
        /* CRT Controller Register - Index (CGA) */
        case 0x3d4:
           vga_index_3d4 = val;
           break;
        /* CRT Controller Register - Other (MDA) */
        case 0x3b5:
        /* CRT Controller Register - Other (CGA) */
        case 0x3d5:
           FIXME("Unsupported index, VGA crt controller register 0x3b4/0x3d4: 0x%02x (value 0x%02x)\n",
                 vga_index_3d4, val);
           break;
        /* Mode control register - 6845 Motorola (MDA) */
        case 0x3b8:
        /* Mode control register - 6845 Motorola (CGA) */
        case 0x3d8:
           /*
            *  xxxxxxx1 = 80x25 text
            *  xxxxxxx0 = 40x25 text (default)
            *  xxxxxx1x = graphics (320x200)
            *  xxxxxx0x = text
            *  xxxxx1xx = B/W
            *  xxxxx0xx = color
            *  xxxx1xxx = enable video signal
            *  xxx1xxxx = 640x200 B/W graphics
            *  xx1xxxxx = blink
            */

           /* check bits 6 and 7 */
           if (val & 0xC0)
           {
              FIXME("Unsupported value, VGA register 0x3d8: 0x%02x - bits 7 and 6 not supported.\n", val);
           }
           /* check bits 5 - blink on */
           if (val & 0x20)
           {
              FIXME("Unsupported value, VGA register 0x3d8: 0x%02x (bit 5) - blink is not supported.\n", val);
           }
           /* Enable Video Signal (bit 3) - Set the enabled bit */
           VGA_SetEnabled((val & 0x08) != 0);

           /* xxx1x010 - Detect 160x200, 16 color mode (CGA composite) */
           if( (val & 0x17) == 0x12 )
           {
             /* Switch to 160x200x4 composite mode - special case of mode 6 */
             CGA_ColorComposite = TRUE;
             VGA_SetMode(6);
           }
           else
           {
             /* turn off composite colors otherwise, including 320x200 (80x200 16 color) */
             CGA_ColorComposite = FALSE;
           }

           /* xxx0x100 - Detect VGA mode 0 */
           if( (val & 0x17) == 0x04 )
           {
             VGA_SetMode(0);
           }
           /* xxx0x000 - Detect VGA mode 1 */
           else if( (val & 0x17) == 0x00 )
           {
             VGA_SetMode(1);
           }
           /* xxx0x101 - Detect VGA mode 2 */
           else if( (val & 0x17) == 0x05 )
           {
             VGA_SetMode(2);
           }
           /* xxx0x001 - Detect VGA mode 3 */
           else if( (val & 0x17) == 0x01 )
           {
             VGA_SetMode(3);
           }
           /* xxx0x010 - Detect VGA mode 4 */
           else if( (val & 0x17) == 0x02 )
           {
             VGA_SetMode(4);
           }
           /* xxx0x110 - Detect VGA mode 5 */
           else if( (val & 0x17) == 0x06 )
           {
             VGA_SetMode(5);
           }
           /* xxx1x110 - Detect VGA mode 6 */
           else if( (val & 0x17) == 0x16 )
           {
             VGA_SetMode(6);
           }
           /* unsupported mode */
           else
           {
             FIXME("Unsupported value, VGA register 0x3d8: 0x%02x - unrecognized MDA/CGA mode\n", val); /* Set the enabled bit */
           }

           break;

        /* Colour control register (CGA) */
        case 0x3d9:
           /* Set bright */
           VGA_SetBright((val & 0x10) != 0);

           /* Set palette index */
           VGA_SetPaletteIndex((val & 0x20) != 0);

           /* Now update the palette */
           VGA_UpdatePalette();
           break;
        default:
            FIXME("Unsupported VGA register: 0x%04x (value 0x%02x)\n", port, val);
    }
}

BYTE VGA_ioport_in( WORD port )
{
    BYTE ret;

    switch (port) {
        /* Attribute Controller - Other */
        case 0x3c1:
           FIXME("Unsupported index, VGA attribute controller register 0x3c0: 0x%02x\n",
                 vga_index_3c0);
           return 0xff;
        /* General Register - Input status 0 */
        case 0x3c2:
           ret=0xff;
           FIXME("Unsupported VGA register: general register - input status 0 0x%04x\n", port);
           break;
        /* General Register - Video subsystem enable */
        case 0x3c3:
           ret=0xff;
           FIXME("Unsupported VGA register: general register - video subsystem enable 0x%04x\n", port);
           break;
        /* Sequencer Register - Other */
        case 0x3c5:
           switch(vga_index_3c4) {
               case 0x04: /* Sequencer: Memory Mode Register */
                    return (VGA_GetWindowStart() == -1) ? 0xf7 : 0xff;
               default:
                   FIXME("Unsupported index, register 0x3c4: 0x%02x\n",
                         vga_index_3c4);
                   return 0xff;
           }
        /* General Register -  DAC State */
        case 0x3c7:
           ret=0xff;
           FIXME("Unsupported VGA register: general register - DAC State 0x%04x\n", port);
           break;
        /* General Register - Feature control */
        case 0x3ca:
           ret=0xff;
           FIXME("Unsupported VGA register: general register - Feature control 0x%04x\n", port);
           break;
        /* General Register - Misc output */
        case 0x3cc:
           ret=0xff;
           FIXME("Unsupported VGA register: general register - Feature control 0x%04x\n", port);
           break;
        /* Graphics Controller Register - Other */
        case 0x3cf:
           FIXME("Unsupported index, register 0x3ce: 0x%02x\n",
                 vga_index_3ce);
           return 0xff;
        /* CRT Controller Register - Other (MDA) */
        case 0x3b5:
        /* CRT Controller Register - Other (CGA) */
        case 0x3d5:
           FIXME("Unsupported index, VGA crt controller register 0x3b4/0x3d4: 0x%02x\n",
                 vga_index_3d4);
           return 0xff;
        /* General Register - Input status 1 (MDA) */
        case 0x3ba:
        /* General Register - Input status 1 (CGA) */
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
