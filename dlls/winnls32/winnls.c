/*
 * Copyright 2001 Andreas Mohr
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
#include "winnls32.h"

/***********************************************************************
 *		WINNLS32GetEnableStatus (WINNLS32.2)
 */
BOOL WINAPI WINNLS32GetEnableStatus(HWND hWnd)
{
    return FALSE;
}

/***********************************************************************
 *		WINNLS32EnableIME (WINNLS32.1)
 */
BOOL WINAPI WINNLS32EnableIME(HWND hWnd, BOOL fEnable)
{
    /* fake return of previous status. is this what this function should do ? */
    return !fEnable;
}
