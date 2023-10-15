/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_KBD_H
#define __WINE_KBD_H

#include "windef.h"

#define KBDBASE          0
#define KBDSHIFT         1
#define KBDCTRL          2
#define KBDALT           4
#define KBDKANA          8
#define KBDROYA          0x10
#define KBDLOYA          0x20
#define KBDGRPSELTAP     0x80

#define WCH_NONE         0xf000
#define WCH_DEAD         0xf001
#define WCH_LGTR         0xf002

#define CAPLOK           0x01
#define SGCAPS           0x02
#define CAPLOKALTGR      0x04
#define KANALOK          0x08
#define GRPSELTAP        0x80

#define KLLF_ALTGR       0x0001
#define KLLF_SHIFTLOCK   0x0002
#define KLLF_LRM_RLM     0x0004

typedef struct
{
    BYTE Vk;
    BYTE ModBits;
} VK_TO_BIT, *PVK_TO_BIT;

typedef struct
{
    VK_TO_BIT *pVkToBit;
    WORD wMaxModBits;
    BYTE ModNumber[];
} MODIFIERS, *PMODIFIERS;

#define TYPEDEF_VK_TO_WCHARS(n) \
    typedef struct _VK_TO_WCHARS##n \
    { \
        BYTE VirtualKey; \
        BYTE Attributes; \
        WCHAR wch[n]; \
    } VK_TO_WCHARS##n, *PVK_TO_WCHARS##n

TYPEDEF_VK_TO_WCHARS(1);
TYPEDEF_VK_TO_WCHARS(2);
TYPEDEF_VK_TO_WCHARS(3);
TYPEDEF_VK_TO_WCHARS(4);
TYPEDEF_VK_TO_WCHARS(5);
TYPEDEF_VK_TO_WCHARS(6);
TYPEDEF_VK_TO_WCHARS(7);
TYPEDEF_VK_TO_WCHARS(8);
TYPEDEF_VK_TO_WCHARS(9);
TYPEDEF_VK_TO_WCHARS(10);

typedef struct _VK_TO_WCHAR_TABLE
{
    VK_TO_WCHARS1 *pVkToWchars;
    BYTE nModifications;
    BYTE cbSize;
} VK_TO_WCHAR_TABLE, *PVK_TO_WCHAR_TABLE;

typedef struct
{
    DWORD dwBoth;
    WCHAR wchComposed;
    USHORT uFlags;
} DEADKEY, *PDEADKEY;

typedef struct
{
    BYTE vsc;
    WCHAR *pwsz;
} VSC_LPWSTR, *PVSC_LPWSTR;

typedef struct _VSC_VK
{
    BYTE Vsc;
    USHORT Vk;
} VSC_VK, *PVSC_VK;

#define TYPEDEF_LIGATURE(n) \
    typedef struct _LIGATURE##n \
    { \
        BYTE VirtualKey; \
        WORD ModificationNumber; \
        WCHAR wch[n]; \
    } LIGATURE##n, *PLIGATURE##n;

TYPEDEF_LIGATURE(1)
TYPEDEF_LIGATURE(2)
TYPEDEF_LIGATURE(3)
TYPEDEF_LIGATURE(4)
TYPEDEF_LIGATURE(5)

typedef struct tagKbdLayer
{
    MODIFIERS *pCharModifiers;
    VK_TO_WCHAR_TABLE *pVkToWcharTable;
    DEADKEY *pDeadKey;
    VSC_LPWSTR *pKeyNames;
    VSC_LPWSTR *pKeyNamesExt;
    WCHAR **pKeyNamesDead;
    USHORT *pusVSCtoVK;
    BYTE bMaxVSCtoVK;
    VSC_VK *pVSCtoVK_E0;
    VSC_VK *pVSCtoVK_E1;
    DWORD fLocaleFlags;
    BYTE nLgMax;
    BYTE cbLgEntry;
    LIGATURE1 *pLigature;
    DWORD dwType;
    DWORD dwSubType;
} KBDTABLES, *PKBDTABLES;

#define KBD_VERSION              1
#define GET_KBD_VERSION(table)   (HIWORD((table)->fLocaleFlags))

#ifndef KBD_TYPE
#define KBD_TYPE 4
#endif

