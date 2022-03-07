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
#include "wine/hid.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

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

static BOOL hid_device_begin_collection(struct hid_report_descriptor *desc, const USAGE_AND_PAGE *usage, BYTE type)
{
    const BYTE template[] =
    {
        USAGE_PAGE(2, usage->UsagePage),
        USAGE(2, usage->Usage),
        COLLECTION(1, type),
    };

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_device_end_collection(struct hid_report_descriptor *desc)
{
    static const BYTE template[] =
    {
        END_COLLECTION,
    };

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_begin_report_descriptor(struct unix_device *iface, const USAGE_AND_PAGE *device_usage)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    memset(desc, 0, sizeof(*desc));
    return hid_device_begin_collection(desc, device_usage, Application);
}

BOOL hid_device_end_report_descriptor(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    return hid_device_end_collection(desc);
}

BOOL hid_device_begin_input_report(struct unix_device *iface, const USAGE_AND_PAGE *physical_usage)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    struct hid_device_state *state = &iface->hid_device_state;
    const BYTE report_id = ++desc->next_report_id[HidP_Input];
    const BYTE template[] =
    {
        REPORT_ID(1, report_id),
    };

    if (state->report_len)
    {
        ERR("input report already created\n");
        return FALSE;
    }

    state->id = report_id;
    state->bit_size += 8;

    if (!hid_device_begin_collection(desc, physical_usage, Physical))
        return FALSE;

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_end_input_report(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    struct hid_device_state *state = &iface->hid_device_state;

    state->report_len = (state->bit_size + 7) / 8;
    if (!(state->report_buf = calloc(1, state->report_len))) return FALSE;
    if (!(state->last_report_buf = calloc(1, state->report_len))) return FALSE;

    state->report_buf[0] = state->id;
    state->last_report_buf[0] = state->id;
    return hid_device_end_collection(desc);
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
        REPORT_SIZE(1, 8),
        REPORT_COUNT(4, count),
        UNIT(1, 0x0e /* none */),
        INPUT(1, Data|Var|Abs|Null),
    };

    if (!hid_device_add_hatswitch_count(iface, count))
        return FALSE;

    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_device_add_axis_count(struct unix_device *iface, BOOL rel, BYTE count,
                                      USAGE usage_page, const USAGE *usages)
{
    struct hid_device_state *state = &iface->hid_device_state;
    USHORT i, offset = state->bit_size / 8;

    if (!rel && state->rel_axis_count)
        ERR("absolute axes should be added before relative axes!\n");
    else if (state->button_count || state->hatswitch_count)
        ERR("axes should be added before buttons or hatswitches!\n");
    else if ((state->bit_size % 8))
        ERR("axes should be byte aligned, missing padding!\n");
    else if (state->bit_size + 32 * count > 0x80000)
        ERR("report size overflow, too many elements!\n");
    else if (rel)
    {
        if (!state->rel_axis_count) state->rel_axis_start = offset;
        state->rel_axis_count += count;
        state->bit_size += 32 * count;
        return TRUE;
    }
    else
    {
        if (state->abs_axis_count + count > ARRAY_SIZE(state->abs_axis_usages))
        {
            ERR("absolute axis usage overflow, too many elements!\n");
            return FALSE;
        }
        for (i = 0; i < count; ++i)
        {
            state->abs_axis_usages[state->abs_axis_count + i].UsagePage = usage_page;
            state->abs_axis_usages[state->abs_axis_count + i].Usage = usages[i];
        }

        if (!state->abs_axis_count) state->abs_axis_start = offset;
        state->abs_axis_count += count;
        state->bit_size += 32 * count;
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
        REPORT_SIZE(1, 32),
        REPORT_COUNT(1, count),
        INPUT(1, Data|Var|(rel ? Rel : Abs)),
    };
    int i;

    if (!hid_device_add_axis_count(iface, rel, count, usage_page, usages))
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

#include "pshpack1.h"
struct hid_haptics_waveform
{
    UINT16 intensity;
    BYTE manual_trigger;
    BYTE repeat_count;
};
#include "poppack.h"

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
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|HAPTICS_WAVEFORM_RUMBLE_ORDINAL), /* HID_USAGE_HAPTICS_WAVEFORM_RUMBLE */
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|HAPTICS_WAVEFORM_BUZZ_ORDINAL), /* HID_USAGE_HAPTICS_WAVEFORM_BUZZ */
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 16),
                FEATURE(1, Data|Var|Abs|Null),
            END_COLLECTION,

            USAGE(1, HID_USAGE_HAPTICS_DURATION_LIST),
            COLLECTION(1, NamedArray),
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|HAPTICS_WAVEFORM_RUMBLE_ORDINAL), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_RUMBLE) */
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|HAPTICS_WAVEFORM_BUZZ_ORDINAL), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_BUZZ) */
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 16),
                FEATURE(1, Data|Var|Abs|Null),
            END_COLLECTION,

            USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME),
            UNIT(2, 0x1001), /* seconds */
            UNIT_EXPONENT(1, -3), /* 10^-3 */
            LOGICAL_MINIMUM(4, 0x00000000),
            LOGICAL_MAXIMUM(4, 0x7fffffff),
            REPORT_SIZE(1, 32),
            REPORT_COUNT(1, 1),
            FEATURE(1, Data|Var|Abs),
            /* reset global items */
            UNIT(1, 0), /* None */
            UNIT_EXPONENT(1, 0),

            REPORT_ID(1, haptics_waveform_report),
            USAGE(1, HID_USAGE_HAPTICS_INTENSITY),
            LOGICAL_MINIMUM(4, 0x00000000),
            LOGICAL_MAXIMUM(4, 0x0000ffff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, HID_USAGE_HAPTICS_MANUAL_TRIGGER),
            LOGICAL_MINIMUM(1, HAPTICS_WAVEFORM_NONE_ORDINAL),
            LOGICAL_MAXIMUM(1, HAPTICS_WAVEFORM_LAST_ORDINAL),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, HID_USAGE_HAPTICS_REPEAT_COUNT),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 1),
            REPORT_SIZE(1, 8),
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

