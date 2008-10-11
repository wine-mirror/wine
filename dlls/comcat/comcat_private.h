/*
 *	includes for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "comcat.h"
#include "wine/unicode.h"


/**********************************************************************
 * Dll lifetime tracking declaration for comcat.dll
 */
extern LONG dll_ref;

extern HRESULT ComCatCF_Create(REFIID riid, LPVOID *ppv);

/**********************************************************************
 * StdComponentCategoriesMgr declaration for comcat.dll
 */
typedef struct
{
    const ICatRegisterVtbl *lpVtbl;
    const ICatInformationVtbl *infVtbl;
} ComCatMgrImpl;

extern ComCatMgrImpl COMCAT_ComCatMgr;
extern const ICatInformationVtbl COMCAT_ICatInformation_Vtbl;

/**********************************************************************
 * Global string constant declarations
 */
extern const WCHAR clsid_keyname[6];
