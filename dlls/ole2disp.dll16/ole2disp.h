/*
 * Copyright 1995 Martin von Loewis
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

#ifndef __WINE_OLEAUT32_OLE2DISP_H
#define __WINE_OLEAUT32_OLE2DISP_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wine/windef16.h"

typedef CHAR OLECHAR16;
typedef LPSTR LPOLESTR16;
typedef LPCSTR LPCOLESTR16;
typedef OLECHAR16 *BSTR16;
typedef BSTR16 *LPBSTR16;

BSTR16 WINAPI SysAllocString16(LPCOLESTR16);
BSTR16 WINAPI SysAllocStringLen16(const char*, int);
VOID   WINAPI SysFreeString16(BSTR16);
INT16  WINAPI SysReAllocString16(LPBSTR16,LPCOLESTR16);
int    WINAPI SysReAllocStringLen16(BSTR16*, const char*,  int);
int    WINAPI SysStringLen16(BSTR16);

typedef struct tagVARIANT16 {
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union {
                    BYTE bVal;
                    SHORT iVal;
                    LONG lVal;
                    FLOAT fltVal;
                    DOUBLE dblVal;
                    VARIANT_BOOL boolVal;
                    SCODE scode;
                    DATE date;
/* BSTR16 */        SEGPTR bstrVal;
                    CY cyVal;
/* IUnknown* */     SEGPTR punkVal;
/* IDispatch* */    SEGPTR pdispVal;
/* SAFEARRAY* */    SEGPTR parray;
/* BYTE* */         SEGPTR pbVal;
/* SHORT* */        SEGPTR piVal;
/* LONG* */         SEGPTR plVal;
/* FLOAT* */        SEGPTR pfltVal;
/* DOUBLE* */       SEGPTR pdblVal;
/* VARIANT_BOOL* */ SEGPTR pboolVal;
/* SCODE* */        SEGPTR pscode;
/* DATE* */         SEGPTR pdate;
/* BSTR16* */       SEGPTR pbstrVal;
/* VARIANT16* */    SEGPTR pvarVal;
/* void* */         SEGPTR byref;
/* CY* */           SEGPTR pcyVal;
/* IUnknown** */    SEGPTR ppunkVal;
/* IDispatch** */   SEGPTR ppdispVal;
/* SAFEARRAY** */   SEGPTR pparray;
    } u;
} VARIANT16;

typedef VARIANT16 VARIANTARG16;

#endif /* !defined(__WINE_OLEAUT32_OLE2DISP_H) */
