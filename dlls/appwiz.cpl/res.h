/*
* Add/Remove Programs resources
*
* Copyright 2001-2002, 2008 Owen Rudge
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

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

/* Dialogs */

#define IDD_MAIN               1
#define IDD_INFO               2

/* Dialog controls */
#define IDC_INSTALL         1010
#define IDL_PROGRAMS        1011
#define IDC_ADDREMOVE       1012
#define IDC_SUPPORT_INFO    1013
#define IDC_MODIFY          1014

#define IDC_INFO_PUBLISHER  1100
#define IDC_INFO_VERSION    1101
#define IDC_INFO_CONTACT    1102
#define IDC_INFO_SUPPORT    1103
#define IDC_INFO_PHONE      1104
#define IDC_INFO_README     1105
#define IDC_INFO_UPDATES    1106
#define IDC_INFO_COMMENTS   1107
#define IDC_INFO_LABEL      1108

#define ID_DWL_GECKO_DIALOG 1200
#define ID_DWL_PROGRESS     1201
#define ID_DWL_INSTALL      1202
#define ID_DWL_STATUS       1203
#define ID_DWL_MONO_DIALOG  1204

/* Icons */

#define ICO_MAIN               1

/* Strings */
#define IDS_CPL_TITLE          1
#define IDS_CPL_DESC           2
#define IDS_TAB1_TITLE         3
#define IDS_UNINSTALL_FAILED   4
#define IDS_NOT_SPECIFIED      5
#define IDS_COLUMN_NAME        6
#define IDS_COLUMN_PUBLISHER   7
#define IDS_COLUMN_VERSION     8
#define IDS_FILTER_INSTALLS    9
#define IDS_FILTER_PROGRAMS    10
#define IDS_FILTER_ALL         11
#define IDS_REMOVE             12
#define IDS_MODIFY_REMOVE      13
#define IDS_DOWNLOADING        14
#define IDS_INSTALLING         15
#define IDS_INVALID_SHA        16
#define IDS_WAIT_COMPLETE      17
#define IDS_DOWNLOAD_FAILED    18
