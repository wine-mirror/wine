/************************************************************
 *    IShellExtInit
 */

#ifndef __WINE_WINE_OBJ_ISHELLEXTINIT_H
#define __WINE_WINE_OBJ_ISHELLEXTINIT_H

#include "winbase.h"
#include "winuser.h"
#include "wine/obj_base.h"
#include "wine/obj_dataobject.h"

typedef struct 	IShellExtInit IShellExtInit, *LPSHELLEXTINIT;
DEFINE_SHLGUID(IID_IShellExtInit,       0x000214E8L, 0, 0);

#define ICOM_INTERFACE IShellExtInit
#define IShellExtInit_METHODS \
	ICOM_METHOD3(HRESULT, Initialize, LPCITEMIDLIST, pidlFolder, LPDATAOBJECT, lpdobj, HKEY, hkeyProgID)
#define IShellExtInit_IMETHODS \
	IUnknown_IMETHODS \
	IShellExtInit_METHODS
ICOM_DEFINE(IShellExtInit,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
#define IShellExtInit_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IShellExtInit_AddRef(p)			ICOM_CALL(AddRef,p)
#define IShellExtInit_Release(p)		ICOM_CALL(Release,p)
#define IShellExtInit_Initialize(p,a,b,c)	ICOM_CALL3(Initialize,p,a,b,c)
#endif


#endif /* __WINE_WINE_OBJ_ISHELLEXTINIT_H */
