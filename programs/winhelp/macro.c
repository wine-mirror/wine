/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <stdio.h>
#include "windows.h"
#include "commdlg.h"
#ifdef WINELIB
#include "shell.h"
#endif
#include "winhelp.h"
#include "macro.h"

VOID MACRO_About(VOID)
{
  fprintf(stderr, "About()\n");
}

VOID MACRO_AddAccelerator(LONG u1, LONG u2, LPCSTR str)
{
  fprintf(stderr, "AddAccelerator(%lu, %lu, \"%s\")\n", u1, u2, str);
}

VOID MACRO_ALink(LPCSTR str1, LONG u, LPCSTR str2)
{
  fprintf(stderr, "ALink(\"%s\", %lu, \"%s\")\n", str1, u, str2);
}

VOID MACRO_Annotate(VOID)
{
  fprintf(stderr, "Annotate()\n");
}

VOID MACRO_AppendItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4)
{
  fprintf(stderr, "AppendItem(\"%s\", \"%s\", \"%s\", \"%s\")\n", str1, str2, str3, str4);
}

VOID MACRO_Back(VOID)
{
  fprintf(stderr, "Back()\n");
}

VOID MACRO_BackFlush(VOID)
{
  fprintf(stderr, "BackFlush()\n");
}

VOID MACRO_BookmarkDefine(VOID)
{
  fprintf(stderr, "BookmarkDefine()\n");
}

VOID MACRO_BookmarkMore(VOID)
{
  fprintf(stderr, "BookmarkMore()\n");
}

VOID MACRO_BrowseButtons(VOID)
{
  MACRO_CreateButton("BTN_PREV", "&<<", "Prev()");
  MACRO_CreateButton("BTN_NEXT", "&>>", "Next()");
}

VOID MACRO_ChangeButtonBinding(LPCSTR str1, LPCSTR str2)
{
  fprintf(stderr, "ChangeButtonBinding(\"%s\", \"%s\")\n", str1, str2);
}

VOID MACRO_ChangeEnable(LPCSTR str1, LPCSTR str2)
{
  fprintf(stderr, "ChangeEnable(\"%s\", \"%s\")\n", str1, str2);
}

VOID MACRO_ChangeItemBinding(LPCSTR str1, LPCSTR str2)
{
  fprintf(stderr, "ChangeItemBinding(\"%s\", \"%s\")\n", str1, str2);
}

VOID MACRO_CheckItem(LPCSTR str)
{
  fprintf(stderr, "CheckItem(\"%s\")\n", str);
}

VOID MACRO_CloseSecondarys(VOID)
{
  WINHELP_WINDOW *win;
  for (win = Globals.win_list; win; win = win->next)
    if (win->lpszName && lstrcmpi(win->lpszName, "main"))
      DestroyWindow(win->hMainWnd);
}

VOID MACRO_CloseWindow(LPCSTR lpszWindow)
{
  WINHELP_WINDOW *win;
  if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

  for (win = Globals.win_list; win; win = win->next)
    if (win->lpszName && !lstrcmpi(win->lpszName, lpszWindow))
      DestroyWindow(win->hMainWnd);
}

VOID MACRO_Compare(LPCSTR str)
{
  fprintf(stderr, "Compare(\"%s\")\n", str);
}

VOID MACRO_Contents(VOID)
{
  if (Globals.active_win->page)
    MACRO_JumpContents(Globals.active_win->page->file->lpszPath, NULL);
}

