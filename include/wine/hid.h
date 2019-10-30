/*
 * Wine internal HID structures
 *
 * Copyright 2015 Aric Stewart
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

#ifndef __WINE_PARSE_H
#define __WINE_PARSE_H

#define HID_MAGIC 0x8491759

typedef enum __WINE_ELEMENT_TYPE {
    UnknownElement = 0,
    ButtonElement,
    ValueElement,
} WINE_ELEMENT_TYPE;

typedef struct __WINE_ELEMENT
{
    WINE_ELEMENT_TYPE ElementType;
    UINT  valueStartBit;
    UINT  bitCount;
    union {
        HIDP_VALUE_CAPS value;
        HIDP_BUTTON_CAPS button;
    } caps;
} WINE_HID_ELEMENT;

typedef struct __WINE_HID_REPORT
{
    UCHAR reportID;
    DWORD bitSize;
    DWORD elementCount;
    DWORD elementIdx;
} WINE_HID_REPORT;

typedef struct __WINE_HIDP_PREPARSED_DATA
{
    DWORD magic;
    DWORD dwSize;
    HIDP_CAPS caps;

    DWORD elementOffset;
    DWORD reportCount[3];
    BYTE reportIdx[3][256];

    WINE_HID_REPORT reports[1];
} WINE_HIDP_PREPARSED_DATA, *PWINE_HIDP_PREPARSED_DATA;

#define HID_INPUT_REPORTS(d) ((d)->reports)
#define HID_OUTPUT_REPORTS(d) ((d)->reports + (d)->reportCount[0])
#define HID_FEATURE_REPORTS(d) ((d)->reports + (d)->reportCount[0] + (d)->reportCount[1])
#define HID_ELEMS(d) ((WINE_HID_ELEMENT*)((BYTE*)(d) + (d)->elementOffset))

#endif /* __WINE_PARSE_H */
