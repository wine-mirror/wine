/*
 * SafeArray test program
 *
 * Copyright 2002 Marcus Meissner
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "wine/test.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

#define VARTYPE_NOT_SUPPORTED 0
static ULONG vttypes[] = {
  /* this is taken from wtypes.h.  Only [S]es are supported by the SafeArray */
VARTYPE_NOT_SUPPORTED,  /* VT_EMPTY    [V]   [P]    nothing			*/
VARTYPE_NOT_SUPPORTED,  /* VT_NULL     [V]   [P]    SQL style Nul	*/
2,                      /* VT_I2       [V][T][P][S] 2 byte signed int */
4,                      /* VT_I4       [V][T][P][S] 4 byte signed int */
4,                      /* VT_R4       [V][T][P][S] 4 byte real	*/
8,                      /* VT_R8       [V][T][P][S] 8 byte real	*/
8,                      /* VT_CY       [V][T][P][S] currency */
8,                      /* VT_DATE     [V][T][P][S] date */
sizeof(BSTR),           /* VT_BSTR     [V][T][P][S] OLE Automation string*/
sizeof(LPDISPATCH),     /* VT_DISPATCH [V][T][P][S] IDispatch *	*/
4,                      /* VT_ERROR    [V][T]   [S] SCODE	*/
2,                      /* VT_BOOL     [V][T][P][S] True=-1, False=0*/
sizeof(VARIANT),        /* VT_VARIANT  [V][T][P][S] VARIANT *	*/
sizeof(LPUNKNOWN),      /* VT_UNKNOWN  [V][T]   [S] IUnknown * */
sizeof(DECIMAL),        /* VT_DECIMAL  [V][T]   [S] 16 byte fixed point	*/
VARTYPE_NOT_SUPPORTED,                         /* no VARTYPE here.....	*/
1,			/* VT_I1          [T]   [S] signed char		*/
1,                      /* VT_UI1      [V][T][P][S] unsigned char	*/
2,			/* VT_UI2         [T][P][S] unsigned short	*/
4,			/* VT_UI4         [T][P][S] unsigned int	*/
VARTYPE_NOT_SUPPORTED,	/* VT_I8          [T][P]    signed 64-bit int			*/
VARTYPE_NOT_SUPPORTED,	/* VT_UI8         [T][P]    unsigned 64-bit int		*/
sizeof(INT),		/* VT_INT         [T]       signed machine int		*/
sizeof(UINT),		/* VT_UINT        [T]       unsigned machine int	*/
VARTYPE_NOT_SUPPORTED,	/* VT_VOID        [T]       C style void			*/
VARTYPE_NOT_SUPPORTED,	/* VT_HRESULT     [T]       Standard return type	*/
VARTYPE_NOT_SUPPORTED,	/* VT_PTR         [T]       pointer type			*/
VARTYPE_NOT_SUPPORTED,	/* VT_SAFEARRAY   [T]       (use VT_ARRAY in VARIANT)*/
VARTYPE_NOT_SUPPORTED,	/* VT_CARRAY      [T]       C style array			*/
VARTYPE_NOT_SUPPORTED,	/* VT_USERDEFINED [T]       user defined type			*/
VARTYPE_NOT_SUPPORTED,	/* VT_LPSTR       [T][P]    null terminated string	*/
VARTYPE_NOT_SUPPORTED,	/* VT_LPWSTR      [T][P]    wide null term string		*/
VARTYPE_NOT_SUPPORTED,	/* VT_FILETIME       [P]    FILETIME			*/
VARTYPE_NOT_SUPPORTED,	/* VT_BLOB           [P]    Length prefixed bytes */
VARTYPE_NOT_SUPPORTED,	/* VT_STREAM         [P]    Name of stream follows		*/
VARTYPE_NOT_SUPPORTED,	/* VT_STORAGE        [P]    Name of storage follows	*/
VARTYPE_NOT_SUPPORTED,	/* VT_STREAMED_OBJECT[P]    Stream contains an object*/
VARTYPE_NOT_SUPPORTED,	/* VT_STORED_OBJECT  [P]    Storage contains object*/
VARTYPE_NOT_SUPPORTED,	/* VT_BLOB_OBJECT    [P]    Blob contains an object*/
VARTYPE_NOT_SUPPORTED,	/* VT_CF             [P]    Clipboard format			*/
VARTYPE_NOT_SUPPORTED,	/* VT_CLSID          [P]    A Class ID			*/
VARTYPE_NOT_SUPPORTED,	/* VT_VECTOR         [P]    simple counted array		*/
VARTYPE_NOT_SUPPORTED,	/* VT_ARRAY    [V]          SAFEARRAY*			*/
VARTYPE_NOT_SUPPORTED 	/* VT_BYREF    [V]          void* for local use	*/
};

START_TEST(safearray)
{
	SAFEARRAY 	*a;
	unsigned short i;
	HRESULT 	hres;
	SAFEARRAYBOUND	bound;

	hres = SafeArrayAllocDescriptor(0,&a);
	ok(E_INVALIDARG == hres,"SAAD(0) failed with hres %lx",hres);

	hres=SafeArrayAllocDescriptor(1,&a);
	ok(S_OK == hres,"SAAD(1) failed with %lx",hres);

	for (i=1;i<100;i++) {
		hres=SafeArrayAllocDescriptor(i,&a);
		ok(S_OK == hres,"SAAD(%d) failed with %lx\n",i,hres);
		
		ok(a->cDims == i,"a->cDims not initialised?\n");

		hres=SafeArrayDestroyDescriptor(a);
		ok(S_OK == hres,"SADD failed with %lx\n",hres);
	}

	hres=SafeArrayAllocDescriptor(65535,&a);
	ok(S_OK == hres,"SAAD(65535) failed with %lx",hres);

	hres=SafeArrayDestroyDescriptor(a);
	ok(S_OK == hres,"SADD failed with %lx",hres);

	hres=SafeArrayAllocDescriptor(65536,&a);
	ok(E_INVALIDARG == hres,"SAAD(65536) failed with %lx",hres);

	hres=SafeArrayAllocDescriptor(1,NULL);
	ok(E_POINTER == hres,"SAAD(1,NULL) failed with %lx",hres);

	
	bound.cElements	= 1;
	bound.lLbound	= 0;
	a = SafeArrayCreate(-1, 1, &bound);
	ok(NULL == a,"SAC(-1,1,[1,0]) not failed?\n");
	
	for (i=0;i<sizeof(vttypes)/sizeof(vttypes[0]);i++) {
		a = SafeArrayCreate(i, 1, &bound);
		ok(	((a == NULL) && (vttypes[i] == 0)) ||
			((a != NULL) && (vttypes[i] == a->cbElements)),
		"SAC(%d,1,[1,0]), result %ld, expected %ld\n",i,(a?a->cbElements:0),vttypes[i]
		);
	}
}
