/*
 * Internet control panel applet
 *
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#ifndef __WINE_INETCPL__
#define __WINE_INETCPL__

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

extern HMODULE hcpl;
INT_PTR CALLBACK content_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK general_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK security_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;

/* ## Memory allocation functions ## */

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

/* ######### */

#define NUM_PROPERTY_PAGES 8

/* icons */
#define ICO_MAIN            100

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2

/* dialogs */
#define IDC_STATIC          -1

#define IDD_GENERAL         1000
#define IDC_HOME_EDIT       1000
#define IDC_HOME_CURRENT    1001
#define IDC_HOME_DEFAULT    1002
#define IDC_HOME_BLANK      1003
#define IDC_HISTORY_DELETE     1004
#define IDC_HISTORY_SETTINGS   1005

#define IDD_DELETE_HISTORY     1010
#define IDC_DELETE_TEMP_FILES  1011
#define IDC_DELETE_COOKIES     1012
#define IDC_DELETE_HISTORY     1013
#define IDC_DELETE_FORM_DATA   1014
#define IDC_DELETE_PASSWORDS   1015

#define IDD_SECURITY        2000
#define IDC_SEC_LISTVIEW    2001

#define IDD_CONTENT         4000
#define IDC_CERT            4100
#define IDC_CERT_PUBLISHER  4101

#endif
