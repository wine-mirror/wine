/*
 * Common controller functions and structures
 *
 * Copyright 2018 Aric Stewart
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

/* Blocks of data for building HID device descriptions */

#include "psh_hid_macros.h"

static const BYTE REPORT_HEADER[] = {
    USAGE_PAGE(1, /* placeholder */ HID_USAGE_PAGE_GENERIC),
    USAGE(1, 0),
    COLLECTION(1, Application),
        USAGE(1, /* placeholder */ HID_USAGE_GENERIC_POINTER),
        COLLECTION(1, Physical),
};
#define IDX_HEADER_PAGE 1
#define IDX_HEADER_USAGE 3

static const BYTE REPORT_BUTTONS[] = {
    USAGE_PAGE(1, /* placeholder */ HID_USAGE_PAGE_BUTTON),
    USAGE_MINIMUM(1, /* placeholder */ 1),
    USAGE_MAXIMUM(1, /* placeholder */ 3),
    LOGICAL_MINIMUM(1, 0),
    LOGICAL_MAXIMUM(1, 1),
    PHYSICAL_MINIMUM(1, 0),
    PHYSICAL_MAXIMUM(1, 1),
    REPORT_COUNT(1, /* placeholder */ 3),
    REPORT_SIZE(1, 1),
    INPUT(1, Data|Var|Abs),
};
#define IDX_BUTTON_USAGE_PAGE 1
#define IDX_BUTTON_MIN_USAGE 3
#define IDX_BUTTON_MAX_USAGE 5
#define IDX_BUTTON_COUNT 15

static const BYTE REPORT_PADDING[] = {
    REPORT_COUNT(1, /* placeholder */ 3),
    REPORT_SIZE(1, 1),
    INPUT(1, Cnst|Var|Abs),
};
#define IDX_PADDING_BIT_COUNT 1

static const BYTE REPORT_AXIS_HEADER[] = {
    USAGE_PAGE(1, /* placeholder */ HID_USAGE_PAGE_GENERIC),
};
#define IDX_AXIS_PAGE 1

static const BYTE REPORT_AXIS_USAGE[] = {
    USAGE(1, /* placeholder */ HID_USAGE_GENERIC_X),
};
#define IDX_AXIS_USAGE 1

static const BYTE REPORT_REL_AXIS_TAIL[] = {
    LOGICAL_MINIMUM(1, 0x81),
    LOGICAL_MAXIMUM(1, 0x7f),
    REPORT_SIZE(1, 8),
    REPORT_COUNT(1, /* placeholder */ 2),
    INPUT(1, Data|Var|Rel),
};
#define IDX_REL_AXIS_COUNT 7

static const BYTE REPORT_HATSWITCH[] = {
    USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
    USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
    LOGICAL_MINIMUM(1, 1),
    LOGICAL_MAXIMUM(1, 8),
    PHYSICAL_MINIMUM(1, 0),
    PHYSICAL_MAXIMUM(1, 8),
    REPORT_SIZE(1, 4),
    REPORT_COUNT(1, /* placeholder */ 1),
    INPUT(1, Data|Var|Abs),
};
#define IDX_HATSWITCH_COUNT 15

static const BYTE REPORT_TAIL[] = {
        END_COLLECTION,
    END_COLLECTION,
};

#include "pop_hid_macros.h"

static inline BYTE *add_button_block(BYTE* report_ptr, BYTE usage_min, BYTE usage_max)
{
    memcpy(report_ptr, REPORT_BUTTONS, sizeof(REPORT_BUTTONS));
    report_ptr[IDX_BUTTON_MIN_USAGE] = usage_min;
    report_ptr[IDX_BUTTON_MAX_USAGE] = usage_max;
    report_ptr[IDX_BUTTON_COUNT] = (usage_max - usage_min) + 1;
    return report_ptr + sizeof(REPORT_BUTTONS);
}


static inline BYTE *add_padding_block(BYTE *report_ptr, BYTE bitcount)
{
    memcpy(report_ptr, REPORT_PADDING, sizeof(REPORT_PADDING));
    report_ptr[IDX_PADDING_BIT_COUNT] = bitcount;
    return report_ptr + sizeof(REPORT_PADDING);
}

static inline BYTE *add_hatswitch(BYTE *report_ptr, INT count)
{
    memcpy(report_ptr, REPORT_HATSWITCH, sizeof(REPORT_HATSWITCH));
    report_ptr[IDX_HATSWITCH_COUNT] = count;
    return report_ptr + sizeof(REPORT_HATSWITCH);
}
