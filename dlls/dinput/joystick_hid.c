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

#include "dinput_private.h"
#include "device_private.h"
#include "joystick_private.h"

#include "initguid.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

DEFINE_GUID( hid_joystick_guid, 0x9e573edb, 0x7734, 0x11d2, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7 );
DEFINE_DEVPROPKEY( DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2 );

static inline const char *debugstr_hidp_link_collection_node( HIDP_LINK_COLLECTION_NODE *node )
{
    if (!node) return "(null)";
    return wine_dbg_sprintf( "Usg %02x:%02x Par %u Nxt %u Cnt %u Chld %u Type %u Alias %d User %p",
                             node->LinkUsagePage, node->LinkUsage, node->Parent, node->NextSibling,
                             node->NumberOfChildren, node->FirstChild, node->CollectionType, node->IsAlias,
                             node->UserContext );
}

static inline const char *debugstr_hidp_button_caps( HIDP_BUTTON_CAPS *caps )
{
    const char *str;
    if (!caps) return "(null)";

    str = wine_dbg_sprintf( "RId %d,", caps->ReportID );
    if (!caps->IsRange)
        str = wine_dbg_sprintf( "%s Usg %02x:%02x Dat %02x,", str, caps->UsagePage, caps->NotRange.Usage,
                                caps->NotRange.DataIndex );
    else
        str = wine_dbg_sprintf( "%s Usg %02x:%02x-%02x Dat %02x-%02x,", str, caps->UsagePage, caps->Range.UsageMin,
                                caps->Range.UsageMax, caps->Range.DataIndexMin, caps->Range.DataIndexMax );
    if (!caps->IsStringRange)
        str = wine_dbg_sprintf( "%s Str %d,", str, caps->NotRange.StringIndex );
    else
        str = wine_dbg_sprintf( "%s Str %d-%d,", str, caps->Range.StringMin, caps->Range.StringMax );
    if (!caps->IsDesignatorRange)
        str = wine_dbg_sprintf( "%s Des %d,", str, caps->NotRange.DesignatorIndex );
    else
        str = wine_dbg_sprintf( "%s Des %d-%d,", str, caps->Range.DesignatorMin, caps->Range.DesignatorMax );
    return wine_dbg_sprintf( "%s Bits %02x Alias %d Abs %d, LCol %u LUsg %02x-%02x", str, caps->BitField, caps->IsAlias,
                             caps->IsAbsolute, caps->LinkCollection, caps->LinkUsagePage, caps->LinkUsage );
}

static inline const char *debugstr_hidp_value_caps( HIDP_VALUE_CAPS *caps )
{
    const char *str;
    if (!caps) return "(null)";

    str = wine_dbg_sprintf( "RId %d,", caps->ReportID );
    if (!caps->IsRange)
        str = wine_dbg_sprintf( "%s Usg %02x:%02x Dat %02x,", str, caps->UsagePage, caps->NotRange.Usage,
                                caps->NotRange.DataIndex );
    else
        str = wine_dbg_sprintf( "%s Usg %02x:%02x-%02x Dat %02x-%02x,", str, caps->UsagePage, caps->Range.UsageMin,
                                caps->Range.UsageMax, caps->Range.DataIndexMin, caps->Range.DataIndexMax );
    if (!caps->IsStringRange)
        str = wine_dbg_sprintf( "%s Str %d,", str, caps->NotRange.StringIndex );
    else
        str = wine_dbg_sprintf( "%s Str %d-%d,", str, caps->Range.StringMin, caps->Range.StringMax );
    if (!caps->IsDesignatorRange)
        str = wine_dbg_sprintf( "%s Des %d,", str, caps->NotRange.DesignatorIndex );
    else
        str = wine_dbg_sprintf( "%s Des %d-%d,", str, caps->Range.DesignatorMin, caps->Range.DesignatorMax );
    return wine_dbg_sprintf( "%s Bits %02x Alias %d Abs %d Null %d, LCol %u LUsg %02x-%02x, "
                             "BitSz %d, RCnt %d, Unit %x E%+d, Log %+d-%+d, Phy %+d-%+d",
                             str, caps->BitField, caps->IsAlias, caps->IsAbsolute, caps->HasNull,
                             caps->LinkCollection, caps->LinkUsagePage, caps->LinkUsage,
                             caps->BitSize, caps->ReportCount, caps->Units, caps->UnitsExp,
                             caps->LogicalMin, caps->LogicalMax, caps->PhysicalMin, caps->PhysicalMax );
}

