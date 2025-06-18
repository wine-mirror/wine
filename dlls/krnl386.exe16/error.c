/*
 * Log internal errors
 *
 * Copyright 1997 Andrew Taylor
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "wine/winbase16.h"
#include "wine/debug.h"


/* LogParamError and LogError values */

/* Error modifier bits */
#define ERR_WARNING             0x8000
#define ERR_PARAM               0x4000

#define ERR_SIZE_MASK           0x3000
#define ERR_BYTE                0x1000
#define ERR_WORD                0x2000
#define ERR_DWORD               0x3000


/* LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE           0x6001
#define ERR_BAD_FLAGS           0x6002
#define ERR_BAD_INDEX           0x6003
#define ERR_BAD_DVALUE          0x7004
#define ERR_BAD_DFLAGS          0x7005
#define ERR_BAD_DINDEX          0x7006
#define ERR_BAD_PTR             0x7007
#define ERR_BAD_FUNC_PTR        0x7008
#define ERR_BAD_SELECTOR        0x6009
#define ERR_BAD_STRING_PTR      0x700a
#define ERR_BAD_HANDLE          0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS          0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069


/* LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001
#define ERR_GREALLOC            0x0002
#define ERR_GLOCK               0x0003
#define ERR_LALLOC              0x0004
#define ERR_LREALLOC            0x0005
#define ERR_LLOCK               0x0006
#define ERR_ALLOCRES            0x0007
#define ERR_LOCKRES             0x0008
#define ERR_LOADMODULE          0x0009

/* USER errors */
#define ERR_CREATEDLG           0x0040
#define ERR_CREATEDLG2          0x0041
#define ERR_REGISTERCLASS       0x0042
#define ERR_DCBUSY              0x0043
#define ERR_CREATEWND           0x0044
#define ERR_STRUCEXTRA          0x0045
#define ERR_LOADSTR             0x0046
#define ERR_LOADMENU            0x0047
#define ERR_NESTEDBEGINPAINT    0x0048
#define ERR_BADINDEX            0x0049
#define ERR_CREATEMENU          0x004a

/* GDI errors */
#define ERR_CREATEDC            0x0080
#define ERR_CREATEMETA          0x0081
#define ERR_DELOBJSELECTED      0x0082
#define ERR_SELBITMAP           0x0083


#define ErrorString(manifest) { manifest, # manifest }

static const struct {
	int constant;
	const char *name;
} ErrorStrings[] = {

	ErrorString(ERR_GALLOC),
	ErrorString(ERR_GREALLOC),
	ErrorString(ERR_GLOCK),
	ErrorString(ERR_LALLOC),
	ErrorString(ERR_LREALLOC),
	ErrorString(ERR_LLOCK),
	ErrorString(ERR_ALLOCRES),
	ErrorString(ERR_LOCKRES),
	ErrorString(ERR_LOADMODULE),
	ErrorString(ERR_CREATEDLG),
	ErrorString(ERR_CREATEDLG2),
	ErrorString(ERR_REGISTERCLASS),
	ErrorString(ERR_DCBUSY),
	ErrorString(ERR_CREATEWND),
	ErrorString(ERR_STRUCEXTRA),
	ErrorString(ERR_LOADSTR),
	ErrorString(ERR_LOADMENU),
	ErrorString(ERR_NESTEDBEGINPAINT),
	ErrorString(ERR_BADINDEX),
	ErrorString(ERR_CREATEMENU),
	ErrorString(ERR_CREATEDC),
	ErrorString(ERR_CREATEMETA),
	ErrorString(ERR_DELOBJSELECTED),
	ErrorString(ERR_SELBITMAP)
};

