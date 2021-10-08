/*  DirectInput HID Joystick device
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"

#include "ddk/hidsdi.h"
#include "setupapi.h"
#include "devguid.h"
#include "dinput.h"
#include "setupapi.h"

#include "wine/debug.h"
#include "wine/hid.h"

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

#include "initguid.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

DEFINE_GUID( GUID_DEVINTERFACE_WINEXINPUT,0x6c53d5fd,0x6480,0x440f,0xb6,0x18,0x47,0x67,0x50,0xc5,0xe1,0xa6 );
DEFINE_GUID( hid_joystick_guid, 0x9e573edb, 0x7734, 0x11d2, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7 );
DEFINE_DEVPROPKEY( DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2 );

static inline const char *debugstr_hid_value_caps( struct hid_value_caps *caps )
{
    if (!caps) return "(null)";
    return wine_dbg_sprintf( "RId %d, Usg %02x:%02x-%02x Dat %02x-%02x, Str %d-%d, Des %d-%d, "
                             "Bits %02x Flags %#x, LCol %d LUsg %02x:%02x, BitSz %d, RCnt %d, Unit %x E%+d, Log %+d-%+d, Phy %+d-%+d",
                             caps->report_id, caps->usage_page, caps->usage_min, caps->usage_max, caps->data_index_min, caps->data_index_max,
                             caps->string_min, caps->string_max, caps->designator_min, caps->designator_max, caps->bit_field, caps->flags,
                             caps->link_collection, caps->link_usage_page, caps->link_usage, caps->bit_size, caps->report_count,
                             caps->units, caps->units_exp, caps->logical_min, caps->logical_max, caps->physical_min, caps->physical_max );
}

static inline const char *debugstr_hid_collection_node( struct hid_collection_node *node )
{
    if (!node) return "(null)";
    return wine_dbg_sprintf( "Usg %02x:%02x, Parent %u, Next %u, NbChild %u, Child %u, Type %02x",
                             node->usage_page, node->usage, node->parent, node->next_sibling,
                             node->number_of_children, node->first_child, node->collection_type );
}

struct extra_caps
{
    LONG deadzone;
    LONG saturation;
};

struct pid_control_report
{
    BYTE id;
    ULONG collection;
    ULONG control_coll;
};

struct pid_effect_update
{
    BYTE id;
    ULONG collection;
    ULONG type_coll;
    ULONG axes_coll;
    ULONG axis_count;
    ULONG direction_coll;
    ULONG direction_count;
    struct hid_value_caps *axis_caps[6];
    struct hid_value_caps *direction_caps[6];
    struct hid_value_caps *duration_caps;
    struct hid_value_caps *gain_caps;
    struct hid_value_caps *sample_period_caps;
    struct hid_value_caps *start_delay_caps;
    struct hid_value_caps *trigger_button_caps;
    struct hid_value_caps *trigger_repeat_interval_caps;
};

struct pid_set_periodic
{
    BYTE id;
    ULONG collection;
    struct hid_value_caps *magnitude_caps;
    struct hid_value_caps *period_caps;
    struct hid_value_caps *phase_caps;
    struct hid_value_caps *offset_caps;
};

struct pid_set_envelope
{
    BYTE id;
    ULONG collection;
    struct hid_value_caps *attack_level_caps;
    struct hid_value_caps *attack_time_caps;
    struct hid_value_caps *fade_level_caps;
    struct hid_value_caps *fade_time_caps;
};

struct pid_set_condition
{
    BYTE id;
    ULONG collection;
    struct hid_value_caps *center_point_offset_caps;
    struct hid_value_caps *positive_coefficient_caps;
    struct hid_value_caps *negative_coefficient_caps;
    struct hid_value_caps *positive_saturation_caps;
    struct hid_value_caps *negative_saturation_caps;
    struct hid_value_caps *dead_band_caps;
};

struct pid_set_constant_force
{
    BYTE id;
    ULONG collection;
    struct hid_value_caps *magnitude_caps;
};

struct pid_set_ramp_force
{
    BYTE id;
    ULONG collection;
    struct hid_value_caps *start_caps;
    struct hid_value_caps *end_caps;
};

#define DEVICE_STATE_MAX_SIZE 1024

struct hid_joystick
{
    IDirectInputDeviceImpl base;
    LONG ref;

    HANDLE device;
    OVERLAPPED read_ovl;
    PHIDP_PREPARSED_DATA preparsed;

    DIDEVICEINSTANCEW instance;
    WCHAR device_path[MAX_PATH];
    HIDD_ATTRIBUTES attrs;
    DIDEVCAPS dev_caps;
    HIDP_CAPS caps;

    struct extra_caps *input_extra_caps;

    char *input_report_buf;
    char *output_report_buf;
    USAGE_AND_PAGE *usages_buf;
    ULONG usages_count;

    BYTE device_state_report_id;
    BYTE device_state[DEVICE_STATE_MAX_SIZE];

    BYTE effect_inuse[255];
    struct list effect_list;
    struct pid_control_report pid_device_control;
    struct pid_control_report pid_effect_control;
    struct pid_effect_update pid_effect_update;
    struct pid_set_periodic pid_set_periodic;
    struct pid_set_envelope pid_set_envelope;
    struct pid_set_condition pid_set_condition;
    struct pid_set_constant_force pid_set_constant_force;
    struct pid_set_ramp_force pid_set_ramp_force;
};

static inline struct hid_joystick *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( CONTAINING_RECORD( iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface ),
                              struct hid_joystick, base );
}

struct hid_joystick_effect
{
    IDirectInputEffect IDirectInputEffect_iface;
    LONG ref;
    USAGE type;
    ULONG index;

    struct list entry;
    struct hid_joystick *joystick;

    DWORD axes[6];
    LONG directions[6];
    DICONSTANTFORCE constant_force;
    DIRAMPFORCE ramp_force;
    DICONDITION condition[2];
    DIENVELOPE envelope;
    DIPERIODIC periodic;
    DIEFFECT params;
    BOOL modified;
    DWORD flags;

    char *effect_control_buf;
    char *effect_update_buf;
    char *type_specific_buf[2];
};

static inline struct hid_joystick_effect *impl_from_IDirectInputEffect( IDirectInputEffect *iface )
{
    return CONTAINING_RECORD( iface, struct hid_joystick_effect, IDirectInputEffect_iface );
}

static const GUID *object_usage_to_guid( USAGE usage_page, USAGE usage )
{
    switch (usage_page)
    {
    case HID_USAGE_PAGE_BUTTON: return &GUID_Button;
    case HID_USAGE_PAGE_GENERIC:
        switch (usage)
        {
        case HID_USAGE_GENERIC_X: return &GUID_XAxis;
        case HID_USAGE_GENERIC_Y: return &GUID_YAxis;
        case HID_USAGE_GENERIC_Z: return &GUID_ZAxis;
        case HID_USAGE_GENERIC_WHEEL: return &GUID_ZAxis;
        case HID_USAGE_GENERIC_RX: return &GUID_RxAxis;
        case HID_USAGE_GENERIC_RY: return &GUID_RyAxis;
        case HID_USAGE_GENERIC_RZ: return &GUID_RzAxis;
        case HID_USAGE_GENERIC_SLIDER: return &GUID_Slider;
        case HID_USAGE_GENERIC_HATSWITCH: return &GUID_POV;
        }
        break;
    }

    return &GUID_Unknown;
}

static inline USAGE effect_guid_to_usage( const GUID *guid )
{
    if (IsEqualGUID( guid, &GUID_CustomForce )) return PID_USAGE_ET_CUSTOM_FORCE_DATA;
    if (IsEqualGUID( guid, &GUID_ConstantForce )) return PID_USAGE_ET_CONSTANT_FORCE;
    if (IsEqualGUID( guid, &GUID_RampForce )) return PID_USAGE_ET_RAMP;
    if (IsEqualGUID( guid, &GUID_Square )) return PID_USAGE_ET_SQUARE;
    if (IsEqualGUID( guid, &GUID_Sine )) return PID_USAGE_ET_SINE;
    if (IsEqualGUID( guid, &GUID_Triangle )) return PID_USAGE_ET_TRIANGLE;
    if (IsEqualGUID( guid, &GUID_SawtoothUp )) return PID_USAGE_ET_SAWTOOTH_UP;
    if (IsEqualGUID( guid, &GUID_SawtoothDown )) return PID_USAGE_ET_SAWTOOTH_DOWN;
    if (IsEqualGUID( guid, &GUID_Spring )) return PID_USAGE_ET_SPRING;
    if (IsEqualGUID( guid, &GUID_Damper )) return PID_USAGE_ET_DAMPER;
    if (IsEqualGUID( guid, &GUID_Inertia )) return PID_USAGE_ET_INERTIA;
    if (IsEqualGUID( guid, &GUID_Friction )) return PID_USAGE_ET_FRICTION;
    return 0;
}

static inline const GUID *effect_usage_to_guid( USAGE usage )
{
    switch (usage)
    {
    case PID_USAGE_ET_CUSTOM_FORCE_DATA: return &GUID_CustomForce;
    case PID_USAGE_ET_CONSTANT_FORCE: return &GUID_ConstantForce;
    case PID_USAGE_ET_RAMP: return &GUID_RampForce;
    case PID_USAGE_ET_SQUARE: return &GUID_Square;
    case PID_USAGE_ET_SINE: return &GUID_Sine;
    case PID_USAGE_ET_TRIANGLE: return &GUID_Triangle;
    case PID_USAGE_ET_SAWTOOTH_UP: return &GUID_SawtoothUp;
    case PID_USAGE_ET_SAWTOOTH_DOWN: return &GUID_SawtoothDown;
    case PID_USAGE_ET_SPRING: return &GUID_Spring;
    case PID_USAGE_ET_DAMPER: return &GUID_Damper;
    case PID_USAGE_ET_INERTIA: return &GUID_Inertia;
    case PID_USAGE_ET_FRICTION: return &GUID_Friction;
    }
    return &GUID_Unknown;
}

static const WCHAR *effect_guid_to_string( const GUID *guid )
{
    static const WCHAR guid_customforce_w[] = {'G','U','I','D','_','C','u','s','t','o','m','F','o','r','c','e',0};
    static const WCHAR guid_constantforce_w[] = {'G','U','I','D','_','C','o','n','s','t','a','n','t','F','o','r','c','e',0};
    static const WCHAR guid_rampforce_w[] = {'G','U','I','D','_','R','a','m','p','F','o','r','c','e',0};
    static const WCHAR guid_square_w[] = {'G','U','I','D','_','S','q','u','a','r','e',0};
    static const WCHAR guid_sine_w[] = {'G','U','I','D','_','S','i','n','e',0};
    static const WCHAR guid_triangle_w[] = {'G','U','I','D','_','T','r','i','a','n','g','l','e',0};
    static const WCHAR guid_sawtoothup_w[] = {'G','U','I','D','_','S','a','w','t','o','o','t','h','U','p',0};
    static const WCHAR guid_sawtoothdown_w[] = {'G','U','I','D','_','S','a','w','t','o','o','t','h','D','o','w','n',0};
    static const WCHAR guid_spring_w[] = {'G','U','I','D','_','S','p','r','i','n','g',0};
    static const WCHAR guid_damper_w[] = {'G','U','I','D','_','D','a','m','p','e','r',0};
    static const WCHAR guid_inertia_w[] = {'G','U','I','D','_','I','n','e','r','t','i','a',0};
    static const WCHAR guid_friction_w[] = {'G','U','I','D','_','F','r','i','c','t','i','o','n',0};
    if (IsEqualGUID( guid, &GUID_CustomForce )) return guid_customforce_w;
    if (IsEqualGUID( guid, &GUID_ConstantForce )) return guid_constantforce_w;
    if (IsEqualGUID( guid, &GUID_RampForce )) return guid_rampforce_w;
    if (IsEqualGUID( guid, &GUID_Square )) return guid_square_w;
    if (IsEqualGUID( guid, &GUID_Sine )) return guid_sine_w;
    if (IsEqualGUID( guid, &GUID_Triangle )) return guid_triangle_w;
    if (IsEqualGUID( guid, &GUID_SawtoothUp )) return guid_sawtoothup_w;
    if (IsEqualGUID( guid, &GUID_SawtoothDown )) return guid_sawtoothdown_w;
    if (IsEqualGUID( guid, &GUID_Spring )) return guid_spring_w;
    if (IsEqualGUID( guid, &GUID_Damper )) return guid_damper_w;
    if (IsEqualGUID( guid, &GUID_Inertia )) return guid_inertia_w;
    if (IsEqualGUID( guid, &GUID_Friction )) return guid_friction_w;
    return NULL;
}

static HRESULT find_next_effect_id( struct hid_joystick *impl, ULONG *index )
{
    ULONG i;

    for (i = 0; i < ARRAY_SIZE(impl->effect_inuse); ++i)
        if (!impl->effect_inuse[i]) break;
    if (i == ARRAY_SIZE(impl->effect_inuse)) return DIERR_DEVICEFULL;
    impl->effect_inuse[i] = TRUE;
    *index = i + 1;

    return DI_OK;
}

typedef BOOL (*enum_object_callback)( struct hid_joystick *impl, struct hid_value_caps *caps,
                                      DIDEVICEOBJECTINSTANCEW *instance, void *data );

static BOOL enum_object( struct hid_joystick *impl, const DIPROPHEADER *filter, DWORD flags,
                         enum_object_callback callback, struct hid_value_caps *caps,
                         DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    if (flags != DIDFT_ALL && !(flags & DIDFT_GETTYPE( instance->dwType ))) return DIENUM_CONTINUE;

    switch (filter->dwHow)
    {
    case DIPH_DEVICE:
        return callback( impl, caps, instance, data );
    case DIPH_BYOFFSET:
        if (filter->dwObj != instance->dwOfs) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    case DIPH_BYID:
        if ((filter->dwObj & 0x00ffffff) != (instance->dwType & 0x00ffffff)) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    case DIPH_BYUSAGE:
        if (HIWORD( filter->dwObj ) != instance->wUsagePage) return DIENUM_CONTINUE;
        if (LOWORD( filter->dwObj ) != instance->wUsage) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    default:
        FIXME( "unimplemented filter dwHow %#x dwObj %#x\n", filter->dwHow, filter->dwObj );
        break;
    }

    return DIENUM_CONTINUE;
}

static void check_pid_effect_axis_caps( struct hid_joystick *impl, DIDEVICEOBJECTINSTANCEW *instance )
{
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    ULONG i;

    for (i = 0; i < effect_update->axis_count; ++i)
    {
        if (effect_update->axis_caps[i]->usage_page != instance->wUsagePage) continue;
        if (effect_update->axis_caps[i]->usage_min > instance->wUsage) continue;
        if (effect_update->axis_caps[i]->usage_max >= instance->wUsage) break;
    }

    if (i == effect_update->axis_count) return;
    instance->dwType |= DIDFT_FFACTUATOR;
    instance->dwFlags |= DIDOI_FFACTUATOR;
}

static void set_axis_type( DIDEVICEOBJECTINSTANCEW *instance, BOOL *seen, DWORD i, DWORD *count )
{
    if (!seen[i]) instance->dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( i );
    else instance->dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 6 + *count++ );
    seen[i] = TRUE;
}

static BOOL enum_objects( struct hid_joystick *impl, const DIPROPHEADER *header, DWORD flags,
                          enum_object_callback callback, void *data )
{
    DWORD collection = 0, object = 0, axis = 0, button = 0, pov = 0, value_ofs = 0, button_ofs = 0, i, j;
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    struct hid_value_caps *caps, *caps_end, *nary, *nary_end, *effect_caps;
    DIDATAFORMAT *format = impl->base.data_format.wine_df;
    int *offsets = impl->base.data_format.offsets;
    struct hid_collection_node *node, *node_end;
    DIPROPHEADER filter = *header;
    BOOL ret, seen_axis[6] = {0};

    if (filter.dwHow == DIPH_BYOFFSET)
    {
        if (!offsets) return DIENUM_CONTINUE;
        for (i = 0; i < format->dwNumObjs; ++i)
            if (offsets[i] == filter.dwObj) break;
        if (i == format->dwNumObjs) return DIENUM_CONTINUE;
        filter.dwObj = format->rgodf[i].dwOfs;
    }

    button_ofs += impl->caps.NumberInputValueCaps * sizeof(LONG);
    button_ofs += impl->caps.NumberOutputValueCaps * sizeof(LONG);
    button_ofs += impl->caps.NumberFeatureValueCaps * sizeof(LONG);

    for (caps = HID_INPUT_VALUE_CAPS( preparsed ), caps_end = caps + preparsed->input_caps_count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;
        if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
        {
            TRACE( "Ignoring input value %s, vendor specific.\n", debugstr_hid_value_caps( caps ) );
            value_ofs += (caps->usage_max - caps->usage_min + 1) * sizeof(LONG);
        }
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            instance.dwOfs = value_ofs;
            switch (MAKELONG(j, caps->usage_page))
            {
            case MAKELONG(HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_Y, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_Z, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RX, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RY, HID_USAGE_PAGE_GENERIC):
            case MAKELONG(HID_USAGE_GENERIC_RZ, HID_USAGE_PAGE_GENERIC):
                set_axis_type( &instance, seen_axis, j - HID_USAGE_GENERIC_X, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_GENERIC_WHEEL, HID_USAGE_PAGE_GENERIC):
                set_axis_type( &instance, seen_axis, 2, &axis );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            case MAKELONG(HID_USAGE_GENERIC_HATSWITCH, HID_USAGE_PAGE_GENERIC):
                instance.dwType = DIDFT_POV | DIDFT_MAKEINSTANCE( pov++ );
                instance.dwFlags = 0;
                break;
            default:
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 6 + axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                break;
            }
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            check_pid_effect_axis_caps( impl, &instance );
            ret = enum_object( impl, &filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
            value_ofs += sizeof(LONG);
            object++;
        }
    }

    effect_caps = impl->pid_effect_update.trigger_button_caps;

    for (caps = HID_INPUT_VALUE_CAPS( preparsed ), caps_end = caps + preparsed->input_caps_count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;
        if (!(caps->flags & HID_VALUE_CAPS_IS_BUTTON)) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
        {
            TRACE( "Ignoring input button %s, vendor specific.\n", debugstr_hid_value_caps( caps ) );
            button_ofs += caps->usage_max - caps->usage_min + 1;
        }
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            instance.dwOfs = button_ofs;
            instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( button++ );
            instance.dwFlags = 0;
            if (effect_caps && effect_caps->logical_min <= j && effect_caps->logical_max >= j)
            {
                instance.dwType |= DIDFT_FFEFFECTTRIGGER;
                instance.dwFlags |= DIDOI_FFEFFECTTRIGGER;
            }
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            ret = enum_object( impl, &filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
            button_ofs++;
            object++;
        }
    }

    for (caps = HID_OUTPUT_VALUE_CAPS( preparsed ), caps_end = caps + preparsed->output_caps_count;
         caps != caps_end; ++caps)
    {
        if (!caps->usage_page) continue;

        if (caps->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
        {
            TRACE( "Ignoring output caps %s, vendor specific.\n", debugstr_hid_value_caps( caps ) );
            if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) button_ofs += caps->usage_max - caps->usage_min + 1;
            else value_ofs += (caps->usage_max - caps->usage_min + 1) * sizeof(LONG);
        }
        else if (caps->flags & HID_VALUE_CAPS_ARRAY_HAS_MORE)
        {
            for (nary_end = caps - 1; caps != caps_end; caps++)
                if (!(caps->flags & HID_VALUE_CAPS_ARRAY_HAS_MORE)) break;

            for (nary = caps; nary != nary_end; nary--)
            {
                instance.dwOfs = button_ofs;
                instance.dwType = DIDFT_NODATA | DIDFT_MAKEINSTANCE( object++ ) | DIDFT_OUTPUT;
                instance.dwFlags = 0x80008000;
                instance.wUsagePage = nary->usage_page;
                instance.wUsage = nary->usage_min;
                instance.guidType = GUID_Unknown;
                instance.wReportId = nary->report_id;
                instance.wCollectionNumber = nary->link_collection;
                instance.dwDimension = caps->units;
                instance.wExponent = caps->units_exp;
                ret = enum_object( impl, &filter, flags, callback, nary, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                button_ofs++;
            }
        }
        else for (j = caps->usage_min; j <= caps->usage_max; ++j)
        {
            if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) instance.dwOfs = button_ofs;
            else instance.dwOfs = value_ofs;

            instance.dwType = DIDFT_NODATA | DIDFT_MAKEINSTANCE( object++ ) | DIDFT_OUTPUT;
            instance.dwFlags = 0x80008000;
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = GUID_Unknown;
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
            instance.dwDimension = caps->units;
            instance.wExponent = caps->units_exp;
            ret = enum_object( impl, &filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;

            if (caps->flags & HID_VALUE_CAPS_IS_BUTTON) button_ofs++;
            else value_ofs += sizeof(LONG);
        }
    }

    for (node = HID_COLLECTION_NODES( preparsed ), node_end = node + preparsed->number_link_collection_nodes;
         node != node_end; ++node)
    {
        if (!node->usage_page) continue;
        if (node->usage_page >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            TRACE( "Ignoring collection %s, vendor specific.\n", debugstr_hid_collection_node( node ) );
        else
        {
            instance.dwOfs = 0;
            instance.dwType = DIDFT_COLLECTION | DIDFT_MAKEINSTANCE( collection++ ) | DIDFT_NODATA;
            instance.dwFlags = 0;
            instance.wUsagePage = node->usage_page;
            instance.wUsage = node->usage;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = 0;
            instance.wCollectionNumber = node->parent;
            instance.dwDimension = 0;
            instance.wExponent = 0;
            ret = enum_object( impl, &filter, flags, callback, NULL, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
        }
    }

    return DIENUM_CONTINUE;
}

static ULONG hid_joystick_private_incref( struct hid_joystick *impl )
{
    return IDirectInputDevice2WImpl_AddRef( &impl->base.IDirectInputDevice8W_iface );
}

static ULONG hid_joystick_private_decref( struct hid_joystick *impl )
{
    struct hid_joystick tmp = *impl;
    ULONG ref;

    if (!(ref = IDirectInputDevice2WImpl_Release( &impl->base.IDirectInputDevice8W_iface )))
    {
        HeapFree( GetProcessHeap(), 0, tmp.usages_buf );
        HeapFree( GetProcessHeap(), 0, tmp.output_report_buf );
        HeapFree( GetProcessHeap(), 0, tmp.input_report_buf );
        HeapFree( GetProcessHeap(), 0, tmp.input_extra_caps );
        HidD_FreePreparsedData( tmp.preparsed );
        CloseHandle( tmp.base.read_event );
        CloseHandle( tmp.device );
    }

    return ref;
}

static ULONG WINAPI hid_joystick_AddRef( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %u.\n", iface, ref );
    return ref;
}

static ULONG WINAPI hid_joystick_Release( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %u.\n", iface, ref );
    if (!ref) hid_joystick_private_decref( impl );
    return ref;
}

static HRESULT WINAPI hid_joystick_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, caps %p.\n", iface, caps );

    if (!caps) return E_POINTER;

    *caps = impl->dev_caps;

    return DI_OK;
}

struct enum_objects_params
{
    LPDIENUMDEVICEOBJECTSCALLBACKW callback;
    void *context;
};

static BOOL enum_objects_callback( struct hid_joystick *impl, struct hid_value_caps *caps,
                                   DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct enum_objects_params *params = data;
    return params->callback( instance, params->context );
}

static HRESULT WINAPI hid_joystick_EnumObjects( IDirectInputDevice8W *iface, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                                                void *context, DWORD flags )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct enum_objects_params params =
    {
        .callback = callback,
        .context = context,
    };
    BOOL ret;

    TRACE( "iface %p, callback %p, context %p, flags %#x.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags & ~(DIDFT_AXIS | DIDFT_POV | DIDFT_BUTTON | DIDFT_NODATA | DIDFT_COLLECTION))
        return DIERR_INVALIDPARAM;

    if (flags == DIDFT_ALL || (flags & DIDFT_AXIS))
    {
        ret = enum_objects( impl, &filter, DIDFT_AXIS, enum_objects_callback, &params );
        if (ret != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_POV))
    {
        ret = enum_objects( impl, &filter, DIDFT_POV, enum_objects_callback, &params );
        if (ret != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_BUTTON))
    {
        ret = enum_objects( impl, &filter, DIDFT_BUTTON, enum_objects_callback, &params );
        if (ret != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & (DIDFT_NODATA | DIDFT_COLLECTION)))
    {
        ret = enum_objects( impl, &filter, DIDFT_NODATA, enum_objects_callback, &params );
        if (ret != DIENUM_CONTINUE) return DI_OK;
    }

    return DI_OK;
}

static BOOL get_property_prop_range( struct hid_joystick *impl, struct hid_value_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIPROPRANGE *value = data;
    value->lMin = caps->physical_min;
    value->lMax = caps->physical_max;
    return DIENUM_STOP;
}

static BOOL get_property_prop_deadzone( struct hid_joystick *impl, struct hid_value_caps *caps,
                                        DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    struct extra_caps *extra;
    DIPROPDWORD *deadzone = data;
    extra = impl->input_extra_caps + (caps - HID_INPUT_VALUE_CAPS( preparsed ));
    deadzone->dwData = extra->deadzone;
    return DIENUM_STOP;
}

static BOOL get_property_prop_saturation( struct hid_joystick *impl, struct hid_value_caps *caps,
                                          DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    struct extra_caps *extra;
    DIPROPDWORD *saturation = data;
    extra = impl->input_extra_caps + (caps - HID_INPUT_VALUE_CAPS( preparsed ));
    saturation->dwData = extra->saturation;
    return DIENUM_STOP;
}

static BOOL get_property_prop_granularity( struct hid_joystick *impl, struct hid_value_caps *caps,
                                           DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIPROPDWORD *granularity = data;
    granularity->dwData = 1;
    return DIENUM_STOP;
}

static HRESULT WINAPI hid_joystick_GetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                DIPROPHEADER *header )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (enum_objects( impl, header, DIDFT_AXIS, get_property_prop_range, header ) == DIENUM_STOP)
            return DI_OK;
        return DIERR_NOTFOUND;
    case (DWORD_PTR)DIPROP_DEADZONE:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (enum_objects( impl, header, DIDFT_AXIS, get_property_prop_deadzone, header ) == DIENUM_STOP)
            return DI_OK;
        return DIERR_NOTFOUND;
    case (DWORD_PTR)DIPROP_SATURATION:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (enum_objects( impl, header, DIDFT_AXIS, get_property_prop_saturation, header ) == DIENUM_STOP)
            return DI_OK;
        return DIERR_NOTFOUND;
    case (DWORD_PTR)DIPROP_GRANULARITY:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (enum_objects( impl, header, DIDFT_AXIS, get_property_prop_granularity, header ) == DIENUM_STOP)
            return DI_OK;
        return DIERR_NOTFOUND;
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        lstrcpynW( value->wsz, impl->instance.tszProductName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        lstrcpynW( value->wsz, impl->instance.tszInstanceName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_VIDPID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (!impl->attrs.VendorID || !impl->attrs.ProductID) return DIERR_UNSUPPORTED;
        value->dwData = MAKELONG( impl->attrs.VendorID, impl->attrs.ProductID );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        value->dwData = impl->instance.guidInstance.Data3;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    {
        DIPROPGUIDANDPATH *value = (DIPROPGUIDANDPATH *)header;
        if (header->dwSize != sizeof(DIPROPGUIDANDPATH)) return DIERR_INVALIDPARAM;
        lstrcpynW( value->wszPath, impl->device_path, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        return DIERR_UNSUPPORTED;
    default:
        return IDirectInputDevice2WImpl_GetProperty( iface, guid, header );
    }

    return DI_OK;
}

static BOOL set_property_prop_range( struct hid_joystick *impl, struct hid_value_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIPROPRANGE *value = data;
    LONG tmp;

    caps->physical_min = value->lMin;
    caps->physical_max = value->lMax;

    if (instance->dwType & DIDFT_AXIS)
    {
        if (!caps->physical_min) tmp = caps->physical_max / 2;
        else tmp = round( (caps->physical_min + caps->physical_max) / 2.0 );
        *(LONG *)(impl->device_state + instance->dwOfs) = tmp;
    }
    else if (instance->dwType & DIDFT_POV)
    {
        tmp = caps->logical_max - caps->logical_min;
        if (tmp > 0) caps->physical_max -= value->lMax / (tmp + 1);
        *(LONG *)(impl->device_state + instance->dwOfs) = -1;
    }
    return DIENUM_CONTINUE;
}

static BOOL set_property_prop_deadzone( struct hid_joystick *impl, struct hid_value_caps *caps,
                                        DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    struct extra_caps *extra;
    DIPROPDWORD *deadzone = data;
    extra = impl->input_extra_caps + (caps - HID_INPUT_VALUE_CAPS( preparsed ));
    extra->deadzone = deadzone->dwData;
    return DIENUM_CONTINUE;
}

static BOOL set_property_prop_saturation( struct hid_joystick *impl, struct hid_value_caps *caps,
                                          DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    struct extra_caps *extra;
    DIPROPDWORD *saturation = data;
    extra = impl->input_extra_caps + (caps - HID_INPUT_VALUE_CAPS( preparsed ));
    extra->saturation = saturation->dwData;
    return DIENUM_CONTINUE;
}

static HRESULT WINAPI hid_joystick_SetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                const DIPROPHEADER *header )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)header;
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        if (value->lMin > value->lMax) return DIERR_INVALIDPARAM;
        enum_objects( impl, header, DIDFT_AXIS, set_property_prop_range, (void *)header );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (value->dwData > 10000) return DIERR_INVALIDPARAM;
        enum_objects( impl, header, DIDFT_AXIS, set_property_prop_deadzone, (void *)header );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (value->dwData > 10000) return DIERR_INVALIDPARAM;
        enum_objects( impl, header, DIDFT_AXIS, set_property_prop_saturation, (void *)header );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        EnterCriticalSection( &impl->base.crit );
        if (impl->base.acquired) hr = DIERR_ACQUIRED;
        else if (value->dwData > DIPROPAUTOCENTER_ON) hr = DIERR_INVALIDPARAM;
        else hr = DIERR_UNSUPPORTED;
        LeaveCriticalSection( &impl->base.crit );
        return hr;
    }
    case (DWORD_PTR)DIPROP_FFLOAD:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_VIDPID:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        return DIERR_READONLY;
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        return DIERR_READONLY;
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
        if (header->dwSize != sizeof(DIPROPGUIDANDPATH)) return DIERR_INVALIDPARAM;
        return DIERR_READONLY;
    default:
        return IDirectInputDevice2WImpl_SetProperty( iface, guid, header );
    }

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_Acquire( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG report_len = impl->caps.InputReportByteLength;
    HRESULT hr = DI_OK;
    BOOL ret;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->base.crit );
    if (impl->base.acquired)
        hr = DI_NOEFFECT;
    else if (!impl->base.data_format.user_df)
        hr = DIERR_INVALIDPARAM;
    else if ((impl->base.dwCoopLevel & DISCL_FOREGROUND) && impl->base.win != GetForegroundWindow())
        hr = DIERR_OTHERAPPHASPRIO;
    else if (impl->device == INVALID_HANDLE_VALUE)
    {
        impl->device = CreateFileW( impl->device_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, 0 );
        if (impl->device == INVALID_HANDLE_VALUE) hr = DIERR_INPUTLOST;
    }

    if (hr == DI_OK)
    {
        memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
        impl->read_ovl.hEvent = impl->base.read_event;
        ret = ReadFile( impl->device, impl->input_report_buf, report_len, NULL, &impl->read_ovl );
        if (!ret && GetLastError() != ERROR_IO_PENDING)
        {
            CloseHandle( impl->device );
            impl->device = INVALID_HANDLE_VALUE;
            hr = DIERR_INPUTLOST;
        }
        else
        {
            impl->base.acquired = TRUE;
            IDirectInputDevice8_SendForceFeedbackCommand( iface, DISFFC_RESET );
        }
    }
    LeaveCriticalSection( &impl->base.crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_acquire_device( iface );
    check_dinput_hooks( iface, TRUE );

    return hr;
}

static HRESULT WINAPI hid_joystick_Unacquire( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;
    BOOL ret;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->base.crit );
    if (!impl->base.acquired) hr = DI_NOEFFECT;
    else
    {
        if (impl->device != INVALID_HANDLE_VALUE)
        {
            ret = CancelIoEx( impl->device, &impl->read_ovl );
            if (!ret) WARN( "CancelIoEx failed, last error %u\n", GetLastError() );
            else WaitForSingleObject( impl->base.read_event, INFINITE );
        }
        IDirectInputDevice8_SendForceFeedbackCommand( iface, DISFFC_RESET );
        impl->base.acquired = FALSE;
    }
    LeaveCriticalSection( &impl->base.crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_unacquire_device( iface );
    check_dinput_hooks( iface, FALSE );

    return hr;
}

static HRESULT WINAPI hid_joystick_GetDeviceState( IDirectInputDevice8W *iface, DWORD len, void *ptr )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    if (!ptr) return DIERR_INVALIDPARAM;
    if (len != impl->base.data_format.user_df->dwDataSize) return DIERR_INVALIDPARAM;

    EnterCriticalSection( &impl->base.crit );
    if (!impl->base.acquired) hr = DIERR_NOTACQUIRED;
    else fill_DataFormat( ptr, len, impl->device_state, &impl->base.data_format );
    LeaveCriticalSection( &impl->base.crit );

    return hr;
}

static BOOL get_object_info( struct hid_joystick *impl, struct hid_value_caps *caps,
                             DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDEVICEOBJECTINSTANCEW *dest = data;
    memcpy( dest, instance, dest->dwSize );
    return DIENUM_STOP;
}

static HRESULT WINAPI hid_joystick_GetObjectInfo( IDirectInputDevice8W *iface, DIDEVICEOBJECTINSTANCEW *instance,
                                                  DWORD obj, DWORD how )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = how,
        .dwObj = obj
    };
    BOOL ret;

    TRACE( "iface %p, instance %p, obj %#x, how %#x.\n", iface, instance, obj, how );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
        return DIERR_INVALIDPARAM;
    if (how == DIPH_DEVICE) return DIERR_INVALIDPARAM;

    ret = enum_objects( impl, &filter, DIDFT_ALL, get_object_info, instance );
    if (ret != DIENUM_CONTINUE) return DI_OK;
    return DIERR_NOTFOUND;
}

static HRESULT WINAPI hid_joystick_GetDeviceInfo( IDirectInputDevice8W *iface, DIDEVICEINSTANCEW *instance )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return DIERR_INVALIDPARAM;

    memcpy( instance, &impl->instance, instance->dwSize );

    return S_OK;
}

static HRESULT hid_joystick_effect_create( struct hid_joystick *joystick, IDirectInputEffect **out );

static HRESULT WINAPI hid_joystick_CreateEffect( IDirectInputDevice8W *iface, const GUID *guid,
                                                 const DIEFFECT *params, IDirectInputEffect **out,
                                                 IUnknown *outer )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD flags = DIEP_ALLPARAMS;
    HRESULT hr;

    TRACE( "iface %p, guid %s, params %p, out %p, outer %p\n", iface, debugstr_guid( guid ),
           params, out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!(impl->dev_caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (FAILED(hr = hid_joystick_effect_create( impl, out ))) return hr;

    hr = IDirectInputEffect_Initialize( *out, DINPUT_instance, impl->base.dinput->dwVersion, guid );
    if (FAILED(hr)) goto failed;

    if (!params) return DI_OK;
    if (!impl->base.acquired || !(impl->base.dwCoopLevel & DISCL_EXCLUSIVE))
        flags |= DIEP_NODOWNLOAD;
    hr = IDirectInputEffect_SetParameters( *out, params, flags );
    if (FAILED(hr)) goto failed;
    return hr;

failed:
    IDirectInputEffect_Release( *out );
    *out = NULL;
    return hr;
}

static HRESULT WINAPI hid_joystick_EnumEffects( IDirectInputDevice8W *iface, LPDIENUMEFFECTSCALLBACKW callback,
                                                void *context, DWORD type )
{
    DIEFFECTINFOW info = {.dwSize = sizeof(info)};
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, type %#x.\n", iface, callback, context, type );

    if (!callback) return DIERR_INVALIDPARAM;

    type = DIEFT_GETTYPE( type );

    if (type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_ConstantForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_RampForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_PERIODIC)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Square );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Sine );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Triangle );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothUp );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothDown );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Spring );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Damper );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Inertia );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Friction );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_GetEffectInfo( IDirectInputDevice8W *iface, DIEFFECTINFOW *info,
                                                  const GUID *guid )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;
    PHIDP_PREPARSED_DATA preparsed = impl->preparsed;
    HIDP_BUTTON_CAPS button;
    ULONG type, collection;
    NTSTATUS status;
    USAGE usage = 0;
    USHORT count;

    TRACE( "iface %p, info %p, guid %s.\n", iface, info, debugstr_guid( guid ) );

    if (!info) return E_POINTER;
    if (info->dwSize != sizeof(DIEFFECTINFOW)) return DIERR_INVALIDPARAM;
    if (!(impl->dev_caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_DEVICENOTREG;

    switch ((usage = effect_guid_to_usage( guid )))
    {
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        type = DIEFT_PERIODIC;
        break;
    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        type = DIEFT_CONDITION;
        break;
    case PID_USAGE_ET_CONSTANT_FORCE:
        type = DIEFT_CONSTANTFORCE;
        break;
    case PID_USAGE_ET_RAMP:
        type = DIEFT_RAMPFORCE;
        break;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        type = DIEFT_CUSTOMFORCE;
        break;
    default:
        return DIERR_DEVICENOTREG;
    }

    if (!(collection = effect_update->collection)) return DIERR_DEVICENOTREG;

    info->dwDynamicParams = DIEP_TYPESPECIFICPARAMS;
    if (effect_update->axis_count) info->dwDynamicParams |= DIEP_AXES;
    if (effect_update->duration_caps) info->dwDynamicParams |= DIEP_DURATION;
    if (effect_update->gain_caps) info->dwDynamicParams |= DIEP_GAIN;
    if (effect_update->sample_period_caps) info->dwDynamicParams |= DIEP_SAMPLEPERIOD;
    if (effect_update->start_delay_caps)
    {
        type |= DIEFT_STARTDELAY;
        info->dwDynamicParams |= DIEP_STARTDELAY;
    }
    if (effect_update->trigger_button_caps) info->dwDynamicParams |= DIEP_TRIGGERBUTTON;
    if (effect_update->trigger_repeat_interval_caps) info->dwDynamicParams |= DIEP_TRIGGERREPEATINTERVAL;
    if (effect_update->direction_coll) info->dwDynamicParams |= DIEP_DIRECTION;
    if (effect_update->axes_coll) info->dwDynamicParams |= DIEP_AXES;

    if (!(collection = effect_update->type_coll)) return DIERR_DEVICENOTREG;
    else
    {
        count = 1;
        status = HidP_GetSpecificButtonCaps( HidP_Output, HID_USAGE_PAGE_PID, collection,
                                             usage, &button, &count, preparsed );
        if (status != HIDP_STATUS_SUCCESS)
        {
            WARN( "HidP_GetSpecificValueCaps %#x returned %#x\n", usage, status );
            return DIERR_DEVICENOTREG;
        }
        else if (!count)
        {
            WARN( "effect usage %#x not found\n", usage );
            return DIERR_DEVICENOTREG;
        }
    }

    if ((type == DIEFT_PERIODIC) && (collection = set_periodic->collection))
    {
        if (set_periodic->magnitude_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->offset_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->period_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_periodic->phase_caps) info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
    }

    if ((type == DIEFT_PERIODIC || type == DIEFT_RAMPFORCE || type == DIEFT_CONSTANTFORCE) &&
        (collection = set_envelope->collection))
    {
        info->dwDynamicParams |= DIEP_ENVELOPE;
        if (set_envelope->attack_level_caps) type |= DIEFT_FFATTACK;
        if (set_envelope->attack_time_caps) type |= DIEFT_FFATTACK;
        if (set_envelope->fade_level_caps) type |= DIEFT_FFFADE;
        if (set_envelope->fade_time_caps) type |= DIEFT_FFFADE;
    }

    if ((collection = set_condition->collection) && (type == DIEFT_CONDITION))
    {
        if (set_condition->center_point_offset_caps)
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
        if (set_condition->positive_coefficient_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_POSNEGCOEFFICIENTS;
        }
        if (set_condition->negative_coefficient_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_POSNEGCOEFFICIENTS;
        }
        if (set_condition->positive_saturation_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_SATURATION | DIEFT_POSNEGSATURATION;
        }
        if (set_condition->negative_saturation_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_SATURATION | DIEFT_POSNEGSATURATION;
        }
        if (set_condition->dead_band_caps)
        {
            info->dwDynamicParams |= DIEP_TYPESPECIFICPARAMS;
            type |= DIEFT_DEADBAND;
        }
    }

    info->guid = *guid;
    info->dwEffType = type;
    info->dwStaticParams = info->dwDynamicParams;
    lstrcpynW( info->tszName, effect_guid_to_string( guid ), MAX_PATH );

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_GetForceFeedbackState( IDirectInputDevice8W *iface, DWORD *out )
{
    FIXME( "iface %p, out %p stub!\n", iface, out );

    if (!out) return E_POINTER;

    return DIERR_UNSUPPORTED;
}

static BOOL CALLBACK unload_effect_object( IDirectInputEffect *effect, void *context )
{
    IDirectInputEffect_Unload( effect );
    return DIENUM_CONTINUE;
}

static HRESULT WINAPI hid_joystick_SendForceFeedbackCommand( IDirectInputDevice8W *iface, DWORD command )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct pid_control_report *report = &impl->pid_device_control;
    ULONG report_len = impl->caps.OutputReportByteLength;
    char *report_buf = impl->output_report_buf;
    NTSTATUS status;
    USAGE usage;
    ULONG count;
    HRESULT hr;

    TRACE( "iface %p, flags %x.\n", iface, command );

    switch (command)
    {
    case DISFFC_RESET: usage = PID_USAGE_DC_DEVICE_RESET; break;
    case DISFFC_STOPALL: usage = PID_USAGE_DC_STOP_ALL_EFFECTS; break;
    case DISFFC_PAUSE: usage = PID_USAGE_DC_DEVICE_PAUSE; break;
    case DISFFC_CONTINUE: usage = PID_USAGE_DC_DEVICE_CONTINUE; break;
    case DISFFC_SETACTUATORSON: usage = PID_USAGE_DC_ENABLE_ACTUATORS; break;
    case DISFFC_SETACTUATORSOFF: usage = PID_USAGE_DC_DISABLE_ACTUATORS; break;
    default: return DIERR_INVALIDPARAM;
    }

    if (!(impl->dev_caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->base.crit );
    if (!impl->base.acquired || !(impl->base.dwCoopLevel & DISCL_EXCLUSIVE))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else
    {
        if (command == DISFFC_RESET) IDirectInputDevice8_EnumCreatedEffectObjects( iface, unload_effect_object, NULL, 0 );

        count = 1;
        status = HidP_InitializeReportForID( HidP_Output, report->id, impl->preparsed, report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, report->control_coll, &usage,
                                      &count, impl->preparsed, report_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else if (WriteFile( impl->device, report_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_GENERIC;
    }
    LeaveCriticalSection( &impl->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_EnumCreatedEffectObjects( IDirectInputDevice8W *iface,
                                                             LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                             void *context, DWORD flags )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct hid_joystick_effect *effect, *next;

    TRACE( "iface %p, callback %p, context %p, flags %#x.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags) return DIERR_INVALIDPARAM;

    LIST_FOR_EACH_ENTRY_SAFE(effect, next, &impl->effect_list, struct hid_joystick_effect, entry)
        if (callback( &effect->IDirectInputEffect_iface, context ) != DIENUM_CONTINUE) break;

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_Poll( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_NOEFFECT;

    EnterCriticalSection( &impl->base.crit );
    if (!impl->base.acquired) hr = DIERR_NOTACQUIRED;
    LeaveCriticalSection( &impl->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_BuildActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                   const WCHAR *username, DWORD flags )
{
    FIXME( "iface %p, format %p, username %s, flags %#x stub!\n", iface, format, debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    return DIERR_UNSUPPORTED;
}

static HRESULT WINAPI hid_joystick_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                 const WCHAR *username, DWORD flags )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, format %p, username %s, flags %#x.\n", iface, format, debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    return _set_action_map( iface, format, username, flags, impl->base.data_format.wine_df );
}

static const IDirectInputDevice8WVtbl hid_joystick_vtbl =
{
    /*** IUnknown methods ***/
    IDirectInputDevice2WImpl_QueryInterface,
    hid_joystick_AddRef,
    hid_joystick_Release,
    /*** IDirectInputDevice methods ***/
    hid_joystick_GetCapabilities,
    hid_joystick_EnumObjects,
    hid_joystick_GetProperty,
    hid_joystick_SetProperty,
    hid_joystick_Acquire,
    hid_joystick_Unacquire,
    hid_joystick_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    hid_joystick_GetObjectInfo,
    hid_joystick_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    /*** IDirectInputDevice2 methods ***/
    hid_joystick_CreateEffect,
    hid_joystick_EnumEffects,
    hid_joystick_GetEffectInfo,
    hid_joystick_GetForceFeedbackState,
    hid_joystick_SendForceFeedbackCommand,
    hid_joystick_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    hid_joystick_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    /*** IDirectInputDevice7 methods ***/
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    /*** IDirectInputDevice8 methods ***/
    hid_joystick_BuildActionMap,
    hid_joystick_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo,
};

