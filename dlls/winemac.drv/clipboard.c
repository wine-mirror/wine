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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "macdrv.h"
#include "winuser.h"
#include "shellapi.h"
#include "shlobj.h"
#include "wine/list.h"
#include "wine/server.h"


WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


/**************************************************************************
 *              Types
 **************************************************************************/

typedef void *(*DRVIMPORTFUNC)(CFDataRef data, size_t *ret_size);
typedef CFDataRef (*DRVEXPORTFUNC)(void *data, size_t size);

typedef struct _WINE_CLIPFORMAT
{
    struct list             entry;
    UINT                    format_id;
    CFStringRef             type;
    DRVIMPORTFUNC           import_func;
    DRVEXPORTFUNC           export_func;
    BOOL                    synthesized;
    struct _WINE_CLIPFORMAT *natural_format;
} WINE_CLIPFORMAT;


/**************************************************************************
 *              Constants
 **************************************************************************/

#define CLIPBOARD_UPDATE_DELAY 2000   /* delay between checks of the Mac pasteboard */


/**************************************************************************
 *              Forward Function Declarations
 **************************************************************************/

static void *import_clipboard_data(CFDataRef data, size_t *ret_size);
static void *import_bmp_to_dib(CFDataRef data, size_t *ret_size);
static void *import_html(CFDataRef data, size_t *ret_size);
static void *import_nsfilenames_to_hdrop(CFDataRef data, size_t *ret_size);
static void *import_utf8_to_unicodetext(CFDataRef data, size_t *ret_size);
static void *import_utf16_to_unicodetext(CFDataRef data, size_t *ret_size);

static CFDataRef export_clipboard_data(void *data, size_t size);
static CFDataRef export_dib_to_bmp(void *data, size_t size);
static CFDataRef export_hdrop_to_filenames(void *data, size_t size);
static CFDataRef export_html(void *data, size_t size);
static CFDataRef export_unicodetext_to_utf8(void *data, size_t size);
static CFDataRef export_unicodetext_to_utf16(void *data, size_t size);


/**************************************************************************
 *              Static Variables
 **************************************************************************/

static const WCHAR clipboard_classname[] =
    {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r',0};

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
    arbitrary type strings.  We ignore these.

    Summary:
    Win32 clipboard format names:
        <none>                              standard clipboard format; maps via
                                            format_list to either a predefined Mac UTI
                                            or org.winehq.builtin.<format>.
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
                                            to a standard clipboard format or ignored
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
    { CF_DIBV5,             CFSTR("org.winehq.builtin.dibv5"),              import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_DIF,               CFSTR("org.winehq.builtin.dif"),                import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_ENHMETAFILE,       CFSTR("org.winehq.builtin.enhmetafile"),        import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_LOCALE,            CFSTR("org.winehq.builtin.locale"),             import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_OEMTEXT,           CFSTR("org.winehq.builtin.oemtext"),            import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_PALETTE,           CFSTR("org.winehq.builtin.palette"),            import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_PENDATA,           CFSTR("org.winehq.builtin.pendata"),            import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_RIFF,              CFSTR("org.winehq.builtin.riff"),               import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_SYLK,              CFSTR("org.winehq.builtin.sylk"),               import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_TEXT,              CFSTR("org.winehq.builtin.text"),               import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_TIFF,              CFSTR("public.tiff"),                           import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_WAVE,              CFSTR("com.microsoft.waveform-audio"),          import_clipboard_data,          export_clipboard_data,      FALSE },

    { CF_DIB,               CFSTR("org.winehq.builtin.dib"),                import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_DIB,               CFSTR("com.microsoft.bmp"),                     import_bmp_to_dib,              export_dib_to_bmp,          TRUE },

    { CF_HDROP,             CFSTR("org.winehq.builtin.hdrop"),              import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_HDROP,             CFSTR("NSFilenamesPboardType"),                 import_nsfilenames_to_hdrop,    export_hdrop_to_filenames,  TRUE },

    { CF_UNICODETEXT,       CFSTR("org.winehq.builtin.unicodetext"),        import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_UNICODETEXT,       CFSTR("public.utf16-plain-text"),               import_utf16_to_unicodetext,    export_unicodetext_to_utf16,TRUE },
    { CF_UNICODETEXT,       CFSTR("public.utf8-plain-text"),                import_utf8_to_unicodetext,     export_unicodetext_to_utf8, TRUE },
};

static const WCHAR wszRichTextFormat[] = {'R','i','c','h',' ','T','e','x','t',' ','F','o','r','m','a','t',0};
static const WCHAR wszGIF[] = {'G','I','F',0};
static const WCHAR wszJFIF[] = {'J','F','I','F',0};
static const WCHAR wszPNG[] = {'P','N','G',0};
static const WCHAR wszHTMLFormat[] = {'H','T','M','L',' ','F','o','r','m','a','t',0};
static const struct
{
    LPCWSTR       name;
    CFStringRef   type;
    DRVIMPORTFUNC import;
    DRVEXPORTFUNC export;
    BOOL          synthesized;
} builtin_format_names[] =
{
    { wszRichTextFormat,    CFSTR("public.rtf"),                            import_clipboard_data,          export_clipboard_data },
    { wszGIF,               CFSTR("com.compuserve.gif"),                    import_clipboard_data,          export_clipboard_data },
    { wszJFIF,              CFSTR("public.jpeg"),                           import_clipboard_data,          export_clipboard_data },
    { wszPNG,               CFSTR("public.png"),                            import_clipboard_data,          export_clipboard_data },
    { wszHTMLFormat,        NULL,                                           import_clipboard_data,          export_clipboard_data },
    { wszHTMLFormat,        CFSTR("public.html"),                           import_html,                    export_html,            TRUE },
    { CFSTR_INETURLW,       CFSTR("public.url"),                            import_utf8_to_unicodetext,     export_unicodetext_to_utf8 },
};

