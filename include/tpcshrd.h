/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#ifndef __WISPSHRD_H
#define __WISPSHRD_H

#define TABLET_DISABLE_PRESSANDHOLD        0x00000001

#define WM_TABLET_DEFBASE                  0x02c0
#define WM_TABLET_MAXOFFSET                0x20
#define WM_TABLET_ADDED                    (WM_TABLET_DEFBASE + 8)
#define WM_TABLET_DELETED                  (WM_TABLET_DEFBASE + 9)
#define WM_TABLET_FLICK                    (WM_TABLET_DEFBASE + 11)
#define WM_TABLET_QUERYSYSTEMGESTURESTATUS (WM_TABLET_DEFBASE + 12)


#endif /* __WISPSHRD_H */