struct parse_device_state_params
{
    BYTE old_state[DEVICE_STATE_MAX_SIZE];
    BYTE buttons[128];
    DWORD time;
    DWORD seq;
};

static BOOL check_device_state_button( struct hid_joystick *impl, struct hid_value_caps *caps,
                                       DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    struct parse_device_state_params *params = data;
    BYTE old_value, value;

    if (instance->wReportId != impl->device_state_report_id) return DIENUM_CONTINUE;

    value = params->buttons[instance->wUsage - 1];
    old_value = params->old_state[instance->dwOfs];
    impl->device_state[instance->dwOfs] = value;
    if (old_value != value)
        queue_event( iface, instance->dwType, value, params->time, params->seq );

    return DIENUM_CONTINUE;
}

static LONG sign_extend( ULONG value, struct hid_value_caps *caps )
{
    UINT sign = 1 << (caps->bit_size - 1);
    if (sign <= 1 || caps->logical_min >= 0) return value;
    return value - ((value & sign) << 1);
}

static LONG scale_value( ULONG value, struct hid_value_caps *caps, LONG min, LONG max )
{
    LONG tmp = sign_extend( value, caps );
    if (caps->logical_min > tmp || caps->logical_max < tmp) return -1; /* invalid / null value */
    return min + MulDiv( tmp - caps->logical_min, max - min, caps->logical_max - caps->logical_min );
}

