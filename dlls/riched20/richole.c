/*
 * RichEdit GUIDs
 *
 * Copyright 2004 by Krzysztof Foltman
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

#include <windef.h>

/* there is no way to be consistent across different sets of headers - mingw, Wine, Win32 SDK*/

#define RICHEDIT_GUID(name, l, w1, w2) \
  GUID name = { l, w1, w2, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}}

RICHEDIT_GUID(IID_IRichEditOle, 0x00020d00, 0, 0);
RICHEDIT_GUID(IID_IRichEditOleCallback, 0x00020d03, 0, 0);

#define TEXTSERV_GUID(name, l, w1, w2, b1, b2) \
  GUID name = { l, w1, w2, {b1, b2, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5}}

TEXTSERV_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d);
TEXTSERV_GUID(IID_ITextHost, 0xc5bdd8d0, 0xd26e, 0x11ce, 0xa8, 0x9e);
TEXTSERV_GUID(IID_ITextHost2, 0xc5bdd8d0, 0xd26e, 0x11ce, 0xa8, 0x9e);
