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

#ifndef _WINE_FINDDLG_H
#define _WINE_FINDDLG_H

#define BUFFILE 512
#define BUFFILEALLOC 512 * sizeof(WCHAR)

struct FSPRIVATE
{
    HWND hwnd; /* file dialog window handle */
    BOOL hook; /* TRUE if the dialog is hooked */
    UINT lbselchstring; /* registered message id */
    UINT fileokstring; /* registered message id */
    LPARAM lParam; /* save original lparam */
    HANDLE16 hDlgTmpl16; /* handle for resource 16 */
    HANDLE16 hResource16; /* handle for allocated resource 16 */
    HANDLE16 hGlobal16; /* 16 bits mem block (resources) */
    LPCVOID template; /* template for 32 bits resource */
    BOOL open; /* TRUE if open dialog, FALSE if save dialog */
    OPENFILENAMEW *ofnW; /* original structure or work struct */
    OPENFILENAMEA *ofnA; /* original structure if 32bits ansi dialog */
    OPENFILENAME16 *ofn16; /* original structure if 16 bits dialog */
};

#define LFSPRIVATE struct FSPRIVATE *
#define LFS16 1
#define LFS32A 2
#define LFS32W 3
#define OFN_PROP "FILEDLG_OFN"

static const WCHAR FILE_star[] = {'*','.','*', 0};
static const WCHAR FILE_bslash[] = {'\\', 0};
static const WCHAR FILE_specc[] = {'%','c',':', 0};
static const int fldrHeight = 16;
static const int fldrWidth = 20;

/* Internal Functions
 * Do not Export to other applications or dlls
 */

LPWSTR FILEDLG_GetFileType(LPWSTR cfptr, LPWSTR fptr, WORD index);
void FILEDLG_MapDrawItemStruct(LPDRAWITEMSTRUCT16 lpdis16, LPDRAWITEMSTRUCT lpdis);
BOOL FILEDLG_ScanDir(HWND hWnd, LPWSTR newPath);
LONG FILEDLG_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
       int savedlg, LPDRAWITEMSTRUCT lpdis);
LRESULT FILEDLG_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
       UINT control, LFSPRIVATE lfs );
BOOL FileDlg_Init(void);
void FILEDLG_DestroyPrivate(LFSPRIVATE lfs);
LFSPRIVATE FILEDLG_AllocPrivate(LPARAM lParam, int type, UINT dlgType);

#endif /* _WINE_FINDDLG_H */
