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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winioctl.h"
#include "hidusage.h"
#include "ddk/wdm.h"
#include "ddk/hidsdi.h"

#include "wine/debug.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

static BOOL hid_report_descriptor_append(struct hid_report_descriptor *desc, const BYTE *buffer, SIZE_T size)
{
    BYTE *tmp = desc->data;

    if (desc->size + size > desc->max_size)
    {
        desc->max_size = max(desc->max_size * 3 / 2, desc->size + size);
        desc->data = realloc(tmp, desc->max_size);
    }

    if (!desc->data)
    {
        free(tmp);
        return FALSE;
    }

    memcpy(desc->data + desc->size, buffer, size);
    desc->size += size;
    return TRUE;
}

#include "psh_hid_macros.h"

static BOOL hid_report_descriptor_append_usage(struct hid_report_descriptor *desc, USAGE usage)
{
    const BYTE template[] =
    {
        USAGE(2, usage),
    };

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_begin_report_descriptor(struct unix_device *iface, USAGE usage_page, USAGE usage)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE template[] =
    {
        USAGE_PAGE(2, usage_page),
        USAGE(2, usage),
        COLLECTION(1, Application),
            USAGE(1, 0),
    };

    memset(desc, 0, sizeof(*desc));
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_end_report_descriptor(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    static const BYTE template[] =
    {
        END_COLLECTION,
    };

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_begin_input_report(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    struct hid_device_state *state = &iface->hid_device_state;
    const BYTE report_id = ++desc->next_report_id[HidP_Input];
    const BYTE template[] =
    {
        COLLECTION(1, Report),
            REPORT_ID(1, report_id),
    };

    if (state->report_len)
    {
        ERR("input report already created\n");
        return FALSE;
    }

    state->id = report_id;
    state->bit_size += 8;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_end_input_report(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    struct hid_device_state *state = &iface->hid_device_state;
    static const BYTE template[] =
    {
        END_COLLECTION,
    };

    state->report_len = (state->bit_size + 7) / 8;
    if (!(state->report_buf = calloc(1, state->report_len))) return FALSE;
    if (!(state->last_report_buf = calloc(1, state->report_len))) return FALSE;

    state->report_buf[0] = state->id;
    state->last_report_buf[0] = state->id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_device_add_button_count(struct unix_device *iface, BYTE count)
{
    USHORT offset = iface->hid_device_state.bit_size / 8;

    if ((iface->hid_device_state.bit_size % 8) && !iface->hid_device_state.button_count)
        ERR("buttons should start byte aligned, missing padding!\n");
    else if (iface->hid_device_state.bit_size + count > 0x80000)
        ERR("report size overflow, too many elements!\n");
    else
    {
        if (!iface->hid_device_state.button_count) iface->hid_device_state.button_start = offset;
        iface->hid_device_state.button_count += count;
        iface->hid_device_state.bit_size += count;
        return TRUE;
    }

    return FALSE;
}

BOOL hid_device_add_buttons(struct unix_device *iface, USAGE usage_page, USAGE usage_min, USAGE usage_max)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const USHORT count = usage_max - usage_min + 1;
    const BYTE template[] =
    {
        USAGE_PAGE(2, usage_page),
        USAGE_MINIMUM(2, usage_min),
        USAGE_MAXIMUM(2, usage_max),
        LOGICAL_MINIMUM(1, 0),
        LOGICAL_MAXIMUM(1, 1),
        PHYSICAL_MINIMUM(1, 0),
        PHYSICAL_MAXIMUM(1, 1),
        REPORT_COUNT(2, count),
        REPORT_SIZE(1, 1),
        INPUT(1, Data|Var|Abs),
    };
    const BYTE template_pad[] =
    {
        REPORT_COUNT(1, 8 - (count % 8)),
        REPORT_SIZE(1, 1),
        INPUT(1, Cnst|Var|Abs),
    };

    if (!hid_device_add_button_count(iface, usage_max - usage_min + 1))
        return FALSE;

    if (!hid_report_descriptor_append(desc, template, sizeof(template)))
        return FALSE;

    if ((count % 8) && !hid_report_descriptor_append(desc, template_pad, sizeof(template_pad)))
        return FALSE;

    return TRUE;
}

static BOOL hid_device_add_hatswitch_count(struct unix_device *iface, BYTE count)
{
    USHORT offset = iface->hid_device_state.bit_size / 8;

    if (iface->hid_device_state.button_count)
        ERR("hatswitches should be added before buttons!\n");
    else if ((iface->hid_device_state.bit_size % 8))
        ERR("hatswitches should be byte aligned, missing padding!\n");
    else if (iface->hid_device_state.bit_size + 8 * count > 0x80000)
        ERR("report size overflow, too many elements!\n");
    else
    {
        if (!iface->hid_device_state.hatswitch_count) iface->hid_device_state.hatswitch_start = offset;
        iface->hid_device_state.hatswitch_count += count;
        iface->hid_device_state.bit_size += 8 * count;
        return TRUE;
    }

    return FALSE;
}

BOOL hid_device_add_hatswitch(struct unix_device *iface, INT count)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE template[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
        LOGICAL_MINIMUM(1, 1),
        LOGICAL_MAXIMUM(1, 8),
        PHYSICAL_MINIMUM(1, 0),
        PHYSICAL_MAXIMUM(2, 8),
        REPORT_SIZE(1, 8),
        REPORT_COUNT(4, count),
        UNIT(1, 0x0e /* none */),
        INPUT(1, Data|Var|Abs|Null),
    };

    if (!hid_device_add_hatswitch_count(iface, count))
        return FALSE;

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_device_add_axis_count(struct unix_device *iface, BOOL rel, BYTE count)
{
    USHORT offset = iface->hid_device_state.bit_size / 8;

    if (!rel && iface->hid_device_state.rel_axis_count)
        ERR("absolute axes should be added before relative axes!\n");
    else if (iface->hid_device_state.button_count || iface->hid_device_state.hatswitch_count)
        ERR("axes should be added before buttons or hatswitches!\n");
    else if ((iface->hid_device_state.bit_size % 8))
        ERR("axes should be byte aligned, missing padding!\n");
    else if (iface->hid_device_state.bit_size + 32 * count > 0x80000)
        ERR("report size overflow, too many elements!\n");
    else if (rel)
    {
        if (!iface->hid_device_state.rel_axis_count) iface->hid_device_state.rel_axis_start = offset;
        iface->hid_device_state.rel_axis_count += count;
        iface->hid_device_state.bit_size += 32 * count;
        return TRUE;
    }
    else
    {
        if (!iface->hid_device_state.abs_axis_count) iface->hid_device_state.abs_axis_start = offset;
        iface->hid_device_state.abs_axis_count += count;
        iface->hid_device_state.bit_size += 32 * count;
        return TRUE;
    }

    return FALSE;
}

BOOL hid_device_add_axes(struct unix_device *iface, BYTE count, USAGE usage_page,
                         const USAGE *usages, BOOL rel, LONG min, LONG max)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE template_begin[] =
    {
        USAGE_PAGE(1, usage_page),
        COLLECTION(1, Physical),
    };
    const BYTE template_end[] =
    {
        END_COLLECTION,
    };
    const BYTE template[] =
    {
        LOGICAL_MINIMUM(4, min),
        LOGICAL_MAXIMUM(4, max),
        PHYSICAL_MINIMUM(4, min),
        PHYSICAL_MAXIMUM(4, max),
        REPORT_SIZE(1, 32),
        REPORT_COUNT(1, count),
        INPUT(1, Data|Var|(rel ? Rel : Abs)),
    };
    int i;

    if (!hid_device_add_axis_count(iface, rel, count))
        return FALSE;

    if (!hid_report_descriptor_append(desc, template_begin, sizeof(template_begin)))
        return FALSE;

    for (i = 0; i < count; i++)
    {
        if (!hid_report_descriptor_append_usage(desc, usages[i]))
            return FALSE;
    }

    if (!hid_report_descriptor_append(desc, template, sizeof(template)))
        return FALSE;

    if (!hid_report_descriptor_append(desc, template_end, sizeof(template_end)))
        return FALSE;

    return TRUE;
}

BOOL hid_device_add_haptics(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE haptics_features_report = ++desc->next_report_id[HidP_Feature];
    const BYTE haptics_waveform_report = ++desc->next_report_id[HidP_Output];
    const BYTE haptics_template[] =
    {
        USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
        USAGE(1, HID_USAGE_HAPTICS_SIMPLE_CONTROLLER),
        COLLECTION(1, Logical),
            REPORT_ID(1, haptics_features_report),

            USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_LIST),
            COLLECTION(1, NamedArray),
                USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                USAGE(1, 3), /* HID_USAGE_HAPTICS_WAVEFORM_RUMBLE */
                USAGE(1, 4), /* HID_USAGE_HAPTICS_WAVEFORM_BUZZ */
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 16),
                FEATURE(1, Data|Var|Abs|Null),
            END_COLLECTION,

            USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
            USAGE(1, HID_USAGE_HAPTICS_DURATION_LIST),
            COLLECTION(1, NamedArray),
                USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                USAGE(1, 3), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_RUMBLE) */
                USAGE(1, 4), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_BUZZ) */
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 16),
                FEATURE(1, Data|Var|Abs|Null),
            END_COLLECTION,

            USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
            USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME),
            UNIT(2, 0x1001), /* seconds */
            UNIT_EXPONENT(1, -3), /* 10^-3 */
            LOGICAL_MINIMUM(4, 0x00000000),
            LOGICAL_MAXIMUM(4, 0x7fffffff),
            PHYSICAL_MINIMUM(4, 0x00000000),
            PHYSICAL_MAXIMUM(4, 0x7fffffff),
            REPORT_SIZE(1, 32),
            REPORT_COUNT(1, 1),
            FEATURE(1, Data|Var|Abs),
            /* reset global items */
            UNIT(1, 0), /* None */
            UNIT_EXPONENT(1, 0),

            REPORT_ID(1, haptics_waveform_report),
            USAGE(1, HID_USAGE_HAPTICS_MANUAL_TRIGGER),
            LOGICAL_MINIMUM(1, 1),
            LOGICAL_MAXIMUM(1, 4),
            PHYSICAL_MINIMUM(1, 1),
            PHYSICAL_MAXIMUM(1, 4),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, HID_USAGE_HAPTICS_INTENSITY),
            LOGICAL_MINIMUM(4, 0x00000000),
            LOGICAL_MAXIMUM(4, 0x0000ffff),
            PHYSICAL_MINIMUM(4, 0x00000000),
            PHYSICAL_MAXIMUM(4, 0x0000ffff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
        END_COLLECTION,
    };

    iface->hid_haptics.features_report = haptics_features_report;
    iface->hid_haptics.waveform_report = haptics_waveform_report;
    iface->hid_haptics.features.waveform_list[0] = HID_USAGE_HAPTICS_WAVEFORM_RUMBLE;
    iface->hid_haptics.features.waveform_list[1] = HID_USAGE_HAPTICS_WAVEFORM_BUZZ;
    iface->hid_haptics.features.duration_list[0] = 0;
    iface->hid_haptics.features.duration_list[1] = 0;
    iface->hid_haptics.features.waveform_cutoff_time_ms = 1000;

    return hid_report_descriptor_append(desc, haptics_template, sizeof(haptics_template));
}

