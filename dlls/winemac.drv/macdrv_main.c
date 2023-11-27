/*
 * MACDRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "config.h"

#include <Security/AuthSession.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "macdrv.h"
#include "shellapi.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(macdrv);

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

C_ASSERT(NUM_EVENT_TYPES <= sizeof(macdrv_event_mask) * 8);

int topmost_float_inactive = TOPMOST_FLOAT_INACTIVE_NONFULLSCREEN;
int capture_displays_for_fullscreen = 0;
BOOL skip_single_buffer_flushes = FALSE;
BOOL allow_vsync = TRUE;
BOOL allow_set_gamma = TRUE;
int left_option_is_alt = 0;
int right_option_is_alt = 0;
int left_command_is_ctrl = 0;
int right_command_is_ctrl = 0;
BOOL allow_software_rendering = FALSE;
BOOL disable_window_decorations = FALSE;
int allow_immovable_windows = TRUE;
int use_confinement_cursor_clipping = TRUE;
int cursor_clipping_locks_windows = TRUE;
int use_precise_scrolling = TRUE;
int gl_surface_mode = GL_SURFACE_IN_FRONT_OPAQUE;
int retina_enabled = FALSE;
int enable_app_nap = FALSE;

CFDictionaryRef localized_strings;


/**************************************************************************
 *              debugstr_cf
 */
const char* debugstr_cf(CFTypeRef t)
{
    CFStringRef s;
    const char* ret;

    if (!t) return "(null)";

    if (CFGetTypeID(t) == CFStringGetTypeID())
        s = t;
    else
        s = CFCopyDescription(t);
    ret = CFStringGetCStringPtr(s, kCFStringEncodingUTF8);
    if (ret) ret = debugstr_a(ret);
    if (!ret)
    {
        const UniChar* u = CFStringGetCharactersPtr(s);
        if (u)
            ret = debugstr_wn((const WCHAR*)u, CFStringGetLength(s));
    }
    if (!ret)
    {
        UniChar buf[200];
        int len = min(CFStringGetLength(s), ARRAY_SIZE(buf));
        CFStringGetCharacters(s, CFRangeMake(0, len), buf);
        ret = debugstr_wn(buf, len);
    }
    if (s != t) CFRelease(s);
    return ret;
}


HKEY reg_open_key(HKEY root, const WCHAR *name, ULONG name_len)
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    return NtOpenKeyEx(&ret, MAXIMUM_ALLOWED, &attr, 0) ? 0 : ret;
}


HKEY open_hkcu_key(const char *name)
{
    WCHAR bufferW[256];
    static HKEY hkcu;

    if (!hkcu)
    {
        char buffer[256];
        DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
        DWORD i, len = sizeof(sid_data);
        SID *sid;

        if (NtQueryInformationToken(GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len))
            return 0;

        sid = ((TOKEN_USER *)sid_data)->User.Sid;
        len = snprintf(buffer, sizeof(buffer), "\\Registry\\User\\S-%u-%u", sid->Revision,
                      (unsigned int)MAKELONG(MAKEWORD(sid->IdentifierAuthority.Value[5],
                                                      sid->IdentifierAuthority.Value[4]),
                                             MAKEWORD(sid->IdentifierAuthority.Value[3],
                                                      sid->IdentifierAuthority.Value[2])));
        for (i = 0; i < sid->SubAuthorityCount; i++)
            len += snprintf(buffer + len, sizeof(buffer) - len, "-%u", (unsigned int)sid->SubAuthority[i]);

        ascii_to_unicode(bufferW, buffer, len);
        hkcu = reg_open_key(NULL, bufferW, len * sizeof(WCHAR));
    }

    return reg_open_key(hkcu, bufferW, asciiz_to_unicode(bufferW, name) - sizeof(WCHAR));
}


