/*
 *  Constants, used in resources.
 *
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
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

#define MAIN_MENU               0x201
#define DIALOG_PAGESETUP        0x202
#define ID_ACCEL                0x203
#define DIALOG_GOTO             0x204

#define IDI_NOTEPAD             0x300

/* Commands */
#define CMD_NEW                 0x100
#define CMD_OPEN                0x101
#define CMD_SAVE                0x102
#define CMD_SAVE_AS             0x103
#define CMD_PRINT               0x104
#define CMD_PAGE_SETUP          0x105
#define CMD_PRINTER_SETUP       0x106
#define CMD_EXIT                0x108

#define CMD_UNDO                0x110
#define CMD_CUT                 0x111
#define CMD_COPY                0x112
#define CMD_PASTE               0x113
#define CMD_DELETE              0x114
#define CMD_SELECT_ALL          0x116
#define CMD_TIME_DATE           0x117

#define CMD_SEARCH              0x120
#define CMD_SEARCH_NEXT         0x121
#define CMD_REPLACE             0x122
#define CMD_GO_TO               0x192

#define CMD_WRAP                0x119
#define CMD_FONT                0x140
#define CMD_SBAR                0x205

#define CMD_HELP_CONTENTS       0x130
#define CMD_HELP_ABOUT_NOTEPAD  0x134

/* Control IDs */
#define IDC_PAGESETUP_HEADERVALUE 0x141
#define IDC_PAGESETUP_FOOTERVALUE 0x143
#define IDC_PAGESETUP_LEFTVALUE   0x147
#define IDC_PAGESETUP_RIGHTVALUE  0x14A
#define IDC_PAGESETUP_TOPVALUE    0x14D
#define IDC_PAGESETUP_BOTTOMVALUE 0x150
#define IDC_GOTO_LINEVALUE        0x194

/* Strings */
#define STRING_PAGESETUP_HEADERVALUE 0x160
#define STRING_PAGESETUP_FOOTERVALUE 0x161

#define STRING_NOTEPAD 0x170
#define STRING_ERROR 0x171
#define STRING_UNTITLED 0x174
#define STRING_ALL_FILES 0x175
#define STRING_TEXT_FILES_TXT 0x176
#define STRING_DOESNOTEXIST 0x179
#define STRING_NOTSAVED 0x17A

#define STRING_NOTFOUND 0x17B

#define STRING_STATUSBAR       0x206

#define STRING_UNICODE_LE      0x180
#define STRING_UNICODE_BE      0x181
#define STRING_UTF8            0x182

#define STRING_LOSS_OF_UNICODE_CHARACTERS 0x183

/* Open/Save As dialog template */
#define IDD_OFN_TEMPLATE       0x190
#define IDC_OFN_ENCCOMBO       0x191
