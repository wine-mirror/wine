/*
 * Internal HID structures
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

#ifndef __HID_PARSE_H
#define __HID_PARSE_H

#define HID_MAGIC 0x8491759

typedef enum {
    UnknownElement = 0,
    ButtonElement,
    ValueElement,
} WINE_ELEMENT_TYPE;

typedef struct
{
    WINE_ELEMENT_TYPE ElementType;
    UINT  valueStartBit;
    UINT  bitCount;
    union {
        HIDP_VALUE_CAPS value;
        HIDP_BUTTON_CAPS button;
    } caps;
} WINE_HID_ELEMENT;

typedef struct
{
    UCHAR reportID;
    DWORD dwSize;
    DWORD elementCount;
    WINE_HID_ELEMENT Elements[1];
} WINE_HID_REPORT;

typedef struct
{
    DWORD magic;
    DWORD dwSize;
    HIDP_CAPS caps;

    DWORD dwInputReportCount;
    DWORD dwOutputReportCount;
    DWORD dwFeatureReportCount;

    DWORD dwOutputReportOffset;
    DWORD dwFeatureReportOffset;

    WINE_HID_REPORT InputReports[1];
} WINE_HIDP_PREPARSED_DATA, *PWINE_HIDP_PREPARSED_DATA;

#define HID_NEXT_REPORT(d,r) ((r)?(WINE_HID_REPORT*)(((BYTE*)(r))+(r)->dwSize):(d)->InputReports)
#define HID_INPUT_REPORTS(d) ((d)->InputReports)
#define HID_OUTPUT_REPORTS(d) ((WINE_HID_REPORT*)(((BYTE*)(d)->InputReports)+(d)->dwOutputReportOffset))
#define HID_FEATURE_REPORTS(d) ((WINE_HID_REPORT*)(((BYTE*)(d)->InputReports)+(d)->dwFeatureReportOffset))

#endif /* __HID_PARSE_H */