struct hid_caps
{
    enum { LINK_COLLECTION_NODE, BUTTON_CAPS, VALUE_CAPS } type;
    union
    {
        HIDP_LINK_COLLECTION_NODE *node;
        HIDP_BUTTON_CAPS *button;
        HIDP_VALUE_CAPS *value;
    };
};

static inline const char *debugstr_hid_caps( struct hid_caps *caps )
{
    switch (caps->type)
    {
    case LINK_COLLECTION_NODE:
        return debugstr_hidp_link_collection_node( caps->node );
    case BUTTON_CAPS:
        return debugstr_hidp_button_caps( caps->button );
    case VALUE_CAPS:
        return debugstr_hidp_value_caps( caps->value );
    }

    return "(unknown type)";
}

struct hid_joystick
{
    IDirectInputDeviceImpl base;
    DIJOYSTATE2 state;

    HANDLE device;
    OVERLAPPED read_ovl;
    PHIDP_PREPARSED_DATA preparsed;

    DIDEVICEINSTANCEW instance;
    WCHAR device_path[MAX_PATH];
    HIDD_ATTRIBUTES attrs;
    DIDEVCAPS dev_caps;
    HIDP_CAPS caps;

    HIDP_LINK_COLLECTION_NODE *collection_nodes;
    HIDP_BUTTON_CAPS *input_button_caps;
    HIDP_VALUE_CAPS *input_value_caps;

    char *input_report_buf;
    USAGE_AND_PAGE *usages_buf;
    ULONG usages_count;

    BYTE device_state_report_id;
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

typedef BOOL (*enum_object_callback)( struct hid_joystick *impl, struct hid_caps *caps, DIDEVICEOBJECTINSTANCEW *instance, void *data );

static BOOL enum_object( struct hid_joystick *impl, const DIPROPHEADER *filter, DWORD flags,
                         enum_object_callback callback, struct hid_caps *caps,
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
        if (LOWORD(filter->dwObj) != instance->wUsagePage || HIWORD(filter->dwObj) != instance->wUsage) return DIENUM_CONTINUE;
        return callback( impl, caps, instance, data );
    default:
        FIXME( "unimplemented filter dwHow %#x dwObj %#x\n", filter->dwHow, filter->dwObj );
        break;
    }

    return DIENUM_CONTINUE;
}