static LONG scale_axis_value( ULONG value, struct hid_value_caps *caps, struct extra_caps *extra )
{
    LONG tmp = sign_extend( value, caps ), log_ctr, log_min, log_max, phy_ctr, phy_min, phy_max;
    ULONG bit_max = (1 << caps->bit_size) - 1;

    log_min = caps->logical_min;
    log_max = caps->logical_max;
    phy_min = caps->physical_min;
    phy_max = caps->physical_max;
    /* xinput HID gamepad have bogus logical value range, let's use the bit range instead */
    if (log_min == 0 && log_max == -1) log_max = bit_max;

    if (phy_min == 0) phy_ctr = phy_max >> 1;
    else phy_ctr = round( (phy_min + phy_max) / 2.0 );
    if (log_min == 0) log_ctr = log_max >> 1;
    else log_ctr = round( (log_min + log_max) / 2.0 );

    tmp -= log_ctr;
    if (tmp <= 0)
    {
        log_max = MulDiv( log_min - log_ctr, extra->deadzone, 10000 );
        log_min = MulDiv( log_min - log_ctr, extra->saturation, 10000 );
        phy_max = phy_ctr;
    }
    else
    {
        log_min = MulDiv( log_max - log_ctr, extra->deadzone, 10000 );
        log_max = MulDiv( log_max - log_ctr, extra->saturation, 10000 );
        phy_min = phy_ctr;
    }

    if (tmp <= log_min) return phy_min;
    if (tmp >= log_max) return phy_max;
    return phy_min + MulDiv( tmp - log_min, phy_max - phy_min, log_max - log_min );
}