/* The prefix prepended to a Win32 clipboard format name to make a Mac pasteboard type. */
static const CFStringRef registered_name_type_prefix = CFSTR("org.winehq.registered.");

static unsigned int clipboard_thread_id;
static HWND clipboard_hwnd;
static BOOL is_clipboard_owner;
static macdrv_window clipboard_cocoa_window;
static unsigned int last_clipboard_update;
static unsigned int last_get_seqno;
static WINE_CLIPFORMAT **current_mac_formats;
static unsigned int nb_current_mac_formats;


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

    if (NtUserGetClipboardFormatName(id, buffer, 256))
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

    format = malloc(sizeof(*format));

    if (format == NULL)
    {
        WARN("No more memory for a new format!\n");
        return NULL;
    }
    format->format_id = id;
    format->import_func = import_clipboard_data;
    format->export_func = export_clipboard_data;
    format->synthesized = FALSE;
    format->natural_format = NULL;

    if (type)
        format->type = CFStringCreateCopy(NULL, type);
    else
    {
        WCHAR buffer[256];

        if (!NtUserGetClipboardFormatName(format->format_id, buffer, ARRAY_SIZE(buffer)))
        {
            WARN("failed to get name for format %s; error 0x%08x\n", debugstr_format(format->format_id),
                 RtlGetLastWin32Error());
            free(format);
            return NULL;
        }

        format->type = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%S"),
                                                registered_name_type_prefix, (const WCHAR*)buffer);
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
 *              natural_format_for_format
 *
 * Find the "natural" format for this format_id (the one which isn't
 * synthesized from another type).
 */
static WINE_CLIPFORMAT* natural_format_for_format(UINT format_id)
{
    WINE_CLIPFORMAT *format;

    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        if (format->format_id == format_id && !format->synthesized) break;

    if (&format->entry == &format_list)
        format = NULL;

    TRACE("%s -> %p/%s\n", debugstr_format(format_id), format, debugstr_cf(format ? format->type : NULL));
    return format;
}


static ATOM register_clipboard_format(const WCHAR *name)
{
    ATOM atom;
    if (NtAddAtom(name, lstrlenW(name) * sizeof(WCHAR), &atom)) return 0;
    return atom;
}


/**************************************************************************
 *              register_builtin_formats
 */
static void register_builtin_formats(void)
{
    UINT i;
    WINE_CLIPFORMAT *format;

    /* Register built-in formats */
    for (i = 0; i < ARRAY_SIZE(builtin_format_ids); i++)
    {
        if (!(format = malloc(sizeof(*format)))) break;
        format->format_id       = builtin_format_ids[i].id;
        format->type            = CFRetain(builtin_format_ids[i].type);
        format->import_func     = builtin_format_ids[i].import;
        format->export_func     = builtin_format_ids[i].export;
        format->synthesized     = builtin_format_ids[i].synthesized;
        format->natural_format  = NULL;
        list_add_tail(&format_list, &format->entry);
    }

    /* Register known mappings between Windows formats and Mac types */
    for (i = 0; i < ARRAY_SIZE(builtin_format_names); i++)
    {
        if (!(format = malloc(sizeof(*format)))) break;
        format->format_id       = register_clipboard_format(builtin_format_names[i].name);
        format->import_func     = builtin_format_names[i].import;
        format->export_func     = builtin_format_names[i].export;
        format->synthesized     = builtin_format_names[i].synthesized;
        format->natural_format  = NULL;

        if (builtin_format_names[i].type)
            format->type = CFRetain(builtin_format_names[i].type);
        else
        {
            format->type = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%S"),
                                                    registered_name_type_prefix, builtin_format_names[i].name);
        }

        list_add_tail(&format_list, &format->entry);
    }

    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
    {
        if (format->synthesized)
            format->natural_format = natural_format_for_format(format->format_id);
    }
}


/**************************************************************************
 *              format_for_type
 */
static WINE_CLIPFORMAT* format_for_type(CFStringRef type)
{
    WINE_CLIPFORMAT *format;

    TRACE("type %s\n", debugstr_cf(type));

    if (list_empty(&format_list)) register_builtin_formats();

    LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
    {
        if (CFEqual(format->type, type))
            goto done;
    }

    format = NULL;
    if (CFStringHasPrefix(type, CFSTR("org.winehq.builtin.")))
    {
        ERR("Shouldn't happen. Built-in type %s should have matched something in format list.\n",
            debugstr_cf(type));
    }
    else if (CFStringHasPrefix(type, registered_name_type_prefix))
    {
        LPWSTR name;
        int len = CFStringGetLength(type) - CFStringGetLength(registered_name_type_prefix);

        name = malloc((len + 1) * sizeof(WCHAR));
        CFStringGetCharacters(type, CFRangeMake(CFStringGetLength(registered_name_type_prefix), len),
                              (UniChar*)name);
        name[len] = 0;

        format = register_format(register_clipboard_format(name), type);
        if (!format)
            ERR("Failed to register format for type %s name %s\n", debugstr_cf(type), debugstr_w(name));

        free(name);
    }

done:
    TRACE(" -> %p/%s\n", format, debugstr_format(format ? format->format_id : 0));
    return format;
}