static BOOL enum_value_objects( struct hid_joystick *impl, const DIPROPHEADER *filter,
                                DWORD flags, enum_object_callback callback, void *data )
{
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    struct hid_caps caps = {.type = VALUE_CAPS};
    DWORD axis = 0, pov = 0, i;
    BOOL ret;

    for (i = 0; i < impl->caps.NumberInputValueCaps; ++i)
    {
        caps.value = impl->input_value_caps + i;

        if (caps.value->UsagePage >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            TRACE( "Ignoring input value %s, vendor specific.\n", debugstr_hid_caps( &caps ) );
        else if (caps.value->IsAlias)
            TRACE( "Ignoring input value %s, aliased.\n", debugstr_hid_caps( &caps ) );
        else if (caps.value->IsRange)
            FIXME( "Ignoring input value %s, usage range not implemented.\n", debugstr_hid_caps( &caps ) );
        else if (caps.value->ReportCount > 1)
            FIXME( "Ignoring input value %s, array not implemented.\n", debugstr_hid_caps( &caps ) );
        else if (caps.value->UsagePage != HID_USAGE_PAGE_GENERIC)
            TRACE( "Ignoring input value %s, usage page not implemented.\n", debugstr_hid_caps( &caps ) );
        else
        {
            instance.wUsagePage = caps.value->UsagePage;
            instance.wUsage = caps.value->NotRange.Usage;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps.value->ReportID;

            switch (instance.wUsage)
            {
            case HID_USAGE_GENERIC_X:
                instance.dwOfs = DIJOFS_X;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_Y:
                instance.dwOfs = DIJOFS_Y;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_Z:
                instance.dwOfs = DIJOFS_Z;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_RX:
                instance.dwOfs = DIJOFS_RX;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_RY:
                instance.dwOfs = DIJOFS_RY;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_RZ:
                instance.dwOfs = DIJOFS_RZ;
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_SLIDER:
                instance.dwOfs = DIJOFS_SLIDER( 0 );
                instance.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( axis++ );
                instance.dwFlags = DIDOI_ASPECTPOSITION;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            case HID_USAGE_GENERIC_HATSWITCH:
                instance.dwOfs = DIJOFS_POV( 0 );
                instance.dwType = DIDFT_POV | DIDFT_MAKEINSTANCE( pov++ );
                instance.dwFlags = 0;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
                break;
            default:
                FIXME( "Ignoring input value %s, usage not implemented.\n", debugstr_hid_caps( &caps ) );
                break;
            }
        }
    }

    return DIENUM_CONTINUE;
}

static BOOL enum_button_objects( struct hid_joystick *impl, const DIPROPHEADER *filter,
                                 DWORD flags, enum_object_callback callback, void *data )
{
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    struct hid_caps caps = {.type = BUTTON_CAPS};
    DWORD button = 0, i, j;
    BOOL ret;

    for (i = 0; i < impl->caps.NumberInputButtonCaps; ++i)
    {
        caps.button = impl->input_button_caps + i;

        if (caps.button->UsagePage >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            TRACE( "Ignoring input button %s, vendor specific.\n", debugstr_hid_caps( &caps ) );
        else if (caps.button->IsAlias)
            TRACE( "Ignoring input button %s, aliased.\n", debugstr_hid_caps( &caps ) );
        else if (caps.button->UsagePage != HID_USAGE_PAGE_BUTTON)
            TRACE( "Ignoring input button %s, usage page not implemented.\n", debugstr_hid_caps( &caps ) );
        else if (caps.button->IsRange)
        {
            if (caps.button->NotRange.Usage >= 128)
                FIXME( "Ignoring input button %s, too many buttons.\n", debugstr_hid_caps( &caps ) );
            else for (j = caps.button->Range.UsageMin; j <= caps.button->Range.UsageMax; ++j)
            {
                instance.dwOfs = DIJOFS_BUTTON( j - 1 );
                instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( button++ );
                instance.dwFlags = 0;
                instance.wUsagePage = caps.button->UsagePage;
                instance.wUsage = j;
                instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
                instance.wReportId = caps.button->ReportID;
                ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
                if (ret != DIENUM_CONTINUE) return ret;
            }
        }
        else if (caps.button->NotRange.Usage >= 128)
            FIXME( "Ignoring input button %s, too many buttons.\n", debugstr_hid_caps( &caps ) );
        else
        {
            instance.dwOfs = DIJOFS_BUTTON( caps.button->NotRange.Usage - 1 );
            instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( button++ );
            instance.dwFlags = 0;
            instance.wUsagePage = caps.button->UsagePage;
            instance.wUsage = caps.button->NotRange.Usage;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = caps.button->ReportID;
            ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
            if (ret != DIENUM_CONTINUE) return ret;
        }
    }

    return DIENUM_CONTINUE;
}

static BOOL enum_collections_objects( struct hid_joystick *impl, const DIPROPHEADER *filter, DWORD flags,
                                      enum_object_callback callback, void *data )
{
    DIDEVICEOBJECTINSTANCEW instance = {.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW)};
    struct hid_caps caps = {.type = LINK_COLLECTION_NODE};
    BOOL ret;
    DWORD i;

    for (i = 0; i < impl->caps.NumberLinkCollectionNodes; ++i)
    {
        caps.node = impl->collection_nodes + i;

        if (caps.node->LinkUsagePage >= HID_USAGE_PAGE_VENDOR_DEFINED_BEGIN)
            TRACE( "Ignoring collection %s, vendor specific.\n", debugstr_hid_caps( &caps ) );
        else if (caps.node->IsAlias)
            TRACE( "Ignoring collection %s, aliased.\n", debugstr_hid_caps( &caps ) );
        else
        {
            instance.dwOfs = 0;
            instance.dwType = DIDFT_COLLECTION | DIDFT_NODATA;
            instance.dwFlags = 0;
            instance.wUsagePage = caps.node->LinkUsagePage;
            instance.wUsage = caps.node->LinkUsage;
            instance.guidType = *object_usage_to_guid( instance.wUsagePage, instance.wUsage );
            instance.wReportId = 0;
            ret = enum_object( impl, filter, flags, callback, &caps, &instance, data );
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
        HeapFree( GetProcessHeap(), 0, tmp.input_value_caps );
        HeapFree( GetProcessHeap(), 0, tmp.input_button_caps );
        HeapFree( GetProcessHeap(), 0, tmp.collection_nodes );
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

static BOOL enum_objects_callback( struct hid_joystick *impl, struct hid_caps *caps,
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

    ret = enum_value_objects( impl, &filter, flags, enum_objects_callback, &params );
    if (ret != DIENUM_CONTINUE) return S_OK;
    ret = enum_button_objects( impl, &filter, flags, enum_objects_callback, &params );
    if (ret != DIENUM_CONTINUE) return S_OK;
    enum_collections_objects( impl, &filter, flags, enum_objects_callback, &params );

    return S_OK;
}

static BOOL get_property_prop_range( struct hid_joystick *impl, struct hid_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    HIDP_VALUE_CAPS *value_caps = caps->value;
    DIPROPRANGE *value = data;
    value->lMin = value_caps->PhysicalMin;
    value->lMax = value_caps->PhysicalMax;
    return DIENUM_CONTINUE;
}

static HRESULT WINAPI hid_joystick_GetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                DIPROPHEADER *header )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
        enum_value_objects( impl, header, DIDFT_AXIS, get_property_prop_range, header );
        return DI_OK;
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, impl->instance.tszProductName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, impl->instance.tszInstanceName, MAX_PATH );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_VIDPID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (!impl->attrs.VendorID || !impl->attrs.ProductID) return DIERR_UNSUPPORTED;
        value->dwData = MAKELONG( impl->attrs.VendorID, impl->attrs.ProductID );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->instance.guidInstance.Data3;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    {
        DIPROPGUIDANDPATH *value = (DIPROPGUIDANDPATH *)header;
        lstrcpynW( value->wszPath, impl->device_path, MAX_PATH );
        return DI_OK;
    }
    default:
        return IDirectInputDevice2WImpl_GetProperty( iface, guid, header );
    }

    return DI_OK;
}

