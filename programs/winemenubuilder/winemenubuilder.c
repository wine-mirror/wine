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
 * interface, then create a KDE/Gnome menu entry for the shortcut.
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
 *  Generate icons for file open handlers to go into the "Open with..."
 * list. What does Windows use, the default icon for the .EXE file? It's
 * not in the registry.
 *  Associate applications under HKCR\Applications to open any MIME type
 * (by associating with application/octet-stream, or how?).
 *  Clean up fd.o MIME types when they are deleted in Windows, their icons
 * too. Very hard - once we associate them with fd.o, we can't tell whether
 * they are ours or not, and the extension <-> MIME type mapping isn't
 * one-to-one either.
 *  Wine's HKCR is broken - it doesn't merge HKCU\Software\Classes, so apps
 * that write associations there won't associate (#17019).
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <stdarg.h>
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif

#define COBJMACROS

#include <windows.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlguid.h>
#include <appmgmt.h>
#include <tlhelp32.h>
#include <intshcut.h>
#include <shlwapi.h>
#include <initguid.h>
#include <wincodec.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/library.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(menubuilder);

#define in_desktop_dir(csidl) ((csidl)==CSIDL_DESKTOPDIRECTORY || \
                               (csidl)==CSIDL_COMMON_DESKTOPDIRECTORY)
#define in_startmenu(csidl)   ((csidl)==CSIDL_STARTMENU || \
                               (csidl)==CSIDL_COMMON_STARTMENU)
        
/* link file formats */

#include "pshpack1.h"

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


#include "poppack.h"

typedef struct
{
        HRSRC *pResInfo;
        int   nIndex;
} ENUMRESSTRUCT;

struct xdg_mime_type
{
    char *mimeType;
    char *glob;
    struct list entry;
};

DEFINE_GUID(CLSID_WICIcnsEncoder, 0x312fb6f1,0xb767,0x409d,0x8a,0x6d,0x0f,0xc1,0x54,0xd4,0xf0,0x5c);

static char *xdg_config_dir;
static char *xdg_data_dir;
static char *xdg_desktop_dir;

static WCHAR* assoc_query(ASSOCSTR assocStr, LPCWSTR name, LPCWSTR extra);
static HRESULT open_icon(LPCWSTR filename, int index, BOOL bWait, IStream **ppStream);

/* Utility routines */
static unsigned short crc16(const char* string)
{
    unsigned short crc = 0;
    int i, j, xor_poly;

    for (i = 0; string[i] != 0; i++)
    {
        char c = string[i];
        for (j = 0; j < 8; c >>= 1, j++)
        {
            xor_poly = (c ^ crc) & 1;
            crc >>= 1;
            if (xor_poly)
                crc ^= 0xa001;
        }
    }
    return crc;
}

static char *strdupA( const char *str )
{
    char *ret;

    if (!str) return NULL;
    if ((ret = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 ))) strcpy( ret, str );
    return ret;
}

static char* heap_printf(const char *format, ...)
{
    va_list args;
    int size = 4096;
    char *buffer, *ret;
    int n;

    va_start(args, format);
    while (1)
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, size);
        if (buffer == NULL)
            break;
        n = vsnprintf(buffer, size, format, args);
        if (n == -1)
            size *= 2;
        else if (n >= size)
            size = n + 1;
        else
            break;
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    va_end(args);
    if (!buffer) return NULL;
    ret = HeapReAlloc(GetProcessHeap(), 0, buffer, strlen(buffer) + 1 );
    if (!ret) ret = buffer;
    return ret;
}

static void write_xml_text(FILE *file, const char *text)
{
    int i;
    for (i = 0; text[i]; i++)
    {
        if (text[i] == '&')
            fputs("&amp;", file);
        else if (text[i] == '<')
            fputs("&lt;", file);
        else if (text[i] == '>')
            fputs("&gt;", file);
        else if (text[i] == '\'')
            fputs("&apos;", file);
        else if (text[i] == '"')
            fputs("&quot;", file);
        else
            fputc(text[i], file);
    }
}

static BOOL create_directories(char *directory)
{
    BOOL ret = TRUE;
    int i;

    for (i = 0; directory[i]; i++)
    {
        if (i > 0 && directory[i] == '/')
        {
            directory[i] = 0;
            mkdir(directory, 0777);
            directory[i] = '/';
        }
    }
    if (mkdir(directory, 0777) && errno != EEXIST)
       ret = FALSE;

    return ret;
}

static char* wchars_to_utf8_chars(LPCWSTR string)
{
    char *ret;
    INT size = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL);
    ret = HeapAlloc(GetProcessHeap(), 0, size);
    if (ret)
        WideCharToMultiByte(CP_UTF8, 0, string, -1, ret, size, NULL, NULL);
    return ret;
}

static char* wchars_to_unix_chars(LPCWSTR string)
{
    char *ret;
    INT size = WideCharToMultiByte(CP_UNIXCP, 0, string, -1, NULL, 0, NULL, NULL);
    ret = HeapAlloc(GetProcessHeap(), 0, size);
    if (ret)
        WideCharToMultiByte(CP_UNIXCP, 0, string, -1, ret, size, NULL, NULL);
    return ret;
}

static WCHAR* utf8_chars_to_wchars(LPCSTR string)
{
    WCHAR *ret;
    INT size = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
    ret = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_UTF8, 0, string, -1, ret, size);
    return ret;
}

/* Icon extraction routines
 *
 * FIXME: should use PrivateExtractIcons and friends
 * FIXME: should not use stdio
 */

static HRESULT convert_to_native_icon(IStream *icoFile, int *indeces, int numIndeces,
                                      const CLSID *outputFormat, const char *outputFileName, LPCWSTR commentW)
{
    WCHAR *dosOutputFileName = NULL;
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IStream *outputFile = NULL;
    int i;
    HRESULT hr = E_FAIL;

    dosOutputFileName = wine_get_dos_file_name(outputFileName);
    if (dosOutputFileName == NULL)
    {
        WINE_ERR("error converting %s to DOS file name\n", outputFileName);
        goto end;
    }
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X creating IWICImagingFactory\n", hr);
        goto end;
    }
    hr = IWICImagingFactory_CreateDecoderFromStream(factory, icoFile, NULL,
        WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X creating IWICBitmapDecoder\n", hr);
        goto end;
    }
    hr = CoCreateInstance(outputFormat, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void**)&encoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X creating bitmap encoder\n", hr);
        goto end;
    }
    hr = SHCreateStreamOnFileW(dosOutputFileName, STGM_CREATE | STGM_WRITE, &outputFile);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X creating output file\n", hr);
        goto end;
    }
    hr = IWICBitmapEncoder_Initialize(encoder, outputFile, GENERIC_WRITE);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X initializing encoder\n", hr);
        goto end;
    }

    for (i = 0; i < numIndeces; i++)
    {
        IWICBitmapFrameDecode *sourceFrame = NULL;
        IWICBitmapSource *sourceBitmap = NULL;
        IWICBitmapFrameEncode *dstFrame = NULL;
        IPropertyBag2 *options = NULL;
        UINT width, height;

        hr = IWICBitmapDecoder_GetFrame(decoder, indeces[i], &sourceFrame);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X getting frame %d\n", hr, indeces[i]);
            goto endloop;
        }
        hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA, (IWICBitmapSource*)sourceFrame, &sourceBitmap);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X converting bitmap to 32bppBGRA\n", hr);
            goto endloop;
        }
        hr = IWICBitmapEncoder_CreateNewFrame(encoder, &dstFrame, &options);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X creating encoder frame\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_Initialize(dstFrame, options);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X initializing encoder frame\n", hr);
            goto endloop;
        }
        hr = IWICBitmapSource_GetSize(sourceBitmap, &width, &height);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X getting source bitmap size\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_SetSize(dstFrame, width, height);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X setting destination bitmap size\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_SetResolution(dstFrame, 96, 96);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X setting destination bitmap resolution\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_WriteSource(dstFrame, sourceBitmap, NULL);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X copying bitmaps\n", hr);
            goto endloop;
        }
        hr = IWICBitmapFrameEncode_Commit(dstFrame);
        if (FAILED(hr))
        {
            WINE_ERR("error 0x%08X committing frame\n", hr);
            goto endloop;
        }
    endloop:
        if (sourceFrame)
            IWICBitmapFrameDecode_Release(sourceFrame);
        if (sourceBitmap)
            IWICBitmapSource_Release(sourceBitmap);
        if (dstFrame)
            IWICBitmapFrameEncode_Release(dstFrame);
    }

    hr = IWICBitmapEncoder_Commit(encoder);
    if (FAILED(hr))
    {
        WINE_ERR("error 0x%08X committing encoder\n", hr);
        goto end;
    }

end:
    HeapFree(GetProcessHeap(), 0, dosOutputFileName);
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

static IStream *add_module_icons_to_stream(HMODULE hModule, GRPICONDIR *grpIconDir)
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
    icons = HeapAlloc(GetProcessHeap(), 0, iconsSize);
    if (icons == NULL)
    {
        WINE_ERR("out of memory allocating icon\n");
        goto end;
    }

    iconDirEntries = HeapAlloc(GetProcessHeap(), 0, grpIconDir->idCount*sizeof(ICONDIRENTRY));
    if (iconDirEntries == NULL)
    {
        WINE_ERR("out of memory allocating icon dir entries\n");
        goto end;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr))
    {
        WINE_ERR("error creating icon stream\n");
        goto end;
    }

    iconOffset = 0;
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
                if ((pIcon = LockResource(hResData)))
                {
                    iconDirEntries[validEntries].bWidth = grpIconDir->idEntries[i].bWidth;
                    iconDirEntries[validEntries].bHeight = grpIconDir->idEntries[i].bHeight;
                    iconDirEntries[validEntries].bColorCount = grpIconDir->idEntries[i].bColorCount;
                    iconDirEntries[validEntries].bReserved = grpIconDir->idEntries[i].bReserved;
                    iconDirEntries[validEntries].wPlanes = grpIconDir->idEntries[i].wPlanes;
                    iconDirEntries[validEntries].wBitCount = grpIconDir->idEntries[i].wBitCount;
                    iconDirEntries[validEntries].dwBytesInRes = grpIconDir->idEntries[i].dwBytesInRes;
                    iconDirEntries[validEntries].dwImageOffset = iconOffset;
                    validEntries++;
                    memcpy(&icons[iconOffset], pIcon, grpIconDir->idEntries[i].dwBytesInRes);
                    iconOffset += grpIconDir->idEntries[i].dwBytesInRes;
                }
                FreeResource(hResData);
            }
        }
    }

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
        WINE_ERR("error 0x%08X writing icon stream\n", hr);
        goto end;
    }
    for (i = 0; i < validEntries; i++)
        iconDirEntries[i].dwImageOffset += sizeof(ICONDIR) + validEntries*sizeof(ICONDIRENTRY);
    hr = IStream_Write(stream, iconDirEntries, validEntries*sizeof(ICONDIRENTRY), &bytesWritten);
    if (FAILED(hr) || bytesWritten != validEntries*sizeof(ICONDIRENTRY))
    {
        WINE_ERR("error 0x%08X writing icon dir entries to stream\n", hr);
        goto end;
    }
    hr = IStream_Write(stream, icons, iconOffset, &bytesWritten);
    if (FAILED(hr) || bytesWritten != iconOffset)
    {
        WINE_ERR("error 0x%08X writing icon images to stream\n", hr);
        goto end;
    }
    zero.QuadPart = 0;
    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

