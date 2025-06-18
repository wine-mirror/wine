/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_HIDPDDI_H
#define __WINE_HIDPDDI_H

#include <hidusage.h>
#include <ddk/hidpi.h>

typedef struct _HIDP_COLLECTION_DESC
{
    USAGE UsagePage;
    USAGE Usage;
    UCHAR CollectionNumber;
    UCHAR Reserved[15];
    USHORT InputLength;
    USHORT OutputLength;
    USHORT FeatureLength;
    USHORT PreparsedDataLength;
    PHIDP_PREPARSED_DATA PreparsedData;
} HIDP_COLLECTION_DESC, *PHIDP_COLLECTION_DESC;

typedef struct _HIDP_REPORT_IDS
{
    UCHAR ReportID;
    UCHAR CollectionNumber;
    USHORT InputLength;
    USHORT OutputLength;
    USHORT FeatureLength;
} HIDP_REPORT_IDS, *PHIDP_REPORT_IDS;

typedef struct _HIDP_GETCOLDESC_DBG
{
    ULONG BreakOffset;
    ULONG ErrorCode;
    ULONG Args[6];
} HIDP_GETCOLDESC_DBG, *PHIDP_GETCOLDESC_DBG;

typedef struct _HIDP_DEVICE_DESC
{
    HIDP_COLLECTION_DESC *CollectionDesc;
    ULONG CollectionDescLength;

    HIDP_REPORT_IDS *ReportIDs;
    ULONG ReportIDsLength;

    HIDP_GETCOLDESC_DBG Dbg;
} HIDP_DEVICE_DESC, *PHIDP_DEVICE_DESC;

NTSTATUS WINAPI HidP_GetCollectionDescription(PHIDP_REPORT_DESCRIPTOR report_desc, ULONG report_desc_len,
                                              POOL_TYPE pool_type, HIDP_DEVICE_DESC *device_desc);
void WINAPI HidP_FreeCollectionDescription(HIDP_DEVICE_DESC *device_desc);

#endif  /* __WINE_HIDPDDI_H */
