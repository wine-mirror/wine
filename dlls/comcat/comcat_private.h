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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COM_NO_WINDOWS_H
#include <stdarg.h>

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
extern DWORD dll_ref;

/**********************************************************************
 * ClassFactory declaration for comcat.dll
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD ref;
} ClassFactoryImpl;

extern ClassFactoryImpl COMCAT_ClassFactory;

/**********************************************************************
 * StdComponentCategoriesMgr declaration for comcat.dll
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IUnknown) *unkVtbl;
    ICOM_VTABLE(ICatRegister) *regVtbl;
    ICOM_VTABLE(ICatInformation) *infVtbl;
    DWORD ref;
} ComCatMgrImpl;

extern ComCatMgrImpl COMCAT_ComCatMgr;
extern ICOM_VTABLE(ICatRegister) COMCAT_ICatRegister_Vtbl;
extern ICOM_VTABLE(ICatInformation) COMCAT_ICatInformation_Vtbl;

/**********************************************************************
 * Global string constant declarations
 */
extern const WCHAR clsid_keyname[6];
