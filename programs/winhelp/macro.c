/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
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

#include "windows.h"
#include "commdlg.h"
#include "winhelp.h"
#include "macro.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

/*******      helper functions     *******/
WINHELP_BUTTON**        MACRO_LookupButton(WINHELP_WINDOW* win, LPCSTR name)
{
    WINHELP_BUTTON**    b;

    for (b = &win->first_button; *b; b = &(*b)->next)
        if (!lstrcmpi(name, (*b)->lpszID)) break;
    return b;
}

/******* real macro implementation *******/

void MACRO_About(void)
{
    WINE_FIXME("About()\n");
}

void MACRO_AddAccelerator(LONG u1, LONG u2, LPCSTR str)
{
    WINE_FIXME("AddAccelerator(%lu, %lu, \"%s\")\n", u1, u2, str);
}

void MACRO_ALink(LPCSTR str1, LONG u, LPCSTR str2)
{
    WINE_FIXME("ALink(\"%s\", %lu, \"%s\")\n", str1, u, str2);
}

void MACRO_Annotate(void)
{
    WINE_FIXME("Annotate()\n");
}

void MACRO_AppendItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("AppendItem(\"%s\", \"%s\", \"%s\", \"%s\")\n", str1, str2, str3, str4);
}

void MACRO_Back(void)
{
    WINE_FIXME("Back()\n");
}

void MACRO_BackFlush(void)
{
    WINE_FIXME("BackFlush()\n");
}

void MACRO_BookmarkDefine(void)
{
    WINE_FIXME("BookmarkDefine()\n");
}

void MACRO_BookmarkMore(void)
{
    WINE_FIXME("BookmarkMore()\n");
}

void MACRO_BrowseButtons(void)
{
    MACRO_CreateButton("BTN_PREV", "&<<", "Prev()");
    MACRO_CreateButton("BTN_NEXT", "&>>", "Next()");
}

void MACRO_ChangeButtonBinding(LPCSTR id, LPCSTR macro)
{
    WINHELP_WINDOW*     win = Globals.active_win;
    WINHELP_BUTTON*     button;
    WINHELP_BUTTON**    b;
    LONG                size;
    LPSTR               ptr;

    b = MACRO_LookupButton(win, id);
    if (!*b) {WINE_FIXME("Couldn't find button '%s'\n", id); return;}

    size = sizeof(WINHELP_BUTTON) + lstrlen(id) + 
        lstrlen((*b)->lpszName) + lstrlen(macro) + 3;

    button = HeapAlloc(GetProcessHeap(), 0, size);
    if (!button) return;

    button->next  = (*b)->next;
    button->hWnd  = (*b)->hWnd;
    button->wParam = (*b)->wParam;

    ptr = (char*)button + sizeof(WINHELP_BUTTON);

    lstrcpy(ptr, (LPSTR) id);
    button->lpszID = ptr;
    ptr += lstrlen(id) + 1;

    lstrcpy(ptr, (LPSTR) (*b)->lpszName);
    button->lpszName = ptr;
    ptr += lstrlen((*b)->lpszName) + 1;

    lstrcpy(ptr, (LPSTR) macro);
    button->lpszMacro = ptr;

    *b = button;

    SendMessage(win->hMainWnd, WM_USER, 0, 0);
}

void MACRO_ChangeEnable(LPCSTR id, LPCSTR macro)
{
    MACRO_ChangeButtonBinding(id, macro);
    MACRO_EnableButton(id);
}

void MACRO_ChangeItemBinding(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("ChangeItemBinding(\"%s\", \"%s\")\n", str1, str2);
}

void MACRO_CheckItem(LPCSTR str)
{
    WINE_FIXME("CheckItem(\"%s\")\n", str);
}

void MACRO_CloseSecondarys(void)
{
    WINHELP_WINDOW *win;
    for (win = Globals.win_list; win; win = win->next)
        if (win->lpszName && lstrcmpi(win->lpszName, "main"))
            DestroyWindow(win->hMainWnd);
}

void MACRO_CloseWindow(LPCSTR lpszWindow)
{
    WINHELP_WINDOW *win;
    if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

    for (win = Globals.win_list; win; win = win->next)
        if (win->lpszName && !lstrcmpi(win->lpszName, lpszWindow))
            DestroyWindow(win->hMainWnd);
}

void MACRO_Compare(LPCSTR str)
{
    WINE_FIXME("Compare(\"%s\")\n", str);
}

void MACRO_Contents(void)
{
    if (Globals.active_win->page)
        MACRO_JumpContents(Globals.active_win->page->file->lpszPath, NULL);
}