static BOOL read_device_state_value( struct hid_joystick *impl, struct hid_value_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct hid_preparsed_data *preparsed = (struct hid_preparsed_data *)impl->preparsed;
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    ULONG logical_value, report_len = impl->caps.InputReportByteLength;
    struct parse_device_state_params *params = data;
    char *report_buf = impl->input_report_buf;
    struct extra_caps *extra;
    LONG old_value, value;
    NTSTATUS status;

    if (instance->wReportId != impl->device_state_report_id) return DIENUM_CONTINUE;

    extra = impl->input_extra_caps + (caps - HID_INPUT_VALUE_CAPS( preparsed ));
    status = HidP_GetUsageValue( HidP_Input, instance->wUsagePage, 0, instance->wUsage,
                                 &logical_value, impl->preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsageValue %04x:%04x returned %#x\n",
                                             instance->wUsagePage, instance->wUsage, status );
    if (instance->dwType & DIDFT_AXIS) value = scale_axis_value( logical_value, caps, extra );
    else value = scale_value( logical_value, caps, caps->physical_min, caps->physical_max );

    old_value = *(LONG *)(params->old_state + instance->dwOfs);
    *(LONG *)(impl->device_state + instance->dwOfs) = value;
    if (old_value != value)
        queue_event( iface, instance->dwType, value, params->time, params->seq );

    return DIENUM_CONTINUE;
}

static HRESULT hid_joystick_read_state( IDirectInputDevice8W *iface )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG i, count, report_len = impl->caps.InputReportByteLength;
    DIDATAFORMAT *format = impl->base.data_format.wine_df;
    struct parse_device_state_params params = {{0}};
    char *report_buf = impl->input_report_buf;
    USAGE_AND_PAGE *usages;
    NTSTATUS status;
    HRESULT hr;
    BOOL ret;

    ret = GetOverlappedResult( impl->device, &impl->read_ovl, &count, FALSE );
    if (ret && TRACE_ON(dinput))
    {
        TRACE( "read size %u report:\n", count );
        for (i = 0; i < report_len;)
        {
            char buffer[256], *buf = buffer;
            buf += sprintf(buf, "%08x ", i);
            do
            {
                buf += sprintf(buf, " %02x", (BYTE)report_buf[i] );
            } while (++i % 16 && i < report_len);
            TRACE("%s\n", buffer);
        }
    }

    EnterCriticalSection( &impl->base.crit );
    while (ret)
    {
        count = impl->usages_count;
        memset( impl->usages_buf, 0, count * sizeof(*impl->usages_buf) );
        status = HidP_GetUsagesEx( HidP_Input, 0, impl->usages_buf, &count,
                                   impl->preparsed, report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsagesEx returned %#x\n", status );

        if (report_buf[0] == impl->device_state_report_id)
        {
            params.time = GetCurrentTime();
            params.seq = impl->base.dinput->evsequence++;
            memcpy( params.old_state, impl->device_state, format->dwDataSize );
            memset( impl->device_state, 0, format->dwDataSize );

            while (count--)
            {
                usages = impl->usages_buf + count;
                if (usages->UsagePage != HID_USAGE_PAGE_BUTTON)
                    FIXME( "unimplemented usage page %x.\n", usages->UsagePage );
                else if (usages->Usage >= 128)
                    FIXME( "ignoring extraneous button %d.\n", usages->Usage );
                else
                    params.buttons[usages->Usage - 1] = 0x80;
            }

            enum_objects( impl, &filter, DIDFT_AXIS | DIDFT_POV, read_device_state_value, &params );
            enum_objects( impl, &filter, DIDFT_BUTTON, check_device_state_button, &params );
            if (impl->base.hEvent && memcmp( &params.old_state, impl->device_state, format->dwDataSize ))
                SetEvent( impl->base.hEvent );
        }

        memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
        impl->read_ovl.hEvent = impl->base.read_event;
        ret = ReadFile( impl->device, report_buf, report_len, &count, &impl->read_ovl );
    }

    if (GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_OPERATION_ABORTED) hr = DI_OK;
    else
    {
        WARN( "GetOverlappedResult/ReadFile failed, error %u\n", GetLastError() );
        CloseHandle(impl->device);
        impl->device = INVALID_HANDLE_VALUE;
        impl->base.acquired = FALSE;
        hr = DIERR_INPUTLOST;
    }
    LeaveCriticalSection( &impl->base.crit );

    return hr;
}

static DWORD device_type_for_version( DWORD type, DWORD version )
{
    if (version >= 0x0800) return type;

    switch (GET_DIDEVICE_TYPE( type ))
    {
    case DI8DEVTYPE_JOYSTICK:
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPEJOYSTICK_LIMITED)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_TRADITIONAL << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_GAMEPAD:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_GAMEPAD << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_DRIVING:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_WHEEL << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_FLIGHT:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_FLIGHTSTICK << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_SUPPLEMENTAL:
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPESUPPLEMENTAL_HEADTRACKER)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_HEADTRACKER << 8) | DIDEVTYPE_HID;
        if (GET_DIDEVICE_SUBTYPE( type ) == DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS)
            return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_RUDDER << 8) | DIDEVTYPE_HID;
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;

    case DI8DEVTYPE_1STPERSON:
        return DIDEVTYPE_JOYSTICK | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_HID;

    default:
        return DIDEVTYPE_DEVICE | DIDEVTYPE_HID;
    }
}

static BOOL hid_joystick_device_try_open( UINT32 handle, const WCHAR *path, HANDLE *device,
                                          PHIDP_PREPARSED_DATA *preparsed, HIDD_ATTRIBUTES *attrs,
                                          HIDP_CAPS *caps, DIDEVICEINSTANCEW *instance, DWORD version )
{
    PHIDP_PREPARSED_DATA preparsed_data = NULL;
    DWORD type = 0, button_count = 0;
    HIDP_BUTTON_CAPS buttons[10];
    HIDP_VALUE_CAPS value;
    HANDLE device_file;
    NTSTATUS status;
    USHORT count;

    device_file = CreateFileW( path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, 0 );
    if (device_file == INVALID_HANDLE_VALUE) return FALSE;

    if (!HidD_GetPreparsedData( device_file, &preparsed_data )) goto failed;
    if (!HidD_GetAttributes( device_file, attrs )) goto failed;
    if (HidP_GetCaps( preparsed_data, caps ) != HIDP_STATUS_SUCCESS) goto failed;

    if (caps->UsagePage == HID_USAGE_PAGE_GAME) FIXME( "game usage page not implemented!\n" );
    if (caps->UsagePage == HID_USAGE_PAGE_SIMULATION) FIXME( "simulation usage page not implemented!\n" );
    if (caps->UsagePage != HID_USAGE_PAGE_GENERIC) goto failed;
    if (caps->Usage != HID_USAGE_GENERIC_GAMEPAD && caps->Usage != HID_USAGE_GENERIC_JOYSTICK) goto failed;

    if (!HidD_GetProductString( device_file, instance->tszInstanceName, MAX_PATH )) goto failed;
    if (!HidD_GetProductString( device_file, instance->tszProductName, MAX_PATH )) goto failed;

    instance->guidInstance = hid_joystick_guid;
    instance->guidInstance.Data1 ^= handle;
    instance->guidProduct = DInput_PIDVID_Product_GUID;
    instance->guidProduct.Data1 = MAKELONG( attrs->VendorID, attrs->ProductID );
    instance->guidFFDriver = GUID_NULL;
    instance->wUsagePage = caps->UsagePage;
    instance->wUsage = caps->Usage;

    count = ARRAY_SIZE(buttons);
    status = HidP_GetSpecificButtonCaps( HidP_Output, HID_USAGE_PAGE_PID, 0,
                                         PID_USAGE_DC_DEVICE_RESET, buttons, &count, preparsed_data );
    if (status == HIDP_STATUS_SUCCESS && count > 0)
        instance->guidFFDriver = IID_IDirectInputPIDDriver;

    count = ARRAY_SIZE(buttons);
    status = HidP_GetSpecificButtonCaps( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, 0, buttons, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS) count = button_count = 0;
    while (count--)
    {
        if (!buttons[count].IsRange) button_count += 1;
        else button_count += buttons[count].Range.UsageMax - buttons[count].Range.UsageMin + 1;
    }

    switch (caps->Usage)
    {
    case HID_USAGE_GENERIC_GAMEPAD:
        type = DI8DEVTYPE_GAMEPAD | DIDEVTYPE_HID;
        if (button_count < 6) type |= DI8DEVTYPEGAMEPAD_LIMITED << 8;
        else type |= DI8DEVTYPEGAMEPAD_STANDARD << 8;
        break;
    case HID_USAGE_GENERIC_JOYSTICK:
        type = DI8DEVTYPE_JOYSTICK | DIDEVTYPE_HID;
        if (button_count < 5) type |= DI8DEVTYPEJOYSTICK_LIMITED << 8;
        else type |= DI8DEVTYPEJOYSTICK_STANDARD << 8;

        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0,
                                            HID_USAGE_GENERIC_Z, &value, &count, preparsed_data );
        if (status != HIDP_STATUS_SUCCESS || !count)
            type = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DIDEVTYPE_HID;

        count = 1;
        status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0,
                                            HID_USAGE_GENERIC_HATSWITCH, &value, &count, preparsed_data );
        if (status != HIDP_STATUS_SUCCESS || !count)
            type = DI8DEVTYPE_JOYSTICK | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DIDEVTYPE_HID;

        break;
    }

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                        &value, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS || !count)
        type = DI8DEVTYPE_SUPPLEMENTAL | (DI8DEVTYPESUPPLEMENTAL_UNKNOWN << 8) | DIDEVTYPE_HID;

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y,
                                        &value, &count, preparsed_data );
    if (status != HIDP_STATUS_SUCCESS || !count)
        type = DI8DEVTYPE_SUPPLEMENTAL | (DI8DEVTYPESUPPLEMENTAL_UNKNOWN << 8) | DIDEVTYPE_HID;

    instance->dwDevType = device_type_for_version( type, version );

    *device = device_file;
    *preparsed = preparsed_data;
    return TRUE;