/* wrapper for NtCreateKey that creates the key recursively if necessary */
HKEY reg_create_key(HKEY root, const WCHAR *name, ULONG name_len,
                    DWORD options, DWORD *disposition)
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateKey(&ret, MAXIMUM_ALLOWED, &attr, 0, NULL, options, disposition);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        static const WCHAR registry_rootW[] = { '\\','R','e','g','i','s','t','r','y','\\' };
        DWORD pos = 0, i = 0, len = name_len / sizeof(WCHAR);

        /* don't try to create registry root */
        if (!root && len > ARRAY_SIZE(registry_rootW) &&
            !memcmp(name, registry_rootW, sizeof(registry_rootW)))
            i += ARRAY_SIZE(registry_rootW);

        while (i < len && name[i] != '\\') i++;
        if (i == len) return 0;
        for (;;)
        {
            unsigned int subkey_options = options;
            if (i < len) subkey_options &= ~(REG_OPTION_CREATE_LINK | REG_OPTION_OPEN_LINK);
            nameW.Buffer = (WCHAR *)name + pos;
            nameW.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey(&ret, MAXIMUM_ALLOWED, &attr, 0, NULL, subkey_options, disposition);

            if (attr.RootDirectory != root) NtClose(attr.RootDirectory);
            if (!NT_SUCCESS(status)) return 0;
            if (i == len) break;
            attr.RootDirectory = ret;
            while (i < len && name[i] == '\\') i++;
            pos = i;
            while (i < len && name[i] != '\\') i++;
        }
    }
    return ret;
}


HKEY reg_create_ascii_key(HKEY root, const char *name, DWORD options, DWORD *disposition)
{
    WCHAR buf[256];
    return reg_create_key(root, buf, asciiz_to_unicode(buf, name) - sizeof(WCHAR),
                          options, disposition);
}


BOOL reg_delete_tree(HKEY parent, const WCHAR *name, ULONG name_len)
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key_info = (KEY_NODE_INFORMATION *)buffer;
    DWORD size;
    HKEY key;
    BOOL ret = TRUE;

    if (!(key = reg_open_key(parent, name, name_len))) return FALSE;

    while (ret && !NtEnumerateKey(key, 0, KeyNodeInformation, key_info, sizeof(buffer), &size))
        ret = reg_delete_tree(key, key_info->Name, key_info->NameLength);

    if (ret) ret = !NtDeleteKey(key);
    NtClose(key);
    return ret;
}


ULONG query_reg_value(HKEY hkey, const WCHAR *name, KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size)
{
    UNICODE_STRING str;

    RtlInitUnicodeString(&str, name);
    if (NtQueryValueKey(hkey, &str, KeyValuePartialInformation, info, size, &size))
        return 0;

    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}


/***********************************************************************
 *              get_config_key
 *
 * Get a config key from either the app-specific or the default config
 */
static inline DWORD get_config_key(HKEY defkey, HKEY appkey, const char *name,
                                   WCHAR *buffer, DWORD size)
{
    WCHAR nameW[128];
    char buf[2048];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buf;

    asciiz_to_unicode(nameW, name);

    if (appkey && query_reg_value(appkey, nameW, info, sizeof(buf)))
    {
        size = min(info->DataLength, size - sizeof(WCHAR));
        memcpy(buffer, info->Data, size);
        buffer[size / sizeof(WCHAR)] = 0;
        return 0;
    }

    if (defkey && query_reg_value(defkey, nameW, info, sizeof(buf)))
    {
        size = min(info->DataLength, size - sizeof(WCHAR));
        memcpy(buffer, info->Data, size);
        buffer[size / sizeof(WCHAR)] = 0;
        return 0;
    }

    return ERROR_FILE_NOT_FOUND;
}


/***********************************************************************
 *              setup_options
 *
 * Set up the Mac driver options.
 */
