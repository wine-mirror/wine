/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ENUMIDLIST_H__
#define __ENUMIDLIST_H__

#include "shlobj.h"

LPENUMIDLIST IEnumIDList_Constructor(void);
BOOL AddToEnumList(IEnumIDList * iface, LPITEMIDLIST pidl);

/* old interface that's going away soon: */
/* kind of enumidlist */
#define EIDL_DESK   0
#define EIDL_MYCOMP 1
#define EIDL_FILE   2

IEnumIDList * IEnumIDList_BadConstructor(LPCSTR lpszPath, DWORD dwFlags,
 DWORD dwKind);

#endif /* ndef __ENUMIDLIST_H__ */
