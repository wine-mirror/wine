/*
 * msvcrtd.dll debugging code
 *
 * Copyright (C) 2003 Adam Gundy
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		_CrtSetDumpClient (MSVCRTD.@)
 */
void *_CrtSetDumpClient()
{
    return NULL;
}


/*********************************************************************
 *		_CrtSetReportHook (MSVCRTD.@)
 */
void *_CrtSetReportHook()
{
    return NULL;
}


/*********************************************************************
 *		_CrtSetReportMode (MSVCRTD.@)
 */
int _CrtSetReportMode()
{
    return 0;
}


/*********************************************************************
 *		_CrtSetDbgFlag (MSVCRTD.@)
 */
int _CrtSetDbgFlag()
{
    return 0;
}


/*********************************************************************
 *		_CrtDbgReport (MSVCRTD.@)
 */
int _CrtDbgReport()
{
    return 0;
}

/*********************************************************************
 *		_CrtDumpMemoryLeaks (MSVCRTD.@)
 */
int _CrtDumpMemoryLeaks()
{
    return 0;
}
