/*
 * DirectDraw HAL XVidMode interface
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_XVIDMODE_H
#define __WINE_XVIDMODE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_LIBXXF86VM
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddrawi.h"

extern LPDDHALMODEINFO xf86vm_modes;
extern unsigned xf86vm_mode_count;

void X11DRV_XF86VM_Init(void);
void X11DRV_XF86VM_Cleanup(void);
int X11DRV_XF86VM_GetCurrentMode(void);
void X11DRV_XF86VM_SetCurrentMode(int mode);
void X11DRV_XF86VM_SetExclusiveMode(int lock);
int X11DRV_XF86VM_CreateDriver(LPDDHALINFO info);

BOOL X11DRV_XF86VM_GetGammaRamp(LPDDGAMMARAMP ramp);
BOOL X11DRV_XF86VM_SetGammaRamp(LPDDGAMMARAMP ramp);

#endif /* HAVE_LIBXXF86VM */
#endif /* __WINE_XVIDMODE_H */
