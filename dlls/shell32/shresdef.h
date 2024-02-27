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
#define IDS_SHV_COLUMN_DELFROM  18
#define IDS_SHV_COLUMN_DELDATE  19
#define IDS_SHV_COL_DOCS        80
#define IDS_SHV_COL_STATUS      81
#define IDS_SHV_COL_LOCATION    82
#define IDS_SHV_COL_MODEL       83

#define IDS_DESKTOP		20
#define IDS_MYCOMPUTER		21
#define IDS_CONTROLPANEL        22

#define IDS_VIEW_LARGE		25
#define IDS_VIEW_SMALL		26
#define IDS_VIEW_LIST		27
#define IDS_VIEW_DETAILS	28

#define IDS_NEW_MENU                29

#define IDS_VERB_EXPLORE            30
#define IDS_VERB_OPEN               31
#define IDS_VERB_PRINT              32
#define IDS_VERB_RUNAS              33

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

#define IDS_NEW_MENU_FOLDER         180

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

#define IDI_SHELL_FILE               1
#define IDI_SHELL_DOCUMENT           2
#define IDI_SHELL_WINDOW             3
#define IDI_SHELL_FOLDER             4
#define IDI_SHELL_FOLDER_OPEN        5
#define IDI_SHELL_5_12_FLOPPY        6
#define IDI_SHELL_3_14_FLOPPY        7
#define IDI_SHELL_FLOPPY             8
#define IDI_SHELL_DRIVE              9
#define IDI_SHELL_NETDRIVE          10
#define IDI_SHELL_NETDRIVE2         11
#define IDI_SHELL_OPTICAL_DRIVE     12
#define IDI_SHELL_RAMDISK           13
#define IDI_SHELL_ENTIRE_NETWORK    14
#define IDI_SHELL_NETWORK           15
#define IDI_SHELL_MY_COMPUTER       16
#define IDI_SHELL_PRINTER           17
#define IDI_SHELL_MY_NETWORK_PLACES 18
#define IDI_SHELL_COMPUTERS_NEAR_ME 19
#define IDI_SHELL_FOLDER_SMALL_XP   20
#define IDI_SHELL_RECENT_DOCUMENTS  21
#define IDI_SHELL_SETTINGS          22
#define IDI_SHELL_SEARCH            23
#define IDI_SHELL_HELP              24
#define IDI_SHELL_RUN               25
#define IDI_SHELL_SLEEP             26
#define IDI_SHELL_EJECT             27
#define IDI_SHELL_SHUT_DOWN         28
#define IDI_SHELL_FOLDER_OPEN_LARGE 29
#define IDI_SHELL_SHORTCUT          30
#define IDI_SHELL_FOLDER_OPEN_SMALL 31
#define IDI_SHELL_EMPTY_RECYCLE_BIN 32
#define IDI_SHELL_FULL_RECYCLE_BIN  33
#define IDI_SHELL_MODEM_FOLDER      34
#define IDI_SHELL_DESKTOP           35
#define IDI_SHELL_CONTROL_PANEL     36
#define IDI_SHELL_APP_FOLDER        37
#define IDI_SHELL_PRINTERS_FOLDER   38
#define IDI_SHELL_FONTS_FOLDER      39
#define IDI_SHELL_START_MENU        40
#define IDI_SHELL_MUSIC_CD          41
#define IDI_SHELL_TREE              42
#define IDI_SHELL_REMOTE_FOLDER     43
#define IDI_SHELL_FAVORITES         44
#define IDI_SHELL_LOG_OFF           45
#define IDI_SHELL_SEARCH_FOLDER     46
#define IDI_SHELL_UPDATE            47
#define IDI_SHELL_SECURITY          48
#define IDI_SHELL_CONFIRM_SHUTDOWN  49
#define IDI_SHELL_BLANK1            50
#define IDI_SHELL_BLANK2            51
#define IDI_SHELL_BLANK3            52
#define IDI_SHELL_BLANK4            53
#define IDI_SHELL_UNKNOWN_DRIVE     54
/* 55 - 132 not used on Windows */
#define IDI_SHELL_FILES            133
#define IDI_SHELL_FILE_SEARCH      134
#define IDI_SHELL_SYSTEM_SEARCH    135
/* 136 not used on Windows */
#define IDI_SHELL_CONTROL_PANEL_XP 137
#define IDI_SHELL_PRINTERS_FOLDER_XP       138
#define IDI_SHELL_ADD_PRINTER      139
#define IDI_SHELL_REMOTE_PRINTER   140
#define IDI_SHELL_TO_FILE_PRINTER  141
#define IDI_SHELL_TRASH_FILE       142
#define IDI_SHELL_TRASH_FOLDER     143
#define IDI_SHELL_TRASH_MIXED      144
#define IDI_SHELL_FILE_OVERWRITE   145
#define IDI_SHELL_FOLDER_OVERWRITE 146
#define IDI_SHELL_FOLDER_RENAME    147
#define IDI_SHELL_INSTALL          148
/* 149 - 150 not used on Windows */
#define IDI_SHELL_CONFIG_FILE      151
#define IDI_SHELL_TEXT_FILE        152
#define IDI_SHELL_BATCH_FILE       153
#define IDI_SHELL_LIBRARY_FILE     154
#define IDI_SHELL_SYSTEM_FONT      155
#define IDI_SHELL_TRUETYPE_FONT    156
#define IDI_SHELL_POSTSCRIPT_FONT  157
/* 158 - 159 not used on Windows */
#define IDI_SHELL_RUN2             160
#define IDI_SHELL_CONFIRM_DELETE   161
/* 162 - 164 not used on Windows */
#define IDI_SHELL_BACKUP           165
#define IDI_SHELL_DISK_CHECK       166
#define IDI_SHELL_DEFRAGMENT       167
#define IDI_SHELL_DEFAULT_PRINTER  168
#define IDI_SHELL_DEFAULT_REMOTE_PRINTER   169
#define IDI_SHELL_DEFAULT_TO_FILE_PRINTER  170
#define IDI_SHELL_TREEVIEW         171
#define IDI_SHELL_NETWORK_FOLDER   172
#define IDI_SHELL_FAVORITES_FOLDER 173
#define IDI_SHELL_CHECKLIST_FOLDER 174
#define IDI_SHELL_NET_CONNECTIONS  175
#define IDI_SHELL_NEW_WEB_FOLDER   176
#define IDI_SHELL_VISUAL_SETTINGS  177
#define IDI_SHELL_NEW_WEB_PAGE     178
#define IDI_SHELL_REMOTE_CONNECTION        179
#define IDI_SHELL_WINDOW_ON_MONITOR        180
#define IDI_SHELL_DESKTOP_ON_MONITOR       181
#define IDI_SHELL_WINDOW_WITH_SIDEBAR      182
#define IDI_SHELL_WINDOW_WITHOUT_SIDEBAR   183
#define IDI_SHELL_MAXIMIZED_WINDOW         184
#define IDI_SHELL_FLOATING_WINDOWS         185
#define IDI_SHELL_ONE_CLICK_OPEN   186
#define IDI_SHELL_TWO_CLICK_OPEN   187
/* 188 - 190 not used on Windows */
#define IDI_SHELL_EMPTY_RECYCLE_BIN_95     191
#define IDI_SHELL_FULL_RECYCLE_BIN_95      192
#define IDI_SHELL_WEB_FOLDERS      193
#define IDI_SHELL_PASSWORDS        194
/* 195 not used on Windows */
#define IDI_SHELL_FAX              196
#define IDI_SHELL_DEFAULT_FAX      197
#define IDI_SHELL_DEFAULT_NETWORK_FAX      198
#define IDI_SHELL_NETWORK_FAX      199
#define IDI_SHELL_RESTRICTED       200
/* 201 - 209 not used on Windows */
#define IDI_SHELL_DEFAULT_FOLDER   210
/* 211 - 219 not used on Windows */
#define IDI_SHELL_USERS            220
#define IDI_SHELL_POWER_OFF        221
#define IDI_SHELL_DVD              222
#define IDI_SHELL_DOCUMENTS        223
#define IDI_SHELL_VIDEO_FILE       224
#define IDI_SHELL_AUDIO_FILE       225
#define IDI_SHELL_IMAGE_FILE       226
#define IDI_SHELL_MULTIMEDIA_FILE  227
#define IDI_SHELL_MUSIC_CD_95      228
#define IDI_SHELL_CARD_READER      229
#define IDI_SHELL_ZIP_DRIVE        230
#define IDI_SHELL_DOWNLOAD         231
#define IDI_SHELL_DOWNLOAD_EMBLEM  232
#define IDI_SHELL_REMOVABLE_DISK   233
#define IDI_SHELL_BAD_REMOVABLE_DISK       234
#define IDI_SHELL_MY_DOCUMENTS     235
#define IDI_SHELL_MY_PICTURES      236
#define IDI_SHELL_MY_MUSIC         237
#define IDI_SHELL_MY_VIDEOS        238
#define IDI_SHELL_MSN              239
#define IDI_SHELL_DELETE           240
#define IDI_SHELL_MOVE             241
#define IDI_SHELL_RENAME           242
#define IDI_SHELL_COPY             243
#define IDI_SHELL_UPLOAD           244
#define IDI_SHELL_PRINT            245
#define IDI_SHELL_PLAY             246
#define IDI_SHELL_INTERNET_RADIO   247
#define IDI_SHELL_UPLOAD_PHOTO     251
#define IDI_SHELL_PRINT_PHOTO      252
#define IDI_SHELL_EMPTY_RECYCLE_BIN2       254
#define IDI_SHELL_NEW_NETWORK_FOLDER       258
#define IDI_SHELL_WRITE_DISC       260
#define IDI_SHELL_DELETE_FILE      261
#define IDI_SHELL_ERASE_DISC       262
#define IDI_SHELL_HELP2            263
#define IDI_SHELL_TO_FOLDER        264
#define IDI_SHELL_BURN_DISC        266
#define IDI_SHELL_SHARED_FOLDER    267
#define IDI_SHELL_USERS_XP         269
#define IDI_SHELL_INSTALL_XP       271
#define IDI_SHELL_MULTIFUNCTION_PRINTER    272
#define IDI_SHELL_INTERNET         273
#define IDI_SHELL_WEB_CALENDAR     276
#define IDI_SHELL_USERS_XP2        279
/* 280 not used on Windows */
#define IDI_SHELL_SEARCH_WINDOW    281
/* 285 - 288 not used on Windows */
#define IDI_SHELL_HELP_FILE        289
#define IDI_SHELL_DVD_DRIVE        291
#define IDI_SHELL_HYBRID_CD        292
#define IDI_SHELL_UNKNOWN_DISC     293
#define IDI_SHELL_CD_ROM           294
#define IDI_SHELL_CD_R             295
#define IDI_SHELL_CD_RW            296
#define IDI_SHELL_DVD_RAM          297
#define IDI_SHELL_DVD_R            298
#define IDI_SHELL_OPTICAL_DISC     302
#define IDI_SHELL_DVD_ROM          304
#define IDI_SHELL_INTERNET_PRINTER 311
#define IDI_SHELL_JAZ_DRIVE        312
#define IDI_SHELL_ZIP_DRIVE2       313
#define IDI_SHELL_DVD_RW           318
#define IDI_SHELL_NEW_FOLDER       319
#define IDI_SHELL_BURN_DISC2       320
#define IDI_SHELL_CONTROL_PANEL_XP2        321
#define IDI_SHELL_FAVORITES_SMALL  322
#define IDI_SHELL_SEARCH_SMALL     323
#define IDI_SHELL_HELP_SMALL       324
#define IDI_SHELL_LOG_OFF_SMALL    325
#define IDI_SHELL_APP_FOLDER_SMALL 326
#define IDI_SHELL_RECENT_SMALL     327
#define IDI_SHELL_RUN_SMALL        328
#define IDI_SHELL_POWER_OFF_SMALL  329
#define IDI_SHELL_CONTROL_PANEL_SMALL      330
#define IDI_SHELL_EJECT_SMALL      331
/* 332 - 336 not used on Windows */
#define IDI_SHELL_SEARCH_DIRECTORY 337
#define IDI_SHELL_RESTRICTED2      338
/* 339 - 511 not used on Windows */
#define IDI_SHELL_WEB_BROWSER      512

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
#define IDB_SORT_INCR           133
#define IDB_SORT_DECR           134

/* explorer toolbar icons
 * FIXME: images are hacky and should be re-drawn; also dark and light bitmaps are same for now
 */
#define IDB_TB_LARGE_LIGHT      214
#define IDB_TB_LARGE_DARK       215
#define IDB_TB_SMALL_LIGHT      216
#define IDB_TB_SMALL_DARK       217

#endif
