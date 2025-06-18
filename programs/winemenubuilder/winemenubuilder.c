/*
 * Helper program to build unix menu entries
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike McCormack for CodeWeavers
 * Copyright 2004 Dmitry Timoshkov
 * Copyright 2005 Bill Medland
 * Copyright 2008 Damjan Jovanovic
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
 *
 *
 *  This program is used to replicate the Windows desktop and start menu
 * into the native desktop's copies.  Desktop entries are merged directly
 * into the native desktop.  The Windows Start Menu corresponds to a Wine
 * entry within the native "start" menu and replicates the whole tree
 * structure of the Windows Start Menu.  Currently it does not differentiate
 * between the user's desktop/start menu and the "All Users" copies.
 *
 *  This program will read a Windows shortcut file using the IShellLink
 * interface, then create a KDE/GNOME menu entry for the shortcut.
 *
 *  winemenubuilder [ -w ] <shortcut.lnk>
 *
 *  If the -w parameter is passed, and the shortcut cannot be created,
 * this program will wait for the parent process to finish and then try
 * again. This covers the case when a ShortCut is created before the
 * executable containing its icon.
 *
 * TODO
 *  Handle data lnk files. There is no icon in the file; the icon is in 
 * the handler for the file type (or pointed to by the lnk file).  Also it 
 * might be better to use a native handler (e.g. a native acroread for pdf
 * files).  
 *  Differentiate between the user's entries and the "All Users" entries.
 * If it is possible to add the desktop files to the native system's
 * shared location for an "All Users" entry then do so.  As a suggestion the
 * shared menu Wine base could be writable to the wine group, or a wineadm 
 * group.
 *  Clean up fd.o menu icons and .directory files when the menu is deleted
 * in Windows.
 *  Associate applications under HKCR\Applications to open any MIME type
 * (by associating with application/octet-stream, or how?).
 *  Clean up fd.o MIME types when they are deleted in Windows, their icons
 * too. Very hard - once we associate them with fd.o, we can't tell whether
 * they are ours or not, and the extension <-> MIME type mapping isn't
 * one-to-one either.
 *  Wine's HKCR is broken - it doesn't merge HKCU\Software\Classes, so apps
 * that write associations there won't associate (#17019).
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#define COBJMACROS
#include <windows.h>
#include <winternl.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlguid.h>
#include <appmgmt.h>
#include <tlhelp32.h>
#include <intshcut.h>
#include <shlwapi.h>
#include <initguid.h>
#include <wincodec.h>

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(menubuilder);

#define in_desktop_dir(csidl) ((csidl)==CSIDL_DESKTOPDIRECTORY || \
                               (csidl)==CSIDL_COMMON_DESKTOPDIRECTORY)
#define in_startmenu(csidl)   ((csidl)==CSIDL_STARTMENU || \
                               (csidl)==CSIDL_COMMON_STARTMENU)

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

/* link file formats */

#pragma pack(push,1)

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID;
} GRPICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    GRPICONDIRENTRY idEntries[1];
} GRPICONDIR;

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONDIR;

typedef struct
{
    WORD offset;
    WORD length;
    WORD flags;
    WORD id;
    WORD handle;
    WORD usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

#define NE_RSCTYPE_ICON        0x8003
#define NE_RSCTYPE_GROUP_ICON  0x800e

#pragma pack(pop)

typedef struct
{
        HRSRC *pResInfo;
        int   nIndex;
} ENUMRESSTRUCT;

struct xdg_mime_type
{
    WCHAR *mimeType;
    WCHAR *glob;
    struct list entry;
};

struct rb_string_entry
{
    WCHAR *string;
    struct wine_rb_entry entry;
};

static WCHAR *xdg_menu_dir;
static WCHAR *xdg_data_dir;
static WCHAR xdg_desktop_dir[MAX_PATH];


/* Utility routines */
static unsigned short crc16(const WCHAR *string)
{
    unsigned short crc = 0;
    int i, j, xor_poly;

    for (i = 0; string[i] != 0; i++)
    {
        WCHAR c = string[i];
        for (j = 0; j < 16; c >>= 1, j++)
        {
            xor_poly = (c ^ crc) & 1;
            crc >>= 1;
            if (xor_poly)
                crc ^= 0xa001;
        }
    }
    return crc;
}

static void *xmalloc( size_t size )
{
    void *ret = malloc( size );
    if (!ret)
    {
        ERR( "out of memory\n" );
        ExitProcess(1);
    }
    return ret;
}

static void *xrealloc( void *ptr, size_t size )
{
    if (!ptr) return xmalloc( size );
    ptr = realloc( ptr, size );
    if (!ptr)
    {
        ERR( "out of memory\n" );
        ExitProcess(1);
    }
    return ptr;
}

static WCHAR *xwcsdup( const WCHAR *str )
{
    WCHAR *ret;

    if (!str) return NULL;
    ret = xmalloc( (lstrlenW(str) + 1) * sizeof(WCHAR) );
    lstrcpyW( ret, str );
    return ret;
}

static void heap_free( void *ptr )
{
    HeapFree( GetProcessHeap(), 0, ptr );
}

static WCHAR * WINAPIV heap_wprintf(const WCHAR *format, ...)
{
    va_list args;
    int size = 4096;
    WCHAR *buffer;
    int n;

    while (1)
    {
        buffer = xmalloc(size * sizeof(WCHAR));
        va_start(args, format);
        n = _vsnwprintf(buffer, size, format, args);
        va_end(args);
        if (n == -1)
            size *= 2;
        else if (n >= size)
            size = n + 1;
        else
            return buffer;
        free(buffer);
    }
}

static int winemenubuilder_rb_string_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct rb_string_entry *t = WINE_RB_ENTRY_VALUE(entry, const struct rb_string_entry, entry);

    return wcscmp((WCHAR *)key, t->string);
}

static void winemenubuilder_rb_destroy(struct wine_rb_entry *entry, void *context)
{
    struct rb_string_entry *t = WINE_RB_ENTRY_VALUE(entry, struct rb_string_entry, entry);
    free(t->string);
    free(t);
}

static BOOL create_directories(WCHAR *directory)
{
    WCHAR *p = PathSkipRootW( directory );

    for ( ; p && *p; p++)
    {
        if (*p == '\\')
        {
            *p = 0;
            CreateDirectoryW( directory, NULL );
            *p = '\\';
        }
    }
    return CreateDirectoryW( directory, NULL ) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static char* wchars_to_utf8_chars(LPCWSTR string)
{
    char *ret;
    INT size = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL);
    ret = xmalloc(size);
    WideCharToMultiByte(CP_UTF8, 0, string, -1, ret, size, NULL, NULL);
    return ret;
}

static WCHAR* utf8_chars_to_wchars(LPCSTR string)
{
    WCHAR *ret;
    INT size = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
    ret = xmalloc(size * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, string, -1, ret, size);
    return ret;
}

static char *wchars_to_xml_text(const WCHAR *string)
{
    int i, pos;
    char *text = wchars_to_utf8_chars( string );
    char *ret = xmalloc( 6 * strlen(text) + 1 );

    for (i = pos = 0; text[i]; i++)
    {
        if (text[i] == '&')
            pos += sprintf(ret + pos, "&amp;");
        else if (text[i] == '<')
            pos += sprintf(ret + pos, "&lt;");
        else if (text[i] == '>')
            pos += sprintf(ret + pos, "&gt;");
        else if (text[i] == '\'')
            pos += sprintf(ret + pos, "&apos;");
        else if (text[i] == '"')
            pos += sprintf(ret + pos, "&quot;");
        else
            ret[pos++] = text[i];
    }
    free(text);
    ret[pos] = 0;
    return ret;
}

/* Icon extraction routines
 *
 * FIXME: should use PrivateExtractIcons and friends
 * FIXME: should not use stdio
 */

static HRESULT convert_to_native_icon(IStream *icoFile, int *indices, int numIndices,
                                      const CLSID *outputFormat, const WCHAR *outputFileName)
{
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IStream *outputFile = NULL;
    int i;
    HRESULT hr = E_FAIL;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX creating IWICImagingFactory\n", hr);
        goto end;
    }
    hr = IWICImagingFactory_CreateDecoderFromStream(factory, icoFile, NULL,
        WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX creating IWICBitmapDecoder\n", hr);
        goto end;
    }
    hr = CoCreateInstance(outputFormat, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void**)&encoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX creating bitmap encoder\n", hr);
        goto end;
    }
    hr = SHCreateStreamOnFileW(outputFileName, STGM_CREATE | STGM_WRITE, &outputFile);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX creating output file %s\n", hr, wine_dbgstr_w(outputFileName));
        goto end;
    }
    hr = IWICBitmapEncoder_Initialize(encoder, outputFile, WICBitmapEncoderNoCache);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX initializing encoder\n", hr);
        goto end;
    }

    for (i = 0; i < numIndices; i++)
    {
        IWICBitmapFrameDecode *sourceFrame = NULL;
        IWICBitmapSource *sourceBitmap = NULL;
        IWICBitmapFrameEncode *dstFrame = NULL;
        IPropertyBag2 *options = NULL;
        UINT width, height;

        hr = IWICBitmapDecoder_GetFrame(decoder, indices[i], &sourceFrame);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX getting frame %d\n", hr, indices[i]);
            goto endloop;
        }
        hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA, (IWICBitmapSource*)sourceFrame, &sourceBitmap);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX converting bitmap to 32bppBGRA\n", hr);
            goto endloop;
        }
        hr = IWICBitmapEncoder_CreateNewFrame(encoder, &dstFrame, &options);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX creating encoder frame\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_Initialize(dstFrame, options);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX initializing encoder frame\n", hr);
            goto endloop;
        }
        hr = IWICBitmapSource_GetSize(sourceBitmap, &width, &height);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX getting source bitmap size\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_SetSize(dstFrame, width, height);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX setting destination bitmap size\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_SetResolution(dstFrame, 96, 96);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX setting destination bitmap resolution\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_WriteSource(dstFrame, sourceBitmap, NULL);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX copying bitmaps\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_Commit(dstFrame);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08lX committing frame\n", hr);
            goto endloop;
        }
    endloop:
        if (sourceFrame)
            IWICBitmapFrameDecode_Release(sourceFrame);
        if (sourceBitmap)
            IWICBitmapSource_Release(sourceBitmap);
        if (dstFrame)
            IWICBitmapFrameEncode_Release(dstFrame);
        if (options)
            IPropertyBag2_Release(options);
    }

    hr = IWICBitmapEncoder_Commit(encoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08lX committing encoder\n", hr);
        goto end;
    }

end:
    if (factory)
        IWICImagingFactory_Release(factory);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (encoder)
        IWICBitmapEncoder_Release(encoder);
    if (outputFile)
        IStream_Release(outputFile);
    return hr;
}

struct IconData16 {
    BYTE *fileBytes;
    DWORD fileSize;
    NE_TYPEINFO *iconResources;
    WORD alignmentShiftCount;
};

