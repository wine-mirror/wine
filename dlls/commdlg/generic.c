/*
 * COMMDLG functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "win.h"
#include "heap.h"
#include "message.h"
#include "commdlg.h"
#include "resource.h"
#include "dialog.h"
#include "dlgs.h"
#include "module.h"
#include "drive.h"
#include "debug.h"
#include "font.h"
#include "winproc.h"

static	DWORD 		CommDlgLastError = 0;



/***********************************************************************
 *           CommDlgExtendedError   (COMMDLG.26)
 */
DWORD WINAPI CommDlgExtendedError(void)
{
  return CommDlgLastError;
}


