/*
 * DirectDraw HAL base interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
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
#ifndef __WINE_X11DDRAW_H
#define __WINE_X11DDRAW_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddrawi.h"

extern LPDDRAWI_DDRAWSURFACE_LCL X11DRV_DD_Primary;
extern LPDDRAWI_DDRAWSURFACE_GBL X11DRV_DD_PrimaryGbl;
extern HWND X11DRV_DD_PrimaryWnd;
extern HBITMAP X11DRV_DD_PrimaryDIB;
extern BOOL X11DRV_DD_IsDirect;

void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr, LPVIDMEM fb_mem);
void X11DRV_DDHAL_SetPalEntries(Colormap pal, DWORD dwBase, DWORD dwNumEntries,
				LPPALETTEENTRY lpEntries);

typedef struct _X11DRIVERINFO {
  const GUID *		lpGuid;
  DWORD			dwSize;
  LPVOID		lpvData;
  struct _X11DRIVERINFO*lpNext;
} X11DRIVERINFO,*LPX11DRIVERINFO;

typedef struct _X11DEVICE {
  LPX11DRIVERINFO	lpInfo;
} X11DEVICE,*LPX11DEVICE;

#endif /* __WINE_X11DDRAW_H */
