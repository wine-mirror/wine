/*
 *                 Shell basics
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "dlgs.h"
#include "shellapi.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"
#include "rpcproxy.h"
#include "shlwapi.h"
#include "propsys.h"
#include "commoncontrols.h"

#include "pidl.h"
#include "shell32_main.h"
#include "version.h"
#include "shresdef.h"
#include "initguid.h"
#include "shfldr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static DWORD shgfi_get_exe_type(LPCWSTR szFullPath)
{
    BOOL status = FALSE;
    HANDLE hfile;
    DWORD BinaryType;
    IMAGE_DOS_HEADER mz_header;
    IMAGE_NT_HEADERS nt;
    DWORD len;
    char magic[4];

    status = GetBinaryTypeW (szFullPath, &BinaryType);
    if (!status)
        return 0;
    if (BinaryType == SCS_DOS_BINARY || BinaryType == SCS_PIF_BINARY)
        return 0x4d5a;

    hfile = CreateFileW( szFullPath, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return 0;

    /*
     * The next section is adapted from MODULE_GetBinaryType, as we need
     * to examine the image header to get OS and version information. We
     * know from calling GetBinaryTypeA that the image is valid and either
     * an NE or PE, so much error handling can be omitted.
     * Seek to the start of the file and read the header information.
     */

    SetFilePointer( hfile, 0, NULL, SEEK_SET );
    ReadFile( hfile, &mz_header, sizeof(mz_header), &len, NULL );

    SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
    ReadFile( hfile, magic, sizeof(magic), &len, NULL );
    if ( *(DWORD*)magic == IMAGE_NT_SIGNATURE )
    {
        SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
        ReadFile( hfile, &nt, sizeof(nt), &len, NULL );
        CloseHandle( hfile );
        /* DLL files are not executable and should return 0 */
        if (nt.FileHeader.Characteristics & IMAGE_FILE_DLL)
            return 0;
        if (nt.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
        {
             return IMAGE_NT_SIGNATURE | 
                   (nt.OptionalHeader.MajorSubsystemVersion << 24) |
                   (nt.OptionalHeader.MinorSubsystemVersion << 16);
        }
        return IMAGE_NT_SIGNATURE;
    }
    else if ( *(WORD*)magic == IMAGE_OS2_SIGNATURE )
    {
        IMAGE_OS2_HEADER ne;
        SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET );
        ReadFile( hfile, &ne, sizeof(ne), &len, NULL );
        CloseHandle( hfile );
        if (ne.ne_exetyp == 2)
            return IMAGE_OS2_SIGNATURE | (ne.ne_expver << 16);
        return 0;
    }
    CloseHandle( hfile );
    return 0;
}

/*************************************************************************
 * SHELL_IsShortcut		[internal]
 *
 * Decide if an item id list points to a shell shortcut
 */
BOOL SHELL_IsShortcut(LPCITEMIDLIST pidlLast)
{
    char szTemp[MAX_PATH];
    HKEY keyCls;
    BOOL ret = FALSE;

    if (_ILGetExtension(pidlLast, szTemp, MAX_PATH) &&
          HCR_MapTypeToValueA(szTemp, szTemp, MAX_PATH, TRUE))
    {
        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_QUERY_VALUE, &keyCls))
        {
          if (ERROR_SUCCESS == RegQueryValueExA(keyCls, "IsShortcut", NULL, NULL, NULL, NULL))
            ret = TRUE;

          RegCloseKey(keyCls);
        }
    }

    return ret;
}

#define SHGFI_KNOWN_FLAGS \
    (SHGFI_SMALLICON | SHGFI_OPENICON | SHGFI_SHELLICONSIZE | SHGFI_PIDL | \
     SHGFI_USEFILEATTRIBUTES | SHGFI_ADDOVERLAYS | SHGFI_OVERLAYINDEX | \
     SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_TYPENAME | SHGFI_ATTRIBUTES | \
     SHGFI_ICONLOCATION | SHGFI_EXETYPE | SHGFI_SYSICONINDEX | \
     SHGFI_LINKOVERLAY | SHGFI_SELECTED | SHGFI_ATTR_SPECIFIED)

/*************************************************************************
 * SHGetFileInfoW            [SHELL32.@]
 *
 */
