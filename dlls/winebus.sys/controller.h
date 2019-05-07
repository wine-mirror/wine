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

static const BYTE REPORT_HEADER[] = {
    0x05, 0x01, /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x00, /* USAGE (??) */
    0xa1, 0x01, /* COLLECTION (Application) */
    0x09, 0x01, /*   USAGE () */
    0xa1, 0x00, /*   COLLECTION (Physical) */
};
#define IDX_HEADER_PAGE 1
#define IDX_HEADER_USAGE 3

static const BYTE REPORT_BUTTONS[] = {
    0x05, 0x09, /* USAGE_PAGE (Button) */
    0x19, 0x01, /* USAGE_MINIMUM (Button 1) */
    0x29, 0x03, /* USAGE_MAXIMUM (Button 3) */
    0x15, 0x00, /* LOGICAL_MINIMUM (0) */
    0x25, 0x01, /* LOGICAL_MAXIMUM (1) */
    0x35, 0x00, /* PHYSICAL_MINIMUM (0) */
    0x45, 0x01, /* PHYSICAL_MAXIMUM (1) */
    0x95, 0x03, /* REPORT_COUNT (3) */
    0x75, 0x01, /* REPORT_SIZE (1) */
    0x81, 0x02, /* INPUT (Data,Var,Abs) */
};
#define IDX_BUTTON_MIN_USAGE 3
#define IDX_BUTTON_MAX_USAGE 5
#define IDX_BUTTON_COUNT 15

static const BYTE REPORT_PADDING[] = {
    0x95, 0x03, /* REPORT_COUNT (3) */
    0x75, 0x01, /* REPORT_SIZE (1) */
    0x81, 0x03, /* INPUT (Cnst,Var,Abs) */
};
#define IDX_PADDING_BIT_COUNT 1

static const BYTE REPORT_AXIS_HEADER[] = {
    0x05, 0x01,  /* USAGE_PAGE (Generic Desktop) */
};
#define IDX_AXIS_PAGE 1

static const BYTE REPORT_AXIS_USAGE[] = {
    0x09, 0x30,  /* USAGE (X) */
};
#define IDX_AXIS_USAGE 1

static const BYTE REPORT_REL_AXIS_TAIL[] = {
    0x15, 0x81,    /* LOGICAL_MINIMUM (0) */
    0x25, 0x7f,    /* LOGICAL_MAXIMUM (0xffff) */
    0x75, 0x08,    /* REPORT_SIZE (16) */
    0x95, 0x02,    /* REPORT_COUNT (2) */
    0x81, 0x06,    /* INPUT (Data,Var,Rel) */
};
#define IDX_REL_AXIS_COUNT 7

static const BYTE REPORT_HATSWITCH[] = {
    0x05, 0x01,  /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x39,  /* USAGE (Hatswitch) */
    0x15, 0x00,  /* LOGICAL_MINIMUM (0) */
    0x25, 0x08,  /* LOGICAL_MAXIMUM (0x08) */
    0x35, 0x00,  /* PHYSICAL_MINIMUM (0) */
    0x45, 0x08,  /* PHYSICAL_MAXIMUM (8) */
    0x75, 0x08,  /* REPORT_SIZE (8) */
    0x95, 0x01,  /* REPORT_COUNT (1) */
    0x81, 0x02,  /* INPUT (Data,Var,Abs) */
};
#define IDX_HATSWITCH_COUNT 15

static const BYTE REPORT_TAIL[] = {
    0xc0, /*   END_COLLECTION */
    0xc0  /* END_COLLECTION */
};

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
