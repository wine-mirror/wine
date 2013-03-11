/*
 * Mac clipboard driver
 *
 * Copyright 1994 Martin Ayotte
 *           1996 Alex Korobka
 *           1999 Noel Borthwick
 *           2003 Ulrich Czekalla for CodeWeavers
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

#include "config.h"

#include "macdrv.h"
#include "winuser.h"
#include "wine/list.h"
#include "wine/server.h"
#include "wine/unicode.h"


WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


/**************************************************************************
 *              Types
 **************************************************************************/

typedef struct
{
    HWND hwnd_owner;
    UINT flags;
} CLIPBOARDINFO, *LPCLIPBOARDINFO;

typedef HANDLE (*DRVIMPORTFUNC)(CFDataRef data);
typedef CFDataRef (*DRVEXPORTFUNC)(HANDLE data);

typedef struct
{
    struct list     entry;
    UINT            format_id;
    CFStringRef     type;
    DRVIMPORTFUNC   import_func;
    DRVEXPORTFUNC   export_func;
    BOOL            synthesized;
} WINE_CLIPFORMAT;


/**************************************************************************
 *              Constants
 **************************************************************************/


/**************************************************************************
 *              Forward Function Declarations
 **************************************************************************/

static HANDLE import_clipboard_data(CFDataRef data);
static HANDLE import_oemtext_to_text(CFDataRef data);
static HANDLE import_oemtext_to_unicodetext(CFDataRef data);
static HANDLE import_text_to_oemtext(CFDataRef data);
static HANDLE import_text_to_unicodetext(CFDataRef data);
static HANDLE import_unicodetext_to_oemtext(CFDataRef data);
static HANDLE import_unicodetext_to_text(CFDataRef data);
static HANDLE import_utf8_to_oemtext(CFDataRef data);
static HANDLE import_utf8_to_text(CFDataRef data);
static HANDLE import_utf8_to_unicodetext(CFDataRef data);

static CFDataRef export_clipboard_data(HANDLE data);
static CFDataRef export_oemtext_to_utf8(HANDLE data);
static CFDataRef export_text_to_utf8(HANDLE data);
static CFDataRef export_unicodetext_to_utf8(HANDLE data);


/**************************************************************************
 *              Static Variables
 **************************************************************************/

/* Clipboard formats */
static struct list format_list = LIST_INIT(format_list);

/*  There are two naming schemes involved and we want to have a mapping between
    them.  There are Win32 clipboard format names and there are Mac pasteboard
    types.

    The Win32 standard clipboard formats don't have names, but they are associated
    with Mac pasteboard types through the following tables, which are used to
    initialize the format_list.  Where possible, the standard clipboard formats
    are mapped to predefined pasteboard type UTIs.  Otherwise, we create Wine-
    specific types of the form "org.winehq.builtin.<format>", where <format> is
    the name of the symbolic constant for the format minus "CF_" and lowercased.
    E.g. CF_BITMAP -> org.winehq.builtin.bitmap.

    Win32 clipboard formats which originate in a Windows program may be registered
    with an arbitrary name.  We construct a Mac pasteboard type from these by
    prepending "org.winehq.registered." to the registered name.

    Likewise, Mac pasteboard types which originate in other apps may have
    arbitrary type strings.  We construct a Win32 clipboard format name from
    these by prepending "org.winehq.mac-type." to the Mac pasteboard type.

    Summary:
    Win32 clipboard format names:
        <none>                              standard clipboard format; maps via
                                            format_list to either a predefined Mac UTI
                                            or org.winehq.builtin.<format>.
        org.winehq.mac-type.<Mac type>      representation of Mac type in Win32 land;
                                            maps to <Mac type>
        <other>                             name registered within Win32 land; maps to
                                            org.winehq.registered.<other>
    Mac pasteboard type names:
        org.winehq.builtin.<format ID>      representation of Win32 standard clipboard
                                            format for which there was no corresponding
                                            predefined Mac UTI; maps via format_list
        org.winehq.registered.<format name> representation of Win32 registered
                                            clipboard format name; maps to <format name>
        <other>                             Mac pasteboard type originating with system
                                            or other apps; either maps via format_list
                                            to a standard clipboard format or maps to
                                            org.winehq.mac-type.<other>
*/