end:
    HeapFree(GetProcessHeap(), 0, icons);
    HeapFree(GetProcessHeap(), 0, iconDirEntries);
    if (FAILED(hr) && stream != NULL)
    {
        IStream_Release(stream);
        stream = NULL;
    }
    return stream;
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

    hModule = LoadLibraryExW(szFileName, 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        WINE_WARN("LoadLibraryExW (%s) failed, error %d\n",
                 wine_dbgstr_w(szFileName), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (nIndex < 0)
    {
        hResInfo = FindResourceW(hModule, MAKEINTRESOURCEW(-nIndex), (LPCWSTR)RT_GROUP_ICON);
        WINE_TRACE("FindResourceW (%s) called, return %p, error %d\n",
                   wine_dbgstr_w(szFileName), hResInfo, GetLastError());
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
            WINE_TRACE("EnumResourceNamesW failed, error %d\n", GetLastError());
        }
    }

    if (hResInfo)
    {
        if ((hResData = LoadResource(hModule, hResInfo)))
        {
            if ((pIconDir = LockResource(hResData)))
            {
                *ppStream = add_module_icons_to_stream(hModule, pIconDir);
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
        WINE_WARN("Invalid ico file format (hr=0x%08X, bytesRead=%d)\n", hr, bytesRead);
        hr = E_FAIL;
        goto end;
    }
    *numEntries = iconDir.idCount;

    if ((*ppIconDirEntries = HeapAlloc(GetProcessHeap(), 0, sizeof(ICONDIRENTRY)*iconDir.idCount)) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    hr = IStream_Read(icoStream, *ppIconDirEntries, sizeof(ICONDIRENTRY)*iconDir.idCount, &bytesRead);
    if (FAILED(hr) || bytesRead != sizeof(ICONDIRENTRY)*iconDir.idCount)
    {
        if (SUCCEEDED(hr)) hr = E_FAIL;
        goto end;
    }

end:
    if (FAILED(hr))
        HeapFree(GetProcessHeap(), 0, *ppIconDirEntries);
    return hr;
}

static HRESULT write_native_icon(IStream *iconStream, const char *icon_name, LPCWSTR szFileName)
{
    ICONDIRENTRY *pIconDirEntry = NULL;
    int numEntries;
    int nMax = 0, nMaxBits = 0;
    int nIndex = 0;
    int i;
    LARGE_INTEGER position;
    HRESULT hr;

    hr = read_ico_direntries(iconStream, &pIconDirEntry, &numEntries);
    if (FAILED(hr))
        goto end;

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
    if (FAILED(hr))
        goto end;
    hr = convert_to_native_icon(iconStream, &nIndex, 1, &CLSID_WICPngEncoder, icon_name, szFileName);

end:
    HeapFree(GetProcessHeap(), 0, pIconDirEntry);
    return hr;
}

static HRESULT open_file_type_icon(LPCWSTR szFileName, IStream **ppStream)
{
    WCHAR *extension;
    WCHAR *icon = NULL;
    WCHAR *comma;
    WCHAR *executable = NULL;
    int index = 0;
    char *output_path = NULL;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    extension = strrchrW(szFileName, '.');
    if (extension == NULL)
        goto end;

    icon = assoc_query(ASSOCSTR_DEFAULTICON, extension, NULL);
    if (icon)
    {
        comma = strrchrW(icon, ',');
        if (comma)
        {
            *comma = 0;
            index = atoiW(comma + 1);
        }
        hr = open_icon(icon, index, FALSE, ppStream);
    }
    else
    {
        executable = assoc_query(ASSOCSTR_EXECUTABLE, extension, NULL);
        if (executable)
            hr = open_icon(executable, 0, FALSE, ppStream);
    }

end:
    HeapFree(GetProcessHeap(), 0, icon);
    HeapFree(GetProcessHeap(), 0, executable);
    HeapFree(GetProcessHeap(), 0, output_path);
    return hr;
}

static HRESULT open_default_icon(IStream **ppStream)
{
    static const WCHAR user32W[] = {'u','s','e','r','3','2',0};

    return open_module_icon(user32W, -(INT_PTR)IDI_WINLOGO, ppStream);
}

static HRESULT open_icon(LPCWSTR filename, int index, BOOL bWait, IStream **ppStream)
{
    HRESULT hr;

    hr = open_module_icon(filename, index, ppStream);
    if (FAILED(hr))
    {
        static const WCHAR dot_icoW[] = {'.','i','c','o',0};
        int len = strlenW(filename);
        if (len >= 4 && strcmpiW(&filename[len - 4], dot_icoW) == 0)
            hr = SHCreateStreamOnFileW(filename, STGM_READ, ppStream);
    }
    if (FAILED(hr))
        hr = open_file_type_icon(filename, ppStream);
    if (FAILED(hr) && !bWait)
        hr = open_default_icon(ppStream);
    return hr;
}

#ifdef __APPLE__
static HRESULT platform_write_icon(IStream *icoStream, int exeIndex,
                                   int bestIndex, LPCWSTR icoPathW,
                                   const char *destFilename, char **nativeIdentifier)
{
    GUID guid;
    WCHAR *guidStrW = NULL;
    char *guidStrA = NULL;
    char *icnsPath = NULL;
    LARGE_INTEGER zero;
    HRESULT hr;

    hr = CoCreateGuid(&guid);
    if (FAILED(hr))
    {
        WINE_WARN("CoCreateGuid failed, error 0x%08X\n", hr);
        goto end;
    }
    hr = StringFromCLSID(&guid, &guidStrW);
    if (FAILED(hr))
    {
        WINE_WARN("StringFromCLSID failed, error 0x%08X\n", hr);
        goto end;
    }
    guidStrA = wchars_to_utf8_chars(guidStrW);
    if (guidStrA == NULL)
    {
        hr = E_OUTOFMEMORY;
        WINE_WARN("out of memory converting GUID string\n");
        goto end;
    }
    icnsPath = heap_printf("/tmp/%s.icns", guidStrA);
    if (icnsPath == NULL)
    {
        hr = E_OUTOFMEMORY;
        WINE_WARN("out of memory creating ICNS path\n");
        goto end;
    }
    zero.QuadPart = 0;
    hr = IStream_Seek(icoStream, zero, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        WINE_WARN("seeking icon stream failed, error 0x%08X\n", hr);
        goto end;
    }
    hr = convert_to_native_icon(icoStream, &bestIndex, 1, &CLSID_WICIcnsEncoder,
                                icnsPath, icoPathW);
    if (FAILED(hr))
    {
        WINE_WARN("converting %s to %s failed, error 0x%08X\n",
            wine_dbgstr_w(icoPathW), wine_dbgstr_a(icnsPath), hr);
        goto end;
    }

end:
    CoTaskMemFree(guidStrW);
    HeapFree(GetProcessHeap(), 0, guidStrA);
    if (SUCCEEDED(hr))
        *nativeIdentifier = icnsPath;
    else
        HeapFree(GetProcessHeap(), 0, icnsPath);
    return hr;
}
#else
static HRESULT platform_write_icon(IStream *icoStream, int exeIndex,
                                   int bestIndex, LPCWSTR icoPathW,
                                   const char *destFilename, char **nativeIdentifier)
{
    char *icoPathA = NULL;
    char *iconsDir = NULL;
    char *pngPath = NULL;
    unsigned short crc;
    char *p, *q;
    HRESULT hr = S_OK;
    LARGE_INTEGER zero;

    icoPathA = wchars_to_utf8_chars(icoPathW);
    if (icoPathA == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    crc = crc16(icoPathA);
    p = strrchr(icoPathA, '\\');
    if (p == NULL)
        p = icoPathA;
    else
    {
        *p = 0;
        p++;
    }
    q = strrchr(p, '.');
    if (q)
        *q = 0;
    if (destFilename)
        *nativeIdentifier = heap_printf("%s", destFilename);
    else
        *nativeIdentifier = heap_printf("%04X_%s.%d", crc, p, exeIndex);
    if (*nativeIdentifier == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    iconsDir = heap_printf("%s/icons", xdg_data_dir);
    if (iconsDir == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    create_directories(iconsDir);
    pngPath = heap_printf("%s/%s.png", iconsDir, *nativeIdentifier);
    if (pngPath == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    zero.QuadPart = 0;
    hr = IStream_Seek(icoStream, zero, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    hr = convert_to_native_icon(icoStream, &bestIndex, 1, &CLSID_WICPngEncoder,
                                pngPath, icoPathW);

end:
    HeapFree(GetProcessHeap(), 0, icoPathA);
    HeapFree(GetProcessHeap(), 0, iconsDir);
    HeapFree(GetProcessHeap(), 0, pngPath);
    return hr;
}
#endif /* defined(__APPLE__) */

/* extract an icon from an exe or icon file; helper for IPersistFile_fnSave */
static char *extract_icon(LPCWSTR icoPathW, int index, const char *destFilename, BOOL bWait)
{
    IStream *stream = NULL;
    HRESULT hr;
    char *nativeIdentifier = NULL;
    ICONDIRENTRY *iconDirEntries = NULL;
    int numEntries;
    int bestIndex = -1;
    int maxPixels = 0;
    int maxBits = 0;
    int i;

    WINE_TRACE("path=[%s] index=%d destFilename=[%s]\n", wine_dbgstr_w(icoPathW), index, wine_dbgstr_a(destFilename));

    hr = open_icon(icoPathW, index, bWait, &stream);
    if (FAILED(hr))
    {
        WINE_WARN("opening icon %s index %d failed, hr=0x%08X\n", wine_dbgstr_w(icoPathW), index, hr);
        goto end;
    }
    hr = read_ico_direntries(stream, &iconDirEntries, &numEntries);
    if (FAILED(hr))
        goto end;
    for (i = 0; i < numEntries; i++)
    {
        WINE_TRACE("[%d]: %d x %d @ %d\n", i, iconDirEntries[i].bWidth,
            iconDirEntries[i].bHeight, iconDirEntries[i].wBitCount);
        if (iconDirEntries[i].wBitCount >= maxBits &&
            (iconDirEntries[i].bHeight * iconDirEntries[i].bWidth) >= maxPixels)
        {
            bestIndex = i;
            maxPixels = iconDirEntries[i].bHeight * iconDirEntries[i].bWidth;
            maxBits = iconDirEntries[i].wBitCount;
        }
    }
    WINE_TRACE("Selected: %d\n", bestIndex);
    hr = platform_write_icon(stream, index, bestIndex, icoPathW, destFilename, &nativeIdentifier);
    if (FAILED(hr))
        WINE_WARN("writing icon failed, error 0x%08X\n", hr);

end:
    if (stream)
        IStream_Release(stream);
    HeapFree(GetProcessHeap(), 0, iconDirEntries);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, nativeIdentifier);
        nativeIdentifier = NULL;
    }
    return nativeIdentifier;
}

static HKEY open_menus_reg_key(void)
{
    static const WCHAR Software_Wine_FileOpenAssociationsW[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','M','e','n','u','F','i','l','e','s',0};
    HKEY assocKey;
    DWORD ret;
    ret = RegCreateKeyW(HKEY_CURRENT_USER, Software_Wine_FileOpenAssociationsW, &assocKey);
    if (ret == ERROR_SUCCESS)
        return assocKey;
    SetLastError(ret);
    return NULL;
}

static DWORD register_menus_entry(const char *unix_file, const char *windows_file)
{
    WCHAR *unix_fileW;
    WCHAR *windows_fileW;
    INT size;
    DWORD ret;

    size = MultiByteToWideChar(CP_UNIXCP, 0, unix_file, -1, NULL, 0);
    unix_fileW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (unix_fileW)
    {
        MultiByteToWideChar(CP_UNIXCP, 0, unix_file, -1, unix_fileW, size);
        size = MultiByteToWideChar(CP_UNIXCP, 0, windows_file, -1, NULL, 0);
        windows_fileW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (windows_fileW)
        {
            HKEY hkey;
            MultiByteToWideChar(CP_UNIXCP, 0, windows_file, -1, windows_fileW, size);
            hkey = open_menus_reg_key();
            if (hkey)
            {
                ret = RegSetValueExW(hkey, unix_fileW, 0, REG_SZ, (const BYTE*)windows_fileW,
                    (strlenW(windows_fileW) + 1) * sizeof(WCHAR));
                RegCloseKey(hkey);
            }
            else
                ret = GetLastError();
            HeapFree(GetProcessHeap(), 0, windows_fileW);
        }
        else
            ret = ERROR_NOT_ENOUGH_MEMORY;
        HeapFree(GetProcessHeap(), 0, unix_fileW);
    }
    else
        ret = ERROR_NOT_ENOUGH_MEMORY;
    return ret;
}

static BOOL write_desktop_entry(const char *unix_link, const char *location, const char *linkname,
                                const char *path, const char *args, const char *descr,
                                const char *workdir, const char *icon)
{
    FILE *file;

    WINE_TRACE("(%s,%s,%s,%s,%s,%s,%s,%s)\n", wine_dbgstr_a(unix_link), wine_dbgstr_a(location),
               wine_dbgstr_a(linkname), wine_dbgstr_a(path), wine_dbgstr_a(args),
               wine_dbgstr_a(descr), wine_dbgstr_a(workdir), wine_dbgstr_a(icon));

    file = fopen(location, "w");
    if (file == NULL)
        return FALSE;

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Name=%s\n", linkname);
    fprintf(file, "Exec=env WINEPREFIX=\"%s\" wine %s %s\n",
            wine_get_config_dir(), path, args);
    fprintf(file, "Type=Application\n");
    fprintf(file, "StartupNotify=true\n");
    if (descr && lstrlenA(descr))
        fprintf(file, "Comment=%s\n", descr);
    if (workdir && lstrlenA(workdir))
        fprintf(file, "Path=%s\n", workdir);
    if (icon && lstrlenA(icon))
        fprintf(file, "Icon=%s\n", icon);

    fclose(file);

    if (unix_link)
    {
        DWORD ret = register_menus_entry(location, unix_link);
        if (ret != ERROR_SUCCESS)
            return FALSE;
    }

    return TRUE;
}

static BOOL write_directory_entry(const char *directory, const char *location)
{
    FILE *file;

    WINE_TRACE("(%s,%s)\n", wine_dbgstr_a(directory), wine_dbgstr_a(location));

    file = fopen(location, "w");
    if (file == NULL)
        return FALSE;

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Type=Directory\n");
    if (strcmp(directory, "wine") == 0)
    {
        fprintf(file, "Name=Wine\n");
        fprintf(file, "Icon=wine\n");
    }
    else
    {
        fprintf(file, "Name=%s\n", directory);
        fprintf(file, "Icon=folder\n");
    }

    fclose(file);
    return TRUE;
}

static BOOL write_menu_file(const char *unix_link, const char *filename)
{
    char *tempfilename;
    FILE *tempfile = NULL;
    char *lastEntry;
    char *name = NULL;
    char *menuPath = NULL;
    int i;
    int count = 0;
    BOOL ret = FALSE;

    WINE_TRACE("(%s)\n", wine_dbgstr_a(filename));

    while (1)
    {
        tempfilename = heap_printf("%s/wine-menu-XXXXXX", xdg_config_dir);
        if (tempfilename)
        {
            int tempfd = mkstemps(tempfilename, 0);
            if (tempfd >= 0)
            {
                tempfile = fdopen(tempfd, "w");
                if (tempfile)
                    break;
                close(tempfd);
                goto end;
            }
            else if (errno == EEXIST)
            {
                HeapFree(GetProcessHeap(), 0, tempfilename);
                continue;
            }
            HeapFree(GetProcessHeap(), 0, tempfilename);
        }
        return FALSE;
    }

    fprintf(tempfile, "<!DOCTYPE Menu PUBLIC \"-//freedesktop//DTD Menu 1.0//EN\"\n");
    fprintf(tempfile, "\"http://www.freedesktop.org/standards/menu-spec/menu-1.0.dtd\">\n");
    fprintf(tempfile, "<Menu>\n");
    fprintf(tempfile, "  <Name>Applications</Name>\n");

    name = HeapAlloc(GetProcessHeap(), 0, lstrlenA(filename) + 1);
    if (name == NULL) goto end;
    lastEntry = name;
    for (i = 0; filename[i]; i++)
    {
        name[i] = filename[i];
        if (filename[i] == '/')
        {
            char *dir_file_name;
            struct stat st;
            name[i] = 0;
            fprintf(tempfile, "  <Menu>\n");
            fprintf(tempfile, "    <Name>%s", count ? "" : "wine-");
            write_xml_text(tempfile, name);
            fprintf(tempfile, "</Name>\n");
            fprintf(tempfile, "    <Directory>%s", count ? "" : "wine-");
            write_xml_text(tempfile, name);
            fprintf(tempfile, ".directory</Directory>\n");
            dir_file_name = heap_printf("%s/desktop-directories/%s%s.directory",
                xdg_data_dir, count ? "" : "wine-", name);
            if (dir_file_name)
            {
                if (stat(dir_file_name, &st) != 0 && errno == ENOENT)
                    write_directory_entry(lastEntry, dir_file_name);
                HeapFree(GetProcessHeap(), 0, dir_file_name);
            }
            name[i] = '-';
            lastEntry = &name[i+1];
            ++count;
        }
    }
    name[i] = 0;

    fprintf(tempfile, "    <Include>\n");
    fprintf(tempfile, "      <Filename>");
    write_xml_text(tempfile, name);
    fprintf(tempfile, "</Filename>\n");
    fprintf(tempfile, "    </Include>\n");
    for (i = 0; i < count; i++)
         fprintf(tempfile, "  </Menu>\n");
    fprintf(tempfile, "</Menu>\n");

    menuPath = heap_printf("%s/%s", xdg_config_dir, name);
    if (menuPath == NULL) goto end;
    strcpy(menuPath + strlen(menuPath) - strlen(".desktop"), ".menu");
    ret = TRUE;

end:
    if (tempfile)
        fclose(tempfile);
    if (ret)
        ret = (rename(tempfilename, menuPath) == 0);
    if (!ret && tempfilename)
        remove(tempfilename);
    HeapFree(GetProcessHeap(), 0, tempfilename);
    if (ret)
        register_menus_entry(menuPath, unix_link);
    HeapFree(GetProcessHeap(), 0, name);
    HeapFree(GetProcessHeap(), 0, menuPath);
    return ret;
}

static BOOL write_menu_entry(const char *unix_link, const char *link, const char *path, const char *args,
                             const char *descr, const char *workdir, const char *icon)
{
    const char *linkname;
    char *desktopPath = NULL;
    char *desktopDir;
    char *filename = NULL;
    BOOL ret = TRUE;

    WINE_TRACE("(%s, %s, %s, %s, %s, %s, %s)\n", wine_dbgstr_a(unix_link), wine_dbgstr_a(link),
               wine_dbgstr_a(path), wine_dbgstr_a(args), wine_dbgstr_a(descr),
               wine_dbgstr_a(workdir), wine_dbgstr_a(icon));

    linkname = strrchr(link, '/');
    if (linkname == NULL)
        linkname = link;
    else
        ++linkname;

    desktopPath = heap_printf("%s/applications/wine/%s.desktop", xdg_data_dir, link);
    if (!desktopPath)
    {
        WINE_WARN("out of memory creating menu entry\n");
        ret = FALSE;
        goto end;
    }
    desktopDir = strrchr(desktopPath, '/');
    *desktopDir = 0;
    if (!create_directories(desktopPath))
    {
        WINE_WARN("couldn't make parent directories for %s\n", wine_dbgstr_a(desktopPath));
        ret = FALSE;
        goto end;
    }
    *desktopDir = '/';
    if (!write_desktop_entry(unix_link, desktopPath, linkname, path, args, descr, workdir, icon))
    {
        WINE_WARN("couldn't make desktop entry %s\n", wine_dbgstr_a(desktopPath));
        ret = FALSE;
        goto end;
    }

    filename = heap_printf("wine/%s.desktop", link);
    if (!filename || !write_menu_file(unix_link, filename))
    {
        WINE_WARN("couldn't make menu file %s\n", wine_dbgstr_a(filename));
        ret = FALSE;
    }

end:
    HeapFree(GetProcessHeap(), 0, desktopPath);
    HeapFree(GetProcessHeap(), 0, filename);
    return ret;
}

/* This escapes reserved characters in .desktop files' Exec keys. */
static LPSTR escape(LPCWSTR arg)
{
    int i, j;
    WCHAR *escaped_string;
    char *utf8_string;

    escaped_string = HeapAlloc(GetProcessHeap(), 0, (4 * strlenW(arg) + 1) * sizeof(WCHAR));
    if (escaped_string == NULL) return NULL;
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
    if (utf8_string == NULL)
    {
        WINE_ERR("out of memory\n");
        goto end;
    }

end:
    HeapFree(GetProcessHeap(), 0, escaped_string);
    return utf8_string;
}

/* Return a heap-allocated copy of the unix format difference between the two
 * Windows-format paths.
 * locn is the owning location
 * link is within locn
 */
static char *relative_path( LPCWSTR link, LPCWSTR locn )
{
    char *unix_locn, *unix_link;
    char *relative = NULL;

    unix_locn = wine_get_unix_file_name(locn);
    unix_link = wine_get_unix_file_name(link);
    if (unix_locn && unix_link)
    {
        size_t len_unix_locn, len_unix_link;
        len_unix_locn = strlen (unix_locn);
        len_unix_link = strlen (unix_link);
        if (len_unix_locn < len_unix_link && memcmp (unix_locn, unix_link, len_unix_locn) == 0 && unix_link[len_unix_locn] == '/')
        {
            size_t len_rel;
            char *p = strrchr (unix_link + len_unix_locn, '/');
            p = strrchr (p, '.');
            if (p)
            {
                *p = '\0';
                len_unix_link = p - unix_link;
            }
            len_rel = len_unix_link - len_unix_locn;
            relative = HeapAlloc(GetProcessHeap(), 0, len_rel);
            if (relative)
            {
                memcpy (relative, unix_link + len_unix_locn + 1, len_rel);
            }
        }
    }
    if (!relative)
        WINE_WARN("Could not separate the relative link path of %s in %s\n", wine_dbgstr_w(link), wine_dbgstr_w(locn));
    HeapFree(GetProcessHeap(), 0, unix_locn);
    HeapFree(GetProcessHeap(), 0, unix_link);
    return relative;
}

/***********************************************************************
 *
 *           GetLinkLocation
 *
 * returns TRUE if successful
 * *loc will contain CS_DESKTOPDIRECTORY, CS_STARTMENU, CS_STARTUP etc.
 * *relative will contain the address of a heap-allocated copy of the portion
 * of the filename that is within the specified location, in unix form
 */
static BOOL GetLinkLocation( LPCWSTR linkfile, DWORD *loc, char **relative )
{
    WCHAR filename[MAX_PATH], shortfilename[MAX_PATH], buffer[MAX_PATH];
    DWORD len, i, r, filelen;
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

    for( i=0; i<sizeof(locations)/sizeof(locations[0]); i++ )
    {
        if (!SHGetSpecialFolderPathW( 0, buffer, locations[i], FALSE ))
            continue;

        len = lstrlenW(buffer);
        if (len >= MAX_PATH)
            continue; /* We've just trashed memory! Hopefully we are OK */

        if (len > filelen || filename[len]!='\\')
            continue;
        /* do a lstrcmpinW */
        filename[len] = 0;
        r = lstrcmpiW( filename, buffer );
        filename[len] = '\\';
        if ( r )
            continue;

        /* return the remainder of the string and link type */
        *loc = locations[i];
        *relative = relative_path (filename, buffer);
        return (*relative != NULL);
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
            szCmdline = HeapAlloc( GetProcessHeap(), 0, cmdSize*sizeof(WCHAR) );
            hr = CommandLineFromMsiDescriptor( dar->szwDarwinID, szCmdline, &cmdSize );
            WINE_TRACE("      command    : %s\n", wine_dbgstr_w(szCmdline));
            if (hr == ERROR_SUCCESS)
            {
                WCHAR *s, *d;
                int bcount, in_quotes;

                /* Extract the application path */
                bcount=0;
                in_quotes=0;
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
            HeapFree( GetProcessHeap(), 0, szCmdline );
        }
        LocalFree( dar );
    }

    IShellLinkDataList_Release( dl );
    return hr;
}

static WCHAR* assoc_query(ASSOCSTR assocStr, LPCWSTR name, LPCWSTR extra)
{
    HRESULT hr;
    WCHAR *value = NULL;
    DWORD size = 0;
    hr = AssocQueryStringW(0, assocStr, name, extra, NULL, &size);
    if (SUCCEEDED(hr))
    {
        value = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (value)
        {
            hr = AssocQueryStringW(0, assocStr, name, extra, value, &size);
            if (FAILED(hr))
            {
                HeapFree(GetProcessHeap(), 0, value);
                value = NULL;
            }
        }
    }
    return value;
}

static char *slashes_to_minuses(const char *string)
{
    int i;
    char *ret = HeapAlloc(GetProcessHeap(), 0, lstrlenA(string) + 1);
    if (ret)
    {
        for (i = 0; string[i]; i++)
        {
            if (string[i] == '/')
                ret[i] = '-';
            else
                ret[i] = string[i];
        }
        ret[i] = 0;
        return ret;
    }
    return NULL;
}

static BOOL next_line(FILE *file, char **line, int *size)
{
    int pos = 0;
    char *cr;
    if (*line == NULL)
    {
        *size = 4096;
        *line = HeapAlloc(GetProcessHeap(), 0, *size);
    }
    while (*line != NULL)
    {
        if (fgets(&(*line)[pos], *size - pos, file) == NULL)
        {
            HeapFree(GetProcessHeap(), 0, *line);
            *line = NULL;
            if (feof(file))
                return TRUE;
            return FALSE;
        }
        pos = strlen(*line);
        cr = strchr(*line, '\n');
        if (cr == NULL)
        {
            char *line2;
            (*size) *= 2;
            line2 = HeapReAlloc(GetProcessHeap(), 0, *line, *size);
            if (line2)
                *line = line2;
            else
            {
                HeapFree(GetProcessHeap(), 0, *line);
                *line = NULL;
            }
        }
        else
        {
            *cr = 0;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL add_mimes(const char *xdg_data_dir, struct list *mime_types)
{
    char *globs_filename = NULL;
    BOOL ret = TRUE;
    globs_filename = heap_printf("%s/mime/globs", xdg_data_dir);
    if (globs_filename)
    {
        FILE *globs_file = fopen(globs_filename, "r");
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
                    mime_type_entry = HeapAlloc(GetProcessHeap(), 0, sizeof(struct xdg_mime_type));
                    if (mime_type_entry)
                    {
                        *pos = 0;
                        mime_type_entry->mimeType = strdupA(line);
                        mime_type_entry->glob = strdupA(pos + 1);
                        if (mime_type_entry->mimeType && mime_type_entry->glob)
                            list_add_tail(mime_types, &mime_type_entry->entry);
                        else
                        {
                            HeapFree(GetProcessHeap(), 0, mime_type_entry->mimeType);
                            HeapFree(GetProcessHeap(), 0, mime_type_entry->glob);
                            HeapFree(GetProcessHeap(), 0, mime_type_entry);
                            ret = FALSE;
                        }
                    }
                    else
                        ret = FALSE;
                }
            }
            HeapFree(GetProcessHeap(), 0, line);
            fclose(globs_file);
        }
        HeapFree(GetProcessHeap(), 0, globs_filename);
    }
    else
        ret = FALSE;
    return ret;
}

static void free_native_mime_types(struct list *native_mime_types)
{
    struct xdg_mime_type *mime_type_entry, *mime_type_entry2;

    LIST_FOR_EACH_ENTRY_SAFE(mime_type_entry, mime_type_entry2, native_mime_types, struct xdg_mime_type, entry)
    {
        list_remove(&mime_type_entry->entry);
        HeapFree(GetProcessHeap(), 0, mime_type_entry->glob);
        HeapFree(GetProcessHeap(), 0, mime_type_entry->mimeType);
        HeapFree(GetProcessHeap(), 0, mime_type_entry);
    }
    HeapFree(GetProcessHeap(), 0, native_mime_types);
}

static BOOL build_native_mime_types(const char *xdg_data_home, struct list **mime_types)
{
    char *xdg_data_dirs;
    BOOL ret;

    *mime_types = NULL;

    xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs == NULL)
        xdg_data_dirs = heap_printf("/usr/local/share/:/usr/share/");
    else
        xdg_data_dirs = strdupA(xdg_data_dirs);

    if (xdg_data_dirs)
    {
        *mime_types = HeapAlloc(GetProcessHeap(), 0, sizeof(struct list));
        if (*mime_types)
        {
            const char *begin;
            char *end;

            list_init(*mime_types);
            ret = add_mimes(xdg_data_home, *mime_types);
            if (ret)
            {
                for (begin = xdg_data_dirs; (end = strchr(begin, ':')); begin = end + 1)
                {
                    *end = '\0';
                    ret = add_mimes(begin, *mime_types);
                    *end = ':';
                    if (!ret)
                        break;
                }
                if (ret)
                    ret = add_mimes(begin, *mime_types);
            }
        }
        else
            ret = FALSE;
        HeapFree(GetProcessHeap(), 0, xdg_data_dirs);
    }
    else
        ret = FALSE;
    if (!ret && *mime_types)
    {
        free_native_mime_types(*mime_types);
        *mime_types = NULL;
    }
    return ret;
}

static BOOL match_glob(struct list *native_mime_types, const char *extension,
                       char **match)
{
#ifdef HAVE_FNMATCH
    struct xdg_mime_type *mime_type_entry;
    int matchLength = 0;

    *match = NULL;

    LIST_FOR_EACH_ENTRY(mime_type_entry, native_mime_types, struct xdg_mime_type, entry)
    {
        if (fnmatch(mime_type_entry->glob, extension, 0) == 0)
        {
            if (*match == NULL || matchLength < strlen(mime_type_entry->glob))
            {
                *match = mime_type_entry->mimeType;
                matchLength = strlen(mime_type_entry->glob);
            }
        }
    }

    if (*match != NULL)
    {
        *match = strdupA(*match);
        if (*match == NULL)
            return FALSE;
    }
#else
    *match = NULL;
#endif
    return TRUE;
}

static BOOL freedesktop_mime_type_for_extension(struct list *native_mime_types,
                                                const char *extensionA,
                                                LPCWSTR extensionW,
                                                char **mime_type)
{
    WCHAR *lower_extensionW;
    INT len;
    BOOL ret = match_glob(native_mime_types, extensionA, mime_type);
    if (ret == FALSE || *mime_type != NULL)
        return ret;
    len = strlenW(extensionW);
    lower_extensionW = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
    if (lower_extensionW)
    {
        char *lower_extensionA;
        memcpy(lower_extensionW, extensionW, (len + 1)*sizeof(WCHAR));
        strlwrW(lower_extensionW);
        lower_extensionA = wchars_to_utf8_chars(lower_extensionW);
        if (lower_extensionA)
        {
            ret = match_glob(native_mime_types, lower_extensionA, mime_type);
            HeapFree(GetProcessHeap(), 0, lower_extensionA);
        }
        else
        {
            ret = FALSE;
            WINE_FIXME("out of memory\n");
        }
        HeapFree(GetProcessHeap(), 0, lower_extensionW);
    }
    else
    {
        ret = FALSE;
        WINE_FIXME("out of memory\n");
    }
    return ret;
}

static WCHAR* reg_get_valW(HKEY key, LPCWSTR subkey, LPCWSTR name)
{
    DWORD size;
    if (RegGetValueW(key, subkey, name, RRF_RT_REG_SZ, NULL, NULL, &size) == ERROR_SUCCESS)
    {
        WCHAR *ret = HeapAlloc(GetProcessHeap(), 0, size);
        if (ret)
        {
            if (RegGetValueW(key, subkey, name, RRF_RT_REG_SZ, NULL, ret, &size) == ERROR_SUCCESS)
                return ret;
        }
        HeapFree(GetProcessHeap(), 0, ret);
    }
    return NULL;
}

static CHAR* reg_get_val_utf8(HKEY key, LPCWSTR subkey, LPCWSTR name)
{
    WCHAR *valW = reg_get_valW(key, subkey, name);
    if (valW)
    {
        char *val = wchars_to_utf8_chars(valW);
        HeapFree(GetProcessHeap(), 0, valW);
        return val;
    }
    return NULL;
}

static HKEY open_associations_reg_key(void)
{
    static const WCHAR Software_Wine_FileOpenAssociationsW[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','F','i','l','e','O','p','e','n','A','s','s','o','c','i','a','t','i','o','n','s',0};
    HKEY assocKey;
    if (RegCreateKeyW(HKEY_CURRENT_USER, Software_Wine_FileOpenAssociationsW, &assocKey) == ERROR_SUCCESS)
        return assocKey;
    return NULL;
}

static BOOL has_association_changed(LPCWSTR extensionW, LPCSTR mimeType, LPCWSTR progId, LPCSTR appName, LPCWSTR docName)
{
    static const WCHAR ProgIDW[] = {'P','r','o','g','I','D',0};
    static const WCHAR DocNameW[] = {'D','o','c','N','a','m','e',0};
    static const WCHAR MimeTypeW[] = {'M','i','m','e','T','y','p','e',0};
    static const WCHAR AppNameW[] = {'A','p','p','N','a','m','e',0};
    HKEY assocKey;
    BOOL ret;

    if ((assocKey = open_associations_reg_key()))
    {
        CHAR *valueA;
        WCHAR *value;

        ret = FALSE;

        valueA = reg_get_val_utf8(assocKey, extensionW, MimeTypeW);
        if (!valueA || lstrcmpA(valueA, mimeType))
            ret = TRUE;
        HeapFree(GetProcessHeap(), 0, valueA);

        value = reg_get_valW(assocKey, extensionW, ProgIDW);
        if (!value || strcmpW(value, progId))
            ret = TRUE;
        HeapFree(GetProcessHeap(), 0, value);

        valueA = reg_get_val_utf8(assocKey, extensionW, AppNameW);
        if (!valueA || lstrcmpA(valueA, appName))
            ret = TRUE;
        HeapFree(GetProcessHeap(), 0, valueA);

        value = reg_get_valW(assocKey, extensionW, DocNameW);
        if (docName && (!value || strcmpW(value, docName)))
            ret = TRUE;
        HeapFree(GetProcessHeap(), 0, value);

        RegCloseKey(assocKey);
    }
    else
    {
        WINE_ERR("error opening associations registry key\n");
        ret = FALSE;
    }
    return ret;
}

static void update_association(LPCWSTR extension, LPCSTR mimeType, LPCWSTR progId, LPCSTR appName, LPCWSTR docName, LPCSTR desktopFile)
{
    static const WCHAR ProgIDW[] = {'P','r','o','g','I','D',0};
    static const WCHAR DocNameW[] = {'D','o','c','N','a','m','e',0};
    static const WCHAR MimeTypeW[] = {'M','i','m','e','T','y','p','e',0};
    static const WCHAR AppNameW[] = {'A','p','p','N','a','m','e',0};
    static const WCHAR DesktopFileW[] = {'D','e','s','k','t','o','p','F','i','l','e',0};
    HKEY assocKey = NULL;
    HKEY subkey = NULL;
    WCHAR *mimeTypeW = NULL;
    WCHAR *appNameW = NULL;
    WCHAR *desktopFileW = NULL;

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

    mimeTypeW = utf8_chars_to_wchars(mimeType);
    if (mimeTypeW == NULL)
    {
        WINE_ERR("out of memory\n");
        goto done;
    }

    appNameW = utf8_chars_to_wchars(appName);
    if (appNameW == NULL)
    {
        WINE_ERR("out of memory\n");
        goto done;
    }

    desktopFileW = utf8_chars_to_wchars(desktopFile);
    if (desktopFileW == NULL)
    {
        WINE_ERR("out of memory\n");
        goto done;
    }

    RegSetValueExW(subkey, MimeTypeW, 0, REG_SZ, (const BYTE*) mimeTypeW, (lstrlenW(mimeTypeW) + 1) * sizeof(WCHAR));
    RegSetValueExW(subkey, ProgIDW, 0, REG_SZ, (const BYTE*) progId, (lstrlenW(progId) + 1) * sizeof(WCHAR));
    RegSetValueExW(subkey, AppNameW, 0, REG_SZ, (const BYTE*) appNameW, (lstrlenW(appNameW) + 1) * sizeof(WCHAR));
    if (docName)
        RegSetValueExW(subkey, DocNameW, 0, REG_SZ, (const BYTE*) docName, (lstrlenW(docName) + 1) * sizeof(WCHAR));
    RegSetValueExW(subkey, DesktopFileW, 0, REG_SZ, (const BYTE*) desktopFileW, (lstrlenW(desktopFileW) + 1) * sizeof(WCHAR));

done:
    RegCloseKey(assocKey);
    RegCloseKey(subkey);
    HeapFree(GetProcessHeap(), 0, mimeTypeW);
    HeapFree(GetProcessHeap(), 0, appNameW);
    HeapFree(GetProcessHeap(), 0, desktopFileW);
}

static BOOL cleanup_associations(void)
{
    static const WCHAR openW[] = {'o','p','e','n',0};
    static const WCHAR DesktopFileW[] = {'D','e','s','k','t','o','p','F','i','l','e',0};
    HKEY assocKey;
    BOOL hasChanged = FALSE;
    if ((assocKey = open_associations_reg_key()))
    {
        int i;
        BOOL done = FALSE;
        for (i = 0; !done; i++)
        {
            WCHAR *extensionW = NULL;
            DWORD size = 1024;
            LSTATUS ret;

            do
            {
                HeapFree(GetProcessHeap(), 0, extensionW);
                extensionW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
                if (extensionW == NULL)
                {
                    WINE_ERR("out of memory\n");
                    ret = ERROR_OUTOFMEMORY;
                    break;
                }
                ret = RegEnumKeyExW(assocKey, i, extensionW, &size, NULL, NULL, NULL, NULL);
                size *= 2;
            } while (ret == ERROR_MORE_DATA);

            if (ret == ERROR_SUCCESS)
            {
                WCHAR *command;
                command = assoc_query(ASSOCSTR_COMMAND, extensionW, openW);
                if (command == NULL)
                {
                    char *desktopFile = reg_get_val_utf8(assocKey, extensionW, DesktopFileW);
                    if (desktopFile)
                    {
                        WINE_TRACE("removing file type association for %s\n", wine_dbgstr_w(extensionW));
                        remove(desktopFile);
                    }
                    RegDeleteKeyW(assocKey, extensionW);
                    hasChanged = TRUE;
                    HeapFree(GetProcessHeap(), 0, desktopFile);
                }
                HeapFree(GetProcessHeap(), 0, command);
            }
            else
            {
                if (ret != ERROR_NO_MORE_ITEMS)
                    WINE_ERR("error %d while reading registry\n", ret);
                done = TRUE;
            }
            HeapFree(GetProcessHeap(), 0, extensionW);
        }
        RegCloseKey(assocKey);
    }
    else
        WINE_ERR("could not open file associations key\n");
    return hasChanged;
}

static BOOL write_freedesktop_mime_type_entry(const char *packages_dir, const char *dot_extension,
                                              const char *mime_type, const char *comment)
{
    BOOL ret = FALSE;
    char *filename;

    WINE_TRACE("writing MIME type %s, extension=%s, comment=%s\n", wine_dbgstr_a(mime_type),
               wine_dbgstr_a(dot_extension), wine_dbgstr_a(comment));

    filename = heap_printf("%s/x-wine-extension-%s.xml", packages_dir, &dot_extension[1]);
    if (filename)
    {
        FILE *packageFile = fopen(filename, "w");
        if (packageFile)
        {
            fprintf(packageFile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            fprintf(packageFile, "<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n");
            fprintf(packageFile, "  <mime-type type=\"");
            write_xml_text(packageFile, mime_type);
            fprintf(packageFile, "\">\n");
            fprintf(packageFile, "    <glob pattern=\"*");
            write_xml_text(packageFile, dot_extension);
            fprintf(packageFile, "\"/>\n");
            if (comment)
            {
                fprintf(packageFile, "    <comment>");
                write_xml_text(packageFile, comment);
                fprintf(packageFile, "</comment>\n");
            }
            fprintf(packageFile, "  </mime-type>\n");
            fprintf(packageFile, "</mime-info>\n");
            ret = TRUE;
            fclose(packageFile);
        }
        else
            WINE_ERR("error writing file %s\n", filename);
        HeapFree(GetProcessHeap(), 0, filename);
    }
    else
        WINE_ERR("out of memory\n");
    return ret;
}

static BOOL is_extension_blacklisted(LPCWSTR extension)
{
    /* These are managed through external tools like wine.desktop, to evade malware created file type associations */
    static const WCHAR comW[] = {'.','c','o','m',0};
    static const WCHAR exeW[] = {'.','e','x','e',0};
    static const WCHAR msiW[] = {'.','m','s','i',0};

    if (!strcmpiW(extension, comW) ||
        !strcmpiW(extension, exeW) ||
        !strcmpiW(extension, msiW))
        return TRUE;
    return FALSE;
}

static const char* get_special_mime_type(LPCWSTR extension)
{
    static const WCHAR lnkW[] = {'.','l','n','k',0};
    if (!strcmpiW(extension, lnkW))
        return "application/x-ms-shortcut";
    return NULL;
}

static BOOL write_freedesktop_association_entry(const char *desktopPath, const char *dot_extension,
                                                const char *friendlyAppName, const char *mimeType,
                                                const char *progId)
{
    BOOL ret = FALSE;
    FILE *desktop;

    WINE_TRACE("writing association for file type %s, friendlyAppName=%s, MIME type %s, progID=%s, to file %s\n",
               wine_dbgstr_a(dot_extension), wine_dbgstr_a(friendlyAppName), wine_dbgstr_a(mimeType),
               wine_dbgstr_a(progId), wine_dbgstr_a(desktopPath));

    desktop = fopen(desktopPath, "w");
    if (desktop)
    {
        fprintf(desktop, "[Desktop Entry]\n");
        fprintf(desktop, "Type=Application\n");
        fprintf(desktop, "Name=%s\n", friendlyAppName);
        fprintf(desktop, "MimeType=%s\n", mimeType);
        fprintf(desktop, "Exec=wine start /ProgIDOpen %s %%f\n", progId);
        fprintf(desktop, "NoDisplay=true\n");
        fprintf(desktop, "StartupNotify=true\n");
        ret = TRUE;
        fclose(desktop);
    }
    else
        WINE_ERR("error writing association file %s\n", wine_dbgstr_a(desktopPath));
    return ret;
}

static BOOL generate_associations(const char *xdg_data_home, const char *packages_dir, const char *applications_dir)
{
    static const WCHAR openW[] = {'o','p','e','n',0};
    struct list *nativeMimeTypes = NULL;
    LSTATUS ret = 0;
    int i;
    BOOL hasChanged = FALSE;

    if (!build_native_mime_types(xdg_data_home, &nativeMimeTypes))
    {
        WINE_ERR("could not build native MIME types\n");
        return FALSE;
    }

    for (i = 0; ; i++)
    {
        WCHAR *extensionW = NULL;
        DWORD size = 1024;

        do
        {
            HeapFree(GetProcessHeap(), 0, extensionW);
            extensionW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
            if (extensionW == NULL)
            {
                WINE_ERR("out of memory\n");
                ret = ERROR_OUTOFMEMORY;
                break;
            }
            ret = RegEnumKeyExW(HKEY_CLASSES_ROOT, i, extensionW, &size, NULL, NULL, NULL, NULL);
            size *= 2;
        } while (ret == ERROR_MORE_DATA);

        if (ret == ERROR_SUCCESS && extensionW[0] == '.' && !is_extension_blacklisted(extensionW))
        {
            char *extensionA = NULL;
            WCHAR *commandW = NULL;
            WCHAR *friendlyDocNameW = NULL;
            char *friendlyDocNameA = NULL;
            WCHAR *iconW = NULL;
            char *iconA = NULL;
            WCHAR *contentTypeW = NULL;
            char *mimeTypeA = NULL;
            WCHAR *friendlyAppNameW = NULL;
            char *friendlyAppNameA = NULL;
            WCHAR *progIdW = NULL;
            char *progIdA = NULL;

            extensionA = wchars_to_utf8_chars(extensionW);
            if (extensionA == NULL)
            {
                WINE_ERR("out of memory\n");
                goto end;
            }

            friendlyDocNameW = assoc_query(ASSOCSTR_FRIENDLYDOCNAME, extensionW, NULL);
            if (friendlyDocNameW)
            {
                friendlyDocNameA = wchars_to_utf8_chars(friendlyDocNameW);
                if (friendlyDocNameA == NULL)
                {
                    WINE_ERR("out of memory\n");
                    goto end;
                }
            }

            iconW = assoc_query(ASSOCSTR_DEFAULTICON, extensionW, NULL);

            contentTypeW = assoc_query(ASSOCSTR_CONTENTTYPE, extensionW, NULL);
            if (contentTypeW)
                strlwrW(contentTypeW);

            if (!freedesktop_mime_type_for_extension(nativeMimeTypes, extensionA, extensionW, &mimeTypeA))
                goto end;

            if (mimeTypeA == NULL)
            {
                if (contentTypeW != NULL && strchrW(contentTypeW, '/'))
                    mimeTypeA = wchars_to_utf8_chars(contentTypeW);
                else if ((get_special_mime_type(extensionW)))
                    mimeTypeA = strdupA(get_special_mime_type(extensionW));
                else
                    mimeTypeA = heap_printf("application/x-wine-extension-%s", &extensionA[1]);

                if (mimeTypeA != NULL)
                {
                    /* Gnome seems to ignore the <icon> tag in MIME packages,
                     * and the default name is more intuitive anyway.
                     */
                    if (iconW)
                    {
                        char *flattened_mime = slashes_to_minuses(mimeTypeA);
                        if (flattened_mime)
                        {
                            int index = 0;
                            WCHAR *comma = strrchrW(iconW, ',');
                            if (comma)
                            {
                                *comma = 0;
                                index = atoiW(comma + 1);
                            }
                            iconA = extract_icon(iconW, index, flattened_mime, FALSE);
                            HeapFree(GetProcessHeap(), 0, flattened_mime);
                        }
                    }

                    write_freedesktop_mime_type_entry(packages_dir, extensionA, mimeTypeA, friendlyDocNameA);
                    hasChanged = TRUE;
                }
                else
                {
                    WINE_FIXME("out of memory\n");
                    goto end;
                }
            }

            commandW = assoc_query(ASSOCSTR_COMMAND, extensionW, openW);
            if (commandW == NULL)
                /* no command => no application is associated */
                goto end;

            friendlyAppNameW = assoc_query(ASSOCSTR_FRIENDLYAPPNAME, extensionW, NULL);
            if (friendlyAppNameW)
            {
                friendlyAppNameA = wchars_to_utf8_chars(friendlyAppNameW);
                if (friendlyAppNameA == NULL)
                {
                    WINE_ERR("out of memory\n");
                    goto end;
                }
            }
            else
            {
                friendlyAppNameA = heap_printf("A Wine application");
                if (friendlyAppNameA == NULL)
                {
                    WINE_ERR("out of memory\n");
                    goto end;
                }
            }

            progIdW = reg_get_valW(HKEY_CLASSES_ROOT, extensionW, NULL);
            if (progIdW)
            {
                progIdA = escape(progIdW);
                if (progIdA == NULL)
                {
                    WINE_ERR("out of memory\n");
                    goto end;
                }
            }
            else
                goto end; /* no progID => not a file type association */

            if (has_association_changed(extensionW, mimeTypeA, progIdW, friendlyAppNameA, friendlyDocNameW))
            {
                char *desktopPath = heap_printf("%s/wine-extension-%s.desktop", applications_dir, &extensionA[1]);
                if (desktopPath)
                {
                    if (write_freedesktop_association_entry(desktopPath, extensionA, friendlyAppNameA, mimeTypeA, progIdA))
                    {
                        hasChanged = TRUE;
                        update_association(extensionW, mimeTypeA, progIdW, friendlyAppNameA, friendlyDocNameW, desktopPath);
                    }
                    HeapFree(GetProcessHeap(), 0, desktopPath);
                }
            }

        end:
            HeapFree(GetProcessHeap(), 0, extensionA);
            HeapFree(GetProcessHeap(), 0, commandW);
            HeapFree(GetProcessHeap(), 0, friendlyDocNameW);
            HeapFree(GetProcessHeap(), 0, friendlyDocNameA);
            HeapFree(GetProcessHeap(), 0, iconW);
            HeapFree(GetProcessHeap(), 0, iconA);
            HeapFree(GetProcessHeap(), 0, contentTypeW);
            HeapFree(GetProcessHeap(), 0, mimeTypeA);
            HeapFree(GetProcessHeap(), 0, friendlyAppNameW);
            HeapFree(GetProcessHeap(), 0, friendlyAppNameA);
            HeapFree(GetProcessHeap(), 0, progIdW);
            HeapFree(GetProcessHeap(), 0, progIdA);
        }
        HeapFree(GetProcessHeap(), 0, extensionW);
        if (ret != ERROR_SUCCESS)
            break;
    }

    free_native_mime_types(nativeMimeTypes);
    return hasChanged;
}

static char *get_start_exe_path(void)
 {
    static const WCHAR startW[] = {'\\','c','o','m','m','a','n','d',
                                   '\\','s','t','a','r','t','.','e','x','e',0};
    WCHAR start_path[MAX_PATH];
    GetWindowsDirectoryW(start_path, MAX_PATH);
    lstrcatW(start_path, startW);
    return escape(start_path);
}

static char* escape_unix_link_arg(LPCSTR unix_link)
{
    char *ret = NULL;
    WCHAR *unix_linkW = utf8_chars_to_wchars(unix_link);
    if (unix_linkW)
    {
        char *escaped_lnk = escape(unix_linkW);
        if (escaped_lnk)
        {
            ret = heap_printf("/Unix %s", escaped_lnk);
            HeapFree(GetProcessHeap(), 0, escaped_lnk);
        }
        HeapFree(GetProcessHeap(), 0, unix_linkW);
    }
    return ret;
}

static BOOL InvokeShellLinker( IShellLinkW *sl, LPCWSTR link, BOOL bWait )
{
    static const WCHAR startW[] = {'\\','c','o','m','m','a','n','d',
                                   '\\','s','t','a','r','t','.','e','x','e',0};
    char *link_name = NULL, *icon_name = NULL, *work_dir = NULL;
    char *escaped_path = NULL, *escaped_args = NULL, *description = NULL;
    WCHAR szTmp[INFOTIPSIZE];
    WCHAR szDescription[INFOTIPSIZE], szPath[MAX_PATH], szWorkDir[MAX_PATH];
    WCHAR szArgs[INFOTIPSIZE], szIconPath[MAX_PATH];
    int iIconId = 0, r = -1;
    DWORD csidl = -1;
    HANDLE hsem = NULL;
    char *unix_link = NULL;
    char *start_path = NULL;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return FALSE;
    }

    if( !GetLinkLocation( link, &csidl, &link_name ) )
    {
        WINE_WARN("Unknown link location %s. Ignoring.\n",wine_dbgstr_w(link));
        return TRUE;
    }
    if (!in_desktop_dir(csidl) && !in_startmenu(csidl))
    {
        WINE_WARN("Not under desktop or start menu. Ignoring.\n");
        return TRUE;
    }
    WINE_TRACE("Link       : %s\n", wine_dbgstr_a(link_name));

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

    unix_link = wine_get_unix_file_name(link);
    if (unix_link == NULL)
    {
        WINE_WARN("couldn't find unix path of %s\n", wine_dbgstr_w(link));
        goto cleanup;
    }

    /* check the path */
    if( szPath[0] )
    {
        static const WCHAR exeW[] = {'.','e','x','e',0};
        WCHAR *p;

        /* check for .exe extension */
        if (!(p = strrchrW( szPath, '.' )) ||
            strchrW( p, '\\' ) || strchrW( p, '/' ) ||
            lstrcmpiW( p, exeW ))
        {
            /* Not .exe - use 'start.exe' to launch this file */
            p = szArgs + lstrlenW(szPath) + 2;
            if (szArgs[0])
            {
                p[0] = ' ';
                memmove( p+1, szArgs, min( (lstrlenW(szArgs) + 1) * sizeof(szArgs[0]),
                                           sizeof(szArgs) - (p + 1 - szArgs) * sizeof(szArgs[0]) ) );
            }
            else
                p[0] = 0;

            szArgs[0] = '"';
            lstrcpyW(szArgs + 1, szPath);
            p[-1] = '"';

            GetWindowsDirectoryW(szPath, MAX_PATH);
            lstrcatW(szPath, startW);
        }

        /* convert app working dir */
        if (szWorkDir[0])
            work_dir = wine_get_unix_file_name( szWorkDir );
    }
    else
    {
        /* if there's no path... try run the link itself */
        lstrcpynW(szArgs, link, MAX_PATH);
        GetWindowsDirectoryW(szPath, MAX_PATH);
        lstrcatW(szPath, startW);
    }

    /* escape the path and parameters */
    escaped_path = escape(szPath);
    escaped_args = escape(szArgs);
    description = wchars_to_utf8_chars(szDescription);
    if (escaped_path == NULL || escaped_args == NULL || description == NULL)
    {
        WINE_ERR("out of memory allocating/escaping parameters\n");
        goto cleanup;
    }

    start_path = get_start_exe_path();
    if (start_path == NULL)
    {
        WINE_ERR("out of memory\n");
        goto cleanup;
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
        char *location;
        const char *lastEntry;
        lastEntry = strrchr(link_name, '/');
        if (lastEntry == NULL)
            lastEntry = link_name;
        else
            ++lastEntry;
        location = heap_printf("%s/%s.desktop", xdg_desktop_dir, lastEntry);
        if (location)
        {
            if (csidl == CSIDL_COMMON_DESKTOPDIRECTORY)
            {
                char *link_arg = escape_unix_link_arg(unix_link);
                if (link_arg)
                {
                    r = !write_desktop_entry(unix_link, location, lastEntry,
                        start_path, link_arg, description, work_dir, icon_name);
                    HeapFree(GetProcessHeap(), 0, link_arg);
                }
            }
            else
                r = !write_desktop_entry(NULL, location, lastEntry, escaped_path, escaped_args, description, work_dir, icon_name);
            if (r == 0)
                chmod(location, 0755);
            HeapFree(GetProcessHeap(), 0, location);
        }
    }
    else
    {
        char *link_arg = escape_unix_link_arg(unix_link);
        if (link_arg)
        {
            r = !write_menu_entry(unix_link, link_name, start_path, link_arg, description, work_dir, icon_name);
            HeapFree(GetProcessHeap(), 0, link_arg);
        }
    }

    ReleaseSemaphore( hsem, 1, NULL );

cleanup:
    if (hsem) CloseHandle( hsem );
    HeapFree( GetProcessHeap(), 0, icon_name );
    HeapFree( GetProcessHeap(), 0, work_dir );
    HeapFree( GetProcessHeap(), 0, link_name );
    HeapFree( GetProcessHeap(), 0, escaped_args );
    HeapFree( GetProcessHeap(), 0, escaped_path );
    HeapFree( GetProcessHeap(), 0, description );
    HeapFree( GetProcessHeap(), 0, unix_link );
    HeapFree( GetProcessHeap(), 0, start_path );

    if (r && !bWait)
        WINE_ERR("failed to build the menu\n" );

    return ( r == 0 );
}

static BOOL InvokeShellLinkerForURL( IUniformResourceLocatorW *url, LPCWSTR link, BOOL bWait )
{
    char *link_name = NULL;
    DWORD csidl = -1;
    LPWSTR urlPath;
    char *escaped_urlPath = NULL;
    HRESULT hr;
    HANDLE hSem = NULL;
    BOOL ret = TRUE;
    int r = -1;
    char *unix_link = NULL;

    if ( !link )
    {
        WINE_ERR("Link name is null\n");
        return TRUE;
    }

    if( !GetLinkLocation( link, &csidl, &link_name ) )
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
    WINE_TRACE("Link       : %s\n", wine_dbgstr_a(link_name));

    hr = url->lpVtbl->GetURL(url, &urlPath);
    if (FAILED(hr))
    {
        ret = TRUE;
        goto cleanup;
    }
    WINE_TRACE("path       : %s\n", wine_dbgstr_w(urlPath));

    unix_link = wine_get_unix_file_name(link);
    if (unix_link == NULL)
    {
        WINE_WARN("couldn't find unix path of %s\n", wine_dbgstr_w(link));
        goto cleanup;
    }

    escaped_urlPath = escape(urlPath);
    if (escaped_urlPath == NULL)
    {
        WINE_ERR("couldn't escape url, out of memory\n");
        goto cleanup;
    }

    hSem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != MsgWaitForMultipleObjects( 1, &hSem, FALSE, INFINITE, QS_ALLINPUT ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        goto cleanup;
    }
    if (in_desktop_dir(csidl))
    {
        char *location;
        const char *lastEntry;
        lastEntry = strrchr(link_name, '/');
        if (lastEntry == NULL)
            lastEntry = link_name;
        else
            ++lastEntry;
        location = heap_printf("%s/%s.desktop", xdg_desktop_dir, lastEntry);
        if (location)
        {
            r = !write_desktop_entry(NULL, location, lastEntry, "winebrowser", escaped_urlPath, NULL, NULL, NULL);
            if (r == 0)
                chmod(location, 0755);
            HeapFree(GetProcessHeap(), 0, location);
        }
    }
    else
        r = !write_menu_entry(unix_link, link_name, "winebrowser", escaped_urlPath, NULL, NULL, NULL);
    ret = (r != 0);
    ReleaseSemaphore(hSem, 1, NULL);

cleanup:
    if (hSem)
        CloseHandle(hSem);
    HeapFree(GetProcessHeap(), 0, link_name);
    CoTaskMemFree( urlPath );
    HeapFree(GetProcessHeap(), 0, escaped_urlPath);
    HeapFree(GetProcessHeap(), 0, unix_link);
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
        WINE_ERR("CreateToolhelp32Snapshot failed, error %d\n", GetLastError());
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
        WINE_WARN("Unable to find current process id %d when listing processes\n", ourpid);
        goto done;
    }

    if ((hprocess = OpenProcess( SYNCHRONIZE, FALSE, procentry.th32ParentProcessID )) ==
        NULL)
    {
        WINE_WARN("OpenProcess failed pid=%d, error %d\n", procentry.th32ParentProcessID,
                 GetLastError());
        goto done;
    }

    if (MsgWaitForMultipleObjects( 1, &hprocess, FALSE, INFINITE, QS_ALLINPUT ) == WAIT_OBJECT_0)
        ret = TRUE;
    else
        WINE_ERR("Unable to wait for parent process, error %d\n", GetLastError());

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
        return 1;
    }

    len=GetFullPathNameW( linkname, MAX_PATH, fullname, NULL );
    if (len==0 || len>MAX_PATH)
    {
        WINE_ERR("couldn't get full path of link file\n");
        return 1;
    }

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IShellLinkW, (LPVOID *) &sl );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IShellLink\n");
        return 1;
    }

    r = IShellLinkW_QueryInterface( sl, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return 1;
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
        return 1;
    }

    len=GetFullPathNameW( urlname, MAX_PATH, fullname, NULL );
    if (len==0 || len>MAX_PATH)
    {
        WINE_ERR("couldn't get full path of URL file\n");
        return 1;
    }

    r = CoCreateInstance( &CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUniformResourceLocatorW, (LPVOID *) &url );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IUniformResourceLocatorW\n");
        return 1;
    }

    r = url->lpVtbl->QueryInterface( url, &IID_IPersistFile, (LPVOID*) &pf );
    if( FAILED( r ) )
    {
        WINE_ERR("No IID_IPersistFile\n");
        return 1;
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
    char *mime_dir = NULL;
    char *packages_dir = NULL;
    char *applications_dir = NULL;
    BOOL hasChanged;

    hSem = CreateSemaphoreA( NULL, 1, 1, "winemenubuilder_semaphore");
    if( WAIT_OBJECT_0 != MsgWaitForMultipleObjects( 1, &hSem, FALSE, INFINITE, QS_ALLINPUT ) )
    {
        WINE_ERR("failed wait for semaphore\n");
        CloseHandle(hSem);
        hSem = NULL;
        goto end;
    }

    mime_dir = heap_printf("%s/mime", xdg_data_dir);
    if (mime_dir == NULL)
    {
        WINE_ERR("out of memory\n");
        goto end;
    }
    create_directories(mime_dir);

    packages_dir = heap_printf("%s/packages", mime_dir);
    if (packages_dir == NULL)
    {
        WINE_ERR("out of memory\n");
        goto end;
    }
    create_directories(packages_dir);

    applications_dir = heap_printf("%s/applications", xdg_data_dir);
    if (applications_dir == NULL)
    {
        WINE_ERR("out of memory\n");
        goto end;
    }
    create_directories(applications_dir);

    hasChanged = generate_associations(xdg_data_dir, packages_dir, applications_dir);
    hasChanged |= cleanup_associations();
    if (hasChanged)
    {
        const char *argv[3];

        argv[0] = "update-mime-database";
        argv[1] = mime_dir;
        argv[2] = NULL;
        spawnvp( _P_NOWAIT, argv[0], argv );

        argv[0] = "update-desktop-database";
        argv[1] = applications_dir;
        spawnvp( _P_NOWAIT, argv[0], argv );
    }

end:
    if (hSem)
    {
        ReleaseSemaphore(hSem, 1, NULL);
        CloseHandle(hSem);
    }
    HeapFree(GetProcessHeap(), 0, mime_dir);
    HeapFree(GetProcessHeap(), 0, packages_dir);
    HeapFree(GetProcessHeap(), 0, applications_dir);
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
                lret = ERROR_OUTOFMEMORY;
                value = HeapAlloc(GetProcessHeap(), 0, valueSize * sizeof(WCHAR));
                if (value == NULL)
                    break;
                data = HeapAlloc(GetProcessHeap(), 0, dataSize * sizeof(WCHAR));
                if (data == NULL)
                    break;
                lret = RegEnumValueW(hkey, i, value, &valueSize, NULL, NULL, (BYTE*)data, &dataSize);
                if (lret == ERROR_SUCCESS || lret != ERROR_MORE_DATA)
                    break;
                valueSize *= 2;
                dataSize *= 2;
                HeapFree(GetProcessHeap(), 0, value);
                HeapFree(GetProcessHeap(), 0, data);
                value = data = NULL;
            }
            if (lret == ERROR_SUCCESS)
            {
                char *unix_file;
                char *windows_file;
                unix_file = wchars_to_unix_chars(value);
                windows_file = wchars_to_unix_chars(data);
                if (unix_file != NULL && windows_file != NULL)
                {
                    struct stat filestats;
                    if (stat(windows_file, &filestats) < 0 && errno == ENOENT)
                    {
                        WINE_TRACE("removing menu related file %s\n", unix_file);
                        remove(unix_file);
                        RegDeleteValueW(hkey, value);
                    }
                    else
                        i++;
                }
                else
                {
                    WINE_ERR("out of memory enumerating menus\n");
                    lret = ERROR_OUTOFMEMORY;
                }
                HeapFree(GetProcessHeap(), 0, unix_file);
                HeapFree(GetProcessHeap(), 0, windows_file);
            }
            else if (lret != ERROR_NO_MORE_ITEMS)
                WINE_ERR("error %d reading registry\n", lret);
            HeapFree(GetProcessHeap(), 0, value);
            HeapFree(GetProcessHeap(), 0, data);
        }
        RegCloseKey(hkey);
    }
    else
        WINE_ERR("error opening registry key, menu cleanup failed\n");
}