failed:
    CloseHandle( device_file );
    HidD_FreePreparsedData( preparsed_data );
    return FALSE;
}

static HRESULT hid_joystick_device_open( int index, DIDEVICEINSTANCEW *filter, WCHAR *device_path,
                                         HANDLE *device, PHIDP_PREPARSED_DATA *preparsed,
                                         HIDD_ATTRIBUTES *attrs, HIDP_CAPS *caps, DWORD version )
{
    static const WCHAR ig_w[] = {'&','I','G','_',0};
    static const WCHAR xi_w[] = {'&','X','I','_',0};
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {.cbSize = sizeof(iface)};
    SP_DEVINFO_DATA devinfo = {.cbSize = sizeof(devinfo)};
    DIDEVICEINSTANCEW instance = *filter;
    WCHAR device_id[MAX_PATH], *tmp;
    HDEVINFO set, xi_set;
    UINT32 i = 0, handle;
    BOOL override;
    DWORD type;
    GUID hid;

    TRACE( "index %d, product %s, instance %s\n", index, debugstr_guid( &filter->guidProduct ),
           debugstr_guid( &filter->guidInstance ) );

    HidD_GetHidGuid( &hid );

    set = SetupDiGetClassDevsW( &hid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    if (set == INVALID_HANDLE_VALUE) return DIERR_DEVICENOTREG;
    xi_set = SetupDiGetClassDevsW( &GUID_DEVINTERFACE_WINEXINPUT, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );

    *device = NULL;
    *preparsed = NULL;
    while (SetupDiEnumDeviceInterfaces( set, NULL, &hid, i++, &iface ))
    {
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, &devinfo ))
            continue;
        if (!SetupDiGetDevicePropertyW( set, &devinfo, &DEVPROPKEY_HID_HANDLE, &type,
                                        (BYTE *)&handle, sizeof(handle), NULL, 0 ) ||
            type != DEVPROP_TYPE_UINT32)
            continue;
        if (!hid_joystick_device_try_open( handle, detail->DevicePath, device, preparsed,
                                           attrs, caps, &instance, version ))
            continue;

        if (device_instance_is_disabled( &instance, &override ))
            goto next;

        if (override)
        {
            if (!SetupDiGetDeviceInstanceIdW( set, &devinfo, device_id, MAX_PATH, NULL ) ||
                !(tmp = strstrW( device_id, ig_w )))
                goto next;
            memcpy( tmp, xi_w, sizeof(xi_w) - sizeof(WCHAR) );
            if (!SetupDiOpenDeviceInfoW( xi_set, device_id, NULL, 0, &devinfo ))
                goto next;
            if (!SetupDiEnumDeviceInterfaces( xi_set, &devinfo, &GUID_DEVINTERFACE_WINEXINPUT, 0, &iface ))
                goto next;
            if (!SetupDiGetDeviceInterfaceDetailW( xi_set, &iface, detail, sizeof(buffer), NULL, &devinfo ))
                goto next;

            CloseHandle( *device );
            HidD_FreePreparsedData( *preparsed );
            if (!hid_joystick_device_try_open( handle, detail->DevicePath, device, preparsed,
                                               attrs, caps, &instance, version ))
                continue;
        }

        /* enumerate device by GUID */
        if (index < 0 && IsEqualGUID( &filter->guidProduct, &instance.guidProduct )) break;
        if (index < 0 && IsEqualGUID( &filter->guidInstance, &instance.guidInstance )) break;

        /* enumerate all devices */
        if (index >= 0 && !index--) break;

    next:
        CloseHandle( *device );
        HidD_FreePreparsedData( *preparsed );
        *device = NULL;
        *preparsed = NULL;
    }

    if (xi_set != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList( xi_set );
    SetupDiDestroyDeviceInfoList( set );
    if (!*device || !*preparsed) return DIERR_DEVICENOTREG;

    lstrcpynW( device_path, detail->DevicePath, MAX_PATH );
    *filter = instance;
    return DI_OK;
}

static HRESULT hid_joystick_enum_device( DWORD type, DWORD flags, DIDEVICEINSTANCEW *instance,
                                         DWORD version, int index )
{
    HIDD_ATTRIBUTES attrs = {.Size = sizeof(attrs)};
    PHIDP_PREPARSED_DATA preparsed;
    WCHAR device_path[MAX_PATH];
    HIDP_CAPS caps;
    HANDLE device;
    HRESULT hr;

    TRACE( "type %#x, flags %#x, instance %p, version %#04x, index %d\n", type, flags, instance, version, index );

    hr = hid_joystick_device_open( index, instance, device_path, &device, &preparsed,
                                   &attrs, &caps, version );
    if (hr != DI_OK) return hr;

    HidD_FreePreparsedData( preparsed );
    CloseHandle( device );

    if (instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return S_FALSE;
    if (version < 0x0800 && type != DIDEVTYPE_JOYSTICK)
        return S_FALSE;
    if (version >= 0x0800 && type != DI8DEVCLASS_ALL && type != DI8DEVCLASS_GAMECTRL)
        return S_FALSE;

    TRACE( "found device %s, usage %04x:%04x, product %s, instance %s, name %s\n", debugstr_w(device_path),
           instance->wUsagePage, instance->wUsage, debugstr_guid( &instance->guidProduct ),
           debugstr_guid( &instance->guidInstance ), debugstr_w(instance->tszInstanceName) );

    return DI_OK;
}

static BOOL init_objects( struct hid_joystick *impl, struct hid_value_caps *caps,
                          DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *format = impl->base.data_format.wine_df;

    format->dwNumObjs++;
    format->dwDataSize = max( format->dwDataSize, instance->dwOfs + sizeof(LONG) );
    if (instance->dwType & DIDFT_BUTTON) impl->dev_caps.dwButtons++;
    if (instance->dwType & DIDFT_AXIS) impl->dev_caps.dwAxes++;
    if (instance->dwType & DIDFT_POV) impl->dev_caps.dwPOVs++;

    if (instance->dwType & (DIDFT_BUTTON|DIDFT_AXIS|DIDFT_POV) &&
        (instance->wUsagePage == HID_USAGE_PAGE_GENERIC ||
         instance->wUsagePage == HID_USAGE_PAGE_BUTTON))
    {
        if (!impl->device_state_report_id)
            impl->device_state_report_id = instance->wReportId;
        else if (impl->device_state_report_id != instance->wReportId)
            FIXME( "multiple device state reports found!\n" );
    }

    return DIENUM_CONTINUE;
}

static BOOL init_data_format( struct hid_joystick *impl, struct hid_value_caps *caps,
                              DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *format = impl->base.data_format.wine_df;
    DIOBJECTDATAFORMAT *obj_format;
    DWORD *index = data;

    obj_format = format->rgodf + *index;
    obj_format->pguid = object_usage_to_guid( instance->wUsagePage, instance->wUsage );
    obj_format->dwOfs = instance->dwOfs;
    obj_format->dwType = instance->dwType;
    obj_format->dwFlags = instance->dwFlags;
    (*index)++;

    return DIENUM_CONTINUE;
}

static BOOL init_pid_reports( struct hid_joystick *impl, struct hid_value_caps *caps,
                              DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct pid_set_constant_force *set_constant_force = &impl->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->pid_set_ramp_force;
    struct pid_control_report *device_control = &impl->pid_device_control;
    struct pid_control_report *effect_control = &impl->pid_effect_control;
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;

#define SET_COLLECTION( rep )                                          \
    do                                                                 \
    {                                                                  \
        if (rep->collection) FIXME( "duplicate " #rep " report!\n" );  \
        else rep->collection = DIDFT_GETINSTANCE( instance->dwType );  \
    } while (0)

#define SET_SUB_COLLECTION( rep, sub )                                 \
    do {                                                               \
        if (instance->wCollectionNumber != rep->collection)            \
            FIXME( "unexpected " #rep "." #sub " parent!\n" );         \
        else if (rep->sub)                                             \
            FIXME( "duplicate " #rep "." #sub " collection!\n" );      \
        else                                                           \
            rep->sub = DIDFT_GETINSTANCE( instance->dwType );          \
    } while (0)

    if (instance->wUsagePage == HID_USAGE_PAGE_PID)
    {
        switch (instance->wUsage)
        {
        case PID_USAGE_DEVICE_CONTROL_REPORT: SET_COLLECTION( device_control ); break;
        case PID_USAGE_EFFECT_OPERATION_REPORT: SET_COLLECTION( effect_control ); break;
        case PID_USAGE_SET_EFFECT_REPORT: SET_COLLECTION( effect_update ); break;
        case PID_USAGE_SET_PERIODIC_REPORT: SET_COLLECTION( set_periodic ); break;
        case PID_USAGE_SET_ENVELOPE_REPORT: SET_COLLECTION( set_envelope ); break;
        case PID_USAGE_SET_CONDITION_REPORT: SET_COLLECTION( set_condition ); break;
        case PID_USAGE_SET_CONSTANT_FORCE_REPORT: SET_COLLECTION( set_constant_force ); break;
        case PID_USAGE_SET_RAMP_FORCE_REPORT: SET_COLLECTION( set_ramp_force ); break;

        case PID_USAGE_DEVICE_CONTROL: SET_SUB_COLLECTION( device_control, control_coll ); break;
        case PID_USAGE_EFFECT_OPERATION: SET_SUB_COLLECTION( effect_control, control_coll ); break;
        case PID_USAGE_EFFECT_TYPE:
            if (instance->wCollectionNumber == effect_update->collection)
                SET_SUB_COLLECTION( effect_update, type_coll );
            break;
        case PID_USAGE_AXES_ENABLE: SET_SUB_COLLECTION( effect_update, axes_coll ); break;
        case PID_USAGE_DIRECTION: SET_SUB_COLLECTION( effect_update, direction_coll ); break;
        }
    }

#undef SET_SUB_COLLECTION
#undef SET_COLLECTION

    return DIENUM_CONTINUE;
}

static BOOL init_pid_caps( struct hid_joystick *impl, struct hid_value_caps *caps,
                           DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct pid_set_constant_force *set_constant_force = &impl->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->pid_set_ramp_force;
    struct pid_control_report *device_control = &impl->pid_device_control;
    struct pid_control_report *effect_control = &impl->pid_effect_control;
    struct pid_effect_update *effect_update = &impl->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->pid_set_envelope;

    if (!(instance->dwType & DIDFT_OUTPUT)) return DIENUM_CONTINUE;

#define SET_REPORT_ID( rep )                                           \
    do                                                                 \
    {                                                                  \
        if (!rep->id)                                                  \
            rep->id = instance->wReportId;                             \
        else if (rep->id != instance->wReportId)                       \
            FIXME( "multiple " #rep " report ids!\n" );                \
    } while (0)

    if (instance->wCollectionNumber == device_control->control_coll)
        SET_REPORT_ID( device_control );
    if (instance->wCollectionNumber == effect_control->control_coll)
        SET_REPORT_ID( effect_control );
    if (instance->wCollectionNumber == effect_update->type_coll)
        SET_REPORT_ID( effect_update );
    if (instance->wCollectionNumber == effect_update->collection)
    {
        SET_REPORT_ID( effect_update );
        if (instance->wUsage == PID_USAGE_DURATION)
            effect_update->duration_caps = caps;
        if (instance->wUsage == PID_USAGE_GAIN)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            effect_update->gain_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_SAMPLE_PERIOD)
            effect_update->sample_period_caps = caps;
        if (instance->wUsage == PID_USAGE_START_DELAY)
            effect_update->start_delay_caps = caps;
        if (instance->wUsage == PID_USAGE_TRIGGER_BUTTON)
            effect_update->trigger_button_caps = caps;
        if (instance->wUsage == PID_USAGE_TRIGGER_REPEAT_INTERVAL)
            effect_update->trigger_repeat_interval_caps = caps;
    }
    if (instance->wCollectionNumber == effect_update->axes_coll)
    {
        SET_REPORT_ID( effect_update );
        caps->physical_min = 0;
        caps->physical_max = 36000;
        if (effect_update->axis_count >= 6) FIXME( "more than 6 PID axes detected\n" );
        else effect_update->axis_caps[effect_update->axis_count] = caps;
        effect_update->axis_count++;
    }
    if (instance->wCollectionNumber == effect_update->direction_coll)
    {
        SET_REPORT_ID( effect_update );
        caps->physical_min = 0;
        caps->physical_max = 36000 - 36000 / (caps->logical_max - caps->logical_min);
        if (effect_update->direction_count >= 6) FIXME( "more than 6 PID directions detected\n" );
        else effect_update->direction_caps[effect_update->direction_count] = caps;
        effect_update->direction_count++;
    }
    if (instance->wCollectionNumber == set_periodic->collection)
    {
        SET_REPORT_ID( set_periodic );
        if (instance->wUsage == PID_USAGE_MAGNITUDE)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_periodic->magnitude_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_PERIOD)
            set_periodic->period_caps = caps;
        if (instance->wUsage == PID_USAGE_PHASE)
        {
            caps->physical_min = 0;
            caps->physical_max = 36000 - 36000 / (caps->logical_max - caps->logical_min);
            set_periodic->phase_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_OFFSET)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_periodic->offset_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_envelope->collection)
    {
        SET_REPORT_ID( set_envelope );
        if (instance->wUsage == PID_USAGE_ATTACK_LEVEL)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_envelope->attack_level_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_ATTACK_TIME)
            set_envelope->attack_time_caps = caps;
        if (instance->wUsage == PID_USAGE_FADE_LEVEL)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_envelope->fade_level_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_FADE_TIME)
            set_envelope->fade_time_caps = caps;
    }
    if (instance->wCollectionNumber == set_condition->collection)
    {
        SET_REPORT_ID( set_condition );
        if (instance->wUsage == PID_USAGE_CP_OFFSET)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->center_point_offset_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_POSITIVE_COEFFICIENT)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->positive_coefficient_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_NEGATIVE_COEFFICIENT)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_condition->negative_coefficient_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_POSITIVE_SATURATION)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->positive_saturation_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_NEGATIVE_SATURATION)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->negative_saturation_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_DEAD_BAND)
        {
            caps->physical_min = 0;
            caps->physical_max = 10000;
            set_condition->dead_band_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_constant_force->collection)
    {
        SET_REPORT_ID( set_constant_force );
        if (instance->wUsage == PID_USAGE_MAGNITUDE)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_constant_force->magnitude_caps = caps;
        }
    }
    if (instance->wCollectionNumber == set_ramp_force->collection)
    {
        SET_REPORT_ID( set_ramp_force );
        if (instance->wUsage == PID_USAGE_RAMP_START)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_ramp_force->start_caps = caps;
        }
        if (instance->wUsage == PID_USAGE_RAMP_END)
        {
            caps->physical_min = -10000;
            caps->physical_max = 10000;
            set_ramp_force->start_caps = caps;
        }
    }

