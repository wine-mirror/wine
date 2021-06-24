/*
 * Human Input Devices
 *
 * Copyright (C) 2015 Aric Stewart
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


#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "hidusage.h"
#include "ddk/hidpi.h"
#include "wine/hid.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hidp);

static NTSTATUS get_value_caps_range( WINE_HIDP_PREPARSED_DATA *preparsed, HIDP_REPORT_TYPE report_type, ULONG report_len,
                                      const struct hid_value_caps **caps, const struct hid_value_caps **caps_end )
{
    if (preparsed->magic != HID_MAGIC) return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch (report_type)
    {
    case HidP_Input:
        if (report_len && report_len != preparsed->caps.InputReportByteLength)
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        *caps = HID_INPUT_VALUE_CAPS( preparsed );
        break;
    case HidP_Output:
        if (report_len && report_len != preparsed->caps.OutputReportByteLength)
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        *caps = HID_OUTPUT_VALUE_CAPS( preparsed );
        break;
    case HidP_Feature:
        if (report_len && report_len != preparsed->caps.FeatureReportByteLength)
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        *caps = HID_FEATURE_VALUE_CAPS( preparsed );
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    *caps_end = *caps + preparsed->value_caps_count[report_type];
    return HIDP_STATUS_SUCCESS;
}

struct caps_filter
{
    BOOLEAN buttons;
    BOOLEAN values;
    USAGE   usage_page;
    USHORT  collection;
    USAGE   usage;
};

static BOOL match_value_caps( const struct hid_value_caps *caps, const struct caps_filter *filter )
{
    if (!caps->usage_min && !caps->usage_max) return FALSE;
    if (filter->buttons && !HID_VALUE_CAPS_IS_BUTTON( caps )) return FALSE;
    if (filter->values && HID_VALUE_CAPS_IS_BUTTON( caps )) return FALSE;
    if (filter->usage_page && filter->usage_page != caps->usage_page) return FALSE;
    if (filter->collection && filter->collection != caps->link_collection) return FALSE;
    if (!filter->usage) return TRUE;
    return caps->usage_min <= filter->usage && caps->usage_max >= filter->usage;
}

typedef NTSTATUS (*enum_value_caps_callback)( const struct hid_value_caps *caps, void *user );

static NTSTATUS enum_value_caps( WINE_HIDP_PREPARSED_DATA *preparsed, HIDP_REPORT_TYPE report_type,
                                 const struct caps_filter *filter, enum_value_caps_callback callback,
                                 void *user, USHORT *count )
{
    const struct hid_value_caps *caps, *caps_end;
    NTSTATUS status;
    LONG remaining = *count;

    for (status = get_value_caps_range( preparsed, report_type, 0, &caps, &caps_end );
         status == HIDP_STATUS_SUCCESS && caps != caps_end; caps++)
    {
        if (!match_value_caps( caps, filter )) continue;
        if (remaining-- > 0) status = callback( caps, user );
    }

    if (status != HIDP_STATUS_SUCCESS) return status;

    *count -= remaining;
    if (*count == 0) return HIDP_STATUS_USAGE_NOT_FOUND;
    if (remaining < 0) return HIDP_STATUS_BUFFER_TOO_SMALL;
    return HIDP_STATUS_SUCCESS;
}

static NTSTATUS get_report_data(BYTE *report, INT reportLength, INT startBit, INT valueSize, PULONG value)
{

    if ((startBit + valueSize) / 8  > reportLength)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;

    if (valueSize == 1)
    {
        ULONG byte_index = startBit / 8;
        ULONG bit_index = startBit - (byte_index * 8);
        INT mask = (1 << bit_index);
        *value = !!(report[byte_index] & mask);
    }
    else
    {
        ULONG remaining_bits = valueSize;
        ULONG byte_index = startBit / 8;
        ULONG bit_index = startBit % 8;
        ULONG data = 0;
        ULONG shift = 0;
        while (remaining_bits)
        {
            ULONG copy_bits = 8 - bit_index;
            if (remaining_bits < copy_bits)
                copy_bits = remaining_bits;

            data |= ((report[byte_index] >> bit_index) & ((1 << copy_bits) - 1)) << shift;

            shift += copy_bits;
            bit_index = 0;
            byte_index++;
            remaining_bits -= copy_bits;
        }
        *value = data;
    }
    return HIDP_STATUS_SUCCESS;
}

static NTSTATUS set_report_data(BYTE *report, INT reportLength, INT startBit, INT valueSize, ULONG value)
{
    if ((startBit + valueSize) / 8  > reportLength)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;

    if (valueSize == 1)
    {
        ULONG byte_index = startBit / 8;
        ULONG bit_index = startBit - (byte_index * 8);
        if (value)
            report[byte_index] |= (1 << bit_index);
        else
            report[byte_index] &= ~(1 << bit_index);
    }
    else
    {
        ULONG byte_index = (startBit + valueSize - 1) / 8;
        ULONG data = value;
        ULONG remainingBits = valueSize;
        while (remainingBits)
        {
            BYTE subvalue = data & 0xff;

            data >>= 8;

            if (remainingBits >= 8)
            {
                report[byte_index] = subvalue;
                byte_index --;
                remainingBits -= 8;
            }
            else if (remainingBits > 0)
            {
                BYTE mask = (0xff << (8-remainingBits)) & subvalue;
                report[byte_index] |= mask;
                remainingBits = 0;
            }
        }
    }
    return HIDP_STATUS_SUCCESS;
}

static NTSTATUS get_report_data_array(BYTE *report, UINT reportLength, UINT startBit, UINT elemSize,
                                      UINT numElements, PCHAR values, UINT valuesSize)
{
    BYTE byte, *end, *p = report + startBit / 8;
    ULONG size = elemSize * numElements;
    ULONG m, bit_index = startBit % 8;
    BYTE *data = (BYTE*)values;

    if ((startBit + size) / 8 > reportLength)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;

    if (valuesSize < (size + 7) / 8)
        return HIDP_STATUS_BUFFER_TOO_SMALL;

    end = report + (startBit + size + 7) / 8;

    data--;
    byte = *p++;
    while (p != end)
    {
        *(++data) = byte >> bit_index;
        byte = *p++;
        *data |= byte << (8 - bit_index);
    }

    /* Handle the end and mask out bits beyond */
    m = (startBit + size) % 8;
    m = m ? m : 8;

    if (m > bit_index)
        *(++data) = (byte >> bit_index) & ((1 << (m - bit_index)) - 1);
    else
        *data &= (1 << (m + 8 - bit_index)) - 1;

    if (++data < (BYTE*)values + valuesSize)
        memset(data, 0, (BYTE*)values + valuesSize - data);

    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetButtonCaps( HIDP_REPORT_TYPE report_type, HIDP_BUTTON_CAPS *caps, USHORT *caps_count,
                                    PHIDP_PREPARSED_DATA preparsed_data )
{
    return HidP_GetSpecificButtonCaps( report_type, 0, 0, 0, caps, caps_count, preparsed_data );
}

