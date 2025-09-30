/*
 * USER Input processing
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Albrecht Kleine
 * Copyright 1996 Frans van Dorsselaer
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 2001 Eric Pouech
 * Copyright 2002 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "kbd.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);

static const WCHAR keyboard_layouts_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','s'
};
static const WCHAR escW[] = {'E','s','c',0};
static const WCHAR backspaceW[] = {'B','a','c','k','s','p','a','c','e',0};
static const WCHAR tabW[] = {'T','a','b',0};
static const WCHAR enterW[] = {'E','n','t','e','r',0};
static const WCHAR ctrlW[] = {'C','t','r','l',0};
static const WCHAR shiftW[] = {'S','h','i','f','t',0};
static const WCHAR right_shiftW[] = {'R','i','g','h','t',' ','S','h','i','f','t',0};
static const WCHAR num_mulW[] = {'N','u','m',' ','*',0};
static const WCHAR altW[] = {'A','l','t',0};
static const WCHAR spaceW[] = {'S','p','a','c','e',0};
static const WCHAR caps_lockW[] = {'C','a','p','s',' ','L','o','c','k',0};
static const WCHAR f1W[] = {'F','1',0};
static const WCHAR f2W[] = {'F','2',0};
static const WCHAR f3W[] = {'F','3',0};
static const WCHAR f4W[] = {'F','4',0};
static const WCHAR f5W[] = {'F','5',0};
static const WCHAR f6W[] = {'F','6',0};
static const WCHAR f7W[] = {'F','7',0};
static const WCHAR f8W[] = {'F','8',0};
static const WCHAR f9W[] = {'F','9',0};
static const WCHAR f10W[] = {'F','1','0',0};
static const WCHAR pauseW[] = {'P','a','u','s','e',0};
static const WCHAR scroll_lockW[] = {'S','c','r','o','l','l',' ','L','o','c','k',0};
static const WCHAR num_7W[] = {'N','u','m',' ','7',0};
static const WCHAR num_8W[] = {'N','u','m',' ','8',0};
static const WCHAR num_9W[] = {'N','u','m',' ','9',0};
static const WCHAR num_minusW[] = {'N','u','m',' ','-',0};
static const WCHAR num_4W[] = {'N','u','m',' ','4',0};
static const WCHAR num_5W[] = {'N','u','m',' ','5',0};
static const WCHAR num_6W[] = {'N','u','m',' ','6',0};
static const WCHAR num_plusW[] = {'N','u','m',' ','+',0};
static const WCHAR num_1W[] = {'N','u','m',' ','1',0};
static const WCHAR num_2W[] = {'N','u','m',' ','2',0};
static const WCHAR num_3W[] = {'N','u','m',' ','3',0};
static const WCHAR num_0W[] = {'N','u','m',' ','0',0};
static const WCHAR num_delW[] = {'N','u','m',' ','D','e','l',0};
static const WCHAR sys_reqW[] = {'S','y','s',' ','R','e','q',0};
static const WCHAR f11W[] = {'F','1','1',0};
static const WCHAR f12W[] = {'F','1','2',0};
static const WCHAR f13W[] = {'F','1','3',0};
static const WCHAR f14W[] = {'F','1','4',0};
static const WCHAR f15W[] = {'F','1','5',0};
static const WCHAR f16W[] = {'F','1','6',0};
static const WCHAR f17W[] = {'F','1','7',0};
static const WCHAR f18W[] = {'F','1','8',0};
static const WCHAR f19W[] = {'F','1','9',0};
static const WCHAR f20W[] = {'F','2','0',0};
static const WCHAR f21W[] = {'F','2','1',0};
static const WCHAR f22W[] = {'F','2','2',0};
static const WCHAR f23W[] = {'F','2','3',0};
static const WCHAR f24W[] = {'F','2','4',0};
static const WCHAR num_enterW[] = {'N','u','m',' ','E','n','t','e','r',0};
static const WCHAR right_ctrlW[] = {'R','i','g','h','t',' ','C','t','r','l',0};
static const WCHAR num_divW[] = {'N','u','m',' ','/',0};
static const WCHAR prnt_scrnW[] = {'P','r','n','t',' ','S','c','r','n',0};
static const WCHAR right_altW[] = {'R','i','g','h','t',' ','A','l','t',0};
static const WCHAR num_lockW[] = {'N','u','m',' ','L','o','c','k',0};
static const WCHAR breakW[] = {'B','r','e','a','k',0};
static const WCHAR homeW[] = {'H','o','m','e',0};
static const WCHAR upW[] = {'U','p',0};
static const WCHAR page_upW[] = {'P','a','g','e',' ','U','p',0};
static const WCHAR leftW[] = {'L','e','f','t',0};
static const WCHAR rightW[] = {'R','i','g','h','t',0};
static const WCHAR endW[] = {'E','n','d',0};
static const WCHAR downW[] = {'D','o','w','n',0};
static const WCHAR page_downW[] = {'P','a','g','e',' ','D','o','w','n',0};
static const WCHAR insertW[] = {'I','n','s','e','r','t',0};
static const WCHAR deleteW[] = {'D','e','l','e','t','e',0};
static const WCHAR zerozeroW[] = {'<','0','0','>',0};
static const WCHAR helpW[] = {'H','e','l','p',0};
static const WCHAR left_windowsW[] = {'L','e','f','t',' ','W','i','n','d','o','w','s',0};
static const WCHAR right_windowsW[] = {'R','i','g','h','t',' ','W','i','n','d','o','w','s',0};
static const WCHAR applicationW[] = {'A','p','p','l','i','c','a','t','i','o','n',0};

static const VK_TO_BIT vk_to_bit[] =
{
    {.Vk = VK_SHIFT, .ModBits = KBDSHIFT},
    {.Vk = VK_CONTROL, .ModBits = KBDCTRL},
    {.Vk = VK_MENU, .ModBits = KBDALT},
    {0},
};

static const MODIFIERS modifiers =
{
    .pVkToBit = (VK_TO_BIT *)vk_to_bit,
    .wMaxModBits = 7,
    .ModNumber = {0, 1, 2, 3, 0, 1, 0, 0},
};

static const VK_TO_WCHARS2 vk_to_wchars2[] =
{
    {.VirtualKey = VK_OEM_3,      .wch = {'`', '~'}},
    {.VirtualKey = '1',           .wch = {'1', '!'}},
    {.VirtualKey = '3',           .wch = {'3', '#'}},
    {.VirtualKey = '4',           .wch = {'4', '$'}},
    {.VirtualKey = '5',           .wch = {'5', '%'}},
    {.VirtualKey = '7',           .wch = {'7', '&'}},
    {.VirtualKey = '8',           .wch = {'8', '*'}},
    {.VirtualKey = '9',           .wch = {'9', '('}},
    {.VirtualKey = '0',           .wch = {'0', ')'}},
    {.VirtualKey = VK_OEM_PLUS,   .wch = {'=', '+'}},
    {.VirtualKey = 'Q',           .wch = {'q', 'Q'}, .Attributes = CAPLOK},
    {.VirtualKey = 'W',           .wch = {'w', 'W'}, .Attributes = CAPLOK},
    {.VirtualKey = 'E',           .wch = {'e', 'E'}, .Attributes = CAPLOK},
    {.VirtualKey = 'R',           .wch = {'r', 'R'}, .Attributes = CAPLOK},
    {.VirtualKey = 'T',           .wch = {'t', 'T'}, .Attributes = CAPLOK},
    {.VirtualKey = 'Y',           .wch = {'y', 'Y'}, .Attributes = CAPLOK},
    {.VirtualKey = 'U',           .wch = {'u', 'U'}, .Attributes = CAPLOK},
    {.VirtualKey = 'I',           .wch = {'i', 'I'}, .Attributes = CAPLOK},
    {.VirtualKey = 'O',           .wch = {'o', 'O'}, .Attributes = CAPLOK},
    {.VirtualKey = 'P',           .wch = {'p', 'P'}, .Attributes = CAPLOK},
    {.VirtualKey = 'A',           .wch = {'a', 'A'}, .Attributes = CAPLOK},
    {.VirtualKey = 'S',           .wch = {'s', 'S'}, .Attributes = CAPLOK},
    {.VirtualKey = 'D',           .wch = {'d', 'D'}, .Attributes = CAPLOK},
    {.VirtualKey = 'F',           .wch = {'f', 'F'}, .Attributes = CAPLOK},
    {.VirtualKey = 'G',           .wch = {'g', 'G'}, .Attributes = CAPLOK},
    {.VirtualKey = 'H',           .wch = {'h', 'H'}, .Attributes = CAPLOK},
    {.VirtualKey = 'J',           .wch = {'j', 'J'}, .Attributes = CAPLOK},
    {.VirtualKey = 'K',           .wch = {'k', 'K'}, .Attributes = CAPLOK},
    {.VirtualKey = 'L',           .wch = {'l', 'L'}, .Attributes = CAPLOK},
    {.VirtualKey = VK_OEM_1,      .wch = {';', ':'}},
    {.VirtualKey = VK_OEM_7,      .wch = {'\'', '\"'}},
    {.VirtualKey = 'Z',           .wch = {'z', 'Z'}, .Attributes = CAPLOK},
    {.VirtualKey = 'X',           .wch = {'x', 'X'}, .Attributes = CAPLOK},
    {.VirtualKey = 'C',           .wch = {'c', 'C'}, .Attributes = CAPLOK},
    {.VirtualKey = 'V',           .wch = {'v', 'V'}, .Attributes = CAPLOK},
    {.VirtualKey = 'B',           .wch = {'b', 'B'}, .Attributes = CAPLOK},
    {.VirtualKey = 'N',           .wch = {'n', 'N'}, .Attributes = CAPLOK},
    {.VirtualKey = 'M',           .wch = {'m', 'M'}, .Attributes = CAPLOK},
    {.VirtualKey = VK_OEM_COMMA,  .wch = {',', '<'}},
    {.VirtualKey = VK_OEM_PERIOD, .wch = {'.', '>'}},
    {.VirtualKey = VK_OEM_2,      .wch = {'/', '?'}},
    {.VirtualKey = VK_DECIMAL,    .wch = {'.', '.'}},
    {.VirtualKey = VK_TAB,        .wch = {'\t', '\t'}},
    {.VirtualKey = VK_ADD,        .wch = {'+', '+'}},
    {.VirtualKey = VK_DIVIDE,     .wch = {'/', '/'}},
    {.VirtualKey = VK_MULTIPLY,   .wch = {'*', '*'}},
    {.VirtualKey = VK_SUBTRACT,   .wch = {'-', '-'}},
    {0},
};

static const VK_TO_WCHARS3 vk_to_wchars3[] =
{
    {.VirtualKey = VK_OEM_4,   .wch = {'[', '{', '\x001b'}},
    {.VirtualKey = VK_OEM_6,   .wch = {']', '}', '\x001d'}},
    {.VirtualKey = VK_OEM_5,   .wch = {'\\', '|', '\x001c'}},
    {.VirtualKey = VK_OEM_102, .wch = {'\\', '|', '\x001c'}},
    {.VirtualKey = VK_BACK,    .wch = {'\b', '\b', '\x007f'}},
    {.VirtualKey = VK_ESCAPE,  .wch = {'\x001b', '\x001b', '\x001b'}},
    {.VirtualKey = VK_RETURN,  .wch = {'\r', '\r', '\n'}},
    {.VirtualKey = VK_SPACE,   .wch = {' ', ' ', ' '}},
    {.VirtualKey = VK_CANCEL,  .wch = {'\x0003', '\x0003', '\x0003'}},
    {0},
};

static const VK_TO_WCHARS4 vk_to_wchars4[] =
{
    {.VirtualKey = '2',          .wch = {'2', '@', WCH_NONE, '\x0000'}},
    {.VirtualKey = '6',          .wch = {'6', '^', WCH_NONE, '\x001e'}},
    {.VirtualKey = VK_OEM_MINUS, .wch = {'-', '_', WCH_NONE, '\x001f'}},
    {0},
};

static const VK_TO_WCHARS1 vk_to_wchars1[] =
{
    {.VirtualKey = VK_NUMPAD0, .wch = {'0'}},
    {.VirtualKey = VK_NUMPAD1, .wch = {'1'}},
    {.VirtualKey = VK_NUMPAD2, .wch = {'2'}},
    {.VirtualKey = VK_NUMPAD3, .wch = {'3'}},
    {.VirtualKey = VK_NUMPAD4, .wch = {'4'}},
    {.VirtualKey = VK_NUMPAD5, .wch = {'5'}},
    {.VirtualKey = VK_NUMPAD6, .wch = {'6'}},
    {.VirtualKey = VK_NUMPAD7, .wch = {'7'}},
    {.VirtualKey = VK_NUMPAD8, .wch = {'8'}},
    {.VirtualKey = VK_NUMPAD9, .wch = {'9'}},
    {0},
};

static const VK_TO_WCHAR_TABLE vk_to_wchar_table[] =
{
    {.pVkToWchars = (VK_TO_WCHARS1 *)vk_to_wchars3, .nModifications = 3, .cbSize = sizeof(vk_to_wchars3[0])},
    {.pVkToWchars = (VK_TO_WCHARS1 *)vk_to_wchars4, .nModifications = 4, .cbSize = sizeof(vk_to_wchars4[0])},
    {.pVkToWchars = (VK_TO_WCHARS1 *)vk_to_wchars2, .nModifications = 2, .cbSize = sizeof(vk_to_wchars2[0])},
    {.pVkToWchars = (VK_TO_WCHARS1 *)vk_to_wchars1, .nModifications = 1, .cbSize = sizeof(vk_to_wchars1[0])},
    {0},
};

static const VSC_LPWSTR key_names[] =
{
    {.vsc = 0x01, .pwsz = (WCHAR *)escW},
    {.vsc = 0x0e, .pwsz = (WCHAR *)backspaceW},
    {.vsc = 0x0f, .pwsz = (WCHAR *)tabW},
    {.vsc = 0x1c, .pwsz = (WCHAR *)enterW},
    {.vsc = 0x1d, .pwsz = (WCHAR *)ctrlW},
    {.vsc = 0x2a, .pwsz = (WCHAR *)shiftW},
    {.vsc = 0x36, .pwsz = (WCHAR *)right_shiftW},
    {.vsc = 0x37, .pwsz = (WCHAR *)num_mulW},
    {.vsc = 0x38, .pwsz = (WCHAR *)altW},
    {.vsc = 0x39, .pwsz = (WCHAR *)spaceW},
    {.vsc = 0x3a, .pwsz = (WCHAR *)caps_lockW},
    {.vsc = 0x3b, .pwsz = (WCHAR *)f1W},
    {.vsc = 0x3c, .pwsz = (WCHAR *)f2W},
    {.vsc = 0x3d, .pwsz = (WCHAR *)f3W},
    {.vsc = 0x3e, .pwsz = (WCHAR *)f4W},
    {.vsc = 0x3f, .pwsz = (WCHAR *)f5W},
    {.vsc = 0x40, .pwsz = (WCHAR *)f6W},
    {.vsc = 0x41, .pwsz = (WCHAR *)f7W},
    {.vsc = 0x42, .pwsz = (WCHAR *)f8W},
    {.vsc = 0x43, .pwsz = (WCHAR *)f9W},
    {.vsc = 0x44, .pwsz = (WCHAR *)f10W},
    {.vsc = 0x45, .pwsz = (WCHAR *)pauseW},
    {.vsc = 0x46, .pwsz = (WCHAR *)scroll_lockW},
    {.vsc = 0x47, .pwsz = (WCHAR *)num_7W},
    {.vsc = 0x48, .pwsz = (WCHAR *)num_8W},
    {.vsc = 0x49, .pwsz = (WCHAR *)num_9W},
    {.vsc = 0x4a, .pwsz = (WCHAR *)num_minusW},
    {.vsc = 0x4b, .pwsz = (WCHAR *)num_4W},
    {.vsc = 0x4c, .pwsz = (WCHAR *)num_5W},
    {.vsc = 0x4d, .pwsz = (WCHAR *)num_6W},
    {.vsc = 0x4e, .pwsz = (WCHAR *)num_plusW},
    {.vsc = 0x4f, .pwsz = (WCHAR *)num_1W},
    {.vsc = 0x50, .pwsz = (WCHAR *)num_2W},
    {.vsc = 0x51, .pwsz = (WCHAR *)num_3W},
    {.vsc = 0x52, .pwsz = (WCHAR *)num_0W},
    {.vsc = 0x53, .pwsz = (WCHAR *)num_delW},
    {.vsc = 0x54, .pwsz = (WCHAR *)sys_reqW},
    {.vsc = 0x57, .pwsz = (WCHAR *)f11W},
    {.vsc = 0x58, .pwsz = (WCHAR *)f12W},
    {.vsc = 0x7c, .pwsz = (WCHAR *)f13W},
    {.vsc = 0x7d, .pwsz = (WCHAR *)f14W},
    {.vsc = 0x7e, .pwsz = (WCHAR *)f15W},
    {.vsc = 0x7f, .pwsz = (WCHAR *)f16W},
    {.vsc = 0x80, .pwsz = (WCHAR *)f17W},
    {.vsc = 0x81, .pwsz = (WCHAR *)f18W},
    {.vsc = 0x82, .pwsz = (WCHAR *)f19W},
    {.vsc = 0x83, .pwsz = (WCHAR *)f20W},
    {.vsc = 0x84, .pwsz = (WCHAR *)f21W},
    {.vsc = 0x85, .pwsz = (WCHAR *)f22W},
    {.vsc = 0x86, .pwsz = (WCHAR *)f23W},
    {.vsc = 0x87, .pwsz = (WCHAR *)f24W},
    {0},
};

static const VSC_LPWSTR key_names_ext[] =
{
    {.vsc = 0x1c, .pwsz = (WCHAR *)num_enterW},
    {.vsc = 0x1d, .pwsz = (WCHAR *)right_ctrlW},
    {.vsc = 0x35, .pwsz = (WCHAR *)num_divW},
    {.vsc = 0x37, .pwsz = (WCHAR *)prnt_scrnW},
    {.vsc = 0x38, .pwsz = (WCHAR *)right_altW},
    {.vsc = 0x45, .pwsz = (WCHAR *)num_lockW},
    {.vsc = 0x46, .pwsz = (WCHAR *)breakW},
    {.vsc = 0x47, .pwsz = (WCHAR *)homeW},
    {.vsc = 0x48, .pwsz = (WCHAR *)upW},
    {.vsc = 0x49, .pwsz = (WCHAR *)page_upW},
    {.vsc = 0x4b, .pwsz = (WCHAR *)leftW},
    {.vsc = 0x4d, .pwsz = (WCHAR *)rightW},
    {.vsc = 0x4f, .pwsz = (WCHAR *)endW},
    {.vsc = 0x50, .pwsz = (WCHAR *)downW},
    {.vsc = 0x51, .pwsz = (WCHAR *)page_downW},
    {.vsc = 0x52, .pwsz = (WCHAR *)insertW},
    {.vsc = 0x53, .pwsz = (WCHAR *)deleteW},
    {.vsc = 0x54, .pwsz = (WCHAR *)zerozeroW},
    {.vsc = 0x56, .pwsz = (WCHAR *)helpW},
    {.vsc = 0x5b, .pwsz = (WCHAR *)left_windowsW},
    {.vsc = 0x5c, .pwsz = (WCHAR *)right_windowsW},
    {.vsc = 0x5d, .pwsz = (WCHAR *)applicationW},
    {0},
};

static const USHORT vsc_to_vk[] =
{
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,
    T10, T11, T12, T13, T14, T15, T16, T17,
    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,
    T20, T21, T22, T23, T24, T25, T26, T27,
    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,
    T30, T31, T32, T33, T34, T35, T36 | KBDEXT, T37 | KBDMULTIVK,
    T38, T39, T3A, T3B, T3C, T3D, T3E, T3F,
    T40, T41, T42, T43, T44, T45 | KBDEXT | KBDMULTIVK, T46 | KBDMULTIVK, T47 | KBDNUMPAD | KBDSPECIAL,
    T48 | KBDNUMPAD | KBDSPECIAL, T49 | KBDNUMPAD | KBDSPECIAL, T4A, T4B | KBDNUMPAD | KBDSPECIAL,
    T4C | KBDNUMPAD | KBDSPECIAL, T4D | KBDNUMPAD | KBDSPECIAL, T4E, T4F | KBDNUMPAD | KBDSPECIAL,
    T50 | KBDNUMPAD | KBDSPECIAL, T51 | KBDNUMPAD | KBDSPECIAL, T52 | KBDNUMPAD | KBDSPECIAL,
    T53 | KBDNUMPAD | KBDSPECIAL, T54, T55, T56, T57,
    T58, T59, T5A, T5B, T5C, T5D, T5E, T5F,
    T60, T61, T62, T63, T64, T65, T66, T67,
    T68, T69, T6A, T6B, T6C, T6D, T6E, T6F,
    T70, T71, T72, T73, T74, T75, T76, T77,
    T78, T79, T7A, T7B, T7C, T7D, T7E
};

static const VSC_VK vsc_to_vk_e0[] =
{
    {0x10, X10 | KBDEXT},
    {0x19, X19 | KBDEXT},
    {0x1d, X1D | KBDEXT},
    {0x20, X20 | KBDEXT},
    {0x21, X21 | KBDEXT},
    {0x22, X22 | KBDEXT},
    {0x24, X24 | KBDEXT},
    {0x2e, X2E | KBDEXT},
    {0x30, X30 | KBDEXT},
    {0x32, X32 | KBDEXT},
    {0x35, X35 | KBDEXT},
    {0x37, X37 | KBDEXT},
    {0x38, X38 | KBDEXT},
    {0x47, X47 | KBDEXT},
    {0x48, X48 | KBDEXT},
    {0x49, X49 | KBDEXT},
    {0x4b, X4B | KBDEXT},
    {0x4d, X4D | KBDEXT},
    {0x4f, X4F | KBDEXT},
    {0x50, X50 | KBDEXT},
    {0x51, X51 | KBDEXT},
    {0x52, X52 | KBDEXT},
    {0x53, X53 | KBDEXT},
    {0x5b, X5B | KBDEXT},
    {0x5c, X5C | KBDEXT},
    {0x5d, X5D | KBDEXT},
    {0x5f, X5F | KBDEXT},
    {0x65, X65 | KBDEXT},
    {0x66, X66 | KBDEXT},
    {0x67, X67 | KBDEXT},
    {0x68, X68 | KBDEXT},
    {0x69, X69 | KBDEXT},
    {0x6a, X6A | KBDEXT},
    {0x6b, X6B | KBDEXT},
    {0x6c, X6C | KBDEXT},
    {0x6d, X6D | KBDEXT},
    {0x1c, X1C | KBDEXT},
    {0x46, X46 | KBDEXT},
    {0},
};

static const VSC_VK vsc_to_vk_e1[] =
{
    {0x1d, Y1D},
    {0},
};

static const KBDTABLES kbdus_tables =
{
    .pCharModifiers = (MODIFIERS *)&modifiers,
    .pVkToWcharTable = (VK_TO_WCHAR_TABLE *)vk_to_wchar_table,
    .pKeyNames = (VSC_LPWSTR *)key_names,
    .pKeyNamesExt = (VSC_LPWSTR *)key_names_ext,
    .pusVSCtoVK = (USHORT *)vsc_to_vk,
    .bMaxVSCtoVK = ARRAY_SIZE(vsc_to_vk),
    .pVSCtoVK_E0 = (VSC_VK *)vsc_to_vk_e0,
    .pVSCtoVK_E1 = (VSC_VK *)vsc_to_vk_e1,
    .fLocaleFlags = MAKELONG(0, KBD_VERSION),
};

static LONG clipping_cursor; /* clipping thread counter */

