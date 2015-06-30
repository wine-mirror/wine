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
