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

#define NONAMELESSUNION
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


NTSTATUS WINAPI HidP_GetButtonCaps(HIDP_REPORT_TYPE ReportType, PHIDP_BUTTON_CAPS ButtonCaps,
                                   PUSHORT ButtonCapsLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT b_count = 0, r_count = 0;
    int i,j,u;

    TRACE("(%i, %p, %p, %p)\n",ReportType, ButtonCaps, ButtonCapsLength, PreparsedData);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            b_count = data->caps.NumberInputButtonCaps;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];

    if (!r_count || !b_count)
    {
        *ButtonCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    b_count = min(b_count, *ButtonCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < b_count; j++)
    {
        for (i = 0; i < report[j].elementCount && u < b_count; i++)
        {
            if (elems[report[j].elementIdx + i].ElementType == ButtonElement)
                ButtonCaps[u++] = elems[report[j].elementIdx + i].caps.button;
        }
    }

    *ButtonCapsLength = b_count;
    return HIDP_STATUS_SUCCESS;
}


NTSTATUS WINAPI HidP_GetCaps(PHIDP_PREPARSED_DATA PreparsedData,
    PHIDP_CAPS Capabilities)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;

    TRACE("(%p, %p)\n",PreparsedData, Capabilities);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    *Capabilities = data->caps;

    return HIDP_STATUS_SUCCESS;
}

static NTSTATUS find_usage(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                           USAGE Usage, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report,
                           WINE_ELEMENT_TYPE ElementType, WINE_HID_ELEMENT *element)
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
        HIDP_VALUE_CAPS *value = &elems[report->elementIdx + i].caps.value;

        if (elems[report->elementIdx + i].ElementType != ElementType ||
            value->UsagePage != UsagePage)
            continue;

        if (value->IsRange && value->u.Range.UsageMin <= Usage && Usage <= value->u.Range.UsageMax)
        {
            *element = elems[report->elementIdx + i];
            element->valueStartBit += value->BitSize * (Usage - value->u.Range.UsageMin);
            element->bitCount = elems[report->elementIdx + i].ElementType == ValueElement ? value->BitSize: 1;
            return HIDP_STATUS_SUCCESS;
        }
        else if (value->u.NotRange.Usage == Usage)
        {
            *element = elems[report->elementIdx + i];
            element->bitCount = elems[report->elementIdx + i].ElementType == ValueElement ? value->BitSize : 1;
            return HIDP_STATUS_SUCCESS;
        }
    }

    return HIDP_STATUS_USAGE_NOT_FOUND;
}

static LONG sign_extend(ULONG value, const WINE_HID_ELEMENT *element)
{
    UINT bit_count = element->bitCount;

    if ((value & (1 << (bit_count - 1)))
            && element->ElementType == ValueElement
            && element->caps.value.LogicalMin < 0)
    {
        value -= (1 << bit_count);
    }
    return value;
}