NTSTATUS WINAPI HidP_GetCaps( PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps )
{
    WINE_HIDP_PREPARSED_DATA *preparsed = (WINE_HIDP_PREPARSED_DATA *)preparsed_data;

    TRACE( "preparsed_data %p, caps %p.\n", preparsed_data, caps );

    if (preparsed->magic != HID_MAGIC) return HIDP_STATUS_INVALID_PREPARSED_DATA;

    *caps = preparsed->caps;
    caps->NumberInputButtonCaps = preparsed->new_caps.NumberInputButtonCaps;
    caps->NumberOutputButtonCaps = preparsed->new_caps.NumberOutputButtonCaps;
    caps->NumberFeatureButtonCaps = preparsed->new_caps.NumberFeatureButtonCaps;
    caps->NumberInputValueCaps = preparsed->new_caps.NumberInputValueCaps;
    caps->NumberOutputValueCaps = preparsed->new_caps.NumberOutputValueCaps;
    caps->NumberFeatureValueCaps = preparsed->new_caps.NumberFeatureValueCaps;

    return HIDP_STATUS_SUCCESS;
}

static NTSTATUS find_usage(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                           USAGE Usage, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report,
                           USHORT bit_size, WINE_HID_ELEMENT *element)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT v_count = 0, r_count = 0;
    int i;

    TRACE("(%i, %x, %i, %i, %p, %p)\n", ReportType, UsagePage, LinkCollection, Usage,
          PreparsedData, Report);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    switch(ReportType)
    {
        case HidP_Input:
            v_count = data->caps.NumberInputValueCaps;
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];
    report = &data->reports[data->reportIdx[ReportType][(BYTE)Report[0]]];

    if (!r_count || !v_count)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    if (report->reportID && report->reportID != Report[0])
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        HIDP_VALUE_CAPS *value = &elems[report->elementIdx + i].caps;

        if ((elems[report->elementIdx + i].caps.BitSize == 1) != (bit_size == 1) ||
            value->UsagePage != UsagePage)
            continue;

        if (value->IsRange && value->Range.UsageMin <= Usage && Usage <= value->Range.UsageMax)
        {
            *element = elems[report->elementIdx + i];
            element->valueStartBit += value->BitSize * (Usage - value->Range.UsageMin);
            element->bitCount = elems[report->elementIdx + i].caps.BitSize;
            return HIDP_STATUS_SUCCESS;
        }
        else if (value->NotRange.Usage == Usage)
        {
            *element = elems[report->elementIdx + i];
            element->bitCount = elems[report->elementIdx + i].caps.BitSize;
            return HIDP_STATUS_SUCCESS;
        }
    }

    return HIDP_STATUS_USAGE_NOT_FOUND;
}

