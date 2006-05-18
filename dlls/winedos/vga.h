/*
 * VGA emulation
 *
 * Copyright 1998 Ove KÅven
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

/* graphics mode */
int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth);
int VGA_GetMode(unsigned*Height,unsigned*Width,unsigned*Depth);
void VGA_SetPalette(PALETTEENTRY*pal,int start,int len);
void VGA_SetColor16(int reg,int color);
char VGA_GetColor16(int reg);
void VGA_Set16Palette(char *Table);
void VGA_Get16Palette(char *Table);
void VGA_SetQuadPalette(RGBQUAD*color,int start,int len);
LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth);
void VGA_Unlock(void);
void VGA_SetWindowStart(int start);
int  VGA_GetWindowStart(void);
void VGA_ShowMouse(BOOL show);

/* text mode */
void VGA_InitAlphaMode(unsigned*Xres,unsigned*Yres);
void VGA_SetAlphaMode(unsigned Xres,unsigned Yres);
BOOL VGA_GetAlphaMode(unsigned*Xres,unsigned*Yres);
void VGA_SetCursorShape(unsigned char start_options,unsigned char end);
void VGA_SetCursorPos(unsigned X,unsigned Y);
void VGA_GetCursorPos(unsigned*X,unsigned*Y);
void VGA_WriteChars(unsigned X,unsigned Y,unsigned ch,int attr,int count);
void VGA_PutChar(BYTE ascii);
void VGA_SetTextAttribute(BYTE attr);
void VGA_ClearText(unsigned row1, unsigned col1,
                  unsigned row2, unsigned col2,
                  BYTE attr);
void VGA_ScrollUpText(unsigned row1, unsigned col1,
                     unsigned row2, unsigned col2,
                     unsigned lines, BYTE attr);
void VGA_ScrollDownText(unsigned row1, unsigned col1,
                       unsigned row2, unsigned col2,
                       unsigned lines, BYTE attr);
void VGA_GetCharacterAtCursor(BYTE *ascii, BYTE *attr);

/* control */
void VGA_ioport_out(WORD port, BYTE val);
BYTE VGA_ioport_in(WORD port);
void VGA_Clean(void);

#endif /* __WINE_VGA_H */
