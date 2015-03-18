/*
 * VGA emulation
 *
 * Copyright 1998 Ove Kåven
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

#ifndef __WINE_VGA_H
#define __WINE_VGA_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

/*
 * VGA VESA definitions
 */
/* mode descriptor */
enum modetype {TEXT=0, GRAPHIC=1};
/* Wine internal information about video modes */
typedef struct {
    WORD Mode;
    BOOL ModeType;
    WORD TextCols;  /* columns of text in display */
    WORD TextRows;  /* rows of text in display */
    WORD CharWidth;
    WORD CharHeight;
    WORD Width;  /* width of display in pixels */
    WORD Height; /* height of display in pixels */
    WORD Depth;  /* bits per pixel */
    WORD Colors; /* total available colors */
    WORD ScreenPages;
    BOOL Supported;
} VGA_MODE;

extern const VGA_MODE VGA_modelist[] DECLSPEC_HIDDEN;

/* all vga modes */
const VGA_MODE *VGA_GetModeInfo(WORD mode) DECLSPEC_HIDDEN;
BOOL VGA_SetMode(WORD mode) DECLSPEC_HIDDEN;

/* graphics mode */
BOOL VGA_GetMode(unsigned *Height, unsigned *Width, unsigned *Depth) DECLSPEC_HIDDEN;
void VGA_SetPalette(PALETTEENTRY*pal,int start,int len) DECLSPEC_HIDDEN;
void VGA_SetColor16(int reg,int color) DECLSPEC_HIDDEN;
char VGA_GetColor16(int reg) DECLSPEC_HIDDEN;
void VGA_Set16Palette(char *Table) DECLSPEC_HIDDEN;
void VGA_Get16Palette(char *Table) DECLSPEC_HIDDEN;
void VGA_SetWindowStart(int start) DECLSPEC_HIDDEN;
int  VGA_GetWindowStart(void) DECLSPEC_HIDDEN;
void VGA_ShowMouse(BOOL show) DECLSPEC_HIDDEN;
void VGA_UpdatePalette(void) DECLSPEC_HIDDEN;
void VGA_SetPaletteIndex(unsigned index) DECLSPEC_HIDDEN;
void VGA_SetBright(BOOL bright) DECLSPEC_HIDDEN;
void VGA_WritePixel(unsigned color, unsigned page, unsigned col, unsigned row) DECLSPEC_HIDDEN;

/* text mode */
void VGA_InitAlphaMode(unsigned*Xres,unsigned*Yres) DECLSPEC_HIDDEN;
void VGA_SetAlphaMode(unsigned Xres,unsigned Yres) DECLSPEC_HIDDEN;
BOOL VGA_GetAlphaMode(unsigned*Xres,unsigned*Yres) DECLSPEC_HIDDEN;
void VGA_SetCursorShape(unsigned char start_options,unsigned char end) DECLSPEC_HIDDEN;
void VGA_SetCursorPos(unsigned X,unsigned Y) DECLSPEC_HIDDEN;
void VGA_GetCursorPos(unsigned*X,unsigned*Y) DECLSPEC_HIDDEN;
void VGA_WriteChars(unsigned X,unsigned Y,unsigned ch,int attr,int count) DECLSPEC_HIDDEN;
void VGA_PutChar(BYTE ascii) DECLSPEC_HIDDEN;
void VGA_ClearText(unsigned row1, unsigned col1,
                  unsigned row2, unsigned col2,
                  BYTE attr) DECLSPEC_HIDDEN;
void VGA_ScrollUpText(unsigned row1, unsigned col1,
                     unsigned row2, unsigned col2,
                     unsigned lines, BYTE attr) DECLSPEC_HIDDEN;
void VGA_ScrollDownText(unsigned row1, unsigned col1,
                       unsigned row2, unsigned col2,
                       unsigned lines, BYTE attr) DECLSPEC_HIDDEN;
void VGA_GetCharacterAtCursor(BYTE *ascii, BYTE *attr) DECLSPEC_HIDDEN;

/* control */
void VGA_ioport_out(WORD port, BYTE val) DECLSPEC_HIDDEN;
BYTE VGA_ioport_in(WORD port) DECLSPEC_HIDDEN;
void VGA_Clean(void) DECLSPEC_HIDDEN;

#endif /* __WINE_VGA_H */
