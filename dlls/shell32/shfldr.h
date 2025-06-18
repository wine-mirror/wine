/*
 *	Virtual Folder
 *	common definitions
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
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

#include "stgprop.h"

#define CHARS_IN_GUID 39

typedef struct {
    const GUID *fmtid;
    DWORD pid;
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

HRESULT SHELL32_GetColumnDetails(const shvheader *data, int column, SHELLDETAILS *details);
HRESULT shellfolder_map_column_to_scid(const shvheader *data, UINT column, SHCOLUMNID *scid);
HRESULT shellfolder_get_file_details(IShellFolder2 *iface, LPCITEMIDLIST pidl, const shvheader *header,
                                     int column, SHELLDETAILS *psd);

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

BOOL SHELL32_GetCustomFolderAttribute (LPCITEMIDLIST pidl, LPCWSTR pwszHeading, LPCWSTR pwszAttribute, LPWSTR pwszValue, DWORD cchValue);

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut);
HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc, LPITEMIDLIST * pidlInOut,
				  LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes);
HRESULT SHELL32_GetItemAttributes (IShellFolder2 *folder, LPCITEMIDLIST pidl, DWORD *attributes);
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR szOut,
				       DWORD dwOutLen);

HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot, const CLSID *clsidChild,
			     LPCWSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut);

HRESULT SHELL32_CompareIDs(IShellFolder2 *iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
LPITEMIDLIST SHELL32_CreatePidlFromBindCtx(IBindCtx *pbc, LPCWSTR path);

BOOL is_trash_available(void);
BOOL trash_file( const WCHAR *path );

static inline int SHELL32_GUIDToStringA (REFGUID guid, LPSTR str)
{
    return sprintf(str, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static inline int SHELL32_GUIDToStringW (REFGUID guid, LPWSTR str)
{
    static const WCHAR fmtW[] =
     { '{','%','0','8','l','x','-','%','0','4','x','-','%','0','4','x','-',
     '%','0','2','x','%','0','2','x','-',
     '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x',
     '%','0','2','x','%','0','2','x','}',0 };
    return swprintf(str, CHARS_IN_GUID, fmtW,
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

void SHELL_FS_ProcessDisplayFilename(LPWSTR szPath, DWORD dwFlags);

DEFINE_GUID( CLSID_UnixFolder, 0xcc702eb2, 0x7dc5, 0x11d9, 0xc6, 0x87, 0x00, 0x04, 0x23, 0x8a, 0x01, 0xcd );
DEFINE_GUID( CLSID_UnixDosFolder, 0x9d20aae8, 0x0625, 0x44b0, 0x9c, 0xa7, 0x71, 0x88, 0x9c, 0x22, 0x54, 0xd9 );
