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


extern HMODULE hcpl;
INT_PTR CALLBACK content_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK general_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;

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

#define IDD_CONTENT         4000
#define IDC_CERT            4100
#define IDC_CERT_PUBLISHER  4101

#endif