static void setup_options(void)
{
    static const WCHAR macdriverW[] = {'\\','M','a','c',' ','D','r','i','v','e','r',0};
    WCHAR buffer[MAX_PATH + 16], *p, *appname;
    HKEY hkey, appkey = 0;
    DWORD len;

    /* @@ Wine registry key: HKCU\Software\Wine\Mac Driver */
    hkey = open_hkcu_key("Software\\Wine\\Mac Driver");

    /* open the app-specific key */

    appname = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    if ((p = wcsrchr(appname, '/'))) appname = p + 1;
    if ((p = wcsrchr(appname, '\\'))) appname = p + 1;
    len = lstrlenW(appname);

    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        memcpy(buffer, appname, len * sizeof(WCHAR));
        memcpy(buffer + len, macdriverW, sizeof(macdriverW));
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Mac Driver */
        if ((tmpkey = open_hkcu_key("Software\\Wine\\AppDefaults")))
        {
            appkey = reg_open_key(tmpkey, buffer, lstrlenW(buffer) * sizeof(WCHAR));
            NtClose(tmpkey);
        }
    }

    if (!get_config_key(hkey, appkey, "WindowsFloatWhenInactive", buffer, sizeof(buffer)))
    {
        static const WCHAR noneW[] = {'n','o','n','e',0};
        static const WCHAR allW[] = {'a','l','l',0};
        if (!wcscmp(buffer, noneW))
            topmost_float_inactive = TOPMOST_FLOAT_INACTIVE_NONE;
        else if (!wcscmp(buffer, allW))
            topmost_float_inactive = TOPMOST_FLOAT_INACTIVE_ALL;
        else
            topmost_float_inactive = TOPMOST_FLOAT_INACTIVE_NONFULLSCREEN;
    }

    if (!get_config_key(hkey, appkey, "CaptureDisplaysForFullscreen", buffer, sizeof(buffer)))
        capture_displays_for_fullscreen = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "SkipSingleBufferFlushes", buffer, sizeof(buffer)))
        skip_single_buffer_flushes = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "AllowVerticalSync", buffer, sizeof(buffer)))
        allow_vsync = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "AllowSetGamma", buffer, sizeof(buffer)))
        allow_set_gamma = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "LeftOptionIsAlt", buffer, sizeof(buffer)))
        left_option_is_alt = IS_OPTION_TRUE(buffer[0]);
    if (!get_config_key(hkey, appkey, "RightOptionIsAlt", buffer, sizeof(buffer)))
        right_option_is_alt = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "LeftCommandIsCtrl", buffer, sizeof(buffer)))
        left_command_is_ctrl = IS_OPTION_TRUE(buffer[0]);
    if (!get_config_key(hkey, appkey, "RightCommandIsCtrl", buffer, sizeof(buffer)))
        right_command_is_ctrl = IS_OPTION_TRUE(buffer[0]);

    if (left_command_is_ctrl && right_command_is_ctrl && !left_option_is_alt && !right_option_is_alt)
        WARN("Both Command keys have been mapped to Control. There is no way to "
             "send an Alt key to Windows applications. Consider enabling "
             "LeftOptionIsAlt or RightOptionIsAlt.\n");

    if (!get_config_key(hkey, appkey, "AllowSoftwareRendering", buffer, sizeof(buffer)))
        allow_software_rendering = IS_OPTION_TRUE(buffer[0]);

    /* Value name chosen to match what's used in the X11 driver. */
    if (!get_config_key(hkey, appkey, "Decorated", buffer, sizeof(buffer)))
        disable_window_decorations = !IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "AllowImmovableWindows", buffer, sizeof(buffer)))
        allow_immovable_windows = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "UseConfinementCursorClipping", buffer, sizeof(buffer)))
        use_confinement_cursor_clipping = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "CursorClippingLocksWindows", buffer, sizeof(buffer)))
        cursor_clipping_locks_windows = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "UsePreciseScrolling", buffer, sizeof(buffer)))
        use_precise_scrolling = IS_OPTION_TRUE(buffer[0]);

    if (!get_config_key(hkey, appkey, "OpenGLSurfaceMode", buffer, sizeof(buffer)))
    {
        static const WCHAR transparentW[] = {'t','r','a','n','s','p','a','r','e','n','t',0};
        static const WCHAR behindW[] = {'b','e','h','i','n','d',0};
        if (!wcscmp(buffer, transparentW))
            gl_surface_mode = GL_SURFACE_IN_FRONT_TRANSPARENT;
        else if (!wcscmp(buffer, behindW))
            gl_surface_mode = GL_SURFACE_BEHIND;
        else
            gl_surface_mode = GL_SURFACE_IN_FRONT_OPAQUE;
    }

    if (!get_config_key(hkey, appkey, "EnableAppNap", buffer, sizeof(buffer)))
        enable_app_nap = IS_OPTION_TRUE(buffer[0]);

    /* Don't use appkey.  The DPI and monitor sizes should be consistent for all
       processes in the prefix. */
    if (!get_config_key(hkey, NULL, "RetinaMode", buffer, sizeof(buffer)))
        retina_enabled = IS_OPTION_TRUE(buffer[0]);

    retina_on = retina_enabled;

    if (appkey) NtClose(appkey);
    if (hkey) NtClose(hkey);
}


