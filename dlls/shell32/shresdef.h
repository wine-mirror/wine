/*
 * Copyright 2000 Juergen Schmied
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

#ifndef __WINE_SHELL_RES_H
#define __WINE_SHELL_RES_H

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <shlobj.h>
#include <dlgs.h>

/*
	columntitles for the shellview
*/
#define IDS_SHV_COLUMN1		7
#define IDS_SHV_COLUMN2		8
#define IDS_SHV_COLUMN3		9
#define IDS_SHV_COLUMN4		10
#define IDS_SHV_COLUMN5		11
#define IDS_SHV_COLUMN6		12
#define IDS_SHV_COLUMN7		13
#define IDS_SHV_COLUMN8		14
#define IDS_SHV_COLUMN9		15
#define IDS_SHV_COLUMN10	16
#define IDS_SHV_COLUMN11	17
#define IDS_SHV_COLUMN_DELFROM  18
#define IDS_SHV_COLUMN_DELDATE  19
#define IDS_SHV_COL_DOCS        80
#define IDS_SHV_COL_STATUS      81
#define IDS_SHV_COL_LOCATION    82
#define IDS_SHV_COL_MODEL       83

#define IDS_DESKTOP		20
#define IDS_MYCOMPUTER		21
#define IDS_CONTROLPANEL        22

#define IDS_SELECT		23
#define IDS_OPEN		24
#define IDS_VIEW_LARGE		25
#define IDS_VIEW_SMALL		26
#define IDS_VIEW_LIST		27
#define IDS_VIEW_DETAILS	28

#define IDS_RESTART_TITLE      40
#define IDS_RESTART_PROMPT     41
#define IDS_SHUTDOWN_TITLE     42
#define IDS_SHUTDOWN_PROMPT    43

#define IDS_PROGRAMS                45
#define IDS_PERSONAL                46
#define IDS_FAVORITES               47
#define IDS_STARTUP                 48
#define IDS_STARTMENU               51
#define IDS_MYMUSIC                 52
#define IDS_MYVIDEOS                53
#define IDS_DESKTOPDIRECTORY        54
#define IDS_NETHOOD                 55
#define IDS_TEMPLATES               56
#define IDS_PRINTHOOD               58
#define IDS_HISTORY                 62
#define IDS_PROGRAM_FILES           63
#define IDS_MYPICTURES              64
#define IDS_COMMON_FILES            65
#define IDS_COMMON_DOCUMENTS        66
#define IDS_ADMINTOOLS              67
#define IDS_COMMON_MUSIC            68
#define IDS_COMMON_PICTURES         69
#define IDS_COMMON_VIDEOS           70
#define IDS_PROGRAM_FILESX86        72
#define IDS_CONTACTS                74
#define IDS_LINKS                   75
#define IDS_SLIDE_SHOWS             76
#define IDS_PLAYLISTS               78
#define IDS_SAMPLE_MUSIC            87
#define IDS_SAMPLE_PICTURES         88
#define IDS_SAMPLE_PLAYLISTS        89
#define IDS_SAMPLE_VIDEOS           90
#define IDS_SAVED_GAMES             91
#define IDS_SAVED_SEARCHES          92
#define IDS_USER_PROFILES           93
#define IDS_DOCUMENTS               95
#define IDS_DOWNLOADS               96

#define IDS_CREATEFOLDER_DENIED     128
#define IDS_CREATEFOLDER_CAPTION    129
#define IDS_DELETEITEM_CAPTION	    130
#define IDS_DELETEFOLDER_CAPTION    131
#define IDS_DELETEITEM_TEXT	    132
#define IDS_DELETEMULTIPLE_TEXT	    133
#define IDS_OVERWRITEFILE_CAPTION   134
#define IDS_OVERWRITEFILE_TEXT	    135
#define IDS_DELETESELECTED_TEXT     136
#define IDS_TRASHFOLDER_TEXT        137
#define IDS_TRASHITEM_TEXT          138
#define IDS_TRASHMULTIPLE_TEXT      139
#define IDS_CANTTRASH_TEXT          140
#define IDS_OVERWRITEFOLDER_TEXT    141

#define IDS_NEWFOLDER 142

#define IDS_CPANEL_TITLE            143
#define IDS_CPANEL_NAME             144
#define IDS_CPANEL_DESCRIPTION      145

#define IDS_RUNDLG_ERROR            160
#define IDS_RUNDLG_BROWSE_ERROR     161
#define IDS_RUNDLG_BROWSE_CAPTION   162
#define IDS_RUNDLG_BROWSE_FILTER_EXE 163
#define IDS_RUNDLG_BROWSE_FILTER_ALL 164

#define IDS_SHLEXEC_NOASSOC         165

#define IDS_RECYCLEBIN_ERASEITEM         166
#define IDS_RECYCLEBIN_ERASEMULTIPLE     167
#define IDS_RECYCLEBIN_ERASE_CAPTION     168
#define IDS_RECYCLEBIN_OVERWRITEFILE     169
#define IDS_RECYCLEBIN_OVERWRITEFOLDER   170
#define IDS_RECYCLEBIN_OVERWRITE_CAPTION 171


#define IDS_LICENSE                 256
#define IDS_LICENSE_CAPTION         257

#define MENU_SHV_FILE 144

#define MENU_CPANEL                 200
#define IDM_CPANEL_EXIT             201
#define IDM_CPANEL_ABOUT            202
#define IDM_CPANEL_APPLET_BASE      210

#define MENU_RECYCLEBIN             300
#define IDM_RECYCLEBIN_RESTORE      301
#define IDM_RECYCLEBIN_ERASE        302

/* Note: this string is referenced from the registry*/
#define IDS_RECYCLEBIN_FOLDER_NAME   8964