#include "pshpack1.h"
struct pid_device_control
{
    BYTE control_index;
};

static const USAGE pid_device_control_usages[] =
{
    0, /* HID nary collection indexes start at 1 */
    PID_USAGE_DC_ENABLE_ACTUATORS,
    PID_USAGE_DC_DISABLE_ACTUATORS,
    PID_USAGE_DC_STOP_ALL_EFFECTS,
    PID_USAGE_DC_DEVICE_RESET,
    PID_USAGE_DC_DEVICE_PAUSE,
    PID_USAGE_DC_DEVICE_CONTINUE,
};

struct pid_device_gain
{
    BYTE value;
};

struct pid_effect_control
{
    BYTE index;
    BYTE control_index;
    BYTE iterations;
};

static const USAGE pid_effect_control_usages[] =
{
    0, /* HID nary collection indexes start at 1 */
    PID_USAGE_OP_EFFECT_START,
    PID_USAGE_OP_EFFECT_START_SOLO,
    PID_USAGE_OP_EFFECT_STOP,
};

struct pid_effect_update
{
    BYTE index;
    BYTE type_index;
    UINT16 duration;
    UINT16 trigger_repeat_interval;
    UINT16 sample_period;
    UINT16 start_delay;
    BYTE trigger_button;
    BYTE enable_bits;
    UINT16 direction[2];
};

struct pid_set_periodic
{
    BYTE index;
    UINT16 magnitude;
    INT16 offset;
    UINT16 phase;
    UINT16 period;
};

struct pid_set_envelope
{
    BYTE index;
    UINT16 attack_level;
    UINT16 fade_level;
    UINT16 attack_time;
    UINT16 fade_time;
};

struct pid_set_condition
{
    BYTE index;
    BYTE condition_index;
    INT16 center_point_offset;
    INT16 positive_coefficient;
    INT16 negative_coefficient;
    UINT16 positive_saturation;
    UINT16 negative_saturation;
    UINT16 dead_band;
};

