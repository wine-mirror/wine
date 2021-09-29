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

#define DEVICE_STATE_MAX_SIZE 1024

struct hid_joystick
{
    IDirectInputDeviceImpl base;

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
    USAGE_AND_PAGE *usages_buf;
    ULONG usages_count;

    BYTE device_state_report_id;
    BYTE device_state[DEVICE_STATE_MAX_SIZE];
};

static inline struct hid_joystick *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( CONTAINING_RECORD( iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface ),
                              struct hid_joystick, base );
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
    struct hid_value_caps *caps, *caps_end, *nary, *nary_end;
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
            ret = enum_object( impl, &filter, flags, callback, caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
            value_ofs += sizeof(LONG);
            object++;
        }
    }

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
            instance.wUsagePage = caps->usage_page;
            instance.wUsage = j;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps->report_id;
            instance.wCollectionNumber = caps->link_collection;
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
            ret = enum_object( impl, &filter, flags, callback, NULL, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
        }
    }

    return DIENUM_CONTINUE;
}

static ULONG WINAPI hid_joystick_Release( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    struct hid_joystick tmp = *impl;
    ULONG ref;

    if (!(ref = IDirectInputDevice2WImpl_Release( iface )))
    {
        HeapFree( GetProcessHeap(), 0, tmp.usages_buf );
        HeapFree( GetProcessHeap(), 0, tmp.input_report_buf );
        HeapFree( GetProcessHeap(), 0, tmp.input_extra_caps );
        HidD_FreePreparsedData( tmp.preparsed );
        CancelIoEx( tmp.device, &tmp.read_ovl );
        CloseHandle( tmp.base.read_event );
        CloseHandle( tmp.device );
    }

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
    if (flags & ~(DIDFT_AXIS | DIDFT_AXIS | DIDFT_BUTTON | DIDFT_NODATA | DIDFT_COLLECTION))
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
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    if ((hr = IDirectInputDevice2WImpl_Acquire( iface )) != DI_OK) return hr;

    memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
    impl->read_ovl.hEvent = impl->base.read_event;
    if (ReadFile( impl->device, impl->input_report_buf, report_len, NULL, &impl->read_ovl ))
        impl->base.read_callback( iface );

    return DI_OK;
}

static HRESULT WINAPI hid_joystick_Unacquire( IDirectInputDevice8W *iface )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;
    BOOL ret;

    TRACE( "iface %p.\n", iface );

    if ((hr = IDirectInputDevice2WImpl_Unacquire( iface )) != DI_OK) return hr;

    ret = CancelIoEx( impl->device, &impl->read_ovl );
    if (!ret) WARN( "CancelIoEx failed, last error %u\n", GetLastError() );

    return DI_OK;
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
    IDirectInputDevice2WImpl_AddRef,
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
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
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
    BOOL ret;

    ret = GetOverlappedResult( impl->device, &impl->read_ovl, &count, FALSE );
    if (!ret) WARN( "ReadFile failed, error %u\n", GetLastError() );
    else if (TRACE_ON(dinput))
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

    do
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
    } while (ReadFile( impl->device, report_buf, report_len, &count, &impl->read_ovl ));

    return DI_OK;
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
    impl->base.read_event = CreateEventA( NULL, FALSE, FALSE, NULL );
    impl->base.read_callback = hid_joystick_read_state;

    hr = hid_joystick_device_open( -1, &instance, impl->device_path, &impl->device, &impl->preparsed,
                                   &attrs, &impl->caps, dinput->dwVersion );
    if (hr != DI_OK) goto failed;

    impl->instance = instance;
    impl->attrs = attrs;
    impl->dev_caps.dwSize = sizeof(impl->dev_caps);
    impl->dev_caps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    impl->dev_caps.dwDevType = instance.dwDevType;

    preparsed = (struct hid_preparsed_data *)impl->preparsed;

    size = preparsed->input_caps_count * sizeof(struct extra_caps);
    if (!(extra = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto failed;
    impl->input_extra_caps = extra;

    size = impl->caps.InputReportByteLength;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->input_report_buf = buffer;
    impl->usages_count = HidP_MaxUsageListLength( HidP_Input, 0, impl->preparsed );
    size = impl->usages_count * sizeof(USAGE_AND_PAGE);
    if (!(usages = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->usages_buf = usages;

    enum_objects( impl, &filter, DIDFT_ALL, init_objects, NULL );

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
