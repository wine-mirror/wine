/*		DirectInput Device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
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

/* This file contains all the Device specific functions that can be used as stubs
   by real device implementations.

   It also contains all the helper functions.
*/

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"
#include "dinputd.h"
#include "hidusage.h"

#include "initguid.h"
#include "device_private.h"
#include "dinput_private.h"

#include "wine/debug.h"

#define WM_WINE_NOTIFY_ACTIVITY WM_USER

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Windows uses this GUID for guidProduct on non-keyboard/mouse devices.
 * Data1 contains the device VID (low word) and PID (high word).
 * Data4 ends with the ASCII bytes "PIDVID".
 */
DEFINE_GUID( dinput_pidvid_guid, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D' );

static inline IDirectInputDeviceImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface);
}

static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl(IDirectInputDeviceImpl *This)
{
    return &This->IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl(IDirectInputDeviceImpl *This)
{
    return &This->IDirectInputDevice8W_iface;
}

/******************************************************************************
 *	Various debugging tools
 */
static void _dump_cooperativelevel_DI(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DISCL_BACKGROUND),
	    FE(DISCL_EXCLUSIVE),
	    FE(DISCL_FOREGROUND),
	    FE(DISCL_NONEXCLUSIVE),
	    FE(DISCL_NOWINKEY)
#undef FE
	};
	TRACE(" cooperative level : ");
	for (i = 0; i < ARRAY_SIZE(flags); i++)
	    if (flags[i].mask & dwFlags)
		TRACE("%s ",flags[i].name);
	TRACE("\n");
    }
}

static void _dump_ObjectDataFormat_flags(DWORD dwFlags) {
    unsigned int   i;
    static const struct {
        DWORD       mask;
        const char  *name;
    } flags[] = {
#define FE(x) { x, #x}
        FE(DIDOI_FFACTUATOR),
        FE(DIDOI_FFEFFECTTRIGGER),
        FE(DIDOI_POLLED),
        FE(DIDOI_GUIDISUSAGE)
#undef FE
    };

    if (!dwFlags) return;

    TRACE("Flags:");

    /* First the flags */
    for (i = 0; i < ARRAY_SIZE(flags); i++) {
        if (flags[i].mask & dwFlags)
        TRACE(" %s",flags[i].name);
    }

    /* Now specific values */
#define FE(x) case x: TRACE(" "#x); break
    switch (dwFlags & DIDOI_ASPECTMASK) {
        FE(DIDOI_ASPECTACCEL);
        FE(DIDOI_ASPECTFORCE);
        FE(DIDOI_ASPECTPOSITION);
        FE(DIDOI_ASPECTVELOCITY);
    }
#undef FE

}

static void _dump_EnumObjects_flags(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	DWORD type, instance;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DIDFT_RELAXIS),
	    FE(DIDFT_ABSAXIS),
	    FE(DIDFT_PSHBUTTON),
	    FE(DIDFT_TGLBUTTON),
	    FE(DIDFT_POV),
	    FE(DIDFT_COLLECTION),
	    FE(DIDFT_NODATA),	    
	    FE(DIDFT_FFACTUATOR),
	    FE(DIDFT_FFEFFECTTRIGGER),
	    FE(DIDFT_OUTPUT),
	    FE(DIDFT_VENDORDEFINED),
	    FE(DIDFT_ALIAS),
	    FE(DIDFT_OPTIONAL)
#undef FE
	};
	type = (dwFlags & 0xFF0000FF);
	instance = ((dwFlags >> 8) & 0xFFFF);
	TRACE("Type:");
	if (type == DIDFT_ALL) {
	    TRACE(" DIDFT_ALL");
	} else {
	    for (i = 0; i < ARRAY_SIZE(flags); i++) {
		if (flags[i].mask & type) {
		    type &= ~flags[i].mask;
		    TRACE(" %s",flags[i].name);
		}
	    }
	    if (type) {
                TRACE(" (unhandled: %08x)", type);
	    }
	}
	TRACE(" / Instance: ");
	if (instance == ((DIDFT_ANYINSTANCE >> 8) & 0xFFFF)) {
	    TRACE("DIDFT_ANYINSTANCE");
	} else {
            TRACE("%3d", instance);
	}
    }
}

void _dump_DIPROPHEADER(LPCDIPROPHEADER diph) {
    if (TRACE_ON(dinput)) {
        TRACE("  - dwObj = 0x%08x\n", diph->dwObj);
        TRACE("  - dwHow = %s\n",
            ((diph->dwHow == DIPH_DEVICE) ? "DIPH_DEVICE" :
            ((diph->dwHow == DIPH_BYOFFSET) ? "DIPH_BYOFFSET" :
            ((diph->dwHow == DIPH_BYID)) ? "DIPH_BYID" : "unknown")));
    }
}

void _dump_OBJECTINSTANCEW(const DIDEVICEOBJECTINSTANCEW *ddoi) {
    TRACE("    - enumerating : %s ('%s'), - %2d - 0x%08x - %s - 0x%x\n",
        debugstr_guid(&ddoi->guidType), _dump_dinput_GUID(&ddoi->guidType), ddoi->dwOfs, ddoi->dwType, debugstr_w(ddoi->tszName), ddoi->dwFlags);
}

/* This function is a helper to convert a GUID into any possible DInput GUID out there */
const char *_dump_dinput_GUID(const GUID *guid) {
    unsigned int i;
    static const struct {
	const GUID *guid;
	const char *name;
    } guids[] = {
#define FE(x) { &x, #x}
	FE(GUID_XAxis),
	FE(GUID_YAxis),
	FE(GUID_ZAxis),
	FE(GUID_RxAxis),
	FE(GUID_RyAxis),
	FE(GUID_RzAxis),
	FE(GUID_Slider),
	FE(GUID_Button),
	FE(GUID_Key),
	FE(GUID_POV),
	FE(GUID_Unknown),
	FE(GUID_SysMouse),
	FE(GUID_SysKeyboard),
	FE(GUID_Joystick),
	FE(GUID_ConstantForce),
	FE(GUID_RampForce),
	FE(GUID_Square),
	FE(GUID_Sine),
	FE(GUID_Triangle),
	FE(GUID_SawtoothUp),
	FE(GUID_SawtoothDown),
	FE(GUID_Spring),
	FE(GUID_Damper),
	FE(GUID_Inertia),
	FE(GUID_Friction),
	FE(GUID_CustomForce)
#undef FE
    };
    if (guid == NULL)
	return "null GUID";
    for (i = 0; i < ARRAY_SIZE(guids); i++) {
	if (IsEqualGUID(guids[i].guid, guid)) {
	    return guids[i].name;
	}
    }
    return debugstr_guid(guid);
}

void _dump_DIDATAFORMAT(const DIDATAFORMAT *df) {
    unsigned int i;

    TRACE("Dumping DIDATAFORMAT structure:\n");
    TRACE("  - dwSize: %d\n", df->dwSize);
    if (df->dwSize != sizeof(DIDATAFORMAT)) {
        WARN("Non-standard DIDATAFORMAT structure size %d\n", df->dwSize);
    }
    TRACE("  - dwObjsize: %d\n", df->dwObjSize);
    if (df->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) {
        WARN("Non-standard DIOBJECTDATAFORMAT structure size %d\n", df->dwObjSize);
    }
    TRACE("  - dwFlags: 0x%08x (", df->dwFlags);
    switch (df->dwFlags) {
        case DIDF_ABSAXIS: TRACE("DIDF_ABSAXIS"); break;
	case DIDF_RELAXIS: TRACE("DIDF_RELAXIS"); break;
	default: TRACE("unknown"); break;
    }
    TRACE(")\n");
    TRACE("  - dwDataSize: %d\n", df->dwDataSize);
    TRACE("  - dwNumObjs: %d\n", df->dwNumObjs);
    
    for (i = 0; i < df->dwNumObjs; i++) {
	TRACE("  - Object %d:\n", i);
	TRACE("      * GUID: %s ('%s')\n", debugstr_guid(df->rgodf[i].pguid), _dump_dinput_GUID(df->rgodf[i].pguid));
        TRACE("      * dwOfs: %d\n", df->rgodf[i].dwOfs);
        TRACE("      * dwType: 0x%08x\n", df->rgodf[i].dwType);
	TRACE("        "); _dump_EnumObjects_flags(df->rgodf[i].dwType); TRACE("\n");
        TRACE("      * dwFlags: 0x%08x\n", df->rgodf[i].dwFlags);
	TRACE("        "); _dump_ObjectDataFormat_flags(df->rgodf[i].dwFlags); TRACE("\n");
    }
}

