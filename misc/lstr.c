/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(resource);

extern const WORD OLE2NLS_CT_CType3_LUT[]; /* FIXME: does not belong here */

/***********************************************************************
 *           IsCharAlphaA   (USER.433) (USER32.331)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaA(CHAR x)
{
    return (OLE2NLS_CT_CType3_LUT[(unsigned char)x] & C3_ALPHA);
}

/***********************************************************************
 *           IsCharAlphaNumericA   (USER.434) (USER32.332)
 * FIXME: handle current locale
 */
BOOL WINAPI IsCharAlphaNumericA(CHAR x)
{
    return IsCharAlphaA(x) || isdigit(x) ;
}
