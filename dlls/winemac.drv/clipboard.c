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
#include "shellapi.h"
#include "shlobj.h"
#include "wine/list.h"
#include "wine/server.h"
#include "wine/unicode.h"


WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


/**************************************************************************
 *              Types
 **************************************************************************/

typedef HANDLE (*DRVIMPORTFUNC)(CFDataRef data);
typedef CFDataRef (*DRVEXPORTFUNC)(HANDLE data);

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

static HANDLE import_clipboard_data(CFDataRef data);
static HANDLE import_bmp_to_bitmap(CFDataRef data);
static HANDLE import_bmp_to_dib(CFDataRef data);
static HANDLE import_enhmetafile(CFDataRef data);
static HANDLE import_html(CFDataRef data);
static HANDLE import_metafilepict(CFDataRef data);
static HANDLE import_nsfilenames_to_hdrop(CFDataRef data);
static HANDLE import_utf8_to_text(CFDataRef data);
static HANDLE import_utf8_to_unicodetext(CFDataRef data);
static HANDLE import_utf16_to_unicodetext(CFDataRef data);

static CFDataRef export_clipboard_data(HANDLE data);
static CFDataRef export_bitmap_to_bmp(HANDLE data);
static CFDataRef export_dib_to_bmp(HANDLE data);
static CFDataRef export_enhmetafile(HANDLE data);
static CFDataRef export_hdrop_to_filenames(HANDLE data);
static CFDataRef export_html(HANDLE data);
static CFDataRef export_metafilepict(HANDLE data);
static CFDataRef export_text_to_utf8(HANDLE data);
static CFDataRef export_unicodetext_to_utf8(HANDLE data);
static CFDataRef export_unicodetext_to_utf16(HANDLE data);


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
    { CF_BITMAP,            CFSTR("org.winehq.builtin.bitmap"),             import_bmp_to_bitmap,           export_bitmap_to_bmp,       FALSE },
    { CF_DIBV5,             CFSTR("org.winehq.builtin.dibv5"),              import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_DIF,               CFSTR("org.winehq.builtin.dif"),                import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_ENHMETAFILE,       CFSTR("org.winehq.builtin.enhmetafile"),        import_enhmetafile,             export_enhmetafile,         FALSE },
    { CF_LOCALE,            CFSTR("org.winehq.builtin.locale"),             import_clipboard_data,          export_clipboard_data,      FALSE },
    { CF_METAFILEPICT,      CFSTR("org.winehq.builtin.metafilepict"),       import_metafilepict,            export_metafilepict,        FALSE },
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
    { CFSTR_SHELLURLW,      CFSTR("public.url"),                            import_utf8_to_text,            export_text_to_utf8 },
};

/* The prefix prepended to a Win32 clipboard format name to make a Mac pasteboard type. */
static const CFStringRef registered_name_type_prefix = CFSTR("org.winehq.registered.");

static DWORD clipboard_thread_id;
static HWND clipboard_hwnd;
static BOOL is_clipboard_owner;
static macdrv_window clipboard_cocoa_window;
static ULONG64 last_clipboard_update;
static DWORD last_get_seqno;
static WINE_CLIPFORMAT **current_mac_formats;
static unsigned int nb_current_mac_formats;
static WCHAR clipboard_pipe_name[256];


/**************************************************************************
 *              Internal Clipboard implementation methods
 **************************************************************************/

/*
 * format_list functions
 */

/**************************************************************************
 *              debugstr_format
 */