DWORD_PTR WINAPI SHGetFileInfoW(LPCWSTR path,DWORD dwFileAttributes,
                                SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags )
{
    WCHAR szLocation[MAX_PATH], szFullPath[MAX_PATH];
    int iIndex;
    DWORD_PTR ret = TRUE;
    DWORD dwAttributes = 0;
    IShellFolder * psfParent = NULL;
    IExtractIconW * pei = NULL;
    LPITEMIDLIST    pidlLast = NULL, pidl = NULL;
    HRESULT hr = S_OK;
    UINT uGilFlags = 0;

    TRACE("%s fattr=0x%lx sfi=%p(attr=0x%08lx) size=0x%x flags=0x%x\n",
          (flags & SHGFI_PIDL)? "pidl" : debugstr_w(path), dwFileAttributes,
          psfi, psfi ? psfi->dwAttributes : 0, sizeofpsfi, flags);

    if (!path)
        return FALSE;

    /* windows initializes these values regardless of the flags */
    if (psfi != NULL)
    {
        psfi->szDisplayName[0] = '\0';
        psfi->szTypeName[0] = '\0';
        psfi->iIcon = 0;
    }

    if (!(flags & SHGFI_PIDL))
    {
        /* SHGetFileInfo should work with absolute and relative paths */
        if (PathIsRelativeW(path))
        {
            GetCurrentDirectoryW(MAX_PATH, szLocation);
            PathCombineW(szFullPath, szLocation, path);
        }
        else
        {
            lstrcpynW(szFullPath, path, MAX_PATH);
        }
    }

    if (flags & SHGFI_EXETYPE)
    {
        if (flags != SHGFI_EXETYPE)
            return 0;
        return shgfi_get_exe_type(szFullPath);
    }

    /*
     * psfi is NULL normally to query EXE type. If it is NULL, none of the
     * below makes sense anyway. Windows allows this and just returns FALSE
     */
    if (psfi == NULL)
        return FALSE;

    /*
     * translate the path into a pidl only when SHGFI_USEFILEATTRIBUTES
     * is not specified.
     * The pidl functions fail on not existing file names
     */

    if (flags & SHGFI_PIDL)
    {
        pidl = ILClone((LPCITEMIDLIST)path);
    }
    else if (!(flags & SHGFI_USEFILEATTRIBUTES))
    {
        hr = SHILCreateFromPathW(szFullPath, &pidl, &dwAttributes);
    }

    if ((flags & SHGFI_PIDL) || !(flags & SHGFI_USEFILEATTRIBUTES))
    {
        /* get the parent shellfolder */
        if (pidl)
        {
            hr = SHBindToParent( pidl, &IID_IShellFolder, (LPVOID*)&psfParent,
                                (LPCITEMIDLIST*)&pidlLast );
            if (SUCCEEDED(hr))
                pidlLast = ILClone(pidlLast);
            ILFree(pidl);
        }
        else
        {
            ERR("pidl is null!\n");
            return FALSE;
        }
    }

    /* get the attributes of the child */
    if (SUCCEEDED(hr) && (flags & SHGFI_ATTRIBUTES))
    {
        if (!(flags & SHGFI_ATTR_SPECIFIED))
        {
            psfi->dwAttributes = 0xffffffff;
        }
        if (psfParent)
            IShellFolder_GetAttributesOf( psfParent, 1, (LPCITEMIDLIST*)&pidlLast,
                                      &(psfi->dwAttributes) );
    }

    /* get the displayname */
    if (SUCCEEDED(hr) && (flags & SHGFI_DISPLAYNAME))
    {
        if (flags & SHGFI_USEFILEATTRIBUTES && !(flags & SHGFI_PIDL))
        {
            lstrcpyW (psfi->szDisplayName, PathFindFileNameW(szFullPath));
        }
        else
        {
            STRRET str;
            hr = IShellFolder_GetDisplayNameOf( psfParent, pidlLast,
                                                SHGDN_INFOLDER, &str);
            StrRetToStrNW (psfi->szDisplayName, MAX_PATH, &str, pidlLast);
        }
    }

    /* get the type name */
    if (SUCCEEDED(hr) && (flags & SHGFI_TYPENAME))
    {
        if (!(flags & SHGFI_USEFILEATTRIBUTES) || (flags & SHGFI_PIDL))
        {
            char ftype[80];

            _ILGetFileType(pidlLast, ftype, 80);
            MultiByteToWideChar(CP_ACP, 0, ftype, -1, psfi->szTypeName, 80 );
        }
        else
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                lstrcatW (psfi->szTypeName, L"Folder");
            else 
            {
                WCHAR sTemp[64];

                lstrcpyW(sTemp,PathFindExtensionW(szFullPath));
                if (sTemp[0] == 0 || (sTemp[0] == '.' && sTemp[1] == 0))
                {
                    /* "name" or "name." => "File" */
                    lstrcpynW (psfi->szTypeName, L"File", 64);
                }
                else if (!( HCR_MapTypeToValueW(sTemp, sTemp, 64, TRUE) &&
                    HCR_MapTypeToValueW(sTemp, psfi->szTypeName, 80, FALSE )))
                {
                    if (sTemp[0])
                    {
                        lstrcpynW (psfi->szTypeName, sTemp, 64);
                        lstrcatW (psfi->szTypeName, L" file");
                    }
                    else
                    {
                        lstrcpynW (psfi->szTypeName, L"File", 64);
                    }
                }
            }
        }
    }

    /* ### icons ###*/
    if (flags & SHGFI_OPENICON)
        uGilFlags |= GIL_OPENICON;

    if (flags & SHGFI_LINKOVERLAY)
        uGilFlags |= GIL_FORSHORTCUT;
    else if ((flags&SHGFI_ADDOVERLAYS) ||
             (flags&(SHGFI_ICON|SHGFI_SMALLICON))==SHGFI_ICON)
    {
        if (SHELL_IsShortcut(pidlLast))
            uGilFlags |= GIL_FORSHORTCUT;
    }

    if (flags & SHGFI_OVERLAYINDEX)
        FIXME("SHGFI_OVERLAYINDEX unhandled\n");

    if (flags & SHGFI_SELECTED)
        FIXME("set icon to selected, stub\n");

    /* get the iconlocation */
    if (SUCCEEDED(hr) && (flags & SHGFI_ICONLOCATION ))
    {
        UINT uDummy,uFlags;

        if (flags & SHGFI_USEFILEATTRIBUTES && !(flags & SHGFI_PIDL))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                lstrcpyW(psfi->szDisplayName, swShell32Name);
                psfi->iIcon = -IDI_SHELL_FOLDER;
            }
            else
            {
                WCHAR* szExt;
                WCHAR sTemp [MAX_PATH];

                szExt = PathFindExtensionW(szFullPath);
                TRACE("szExt=%s\n", debugstr_w(szExt));
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetDefaultIconW(sTemp, sTemp, MAX_PATH, &psfi->iIcon))
                {
                    if (wcscmp(L"%1", sTemp))
                        lstrcpyW(psfi->szDisplayName, sTemp);
                    else
                    {
                        /* the icon is in the file */
                        lstrcpyW(psfi->szDisplayName, szFullPath);
                    }
                }
                else
                    ret = FALSE;
            }
        }
        else
        {
            hr = IShellFolder_GetUIObjectOf(psfParent, 0, 1,
                (LPCITEMIDLIST*)&pidlLast, &IID_IExtractIconW,
                &uDummy, (LPVOID*)&pei);
            if (SUCCEEDED(hr))
            {
                hr = IExtractIconW_GetIconLocation(pei, uGilFlags,
                    szLocation, MAX_PATH, &iIndex, &uFlags);

                if (uFlags & GIL_NOTFILENAME)
                    ret = FALSE;
                else
                {
                    lstrcpyW (psfi->szDisplayName, szLocation);
                    psfi->iIcon = iIndex;
                }
                IExtractIconW_Release(pei);
            }
        }
    }

    /* get icon index (or load icon)*/
    if (SUCCEEDED(hr) && (flags & (SHGFI_ICON | SHGFI_SYSICONINDEX)))
    {
        IImageList *icon_list;
        SHGetImageList( (flags & SHGFI_SMALLICON) ? SHIL_SMALL : SHIL_LARGE, &IID_IImageList, (void **)&icon_list );

        if (flags & SHGFI_USEFILEATTRIBUTES && !(flags & SHGFI_PIDL))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                psfi->iIcon = SIC_GetIconIndex(swShell32Name, -IDI_SHELL_FOLDER, 0);
            else
            {
                WCHAR sTemp[MAX_PATH];
                WCHAR *szExt;
                int icon_idx = 0;

                lstrcpynW(sTemp, szFullPath, MAX_PATH);

                psfi->iIcon = 0;
                szExt = PathFindExtensionW(sTemp);
                if ( szExt &&
                     HCR_MapTypeToValueW(szExt, sTemp, MAX_PATH, TRUE) &&
                     HCR_GetDefaultIconW(sTemp, sTemp, MAX_PATH, &icon_idx))
                {
                    if (!wcscmp(L"%1",sTemp))            /* icon is in the file */
                        lstrcpyW(sTemp, szFullPath);

                    psfi->iIcon = SIC_GetIconIndex(sTemp, icon_idx, 0);
                    if (psfi->iIcon == -1)
                        psfi->iIcon = 0;
                }
            }
        }
        else
        {
            if (!(PidlToSicIndex(psfParent, pidlLast, !(flags & SHGFI_SMALLICON),
                uGilFlags, &(psfi->iIcon))))
            {
                ret = FALSE;
            }
        }
        if (ret && (flags & SHGFI_SYSICONINDEX))
        {
            IImageList_AddRef( icon_list );
            ret = (DWORD_PTR)icon_list;
        }
        if (ret && (flags & SHGFI_ICON))
        {
            if (flags & SHGFI_SHELLICONSIZE)
                hr = IImageList_GetIcon( icon_list, psfi->iIcon, ILD_NORMAL, &psfi->hIcon );
            else
            {
                int width = GetSystemMetrics( (flags & SHGFI_SMALLICON) ? SM_CXSMICON : SM_CXICON );
                int height = GetSystemMetrics( (flags & SHGFI_SMALLICON) ? SM_CYSMICON : SM_CYICON );
                int list_width, list_height;

                IImageList_GetIconSize( icon_list, &list_width, &list_height );
                if (list_width == width && list_height == height)
                    hr = IImageList_GetIcon( icon_list, psfi->iIcon, ILD_NORMAL, &psfi->hIcon );
                else /* Use SHIL_SYSSMALL for SHFI_SMALLICONS when we implement it */
                {
                    WCHAR buf[MAX_PATH], *file = buf;
                    DWORD size = sizeof(buf);
                    int icon_idx;

                    while ((hr = SIC_get_location( psfi->iIcon, file, &size, &icon_idx )) == E_NOT_SUFFICIENT_BUFFER)
                    {
                        if (file == buf) file = malloc( size );
                        else file = realloc( file, size );
                        if (!file) break;
                    }
                    if (SUCCEEDED(hr))
                    {
                        ret = PrivateExtractIconsW( file, icon_idx, width, height, &psfi->hIcon, 0, 1, 0);
                        if (ret == 0 || ret == (UINT)-1) hr = E_FAIL;
                    }
                    if (file != buf) free( file );
                }
            }
        }
        IImageList_Release( icon_list );
    }

    if (flags & ~SHGFI_KNOWN_FLAGS)
        FIXME("unknown flags %08x\n", flags & ~SHGFI_KNOWN_FLAGS);

    if (psfParent)
        IShellFolder_Release(psfParent);

    if (hr != S_OK)
        ret = FALSE;

    SHFree(pidlLast);

    TRACE ("icon=%p index=0x%08x attr=0x%08lx name=%s type=%s ret=0x%08Ix\n",
           psfi->hIcon, psfi->iIcon, psfi->dwAttributes,
           debugstr_w(psfi->szDisplayName), debugstr_w(psfi->szTypeName), ret);

    return ret;
}