VOID MACRO_ControlPanel(LPCSTR str1, LPCSTR str2, LONG u)
{
  fprintf(stderr, "ControlPanel(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

VOID MACRO_CopyDialog(VOID)
{
  fprintf(stderr, "CopyDialog()\n");
}

VOID MACRO_CopyTopic(VOID)
{
  fprintf(stderr, "CopyTopic()\n");
}

VOID MACRO_CreateButton(LPCSTR id, LPCSTR name, LPCSTR macro)
{
  WINHELP_WINDOW *win = Globals.active_win;
  WINHELP_BUTTON *button, **b;
  LONG            size;
  HGLOBAL         handle;
  LPSTR           ptr;

  size = sizeof(WINHELP_BUTTON) + lstrlen(id) + lstrlen(name) + lstrlen(macro) + 3;
  handle = GlobalAlloc(GMEM_FIXED, size);
  if (!handle) return;

  button = GlobalLock(handle);
  button->hSelf = handle;
  button->next  = 0;
  button->hWnd  = 0;

  ptr = GlobalLock(handle);
  ptr += sizeof(WINHELP_BUTTON);

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

VOID MACRO_DeleteItem(LPCSTR str)
{
  fprintf(stderr, "DeleteItem(\"%s\")\n", str);
}

VOID MACRO_DeleteMark(LPCSTR str)
{
  fprintf(stderr, "DeleteMark(\"%s\")\n", str);
}

VOID MACRO_DestroyButton(LPCSTR str)
{
  fprintf(stderr, "DestroyButton(\"%s\")\n", str);
}

VOID MACRO_DisableButton(LPCSTR str)
{
  fprintf(stderr, "DisableButton(\"%s\")\n", str);
}

VOID MACRO_DisableItem(LPCSTR str)
{
  fprintf(stderr, "DisableItem(\"%s\")\n", str);
}

VOID MACRO_EnableButton(LPCSTR str)
{
  fprintf(stderr, "EnableButton(\"%s\")\n", str);
}

VOID MACRO_EnableItem(LPCSTR str)
{
  fprintf(stderr, "EnableItem(\"%s\")\n", str);
}

VOID MACRO_EndMPrint(VOID)
{
  fprintf(stderr, "EndMPrint()\n");
}

VOID MACRO_ExecFile(LPCSTR str1, LPCSTR str2, LONG u, LPCSTR str3)
{
  fprintf(stderr, "ExecFile(\"%s\", \"%s\", %lu, \"%s\")\n", str1, str2, u, str3);
}

VOID MACRO_ExecProgram(LPCSTR str, LONG u)
{
  fprintf(stderr, "ExecProgram(\"%s\", %lu)\n", str, u);
}

VOID MACRO_Exit(VOID)
{
  while(Globals.win_list)
    DestroyWindow(Globals.win_list->hMainWnd);
}

VOID MACRO_ExtAbleItem(LPCSTR str, LONG u)
{
  fprintf(stderr, "ExtAbleItem(\"%s\", %lu)\n", str, u);
}

VOID MACRO_ExtInsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u1, LONG u2)
{
  fprintf(stderr, "ExtInsertItem(\"%s\", \"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, str4, u1, u2);
}

VOID MACRO_ExtInsertMenu(LPCSTR str1, LPCSTR str2, LPCSTR str3, LONG u1, LONG u2)
{
  fprintf(stderr, "ExtInsertMenu(\"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, u1, u2);
}

BOOL MACRO_FileExist(LPCSTR str)
{
  fprintf(stderr, "FileExist(\"%s\")\n", str);
  return TRUE;
}

VOID MACRO_FileOpen(VOID)
{
  OPENFILENAME openfilename;
  CHAR szPath[MAX_PATHNAME_LEN];
  CHAR szDir[MAX_PATHNAME_LEN];
  CHAR szzFilter[2 * MAX_STRING_LEN + 100];
  LPSTR p = szzFilter;

  LoadString(Globals.hInstance, IDS_HELP_FILES_HLP, p, MAX_STRING_LEN);
  p += strlen(p) + 1;
  lstrcpy(p, "*.hlp");
  p += strlen(p) + 1;
  LoadString(Globals.hInstance, IDS_ALL_FILES, p, MAX_STRING_LEN);
  p += strlen(p) + 1;
  lstrcpy(p, "*.*");
  p += strlen(p) + 1;
  *p = '\0';

  GetWindowsDirectory(szDir, sizeof(szDir));

  openfilename.lStructSize       = 0;
  openfilename.hwndOwner         = Globals.active_win->hMainWnd;
  openfilename.hInstance         = Globals.hInstance;
  openfilename.lpstrFilter       = szzFilter;
  openfilename.lpstrCustomFilter = 0;
  openfilename.nMaxCustFilter    = 0;
  openfilename.nFilterIndex      = 0;
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
    WINHELP_CreateHelpWindow(szPath, 0, "main", FALSE, 0, NULL, SW_SHOWNORMAL);
}

VOID MACRO_Find(VOID)
{
  fprintf(stderr, "Find()\n");
}

VOID MACRO_Finder(VOID)
{
  fprintf(stderr, "Finder()\n");
}

VOID MACRO_FloatingMenu(VOID)
{
  fprintf(stderr, "FloatingMenu()\n");
}

VOID MACRO_Flush(VOID)
{
  fprintf(stderr, "Flush()\n");
}

VOID MACRO_FocusWindow(LPCSTR str)
{
  fprintf(stderr, "FocusWindow(\"%s\")\n", str);
}

VOID MACRO_Generate(LPCSTR str, WPARAM w, LPARAM l)
{
  fprintf(stderr, "Generate(\"%s\", %x, %lx)\n", str, w, l);
}

VOID MACRO_GotoMark(LPCSTR str)
{
  fprintf(stderr, "GotoMark(\"%s\")\n", str);
}

VOID MACRO_HelpOn(VOID)
{
  MACRO_JumpContents((Globals.wVersion > 4) ? "winhelp32.hlp" : "winhelp.hlp", NULL);
}

VOID MACRO_HelpOnTop(VOID)
{
  fprintf(stderr, "HelpOnTop()\n");
}

VOID MACRO_History(VOID)
{
  fprintf(stderr, "History()\n");
}

BOOL MACRO_InitMPrint(VOID)
{
  fprintf(stderr, "InitMPrint()\n");
  return FALSE;
}

VOID MACRO_InsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u)
{
  fprintf(stderr, "InsertItem(\"%s\", \"%s\", \"%s\", \"%s\", %lu)\n", str1, str2, str3, str4, u);
}

VOID MACRO_InsertMenu(LPCSTR str1, LPCSTR str2, LONG u)
{
  fprintf(stderr, "InsertMenu(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

BOOL MACRO_IsBook(VOID)
{
  fprintf(stderr, "IsBook()\n");
  return TRUE;
}

BOOL MACRO_IsMark(LPCSTR str)
{
  fprintf(stderr, "IsMark(\"%s\")\n", str);
  return FALSE;
}

BOOL MACRO_IsNotMark(LPCSTR str)
{
  fprintf(stderr, "IsNotMark(\"%s\")\n", str);
  return TRUE;
}

VOID MACRO_JumpContents(LPCSTR lpszPath, LPCSTR lpszWindow)
{
  WINHELP_CreateHelpWindow(lpszPath, 0, lpszWindow, FALSE, 0, NULL, SW_NORMAL);
}

VOID MACRO_JumpContext(LPCSTR lpszPath, LPCSTR lpszWindow, LONG context)
{
  fprintf(stderr, "JumpContext(\"%s\", \"%s\", %lu)\n", lpszPath, lpszWindow, context);
}

VOID MACRO_JumpHash(LPCSTR lpszPath, LPCSTR lpszWindow, LONG lHash)
{
  WINHELP_CreateHelpWindow(lpszPath, lHash, lpszWindow, FALSE, 0, NULL, SW_NORMAL);
}

VOID MACRO_JumpHelpOn(VOID)
{
  fprintf(stderr, "JumpHelpOn()\n");
}

VOID MACRO_JumpID(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR topic_id)
{
  MACRO_JumpHash(lpszPath, lpszWindow, HLPFILE_Hash(topic_id));
}

VOID MACRO_JumpKeyword(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR keyword)
{
  fprintf(stderr, "JumpKeyword(\"%s\", \"%s\", \"%s\")\n", lpszPath, lpszWindow, keyword);
}

VOID MACRO_KLink(LPCSTR str1, LONG u, LPCSTR str2, LPCSTR str3)
{
  fprintf(stderr, "KLink(\"%s\", %lu, \"%s\", \"%s\")\n", str1, u, str2, str3);
}

VOID MACRO_Menu(VOID)
{
  fprintf(stderr, "Menu()\n");
}

VOID MACRO_MPrintHash(LONG u)
{
  fprintf(stderr, "MPrintHash(%lu)\n", u);
}

VOID MACRO_MPrintID(LPCSTR str)
{
  fprintf(stderr, "MPrintID(\"%s\")\n", str);
}

VOID MACRO_Next(VOID)
{
  fprintf(stderr, "Next()\n");
}

VOID MACRO_NoShow(VOID)
{
  fprintf(stderr, "NoShow()\n");
}

VOID MACRO_PopupContext(LPCSTR str, LONG u)
{
  fprintf(stderr, "PopupContext(\"%s\", %lu)\n", str, u);
}

VOID MACRO_PopupHash(LPCSTR str, LONG u)
{
  fprintf(stderr, "PopupHash(\"%s\", %lu)\n", str, u);
}

VOID MACRO_PopupId(LPCSTR str1, LPCSTR str2)
{
  fprintf(stderr, "PopupId(\"%s\", \"%s\")\n", str1, str2);
}

VOID MACRO_PositionWindow(LONG i1, LONG i2, LONG u1, LONG u2, LONG u3, LPCSTR str)
{
  fprintf(stderr, "PositionWindow(%li, %li, %lu, %lu, %lu, \"%s\")\n", i1, i2, u1, u2, u3, str);
}

VOID MACRO_Prev(VOID)
{
  fprintf(stderr, "Prev()\n");
}

VOID MACRO_Print(VOID)
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
        fprintf(stderr, "Print()\n");
    };
}

VOID MACRO_PrinterSetup(VOID)
{
  fprintf(stderr, "PrinterSetup()\n");
}

VOID MACRO_RegisterRoutine(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
  fprintf(stderr, "RegisterRoutine(\"%s\", \"%s\", \"%s\")\n", str1, str2, str3);
}

VOID MACRO_RemoveAccelerator(LONG u1, LONG u2)
{
  fprintf(stderr, "RemoveAccelerator(%lu, %lu)\n", u1, u2);
}

VOID MACRO_ResetMenu(VOID)
{
  fprintf(stderr, "ResetMenu()\n");
}

VOID MACRO_SaveMark(LPCSTR str)
{
  fprintf(stderr, "SaveMark(\"%s\")\n", str);
}

VOID MACRO_Search(VOID)
{
  fprintf(stderr, "Search()\n");
}

VOID MACRO_SetContents(LPCSTR str, LONG u)
{
  fprintf(stderr, "SetContents(\"%s\", %lu)\n", str, u);
}

VOID MACRO_SetHelpOnFile(LPCSTR str)
{
  fprintf(stderr, "SetHelpOnFile(\"%s\")\n", str);
}

VOID MACRO_SetPopupColor(LONG u1, LONG u2, LONG u3)
{
  fprintf(stderr, "SetPopupColor(%lu, %lu, %lu)\n", u1, u2, u3);
}

VOID MACRO_ShellExecute(LPCSTR str1, LPCSTR str2, LONG u1, LONG u2, LPCSTR str3, LPCSTR str4)
{
  fprintf(stderr, "ShellExecute(\"%s\", \"%s\", %lu, %lu, \"%s\", \"%s\")\n", str1, str2, u1, u2, str3, str4);
}

VOID MACRO_ShortCut(LPCSTR str1, LPCSTR str2, WPARAM w, LPARAM l, LPCSTR str)
{
  fprintf(stderr, "ShortCut(\"%s\", \"%s\", %x, %lx, \"%s\")\n", str1, str2, w, l, str);
}

VOID MACRO_TCard(LONG u)
{
  fprintf(stderr, "TCard(%lu)\n", u);
}

VOID MACRO_Test(LONG u)
{
  fprintf(stderr, "Test(%lu)\n", u);
}

BOOL MACRO_TestALink(LPCSTR str)
{
  fprintf(stderr, "TestALink(\"%s\")\n", str);
  return FALSE;
}

BOOL MACRO_TestKLink(LPCSTR str)
{
  fprintf(stderr, "TestKLink(\"%s\")\n", str);
  return FALSE;
}

VOID MACRO_UncheckItem(LPCSTR str)
{
  fprintf(stderr, "UncheckItem(\"%s\")\n", str);
}

VOID MACRO_UpdateWindow(LPCSTR str1, LPCSTR str2)
{
  fprintf(stderr, "UpdateWindow(\"%s\", \"%s\")\n", str1, str2);
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