static void thumbnail_lnk(LPCWSTR lnkPath, LPCWSTR outputPath)
{
    char *utf8lnkPath = NULL;
    char *utf8OutputPath = NULL;
    WCHAR *winLnkPath = NULL;
    IShellLinkW *shellLink = NULL;
    IPersistFile *persistFile = NULL;
    WCHAR szTmp[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    WCHAR szArgs[INFOTIPSIZE];
    WCHAR szIconPath[MAX_PATH];
    int iconId;
    IStream *stream = NULL;
    HRESULT hr;

    utf8lnkPath = wchars_to_utf8_chars(lnkPath);
    if (utf8lnkPath == NULL)
    {
        WINE_ERR("out of memory converting paths\n");
        goto end;
    }

    utf8OutputPath = wchars_to_utf8_chars(outputPath);
    if (utf8OutputPath == NULL)
    {
        WINE_ERR("out of memory converting paths\n");
        goto end;
    }

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
        WINE_ERR("could not create IShellLinkW, error 0x%08X\n", hr);
        goto end;
    }

    hr = IShellLinkW_QueryInterface(shellLink, &IID_IPersistFile, (LPVOID)&persistFile);
    if (FAILED(hr))
    {
        WINE_ERR("could not query IPersistFile, error 0x%08X\n", hr);
        goto end;
    }

    hr = IPersistFile_Load(persistFile, winLnkPath, STGM_READ);
    if (FAILED(hr))
    {
        WINE_ERR("could not read .lnk, error 0x%08X\n", hr);
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
        hr = open_icon(szIconPath, iconId, FALSE, &stream);
        if (SUCCEEDED(hr))
            hr = write_native_icon(stream, utf8OutputPath, NULL);
    }
    else
    {
        hr = open_icon(szPath, iconId, FALSE, &stream);
        if (SUCCEEDED(hr))
            hr = write_native_icon(stream, utf8OutputPath, NULL);
    }

end:
    HeapFree(GetProcessHeap(), 0, utf8lnkPath);
    HeapFree(GetProcessHeap(), 0, utf8OutputPath);
    HeapFree(GetProcessHeap(), 0, winLnkPath);
    if (shellLink != NULL)
        IShellLinkW_Release(shellLink);
    if (persistFile != NULL)
        IPersistFile_Release(persistFile);
    if (stream != NULL)
        IStream_Release(stream);
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
            t = strchrW( token, '"' );
            if( t )
                 *t++ = 0;
            break;
        case 0:
            t = NULL;
            break;
        default:
            token = t;
            t = strchrW( token, ' ' );
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
    WCHAR shellDesktopPath[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, shellDesktopPath);
    if (SUCCEEDED(hr))
        xdg_desktop_dir = wine_get_unix_file_name(shellDesktopPath);
    if (xdg_desktop_dir == NULL)
    {
        WINE_ERR("error looking up the desktop directory\n");
        return FALSE;
    }

    if (getenv("XDG_CONFIG_HOME"))
        xdg_config_dir = heap_printf("%s/menus/applications-merged", getenv("XDG_CONFIG_HOME"));
    else
        xdg_config_dir = heap_printf("%s/.config/menus/applications-merged", getenv("HOME"));
    if (xdg_config_dir)
    {
        create_directories(xdg_config_dir);
        if (getenv("XDG_DATA_HOME"))
            xdg_data_dir = strdupA(getenv("XDG_DATA_HOME"));
        else
            xdg_data_dir = heap_printf("%s/.local/share", getenv("HOME"));
        if (xdg_data_dir)
        {
            char *buffer;
            create_directories(xdg_data_dir);
            buffer = heap_printf("%s/desktop-directories", xdg_data_dir);
            if (buffer)
            {
                mkdir(buffer, 0777);
                HeapFree(GetProcessHeap(), 0, buffer);
            }
            return TRUE;
        }
        HeapFree(GetProcessHeap(), 0, xdg_config_dir);
    }
    WINE_ERR("out of memory\n");
    return FALSE;
}

/***********************************************************************
 *
 *           wWinMain
 */
int PASCAL wWinMain (HINSTANCE hInstance, HINSTANCE prev, LPWSTR cmdline, int show)
{
    static const WCHAR dash_aW[] = {'-','a',0};
    static const WCHAR dash_rW[] = {'-','r',0};
    static const WCHAR dash_tW[] = {'-','t',0};
    static const WCHAR dash_uW[] = {'-','u',0};
    static const WCHAR dash_wW[] = {'-','w',0};

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
        WINE_ERR("could not initialize COM, error 0x%08X\n", hr);
        return 1;
    }

    for( p = cmdline; p && *p; )
    {
        token = next_token( &p );
	if( !token )
	    break;
        if( !strcmpW( token, dash_aW ) )
        {
            RefreshFileTypeAssociations();
            continue;
        }
        if( !strcmpW( token, dash_rW ) )
        {
            cleanup_menus();
            continue;
        }
        if( !strcmpW( token, dash_wW ) )
            bWait = TRUE;
        else if ( !strcmpW( token, dash_uW ) )
            bURL = TRUE;
        else if ( !strcmpW( token, dash_tW ) )
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