static LONG sign_extend(ULONG value, const WINE_HID_ELEMENT *element)
{
    UINT bit_count = element->bitCount;

    if ((value & (1 << (bit_count - 1)))
            && element->caps.BitSize != 1
            && element->caps.LogicalMin < 0)
    {
        value -= (1 << bit_count);
    }
    return value;
}

static LONG logical_to_physical(LONG value, const WINE_HID_ELEMENT *element)
{
    if (element->caps.PhysicalMin || element->caps.PhysicalMax)
    {
        value = (((ULONGLONG)(value - element->caps.LogicalMin)
                * (element->caps.PhysicalMax - element->caps.PhysicalMin))
                / (element->caps.LogicalMax - element->caps.LogicalMin))
                + element->caps.PhysicalMin;
    }
    return value;
}

NTSTATUS WINAPI HidP_GetScaledUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage,
                                         USHORT LinkCollection, USAGE Usage, PLONG UsageValue,
                                         PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength)
{
    NTSTATUS rc;
    WINE_HID_ELEMENT element;
    TRACE("(%i, %x, %i, %i, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          PreparsedData, Report, ReportLength);

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, 0, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        ULONG rawValue;
        rc = get_report_data((BYTE*)Report, ReportLength,
                             element.valueStartBit, element.bitCount, &rawValue);
        if (rc != HIDP_STATUS_SUCCESS)
            return rc;
        *UsageValue = logical_to_physical(sign_extend(rawValue, &element), &element);
    }

    return rc;
}


NTSTATUS WINAPI HidP_GetUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                                   USAGE Usage, PULONG UsageValue, PHIDP_PREPARSED_DATA PreparsedData,
                                   PCHAR Report, ULONG ReportLength)
{
    WINE_HID_ELEMENT element;
    NTSTATUS rc;

    TRACE("(%i, %x, %i, %i, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          PreparsedData, Report, ReportLength);

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, 0, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        return get_report_data((BYTE*)Report, ReportLength,
                               element.valueStartBit, element.bitCount, UsageValue);
    }

    return rc;
}


