/*
 * System metrics definitions
 *
 * Copyright 1994 Alexandre Julliard
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

#ifndef __WINE_SYSMETRICS_H
#define __WINE_SYSMETRICS_H

extern void SYSMETRICS_Init(void);  /* sysmetrics.c */
extern INT SYSMETRICS_Set( INT index, INT value );  /* sysmetrics.c */
extern void SYSCOLOR_Init(void);  /* syscolor.c */
extern void SYSPARAMS_GetDoubleClickSize( INT *width, INT *height );
extern INT SYSPARAMS_GetMouseButtonSwap( void );

/* Wine extensions */
#define SM_WINE_BPP (SM_CMETRICS+1)  /* screen bpp */
#define SM_WINE_CMETRICS SM_WINE_BPP

#endif  /* __WINE_SYSMETRICS_H */