/***********************************************************************
 *              bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size(const BITMAPINFO *info, WORD coloruse)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER*)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = min(info->bmiHeader.biClrUsed, 256);
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max(info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD));
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/**************************************************************************
 *		get_html_description_field
 *
 *  Find the value of a field in an HTML Format description.
 */
static const char* get_html_description_field(const char* data, const char* keyword)
{
    const char* pos = data;

    while (pos && *pos && *pos != '<')
    {
        if (memcmp(pos, keyword, strlen(keyword)) == 0)
            return pos + strlen(keyword);

        pos = strchr(pos, '\n');
        if (pos) pos++;
    }

    return NULL;
}


/**************************************************************************
 *              import_clipboard_data
 *
 *  Generic import clipboard data routine.
 */
static void *import_clipboard_data(CFDataRef data, size_t *ret_size)
{
    void *ret = NULL;

    size_t len = CFDataGetLength(data);
    if (len && (ret = malloc(len)))
    {
        memcpy(ret, CFDataGetBytePtr(data), len);
        *ret_size = len;
    }

    return ret;
}


/**************************************************************************
 *              import_bmp_to_dib
 *
 *  Import BMP data, converting to CF_DIB or CF_DIBV5 format.  This just
 *  entails stripping the BMP file format header.
 */
static void *import_bmp_to_dib(CFDataRef data, size_t *ret_size)
{
    BITMAPFILEHEADER *bfh = (BITMAPFILEHEADER*)CFDataGetBytePtr(data);
    CFIndex len = CFDataGetLength(data);
    void *ret = NULL;

    if (len >= sizeof(*bfh) + sizeof(BITMAPCOREHEADER) &&
        bfh->bfType == 0x4d42 /* "BM" */)
    {
        BITMAPINFO *bmi = (BITMAPINFO*)(bfh + 1);

        len -= sizeof(*bfh);
        if ((ret = malloc(len)))
        {
            memcpy(ret, bmi, len);
            *ret_size = len;
        }
    }

    return ret;
}


/**************************************************************************
 *              import_html
 *
 *  Import HTML data.
 */
static void *import_html(CFDataRef data, size_t *ret_size)
{
    static const char header[] =
        "Version:0.9\n"
        "StartHTML:0000000100\n"
        "EndHTML:%010lu\n"
        "StartFragment:%010lu\n"
        "EndFragment:%010lu\n"
        "<!--StartFragment-->";
    static const char trailer[] = "\n<!--EndFragment-->";
    void *ret;
    SIZE_T len, total;
    size_t size = CFDataGetLength(data);

    len = strlen(header) + 12;  /* 3 * 4 extra chars for %010lu */
    total = len + size + sizeof(trailer);
    if ((ret = malloc(total)))
    {
        char *p = ret;
        p += snprintf(p, total, header, total - 1, len, len + size + 1 /* include the final \n in the data */);
        CFDataGetBytes(data, CFRangeMake(0, size), (UInt8*)p);
        strcpy(p + size, trailer);
        *ret_size = total;
        TRACE("returning %s\n", debugstr_a(ret));
    }
    return ret;
}


static CPTABLEINFO *get_ansi_cp(void)
{
    USHORT utf8_hdr[2] = { 0, CP_UTF8 };
    static CPTABLEINFO cp;
    if (!cp.CodePage)
    {
        if (NtCurrentTeb()->Peb->AnsiCodePageData)
            RtlInitCodePageTable(NtCurrentTeb()->Peb->AnsiCodePageData, &cp);
        else
            RtlInitCodePageTable(utf8_hdr, &cp);
    }
    return &cp;
}


/***********************************************************************
 *           get_nt_pathname
 *
 * Simplified version of RtlDosPathNameToNtPathName_U.
 */
static BOOL get_nt_pathname(const WCHAR *name, UNICODE_STRING *nt_name)
{
    static const WCHAR ntprefixW[] = {'\\','?','?','\\'};
    static const WCHAR uncprefixW[] = {'U','N','C','\\'};
    size_t len = lstrlenW(name);
    WCHAR *ptr;

    nt_name->MaximumLength = (len + 8) * sizeof(WCHAR);
    if (!(ptr = malloc(nt_name->MaximumLength))) return FALSE;
    nt_name->Buffer = ptr;

    memcpy(ptr, ntprefixW, sizeof(ntprefixW));
    ptr += ARRAYSIZE(ntprefixW);
    if (name[0] == '\\' && name[1] == '\\')
    {
        if ((name[2] == '.' || name[2] == '?') && name[3] == '\\')
        {
            name += 4;
            len -= 4;
        }
        else
        {
            memcpy(ptr, uncprefixW, sizeof(uncprefixW));
            ptr += ARRAYSIZE(uncprefixW);
            name += 2;
            len -= 2;
        }
    }
    memcpy(ptr, name, (len + 1) * sizeof(WCHAR));
    ptr += len;
    nt_name->Length = (ptr - nt_name->Buffer) * sizeof(WCHAR);
    return TRUE;
}


