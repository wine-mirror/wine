/*
 *	file type mapping 
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 *
 */
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "winerror.h"
#include "winreg.h"

#include "shlobj.h"
#include "shell32_main.h"

BOOL32 HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len)
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
BOOL32 HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len )
{	HKEY	hkey;
	char	sTemp[256];
	
	TRACE(shell, "%s %s\n",szClass, szVerb );

	sprintf(sTemp, "%s\\shell\\%s\\command",szClass, szVerb);

	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,sTemp,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValue32A(hkey,NULL,szDest,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	
	RegCloseKey(hkey);

	TRACE(shell, "-- %s\n", szDest );

	return TRUE;

}
/***************************************************************************************
*	HCR_GetDefaultIcon	[internal]
*
* Gets the icon for a filetype
*/
BOOL32 HCR_GetDefaultIcon (LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr)
{	HKEY	hkey;
	char	sTemp[256];
	char	sNum[5];

	TRACE(shell, "%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,sTemp,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValue32A(hkey,NULL,szDest,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	if (ParseField32A (szDest, 2, sNum, 5))
	{ *dwNr=atoi(sNum);
	}
	
	ParseField32A (szDest, 1, szDest, len);
	
	TRACE(shell, "-- %s %li\n", szDest, *dwNr );

	return TRUE;

}