#undef SET_REPORT_ID

    return DIENUM_CONTINUE;
}

static HRESULT hid_joystick_create_device( IDirectInputImpl *dinput, const GUID *guid, IDirectInputDevice8W **out )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    DIDEVICEINSTANCEW instance =
    {
        .dwSize = sizeof(instance),
        .guidProduct = *guid,
        .guidInstance = *guid
    };
    DIPROPRANGE range =
    {
        .diph =
        {
            .dwSize = sizeof(range),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPDWORD saturation =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    HIDD_ATTRIBUTES attrs = {.Size = sizeof(attrs)};
    struct hid_preparsed_data *preparsed;
    struct hid_joystick *impl = NULL;
    struct extra_caps *extra;
    DIDATAFORMAT *format = NULL;
    USAGE_AND_PAGE *usages;
    DWORD size, index;
    char *buffer;
    HRESULT hr;

    TRACE( "dinput %p, guid %s, out %p\n", dinput, debugstr_guid( guid ), out );

    *out = NULL;
    instance.guidProduct.Data1 = DInput_PIDVID_Product_GUID.Data1;
    instance.guidInstance.Data1 = hid_joystick_guid.Data1;
    if (IsEqualGUID( &DInput_PIDVID_Product_GUID, &instance.guidProduct ))
        instance.guidProduct = *guid;
    else if (IsEqualGUID( &hid_joystick_guid, &instance.guidInstance ))
        instance.guidInstance = *guid;
    else
        return DIERR_DEVICENOTREG;

    hr = direct_input_device_alloc( sizeof(struct hid_joystick), &hid_joystick_vtbl, guid,
                                    dinput, (void **)&impl );
    if (FAILED(hr)) return hr;
    impl->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": hid_joystick.base.crit");
    impl->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
    impl->base.read_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    impl->base.read_callback = hid_joystick_read_state;

    hr = hid_joystick_device_open( -1, &instance, impl->device_path, &impl->device, &impl->preparsed,
                                   &attrs, &impl->caps, dinput->dwVersion );
    if (hr != DI_OK) goto failed;

    impl->ref = 1;
    impl->instance = instance;
    impl->attrs = attrs;
    impl->dev_caps.dwSize = sizeof(impl->dev_caps);
    impl->dev_caps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    impl->dev_caps.dwDevType = instance.dwDevType;
    list_init( &impl->effect_list );

    preparsed = (struct hid_preparsed_data *)impl->preparsed;

    size = preparsed->input_caps_count * sizeof(struct extra_caps);
    if (!(extra = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto failed;
    impl->input_extra_caps = extra;

    size = impl->caps.InputReportByteLength;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->input_report_buf = buffer;
    size = impl->caps.OutputReportByteLength;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->output_report_buf = buffer;
    impl->usages_count = HidP_MaxUsageListLength( HidP_Input, 0, impl->preparsed );
    size = impl->usages_count * sizeof(USAGE_AND_PAGE);
    if (!(usages = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->usages_buf = usages;

    enum_objects( impl, &filter, DIDFT_ALL, init_objects, NULL );
    enum_objects( impl, &filter, DIDFT_COLLECTION, init_pid_reports, NULL );
    enum_objects( impl, &filter, DIDFT_NODATA, init_pid_caps, NULL );

    TRACE( "device control id %u, coll %u, control coll %u\n", impl->pid_device_control.id,
           impl->pid_device_control.collection, impl->pid_device_control.control_coll );
    TRACE( "effect control id %u, coll %u\n", impl->pid_effect_control.id, impl->pid_effect_control.collection );
    TRACE( "effect update id %u, coll %u, type_coll %u\n", impl->pid_effect_update.id,
           impl->pid_effect_update.collection, impl->pid_effect_update.type_coll );
    TRACE( "set periodic id %u, coll %u\n", impl->pid_set_periodic.id, impl->pid_set_periodic.collection );
    TRACE( "set envelope id %u, coll %u\n", impl->pid_set_envelope.id, impl->pid_set_envelope.collection );
    TRACE( "set condition id %u, coll %u\n", impl->pid_set_condition.id, impl->pid_set_condition.collection );
    TRACE( "set constant force id %u, coll %u\n", impl->pid_set_constant_force.id,
           impl->pid_set_constant_force.collection );
    TRACE( "set ramp force id %u, coll %u\n", impl->pid_set_ramp_force.id, impl->pid_set_ramp_force.collection );

    if (impl->pid_device_control.id)
    {
        impl->dev_caps.dwFlags |= DIDC_FORCEFEEDBACK;
        if (impl->pid_effect_update.start_delay_caps)
            impl->dev_caps.dwFlags |= DIDC_STARTDELAY;
        if (impl->pid_set_envelope.attack_level_caps ||
            impl->pid_set_envelope.attack_time_caps)
            impl->dev_caps.dwFlags |= DIDC_FFATTACK;
        if (impl->pid_set_envelope.fade_level_caps ||
            impl->pid_set_envelope.fade_time_caps)
            impl->dev_caps.dwFlags |= DIDC_FFFADE;
        if (impl->pid_set_condition.positive_coefficient_caps ||
            impl->pid_set_condition.negative_coefficient_caps)
            impl->dev_caps.dwFlags |= DIDC_POSNEGCOEFFICIENTS;
        if (impl->pid_set_condition.positive_saturation_caps ||
            impl->pid_set_condition.negative_saturation_caps)
            impl->dev_caps.dwFlags |= DIDC_SATURATION|DIDC_POSNEGSATURATION;
        if (impl->pid_set_condition.dead_band_caps)
            impl->dev_caps.dwFlags |= DIDC_DEADBAND;
        impl->dev_caps.dwFFSamplePeriod = 1000000;
        impl->dev_caps.dwFFMinTimeResolution = 1000000;
        impl->dev_caps.dwHardwareRevision = 1;
        impl->dev_caps.dwFFDriverVersion = 1;
    }

    format = impl->base.data_format.wine_df;
    if (format->dwDataSize > DEVICE_STATE_MAX_SIZE)
    {
        FIXME( "unable to create device, state is too large\n" );
        goto failed;
    }

    size = format->dwNumObjs * sizeof(*format->rgodf);
    if (!(format->rgodf = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto failed;
    format->dwSize = sizeof(*format);
    format->dwObjSize = sizeof(*format->rgodf);
    format->dwFlags = DIDF_ABSAXIS;

    index = 0;
    enum_objects( impl, &filter, DIDFT_ALL, init_data_format, &index );

    _dump_DIDATAFORMAT( impl->base.data_format.wine_df );

    range.lMax = 65535;
    enum_objects( impl, &range.diph, DIDFT_AXIS, set_property_prop_range, &range );
    range.lMax = 36000;
    enum_objects( impl, &range.diph, DIDFT_POV, set_property_prop_range, &range );
    saturation.dwData = 10000;
    enum_objects( impl, &range.diph, DIDFT_AXIS, set_property_prop_saturation, &saturation );

    *out = &impl->base.IDirectInputDevice8W_iface;
    return DI_OK;

failed:
    IDirectInputDevice_Release( &impl->base.IDirectInputDevice8W_iface );
    return hr;
}

const struct dinput_device joystick_hid_device =
{
    "Wine HID joystick driver",
    hid_joystick_enum_device,
    hid_joystick_create_device,
};

static HRESULT WINAPI hid_joystick_effect_QueryInterface( IDirectInputEffect *iface, REFIID iid, void **out )
{
    TRACE( "iface %p, iid %s, out %p\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IDirectInputEffect ))
    {
        IDirectInputEffect_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI hid_joystick_effect_AddRef( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %u.\n", iface, ref );
    return ref;
}

static ULONG WINAPI hid_joystick_effect_Release( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %u.\n", iface, ref );
    if (!ref)
    {
        IDirectInputEffect_Unload( iface );
        EnterCriticalSection( &impl->joystick->base.crit );
        list_remove( &impl->entry );
        LeaveCriticalSection( &impl->joystick->base.crit );
        hid_joystick_private_decref( impl->joystick );
        HeapFree( GetProcessHeap(), 0, impl->type_specific_buf[1] );
        HeapFree( GetProcessHeap(), 0, impl->type_specific_buf[0] );
        HeapFree( GetProcessHeap(), 0, impl->effect_update_buf );
        HeapFree( GetProcessHeap(), 0, impl->effect_control_buf );
        HeapFree( GetProcessHeap(), 0, impl );
    }
    return ref;
}

static HRESULT WINAPI hid_joystick_effect_Initialize( IDirectInputEffect *iface, HINSTANCE inst,
                                                      DWORD version, REFGUID guid )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct hid_joystick *joystick = impl->joystick;
    ULONG count, report_len = joystick->caps.OutputReportByteLength;
    NTSTATUS status;
    USAGE type;

    TRACE( "iface %p, inst %p, version %u, guid %s\n", iface, inst, version, debugstr_guid( guid ) );

    if (!inst) return DIERR_INVALIDPARAM;
    if (!guid) return E_POINTER;
    if (!(type = effect_guid_to_usage( guid ))) return DIERR_DEVICENOTREG;

    status = HidP_InitializeReportForID( HidP_Output, joystick->pid_effect_update.id,
                                         joystick->preparsed, impl->effect_update_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;

    switch (type)
    {
    case PID_USAGE_ET_SQUARE:
    case PID_USAGE_ET_SINE:
    case PID_USAGE_ET_TRIANGLE:
    case PID_USAGE_ET_SAWTOOTH_UP:
    case PID_USAGE_ET_SAWTOOTH_DOWN:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_periodic.id,
                                             joystick->preparsed, impl->type_specific_buf[0], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_envelope.id, joystick->preparsed,
                                             impl->type_specific_buf[1], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        break;
    case PID_USAGE_ET_SPRING:
    case PID_USAGE_ET_DAMPER:
    case PID_USAGE_ET_INERTIA:
    case PID_USAGE_ET_FRICTION:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_condition.id, joystick->preparsed,
                                             impl->type_specific_buf[0], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_condition.id, joystick->preparsed,
                                             impl->type_specific_buf[1], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        break;
    case PID_USAGE_ET_CONSTANT_FORCE:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_constant_force.id, joystick->preparsed,
                                             impl->type_specific_buf[0], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_envelope.id, joystick->preparsed,
                                             impl->type_specific_buf[1], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        break;
    case PID_USAGE_ET_RAMP:
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_ramp_force.id, joystick->preparsed,
                                             impl->type_specific_buf[0], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        status = HidP_InitializeReportForID( HidP_Output, joystick->pid_set_envelope.id, joystick->preparsed,
                                             impl->type_specific_buf[1], report_len );
        if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;
        break;
    case PID_USAGE_ET_CUSTOM_FORCE_DATA:
        FIXME( "effect type %#x not implemented!\n", type );
        impl->type_specific_buf[0][0] = 0;
        impl->type_specific_buf[1][0] = 0;
        break;
    }

    count = 1;
    status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, joystick->pid_effect_update.type_coll,
                             &type, &count, joystick->preparsed, impl->effect_update_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) return DIERR_DEVICENOTREG;

    impl->type = type;
    return DI_OK;
}

static HRESULT WINAPI hid_joystick_effect_GetEffectGuid( IDirectInputEffect *iface, GUID *guid )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );

    TRACE( "iface %p, guid %p.\n", iface, guid );

    if (!guid) return E_POINTER;
    *guid = *effect_usage_to_guid( impl->type );

    return DI_OK;
}

static BOOL get_parameters_object_id( struct hid_joystick *impl, struct hid_value_caps *caps,
                                      DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    *(DWORD *)data = instance->dwType;
    return DIENUM_STOP;
}

static BOOL get_parameters_object_ofs( struct hid_joystick *impl, struct hid_value_caps *caps,
                                       DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *format = impl->base.data_format.wine_df;
    int *offsets = impl->base.data_format.offsets;
    ULONG i;

    if (!offsets) return DIENUM_CONTINUE;
    for (i = 0; i < format->dwNumObjs; ++i)
        if (format->rgodf[i].dwOfs == instance->dwOfs) break;
    if (i == format->dwNumObjs) return DIENUM_CONTINUE;
    *(DWORD *)data = offsets[i];

    return DIENUM_STOP;
}

static HRESULT WINAPI hid_joystick_effect_GetParameters( IDirectInputEffect *iface, DIEFFECT *params, DWORD flags )
{
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(DIPROPHEADER),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_BYUSAGE,
    };
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG i, j, count, capacity, object_flags, direction_flags;
    LONG tmp, directions[6] = {0};
    BOOL ret;

    TRACE( "iface %p, params %p, flags %#x.\n", iface, params, flags );

    if (!params) return DI_OK;
    if (params->dwSize != sizeof(DIEFFECT)) return DIERR_INVALIDPARAM;
    capacity = params->cAxes;
    object_flags = params->dwFlags & (DIEFF_OBJECTIDS | DIEFF_OBJECTOFFSETS);
    direction_flags = params->dwFlags & (DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);

    if (flags & DIEP_AXES)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;
        params->cAxes = impl->params.cAxes;
        if (capacity < impl->params.cAxes) return DIERR_MOREDATA;

        for (i = 0; i < impl->params.cAxes; ++i)
        {
            if (!params->rgdwAxes) return DIERR_INVALIDPARAM;
            filter.dwObj = impl->params.rgdwAxes[i];
            if (object_flags & DIEFF_OBJECTIDS)
                ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, get_parameters_object_id,
                                    &params->rgdwAxes[i] );
            else
                ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, get_parameters_object_ofs,
                                    &params->rgdwAxes[i] );
            if (ret != DIENUM_STOP) params->rgdwAxes[i] = 0;
        }
    }

    if (flags & DIEP_DIRECTION)
    {
        if (!direction_flags) return DIERR_INVALIDPARAM;

        count = params->cAxes = impl->params.cAxes;
        if (capacity < params->cAxes) return DIERR_MOREDATA;
        if (!count) params->dwFlags &= ~(DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);

        if (direction_flags & DIEFF_SPHERICAL)
            memcpy( directions, impl->params.rglDirection, count * sizeof(LONG) );
        else if (direction_flags & DIEFF_POLAR)
        {
            if (count != 2) return DIERR_INVALIDPARAM;
            directions[0] = (impl->params.rglDirection[0] + 9000) % 36000;
            if (directions[0] < 0) directions[0] += 36000;
        }
        else if (direction_flags & DIEFF_CARTESIAN)
        {
            directions[0] = 10000;
            for (i = 1; i <= count; ++i)
            {
                tmp = cos( impl->params.rglDirection[i - 1] * M_PI / 18000 ) * 10000;
                for (j = 0; j < i; ++j) directions[j] = round( directions[j] * tmp / 10000.0 );
                directions[i] = sin( impl->params.rglDirection[i - 1] * M_PI / 18000 ) * 10000;
            }
        }

        if (!count) params->rglDirection = NULL;
        else if (!params->rglDirection) return DIERR_INVALIDPARAM;
        else memcpy( params->rglDirection, directions, count * sizeof(LONG) );
    }

    if (flags & DIEP_TYPESPECIFICPARAMS)
    {
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DIPERIODIC)) return DIERR_INVALIDPARAM;
            memcpy( params->lpvTypeSpecificParams, &impl->periodic, sizeof(DIPERIODIC) );
            break;
        case PID_USAGE_ET_SPRING:
        case PID_USAGE_ET_DAMPER:
        case PID_USAGE_ET_INERTIA:
        case PID_USAGE_ET_FRICTION:
            count = impl->params.cbTypeSpecificParams;
            capacity = params->cbTypeSpecificParams;
            params->cbTypeSpecificParams = count;
            if (capacity < count) return DIERR_MOREDATA;
            memcpy( params->lpvTypeSpecificParams, impl->condition, params->cbTypeSpecificParams );
            break;
        case PID_USAGE_ET_CONSTANT_FORCE:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DICONSTANTFORCE)) return DIERR_INVALIDPARAM;
            memcpy( params->lpvTypeSpecificParams, &impl->constant_force, sizeof(DICONSTANTFORCE) );
            break;
        case PID_USAGE_ET_RAMP:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DIRAMPFORCE)) return DIERR_INVALIDPARAM;
            memcpy( params->lpvTypeSpecificParams, &impl->constant_force, sizeof(DIRAMPFORCE) );
            break;
        case PID_USAGE_ET_CUSTOM_FORCE_DATA:
            FIXME( "DIEP_TYPESPECIFICPARAMS not implemented!\n" );
            return DIERR_UNSUPPORTED;
        }
    }

    if (flags & DIEP_ENVELOPE)
    {
        if (!params->lpEnvelope) return E_POINTER;
        if (params->lpEnvelope->dwSize != sizeof(DIENVELOPE)) return DIERR_INVALIDPARAM;
        memcpy( params->lpEnvelope, &impl->envelope, sizeof(DIENVELOPE) );
    }

    if (flags & DIEP_DURATION) params->dwDuration = impl->params.dwDuration;
    if (flags & DIEP_GAIN) params->dwGain = impl->params.dwGain;
    if (flags & DIEP_SAMPLEPERIOD) params->dwSamplePeriod = impl->params.dwSamplePeriod;
    if (flags & DIEP_STARTDELAY) params->dwStartDelay = impl->params.dwStartDelay;
    if (flags & DIEP_TRIGGERREPEATINTERVAL) params->dwTriggerRepeatInterval = impl->params.dwTriggerRepeatInterval;

    if (flags & DIEP_TRIGGERBUTTON)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;

        filter.dwObj = impl->params.dwTriggerButton;
        if (object_flags & DIEFF_OBJECTIDS)
            ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, get_parameters_object_id,
                                &params->dwTriggerButton );
        else
            ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, get_parameters_object_ofs,
                                &params->dwTriggerButton );
        if (ret != DIENUM_STOP) params->dwTriggerButton = -1;
    }

    return DI_OK;
}