/*************************************************************************
 * SHGetFileInfoA            [SHELL32.@]
 *
 * Note:
 *    MSVBVM60.__vbaNew2 expects this function to return a value in range
 *    1 .. 0x7fff when the function succeeds and flags does not contain
 *    SHGFI_EXETYPE or SHGFI_SYSICONINDEX (see bug 7701)
 */
DWORD_PTR WINAPI SHGetFileInfoA(LPCSTR path,DWORD dwFileAttributes,
                                SHFILEINFOA *psfi, UINT sizeofpsfi,
                                UINT flags )
{
    INT len;
    LPWSTR temppath = NULL;
    LPCWSTR pathW;
    DWORD_PTR ret;
    SHFILEINFOW temppsfi;

    if (flags & SHGFI_PIDL)
    {
        /* path contains a pidl */
        pathW = (LPCWSTR)path;
    }
    else
    {
        len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        temppath = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, path, -1, temppath, len);
        pathW = temppath;
    }

    if (psfi && (flags & SHGFI_ATTR_SPECIFIED))
        temppsfi.dwAttributes=psfi->dwAttributes;

    if (psfi == NULL)
        ret = SHGetFileInfoW(pathW, dwFileAttributes, NULL, sizeof(temppsfi), flags);
    else
        ret = SHGetFileInfoW(pathW, dwFileAttributes, &temppsfi, sizeof(temppsfi), flags);

    if (psfi)
    {
        if(flags & SHGFI_ICON)
            psfi->hIcon=temppsfi.hIcon;
        if(flags & (SHGFI_SYSICONINDEX|SHGFI_ICON|SHGFI_ICONLOCATION))
            psfi->iIcon=temppsfi.iIcon;
        if(flags & SHGFI_ATTRIBUTES)
            psfi->dwAttributes=temppsfi.dwAttributes;
        if(flags & (SHGFI_DISPLAYNAME|SHGFI_ICONLOCATION))
        {
            WideCharToMultiByte(CP_ACP, 0, temppsfi.szDisplayName, -1,
                  psfi->szDisplayName, sizeof(psfi->szDisplayName), NULL, NULL);
        }
        if(flags & SHGFI_TYPENAME)
        {
            WideCharToMultiByte(CP_ACP, 0, temppsfi.szTypeName, -1,
                  psfi->szTypeName, sizeof(psfi->szTypeName), NULL, NULL);
        }
    }

    free(temppath);

    return ret;
}