static const struct
{
    UINT          id;
    CFStringRef   type;
    DRVIMPORTFUNC import;
    DRVEXPORTFUNC export;
    BOOL          synthesized;
} builtin_format_ids[] =
{
    { CF_UNICODETEXT,       CFSTR("org.winehq.builtin.unicodetext"),        import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_TEXT,              CFSTR("org.winehq.builtin.unicodetext"),        import_unicodetext_to_text,     NULL,                       TRUE },
    { CF_OEMTEXT,           CFSTR("org.winehq.builtin.unicodetext"),        import_unicodetext_to_oemtext,  NULL,                       TRUE },

    { CF_TEXT,              CFSTR("org.winehq.builtin.text"),               import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_UNICODETEXT,       CFSTR("org.winehq.builtin.text"),               import_text_to_unicodetext,     NULL,                       TRUE },
    { CF_OEMTEXT,           CFSTR("org.winehq.builtin.text"),               import_text_to_oemtext,         NULL,                       TRUE },

    { CF_OEMTEXT,           CFSTR("org.winehq.builtin.oemtext"),            import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_UNICODETEXT,       CFSTR("org.winehq.builtin.oemtext"),            import_oemtext_to_unicodetext,  NULL,                       TRUE },
    { CF_TEXT,              CFSTR("org.winehq.builtin.oemtext"),            import_oemtext_to_text,         NULL,                       TRUE },

    { CF_UNICODETEXT,       CFSTR("public.utf8-plain-text"),                import_utf8_to_unicodetext,     export_unicodetext_to_utf8, TRUE },
    { CF_TEXT,              CFSTR("public.utf8-plain-text"),                import_utf8_to_text,            export_text_to_utf8,        TRUE },
    { CF_OEMTEXT,           CFSTR("public.utf8-plain-text"),                import_utf8_to_oemtext,         export_oemtext_to_utf8,     TRUE },
};

/* The prefix prepended to an external Mac pasteboard type to make a Win32 clipboard format name. org.winehq.mac-type. */
static const WCHAR mac_type_name_prefix[] = {'o','r','g','.','w','i','n','e','h','q','.','m','a','c','-','t','y','p','e','.',0};

/* The prefix prepended to a Win32 clipboard format name to make a Mac pasteboard type. */
static const CFStringRef registered_name_type_prefix = CFSTR("org.winehq.registered.");


/**************************************************************************
 *              Internal Clipboard implementation methods
 **************************************************************************/

/*
 * format_list functions
 */

/**************************************************************************
 *              debugstr_format
 */
static const char *debugstr_format(UINT id)
{
    WCHAR buffer[256];

    if (GetClipboardFormatNameW(id, buffer, 256))
        return wine_dbg_sprintf("0x%04x %s", id, debugstr_w(buffer));

    switch (id)
    {
#define BUILTIN(id) case id: return #id;
    BUILTIN(CF_TEXT)
    BUILTIN(CF_BITMAP)
    BUILTIN(CF_METAFILEPICT)
    BUILTIN(CF_SYLK)
    BUILTIN(CF_DIF)
    BUILTIN(CF_TIFF)
    BUILTIN(CF_OEMTEXT)
    BUILTIN(CF_DIB)
    BUILTIN(CF_PALETTE)
    BUILTIN(CF_PENDATA)
    BUILTIN(CF_RIFF)
    BUILTIN(CF_WAVE)
    BUILTIN(CF_UNICODETEXT)
    BUILTIN(CF_ENHMETAFILE)
    BUILTIN(CF_HDROP)
    BUILTIN(CF_LOCALE)
    BUILTIN(CF_DIBV5)
    BUILTIN(CF_OWNERDISPLAY)
    BUILTIN(CF_DSPTEXT)
    BUILTIN(CF_DSPBITMAP)
    BUILTIN(CF_DSPMETAFILEPICT)
    BUILTIN(CF_DSPENHMETAFILE)
#undef BUILTIN
    default: return wine_dbg_sprintf("0x%04x", id);
    }
}


/**************************************************************************
 *              insert_clipboard_format
 */
static WINE_CLIPFORMAT *insert_clipboard_format(UINT id, CFStringRef type)
{
    WINE_CLIPFORMAT *format;

    format = HeapAlloc(GetProcessHeap(), 0, sizeof(*format));

    if (format == NULL)
    {
        WARN("No more memory for a new format!\n");
        return NULL;
    }
    format->format_id = id;
    format->import_func = import_clipboard_data;
    format->export_func = export_clipboard_data;
    format->synthesized = FALSE;

    if (type)
        format->type = CFStringCreateCopy(NULL, type);
    else
    {
        WCHAR buffer[256];

        GetClipboardFormatNameW(format->format_id, buffer, 256);
        if (!strncmpW(buffer, mac_type_name_prefix, strlenW(mac_type_name_prefix)))
        {
            const WCHAR *p = buffer + strlenW(mac_type_name_prefix);
            format->type = CFStringCreateWithCharacters(NULL, (UniChar*)p, strlenW(p));
        }
        else
        {
            format->type = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%S"),
                                                    registered_name_type_prefix, buffer);
        }
    }

    list_add_tail(&format_list, &format->entry);

    TRACE("Registering format %s type %s\n", debugstr_format(format->format_id),
          debugstr_cf(format->type));

    return format;
}