NTSTATUS WINAPI HidP_GetUsageValueArray(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                                        USAGE Usage, PCHAR UsageValue, USHORT UsageValueByteLength,
                                        PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength)
{
    WINE_HID_ELEMENT element;
    NTSTATUS rc;

    TRACE("(%i, %x, %i, %i, %p, %u, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          UsageValueByteLength, PreparsedData, Report, ReportLength);

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, 0, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        if (element.caps.IsRange || element.caps.ReportCount <= 1 || !element.bitCount)
            return HIDP_STATUS_NOT_VALUE_ARRAY;

        return get_report_data_array((BYTE*)Report, ReportLength, element.valueStartBit, element.bitCount,
                                     element.caps.ReportCount, UsageValue, UsageValueByteLength);
    }

    return rc;
}


NTSTATUS WINAPI HidP_GetUsages(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                               PUSAGE UsageList, PULONG UsageLength, PHIDP_PREPARSED_DATA PreparsedData,
                               PCHAR Report, ULONG ReportLength)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    BOOL found = FALSE;
    USHORT b_count = 0, r_count = 0;
    int i,uCount;

    TRACE("(%i, %x, %i, %p, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, UsageList,
          UsageLength, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
    {
        *UsageLength = 0;
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch(ReportType)
    {
        case HidP_Input:
            b_count = data->caps.NumberInputButtonCaps;
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];
    report = &data->reports[data->reportIdx[ReportType][(BYTE)Report[0]]];

    if (!r_count || !b_count)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    if (report->reportID && report->reportID != Report[0])
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    uCount = 0;
    for (i = 0; i < report->elementCount && uCount < *UsageLength; i++)
    {
        if (elems[report->elementIdx + i].caps.BitSize == 1 &&
            elems[report->elementIdx + i].caps.UsagePage == UsagePage)
        {
            int k;
            WINE_HID_ELEMENT *element = &elems[report->elementIdx + i];
            for (k=0; k < element->bitCount; k++)
            {
                UINT v = 0;
                NTSTATUS rc = get_report_data((BYTE*)Report, ReportLength,
                                element->valueStartBit + k, 1, &v);
                if (rc != HIDP_STATUS_SUCCESS)
                    return rc;
                found = TRUE;
                if (v)
                {
                    if (uCount == *UsageLength)
                        return HIDP_STATUS_BUFFER_TOO_SMALL;
                    UsageList[uCount] = element->caps.Range.UsageMin + k;
                    uCount++;
                }
            }
        }
    }

    *UsageLength = uCount;

    if (!found)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetValueCaps( HIDP_REPORT_TYPE report_type, HIDP_VALUE_CAPS *caps, USHORT *caps_count,
                                   PHIDP_PREPARSED_DATA preparsed_data )
{
    return HidP_GetSpecificValueCaps( report_type, 0, 0, 0, caps, caps_count, preparsed_data );
}

NTSTATUS WINAPI HidP_InitializeReportForID( HIDP_REPORT_TYPE report_type, UCHAR report_id,
                                            PHIDP_PREPARSED_DATA preparsed_data, char *report_buf, ULONG report_len )
{
    WINE_HIDP_PREPARSED_DATA *preparsed = (WINE_HIDP_PREPARSED_DATA *)preparsed_data;
    const struct hid_value_caps *caps, *end;
    NTSTATUS status;

    TRACE( "report_type %d, report_id %x, preparsed_data %p, report_buf %p, report_len %u.\n", report_type,
           report_id, preparsed_data, report_buf, report_len );

    if (!report_len) return HIDP_STATUS_INVALID_REPORT_LENGTH;

    status = get_value_caps_range( preparsed, report_type, report_len, &caps, &end );
    if (status != HIDP_STATUS_SUCCESS) return status;

    while (caps != end && (caps->report_id != report_id || (!caps->usage_min && !caps->usage_max))) caps++;
    if (caps == end) return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    memset( report_buf, 0, report_len );
    report_buf[0] = report_id;
    return HIDP_STATUS_SUCCESS;
}

ULONG WINAPI HidP_MaxUsageListLength(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, PHIDP_PREPARSED_DATA PreparsedData)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    int r_count;
    int i;
    int count = 0;

    TRACE("(%i, %x, %p)\n", ReportType, UsagePage, PreparsedData);

    if (data->magic != HID_MAGIC)
        return 0;

    switch(ReportType)
    {
        case HidP_Input:
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];


    if (!r_count)
        return 0;

    for (i = 0; i < r_count; i++)
    {
        int j;
        for (j = 0; j < report[i].elementCount; j++)
        {
            if (elems[report[i].elementIdx + j].caps.BitSize == 1 &&
               (UsagePage == 0 || elems[report[i].elementIdx + j].caps.UsagePage == UsagePage))
            {
                if (elems[report[i].elementIdx + j].caps.IsRange)
                    count += (elems[report[i].elementIdx + j].caps.Range.UsageMax -
                             elems[report[i].elementIdx + j].caps.Range.UsageMin) + 1;
                else
                    count++;
            }
        }
    }
    return count;
}

NTSTATUS WINAPI HidP_SetUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                                   USAGE Usage, ULONG UsageValue, PHIDP_PREPARSED_DATA PreparsedData,
                                   CHAR *Report, ULONG ReportLength)
{
    WINE_HID_ELEMENT element;
    NTSTATUS rc;

    TRACE("(%i, %x, %i, %i, %i, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          PreparsedData, Report, ReportLength);

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, 0, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        return set_report_data((BYTE*)Report, ReportLength,
                               element.valueStartBit, element.bitCount, UsageValue);
    }

    return rc;
}