BOOL grab_pointer = TRUE;
BOOL grab_fullscreen = FALSE;

static void kbd_tables_init_vsc2vk( const KBDTABLES *tables, USHORT vsc2vk[0x300] )
{
    const VSC_VK *entry;
    WORD vsc;

    memset( vsc2vk, 0, 0x300 * sizeof(USHORT) );

    for (vsc = 0; tables->pusVSCtoVK && vsc <= tables->bMaxVSCtoVK; ++vsc)
    {
        if (tables->pusVSCtoVK[vsc] == VK__none_) continue;
        vsc2vk[vsc] = tables->pusVSCtoVK[vsc];
    }
    for (entry = tables->pVSCtoVK_E0; entry && entry->Vsc; entry++)
    {
        if (entry->Vk == VK__none_) continue;
        vsc2vk[entry->Vsc + 0x100] = entry->Vk;
    }
    for (entry = tables->pVSCtoVK_E1; entry && entry->Vsc; entry++)
    {
        if (entry->Vk == VK__none_) continue;
        vsc2vk[entry->Vsc + 0x200] = entry->Vk;
    }
}

#define NEXT_ENTRY(t, e) ((void *)&(e)->wch[(t)->nModifications])

static void kbd_tables_init_vk2char( const KBDTABLES *tables, BYTE vk2char[0x100] )
{
    const VK_TO_WCHAR_TABLE *table;
    const VK_TO_WCHARS1 *entry;

    memset( vk2char, 0, 0x100 );

    for (table = tables->pVkToWcharTable; table->pVkToWchars; table++)
    {
        for (entry = table->pVkToWchars; entry->VirtualKey; entry = NEXT_ENTRY(table, entry))
        {
            if (entry->VirtualKey & ~0xff) continue;
            vk2char[entry->VirtualKey] = entry->wch[0];
        }
    }
}