/***********************************************************************
 *              load_strings
 */
static void load_strings(struct localized_string *str)
{
    CFMutableDictionaryRef dict;

    dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
    if (!dict)
    {
        ERR("Failed to create localized strings dictionary\n");
        return;
    }

    while (str->id)
    {
        if (str->str && str->len)
        {
            const UniChar *ptr = param_ptr(str->str);
            CFNumberRef key = CFNumberCreate(NULL, kCFNumberIntType, &str->id);
            CFStringRef value = CFStringCreateWithCharacters(NULL, ptr, str->len);
            if (key && value)
                CFDictionarySetValue(dict, key, value);
            else
                ERR("Failed to add string ID 0x%04x %s\n", str->id, debugstr_wn(ptr, str->len));
        }
        else
            ERR("Failed to load string ID 0x%04x\n", str->id);
        str++;
    }

    localized_strings = dict;
}


/***********************************************************************
 *              macdrv_init
 */
static NTSTATUS macdrv_init(void *arg)
{
    struct init_params *params = arg;
    SessionAttributeBits attributes;
    OSStatus status;

    status = SessionGetInfo(callerSecuritySession, NULL, &attributes);
    if (status != noErr || !(attributes & sessionHasGraphicAccess))
        return STATUS_UNSUCCESSFUL;

    init_win_context();
    setup_options();
    load_strings(params->strings);

    macdrv_err_on = ERR_ON(macdrv);
    if (macdrv_start_cocoa_app(NtGetTickCount()))
    {
        ERR("Failed to start Cocoa app main loop\n");
        return STATUS_UNSUCCESSFUL;
    }

    init_user_driver();
    macdrv_init_display_devices(FALSE);
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              ThreadDetach   (MACDRV.@)
 */
void macdrv_ThreadDetach(void)
{
    struct macdrv_thread_data *data = macdrv_thread_data();

    if (data)
    {
        macdrv_destroy_event_queue(data->queue);
        if (data->keyboard_layout_uchr)
            CFRelease(data->keyboard_layout_uchr);
        free(data);
        /* clear data in case we get re-entered from user32 before the thread is truly dead */
        NtUserGetThreadInfo()->driver_data = 0;
    }
}


/***********************************************************************
 *              set_queue_display_fd
 *
 * Store the event queue fd into the message queue
 */
static void set_queue_display_fd(int fd)
{
    HANDLE handle;
    int ret;

    if (wine_server_fd_to_handle(fd, GENERIC_READ | SYNCHRONIZE, 0, &handle))
    {
        MESSAGE("macdrv: Can't allocate handle for event queue fd\n");
        NtTerminateProcess(0, 1);
    }
    SERVER_START_REQ(set_queue_fd)
    {
        req->handle = wine_server_obj_handle(handle);
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;
    if (ret)
    {
        MESSAGE("macdrv: Can't store handle for event queue fd\n");
        NtTerminateProcess(0, 1);
    }
    NtClose(handle);
}


/***********************************************************************
 *              macdrv_init_thread_data
 */
struct macdrv_thread_data *macdrv_init_thread_data(void)
{
    struct macdrv_thread_data *data = macdrv_thread_data();
    TISInputSourceRef input_source;

    if (data) return data;

    if (!(data = calloc(1, sizeof(*data))))
    {
        ERR("could not create data\n");
        NtTerminateProcess(0, 1);
    }

    if (!(data->queue = macdrv_create_event_queue(macdrv_handle_event)))
    {
        ERR("macdrv: Can't create event queue.\n");
        NtTerminateProcess(0, 1);
    }

    macdrv_get_input_source_info(&data->keyboard_layout_uchr, &data->keyboard_type, &data->iso_keyboard, &input_source);
    data->active_keyboard_layout = macdrv_get_hkl_from_source(input_source);
    CFRelease(input_source);
    macdrv_compute_keyboard_layout(data);

    set_queue_display_fd(macdrv_get_event_queue_fd(data->queue));
    NtUserGetThreadInfo()->driver_data = (UINT_PTR)data;

    NtUserActivateKeyboardLayout(data->active_keyboard_layout, 0);
    return data;
}


/***********************************************************************
 *              SystemParametersInfo (MACDRV.@)
 */
BOOL macdrv_SystemParametersInfo( UINT action, UINT int_param, void *ptr_param, UINT flags )
{
    switch (action)
    {
    case SPI_GETSCREENSAVEACTIVE:
        if (ptr_param)
        {
            CFDictionaryRef assertionStates;
            IOReturn status = IOPMCopyAssertionsStatus(&assertionStates);
            if (status == kIOReturnSuccess)
            {
                CFNumberRef count = CFDictionaryGetValue(assertionStates, kIOPMAssertionTypeNoDisplaySleep);
                CFNumberRef count2 = CFDictionaryGetValue(assertionStates, kIOPMAssertionTypePreventUserIdleDisplaySleep);
                long longCount = 0, longCount2 = 0;

                if (count)
                    CFNumberGetValue(count, kCFNumberLongType, &longCount);
                if (count2)
                    CFNumberGetValue(count2, kCFNumberLongType, &longCount2);

                *(BOOL *)ptr_param = !longCount && !longCount2;
                CFRelease(assertionStates);
            }
            else
            {
                WARN("Could not determine screen saver state, error code %d\n", status);
                *(BOOL *)ptr_param = TRUE;
            }
            return TRUE;
        }
        break;

    case SPI_SETSCREENSAVEACTIVE:
        {
            static IOPMAssertionID powerAssertion = kIOPMNullAssertionID;
            if (int_param)
            {
                if (powerAssertion != kIOPMNullAssertionID)
                {
                    IOPMAssertionRelease(powerAssertion);
                    powerAssertion = kIOPMNullAssertionID;
                }
            }
            else if (powerAssertion == kIOPMNullAssertionID)
            {
                IOPMAssertionCreateWithName( kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn,
                                             CFSTR("Wine Process requesting no screen saver"),
                                             &powerAssertion);
            }
        }
        break;
    }
    return FALSE;
}


NTSTATUS macdrv_client_func(enum macdrv_client_funcs id, const void *params, ULONG size)
{
    void *ret_ptr;
    ULONG ret_len;
    return KeUserModeCallback(id, params, size, &ret_ptr, &ret_len);
}


static NTSTATUS macdrv_quit_result(void *arg)
{
    struct quit_result_params *params = arg;
    macdrv_quit_reply(params->result);
    return 0;
}


const unixlib_entry_t __wine_unix_call_funcs[] =
{
    macdrv_dnd_get_data,
    macdrv_dnd_get_formats,
    macdrv_dnd_have_format,
    macdrv_dnd_release,
    macdrv_dnd_retain,
    macdrv_init,
    macdrv_quit_result,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

static NTSTATUS wow64_dnd_get_data(void *arg)
{
    struct
    {
        UINT64 handle;
        UINT format;
        ULONG size;
        ULONG data;
    } *params32 = arg;
    struct dnd_get_data_params params;

    params.handle = params32->handle;
    params.format = params32->format;
    params.size = params32->size;
    params.data = UlongToPtr(params32->data);
    return macdrv_dnd_get_data(&params);
}

static NTSTATUS wow64_init(void *arg)
{
    struct
    {
        ULONG strings;
    } *params32 = arg;
    struct init_params params;

    params.strings = UlongToPtr(params32->strings);
    return macdrv_init(&params);
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    wow64_dnd_get_data,
    macdrv_dnd_get_formats,
    macdrv_dnd_have_format,
    macdrv_dnd_release,
    macdrv_dnd_retain,
    wow64_init,
    macdrv_quit_result,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif /* _WIN64 */