/* based on wine_get_unix_file_name */
static char *get_unix_file_name(const WCHAR *dosW)
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;

    if (!get_nt_pathname(dosW, &nt_name)) return NULL;
    InitializeObjectAttributes(&attr, &nt_name, 0, 0, NULL);
    for (;;)
    {
        if (!(buffer = malloc(size)))
        {
            free(nt_name.Buffer);
            return NULL;
        }
        status = wine_nt_to_unix_file_name(&attr, buffer, &size, FILE_OPEN_IF);
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        free(buffer);
    }
    free(nt_name.Buffer);
    if (status)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}


/**************************************************************************
 *              import_nsfilenames_to_hdrop
 *
 *  Import NSFilenamesPboardType data, converting the property-list-
 *  serialized array of path strings to CF_HDROP.
 */
static void *import_nsfilenames_to_hdrop(CFDataRef data, size_t *ret_size)
{
    CFArrayRef names;
    CFIndex count, i;
    size_t len;
    char *buffer = NULL;
    WCHAR **paths = NULL;
    DROPFILES *dropfiles = NULL;
    UniChar* p;

    TRACE("data %s\n", debugstr_cf(data));

    names = (CFArrayRef)CFPropertyListCreateWithData(NULL, data, kCFPropertyListImmutable,
                                                     NULL, NULL);
    if (!names || CFGetTypeID(names) != CFArrayGetTypeID())
    {
        WARN("failed to interpret data as a CFArray\n");
        goto done;
    }

    count = CFArrayGetCount(names);

    len = 0;
    for (i = 0; i < count; i++)
    {
        CFIndex this_len;
        CFStringRef name = (CFStringRef)CFArrayGetValueAtIndex(names, i);
        TRACE("    %s\n", debugstr_cf(name));
        if (CFGetTypeID(name) != CFStringGetTypeID())
        {
            WARN("non-string in array\n");
            goto done;
        }

        this_len = CFStringGetMaximumSizeOfFileSystemRepresentation(name);
        if (this_len > len)
            len = this_len;
    }

    buffer = malloc(len);
    if (!buffer)
    {
        WARN("failed to allocate buffer for file-system representations\n");
        goto done;
    }

    paths = calloc(count, sizeof(paths[0]));
    if (!paths)
    {
        WARN("failed to allocate array of DOS paths\n");
        goto done;
    }

    for (i = 0; i < count; i++)
    {
        CFStringRef name = (CFStringRef)CFArrayGetValueAtIndex(names, i);
        if (!CFStringGetFileSystemRepresentation(name, buffer, len))
        {
            WARN("failed to get file-system representation for %s\n", debugstr_cf(name));
            goto done;
        }
        if (ntdll_get_dos_file_name( buffer, &paths[i], FILE_OPEN ))
        {
            WARN("failed to get DOS path for %s\n", debugstr_a(buffer));
            goto done;
        }
    }

    len = 1; /* for the terminating null */
    for (i = 0; i < count; i++)
        len += wcslen(paths[i]) + 1;

    *ret_size = sizeof(*dropfiles) + len * sizeof(WCHAR);
    if (!(dropfiles = malloc(*ret_size)))
    {
        WARN("failed to allocate HDROP\n");
        goto done;
    }

    dropfiles->pFiles   = sizeof(*dropfiles);
    dropfiles->pt.x     = 0;
    dropfiles->pt.y     = 0;
    dropfiles->fNC      = FALSE;
    dropfiles->fWide    = TRUE;

    p = (WCHAR*)(dropfiles + 1);
    for (i = 0; i < count; i++)
    {
        wcscpy(p, paths[i]);
        p += wcslen(p) + 1;
    }
    *p = 0;

done:
    if (paths)
    {
        for (i = 0; i < count; i++) free(paths[i]);
        free(paths);
    }
    free(buffer);
    if (names) CFRelease(names);
    return dropfiles;
}


/**************************************************************************
 *              import_utf8_to_unicodetext
 *
 *  Import a UTF-8 string, converting the string to CF_UNICODETEXT.
 */
static void *import_utf8_to_unicodetext(CFDataRef data, size_t *ret_size)
{
    const BYTE *src;
    unsigned long src_len;
    unsigned long new_lines = 0;
    LPSTR dst;
    unsigned long i, j;
    WCHAR *ret = NULL;

    src = CFDataGetBytePtr(data);
    src_len = CFDataGetLength(data);
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == '\n')
            new_lines++;
    }

    if ((dst = malloc(src_len + new_lines + 1)))
    {
        for (i = 0, j = 0; i < src_len; i++)
        {
            if (src[i] == '\n')
                dst[j++] = '\r';

            dst[j++] = src[i];
        }
        dst[j++] = 0;

        if ((ret = malloc(j * sizeof(WCHAR))))
        {
            DWORD dst_size;
            RtlUTF8ToUnicodeN(ret, j * sizeof(WCHAR), &dst_size, dst, j);
            *ret_size = dst_size;
        }

        free(dst);
    }

    return ret;
}


/**************************************************************************
 *              import_utf16_to_unicodetext
 *
 *  Import a UTF-8 string, converting the string to CF_UNICODETEXT.
 */