static BOOL set_property_prop_range( struct hid_joystick *impl, struct hid_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    HIDP_VALUE_CAPS *value_caps = caps->value;
    DIPROPRANGE *value = data;
    LONG range = value_caps->LogicalMax - value_caps->LogicalMin;
    value_caps->PhysicalMin = value->lMin;
    value_caps->PhysicalMax = value->lMax;
    if (instance->dwType & DIDFT_POV && range > 0)
        value_caps->PhysicalMax -= value->lMax / (range + 1);
    return DIENUM_CONTINUE;
}

static HRESULT WINAPI hid_joystick_SetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                const DIPROPHEADER *header )
{
    struct hid_joystick *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
        enum_value_objects( impl, header, DIDFT_AXIS, set_property_prop_range, (void *)header );
        return DI_OK;
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

    if (!ptr) return DIERR_INVALIDPARAM;

    fill_DataFormat( ptr, len, &impl->state, &impl->base.data_format );

    return DI_OK;
}

static BOOL get_object_info( struct hid_joystick *impl, struct hid_caps *caps,
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

    ret = enum_value_objects( impl, &filter, DIDFT_ALL, get_object_info, NULL );
    if (ret != DIENUM_CONTINUE) return S_OK;
    ret = enum_button_objects( impl, &filter, DIDFT_ALL, get_object_info, NULL );
    if (ret != DIENUM_CONTINUE) return S_OK;
    enum_collections_objects( impl, &filter, DIDFT_ALL, get_object_info, NULL );

    return S_OK;
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
    IDirectInputDevice2WImpl_Poll,
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
    DIJOYSTATE2 old_state;
    DWORD time;
    DWORD seq;
};