static UINT kbd_tables_get_mod_bits( const KBDTABLES *tables, UINT mod )
{
    const MODIFIERS *mods = tables->pCharModifiers;
    WORD bits;

    for (bits = 0; bits <= mods->wMaxModBits; ++bits)
        if (mods->ModNumber[bits] == mod) return bits;

    return -1;
}

static UINT kbd_tables_get_mod_num( const KBDTABLES *tables, const BYTE *state, BOOL caps )
{
    const MODIFIERS *mods = tables->pCharModifiers;
    const VK_TO_BIT *entry;
    WORD bits = 0;

    for (entry = mods->pVkToBit; entry->Vk; ++entry)
        if (state[entry->Vk] & 0x80) bits |= entry->ModBits;
    if (caps) bits |= KBDSHIFT;

    if (bits > mods->wMaxModBits) return -1;
    return mods->ModNumber[bits];
}

static WORD kbd_tables_wchar_to_vkey( const KBDTABLES *tables, WCHAR wch )
{
    const VK_TO_WCHAR_TABLE *table;
    const VK_TO_WCHARS1 *entry;
    WORD bits;
    BYTE mod;

    if (wch == '\x001b') return VK_ESCAPE;

    for (table = tables->pVkToWcharTable; table->pVkToWchars; table++)
    {
        for (entry = table->pVkToWchars; entry->VirtualKey; entry = NEXT_ENTRY(table, entry))
        {
            for (mod = 0; mod < table->nModifications; ++mod)
            {
                if (entry->wch[mod] == WCH_NONE || entry->wch[mod] != wch) continue;
                bits = kbd_tables_get_mod_bits( tables, mod );
                return (bits << 8) | entry->VirtualKey;
            }
        }
    }

    if (wch >= 0x0001 && wch <= 0x001a) return (0x200) | ('A' + wch - 1);  /* CTRL + A-Z */
    return wch >= 0x0080 ? -1 : 0;
}

static WCHAR kbd_tables_vkey_to_wchar( const KBDTABLES *tables, UINT vkey, const BYTE *state )
{
    UINT mod, caps_mod, alt, ctrl, caps;
    const VK_TO_WCHAR_TABLE *table;
    const VK_TO_WCHARS1 *entry;

    alt = state[VK_MENU] & 0x80;
    ctrl = state[VK_CONTROL] & 0x80;
    caps = state[VK_CAPITAL] & 1;

    if (ctrl && alt && !(tables->fLocaleFlags & KLLF_ALTGR)) return WCH_NONE;
    if (!ctrl && vkey == VK_ESCAPE) return VK_ESCAPE;
    if (ctrl && !alt)
    {
        if (vkey >= 'A' && vkey <= 'Z') return vkey - 'A' + 1;
        tables = &kbdus_tables;
    }
    if (vkey >= VK_NUMPAD0 && vkey <= VK_NUMPAD9) tables = &kbdus_tables;

    mod = caps_mod = kbd_tables_get_mod_num( tables, state, FALSE );
    if (caps) caps_mod = kbd_tables_get_mod_num( tables, state, TRUE );

    for (table = tables->pVkToWcharTable; table->pVkToWchars; table++)
    {
        if (table->nModifications <= mod) continue;
        for (entry = table->pVkToWchars; entry->VirtualKey; entry = NEXT_ENTRY(table, entry))
        {
            if (entry->VirtualKey != vkey) continue;
            /* SGCAPS attribute may be set on entries where VK_CAPITAL and VK_SHIFT behave differently.
             * The entry corresponds to the mapping when Caps Lock is on, and a second entry follows it
             * with the mapping when Caps Lock is off.
             */
            if ((entry->Attributes & SGCAPS) && !caps) entry = NEXT_ENTRY(table, entry);
            if ((entry->Attributes & CAPLOK) && table->nModifications > caps_mod) return entry->wch[caps_mod];
            return entry->wch[mod];
        }
    }

    return WCH_NONE;
}

#undef NEXT_ENTRY

/*******************************************************************
 *           NtUserGetForegroundWindow  (win32u.@)
 */
HWND WINAPI NtUserGetForegroundWindow(void)
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const input_shm_t *input_shm;
    NTSTATUS status;
    HWND hwnd = 0;

    while ((status = get_shared_input( 0, &lock, &input_shm )) == STATUS_PENDING)
        hwnd = wine_server_ptr_handle( input_shm->active );
    if (status) hwnd = 0;

    return hwnd;
}

/* see GetActiveWindow */
HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}

/* see GetCapture */
HWND get_capture(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndCapture : 0;
}

/* see GetFocus */
HWND get_focus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}

/**********************************************************************
 *	     NtUserAttachThreadInput    (win32u.@)
 */
BOOL WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach )
{
    BOOL ret;

    SERVER_START_REQ( attach_thread_input )
    {
        req->tid_from = from;
        req->tid_to   = to;
        req->attach   = attach;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *		update_mouse_coords
 *
 * Helper for NtUserSendInput.
 */
static void update_mouse_coords( INPUT *input )
{
    if (!(input->mi.dwFlags & MOUSEEVENTF_MOVE)) return;

    if (input->mi.dwFlags & MOUSEEVENTF_ABSOLUTE)
    {
        RECT rc;

        if (input->mi.dwFlags & MOUSEEVENTF_VIRTUALDESK)
            rc = get_virtual_screen_rect( 0, MDT_DEFAULT );
        else
            rc = get_primary_monitor_rect( 0 );

        input->mi.dx = rc.left + ((input->mi.dx * (rc.right - rc.left)) >> 16);
        input->mi.dy = rc.top  + ((input->mi.dy * (rc.bottom - rc.top)) >> 16);
    }
    else
    {
        int accel[3];

        /* dx and dy can be negative numbers for relative movements */
        NtUserSystemParametersInfo( SPI_GETMOUSE, 0, accel, 0 );

        if (!accel[2]) return;

        if (abs( input->mi.dx ) > accel[0])
        {
            input->mi.dx *= 2;
            if (abs( input->mi.dx ) > accel[1] && accel[2] == 2) input->mi.dx *= 2;
        }
        if (abs(input->mi.dy) > accel[0])
        {
            input->mi.dy *= 2;
            if (abs( input->mi.dy ) > accel[1] && accel[2] == 2) input->mi.dy *= 2;
        }
    }
}

/***********************************************************************
 *           NtUserSendInput  (win32u.@)
 */
UINT WINAPI NtUserSendInput( UINT count, INPUT *inputs, int size )
{
    UINT i;
    NTSTATUS status = STATUS_SUCCESS;

    if (size != sizeof(INPUT))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!count)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!inputs)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        INPUT input = inputs[i];
        switch (input.type)
        {
        case INPUT_MOUSE:
            /* we need to update the coordinates to what the server expects */
            update_mouse_coords( &input );
            /* fallthrough */
        case INPUT_KEYBOARD:
            status = send_hardware_message( 0, SEND_HWMSG_INJECTED, &input, 0 );
            break;
        case INPUT_HARDWARE:
            RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
            return 0;
        }

        if (status)
        {
            RtlSetLastWin32Error( RtlNtStatusToDosError(status) );
            break;
        }
    }

    return i;
}

/***********************************************************************
 *	     NtUserSetCursorPos (win32u.@)
 */
BOOL WINAPI NtUserSetCursorPos( INT x, INT y )
{
    RECT rect = {x, y, x, y};
    BOOL ret;
    INT prev_x, prev_y, new_x, new_y;

    rect = map_rect_virt_to_raw( rect, get_thread_dpi() );
    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_POS;
        req->x     = rect.left;
        req->y     = rect.top;
        if ((ret = !wine_server_call( req )))
        {
            prev_x = reply->prev_x;
            prev_y = reply->prev_y;
            new_x  = reply->new_x;
            new_y  = reply->new_y;
        }
    }
    SERVER_END_REQ;
    if (ret && (prev_x != new_x || prev_y != new_y)) user_driver->pSetCursorPos( new_x, new_y );
    return ret;
}

/***********************************************************************
 *	     NtUserGetCursorPos (win32u.@)
 */
BOOL WINAPI NtUserGetCursorPos( POINT *pt )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const desktop_shm_t *desktop_shm;
    BOOL ret = TRUE;
    DWORD last_change = 0;
    NTSTATUS status;
    RECT rect;

    if (!pt) return FALSE;

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
    {
        pt->x = desktop_shm->cursor.x;
        pt->y = desktop_shm->cursor.y;
        last_change = desktop_shm->cursor.last_change;
    }
    if (status) return FALSE;

    /* query new position from graphics driver if we haven't updated recently */
    if (NtGetTickCount() - last_change > 100) ret = user_driver->pGetCursorPos( pt );
    if (!ret) return FALSE;

    SetRect( &rect, pt->x, pt->y, pt->x, pt->y );
    rect = map_rect_raw_to_virt( rect, get_thread_dpi() );
    *pt = *(POINT *)&rect.left;
    return ret;
}

/***********************************************************************
 *	     NtUserGetCursorInfo (win32u.@)
 */
BOOL WINAPI NtUserGetCursorInfo( CURSORINFO *info )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const input_shm_t *input_shm;
    NTSTATUS status;

    if (!info) return FALSE;

    while ((status = get_shared_input( 0, &lock, &input_shm )) == STATUS_PENDING)
    {
        info->hCursor = wine_server_ptr_handle( input_shm->cursor );
        info->flags = (input_shm->cursor_count >= 0) ? CURSOR_SHOWING : 0;
    }
    if (status)
    {
        info->hCursor = 0;
        info->flags = CURSOR_SHOWING;
    }

    NtUserGetCursorPos( &info->ptScreenPos );
    return TRUE;
}

/**********************************************************************
 *           GetAsyncKeyState (win32u.@)
 */