/******************************************************************************
 * Get the default and the app-specific config keys.
 */
BOOL get_app_key(HKEY *defkey, HKEY *appkey)
{
    char buffer[MAX_PATH+16];
    DWORD len;

    *appkey = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\DirectInput */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\DirectInput", defkey))
        *defkey = 0;

    len = GetModuleFileNameA(0, buffer, MAX_PATH);
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectInput */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            char *p, *appname = buffer;
            if ((p = strrchr(appname, '/'))) appname = p + 1;
            if ((p = strrchr(appname, '\\'))) appname = p + 1;
            strcat(appname, "\\DirectInput");

            if (RegOpenKeyA(tmpkey, appname, appkey)) *appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    return *defkey || *appkey;
}

/******************************************************************************
 * Get a config key from either the app-specific or the default config
 */
DWORD get_config_key( HKEY defkey, HKEY appkey, const WCHAR *name, WCHAR *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExW( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    if (defkey && !RegQueryValueExW( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    return ERROR_FILE_NOT_FOUND;
}

BOOL device_instance_is_disabled( DIDEVICEINSTANCEW *instance, BOOL *override )
{
    static const WCHAR disabled_str[] = {'d', 'i', 's', 'a', 'b', 'l', 'e', 'd', 0};
    static const WCHAR override_str[] = {'o', 'v', 'e', 'r', 'r', 'i', 'd', 'e', 0};
    static const WCHAR joystick_key[] = {'J', 'o', 'y', 's', 't', 'i', 'c', 'k', 's', 0};
    WCHAR buffer[MAX_PATH];
    HKEY hkey, appkey, temp;
    BOOL disable = FALSE;

    get_app_key( &hkey, &appkey );
    if (override) *override = FALSE;

    /* Joystick settings are in the 'joysticks' subkey */
    if (appkey)
    {
        if (RegOpenKeyW( appkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( appkey );
        appkey = temp;
    }

    if (hkey)
    {
        if (RegOpenKeyW( hkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( hkey );
        hkey = temp;
    }

    /* Look for the "controllername"="disabled" key */
    if (!get_config_key( hkey, appkey, instance->tszInstanceName, buffer, sizeof(buffer) ))
    {
        if (!wcscmp( disabled_str, buffer ))
        {
            TRACE( "Disabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            disable = TRUE;
        }
        else if (override && !wcscmp( override_str, buffer ))
        {
            TRACE( "Force enabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            *override = TRUE;
        }
    }

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    return disable;
}

/* Conversion between internal data buffer and external data buffer */
void fill_DataFormat(void *out, DWORD size, const void *in, const DataFormat *df)
{
    int i;
    const char *in_c = in;
    char *out_c = out;

    memset(out, 0, size);
    if (df->dt == NULL) {
	/* This means that the app uses Wine's internal data format */
        memcpy(out, in, min(size, df->internal_format_size));
    } else {
	for (i = 0; i < df->size; i++) {
	    if (df->dt[i].offset_in >= 0) {
		switch (df->dt[i].size) {
		    case 1:
		        TRACE("Copying (c) to %d from %d (value %d)\n",
                              df->dt[i].offset_out, df->dt[i].offset_in, *(in_c + df->dt[i].offset_in));
			*(out_c + df->dt[i].offset_out) = *(in_c + df->dt[i].offset_in);
			break;

		    case 2:
			TRACE("Copying (s) to %d from %d (value %d)\n",
			      df->dt[i].offset_out, df->dt[i].offset_in, *((const short *)(in_c + df->dt[i].offset_in)));
			*((short *)(out_c + df->dt[i].offset_out)) = *((const short *)(in_c + df->dt[i].offset_in));
			break;

		    case 4:
			TRACE("Copying (i) to %d from %d (value %d)\n",
                              df->dt[i].offset_out, df->dt[i].offset_in, *((const int *)(in_c + df->dt[i].offset_in)));
                        *((int *)(out_c + df->dt[i].offset_out)) = *((const int *)(in_c + df->dt[i].offset_in));
			break;

		    default:
			memcpy((out_c + df->dt[i].offset_out), (in_c + df->dt[i].offset_in), df->dt[i].size);
			break;
		}
	    } else {
		switch (df->dt[i].size) {
		    case 1:
		        TRACE("Copying (c) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*(out_c + df->dt[i].offset_out) = (char) df->dt[i].value;
			break;
			
		    case 2:
			TRACE("Copying (s) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((short *) (out_c + df->dt[i].offset_out)) = (short) df->dt[i].value;
			break;
			
		    case 4:
			TRACE("Copying (i) to %d default value %d\n",
			      df->dt[i].offset_out, df->dt[i].value);
			*((int *) (out_c + df->dt[i].offset_out)) = df->dt[i].value;
			break;
			
		    default:
			memset((out_c + df->dt[i].offset_out), 0, df->dt[i].size);
			break;
		}
	    }
	}
    }
}

static void release_DataFormat( DataFormat *format )
{
    TRACE("Deleting DataFormat: %p\n", format);

    free( format->dt );
    format->dt = NULL;
    free( format->offsets );
    format->offsets = NULL;
    free( format->user_df );
    format->user_df = NULL;
}

static inline LPDIOBJECTDATAFORMAT dataformat_to_odf(LPCDIDATAFORMAT df, int idx)
{
    if (idx < 0 || idx >= df->dwNumObjs) return NULL;
    return (LPDIOBJECTDATAFORMAT)((LPBYTE)df->rgodf + idx * df->dwObjSize);
}

/* dataformat_to_odf_by_type
 *  Find the Nth object of the selected type in the DataFormat
 */
LPDIOBJECTDATAFORMAT dataformat_to_odf_by_type(LPCDIDATAFORMAT df, int n, DWORD type)
{
    int i, nfound = 0;

    for (i=0; i < df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(df, i);

        if (odf->dwType & type)
        {
            if (n == nfound)
                return odf;

            nfound++;
        }
    }

    return NULL;
}

static HRESULT create_DataFormat(LPCDIDATAFORMAT asked_format, DataFormat *format)
{
    DataTransform *dt;
    unsigned int i, j;
    int same = 1;
    int *done;
    int index = 0;
    DWORD next = 0;

    if (!format->wine_df) return DIERR_INVALIDPARAM;
    done = calloc( asked_format->dwNumObjs, sizeof(int) );
    dt = malloc( asked_format->dwNumObjs * sizeof(DataTransform) );
    if (!dt || !done) goto failed;

    if (!(format->offsets = malloc( format->wine_df->dwNumObjs * sizeof(int) ))) goto failed;
    if (!(format->user_df = malloc( asked_format->dwSize ))) goto failed;
    memcpy(format->user_df, asked_format, asked_format->dwSize);

    TRACE("Creating DataTransform :\n");
    
    for (i = 0; i < format->wine_df->dwNumObjs; i++)
    {
        format->offsets[i] = -1;

	for (j = 0; j < asked_format->dwNumObjs; j++) {
	    if (done[j] == 1)
		continue;
	    
	    if (/* Check if the application either requests any GUID and if not, it if matches
		 * the GUID of the Wine object.
		 */
		((asked_format->rgodf[j].pguid == NULL) ||
		 (format->wine_df->rgodf[i].pguid == NULL) ||
		 (IsEqualGUID(format->wine_df->rgodf[i].pguid, asked_format->rgodf[j].pguid)))
		&&
		(/* Then check if it accepts any instance id, and if not, if it matches Wine's
		  * instance id.
		  */
		 ((asked_format->rgodf[j].dwType & DIDFT_INSTANCEMASK) == DIDFT_ANYINSTANCE) ||
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == 0x00FF) || /* This is mentioned in no DX docs, but it works fine - tested on WinXP */
		 (DIDFT_GETINSTANCE(asked_format->rgodf[j].dwType) == DIDFT_GETINSTANCE(format->wine_df->rgodf[i].dwType)))
		&&
		( /* Then if the asked type matches the one Wine provides */
                 DIDFT_GETTYPE(asked_format->rgodf[j].dwType) & format->wine_df->rgodf[i].dwType))
            {
		done[j] = 1;
		
		TRACE("Matching :\n");
		TRACE("   - Asked (%d) :\n", j);
		TRACE("       * GUID: %s ('%s')\n",
		      debugstr_guid(asked_format->rgodf[j].pguid),
		      _dump_dinput_GUID(asked_format->rgodf[j].pguid));
                TRACE("       * Offset: %3d\n", asked_format->rgodf[j].dwOfs);
                TRACE("       * dwType: 0x%08x\n", asked_format->rgodf[j].dwType);
		TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
                TRACE("       * dwFlags: 0x%08x\n", asked_format->rgodf[j].dwFlags);
		TRACE("         "); _dump_ObjectDataFormat_flags(asked_format->rgodf[j].dwFlags); TRACE("\n");
		
		TRACE("   - Wine  (%d) :\n", i);
		TRACE("       * GUID: %s ('%s')\n",
                      debugstr_guid(format->wine_df->rgodf[i].pguid),
                      _dump_dinput_GUID(format->wine_df->rgodf[i].pguid));
                TRACE("       * Offset: %3d\n", format->wine_df->rgodf[i].dwOfs);
                TRACE("       * dwType: 0x%08x\n", format->wine_df->rgodf[i].dwType);
                TRACE("         "); _dump_EnumObjects_flags(format->wine_df->rgodf[i].dwType); TRACE("\n");
                TRACE("       * dwFlags: 0x%08x\n", format->wine_df->rgodf[i].dwFlags);
                TRACE("         "); _dump_ObjectDataFormat_flags(format->wine_df->rgodf[i].dwFlags); TRACE("\n");
		
                if (format->wine_df->rgodf[i].dwType & DIDFT_BUTTON)
		    dt[index].size = sizeof(BYTE);
		else
		    dt[index].size = sizeof(DWORD);
                dt[index].offset_in = format->wine_df->rgodf[i].dwOfs;
                dt[index].offset_out = asked_format->rgodf[j].dwOfs;
                format->offsets[i]   = asked_format->rgodf[j].dwOfs;
		dt[index].value = 0;
                next = next + dt[index].size;
		
                if (format->wine_df->rgodf[i].dwOfs != dt[index].offset_out)
		    same = 0;
		
		index++;
		break;
	    }
	}
    }

    TRACE("Setting to default value :\n");
    for (j = 0; j < asked_format->dwNumObjs; j++) {
	if (done[j] == 0) {
	    TRACE("   - Asked (%d) :\n", j);
	    TRACE("       * GUID: %s ('%s')\n",
		  debugstr_guid(asked_format->rgodf[j].pguid),
		  _dump_dinput_GUID(asked_format->rgodf[j].pguid));
            TRACE("       * Offset: %3d\n", asked_format->rgodf[j].dwOfs);
            TRACE("       * dwType: 0x%08x\n", asked_format->rgodf[j].dwType);
	    TRACE("         "); _dump_EnumObjects_flags(asked_format->rgodf[j].dwType); TRACE("\n");
            TRACE("       * dwFlags: 0x%08x\n", asked_format->rgodf[j].dwFlags);
	    TRACE("         "); _dump_ObjectDataFormat_flags(asked_format->rgodf[j].dwFlags); TRACE("\n");

            same = 0;

            if (!(asked_format->rgodf[j].dwType & DIDFT_POV))
                continue; /* fill_DataFormat memsets the buffer to 0 */

	    if (asked_format->rgodf[j].dwType & DIDFT_BUTTON)
		dt[index].size = sizeof(BYTE);
	    else
		dt[index].size = sizeof(DWORD);
	    dt[index].offset_in  = -1;
	    dt[index].offset_out = asked_format->rgodf[j].dwOfs;
            dt[index].value      = -1;
	    index++;
	}
    }
    
    format->internal_format_size = format->wine_df->dwDataSize;
    format->size = index;
    if (same) {
        free( dt );
        dt = NULL;
    }
    format->dt = dt;

    free( done );

    return DI_OK;

failed:
    free( done );
    free( dt );
    format->dt = NULL;
    free( format->offsets );
    format->offsets = NULL;
    free( format->user_df );
    format->user_df = NULL;

    return DIERR_OUTOFMEMORY;
}

static int verify_offset(const DataFormat *df, int offset)
{
    int i;

    if (!df->offsets)
        return -1;

    for (i = df->wine_df->dwNumObjs - 1; i >= 0; i--)
    {
        if (df->offsets[i] == offset)
            return offset;
    }

    return -1;
}

static int id_to_object( LPCDIDATAFORMAT df, int id )
{
    int i;

    id &= 0x00ffffff;
    for (i = 0; i < df->dwNumObjs; i++)
        if ((dataformat_to_odf(df, i)->dwType & 0x00ffffff) == id)
            return i;

    return -1;
}

static int id_to_offset(const DataFormat *df, int id)
{
    int obj = id_to_object(df->wine_df, id);

    return obj >= 0 && df->offsets ? df->offsets[obj] : -1;
}

static DWORD semantic_to_obj_id(IDirectInputDeviceImpl* This, DWORD dwSemantic)
{
    DWORD type = (0x0000ff00 & dwSemantic) >> 8;
    BOOL byofs = (dwSemantic & 0x80000000) != 0;
    DWORD value = (dwSemantic & 0x000000ff);
    BOOL found = FALSE;
    DWORD instance;
    int i;

    for (i = 0; i < This->data_format.wine_df->dwNumObjs && !found; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(This->data_format.wine_df, i);

        if (byofs && value != odf->dwOfs) continue;
        if (!byofs && value != DIDFT_GETINSTANCE(odf->dwType)) continue;
        instance = DIDFT_GETINSTANCE(odf->dwType);
        found = TRUE;
    }

    if (!found) return 0;

    if (type & DIDFT_AXIS)   type = DIDFT_RELAXIS;
    if (type & DIDFT_BUTTON) type = DIDFT_PSHBUTTON;

    return type | (0x0000ff00 & (instance << 8));
}

/*
 * get_mapping_key
 * Retrieves an open registry key to save the mapping, parametrized for an username,
 * specific device and specific action mapping guid.
 */
static HKEY get_mapping_key(const WCHAR *device, const WCHAR *username, const WCHAR *guid)
{
    static const WCHAR *subkey = L"Software\\Wine\\DirectInput\\Mappings\\%s\\%s\\%s";
    HKEY hkey;
    WCHAR *keyname;

    SIZE_T len = wcslen( subkey ) + wcslen( username ) + wcslen( device ) + wcslen( guid ) + 1;
    keyname = malloc( sizeof(WCHAR) * len );
    swprintf( keyname, len, subkey, username, device, guid );

    /* The key used is HKCU\Software\Wine\DirectInput\Mappings\[username]\[device]\[mapping_guid] */
    if (RegCreateKeyW(HKEY_CURRENT_USER, keyname, &hkey))
        hkey = 0;

    free( keyname );

    return hkey;
}

static HRESULT save_mapping_settings(IDirectInputDevice8W *iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUsername)
{
    WCHAR *guid_str = NULL;
    DIDEVICEINSTANCEW didev;
    HKEY hkey;
    int i;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return DI_SETTINGSNOTSAVED;

    hkey = get_mapping_key(didev.tszInstanceName, lpszUsername, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return DI_SETTINGSNOTSAVED;
    }

    /* Write each of the actions mapped for this device.
       Format is "dwSemantic"="dwObjID" and key is of type REG_DWORD
    */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        WCHAR label[9];

        if (IsEqualGUID(&didev.guidInstance, &lpdiaf->rgoAction[i].guidInstance) &&
            lpdiaf->rgoAction[i].dwHow != DIAH_UNMAPPED)
        {
            swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );
            RegSetValueExW( hkey, label, 0, REG_DWORD, (const BYTE *)&lpdiaf->rgoAction[i].dwObjID,
                            sizeof(DWORD) );
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return DI_OK;
}

static BOOL load_mapping_settings(IDirectInputDeviceImpl *This, LPDIACTIONFORMATW lpdiaf, const WCHAR *username)
{
    HKEY hkey;
    WCHAR *guid_str;
    DIDEVICEINSTANCEW didev;
    int i, mapped = 0;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(&This->IDirectInputDevice8W_iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return FALSE;

    hkey = get_mapping_key(didev.tszInstanceName, username, guid_str);

    if (!hkey)
    {
        CoTaskMemFree(guid_str);
        return FALSE;
    }

    /* Try to read each action in the DIACTIONFORMAT from registry */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        DWORD id, size = sizeof(DWORD);
        WCHAR label[9];

        swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );

        if (!RegQueryValueExW(hkey, label, 0, NULL, (LPBYTE) &id, &size))
        {
            lpdiaf->rgoAction[i].dwObjID = id;
            lpdiaf->rgoAction[i].guidInstance = didev.guidInstance;
            lpdiaf->rgoAction[i].dwHow = DIAH_DEFAULT;
            mapped += 1;
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return mapped > 0;
}

static BOOL set_app_data(IDirectInputDeviceImpl *dev, int offset, UINT_PTR app_data)
{
    int num_actions = dev->num_actions;
    ActionMap *action_map = dev->action_map, *target_map = NULL;

    if (num_actions == 0)
    {
        num_actions = 1;
        action_map = malloc( sizeof(ActionMap) );
        if (!action_map) return FALSE;
        target_map = &action_map[0];
    }
    else
    {
        int i;
        for (i = 0; i < num_actions; i++)
        {
            if (dev->action_map[i].offset != offset) continue;
            target_map = &dev->action_map[i];
            break;
        }

        if (!target_map)
        {
            num_actions++;
            action_map = realloc( action_map, sizeof(ActionMap) * num_actions );
            if (!action_map) return FALSE;
            target_map = &action_map[num_actions-1];
        }
    }

    target_map->offset = offset;
    target_map->uAppData = app_data;

    dev->action_map = action_map;
    dev->num_actions = num_actions;

    return TRUE;
}

/******************************************************************************
 *	queue_event - add new event to the ring queue
 */

void queue_event( IDirectInputDevice8W *iface, int inst_id, DWORD data, DWORD time, DWORD seq )
{
    static ULONGLONG notify_ms = 0;
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W( iface );
    int next_pos, ofs = id_to_offset(&This->data_format, inst_id);
    ULONGLONG time_ms = GetTickCount64();

    if (time_ms - notify_ms > 1000)
    {
        PostMessageW(GetDesktopWindow(), WM_WINE_NOTIFY_ACTIVITY, 0, 0);
        notify_ms = time_ms;
    }

    if (!This->queue_len || This->overflow || ofs < 0) return;

    next_pos = (This->queue_head + 1) % This->queue_len;
    if (next_pos == This->queue_tail)
    {
        TRACE(" queue overflowed\n");
        This->overflow = TRUE;
        return;
    }

    TRACE(" queueing %d at offset %d (queue head %d / size %d)\n",
          data, ofs, This->queue_head, This->queue_len);

    This->data_queue[This->queue_head].dwOfs       = ofs;
    This->data_queue[This->queue_head].dwData      = data;
    This->data_queue[This->queue_head].dwTimeStamp = time;
    This->data_queue[This->queue_head].dwSequence  = seq;
    This->data_queue[This->queue_head].uAppData    = -1;

    /* Set uAppData by means of action mapping */
    if (This->num_actions > 0)
    {
        int i;
        for (i=0; i < This->num_actions; i++)
        {
            if (This->action_map[i].offset == ofs)
            {
                TRACE("Offset %d mapped to uAppData %lu\n", ofs, This->action_map[i].uAppData);
                This->data_queue[This->queue_head].uAppData = This->action_map[i].uAppData;
                break;
            }
        }
    }

    This->queue_head = next_pos;
    /* Send event if asked */
}

/******************************************************************************
 *	Acquire
 */

HRESULT WINAPI IDirectInputDevice2WImpl_Acquire( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->acquired)
        hr = DI_NOEFFECT;
    else if (!impl->data_format.user_df)
        hr = DIERR_INVALIDPARAM;
    else if ((impl->dwCoopLevel & DISCL_FOREGROUND) && impl->win != GetForegroundWindow())
        hr = DIERR_OTHERAPPHASPRIO;
    else
    {
        impl->acquired = TRUE;
        if (FAILED(hr = impl->vtbl->acquire( iface ))) impl->acquired = FALSE;
    }
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_acquire_device( iface );
    check_dinput_hooks( iface, TRUE );

    return hr;
}

/******************************************************************************
 *	Unacquire
 */

HRESULT WINAPI IDirectInputDevice2WImpl_Unacquire( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (!impl->acquired) hr = DI_NOEFFECT;
    else hr = impl->vtbl->unacquire( iface );
    impl->acquired = FALSE;
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_unacquire_device( iface );
    check_dinput_hooks( iface, FALSE );

    return hr;
}

/******************************************************************************
 *	IDirectInputDeviceA
 */

HRESULT WINAPI IDirectInputDevice2WImpl_SetDataFormat(LPDIRECTINPUTDEVICE8W iface, LPCDIDATAFORMAT df)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res = DI_OK;

    if (!df) return E_POINTER;
    TRACE("(%p) %p\n", This, df);
    _dump_DIDATAFORMAT(df);

    if (df->dwSize != sizeof(DIDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (df->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (This->acquired) return DIERR_ACQUIRED;

    EnterCriticalSection(&This->crit);

    free( This->action_map );
    This->action_map = NULL;
    This->num_actions = 0;

    release_DataFormat(&This->data_format);
    res = create_DataFormat(df, &This->data_format);

    LeaveCriticalSection(&This->crit);
    return res;
}

/******************************************************************************
  *     SetCooperativeLevel
  *
  *  Set cooperative level and the source window for the events.
  */
HRESULT WINAPI IDirectInputDevice2WImpl_SetCooperativeLevel(LPDIRECTINPUTDEVICE8W iface, HWND hwnd, DWORD dwflags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT hr;

    TRACE("(%p) %p,0x%08x\n", This, hwnd, dwflags);
    _dump_cooperativelevel_DI(dwflags);

    if ((dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == 0 ||
        (dwflags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE) ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == 0 ||
        (dwflags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == (DISCL_FOREGROUND | DISCL_BACKGROUND))
        return DIERR_INVALIDPARAM;

    if (hwnd && GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD) return E_HANDLE;

    if (!hwnd && dwflags == (DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))
        hwnd = GetDesktopWindow();

    if (!IsWindow(hwnd)) return E_HANDLE;

    /* For security reasons native does not allow exclusive background level
       for mouse and keyboard only */
    if (dwflags & DISCL_EXCLUSIVE && dwflags & DISCL_BACKGROUND &&
        (IsEqualGUID(&This->guid, &GUID_SysMouse) ||
         IsEqualGUID(&This->guid, &GUID_SysKeyboard)))
        return DIERR_UNSUPPORTED;

    /* Store the window which asks for the mouse */
    EnterCriticalSection(&This->crit);
    if (This->acquired) hr = DIERR_ACQUIRED;
    else
    {
        This->win = hwnd;
        This->dwCoopLevel = dwflags;
        hr = DI_OK;
    }
    LeaveCriticalSection(&This->crit);

    return hr;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceInfo( IDirectInputDevice8W *iface, DIDEVICEINSTANCEW *instance )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return DIERR_INVALIDPARAM;

    size = instance->dwSize;
    memcpy( instance, &impl->instance, size );
    instance->dwSize = size;

    return S_OK;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
HRESULT WINAPI IDirectInputDevice2WImpl_SetEventNotification(LPDIRECTINPUTDEVICE8W iface, HANDLE event)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p) %p\n", This, event);

    EnterCriticalSection(&This->crit);
    This->hEvent = event;
    LeaveCriticalSection(&This->crit);
    return DI_OK;
}

void direct_input_device_destroy( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE( "iface %p.\n", iface );

    IDirectInputDevice_Unacquire(iface);
    /* Reset the FF state, free all effects, etc */
    IDirectInputDevice8_SendForceFeedbackCommand(iface, DISFFC_RESET);

    free( This->data_queue );

    /* Free data format */
    free( This->data_format.wine_df->rgodf );
    free( This->data_format.wine_df );
    release_DataFormat(&This->data_format);

    /* Free action mapping */
    free( This->action_map );

    IDirectInput_Release(&This->dinput->IDirectInput7A_iface);
    This->crit.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crit);

    free( This );
}

ULONG WINAPI IDirectInputDevice2WImpl_Release( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %u.\n", iface, ref );

    if (!ref)
    {
        if (impl->vtbl->release) impl->vtbl->release( iface );
        else direct_input_device_destroy( iface );
    }

    return ref;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, caps %p.\n", iface, caps );

    if (!caps) return E_POINTER;
    if (caps->dwSize != sizeof(DIDEVCAPS_DX3) &&
        caps->dwSize != sizeof(DIDEVCAPS))
        return DIERR_INVALIDPARAM;

    size = caps->dwSize;
    memcpy( caps, &impl->caps, size );
    caps->dwSize = size;

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_QueryInterface(LPDIRECTINPUTDEVICE8W iface, REFIID riid, LPVOID *ppobj)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(&IID_IDirectInputDeviceA, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice2A, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice7A, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice8A, riid))
    {
        IDirectInputDevice2_AddRef(iface);
        *ppobj = IDirectInputDevice8A_from_impl(This);
        return DI_OK;
    }

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IDirectInputDeviceW, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice2W, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice7W, riid) ||
        IsEqualGUID(&IID_IDirectInputDevice8W, riid))
    {
        IDirectInputDevice2_AddRef(iface);
        *ppobj = IDirectInputDevice8W_from_impl(This);
        return DI_OK;
    }

    WARN("Unsupported interface!\n");
    return E_NOINTERFACE;
}

ULONG WINAPI IDirectInputDevice2WImpl_AddRef( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %u.\n", iface, ref );
    return ref;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumObjects( IDirectInputDevice8W *iface, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                                                     void *context, DWORD flags )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, flags %#x.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags & ~(DIDFT_AXIS | DIDFT_POV | DIDFT_BUTTON | DIDFT_NODATA | DIDFT_COLLECTION))
        return DIERR_INVALIDPARAM;

    if (flags == DIDFT_ALL || (flags & DIDFT_AXIS))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_POV))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_POV, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_BUTTON))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_BUTTON, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & (DIDFT_NODATA | DIDFT_COLLECTION)))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_NODATA, callback, context );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }

    return DI_OK;
}

static HRESULT enum_object_filter_init( IDirectInputDeviceImpl *impl, DIPROPHEADER *filter )
{
    DIDATAFORMAT *format = impl->data_format.wine_df;
    int i, *offsets = impl->data_format.offsets;

    if (filter->dwHow > DIPH_BYUSAGE) return DIERR_INVALIDPARAM;
    if (filter->dwHow == DIPH_BYUSAGE && !(impl->instance.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
    if (filter->dwHow != DIPH_BYOFFSET) return DI_OK;

    if (!offsets) return DIERR_NOTFOUND;

    for (i = 0; i < format->dwNumObjs; ++i) if (offsets[i] == filter->dwObj) break;
    if (i == format->dwNumObjs) return DIERR_NOTFOUND;

    filter->dwObj = format->rgodf[i].dwOfs;
    return DI_OK;
}

static BOOL CALLBACK find_object( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    *(DIDEVICEOBJECTINSTANCEW *)context = *instance;
    return DIENUM_STOP;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                     DIPROPHEADER *header )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD object_mask = DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV;
    DIDEVICEOBJECTINSTANCEW instance;
    DIPROPHEADER filter;
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_INSTANCENAME:
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, NULL );

    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, NULL );

    case (DWORD_PTR)DIPROP_GUIDANDPATH:
        if (header->dwSize != sizeof(DIPROPGUIDANDPATH)) return DIERR_INVALIDPARAM;
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, NULL );

    case (DWORD_PTR)DIPROP_RANGE:
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, find_object, &instance );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
        if (!(instance.dwType & DIDFT_AXIS)) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, &instance );

    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_GRANULARITY:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, find_object, &instance );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
        if (!(instance.dwType & DIDFT_AXIS)) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, &instance );

    case (DWORD_PTR)DIPROP_KEYNAME:
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, find_object, &instance );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
        if (!(instance.dwType & DIDFT_BUTTON)) return DIERR_UNSUPPORTED;
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, &instance );

    case (DWORD_PTR)DIPROP_AUTOCENTER:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        return DIERR_UNSUPPORTED;

    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        value->dwData = impl->buffersize;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_USERNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        struct DevicePlayer *device_player;
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
        {
            if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
            {
                if (!*device_player->username) break;
                lstrcpynW( value->wsz, device_player->username, ARRAY_SIZE(value->wsz) );
                return DI_OK;
            }
        }
        return S_FALSE;
    }
    case (DWORD_PTR)DIPROP_FFGAIN:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        value->dwData = 10000;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_CALIBRATION:
        return DIERR_INVALIDPARAM;
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

