/*
 * Log internal errors 
 *
 * Copyright 1997 Andrew Taylor
 */

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "stackframe.h"
#include "debugtools.h"

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
#define ErrorStringCount (sizeof(ErrorStrings) / sizeof(ErrorStrings[0]))
#define ParamErrorStringCount (sizeof(ParamErrorStrings) / sizeof(ParamErrorStrings[0]))

/***********************************************************************
*	GetErrorString (internal)
*/
static const char *GetErrorString(UINT16 uErr) 
{
  static char buffer[80];
  int i;

  for (i = 0; i < ErrorStringCount; i++) {
    if (uErr == ErrorStrings[i].constant)
      return ErrorStrings[i].name;
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
		int i;

		for (i = 0; i < ParamErrorStringCount; i++) {
			if (uErr == ParamErrorStrings[i].constant) {
				strcat(buffer, ParamErrorStrings[i].name);
				return buffer;
			}
		}
	}

	sprintf(buffer + strlen(buffer), "%x", uErr);
	return buffer;
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
*	HandleParamError (KERNEL.327)
*/
void WINAPI HandleParamError( CONTEXT86 *context )
{
	UINT16 uErr = BX_reg( context );
        FARPROC16 lpfn = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( CS_reg(context),
                                                           EIP_reg(context) );
        LPVOID lpvParam = (LPVOID)MAKELONG( AX_reg( context ), 
                                            CX_reg( context ) );
	
	LogParamError16( uErr, lpfn, lpvParam );

	if (!(uErr & ERR_WARNING))
	{
		/* Abort current procedure: Unwind stack frame and jump
		   to error handler (location at [bp-2]) */

		WORD *stack = PTR_SEG_OFF_TO_LIN( SS_reg( context ), 
						  LOWORD(EBP_reg( context )) );
		ESP_reg( context ) = LOWORD(EBP_reg( context )) - 2;
		EBP_reg( context ) = stack[0] & 0xfffe;

		EIP_reg( context ) = stack[-1];

		EAX_reg( context ) = ECX_reg( context ) = EDX_reg( context ) = 0;
		ES_reg( context) = 0;
	}
}