void MACRO_ControlPanel(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("ControlPanel(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

void MACRO_CopyDialog(void)
{
    WINE_FIXME("CopyDialog()\n");
}

void MACRO_CopyTopic(void)
{
    WINE_FIXME("CopyTopic()\n");
}

void MACRO_CreateButton(LPCSTR id, LPCSTR name, LPCSTR macro)
{
    WINHELP_WINDOW *win = Globals.active_win;
    WINHELP_BUTTON *button, **b;
    LONG            size;
    LPSTR           ptr;

    size = sizeof(WINHELP_BUTTON) + lstrlen(id) + lstrlen(name) + lstrlen(macro) + 3;

    button = HeapAlloc(GetProcessHeap(), 0, size);
    if (!button) return;

    button->next  = 0;
    button->hWnd  = 0;

    ptr = (char*)button + sizeof(WINHELP_BUTTON);

    lstrcpy(ptr, (LPSTR) id);
    button->lpszID = ptr;
    ptr += lstrlen(id) + 1;

    lstrcpy(ptr, (LPSTR) name);
    button->lpszName = ptr;
    ptr += lstrlen(name) + 1;

    lstrcpy(ptr, (LPSTR) macro);
    button->lpszMacro = ptr;

    button->wParam = WH_FIRST_BUTTON;
    for (b = &win->first_button; *b; b = &(*b)->next)
        button->wParam = max(button->wParam, (*b)->wParam + 1);
    *b = button;

    SendMessage(win->hMainWnd, WM_USER, 0, 0);
}

void MACRO_DeleteItem(LPCSTR str)
{
    WINE_FIXME("DeleteItem(\"%s\")\n", str);
}

void MACRO_DeleteMark(LPCSTR str)
{
    WINE_FIXME("DeleteMark(\"%s\")\n", str);
}

void MACRO_DestroyButton(LPCSTR str)
{
    WINE_FIXME("DestroyButton(\"%s\")\n", str);
}

void MACRO_DisableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    b = MACRO_LookupButton(Globals.active_win, id);
    if (!*b) {WINE_FIXME("Couldn't find button '%s'\n", id); return;}

    EnableWindow((*b)->hWnd, FALSE);
}

void MACRO_DisableItem(LPCSTR str)
{
    WINE_FIXME("DisableItem(\"%s\")\n", str);
}

void MACRO_EnableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    b = MACRO_LookupButton(Globals.active_win, id);
    if (!*b) {WINE_FIXME("Couldn't find button '%s'\n", id); return;}

    EnableWindow((*b)->hWnd, TRUE);
}

void MACRO_EnableItem(LPCSTR str)
{
    WINE_FIXME("EnableItem(\"%s\")\n", str);
}

void MACRO_EndMPrint(void)
{
    WINE_FIXME("EndMPrint()\n");
}

void MACRO_ExecFile(LPCSTR str1, LPCSTR str2, LONG u, LPCSTR str3)
{
    WINE_FIXME("ExecFile(\"%s\", \"%s\", %lu, \"%s\")\n", str1, str2, u, str3);
}

void MACRO_ExecProgram(LPCSTR str, LONG u)
{
    WINE_FIXME("ExecProgram(\"%s\", %lu)\n", str, u);
}

void MACRO_Exit(void)
{
    while (Globals.win_list)
        DestroyWindow(Globals.win_list->hMainWnd);
}

void MACRO_ExtAbleItem(LPCSTR str, LONG u)
{
    WINE_FIXME("ExtAbleItem(\"%s\", %lu)\n", str, u);
}