struct set_object_property_params
{
    IDirectInputDevice8W *iface;
    const DIPROPHEADER *header;
    DWORD property;
};

static BOOL CALLBACK set_object_property( const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    struct set_object_property_params *params = context;
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( params->iface );
    impl->vtbl->set_property( params->iface, params->property, params->header, instance );
    return DIENUM_CONTINUE;
}

HRESULT WINAPI IDirectInputDevice2WImpl_SetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                     const DIPROPHEADER *header )
{
    struct set_object_property_params params = {.iface = iface, .header = header, .property = LOWORD( guid )};
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter;
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
    {
        const DIPROPRANGE *value = (const DIPROPRANGE *)header;
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        if (value->lMin > value->lMax) return DIERR_INVALIDPARAM;
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (value->dwData > 10000) return DIERR_INVALIDPARAM;
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        EnterCriticalSection( &impl->crit );
        if (impl->acquired) hr = DIERR_ACQUIRED;
        else if (value->dwData > DIPROPAUTOCENTER_ON) hr = DIERR_INVALIDPARAM;
        else hr = DIERR_UNSUPPORTED;
        LeaveCriticalSection( &impl->crit );
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
    case (DWORD_PTR)DIPROP_AXISMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (header->dwHow == DIPH_DEVICE && header->dwObj) return DIERR_INVALIDPARAM;

        TRACE( "Axis mode: %s\n", value->dwData == DIPROPAXISMODE_ABS ? "absolute" : "relative" );
        EnterCriticalSection( &impl->crit );
        if (impl->acquired) hr = DIERR_ACQUIRED;
        else if (!impl->data_format.user_df) hr = DI_OK;
        else
        {
            impl->data_format.user_df->dwFlags &= ~DIDFT_AXIS;
            impl->data_format.user_df->dwFlags |= value->dwData == DIPROPAXISMODE_ABS ? DIDF_ABSAXIS : DIDF_RELAXIS;
            hr = DI_OK;
        }
        LeaveCriticalSection( &impl->crit );
        return hr;
    }
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;

        TRACE( "buffersize = %d\n", value->dwData );

        EnterCriticalSection( &impl->crit );
        if (impl->acquired) hr = DIERR_ACQUIRED;
        else
        {
            impl->buffersize = value->dwData;
            impl->queue_len = min( impl->buffersize, 1024 );
            free( impl->data_queue );

            impl->data_queue = impl->queue_len ? malloc( impl->queue_len * sizeof(DIDEVICEOBJECTDATA) ) : NULL;
            impl->queue_head = impl->queue_tail = impl->overflow = 0;
            hr = DI_OK;
        }
        LeaveCriticalSection( &impl->crit );
        return hr;
    }
    case (DWORD_PTR)DIPROP_APPDATA:
    {
        const DIPROPPOINTER *value = (const DIPROPPOINTER *)header;
        int offset = -1;
        if (header->dwSize != sizeof(DIPROPPOINTER)) return DIERR_INVALIDPARAM;

        if (header->dwHow == DIPH_BYID)
            offset = id_to_offset( &impl->data_format, header->dwObj );
        else if (header->dwHow == DIPH_BYOFFSET)
            offset = verify_offset( &impl->data_format, header->dwObj );
        else
            return DIERR_UNSUPPORTED;

        if (offset == -1) return DIERR_OBJECTNOTFOUND;
        if (!set_app_data( impl, offset, value->uData )) return DIERR_OUTOFMEMORY;
        return DI_OK;
    }
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

