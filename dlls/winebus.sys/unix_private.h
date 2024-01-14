/*
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

#ifndef __WINEBUS_UNIX_PRIVATE_H
#define __WINEBUS_UNIX_PRIVATE_H

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <ddk/hidsdi.h>

#include "unixlib.h"

#include "wine/list.h"

struct effect_periodic
{
    UINT16 magnitude;
    INT16 offset;
    UINT16 phase;
    UINT16 period;
};

struct effect_envelope
{
    UINT16 attack_level;
    UINT16 fade_level;
    UINT16 attack_time;
    UINT16 fade_time;
};

struct effect_condition
{
    INT16 center_point_offset;
    INT16 positive_coefficient;
    INT16 negative_coefficient;
    UINT16 positive_saturation;
    UINT16 negative_saturation;
    UINT16 dead_band;
};

struct effect_constant_force
{
    INT16 magnitude;
};

struct effect_ramp_force
{
    INT16 ramp_start;
    INT16 ramp_end;
};

struct effect_params
{
    USAGE effect_type;
    UINT16 duration;
    UINT16 trigger_repeat_interval;
    UINT16 sample_period;
    UINT16 start_delay;
    BYTE trigger_button;
    BOOL axis_enabled[2];
    BOOL direction_enabled;
    UINT16 direction[2];
    BYTE gain_percent;
    BYTE condition_count;
    /* only for periodic, constant or ramp forces */
    struct effect_envelope envelope;
    union
    {
        struct effect_periodic periodic;
        struct effect_condition condition[2];
        struct effect_constant_force constant_force;
        struct effect_ramp_force ramp_force;
    };
};

struct unix_device;

struct raw_device_vtbl
{
    void (*destroy)(struct unix_device *iface);
    NTSTATUS (*start)(struct unix_device *iface);
    void (*stop)(struct unix_device *iface);
    NTSTATUS (*get_report_descriptor)(struct unix_device *iface, BYTE *buffer, UINT length, UINT *out_length);
    void (*set_output_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*get_feature_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
    void (*set_feature_report)(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io);
};

struct hid_device_vtbl
{
    void (*destroy)(struct unix_device *iface);
    NTSTATUS (*start)(struct unix_device *iface);
    void (*stop)(struct unix_device *iface);
    NTSTATUS (*haptics_start)(struct unix_device *iface, UINT duration_ms,
                              USHORT rumble_intensity, USHORT buzz_intensity,
                              USHORT left_intensity, USHORT right_intensity);
    NTSTATUS (*haptics_stop)(struct unix_device *iface);
    NTSTATUS (*physical_device_control)(struct unix_device *iface, USAGE control);
    NTSTATUS (*physical_device_set_gain)(struct unix_device *iface, BYTE percent);
    NTSTATUS (*physical_effect_control)(struct unix_device *iface, BYTE index, USAGE control, BYTE iterations);
    NTSTATUS (*physical_effect_update)(struct unix_device *iface, BYTE index, struct effect_params *params);
};

struct hid_report_descriptor
{
    BYTE *data;
    SIZE_T size;
    SIZE_T max_size;
    BYTE next_report_id[3];
};

#include "pshpack1.h"
struct hid_haptics_feature
{
    WORD waveform;
    WORD duration;
    UINT cutoff_time_ms;
};

struct hid_haptics_features
{
    struct hid_haptics_feature rumble;
    struct hid_haptics_feature buzz;
    struct hid_haptics_feature left;
    struct hid_haptics_feature right;
};
#include "poppack.h"

struct hid_haptics
{
    struct hid_haptics_features features;
    BYTE features_report;
    BYTE intensity_report;
};

/* must match the order and number of usages in the
 * PID_USAGE_STATE_REPORT report */
enum effect_state_flags
{
    EFFECT_STATE_DEVICE_PAUSED = 0x01,
    EFFECT_STATE_ACTUATORS_ENABLED = 0x02,
    EFFECT_STATE_EFFECT_PLAYING = 0x04,
};

struct hid_effect_state
{
    USHORT report_len;
    BYTE *report_buf;
    BYTE id;
};

struct hid_physical
{
    USAGE effect_types[32];
    struct effect_params effect_params[256];

