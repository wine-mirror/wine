/*
 * Image Color Management
 *
 * Copyright 2004 Marcus Meissner
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

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(icm);


/***********************************************************************
 *           EnumICMProfilesA    (GDI32.@)
 */
INT WINAPI EnumICMProfilesA(HDC hdc,ICMENUMPROCA func,LPARAM lParam) {
	FIXME("(%p, %p, 0x%08lx), stub.\n",hdc,func,lParam);
	return -1;
}

/***********************************************************************
 *           EnumICMProfilesW    (GDI32.@)
 */
INT WINAPI EnumICMProfilesW(HDC hdc,ICMENUMPROCW func,LPARAM lParam) {
	FIXME("(%p, %p, 0x%08lx), stub.\n",hdc,func,lParam);
	return -1;
}