static void dinput_device_set_username( IDirectInputDeviceImpl *impl, const DIPROPSTRING *value )
{
    struct DevicePlayer *device_player;
    BOOL found = FALSE;

    LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
    {
        if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
        {
            found = TRUE;
            break;
        }
    }
    if (!found && (device_player = malloc( sizeof(struct DevicePlayer) )))
    {
        list_add_tail( &impl->dinput->device_players, &device_player->entry );
        device_player->instance_guid = impl->guid;
    }
    if (device_player)
        lstrcpynW( device_player->username, value->wsz, ARRAY_SIZE(device_player->username) );
}

static BOOL CALLBACK get_object_info( const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDEVICEOBJECTINSTANCEW *dest = data;
    DWORD size = dest->dwSize;

    memcpy( dest, instance, size );
    dest->dwSize = size;

    return DIENUM_STOP;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetObjectInfo( IDirectInputDevice8W *iface, DIDEVICEOBJECTINSTANCEW *instance,
                                                       DWORD obj, DWORD how )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = how,
        .dwObj = obj
    };
    HRESULT hr;

    TRACE( "iface %p, instance %p, obj %#x, how %#x.\n", iface, instance, obj, how );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W) && instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
        return DIERR_INVALIDPARAM;
    if (how == DIPH_DEVICE) return DIERR_INVALIDPARAM;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;

    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, get_object_info, instance );
    if (FAILED(hr)) return hr;
    if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
    return DI_OK;
}