/*************************************************************************
 * DuplicateIcon            [SHELL32.@]
 */
HICON WINAPI DuplicateIcon( HINSTANCE hInstance, HICON hIcon)
{
    ICONINFO IconInfo;
    HICON hDupIcon = 0;

    TRACE("%p %p\n", hInstance, hIcon);

    if (GetIconInfo(hIcon, &IconInfo))
    {
        hDupIcon = CreateIconIndirect(&IconInfo);

        /* clean up hbmMask and hbmColor */
        DeleteObject(IconInfo.hbmMask);
        DeleteObject(IconInfo.hbmColor);
    }

    return hDupIcon;
}

/*************************************************************************
 * ExtractIconA                [SHELL32.@]
 */
HICON WINAPI ExtractIconA(HINSTANCE hInstance, const char *file, UINT nIconIndex)
{
    WCHAR *fileW;
    HICON ret;

    TRACE("%p %s %d\n", hInstance, debugstr_a(file), nIconIndex);

    fileW = strdupAtoW(file);
    ret = ExtractIconW(hInstance, fileW, nIconIndex);
    free(fileW);

    return ret;
}

/*************************************************************************
 * ExtractIconW                [SHELL32.@]
 */
HICON WINAPI ExtractIconW(HINSTANCE hInstance, LPCWSTR lpszFile, UINT nIconIndex)
{
    HICON  hIcon = NULL;
    UINT ret;
    UINT cx = GetSystemMetrics(SM_CXICON), cy = GetSystemMetrics(SM_CYICON);

    TRACE("%p %s %d\n", hInstance, debugstr_w(lpszFile), nIconIndex);

    if (nIconIndex == (UINT)-1)
    {
        ret = PrivateExtractIconsW(lpszFile, 0, cx, cy, NULL, NULL, 0, LR_DEFAULTCOLOR);
        if (ret != (UINT)-1 && ret)
            return (HICON)(UINT_PTR)ret;
    }
    else
    {
        ret = PrivateExtractIconsW(lpszFile, nIconIndex, cx, cy, &hIcon, NULL, 1, LR_DEFAULTCOLOR);
        if (ret > 0 && hIcon)
            return hIcon;
    }
    return NULL;
}

