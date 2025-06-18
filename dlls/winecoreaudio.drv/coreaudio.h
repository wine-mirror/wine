/* Definition for CoreAudio drivers : wine multimedia system
 *
 * Copyright 2005-2007 Emmanuel Maillard
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

#ifndef __WINE_COREAUDIO_H
#define __WINE_COREAUDIO_H

#include "wine/debug.h"

/* fourcc is in native order, where MSB is the first character. */
static inline const char* coreaudio_dbgstr_fourcc(INT32 fourcc)
{
    char buf[4] = { (char) (fourcc >> 24), (char) (fourcc >> 16),
                    (char) (fourcc >> 8),  (char) fourcc };
    /* OSStatus is a signed decimal except in parts of CoreAudio */
    if ((buf[0] < 32) || (buf[1] < 32) || (buf[2] < 32) || (buf[3] < 32))
	return wine_dbg_sprintf("%d", fourcc);
    return wine_dbgstr_an(buf, sizeof(buf));
}

#endif /* __WINE_COREAUDIO_H */
