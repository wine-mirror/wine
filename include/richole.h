/*
 * Copyright (C) 2002 Andriy Palamarchuk
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

#ifndef __WINE_RICHOLE_H
#define __WINE_RICHOLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* REOBJECT structure flags */
#define REO_GETOBJ_NO_INTERFACES        0x00000000
#define REO_GETOBJ_POLEOBJ              0x00000001
#define REO_GETOBJ_PSTG                 0x00000002
#define REO_GETOBJ_POLESITE             0x00000004
#define REO_GETOBJ_ALL_INTERFACES       0x00000007
#define REO_CP_SELECTION                0xFFFFFFFF
#define REO_IOB_SELECTION               0xFFFFFFFF
#define REO_IOB_USE_CP                  0xFFFFFFFE
#define REO_NULL                        0x00000000
#define REO_READWRITEMASK               0x0000003F
#define REO_DONTNEEDPALETTE             0x00000020
#define REO_BLANK                       0x00000010
#define REO_DYNAMICSIZE                 0x00000008
#define REO_INVERTEDSELECT              0x00000004
#define REO_BELOWBASELINE               0x00000002
#define REO_RESIZABLE                   0x00000001
#define REO_LINK                        0x80000000
#define REO_STATIC                      0x40000000
#define REO_SELECTED                    0x08000000
#define REO_OPEN                        0x04000000
#define REO_INPLACEACTIVE               0x02000000
#define REO_HILITED                     0x01000000
#define REO_LINKAVAILABLE               0x00800000
#define REO_GETMETAFILE                 0x00400000

/* clipboard operation flags */
#define RECO_PASTE            0x00000000
#define RECO_DROP             0x00000001
#define RECO_COPY             0x00000002
#define RECO_CUT              0x00000003
#define RECO_DRAG             0x00000004

#ifdef __cplusplus
}
#endif

#endif /* __WINE_RICHOLE_H */
