/*
 * wineconsole resource definitions
 *
 * Copyright 2001 Eric Pouech
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

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

/* strings */
#define IDS_EDIT		0x100
#define IDS_DEFAULT		0x101
#define IDS_PROPERTIES		0x102

#define IDS_MARK		0x110
#define IDS_COPY		0x111
#define IDS_PASTE		0x112
#define IDS_SELECTALL		0x113
#define IDS_SCROLL		0x114
#define IDS_SEARCH		0x115

#define IDS_DLG_TIT_DEFAULT     0x120
#define IDS_DLG_TIT_CURRENT     0x121
#define IDS_DLG_TIT_ERROR       0x122

#define IDS_DLG_ERR_SBWINSIZE   0x130

#define IDS_FNT_DISPLAY		0x200
#define IDS_FNT_PREVIEW         0x201

#define IDS_CMD_INVALID_EVENT_ID   0x300
#define IDS_CMD_INVALID_BACKEND    0x301
#define IDS_CMD_INVALID_OPTION     0x302
#define IDS_CMD_ABOUT              0x303
#define IDS_CMD_LAUNCH_FAILED      0x304

#define IDS_USAGE_HEADER           0x310
#define IDS_USAGE_COMMAND          0x312
#define IDS_USAGE_FOOTER           0x313

/* dialog boxes */
#define IDD_OPTION		0x0100
#define IDD_FONT		0x0200
#define IDD_CONFIG		0x0300
#define IDD_SAVE_SETTINGS       0x0400

/* dialog boxes controls */
#define IDC_OPT_CURSOR_SMALL	0x0101
#define IDC_OPT_CURSOR_MEDIUM	0x0102
#define IDC_OPT_CURSOR_LARGE	0x0103
#define IDC_OPT_HIST_SIZE	0x0104
#define IDC_OPT_HIST_SIZE_UD	0x0105
#define IDC_OPT_HIST_NODOUBLE	0x0106
#define IDC_OPT_CONF_CTRL       0x0107
#define IDC_OPT_CONF_SHIFT      0x0108
#define IDC_OPT_QUICK_EDIT      0x0109
#define IDC_OPT_INSERT_MODE     0x0110

#define IDC_FNT_LIST_FONT	0x0201
#define IDC_FNT_LIST_SIZE	0x0202
#define IDC_FNT_COLOR_BK 	0x0203
#define IDC_FNT_COLOR_FG 	0x0204
#define IDC_FNT_FONT_INFO	0x0205
#define IDC_FNT_PREVIEW		0x0206

#define IDC_CNF_SB_WIDTH	0x0301
#define IDC_CNF_SB_WIDTH_UD	0x0302
#define IDC_CNF_SB_HEIGHT	0x0303
#define IDC_CNF_SB_HEIGHT_UD	0x0304
#define IDC_CNF_WIN_WIDTH	0x0305
#define IDC_CNF_WIN_WIDTH_UD	0x0306
#define IDC_CNF_WIN_HEIGHT	0x0307
#define IDC_CNF_WIN_HEIGHT_UD	0x0308
#define IDC_CNF_CLOSE_EXIT      0x0309
#define IDC_CNF_EDITION_MODE    0x030a

#define IDC_SAV_SAVE            0x0401
#define IDC_SAV_SESSION         0x0402