NTSTATUS WINAPI HidP_SetUsageValueArray( HIDP_REPORT_TYPE report_type, USAGE usage_page, USHORT collection,
                                         USAGE usage, char *value_buf, USHORT value_len,
                                         PHIDP_PREPARSED_DATA preparsed_data, char *report_buf, ULONG report_len )
{
    FIXME( "report_type %d, usage_page %x, collection %d, usage %x, value_buf %p, value_len %u, "
           "preparsed_data %p, report_buf %p, report_len %u stub!\n",
           report_type, usage_page, collection, usage, value_buf, value_len, preparsed_data, report_buf, report_len );

    return HIDP_STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI HidP_SetUsages(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                               PUSAGE UsageList, PULONG UsageLength, PHIDP_PREPARSED_DATA PreparsedData,
                               PCHAR Report, ULONG ReportLength)
{
    WINE_HID_ELEMENT element;
    NTSTATUS rc;
    ULONG i;

    TRACE("(%i, %x, %i, %p, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, UsageList,
            UsageLength, PreparsedData, Report, ReportLength);

    for (i = 0; i < *UsageLength; i++)
    {
        rc = find_usage(ReportType, UsagePage, LinkCollection,
                UsageList[i], PreparsedData, Report, 1, &element);
        if (rc == HIDP_STATUS_SUCCESS)
        {
            rc = set_report_data((BYTE*)Report, ReportLength,
                    element.valueStartBit, element.bitCount, -1);
        }

        if (rc != HIDP_STATUS_SUCCESS)
        {
            *UsageLength = i;
            return rc;
        }
    }

    return HIDP_STATUS_SUCCESS;
}


NTSTATUS WINAPI HidP_TranslateUsagesToI8042ScanCodes(USAGE *ChangedUsageList,
    ULONG UsageListLength, HIDP_KEYBOARD_DIRECTION KeyAction,
    HIDP_KEYBOARD_MODIFIER_STATE *ModifierState,
    PHIDP_INSERT_SCANCODES InsertCodesProcedure, VOID *InsertCodesContext)
{
    FIXME("stub: %p, %i, %i, %p, %p, %p\n", ChangedUsageList, UsageListLength,
        KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_button_caps( const struct hid_value_caps *caps, void *user )
{
    HIDP_BUTTON_CAPS **iter = user, *dst = *iter;
    dst->UsagePage = caps->usage_page;
    dst->ReportID = caps->report_id;
    dst->LinkCollection = caps->link_collection;
    dst->LinkUsagePage = caps->link_usage_page;
    dst->LinkUsage = caps->link_usage;
    dst->BitField = caps->bit_field;
    dst->IsAlias = FALSE;
    dst->IsAbsolute = HID_VALUE_CAPS_IS_ABSOLUTE( caps );
    if (!(dst->IsRange = caps->is_range))
    {
        dst->NotRange.Usage = caps->usage_min;
        dst->NotRange.DataIndex = caps->data_index_min;
    }
    else
    {
        dst->Range.UsageMin = caps->usage_min;
        dst->Range.UsageMax = caps->usage_max;
        dst->Range.DataIndexMin = caps->data_index_min;
        dst->Range.DataIndexMax = caps->data_index_max;
    }
    if (!(dst->IsStringRange = caps->is_string_range))
        dst->NotRange.StringIndex = caps->string_min;
    else
    {
        dst->Range.StringMin = caps->string_min;
        dst->Range.StringMax = caps->string_max;
    }
    if ((dst->IsDesignatorRange = caps->is_designator_range))
        dst->NotRange.DesignatorIndex = caps->designator_min;
    else
    {
        dst->Range.DesignatorMin = caps->designator_min;
        dst->Range.DesignatorMax = caps->designator_max;
    }
    *iter += 1;
    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetSpecificButtonCaps( HIDP_REPORT_TYPE report_type, USAGE usage_page, USHORT collection,
                                            USAGE usage, HIDP_BUTTON_CAPS *caps, USHORT *caps_count,
                                            PHIDP_PREPARSED_DATA preparsed_data )
{
    WINE_HIDP_PREPARSED_DATA *preparsed = (WINE_HIDP_PREPARSED_DATA *)preparsed_data;
    const struct caps_filter filter = {.buttons = TRUE, .usage_page = usage_page, .collection = collection, .usage = usage};

    TRACE( "report_type %d, usage_page %x, collection %d, usage %x, caps %p, caps_count %p, preparsed_data %p.\n",
           report_type, usage_page, collection, usage, caps, caps_count, preparsed_data );

    return enum_value_caps( preparsed, report_type, &filter, get_button_caps, &caps, caps_count );
}

static NTSTATUS get_value_caps( const struct hid_value_caps *caps, void *user )
{
    HIDP_VALUE_CAPS **iter = user, *dst = *iter;
    dst->UsagePage = caps->usage_page;
    dst->ReportID = caps->report_id;
    dst->LinkCollection = caps->link_collection;
    dst->LinkUsagePage = caps->link_usage_page;
    dst->LinkUsage = caps->link_usage;
    dst->BitField = caps->bit_field;
    dst->IsAlias = FALSE;
    dst->IsAbsolute = HID_VALUE_CAPS_IS_ABSOLUTE( caps );
    dst->HasNull = HID_VALUE_CAPS_HAS_NULL( caps );
    dst->BitSize = caps->bit_size;
    dst->ReportCount = caps->is_range ? 1 : caps->report_count;
    dst->UnitsExp = caps->units_exp;
    dst->Units = caps->units;
    dst->LogicalMin = caps->logical_min;
    dst->LogicalMax = caps->logical_max;
    dst->PhysicalMin = caps->physical_min;
    dst->PhysicalMax = caps->physical_max;
    if (!(dst->IsRange = caps->is_range))
    {
        dst->NotRange.Usage = caps->usage_min;
        dst->NotRange.DataIndex = caps->data_index_min;
    }
    else
    {
        dst->Range.UsageMin = caps->usage_min;
        dst->Range.UsageMax = caps->usage_max;
        dst->Range.DataIndexMin = caps->data_index_min;
        dst->Range.DataIndexMax = caps->data_index_max;
    }
    if (!(dst->IsStringRange = caps->is_string_range))
        dst->NotRange.StringIndex = caps->string_min;
    else
    {
        dst->Range.StringMin = caps->string_min;
        dst->Range.StringMax = caps->string_max;
    }
    if ((dst->IsDesignatorRange = caps->is_designator_range))
        dst->NotRange.DesignatorIndex = caps->designator_min;
    else
    {
        dst->Range.DesignatorMin = caps->designator_min;
        dst->Range.DesignatorMax = caps->designator_max;
    }
    *iter += 1;
    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetSpecificValueCaps( HIDP_REPORT_TYPE report_type, USAGE usage_page, USHORT collection,
                                           USAGE usage, HIDP_VALUE_CAPS *caps, USHORT *caps_count,
                                           PHIDP_PREPARSED_DATA preparsed_data )
{
    WINE_HIDP_PREPARSED_DATA *preparsed = (WINE_HIDP_PREPARSED_DATA *)preparsed_data;
    const struct caps_filter filter = {.values = TRUE, .usage_page = usage_page, .collection = collection, .usage = usage};

    TRACE( "report_type %d, usage_page %x, collection %d, usage %x, caps %p, caps_count %p, preparsed_data %p.\n",
           report_type, usage_page, collection, usage, caps, caps_count, preparsed_data );

    return enum_value_caps( preparsed, report_type, &filter, get_value_caps, &caps, caps_count );
}

NTSTATUS WINAPI HidP_GetUsagesEx(HIDP_REPORT_TYPE ReportType, USHORT LinkCollection, USAGE_AND_PAGE *ButtonList,
    ULONG *UsageLength, PHIDP_PREPARSED_DATA PreparsedData, CHAR *Report, ULONG ReportLength)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA*)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT b_count = 0, r_count = 0;
    int i,uCount = 0;
    NTSTATUS rc;

    TRACE("(%i, %i, %p, %p(%i), %p, %p, %i)\n", ReportType, LinkCollection, ButtonList,
        UsageLength, *UsageLength, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            b_count = data->caps.NumberInputButtonCaps;
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];
    report = &data->reports[data->reportIdx[ReportType][(BYTE)Report[0]]];

    if (!r_count || !b_count)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    if (report->reportID && report->reportID != Report[0])
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        if (elems[report->elementIdx + i].caps.BitSize == 1)
        {
            int k;
            WINE_HID_ELEMENT *element = &elems[report->elementIdx + i];
            for (k=0; k < element->bitCount; k++)
            {
                UINT v = 0;
                NTSTATUS rc = get_report_data((BYTE*)Report, ReportLength,
                                element->valueStartBit + k, 1, &v);
                if (rc != HIDP_STATUS_SUCCESS)
                    return rc;
                if (v)
                {
                    if (uCount < *UsageLength)
                    {
                        ButtonList[uCount].Usage = element->caps.Range.UsageMin + k;
                        ButtonList[uCount].UsagePage = element->caps.UsagePage;
                    }
                    uCount++;
                }
            }
        }
    }

    TRACE("Returning %i usages\n", uCount);

    if (*UsageLength < uCount)
        rc = HIDP_STATUS_BUFFER_TOO_SMALL;
    else
        rc = HIDP_STATUS_SUCCESS;

    *UsageLength = uCount;

    return rc;
}

ULONG WINAPI HidP_MaxDataListLength(HIDP_REPORT_TYPE ReportType, PHIDP_PREPARSED_DATA PreparsedData)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA *)PreparsedData;
    TRACE("(%i, %p)\n", ReportType, PreparsedData);
    if (data->magic != HID_MAGIC)
        return 0;

    switch(ReportType)
    {
        case HidP_Input:
            return data->caps.NumberInputDataIndices;
        case HidP_Output:
            return data->caps.NumberOutputDataIndices;
        case HidP_Feature:
            return data->caps.NumberFeatureDataIndices;
        default:
            return 0;
    }
}