/**************************************************************************
 *              register_format
 *
 * Register a custom Mac clipboard format.
 */
static WINE_CLIPFORMAT* register_format(UINT id, CFStringRef type)
{
    WINE_CLIPFORMAT *format;

    /* walk format chain to see if it's already registered */
    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        if (format->format_id == id) return format;

    return insert_clipboard_format(id, type);
}


/**************************************************************************
 *              format_for_type
 */
static WINE_CLIPFORMAT* format_for_type(WINE_CLIPFORMAT *current, CFStringRef type)
{
    struct list *ptr = current ? &current->entry : &format_list;
    WINE_CLIPFORMAT *format = NULL;

    TRACE("current %p/%s type %s\n", current, debugstr_format(current ? current->format_id : 0), debugstr_cf(type));

    while ((ptr = list_next(&format_list, ptr)))
    {
        format = LIST_ENTRY(ptr, WINE_CLIPFORMAT, entry);
        if (CFEqual(format->type, type))
        {
            TRACE(" -> %p/%s\n", format, debugstr_format(format->format_id));
            return format;
        }
    }

    if (!current)
    {
        LPWSTR name;

        if (CFStringHasPrefix(type, CFSTR("org.winehq.builtin.")))
        {
            ERR("Shouldn't happen. Built-in type %s should have matched something in format list.\n",
                debugstr_cf(type));
            return NULL;
        }
        else if (CFStringHasPrefix(type, registered_name_type_prefix))
        {
            int len = CFStringGetLength(type) - CFStringGetLength(registered_name_type_prefix);
            name = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
            CFStringGetCharacters(type, CFRangeMake(CFStringGetLength(registered_name_type_prefix), len),
                                  (UniChar*)name);
            name[len] = 0;
        }
        else
        {
            int len = strlenW(mac_type_name_prefix) + CFStringGetLength(type);
            name = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
            memcpy(name, mac_type_name_prefix, sizeof(mac_type_name_prefix));
            CFStringGetCharacters(type, CFRangeMake(0, CFStringGetLength(type)),
                                  (UniChar*)name + strlenW(mac_type_name_prefix));
            name[len] = 0;
        }

        format = register_format(RegisterClipboardFormatW(name), type);
        if (!format)
            ERR("Failed to register format for type %s name %s\n", debugstr_cf(type), debugstr_w(name));

        HeapFree(GetProcessHeap(), 0, name);
    }

    TRACE(" -> %p/%s\n", format, debugstr_format(format ? format->format_id : 0));
    return format;
}


/**************************************************************************
 *              convert_text
 *
 *  Convert string data between code pages or to/from wide characters.  The
 *  special value of (UINT)-1 for a code page indicates to use wide
 *  characters.
 */
static HANDLE convert_text(const void *src, int src_len, UINT src_cp, UINT dest_cp)
{
    HANDLE ret = NULL;
    const WCHAR *wstr;
    int wstr_len;
    HANDLE handle;
    char *p;

    if (src_cp == (UINT)-1)
    {
        wstr = src;
        wstr_len = src_len / sizeof(WCHAR);
    }
    else
    {
        WCHAR *temp;

        wstr_len = MultiByteToWideChar(src_cp, 0, src, src_len, NULL, 0);
        if (!src_len || ((const char*)src)[src_len - 1]) wstr_len += 1;
        temp = HeapAlloc(GetProcessHeap(), 0, wstr_len * sizeof(WCHAR));
        MultiByteToWideChar(src_cp, 0, src, src_len, temp, wstr_len);
        temp[wstr_len - 1] = 0;
        wstr = temp;
    }

    if (dest_cp == (UINT)-1)
    {
        handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, wstr_len * sizeof(WCHAR));
        if (handle && (p = GlobalLock(handle)))
        {
            memcpy(p, wstr, wstr_len * sizeof(WCHAR));
            GlobalUnlock(handle);
            ret = handle;
        }
    }
    else
    {
        INT len;

        len = WideCharToMultiByte(dest_cp, 0, wstr, wstr_len, NULL, 0, NULL, NULL);
        if (!wstr_len || wstr[wstr_len - 1]) len += 1;
        handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len);

        if (handle && (p = GlobalLock(handle)))
        {
            WideCharToMultiByte(dest_cp, 0, wstr, wstr_len, p, len, NULL, NULL);
            p[len - 1] = 0;
            GlobalUnlock(handle);
            ret = handle;
        }
    }

    return ret;
}


/**************************************************************************
 *              convert_unicodetext_to_codepage
 */
static HANDLE convert_unicodetext_to_codepage(HANDLE unicode_handle, UINT cp)
{
    LPWSTR unicode_string = GlobalLock(unicode_handle);
    HANDLE ret = NULL;

    if (unicode_string)
    {
        ret = convert_text(unicode_string, GlobalSize(unicode_handle), -1, cp);
        GlobalUnlock(unicode_handle);
    }

    return ret;
}