SHORT WINAPI NtUserGetAsyncKeyState( INT key )
{
    const desktop_shm_t *desktop_shm;
    struct object_lock lock = OBJECT_LOCK_INIT;
    NTSTATUS status;
    BYTE state = 0;
    SHORT ret = 0;

    if (key < 0 || key >= 256) return 0;

    check_for_events( QS_INPUT );

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
        state = desktop_shm->keystate[key];

    if (status) return 0;
    if (!(state & 0x40)) return (state & 0x80) << 8;

    /* Need to make a server call to reset the last pressed bit */
    SERVER_START_REQ( get_key_state )
    {
        req->async = 1;
        req->key = key;
        if (!wine_server_call( req ))
        {
            if (reply->state & 0x40) ret |= 0x0001;
            if (reply->state & 0x80) ret |= 0x8000;
        }
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           get_shared_queue_bits
 */
static BOOL get_shared_queue_bits( UINT *wake_bits, UINT *changed_bits )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const queue_shm_t *queue_shm;
    UINT status;

    *wake_bits = *changed_bits = 0;
    while ((status = get_shared_queue( &lock, &queue_shm )) == STATUS_PENDING)
    {
        *wake_bits = queue_shm->wake_bits;
        *changed_bits = queue_shm->changed_bits;
    }

    if (status) return FALSE;
    return TRUE;
}

/***********************************************************************
 *           NtUserGetQueueStatus (win32u.@)
 */
DWORD WINAPI NtUserGetQueueStatus( UINT flags )
{
    UINT ret, wake_bits, changed_bits;

    if (flags & ~(QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_SMRESULT))
    {
        RtlSetLastWin32Error( ERROR_INVALID_FLAGS );
        return 0;
    }

    check_for_events( flags );

    if (get_shared_queue_bits( &wake_bits, &changed_bits ) && !(changed_bits & flags))
        ret = MAKELONG( changed_bits & flags, wake_bits & flags );
    else SERVER_START_REQ( get_queue_status )
    {
        req->clear_bits = flags;
        wine_server_call( req );
        ret = MAKELONG( reply->changed_bits & flags, reply->wake_bits & flags );
    }
    SERVER_END_REQ;
    return ret;
}

/*******************************************************************
 *           NtUserGetThreadInfo (win32u.@)
 */
ULONG_PTR WINAPI NtUserGetThreadState( USERTHREADSTATECLASS cls )
{
    GUITHREADINFO info;

    switch (cls)
    {
    case UserThreadStateFocusWindow:
        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
        return (ULONG_PTR)info.hwndFocus;

    case UserThreadStateActiveWindow:
        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
        return (ULONG_PTR)info.hwndActive;

    case UserThreadStateCaptureWindow:
        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
        return (ULONG_PTR)info.hwndCapture;

    case UserThreadStateDefaultImeWindow:
        return (ULONG_PTR)get_default_ime_window( 0 );

    case UserThreadStateDefaultInputContext:
        return NtUserGetThreadInfo()->default_imc;

    case UserThreadStateInputState:
        return get_input_state();

    case UserThreadStateCursor:
        return (ULONG_PTR)NtUserGetCursor();

    case UserThreadStateExtraInfo:
        return NtUserGetThreadInfo()->message_extra;

    case UserThreadStateInSendMessage:
        return NtUserGetThreadInfo()->receive_flags;

    case UserThreadStateMessageTime:
        return NtUserGetThreadInfo()->message_time;

    case UserThreadStateIsForeground:
    default:
        WARN( "unsupported class %u\n", cls );
        return 0;
    }
}

/***********************************************************************
 *           get_input_state
 */
DWORD get_input_state(void)
{
    UINT wake_bits, changed_bits;

    check_for_events( QS_INPUT );

    if (!get_shared_queue_bits( &wake_bits, &changed_bits )) return 0;
    return wake_bits & (QS_KEY | QS_MOUSEBUTTON);
}

/***********************************************************************
 *           get_last_input_time
 */
DWORD get_last_input_time(void)
{
    DWORD ret;

    SERVER_START_REQ( get_last_input_time )
    {
        wine_server_call( req );
        ret = reply->time;
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           get_locale_kbd_layout
 */
static HKL get_locale_kbd_layout(void)
{
    LCID layout;

    /* FIXME:
     *
     * layout = main_key_tab[kbd_layout].lcid;
     *
     * Winword uses return value of GetKeyboardLayout as a codepage
     * to translate ANSI keyboard messages to unicode. But we have
     * a problem with it: for instance Polish keyboard layout is
     * identical to the US one, and therefore instead of the Polish
     * locale id we return the US one.
     */

    NtQueryDefaultLocale( TRUE, &layout );
    layout = MAKELONG( layout, layout );
    return ULongToHandle( layout );
}

/***********************************************************************
 *	     NtUserGetKeyboardLayout    (win32u.@)
 *
 * Device handle for keyboard layout defaulted to
 * the language id. This is the way Windows default works.
 */
HKL WINAPI NtUserGetKeyboardLayout( DWORD thread_id )
{
    struct user_thread_info *thread = get_user_thread_info();
    HKL layout = thread->kbd_layout;

    if (thread_id && thread_id != GetCurrentThreadId())
        FIXME( "couldn't return keyboard layout for thread %04x\n", thread_id );

    if (!layout) return get_locale_kbd_layout();
    return layout;
}

/**********************************************************************
 *	     NtUserGetKeyState    (win32u.@)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.
 */
SHORT WINAPI NtUserGetKeyState( INT vkey )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const input_shm_t *input_shm;
    UINT64 keystate_serial = 0;
    BOOL ret = FALSE;
    SHORT retval = 0;
    NTSTATUS status;

    while ((status = get_shared_input( GetCurrentThreadId(), &lock, &input_shm )) == STATUS_PENDING)
    {
        ret = !!input_shm->keystate_lock; /* needs a request for sync_input_keystate if desktop keystate differs. */
        keystate_serial = input_shm->keystate_serial;
        retval = (signed char)(input_shm->keystate[vkey & 0xff] & 0x81);
    }

    if (!ret)
    {
        struct object_lock lock = OBJECT_LOCK_INIT;
        const desktop_shm_t *desktop_shm;

        while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
            ret = keystate_serial == desktop_shm->keystate_serial;
    }

    if (!ret) SERVER_START_REQ( get_key_state )
    {
        req->key = vkey;
        if (!wine_server_call( req )) retval = (signed char)(reply->state & 0x81);
    }
    SERVER_END_REQ;
    TRACE("key (0x%x) -> %x\n", vkey, retval);
    return retval;
}

/**********************************************************************
 *	     NtUserGetKeyboardState    (win32u.@)
 */
BOOL WINAPI NtUserGetKeyboardState( BYTE *state )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const input_shm_t *input_shm;
    NTSTATUS status;
    UINT i;

    TRACE("(%p)\n", state);

    while ((status = get_shared_input( GetCurrentThreadId(), &lock, &input_shm )) == STATUS_PENDING)
        memcpy( state, (const void *)input_shm->keystate, 256 );
    if (status) memset( state, 0, 256 );

    for (i = 0; i < 256; i++) state[i] &= 0x81;
    return TRUE;
}

/***********************************************************************
 *           get_async_keyboard_state
 */
BOOL get_async_keyboard_state( BYTE state[256] )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const desktop_shm_t *desktop_shm;
    NTSTATUS status;

    TRACE("(%p)\n", state);

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
        memcpy( state, (const void *)desktop_shm->keystate, 256 );
    if (status) memset( state, 0, 256 );

    return !status;
}

/**********************************************************************
 *	     NtUserSetKeyboardState    (win32u.@)
 */
BOOL WINAPI NtUserSetKeyboardState( BYTE *state )
{
    BOOL ret;

    SERVER_START_REQ( set_key_state )
    {
        wine_server_add_data( req, state, 256 );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *	     NtUserVkKeyScanEx    (win32u.@)
 */
WORD WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout )
{
    const KBDTABLES *kbd_tables;
    SHORT ret;

    TRACE_(keyboard)( "chr %s, layout %p\n", debugstr_wn(&chr, 1), layout );

    if ((ret = user_driver->pVkKeyScanEx( chr, layout )) != -256) return ret;

    if (!(kbd_tables = user_driver->pKbdLayerDescriptor( layout ))) kbd_tables = &kbdus_tables;
    ret = kbd_tables_wchar_to_vkey( kbd_tables, chr );
    if (kbd_tables != &kbdus_tables) user_driver->pReleaseKbdTables( kbd_tables );

    TRACE_(keyboard)( "ret %04x\n", ret );
    return ret;
}


/******************************************************************************
 *	     NtUserMapVirtualKeyEx    (win32u.@)
 */
UINT WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    USHORT vsc2vk[0x300];
    BYTE vk2char[0x100];
    const KBDTABLES *kbd_tables;
    UINT ret = 0;

    TRACE_(keyboard)( "code %u, type %u, layout %p.\n", code, type, layout );

    if ((ret = user_driver->pMapVirtualKeyEx( code, type, layout )) != -1) return ret;

    if (!(kbd_tables = user_driver->pKbdLayerDescriptor( layout ))) kbd_tables = &kbdus_tables;

    switch (type)
    {
    case MAPVK_VK_TO_VSC_EX:
    case MAPVK_VK_TO_VSC:
        switch (code)
        {
        case VK_SHIFT:   code = VK_LSHIFT; break;
        case VK_CONTROL: code = VK_LCONTROL; break;
        case VK_MENU:    code = VK_LMENU; break;
        case VK_NUMPAD0: code = VK_INSERT; break;
        case VK_NUMPAD1: code = VK_END; break;
        case VK_NUMPAD2: code = VK_DOWN; break;
        case VK_NUMPAD3: code = VK_NEXT; break;
        case VK_NUMPAD4: code = VK_LEFT; break;
        case VK_NUMPAD5: code = VK_CLEAR; break;
        case VK_NUMPAD6: code = VK_RIGHT; break;
        case VK_NUMPAD7: code = VK_HOME; break;
        case VK_NUMPAD8: code = VK_UP; break;
        case VK_NUMPAD9: code = VK_PRIOR; break;
        case VK_DECIMAL: code = VK_DELETE; break;
        }

        kbd_tables_init_vsc2vk( kbd_tables, vsc2vk );
        for (ret = 0; ret < ARRAY_SIZE(vsc2vk); ++ret) if ((vsc2vk[ret] & 0xff) == code) break;
        if (ret >= ARRAY_SIZE(vsc2vk)) ret = 0;

        if (type == MAPVK_VK_TO_VSC)
        {
            if (ret >= 0x200) ret = 0;
            else ret &= 0xff;
        }
        else if (ret >= 0x100) ret += 0xdf00;
        break;
    case MAPVK_VSC_TO_VK:
    case MAPVK_VSC_TO_VK_EX:
        kbd_tables_init_vsc2vk( kbd_tables, vsc2vk );

        if (code & 0xe000) code -= 0xdf00;
        if (code >= ARRAY_SIZE(vsc2vk)) ret = 0;
        else ret = vsc2vk[code] & 0xff;

        if (type == MAPVK_VSC_TO_VK)
        {
            switch (ret)
            {
            case VK_LSHIFT:   case VK_RSHIFT:   ret = VK_SHIFT; break;
            case VK_LCONTROL: case VK_RCONTROL: ret = VK_CONTROL; break;
            case VK_LMENU:    case VK_RMENU:    ret = VK_MENU; break;
            }
        }
        break;
    case MAPVK_VK_TO_CHAR:
        kbd_tables_init_vk2char( kbd_tables, vk2char );
        if (code >= ARRAY_SIZE(vk2char)) ret = 0;
        else if (code >= 'A' && code <= 'Z') ret = code;
        else ret = vk2char[code];
        break;
    default:
        FIXME_(keyboard)( "unknown type %d\n", type );
        break;
    }

    if (kbd_tables != &kbdus_tables) user_driver->pReleaseKbdTables( kbd_tables );

    TRACE_(keyboard)( "returning 0x%04x\n", ret );
    return ret;
}

/***********************************************************************
 *      map_scan_to_kbd_vkey
 *
 * Map a scancode to a virtual key with KBD information.
 */