#include "pop_hid_macros.h"

static void hid_device_destroy(struct unix_device *iface)
{
    iface->hid_vtbl->destroy(iface);
    free(iface->hid_report_descriptor.data);
    free(iface->hid_device_state.report_buf);
    free(iface->hid_device_state.last_report_buf);
}

static NTSTATUS hid_device_start(struct unix_device *iface)
{
    return iface->hid_vtbl->start(iface);
}

static void hid_device_stop(struct unix_device *iface)
{
    iface->hid_vtbl->stop(iface);
}

NTSTATUS hid_device_get_report_descriptor(struct unix_device *iface, BYTE *buffer, DWORD length, DWORD *out_length)
{
    *out_length = iface->hid_report_descriptor.size;
    if (length < iface->hid_report_descriptor.size) return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, iface->hid_report_descriptor.data, iface->hid_report_descriptor.size);
    return STATUS_SUCCESS;
}

static void hid_device_set_output_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct hid_haptics *haptics = &iface->hid_haptics;

    if (packet->reportId == haptics->waveform_report)
    {
        struct hid_haptics_waveform *waveform = (struct hid_haptics_waveform *)(packet->reportBuffer + 1);
        struct hid_haptics_waveform *rumble = haptics->waveforms + HAPTICS_WAVEFORM_RUMBLE_INDEX;
        struct hid_haptics_waveform *buzz = haptics->waveforms + HAPTICS_WAVEFORM_BUZZ_INDEX;
        ULONG duration_ms;

        io->Information = sizeof(*waveform) + 1;
        assert(packet->reportBufferLen == io->Information);

        if (waveform->manual_trigger == 0 || waveform->manual_trigger > HAPTICS_WAVEFORM_LAST_INDEX)
            io->Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (waveform->manual_trigger == HAPTICS_WAVEFORM_STOP_INDEX)
                memset(haptics->waveforms, 0, sizeof(haptics->waveforms));
            else
                haptics->waveforms[waveform->manual_trigger] = *waveform;

            duration_ms = haptics->features.waveform_cutoff_time_ms;
            io->Status = iface->hid_vtbl->haptics_start(iface, duration_ms, rumble->intensity, buzz->intensity);
        }
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_NOT_IMPLEMENTED;
    }
}