static LONG logical_to_physical(LONG value, const WINE_HID_ELEMENT *element)
{
    if (element->caps.value.PhysicalMin || element->caps.value.PhysicalMax)
    {
        value = (((ULONGLONG)(value - element->caps.value.LogicalMin)
                * (element->caps.value.PhysicalMax - element->caps.value.PhysicalMin))
                / (element->caps.value.LogicalMax - element->caps.value.LogicalMin))
                + element->caps.value.PhysicalMin;
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

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, ValueElement, &element);

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

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, ValueElement, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        return get_report_data((BYTE*)Report, ReportLength,
                               element.valueStartBit, element.bitCount, UsageValue);
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
        if (elems[report->elementIdx + i].ElementType == ButtonElement &&
            elems[report->elementIdx + i].caps.button.UsagePage == UsagePage)
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
                    UsageList[uCount] = element->caps.button.u.Range.UsageMin + k;
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


NTSTATUS WINAPI HidP_GetValueCaps(HIDP_REPORT_TYPE ReportType, PHIDP_VALUE_CAPS ValueCaps,
                                  PUSHORT ValueCapsLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT v_count = 0, r_count = 0;
    int i,j,u;

    TRACE("(%i, %p, %p, %p)\n", ReportType, ValueCaps, ValueCapsLength, PreparsedData);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            v_count = data->caps.NumberInputValueCaps;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];

    if (!r_count || !v_count)
    {
        *ValueCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    v_count = min(v_count, *ValueCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < v_count; j++)
    {
        for (i = 0; i < report[j].elementCount && u < v_count; i++)
        {
            if (elems[report[j].elementIdx + i].ElementType == ValueElement)
                ValueCaps[u++] = elems[report[j].elementIdx + i].caps.value;
        }
    }

    *ValueCapsLength = v_count;
    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_InitializeReportForID(HIDP_REPORT_TYPE ReportType, UCHAR ReportID,
                                           PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report,
                                           ULONG ReportLength)
{
    int size;
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_REPORT *report = NULL;
    int r_count;

    TRACE("(%i, %i, %p, %p, %i)\n",ReportType, ReportID, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            size = data->caps.InputReportByteLength;
            break;
        case HidP_Output:
            size = data->caps.OutputReportByteLength;
            break;
        case HidP_Feature:
            size = data->caps.FeatureReportByteLength;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];
    report = &data->reports[data->reportIdx[ReportType][(BYTE)Report[0]]];

    if (!r_count || !size)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    if (size != ReportLength)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;

    if (report->reportID && report->reportID != Report[0])
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    ZeroMemory(Report, size);
    Report[0] = ReportID;
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
            if (elems[report[i].elementIdx + j].ElementType == ButtonElement &&
               (UsagePage == 0 || elems[report[i].elementIdx + j].caps.button.UsagePage == UsagePage))
            {
                if (elems[report[i].elementIdx + j].caps.button.IsRange)
                    count += (elems[report[i].elementIdx + j].caps.button.u.Range.UsageMax -
                             elems[report[i].elementIdx + j].caps.button.u.Range.UsageMin) + 1;
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

    rc = find_usage(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, ValueElement, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        return set_report_data((BYTE*)Report, ReportLength,
                               element.valueStartBit, element.bitCount, UsageValue);
    }

    return rc;
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
                UsageList[i], PreparsedData, Report, ButtonElement, &element);
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

NTSTATUS WINAPI HidP_GetSpecificButtonCaps(HIDP_REPORT_TYPE ReportType,
    USAGE UsagePage, USHORT LinkCollection, USAGE Usage,
    HIDP_BUTTON_CAPS *ButtonCaps, USHORT *ButtonCapsLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA*)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT b_count = 0, r_count = 0;
    int i,j,u;

    TRACE("(%i, 0x%x, %i, 0x%x, %p %p %p)\n", ReportType, UsagePage, LinkCollection,
        Usage, ButtonCaps, ButtonCapsLength, PreparsedData);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            b_count = data->caps.NumberInputButtonCaps;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];

    if (!r_count || !b_count)
    {
        *ButtonCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    b_count = min(b_count, *ButtonCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < b_count; j++)
    {
        for (i = 0; i < report[j].elementCount && u < b_count; i++)
        {
            if (elems[report[j].elementIdx + i].ElementType == ButtonElement &&
                (UsagePage == 0 || UsagePage == elems[report[j].elementIdx + i].caps.button.UsagePage) &&
                (LinkCollection == 0 || LinkCollection == elems[report[j].elementIdx + i].caps.button.LinkCollection) &&
                (Usage == 0 || (
                  (!elems[report[j].elementIdx + i].caps.button.IsRange &&
                    Usage == elems[report[j].elementIdx + i].caps.button.u.NotRange.Usage)) ||
                  (elems[report[j].elementIdx + i].caps.button.IsRange &&
                    Usage >= elems[report[j].elementIdx + i].caps.button.u.Range.UsageMin &&
                    Usage <= elems[report[j].elementIdx + i].caps.button.u.Range.UsageMax)))
            {
                ButtonCaps[u++] = elems[report[j].elementIdx + i].caps.button;
            }
        }
    }
    TRACE("Matched %i usages\n", u);

    *ButtonCapsLength = u;

    return HIDP_STATUS_SUCCESS;
}


NTSTATUS WINAPI HidP_GetSpecificValueCaps(HIDP_REPORT_TYPE ReportType,
    USAGE UsagePage, USHORT LinkCollection, USAGE Usage,
    HIDP_VALUE_CAPS *ValueCaps, USHORT *ValueCapsLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    WINE_HIDP_PREPARSED_DATA *data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
    WINE_HID_ELEMENT *elems = HID_ELEMS(data);
    WINE_HID_REPORT *report = NULL;
    USHORT v_count = 0, r_count = 0;
    int i,j,u;

    TRACE("(%i, 0x%x, %i, 0x%x, %p %p %p)\n", ReportType, UsagePage, LinkCollection,
        Usage, ValueCaps, ValueCapsLength, PreparsedData);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            v_count = data->caps.NumberInputValueCaps;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }
    r_count = data->reportCount[ReportType];

    if (!r_count || !v_count)
    {
        *ValueCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    v_count = min(v_count, *ValueCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < v_count; j++)
    {
        for (i = 0; i < report[j].elementCount && u < v_count; i++)
        {
            if (elems[report[j].elementIdx + i].ElementType == ValueElement &&
                (UsagePage == 0 || UsagePage == elems[report[j].elementIdx + i].caps.value.UsagePage) &&
                (LinkCollection == 0 || LinkCollection == elems[report[j].elementIdx + i].caps.value.LinkCollection) &&
                (Usage == 0 || Usage == elems[report[j].elementIdx + i].caps.value.u.NotRange.Usage))
            {
                ValueCaps[u++] = elems[report[j].elementIdx + i].caps.value;
            }
        }
    }
    TRACE("Matched %i usages\n", u);

    *ValueCapsLength = u;

    return HIDP_STATUS_SUCCESS;
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
        if (elems[report->elementIdx + i].ElementType == ButtonElement)
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
                        ButtonList[uCount].Usage = element->caps.button.u.Range.UsageMin + k;
                        ButtonList[uCount].UsagePage = element->caps.button.UsagePage;
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
        if (element->ElementType == ButtonElement)
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
                        DataList[uCount].DataIndex = element->caps.button.u.Range.DataIndexMin + k;
                        DataList[uCount].u.On = v;
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
                DataList[uCount].DataIndex = element->caps.value.u.NotRange.DataIndex;
                DataList[uCount].u.RawValue = v;
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