NTSTATUS WINAPI HidP_GetData(HIDP_REPORT_TYPE ReportType, HIDP_DATA *DataList, ULONG *DataLength,
    PHIDP_PREPARSED_DATA PreparsedData,CHAR *Report, ULONG ReportLength)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA*)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT r_count = 0;
    int i,uCount = 0;
    NTSTATUS rc;

    TRACE("(%i, %p, %p(%i), %p, %p, %i)\n", ReportType, DataList, DataLength,
    DataLength?*DataLength:0, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
        return 0;

    if (ReportType != HidP_Input && ReportType != HidP_Output && ReportType != HidP_Feature)
        return HIDP_STATUS_INVALID_REPORT_TYPE;

    r_count = data->reportCount[ReportType];
    report = &data->reports[data->reportIdx[ReportType][(BYTE)Report[0]]];

    if (!r_count || (report->reportID && report->reportID != Report[0]))
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        WINE_HID_ELEMENT *element = &elems[report->elementIdx + i];
        if (element->caps.BitSize == 1)
        {
            int k;
            for (k=0; k < element->bitCount; k++)
            {
                UINT v = 0;
                NTSTATUS rc = get_report_data((BYTE*)Report, ReportLength,
                                element->valueStartBit + k, 1, &v);
                if (rc != HIDP_STATUS_SUCCESS)
                    return rc;
                if (v)
                {
                    if (uCount < *DataLength)
                    {
                        DataList[uCount].DataIndex = element->caps.Range.DataIndexMin + k;
                        DataList[uCount].On = v;
                    }
                    uCount++;
                }
            }
        }
        else
        {
            if (uCount < *DataLength)
            {
                UINT v;
                NTSTATUS rc = get_report_data((BYTE*)Report, ReportLength,
                                     element->valueStartBit, element->bitCount, &v);
                if (rc != HIDP_STATUS_SUCCESS)
                    return rc;
                DataList[uCount].DataIndex = element->caps.NotRange.DataIndex;
                DataList[uCount].RawValue = v;
            }
            uCount++;
        }
    }

    if (*DataLength < uCount)
        rc = HIDP_STATUS_BUFFER_TOO_SMALL;
    else
        rc = HIDP_STATUS_SUCCESS;

    *DataLength = uCount;

    return rc;
}

