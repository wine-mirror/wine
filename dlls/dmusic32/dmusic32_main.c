/* DirectMusic32 Main
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdarg.h>

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

HRESULT WINAPI DMUSIC32_CreateCDirectMusicEmulatePort (LPVOID ptr1, LPVOID ptr2, LPVOID ptr3)
{	
	FIXME("stub (undocumented function); if you see this, you're probably using native dmusic.dll. Use native dmusic32.dll as well!\n");
	return S_OK;
}

HRESULT WINAPI DMUSIC32_EnumLegacyDevices (LPVOID ptr1, LPVOID ptr2)
{	
	FIXME("stub (undocumented function); if you see this, you're probably using native dmusic.dll. Use native dmusic32.dll as well!\n");
	return S_OK;
}
