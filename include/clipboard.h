/*
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Alex Korobka
 * Copyright 1999 Noel Borthwick
 * Copyright 2003 Ulrich Czekalla for CodeWeavers
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

#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "windef.h"

typedef struct tagCLIPBOARDINFO
{
    HWND hWndOpen;
    HWND hWndOwner;
    HWND hWndViewer;
    UINT seqno;
    UINT flags;
} CLIPBOARDINFO, *LPCLIPBOARDINFO;

#endif /* __WINE_CLIPBOARD_H */