static BOOL set_parameters_object( struct hid_joystick *impl, struct hid_value_caps *caps,
                                   DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DWORD usages = MAKELONG( instance->wUsage, instance->wUsagePage );
    *(DWORD *)data = usages;
    return DIENUM_STOP;
}

static HRESULT WINAPI hid_joystick_effect_SetParameters( IDirectInputEffect *iface,
                                                         const DIEFFECT *params, DWORD flags )
{
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(DIPROPHEADER),
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_BYUSAGE,
    };
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    ULONG i, count, old_value, object_flags, direction_flags;
    LONG directions[6] = {0};
    HRESULT hr;
    BOOL ret;

    TRACE( "iface %p, params %p, flags %#x.\n", iface, params, flags );

    if (!params) return E_POINTER;
    if (params->dwSize != sizeof(DIEFFECT)) return DIERR_INVALIDPARAM;
    object_flags = params->dwFlags & (DIEFF_OBJECTIDS | DIEFF_OBJECTOFFSETS);
    direction_flags = params->dwFlags & (DIEFF_CARTESIAN | DIEFF_POLAR | DIEFF_SPHERICAL);

    if (object_flags & DIEFF_OBJECTIDS) filter.dwHow = DIPH_BYID;
    else filter.dwHow = DIPH_BYOFFSET;

    if (flags & DIEP_AXES)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;
        if (!params->rgdwAxes) return DIERR_INVALIDPARAM;
        if (impl->params.cAxes) return DIERR_ALREADYINITIALIZED;
        count = impl->joystick->pid_effect_update.axis_count;
        if (params->cAxes > count) return DIERR_INVALIDPARAM;

        impl->params.cAxes = params->cAxes;
        for (i = 0; i < params->cAxes; ++i)
        {
            filter.dwObj = params->rgdwAxes[i];
            ret = enum_objects( impl->joystick, &filter, DIDFT_AXIS, set_parameters_object,
                                &impl->params.rgdwAxes[i] );
            if (ret != DIENUM_STOP) impl->params.rgdwAxes[i] = 0;
        }

        impl->modified = TRUE;
    }

    if (flags & DIEP_DIRECTION)
    {
        if (!direction_flags) return DIERR_INVALIDPARAM;
        if (!params->rglDirection) return DIERR_INVALIDPARAM;

        count = impl->params.cAxes;
        if (params->cAxes < count) return DIERR_INVALIDPARAM;
        if ((direction_flags & DIEFF_POLAR) && count != 2) return DIERR_INVALIDPARAM;
        if ((direction_flags & DIEFF_CARTESIAN) && count < 2) return DIERR_INVALIDPARAM;

        if (!count) memset( directions, 0, sizeof(directions) );
        else if (direction_flags & DIEFF_POLAR)
        {
            directions[0] = (params->rglDirection[0] % 36000) - 9000;
            if (directions[0] < 0) directions[0] += 36000;
            for (i = 1; i < count; ++i) directions[i] = 0;
        }
        else if (direction_flags & DIEFF_CARTESIAN)
        {
            for (i = 1; i < count; ++i)
                directions[i - 1] = atan2( params->rglDirection[i], params->rglDirection[0] );
            directions[count - 1] = 0;
        }
        else
        {
            for (i = 0; i < count; ++i)
            {
                directions[i] = params->rglDirection[i] % 36000;
                if (directions[i] < 0) directions[i] += 36000;
            }
        }

        if (memcmp( impl->params.rglDirection, directions, count * sizeof(LONG) ))
            impl->modified = TRUE;
        memcpy( impl->params.rglDirection, directions, count * sizeof(LONG) );
    }

    if (flags & DIEP_TYPESPECIFICPARAMS)
    {
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DIPERIODIC)) return DIERR_INVALIDPARAM;
            if (memcmp( &impl->periodic, params->lpvTypeSpecificParams, sizeof(DIPERIODIC) ))
                impl->modified = TRUE;
            memcpy( &impl->periodic, params->lpvTypeSpecificParams, sizeof(DIPERIODIC) );
            impl->params.cbTypeSpecificParams = sizeof(DIPERIODIC);
            break;
        case PID_USAGE_ET_SPRING:
        case PID_USAGE_ET_DAMPER:
        case PID_USAGE_ET_INERTIA:
        case PID_USAGE_ET_FRICTION:
            if ((count = impl->params.cAxes))
            {
                if (!params->lpvTypeSpecificParams) return E_POINTER;
                if (params->cbTypeSpecificParams != count * sizeof(DICONDITION) &&
                    params->cbTypeSpecificParams != sizeof(DICONDITION))
                    return DIERR_INVALIDPARAM;
                memcpy( impl->condition, params->lpvTypeSpecificParams, params->cbTypeSpecificParams );
                impl->params.cbTypeSpecificParams = params->cbTypeSpecificParams;
            }
            break;
        case PID_USAGE_ET_CONSTANT_FORCE:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DICONSTANTFORCE)) return DIERR_INVALIDPARAM;
            if (memcmp( &impl->constant_force, params->lpvTypeSpecificParams, sizeof(DICONSTANTFORCE) ))
                impl->modified = TRUE;
            memcpy( &impl->constant_force, params->lpvTypeSpecificParams, sizeof(DICONSTANTFORCE) );
            impl->params.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
            break;
        case PID_USAGE_ET_RAMP:
            if (!params->lpvTypeSpecificParams) return E_POINTER;
            if (params->cbTypeSpecificParams != sizeof(DIRAMPFORCE)) return DIERR_INVALIDPARAM;
            if (memcmp( &impl->constant_force, params->lpvTypeSpecificParams, sizeof(DIRAMPFORCE) ))
                impl->modified = TRUE;
            memcpy( &impl->constant_force, params->lpvTypeSpecificParams, sizeof(DIRAMPFORCE) );
            impl->params.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
            break;
        case PID_USAGE_ET_CUSTOM_FORCE_DATA:
            FIXME( "DIEP_TYPESPECIFICPARAMS not implemented!\n" );
            return DIERR_UNSUPPORTED;
        }
    }

    if ((flags & DIEP_ENVELOPE) && params->lpEnvelope)
    {
        if (params->lpEnvelope->dwSize != sizeof(DIENVELOPE)) return DIERR_INVALIDPARAM;
        if (memcmp( &impl->envelope, params->lpEnvelope, sizeof(DIENVELOPE) ))
            impl->modified = TRUE;
        memcpy( &impl->envelope, params->lpEnvelope, sizeof(DIENVELOPE) );
    }

    if (flags & DIEP_DURATION)
    {
        if (impl->params.dwDuration != params->dwDuration) impl->modified = TRUE;
        impl->params.dwDuration = params->dwDuration;
    }
    if (flags & DIEP_GAIN)
    {
        if (impl->params.dwGain != params->dwGain) impl->modified = TRUE;
        impl->params.dwGain = params->dwGain;
    }
    if (flags & DIEP_SAMPLEPERIOD)
    {
        if (impl->params.dwSamplePeriod != params->dwSamplePeriod) impl->modified = TRUE;
        impl->params.dwSamplePeriod = params->dwSamplePeriod;
    }
    if (flags & DIEP_STARTDELAY)
    {
        if (impl->params.dwStartDelay != params->dwStartDelay) impl->modified = TRUE;
        impl->params.dwStartDelay = params->dwStartDelay;
    }
    if (flags & DIEP_TRIGGERREPEATINTERVAL)
    {
        if (impl->params.dwTriggerRepeatInterval != params->dwTriggerRepeatInterval)
            impl->modified = TRUE;
        impl->params.dwTriggerRepeatInterval = params->dwTriggerRepeatInterval;
    }

    if (flags & DIEP_TRIGGERBUTTON)
    {
        if (!object_flags) return DIERR_INVALIDPARAM;

        filter.dwObj = params->dwTriggerButton;
        old_value = impl->params.dwTriggerButton;
        ret = enum_objects( impl->joystick, &filter, DIDFT_BUTTON, set_parameters_object,
                            &impl->params.dwTriggerButton );
        if (ret != DIENUM_STOP) impl->params.dwTriggerButton = -1;
        if (impl->params.dwTriggerButton != old_value) impl->modified = TRUE;
    }

    impl->flags |= flags;

    if (flags & DIEP_NODOWNLOAD) return DI_DOWNLOADSKIPPED;
    if (flags & DIEP_START) return IDirectInputEffect_Start( iface, 1, 0 );
    if (FAILED(hr = IDirectInputEffect_Download( iface ))) return hr;
    if (hr == DI_NOEFFECT) return DI_DOWNLOADSKIPPED;
    return DI_OK;
}