static int populate_module16_icons(struct IconData16 *iconData16, GRPICONDIR *grpIconDir, ICONDIRENTRY *iconDirEntries, BYTE *icons, SIZE_T *iconOffset)
{
    int i, j;
    int validEntries = 0;

    for (i = 0; i < grpIconDir->idCount; i++)
    {
        BYTE *iconPtr = (BYTE*)iconData16->iconResources;
        NE_NAMEINFO *matchingIcon = NULL;
        iconPtr += sizeof(NE_TYPEINFO);
        for (j = 0; j < iconData16->iconResources->count; j++)
        {
            NE_NAMEINFO *iconInfo = (NE_NAMEINFO*)iconPtr;
            if ((iconPtr + sizeof(NE_NAMEINFO)) > (iconData16->fileBytes + iconData16->fileSize))
            {
                WINE_WARN("file too small for icon NE_NAMEINFO\n");
                break;
            }
            if (iconInfo->id == (0x8000 | grpIconDir->idEntries[i].nID))
            {
                matchingIcon = iconInfo;
                break;
            }
            iconPtr += sizeof(NE_NAMEINFO);
        }

        if (matchingIcon == NULL)
            continue;
        if (((matchingIcon->offset << iconData16->alignmentShiftCount) + grpIconDir->idEntries[i].dwBytesInRes) > iconData16->fileSize)
        {
            WINE_WARN("file too small for icon contents\n");
            break;
        }

        iconDirEntries[validEntries].bWidth = grpIconDir->idEntries[i].bWidth;
        iconDirEntries[validEntries].bHeight = grpIconDir->idEntries[i].bHeight;
        iconDirEntries[validEntries].bColorCount = grpIconDir->idEntries[i].bColorCount;
        iconDirEntries[validEntries].bReserved = grpIconDir->idEntries[i].bReserved;
        iconDirEntries[validEntries].wPlanes = grpIconDir->idEntries[i].wPlanes;
        iconDirEntries[validEntries].wBitCount = grpIconDir->idEntries[i].wBitCount;
        iconDirEntries[validEntries].dwBytesInRes = grpIconDir->idEntries[i].dwBytesInRes;
        iconDirEntries[validEntries].dwImageOffset = *iconOffset;
        validEntries++;
        memcpy(&icons[*iconOffset], &iconData16->fileBytes[matchingIcon->offset << iconData16->alignmentShiftCount], grpIconDir->idEntries[i].dwBytesInRes);
        *iconOffset += grpIconDir->idEntries[i].dwBytesInRes;
    }
    return validEntries;
}

static int populate_module_icons(HMODULE hModule, GRPICONDIR *grpIconDir, ICONDIRENTRY *iconDirEntries, BYTE *icons, SIZE_T *iconOffset)
{
    int i;
    int validEntries = 0;

    for (i = 0; i < grpIconDir->idCount; i++)
    {
        HRSRC hResInfo;
        LPCWSTR lpName = MAKEINTRESOURCEW(grpIconDir->idEntries[i].nID);
        if ((hResInfo = FindResourceW(hModule, lpName, (LPCWSTR)RT_ICON)))
        {
            HGLOBAL hResData;
            if ((hResData = LoadResource(hModule, hResInfo)))
            {
                BITMAPINFO *pIcon;
                DWORD size = min( grpIconDir->idEntries[i].dwBytesInRes, ((IMAGE_RESOURCE_DATA_ENTRY *)hResInfo)->Size );
                if ((pIcon = LockResource(hResData)))
                {
                    iconDirEntries[validEntries].bWidth = grpIconDir->idEntries[i].bWidth;
                    iconDirEntries[validEntries].bHeight = grpIconDir->idEntries[i].bHeight;
                    iconDirEntries[validEntries].bColorCount = grpIconDir->idEntries[i].bColorCount;
                    iconDirEntries[validEntries].bReserved = grpIconDir->idEntries[i].bReserved;
                    iconDirEntries[validEntries].wPlanes = grpIconDir->idEntries[i].wPlanes;
                    iconDirEntries[validEntries].wBitCount = grpIconDir->idEntries[i].wBitCount;
                    iconDirEntries[validEntries].dwBytesInRes = size;
                    iconDirEntries[validEntries].dwImageOffset = *iconOffset;
                    validEntries++;
                    memcpy(&icons[*iconOffset], pIcon, size);
                    *iconOffset += size;
                }
                FreeResource(hResData);
            }
        }
    }
    return validEntries;
}

static IStream *add_module_icons_to_stream(struct IconData16 *iconData16, HMODULE hModule, GRPICONDIR *grpIconDir)
{
    int i;
    SIZE_T iconsSize = 0;
    BYTE *icons = NULL;
    ICONDIRENTRY *iconDirEntries = NULL;
    IStream *stream = NULL;
    HRESULT hr = E_FAIL;
    ULONG bytesWritten;
    ICONDIR iconDir;
    SIZE_T iconOffset;
    int validEntries = 0;
    LARGE_INTEGER zero;

    for (i = 0; i < grpIconDir->idCount; i++)
        iconsSize += grpIconDir->idEntries[i].dwBytesInRes;
    icons = xmalloc(iconsSize);
    iconDirEntries = xmalloc(grpIconDir->idCount*sizeof(ICONDIRENTRY));
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr))
    {
        WINE_ERR("error creating icon stream\n");
        goto end;
    }

    iconOffset = 0;
    if (iconData16)
        validEntries = populate_module16_icons(iconData16, grpIconDir, iconDirEntries, icons, &iconOffset);
    else if (hModule)
        validEntries = populate_module_icons(hModule, grpIconDir, iconDirEntries, icons, &iconOffset);

    if (validEntries == 0)
    {
        WINE_ERR("no valid icon entries\n");
        goto end;
    }

    iconDir.idReserved = 0;
    iconDir.idType = 1;
    iconDir.idCount = validEntries;
    hr = IStream_Write(stream, &iconDir, sizeof(iconDir), &bytesWritten);
    if (FAILED(hr) || bytesWritten != sizeof(iconDir))
    {
        WINE_ERR("error 0x%08lX writing icon stream\n", hr);
        goto end;
    }
    for (i = 0; i < validEntries; i++)
        iconDirEntries[i].dwImageOffset += sizeof(ICONDIR) + validEntries*sizeof(ICONDIRENTRY);
    hr = IStream_Write(stream, iconDirEntries, validEntries*sizeof(ICONDIRENTRY), &bytesWritten);
    if (FAILED(hr) || bytesWritten != validEntries*sizeof(ICONDIRENTRY))
    {
        WINE_ERR("error 0x%08lX writing icon dir entries to stream\n", hr);
        goto end;
    }
    hr = IStream_Write(stream, icons, iconOffset, &bytesWritten);
    if (FAILED(hr) || bytesWritten != iconOffset)
    {
        WINE_ERR("error 0x%08lX writing icon images to stream\n", hr);
        goto end;
    }
    zero.QuadPart = 0;
    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

end:
    free(icons);
    free(iconDirEntries);
    if (FAILED(hr) && stream != NULL)
    {
        IStream_Release(stream);
        stream = NULL;
    }
    return stream;
}

static HRESULT open_module16_icon(LPCWSTR szFileName, int nIndex, IStream **ppStream)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = NULL;
    DWORD fileSize;
    BYTE *fileBytes = NULL;
    IMAGE_DOS_HEADER *dosHeader;
    IMAGE_OS2_HEADER *neHeader;
    BYTE *rsrcTab;
    NE_TYPEINFO *iconGroupResources;
    NE_TYPEINFO *iconResources;
    NE_NAMEINFO *iconDirPtr;
    GRPICONDIR *iconDir;
    WORD alignmentShiftCount;
    struct IconData16 iconData16;
    HRESULT hr = E_FAIL;

    hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        WINE_WARN("opening %s failed with error %ld\n", wine_dbgstr_w(szFileName), GetLastError());
        goto end;
    }

    hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    if (hFileMapping == NULL)
    {
        WINE_WARN("CreateFileMapping failed, error %ld\n", GetLastError());
        goto end;
    }

    fileSize = GetFileSize(hFile, NULL);

    fileBytes = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (fileBytes == NULL)
    {
        WINE_WARN("MapViewOfFile failed, error %ld\n", GetLastError());
        goto end;
    }

    dosHeader = (IMAGE_DOS_HEADER*)fileBytes;
    if (sizeof(IMAGE_DOS_HEADER) >= fileSize || dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        WINE_WARN("file too small for MZ header\n");
        goto end;
    }

    neHeader = (IMAGE_OS2_HEADER*)(fileBytes + dosHeader->e_lfanew);
    if ((((BYTE*)neHeader) + sizeof(IMAGE_OS2_HEADER)) > (fileBytes + fileSize) ||
        neHeader->ne_magic != IMAGE_OS2_SIGNATURE)
    {
        WINE_WARN("file too small for NE header\n");
        goto end;
    }

    rsrcTab = ((BYTE*)neHeader) + neHeader->ne_rsrctab;
    if ((rsrcTab + 2) > (fileBytes + fileSize))
    {
        WINE_WARN("file too small for resource table\n");
        goto end;
    }

    alignmentShiftCount = *(WORD*)rsrcTab;
    rsrcTab += 2;
    iconGroupResources = NULL;
    iconResources = NULL;
    for (;;)
    {
        NE_TYPEINFO *neTypeInfo = (NE_TYPEINFO*)rsrcTab;
        if ((rsrcTab + sizeof(NE_TYPEINFO)) > (fileBytes + fileSize))
        {
            WINE_WARN("file too small for resource table\n");
            goto end;
        }
        if (neTypeInfo->type_id == 0)
            break;
        else if (neTypeInfo->type_id == NE_RSCTYPE_GROUP_ICON)
            iconGroupResources = neTypeInfo;
        else if (neTypeInfo->type_id == NE_RSCTYPE_ICON)
            iconResources = neTypeInfo;
        rsrcTab += sizeof(NE_TYPEINFO) + neTypeInfo->count*sizeof(NE_NAMEINFO);
    }
    if (iconGroupResources == NULL)
    {
        WINE_WARN("no group icon resource type found\n");
        goto end;
    }
    if (iconResources == NULL)
    {
        WINE_WARN("no icon resource type found\n");
        goto end;
    }

    if (nIndex >= iconGroupResources->count)
    {
        WINE_WARN("icon index out of range\n");
        goto end;
    }

    iconDirPtr = (NE_NAMEINFO*)(((BYTE*)iconGroupResources) + sizeof(NE_TYPEINFO) + nIndex*sizeof(NE_NAMEINFO));
    if ((((BYTE*)iconDirPtr) + sizeof(NE_NAMEINFO)) > (fileBytes + fileSize))
    {
        WINE_WARN("file too small for icon group NE_NAMEINFO\n");
        goto end;
    }
    iconDir = (GRPICONDIR*)(fileBytes + (iconDirPtr->offset << alignmentShiftCount));
    if ((((BYTE*)iconDir) + sizeof(GRPICONDIR) + iconDir->idCount*sizeof(GRPICONDIRENTRY)) > (fileBytes + fileSize))
    {
        WINE_WARN("file too small for GRPICONDIR\n");
        goto end;
    }

    iconData16.fileBytes = fileBytes;
    iconData16.fileSize = fileSize;
    iconData16.iconResources = iconResources;
    iconData16.alignmentShiftCount = alignmentShiftCount;
    *ppStream = add_module_icons_to_stream(&iconData16, NULL, iconDir);
    if (*ppStream)
        hr = S_OK;

end:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hFileMapping != NULL)
        CloseHandle(hFileMapping);
    if (fileBytes != NULL)
        UnmapViewOfFile(fileBytes);
    return hr;
}

static BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    ENUMRESSTRUCT *sEnumRes = (ENUMRESSTRUCT *) lParam;

    if (!sEnumRes->nIndex--)
    {
        *sEnumRes->pResInfo = FindResourceW(hModule, lpszName, (LPCWSTR)RT_GROUP_ICON);
        return FALSE;
    }
    else
        return TRUE;
}