HRESULT WINAPI SHCreateFileExtractIconW(LPCWSTR file, DWORD attribs, REFIID riid, void **ppv)
{
  FIXME("%s, %lx, %s, %p\n", debugstr_w(file), attribs, debugstr_guid(riid), ppv);
  *ppv = NULL;
  return E_NOTIMPL;
}

/*************************************************************************
 * Printer_LoadIconsW        [SHELL32.205]
 */
VOID WINAPI Printer_LoadIconsW(LPCWSTR wsPrinterName, HICON * pLargeIcon, HICON * pSmallIcon)
{
    INT iconindex=IDI_SHELL_PRINTER;

    TRACE("(%s, %p, %p)\n", debugstr_w(wsPrinterName), pLargeIcon, pSmallIcon);

    /* We should check if wsPrinterName is
       1. the Default Printer or not
       2. connected or not
       3. a Local Printer or a Network-Printer
       and use different Icons
    */
    if((wsPrinterName != NULL) && (wsPrinterName[0] != 0))
    {
        FIXME("(select Icon by PrinterName %s not implemented)\n", debugstr_w(wsPrinterName));
    }

    if(pLargeIcon != NULL)
        *pLargeIcon = LoadImageW(shell32_hInstance,
                                 (LPCWSTR) MAKEINTRESOURCE(iconindex), IMAGE_ICON,
                                 0, 0, LR_DEFAULTCOLOR|LR_DEFAULTSIZE);

    if(pSmallIcon != NULL)
        *pSmallIcon = LoadImageW(shell32_hInstance,
                                 (LPCWSTR) MAKEINTRESOURCE(iconindex), IMAGE_ICON,
                                 16, 16, LR_DEFAULTCOLOR);
}

/*************************************************************************
 * Printers_RegisterWindowW        [SHELL32.213]
 * used by "printui.dll":
 * find the Window of the given Type for the specific Printer and 
 * return the already existent hwnd or open a new window
 */
BOOL WINAPI Printers_RegisterWindowW(LPCWSTR wsPrinter, DWORD dwType,
            HANDLE * phClassPidl, HWND * phwnd)
{
    FIXME("(%s, %lx, %p (%p), %p (%p)) stub!\n", debugstr_w(wsPrinter), dwType,
                phClassPidl, (phClassPidl != NULL) ? *(phClassPidl) : NULL,
                phwnd, (phwnd != NULL) ? *(phwnd) : NULL);

    return FALSE;
} 

/*************************************************************************
 * Printers_UnregisterWindow      [SHELL32.214]
 */
VOID WINAPI Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd)
{
    FIXME("(%p, %p) stub!\n", hClassPidl, hwnd);
} 

struct window_prop_store
{
    IPropertyStore IPropertyStore_iface;
    LONG           ref;
};

static inline struct window_prop_store *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct window_prop_store, IPropertyStore_iface);
}

static ULONG WINAPI window_prop_store_AddRef(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedIncrement(&store->ref);
    TRACE("returning %ld\n", ref);
    return ref;
}

static ULONG WINAPI window_prop_store_Release(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedDecrement(&store->ref);
    if (!ref) free(store);
    TRACE("returning %ld\n", ref);
    return ref;
}