struct pid_set_constant_force
{
    BYTE index;
    INT16 magnitude;
};

struct pid_set_ramp_force
{
    BYTE index;
    INT16 ramp_start;
    INT16 ramp_end;
};

struct pid_effect_state
{
    BYTE flags;
    BYTE index;
};
#include "poppack.h"

static BOOL hid_descriptor_add_set_periodic(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE report_id = ++desc->next_report_id[HidP_Output];
    const BYTE template[] =
    {
        /* Periodic Report Definition */
        USAGE(1, PID_USAGE_SET_PERIODIC_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, report_id),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_MAGNITUDE),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(2, 10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),

            USAGE(1, PID_USAGE_OFFSET),
            LOGICAL_MINIMUM(2, 0x8000),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(2, -10000),
            PHYSICAL_MAXIMUM(2, +10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),

            USAGE(1, PID_USAGE_PHASE),
            UNIT(1, 0x14), /* Eng Rot:Angular Pos */
            UNIT_EXPONENT(1, -2),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(4, 36000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_PERIOD),
            UNIT(2, 0x1003), /* Eng Lin:Time */
            UNIT_EXPONENT(1, -3), /* 10^-3 */
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x7fff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            UNIT_EXPONENT(1, 0),
            UNIT(1, 0), /* None */
        END_COLLECTION,
    };

    iface->hid_physical.set_periodic_report = report_id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_descriptor_add_set_envelope(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE report_id = ++desc->next_report_id[HidP_Output];
    const BYTE template[] =
    {
        /* Envelope Report Definition */
        USAGE(1, PID_USAGE_SET_ENVELOPE_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, report_id),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_ATTACK_LEVEL),
            USAGE(1, PID_USAGE_FADE_LEVEL),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(2, 10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 2),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),

            USAGE(1, PID_USAGE_ATTACK_TIME),
            USAGE(1, PID_USAGE_FADE_TIME),
            UNIT(2, 0x1003), /* Eng Lin:Time */
            UNIT_EXPONENT(1, -3),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x7fff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 2),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MAXIMUM(1, 0),
            UNIT_EXPONENT(1, 0),
            UNIT(1, 0),
        END_COLLECTION,
    };

    iface->hid_physical.set_envelope_report = report_id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_descriptor_add_set_condition(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE report_id = ++desc->next_report_id[HidP_Output];
    const BYTE template[] =
    {
        /* Condition Report Definition */
        USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, report_id),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_PARAMETER_BLOCK_OFFSET),
            LOGICAL_MINIMUM(1, 0x00),
            LOGICAL_MAXIMUM(1, 0x01),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_CP_OFFSET),
            USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
            USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
            LOGICAL_MINIMUM(2, 0x8000),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(2, -10000),
            PHYSICAL_MAXIMUM(2, +10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 3),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),

            USAGE(1, PID_USAGE_POSITIVE_SATURATION),
            USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
            USAGE(1, PID_USAGE_DEAD_BAND),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(4, 0xffff),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(2, +10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 3),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),
        END_COLLECTION,
    };

    iface->hid_physical.set_condition_report = report_id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_descriptor_add_set_constant_force(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE report_id = ++desc->next_report_id[HidP_Output];
    const BYTE template[] =
    {
        /* Constant Force Report Definition */
        USAGE(1, PID_USAGE_SET_CONSTANT_FORCE_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, report_id),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_MAGNITUDE),
            LOGICAL_MINIMUM(2, 0x8000),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(2, -10000),
            PHYSICAL_MAXIMUM(2, +10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),
        END_COLLECTION,
    };

    iface->hid_physical.set_constant_force_report = report_id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

