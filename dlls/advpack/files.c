/*
 * Advpack file functions
 *
 * Copyright 2006 James Hawkins
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "advpub.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

/***********************************************************************
 *      FileSaveMarkNotExist (ADVPACK.@)
 *
 * Marks the files in the file list as not existing so they won't be
 * backed up during a save.
 *
 * PARAMS
 *   pszFileList [I] NULL-separated list of filenames.
 *   pszDir      [I] Path of the backup directory.
 *   pszBaseName [I] Basename of the backup files.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI FileSaveMarkNotExist(LPSTR pszFileList, LPSTR pszDir, LPSTR pszBaseName)
{
    FIXME("(%p, %p, %p) stub\n", pszFileList, pszDir, pszBaseName);

    return E_FAIL;
}

/***********************************************************************
 *      FileSaveRestore (ADVPACK.@)
 *
 * Saves or restores the files in the specified file list.
 *
 * PARAMS
 *   hDlg        [I] Handle to the dialog used for the display.
 *   pszFileList [I] NULL-separated list of filenames.
 *   pszDir      [I] Path of the backup directory.
 *   pszBaseName [I] Basename of the backup files.
 *   dwFlags     [I] See advpub.h.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If pszFileList is NULL on restore, all files will be restored.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI FileSaveRestore(HWND hDlg, LPSTR pszFileList, LPSTR pszDir,
                               LPSTR pszBaseName, DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %p, %ld) stub\n", hDlg, pszFileList, pszDir,
          pszBaseName, dwFlags);

    return E_FAIL;
}

/***********************************************************************
 *      FileSaveRestoreOnINF (ADVPACK.@)
 *
 *
 * PARAMS
 *   hWnd              [I] Handle to the window used for the display.
 *   pszTitle          [I] Title of the window.
 *   pszINF            [I] Fully-qualified INF filename.
 *   pszSection        [I] GenInstall INF section name.
 *   pszBackupDir      [I] Directory to store the backup file.
 *   pszBaseBackupFile [I] Basename of the backup files.
 *   dwFlags           [I] See advpub.h
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   If pszSection is NULL, the default section will be used.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI FileSaveRestoreOnINF(HWND hWnd, PCSTR pszTitle, PCSTR pszINF,
                                    PCSTR pszSection, PCSTR pszBackupDir,
                                    PCSTR pszBaseBackupFile, DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %ld) stub\n", hWnd, pszTitle, pszINF,
          pszSection, pszBackupDir, pszBaseBackupFile, dwFlags);

    return E_FAIL;
}