static HRESULT WINAPI window_prop_store_QueryInterface(IPropertyStore *iface, REFIID iid, void **obj)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    TRACE("%p, %s, %p\n", iface, debugstr_guid(iid), obj);

    if (!obj) return E_POINTER;
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IPropertyStore))
    {
        *obj = &store->IPropertyStore_iface;
    }
    else
    {
        FIXME("no interface for %s\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    FIXME("%p, %p\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    FIXME("%p, %lu,%p\n", iface, prop, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetValue(IPropertyStore *iface, const PROPERTYKEY *key, PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_SetValue(IPropertyStore *iface, const PROPERTYKEY *key, const PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_Commit(IPropertyStore *iface)
{
    FIXME("%p\n", iface);
    return S_OK;
}

static const IPropertyStoreVtbl window_prop_store_vtbl =
{
    window_prop_store_QueryInterface,
    window_prop_store_AddRef,
    window_prop_store_Release,
    window_prop_store_GetCount,
    window_prop_store_GetAt,
    window_prop_store_GetValue,
    window_prop_store_SetValue,
    window_prop_store_Commit
};

static HRESULT create_window_prop_store(IPropertyStore **obj)
{
    struct window_prop_store *store;

    if (!(store = malloc(sizeof(*store)))) return E_OUTOFMEMORY;
    store->IPropertyStore_iface.lpVtbl = &window_prop_store_vtbl;
    store->ref = 1;

    *obj = &store->IPropertyStore_iface;
    return S_OK;
}

/*************************************************************************
 * SHGetPropertyStoreForWindow [SHELL32.@]
 */
HRESULT WINAPI SHGetPropertyStoreForWindow(HWND hwnd, REFIID riid, void **ppv)
{
    IPropertyStore *store;
    HRESULT hr;

    FIXME("(%p %p %p) stub!\n", hwnd, riid, ppv);

    if ((hr = create_window_prop_store( &store )) != S_OK) return hr;
    hr = IPropertyStore_QueryInterface( store, riid, ppv );
    IPropertyStore_Release( store );
    return hr;
}

/*************************************************************************/

typedef struct
{
    LPCWSTR  szApp;
    LPCWSTR  szOtherStuff;
    HICON hIcon;
    HFONT hFont;
} ABOUT_INFO;

#define DROP_FIELD_TOP    (-12)

static void paint_dropline( HDC hdc, HWND hWnd )
{
    HWND hWndCtl = GetDlgItem(hWnd, IDC_ABOUT_WINE_TEXT);
    RECT rect;

    if (!hWndCtl) return;
    GetWindowRect( hWndCtl, &rect );
    MapWindowPoints( 0, hWnd, (LPPOINT)&rect, 2 );
    rect.top += DROP_FIELD_TOP;
    rect.bottom = rect.top + 2;
    DrawEdge( hdc, &rect, BDR_SUNKENOUTER, BF_RECT );
}

/*************************************************************************
 * SHHelpShortcuts_RunDLLA        [SHELL32.@]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLLA(DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{
    FIXME("(%lx, %lx, %lx, %lx) stub!\n", dwArg1, dwArg2, dwArg3, dwArg4);
    return 0;
}

/*************************************************************************
 * SHHelpShortcuts_RunDLLA        [SHELL32.@]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLLW(DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{
    FIXME("(%lx, %lx, %lx, %lx) stub!\n", dwArg1, dwArg2, dwArg3, dwArg4);
    return 0;
}

/*************************************************************************
 * SHLoadInProc                [SHELL32.@]
 * Create an instance of specified object class from within
 * the shell process and release it immediately
 */
HRESULT WINAPI SHLoadInProc (REFCLSID rclsid)
{
    void *ptr = NULL;

    TRACE("%s\n", debugstr_guid(rclsid));

    CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown,&ptr);
    if(ptr)
    {
        IUnknown * pUnk = ptr;
        IUnknown_Release(pUnk);
        return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
}

static void add_authors( HWND list )
{
    WCHAR *strW, *start, *end;
    HRSRC rsrc = FindResourceW( shell32_hInstance, L"AUTHORS", (LPCWSTR)RT_RCDATA );
    char *strA = LockResource( LoadResource( shell32_hInstance, rsrc ));
    DWORD sizeW, sizeA = SizeofResource( shell32_hInstance, rsrc );

    if (!strA) return;
    sizeW = MultiByteToWideChar( CP_UTF8, 0, strA, sizeA, NULL, 0 ) + 1;
    if (!(strW = malloc( sizeW * sizeof(WCHAR) ))) return;
    MultiByteToWideChar( CP_UTF8, 0, strA, sizeA, strW, sizeW );
    strW[sizeW - 1] = 0;

    start = wcspbrk( strW, L"\r\n" );  /* skip the header line */
    while (start)
    {
        while (*start && wcschr( L"\r\n", *start )) start++;
        if (!*start) break;
        end = wcspbrk( start, L"\r\n" );
        if (end) *end++ = 0;
        SendMessageW( list, LB_ADDSTRING, -1, (LPARAM)start );
        start = end;
    }
    free( strW );
}

/*************************************************************************
 * AboutDlgProc            (internal)
 */
static INT_PTR CALLBACK AboutDlgProc( HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam )
{
    HWND hWndCtl;

    TRACE("\n");

    switch(msg)
    {
    case WM_INITDIALOG:
        {
            ABOUT_INFO *info = (ABOUT_INFO *)lParam;
            WCHAR template[512], buffer[512], version[64];
            const char *(CDECL *wine_get_build_id)(void);

            if (info)
            {
                wine_get_build_id = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"),
                                                            "wine_get_build_id");
                SendDlgItemMessageW(hWnd, stc1, STM_SETICON,(WPARAM)info->hIcon, 0);
                GetWindowTextW( hWnd, template, ARRAY_SIZE(template) );
                swprintf( buffer, ARRAY_SIZE(buffer), template, info->szApp );
                SetWindowTextW( hWnd, buffer );
                SetWindowTextW( GetDlgItem(hWnd, IDC_ABOUT_STATIC_TEXT1), info->szApp );
                SetWindowTextW( GetDlgItem(hWnd, IDC_ABOUT_STATIC_TEXT2), info->szOtherStuff );
                GetWindowTextW( GetDlgItem(hWnd, IDC_ABOUT_STATIC_TEXT3),
                                template, ARRAY_SIZE(template) );
                if (wine_get_build_id)
                {
                    MultiByteToWideChar( CP_UTF8, 0, wine_get_build_id(), -1, version, ARRAY_SIZE(version) );
                    swprintf( buffer, ARRAY_SIZE(buffer), template, version );
                    SetWindowTextW( GetDlgItem(hWnd, IDC_ABOUT_STATIC_TEXT3), buffer );
                }
                hWndCtl = GetDlgItem(hWnd, IDC_ABOUT_LISTBOX);
                SendMessageW( hWndCtl, WM_SETREDRAW, 0, 0 );
                SendMessageW( hWndCtl, WM_SETFONT, (WPARAM)info->hFont, 0 );
                add_authors( hWndCtl );
                SendMessageW( hWndCtl, WM_SETREDRAW, 1, 0 );
            }
        }
        return 1;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint( hWnd, &ps );
            paint_dropline( hDC, hWnd );
            EndPaint( hWnd, &ps );
        }
    break;

    case WM_COMMAND:
        if (wParam == IDOK || wParam == IDCANCEL)
        {
            EndDialog(hWnd, TRUE);
            return TRUE;
        }
        if (wParam == IDC_ABOUT_LICENSE)
        {
            MSGBOXPARAMSW params;

            params.cbSize = sizeof(params);
            params.hwndOwner = hWnd;
            params.hInstance = shell32_hInstance;
            params.lpszText = MAKEINTRESOURCEW(IDS_LICENSE);
            params.lpszCaption = MAKEINTRESOURCEW(IDS_LICENSE_CAPTION);
            params.dwStyle = MB_ICONINFORMATION | MB_OK;
            params.lpszIcon = 0;
            params.dwContextHelpId = 0;
            params.lpfnMsgBoxCallback = NULL;
            params.dwLanguageId = LANG_NEUTRAL;
            MessageBoxIndirectW( &params );
        }
        break;
    case WM_CLOSE:
      EndDialog(hWnd, TRUE);
      break;
    }

    return 0;
}


/*************************************************************************
 * ShellAboutA                [SHELL32.288]
 */
BOOL WINAPI ShellAboutA( HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon )
{
    BOOL ret;
    LPWSTR appW = NULL, otherW = NULL;
    int len;

    if (szApp)
    {
        len = MultiByteToWideChar(CP_ACP, 0, szApp, -1, NULL, 0);
        appW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szApp, -1, appW, len);
    }
    if (szOtherStuff)
    {
        len = MultiByteToWideChar(CP_ACP, 0, szOtherStuff, -1, NULL, 0);
        otherW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, szOtherStuff, -1, otherW, len);
    }

    ret = ShellAboutW(hWnd, appW, otherW, hIcon);

    free(otherW);
    free(appW);
    return ret;
}


/*************************************************************************
 * ShellAboutW                [SHELL32.289]
 */
BOOL WINAPI ShellAboutW( HWND hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff,
                             HICON hIcon )
{
    ABOUT_INFO info;
    LOGFONTW logFont;
    BOOL bRet;

    TRACE("\n");

    if (!hIcon) hIcon = LoadImageW( 0, (LPWSTR)IDI_WINLOGO, IMAGE_ICON, 48, 48, LR_SHARED );
    info.szApp        = szApp;
    info.szOtherStuff = szOtherStuff;
    info.hIcon        = hIcon;

    SystemParametersInfoW( SPI_GETICONTITLELOGFONT, 0, &logFont, 0 );
    info.hFont = CreateFontIndirectW( &logFont );

    bRet = DialogBoxParamW( shell32_hInstance, L"SHELL_ABOUT_MSGBOX", hWnd, AboutDlgProc, (LPARAM)&info );
    DeleteObject(info.hFont);
    return bRet;
}

/*************************************************************************
 * FreeIconList (SHELL32.@)
 */
void WINAPI FreeIconList( DWORD dw )
{
    FIXME("%lx: stub\n",dw);
}

/*************************************************************************
 * SHLoadNonloadedIconOverlayIdentifiers (SHELL32.@)
 */
HRESULT WINAPI SHLoadNonloadedIconOverlayIdentifiers( VOID )
{
    FIXME("stub\n");
    return S_OK;
}

/***********************************************************************
 * DllGetVersion [SHELL32.@]
 *
 * Retrieves version information of the 'SHELL32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a shell32.dll from IE4.01 SP1.
 */

HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
    /* FIXME: shouldn't these values come from the version resource? */
    if (pdvi->cbSize == sizeof(DLLVERSIONINFO) ||
        pdvi->cbSize == sizeof(DLLVERSIONINFO2))
    {
        pdvi->dwMajorVersion = WINE_FILEVERSION_MAJOR;
        pdvi->dwMinorVersion = WINE_FILEVERSION_MINOR;
        pdvi->dwBuildNumber = WINE_FILEVERSION_BUILD;
        pdvi->dwPlatformID = WINE_FILEVERSION_PLATFORMID;
        if (pdvi->cbSize == sizeof(DLLVERSIONINFO2))
        {
            DLLVERSIONINFO2 *pdvi2 = (DLLVERSIONINFO2 *)pdvi;

            pdvi2->dwFlags = 0;
            pdvi2->ullVersion = MAKEDLLVERULL(WINE_FILEVERSION_MAJOR,
                                              WINE_FILEVERSION_MINOR,
                                              WINE_FILEVERSION_BUILD,
                                              WINE_FILEVERSION_PLATFORMID);
        }
        TRACE("%lu.%lu.%lu.%lu\n",
              pdvi->dwMajorVersion, pdvi->dwMinorVersion,
              pdvi->dwBuildNumber, pdvi->dwPlatformID);
        return S_OK;
    }
    else
    {
        WARN("wrong DLLVERSIONINFO size from app\n");
        return E_INVALIDARG;
    }
}