static void hid_device_get_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct hid_haptics *haptics = &iface->hid_haptics;

    if (packet->reportId == haptics->features_report)
    {
        struct hid_haptics_features *features = (struct hid_haptics_features *)(packet->reportBuffer + 1);

        io->Information = sizeof(*features) + 1;
        assert(packet->reportBufferLen == io->Information);

        *features = haptics->features;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_NOT_IMPLEMENTED;
    }
}

static void hid_device_set_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct hid_haptics *haptics = &iface->hid_haptics;

    if (packet->reportId == haptics->features_report)
    {
        struct hid_haptics_features *features = (struct hid_haptics_features *)(packet->reportBuffer + 1);

        io->Information = sizeof(*features) + 1;
        assert(packet->reportBufferLen == io->Information);

        haptics->features.waveform_cutoff_time_ms = features->waveform_cutoff_time_ms;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_NOT_IMPLEMENTED;
    }
}

static const struct raw_device_vtbl raw_device_vtbl =
{
    hid_device_destroy,
    hid_device_start,
    hid_device_stop,
    hid_device_get_report_descriptor,
    hid_device_set_output_report,
    hid_device_get_feature_report,
    hid_device_set_feature_report,
};

void *hid_device_create(const struct hid_device_vtbl *vtbl, SIZE_T size)
{
    struct unix_device *impl;

    if (!(impl = raw_device_create(&raw_device_vtbl, size))) return NULL;
    impl->hid_vtbl = vtbl;

    return impl;
}

