/*
 * DirectDraw HAL XVidMode interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */
#ifndef __WINE_DGA2_H
#define __WINE_DGA2_H
#include "config.h"
#ifdef HAVE_LIBXXF86DGA2
#include "ddrawi.h"

extern LPDDHALMODEINFO xf86dga2_modes;
extern unsigned xf86dga2_mode_count;

void X11DRV_XF86DGA2_Init(void);
void X11DRV_XF86DGA2_Cleanup(void);
int X11DRV_XF86DGA2_CreateDriver(LPDDHALINFO info);

#endif /* HAVE_LIBXXF86DGA2 */
#endif /* __WINE_DGA2_H */
