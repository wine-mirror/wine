/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "winbase.h"
#include "winnls.h"
#include "commdlg.h"
#include "wine/debug.h"

#include "heap.h"	/* Has to go */

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

/***********************************************************************
 *	GetFileTitleA		(COMDLG32.@)
 *
 */
short WINAPI GetFileTitleA(LPCSTR lpFile, LPSTR lpTitle, UINT cbBuf)
{
	int i, len;

	TRACE("(%p %p %d); \n", lpFile, lpTitle, cbBuf);

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

	TRACE("---> '%s' \n", &lpFile[i]);

	len = strlen(lpFile+i)+1;
	if(cbBuf < len)
		return len;

	strncpy(lpTitle, &lpFile[i], len);
	return 0;
}


/***********************************************************************
 *	GetFileTitleW		(COMDLG32.@)
 *
 */
short WINAPI GetFileTitleW(LPCWSTR lpFile, LPWSTR lpTitle, UINT cbBuf)
{
	LPSTR file = HEAP_strdupWtoA(GetProcessHeap(), 0, lpFile);	/* Has to go */
	LPSTR title = HeapAlloc(GetProcessHeap(), 0, cbBuf);
	short	ret;

	ret = GetFileTitleA(file, title, cbBuf);

        if (cbBuf > 0 && !MultiByteToWideChar( CP_ACP, 0, title, -1, lpTitle, cbBuf ))
            lpTitle[cbBuf-1] = 0;
	HeapFree(GetProcessHeap(), 0, file);
	HeapFree(GetProcessHeap(), 0, title);
	return ret;
}


/***********************************************************************
 *	GetFileTitle		(COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
	return GetFileTitleA(lpFile, lpTitle, cbBuf);
}