/**************************************************************************
 *              import_clipboard_data
 *
 *  Generic import clipboard data routine.
 */
static HANDLE import_clipboard_data(CFDataRef data)
{
    HANDLE data_handle = NULL;

    size_t len = CFDataGetLength(data);
    if (len)
    {
        LPVOID p;

        /* Turn on the DDESHARE flag to enable shared 32 bit memory */
        data_handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len);
        if (!data_handle)
            return NULL;

        if ((p = GlobalLock(data_handle)))
        {
            memcpy(p, CFDataGetBytePtr(data), len);
            GlobalUnlock(data_handle);
        }
        else
        {
            GlobalFree(data_handle);
            data_handle = NULL;
        }
    }

    return data_handle;
}


/**************************************************************************
 *              import_oemtext_to_text
 *
 *  Import CF_OEMTEXT data, converting the string to CF_TEXT.
 */
static HANDLE import_oemtext_to_text(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), CP_OEMCP, CP_ACP);
}


/**************************************************************************
 *              import_oemtext_to_unicodetext
 *
 *  Import CF_OEMTEXT data, converting the string to CF_UNICODETEXT.
 */
static HANDLE import_oemtext_to_unicodetext(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), CP_OEMCP, -1);
}


/**************************************************************************
 *              import_text_to_oemtext
 *
 *  Import CF_TEXT data, converting the string to CF_OEMTEXT.
 */
static HANDLE import_text_to_oemtext(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), CP_ACP, CP_OEMCP);
}


/**************************************************************************
 *              import_text_to_unicodetext
 *
 *  Import CF_TEXT data, converting the string to CF_UNICODETEXT.
 */
static HANDLE import_text_to_unicodetext(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), CP_ACP, -1);
}


/**************************************************************************
 *              import_unicodetext_to_oemtext
 *
 *  Import a CF_UNICODETEXT string, converting the string to CF_OEMTEXT.
 */
static HANDLE import_unicodetext_to_oemtext(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), -1, CP_OEMCP);
}


/**************************************************************************
 *              import_unicodetext_to_text
 *
 *  Import a CF_UNICODETEXT string, converting the string to CF_TEXT.
 */
static HANDLE import_unicodetext_to_text(CFDataRef data)
{
    return convert_text(CFDataGetBytePtr(data), CFDataGetLength(data), -1, CP_ACP);
}


/**************************************************************************
 *              import_utf8_to_oemtext
 *
 *  Import a UTF-8 string, converting the string to CF_OEMTEXT.
 */
static HANDLE import_utf8_to_oemtext(CFDataRef data)
{
    HANDLE unicode_handle = import_utf8_to_unicodetext(data);
    HANDLE ret = convert_unicodetext_to_codepage(unicode_handle, CP_OEMCP);

    GlobalFree(unicode_handle);
    return ret;
}


/**************************************************************************
 *              import_utf8_to_text
 *
 *  Import a UTF-8 string, converting the string to CF_TEXT.
 */
static HANDLE import_utf8_to_text(CFDataRef data)
{
    HANDLE unicode_handle = import_utf8_to_unicodetext(data);
    HANDLE ret = convert_unicodetext_to_codepage(unicode_handle, CP_ACP);

    GlobalFree(unicode_handle);
    return ret;
}


/**************************************************************************
 *              import_utf8_to_unicodetext
 *
 *  Import a UTF-8 string, converting the string to CF_UNICODETEXT.
 */
static HANDLE import_utf8_to_unicodetext(CFDataRef data)
{
    const BYTE *src;
    unsigned long data_len;
    unsigned long new_lines = 0;
    LPSTR dst;
    unsigned long i, j;
    HANDLE unicode_handle = NULL;

    src = CFDataGetBytePtr(data);
    data_len = CFDataGetLength(data);
    for (i = 0; i < data_len; i++)
    {
        if (src[i] == '\n')
            new_lines++;
    }

    if ((dst = HeapAlloc(GetProcessHeap(), 0, data_len + new_lines + 1)))
    {
        UINT count;

        for (i = 0, j = 0; i < data_len; i++)
        {
            if (src[i] == '\n')
                dst[j++] = '\r';

            dst[j++] = src[i];
        }
        dst[j] = 0;

        count = MultiByteToWideChar(CP_UTF8, 0, dst, -1, NULL, 0);
        unicode_handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, count * sizeof(WCHAR));

        if (unicode_handle)
        {
            WCHAR *textW = GlobalLock(unicode_handle);
            MultiByteToWideChar(CP_UTF8, 0, dst, -1, textW, count);
            GlobalUnlock(unicode_handle);
        }

        HeapFree(GetProcessHeap(), 0, dst);
    }

    return unicode_handle;
}