USHORT map_scan_to_kbd_vkey( USHORT scan, HKL layout )
{
    const KBDTABLES *kbd_tables;
    USHORT vsc2vk[0x300];
    UINT vkey;

    if ((vkey = user_driver->pMapVirtualKeyEx( scan, MAPVK_VSC_TO_VK_EX, layout )) != -1) return vkey;

    if (!(kbd_tables = user_driver->pKbdLayerDescriptor( layout ))) kbd_tables = &kbdus_tables;

    kbd_tables_init_vsc2vk( kbd_tables, vsc2vk );
    if (scan & 0xe000) scan -= 0xdf00;
    if (scan >= ARRAY_SIZE(vsc2vk)) vkey = 0;
    else vkey = vsc2vk[scan];

    if (kbd_tables != &kbdus_tables) user_driver->pReleaseKbdTables( kbd_tables );

    return vkey;
}

/****************************************************************************
 *	     NtUserGetKeyNameText    (win32u.@)
 */
INT WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size )
{
    INT code = ((lparam >> 16) & 0x1ff), vkey, len;
    HKL layout = NtUserGetKeyboardLayout( 0 );
    const KBDTABLES *kbd_tables;
    VSC_LPWSTR *key_name;

    TRACE_(keyboard)( "lparam %#x, buffer %p, size %d.\n", lparam, buffer, size );

    if (!buffer || !size) return 0;
    if ((len = user_driver->pGetKeyNameText( lparam, buffer, size )) >= 0) return len;

    if (!(kbd_tables = user_driver->pKbdLayerDescriptor( layout ))) kbd_tables = &kbdus_tables;

    if (lparam & 0x2000000)
    {
        USHORT vsc2vk[0x300];
        kbd_tables_init_vsc2vk( kbd_tables, vsc2vk );
        switch ((vkey = vsc2vk[code] & 0xff))
        {
        case VK_RSHIFT:
        case VK_RCONTROL:
        case VK_RMENU:
            for (code = 0; code < ARRAY_SIZE(vsc2vk); ++code)
                if ((vsc2vk[code] & 0xff) == (vkey - 1)) break;
            break;
        }
    }

    if (code < 0x100) key_name = kbd_tables->pKeyNames;
    else key_name = kbd_tables->pKeyNamesExt;
    while (key_name->vsc && key_name->vsc != (BYTE)code) key_name++;

    if (key_name->vsc == (BYTE)code && key_name->pwsz)
    {
        len = min( size - 1, wcslen( key_name->pwsz ) );
        memcpy( buffer, key_name->pwsz, len * sizeof(WCHAR) );
    }
    else if (size > 1)
    {
        HKL hkl = NtUserGetKeyboardLayout( 0 );
        vkey = NtUserMapVirtualKeyEx( code & 0xff, MAPVK_VSC_TO_VK, hkl );
        buffer[0] = NtUserMapVirtualKeyEx( vkey, MAPVK_VK_TO_CHAR, hkl );
        len = buffer[0] ? 1 : 0;
    }
    buffer[len] = 0;

    if (kbd_tables != &kbdus_tables) user_driver->pReleaseKbdTables( kbd_tables );

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_w(buffer) );
    return len;
}

/****************************************************************************
 *	     NtUserToUnicodeEx    (win32u.@)
 */
INT WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                              WCHAR *str, int size, UINT flags, HKL layout )
{
    const KBDTABLES *kbd_tables;
    INT len;

    TRACE_(keyboard)( "virt %#x, scan %#x, state %p, str %p, size %d, flags %#x, layout %p.\n",
                      virt, scan, state, str, size, flags, layout );

    if (!state || !size) return 0;
    if ((len = user_driver->pToUnicodeEx( virt, scan, state, str, size, flags, layout )) >= -1) return len;

    if (!(kbd_tables = user_driver->pKbdLayerDescriptor( layout ))) kbd_tables = &kbdus_tables;
    if (scan & 0x8000) str[0] = 0; /* key up */
    else str[0] = kbd_tables_vkey_to_wchar( kbd_tables, virt, state );
    if (size > 1) str[1] = 0;

    if (str[0] != WCH_NONE) len = 1;
    else str[0] = len = 0;

    if (kbd_tables != &kbdus_tables) user_driver->pReleaseKbdTables( kbd_tables );

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_wn(str, len) );
    return len;
}

/**********************************************************************
 *	     NtUserActivateKeyboardLayout    (win32u.@)
 */
HKL WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags )
{
    struct user_thread_info *info = get_user_thread_info();
    HKL old_layout;
    LCID locale;
    HWND focus;

    TRACE_(keyboard)( "layout %p, flags %x\n", layout, flags );

    if (flags) FIXME_(keyboard)( "flags %x not supported\n", flags );

    if (layout == (HKL)HKL_NEXT || layout == (HKL)HKL_PREV)
    {
        RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
        FIXME_(keyboard)( "HKL_NEXT and HKL_PREV not supported\n" );
        return 0;
    }

    if (LOWORD(layout) != MAKELANGID(LANG_INVARIANT, SUBLANG_DEFAULT) &&
        (NtQueryDefaultLocale( TRUE, &locale ) || LOWORD(layout) != locale))
    {
        RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
        FIXME_(keyboard)( "Changing user locale is not supported\n" );
        return 0;
    }

    if (!user_driver->pActivateKeyboardLayout( layout, flags ))
        return 0;

    old_layout = info->kbd_layout;
    if (old_layout != layout)
    {
        HWND ime_hwnd = get_default_ime_window( 0 );
        const NLS_LOCALE_DATA *data;
        CHARSETINFO cs = {0};

        if (ime_hwnd) send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_HKL_DEACTIVATE, HandleToUlong(old_layout) );

        if (HIWORD(layout) & 0x8000)
            FIXME( "Aliased keyboard layout not yet implemented\n" );
        else if (!(data = get_locale_data( HIWORD(layout) )))
            WARN( "Failed to find locale data for %04x\n", HIWORD(layout) );
        else
            translate_charset_info( ULongToPtr(data->idefaultansicodepage), &cs, TCI_SRCCODEPAGE );

        info->kbd_layout = layout;
        info->kbd_layout_id = 0;

        if (ime_hwnd) send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_HKL_ACTIVATE, HandleToUlong(layout) );

        if ((focus = get_focus()) && get_window_thread( focus, NULL ) == GetCurrentThreadId())
            send_message( focus, WM_INPUTLANGCHANGE, cs.ciCharset, (LPARAM)layout );
    }

    if (!old_layout) return get_locale_kbd_layout();
    return old_layout;
}



/***********************************************************************
 *	     NtUserGetKeyboardLayoutList    (win32u.@)
 *
 * Return number of values available if either input parm is
 *  0, per MS documentation.
 */
UINT WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts )
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key_info = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value_info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD count, tmp, i = 0;
    HKEY hkey, subkey;
    HKL layout;

    TRACE_(keyboard)( "size %d, layouts %p.\n", size, layouts );

    if ((count = user_driver->pGetKeyboardLayoutList( size, layouts )) != ~0) return count;

    layout = get_locale_kbd_layout();
    count = 0;

    count++;
    if (size && layouts)
    {
        layouts[count - 1] = layout;
        if (count == size) return count;
    }

    if ((hkey = reg_open_key( NULL, keyboard_layouts_keyW, sizeof(keyboard_layouts_keyW) )))
    {
        while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key_info,
                                sizeof(buffer) - sizeof(WCHAR), &tmp ))
        {
            if (!(subkey = reg_open_key( hkey, key_info->Name, key_info->NameLength ))) continue;
            key_info->Name[key_info->NameLength / sizeof(WCHAR)] = 0;
            tmp = wcstoul( key_info->Name, NULL, 16 );
            if (query_reg_ascii_value( subkey, "Layout Id", value_info, sizeof(buffer) ) &&
                value_info->Type == REG_SZ)
                tmp = 0xf000 | (wcstoul( (const WCHAR *)value_info->Data, NULL, 16 ) & 0xfff);
            NtClose( subkey );

            tmp = MAKELONG( LOWORD( layout ), LOWORD( tmp ) );
            if (layout == UlongToHandle( tmp )) continue;

            count++;
            if (size && layouts)
            {
                layouts[count - 1] = UlongToHandle( tmp );
                if (count == size) break;
            }
        }
        NtClose( hkey );
    }

    return count;
}

/****************************************************************************
 *	     NtUserGetKeyboardLayoutName    (win32u.@)
 */
BOOL WINAPI NtUserGetKeyboardLayoutName( WCHAR *name )
{
    struct user_thread_info *info = get_user_thread_info();
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    WCHAR klid[KL_NAMELENGTH];
    UINT id;
    ULONG len, i = 0;
    HKEY hkey, subkey;
    HKL layout;

    TRACE_(keyboard)( "name %p\n", name );

    if (!name)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    if (info->kbd_layout_id)
    {
        snprintf( buffer, sizeof(buffer), "%08X", info->kbd_layout_id );
        asciiz_to_unicode( name, buffer );
        return TRUE;
    }

    layout = NtUserGetKeyboardLayout( 0 );
    id = HandleToUlong( layout );
    if (HIWORD( id ) == LOWORD( id )) id = LOWORD( id );
    snprintf( buffer, sizeof(buffer), "%08X", id );
    asciiz_to_unicode( name, buffer );

    if ((hkey = reg_open_key( NULL, keyboard_layouts_keyW, sizeof(keyboard_layouts_keyW) )))
    {
        while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key,
                                sizeof(buffer) - sizeof(WCHAR), &len ))
        {
            if (!(subkey = reg_open_key( hkey, key->Name, key->NameLength ))) continue;
            memcpy( klid, key->Name, key->NameLength );
            klid[key->NameLength / sizeof(WCHAR)] = 0;
            if (query_reg_ascii_value( subkey, "Layout Id", value, sizeof(buffer) ) &&
                value->Type == REG_SZ)
                id = 0xf000 | (wcstoul( (const WCHAR *)value->Data, NULL, 16 ) & 0xfff);
            else
                id = wcstoul( klid, NULL, 16 );
            NtClose( subkey );

            if (HIWORD( layout ) == id)
            {
                lstrcpynW( name, klid, KL_NAMELENGTH );
                break;
            }
        }
        NtClose( hkey );
    }

    info->kbd_layout_id = wcstoul( name, NULL, 16 );

    TRACE_(keyboard)( "ret %s\n", debugstr_w( name ) );
    return TRUE;
}

/***********************************************************************
 *	     NtUserRegisterHotKey (win32u.@)
 */