static BOOL CALLBACK reset_axis_data( const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    *(ULONG *)((char *)data + instance->dwOfs) = 0;
    return DIENUM_CONTINUE;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceState( IDirectInputDevice8W *iface, DWORD size, void *data )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
        .dwObj = 0,
    };
    HRESULT hr;

    TRACE( "iface %p, size %u, data %p.\n", iface, size, data );

    if (!data) return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll( iface );

    EnterCriticalSection( &impl->crit );
    if (!impl->acquired)
        hr = DIERR_NOTACQUIRED;
    else if (size != impl->data_format.user_df->dwDataSize)
        hr = DIERR_INVALIDPARAM;
    else
    {
        fill_DataFormat( data, size, impl->device_state, &impl->data_format );
        if (!(impl->data_format.user_df->dwFlags & DIDF_ABSAXIS))
            impl->vtbl->enum_objects( iface, &filter, DIDFT_RELAXIS, reset_axis_data, impl->device_state );
        hr = DI_OK;
    }
    LeaveCriticalSection( &impl->crit );

    return hr;
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD dodsize,
                                                      LPDIDEVICEOBJECTDATA dod, LPDWORD entries, DWORD flags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT ret = DI_OK;
    int len;

    TRACE("(%p) %p -> %p(%d) x%d, 0x%08x\n",
          This, dod, entries, entries ? *entries : 0, dodsize, flags);

    if (This->dinput->dwVersion == 0x0800 || dodsize == sizeof(DIDEVICEOBJECTDATA_DX3))
    {
        if (!This->queue_len) return DIERR_NOTBUFFERED;
        if (!This->acquired) return DIERR_NOTACQUIRED;
    }

    if (!This->queue_len)
        return DI_OK;
    if (dodsize < sizeof(DIDEVICEOBJECTDATA_DX3))
        return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll(iface);
    EnterCriticalSection(&This->crit);

    len = This->queue_head - This->queue_tail;
    if (len < 0) len += This->queue_len;

    if ((*entries != INFINITE) && (len > *entries)) len = *entries;

    if (dod)
    {
        int i;
        for (i = 0; i < len; i++)
        {
            int n = (This->queue_tail + i) % This->queue_len;
            memcpy((char *)dod + dodsize * i, This->data_queue + n, dodsize);
        }
    }
    *entries = len;

    if (This->overflow && This->dinput->dwVersion == 0x0800)
        ret = DI_BUFFEROVERFLOW;

    if (!(flags & DIGDD_PEEK))
    {
        /* Advance reading position */
        This->queue_tail = (This->queue_tail + len) % This->queue_len;
        This->overflow = FALSE;
    }

    LeaveCriticalSection(&This->crit);

    TRACE("Returning %d events queued\n", *entries);
    return ret;
}