static const struct {
	int constant;
	const char *name;
} ParamErrorStrings[] = {

	ErrorString(ERR_BAD_VALUE),
	ErrorString(ERR_BAD_FLAGS),
	ErrorString(ERR_BAD_INDEX),
	ErrorString(ERR_BAD_DVALUE),
	ErrorString(ERR_BAD_DFLAGS),
	ErrorString(ERR_BAD_DINDEX),
	ErrorString(ERR_BAD_PTR),
	ErrorString(ERR_BAD_FUNC_PTR),
	ErrorString(ERR_BAD_SELECTOR),
	ErrorString(ERR_BAD_STRING_PTR),
	ErrorString(ERR_BAD_HANDLE),
	ErrorString(ERR_BAD_HINSTANCE),
	ErrorString(ERR_BAD_HMODULE),
	ErrorString(ERR_BAD_GLOBAL_HANDLE),
	ErrorString(ERR_BAD_LOCAL_HANDLE),
	ErrorString(ERR_BAD_ATOM),
	ErrorString(ERR_BAD_HFILE),
	ErrorString(ERR_BAD_HWND),
	ErrorString(ERR_BAD_HMENU),
	ErrorString(ERR_BAD_HCURSOR),
	ErrorString(ERR_BAD_HICON),
	ErrorString(ERR_BAD_HDWP),
	ErrorString(ERR_BAD_CID),
	ErrorString(ERR_BAD_HDRVR),
	ErrorString(ERR_BAD_COORDS),
	ErrorString(ERR_BAD_GDI_OBJECT),
	ErrorString(ERR_BAD_HDC),
	ErrorString(ERR_BAD_HPEN),
	ErrorString(ERR_BAD_HFONT),
	ErrorString(ERR_BAD_HBRUSH),
	ErrorString(ERR_BAD_HBITMAP),
	ErrorString(ERR_BAD_HRGN),
	ErrorString(ERR_BAD_HPALETTE),
	ErrorString(ERR_BAD_HMETAFILE)
};

#undef  ErrorString

/***********************************************************************
*	GetErrorString (internal)
*/
static const char *GetErrorString(UINT16 uErr)
{
  static char buffer[80];
  unsigned int n;

  for (n = 0; n < ARRAY_SIZE(ErrorStrings); n++) {
    if (uErr == ErrorStrings[n].constant)
      return ErrorStrings[n].name;
  }

  sprintf(buffer, "%x", uErr);
  return buffer;
}


/***********************************************************************
*	GetParamErrorString (internal)
*/
static const char *GetParamErrorString(UINT16 uErr) {
	static char buffer[80];

	if (uErr & ERR_WARNING) {
		strcpy(buffer, "ERR_WARNING | ");
		uErr &= ~ERR_WARNING;
	} else
		buffer[0] = '\0';

	{
		unsigned int n;

		for (n = 0; n < ARRAY_SIZE(ParamErrorStrings); n++) {
			if (uErr == ParamErrorStrings[n].constant) {
				strcat(buffer, ParamErrorStrings[n].name);
				return buffer;
			}
		}
	}

	sprintf(buffer + strlen(buffer), "%x", uErr);
	return buffer;
}


/***********************************************************************
 *		SetLastError (KERNEL.147)
 */
void WINAPI SetLastError16( DWORD error )
{
    SetLastError( error );
}


/***********************************************************************
 *		GetLastError (KERNEL.148)
 */
DWORD WINAPI GetLastError16(void)
{
    return GetLastError();
}


/***********************************************************************
*	LogError (KERNEL.324)
*/
VOID WINAPI LogError16(UINT16 uErr, LPVOID lpvInfo)
{
	MESSAGE("(%s, %p)\n", GetErrorString(uErr), lpvInfo);
}


/***********************************************************************
*	LogParamError (KERNEL.325)
*/
void WINAPI LogParamError16(UINT16 uErr, FARPROC16 lpfn, LPVOID lpvParam)
{
	/* FIXME: is it possible to get the module name/function
	 * from the lpfn param?
	 */
	MESSAGE("(%s, %p, %p)\n", GetParamErrorString(uErr), lpfn, lpvParam);
}

/***********************************************************************
*	K327 (KERNEL.327)
*/
void WINAPI HandleParamError( CONTEXT *context )
{
	UINT16 uErr = LOWORD(context->Ebx);
        FARPROC16 lpfn = (FARPROC16)MAKESEGPTR( context->SegCs, context->Eip );
        LPVOID lpvParam = (LPVOID)MAKELONG( LOWORD(context->Eax), LOWORD(context->Ecx) );

	LogParamError16( uErr, lpfn, lpvParam );

	if (!(uErr & ERR_WARNING))
	{
		/* Abort current procedure: Unwind stack frame and jump
		   to error handler (location at [bp-2]) */

		WORD *stack = MapSL( MAKESEGPTR( context->SegSs, LOWORD(context->Ebp) ));
		context->Esp = LOWORD(context->Ebp) - 2;
		context->Ebp = stack[0] & 0xfffe;

		context->Eip = stack[-1];

		context->Eax = context->Ecx = context->Edx = 0;
		context->SegEs = 0;
	}
}
