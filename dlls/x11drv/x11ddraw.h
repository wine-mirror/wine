/*
 * DirectDraw HAL base interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */
#ifndef __WINE_X11DDRAW_H
#define __WINE_X11DDRAW_H
#include "config.h"
#include "ddrawi.h"

extern LPDDRAWI_DDRAWSURFACE_LCL X11DRV_DD_Primary;
extern LPDDRAWI_DDRAWSURFACE_GBL X11DRV_DD_PrimaryGbl;
extern HBITMAP X11DRV_DD_PrimaryDIB;
extern BOOL X11DRV_DD_IsDirect;

void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr);
void X11DRV_DDHAL_SetPalEntries(Colormap pal, DWORD dwBase, DWORD dwNumEntries,
				LPPALETTEENTRY lpEntries);


#endif /* __WINE_X11DDRAW_H */