HRESULT WINAPI IDirectInputDevice2WImpl_RunControlPanel(LPDIRECTINPUTDEVICE8W iface, HWND hwndOwner, DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("%p)->(%p,0x%08x): stub!\n", This, hwndOwner, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_Initialize(LPDIRECTINPUTDEVICE8W iface, HINSTANCE hinst, DWORD dwVersion,
                                                   REFGUID rguid)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%p,%d,%s): stub!\n", This, hinst, dwVersion, debugstr_guid(rguid));
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_CreateEffect( IDirectInputDevice8W *iface, const GUID *guid,
                                                      const DIEFFECT *params, IDirectInputEffect **out,
                                                      IUnknown *outer )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD flags = DIEP_ALLPARAMS;
    HRESULT hr;

    TRACE( "iface %p, guid %s, params %p, out %p, outer %p\n", iface, debugstr_guid( guid ),
           params, out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->create_effect) return DIERR_UNSUPPORTED;
    if (FAILED(hr = impl->vtbl->create_effect( iface, out ))) return hr;

    hr = IDirectInputEffect_Initialize( *out, DINPUT_instance, impl->dinput->dwVersion, guid );
    if (FAILED(hr)) goto failed;

    if (!params) return DI_OK;
    if (!impl->acquired || !(impl->dwCoopLevel & DISCL_EXCLUSIVE)) flags |= DIEP_NODOWNLOAD;
    hr = IDirectInputEffect_SetParameters( *out, params, flags );
    if (FAILED(hr)) goto failed;
    return hr;

failed:
    IDirectInputEffect_Release( *out );
    *out = NULL;
    return hr;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumEffects( IDirectInputDevice8W *iface, LPDIENUMEFFECTSCALLBACKW callback,
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

HRESULT WINAPI IDirectInputDevice2WImpl_GetEffectInfo( IDirectInputDevice8W *iface, DIEFFECTINFOW *info,
                                                       const GUID *guid )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, info %p, guid %s.\n", iface, info, debugstr_guid( guid ) );

    if (!info) return E_POINTER;
    if (info->dwSize != sizeof(DIEFFECTINFOW)) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_DEVICENOTREG;
    if (!impl->vtbl->get_effect_info) return DIERR_UNSUPPORTED;
    return impl->vtbl->get_effect_info( iface, info, guid );
}

HRESULT WINAPI IDirectInputDevice2WImpl_GetForceFeedbackState( IDirectInputDevice8W *iface, DWORD *out )
{
    FIXME( "iface %p, out %p stub!\n", iface, out );
    if (!out) return E_POINTER;
    return DIERR_UNSUPPORTED;
}

HRESULT WINAPI IDirectInputDevice2WImpl_SendForceFeedbackCommand( IDirectInputDevice8W *iface, DWORD command )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, flags %x.\n", iface, command );

    switch (command)
    {
    case DISFFC_RESET: break;
    case DISFFC_STOPALL: break;
    case DISFFC_PAUSE: break;
    case DISFFC_CONTINUE: break;
    case DISFFC_SETACTUATORSON: break;
    case DISFFC_SETACTUATORSOFF: break;
    default: return DIERR_INVALIDPARAM;
    }

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->send_force_feedback_command) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->crit );
    if (!impl->acquired || !(impl->dwCoopLevel & DISCL_EXCLUSIVE)) hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else hr = impl->vtbl->send_force_feedback_command( iface, command );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