void MACRO_ExtInsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u1, LONG u2)
{
    WINE_FIXME("ExtInsertItem(\"%s\", \"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, str4, u1, u2);
}

void MACRO_ExtInsertMenu(LPCSTR str1, LPCSTR str2, LPCSTR str3, LONG u1, LONG u2)
{
    WINE_FIXME("ExtInsertMenu(\"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, u1, u2);
}

BOOL MACRO_FileExist(LPCSTR str)
{
    return GetFileAttributes(str) != 0xFFFFFFFF;
}

void MACRO_FileOpen(void)
{
    OPENFILENAME openfilename;
    CHAR szPath[MAX_PATHNAME_LEN];
    CHAR szDir[MAX_PATHNAME_LEN];
    CHAR szzFilter[2 * MAX_STRING_LEN + 100];
    LPSTR p = szzFilter;

    LoadString(Globals.hInstance, 0X12B, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.hlp");
    p += strlen(p) + 1;
    LoadString(Globals.hInstance, 0x12A, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.*");
    p += strlen(p) + 1;
    *p = '\0';

    GetCurrentDirectory(sizeof(szDir), szDir);

    szPath[0]='\0';

    openfilename.lStructSize       = 0;
    openfilename.hwndOwner         = Globals.active_win->hMainWnd;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = szzFilter;
    openfilename.lpstrCustomFilter = 0;
    openfilename.nMaxCustFilter    = 0;
    openfilename.nFilterIndex      = 1;
    openfilename.lpstrFile         = szPath;
    openfilename.nMaxFile          = sizeof(szPath);
    openfilename.lpstrFileTitle    = 0;
    openfilename.nMaxFileTitle     = 0;
    openfilename.lpstrInitialDir   = szDir;
    openfilename.lpstrTitle        = 0;
    openfilename.Flags             = 0;
    openfilename.nFileOffset       = 0;
    openfilename.nFileExtension    = 0;
    openfilename.lpstrDefExt       = 0;
    openfilename.lCustData         = 0;
    openfilename.lpfnHook          = 0;
    openfilename.lpTemplateName    = 0;

    if (GetOpenFileName(&openfilename))
        WINHELP_CreateHelpWindowByHash(szPath, 0, "main", FALSE, 0, NULL, SW_SHOWNORMAL);
}

void MACRO_Find(void)
{
    WINE_FIXME("Find()\n");
}

void MACRO_Finder(void)
{
    WINE_FIXME("Finder()\n");
}

void MACRO_FloatingMenu(void)
{
    WINE_FIXME("FloatingMenu()\n");
}

void MACRO_Flush(void)
{
    WINE_FIXME("Flush()\n");
}

void MACRO_FocusWindow(LPCSTR str)
{
    WINE_FIXME("FocusWindow(\"%s\")\n", str);
}

void MACRO_Generate(LPCSTR str, WPARAM w, LPARAM l)
{
    WINE_FIXME("Generate(\"%s\", %x, %lx)\n", str, w, l);
}

void MACRO_GotoMark(LPCSTR str)
{
    WINE_FIXME("GotoMark(\"%s\")\n", str);
}

void MACRO_HelpOn(void)
{
    MACRO_JumpContents((Globals.wVersion > 4) ? "winhelp32.hlp" : "winhelp.hlp", NULL);
}

void MACRO_HelpOnTop(void)
{
    WINE_FIXME("HelpOnTop()\n");
}

void MACRO_History(void)
{
    WINE_FIXME("History()\n");
}

BOOL MACRO_InitMPrint(void)
{
    WINE_FIXME("InitMPrint()\n");
    return FALSE;
}

void MACRO_InsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u)
{
    WINE_FIXME("InsertItem(\"%s\", \"%s\", \"%s\", \"%s\", %lu)\n", str1, str2, str3, str4, u);
}

void MACRO_InsertMenu(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("InsertMenu(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

BOOL MACRO_IsBook(void)
{
    WINE_FIXME("IsBook()\n");
    return TRUE;
}

BOOL MACRO_IsMark(LPCSTR str)
{
    WINE_FIXME("IsMark(\"%s\")\n", str);
    return FALSE;
}

BOOL MACRO_IsNotMark(LPCSTR str)
{
    WINE_FIXME("IsNotMark(\"%s\")\n", str);
    return TRUE;
}

void MACRO_JumpContents(LPCSTR lpszPath, LPCSTR lpszWindow)
{
    WINHELP_CreateHelpWindowByHash(lpszPath, 0, lpszWindow, FALSE, 0, NULL, SW_NORMAL);
}

void MACRO_JumpContext(LPCSTR lpszPath, LPCSTR lpszWindow, LONG context)
{
    WINE_FIXME("JumpContext(\"%s\", \"%s\", %lu)\n", lpszPath, lpszWindow, context);
}

void MACRO_JumpHash(LPCSTR lpszPath, LPCSTR lpszWindow, LONG lHash)
{
    WINHELP_CreateHelpWindowByHash(lpszPath, lHash, lpszWindow, FALSE, 0, NULL, SW_NORMAL);
}

void MACRO_JumpHelpOn(void)
{
    WINE_FIXME("JumpHelpOn()\n");
}

void MACRO_JumpID(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR topic_id)
{
    MACRO_JumpHash(lpszPath, lpszWindow, HLPFILE_Hash(topic_id));
}

void MACRO_JumpKeyword(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR keyword)
{
    WINE_FIXME("JumpKeyword(\"%s\", \"%s\", \"%s\")\n", lpszPath, lpszWindow, keyword);
}

void MACRO_KLink(LPCSTR str1, LONG u, LPCSTR str2, LPCSTR str3)
{
    WINE_FIXME("KLink(\"%s\", %lu, \"%s\", \"%s\")\n", str1, u, str2, str3);
}

void MACRO_Menu(void)
{
    WINE_FIXME("Menu()\n");
}

void MACRO_MPrintHash(LONG u)
{
    WINE_FIXME("MPrintHash(%lu)\n", u);
}

void MACRO_MPrintID(LPCSTR str)
{
    WINE_FIXME("MPrintID(\"%s\")\n", str);
}

void MACRO_Next(void)
{
    if (Globals.active_win->page->next)
        WINHELP_CreateHelpWindowByPage(Globals.active_win->page->next, "main", FALSE, 0, NULL, SW_NORMAL);
}

void MACRO_NoShow(void)
{
    WINE_FIXME("NoShow()\n");
}

void MACRO_PopupContext(LPCSTR str, LONG u)
{
    WINE_FIXME("PopupContext(\"%s\", %lu)\n", str, u);
}

void MACRO_PopupHash(LPCSTR str, LONG u)
{
    WINE_FIXME("PopupHash(\"%s\", %lu)\n", str, u);
}

void MACRO_PopupId(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("PopupId(\"%s\", \"%s\")\n", str1, str2);
}

void MACRO_PositionWindow(LONG i1, LONG i2, LONG u1, LONG u2, LONG u3, LPCSTR str)
{
    WINE_FIXME("PositionWindow(%li, %li, %lu, %lu, %lu, \"%s\")\n", i1, i2, u1, u2, u3, str);
}

void MACRO_Prev(void)
{
    if (Globals.active_win->page->prev)
        WINHELP_CreateHelpWindowByPage(Globals.active_win->page->prev, "main", FALSE, 0, NULL, SW_NORMAL);
}

void MACRO_Print(void)
{
    PRINTDLG printer;

    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = Globals.active_win->hMainWnd;
    printer.hInstance           = Globals.hInstance;
    printer.hDevMode            = 0;
    printer.hDevNames           = 0;
    printer.hDC                 = 0;
    printer.Flags               = 0;
    printer.nFromPage           = 0;
    printer.nToPage             = 0;
    printer.nMinPage            = 0;
    printer.nMaxPage            = 0;
    printer.nCopies             = 0;
    printer.lCustData           = 0;
    printer.lpfnPrintHook       = 0;
    printer.lpfnSetupHook       = 0;
    printer.lpPrintTemplateName = 0;
    printer.lpSetupTemplateName = 0;
    printer.hPrintTemplate      = 0;
    printer.hSetupTemplate      = 0;

    if (PrintDlgA(&printer)) {
        WINE_FIXME("Print()\n");
    }
}

void MACRO_PrinterSetup(void)
{
    WINE_FIXME("PrinterSetup()\n");
}

void MACRO_RegisterRoutine(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
    WINE_FIXME("RegisterRoutine(\"%s\", \"%s\", \"%s\")\n", str1, str2, str3);
}

void MACRO_RemoveAccelerator(LONG u1, LONG u2)
{
    WINE_FIXME("RemoveAccelerator(%lu, %lu)\n", u1, u2);
}

void MACRO_ResetMenu(void)
{
    WINE_FIXME("ResetMenu()\n");
}

void MACRO_SaveMark(LPCSTR str)
{
    WINE_FIXME("SaveMark(\"%s\")\n", str);
}

void MACRO_Search(void)
{
    WINE_FIXME("Search()\n");
}

void MACRO_SetContents(LPCSTR str, LONG u)
{
    WINE_FIXME("SetContents(\"%s\", %lu)\n", str, u);
}

void MACRO_SetHelpOnFile(LPCSTR str)
{
    WINE_FIXME("SetHelpOnFile(\"%s\")\n", str);
}

void MACRO_SetPopupColor(LONG u1, LONG u2, LONG u3)
{
    WINE_FIXME("SetPopupColor(%lu, %lu, %lu)\n", u1, u2, u3);
}

void MACRO_ShellExecute(LPCSTR str1, LPCSTR str2, LONG u1, LONG u2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("ShellExecute(\"%s\", \"%s\", %lu, %lu, \"%s\", \"%s\")\n", str1, str2, u1, u2, str3, str4);
}

void MACRO_ShortCut(LPCSTR str1, LPCSTR str2, WPARAM w, LPARAM l, LPCSTR str)
{
    WINE_FIXME("ShortCut(\"%s\", \"%s\", %x, %lx, \"%s\")\n", str1, str2, w, l, str);
}

void MACRO_TCard(LONG u)
{
    WINE_FIXME("TCard(%lu)\n", u);
}

void MACRO_Test(LONG u)
{
    WINE_FIXME("Test(%lu)\n", u);
}

BOOL MACRO_TestALink(LPCSTR str)
{
    WINE_FIXME("TestALink(\"%s\")\n", str);
    return FALSE;
}

BOOL MACRO_TestKLink(LPCSTR str)
{
    WINE_FIXME("TestKLink(\"%s\")\n", str);
    return FALSE;
}

void MACRO_UncheckItem(LPCSTR str)
{
    WINE_FIXME("UncheckItem(\"%s\")\n", str);
}

void MACRO_UpdateWindow(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("UpdateWindow(\"%s\", \"%s\")\n", str1, str2);
}
