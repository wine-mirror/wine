/*
 *	file type mapping 
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 *
 */
#include <stdlib.h>
#include <string.h>
#include "debugtools.h"
#include "winerror.h"
#include "winreg.h"

#include "shlobj.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

BOOL HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len)
{	HKEY	hkey;

	TRACE("%s %p\n",szExtension, szFileType );

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,szExtension,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValueA(hkey,NULL,szFileType,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	TRACE("-- %s\n", szFileType );

	return TRUE;
}
BOOL HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len )
{	HKEY	hkey;
	char	sTemp[256];
	
	TRACE("%s %s\n",szClass, szVerb );

	sprintf(sTemp, "%s\\shell\\%s\\command",szClass, szVerb);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,sTemp,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValueA(hkey,NULL,szDest,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	
	RegCloseKey(hkey);

	TRACE("-- %s\n", szDest );

	return TRUE;

}
/***************************************************************************************
*	HCR_GetDefaultIcon	[internal]
*
* Gets the icon for a filetype
*/
BOOL HCR_GetDefaultIcon (LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr)
{	HKEY	hkey;
	char	sTemp[256];
	char	sNum[5];

	TRACE("%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,sTemp,0,0x02000000,&hkey))
	{ return FALSE;
	}

	if (RegQueryValueA(hkey,NULL,szDest,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	if (ParseFieldA (szDest, 2, sNum, 5))
	{ *dwNr=atoi(sNum);
	}
	
	ParseFieldA (szDest, 1, szDest, len);
	
	TRACE("-- %s %li\n", szDest, *dwNr );

	return TRUE;

}