/**************************************************************************
 *              export_clipboard_data
 *
 *  Generic export clipboard data routine.
 */
static CFDataRef export_clipboard_data(HANDLE data)
{
    CFDataRef ret;
    UINT len;
    LPVOID src;

    len = GlobalSize(data);
    src = GlobalLock(data);
    if (!src) return NULL;

    ret = CFDataCreate(NULL, src, len);
    GlobalUnlock(data);

    return ret;
}


/**************************************************************************
 *              export_codepage_to_utf8
 *
 *  Export string data in a specified codepage to UTF-8.
 */
static CFDataRef export_codepage_to_utf8(HANDLE data, UINT cp)
{
    CFDataRef ret = NULL;
    const char* str;

    if ((str = GlobalLock(data)))
    {
        HANDLE unicode = convert_text(str, GlobalSize(data), cp, -1);

        ret = export_unicodetext_to_utf8(unicode);

        GlobalFree(unicode);
        GlobalUnlock(data);
    }

    return ret;
}


/**************************************************************************
 *              export_oemtext_to_utf8
 *
 *  Export CF_OEMTEXT to UTF-8.
 */
static CFDataRef export_oemtext_to_utf8(HANDLE data)
{
    return export_codepage_to_utf8(data, CP_OEMCP);
}


/**************************************************************************
 *              export_text_to_utf8
 *
 *  Export CF_TEXT to UTF-8.
 */
static CFDataRef export_text_to_utf8(HANDLE data)
{
    return export_codepage_to_utf8(data, CP_ACP);
}


/**************************************************************************
 *              export_unicodetext_to_utf8
 *
 *  Export CF_UNICODETEXT to UTF-8.
 */
static CFDataRef export_unicodetext_to_utf8(HANDLE data)
{
    CFMutableDataRef ret;
    LPVOID src;
    INT dst_len;

    src = GlobalLock(data);
    if (!src) return NULL;

    dst_len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
    if (dst_len) dst_len--; /* Leave off null terminator. */
    ret = CFDataCreateMutable(NULL, dst_len);
    if (ret)
    {
        LPSTR dst;
        int i, j;

        CFDataSetLength(ret, dst_len);
        dst = (LPSTR)CFDataGetMutableBytePtr(ret);
        WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dst_len, NULL, NULL);

        /* Remove carriage returns */
        for (i = 0, j = 0; i < dst_len; i++)
        {
            if (dst[i] == '\r' &&
                (i + 1 >= dst_len || dst[i + 1] == '\n' || dst[i + 1] == '\0'))
                continue;
            dst[j++] = dst[i];
        }
        CFDataSetLength(ret, j);
    }
    GlobalUnlock(data);

    return ret;
}


/**************************************************************************
 *              get_clipboard_info
 */
static BOOL get_clipboard_info(LPCLIPBOARDINFO cbinfo)
{
    BOOL ret = FALSE;

    SERVER_START_REQ(set_clipboard_info)
    {
        req->flags = 0;

        if (wine_server_call_err(req))
        {
            ERR("Failed to get clipboard owner.\n");
        }
        else
        {
            cbinfo->hwnd_owner = wine_server_ptr_handle(reply->old_owner);
            cbinfo->flags = reply->flags;

            ret = TRUE;
        }
    }
    SERVER_END_REQ;

    return ret;
}


/**************************************************************************
 *              release_ownership
 */
static BOOL release_ownership(void)
{
    BOOL ret = FALSE;

    SERVER_START_REQ(set_clipboard_info)
    {
        req->flags = SET_CB_RELOWNER | SET_CB_SEQNO;

        if (wine_server_call_err(req))
            ERR("Failed to set clipboard.\n");
        else
            ret = TRUE;
    }
    SERVER_END_REQ;

    return ret;
}


/**************************************************************************
 *              check_clipboard_ownership
 */
static void check_clipboard_ownership(HWND *owner)
{
    CLIPBOARDINFO cbinfo;

    if (owner) *owner = NULL;

    /* If Wine thinks we're the clipboard owner but Mac OS X thinks we're not
       the pasteboard owner, update Wine. */
    if (get_clipboard_info(&cbinfo) && (cbinfo.flags & CB_PROCESS))
    {
        if (!(cbinfo.flags & CB_OPEN) && !macdrv_is_pasteboard_owner())
        {
            TRACE("Lost clipboard ownership\n");

            if (OpenClipboard(cbinfo.hwnd_owner))
            {
                /* Destroy private objects */
                SendMessageW(cbinfo.hwnd_owner, WM_DESTROYCLIPBOARD, 0, 0);

                /* Give up ownership of the windows clipboard */
                release_ownership();
                CloseClipboard();
            }
        }
        else if (owner)
            *owner = cbinfo.hwnd_owner;
    }
}