static HRESULT open_module_icon(LPCWSTR szFileName, int nIndex, IStream **ppStream)
{
    HMODULE hModule;
    HRSRC hResInfo;
    HGLOBAL hResData;
    GRPICONDIR *pIconDir;
    ENUMRESSTRUCT sEnumRes;
    HRESULT hr = E_FAIL;
    WCHAR fullPathW[MAX_PATH];
    DWORD len;

    len = SearchPathW(NULL, szFileName, L".exe", MAX_PATH, fullPathW, NULL);
    if (len == 0 || len > MAX_PATH)
    {
        WINE_WARN("SearchPath failed\n");
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    hModule = LoadLibraryExW(fullPathW, 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        if (GetLastError() == ERROR_BAD_EXE_FORMAT)
            return open_module16_icon(fullPathW, nIndex, ppStream);
        else
        {
            WINE_WARN("LoadLibraryExW (%s) failed, error %ld\n",
                     wine_dbgstr_w(fullPathW), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (nIndex < 0)
    {
        hResInfo = FindResourceW(hModule, MAKEINTRESOURCEW(-nIndex), (LPCWSTR)RT_GROUP_ICON);
        WINE_TRACE("FindResourceW (%s) called, return %p, error %ld\n",
                   wine_dbgstr_w(fullPathW), hResInfo, GetLastError());
    }
    else
    {
        hResInfo=NULL;
        sEnumRes.pResInfo = &hResInfo;
        sEnumRes.nIndex = nIndex;
        if (!EnumResourceNamesW(hModule, (LPCWSTR)RT_GROUP_ICON,
                                EnumResNameProc, (LONG_PTR)&sEnumRes) &&
            sEnumRes.nIndex != -1)
        {
            WINE_TRACE("EnumResourceNamesW failed, error %ld\n", GetLastError());
        }
    }

    if (hResInfo)
    {
        if ((hResData = LoadResource(hModule, hResInfo)))
        {
            if ((pIconDir = LockResource(hResData)))
            {
                *ppStream = add_module_icons_to_stream(0, hModule, pIconDir);
                if (*ppStream)
                    hr = S_OK;
            }

            FreeResource(hResData);
        }
    }
    else
    {
        WINE_WARN("found no icon\n");
        FreeLibrary(hModule);
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    FreeLibrary(hModule);
    return hr;
}

static HRESULT read_ico_direntries(IStream *icoStream, ICONDIRENTRY **ppIconDirEntries, int *numEntries)
{
    ICONDIR iconDir;
    ULONG bytesRead;
    HRESULT hr;

    *ppIconDirEntries = NULL;

    hr = IStream_Read(icoStream, &iconDir, sizeof(ICONDIR), &bytesRead);
    if (FAILED(hr) || bytesRead != sizeof(ICONDIR) ||
        (iconDir.idReserved != 0) || (iconDir.idType != 1))
    {
        WINE_WARN("Invalid ico file format (hr=0x%08lX, bytesRead=%ld)\n", hr, bytesRead);
        hr = E_FAIL;
        goto end;
    }
    *numEntries = iconDir.idCount;

    *ppIconDirEntries = xmalloc(sizeof(ICONDIRENTRY)*iconDir.idCount);
    hr = IStream_Read(icoStream, *ppIconDirEntries, sizeof(ICONDIRENTRY)*iconDir.idCount, &bytesRead);
    if (FAILED(hr) || bytesRead != sizeof(ICONDIRENTRY)*iconDir.idCount)
    {
        if (SUCCEEDED(hr)) hr = E_FAIL;
        goto end;
    }

end:
    if (FAILED(hr))
        free(*ppIconDirEntries);
    return hr;
}

static HRESULT validate_ico(IStream **ppStream, ICONDIRENTRY **ppIconDirEntries, int *numEntries)
{
    HRESULT hr;

    hr = read_ico_direntries(*ppStream, ppIconDirEntries, numEntries);
    if (SUCCEEDED(hr))
    {
        if (*numEntries)
            return hr;
        free(*ppIconDirEntries);
        *ppIconDirEntries = NULL;
    }
    IStream_Release(*ppStream);
    *ppStream = NULL;
    return E_FAIL;
}

static HRESULT write_native_icon(IStream *iconStream, ICONDIRENTRY *pIconDirEntry,
                                 int numEntries, const WCHAR *icon_name)
{
    int nMax = 0, nMaxBits = 0;
    int nIndex = 0;
    int i;
    LARGE_INTEGER position;
    HRESULT hr;

    for (i = 0; i < numEntries; i++)
    {
        WINE_TRACE("[%d]: %d x %d @ %d\n", i, pIconDirEntry[i].bWidth, pIconDirEntry[i].bHeight, pIconDirEntry[i].wBitCount);
        if (pIconDirEntry[i].wBitCount >= nMaxBits &&
            (pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth) >= nMax)
        {
            nIndex = i;
            nMax = pIconDirEntry[i].bHeight * pIconDirEntry[i].bWidth;
            nMaxBits = pIconDirEntry[i].wBitCount;
        }
    }
    WINE_TRACE("Selected: %d\n", nIndex);

    position.QuadPart = 0;
    hr = IStream_Seek(iconStream, position, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;
    return convert_to_native_icon(iconStream, &nIndex, 1, &CLSID_WICPngEncoder, icon_name);
}

static WCHAR* assoc_query(ASSOCSTR assocStr, LPCWSTR name, LPCWSTR extra)
{
    HRESULT hr;
    WCHAR *value = NULL;
    DWORD size = 0;
    hr = AssocQueryStringW(0, assocStr, name, extra, NULL, &size);
    if (SUCCEEDED(hr))
    {
        value = xmalloc(size * sizeof(WCHAR));
        hr = AssocQueryStringW(0, assocStr, name, extra, value, &size);
        if (FAILED(hr))
        {
            free(value);
            value = NULL;
        }
    }
    return value;
}

static HRESULT open_file_type_icon(LPCWSTR szFileName, IStream **ppStream)
{
    WCHAR *extension;
    WCHAR *icon = NULL;
    WCHAR *comma;
    WCHAR *executable = NULL;
    int index = 0;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    extension = wcsrchr(szFileName, '.');
    if (extension == NULL)
        goto end;

    icon = assoc_query(ASSOCSTR_DEFAULTICON, extension, NULL);
    if (icon)
    {
        comma = wcsrchr(icon, ',');
        if (comma)
        {
            *comma = 0;
            index = wcstol(comma + 1, NULL, 10);
        }
        hr = open_module_icon(icon, index, ppStream);
    }
    else
    {
        executable = assoc_query(ASSOCSTR_EXECUTABLE, extension, L"open");
        if (executable)
            hr = open_module_icon(executable, 0, ppStream);
    }

end:
    free(icon);
    free(executable);
    return hr;
}

static HRESULT open_default_icon(IStream **ppStream)
{
    return open_module_icon(L"user32", -(INT_PTR)IDI_WINLOGO, ppStream);
}

static HRESULT open_icon(LPCWSTR filename, int index, BOOL bWait, IStream **ppStream, ICONDIRENTRY **ppIconDirEntries, int *numEntries)
{
    HRESULT hr;

    hr = open_module_icon(filename, index, ppStream);
    if (FAILED(hr))
    {
        if(bWait && hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND))
        {
            WINE_WARN("Can't find file: %s, give a chance to parent process to create it\n",
                                    wine_dbgstr_w(filename));
            return hr;
        }
        else
        {
            /* This might be a raw .ico file */
            hr = SHCreateStreamOnFileW(filename, STGM_READ, ppStream);
        }
    }
    if (SUCCEEDED(hr))
        hr = validate_ico(ppStream, ppIconDirEntries, numEntries);

    if (FAILED(hr))
    {
        hr = open_file_type_icon(filename, ppStream);
        if (SUCCEEDED(hr))
            hr = validate_ico(ppStream, ppIconDirEntries, numEntries);
    }
    if (FAILED(hr) && !bWait)
    {
        hr = open_default_icon(ppStream);
        if (SUCCEEDED(hr))
            hr = validate_ico(ppStream, ppIconDirEntries, numEntries);
    }
    return hr;
}

static WCHAR *compute_native_identifier(int exeIndex, LPCWSTR icoPathW, LPCWSTR filename)
{
    unsigned short crc;
    const WCHAR *basename, *ext;

    if (filename) return xwcsdup( filename );

    crc = crc16(icoPathW);
    basename = wcsrchr(icoPathW, '\\');
    if (basename == NULL) basename = icoPathW;
    else basename++;
    ext = wcsrchr(basename, '.');
    if (!ext) ext = basename + lstrlenW(basename);

    return heap_wprintf(L"%04X_%.*s.%d", crc, (int)(ext - basename), basename, exeIndex);
}

static void refresh_icon_cache(const WCHAR *iconsDir)
{
    WCHAR buffer[MAX_PATH];

    /* The icon theme spec only requires the mtime on the "toplevel"
     * directory (whatever that is) to be changed for a refresh,
     * but on GNOME you have to create a file in that directory
     * instead. Creating a file also works on KDE, Xfce and LXDE.
     */
    GetTempFileNameW( iconsDir, L"icn", 0, buffer );
    DeleteFileW( buffer );
}

static HRESULT platform_write_icon(IStream *icoStream, ICONDIRENTRY *iconDirEntries,
                                   int numEntries, int exeIndex, LPCWSTR icoPathW,
                                   const WCHAR *destFilename, WCHAR **nativeIdentifier)
{
    int i;
    WCHAR *iconsDir;
    HRESULT hr = S_OK;
    LARGE_INTEGER zero;

    *nativeIdentifier = compute_native_identifier(exeIndex, icoPathW, destFilename);
    iconsDir = heap_wprintf(L"%s\\icons\\hicolor", xdg_data_dir);

    for (i = 0; i < numEntries; i++)
    {
        int bestIndex = i;
        int j;
        BOOLEAN duplicate = FALSE;
        int w, h;
        WCHAR *iconDir;
        WCHAR *pngPath;

        WINE_TRACE("[%d]: %d x %d @ %d\n", i, iconDirEntries[i].bWidth,
            iconDirEntries[i].bHeight, iconDirEntries[i].wBitCount);

        for (j = 0; j < i; j++)
        {
            if (iconDirEntries[j].bWidth == iconDirEntries[i].bWidth &&
                iconDirEntries[j].bHeight == iconDirEntries[i].bHeight)
            {
                duplicate = TRUE;
                break;
            }
        }
        if (duplicate)
            continue;
        for (j = i + 1; j < numEntries; j++)
        {
            if (iconDirEntries[j].bWidth == iconDirEntries[i].bWidth &&
                iconDirEntries[j].bHeight == iconDirEntries[i].bHeight &&
                iconDirEntries[j].wBitCount >= iconDirEntries[bestIndex].wBitCount)
            {
                bestIndex = j;
            }
        }
        WINE_TRACE("Selected: %d\n", bestIndex);

        w = iconDirEntries[bestIndex].bWidth ? iconDirEntries[bestIndex].bWidth : 256;
        h = iconDirEntries[bestIndex].bHeight ? iconDirEntries[bestIndex].bHeight : 256;
        iconDir = heap_wprintf(L"%s\\%dx%d\\apps", iconsDir, w, h);
        create_directories(iconDir);
        pngPath = heap_wprintf(L"%s\\%s.png", iconDir, *nativeIdentifier);
        zero.QuadPart = 0;
        hr = IStream_Seek(icoStream, zero, STREAM_SEEK_SET, NULL);
        if (SUCCEEDED(hr))
            hr = convert_to_native_icon(icoStream, &bestIndex, 1, &CLSID_WICPngEncoder, pngPath);

        free(iconDir);
        free(pngPath);
    }
    refresh_icon_cache(iconsDir);
    free(iconsDir);
    return hr;
}

/* extract an icon from an exe or icon file; helper for IPersistFile_fnSave */
static WCHAR *extract_icon(LPCWSTR icoPathW, int index, const WCHAR *destFilename, BOOL bWait)
{
    IStream *stream = NULL;
    ICONDIRENTRY *pIconDirEntries = NULL;
    int numEntries;
    HRESULT hr;
    WCHAR *nativeIdentifier = NULL;
    WCHAR fullPathW[MAX_PATH];
    DWORD len;

    WINE_TRACE("path=[%s] index=%d destFilename=[%s]\n", wine_dbgstr_w(icoPathW), index, wine_dbgstr_w(destFilename));

    len = GetFullPathNameW(icoPathW, MAX_PATH, fullPathW, NULL);
    if (len == 0 || len > MAX_PATH)
    {
        WINE_WARN("GetFullPathName failed\n");
        return NULL;
    }

    hr = open_icon(fullPathW, index, bWait, &stream, &pIconDirEntries, &numEntries);
    if (FAILED(hr))
    {
        WINE_WARN("opening icon %s index %d failed, hr=0x%08lX\n", wine_dbgstr_w(fullPathW), index, hr);
        goto end;
    }
    hr = platform_write_icon(stream, pIconDirEntries, numEntries, index, fullPathW, destFilename, &nativeIdentifier);
    if (FAILED(hr))
        WINE_WARN("writing icon failed, error 0x%08lX\n", hr);

end:
    if (stream)
        IStream_Release(stream);
    free(pIconDirEntries);
    if (FAILED(hr))
    {
        free(nativeIdentifier);
        nativeIdentifier = NULL;
    }
    return nativeIdentifier;
}

static HKEY open_menus_reg_key(void)
{
    HKEY assocKey;
    DWORD ret;
    ret = RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Wine\\MenuFiles", &assocKey);
    if (ret == ERROR_SUCCESS)
        return assocKey;
    SetLastError(ret);
    return NULL;
}

static DWORD register_menus_entry(const WCHAR *menu_file, const WCHAR *windows_file)
{
    HKEY hkey;
    DWORD ret;

    hkey = open_menus_reg_key();
    if (hkey)
    {
        ret = RegSetValueExW(hkey, menu_file, 0, REG_SZ, (const BYTE*)windows_file,
                    (lstrlenW(windows_file) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
    }
    else
        ret = GetLastError();
    return ret;
}

/* This escapes reserved characters in .desktop files' Exec keys. */
static LPSTR escape(LPCWSTR arg)
{
    int i, j;
    WCHAR *escaped_string;
    char *utf8_string;

    escaped_string = xmalloc((4 * lstrlenW(arg) + 1) * sizeof(WCHAR));
    for (i = j = 0; arg[i]; i++)
    {
        switch (arg[i])
        {
        case '\\':
            escaped_string[j++] = '\\';
            escaped_string[j++] = '\\';
            escaped_string[j++] = '\\';
            escaped_string[j++] = '\\';
            break;
        case ' ':
        case '\t':
        case '\n':
        case '"':
        case '\'':
        case '>':
        case '<':
        case '~':
        case '|':
        case '&':
        case ';':
        case '$':
        case '*':
        case '?':
        case '#':
        case '(':
        case ')':
        case '`':
            escaped_string[j++] = '\\';
            escaped_string[j++] = '\\';
            /* fall through */
        default:
            escaped_string[j++] = arg[i];
            break;
        }
    }
    escaped_string[j] = 0;
    utf8_string = wchars_to_utf8_chars(escaped_string);
    free(escaped_string);
    return utf8_string;
}

static BOOL write_desktop_entry(const WCHAR *link, const WCHAR *location, const WCHAR *linkname,
                                const WCHAR *path, const WCHAR *args, const WCHAR *descr,
                                const WCHAR *workdir, const WCHAR *icon, const WCHAR *wmclass)
{
    FILE *file;
    char *workdir_unix;
    int needs_chmod = FALSE;
    const WCHAR *name;
    const WCHAR *prefix = _wgetenv( L"WINECONFIGDIR" );

    WINE_TRACE("(%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", wine_dbgstr_w(link), wine_dbgstr_w(location),
               wine_dbgstr_w(linkname), wine_dbgstr_w(path), wine_dbgstr_w(args),
               wine_dbgstr_w(descr), wine_dbgstr_w(workdir), wine_dbgstr_w(icon),
               wine_dbgstr_w(wmclass));

    name = PathFindFileNameW( linkname );
    if (!location)
    {
        location = heap_wprintf(L"%s\\%s.desktop", xdg_desktop_dir, name);
        needs_chmod = TRUE;
    }

    file = _wfopen( location, L"wb" );
    if (file == NULL)
        return FALSE;

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Name=%s\n", wchars_to_utf8_chars(name));
    fprintf(file, "Exec=" );
    if (prefix)
    {
        char *path = wine_get_unix_file_name( prefix );
        fprintf(file, "env WINEPREFIX=\"%s\" ", path);
        heap_free( path );
    }
    fprintf(file, "wine %s", escape(path));
    if (args) fprintf(file, " %s", escape(args) );
    fputc( '\n', file );
    fprintf(file, "Type=Application\n");
    fprintf(file, "StartupNotify=true\n");
    if (descr && *descr)
        fprintf(file, "Comment=%s\n", wchars_to_utf8_chars(descr));
    if (workdir && *workdir && (workdir_unix = wine_get_unix_file_name(workdir)))
        fprintf(file, "Path=%s\n", workdir_unix);
    if (icon && *icon)
        fprintf(file, "Icon=%s\n", wchars_to_utf8_chars(icon));
    if (wmclass && *wmclass)
        fprintf(file, "StartupWMClass=%s\n", wchars_to_utf8_chars(wmclass));

    fclose(file);

    if (needs_chmod)
    {
        const char *argv[] = { "chmod", "+x", wine_get_unix_file_name(location), NULL };
        __wine_unix_spawnvp( (char **)argv, FALSE );
    }

    if (link)
    {
        DWORD ret = register_menus_entry(location, link);
        if (ret != ERROR_SUCCESS)
            return FALSE;
    }

    return TRUE;
}

static BOOL write_directory_entry(const WCHAR *directory, const WCHAR *location)
{
    FILE *file;

    WINE_TRACE("(%s,%s)\n", wine_dbgstr_w(directory), wine_dbgstr_w(location));

    file = _wfopen( location, L"wb" );
    if (file == NULL)
        return FALSE;

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Type=Directory\n");
    if (wcscmp(directory, L"wine") == 0)
    {
        fprintf(file, "Name=Wine\n");
        fprintf(file, "Icon=wine\n");
    }
    else
    {
        fprintf(file, "Name=%s\n", wchars_to_utf8_chars(directory));
        fprintf(file, "Icon=folder\n");
    }

    fclose(file);
    return TRUE;
}

static BOOL write_menu_file(const WCHAR *windows_link, const WCHAR *link)
{
    WCHAR tempfilename[MAX_PATH];
    FILE *tempfile = NULL;
    WCHAR *filename, *lastEntry, *menuPath;
    int i;
    int count = 0;
    BOOL ret = FALSE;

    WINE_TRACE("(%s)\n", wine_dbgstr_w(link));

    GetTempFileNameW( xdg_menu_dir, L"mnu", 0, tempfilename );
    if (!(tempfile = _wfopen( tempfilename, L"wb" ))) return FALSE;

    fprintf(tempfile, "<!DOCTYPE Menu PUBLIC \"-//freedesktop//DTD Menu 1.0//EN\"\n");
    fprintf(tempfile, "\"http://www.freedesktop.org/standards/menu-spec/menu-1.0.dtd\">\n");
    fprintf(tempfile, "<Menu>\n");
    fprintf(tempfile, "  <Name>Applications</Name>\n");

    filename = heap_wprintf(L"wine\\%s.desktop", link);
    lastEntry = filename;
    for (i = 0; filename[i]; i++)
    {
        if (filename[i] == '\\')
        {
            WCHAR *dir_file_name;
            const char *prefix = count ? "" : "wine-";

            filename[i] = 0;
            fprintf(tempfile, "  <Menu>\n");
            fprintf(tempfile, "    <Name>%s%s</Name>\n",
                    prefix, wchars_to_xml_text(filename));
            fprintf(tempfile, "    <Directory>%s%s.directory</Directory>\n",
                    prefix, wchars_to_xml_text(filename));
            dir_file_name = heap_wprintf(L"%s\\desktop-directories\\%s%s.directory",
                                         xdg_data_dir, count ? L"" : L"wine-", filename);
            if (GetFileAttributesW( dir_file_name ) == INVALID_FILE_ATTRIBUTES)
                write_directory_entry(lastEntry, dir_file_name);
            free(dir_file_name);
            filename[i] = '-';
            lastEntry = &filename[i+1];
            ++count;
        }
    }
    filename[i] = 0;

    fprintf(tempfile, "    <Include>\n");
    fprintf(tempfile, "      <Filename>%s</Filename>\n", wchars_to_xml_text(filename));
    fprintf(tempfile, "    </Include>\n");
    for (i = 0; i < count; i++)
         fprintf(tempfile, "  </Menu>\n");
    fprintf(tempfile, "</Menu>\n");

    menuPath = heap_wprintf(L"%s\\%s", xdg_menu_dir, filename);
    lstrcpyW(menuPath + lstrlenW(menuPath) - lstrlenW(L".desktop"), L".menu");

    fclose(tempfile);
    ret = MoveFileExW( tempfilename, menuPath, MOVEFILE_REPLACE_EXISTING );
    if (ret)
        register_menus_entry(menuPath, windows_link);
    else
        DeleteFileW( tempfilename );
    free(filename);
    free(menuPath);
    return ret;
}

static BOOL write_menu_entry(const WCHAR *windows_link, const WCHAR *link, const WCHAR *path, const WCHAR *args,
                             const WCHAR *descr, const WCHAR *workdir, const WCHAR *icon, const WCHAR *wmclass)
{
    WCHAR *desktopPath;
    WCHAR *desktopDir;
    WCHAR *filename = NULL;
    BOOL ret = TRUE;

    WINE_TRACE("(%s, %s, %s, %s, %s, %s, %s, %s)\n", wine_dbgstr_w(windows_link), wine_dbgstr_w(link),
               wine_dbgstr_w(path), wine_dbgstr_w(args), wine_dbgstr_w(descr),
               wine_dbgstr_w(workdir), wine_dbgstr_w(icon), wine_dbgstr_w(wmclass));

    desktopPath = heap_wprintf(L"%s\\applications\\wine\\%s.desktop", xdg_data_dir, link);
    desktopDir = wcsrchr(desktopPath, '\\');
    *desktopDir = 0;
    if (!create_directories(desktopPath))
    {
        WINE_WARN("couldn't make parent directories for %s\n", wine_dbgstr_w(desktopPath));
        ret = FALSE;
        goto end;
    }
    *desktopDir = '\\';
    if (!write_desktop_entry(windows_link, desktopPath, link, path, args, descr, workdir, icon, wmclass))
    {
        WINE_WARN("couldn't make desktop entry %s\n", wine_dbgstr_w(desktopPath));
        ret = FALSE;
        goto end;
    }

    if (!write_menu_file(windows_link, link))
    {
        WINE_WARN("couldn't make menu file %s\n", wine_dbgstr_w(filename));
        ret = FALSE;
    }

end:
    free(desktopPath);
    free(filename);
    return ret;
}

/***********************************************************************
 *           get_link_location
 *
 * returns TRUE if successful
 * *loc will contain CS_DESKTOPDIRECTORY, CS_STARTMENU, CS_STARTUP etc.
 * *relative will contain the address of a heap-allocated copy of the portion
 * of the filename that is within the specified location, in unix form
 */
static BOOL get_link_location( LPCWSTR linkfile, DWORD *loc, WCHAR **relative )
{
    WCHAR filename[MAX_PATH], shortfilename[MAX_PATH], buffer[MAX_PATH];
    DWORD len, i, filelen;
    const DWORD locations[] = {
        CSIDL_STARTUP, CSIDL_DESKTOPDIRECTORY, CSIDL_STARTMENU,
        CSIDL_COMMON_STARTUP, CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_COMMON_STARTMENU };

    WINE_TRACE("%s\n", wine_dbgstr_w(linkfile));
    filelen=GetFullPathNameW( linkfile, MAX_PATH, shortfilename, NULL );
    if (filelen==0 || filelen>MAX_PATH)
        return FALSE;

    WINE_TRACE("%s\n", wine_dbgstr_w(shortfilename));

    /* the CSLU Toolkit uses a short path name when creating .lnk files;
     * expand or our hardcoded list won't match.
     */
    filelen=GetLongPathNameW(shortfilename, filename, MAX_PATH);
    if (filelen==0 || filelen>MAX_PATH)
        return FALSE;

    WINE_TRACE("%s\n", wine_dbgstr_w(filename));

    for( i=0; i<ARRAY_SIZE( locations ); i++ )
    {
        if (!SHGetSpecialFolderPathW( 0, buffer, locations[i], FALSE ))
            continue;

        len = lstrlenW(buffer);
        if (len >= MAX_PATH)
            continue; /* We've just trashed memory! Hopefully we are OK */

        if (len > filelen || filename[len]!='\\')
            continue;
        if (wcsnicmp( filename, buffer, len )) continue;

        /* return the remainder of the string and link type */
        *loc = locations[i];
        *relative = xwcsdup( filename + len + 1 );
        PathRemoveExtensionW( *relative );
        return TRUE;
    }

    return FALSE;
}

/* gets the target path directly or through MSI */
static HRESULT get_cmdline( IShellLinkW *sl, LPWSTR szPath, DWORD pathSize,
                            LPWSTR szArgs, DWORD argsSize)
{
    IShellLinkDataList *dl = NULL;
    EXP_DARWIN_LINK *dar = NULL;
    HRESULT hr;

    szPath[0] = 0;
    szArgs[0] = 0;

    hr = IShellLinkW_GetPath( sl, szPath, pathSize, NULL, SLGP_RAWPATH );
    if (hr == S_OK && szPath[0])
    {
        IShellLinkW_GetArguments( sl, szArgs, argsSize );
        return hr;
    }

    hr = IShellLinkW_QueryInterface( sl, &IID_IShellLinkDataList, (LPVOID*) &dl );
    if (FAILED(hr))
        return hr;

    hr = IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG, (LPVOID*) &dar );
    if (SUCCEEDED(hr))
    {
        WCHAR* szCmdline;
        DWORD cmdSize;

        cmdSize=0;
        hr = CommandLineFromMsiDescriptor( dar->szwDarwinID, NULL, &cmdSize );
        if (hr == ERROR_SUCCESS)
        {
            cmdSize++;
            szCmdline = xmalloc(cmdSize*sizeof(WCHAR) );
            hr = CommandLineFromMsiDescriptor( dar->szwDarwinID, szCmdline, &cmdSize );
            WINE_TRACE("      command    : %s\n", wine_dbgstr_w(szCmdline));
            if (hr == ERROR_SUCCESS)
            {
                WCHAR *s, *d;
                int bcount = 0;
                BOOL in_quotes = FALSE;

                /* Extract the application path */
                s=szCmdline;
                d=szPath;
                while (*s)
                {
                    if ((*s==0x0009 || *s==0x0020) && !in_quotes)
                    {
                        /* skip the remaining spaces */
                        do {
                            s++;
                        } while (*s==0x0009 || *s==0x0020);
                        break;
                    }
                    else if (*s==0x005c)
                    {
                        /* '\\' */
                        *d++=*s++;
                        bcount++;
                    }
                    else if (*s==0x0022)
                    {
                        /* '"' */
                        if ((bcount & 1)==0)
                        {
                            /* Preceded by an even number of '\', this is
                             * half that number of '\', plus a quote which
                             * we erase.
                             */
                            d-=bcount/2;
                            in_quotes=!in_quotes;
                            s++;
                        }
                        else
                        {
                            /* Preceded by an odd number of '\', this is
                             * half that number of '\' followed by a '"'
                             */
                            d=d-bcount/2-1;
                            *d++='"';
                            s++;
                        }
                        bcount=0;
                    }
                    else
                    {
                        /* a regular character */
                        *d++=*s++;
                        bcount=0;
                    }
                    if ((d-szPath) == pathSize)
                    {
                        /* Keep processing the path till we get to the
                         * arguments, but 'stand still'
                         */
                        d--;
                    }
                }
                /* Close the application path */
                *d=0;

                lstrcpynW(szArgs, s, argsSize);
            }
            free(szCmdline );
        }
        LocalFree( dar );
    }

    IShellLinkDataList_Release( dl );
    return hr;
}

static WCHAR *slashes_to_minuses(const WCHAR *string)
{
    int i;
    WCHAR *ret = xwcsdup(string);

    for (i = 0; ret[i]; i++) if (ret[i] == '/') ret[i] = '-';
    return ret;
}

static BOOL next_line(FILE *file, char **line, int *size)
{
    int pos = 0;
    char *cr;
    if (*line == NULL)
    {
        *size = 4096;
        *line = xmalloc(*size);
    }
    while (*line != NULL)
    {
        if (fgets(&(*line)[pos], *size - pos, file) == NULL)
        {
            free(*line);
            *line = NULL;
            if (feof(file))
                return TRUE;
            return FALSE;
        }
        pos = strlen(*line);
        cr = strchr(*line, '\n');
        if (cr == NULL)
        {
            (*size) *= 2;
            *line = xrealloc(*line, *size);
        }
        else
        {
            *cr = 0;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL add_mimes(const WCHAR *dir, struct list *mime_types)
{
    WCHAR *globs_filename;
    BOOL ret = TRUE;
    FILE *globs_file;

    globs_filename = heap_wprintf(L"%s\\mime\\globs", dir);
    globs_file = _wfopen( globs_filename, L"r" );
    if (globs_file) /* doesn't have to exist */
    {
        char *line = NULL;
        int size = 0;
        while (ret && (ret = next_line(globs_file, &line, &size)) && line)
        {
            char *pos;
            struct xdg_mime_type *mime_type_entry = NULL;
            if (line[0] != '#' && (pos = strchr(line, ':')))
            {
                mime_type_entry = xmalloc(sizeof(struct xdg_mime_type));
                *pos = 0;
                mime_type_entry->mimeType = utf8_chars_to_wchars(line);
                mime_type_entry->glob = utf8_chars_to_wchars(pos + 1);
                list_add_tail(mime_types, &mime_type_entry->entry);
            }
        }
        free(line);
        fclose(globs_file);
    }
    free(globs_filename);
    return ret;
}

static void free_native_mime_types(struct list *native_mime_types)
{
    struct xdg_mime_type *mime_type_entry, *mime_type_entry2;

    LIST_FOR_EACH_ENTRY_SAFE(mime_type_entry, mime_type_entry2, native_mime_types, struct xdg_mime_type, entry)
    {
        list_remove(&mime_type_entry->entry);
        free(mime_type_entry->glob);
        free(mime_type_entry->mimeType);
        free(mime_type_entry);
    }
}

static BOOL build_native_mime_types(struct list *mime_types)
{
    WCHAR *dirs, *dir, *dos_name, *ctx, *p;
    BOOL ret;

    if (_wgetenv( L"XDG_DATA_DIRS" ))
        dirs = xwcsdup( _wgetenv( L"XDG_DATA_DIRS" ));
    else
        dirs = xwcsdup( L"/usr/local/share/:/usr/share/" );

    ret = add_mimes(xdg_data_dir, mime_types);
    if (ret)
    {
        for (dir = wcstok( dirs, L":", &ctx ); dir; dir = wcstok( NULL, L":", &ctx ))
        {
            dos_name = heap_wprintf( L"\\\\?\\unix%s", dir );
            for (p = dos_name; *p; p++) if (*p == '/') *p = '\\';
            if (p > dos_name + 9 && p[-1] == '\\') p[-1] = 0;
            ret = add_mimes(dos_name, mime_types);
            free(dos_name);
            if (!ret)
                break;
        }
    }
    free(dirs);

    if (!ret)
        free_native_mime_types(mime_types);
    return ret;
}

static WCHAR *freedesktop_mime_type_for_extension(struct list *native_mime_types,
                                                  const WCHAR *extensionW)
{
    struct xdg_mime_type *mime_type_entry;
    int matchLength = 0;
    const WCHAR* match = NULL;

    LIST_FOR_EACH_ENTRY(mime_type_entry, native_mime_types, struct xdg_mime_type, entry)
    {
        if (PathMatchSpecW( extensionW, mime_type_entry->glob ))
        {
            if (match == NULL || matchLength < lstrlenW(mime_type_entry->glob))
            {
                match = mime_type_entry->mimeType;
                matchLength = lstrlenW(mime_type_entry->glob);
            }
        }
    }

    return match ? xwcsdup(match) : NULL;
}

static WCHAR *reg_enum_keyW(HKEY key, DWORD index)
{
    WCHAR *subkey;
    DWORD size = 1024 * sizeof(WCHAR);
    LSTATUS ret;

    for (;;)
    {
        subkey = xmalloc(size);
        ret = RegEnumKeyExW(key, index, subkey, &size, NULL, NULL, NULL, NULL);
        if (ret == ERROR_SUCCESS)
        {
            return subkey;
        }
        if (ret != ERROR_MORE_DATA)
        {
            free(subkey);
            return NULL;
        }
        size *= 2;
        free(subkey);
    }
}

static WCHAR* reg_get_valW(HKEY key, LPCWSTR subkey, LPCWSTR name)
{
    DWORD size;
    if (RegGetValueW(key, subkey, name, RRF_RT_REG_SZ, NULL, NULL, &size) == ERROR_SUCCESS)
    {
        WCHAR *ret = xmalloc(size);
        if (RegGetValueW(key, subkey, name, RRF_RT_REG_SZ, NULL, ret, &size) == ERROR_SUCCESS)
            return ret;
        free(ret);
    }
    return NULL;
}

static HKEY open_associations_reg_key(void)
{
    HKEY assocKey;
    if (RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Wine\\FileOpenAssociations", &assocKey) == ERROR_SUCCESS)
        return assocKey;
    return NULL;
}

static BOOL has_association_changed(LPCWSTR extensionW, const WCHAR *mimeType, const WCHAR *progId,
                                    const WCHAR *appName, const WCHAR *openWithIcon)
{
    HKEY assocKey;
    BOOL ret;

    if ((assocKey = open_associations_reg_key()))
    {
        WCHAR *value;

        ret = FALSE;

        value = reg_get_valW(assocKey, extensionW, L"MimeType");
        if (!value || wcscmp(value, mimeType))
            ret = TRUE;
        free(value);

        if (progId)
        {
            value = reg_get_valW(assocKey, extensionW, L"ProgID");
            if (!value || wcscmp(value, progId))
                ret = TRUE;
            free(value);
        }

        value = reg_get_valW(assocKey, extensionW, L"AppName");
        if (!value || wcscmp(value, appName))
            ret = TRUE;
        free(value);

        value = reg_get_valW(assocKey, extensionW, L"OpenWithIcon");
        if ((openWithIcon && !value) ||
            (!openWithIcon && value) ||
            (openWithIcon && value && wcscmp(value, openWithIcon)))
            ret = TRUE;
        free(value);

        RegCloseKey(assocKey);
    }
    else
    {
        WINE_ERR("error opening associations registry key\n");
        ret = FALSE;
    }
    return ret;
}

static void update_association(LPCWSTR extension, const WCHAR *mimeType, const WCHAR *progId,
                               const WCHAR *appName, const WCHAR *desktopFile, const WCHAR *openWithIcon)
{
    HKEY assocKey = NULL;
    HKEY subkey = NULL;

    assocKey = open_associations_reg_key();
    if (assocKey == NULL)
    {
        WINE_ERR("could not open file associations key\n");
        goto done;
    }

    if (RegCreateKeyW(assocKey, extension, &subkey) != ERROR_SUCCESS)
    {
        WINE_ERR("could not create extension subkey\n");
        goto done;
    }

    RegSetValueExW(subkey, L"MimeType", 0, REG_SZ, (const BYTE*) mimeType, (lstrlenW(mimeType) + 1) * sizeof(WCHAR));
    if (progId) RegSetValueExW(subkey, L"ProgID", 0, REG_SZ, (const BYTE*) progId, (lstrlenW(progId) + 1) * sizeof(WCHAR));
    RegSetValueExW(subkey, L"AppName", 0, REG_SZ, (const BYTE*) appName, (lstrlenW(appName) + 1) * sizeof(WCHAR));
    RegSetValueExW(subkey, L"DesktopFile", 0, REG_SZ, (const BYTE*) desktopFile, (lstrlenW(desktopFile) + 1) * sizeof(WCHAR));
    if (openWithIcon)
        RegSetValueExW(subkey, L"OpenWithIcon", 0, REG_SZ, (const BYTE*) openWithIcon, (lstrlenW(openWithIcon) + 1) * sizeof(WCHAR));
    else
        RegDeleteValueW(subkey, L"OpenWithIcon");

done:
    RegCloseKey(assocKey);
    RegCloseKey(subkey);
}

static BOOL cleanup_associations(void)
{
    HKEY assocKey;
    BOOL hasChanged = FALSE;
    if ((assocKey = open_associations_reg_key()))
    {
        int i = 0;
        for (;;)
        {
            WCHAR *extensionW;
            WCHAR *command;

            if (!(extensionW = reg_enum_keyW(assocKey, i)))
                break;

            if (!(command = assoc_query(ASSOCSTR_COMMAND, extensionW, L"open")))
            {
                WCHAR *desktopFile = reg_get_valW(assocKey, extensionW, L"DesktopFile");
                if (desktopFile)
                {
                    WINE_TRACE("removing file type association for %s\n", wine_dbgstr_w(extensionW));
                    DeleteFileW(desktopFile);
                }
                RegDeleteKeyW(assocKey, extensionW);
                hasChanged = TRUE;
                free(desktopFile);
            }
            else
            {
                i++;
                free(command);
            }
            free(extensionW);
        }
        RegCloseKey(assocKey);
    }
    else
        WINE_ERR("could not open file associations key\n");
    return hasChanged;
}

static BOOL write_freedesktop_mime_type_entry(const WCHAR *packages_dir, const WCHAR *dot_extension,
                                              const WCHAR *mime_type, const WCHAR *comment)
{
    BOOL ret = FALSE;
    WCHAR *filename;
    FILE *packageFile;

    WINE_TRACE("writing MIME type %s, extension=%s, comment=%s\n", wine_dbgstr_w(mime_type),
               wine_dbgstr_w(dot_extension), wine_dbgstr_w(comment));

    filename = heap_wprintf(L"%s\\x-wine-extension-%s.xml", packages_dir, dot_extension + 1);
    packageFile = _wfopen( filename, L"wb" );
    if (packageFile)
    {
        fprintf(packageFile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(packageFile, "<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n");
        fprintf(packageFile, "  <mime-type type=\"%s\">\n", wchars_to_xml_text(mime_type));
        fprintf(packageFile, "    <glob pattern=\"*%s\"/>\n", wchars_to_xml_text(dot_extension));
        if (comment) fprintf(packageFile, "    <comment>%s</comment>\n", wchars_to_xml_text(comment));
        fprintf(packageFile, "  </mime-type>\n");
        fprintf(packageFile, "</mime-info>\n");
        ret = TRUE;
        fclose(packageFile);
    }
    else
        WINE_ERR("error writing file %s\n", debugstr_w(filename));
    free(filename);
    return ret;
}

static BOOL is_type_banned(const WCHAR *win_type)
{
    /* These are managed through external tools like wine.desktop, to evade malware created file type associations */
    if (!wcsicmp(win_type, L".bat") ||
        !wcsicmp(win_type, L".com") ||
        !wcsicmp(win_type, L".exe") ||
        !wcsicmp(win_type, L".msi") ||
        !wcsicmp(win_type, L".url"))
        return TRUE;
    /* Associating a program with the file URI scheme is like associating it with all file types, which is not allowed
     * for the same reasons */
    if (!wcsicmp(win_type, L"file"))
        return TRUE;
    return FALSE;
}

static BOOL on_exclude_list(const WCHAR *command)
{
    static const WCHAR default_exclude_list[] = L"ieframe.dll\0iexplore.exe\0notepad.exe\0"
                                                L"winebrowser.exe\0wordpad.exe\0";
    WCHAR *exclude_list = NULL;
    const WCHAR *pattern;
    HKEY key;
    DWORD size;
    LSTATUS status;
    BOOL found = FALSE;

    if ((key = open_associations_reg_key()))
    {
        status = RegGetValueW(key, NULL, L"Exclude", RRF_RT_REG_MULTI_SZ, NULL, NULL, &size);
        if (status == ERROR_SUCCESS)
        {
            exclude_list = xmalloc(size);
            status = RegGetValueW(key, NULL, L"Exclude", RRF_RT_REG_MULTI_SZ, NULL, exclude_list, &size);
            if (status != ERROR_SUCCESS)
            {
                free(exclude_list);
                exclude_list = NULL;
            }
        }
        RegCloseKey(key);
    }

    for (pattern = exclude_list ? exclude_list : default_exclude_list; *pattern; pattern += wcslen(pattern) + 1)
    {
        if (wcsstr(command, pattern))
        {
            found = TRUE;
            break;
        }
    }

    free(exclude_list);
    return found;
}

static WCHAR *get_special_mime_type(LPCWSTR extension)
{
    if (!wcsicmp(extension, L".lnk"))
        return xwcsdup(L"application/x-ms-shortcut");
    return NULL;
}

static BOOL write_freedesktop_association_entry(const WCHAR *desktopPath, const WCHAR *friendlyAppName,
                                                const WCHAR *mimeType, const WCHAR *progId,
                                                const WCHAR *openWithIcon)
{
    BOOL ret = FALSE;
    FILE *desktop;
    const WCHAR *prefix = _wgetenv( L"WINECONFIGDIR" );

    WINE_TRACE("friendlyAppName=%s, MIME type %s, progID=%s, icon=%s to file %s\n",
               wine_dbgstr_w(friendlyAppName), wine_dbgstr_w(mimeType),
               wine_dbgstr_w(progId), wine_dbgstr_w(openWithIcon), wine_dbgstr_w(desktopPath));

    desktop = _wfopen( desktopPath, L"wb" );
    if (desktop)
    {
        fprintf(desktop, "[Desktop Entry]\n");
        fprintf(desktop, "Type=Application\n");
        fprintf(desktop, "Name=%s\n", wchars_to_utf8_chars(friendlyAppName));
        fprintf(desktop, "MimeType=%s;\n", wchars_to_utf8_chars(mimeType));
        if (prefix)
        {
            char *path = wine_get_unix_file_name( prefix );
            fprintf(desktop, "Exec=env WINEPREFIX=\"%s\" wine start ", path);
            heap_free( path );
        }
        else
            fprintf(desktop, "Exec=wine start ");
        if (progId) /* file association */
            fprintf(desktop, "/ProgIDOpen %s %%f\n", escape(progId));
        else /* protocol association */
            fprintf(desktop, "%%u\n");
        fprintf(desktop, "NoDisplay=true\n");
        fprintf(desktop, "StartupNotify=true\n");
        if (openWithIcon)
            fprintf(desktop, "Icon=%s\n", wchars_to_utf8_chars(openWithIcon));
        ret = TRUE;
        fclose(desktop);
    }
    else
        WINE_ERR("error writing association file %s\n", wine_dbgstr_w(desktopPath));
    return ret;
}

static BOOL generate_associations(const WCHAR *packages_dir, const WCHAR *applications_dir)
{
    struct wine_rb_tree mimeProgidTree = { winemenubuilder_rb_string_compare };
    struct list nativeMimeTypes = LIST_INIT(nativeMimeTypes);
    int i;
    BOOL hasChanged = FALSE;

    if (!build_native_mime_types(&nativeMimeTypes))
    {
        WINE_ERR("could not build native MIME types\n");
        return FALSE;
    }

    for (i = 0; ; i++)
    {
        WCHAR *winTypeW;
        BOOL isProtocolType = FALSE;

        if (!(winTypeW = reg_enum_keyW(HKEY_CLASSES_ROOT, i)))
            break;

        if (winTypeW[0] != '.')
        {
            if (RegGetValueW(HKEY_CLASSES_ROOT, winTypeW, L"URL Protocol", RRF_RT_ANY, NULL, NULL, NULL) == ERROR_SUCCESS)
                isProtocolType = TRUE;
        }

        if ((winTypeW[0] == '.' || isProtocolType) && !is_type_banned(winTypeW))
        {
            WCHAR *commandW = NULL;
            WCHAR *executableW = NULL;
            WCHAR *openWithIcon = NULL;
            WCHAR *friendlyDocNameW = NULL;
            WCHAR *iconW = NULL;
            WCHAR *contentTypeW = NULL;
            WCHAR *mimeType = NULL;
            const WCHAR *friendlyAppName;
            WCHAR *progIdW = NULL;
            WCHAR *mimeProgId = NULL;
            struct rb_string_entry *entry;

            commandW = assoc_query(ASSOCSTR_COMMAND, winTypeW, L"open");
            if (commandW == NULL)
                /* no command => no application is associated */
                goto end;

            if (on_exclude_list(commandW))
                /* command is on the exclude list => desktop integration is not desirable */
                goto end;

            iconW = assoc_query(ASSOCSTR_DEFAULTICON, winTypeW, NULL);

            if (isProtocolType)
            {
                mimeType = heap_wprintf(L"x-scheme-handler/%s", winTypeW);
            }
            else
            {
                wcslwr(winTypeW);
                friendlyDocNameW = assoc_query(ASSOCSTR_FRIENDLYDOCNAME, winTypeW, NULL);

                contentTypeW = assoc_query(ASSOCSTR_CONTENTTYPE, winTypeW, NULL);
                if (contentTypeW)
                    wcslwr(contentTypeW);

                mimeType = freedesktop_mime_type_for_extension(&nativeMimeTypes, winTypeW);

                if (mimeType == NULL)
                {
                    if (contentTypeW != NULL && wcschr(contentTypeW, '/'))
                        mimeType = xwcsdup(contentTypeW);
                    else if (!(mimeType = get_special_mime_type(winTypeW)))
                        mimeType = heap_wprintf(L"application/x-wine-extension-%s", &winTypeW[1]);

                    /* GNOME seems to ignore the <icon> tag in MIME packages,
                     * and the default name is more intuitive anyway.
                     */
                    if (iconW)
                    {
                        WCHAR *flattened_mime = slashes_to_minuses(mimeType);
                        int index = 0;
                        WCHAR *comma = wcsrchr(iconW, ',');
                        if (comma)
                        {
                            *comma = 0;
                            index = wcstol(comma + 1, NULL, 10);
                        }
                        extract_icon(iconW, index, flattened_mime, FALSE);
                        free(flattened_mime);
                    }

                    write_freedesktop_mime_type_entry(packages_dir, winTypeW, mimeType, friendlyDocNameW);
                    hasChanged = TRUE;
                }

                progIdW = reg_get_valW(HKEY_CLASSES_ROOT, winTypeW, NULL);
                if (!progIdW) goto end; /* no progID => not a file type association */

                /* Do not allow duplicate ProgIDs for a MIME type, it causes unnecessary duplication in Open dialogs */
                mimeProgId = heap_wprintf(L"%s=>%s", mimeType, progIdW);
                if (wine_rb_get(&mimeProgidTree, mimeProgId))
                {
                    heap_free(mimeProgId);
                    goto end;
                }
                entry = xmalloc(sizeof(struct rb_string_entry));
                entry->string = mimeProgId;
                if (wine_rb_put(&mimeProgidTree, mimeProgId, &entry->entry))
                {
                    WINE_ERR("error updating rb tree\n");
                    goto end;
                }
            }

            executableW = assoc_query(ASSOCSTR_EXECUTABLE, winTypeW, L"open");
            if (executableW)
                openWithIcon = compute_native_identifier(0, executableW, NULL);

            friendlyAppName = assoc_query(ASSOCSTR_FRIENDLYAPPNAME, winTypeW, L"open");
            if (!friendlyAppName) friendlyAppName = L"A Wine application";

            if (has_association_changed(winTypeW, mimeType, progIdW, friendlyAppName, openWithIcon))
            {
                WCHAR *desktopPath;
                if (isProtocolType)
                    desktopPath = heap_wprintf(L"%s\\wine-protocol-%s.desktop", applications_dir, winTypeW);
                else
                    desktopPath = heap_wprintf(L"%s\\wine-extension-%s.desktop", applications_dir, winTypeW + 1);
                if (write_freedesktop_association_entry(desktopPath, friendlyAppName, mimeType, progIdW, openWithIcon))
                {
                    hasChanged = TRUE;
                    update_association(winTypeW, mimeType, progIdW, friendlyAppName, desktopPath, openWithIcon);
                }
                free(desktopPath);
            }

            if (hasChanged && openWithIcon) extract_icon(executableW, 0, openWithIcon, FALSE);

        end:
            free(commandW);
            free(executableW);
            free(openWithIcon);
            free(friendlyDocNameW);
            free(iconW);
            free(contentTypeW);
            free(mimeType);
            free(progIdW);
        }
        free(winTypeW);
    }

    wine_rb_destroy(&mimeProgidTree, winemenubuilder_rb_destroy, NULL);
    free_native_mime_types(&nativeMimeTypes);
    return hasChanged;
}

static BOOL InvokeShellLinker( IShellLinkW *sl, LPCWSTR link, BOOL bWait )
{
    WCHAR *icon_name, *link_name;
    WCHAR szTmp[INFOTIPSIZE];
    WCHAR szDescription[INFOTIPSIZE], szPath[MAX_PATH], szWorkDir[MAX_PATH];
    WCHAR szArgs[INFOTIPSIZE], szIconPath[MAX_PATH], szWMClass[MAX_PATH];
    int iIconId = 0, r = -1;
    DWORD csidl = -1;
    HANDLE hsem = NULL;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return FALSE;
    }

    if( !get_link_location( link, &csidl, &link_name ) )
    {
        WINE_WARN("Unknown link location %s. Ignoring.\n",wine_dbgstr_w(link));
        return TRUE;
    }
    if (!in_desktop_dir(csidl) && !in_startmenu(csidl))
    {
        WINE_WARN("Not under desktop or start menu. Ignoring.\n");
        return TRUE;
    }
    WINE_TRACE("Link       : %s\n", wine_dbgstr_w(link_name));

    szTmp[0] = 0;
    IShellLinkW_GetWorkingDirectory( sl, szTmp, MAX_PATH );
    ExpandEnvironmentStringsW(szTmp, szWorkDir, MAX_PATH);
    WINE_TRACE("workdir    : %s\n", wine_dbgstr_w(szWorkDir));

    szTmp[0] = 0;
    IShellLinkW_GetDescription( sl, szTmp, INFOTIPSIZE );
    ExpandEnvironmentStringsW(szTmp, szDescription, INFOTIPSIZE);
    WINE_TRACE("description: %s\n", wine_dbgstr_w(szDescription));

    get_cmdline( sl, szTmp, MAX_PATH, szArgs, INFOTIPSIZE);
    ExpandEnvironmentStringsW(szTmp, szPath, MAX_PATH);
    WINE_TRACE("path       : %s\n", wine_dbgstr_w(szPath));
    WINE_TRACE("args       : %s\n", wine_dbgstr_w(szArgs));

    szTmp[0] = 0;
    IShellLinkW_GetIconLocation( sl, szTmp, MAX_PATH, &iIconId );
    ExpandEnvironmentStringsW(szTmp, szIconPath, MAX_PATH);
    WINE_TRACE("icon file  : %s\n", wine_dbgstr_w(szIconPath) );

    szWMClass[0] = 0;

    if( !szPath[0] )
    {
        LPITEMIDLIST pidl = NULL;
        IShellLinkW_GetIDList( sl, &pidl );
        if( pidl && SHGetPathFromIDListW( pidl, szPath ) )
            WINE_TRACE("pidl path  : %s\n", wine_dbgstr_w(szPath));
    }

    /* extract the icon */
    if( szIconPath[0] )
        icon_name = extract_icon( szIconPath , iIconId, NULL, bWait );
    else
        icon_name = extract_icon( szPath, iIconId, NULL, bWait );

    /* fail - try once again after parent process exit */
    if( !icon_name )
    {
        if (bWait)
        {
            WINE_WARN("Unable to extract icon, deferring.\n");
            goto cleanup;
        }
        WINE_ERR("failed to extract icon from %s\n",
                 wine_dbgstr_w( szIconPath[0] ? szIconPath : szPath ));
    }

    /* check the path */
    if( szPath[0] )
    {
        /* FIXME: Use AppUserModelID if present. */
        WCHAR *p = PathFindFileNameW(szPath);

        if (p)
        {
            lstrcpyW(szWMClass, p);
            CharLowerW(szWMClass);
        }
    }

    /* building multiple menus concurrently has race conditions */
    hsem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != MsgWaitForMultipleObjects( 1, &hsem, FALSE, INFINITE, QS_ALLINPUT ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        goto cleanup;
    }

    if (in_desktop_dir(csidl))
    {
        if (csidl == CSIDL_COMMON_DESKTOPDIRECTORY || !szPath[0])
            r = !write_desktop_entry(link, NULL, link_name, link, NULL,
                                     szDescription, szWorkDir, icon_name, szWMClass);
        else
            r = !write_desktop_entry(NULL, NULL, link_name, szPath, szArgs,
                                     szDescription, szWorkDir, icon_name, szWMClass);
    }
    else
        r = !write_menu_entry(link, link_name, link, NULL, szDescription, szWorkDir, icon_name, szWMClass);

    ReleaseSemaphore( hsem, 1, NULL );

cleanup:
    if (hsem) CloseHandle( hsem );
    free(icon_name);
    free(link_name);

    if (r && !bWait)
        WINE_ERR("failed to build the menu\n" );

    return ( r == 0 );
}

static BOOL InvokeShellLinkerForURL( IUniformResourceLocatorW *url, LPCWSTR link, BOOL bWait )
{
    WCHAR *link_name, *icon_name = NULL;
    DWORD csidl = -1;
    LPWSTR urlPath = NULL;
    HRESULT hr;
    HANDLE hSem = NULL;
    BOOL ret = TRUE;
    int r = -1;
    IPropertySetStorage *pPropSetStg;
    IPropertyStorage *pPropStg;
    PROPSPEC ps[2];
    PROPVARIANT pv[2];
    BOOL has_icon = FALSE;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return TRUE;
    }

    if( !get_link_location( link, &csidl, &link_name ) )
    {
        WINE_WARN("Unknown link location %s. Ignoring.\n",wine_dbgstr_w(link));
        return TRUE;
    }
    if (!in_desktop_dir(csidl) && !in_startmenu(csidl))
    {
        WINE_WARN("Not under desktop or start menu. Ignoring.\n");
        ret = TRUE;
        goto cleanup;
    }
    WINE_TRACE("Link       : %s\n", wine_dbgstr_w(link_name));

    hr = url->lpVtbl->GetURL(url, &urlPath);
    if (FAILED(hr))
    {
        ret = TRUE;
        goto cleanup;
    }
    WINE_TRACE("path       : %s\n", wine_dbgstr_w(urlPath));

    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = PID_IS_ICONFILE;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = PID_IS_ICONINDEX;

    hr = url->lpVtbl->QueryInterface(url, &IID_IPropertySetStorage, (void **) &pPropSetStg);
    if (SUCCEEDED(hr))
    {
        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
        if (SUCCEEDED(hr))
        {
            hr = IPropertyStorage_ReadMultiple(pPropStg, 2, ps, pv);
            if (SUCCEEDED(hr))
            {
                if (pv[0].vt == VT_LPWSTR && pv[0].pwszVal && pv[0].pwszVal[0])
                {
                    has_icon = TRUE;
                    icon_name = extract_icon( pv[0].pwszVal, pv[1].iVal, NULL, bWait );

                    WINE_TRACE("URL icon path: %s icon index: %d icon name: %s\n", wine_dbgstr_w(pv[0].pwszVal), pv[1].iVal, debugstr_w(icon_name));
                }
                PropVariantClear(&pv[0]);
                PropVariantClear(&pv[1]);
            }
            IPropertyStorage_Release(pPropStg);
        }
        IPropertySetStorage_Release(pPropSetStg);
    }

    /* fail - try once again after parent process exit */
    if( has_icon && !icon_name )
    {
        if (bWait)
        {
            WINE_WARN("Unable to extract icon, deferring.\n");
            ret = FALSE;
            goto cleanup;
        }
        WINE_ERR("failed to extract icon from %s\n",
                 wine_dbgstr_w( pv[0].pwszVal ));
    }

    hSem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != MsgWaitForMultipleObjects( 1, &hSem, FALSE, INFINITE, QS_ALLINPUT ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        goto cleanup;
    }
    if (in_desktop_dir(csidl))
        r = !write_desktop_entry(NULL, NULL, link_name, L"start.exe", urlPath, NULL, NULL, icon_name, NULL);
    else
        r = !write_menu_entry(link, link_name, L"start.exe", urlPath, NULL, NULL, icon_name, NULL);
    ret = (r == 0);
    ReleaseSemaphore(hSem, 1, NULL);

cleanup:
    if (hSem)
        CloseHandle(hSem);
    free(icon_name);
    free(link_name);
    CoTaskMemFree( urlPath );
    return ret;
}

static BOOL WaitForParentProcess( void )
{
    PROCESSENTRY32 procentry;
    HANDLE hsnapshot = NULL, hprocess = NULL;
    DWORD ourpid = GetCurrentProcessId();
    BOOL ret = FALSE, rc;

    WINE_TRACE("Waiting for parent process\n");
    if ((hsnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 )) ==
        INVALID_HANDLE_VALUE)
    {
        WINE_ERR("CreateToolhelp32Snapshot failed, error %ld\n", GetLastError());
        goto done;
    }

    procentry.dwSize = sizeof(PROCESSENTRY32);
    rc = Process32First( hsnapshot, &procentry );
    while (rc)
    {
        if (procentry.th32ProcessID == ourpid) break;
        rc = Process32Next( hsnapshot, &procentry );
    }
    if (!rc)
    {
        WINE_WARN("Unable to find current process id %ld when listing processes\n", ourpid);
        goto done;
    }

    if ((hprocess = OpenProcess( SYNCHRONIZE, FALSE, procentry.th32ParentProcessID )) ==
        NULL)
    {
        WINE_WARN("OpenProcess failed pid=%ld, error %ld\n", procentry.th32ParentProcessID,
                 GetLastError());
        goto done;
    }

    if (MsgWaitForMultipleObjects( 1, &hprocess, FALSE, INFINITE, QS_ALLINPUT ) == WAIT_OBJECT_0)
        ret = TRUE;
    else
        WINE_ERR("Unable to wait for parent process, error %ld\n", GetLastError());

done:
    if (hprocess) CloseHandle( hprocess );
    if (hsnapshot) CloseHandle( hsnapshot );
    return ret;
}

static BOOL Process_Link( LPCWSTR linkname, BOOL bWait )
{
    IShellLinkW *sl;
    IPersistFile *pf;
    HRESULT r;
    WCHAR fullname[MAX_PATH];
    DWORD len;

    WINE_TRACE("%s, wait %d\n", wine_dbgstr_w(linkname), bWait);

    if( !linkname[0] )
    {
        WINE_ERR("link name missing\n");
        return FALSE;
    }

    len=GetFullPathNameW( linkname, MAX_PATH, fullname, NULL );
    if (len==0 || len>MAX_PATH)
    {
        WINE_ERR("couldn't get full path of link file\n");
        return FALSE;
    }

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (LPVOID *) &sl );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IShellLink\n");
        return FALSE;
    }

    r = IShellLinkW_QueryInterface( sl, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return FALSE;
    }

    r = IPersistFile_Load( pf, fullname, STGM_READ );
    if( SUCCEEDED( r ) )
    {
        /* If something fails (eg. Couldn't extract icon)
         * wait for parent process and try again
         */
        if( ! InvokeShellLinker( sl, fullname, bWait ) && bWait )
        {
            WaitForParentProcess();
            InvokeShellLinker( sl, fullname, FALSE );
        }
    }
    else
    {
        WINE_ERR("unable to load %s\n", wine_dbgstr_w(linkname));
    }

    IPersistFile_Release( pf );
    IShellLinkW_Release( sl );

    return !r;
}

static BOOL Process_URL( LPCWSTR urlname, BOOL bWait )
{
    IUniformResourceLocatorW *url;
    IPersistFile *pf;
    HRESULT r;
    WCHAR fullname[MAX_PATH];
    DWORD len;

    WINE_TRACE("%s, wait %d\n", wine_dbgstr_w(urlname), bWait);

    if( !urlname[0] )
    {
        WINE_ERR("URL name missing\n");
        return FALSE;
    }

    len=GetFullPathNameW( urlname, MAX_PATH, fullname, NULL );
    if (len==0 || len>MAX_PATH)
    {
        WINE_ERR("couldn't get full path of URL file\n");
        return FALSE;
    }

    r = CoCreateInstance( &CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUniformResourceLocatorW, (LPVOID *) &url );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IUniformResourceLocatorW\n");
        return FALSE;
    }

    r = url->lpVtbl->QueryInterface( url, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return FALSE;
    }
    r = IPersistFile_Load( pf, fullname, STGM_READ );
    if( SUCCEEDED( r ) )
    {
        /* If something fails (eg. Couldn't extract icon)
         * wait for parent process and try again
         */
        if( ! InvokeShellLinkerForURL( url, fullname, bWait ) && bWait )
        {
            WaitForParentProcess();
            InvokeShellLinkerForURL( url, fullname, FALSE );
        }
    }

    IPersistFile_Release( pf );
    url->lpVtbl->Release( url );

    return !r;
}

static void RefreshFileTypeAssociations(void)
{
    HANDLE hSem = NULL;
    WCHAR *mime_dir;
    WCHAR *packages_dir;
    WCHAR *applications_dir;
    BOOL hasChanged;

    hSem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != MsgWaitForMultipleObjects( 1, &hSem, FALSE, INFINITE, QS_ALLINPUT ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        CloseHandle(hSem);
        return;
    }

    mime_dir = heap_wprintf(L"%s\\mime", xdg_data_dir);
    packages_dir = heap_wprintf(L"%s\\packages", mime_dir);
    create_directories(packages_dir);

    applications_dir = heap_wprintf(L"%s\\applications", xdg_data_dir);
    create_directories(applications_dir);

    hasChanged = generate_associations(packages_dir, applications_dir);
    hasChanged |= cleanup_associations();
    if (hasChanged)
    {
        const char *argv[3];

        argv[0] = "update-mime-database";
        argv[1] = wine_get_unix_file_name(mime_dir);
        argv[2] = NULL;
        __wine_unix_spawnvp( (char **)argv, FALSE );

        argv[0] = "update-desktop-database";
        argv[1] = wine_get_unix_file_name(applications_dir);
        __wine_unix_spawnvp( (char **)argv, FALSE );
    }

    ReleaseSemaphore(hSem, 1, NULL);
    CloseHandle(hSem);
    free(mime_dir);
    free(packages_dir);
    free(applications_dir);
}

static void cleanup_menus(void)
{
    HKEY hkey;

    hkey = open_menus_reg_key();
    if (hkey)
    {
        int i;
        LSTATUS lret = ERROR_SUCCESS;
        for (i = 0; lret == ERROR_SUCCESS; )
        {
            WCHAR *value = NULL;
            WCHAR *data = NULL;
            DWORD valueSize = 4096;
            DWORD dataSize = 4096;
            while (1)
            {
                value = xmalloc(valueSize * sizeof(WCHAR));
                data = xmalloc(dataSize * sizeof(WCHAR));
                lret = RegEnumValueW(hkey, i, value, &valueSize, NULL, NULL, (BYTE*)data, &dataSize);
                if (lret != ERROR_MORE_DATA)
                    break;
                valueSize *= 2;
                dataSize *= 2;
                free(value);
                free(data);
                value = data = NULL;
            }
            if (lret == ERROR_SUCCESS)
            {
                if (GetFileAttributesW( data ) == INVALID_FILE_ATTRIBUTES)
                {
                    WINE_TRACE("removing menu related file %s\n", debugstr_w(value));
                    DeleteFileW( value );
                    RegDeleteValueW(hkey, value);
                }
                else
                    i++;
            }
            else if (lret != ERROR_NO_MORE_ITEMS)
                WINE_ERR("error %ld reading registry\n", lret);
            free(value);
            free(data);
        }
        RegCloseKey(hkey);
    }
}

static void thumbnail_lnk(LPCWSTR lnkPath, LPCWSTR outputPath)
{
    char *utf8lnkPath = NULL;
    WCHAR *winLnkPath = NULL;
    IShellLinkW *shellLink = NULL;
    IPersistFile *persistFile = NULL;
    WCHAR szTmp[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    WCHAR szArgs[INFOTIPSIZE];
    WCHAR szIconPath[MAX_PATH];
    int iconId;
    IStream *stream = NULL;
    ICONDIRENTRY *pIconDirEntries = NULL;
    int numEntries;
    HRESULT hr;

    utf8lnkPath = wchars_to_utf8_chars(lnkPath);
    winLnkPath = wine_get_dos_file_name(utf8lnkPath);
    if (winLnkPath == NULL)
    {
        WINE_ERR("could not convert %s to DOS path\n", utf8lnkPath);
        goto end;
    }

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (LPVOID*)&shellLink);
    if (FAILED(hr))
    {
        WINE_ERR("could not create IShellLinkW, error 0x%08lX\n", hr);
        goto end;
    }

    hr = IShellLinkW_QueryInterface(shellLink, &IID_IPersistFile, (LPVOID)&persistFile);
    if (FAILED(hr))
    {
        WINE_ERR("could not query IPersistFile, error 0x%08lX\n", hr);
        goto end;
    }

    hr = IPersistFile_Load(persistFile, winLnkPath, STGM_READ);
    if (FAILED(hr))
    {
        WINE_ERR("could not read .lnk, error 0x%08lX\n", hr);
        goto end;
    }

    get_cmdline(shellLink, szTmp, MAX_PATH, szArgs, INFOTIPSIZE);
    ExpandEnvironmentStringsW(szTmp, szPath, MAX_PATH);
    szTmp[0] = 0;
    IShellLinkW_GetIconLocation(shellLink, szTmp, MAX_PATH, &iconId);
    ExpandEnvironmentStringsW(szTmp, szIconPath, MAX_PATH);

    if(!szPath[0])
    {
        LPITEMIDLIST pidl = NULL;
        IShellLinkW_GetIDList(shellLink, &pidl);
        if (pidl && SHGetPathFromIDListW(pidl, szPath))
            WINE_TRACE("pidl path  : %s\n", wine_dbgstr_w(szPath));
    }

    if (szIconPath[0])
    {
        hr = open_icon(szIconPath, iconId, FALSE, &stream, &pIconDirEntries, &numEntries);
        if (SUCCEEDED(hr))
            hr = write_native_icon(stream, pIconDirEntries, numEntries, outputPath);
    }
    else
    {
        hr = open_icon(szPath, iconId, FALSE, &stream, &pIconDirEntries, &numEntries);
        if (SUCCEEDED(hr))
            hr = write_native_icon(stream, pIconDirEntries, numEntries, outputPath);
    }

end:
    free(utf8lnkPath);
    heap_free(winLnkPath);
    if (shellLink != NULL)
        IShellLinkW_Release(shellLink);
    if (persistFile != NULL)
        IPersistFile_Release(persistFile);
    if (stream != NULL)
        IStream_Release(stream);
    free(pIconDirEntries);
}

static WCHAR *next_token( LPWSTR *p )
{
    LPWSTR token = NULL, t = *p;

    if( !t )
        return NULL;

    while( t && !token )
    {
        switch( *t )
        {
        case ' ':
            t++;
            continue;
        case '"':
            /* unquote the token */
            token = ++t;
            t = wcschr( token, '"' );
            if( t )
                 *t++ = 0;
            break;
        case 0:
            t = NULL;
            break;
        default:
            token = t;
            t = wcschr( token, ' ' );
            if( t )
                 *t++ = 0;
            break;
        }
    }
    *p = t;
    return token;
}

static BOOL init_xdg(void)
{
    WCHAR *p;
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, xdg_desktop_dir);

    if (FAILED(hr)) return FALSE;

    if ((p = _wgetenv( L"XDG_CONFIG_HOME" )))
        xdg_menu_dir = heap_wprintf( L"\\??\\unix%s/menus/applications-merged", p );
    else
        xdg_menu_dir = heap_wprintf( L"%s/.config/menus/applications-merged", _wgetenv(L"WINEHOMEDIR") );
    for (p = xdg_menu_dir; *p; p++) if (*p == '/') *p = '\\';
    xdg_menu_dir[1] = '\\';  /* change \??\ to \\?\ */
    create_directories(xdg_menu_dir);

    if ((p = _wgetenv( L"XDG_DATA_HOME" )))
        xdg_data_dir = heap_wprintf( L"\\??\\unix%s", p );
    else
        xdg_data_dir = heap_wprintf( L"%s/.local/share", _wgetenv(L"WINEHOMEDIR") );
    for (p = xdg_data_dir; *p; p++) if (*p == '/') *p = '\\';
    xdg_data_dir[1] = '\\';  /* change \??\ to \\?\ */

    p = heap_wprintf( L"%s\\desktop-directories", xdg_data_dir );
    create_directories(p);
    free(p);
    return TRUE;
}

static BOOL associations_enabled(void)
{
    BOOL ret = TRUE;
    HKEY hkey;
    BYTE buf[32];
    DWORD len;

    if ((hkey = open_associations_reg_key()))
    {
        len = sizeof(buf);
        if (!RegQueryValueExA(hkey, "Enable", NULL, NULL, buf, &len))
            ret = IS_OPTION_TRUE(buf[0]);
        RegCloseKey( hkey );
    }

    return ret;
}

/***********************************************************************
 *
 *           wWinMain
 */
int PASCAL wWinMain (HINSTANCE hInstance, HINSTANCE prev, LPWSTR cmdline, int show)
{
    LPWSTR token = NULL, p;
    BOOL bWait = FALSE;
    BOOL bURL = FALSE;
    HRESULT hr;
    int ret = 0;

    if (!init_xdg())
        return 1;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        WINE_ERR("could not initialize COM, error 0x%08lX\n", hr);
        return 1;
    }

    for( p = cmdline; p && *p; )
    {
        token = next_token( &p );
	if( !token )
	    break;
        if( !wcscmp( token, L"-a" ) )
        {
            if (associations_enabled())
                RefreshFileTypeAssociations();
            continue;
        }
        if( !wcscmp( token, L"-r" ) )
        {
            cleanup_menus();
            continue;
        }
        if( !wcscmp( token, L"-w" ) )
            bWait = TRUE;
        else if ( !wcscmp( token, L"-u" ) )
            bURL = TRUE;
        else if ( !wcscmp( token, L"-t" ) )
        {
            WCHAR *lnkFile = next_token( &p );
            if (lnkFile)
            {
                 WCHAR *outputFile = next_token( &p );
                 if (outputFile)
                     thumbnail_lnk(lnkFile, outputFile);
            }
        }
	else if( token[0] == '-' )
	{
	    WINE_ERR( "unknown option %s\n", wine_dbgstr_w(token) );
	}
        else
        {
            BOOL bRet;

            if (bURL)
                bRet = Process_URL( token, bWait );
            else
                bRet = Process_Link( token, bWait );
            if (!bRet)
            {
                WINE_ERR( "failed to build menu item for %s\n", wine_dbgstr_w(token) );
                ret = 1;
            }
        }
    }

    CoUninitialize();
    return ret;
}