const char *debugstr_format(UINT id)
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
    format->natural_format = NULL;

    if (type)
        format->type = CFStringCreateCopy(NULL, type);
    else
    {
        WCHAR buffer[256];

        if (!GetClipboardFormatNameW(format->format_id, buffer, ARRAY_SIZE(buffer)))
        {
            WARN("failed to get name for format %s; error 0x%08x\n", debugstr_format(format->format_id), GetLastError());
            HeapFree(GetProcessHeap(), 0, format);
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
        if (!(format = HeapAlloc(GetProcessHeap(), 0, sizeof(*format)))) break;
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
        if (!(format = HeapAlloc(GetProcessHeap(), 0, sizeof(*format)))) break;
        format->format_id       = RegisterClipboardFormatW(builtin_format_names[i].name);
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

        name = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
        CFStringGetCharacters(type, CFRangeMake(CFStringGetLength(registered_name_type_prefix), len),
                              (UniChar*)name);
        name[len] = 0;

        format = register_format(RegisterClipboardFormatW(name), type);
        if (!format)
            ERR("Failed to register format for type %s name %s\n", debugstr_cf(type), debugstr_w(name));

        HeapFree(GetProcessHeap(), 0, name);
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


/***********************************************************************
 *              create_dib_from_bitmap
 *
 * Allocates a packed DIB and copies the bitmap data into it.
 */
static HGLOBAL create_dib_from_bitmap(HBITMAP bitmap)
{
    HANDLE ret = 0;
    BITMAPINFOHEADER header;
    HDC hdc = GetDC(0);
    DWORD header_size;
    BITMAPINFO *bmi;

    memset(&header, 0, sizeof(header));
    header.biSize = sizeof(header);
    if (!GetDIBits(hdc, bitmap, 0, 0, NULL, (BITMAPINFO *)&header, DIB_RGB_COLORS)) goto done;

    header_size = bitmap_info_size((BITMAPINFO *)&header, DIB_RGB_COLORS);
    if (!(ret = GlobalAlloc(GMEM_FIXED, header_size + header.biSizeImage))) goto done;
    bmi = (BITMAPINFO *)ret;
    memset(bmi, 0, header_size);
    memcpy(bmi, &header, header.biSize);
    GetDIBits(hdc, bitmap, 0, abs(header.biHeight), (char *)bmi + header_size, bmi, DIB_RGB_COLORS);

done:
    ReleaseDC(0, hdc);
    return ret;
}


/**************************************************************************
 *              create_bitmap_from_dib
 *
 *  Given a packed DIB, creates a bitmap object from it.
 */
static HANDLE create_bitmap_from_dib(HANDLE dib)
{
    HANDLE ret = 0;
    BITMAPINFO *bmi;

    if (dib && (bmi = GlobalLock(dib)))
    {
        HDC hdc;
        unsigned int offset;

        hdc = GetDC(NULL);

        offset = bitmap_info_size(bmi, DIB_RGB_COLORS);

        ret = CreateDIBitmap(hdc, &bmi->bmiHeader, CBM_INIT, (LPBYTE)bmi + offset,
                             bmi, DIB_RGB_COLORS);

        GlobalUnlock(dib);
        ReleaseDC(NULL, hdc);
    }

    return ret;
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
static HANDLE import_clipboard_data(CFDataRef data)
{
    HANDLE data_handle = NULL;

    size_t len = CFDataGetLength(data);
    if (len)
    {
        LPVOID p;

        /* Turn on the DDESHARE flag to enable shared 32 bit memory */
        data_handle = GlobalAlloc(GMEM_FIXED, len);
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
 *              import_bmp_to_bitmap
 *
 *  Import BMP data, converting to CF_BITMAP format.
 */
static HANDLE import_bmp_to_bitmap(CFDataRef data)
{
    HANDLE ret;
    HANDLE dib = import_bmp_to_dib(data);

    ret = create_bitmap_from_dib(dib);

    GlobalFree(dib);
    return ret;
}


/**************************************************************************
 *              import_bmp_to_dib
 *
 *  Import BMP data, converting to CF_DIB or CF_DIBV5 format.  This just
 *  entails stripping the BMP file format header.
 */
static HANDLE import_bmp_to_dib(CFDataRef data)
{
    HANDLE ret = 0;
    BITMAPFILEHEADER *bfh = (BITMAPFILEHEADER*)CFDataGetBytePtr(data);
    CFIndex len = CFDataGetLength(data);

    if (len >= sizeof(*bfh) + sizeof(BITMAPCOREHEADER) &&
        bfh->bfType == 0x4d42 /* "BM" */)
    {
        BITMAPINFO *bmi = (BITMAPINFO*)(bfh + 1);
        BYTE* p;

        len -= sizeof(*bfh);
        ret = GlobalAlloc(GMEM_FIXED, len);
        if (!ret || !(p = GlobalLock(ret)))
        {
            GlobalFree(ret);
            return 0;
        }

        memcpy(p, bmi, len);
        GlobalUnlock(ret);
    }

    return ret;
}


/**************************************************************************
 *              import_enhmetafile
 *
 *  Import enhanced metafile data, converting it to CF_ENHMETAFILE.
 */
static HANDLE import_enhmetafile(CFDataRef data)
{
    HANDLE ret = 0;
    CFIndex len = CFDataGetLength(data);

    TRACE("data %s\n", debugstr_cf(data));

    if (len)
        ret = SetEnhMetaFileBits(len, (const BYTE*)CFDataGetBytePtr(data));

    return ret;
}


/**************************************************************************
 *              import_html
 *
 *  Import HTML data.
 */
static HANDLE import_html(CFDataRef data)
{
    static const char header[] =
        "Version:0.9\n"
        "StartHTML:0000000100\n"
        "EndHTML:%010lu\n"
        "StartFragment:%010lu\n"
        "EndFragment:%010lu\n"
        "<!--StartFragment-->";
    static const char trailer[] = "\n<!--EndFragment-->";
    HANDLE ret;
    SIZE_T len, total;
    size_t size = CFDataGetLength(data);

    len = strlen(header) + 12;  /* 3 * 4 extra chars for %010lu */
    total = len + size + sizeof(trailer);
    if ((ret = GlobalAlloc(GMEM_FIXED, total)))
    {
        char *p = ret;
        p += sprintf(p, header, total - 1, len, len + size + 1 /* include the final \n in the data */);
        CFDataGetBytes(data, CFRangeMake(0, size), (UInt8*)p);
        strcpy(p + size, trailer);
        TRACE("returning %s\n", debugstr_a(ret));
    }
    return ret;
}


/**************************************************************************
 *              import_metafilepict
 *
 *  Import metafile picture data, converting it to CF_METAFILEPICT.
 */
static HANDLE import_metafilepict(CFDataRef data)
{
    HANDLE ret = 0;
    CFIndex len = CFDataGetLength(data);
    METAFILEPICT *mfp;

    TRACE("data %s\n", debugstr_cf(data));

    if (len >= sizeof(*mfp) && (ret = GlobalAlloc(GMEM_FIXED, sizeof(*mfp))))
    {
        const BYTE *bytes = (const BYTE*)CFDataGetBytePtr(data);

        mfp = GlobalLock(ret);
        memcpy(mfp, bytes, sizeof(*mfp));
        mfp->hMF = SetMetaFileBitsEx(len - sizeof(*mfp), bytes + sizeof(*mfp));
        GlobalUnlock(ret);
    }

    return ret;
}


/**************************************************************************
 *              import_nsfilenames_to_hdrop
 *
 *  Import NSFilenamesPboardType data, converting the property-list-
 *  serialized array of path strings to CF_HDROP.
 */
static HANDLE import_nsfilenames_to_hdrop(CFDataRef data)
{
    HDROP hdrop = NULL;
    CFArrayRef names;
    CFIndex count, i;
    size_t len;
    char *buffer = NULL;
    WCHAR **paths = NULL;
    DROPFILES* dropfiles;
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

    buffer = HeapAlloc(GetProcessHeap(), 0, len);
    if (!buffer)
    {
        WARN("failed to allocate buffer for file-system representations\n");
        goto done;
    }

    paths = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * sizeof(paths[0]));
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
        paths[i] = wine_get_dos_file_name(buffer);
        if (!paths[i])
        {
            WARN("failed to get DOS path for %s\n", debugstr_a(buffer));
            goto done;
        }
    }

    len = 1; /* for the terminating null */
    for (i = 0; i < count; i++)
        len += strlenW(paths[i]) + 1;

    hdrop = GlobalAlloc(GMEM_FIXED, sizeof(*dropfiles) + len * sizeof(WCHAR));
    if (!hdrop || !(dropfiles = GlobalLock(hdrop)))
    {
        WARN("failed to allocate HDROP\n");
        GlobalFree(hdrop);
        hdrop = NULL;
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
        strcpyW(p, paths[i]);
        p += strlenW(p) + 1;
    }
    *p = 0;

    GlobalUnlock(hdrop);

done:
    if (paths)
    {
        for (i = 0; i < count; i++)
            HeapFree(GetProcessHeap(), 0, paths[i]);
        HeapFree(GetProcessHeap(), 0, paths);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    if (names) CFRelease(names);
    return hdrop;
}


/**************************************************************************
 *              import_utf8_to_text
 *
 *  Import a UTF-8 string, converting the string to CF_TEXT.
 */
static HANDLE import_utf8_to_text(CFDataRef data)
{
    HANDLE ret = NULL;
    HANDLE unicode_handle = import_utf8_to_unicodetext(data);
    LPWSTR unicode_string = GlobalLock(unicode_handle);

    if (unicode_string)
    {
        int unicode_len;
        HANDLE handle;
        char *p;
        INT len;

        unicode_len = GlobalSize(unicode_handle) / sizeof(WCHAR);

        len = WideCharToMultiByte(CP_ACP, 0, unicode_string, unicode_len, NULL, 0, NULL, NULL);
        if (!unicode_len || unicode_string[unicode_len - 1]) len += 1;
        handle = GlobalAlloc(GMEM_FIXED, len);

        if (handle && (p = GlobalLock(handle)))
        {
            WideCharToMultiByte(CP_ACP, 0, unicode_string, unicode_len, p, len, NULL, NULL);
            p[len - 1] = 0;
            GlobalUnlock(handle);
            ret = handle;
        }
        GlobalUnlock(unicode_handle);
    }

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
    unsigned long src_len;
    unsigned long new_lines = 0;
    LPSTR dst;
    unsigned long i, j;
    HANDLE unicode_handle = NULL;

    src = CFDataGetBytePtr(data);
    src_len = CFDataGetLength(data);
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == '\n')
            new_lines++;
    }

    if ((dst = HeapAlloc(GetProcessHeap(), 0, src_len + new_lines + 1)))
    {
        UINT count;

        for (i = 0, j = 0; i < src_len; i++)
        {
            if (src[i] == '\n')
                dst[j++] = '\r';

            dst[j++] = src[i];
        }
        dst[j] = 0;

        count = MultiByteToWideChar(CP_UTF8, 0, dst, -1, NULL, 0);
        unicode_handle = GlobalAlloc(GMEM_FIXED, count * sizeof(WCHAR));

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
 *              import_utf16_to_unicodetext
 *
 *  Import a UTF-8 string, converting the string to CF_UNICODETEXT.
 */
static HANDLE import_utf16_to_unicodetext(CFDataRef data)
{
    const WCHAR *src;
    unsigned long src_len;
    unsigned long new_lines = 0;
    LPWSTR dst;
    unsigned long i, j;
    HANDLE unicode_handle;

    src = (const WCHAR *)CFDataGetBytePtr(data);
    src_len = CFDataGetLength(data) / sizeof(WCHAR);
    for (i = 0; i < src_len; i++)
    {
        if (src[i] == '\n')
            new_lines++;
        else if (src[i] == '\r' && (i + 1 >= src_len || src[i + 1] != '\n'))
            new_lines++;
    }

    if ((unicode_handle = GlobalAlloc(GMEM_FIXED, (src_len + new_lines + 1) * sizeof(WCHAR))))
    {
        dst = GlobalLock(unicode_handle);

        for (i = 0, j = 0; i < src_len; i++)
        {
            if (src[i] == '\n')
                dst[j++] = '\r';

            dst[j++] = src[i];

            if (src[i] == '\r' && (i + 1 >= src_len || src[i + 1] != '\n'))
                dst[j++] = '\n';
        }
        dst[j] = 0;

        GlobalUnlock(unicode_handle);
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
 *              export_bitmap_to_bmp
 *
 *  Export CF_BITMAP to BMP file format.
 */
static CFDataRef export_bitmap_to_bmp(HANDLE data)
{
    CFDataRef ret = NULL;
    HGLOBAL dib;

    dib = create_dib_from_bitmap(data);
    if (dib)
    {
        ret = export_dib_to_bmp(dib);
        GlobalFree(dib);
    }

    return ret;
}


/**************************************************************************
 *              export_dib_to_bmp
 *
 *  Export CF_DIB or CF_DIBV5 to BMP file format.  This just entails
 *  prepending a BMP file format header to the data.
 */
static CFDataRef export_dib_to_bmp(HANDLE data)
{
    CFMutableDataRef ret = NULL;
    BYTE *dibdata;
    CFIndex len;
    BITMAPFILEHEADER bfh;

    dibdata = GlobalLock(data);
    if (!dibdata)
        return NULL;

    len = sizeof(bfh) + GlobalSize(data);
    ret = CFDataCreateMutable(NULL, len);
    if (ret)
    {
        bfh.bfType = 0x4d42; /* "BM" */
        bfh.bfSize = len;
        bfh.bfReserved1 = 0;
        bfh.bfReserved2 = 0;
        bfh.bfOffBits = sizeof(bfh) + bitmap_info_size((BITMAPINFO*)dibdata, DIB_RGB_COLORS);
        CFDataAppendBytes(ret, (UInt8*)&bfh, sizeof(bfh));

        /* rest of bitmap is the same as the packed dib */
        CFDataAppendBytes(ret, (UInt8*)dibdata, len - sizeof(bfh));
    }

    GlobalUnlock(data);

    return ret;
}


/**************************************************************************
 *              export_enhmetafile
 *
 *  Export an enhanced metafile to data.
 */
static CFDataRef export_enhmetafile(HANDLE data)
{
    CFMutableDataRef ret = NULL;
    unsigned int size = GetEnhMetaFileBits(data, 0, NULL);

    TRACE("data %p\n", data);

    ret = CFDataCreateMutable(NULL, size);
    if (ret)
    {
        CFDataSetLength(ret, size);
        GetEnhMetaFileBits(data, size, (BYTE*)CFDataGetMutableBytePtr(ret));
    }

    TRACE(" -> %s\n", debugstr_cf(ret));
    return ret;
}


/**************************************************************************
 *              export_hdrop_to_filenames
 *
 *  Export CF_HDROP to NSFilenamesPboardType data, which is a CFArray of
 *  CFStrings (holding Unix paths) which is serialized as a property list.
 */
static CFDataRef export_hdrop_to_filenames(HANDLE data)
{
    CFDataRef ret = NULL;
    DROPFILES *dropfiles;
    CFMutableArrayRef filenames = NULL;
    void *p;
    WCHAR *buffer = NULL;
    size_t buffer_len = 0;

    TRACE("data %p\n", data);

    if (!(dropfiles = GlobalLock(data)))
    {
        WARN("failed to lock data %p\n", data);
        goto done;
    }

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
            unixname = wine_get_unix_file_name(p);
        else
        {
            int len = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
            if (len)
            {
                if (len > buffer_len)
                {
                    HeapFree(GetProcessHeap(), 0, buffer);
                    buffer_len = len * 2;
                    buffer = HeapAlloc(GetProcessHeap(), 0, buffer_len * sizeof(*buffer));
                }

                MultiByteToWideChar(CP_ACP, 0, p, -1, buffer, buffer_len);
                unixname = wine_get_unix_file_name(buffer);
            }
            else
                unixname = NULL;
        }
        if (!unixname)
        {
            WARN("failed to convert DOS path to Unix: %s\n",
                 dropfiles->fWide ? debugstr_w(p) : debugstr_a(p));
            goto done;
        }

        if (dropfiles->fWide)
            p = (WCHAR*)p + strlenW(p) + 1;
        else
            p = (char*)p + strlen(p) + 1;

        filename = CFStringCreateWithFileSystemRepresentation(NULL, unixname);
        HeapFree(GetProcessHeap(), 0, unixname);
        if (!filename)
        {
            WARN("failed to create CFString from Unix path %s\n", debugstr_a(unixname));
            goto done;
        }

        CFArrayAppendValue(filenames, filename);
        CFRelease(filename);
    }

    ret = CFPropertyListCreateData(NULL, filenames, kCFPropertyListXMLFormat_v1_0, 0, NULL);

done:
    HeapFree(GetProcessHeap(), 0, buffer);
    GlobalUnlock(data);
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
static CFDataRef export_html(HANDLE handle)
{
    CFDataRef ret;
    const char *data, *field_value;
    int fragmentstart, fragmentend;

    data = GlobalLock(handle);

    /* read the important fields */
    field_value = get_html_description_field(data, "StartFragment:");
    if (!field_value)
    {
        ERR("Couldn't find StartFragment value\n");
        goto failed;
    }
    fragmentstart = atoi(field_value);

    field_value = get_html_description_field(data, "EndFragment:");
    if (!field_value)
    {
        ERR("Couldn't find EndFragment value\n");
        goto failed;
    }
    fragmentend = atoi(field_value);

    /* export only the fragment */
    ret = CFDataCreate(NULL, (const UInt8*)&data[fragmentstart], fragmentend - fragmentstart);
    GlobalUnlock(handle);
    return ret;

failed:
    GlobalUnlock(handle);
    return NULL;
}


/**************************************************************************
 *              export_metafilepict
 *
 *  Export a metafile to data.
 */
static CFDataRef export_metafilepict(HANDLE data)
{
    CFMutableDataRef ret = NULL;
    METAFILEPICT *mfp = GlobalLock(data);
    unsigned int size = GetMetaFileBitsEx(mfp->hMF, 0, NULL);

    TRACE("data %p\n", data);

    ret = CFDataCreateMutable(NULL, sizeof(*mfp) + size);
    if (ret)
    {
        CFDataAppendBytes(ret, (UInt8*)mfp, sizeof(*mfp));
        CFDataIncreaseLength(ret, size);
        GetMetaFileBitsEx(mfp->hMF, size, (BYTE*)CFDataGetMutableBytePtr(ret) + sizeof(*mfp));
    }

    GlobalUnlock(data);
    TRACE(" -> %s\n", debugstr_cf(ret));
    return ret;
}


/**************************************************************************
 *              export_text_to_utf8
 *
 *  Export CF_TEXT to UTF-8.
 */
static CFDataRef export_text_to_utf8(HANDLE data)
{
    CFDataRef ret = NULL;
    const char* str;

    if ((str = GlobalLock(data)))
    {
        int str_len = GlobalSize(data);
        int wstr_len;
        WCHAR *wstr;
        HANDLE unicode;
        char *p;

        wstr_len = MultiByteToWideChar(CP_ACP, 0, str, str_len, NULL, 0);
        if (!str_len || str[str_len - 1]) wstr_len += 1;
        wstr = HeapAlloc(GetProcessHeap(), 0, wstr_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, str_len, wstr, wstr_len);
        wstr[wstr_len - 1] = 0;

        unicode = GlobalAlloc(GMEM_FIXED, wstr_len * sizeof(WCHAR));
        if (unicode && (p = GlobalLock(unicode)))
        {
            memcpy(p, wstr, wstr_len * sizeof(WCHAR));
            GlobalUnlock(unicode);
        }

        ret = export_unicodetext_to_utf8(unicode);

        GlobalFree(unicode);
        GlobalUnlock(data);
    }

    return ret;
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
 *              export_unicodetext_to_utf16
 *
 *  Export CF_UNICODETEXT to UTF-16.
 */
static CFDataRef export_unicodetext_to_utf16(HANDLE data)
{
    CFMutableDataRef ret;
    const WCHAR *src;
    INT src_len;

    src = GlobalLock(data);
    if (!src) return NULL;

    src_len = GlobalSize(data) / sizeof(WCHAR);
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
    GlobalUnlock(data);

    return ret;
}


/**************************************************************************
 *              macdrv_get_pasteboard_data
 */
HANDLE macdrv_get_pasteboard_data(CFTypeRef pasteboard, UINT desired_format)
{
    CFArrayRef types;
    CFIndex count;
    CFIndex i;
    CFStringRef type, best_type;
    WINE_CLIPFORMAT* best_format = NULL;
    HANDLE data = NULL;

    TRACE("pasteboard %p, desired_format %s\n", pasteboard, debugstr_format(desired_format));

    types = macdrv_copy_pasteboard_types(pasteboard);
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

        if ((format = format_for_type(type)))
        {
            TRACE("for type %s got format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));

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
        CFDataRef pasteboard_data = macdrv_copy_pasteboard_data(pasteboard, best_type);

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
 *              macdrv_pasteboard_has_format
 */
BOOL macdrv_pasteboard_has_format(CFTypeRef pasteboard, UINT desired_format)
{
    CFArrayRef types;
    int count;
    UINT i;
    BOOL found = FALSE;

    TRACE("pasteboard %p, desired_format %s\n", pasteboard, debugstr_format(desired_format));

    types = macdrv_copy_pasteboard_types(pasteboard);
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return FALSE;
    }

    count = CFArrayGetCount(types);
    TRACE("got %d types\n", count);

    for (i = 0; i < count; i++)
    {
        CFStringRef type = CFArrayGetValueAtIndex(types, i);
        WINE_CLIPFORMAT* format = format_for_type(type);

        if (format)
        {
            TRACE("for type %s got format %s\n", debugstr_cf(type), debugstr_format(format->format_id));

            if (format->format_id == desired_format)
            {
                found = TRUE;
                break;
            }
        }
    }

    CFRelease(types);
    TRACE(" -> %d\n", found);
    return found;
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

    formats = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*formats));
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
            CFSetAddValue(seen_formats, (void*)format->format_id);
            formats[pos++] = format;
        }
        else if (format->natural_format &&
                 CFArrayContainsValue(types, CFRangeMake(0, count), format->natural_format->type))
        {
            TRACE("for type %s deferring synthesized formats because type %s is also present\n",
                  debugstr_cf(type), debugstr_cf(format->natural_format->type));
        }
        else if (CFSetContainsValue(seen_formats, (void*)format->format_id))
        {
            TRACE("for type %s got duplicate synthesized format %p/%s; skipping\n", debugstr_cf(type), format,
                  debugstr_format(format->format_id));
        }
        else
        {
            TRACE("for type %s got synthesized format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));
            CFSetAddValue(seen_formats, (void*)format->format_id);
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
        if (CFSetContainsValue(seen_formats, (void*)format->format_id)) continue;

        TRACE("for type %s got synthesized format %p/%s\n", debugstr_cf(type), format, debugstr_format(format->format_id));
        CFSetAddValue(seen_formats, (void*)format->format_id);
        formats[pos++] = format;
    }

    CFRelease(seen_formats);

    if (!pos)
    {
        HeapFree(GetProcessHeap(), 0, formats);
        formats = NULL;
    }

    *num_formats = pos;
    return formats;
}


/**************************************************************************
 *              get_formats_for_pasteboard
 */
static WINE_CLIPFORMAT** get_formats_for_pasteboard(CFTypeRef pasteboard, UINT *num_formats)
{
    CFArrayRef types;
    WINE_CLIPFORMAT** formats;

    TRACE("pasteboard %s\n", debugstr_cf(pasteboard));

    types = macdrv_copy_pasteboard_types(pasteboard);
    if (!types)
    {
        WARN("Failed to copy pasteboard types\n");
        return NULL;
    }

    formats = get_formats_for_pasteboard_types(types, num_formats);
    CFRelease(types);
    return formats;
}


/**************************************************************************
 *              macdrv_get_pasteboard_formats
 */
UINT* macdrv_get_pasteboard_formats(CFTypeRef pasteboard, UINT* num_formats)
{
    WINE_CLIPFORMAT** formats;
    UINT count, i;
    UINT* format_ids;

    formats = get_formats_for_pasteboard(pasteboard, &count);
    if (!formats)
        return NULL;

    format_ids = HeapAlloc(GetProcessHeap(), 0, count);
    if (!format_ids)
    {
        WARN("Failed to allocate formats IDs array\n");
        HeapFree(GetProcessHeap(), 0, formats);
        return NULL;
    }

    for (i = 0; i < count; i++)
        format_ids[i] = formats[i]->format_id;

    HeapFree(GetProcessHeap(), 0, formats);

    *num_formats = count;
    return format_ids;
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
        if (!(ids = HeapAlloc(GetProcessHeap(), 0, *size * sizeof(*ids)))) return NULL;
        if (GetUpdatedClipboardFormats(ids, *size, size)) break;
        HeapFree(GetProcessHeap(), 0, ids);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;
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

    HeapFree(GetProcessHeap(), 0, formats);
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
        TRACE("adding format %s\n", debugstr_format(formats[i]->format_id));
        SetClipboardData(formats[i]->format_id, 0);
    }

    HeapFree(GetProcessHeap(), 0, current_mac_formats);
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
            HANDLE handle = current_mac_formats[i]->import_func(pasteboard_data);
            CFRelease(pasteboard_data);
            if (handle) SetClipboardData(id, handle);
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

    if (!OpenClipboard(clipboard_hwnd)) return;
    EmptyClipboard();
    is_clipboard_owner = TRUE;
    last_clipboard_update = GetTickCount64();
    set_win32_clipboard_formats_from_mac_pasteboard(types);
    CloseClipboard();
    SetTimer(clipboard_hwnd, 1, CLIPBOARD_UPDATE_DELAY, NULL);
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

    TRACE("is_clipboard_owner %d last_clipboard_update %llu now %llu\n",
          is_clipboard_owner, (unsigned long long)last_clipboard_update, (unsigned long long)GetTickCount64());

    if (updating) return;
    updating = TRUE;

    if (is_clipboard_owner)
    {
        if (GetTickCount64() - last_clipboard_update > CLIPBOARD_UPDATE_DELAY)
            grab_win32_clipboard();
    }
    else if (!macdrv_is_pasteboard_owner(clipboard_cocoa_window))
        grab_win32_clipboard();

    updating = FALSE;
}


/**************************************************************************
 *              clipboard_wndproc
 *
 * Window procedure for the clipboard manager.
 */
static LRESULT CALLBACK clipboard_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NCCREATE:
            return TRUE;
        case WM_CLIPBOARDUPDATE:
            if (is_clipboard_owner) break;  /* ignore our own changes */
            if ((LONG)(GetClipboardSequenceNumber() - last_get_seqno) <= 0) break;
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
            KillTimer(hwnd, 1);
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}


/**************************************************************************
 *              wait_clipboard_mutex
 *
 * Make sure that there's only one clipboard thread per window station.
 */
static BOOL wait_clipboard_mutex(void)
{
    static const WCHAR prefix[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_'};
    WCHAR buffer[MAX_PATH + ARRAY_SIZE(prefix)];
    HANDLE mutex;

    memcpy(buffer, prefix, sizeof(prefix));
    if (!GetUserObjectInformationW(GetProcessWindowStation(), UOI_NAME,
                                   buffer + ARRAY_SIZE(prefix),
                                   sizeof(buffer) - sizeof(prefix), NULL))
    {
        ERR("failed to get winstation name\n");
        return FALSE;
    }
    mutex = CreateMutexW(NULL, TRUE, buffer);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE("waiting for mutex %s\n", debugstr_w(buffer));
        WaitForSingleObject(mutex, INFINITE);
    }
    return TRUE;
}


/**************************************************************************
 *              init_pipe_name
 *
 * Init-once helper for get_pipe_name.
 */
static BOOL CALLBACK init_pipe_name(INIT_ONCE* once, void* param, void** context)
{
    static const WCHAR prefix[] = {'\\','\\','.','\\','p','i','p','e','\\','_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_'};

    memcpy(clipboard_pipe_name, prefix, sizeof(prefix));
    if (!GetUserObjectInformationW(GetProcessWindowStation(), UOI_NAME,
                                   clipboard_pipe_name + ARRAY_SIZE(prefix),
                                   sizeof(clipboard_pipe_name) - sizeof(prefix), NULL))
    {
        ERR("failed to get winstation name\n");
        return FALSE;
    }

    return TRUE;
}


/**************************************************************************
 *              get_pipe_name
 *
 * Get the name of the pipe used to communicate with the per-window-station
 * clipboard manager thread.
 */
static const WCHAR* get_pipe_name(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&once, init_pipe_name, NULL, NULL);
    return clipboard_pipe_name[0] ? clipboard_pipe_name : NULL;
}


/**************************************************************************
 *              clipboard_thread
 *
 * Thread running inside the desktop process to manage the clipboard
 */
static DWORD WINAPI clipboard_thread(void *arg)
{
    static const WCHAR clipboard_classname[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r',0};
    WNDCLASSW class;
    struct macdrv_window_features wf;
    const WCHAR* pipe_name;
    HANDLE pipe = NULL;
    HANDLE event = NULL;
    OVERLAPPED overlapped;
    BOOL need_connect = TRUE, pending = FALSE;
    MSG msg;

    if (!wait_clipboard_mutex()) return 0;

    memset(&class, 0, sizeof(class));
    class.lpfnWndProc   = clipboard_wndproc;
    class.lpszClassName = clipboard_classname;

    if (!RegisterClassW(&class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR("could not register clipboard window class err %u\n", GetLastError());
        return 0;
    }
    if (!(clipboard_hwnd = CreateWindowW(clipboard_classname, NULL, 0, 0, 0, 0, 0,
                                         HWND_MESSAGE, 0, 0, NULL)))
    {
        ERR("failed to create clipboard window err %u\n", GetLastError());
        return 0;
    }

    memset(&wf, 0, sizeof(wf));
    clipboard_cocoa_window = macdrv_create_cocoa_window(&wf, CGRectMake(100, 100, 100, 100), clipboard_hwnd,
                                                        macdrv_init_thread_data()->queue);
    if (!clipboard_cocoa_window)
    {
        ERR("failed to create clipboard Cocoa window\n");
        goto done;
    }

    pipe_name = get_pipe_name();
    if (!pipe_name)
    {
        ERR("failed to get pipe name\n");
        goto done;
    }

    pipe = CreateNamedPipeW(pipe_name, PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_BYTE, PIPE_UNLIMITED_INSTANCES, 1, 1, 0, NULL);
    if (!pipe)
    {
        ERR("failed to create named pipe: %u\n", GetLastError());
        goto done;
    }

    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!event)
    {
        ERR("failed to create event: %d\n", GetLastError());
        goto done;
    }

    clipboard_thread_id = GetCurrentThreadId();
    AddClipboardFormatListener(clipboard_hwnd);
    register_builtin_formats();
    grab_win32_clipboard();

    TRACE("clipboard thread %04x running\n", GetCurrentThreadId());
    while (1)
    {
        DWORD result;

        if (need_connect)
        {
            pending = FALSE;
            memset(&overlapped, 0, sizeof(overlapped));
            overlapped.hEvent = event;
            if (ConnectNamedPipe(pipe, &overlapped))
            {
                ERR("asynchronous ConnectNamedPipe unexpectedly returned true: %d\n", GetLastError());
                ResetEvent(event);
            }
            else
            {
                result = GetLastError();
                switch (result)
                {
                    case ERROR_PIPE_CONNECTED:
                    case ERROR_NO_DATA:
                        SetEvent(event);
                        need_connect = FALSE;
                        break;
                    case ERROR_IO_PENDING:
                        need_connect = FALSE;
                        pending = TRUE;
                        break;
                    default:
                        ERR("failed to initiate pipe connection: %d\n", result);
                        break;
                }
            }
        }

        result = MsgWaitForMultipleObjectsEx(1, &event, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
        switch (result)
        {
            case WAIT_OBJECT_0:
            {
                DWORD written;

                if (pending && !GetOverlappedResult(pipe, &overlapped, &written, FALSE))
                    ERR("failed to connect pipe: %d\n", GetLastError());

                update_clipboard();
                DisconnectNamedPipe(pipe);
                need_connect = TRUE;
                break;
            }
            case WAIT_OBJECT_0 + 1:
                while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        goto done;
                    DispatchMessageW(&msg);
                }
                break;
            case WAIT_IO_COMPLETION:
                break;
            default:
                ERR("failed to wait for connection or input: %d\n", GetLastError());
                break;
        }
    }

done:
    if (event) CloseHandle(event);
    if (pipe) CloseHandle(pipe);
    macdrv_destroy_cocoa_window(clipboard_cocoa_window);
    DestroyWindow(clipboard_hwnd);
    return 0;
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
    ULONG now, end;
    const WCHAR* pipe_name;
    HANDLE pipe;
    BYTE dummy;
    DWORD count;
    OVERLAPPED overlapped = { 0 };
    BOOL canceled = FALSE;

    if (GetCurrentThreadId() == clipboard_thread_id) return;

    TRACE("\n");

    now = GetTickCount();
    if ((int)(now - last_update) <= CLIPBOARD_UPDATE_DELAY) return;
    last_update = now;

    if (!(pipe_name = get_pipe_name())) return;
    pipe = CreateFileW(pipe_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        WARN("failed to open pipe to clipboard manager: %d\n", GetLastError());
        return;
    }

    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent)
    {
        ERR("failed to create event: %d\n", GetLastError());
        goto done;
    }

    /* We expect the read to fail because the server just closes our connection.  This
       is just waiting for that close to happen. */
    if (ReadFile(pipe, &dummy, sizeof(dummy), NULL, &overlapped))
    {
        WARN("asynchronous ReadFile unexpectedly returned true: %d\n", GetLastError());
        goto done;
    }
    else
    {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_NOT_CONNECTED || error == ERROR_BROKEN_PIPE)
        {
            /* The server accepted, handled, and closed our connection before we
               attempted the read, which is fine. */
            goto done;
        }
        else if (error != ERROR_IO_PENDING)
        {
            ERR("failed to initiate read from pipe: %d\n", error);
            goto done;
        }
    }

    end = now + 500;
    while (1)
    {
        DWORD result, timeout;

        if (canceled)
            timeout = INFINITE;
        else
        {
            now = GetTickCount();
            timeout = end - now;
            if ((int)timeout < 0)
                timeout = 0;
        }

        result = MsgWaitForMultipleObjectsEx(1, &overlapped.hEvent, timeout, QS_SENDMESSAGE, MWMO_ALERTABLE);
        switch (result)
        {
            case WAIT_OBJECT_0:
            {
                if (GetOverlappedResult(pipe, &overlapped, &count, FALSE))
                    WARN("unexpectedly succeeded in reading from pipe\n");
                else
                {
                    result = GetLastError();
                    if (result != ERROR_BROKEN_PIPE && result != ERROR_OPERATION_ABORTED &&
                        result != ERROR_HANDLES_CLOSED)
                        WARN("failed to read from pipe: %d\n", result);
                }

                goto done;
            }
            case WAIT_OBJECT_0 + 1:
            {
                MSG msg;
                while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE))
                    DispatchMessageW(&msg);
                break;
            }
            case WAIT_IO_COMPLETION:
                break;
            case WAIT_TIMEOUT:
                WARN("timed out waiting for read\n");
                CancelIoEx(pipe, &overlapped);
                canceled = TRUE;
                break;
            default:
                if (canceled)
                {
                    ERR("failed to wait for cancel: %d\n", GetLastError());
                    goto done;
                }

                ERR("failed to wait for read: %d\n", GetLastError());
                CancelIoEx(pipe, &overlapped);
                canceled = TRUE;
                break;
        }
    }

done:
    if (overlapped.hEvent) CloseHandle(overlapped.hEvent);
    CloseHandle(pipe);
}


/**************************************************************************
 *              MACDRV Private Clipboard Exports
 **************************************************************************/


/**************************************************************************
 *              query_pasteboard_data
 */
BOOL query_pasteboard_data(HWND hwnd, CFStringRef type)
{
    WINE_CLIPFORMAT *format;
    BOOL ret = FALSE;
    HANDLE handle;

    TRACE("win %p/%p type %s\n", hwnd, clipboard_cocoa_window, debugstr_cf(type));

    format = format_for_type(type);
    if (!format) return FALSE;

    if (!OpenClipboard(clipboard_hwnd))
    {
        ERR("failed to open clipboard for %s\n", debugstr_cf(type));
        return FALSE;
    }

    if ((handle = GetClipboardData(format->format_id)))
    {
        CFDataRef data;

        TRACE("exporting %s %p\n", debugstr_format(format->format_id), handle);

        if ((data = format->export_func(handle)))
        {
            ret = macdrv_set_pasteboard_data(format->type, data, clipboard_cocoa_window);
            CFRelease(data);
        }
    }

    last_get_seqno = GetClipboardSequenceNumber();

    CloseClipboard();

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


/**************************************************************************
 *              macdrv_init_clipboard
 */
void macdrv_init_clipboard(void)
{
    DWORD id;
    HANDLE handle = CreateThread(NULL, 0, clipboard_thread, NULL, 0, &id);

    if (handle) CloseHandle(handle);
    else ERR("failed to create clipboard thread\n");
}
