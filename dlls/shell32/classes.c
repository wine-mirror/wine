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

#define MAX_EXTENSION_LENGTH 20

BOOL HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len)
{	HKEY	hkey;
	char	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n",szExtension, szFileType );

	strcpy(szTemp, ".");
	strncat(szTemp, szExtension, MAX_EXTENSION_LENGTH);
	
	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,szTemp,0,0x02000000,&hkey))
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

/***************************************************************************************
*	HCR_GetClassName	[internal]
*
* Gets the name of a registred class
*/
BOOL HCR_GetClassName (REFIID riid, LPSTR szDest, DWORD len)
{	HKEY	hkey;
	char	xriid[50];

	strcpy(xriid,"CLSID\\");
	WINE_StringFromCLSID(riid,&xriid[strlen(xriid)]);

	TRACE("%s\n",xriid );

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,xriid,0,KEY_READ,&hkey))
	{ return FALSE;
	}

	if (RegQueryValueExA(hkey,"",0,NULL,szDest,&len))
	{ RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	TRACE("-- %s\n", szDest);

	return TRUE;
}

/***************************************************************************************
*	HCR_GetFolderAttributes	[internal]
*
* gets the folder attributes of a class
*/
BOOL HCR_GetFolderAttributes (REFIID riid, LPDWORD szDest)
{	HKEY	hkey;
	char	xriid[60];
	DWORD	attributes;
	DWORD	len = 4;

	strcpy(xriid,"CLSID\\");
	WINE_StringFromCLSID(riid,&xriid[strlen(xriid)]);
	TRACE("%s\n",xriid );

	strcat (xriid, "\\ShellFolder");

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,xriid,0,KEY_READ,&hkey))
	{
	  return FALSE;
	}

	if (RegQueryValueExA(hkey,"Attributes",0,NULL,(LPBYTE)&attributes,&len))
	{
	  RegCloseKey(hkey);
	  return FALSE;
	}	

	RegCloseKey(hkey);

	TRACE("-- 0x%08lx\n", attributes);

	*szDest = attributes;

	return TRUE;
}