#ifdef WORDS_BIGENDIAN
# define LE_ULONG(x) RtlUlongByteSwap((ULONG)(x))
#else
# define LE_ULONG(x) ((ULONG)(x))
#endif

BOOL hid_device_set_abs_axis(struct unix_device *iface, ULONG index, LONG value)
{
    struct hid_device_state *state = &iface->hid_device_state;
    ULONG offset = state->abs_axis_start + index * 4;
    if (index > state->abs_axis_count) return FALSE;
    *(ULONG *)(state->report_buf + offset) = LE_ULONG(value);
    return TRUE;
}

BOOL hid_device_set_rel_axis(struct unix_device *iface, ULONG index, LONG value)
{
    struct hid_device_state *state = &iface->hid_device_state;
    ULONG offset = state->rel_axis_start + index * 4;
    if (index > state->rel_axis_count) return FALSE;
    *(ULONG *)(state->report_buf + offset) = LE_ULONG(value);
    return TRUE;
}

BOOL hid_device_set_button(struct unix_device *iface, ULONG index, BOOL is_set)
{
    struct hid_device_state *state = &iface->hid_device_state;
    ULONG offset = state->button_start + (index / 8);
    BYTE mask = (1 << (index % 8));
    if (index > state->button_count) return FALSE;
    if (is_set) state->report_buf[offset] |= mask;
    else state->report_buf[offset] &= ~mask;
    return TRUE;
}

