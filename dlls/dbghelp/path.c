/*
 * File path.c - managing path in debugging environments
 *
 * Copyright (C) 2004, Eric Pouech
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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PSTR FileName, PSTR SymbolPath, PSTR DebugFilePath)
{
    HANDLE      h;

    h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        const char* p = strrchr(FileName, '/');
        if (!p) p = FileName;
        if (!SearchPathA(SymbolPath, p, NULL, MAX_PATH, DebugFilePath, NULL))
            return NULL;
        h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}
 
/******************************************************************
 *		FindDebugInfoFileEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFileEx(PSTR FileName, PSTR SymbolPath,
                                  PSTR DebugFilePath, 
                                  PFIND_DEBUG_FILE_CALLBACK Callback,
                                  PVOID CallerData)
{
    FIXME("(%s %s %p %p %p): stub\n", 
          FileName, SymbolPath, DebugFilePath, Callback, CallerData);
    return NULL;
}

/******************************************************************
 *		FindExecutableImage (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImage(PSTR FileName, PSTR SymbolPath, PSTR ImageFilePath)
{
    HANDLE h;
    if (!SearchPathA(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileA(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (DBGHELP.@)
 */
BOOL WINAPI MakeSureDirectoryPathExists(LPCSTR DirPath)
{
    if (CreateDirectoryA(DirPath, NULL)) return TRUE;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           SearchTreeForFile (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFile(LPSTR RootPath, LPSTR InputPathName, 
                              LPSTR OutputPathBuffer)
{
    FIXME("(%s, %s, %s): stub\n",
          debugstr_a(RootPath), debugstr_a(InputPathName),
          debugstr_a(OutputPathBuffer));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
