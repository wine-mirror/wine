
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
< * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct {
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut);
HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc, LPITEMIDLIST * pidlInOut,
				  LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes);
HRESULT SHELL32_GetItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes);
HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlChild, REFCLSID clsid, REFIID iid,
				LPVOID * ppvOut);
HRESULT SHELL32_CoCreateInitSFEx (LPCITEMIDLIST pidlRoot, LPCSTR pathRoot, LPCITEMIDLIST pidlChild, REFCLSID clsid,
				  REFIID iid, LPVOID * ppvOut);
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTR szOut,
				       DWORD dwOutLen);

HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot,
			     LPCSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut);

HRESULT SHELL32_CompareIDs (IShellFolder * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