static BOOL hid_descriptor_add_set_ramp_force(struct unix_device *iface)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE report_id = ++desc->next_report_id[HidP_Output];
    const BYTE template[] =
    {
        /* Ramp Force Report Definition */
        USAGE(1, PID_USAGE_SET_RAMP_FORCE_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, report_id),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_RAMP_START),
            USAGE(1, PID_USAGE_RAMP_END),
            LOGICAL_MINIMUM(2, 0x8000),
            LOGICAL_MAXIMUM(2, 0x7fff),
            PHYSICAL_MINIMUM(2, -10000),
            PHYSICAL_MAXIMUM(2, +10000),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 2),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),
        END_COLLECTION,
    };

    iface->hid_physical.set_ramp_force_report = report_id;
    return hid_report_descriptor_append(desc, template, sizeof(template));
}

BOOL hid_device_add_physical(struct unix_device *iface, USAGE *usages, USHORT count)
{
    struct hid_report_descriptor *desc = &iface->hid_report_descriptor;
    const BYTE device_control_report = ++desc->next_report_id[HidP_Output];
    struct hid_device_state *state = &iface->hid_device_state;
    const BYTE device_control_header[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_PID),
        USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, device_control_report),

            USAGE(1, PID_USAGE_DEVICE_CONTROL),
            COLLECTION(1, Logical),
    };
    const BYTE device_control_footer[] =
    {
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 6),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Ary|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };

    const BYTE device_gain_report = ++desc->next_report_id[HidP_Output];
    const BYTE device_gain[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_PID),
        USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, device_gain_report),

            USAGE(1, PID_USAGE_DEVICE_GAIN),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 100),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(2, 10000),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
            PHYSICAL_MINIMUM(1, 0),
            PHYSICAL_MAXIMUM(1, 0),
        END_COLLECTION,
    };

    const BYTE effect_control_report = ++desc->next_report_id[HidP_Output];
    const BYTE effect_control_header[] =
    {
        /* Control effect state */
        USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, effect_control_report),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_EFFECT_OPERATION),
            COLLECTION(1, Logical),
    };
    const BYTE effect_control_footer[] =
    {
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 3),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Ary|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_LOOP_COUNT),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x00ff),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
        END_COLLECTION,
    };

    const BYTE effect_update_report = ++desc->next_report_id[HidP_Output];
    const BYTE effect_update_header[] =
    {
        /* Set effect properties */
        USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, effect_update_report),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_EFFECT_TYPE),
            COLLECTION(1, Logical),
    };
    const BYTE effect_update_footer[] =
    {
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, count),
                REPORT_SIZE(1, 8),
                OUTPUT(1, Data|Ary|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_DURATION),
            USAGE(1, PID_USAGE_TRIGGER_REPEAT_INTERVAL),
            USAGE(1, PID_USAGE_SAMPLE_PERIOD),
            USAGE(1, PID_USAGE_START_DELAY),
            UNIT(2, 0x1003), /* Eng Lin:Time */
            UNIT_EXPONENT(1, -3), /* 10^-3 */
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, 0x7fff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 4),
            OUTPUT(1, Data|Var|Abs),
            UNIT_EXPONENT(1, 0),
            UNIT(1, 0), /* None */

            USAGE(1, PID_USAGE_TRIGGER_BUTTON),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(2, state->button_count),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs|Null),

            USAGE(1, PID_USAGE_AXES_ENABLE),
            COLLECTION(1, Logical),
                USAGE(4, (state->abs_axis_usages[0].UsagePage<<16)|state->abs_axis_usages[0].Usage),
                USAGE(4, (state->abs_axis_usages[1].UsagePage<<16)|state->abs_axis_usages[1].Usage),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,
            USAGE(1, PID_USAGE_DIRECTION_ENABLE),
            REPORT_COUNT(1, 1),
            OUTPUT(1, Data|Var|Abs),
            REPORT_COUNT(1, 5),
            OUTPUT(1, Cnst|Var|Abs), /* 5-bit pad */

            USAGE(1, PID_USAGE_DIRECTION),
            COLLECTION(1, Logical),
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|1),
                USAGE(4, (HID_USAGE_PAGE_ORDINAL<<16)|2),
                UNIT(1, 0x14), /* Eng Rot:Angular Pos */
                UNIT_EXPONENT(1, -2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 36000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,
            UNIT_EXPONENT(1, 0),
            UNIT(1, 0), /* None */
        END_COLLECTION,
    };

    const BYTE effect_state_report = ++desc->next_report_id[HidP_Input];
    const BYTE effect_state_template[] =
    {
        /* Report effect state */
        USAGE(1, PID_USAGE_STATE_REPORT),
        COLLECTION(1, Logical),
            REPORT_ID(1, effect_state_report),

            USAGE(1, PID_USAGE_DEVICE_PAUSED),
            USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
            USAGE(1, PID_USAGE_EFFECT_PLAYING),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 1),
            REPORT_SIZE(1, 1),
            REPORT_COUNT(1, 8),
            INPUT(1, Data|Var|Abs),

            USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
            LOGICAL_MINIMUM(1, 0),
            LOGICAL_MAXIMUM(1, 0x7f),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            INPUT(1, Data|Var|Abs),
        END_COLLECTION,
    };
    struct hid_effect_state *effect_state = &iface->hid_physical.effect_state;
    BOOL periodic = FALSE;
    BOOL envelope = FALSE;
    BOOL condition = FALSE;
    BOOL constant_force = FALSE;
    BOOL ramp_force = FALSE;
    ULONG i;

    if (!hid_report_descriptor_append(desc, device_control_header, sizeof(device_control_header)))
        return FALSE;
    for (i = 1; i < ARRAY_SIZE(pid_device_control_usages); ++i)
    {
        if (!hid_report_descriptor_append_usage(desc, pid_device_control_usages[i]))
            return FALSE;
    }
    if (!hid_report_descriptor_append(desc, device_control_footer, sizeof(device_control_footer)))
        return FALSE;

    if (!hid_report_descriptor_append(desc, device_gain, sizeof(device_gain)))
        return FALSE;

    if (!hid_report_descriptor_append(desc, effect_control_header, sizeof(effect_control_header)))
        return FALSE;
    for (i = 1; i < ARRAY_SIZE(pid_effect_control_usages); ++i)
    {
        if (!hid_report_descriptor_append_usage(desc, pid_effect_control_usages[i]))
            return FALSE;
    }
    if (!hid_report_descriptor_append(desc, effect_control_footer, sizeof(effect_control_footer)))
        return FALSE;

    if (!hid_report_descriptor_append(desc, effect_update_header, sizeof(effect_update_header)))
        return FALSE;
    for (i = 0; i < count; ++i)
    {
        if (!hid_report_descriptor_append_usage(desc, usages[i]))
            return FALSE;
    }
    if (!hid_report_descriptor_append(desc, effect_update_footer, sizeof(effect_update_footer)))
        return FALSE;

    for (i = 0; i < count; ++i)
    {
        if (usages[i] == PID_USAGE_ET_SINE ||
            usages[i] == PID_USAGE_ET_SQUARE ||
            usages[i] == PID_USAGE_ET_TRIANGLE ||
            usages[i] == PID_USAGE_ET_SAWTOOTH_UP ||
            usages[i] == PID_USAGE_ET_SAWTOOTH_DOWN)
            periodic = envelope = TRUE;
        if (usages[i] == PID_USAGE_ET_SPRING ||
            usages[i] == PID_USAGE_ET_DAMPER ||
            usages[i] == PID_USAGE_ET_INERTIA ||
            usages[i] == PID_USAGE_ET_FRICTION)
            condition = TRUE;
        if (usages[i] == PID_USAGE_ET_CONSTANT_FORCE)
            envelope = constant_force = TRUE;
        if (usages[i] == PID_USAGE_ET_RAMP)
            envelope = ramp_force = TRUE;
    }

    if (periodic && !hid_descriptor_add_set_periodic(iface))
        return FALSE;
    if (envelope && !hid_descriptor_add_set_envelope(iface))
        return FALSE;
    if (condition && !hid_descriptor_add_set_condition(iface))
        return FALSE;
    if (constant_force && !hid_descriptor_add_set_constant_force(iface))
        return FALSE;
    if (ramp_force && !hid_descriptor_add_set_ramp_force(iface))
        return FALSE;

    if (!hid_report_descriptor_append(desc, effect_state_template, sizeof(effect_state_template)))
        return FALSE;

    /* HID nary collection indexes start at 1 */
    memcpy(iface->hid_physical.effect_types + 1, usages, count * sizeof(*usages));

    iface->hid_physical.device_control_report = device_control_report;
    iface->hid_physical.device_gain_report = device_gain_report;
    iface->hid_physical.effect_control_report = effect_control_report;
    iface->hid_physical.effect_update_report = effect_update_report;

    effect_state->id = effect_state_report;
    effect_state->report_len = sizeof(struct pid_effect_state) + 1;
    if (!(effect_state->report_buf = calloc(1, effect_state->report_len))) return FALSE;
    effect_state->report_buf[0] = effect_state->id;

    return TRUE;
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

