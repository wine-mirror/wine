/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
 */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "debugtools.h"
#include "ole2.h"
#include "olectl.h"

DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 * OleIconToCursor (OLEAUT32.415)
 */
HCURSOR WINAPI OleIconToCursor( HINSTANCE hinstExe, HICON hicon)
{
	FIXME("(%x,%x), not implemented (olepro32.dll)\n",hinstExe,hicon);
	return S_OK;
}

 
