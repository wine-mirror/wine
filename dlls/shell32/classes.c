/*
 *	file type mapping 
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 *
 */
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "commctrl.h"

#include "shell32_main.h"

BOOL32 WINAPI HCR_MapTypeToValue ( LPSTR szExtension, LPSTR szFileType, DWORD len)
{	HKEY	hkey;

	TRACE(shell, "%s %p\n",szExtension, szFileType );
	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,szExtension,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValue32A(hkey,NULL,szFileType,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	TRACE(shell, "-- %s\n", szFileType );

	return TRUE;
}