static NTSTATUS hid_device_get_report_descriptor(struct unix_device *iface, BYTE *buffer, UINT length, UINT *out_length)
{
    *out_length = iface->hid_report_descriptor.size;
    if (length < iface->hid_report_descriptor.size) return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, iface->hid_report_descriptor.data, iface->hid_report_descriptor.size);
    return STATUS_SUCCESS;
}

static void hid_device_set_output_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct hid_physical *physical = &iface->hid_physical;
    struct hid_haptics *haptics = &iface->hid_haptics;

    if (packet->reportId == haptics->waveform_report)
    {
        struct hid_haptics_waveform *report = (struct hid_haptics_waveform *)(packet->reportBuffer + 1);
        UINT16 *rumble_intensity = haptics->waveform_intensity + HAPTICS_WAVEFORM_RUMBLE_ORDINAL;
        UINT16 *buzz_intensity = haptics->waveform_intensity + HAPTICS_WAVEFORM_BUZZ_ORDINAL;
        ULONG duration_ms;

        io->Information = sizeof(*report) + 1;
        assert(packet->reportBufferLen == io->Information);

        if (report->manual_trigger == 0 || report->manual_trigger > HAPTICS_WAVEFORM_LAST_ORDINAL)
            io->Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (report->manual_trigger == HAPTICS_WAVEFORM_STOP_ORDINAL)
            {
                memset(haptics->waveform_intensity, 0, sizeof(haptics->waveform_intensity));
                io->Status = iface->hid_vtbl->haptics_stop(iface);
            }
            else
            {
                haptics->waveform_intensity[report->manual_trigger] = report->intensity;
                duration_ms = haptics->features.waveform_cutoff_time_ms;
                if (!report->repeat_count) io->Status = STATUS_SUCCESS;
                else io->Status = iface->hid_vtbl->haptics_start(iface, duration_ms, *rumble_intensity, *buzz_intensity);
            }
        }
    }
    else if (packet->reportId == physical->device_control_report)
    {
        struct pid_device_control *report = (struct pid_device_control *)(packet->reportBuffer + 1);
        USAGE control;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else if (report->control_index >= ARRAY_SIZE(pid_device_control_usages))
            io->Status = STATUS_INVALID_PARAMETER;
        else if (!(control = pid_device_control_usages[report->control_index]))
            io->Status = STATUS_INVALID_PARAMETER;
        else
        {
            io->Status = iface->hid_vtbl->physical_device_control(iface, control);
            if (control == PID_USAGE_DC_DEVICE_RESET && io->Status == STATUS_SUCCESS)
                memset(physical->effect_params, 0, sizeof(physical->effect_params));
        }
    }
    else if (packet->reportId == physical->device_gain_report)
    {
        struct pid_device_gain *report = (struct pid_device_gain *)(packet->reportBuffer + 1);

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else
            io->Status = iface->hid_vtbl->physical_device_set_gain(iface, report->value);
    }
    else if (packet->reportId == physical->effect_control_report)
    {
        struct pid_effect_control *report = (struct pid_effect_control *)(packet->reportBuffer + 1);
        USAGE control;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else if (report->control_index >= ARRAY_SIZE(pid_effect_control_usages))
            io->Status = STATUS_INVALID_PARAMETER;
        else if (!(control = pid_effect_control_usages[report->control_index]))
            io->Status = STATUS_INVALID_PARAMETER;
        else
            io->Status = iface->hid_vtbl->physical_effect_control(iface, report->index, control, report->iterations);
    }
    else if (packet->reportId == physical->effect_update_report)
    {
        struct pid_effect_update *report = (struct pid_effect_update *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;
        USAGE effect_type;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else if (report->type_index >= ARRAY_SIZE(iface->hid_physical.effect_types))
            io->Status = STATUS_INVALID_PARAMETER;
        else if (!(effect_type = iface->hid_physical.effect_types[report->type_index]))
            io->Status = STATUS_INVALID_PARAMETER;
        else
        {
            params->effect_type = effect_type;
            params->duration = report->duration;
            params->trigger_repeat_interval = report->trigger_repeat_interval;
            params->sample_period = report->sample_period;
            params->start_delay = report->start_delay;
            params->trigger_button = report->trigger_button == 0xff ? 0 : report->trigger_button;
            params->axis_enabled[0] = (report->enable_bits & 1) != 0;
            params->axis_enabled[1] = (report->enable_bits & 2) != 0;
            params->direction_enabled = (report->enable_bits & 4) != 0;
            params->direction[0] = report->direction[0];
            params->direction[1] = report->direction[1];

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
        }
    }
    else if (packet->reportId == physical->set_periodic_report)
    {
        struct pid_set_periodic *report = (struct pid_set_periodic *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            params->periodic.magnitude = report->magnitude;
            params->periodic.offset = report->offset;
            params->periodic.phase = report->phase;
            params->periodic.period = report->period;

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
        }
    }
    else if (packet->reportId == physical->set_envelope_report)
    {
        struct pid_set_envelope *report = (struct pid_set_envelope *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            params->envelope.attack_level = report->attack_level;
            params->envelope.fade_level = report->fade_level;
            params->envelope.attack_time = report->attack_time;
            params->envelope.fade_time = report->fade_time;

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
        }
    }
    else if (packet->reportId == physical->set_condition_report)
    {
        struct pid_set_condition *report = (struct pid_set_condition *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;
        struct effect_condition *condition;
        UINT index;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else if ((index = report->condition_index) >= ARRAY_SIZE(params->condition))
            io->Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (params->condition_count <= index) params->condition_count = index + 1;
            condition = params->condition + index;
            condition->center_point_offset = report->center_point_offset;
            condition->positive_coefficient = report->positive_coefficient;
            condition->negative_coefficient = report->negative_coefficient;
            condition->positive_saturation = report->positive_saturation;
            condition->negative_saturation = report->negative_saturation;
            condition->dead_band = report->dead_band;

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
        }
    }
    else if (packet->reportId == physical->set_constant_force_report)
    {
        struct pid_set_constant_force *report = (struct pid_set_constant_force *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            params->constant_force.magnitude = report->magnitude;

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
        }
    }
    else if (packet->reportId == physical->set_ramp_force_report)
    {
        struct pid_set_ramp_force *report = (struct pid_set_ramp_force *)(packet->reportBuffer + 1);
        struct effect_params *params = iface->hid_physical.effect_params + report->index;

        io->Information = sizeof(*report) + 1;
        if (packet->reportBufferLen < io->Information)
            io->Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            params->ramp_force.ramp_start = report->ramp_start;
            params->ramp_force.ramp_end = report->ramp_end;

            io->Status = iface->hid_vtbl->physical_effect_update(iface, report->index, params);
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

void hid_device_set_effect_state(struct unix_device *iface, BYTE index, BYTE flags)
{
    struct hid_effect_state *state = &iface->hid_physical.effect_state;
    struct pid_effect_state *report = (struct pid_effect_state *)(state->report_buf + 1);
    report->index = index;
    report->flags = flags;
}