/* hatswitch x / y vs value:
 *      -1  x +1
 *     +-------->
 *  -1 | 8  1  2
 *   y | 7  0  3
 *  +1 | 6  5  4
 *     v
 */
static void hatswitch_decompose(BYTE value, LONG *x, LONG *y)
{
    *x = *y = 0;
    if (value == 8 || value == 1 || value == 2) *y = -1;
    if (value == 6 || value == 5 || value == 4) *y = +1;
    if (value == 8 || value == 7 || value == 6) *x = -1;
    if (value == 2 || value == 3 || value == 4) *x = +1;
}

static void hatswitch_compose(LONG x, LONG y, BYTE *value)
{
    if (x == 0 && y == 0) *value = 0;
    else if (x == 0 && y < 0) *value = 1;
    else if (x > 0 && y < 0) *value = 2;
    else if (x > 0 && y == 0) *value = 3;
    else if (x > 0 && y > 0) *value = 4;
    else if (x == 0 && y > 0) *value = 5;
    else if (x < 0 && y > 0) *value = 6;
    else if (x < 0 && y == 0) *value = 7;
    else if (x < 0 && y < 0) *value = 8;
}

BOOL hid_device_set_hatswitch_x(struct unix_device *iface, ULONG index, LONG new_x)
{
    struct hid_device_state *state = &iface->hid_device_state;
    ULONG offset = state->hatswitch_start + index;
    LONG x, y;
    if (index > state->hatswitch_count) return FALSE;
    hatswitch_decompose(state->report_buf[offset], &x, &y);
    hatswitch_compose(new_x, y, &state->report_buf[offset]);
    return TRUE;
}

BOOL hid_device_set_hatswitch_y(struct unix_device *iface, ULONG index, LONG new_y)
{
    struct hid_device_state *state = &iface->hid_device_state;
    ULONG offset = state->hatswitch_start + index;
    LONG x, y;
    if (index > state->hatswitch_count) return FALSE;
    hatswitch_decompose(state->report_buf[offset], &x, &y);
    hatswitch_compose(x, new_y, &state->report_buf[offset]);
    return TRUE;
}

BOOL hid_device_sync_report(struct unix_device *iface)
{
    BOOL dropped;

    if (!(dropped = iface->hid_device_state.dropped))
        memcpy(iface->hid_device_state.last_report_buf, iface->hid_device_state.report_buf,
               iface->hid_device_state.report_len);
    else
        memcpy(iface->hid_device_state.report_buf, iface->hid_device_state.last_report_buf,
               iface->hid_device_state.report_len);
    iface->hid_device_state.dropped = FALSE;

    return !dropped;
}

void hid_device_drop_report(struct unix_device *iface)
{
    iface->hid_device_state.dropped = TRUE;
}