static void *import_utf16_to_unicodetext(CFDataRef data, size_t *ret_size)
{
    const WCHAR *src;
    unsigned long src_len;
    unsigned long new_lines = 0;
    LPWSTR dst;
    unsigned long i, j;

    src = (const WCHAR *)CFDataGetBytePtr(data);
    src_len = CFDataGetLength(data) / sizeof(WCHAR);
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == '\n')
            new_lines++;
        else if (src[i] == '\r' && (i + 1 >= src_len || src[i + 1] != '\n'))
            new_lines++;
    }

    *ret_size = (src_len + new_lines + 1) * sizeof(WCHAR);
    if ((dst = malloc(*ret_size)))
    {
        for (i = 0, j = 0; i < src_len; i++)
        {
            if (src[i] == '\n')
                dst[j++] = '\r';

            dst[j++] = src[i];

            if (src[i] == '\r' && (i + 1 >= src_len || src[i + 1] != '\n'))
                dst[j++] = '\n';
        }
        dst[j] = 0;
    }

    return dst;
}


/**************************************************************************
 *              export_clipboard_data
 *
 *  Generic export clipboard data routine.
 */
static CFDataRef export_clipboard_data(void *data, size_t size)
{
    return CFDataCreate(NULL, data, size);
}


/**************************************************************************
 *              export_dib_to_bmp
 *
 *  Export CF_DIB or CF_DIBV5 to BMP file format.  This just entails
 *  prepending a BMP file format header to the data.
 */
static CFDataRef export_dib_to_bmp(void *data, size_t size)
{
    CFMutableDataRef ret = NULL;
    CFIndex len;
    BITMAPFILEHEADER bfh;

    len = sizeof(bfh) + size;
    ret = CFDataCreateMutable(NULL, len);
    if (ret)
    {
        bfh.bfType = 0x4d42; /* "BM" */
        bfh.bfSize = len;
        bfh.bfReserved1 = 0;
        bfh.bfReserved2 = 0;
        bfh.bfOffBits = sizeof(bfh) + bitmap_info_size(data, DIB_RGB_COLORS);
        CFDataAppendBytes(ret, (UInt8*)&bfh, sizeof(bfh));

        /* rest of bitmap is the same as the packed dib */
        CFDataAppendBytes(ret, data, size);
    }

    return ret;
}


/**************************************************************************
 *              export_hdrop_to_filenames
 *
 *  Export CF_HDROP to NSFilenamesPboardType data, which is a CFArray of
 *  CFStrings (holding Unix paths) which is serialized as a property list.
 */
static CFDataRef export_hdrop_to_filenames(void *data, size_t size)
{
    CFDataRef ret = NULL;
    DROPFILES *dropfiles = data;
    CFMutableArrayRef filenames = NULL;
    void *p;
    WCHAR *buffer = NULL;
    size_t buffer_len = 0;

    TRACE("data %p\n", data);

    filenames = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    if (!filenames)
    {
        WARN("failed to create filenames array\n");
        goto done;
    }

    p = (char*)dropfiles + dropfiles->pFiles;
    while (dropfiles->fWide ? *(WCHAR*)p : *(char*)p)
    {
        char *unixname;
        CFStringRef filename;

        TRACE("    %s\n", dropfiles->fWide ? debugstr_w(p) : debugstr_a(p));

        if (dropfiles->fWide)
            unixname = get_unix_file_name(p);
        else
        {
            CPTABLEINFO *cp = get_ansi_cp();
            DWORD len = strlen(p) + 1;

            if (len * 3 > buffer_len)
            {
                free(buffer);
                buffer_len = len * 3;
                buffer = malloc(buffer_len * sizeof(*buffer));
            }

            if (cp->CodePage == CP_UTF8)
                RtlUTF8ToUnicodeN(buffer, buffer_len * sizeof(WCHAR), &len, p, len);
            else
                RtlCustomCPToUnicodeN(cp, buffer, buffer_len * sizeof(WCHAR), &len, p, len);

            unixname = get_unix_file_name(buffer);
        }
        if (!unixname)
        {
            WARN("failed to convert DOS path to Unix: %s\n",
                 dropfiles->fWide ? debugstr_w(p) : debugstr_a(p));
            goto done;
        }

        if (dropfiles->fWide)
            p = (WCHAR*)p + wcslen(p) + 1;
        else
            p = (char*)p + strlen(p) + 1;

        filename = CFStringCreateWithFileSystemRepresentation(NULL, unixname);
        if (!filename)
        {
            WARN("failed to create CFString from Unix path %s\n", debugstr_a(unixname));
            free(unixname);
            goto done;
        }

        free(unixname);
        CFArrayAppendValue(filenames, filename);
        CFRelease(filename);
    }

    ret = CFPropertyListCreateData(NULL, filenames, kCFPropertyListXMLFormat_v1_0, 0, NULL);

done:
    free(buffer);
    if (filenames) CFRelease(filenames);
    TRACE(" -> %s\n", debugstr_cf(ret));
    return ret;
}


/**************************************************************************
 *              export_html
 *
 *  Export HTML Format to public.html data.
 *
 * FIXME: We should attempt to add an <a base> tag and convert windows paths.
 */
static CFDataRef export_html(void *data, size_t size)
{
    const char *field_value;
    int fragmentstart, fragmentend;

    /* read the important fields */
    field_value = get_html_description_field(data, "StartFragment:");
    if (!field_value)
    {
        ERR("Couldn't find StartFragment value\n");
        return NULL;
    }
    fragmentstart = atoi(field_value);

    field_value = get_html_description_field(data, "EndFragment:");
    if (!field_value)
    {
        ERR("Couldn't find EndFragment value\n");
        return NULL;
    }
    fragmentend = atoi(field_value);

    /* export only the fragment */
    return CFDataCreate(NULL, &((const UInt8*)data)[fragmentstart], fragmentend - fragmentstart);
}


