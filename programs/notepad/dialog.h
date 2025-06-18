/*
 *  Notepad (dialog.h)
 *
 *  Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

VOID DIALOG_FileNew(VOID);
VOID DIALOG_FileOpen(VOID);
BOOL DIALOG_FileSave(VOID);
BOOL DIALOG_FileSaveAs(VOID);
VOID DIALOG_FilePrint(VOID);
VOID DIALOG_FilePageSetup(VOID);
VOID DIALOG_FilePrinterSetup(VOID);
VOID DIALOG_FileExit(VOID);

VOID DIALOG_EditUndo(VOID);
VOID DIALOG_EditCut(VOID);
VOID DIALOG_EditCopy(VOID);
VOID DIALOG_EditPaste(VOID);
VOID DIALOG_EditDelete(VOID);
VOID DIALOG_EditSelectAll(VOID);
VOID DIALOG_EditTimeDate(VOID);
VOID DIALOG_EditWrap(VOID);

VOID DIALOG_Search(VOID);
VOID DIALOG_SearchNext(VOID);
VOID DIALOG_Replace(VOID);
void DIALOG_EditGoTo(void);

VOID DIALOG_SelectFont(VOID);

VOID DIALOG_HelpContents(VOID);
VOID DIALOG_HelpSearch(VOID);
VOID DIALOG_HelpAboutNotepad(VOID);

VOID DIALOG_TimeDate(VOID);
int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCWSTR szString, DWORD dwFlags);

/* utility functions */
VOID ShowLastError(void);
void UpdateWindowCaption(void);
BOOL FileExists(LPCWSTR szFilename);
BOOL DoCloseFile(void);
void DoOpenFile(LPCWSTR szFileName, ENCODING enc);