/**************************************************************************
 *              Mac User Driver Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              AcquireClipboard (MACDRV.@)
 */
int CDECL macdrv_AcquireClipboard(HWND hwnd)
{
    TRACE("hwnd %p\n", hwnd);
    check_clipboard_ownership(NULL);
    return 0;
}


/**************************************************************************
 *              CountClipboardFormats (MACDRV.@)
 */
INT CDECL macdrv_CountClipboardFormats(void)
{
    CFMutableSetRef seen_formats;
    CFArrayRef types;
    CFIndex count;
    CFIndex i;
    INT ret = 0;

    TRACE("()\n");
    check_clipboard_ownership(NULL);

    seen_formats = CFSetCreateMutable(NULL, 0, NULL);
    if (!seen_formats)
    {
        WARN("Failed to allocate set to track seen formats\n");
        return 0;
    }

    types = macdrv_copy_pasteboard_types();
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        CFRelease(seen_formats);
        return 0;
    }

    count = CFArrayGetCount(types);
    TRACE("got %ld types\n", count);

    for (i = 0; i < count; i++)
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, i);
        WINE_CLIPFORMAT* format;

        format = NULL;
        while ((format = format_for_type(format, type)))
        {
            TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));

            if (!CFSetContainsValue(seen_formats, (void*)format->format_id))
            {
                ret++;
                CFSetAddValue(seen_formats, (void*)format->format_id);
            }
        }
    }

    CFRelease(seen_formats);
    TRACE(" -> %d\n", ret);
    return ret;
}


/**************************************************************************
 *              EmptyClipboard (MACDRV.@)
 *
 * Empty cached clipboard data.
 */
void CDECL macdrv_EmptyClipboard(BOOL keepunowned)
{
    TRACE("keepunowned %d\n", keepunowned);
    macdrv_clear_pasteboard();
}


/**************************************************************************
 *              EndClipboardUpdate (MACDRV.@)
 */
void CDECL macdrv_EndClipboardUpdate(void)
{
    TRACE("()\n");
    check_clipboard_ownership(NULL);
}


/**************************************************************************
 *              EnumClipboardFormats (MACDRV.@)
 */
UINT CDECL macdrv_EnumClipboardFormats(UINT prev_format)
{
    CFArrayRef types;
    CFIndex count;
    CFIndex i;
    UINT ret;

    TRACE("prev_format %s\n", debugstr_format(prev_format));
    check_clipboard_ownership(NULL);

    types = macdrv_copy_pasteboard_types();
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return 0;
    }

    count = CFArrayGetCount(types);
    TRACE("got %ld types\n", count);

    if (!count)
    {
        CFRelease(types);
        return 0;
    }

    if (prev_format)
    {
        CFMutableArrayRef formats = CFArrayCreateMutable(NULL, 0, NULL);
        if (!formats)
        {
            WARN("Failed to allocate array to track formats\n");
            CFRelease(types);
            return 0;
        }

        for (i = 0; i < count; i++)
        {
            CFStringRef type = CFArrayGetValueAtIndex(types, i);
            WINE_CLIPFORMAT* format;

            format = NULL;
            while ((format = format_for_type(format, type)))
            {
                TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));

                if (format->synthesized)
                {
                    /* Don't override a real value with a synthesized value. */
                    if (!CFArrayContainsValue(formats, CFRangeMake(0, CFArrayGetCount(formats)), (void*)format->format_id))
                        CFArrayAppendValue(formats, (void*)format->format_id);
                }
                else
                {
                    /* If the type was already in the array, it must have been synthesized
                       because this one's real.  Remove the synthesized entry in favor of
                       this one. */
                    CFIndex index = CFArrayGetFirstIndexOfValue(formats, CFRangeMake(0, CFArrayGetCount(formats)),
                                                                (void*)format->format_id);
                    if (index != kCFNotFound)
                        CFArrayRemoveValueAtIndex(formats, index);
                    CFArrayAppendValue(formats, (void*)format->format_id);
                }
            }
        }

        count = CFArrayGetCount(formats);
        i = CFArrayGetFirstIndexOfValue(formats, CFRangeMake(0, count), (void*)prev_format);
        if (i == kCFNotFound || i + 1 >= count)
            ret = 0;
        else
            ret = (UINT)CFArrayGetValueAtIndex(formats, i + 1);

        CFRelease(formats);
    }
    else
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, 0);
        WINE_CLIPFORMAT *format = format_for_type(NULL, type);

        ret = format ? format->format_id : 0;
    }

    CFRelease(types);
    TRACE(" -> %u\n", ret);
    return ret;
}


/**************************************************************************
 *              GetClipboardData (MACDRV.@)
 */
