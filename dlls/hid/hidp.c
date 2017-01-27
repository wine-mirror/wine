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

#include "config.h"

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
#include "parse.h"
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
        *value = (report[byte_index] & mask);
    }
    else
    {
        ULONG byte_index = startBit / 8;
        ULONG data = 0;
        ULONG remainingBits = valueSize;
        ULONG shift = 0;
        ULONG begin_offset = startBit % 8;
        while (remainingBits)
        {
            if (remainingBits >= 8)
            {
                BYTE mask = 0xff << begin_offset;
                data |= (report[byte_index] & mask) << shift;
                byte_index ++;
                remainingBits -= (8-begin_offset);
                shift += (8-begin_offset);
                begin_offset = 0;
            }
            else if (remainingBits > 0)
            {
                BYTE mask = (0xff >> (8-remainingBits)) << begin_offset;
                data |= (report[byte_index] & mask) << shift;
                remainingBits = 0;
            }
        }
        *value = data;
    }
    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetButtonCaps(HIDP_REPORT_TYPE ReportType, PHIDP_BUTTON_CAPS ButtonCaps,
                                   PUSHORT ButtonCapsLength, PHIDP_PREPARSED_DATA PreparsedData)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !b_count || !report)
    {
        *ButtonCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    b_count = min(b_count, *ButtonCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < b_count; j++)
    {
        for (i = 0; i < report->elementCount && u < b_count; i++)
        {
            if (report->Elements[i].ElementType == ButtonElement)
                ButtonCaps[u++] = report->Elements[i].caps.button;
        }
        report = HID_NEXT_REPORT(data, report);
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

static NTSTATUS find_value(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                           USAGE Usage, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report,
                           WINE_HID_ELEMENT **element)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !v_count || !report)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    for (i = 0; i < r_count; i++)
    {
        if (!report->reportID || report->reportID == Report[0])
            break;
        report = HID_NEXT_REPORT(data, report);
    }

    if (i == r_count)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        if (report->Elements[i].ElementType == ValueElement &&
            report->Elements[i].caps.value.UsagePage == UsagePage &&
            report->Elements[i].caps.value.u.NotRange.Usage == Usage)
        {
            *element = &report->Elements[i];
            return HIDP_STATUS_SUCCESS;
        }
    }

    return HIDP_STATUS_USAGE_NOT_FOUND;
}