static BOOL check_device_state_button( struct hid_joystick *impl, struct hid_caps *caps,
                                       DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    struct parse_device_state_params *params = data;
    DWORD i = DIDFT_GETINSTANCE( instance->dwType );

    if (!(instance->dwType & DIDFT_BUTTON))
        FIXME( "unexpected object type %#x, expected DIDFT_BUTTON\n", instance->dwType );
    else if (params->old_state.rgbButtons[i] != impl->state.rgbButtons[i])
        queue_event( iface, instance->dwType, impl->state.rgbButtons[i], params->time, params->seq );

    return DIENUM_CONTINUE;
}

static LONG sign_extend( ULONG value, const HIDP_VALUE_CAPS *caps )
{
    UINT sign = 1 << (caps->BitSize - 1);
    if (sign <= 1 || caps->LogicalMin >= 0) return value;
    return value - ((value & sign) << 1);
}

static LONG scale_value( ULONG value, const HIDP_VALUE_CAPS *caps, LONG min, LONG max )
{
    ULONG bit_max = (1 << caps->BitSize) - 1;
    LONG tmp = sign_extend( value, caps );

    /* xinput HID gamepad have bogus logical value range, let's use the bit range instead */
    if (caps->LogicalMin == 0 && caps->LogicalMax == -1) return min + MulDiv( tmp, max - min, bit_max );
    if (caps->LogicalMin > tmp || caps->LogicalMax < tmp) return -1; /* invalid / null value */
    return min + MulDiv( tmp - caps->LogicalMin, max - min, caps->LogicalMax - caps->LogicalMin );
}

static BOOL read_device_state_value( struct hid_joystick *impl, struct hid_caps *caps,
                                     DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    IDirectInputDevice8W *iface = &impl->base.IDirectInputDevice8W_iface;
    ULONG logical_value, report_len = impl->caps.InputReportByteLength;
    struct parse_device_state_params *params = data;
    char *report_buf = impl->input_report_buf;
    HIDP_VALUE_CAPS *value_caps = caps->value;
    NTSTATUS status;
    LONG value;

    if (!(instance->dwType & (DIDFT_POV | DIDFT_AXIS)))
        FIXME( "unexpected object type %#x, expected DIDFT_POV | DIDFT_AXIS\n", instance->dwType );
    else
    {
        status = HidP_GetUsageValue( HidP_Input, instance->wUsagePage, 0, instance->wUsage,
                                     &logical_value, impl->preparsed, report_buf, report_len );
        if (status != HIDP_STATUS_SUCCESS) WARN( "HidP_GetUsageValue %04x:%04x returned %#x\n",
                                                 instance->wUsagePage, instance->wUsage, status );
        value = scale_value( logical_value, value_caps, value_caps->PhysicalMin, value_caps->PhysicalMax );

        switch (instance->dwOfs)
        {
        case DIJOFS_X:
            if (impl->state.lX == value) break;
            impl->state.lX = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_Y:
            if (impl->state.lY == value) break;
            impl->state.lY = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_Z:
            if (impl->state.lZ == value) break;
            impl->state.lZ = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_RX:
            if (impl->state.lRx == value) break;
            impl->state.lRx = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_RY:
            if (impl->state.lRy == value) break;
            impl->state.lRy = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_RZ:
            if (impl->state.lRz == value) break;
            impl->state.lRz = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        case DIJOFS_POV( 0 ):
            if (impl->state.rgdwPOV[0] == value) break;
            impl->state.rgdwPOV[0] = value;
            queue_event( iface, instance->dwType, value, params->time, params->seq );
            break;
        default:
            FIXME( "unimplemented offset %#x.\n", instance->dwOfs );
            break;
        }
    }

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
    struct parse_device_state_params params = {0};
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
            params.old_state = impl->state;
            params.time = GetCurrentTime();
            params.seq = impl->base.dinput->evsequence++;

            memset( impl->state.rgbButtons, 0, sizeof(impl->state.rgbButtons) );
            while (count--)
            {
                usages = impl->usages_buf + count;
                if (usages->UsagePage != HID_USAGE_PAGE_BUTTON)
                    FIXME( "unimplemented usage page %x.\n", usages->UsagePage );
                else if (usages->Usage >= 128)
                    FIXME( "ignoring extraneous button %d.\n", usages->Usage );
                else
                    impl->state.rgbButtons[usages->Usage - 1] = 0x80;
            }

            enum_value_objects( impl, &filter, DIDFT_ALL, read_device_state_value, &params );
            enum_button_objects( impl, &filter, DIDFT_ALL, check_device_state_button, &params );
        }

        memset( &impl->read_ovl, 0, sizeof(impl->read_ovl) );
        impl->read_ovl.hEvent = impl->base.read_event;
    } while (ReadFile( impl->device, report_buf, report_len, &count, &impl->read_ovl ));

    return DI_OK;
}

