/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 */

#include <string.h>

#include "wine/winestring.h"
#include "winbase.h"
#include "commdlg.h"
#include "debug.h"

#include "heap.h"	/* Has to go */

DEFAULT_DEBUG_CHANNEL(commdlg)

#include "cdlg.h"

/***********************************************************************
 *	GetFileTitleA		(COMDLG32.8)
 *
 */
short WINAPI GetFileTitleA(LPCSTR lpFile, LPSTR lpTitle, UINT cbBuf)
{
	int i, len;

	TRACE(commdlg,"(%p %p %d); \n", lpFile, lpTitle, cbBuf);

	if(lpFile == NULL || lpTitle == NULL)
		return -1;

	len = strlen(lpFile);

	if (len == 0)
		return -1;

	if(strpbrk(lpFile, "*[]"))
		return -1;

	len--;

	if(lpFile[len] == '/' || lpFile[len] == '\\' || lpFile[len] == ':')
		return -1;

	for(i = len; i >= 0; i--)
	{
		if (lpFile[i] == '/' ||  lpFile[i] == '\\' ||  lpFile[i] == ':')
		{
			i++;
			break;
		}
	}

	if(i == -1)
		i++;

	TRACE(commdlg,"---> '%s' \n", &lpFile[i]);
    
	len = strlen(lpFile+i)+1;
	if(cbBuf < len)
		return len;

	strncpy(lpTitle, &lpFile[i], len);
	return 0;
}


/***********************************************************************
 *	GetFileTitleW		(COMDLG32.9)
 *
 */
short WINAPI GetFileTitleW(LPCWSTR lpFile, LPWSTR lpTitle, UINT cbBuf)
{
	LPSTR file = HEAP_strdupWtoA(GetProcessHeap(), 0, lpFile);	/* Has to go */
	LPSTR title = HeapAlloc(GetProcessHeap(), 0, cbBuf);
	short	ret;

	ret = GetFileTitleA(file, title, cbBuf);

	lstrcpynAtoW(lpTitle, title, cbBuf);
	HeapFree(GetProcessHeap(), 0, file);
	HeapFree(GetProcessHeap(), 0, title);
	return ret;
}


/***********************************************************************
 *	GetFileTitle16		(COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
	return GetFileTitleA(lpFile, lpTitle, cbBuf);
}

