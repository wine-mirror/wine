/*
 * Copyright 2002 Mike McCormack for Codeweavers
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
#include "winbase.h"

/* this function is called by Internet Explorer when it is about to verify a downloaded component */
BOOL WINAPI I_CryptCreateLruCache(DWORD x, DWORD y)
{
    return FALSE;
}

/* these functions all have an unknown number of args */
BOOL WINAPI I_CryptFindLruEntryData(DWORD x)
{
    return FALSE;
}

BOOL WINAPI I_CryptFlushLruCache(DWORD x)
{
    return FALSE;
}

BOOL WINAPI I_CryptFreeLruCache(DWORD x)
{
    return FALSE;
}