/**************************************************************************
 *              export_unicodetext_to_utf8
 *
 *  Export CF_UNICODETEXT to UTF-8.
 */
static CFDataRef export_unicodetext_to_utf8(void *data, size_t size)
{
    CFMutableDataRef ret;
    WCHAR *src = data;
    DWORD dst_len = 0;

    /* Leave off null terminator. */
    if (size >= sizeof(WCHAR) && !src[size / sizeof(WCHAR) - 1]) size -= sizeof(WCHAR);
    RtlUnicodeToUTF8N(NULL, 0, &dst_len, src, size);
    ret = CFDataCreateMutable(NULL, dst_len);
    if (ret)
    {
        LPSTR dst;
        int i, j;

        CFDataSetLength(ret, dst_len);
        dst = (LPSTR)CFDataGetMutableBytePtr(ret);
        RtlUnicodeToUTF8N(dst, dst_len, &dst_len, src, size);

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

    return ret;
}


/**************************************************************************
 *              export_unicodetext_to_utf16
 *
 *  Export CF_UNICODETEXT to UTF-16.
 */
static CFDataRef export_unicodetext_to_utf16(void *data, size_t size)
{
    CFMutableDataRef ret;
    const WCHAR *src = data;
    INT src_len;

    src_len = size / sizeof(WCHAR);
    if (src_len) src_len--; /* Leave off null terminator. */
    ret = CFDataCreateMutable(NULL, src_len * sizeof(WCHAR));
    if (ret)
    {
        LPWSTR dst;
        int i, j;

        CFDataSetLength(ret, src_len * sizeof(WCHAR));
        dst = (LPWSTR)CFDataGetMutableBytePtr(ret);

        /* Remove carriage returns */
        for (i = 0, j = 0; i < src_len; i++)
        {
            if (src[i] == '\r' &&
                (i + 1 >= src_len || src[i + 1] == '\n' || src[i + 1] == '\0'))
                continue;
            dst[j++] = src[i];
        }
        CFDataSetLength(ret, j * sizeof(WCHAR));
    }

    return ret;
}

struct format_entry *get_format_entries(CFTypeRef pasteboard, UINT *entries_size)
{
    struct format_entry *entries = NULL;
    CFStringRef type;
    CFArrayRef types;
    size_t size = 0;
    CFIndex i;
    char *tmp;

    TRACE("pasteboard %p\n", pasteboard);

    types = macdrv_copy_pasteboard_types(pasteboard);
    if (!types) return NULL;

    for (i = 0; i < CFArrayGetCount(types); i++)
    {
        WINE_CLIPFORMAT *format;
        size_t import_size;
        CFDataRef data;
        void *import;

        type = CFArrayGetValueAtIndex(types, i);
        if (!(format = format_for_type(type))) continue;

        data = macdrv_copy_pasteboard_data(pasteboard, type);
        import = format->import_func(data, &import_size);

        if ((tmp = realloc(entries, size + sizeof(*entries) + import_size)))
        {
            struct format_entry *entry = (struct format_entry *)(tmp + size);
            entry->format = format->format_id;
            entry->size = import_size;
            memcpy(entry->data, import, import_size);

            entries = (struct format_entry *)tmp;
            size += sizeof(*entry) + import_size;
        }

        free(import);
    }

    CFRelease(types);

    *entries_size = size;
    return entries;
}

/**************************************************************************
 *              get_formats_for_pasteboard_types
 */
static WINE_CLIPFORMAT** get_formats_for_pasteboard_types(CFArrayRef types, UINT *num_formats)
{
    CFIndex count, i;
    CFMutableSetRef seen_formats;
    WINE_CLIPFORMAT** formats;
    UINT pos;

    count = CFArrayGetCount(types);
    TRACE("got %ld types\n", count);

    if (!count)
        return NULL;

    seen_formats = CFSetCreateMutable(NULL, count, NULL);
    if (!seen_formats)
    {
        WARN("Failed to allocate seen formats set\n");
        return NULL;
    }

    formats = malloc(count * sizeof(*formats));
    if (!formats)
    {
        WARN("Failed to allocate formats array\n");
        CFRelease(seen_formats);
        return NULL;
    }

    pos = 0;
    for (i = 0; i < count; i++)
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, i);
        WINE_CLIPFORMAT* format = format_for_type(type);

        if (!format)
        {
            TRACE("ignoring type %s\n", debugstr_cf(type));
            continue;
        }

        if (!format->synthesized)
        {
            TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));
            CFSetAddValue(seen_formats, ULongToPtr(format->format_id));
            formats[pos++] = format;
        }
        else if (format->natural_format &&
                 CFArrayContainsValue(types, CFRangeMake(0, count), format->natural_format->type))
        {
            TRACE("for type %s deferring synthesized formats because type %s is also present\n",
                  debugstr_cf(type), debugstr_cf(format->natural_format->type));
        }
        else if (CFSetContainsValue(seen_formats, ULongToPtr(format->format_id)))
        {
            TRACE("for type %s got duplicate synthesized format %p/%s; skipping\n", debugstr_cf(type), format,
                  debugstr_format(format->format_id));
        }
        else
        {
            TRACE("for type %s got synthesized format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));
            CFSetAddValue(seen_formats, ULongToPtr(format->format_id));
            formats[pos++] = format;
        }
    }

    /* Now go back through the types adding the synthesized formats that we deferred before. */
    for (i = 0; i < count; i++)
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, i);
        WINE_CLIPFORMAT* format = format_for_type(type);

        if (!format) continue;
        if (!format->synthesized) continue;

        /* Don't duplicate a real value with a synthesized value. */
        if (CFSetContainsValue(seen_formats, ULongToPtr(format->format_id))) continue;

        TRACE("for type %s got synthesized format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));
        CFSetAddValue(seen_formats, ULongToPtr(format->format_id));
        formats[pos++] = format;
    }

    CFRelease(seen_formats);

    if (!pos)
    {
        free(formats);
        formats = NULL;
    }

    *num_formats = pos;
    return formats;
}