NTSTATUS WINAPI HidP_GetLinkCollectionNodes(HIDP_LINK_COLLECTION_NODE *LinkCollectionNode,
    ULONG *LinkCollectionNodeLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA*)PreparsedData;
    WINE_HID_LINK_COLLECTION_NODE *nodes = HID_NODES(data);
    ULONG i;

    TRACE("(%p, %p, %p)\n", LinkCollectionNode, LinkCollectionNodeLength, PreparsedData);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    if (*LinkCollectionNodeLength < data->caps.NumberLinkCollectionNodes)
        return HIDP_STATUS_BUFFER_TOO_SMALL;

    for (i = 0; i < data->caps.NumberLinkCollectionNodes; ++i)
    {
        LinkCollectionNode[i].LinkUsage = nodes[i].LinkUsage;
        LinkCollectionNode[i].LinkUsagePage = nodes[i].LinkUsagePage;
        LinkCollectionNode[i].Parent = nodes[i].Parent;
        LinkCollectionNode[i].NumberOfChildren = nodes[i].NumberOfChildren;
        LinkCollectionNode[i].NextSibling = nodes[i].NextSibling;
        LinkCollectionNode[i].FirstChild = nodes[i].FirstChild;
        LinkCollectionNode[i].CollectionType = nodes[i].CollectionType;
        LinkCollectionNode[i].IsAlias = nodes[i].IsAlias;
    }
    *LinkCollectionNodeLength = data->caps.NumberLinkCollectionNodes;

    return HIDP_STATUS_SUCCESS;
}