static BOOL hid_joystick_device_try_open( UINT32 handle, const WCHAR *path, HANDLE *device,
                                          PHIDP_PREPARSED_DATA *preparsed, HIDD_ATTRIBUTES *attrs,
                                          HIDP_CAPS *caps, DIDEVICEINSTANCEW *instance, DWORD version )
{
    PHIDP_PREPARSED_DATA preparsed_data = NULL;
    HANDLE device_file;

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
    instance->dwDevType = get_device_type( version, caps->Usage != HID_USAGE_GENERIC_GAMEPAD ) | DIDEVTYPE_HID;
    instance->guidFFDriver = GUID_NULL;
    instance->wUsagePage = caps->UsagePage;
    instance->wUsage = caps->Usage;

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
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {.cbSize = sizeof(iface)};
    SP_DEVINFO_DATA devinfo = {.cbSize = sizeof(devinfo)};
    DIDEVICEINSTANCEW instance = *filter;
    UINT32 i = 0, handle;
    HDEVINFO set;
    DWORD type;
    GUID hid;

    TRACE( "index %d, product %s, instance %s\n", index, debugstr_guid( &filter->guidProduct ),
           debugstr_guid( &filter->guidInstance ) );

    HidD_GetHidGuid( &hid );

    set = SetupDiGetClassDevsW( &hid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    if (set == INVALID_HANDLE_VALUE) return DIERR_DEVICENOTREG;

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

        /* enumerate device by GUID */
        if (index < 0 && IsEqualGUID( &filter->guidProduct, &instance.guidProduct )) break;
        if (index < 0 && IsEqualGUID( &filter->guidInstance, &instance.guidInstance )) break;

        /* enumerate all devices */
        if (index >= 0 && !index--) break;

        CloseHandle( *device );
        HidD_FreePreparsedData( *preparsed );
        *device = NULL;
        *preparsed = NULL;
    }

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

    if (device_disabled_registry( "HID", TRUE ))
        return DIERR_DEVICENOTREG;

    TRACE( "found device %s, usage %04x:%04x, product %s, instance %s, name %s\n", debugstr_w(device_path),
           instance->wUsagePage, instance->wUsage, debugstr_guid( &instance->guidProduct ),
           debugstr_guid( &instance->guidInstance ), debugstr_w(instance->tszInstanceName) );

    return DI_OK;
}