/**************************************************************************
 *              register_win32_formats
 *
 * Register Win32 clipboard formats the first time we encounter them.
 */
static void register_win32_formats(const UINT *ids, UINT size)
{
    unsigned int i;

    if (list_empty(&format_list)) register_builtin_formats();

    for (i = 0; i < size; i++)
        register_format(ids[i], NULL);
}


/***********************************************************************
 *              get_clipboard_formats
 *
 * Return a list of all formats currently available on the Win32 clipboard.
 * Helper for set_mac_pasteboard_types_from_win32_clipboard.
 */
static UINT *get_clipboard_formats(UINT *size)
{
    UINT *ids;

    *size = 256;
    for (;;)
    {
        if (!(ids = malloc(*size * sizeof(*ids)))) return NULL;
        if (NtUserGetUpdatedClipboardFormats(ids, *size, size)) break;
        free(ids);
        if (RtlGetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    }
    register_win32_formats(ids, *size);
    return ids;
}


/**************************************************************************
 *              set_mac_pasteboard_types_from_win32_clipboard
 */
static void set_mac_pasteboard_types_from_win32_clipboard(void)
{
    WINE_CLIPFORMAT *format;
    UINT count, i, *formats;

    if (!(formats = get_clipboard_formats(&count))) return;

    macdrv_clear_pasteboard(clipboard_cocoa_window);

    for (i = 0; i < count; i++)
    {
        LIST_FOR_EACH_ENTRY(format, &format_list, WINE_CLIPFORMAT, entry)
        {
            if (format->format_id != formats[i]) continue;
            TRACE("%s -> %s\n", debugstr_format(format->format_id), debugstr_cf(format->type));
            macdrv_set_pasteboard_data(format->type, NULL, clipboard_cocoa_window);
        }
    }

    free(formats);
    return;
}


/**************************************************************************
 *              set_win32_clipboard_formats_from_mac_pasteboard
 */
static void set_win32_clipboard_formats_from_mac_pasteboard(CFArrayRef types)
{
    WINE_CLIPFORMAT** formats;
    UINT count, i;

    formats = get_formats_for_pasteboard_types(types, &count);
    if (!formats)
        return;

    for (i = 0; i < count; i++)
    {
        struct set_clipboard_params params = { 0 };
        TRACE("adding format %s\n", debugstr_format(formats[i]->format_id));
        NtUserSetClipboardData(formats[i]->format_id, 0, &params);
    }

    free(current_mac_formats);
    current_mac_formats = formats;
    nb_current_mac_formats = count;
}


/**************************************************************************
 *              render_format
 */
static void render_format(UINT id)
{
    unsigned int i;

    for (i = 0; i < nb_current_mac_formats; i++)
    {
        CFDataRef pasteboard_data;

        if (current_mac_formats[i]->format_id != id) continue;

        pasteboard_data = macdrv_copy_pasteboard_data(NULL, current_mac_formats[i]->type);
        if (pasteboard_data)
        {
            struct set_clipboard_params params = { 0 };
            params.data = current_mac_formats[i]->import_func(pasteboard_data, &params.size);
            CFRelease(pasteboard_data);
            if (!params.data) continue;
            NtUserSetClipboardData(id, 0, &params);
            free(params.data);
            break;
        }
    }
}


/**************************************************************************
 *              grab_win32_clipboard
 *
 * Grab the Win32 clipboard when a Mac app has taken ownership of the
 * pasteboard, and fill it with the pasteboard data types.
 */
static void grab_win32_clipboard(void)
{
    static CFArrayRef last_types;
    CFArrayRef types;

    types = macdrv_copy_pasteboard_types(NULL);
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return;
    }

    if (!macdrv_has_pasteboard_changed() && last_types && CFEqual(types, last_types))
    {
        CFRelease(types);
        return;
    }

    if (last_types) CFRelease(last_types);
    last_types = types; /* takes ownership */

    if (!NtUserOpenClipboard(clipboard_hwnd, 0)) return;
    NtUserEmptyClipboard();
    is_clipboard_owner = TRUE;
    last_clipboard_update = NtGetTickCount();
    set_win32_clipboard_formats_from_mac_pasteboard(types);
    NtUserCloseClipboard();
    NtUserSetTimer(clipboard_hwnd, 1, CLIPBOARD_UPDATE_DELAY, NULL, TIMERV_DEFAULT_COALESCING);
}


/**************************************************************************
 *              update_clipboard
 *
 * Periodically update the clipboard while the clipboard is owned by a
 * Mac app.
 */
