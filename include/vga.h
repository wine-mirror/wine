/*
 * VGA emulation
 * 
 * Copyright 1998 Ove KÅven
 *
 */

#ifndef __WINE_VGA_H
#define __WINE_VGA_H

#include "winbase.h"
#include "wingdi.h"

/* graphics mode */
int VGA_SetMode(unsigned Xres,unsigned Yres,unsigned Depth);
int VGA_GetMode(unsigned*Height,unsigned*Width,unsigned*Depth);
void VGA_Exit(void);
void VGA_SetPalette(PALETTEENTRY*pal,int start,int len);
void VGA_SetQuadPalette(RGBQUAD*color,int start,int len);
LPSTR VGA_Lock(unsigned*Pitch,unsigned*Height,unsigned*Width,unsigned*Depth);
void VGA_Unlock(void);

/* text mode */
int VGA_SetAlphaMode(unsigned Xres,unsigned Yres);
void VGA_GetAlphaMode(unsigned*Xres,unsigned*Yres);
void VGA_SetCursorPos(unsigned X,unsigned Y);
void VGA_GetCursorPos(unsigned*X,unsigned*Y);

/* control */
void CALLBACK VGA_Poll(ULONG_PTR arg);
void VGA_ioport_out(WORD port, BYTE val);
BYTE VGA_ioport_in(WORD port);
void VGA_Clean(void);

#endif /* __WINE_VGA_H */