BOOL WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk )
{
    BOOL ret;
    int replaced = 0;

    TRACE_(keyboard)( "(%p,%d,0x%08x,%X)\n", hwnd, id, modifiers, vk );

    if ((!hwnd || is_current_thread_window( hwnd )) &&
        !user_driver->pRegisterHotKey( hwnd, modifiers, vk ))
        return FALSE;

    SERVER_START_REQ( register_hotkey )
    {
        req->window = wine_server_user_handle( hwnd );
        req->id = id;
        req->flags = modifiers;
        req->vkey = vk;
        if ((ret = !wine_server_call_err( req )))
        {
            replaced = reply->replaced;
            modifiers = reply->flags;
            vk = reply->vkey;
        }
    }
    SERVER_END_REQ;

    if (ret && replaced)
        user_driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *	     NtUserUnregisterHotKey    (win32u.@)
 */
BOOL WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id )
{
    BOOL ret;
    UINT modifiers, vk;

    TRACE_(keyboard)("(%p,%d)\n",hwnd,id);

    SERVER_START_REQ( unregister_hotkey )
    {
        req->window = wine_server_user_handle( hwnd );
        req->id = id;
        if ((ret = !wine_server_call_err( req )))
        {
            modifiers = reply->flags;
            vk = reply->vkey;
        }
    }
    SERVER_END_REQ;

    if (ret)
        user_driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *           NtUserGetMouseMovePointsEx    (win32u.@)
 */
int WINAPI NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                       int count, DWORD resolution )
{
    struct cursor_pos *pos, positions[64];
    int copied;
    unsigned int i;


    TRACE( "%d, %p, %p, %d, %d\n", size, ptin, ptout, count, resolution );

    if ((size != sizeof(MOUSEMOVEPOINT)) || (count < 0) || (count > ARRAY_SIZE( positions )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (!ptin || (!ptout && count))
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return -1;
    }

    if (resolution != GMMP_USE_DISPLAY_POINTS)
    {
        FIXME( "only GMMP_USE_DISPLAY_POINTS is supported for now\n" );
        RtlSetLastWin32Error( ERROR_POINT_NOT_FOUND );
        return -1;
    }

    SERVER_START_REQ( get_cursor_history )
    {
        wine_server_set_reply( req, &positions, sizeof(positions) );
        if (wine_server_call_err( req )) return -1;
    }
    SERVER_END_REQ;

    for (i = 0; i < ARRAY_SIZE( positions ); i++)
    {
        pos = &positions[i];
        if (ptin->x == pos->x && ptin->y == pos->y && (!ptin->time || ptin->time == pos->time))
            break;
    }

    if (i == ARRAY_SIZE( positions ))
    {
        RtlSetLastWin32Error( ERROR_POINT_NOT_FOUND );
        return -1;
    }

    for (copied = 0; copied < count && i < ARRAY_SIZE( positions ); copied++, i++)
    {
        pos = &positions[i];
        ptout[copied].x = pos->x;
        ptout[copied].y = pos->y;
        ptout[copied].time = pos->time;
        ptout[copied].dwExtraInfo = pos->info;
    }

    return copied;
}

static WORD get_key_state(void)
{
    WORD ret = 0;

    if (get_system_metrics( SM_SWAPBUTTON ))
    {
        if (NtUserGetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (NtUserGetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (NtUserGetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (NtUserGetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    if (NtUserGetAsyncKeyState(VK_MBUTTON) & 0x80)  ret |= MK_MBUTTON;
    if (NtUserGetAsyncKeyState(VK_SHIFT) & 0x80)    ret |= MK_SHIFT;
    if (NtUserGetAsyncKeyState(VK_CONTROL) & 0x80)  ret |= MK_CONTROL;
    if (NtUserGetAsyncKeyState(VK_XBUTTON1) & 0x80) ret |= MK_XBUTTON1;
    if (NtUserGetAsyncKeyState(VK_XBUTTON2) & 0x80) ret |= MK_XBUTTON2;
    return ret;
}

static struct mouse_tracking_info *get_mouse_tracking_info(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    if (!thread_info->mouse_tracking_info)
        thread_info->mouse_tracking_info = calloc(1, sizeof(*thread_info->mouse_tracking_info));
    return thread_info->mouse_tracking_info;
}

static void check_mouse_leave( HWND hwnd, int hittest, struct mouse_tracking_info *tracking )
{
    if (tracking->info.hwndTrack != hwnd)
    {
        if (tracking->info.dwFlags & TME_NONCLIENT)
            NtUserPostMessage( tracking->info.hwndTrack, WM_NCMOUSELEAVE, 0, 0 );
        else
            NtUserPostMessage( tracking->info.hwndTrack, WM_MOUSELEAVE, 0, 0 );

        tracking->info.dwFlags &= ~TME_LEAVE;
    }
    else
    {
        if (hittest == HTCLIENT)
        {
            if (tracking->info.dwFlags & TME_NONCLIENT)
            {
                NtUserPostMessage( tracking->info.hwndTrack, WM_NCMOUSELEAVE, 0, 0 );
                tracking->info.dwFlags &= ~TME_LEAVE;
            }
        }
        else
        {
            if (!(tracking->info.dwFlags & TME_NONCLIENT))
            {
                NtUserPostMessage( tracking->info.hwndTrack, WM_MOUSELEAVE, 0, 0 );
                tracking->info.dwFlags &= ~TME_LEAVE;
            }
        }
    }
}

void update_mouse_tracking_info( HWND hwnd )
{
    struct mouse_tracking_info *tracking = get_mouse_tracking_info();
    int hover_width = 0, hover_height = 0, hittest;
    POINT pos;

    TRACE( "hwnd %p\n", hwnd );

    NtUserGetCursorPos( &pos );
    hwnd = window_from_point( hwnd, pos, &hittest );

    TRACE( "point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest );

    NtUserSystemParametersInfo( SPI_GETMOUSEHOVERWIDTH, 0, &hover_width, 0 );
    NtUserSystemParametersInfo( SPI_GETMOUSEHOVERHEIGHT, 0, &hover_height, 0 );

    TRACE( "tracked pos %s, current pos %s, hover width %d, hover height %d\n",
           wine_dbgstr_point(&tracking->pos), wine_dbgstr_point(&pos),
           hover_width, hover_height );

    if (tracking->info.dwFlags & TME_LEAVE)
        check_mouse_leave( hwnd, hittest, tracking );

    if (tracking->info.hwndTrack != hwnd)
        tracking->info.dwFlags &= ~TME_HOVER;

    if (tracking->info.dwFlags & TME_HOVER)
    {
        /* has the cursor moved outside the rectangle centered around pos? */
        if ((abs( pos.x - tracking->pos.x ) > (hover_width / 2)) ||
            (abs( pos.y - tracking->pos.y ) > (hover_height / 2)))
        {
            tracking->pos = pos;
        }
        else
        {
            if (hittest == HTCLIENT)
            {
                screen_to_client(hwnd, &pos);
                TRACE( "client cursor pos %s\n", wine_dbgstr_point(&pos) );

                NtUserPostMessage( tracking->info.hwndTrack, WM_MOUSEHOVER,
                                   get_key_state(), MAKELPARAM( pos.x, pos.y ) );
            }
            else
            {
                if (tracking->info.dwFlags & TME_NONCLIENT)
                    NtUserPostMessage( tracking->info.hwndTrack, WM_NCMOUSEHOVER,
                                       hittest, MAKELPARAM( pos.x, pos.y ) );
            }

            /* stop tracking mouse hover */
            tracking->info.dwFlags &= ~TME_HOVER;
        }
    }

    /* stop the timer if the tracking list is empty */
    if (!(tracking->info.dwFlags & (TME_HOVER | TME_LEAVE)))
    {
        NtUserKillSystemTimer( tracking->info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
        tracking->info.hwndTrack = 0;
        tracking->info.dwFlags = 0;
        tracking->info.dwHoverTime = 0;
    }
}

/***********************************************************************
 *           NtUserTrackMouseEvent    (win32u.@)
 */
BOOL WINAPI NtUserTrackMouseEvent( TRACKMOUSEEVENT *info )
{
    struct mouse_tracking_info *tracking = get_mouse_tracking_info();
    DWORD hover_time;
    int hittest;
    HWND hwnd;
    POINT pos;

    TRACE( "size %u, flags %#x, hwnd %p, time %u\n",
           info->cbSize, info->dwFlags, info->hwndTrack, info->dwHoverTime );

    if (info->cbSize != sizeof(TRACKMOUSEEVENT))
    {
        WARN( "wrong size %u\n", info->cbSize );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (info->dwFlags & TME_QUERY)
    {
        *info = tracking->info;
        info->cbSize = sizeof(TRACKMOUSEEVENT);
        return TRUE;
    }

    if (!is_window( info->hwndTrack ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    if (!is_current_thread_window( info->hwndTrack ))
        return send_notify_message( info->hwndTrack, WM_WINE_TRACKMOUSEEVENT, info->dwFlags, info->dwHoverTime, FALSE );

    hover_time = (info->dwFlags & TME_HOVER) ? info->dwHoverTime : HOVER_DEFAULT;

    if (hover_time == HOVER_DEFAULT || hover_time == 0)
        NtUserSystemParametersInfo( SPI_GETMOUSEHOVERTIME, 0, &hover_time, 0 );

    NtUserGetCursorPos( &pos );
    hwnd = window_from_point( info->hwndTrack, pos, &hittest );
    TRACE( "point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest );

    if (info->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT))
        FIXME( "ignoring flags %#x\n", info->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT) );

    if (info->dwFlags & TME_CANCEL)
    {
        if (tracking->info.hwndTrack == info->hwndTrack)
        {
            tracking->info.dwFlags &= ~(info->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if (!(tracking->info.dwFlags & (TME_HOVER | TME_LEAVE)))
            {
                NtUserKillSystemTimer( tracking->info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
                tracking->info.hwndTrack = 0;
                tracking->info.dwFlags = 0;
                tracking->info.dwHoverTime = 0;
            }
        }
    }
    else
    {
        /* If TME_LEAVE is set and the mouse is not in the tracking window, post WM_MOUSELEAVE
         * now and don't overwrite the current tracking information */
        if (info->dwFlags & TME_LEAVE && !hwnd)
        {
            if (info->dwFlags & TME_NONCLIENT)
                NtUserPostMessage( info->hwndTrack, WM_NCMOUSELEAVE, 0, 0 );
            else
                NtUserPostMessage( info->hwndTrack, WM_MOUSELEAVE, 0, 0 );
            return TRUE;
        }

        /* In our implementation, it's possible that another window will receive
         * WM_MOUSEMOVE and call TrackMouseEvent before TrackMouseEventProc is
         * called. In such a situation, post the WM_MOUSELEAVE now. */
        if ((tracking->info.dwFlags & TME_LEAVE) && tracking->info.hwndTrack != NULL)
            check_mouse_leave( hwnd, hittest, tracking );

        NtUserKillSystemTimer( tracking->info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
        tracking->info.hwndTrack = 0;
        tracking->info.dwFlags = 0;
        tracking->info.dwHoverTime = 0;

        if (info->hwndTrack == hwnd)
        {
            /* Adding new mouse event to the tracking list */
            tracking->info = *info;
            tracking->info.dwHoverTime = hover_time;

            /* Initialize HoverInfo variables even if not hover tracking */
            tracking->pos = pos;

            NtUserSetSystemTimer( tracking->info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE, hover_time );
        }
    }

    return TRUE;
}

/**********************************************************************
 *		set_capture_window
 */
BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret )
{
    HWND previous = 0;
    UINT flags = 0;
    BOOL ret;

    if (gui_flags & GUI_INMENUMODE) flags |= CAPTURE_MENU;
    if (gui_flags & GUI_INMOVESIZE) flags |= CAPTURE_MOVESIZE;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->flags  = flags;
        if ((ret = !wine_server_call_err( req )))
        {
            previous = wine_server_ptr_handle( reply->previous );
            hwnd = wine_server_ptr_handle( reply->full_handle );
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        user_driver->pSetCapture( hwnd, gui_flags );

        if (previous)
            NtUserNotifyWinEvent( EVENT_SYSTEM_CAPTUREEND, previous, OBJID_WINDOW, 0 );

        if (hwnd)
            NtUserNotifyWinEvent( EVENT_SYSTEM_CAPTURESTART, hwnd, OBJID_WINDOW, 0 );

        if (previous)
            send_message( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );

        if (prev_ret) *prev_ret = previous;
    }
    return ret;
}

/**********************************************************************
 *           NtUserSetCapture (win32u.@)
 */
HWND WINAPI NtUserSetCapture( HWND hwnd )
{
    HWND previous = 0;

    set_capture_window( hwnd, 0, &previous );
    return previous;
}

/**********************************************************************
 *           NtUserReleaseCapture (win32u.@)
 */
BOOL WINAPI NtUserReleaseCapture(void)
{
    HWND previous = NULL;

    return set_capture_window( 0, 0, &previous );
}

/*****************************************************************
 *		set_focus_window
 *
 * Change the focus window, sending the WM_SETFOCUS and WM_KILLFOCUS messages
 */
static HWND set_focus_window( HWND hwnd )
{
    HWND previous = 0, ime_hwnd;
    BOOL ret;

    SERVER_START_REQ( set_focus_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
            previous = wine_server_ptr_handle( reply->previous );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    if (previous == hwnd) return previous;

    if (previous)
    {
        send_message( previous, WM_KILLFOCUS, (WPARAM)hwnd, 0 );

        ime_hwnd = get_default_ime_window( previous );
        if (ime_hwnd)
            send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_DEACTIVATE,
                          HandleToUlong(previous) );

        if (hwnd != get_focus()) return previous;  /* changed by the message */
    }
    if (is_window(hwnd))
    {
        ime_hwnd = get_default_ime_window( hwnd );
        if (ime_hwnd)
            send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_ACTIVATE,
                          HandleToUlong(hwnd) );

        NtUserNotifyWinEvent( EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, 0 );

        send_message( hwnd, WM_SETFOCUS, (WPARAM)previous, 0 );
    }
    return previous;
}

/*******************************************************************
 *		set_active_window
 */
BOOL set_active_window( HWND hwnd, HWND *prev, BOOL mouse, BOOL focus, DWORD new_active_thread_id )
{
    HWND previous = get_active_window();
    BOOL ret;
    DWORD old_thread, new_thread;
    CBTACTIVATESTRUCT cbt;

    if (previous == hwnd)
    {
        if (prev) *prev = hwnd;
        goto done;
    }

    /* call CBT hook chain */
    cbt.fMouse     = mouse;
    cbt.hWndActive = previous;
    if (call_hooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hwnd, (LPARAM)&cbt, sizeof(cbt) )) return FALSE;

    if (is_window( previous ))
    {
        send_message( previous, WM_NCACTIVATE, FALSE, (LPARAM)hwnd );
        send_message( previous, WM_ACTIVATE,
                      MAKEWPARAM( WA_INACTIVE, is_iconic(previous) ), (LPARAM)hwnd );
    }

    SERVER_START_REQ( set_active_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
            previous = wine_server_ptr_handle( reply->previous );
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;
    if (prev) *prev = previous;
    if (previous == hwnd) goto done;

    if (hwnd)
    {
        NtUserNotifyWinEvent( EVENT_SYSTEM_FOREGROUND, hwnd, 0, 0 );

        /* send palette messages */
        if (send_message( hwnd, WM_QUERYNEWPALETTE, 0, 0 ))
            send_message_timeout( HWND_BROADCAST, WM_PALETTEISCHANGING, (WPARAM)hwnd, 0,
                                  SMTO_ABORTIFHUNG, 2000, FALSE );
        if (!is_window(hwnd)) return FALSE;
    }

    old_thread = previous ? get_window_thread( previous, NULL ) : 0;
    new_thread = hwnd ? get_window_thread( hwnd, NULL ) : 0;

    if (old_thread != new_thread)
    {
        HWND *list, *phwnd;

        if ((list = list_window_children( 0 )))
        {
            if (old_thread)
            {
                if (!new_active_thread_id) new_active_thread_id = new_thread;
                for (phwnd = list; *phwnd; phwnd++)
                {
                    if (get_window_thread( *phwnd, NULL ) == old_thread)
                        send_message( *phwnd, WM_ACTIVATEAPP, 0, new_active_thread_id );
                }
            }
            if (new_thread)
            {
                for (phwnd = list; *phwnd; phwnd++)
                {
                    if (get_window_thread( *phwnd, NULL ) == new_thread)
                        send_message( *phwnd, WM_ACTIVATEAPP, 1, old_thread );
                }
            }
            free( list );
        }
    }

    if (is_window(hwnd))
    {
        send_message( hwnd, WM_NCACTIVATE, hwnd == NtUserGetForegroundWindow(), (LPARAM)previous );
        send_message( hwnd, WM_ACTIVATE,
                      MAKEWPARAM( mouse ? WA_CLICKACTIVE : WA_ACTIVE, is_iconic(hwnd) ),
                      (LPARAM)previous );
        if (NtUserGetAncestor( hwnd, GA_PARENT ) == get_desktop_window())
            NtUserPostMessage( get_desktop_window(), WM_PARENTNOTIFY, WM_NCACTIVATE, (LPARAM)hwnd );
    }

    /* now change focus if necessary */
    if (focus)
    {
        GUITHREADINFO info;

        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
        /* Do not change focus if the window is no more active */
        if (hwnd == info.hwndActive)
        {
            if (!info.hwndFocus || !hwnd || NtUserGetAncestor( info.hwndFocus, GA_ROOT ) != hwnd)
                set_focus_window( hwnd );
        }
    }

done:
    if (hwnd)
    {
        if (hwnd == NtUserGetForegroundWindow()) user_driver->pActivateWindow( hwnd, previous );
        clip_fullscreen_window( hwnd, FALSE );
    }
    return TRUE;
}

/**********************************************************************
 *           NtUserSetActiveWindow    (win32u.@)
 */
HWND WINAPI NtUserSetActiveWindow( HWND hwnd )
{
    HWND prev;

    TRACE( "%p\n", hwnd );

    if (hwnd)
    {
        DWORD style;

        hwnd = get_full_window_handle( hwnd );
        if (!is_window( hwnd ))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }

        style = get_window_long( hwnd, GWL_STYLE );
        if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD)
            return get_active_window();  /* Windows doesn't seem to return an error here */
    }

    if (!set_active_window( hwnd, &prev, FALSE, TRUE, 0 )) return 0;
    return prev;
}

/*****************************************************************
 *           NtUserSetFocus  (win32u.@)
 */
HWND WINAPI NtUserSetFocus( HWND hwnd )
{
    HWND hwndTop = hwnd, active;
    HWND previous = get_focus();

    TRACE( "%p prev %p\n", hwnd, previous );

    if (hwnd)
    {
        /* Check if we can set the focus to this window */
        hwnd = get_full_window_handle( hwnd );
        if (!is_window( hwnd ))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (hwnd == previous) return previous;  /* nothing to do */
        for (;;)
        {
            HWND parent;
            DWORD style = get_window_long( hwndTop, GWL_STYLE );
            if (style & (WS_MINIMIZE | WS_DISABLED)) return 0;
            if (!(style & WS_CHILD)) break;
            parent = NtUserGetAncestor( hwndTop, GA_PARENT );
            if (!parent || parent == get_desktop_window())
            {
                if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return 0;
                break;
            }
            if (parent == get_hwnd_message_parent()) return 0;
            hwndTop = parent;
        }

        /* call hooks */
        if (call_hooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)previous, 0 )) return 0;

        /* activate hwndTop if needed. */
        if (!(active = get_active_window()) && !set_foreground_window( hwndTop, FALSE )) return 0;
        if (hwndTop != active)
        {
            if (!set_active_window( hwndTop, NULL, FALSE, FALSE, 0 )) return 0;
            if (!is_window( hwnd )) return 0;  /* Abort if window destroyed */

            /* Do not change focus if the window is no longer active */
            if (hwndTop != get_active_window()) return 0;
        }
    }
    else /* NULL hwnd passed in */
    {
        if (!previous) return 0;  /* nothing to do */
        if (call_hooks( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)previous, 0 )) return 0;
    }

    /* change focus and send messages */
    return set_focus_window( hwnd );
}

/*****************************************************************
 *           NtUserSetForegroundWindow  (win32u.@)
 */
BOOL WINAPI NtUserSetForegroundWindow( HWND hwnd )
{
    return set_foreground_window( hwnd, FALSE );
}

/*******************************************************************
 *		set_foreground_window
 */
BOOL set_foreground_window( HWND hwnd, BOOL mouse )
{
    BOOL ret, send_msg_old = FALSE, send_msg_new = FALSE;
    DWORD new_thread_id;
    HWND previous = 0;

    if (mouse) hwnd = get_full_window_handle( hwnd );
    new_thread_id = get_window_thread( hwnd, NULL );

    SERVER_START_REQ( set_foreground_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
        {
            previous = wine_server_ptr_handle( reply->previous );
            send_msg_old = reply->send_msg_old;
            send_msg_new = reply->send_msg_new;
        }
    }
    SERVER_END_REQ;

    if (ret && previous != hwnd)
    {
        if (send_msg_old)  /* old window belongs to other thread */
            NtUserMessageCall( previous, WM_WINE_SETACTIVEWINDOW, 0, new_thread_id,
                               0, NtUserSendNotifyMessage, FALSE );
        else if (send_msg_new)  /* old window belongs to us but new one to other thread */
            ret = set_active_window( 0, NULL, mouse, TRUE, new_thread_id );

        if (send_msg_new)  /* new window belongs to other thread */
            NtUserMessageCall( hwnd, WM_WINE_SETACTIVEWINDOW, (WPARAM)hwnd, 0,
                               0, NtUserSendNotifyMessage, FALSE );
        else  /* new window belongs to us */
            ret = set_active_window( hwnd, NULL, mouse, TRUE, 0 );
    }
    return ret;
}

static struct
{
    HBITMAP bitmap;
    unsigned int timeout;
} caret = {0, 500};

static void display_caret( HWND hwnd, const RECT *r )
{
    HDC dc, mem_dc;

    /* do not use DCX_CACHE here, since coördinates are in logical units */
    if (!(dc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE )))
        return;
    mem_dc = NtGdiCreateCompatibleDC(dc);
    if (mem_dc)
    {
        HBITMAP prev_bitmap;

        prev_bitmap = NtGdiSelectBitmap( mem_dc, caret.bitmap );
        NtGdiBitBlt( dc, r->left, r->top, r->right-r->left, r->bottom-r->top, mem_dc, 0, 0, SRCINVERT, 0, 0 );
        NtGdiSelectBitmap( mem_dc, prev_bitmap );
        NtGdiDeleteObjectApp( mem_dc );
    }
    NtUserReleaseDC( hwnd, dc );
}

static unsigned int get_caret_registry_timeout(void)
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[11 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)value_buffer;
    unsigned int ret = 500;
    HKEY key;

    if (!(key = reg_open_hkcu_key( "Control Panel\\Desktop" )))
        return ret;

    if (query_reg_ascii_value( key, "CursorBlinkRate", value, sizeof(value_buffer) ))
        ret = wcstoul( (WCHAR *)value->Data, NULL, 10 );
    NtClose( key );
    return ret;
}

/*****************************************************************
 *           NtUserCreateCaret  (win32u.@)
 */
BOOL WINAPI NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height )
{
    HBITMAP caret_bitmap = 0;
    int old_state = 0;
    int hidden = 0;
    HWND prev = 0;
    BOOL ret;
    RECT r;

    TRACE( "hwnd %p, bitmap %p, width %d, height %d\n", hwnd, bitmap, width, height );

    if (!hwnd) return FALSE;

    if (bitmap && bitmap != (HBITMAP)1)
    {
        BITMAP bitmap_data;

        if (!NtGdiExtGetObjectW( bitmap, sizeof(bitmap_data), &bitmap_data )) return FALSE;
        width = bitmap_data.bmWidth;
        height = bitmap_data.bmHeight;
        caret_bitmap = NtGdiCreateBitmap( bitmap_data.bmWidth, bitmap_data.bmHeight,
                                          bitmap_data.bmPlanes, bitmap_data.bmBitsPixel, NULL );
        if (caret_bitmap)
        {
            size_t size = bitmap_data.bmWidthBytes * bitmap_data.bmHeight;
            BYTE *bits = malloc( size );

            NtGdiGetBitmapBits( bitmap, size, bits );
            NtGdiSetBitmapBits( caret_bitmap, size, bits );
            free( bits );
        }
    }
    else
    {
        HDC dc;

        if (!width) width = get_system_metrics( SM_CXBORDER );
        if (!height) height = get_system_metrics( SM_CYBORDER );

        /* create the uniform bitmap on the fly */
        dc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
        if (dc)
        {
            HDC mem_dc = NtGdiCreateCompatibleDC( dc );
            if (mem_dc)
            {
                if ((caret_bitmap = NtGdiCreateCompatibleBitmap( mem_dc, width, height )))
                {
                    HBITMAP prev_bitmap = NtGdiSelectBitmap( mem_dc, caret_bitmap );
                    SetRect( &r, 0, 0, width, height );
                    fill_rect( mem_dc, &r, GetStockObject( bitmap ? GRAY_BRUSH : WHITE_BRUSH ));
                    NtGdiSelectBitmap( mem_dc, prev_bitmap );
                }
                NtGdiDeleteObjectApp( mem_dc );
            }
            NtUserReleaseDC( hwnd, dc );
        }
    }
    if (!caret_bitmap) return FALSE;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->width  = width;
        req->height = height;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r         = wine_server_get_rect( reply->old_rect );
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    if (prev && !hidden)  /* hide the previous one */
    {
        /* FIXME: won't work if prev belongs to a different process */
        NtUserKillSystemTimer( prev, SYSTEM_TIMER_CARET );
        if (old_state) display_caret( prev, &r );
    }

    if (caret.bitmap) NtGdiDeleteObjectApp( caret.bitmap );
    caret.bitmap = caret_bitmap;
    caret.timeout = get_caret_registry_timeout();
    return TRUE;
}

/*******************************************************************
 *              NtUserDestroyCaret  (win32u.@)
 */
BOOL WINAPI NtUserDestroyCaret(void)
{
    int old_state = 0;
    int hidden = 0;
    HWND prev = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = 0;
        req->width  = 0;
        req->height = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r         = wine_server_get_rect( reply->old_rect );
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && prev && !hidden)
    {
        /* FIXME: won't work if prev belongs to a different process */
        NtUserKillSystemTimer( prev, SYSTEM_TIMER_CARET );
        if (old_state) display_caret( prev, &r );
    }
    if (caret.bitmap) NtGdiDeleteObjectApp( caret.bitmap );
    caret.bitmap = 0;
    return ret;
}

/*****************************************************************
 *           NtUserGetCaretBlinkTime  (win32u.@)
 */
UINT WINAPI NtUserGetCaretBlinkTime(void)
{
    return caret.timeout;
}

/*******************************************************************
 *           NtUserSetCaretBlinkTime  (win32u.@)
 */
BOOL WINAPI NtUserSetCaretBlinkTime( unsigned int time )
{
    TRACE( "time %u\n", time );

    caret.timeout = time;
    /* FIXME: update the timer */
    return TRUE;
}

/*****************************************************************
 *           NtUserGetCaretPos  (win32u.@)
 */
BOOL WINAPI NtUserGetCaretPos( POINT *pt )
{
    BOOL ret;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = 0;  /* don't set anything */
        req->handle = 0;
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            pt->x = reply->old_rect.left;
            pt->y = reply->old_rect.top;
        }
    }
    SERVER_END_REQ;
    return ret;
}