static HRESULT WINAPI hid_joystick_effect_Start( IDirectInputEffect *iface, DWORD iterations, DWORD flags )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_control_report *effect_control = &impl->joystick->pid_effect_control;
    ULONG count, report_len = impl->joystick->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->joystick->preparsed;
    HANDLE device = impl->joystick->device;
    NTSTATUS status;
    USAGE control;
    HRESULT hr;

    TRACE( "iface %p, iterations %u, flags %#x.\n", iface, iterations, flags );

    if ((flags & ~(DIES_NODOWNLOAD|DIES_SOLO))) return DIERR_INVALIDPARAM;
    if (flags & DIES_SOLO) control = PID_USAGE_OP_EFFECT_START_SOLO;
    else control = PID_USAGE_OP_EFFECT_START;

    EnterCriticalSection( &impl->joystick->base.crit );
    if (!impl->joystick->base.acquired || !(impl->joystick->base.dwCoopLevel & DISCL_EXCLUSIVE))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if ((flags & DIES_NODOWNLOAD) && !impl->index)
        hr = DIERR_NOTDOWNLOADED;
    else if ((flags & DIES_NODOWNLOAD) || !FAILED(hr = IDirectInputEffect_Download( iface )))
    {
        count = 1;
        status = HidP_InitializeReportForID( HidP_Output, effect_control->id, preparsed,
                                             impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, effect_control->control_coll,
                                      &control, &count, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_LOOP_COUNT,
                                          iterations, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else if (WriteFile( device, impl->effect_control_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_INPUTLOST;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_Stop( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_control_report *effect_control = &impl->joystick->pid_effect_control;
    ULONG count, report_len = impl->joystick->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->joystick->preparsed;
    HANDLE device = impl->joystick->device;
    NTSTATUS status;
    USAGE control;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->joystick->base.crit );
    if (!impl->joystick->base.acquired || !(impl->joystick->base.dwCoopLevel & DISCL_EXCLUSIVE))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if (!impl->index)
        hr = DIERR_NOTDOWNLOADED;
    else
    {
        count = 1;
        control = PID_USAGE_OP_EFFECT_STOP;
        status = HidP_InitializeReportForID( HidP_Output, effect_control->id, preparsed,
                                             impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, effect_control->control_coll,
                                      &control, &count, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_LOOP_COUNT,
                                          0, preparsed, impl->effect_control_buf, report_len );

        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else if (WriteFile( device, impl->effect_control_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_INPUTLOST;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_GetEffectStatus( IDirectInputEffect *iface, DWORD *status )
{
    FIXME( "iface %p, status %p stub!\n", iface, status );

    if (!status) return E_POINTER;

    return DIERR_UNSUPPORTED;
}

static void set_parameter_value( struct hid_joystick_effect *impl, char *report_buf,
                                 struct hid_value_caps *caps, LONG value )
{
    ULONG report_len = impl->joystick->caps.OutputReportByteLength;
    PHIDP_PREPARSED_DATA preparsed = impl->joystick->preparsed;
    LONG log_min, log_max, phy_min, phy_max;
    NTSTATUS status;

    if (!caps) return;

    log_min = caps->logical_min;
    log_max = caps->logical_max;
    phy_min = caps->physical_min;
    phy_max = caps->physical_max;

    if (value > phy_max || value < phy_min) value = -1;
    else value = log_min + (value - phy_min) * (log_max - log_min) / (phy_max - phy_min);
    status = HidP_SetUsageValue( HidP_Output, caps->usage_page, caps->link_collection,
                                 caps->usage_min, value, preparsed, report_buf, report_len );
    if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue %04x:%04x returned %#x\n",
                                             caps->usage_page, caps->usage_min, status );
}

static void set_parameter_value_us( struct hid_joystick_effect *impl, char *report_buf,
                                    struct hid_value_caps *caps, LONG value )
{
    LONG exp;
    if (!caps) return;
    exp = caps->units_exp;
    if (caps->units != 0x1003) WARN( "unknown time unit caps %x\n", caps->units );
    else if (exp < -6) while (exp++ < -6) value *= 10;
    else if (exp > -6) while (exp-- > -6) value /= 10;
    set_parameter_value( impl, report_buf, caps, value );
}

static HRESULT WINAPI hid_joystick_effect_Download( IDirectInputEffect *iface )
{
    static const DWORD complete_mask = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS;
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct pid_set_constant_force *set_constant_force = &impl->joystick->pid_set_constant_force;
    struct pid_set_ramp_force *set_ramp_force = &impl->joystick->pid_set_ramp_force;
    struct pid_effect_update *effect_update = &impl->joystick->pid_effect_update;
    struct pid_set_condition *set_condition = &impl->joystick->pid_set_condition;
    struct pid_set_periodic *set_periodic = &impl->joystick->pid_set_periodic;
    struct pid_set_envelope *set_envelope = &impl->joystick->pid_set_envelope;
    ULONG report_len = impl->joystick->caps.OutputReportByteLength;
    HANDLE device = impl->joystick->device;
    struct hid_value_caps *caps;
    DWORD i, tmp, count;
    NTSTATUS status;
    USAGE usage;
    HRESULT hr;

    TRACE( "iface %p\n", iface );

    EnterCriticalSection( &impl->joystick->base.crit );
    if (impl->modified) hr = DI_OK;
    else hr = DI_NOEFFECT;

    if (!impl->joystick->base.acquired || !(impl->joystick->base.dwCoopLevel & DISCL_EXCLUSIVE))
        hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else if ((impl->flags & complete_mask) != complete_mask)
        hr = DIERR_INCOMPLETEEFFECT;
    else if (!impl->index && !FAILED(hr = find_next_effect_id( impl->joystick, &impl->index )))
    {
        if (!impl->type_specific_buf[0][0]) status = HIDP_STATUS_SUCCESS;
        else status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                          impl->index, impl->joystick->preparsed, impl->type_specific_buf[0], report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue returned %#x\n", status );

        status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_EFFECT_BLOCK_INDEX,
                                     impl->index, impl->joystick->preparsed, impl->effect_update_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) hr = status;
        else hr = DI_OK;
    }

    if (hr == DI_OK)
    {
        switch (impl->type)
        {
        case PID_USAGE_ET_SQUARE:
        case PID_USAGE_ET_SINE:
        case PID_USAGE_ET_TRIANGLE:
        case PID_USAGE_ET_SAWTOOTH_UP:
        case PID_USAGE_ET_SAWTOOTH_DOWN:
            set_parameter_value( impl, impl->type_specific_buf[0], set_periodic->magnitude_caps,
                                 impl->periodic.dwMagnitude );
            set_parameter_value_us( impl, impl->type_specific_buf[0], set_periodic->period_caps,
                                    impl->periodic.dwPeriod );
            set_parameter_value( impl, impl->type_specific_buf[0], set_periodic->phase_caps,
                                 impl->periodic.dwPhase );
            set_parameter_value( impl, impl->type_specific_buf[0], set_periodic->offset_caps,
                                 impl->periodic.lOffset );

            if (WriteFile( device, impl->type_specific_buf[0], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;

            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->attack_level_caps,
                                 impl->envelope.dwAttackLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->attack_time_caps,
                                    impl->envelope.dwAttackTime );
            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->fade_level_caps,
                                 impl->envelope.dwFadeLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->fade_time_caps,
                                    impl->envelope.dwFadeTime );

            if (WriteFile( device, impl->type_specific_buf[1], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;
            break;
        case PID_USAGE_ET_SPRING:
        case PID_USAGE_ET_DAMPER:
        case PID_USAGE_ET_INERTIA:
        case PID_USAGE_ET_FRICTION:
            for (i = 0; i < min( 2, impl->params.cbTypeSpecificParams / sizeof(DICONDITION) ); ++i)
            {
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->center_point_offset_caps,
                                     impl->condition[i].lOffset );
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->positive_coefficient_caps,
                                     impl->condition[i].lPositiveCoefficient );
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->negative_coefficient_caps,
                                     impl->condition[i].lNegativeCoefficient );
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->positive_saturation_caps,
                                     impl->condition[i].dwPositiveSaturation );
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->negative_saturation_caps,
                                     impl->condition[i].dwNegativeSaturation );
                set_parameter_value( impl, impl->type_specific_buf[i], set_condition->dead_band_caps,
                                     impl->condition[i].lDeadBand );

                if (WriteFile( device, impl->type_specific_buf[i], report_len, NULL, NULL )) hr = DI_OK;
                else hr = DIERR_INPUTLOST;
            }
            break;
        case PID_USAGE_ET_CONSTANT_FORCE:
            set_parameter_value( impl, impl->type_specific_buf[0], set_constant_force->magnitude_caps,
                                 impl->constant_force.lMagnitude );

            if (WriteFile( device, impl->type_specific_buf[0], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;

            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->attack_level_caps,
                                 impl->envelope.dwAttackLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->attack_time_caps,
                                    impl->envelope.dwAttackTime );
            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->fade_level_caps,
                                 impl->envelope.dwFadeLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->fade_time_caps,
                                    impl->envelope.dwFadeTime );

            if (WriteFile( device, impl->type_specific_buf[1], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;
            break;
        case PID_USAGE_ET_RAMP:
            set_parameter_value( impl, impl->type_specific_buf[0], set_ramp_force->start_caps,
                                 impl->ramp_force.lStart );
            set_parameter_value( impl, impl->type_specific_buf[0], set_ramp_force->end_caps,
                                 impl->ramp_force.lEnd );

            if (WriteFile( device, impl->type_specific_buf[0], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;

            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->attack_level_caps,
                                 impl->envelope.dwAttackLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->attack_time_caps,
                                    impl->envelope.dwAttackTime );
            set_parameter_value( impl, impl->type_specific_buf[1], set_envelope->fade_level_caps,
                                 impl->envelope.dwFadeLevel );
            set_parameter_value_us( impl, impl->type_specific_buf[1], set_envelope->fade_time_caps,
                                    impl->envelope.dwFadeTime );

            if (WriteFile( device, impl->type_specific_buf[1], report_len, NULL, NULL )) hr = DI_OK;
            else hr = DIERR_INPUTLOST;
            break;
        }

        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->duration_caps,
                                impl->params.dwDuration );
        set_parameter_value( impl, impl->effect_update_buf, effect_update->gain_caps,
                             impl->params.dwGain );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->sample_period_caps,
                                impl->params.dwSamplePeriod );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->start_delay_caps,
                                impl->params.dwStartDelay );
        set_parameter_value_us( impl, impl->effect_update_buf, effect_update->trigger_repeat_interval_caps,
                                impl->params.dwTriggerRepeatInterval );

        if (impl->flags & DIEP_DIRECTION)
        {
            count = 1;
            usage = PID_USAGE_DIRECTION_ENABLE;
            status = HidP_SetUsages( HidP_Output, HID_USAGE_PAGE_PID, 0, &usage, &count,
                                     impl->joystick->preparsed, impl->effect_update_buf, report_len );
            if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsages returned %#x\n", status );

            if (!effect_update->direction_count) WARN( "no PID effect direction caps found\n" );
            else for (i = 0; i < impl->params.cAxes - 1; ++i)
            {
                tmp = impl->directions[i] + (i == 0 ? 9000 : 0);
                caps = effect_update->direction_caps[effect_update->direction_count - i - 1];
                set_parameter_value( impl, impl->effect_update_buf, caps, tmp % 36000 );
            }
        }

        status = HidP_SetUsageValue( HidP_Output, HID_USAGE_PAGE_PID, 0, PID_USAGE_TRIGGER_BUTTON,
                                     impl->params.dwTriggerButton, impl->joystick->preparsed,
                                     impl->effect_update_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_SetUsageValue returned %#x\n", status );

        if (WriteFile( device, impl->effect_update_buf, report_len, NULL, NULL )) hr = DI_OK;
        else hr = DIERR_INPUTLOST;

        impl->modified = FALSE;
    }
    LeaveCriticalSection( &impl->joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_Unload( IDirectInputEffect *iface )
{
    struct hid_joystick_effect *impl = impl_from_IDirectInputEffect( iface );
    struct hid_joystick *joystick = impl->joystick;
    HRESULT hr = DI_OK;

    TRACE( "iface %p\n", iface );

    EnterCriticalSection( &joystick->base.crit );
    if (!impl->index)
        hr = DI_NOEFFECT;
    else if (!FAILED(hr = IDirectInputEffect_Stop( iface )))
    {
        impl->joystick->effect_inuse[impl->index - 1] = FALSE;
        impl->index = 0;
    }
    LeaveCriticalSection( &joystick->base.crit );

    return hr;
}

static HRESULT WINAPI hid_joystick_effect_Escape( IDirectInputEffect *iface, DIEFFESCAPE *escape )
{
    FIXME( "iface %p, escape %p stub!\n", iface, escape );
    return DIERR_UNSUPPORTED;
}

static IDirectInputEffectVtbl hid_joystick_effect_vtbl =
{
    /*** IUnknown methods ***/
    hid_joystick_effect_QueryInterface,
    hid_joystick_effect_AddRef,
    hid_joystick_effect_Release,
    /*** IDirectInputEffect methods ***/
    hid_joystick_effect_Initialize,
    hid_joystick_effect_GetEffectGuid,
    hid_joystick_effect_GetParameters,
    hid_joystick_effect_SetParameters,
    hid_joystick_effect_Start,
    hid_joystick_effect_Stop,
    hid_joystick_effect_GetEffectStatus,
    hid_joystick_effect_Download,
    hid_joystick_effect_Unload,
    hid_joystick_effect_Escape,
};

static HRESULT hid_joystick_effect_create( struct hid_joystick *joystick, IDirectInputEffect **out )
{
    struct hid_joystick_effect *impl;
    ULONG report_len;

    if (!(impl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*impl) )))
        return DIERR_OUTOFMEMORY;
    impl->IDirectInputEffect_iface.lpVtbl = &hid_joystick_effect_vtbl;
    impl->ref = 1;
    impl->joystick = joystick;
    hid_joystick_private_incref( joystick );

    EnterCriticalSection( &joystick->base.crit );
    list_add_tail( &joystick->effect_list, &impl->entry );
    LeaveCriticalSection( &joystick->base.crit );

    report_len = joystick->caps.OutputReportByteLength;
    if (!(impl->effect_control_buf = HeapAlloc( GetProcessHeap(), 0, report_len ))) goto failed;
    if (!(impl->effect_update_buf = HeapAlloc( GetProcessHeap(), 0, report_len ))) goto failed;
    if (!(impl->type_specific_buf[0] = HeapAlloc( GetProcessHeap(), 0, report_len ))) goto failed;
    if (!(impl->type_specific_buf[1] = HeapAlloc( GetProcessHeap(), 0, report_len ))) goto failed;

    impl->envelope.dwSize = sizeof(DIENVELOPE);
    impl->params.dwSize = sizeof(DIEFFECT);
    impl->params.lpEnvelope = &impl->envelope;
    impl->params.rgdwAxes = impl->axes;
    impl->params.rglDirection = impl->directions;
    impl->params.dwTriggerButton = -1;

    *out = &impl->IDirectInputEffect_iface;
    return DI_OK;

failed:
    IDirectInputEffect_Release( &impl->IDirectInputEffect_iface );
    return DIERR_OUTOFMEMORY;
}