HRESULT WINAPI IDirectInputDevice2WImpl_EnumCreatedEffectObjects( IDirectInputDevice8W *iface,
                                                                  LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                                  void *context, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, callback %p, context %p, flags %#x.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DI_OK;
    if (!impl->vtbl->enum_created_effect_objects) return DIERR_UNSUPPORTED;

    return impl->vtbl->enum_created_effect_objects( iface, callback, context, flags );
}

HRESULT WINAPI IDirectInputDevice2WImpl_Escape(LPDIRECTINPUTDEVICE8W iface, LPDIEFFESCAPE lpDIEEsc)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%p): stub!\n", This, lpDIEEsc);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_Poll( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_NOEFFECT;

    EnterCriticalSection( &impl->crit );
    if (!impl->acquired) hr = DIERR_NOTACQUIRED;
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    if (impl->vtbl->poll) return impl->vtbl->poll( iface );
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice2WImpl_SendDeviceData(LPDIRECTINPUTDEVICE8W iface, DWORD cbObjectData,
                                                       LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut,
                                                       DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(0x%08x,%p,%p,0x%08x): stub!\n", This, cbObjectData, rgdod, pdwInOut, dwFlags);

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_EnumEffectsInFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  LPDIENUMEFFECTSINFILECALLBACK pec,
							  LPVOID pvRef,
							  DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%s,%p,%p,%08x): stub !\n", This, debugstr_w(lpszFileName), pec, pvRef, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice7WImpl_WriteEffectToFile(LPDIRECTINPUTDEVICE8W iface,
							  LPCWSTR lpszFileName,
							  DWORD dwEntries,
							  LPDIFILEEFFECT rgDiFileEft,
							  DWORD dwFlags)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%s,%08x,%p,%08x): stub !\n", This, debugstr_w(lpszFileName), dwEntries, rgDiFileEft, dwFlags);
    
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_BuildActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                        const WCHAR *username, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    BOOL load_success = FALSE, has_actions = FALSE;
    DWORD genre, username_len = MAX_PATH;
    WCHAR username_buf[MAX_PATH];
    const DIDATAFORMAT *df;
    DWORD devMask;
    int i;

    FIXME( "iface %p, format %p, username %s, flags %#x semi-stub!\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    switch (GET_DIDEVICE_TYPE( impl->instance.dwDevType ))
    {
    case DIDEVTYPE_KEYBOARD:
    case DI8DEVTYPE_KEYBOARD:
        devMask = DIKEYBOARD_MASK;
        df = &c_dfDIKeyboard;
        break;
    case DIDEVTYPE_MOUSE:
    case DI8DEVTYPE_MOUSE:
        devMask = DIMOUSE_MASK;
        df = &c_dfDIMouse2;
        break;
    default:
        devMask = DIGENRE_ANY;
        df = impl->data_format.wine_df;
        break;
    }

    /* Unless asked the contrary by these flags, try to load a previous mapping */
    if (!(flags & DIDBAM_HWDEFAULTS))
    {
        /* Retrieve logged user name if necessary */
        if (username == NULL) GetUserNameW( username_buf, &username_len );
        else lstrcpynW( username_buf, username, MAX_PATH );
        load_success = load_mapping_settings( impl, format, username_buf );
    }

    if (load_success) return DI_OK;

    for (i = 0; i < format->dwNumActions; i++)
    {
        /* Don't touch a user configured action */
        if (format->rgoAction[i].dwHow == DIAH_USERCONFIG) continue;

        genre = format->rgoAction[i].dwSemantic & DIGENRE_ANY;
        if (devMask == genre || (devMask == DIGENRE_ANY && genre != DIMOUSE_MASK && genre != DIKEYBOARD_MASK))
        {
            DWORD obj_id = semantic_to_obj_id( impl, format->rgoAction[i].dwSemantic );
            DWORD type = DIDFT_GETTYPE( obj_id );
            DWORD inst = DIDFT_GETINSTANCE( obj_id );

            LPDIOBJECTDATAFORMAT odf;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            /* Make sure the object exists */
            odf = dataformat_to_odf_by_type( df, inst, type );

            if (odf != NULL)
            {
                format->rgoAction[i].dwObjID = obj_id;
                format->rgoAction[i].guidInstance = impl->guid;
                format->rgoAction[i].dwHow = DIAH_DEFAULT;
                has_actions = TRUE;
            }
        }
        else if (!(flags & DIDBAM_PRESERVE))
        {
            /* We must clear action data belonging to other devices */
            memset( &format->rgoAction[i].guidInstance, 0, sizeof(GUID) );
            format->rgoAction[i].dwHow = DIAH_UNMAPPED;
        }
    }

    if (!has_actions) return DI_NOEFFECT;
    if (flags & (DIDBAM_DEFAULT|DIDBAM_PRESERVE|DIDBAM_INITIALIZE|DIDBAM_HWDEFAULTS))
        FIXME("Unimplemented flags %#x\n", flags);
    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                      const WCHAR *username, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT data_format;
    DIOBJECTDATAFORMAT *obj_df = NULL;
    DIPROPDWORD dp;
    DIPROPRANGE dpr;
    DIPROPSTRING dps;
    WCHAR username_buf[MAX_PATH];
    DWORD username_len = MAX_PATH;
    int i, action = 0, num_actions = 0;
    unsigned int offset = 0;
    const DIDATAFORMAT *df;
    ActionMap *action_map;

    FIXME( "iface %p, format %p, username %s, flags %#x semi-stub!\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;

    switch (GET_DIDEVICE_TYPE( impl->instance.dwDevType ))
    {
    case DIDEVTYPE_KEYBOARD:
    case DI8DEVTYPE_KEYBOARD:
        df = &c_dfDIKeyboard;
        break;
    case DIDEVTYPE_MOUSE:
    case DI8DEVTYPE_MOUSE:
        df = &c_dfDIMouse2;
        break;
    default:
        df = impl->data_format.wine_df;
        break;
    }

    if (impl->acquired) return DIERR_ACQUIRED;

    data_format.dwSize = sizeof(data_format);
    data_format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
    data_format.dwFlags = DIDF_RELAXIS;
    data_format.dwDataSize = format->dwDataSize;

    /* Count the actions */
    for (i = 0; i < format->dwNumActions; i++)
        if (IsEqualGUID( &impl->guid, &format->rgoAction[i].guidInstance ))
            num_actions++;

    if (num_actions == 0) return DI_NOEFFECT;

    /* Construct the dataformat and actionmap */
    obj_df = malloc( sizeof(DIOBJECTDATAFORMAT) * num_actions );
    data_format.rgodf = (LPDIOBJECTDATAFORMAT)obj_df;
    data_format.dwNumObjs = num_actions;

    action_map = malloc( sizeof(ActionMap) * num_actions );

    for (i = 0; i < format->dwNumActions; i++)
    {
        if (IsEqualGUID( &impl->guid, &format->rgoAction[i].guidInstance ))
        {
            DWORD inst = DIDFT_GETINSTANCE( format->rgoAction[i].dwObjID );
            DWORD type = DIDFT_GETTYPE( format->rgoAction[i].dwObjID );
            LPDIOBJECTDATAFORMAT obj;

            if (type == DIDFT_PSHBUTTON) type = DIDFT_BUTTON;
            if (type == DIDFT_RELAXIS) type = DIDFT_AXIS;

            obj = dataformat_to_odf_by_type( df, inst, type );

            memcpy( &obj_df[action], obj, df->dwObjSize );

            action_map[action].uAppData = format->rgoAction[i].uAppData;
            action_map[action].offset = offset;
            obj_df[action].dwOfs = offset;
            offset += (type & DIDFT_BUTTON) ? 1 : 4;

            action++;
        }
    }

    IDirectInputDevice8_SetDataFormat( iface, &data_format );

    impl->action_map = action_map;
    impl->num_actions = num_actions;

    free( obj_df );

    /* Set the device properties according to the action format */
    dpr.diph.dwSize = sizeof(DIPROPRANGE);
    dpr.lMin = format->lAxisMin;
    dpr.lMax = format->lAxisMax;
    dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dpr.diph.dwHow = DIPH_DEVICE;
    IDirectInputDevice8_SetProperty( iface, DIPROP_RANGE, &dpr.diph );

    if (format->dwBufferSize > 0)
    {
        dp.diph.dwSize = sizeof(DIPROPDWORD);
        dp.dwData = format->dwBufferSize;
        dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dp.diph.dwHow = DIPH_DEVICE;
        IDirectInputDevice8_SetProperty( iface, DIPROP_BUFFERSIZE, &dp.diph );
    }

    /* Retrieve logged user name if necessary */
    if (username == NULL) GetUserNameW( username_buf, &username_len );
    else lstrcpynW( username_buf, username, MAX_PATH );

    dps.diph.dwSize = sizeof(dps);
    dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dps.diph.dwObj = 0;
    dps.diph.dwHow = DIPH_DEVICE;
    if (flags & DIDSAM_NOUSER) dps.wsz[0] = '\0';
    else lstrcpynW( dps.wsz, username_buf, ARRAY_SIZE(dps.wsz) );
    dinput_device_set_username( impl, &dps );

    /* Save the settings to disk */
    save_mapping_settings( iface, format, username_buf );

    return DI_OK;
}

HRESULT WINAPI IDirectInputDevice8WImpl_GetImageInfo(LPDIRECTINPUTDEVICE8W iface,
						     LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader)
{
    IDirectInputDeviceImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%p): stub !\n", This, lpdiDevImageInfoHeader);
    
    return DI_OK;
}