BOOL set_ime_composition_rect( HWND hwnd, RECT rect )
{
    if (!NtUserIsWindow( hwnd )) return FALSE;
    NtUserMapWindowPoints( hwnd, 0, (POINT *)&rect, 2, 0 /* per-monitor DPI */ );
    rect = map_rect_virt_to_raw( rect, 0 /* per-monitor DPI */ );
    return user_driver->pSetIMECompositionRect( NtUserGetAncestor( hwnd, GA_ROOT ), rect );
}

/*****************************************************************
 *           NtUserSetCaretPos  (win32u.@)
 */
BOOL WINAPI NtUserSetCaretPos( INT x, INT y )
{
    int old_state = 0;
    int hidden = 0;
    HWND hwnd = 0;
    BOOL ret;
    RECT r;

    TRACE( "(%d, %d)\n", x, y );

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_POS|SET_CARET_STATE;
        req->handle = 0;
        req->x      = x;
        req->y      = y;
        req->hide   = 0;
        req->state  = CARET_STATE_ON_IF_MOVED;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r         = wine_server_get_rect( reply->old_rect );
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (ret && !hidden && (x != r.left || y != r.top))
    {
        if (old_state) display_caret( hwnd, &r );
        r.right += x - r.left;
        r.bottom += y - r.top;
        r.left = x;
        r.top = y;
        display_caret( hwnd, &r );
        set_ime_composition_rect( hwnd, r );
        NtUserSetSystemTimer( hwnd, SYSTEM_TIMER_CARET, caret.timeout );
    }
    return ret;
}