NTSTATUS WINAPI HidP_GetScaledUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage,
                                         USHORT LinkCollection, USAGE Usage, PLONG UsageValue,
                                         PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength)
{
    NTSTATUS rc;
    WINE_HID_ELEMENT *element;
    TRACE("(%i, %x, %i, %i, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          PreparsedData, Report, ReportLength);

    rc = find_value(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        ULONG rawValue;
        rc = get_report_data((BYTE*)Report, ReportLength,
                             element->valueStartBit, element->bitCount, &rawValue);
        if (rc != HIDP_STATUS_SUCCESS)
            return rc;
        if (element->caps.value.BitSize == 16)
            rawValue = (short)rawValue;
        *UsageValue = rawValue;
    }

    return rc;
}


NTSTATUS WINAPI HidP_GetUsageValue(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                                   USAGE Usage, PULONG UsageValue, PHIDP_PREPARSED_DATA PreparsedData,
                                   PCHAR Report, ULONG ReportLength)
{
    WINE_HID_ELEMENT *element;
    NTSTATUS rc;

    TRACE("(%i, %x, %i, %i, %p, %p, %p, %i)\n", ReportType, UsagePage, LinkCollection, Usage, UsageValue,
          PreparsedData, Report, ReportLength);

    rc = find_value(ReportType, UsagePage, LinkCollection, Usage, PreparsedData, Report, &element);

    if (rc == HIDP_STATUS_SUCCESS)
    {
        return get_report_data((BYTE*)Report, ReportLength,
                               element->valueStartBit, element->bitCount, UsageValue);
    }

    return rc;
}


NTSTATUS WINAPI HidP_GetUsages(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection,
                               PUSAGE UsageList, PULONG UsageLength, PHIDP_PREPARSED_DATA PreparsedData,
                               PCHAR Report, ULONG ReportLength)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !b_count || !report)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    for (i = 0; i < r_count; i++)
    {
        if (!report->reportID || report->reportID == Report[0])
            break;
        report = HID_NEXT_REPORT(data, report);
    }

    if (i == r_count)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    uCount = 0;
    for (i = 0; i < report->elementCount && uCount < *UsageLength; i++)
    {
        if (report->Elements[i].ElementType == ButtonElement &&
            report->Elements[i].caps.button.UsagePage == UsagePage)
        {
            int k;
            WINE_HID_ELEMENT *element = &report->Elements[i];
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !v_count || !report)
    {
        *ValueCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    v_count = min(v_count, *ValueCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < v_count; j++)
    {
        for (i = 0; i < report->elementCount && u < v_count; i++)
        {
            if (report->Elements[i].ElementType == ValueElement)
                ValueCaps[u++] = report->Elements[i].caps.value;
        }
        report = HID_NEXT_REPORT(data, report);
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
    BOOL found=FALSE;
    int r_count;
    int i;

    TRACE("(%i, %i, %p, %p, %i)\n",ReportType, ReportID, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
        return HIDP_STATUS_INVALID_PREPARSED_DATA;

    switch(ReportType)
    {
        case HidP_Input:
            size = data->caps.InputReportByteLength;
            report = HID_INPUT_REPORTS(data);
            r_count = data->dwInputReportCount;
            break;
        case HidP_Output:
            size = data->caps.OutputReportByteLength;
            report = HID_OUTPUT_REPORTS(data);
            r_count = data->dwOutputReportCount;
            break;
        case HidP_Feature:
            size = data->caps.FeatureReportByteLength;
            report = HID_FEATURE_REPORTS(data);
            r_count = data->dwFeatureReportCount;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !size || !report)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    if (size != ReportLength)
        return HIDP_STATUS_INVALID_REPORT_LENGTH;

    ZeroMemory(Report, size);

    for (i = 0; i < r_count; i++)
    {
        if (report->reportID == ReportID)
        {
            found = TRUE;
            if (report->reportID)
                Report[0] = ReportID;
            /* TODO: Handle null and default values */
        }
        report = HID_NEXT_REPORT(data, report);
    }

    if (!found)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    return HIDP_STATUS_SUCCESS;
}

ULONG WINAPI HidP_MaxUsageListLength(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, PHIDP_PREPARSED_DATA PreparsedData)
{
    PWINE_HIDP_PREPARSED_DATA data = (PWINE_HIDP_PREPARSED_DATA)PreparsedData;
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
            r_count = data->dwInputReportCount;
            break;
        case HidP_Output:
            report = HID_OUTPUT_REPORTS(data);
            r_count = data->dwOutputReportCount;
            break;
        case HidP_Feature:
            report = HID_FEATURE_REPORTS(data);
            r_count = data->dwFeatureReportCount;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !report)
        return 0;

    for (i = 0; i < r_count; i++)
    {
        int j;
        for (j = 0; j < report->elementCount; j++)
        {
            if (report->Elements[j].ElementType == ButtonElement &&
               (UsagePage == 0 || report->Elements[j].caps.button.UsagePage == UsagePage))
            {
                if (report->Elements[j].caps.button.IsRange)
                    count += (report->Elements[j].caps.button.u.Range.UsageMax -
                             report->Elements[j].caps.button.u.Range.UsageMin) + 1;
                else
                    count++;
            }
        }
        report = HID_NEXT_REPORT(data, report);
    }
    return count;
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !b_count || !report)
    {
        *ButtonCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    b_count = min(b_count, *ButtonCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < b_count; j++)
    {
        for (i = 0; i < report->elementCount && u < b_count; i++)
        {
            if (report->Elements[i].ElementType == ButtonElement &&
                (UsagePage == 0 || UsagePage == report->Elements[i].caps.button.UsagePage) &&
                (LinkCollection == 0 || LinkCollection == report->Elements[i].caps.button.LinkCollection) &&
                (Usage == 0 || (
                  (!report->Elements[i].caps.button.IsRange &&
                    Usage == report->Elements[i].caps.button.u.NotRange.Usage)) ||
                  (report->Elements[i].caps.button.IsRange &&
                    Usage >=report->Elements[i].caps.button.u.Range.UsageMin &&
                    Usage <= report->Elements[i].caps.button.u.Range.UsageMax)))
            {
                ButtonCaps[u++] = report->Elements[i].caps.button;
            }
        }
        report = HID_NEXT_REPORT(data, report);
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            v_count = data->caps.NumberOutputValueCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            v_count = data->caps.NumberFeatureValueCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !v_count || !report)
    {
        *ValueCapsLength = 0;
        return HIDP_STATUS_SUCCESS;
    }

    v_count = min(v_count, *ValueCapsLength);

    u = 0;
    for (j = 0; j < r_count && u < v_count; j++)
    {
        for (i = 0; i < report->elementCount && u < v_count; i++)
        {
            if (report->Elements[i].ElementType == ValueElement &&
                (UsagePage == 0 || UsagePage == report->Elements[i].caps.value.UsagePage) &&
                (LinkCollection == 0 || LinkCollection == report->Elements[i].caps.value.LinkCollection) &&
                (Usage == 0 || Usage == report->Elements[i].caps.value.u.NotRange.Usage))
            {
                ValueCaps[u++] = report->Elements[i].caps.value;
            }
        }
        report = HID_NEXT_REPORT(data, report);
    }
    TRACE("Matched %i usages\n", u);

    *ValueCapsLength = u;

    return HIDP_STATUS_SUCCESS;
}

NTSTATUS WINAPI HidP_GetUsagesEx(HIDP_REPORT_TYPE ReportType, USHORT LinkCollection, USAGE_AND_PAGE *ButtonList,
    ULONG *UsageLength, PHIDP_PREPARSED_DATA PreparsedData, CHAR *Report, ULONG ReportLength)
{
    WINE_HIDP_PREPARSED_DATA *data = (WINE_HIDP_PREPARSED_DATA*)PreparsedData;
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
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            b_count = data->caps.NumberOutputButtonCaps;
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            b_count = data->caps.NumberFeatureButtonCaps;
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (!r_count || !b_count || !report)
        return HIDP_STATUS_USAGE_NOT_FOUND;

    for (i = 0; i < r_count; i++)
    {
        if (!report->reportID || report->reportID == Report[0])
            break;
        report = HID_NEXT_REPORT(data, report);
    }

    if (i == r_count)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        if (report->Elements[i].ElementType == ButtonElement)
        {
            int k;
            WINE_HID_ELEMENT *element = &report->Elements[i];
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
    WINE_HID_REPORT *report = NULL;
    USHORT r_count = 0;
    int i,uCount = 0;
    NTSTATUS rc;

    TRACE("(%i, %p, %p(%i), %p, %p, %i)\n", ReportType, DataList, DataLength,
    DataLength?*DataLength:0, PreparsedData, Report, ReportLength);

    if (data->magic != HID_MAGIC)
        return 0;

    switch(ReportType)
    {
        case HidP_Input:
            r_count = data->dwInputReportCount;
            report = HID_INPUT_REPORTS(data);
            break;
        case HidP_Output:
            r_count = data->dwOutputReportCount;
            report = HID_OUTPUT_REPORTS(data);
            break;
        case HidP_Feature:
            r_count = data->dwFeatureReportCount;
            report = HID_FEATURE_REPORTS(data);
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    for (i = 0; i < r_count; i++)
    {
        if (!report->reportID || report->reportID == Report[0])
            break;
        report = HID_NEXT_REPORT(data, report);
    }

    if (i == r_count)
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;

    for (i = 0; i < report->elementCount; i++)
    {
        WINE_HID_ELEMENT *element = &report->Elements[i];
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
                if (element->caps.value.BitSize == 16)
                    v = (short)v;
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