/*************************************************************************
 * global variables of the shell32.dll
 * all are once per process
 *
 */
HINSTANCE    shell32_hInstance = 0;


/*************************************************************************
 * SHELL32 DllMain
 *
 * NOTES
 *  calling oleinitialize here breaks some apps.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        shell32_hInstance = hinstDLL;
        DisableThreadLibraryCalls(shell32_hInstance);

        /* get full path to this DLL for IExtractIconW_fnGetIconLocation() */
        GetModuleFileNameW(hinstDLL, swShell32Name, MAX_PATH);
        swShell32Name[MAX_PATH - 1] = '\0';

        InitChangeNotifications();
        break;

    case DLL_PROCESS_DETACH:
        if (fImpLoad) break;
        SIC_Destroy();
        FreeChangeNotifications();
        release_desktop_folder();
        release_typelib();
        break;
    }
    return TRUE;
}

/*************************************************************************
 * DllInstall         [SHELL32.@]
 *
 * PARAMETERS
 *
 *    BOOL bInstall - TRUE for install, FALSE for uninstall
 *    LPCWSTR pszCmdLine - command line (unused by shell32?)
 */

HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("%s %s: stub\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));
    return S_OK;        /* indicate success */
}

/***********************************************************************
 *		DllRegisterServer (SHELL32.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr = __wine_register_resources();
    if (SUCCEEDED(hr)) hr = SHELL_RegisterShellFolders();
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (SHELL32.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources();
}

/***********************************************************************
 *              ExtractVersionResource16W (SHELL32.@)
 */