HANDLE CDECL macdrv_GetClipboardData(UINT desired_format)
{
    CFArrayRef types;
    CFIndex count;
    CFIndex i;
    CFStringRef type, best_type;
    WINE_CLIPFORMAT* best_format = NULL;
    HANDLE data = NULL;

    TRACE("desired_format %s\n", debugstr_format(desired_format));
    check_clipboard_ownership(NULL);

    types = macdrv_copy_pasteboard_types();
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return NULL;
    }

    count = CFArrayGetCount(types);
    TRACE("got %ld types\n", count);

    for (i = 0; (!best_format || best_format->synthesized) && i < count; i++)
    {
        WINE_CLIPFORMAT* format;

        type = CFArrayGetValueAtIndex(types, i);

        format = NULL;
        while ((!best_format || best_format->synthesized) && (format = format_for_type(format, type)))
        {
            TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format ? format->format_id : 0));

            if (format->format_id == desired_format)
            {
                /* The best format is the matching one which is not synthesized.  Failing that,
                   the best format is the first matching synthesized format. */
                if (!format->synthesized || !best_format)
                {
                    best_type = type;
                    best_format = format;
                }
            }
        }
    }

    if (best_format)
    {
        CFDataRef pasteboard_data = macdrv_copy_pasteboard_data(best_type);

        TRACE("got pasteboard data for type %s: %s\n", debugstr_cf(best_type), debugstr_cf(pasteboard_data));

        if (pasteboard_data)
        {
            data = best_format->import_func(pasteboard_data);
            CFRelease(pasteboard_data);
        }
    }

    CFRelease(types);
    TRACE(" -> %p\n", data);
    return data;
}


/**************************************************************************
 *              IsClipboardFormatAvailable (MACDRV.@)
 */
BOOL CDECL macdrv_IsClipboardFormatAvailable(UINT desired_format)
{
    CFArrayRef types;
    int count;
    UINT i;
    BOOL found = FALSE;

    TRACE("desired_format %s\n", debugstr_format(desired_format));
    check_clipboard_ownership(NULL);

    types = macdrv_copy_pasteboard_types();
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return FALSE;
    }

    count = CFArrayGetCount(types);
    TRACE("got %d types\n", count);

    for (i = 0; !found && i < count; i++)
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, i);
        WINE_CLIPFORMAT* format;

        format = NULL;
        while (!found && (format = format_for_type(format, type)))
        {
            TRACE("for type %s got format %s\n", debugstr_cf(type), debugstr_format(format->format_id));

            if (format->format_id == desired_format)
                found = TRUE;
        }
    }

    CFRelease(types);
    TRACE(" -> %d\n", found);
    return found;
}


/**************************************************************************
 *              SetClipboardData (MACDRV.@)
 */
BOOL CDECL macdrv_SetClipboardData(UINT format_id, HANDLE data, BOOL owner)
{
    HWND hwnd_owner;
    macdrv_window window;
    WINE_CLIPFORMAT *format;
    CFDataRef cfdata = NULL;

    check_clipboard_ownership(&hwnd_owner);
    window = macdrv_get_cocoa_window(GetAncestor(hwnd_owner, GA_ROOT), FALSE);
    TRACE("format_id %s data %p owner %d hwnd_owner %p window %p)\n", debugstr_format(format_id), data, owner, hwnd_owner, window);

    /* Find the "natural" format for this format_id (the one which isn't
       synthesized from another type). */
    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        if (format->format_id == format_id && !format->synthesized) break;

    if (&format->entry == &format_list && !(format = insert_clipboard_format(format_id, NULL)))
    {
        WARN("Failed to register clipboard format %s\n", debugstr_format(format_id));
        return FALSE;
    }

    /* Export the data to the Mac pasteboard. */
    if (data)
    {
        if (!format->export_func || !(cfdata = format->export_func(data)))
        {
            WARN("Failed to export %s data to type %s\n", debugstr_format(format_id), debugstr_cf(format->type));
            return FALSE;
        }
    }

    if (macdrv_set_pasteboard_data(format->type, cfdata, window))
        TRACE("Set pasteboard data for type %s: %s\n", debugstr_cf(format->type), debugstr_cf(cfdata));
    else
    {
        WARN("Failed to set pasteboard data for type %s: %s\n", debugstr_cf(format->type), debugstr_cf(cfdata));
        if (cfdata) CFRelease(cfdata);
        return FALSE;
    }

    if (cfdata) CFRelease(cfdata);

    /* Find any other formats for this format_id (the exportable synthesized ones). */
    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
    {
        if (format->format_id == format_id && format->synthesized && format->export_func)
        {
            /* We have a synthesized format for this format ID.  Add its type to the pasteboard. */
            TRACE("Synthesized from format %s: type %s\n", debugstr_format(format_id), debugstr_cf(format->type));

            if (data)
            {
                cfdata = format->export_func(data);
                if (!cfdata)
                {
                    WARN("Failed to export %s data to type %s\n", debugstr_format(format->format_id), debugstr_cf(format->type));
                    continue;
                }
            }
            else
                cfdata = NULL;

            if (macdrv_set_pasteboard_data(format->type, cfdata, window))
                TRACE("    ... set pasteboard data: %s\n", debugstr_cf(cfdata));
            else
                WARN("    ... failed to set pasteboard data: %s\n", debugstr_cf(cfdata));

            if (cfdata) CFRelease(cfdata);
        }
    }

    if (data)
    {
        /* FIXME: According to MSDN, the caller is entitled to lock and read from
           data until CloseClipboard is called.  So, we should defer this cleanup. */
        if ((format_id >= CF_GDIOBJFIRST && format_id <= CF_GDIOBJLAST) ||
            format_id == CF_BITMAP ||
            format_id == CF_DIB ||
            format_id == CF_PALETTE)
        {
            DeleteObject(data);
        }
        else if (format_id == CF_METAFILEPICT)
        {
            DeleteMetaFile(((METAFILEPICT *)GlobalLock(data))->hMF);
            GlobalFree(data);
        }
        else if (format_id == CF_ENHMETAFILE)
        {
            DeleteEnhMetaFile(data);
        }
        else if (format_id < CF_PRIVATEFIRST || CF_PRIVATELAST < format_id)
        {
            GlobalFree(data);
        }
    }

    return TRUE;
}