static BOOL init_objects( struct hid_joystick *impl, struct hid_caps *caps,
                          DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *format = impl->base.data_format.wine_df;

    format->dwNumObjs++;
    if (instance->dwType & DIDFT_PSHBUTTON) impl->dev_caps.dwButtons++;
    if (instance->dwType & DIDFT_AXIS) impl->dev_caps.dwAxes++;
    if (instance->dwType & DIDFT_POV) impl->dev_caps.dwPOVs++;

    if (!impl->device_state_report_id)
        impl->device_state_report_id = instance->wReportId;
    else if (impl->device_state_report_id != instance->wReportId)
        FIXME( "multiple device state reports found!\n" );

    return DIENUM_CONTINUE;
}

static BOOL init_data_format( struct hid_joystick *impl, struct hid_caps *caps,
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
    HIDD_ATTRIBUTES attrs = {.Size = sizeof(attrs)};
    HIDP_LINK_COLLECTION_NODE *nodes;
    struct hid_joystick *impl = NULL;
    DIDATAFORMAT *format = NULL;
    HIDP_BUTTON_CAPS *buttons;
    HIDP_VALUE_CAPS *values;
    USAGE_AND_PAGE *usages;
    DWORD size, index;
    NTSTATUS status;
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

    size = impl->caps.NumberLinkCollectionNodes * sizeof(HIDP_LINK_COLLECTION_NODE);
    if (!(nodes = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->collection_nodes = nodes;
    size = impl->caps.NumberInputButtonCaps * sizeof(HIDP_BUTTON_CAPS);
    if (!(buttons = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->input_button_caps = buttons;
    size = impl->caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS);
    if (!(values = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->input_value_caps = values;

    size = impl->caps.InputReportByteLength;
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->input_report_buf = buffer;
    impl->usages_count = HidP_MaxUsageListLength( HidP_Input, 0, impl->preparsed );
    size = impl->usages_count * sizeof(USAGE_AND_PAGE);
    if (!(usages = HeapAlloc( GetProcessHeap(), 0, size ))) goto failed;
    impl->usages_buf = usages;

    size = impl->caps.NumberLinkCollectionNodes;
    status = HidP_GetLinkCollectionNodes( nodes, &size, impl->preparsed );
    if (status != HIDP_STATUS_SUCCESS) goto failed;
    impl->caps.NumberLinkCollectionNodes = size;
    status = HidP_GetButtonCaps( HidP_Input, impl->input_button_caps,
                                 &impl->caps.NumberInputButtonCaps, impl->preparsed );
    if (status != HIDP_STATUS_SUCCESS && status != HIDP_STATUS_USAGE_NOT_FOUND) goto failed;
    status = HidP_GetValueCaps( HidP_Input, impl->input_value_caps,
                                 &impl->caps.NumberInputValueCaps, impl->preparsed );
    if (status != HIDP_STATUS_SUCCESS && status != HIDP_STATUS_USAGE_NOT_FOUND) goto failed;

    /* enumerate collections first, so we can find report collections */
    enum_collections_objects( impl, &filter, DIDFT_ALL, init_objects, NULL );
    enum_value_objects( impl, &filter, DIDFT_ALL, init_objects, NULL );
    enum_button_objects( impl, &filter, DIDFT_ALL, init_objects, NULL );

    format = impl->base.data_format.wine_df;
    size = format->dwNumObjs * sizeof(*format->rgodf);
    if (!(format->rgodf = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size ))) goto failed;
    format->dwSize = sizeof(*format);
    format->dwObjSize = sizeof(*format->rgodf);
    format->dwFlags = DIDF_ABSAXIS;
    format->dwDataSize = sizeof(impl->state);

    index = 0;
    enum_value_objects( impl, &filter, DIDFT_ALL, init_data_format, &index );
    enum_button_objects( impl, &filter, DIDFT_ALL, init_data_format, &index );
    enum_collections_objects( impl, &filter, DIDFT_ALL, init_data_format, &index );

    _dump_DIDATAFORMAT( impl->base.data_format.wine_df );

    range.lMax = 65535;
    enum_value_objects( impl, &range.diph, DIDFT_AXIS, set_property_prop_range, &range );
    range.lMax = 36000;
    enum_value_objects( impl, &range.diph, DIDFT_POV, set_property_prop_range, &range );

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
