/*
 * Common HID report descriptor helpers
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "controller.h"

BOOL hid_descriptor_append(struct hid_descriptor *desc, const BYTE *buffer, SIZE_T size)
{
    BYTE *tmp = desc->data;

    if (desc->size + size > desc->max_size)
    {
        desc->max_size = max(desc->max_size * 3 / 2, desc->size + size);
        if (!desc->data) desc->data = HeapAlloc(GetProcessHeap(), 0, desc->max_size);
        else desc->data = HeapReAlloc(GetProcessHeap(), 0, tmp, desc->max_size);
    }

    if (!desc->data)
    {
        HeapFree(GetProcessHeap(), 0, tmp);
        return FALSE;
    }

    memcpy(desc->data + desc->size, buffer, size);
    desc->size += size;
    return TRUE;
}

#include "psh_hid_macros.h"

BOOL hid_descriptor_begin(struct hid_descriptor *desc, USAGE usage_page, USAGE usage)
{
    const BYTE template[] =
    {
        USAGE_PAGE(2, usage_page),
        USAGE(2, usage),
        COLLECTION(1, Application),
            USAGE(1, 0),
    };

    memset(desc, 0, sizeof(*desc));
    return hid_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_descriptor_end(struct hid_descriptor *desc)
{
    static const BYTE template[] =
    {
        END_COLLECTION,
    };

    return hid_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_descriptor_add_buttons(struct hid_descriptor *desc, USAGE usage_page,
                                USAGE usage_min, USAGE usage_max)
{
    const BYTE template[] =
    {
        USAGE_PAGE(2, usage_page),
        USAGE_MINIMUM(2, usage_min),
        USAGE_MAXIMUM(2, usage_max),
        LOGICAL_MINIMUM(1, 0),
        LOGICAL_MAXIMUM(1, 1),
        PHYSICAL_MINIMUM(1, 0),
        PHYSICAL_MAXIMUM(1, 1),
        REPORT_COUNT(2, usage_max - usage_min + 1),
        REPORT_SIZE(1, 1),
        INPUT(1, Data|Var|Abs),
    };

    return hid_descriptor_append(desc, template, sizeof(template));
}

#include "pop_hid_macros.h"