/**************************************************************************
 *              MACDRV Private Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              macdrv_clipboard_process_attach
 */
void macdrv_clipboard_process_attach(void)
{
    UINT i;

    /* Register built-in formats */
    for (i = 0; i < sizeof(builtin_format_ids)/sizeof(builtin_format_ids[0]); i++)
    {
        WINE_CLIPFORMAT *format;

        if (!(format = HeapAlloc(GetProcessHeap(), 0, sizeof(*format)))) break;
        format->format_id   = builtin_format_ids[i].id;
        format->type        = CFRetain(builtin_format_ids[i].type);
        format->import_func = builtin_format_ids[i].import;
        format->export_func = builtin_format_ids[i].export;
        format->synthesized = builtin_format_ids[i].synthesized;
        list_add_tail(&format_list, &format->entry);
    }
}


/**************************************************************************
 *              query_pasteboard_data
 */
BOOL query_pasteboard_data(HWND hwnd, CFStringRef type)
{
    BOOL ret = FALSE;
    CLIPBOARDINFO cbinfo;
    WINE_CLIPFORMAT* format;
    CFArrayRef types = NULL;
    CFRange range;

    TRACE("hwnd %p type %s\n", hwnd, debugstr_cf(type));

    if (get_clipboard_info(&cbinfo))
        hwnd = cbinfo.hwnd_owner;

    format = NULL;
    while ((format = format_for_type(format, type)))
    {
        WINE_CLIPFORMAT* base_format;

        TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));

        if (!format->synthesized)
        {
            TRACE("Sending WM_RENDERFORMAT message for format %s to hwnd %p\n", debugstr_format(format->format_id), hwnd);
            SendMessageW(hwnd, WM_RENDERFORMAT, format->format_id, 0);
            ret = TRUE;
            goto done;
        }

        if (!types)
        {
            types = macdrv_copy_pasteboard_types();
            if (!types)
            {
                WARN("Failed to copy pasteboard types\n");
                break;
            }

            range = CFRangeMake(0, CFArrayGetCount(types));
        }

        /* The type maps to a synthesized format.  Now look up what type that format maps to natively
           (not synthesized).  For example, if type is "public.utf8-plain-text", then this format may
           have an ID of CF_TEXT.  From CF_TEXT, we want to find "org.winehq.builtin.text" to see if
           that type is present in the pasteboard.  If it is, then the app must have promised it and
           we can ask it to render it.  (If it had put it on the clipboard immediately, then the
           pasteboard would also have data for "public.utf8-plain-text" and we wouldn't be here.)  If
           "org.winehq.builtin.text" is not on the pasteboard, then one of the other text formats is
           presumably responsible for the promise that we're trying to satisfy, so we keep looking. */
        LIST_FOR_EACH_ENTRY(base_format, &format_list, WINE_CLIPFORMAT, entry)
        {
            if (base_format->format_id == format->format_id && !base_format->synthesized &&
                CFArrayContainsValue(types, range, base_format->type))
            {
                TRACE("Sending WM_RENDERFORMAT message for format %s to hwnd %p\n", debugstr_format(base_format->format_id), hwnd);
                SendMessageW(hwnd, WM_RENDERFORMAT, base_format->format_id, 0);
                ret = TRUE;
                goto done;
            }
        }
    }

done:
    if (types) CFRelease(types);

    return ret;
}