    BYTE device_control_report;
    BYTE device_gain_report;
    BYTE effect_control_report;
    BYTE effect_update_report;
    BYTE set_periodic_report;
    BYTE set_envelope_report;
    BYTE set_condition_report;
    BYTE set_constant_force_report;
    BYTE set_ramp_force_report;

    struct hid_effect_state effect_state;
};

struct hid_device_state
{
    ULONG bit_size;
    USAGE_AND_PAGE abs_axis_usages[32];
    USHORT abs_axis_start;
    USHORT abs_axis_count;
    USHORT rel_axis_start;
    USHORT rel_axis_count;
    USHORT hatswitch_start;
    USHORT hatswitch_count;
    USHORT button_start;
    USHORT button_count;
    USHORT report_len;
    BYTE *report_buf;
    BYTE *last_report_buf;
    BOOL dropped;
    BYTE id;
};

struct unix_device
{
    const struct raw_device_vtbl *vtbl;
    struct list entry;
    LONG ref;

    const struct hid_device_vtbl *hid_vtbl;
    struct hid_report_descriptor hid_report_descriptor;
    struct hid_device_state hid_device_state;
    struct hid_haptics hid_haptics;
    struct hid_physical hid_physical;
};

extern void *raw_device_create(const struct raw_device_vtbl *vtbl, SIZE_T size);
extern void *hid_device_create(const struct hid_device_vtbl *vtbl, SIZE_T size);

extern NTSTATUS sdl_bus_init(void *);
extern NTSTATUS sdl_bus_wait(void *);
extern NTSTATUS sdl_bus_stop(void *);

extern NTSTATUS udev_bus_init(void *);
extern NTSTATUS udev_bus_wait(void *);
extern NTSTATUS udev_bus_stop(void *);

extern NTSTATUS iohid_bus_init(void *);
extern NTSTATUS iohid_bus_wait(void *);
extern NTSTATUS iohid_bus_stop(void *);

extern void bus_event_cleanup(struct bus_event *event);
extern void bus_event_queue_destroy(struct list *queue);
extern BOOL bus_event_queue_device_removed(struct list *queue, struct unix_device *device);
extern BOOL bus_event_queue_device_created(struct list *queue, struct unix_device *device, struct device_desc *desc);
extern BOOL bus_event_queue_input_report(struct list *queue, struct unix_device *device,
                                         BYTE *report, USHORT length);
extern BOOL bus_event_queue_pop(struct list *queue, struct bus_event *event);

extern BOOL hid_device_begin_report_descriptor(struct unix_device *iface, const USAGE_AND_PAGE *device_usage);
extern BOOL hid_device_end_report_descriptor(struct unix_device *iface);

extern BOOL hid_device_begin_input_report(struct unix_device *iface, const USAGE_AND_PAGE *physical_usage);
extern BOOL hid_device_end_input_report(struct unix_device *iface);
extern BOOL hid_device_add_buttons(struct unix_device *iface, USAGE usage_page,
                                   USAGE usage_min, USAGE usage_max);
extern BOOL hid_device_add_hatswitch(struct unix_device *iface, INT count);
extern BOOL hid_device_add_axes(struct unix_device *iface, BYTE count, USAGE usage_page,
                                const USAGE *usages, BOOL rel, LONG min, LONG max);

extern BOOL hid_device_add_haptics(struct unix_device *iface);
extern BOOL hid_device_add_physical(struct unix_device *iface, USAGE *usages, USHORT count);

extern BOOL hid_device_set_abs_axis(struct unix_device *iface, ULONG index, LONG value);
extern BOOL hid_device_set_rel_axis(struct unix_device *iface, ULONG index, LONG value);
extern BOOL hid_device_set_button(struct unix_device *iface, ULONG index, BOOL is_set);
extern BOOL hid_device_set_hatswitch_x(struct unix_device *iface, ULONG index, LONG new_x);
extern BOOL hid_device_set_hatswitch_y(struct unix_device *iface, ULONG index, LONG new_y);
extern BOOL hid_device_move_hatswitch(struct unix_device *iface, ULONG index, LONG x, LONG y);

extern BOOL hid_device_sync_report(struct unix_device *iface);
extern void hid_device_drop_report(struct unix_device *iface);

extern void hid_device_set_effect_state(struct unix_device *iface, BYTE index, BYTE flags);

#endif /* __WINEBUS_UNIX_PRIVATE_H */