#define KBDEXT         (USHORT)0x0100
#define KBDMULTIVK     (USHORT)0x0200
#define KBDSPECIAL     (USHORT)0x0400
#define KBDNUMPAD      (USHORT)0x0800
#define KBDUNICODE     (USHORT)0x1000
#define KBDINJECTEDVK  (USHORT)0x2000
#define KBDMAPPEDVK    (USHORT)0x4000
#define KBDBREAK       (USHORT)0x8000

#define VK__none_      0xff
#define VK_ABNT_C1     0xc1
#define VK_ABNT_C2     0xc2

#if (KBD_TYPE <= 6)
#define _EQ(v4)                     (VK_##v4)
#if (KBD_TYPE == 1)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v1)
#elif (KBD_TYPE == 2)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v2)
#elif (KBD_TYPE == 3)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v3)
#elif (KBD_TYPE == 4)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v4)
#elif (KBD_TYPE == 5)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v5)
#elif (KBD_TYPE == 6)
#define _NE(v1, v2, v3, v4, v5, v6) (VK_##v6)
#endif
#define T00 _EQ(_none_)
#define T01 _EQ(ESCAPE)
#define T02 '1'
#define T03 '2'
#define T04 '3'
#define T05 '4'
#define T06 '5'
#define T07 '6'
#define T08 '7'
#define T09 '8'
#define T0A '9'
#define T0B '0'
#define T0C _EQ(OEM_MINUS)
#define T0D _NE(OEM_PLUS,OEM_4, OEM_PLUS, OEM_PLUS, OEM_PLUS, OEM_PLUS)
#define T0E _EQ(BACK)
#define T0F _EQ(TAB)
#define T10 'Q'
#define T11 'W'
#define T12 'E'
#define T13 'R'
#define T14 'T'
#define T15 'Y'
#define T16 'U'
#define T17 'I'
#define T18 'O'
#define T19 'P'
#define T1A _NE(OEM_4, OEM_6, OEM_4, OEM_4, OEM_4, OEM_4)
#define T1B _NE(OEM_6, OEM_1, OEM_6, OEM_6, OEM_6, OEM_6)
#define T1C _EQ(RETURN)
#define T1D _EQ(LCONTROL)
#define T1E 'A'
#define T1F 'S'
#define T20 'D'
#define T21 'F'
#define T22 'G'
#define T23 'H'
#define T24 'J'
#define T25 'K'
#define T26 'L'
#define T27 _NE(OEM_1, OEM_PLUS, OEM_1, OEM_1, OEM_1, OEM_1)
#define T28 _NE(OEM_7, OEM_3, OEM_7, OEM_7, OEM_3, OEM_3)
#define T29 _NE(OEM_3, OEM_7, OEM_3, OEM_3, OEM_7, OEM_7)
#define T2A _EQ(LSHIFT)
#define T2B _EQ(OEM_5)
#define T2C 'Z'
#define T2D 'X'
#define T2E 'C'
#define T2F 'V'
#define T30 'B'
#define T31 'N'
#define T32 'M'
#define T33 _EQ(OEM_COMMA)
#define T34 _EQ(OEM_PERIOD)
#define T35 _EQ(OEM_2)
#define T36 _EQ(RSHIFT)
#define T37 _EQ(MULTIPLY)
#define T38 _EQ(LMENU)
#define T39 ' '
#define T3A _EQ(CAPITAL)
#define T3B _EQ(F1)
#define T3C _EQ(F2)
#define T3D _EQ(F3)
#define T3E _EQ(F4)
#define T3F _EQ(F5)
#define T40 _EQ(F6)
#define T41 _EQ(F7)
#define T42 _EQ(F8)
#define T43 _EQ(F9)
#define T44 _EQ(F10)
#define T45 _EQ(NUMLOCK)
#define T46 _EQ(SCROLL)
#define T47 _EQ(HOME)
#define T48 _EQ(UP)
#define T49 _EQ(PRIOR)
#define T4A _EQ(SUBTRACT)
#define T4B _EQ(LEFT)
#define T4C _EQ(CLEAR)
#define T4D _EQ(RIGHT)
#define T4E _EQ(ADD)
#define T4F _EQ(END)
#define T50 _EQ(DOWN)
#define T51 _EQ(NEXT)
#define T52 _EQ(INSERT)
#define T53 _EQ(DELETE)
#define T54 _EQ(SNAPSHOT)
#define T55 _EQ(_none_)
#define T56 _NE(OEM_102, HELP, OEM_102, OEM_102, _none_, OEM_PA2)
#define T57 _NE(F11, RETURN, F11, F11, _none_, HELP)
#define T58 _NE(F12, LEFT, F12, F12, _none_, OEM_102)
#define T59 _EQ(CLEAR)
#define T5A _EQ(OEM_WSCTRL)
#define T5B _EQ(OEM_FINISH)
#define T5C _EQ(OEM_JUMP)
#define T5D _EQ(EREOF)
#define T5E _EQ(OEM_BACKTAB)
#define T5F _EQ(OEM_AUTO)
#define T60 _EQ(_none_)
#define T61 _EQ(_none_)
#define T62 _EQ(ZOOM)
#define T63 _EQ(HELP)
#define T64 _EQ(F13)
#define T65 _EQ(F14)
#define T66 _EQ(F15)
#define T67 _EQ(F16)
#define T68 _EQ(F17)
#define T69 _EQ(F18)
#define T6A _EQ(F19)
#define T6B _EQ(F20)
#define T6C _EQ(F21)
#define T6D _EQ(F22)
#define T6E _EQ(F23)
#define T6F _EQ(OEM_PA3)
#define T70 _EQ(_none_)
#define T71 _EQ(OEM_RESET)
#define T72 _EQ(_none_)
#define T73 _EQ(ABNT_C1)
#define T74 _EQ(_none_)
#define T75 _EQ(_none_)
#define T76 _EQ(F24)
#define T77 _EQ(_none_)
#define T78 _EQ(_none_)
#define T79 _EQ(_none_)
#define T7A _EQ(_none_)
#define T7B _EQ(OEM_PA1)
#define T7C _EQ(TAB)
#define T7D _EQ(_none_)
#define T7E _EQ(ABNT_C2)
#define T7F _EQ(OEM_PA2)
#define X10 _EQ(MEDIA_PREV_TRACK)
#define X19 _EQ(MEDIA_NEXT_TRACK)
#define X1C _EQ(RETURN)
#define X1D _EQ(RCONTROL)
#define X20 _EQ(VOLUME_MUTE)
#define X21 _EQ(LAUNCH_APP2)
#define X22 _EQ(MEDIA_PLAY_PAUSE)
#define X24 _EQ(MEDIA_STOP)
#define X2E _EQ(VOLUME_DOWN)
#define X30 _EQ(VOLUME_UP)
#define X32 _EQ(BROWSER_HOME)
#define X35 _EQ(DIVIDE)
#define X37 _EQ(SNAPSHOT)
#define X38 _EQ(RMENU)
#define X46 _EQ(CANCEL)
#define X47 _EQ(HOME)
#define X48 _EQ(UP)
#define X49 _EQ(PRIOR)
#define X4B _EQ(LEFT)
#define X4D _EQ(RIGHT)
#define X4F _EQ(END)
#define X50 _EQ(DOWN)
#define X51 _NE(NEXT, F1, NEXT, NEXT, _none_, OEM_PA2)
#define X52 _EQ(INSERT)
#define X53 _EQ(DELETE)
#define X5B _EQ(LWIN)
#define X5C _EQ(RWIN)
#define X5D _EQ(APPS)
#define X5E _EQ(POWER)
#define X5F _EQ(SLEEP)
#define X65 _EQ(BROWSER_SEARCH)
#define X66 _EQ(BROWSER_FAVORITES)
#define X67 _EQ(BROWSER_REFRESH)
#define X68 _EQ(BROWSER_STOP)
#define X69 _EQ(BROWSER_FORWARD)
#define X6A _EQ(BROWSER_BACK)
#define X6B _EQ(LAUNCH_APP1)
#define X6C _EQ(LAUNCH_MAIL)
#define X6D _EQ(LAUNCH_MEDIA_SELECT)
#define Y1D _EQ(PAUSE)
#else
#error "Unsupported KBD_TYPE"
#endif

#endif /* __WINE_KBD_H */