/*****************************************************************
 *           NtUserShowCaret  (win32u.@)
 */
BOOL WINAPI NtUserShowCaret( HWND hwnd )
{
    int hidden = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE | SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = -1;
        req->state  = CARET_STATE_ON;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r         = wine_server_get_rect( reply->old_rect );
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && hidden == 1)  /* hidden was 1 so it's now 0 */
    {
        display_caret( hwnd, &r );
        set_ime_composition_rect( hwnd, r );
        NtUserSetSystemTimer( hwnd, SYSTEM_TIMER_CARET, caret.timeout );
    }
    return ret;
}

/*****************************************************************
 *           NtUserHideCaret  (win32u.@)
 */
BOOL WINAPI NtUserHideCaret( HWND hwnd )
{
    int old_state = 0;
    int hidden = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE | SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 1;
        req->state  = CARET_STATE_OFF;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r         = wine_server_get_rect( reply->old_rect );
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden)
    {
        if (old_state) display_caret( hwnd, &r );
        NtUserKillSystemTimer( hwnd, SYSTEM_TIMER_CARET );
    }
    return ret;
}

void toggle_caret( HWND hwnd )
{
    BOOL ret;
    RECT r;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = CARET_STATE_TOGGLE;
        if ((ret = !wine_server_call( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r         = wine_server_get_rect( reply->old_rect );
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden) display_caret( hwnd, &r );
}


/**********************************************************************
 *       NtUserEnableMouseInPointer    (win32u.@)
 */
BOOL WINAPI NtUserEnableMouseInPointer( BOOL enable )
{
    FIXME( "enable %u stub!\n", enable );
    RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *       NtUserEnableMouseInPointerForThread    (win32u.@)
 */
BOOL WINAPI NtUserEnableMouseInPointerForThread( void )
{
    FIXME( "stub!\n" );
    RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *       NtUserIsMouseInPointerEnabled    (win32u.@)
 */
BOOL WINAPI NtUserIsMouseInPointerEnabled(void)
{
    FIXME( "stub!\n" );
    RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

static BOOL is_captured_by_system(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) && info.hwndCapture && (info.flags & (GUI_INMOVESIZE | GUI_INMENUMODE));
}

/***********************************************************************
 *      clip_fullscreen_window
 *
 * Turn on clipping if the active window is fullscreen.
 */
BOOL clip_fullscreen_window( HWND hwnd, BOOL reset )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    MONITORINFO monitor_info = {.cbSize = sizeof(MONITORINFO)};
    RECT rect, virtual_rect;
    DWORD style;
    UINT dpi, ctx;
    BOOL ret;

    if (hwnd == NtUserGetDesktopWindow()) return FALSE;
    if (hwnd != NtUserGetForegroundWindow()) return FALSE;

    style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP | WS_CHILD)) == WS_CHILD) return FALSE;
    /* maximized windows don't count as full screen */
    if ((style & WS_MAXIMIZE) && (style & WS_CAPTION) == WS_CAPTION) return FALSE;

    dpi = get_dpi_for_window( hwnd );
    if (!get_window_rect( hwnd, &rect, dpi )) return FALSE;
    if (!is_window_rect_full_screen( &rect, dpi )) return FALSE;
    if (is_captured_by_system()) return FALSE;
    if (NtGetTickCount() - thread_info->clipping_reset < 1000) return FALSE;
    if (!reset && clipping_cursor && thread_info->clipping_cursor) return FALSE;  /* already clipping */

    ctx = set_thread_dpi_awareness_context( NTUSER_DPI_PER_MONITOR_AWARE );
    monitor_info = monitor_info_from_window( hwnd, MONITOR_DEFAULTTONEAREST );
    virtual_rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
    rect = map_rect_virt_to_raw( monitor_info.rcMonitor, get_thread_dpi() );
    set_thread_dpi_awareness_context( ctx );

    if (!grab_fullscreen)
    {
        if (!EqualRect( &monitor_info.rcMonitor, &virtual_rect )) return FALSE;
        if (is_virtual_desktop()) return FALSE;
    }

    TRACE( "win %p clipping fullscreen\n", hwnd );

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_CLIP | SET_CURSOR_FSCLIP;
        req->clip  = wine_server_rectangle( rect );
        ret = !wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}

/**********************************************************************
 *       NtUserGetPointerInfoList    (win32u.@)
 */
BOOL WINAPI NtUserGetPointerInfoList( UINT32 id, POINTER_INPUT_TYPE type, UINT_PTR unk0, UINT_PTR unk1, SIZE_T size,
                                      UINT32 *entry_count, UINT32 *pointer_count, void *pointer_info )
{
    FIXME( "id %#x, type %#x, unk0 %#lx, unk1 %#lx, size %#lx, entry_count %p, pointer_count %p, pointer_info %p stub!\n",
           id, type, (long)unk0, (long)unk1, size, entry_count, pointer_count, pointer_info );
    RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

static BOOL get_clip_cursor( RECT *rect, UINT dpi, MONITOR_DPI_TYPE type )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const desktop_shm_t *desktop_shm;
    NTSTATUS status;

    if (!rect) return FALSE;

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
        *rect = wine_server_get_rect( desktop_shm->cursor.clip );

    if (!status && type == MDT_EFFECTIVE_DPI) *rect = map_rect_raw_to_virt( *rect, dpi );
    return !status;
}

BOOL process_wine_clipcursor( HWND hwnd, UINT flags, BOOL reset )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    RECT rect, virtual_rect = get_virtual_screen_rect( 0, MDT_RAW_DPI );
    BOOL was_clipping, empty = !!(flags & SET_CURSOR_NOCLIP);

    TRACE( "hwnd %p, flags %#x, reset %u\n", hwnd, flags, reset );

    if ((was_clipping = thread_info->clipping_cursor)) InterlockedDecrement( &clipping_cursor );
    thread_info->clipping_cursor = FALSE;

    if (reset)
    {
        thread_info->clipping_reset = NtGetTickCount();
        return user_driver->pClipCursor( NULL, TRUE );
    }

    if (!grab_pointer) return TRUE;

    /* we are clipping if the clip rectangle is smaller than the screen */
    get_clip_cursor( &rect, 0, MDT_RAW_DPI );
    intersect_rect( &rect, &rect, &virtual_rect );
    if (EqualRect( &rect, &virtual_rect )) empty = TRUE;
    if (empty && !(flags & SET_CURSOR_FSCLIP))
    {
        /* if currently clipping, check if we should switch to fullscreen clipping */
        if (was_clipping && clip_fullscreen_window( hwnd, TRUE )) return TRUE;
        return user_driver->pClipCursor( NULL, FALSE );
    }

    if (!user_driver->pClipCursor( &rect, FALSE )) return FALSE;
    InterlockedIncrement( &clipping_cursor );
    thread_info->clipping_cursor = TRUE;
    return TRUE;
}

/**********************************************************************
 *       NtUserGetClipCursor (win32u.@)
 */
BOOL WINAPI NtUserGetClipCursor( RECT *rect )
{
    return get_clip_cursor( rect, get_thread_dpi(), MDT_DEFAULT );
}

/***********************************************************************
 *       NtUserClipCursor (win32u.@)
 */
BOOL WINAPI NtUserClipCursor( const RECT *rect )
{
    RECT new_rect;
    BOOL ret;

    TRACE( "Clipping to %s\n", wine_dbgstr_rect(rect) );

    if (rect)
    {
        if (rect->left > rect->right || rect->top > rect->bottom) return FALSE;
        new_rect = map_rect_virt_to_raw( *rect, get_thread_dpi() );
        rect = &new_rect;
    }

    SERVER_START_REQ( set_cursor )
    {
        if (rect)
        {
            req->flags       = SET_CURSOR_CLIP;
            req->clip        = wine_server_rectangle( *rect );
        }
        else req->flags = SET_CURSOR_NOCLIP;

        ret = !wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}

/**********************************************************************
 *       NtUserRegisterTouchPadCapable    (win32u.@)
 */
BOOL WINAPI NtUserRegisterTouchPadCapable( BOOL capable )
{
    FIXME( "capable %u stub!\n", capable );
    RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *       NtUserScheduleDispatchNotification    (win32u.@)
 */
INT WINAPI NtUserScheduleDispatchNotification( HWND hwnd )
{
    FIXME("hwnd %p stub!\n", hwnd);

    if (is_window(hwnd))
        return 2;

    return 0;
}
