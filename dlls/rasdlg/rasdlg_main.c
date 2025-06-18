/*
 * Implementation of the Remote Access Common Dialog API
 *
 * Copyright 2009 CodeWeavers, Aric Stewart
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rasdlg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rasdlg);

BOOL WINAPI RasEntryDlgW(LPWSTR phonebook, LPWSTR entry, RASENTRYDLGW *lpInfo)
{
    FIXME("(%s %s %p) stub\n", debugstr_w(phonebook), debugstr_w(entry), lpInfo);
    return FALSE;
}