/* Properties dialog */
#define IDD_FILE_PROPERTIES        8
#define IDD_FOLDER_PROPERTIES      9

#define IDD_ICON                0x4300
#define IDD_MESSAGE             0x4301

/* these IDs are the same as on native */
#define IDD_YESTOALL            0x3207
/* browse for folder dialog box */
#define IDD_MAKENEWFOLDER       0x3746
#define IDD_FOLDERTEXT          0x3745
#define IDD_FOLDER              0x3744
#define IDD_STATUS		0x3743
#define IDD_TITLE		0x3742
#define IDD_TREEVIEW		0x3741

#define IDI_SHELL_DOCUMENT           1
#define IDI_SHELL_FOLDER             4
#define IDI_SHELL_FOLDER_OPEN        5
#define IDI_SHELL_5_12_FLOPPY        6
#define IDI_SHELL_3_14_FLOPPY        7
#define IDI_SHELL_FLOPPY             8
#define IDI_SHELL_DRIVE              9
#define IDI_SHELL_NETDRIVE          10
#define IDI_SHELL_NETDRIVE2         11
#define IDI_SHELL_CDROM             12
#define IDI_SHELL_RAMDISK           13
#define IDI_SHELL_ENTIRE_NETWORK    14
#define IDI_SHELL_NETWORK           15
#define IDI_SHELL_MY_COMPUTER       16
#define IDI_SHELL_PRINTER           17
#define IDI_SHELL_MY_NETWORK_PLACES 18
#define IDI_SHELL_COMPUTERS_NEAR_ME 19
#define IDI_SHELL_FOLDER_SMALL_XP   20
#define IDI_SHELL_SEARCH            23
#define IDI_SHELL_HELP              24
#define IDI_SHELL_FOLDER_OPEN_LARGE 29
#define IDI_SHELL_SHORTCUT          30
#define IDI_SHELL_FOLDER_OPEN_SMALL 31
#define IDI_SHELL_EMPTY_RECYCLE_BIN 32
#define IDI_SHELL_FULL_RECYCLE_BIN  33
#define IDI_SHELL_DESKTOP           35
#define IDI_SHELL_CONTROL_PANEL     36
#define IDI_SHELL_PRINTERS_FOLDER   38
#define IDI_SHELL_FONTS_FOLDER      39
#define IDI_SHELL_REMOTE_PRINTER   140
#define IDI_SHELL_TO_FILE_PRINTER  141
#define IDI_SHELL_TRASH_FILE       142
#define IDI_SHELL_CONFIRM_DELETE   161
#define IDI_SHELL_DEFAULT_PRINTER  168
#define IDI_SHELL_DEFAULT_REMOTE_PRINTER   169
#define IDI_SHELL_DEFAULT_TO_FILE_PRINTER  170
#define IDI_SHELL_MY_DOCUMENTS     235
#define IDI_SHELL_FAVORITES        319

/* 
AVI resources, windows shell32 has 14 of them: 150-152 and 160-170
FIXME: Need to add them, but for now just let them use the same: searching.avi 
(also to limit shell32's size)
*/
#define IDR_AVI_SEARCH             150
#define IDR_AVI_SEARCHING          151
#define IDR_AVI_FINDCOMPUTER       152
#define IDR_AVI_FILEMOVE           160
#define IDR_AVI_FILECOPY           161
#define IDR_AVI_FILENUKE           163
#define IDR_AVI_FILEDELETE         164

/* about box */
#define IDC_ABOUT_LICENSE        97
#define IDC_ABOUT_WINE_TEXT      98
#define IDC_ABOUT_LISTBOX        99
#define IDC_ABOUT_STATIC_TEXT1   100
#define IDC_ABOUT_STATIC_TEXT2   101
#define IDC_ABOUT_STATIC_TEXT3   102

/* run dialog */
#define IDC_RUNDLG_DESCRIPTION  12289
#define IDC_RUNDLG_BROWSE       12288
#define IDC_RUNDLG_ICON         12297
#define IDC_RUNDLG_EDITPATH     12298
#define IDC_RUNDLG_LABEL        12305

/* file property dialog */
#define IDC_FPROP_ICON              13000
#define IDC_FPROP_PATH              13001
#define IDC_FPROP_TYPE_LABEL        13002
#define IDC_FPROP_TYPE              13003
#define IDC_FPROP_OPENWITH_LABEL    13004
#define IDC_FPROP_PROG_ICON         13005
#define IDC_FPROP_PROG_NAME         13006
#define IDC_FPROP_PROG_CHANGE       13007
#define IDC_FPROP_LOCATION_LABEL    13008
#define IDC_FPROP_LOCATION          13009
#define IDC_FPROP_SIZE_LABEL        13010
#define IDC_FPROP_SIZE              13011
#define IDC_FPROP_CREATED_LABEL     13012
#define IDC_FPROP_CREATED           13013
#define IDC_FPROP_MODIFIED_LABEL    13014
#define IDC_FPROP_MODIFIED          13015
#define IDC_FPROP_ACCESSED_LABEL    13016
#define IDC_FPROP_ACCESSED          13017
#define IDC_FPROP_ATTRIB_LABEL      13018
#define IDC_FPROP_READONLY          13019
#define IDC_FPROP_HIDDEN            13020
#define IDC_FPROP_ARCHIVE           13021

/* bitmaps */
/* explorer toolbar icons
 * FIXME: images are hacky and should be re-drawn; also dark and light bitmaps are same for now
 */
#define IDB_TB_LARGE_LIGHT      214
#define IDB_TB_LARGE_DARK       215
#define IDB_TB_SMALL_LIGHT      216
#define IDB_TB_SMALL_DARK       217

#endif