static void update_clipboard(void)
{
    static BOOL updating;

    TRACE("is_clipboard_owner %d last_clipboard_update %u now %u\n",
          is_clipboard_owner, last_clipboard_update, NtGetTickCount());

    if (updating) return;
    updating = TRUE;

    if (is_clipboard_owner)
    {
        if (NtGetTickCount() - last_clipboard_update > CLIPBOARD_UPDATE_DELAY)
            grab_win32_clipboard();
    }
    else if (!macdrv_is_pasteboard_owner(clipboard_cocoa_window))
        grab_win32_clipboard();

    updating = FALSE;
}


static BOOL init_clipboard(HWND hwnd)
{
    struct macdrv_window_features wf;

    memset(&wf, 0, sizeof(wf));
    clipboard_cocoa_window = macdrv_create_cocoa_window(&wf, CGRectMake(100, 100, 100, 100), hwnd,
                                                        macdrv_init_thread_data()->queue);
    if (!clipboard_cocoa_window)
    {
        ERR("failed to create clipboard Cocoa window\n");
        return FALSE;
    }

    clipboard_hwnd = hwnd;
    clipboard_thread_id = GetCurrentThreadId();
    NtUserAddClipboardFormatListener(clipboard_hwnd);
    register_builtin_formats();
    grab_win32_clipboard();

    TRACE("clipboard thread %04x running\n", clipboard_thread_id);
    return TRUE;
}


/**************************************************************************
 *              macdrv_ClipboardWindowProc
 *
 * Window procedure for the clipboard manager.
 */
LRESULT macdrv_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NCCREATE:
            return init_clipboard(hwnd);
        case WM_CLIPBOARDUPDATE:
            if (is_clipboard_owner) break;  /* ignore our own changes */
            if ((LONG)(NtUserGetClipboardSequenceNumber() - last_get_seqno) <= 0) break;
            set_mac_pasteboard_types_from_win32_clipboard();
            break;
        case WM_RENDERFORMAT:
            render_format(wp);
            break;
        case WM_TIMER:
            if (!is_clipboard_owner) break;
            grab_win32_clipboard();
            break;
        case WM_DESTROYCLIPBOARD:
            TRACE("WM_DESTROYCLIPBOARD: lost ownership\n");
            is_clipboard_owner = FALSE;
            NtUserKillTimer(hwnd, 1);
            break;
        case WM_USER:
            update_clipboard();
            break;
    }
    return NtUserMessageCall(hwnd, msg, wp, lp, NULL, NtUserDefWindowProc, FALSE);
}


/**************************************************************************
 *              Mac User Driver Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              macdrv_UpdateClipboard
 */
void macdrv_UpdateClipboard(void)
{
    static ULONG last_update;
    static HWND clipboard_manager;
    ULONG now;
    DWORD_PTR ret;

    if (GetCurrentThreadId() == clipboard_thread_id) return;

    TRACE("\n");

    now = NtGetTickCount();
    if (last_update && (int)(now - last_update) <= CLIPBOARD_UPDATE_DELAY) return;

    if (!NtUserIsWindow(clipboard_manager))
    {
        UNICODE_STRING str = RTL_CONSTANT_STRING(clipboard_classname);
        clipboard_manager = NtUserFindWindowEx(NULL, NULL, &str, NULL, 0);
        if (!clipboard_manager)
        {
            ERR("clipboard manager not found\n");
            return;
        }
    }

    send_message_timeout(clipboard_manager, WM_USER, 0, 0,
                         SMTO_ABORTIFHUNG, 5000, &ret);
    last_update = now;
}


/**************************************************************************
 *              MACDRV Private Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              query_pasteboard_data
 */
BOOL query_pasteboard_data(HWND hwnd, CFStringRef type)
{
    struct get_clipboard_params params = { .data_only = TRUE, .size = 1024 };
    WINE_CLIPFORMAT *format;
    BOOL ret = FALSE;

    TRACE("win %p/%p type %s\n", hwnd, clipboard_cocoa_window, debugstr_cf(type));

    format = format_for_type(type);
    if (!format) return FALSE;

    if (!NtUserOpenClipboard(clipboard_hwnd, 0))
    {
        ERR("failed to open clipboard for %s\n", debugstr_cf(type));
        return FALSE;
    }

    for (;;)
    {
        if (!(params.data = malloc(params.size))) break;
        if (NtUserGetClipboardData(format->format_id, &params))
        {
            CFDataRef data;

            TRACE("exporting %s\n", debugstr_format(format->format_id));

            if ((data = format->export_func(params.data, params.size)))
            {
                ret = macdrv_set_pasteboard_data(format->type, data, clipboard_cocoa_window);
                CFRelease(data);
            }
            free(params.data);
            break;
        }
        free(params.data);
        if (!params.data_size) break;
        params.size = params.data_size;
        params.data_size = 0;
    }

    last_get_seqno = NtUserGetClipboardSequenceNumber();

    NtUserCloseClipboard();

    return ret;
}


/**************************************************************************
 *              macdrv_lost_pasteboard_ownership
 *
 * Handler for the LOST_PASTEBOARD_OWNERSHIP event.
 */
void macdrv_lost_pasteboard_ownership(HWND hwnd)
{
    TRACE("win %p\n", hwnd);
    if (!macdrv_is_pasteboard_owner(clipboard_cocoa_window))
        grab_win32_clipboard();
}
