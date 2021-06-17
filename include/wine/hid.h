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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "hidusage.h"
#include "ddk/hidpi.h"

#define HID_MAGIC 0x8491759

typedef struct __WINE_ELEMENT
{
    UINT  valueStartBit;
    UINT  bitCount;
    HIDP_VALUE_CAPS caps;
} WINE_HID_ELEMENT;

/* make sure HIDP_BUTTON_CAPS is a subset of HIDP_VALUE_CAPS */
C_ASSERT( sizeof(HIDP_BUTTON_CAPS) == sizeof(HIDP_VALUE_CAPS) );

C_ASSERT( offsetof(HIDP_BUTTON_CAPS, UsagePage) == offsetof(HIDP_VALUE_CAPS, UsagePage) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, ReportID) == offsetof(HIDP_VALUE_CAPS, ReportID) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, IsAlias) == offsetof(HIDP_VALUE_CAPS, IsAlias) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, BitField) == offsetof(HIDP_VALUE_CAPS, BitField) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, LinkCollection) == offsetof(HIDP_VALUE_CAPS, LinkCollection) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, LinkUsage) == offsetof(HIDP_VALUE_CAPS, LinkUsage) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, LinkUsagePage) == offsetof(HIDP_VALUE_CAPS, LinkUsagePage) );

C_ASSERT( offsetof(HIDP_BUTTON_CAPS, IsRange) == offsetof(HIDP_VALUE_CAPS, IsRange) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, IsStringRange) == offsetof(HIDP_VALUE_CAPS, IsStringRange) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, IsDesignatorRange) == offsetof(HIDP_VALUE_CAPS, IsDesignatorRange) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, IsAbsolute) == offsetof(HIDP_VALUE_CAPS, IsAbsolute) );

C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.UsageMin) == offsetof(HIDP_VALUE_CAPS, Range.UsageMin) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.UsageMax) == offsetof(HIDP_VALUE_CAPS, Range.UsageMax) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.StringMin) == offsetof(HIDP_VALUE_CAPS, Range.StringMin) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.StringMax) == offsetof(HIDP_VALUE_CAPS, Range.StringMax) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.DesignatorMin) == offsetof(HIDP_VALUE_CAPS, Range.DesignatorMin) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.DesignatorMax) == offsetof(HIDP_VALUE_CAPS, Range.DesignatorMax) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.DataIndexMin) == offsetof(HIDP_VALUE_CAPS, Range.DataIndexMin) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, Range.DataIndexMax) == offsetof(HIDP_VALUE_CAPS, Range.DataIndexMax) );

C_ASSERT( offsetof(HIDP_BUTTON_CAPS, NotRange.Usage) == offsetof(HIDP_VALUE_CAPS, NotRange.Usage) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, NotRange.StringIndex) == offsetof(HIDP_VALUE_CAPS, NotRange.StringIndex) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, NotRange.DesignatorIndex) == offsetof(HIDP_VALUE_CAPS, NotRange.DesignatorIndex) );
C_ASSERT( offsetof(HIDP_BUTTON_CAPS, NotRange.DataIndex) == offsetof(HIDP_VALUE_CAPS, NotRange.DataIndex) );

struct hid_value_caps
{
    USAGE   usage_page;
    USAGE   usage_min;
    USAGE   usage_max;
    USHORT  string_min;
    USHORT  string_max;
    USHORT  designator_min;
    USHORT  designator_max;
    BOOLEAN is_range;
    BOOLEAN is_string_range;
    BOOLEAN is_designator_range;
    UCHAR   report_id;
    USHORT  link_collection;
    USAGE   link_usage_page;
    USAGE   link_usage;
    USHORT  bit_field;
    USHORT  bit_size;
    USHORT  report_count;
    LONG    logical_min;
    LONG    logical_max;
    LONG    physical_min;
    LONG    physical_max;
    ULONG   units;
    ULONG   units_exp;
};

#define HID_VALUE_CAPS_IS_ABSOLUTE(x) (((x)->bit_field & 0x04) == 0)
#define HID_VALUE_CAPS_HAS_NULL(x) (((x)->bit_field & 0x40) != 0)

typedef struct __WINE_HID_REPORT
{
    UCHAR reportID;
    DWORD bitSize;
    DWORD elementCount;
    DWORD elementIdx;
} WINE_HID_REPORT;

typedef struct __WINE_HID_LINK_COLLECTION_NODE {
    USAGE  LinkUsage;
    USAGE  LinkUsagePage;
    USHORT Parent;
    USHORT NumberOfChildren;
    USHORT NextSibling;
    USHORT FirstChild;
    BYTE   CollectionType;
    BYTE   IsAlias;
} WINE_HID_LINK_COLLECTION_NODE;

typedef struct __WINE_HIDP_PREPARSED_DATA
{
    DWORD magic;
    DWORD dwSize;
    HIDP_CAPS caps;

    DWORD elementOffset;
    DWORD nodesOffset;
    DWORD reportCount[3];
    BYTE reportIdx[3][256];

    WINE_HID_REPORT reports[1];
} WINE_HIDP_PREPARSED_DATA, *PWINE_HIDP_PREPARSED_DATA;

#define HID_INPUT_REPORTS(d) ((d)->reports)
#define HID_OUTPUT_REPORTS(d) ((d)->reports + (d)->reportCount[0])
#define HID_FEATURE_REPORTS(d) ((d)->reports + (d)->reportCount[0] + (d)->reportCount[1])
#define HID_ELEMS(d) ((WINE_HID_ELEMENT*)((BYTE*)(d) + (d)->elementOffset))
#define HID_NODES(d) ((WINE_HID_LINK_COLLECTION_NODE*)((BYTE*)(d) + (d)->nodesOffset))

#endif /* __WINE_PARSE_H */