HRESULT direct_input_device_alloc( SIZE_T size, const IDirectInputDevice8WVtbl *vtbl, const struct dinput_device_vtbl *internal_vtbl,
                                   const GUID *guid, IDirectInputImpl *dinput, void **out )
{
    IDirectInputDeviceImpl *This;
    DIDATAFORMAT *format;

    if (!(This = calloc( 1, size ))) return DIERR_OUTOFMEMORY;
    if (!(format = calloc( 1, sizeof(*format) )))
    {
        free( This );
        return DIERR_OUTOFMEMORY;
    }

    This->IDirectInputDevice8A_iface.lpVtbl = &dinput_device_a_vtbl;
    This->IDirectInputDevice8W_iface.lpVtbl = vtbl;
    This->ref = 1;
    This->guid = *guid;
    This->instance.dwSize = sizeof(DIDEVICEINSTANCEW);
    This->caps.dwSize = sizeof(DIDEVCAPS);
    This->caps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    This->data_format.wine_df = format;
    InitializeCriticalSection( &This->crit );
    This->dinput = dinput;
    IDirectInput_AddRef( &dinput->IDirectInput7A_iface );
    This->vtbl = internal_vtbl;

    *out = This;
    return DI_OK;
}

static const GUID *object_instance_guid( const DIDEVICEOBJECTINSTANCEW *instance )
{
    if (IsEqualGUID( &instance->guidType, &GUID_XAxis )) return &GUID_XAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_YAxis )) return &GUID_YAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_ZAxis )) return &GUID_ZAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RxAxis )) return &GUID_RxAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RyAxis )) return &GUID_RyAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RzAxis )) return &GUID_RzAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_Slider )) return &GUID_Slider;
    if (IsEqualGUID( &instance->guidType, &GUID_Button )) return &GUID_Button;
    if (IsEqualGUID( &instance->guidType, &GUID_Key )) return &GUID_Key;
    if (IsEqualGUID( &instance->guidType, &GUID_POV )) return &GUID_POV;
    return &GUID_Unknown;
}

static BOOL CALLBACK enum_objects_init( const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( data );
    DIDATAFORMAT *format = impl->data_format.wine_df;
    DIOBJECTDATAFORMAT *obj_format;

    if (!format->rgodf)
    {
        format->dwDataSize = max( format->dwDataSize, instance->dwOfs + sizeof(LONG) );
        if (instance->dwType & DIDFT_BUTTON) impl->caps.dwButtons++;
        if (instance->dwType & DIDFT_AXIS) impl->caps.dwAxes++;
        if (instance->dwType & DIDFT_POV) impl->caps.dwPOVs++;
        if (instance->dwType & (DIDFT_BUTTON|DIDFT_AXIS|DIDFT_POV))
        {
            if (!impl->device_state_report_id)
                impl->device_state_report_id = instance->wReportId;
            else if (impl->device_state_report_id != instance->wReportId)
                FIXME( "multiple device state reports found!\n" );
        }
    }
    else
    {
        obj_format = format->rgodf + format->dwNumObjs;
        obj_format->pguid = object_instance_guid( instance );
        obj_format->dwOfs = instance->dwOfs;
        obj_format->dwType = instance->dwType;
        obj_format->dwFlags = instance->dwFlags;
    }

    format->dwNumObjs++;
    return DIENUM_CONTINUE;
}

HRESULT direct_input_device_init( IDirectInputDevice8W *iface )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT *format = impl->data_format.wine_df;
    ULONG size;

    IDirectInputDevice8_EnumObjects( iface, enum_objects_init, iface, DIDFT_ALL );
    if (format->dwDataSize > DEVICE_STATE_MAX_SIZE)
    {
        FIXME( "unable to create device, state is too large\n" );
        return DIERR_OUTOFMEMORY;
    }

    size = format->dwNumObjs * sizeof(*format->rgodf);
    if (!(format->rgodf = calloc( 1, size ))) return DIERR_OUTOFMEMORY;
    format->dwSize = sizeof(*format);
    format->dwObjSize = sizeof(*format->rgodf);
    format->dwFlags = DIDF_ABSAXIS;
    format->dwNumObjs = 0;
    IDirectInputDevice8_EnumObjects( iface, enum_objects_init, iface, DIDFT_ALL );

    return DI_OK;
}
