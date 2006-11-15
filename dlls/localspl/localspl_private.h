/*
 * Implementation of the Local Printmonitor: internal include file
 *
 * Copyright 2006 Detlef Riekenberg
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

#ifndef __WINE_LOCALSPL_PRIVATE__
#define __WINE_LOCALSPL_PRIVATE__


/* ## DLL-wide Globals ## */
extern HINSTANCE LOCALSPL_hInstance;

/* ## Resource-ID ## */
#define IDS_LOCALPORT       500
#define IDS_LOCALMONITOR    507
#define IDS_NOTHINGTOCONFIG 508

/* ## Reserved memorysize for the strings (in WCHAR) ## */
#define IDS_LOCALMONITOR_MAXLEN 64
#define IDS_LOCALPORT_MAXLEN 32
#define IDS_NOTHINGTOCONFIG_MAXLEN 80

#endif /* __WINE_LOCALSPL_PRIVATE__ */