BOOL WINAPI ExtractVersionResource16W(LPWSTR s, DWORD d)
{
    FIXME("(%s %lx) stub!\n", debugstr_w(s), d);
    return FALSE;
}

/***********************************************************************
 *              InitNetworkAddressControl (SHELL32.@)
 */
BOOL WINAPI InitNetworkAddressControl(void)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *              ShellHookProc (SHELL32.@)
 */
LRESULT CALLBACK ShellHookProc(DWORD a, DWORD b, DWORD c)
{
    FIXME("Stub\n");
    return 0;
}

/***********************************************************************
 *              SHGetLocalizedName (SHELL32.@)
 */
HRESULT WINAPI SHGetLocalizedName(LPCWSTR path, LPWSTR module, UINT size, INT *res)
{
    FIXME("%s %p %u %p: stub\n", debugstr_w(path), module, size, res);
    return E_NOTIMPL;
}

/***********************************************************************
 *              SHSetUnreadMailCountW (SHELL32.@)
 */
HRESULT WINAPI SHSetUnreadMailCountW(LPCWSTR mailaddress, DWORD count, LPCWSTR executecommand)
{
    FIXME("%s %lx %s: stub\n", debugstr_w(mailaddress), count, debugstr_w(executecommand));
    return E_NOTIMPL;
}

/***********************************************************************
 *              SHEnumerateUnreadMailAccountsW (SHELL32.@)
 */
HRESULT WINAPI SHEnumerateUnreadMailAccountsW(HKEY user, DWORD idx, LPWSTR mailaddress, INT mailaddresslen)
{
    FIXME("%p %ld %p %d: stub\n", user, idx, mailaddress, mailaddresslen);
    return E_NOTIMPL;
}

/***********************************************************************
 *              SHQueryUserNotificationState (SHELL32.@)
 */
HRESULT WINAPI SHQueryUserNotificationState(QUERY_USER_NOTIFICATION_STATE *state)
{
    FIXME("%p: stub\n", state);
    *state = QUNS_ACCEPTS_NOTIFICATIONS;
    return S_OK;
}

/***********************************************************************
 *              SHCreateDataObject (SHELL32.@)
 */
HRESULT WINAPI SHCreateDataObject(PCIDLIST_ABSOLUTE pidl_folder, UINT count, PCUITEMID_CHILD_ARRAY pidl_array,
                                  IDataObject *object, REFIID riid, void **ppv)
{
    FIXME("%p %d %p %p %s %p: stub\n", pidl_folder, count, pidl_array, object, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}
