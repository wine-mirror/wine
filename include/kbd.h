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

#endif /* __WINE_KBD_H */
