/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Eric Pouech
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

#include <stdio.h>

#include "windows.h"
#include "commdlg.h"
#include "winhelp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

/**************************************************/
/*               Macro table                      */
/**************************************************/
struct MacroDesc {
    char*       name;
    char*       alias;
    BOOL        isBool;
    char*       arguments;
    void        (*fn)();
};

/* types:
 *      U:      32 bit unsigned int
 *      I:      32 bit signed int
 *      S:      string
 *      v:      unknown (32 bit entity)
 */

static struct MacroDesc MACRO_Builtins[] = {
    {"About",               NULL, 0, "",       MACRO_About},
    {"AddAccelerator",      "AA", 0, "UUS",    MACRO_AddAccelerator},
    {"ALink",               "AL", 0, "SUS",    MACRO_ALink},
    {"Annotate",            NULL, 0, "",       MACRO_Annotate},
    {"AppendItem",          NULL, 0, "SSSS",   MACRO_AppendItem},
    {"Back",                NULL, 0, "",       MACRO_Back},
    {"BackFlush",           "BF", 0, "",       MACRO_BackFlush},
    {"BookmarkDefine",      NULL, 0, "",       MACRO_BookmarkDefine},
    {"BookmarkMore",        NULL, 0, "",       MACRO_BookmarkMore},
    {"BrowseButtons",       NULL, 0, "",       MACRO_BrowseButtons},
    {"ChangeButtonBinding", "CBB",0, "SS",     MACRO_ChangeButtonBinding},
    {"ChangeEnable",        "CE", 0, "SS",     MACRO_ChangeEnable},
    {"ChangeItemBinding",   "CIB",0, "SS",     MACRO_ChangeItemBinding},
    {"CheckItem",           "CI", 0, "S",      MACRO_CheckItem},
    {"CloseSecondarys",     "CS", 0, "",       MACRO_CloseSecondarys},
    {"CloseWindow",         "CW", 0, "S",      MACRO_CloseWindow},
    {"Compare",             NULL, 0, "S",      MACRO_Compare},
    {"Contents",            NULL, 0, "",       MACRO_Contents},
    {"ControlPanel",        NULL, 0, "SSU",    MACRO_ControlPanel},
    {"CopyDialog",          NULL, 0, "",       MACRO_CopyDialog},
    {"CopyTopic",           "CT", 0, "",       MACRO_CopyTopic},
    {"CreateButton",        "CB", 0, "SSS",    MACRO_CreateButton},
    {"DeleteItem",          NULL, 0, "S",      MACRO_DeleteItem},
    {"DeleteMark",          NULL, 0, "S",      MACRO_DeleteMark},
    {"DestroyButton",       NULL, 0, "S",      MACRO_DestroyButton},
    {"DisableButton",       "DB", 0, "S",      MACRO_DisableButton},
    {"DisableItem",         "DI", 0, "S",      MACRO_DisableItem},
    {"EnableButton",        "EB", 0, "S",      MACRO_EnableButton},
    {"EnableItem",          "EI", 0, "S",      MACRO_EnableItem},
    {"EndMPrint",           NULL, 0, "",       MACRO_EndMPrint},
    {"ExecFile",            "EF", 0, "SSUS",   MACRO_ExecFile},
    {"ExecProgram",         "EP", 0, "SU",     MACRO_ExecProgram},
    {"Exit",                NULL, 0, "",       MACRO_Exit},
    {"ExtAbleItem",         NULL, 0, "SU",     MACRO_ExtAbleItem},
    {"ExtInsertItem",       NULL, 0, "SSSSUU", MACRO_ExtInsertItem},
    {"ExtInsertMenu",       NULL, 0, "SSSUU",  MACRO_ExtInsertMenu},
    {"FileExist",           "FE", 1, "S",      (void (*)())MACRO_FileExist},
    {"FileOpen",            "FO", 0, "",       MACRO_FileOpen},
    {"Find",                NULL, 0, "",       MACRO_Find},
    {"Finder",              "FD", 0, "",       MACRO_Finder},
    {"FloatingMenu",        NULL, 0, "",       MACRO_FloatingMenu},
    {"Flush",               "FH", 0, "",       MACRO_Flush},
    {"FocusWindow",         NULL, 0, "S",      MACRO_FocusWindow},
    {"Generate",            NULL, 0, "SUU",    MACRO_Generate},
    {"GotoMark",            NULL, 0, "S",      MACRO_GotoMark},
    {"HelpOn",              NULL, 0, "",       MACRO_HelpOn},
    {"HelpOnTop",           NULL, 0, "",       MACRO_HelpOnTop},
    {"History",             NULL, 0, "",       MACRO_History},
    {"InitMPrint",          NULL, 1, "",       (void (*)())MACRO_InitMPrint},
    {"InsertItem",          NULL, 0, "SSSSU",  MACRO_InsertItem},
    {"InsertMenu",          NULL, 0, "SSU",    MACRO_InsertMenu},
    {"IfThen",              "IF", 0, "BS",     MACRO_IfThen},
    {"IfThenElse",          "IE", 0, "BSS",    MACRO_IfThenElse},
    {"IsBook",              NULL, 1, "",       (void (*)())MACRO_IsBook},
    {"IsMark",              NULL, 1, "S",      (void (*)())MACRO_IsMark},
    {"IsNotMark",           "NM", 1, "S",      (void (*)())MACRO_IsNotMark},
    {"JumpContents",        NULL, 0, "SS",     MACRO_JumpContents},
    {"JumpContext",         "JC", 0, "SSU",    MACRO_JumpContext},
    {"JumpHash",            "JH", 0, "SSU",    MACRO_JumpHash},
    {"JumpHelpOn",          NULL, 0, "",       MACRO_JumpHelpOn},
    {"JumpID",              "JI", 0, "SSS",    MACRO_JumpID},
    {"JumpKeyword",         "JK", 0, "SSS",    MACRO_JumpKeyword},
    {"KLink",               "KL", 0, "SUSS",   MACRO_KLink},
    {"Menu",                "MU", 0, "",       MACRO_Menu},
    {"MPrintHash",          NULL, 0, "U",      MACRO_MPrintHash},
    {"MPrintID",            NULL, 0, "S",      MACRO_MPrintID},
    {"Next",                NULL, 0, "",       MACRO_Next},
    {"NoShow",              NULL, 0, "",       MACRO_NoShow},
    {"PopupContext",        "PC", 0, "SU",     MACRO_PopupContext},
    {"PopupHash",           NULL, 0, "SU",     MACRO_PopupHash},
    {"PopupId",             "PI", 0, "SS",     MACRO_PopupId},
    {"PositionWindow",      "PW", 0, "IIUUUS", MACRO_PositionWindow},
    {"Prev",                NULL, 0, "",       MACRO_Prev},
    {"Print",               NULL, 0, "",       MACRO_Print},
    {"PrinterSetup",        NULL, 0, "",       MACRO_PrinterSetup},
    {"RegisterRoutine",     "RR", 0, "SSS",    MACRO_RegisterRoutine},
    {"RemoveAccelerator",   "RA", 0, "UU",     MACRO_RemoveAccelerator},
    {"ResetMenu",           NULL, 0, "",       MACRO_ResetMenu},
    {"SaveMark",            NULL, 0, "S",      MACRO_SaveMark},
    {"Search",              NULL, 0, "",       MACRO_Search},
    {"SetContents",         NULL, 0, "SU",     MACRO_SetContents},
    {"SetHelpOnFile",       NULL, 0, "S",      MACRO_SetHelpOnFile},
    {"SetPopupColor",       "SPC",0, "UUU",    MACRO_SetPopupColor},
    {"ShellExecute",        "SE", 0, "SSUUSS", MACRO_ShellExecute},
    {"ShortCut",            "SH", 0, "SSUUS",  MACRO_ShortCut},
    {"TCard",               NULL, 0, "U",      MACRO_TCard},
    {"Test",                NULL, 0, "U",      MACRO_Test},
    {"TestALink",           NULL, 1, "S",      (void (*)())MACRO_TestALink},
    {"TestKLink",           NULL, 1, "S",      (void (*)())MACRO_TestKLink},
    {"UncheckItem",         "UI", 0, "S",      MACRO_UncheckItem},
    {"UpdateWindow",        "UW", 0, "SS",     MACRO_UpdateWindow},
    {NULL,                  NULL, 0, NULL,     NULL}
};

static struct MacroDesc*MACRO_Loaded /* = NULL */;
static unsigned         MACRO_NumLoaded /* = 0 */;

static int MACRO_DoLookUp(struct MacroDesc* start, const char* name, struct lexret* lr, unsigned len)
{
    struct MacroDesc*   md;

    for (md = start; md->name && len != 0; md++, len--)
    {
        if (strcasecmp(md->name, name) == 0 || (md->alias != NULL && strcasecmp(md->alias, name) == 0))
        {
            lr->proto = md->arguments;
            if (md->isBool)
            {       
                lr->bool_function = (BOOL (*)())md->fn;
                return BOOL_FUNCTION;
            }
            else
            {
                lr->void_function = md->fn;
                return VOID_FUNCTION;
            }
        }
    }
    return EMPTY;
}

int MACRO_Lookup(const char* name, struct lexret* lr)
{
    int ret;
    
    if ((ret = MACRO_DoLookUp(MACRO_Builtins, name, lr, -1)) != EMPTY)
        return ret;
    if (MACRO_Loaded && (ret = MACRO_DoLookUp(MACRO_Loaded, name, lr, MACRO_NumLoaded)) != EMPTY)
        return ret;

    lr->string = name;
    return IDENTIFIER;
}

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
    WINE_FIXME("()\n");
}

void MACRO_AddAccelerator(LONG u1, LONG u2, LPCSTR str)
{
    WINE_FIXME("(%lu, %lu, \"%s\")\n", u1, u2, str);
}

void MACRO_ALink(LPCSTR str1, LONG u, LPCSTR str2)
{
    WINE_FIXME("(\"%s\", %lu, \"%s\")\n", str1, u, str2);
}

void MACRO_Annotate(void)
{
    WINE_FIXME("()\n");
}

void MACRO_AppendItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("(\"%s\", \"%s\", \"%s\", \"%s\")\n", str1, str2, str3, str4);
}

void MACRO_Back(void)
{
    WINHELP_WINDOW* win = Globals.active_win;

    WINE_TRACE("()\n");

    if (win && win->backIndex >= 2)
        WINHELP_CreateHelpWindow(win->back[--win->backIndex - 1],
                                 win->info, SW_SHOW);
}

void MACRO_BackFlush(void)
{
    WINHELP_WINDOW* win = Globals.active_win;

    WINE_TRACE("()\n");

    if (win)
    {
        int     i;

        for (i = 0; i < win->backIndex; i++)
        {
            HLPFILE_FreeHlpFile(win->back[i]->file);
            win->back[i] = NULL;
        }
        win->backIndex = 0;
    }
}

void MACRO_BookmarkDefine(void)
{
    WINE_FIXME("()\n");
}

void MACRO_BookmarkMore(void)
{
    WINE_FIXME("()\n");
}

void MACRO_BrowseButtons(void)
{
    WINE_TRACE("()\n");

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

    WINE_TRACE("(\"%s\", \"%s\")\n", id, macro);

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
    WINE_TRACE("(\"%s\", \"%s\")\n", id, macro);

    MACRO_ChangeButtonBinding(id, macro);
    MACRO_EnableButton(id);
}

void MACRO_ChangeItemBinding(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(\"%s\", \"%s\")\n", str1, str2);
}

void MACRO_CheckItem(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_CloseSecondarys(void)
{
    WINHELP_WINDOW *win;

    WINE_TRACE("()\n");
    for (win = Globals.win_list; win; win = win->next)
        if (win->lpszName && lstrcmpi(win->lpszName, "main"))
            DestroyWindow(win->hMainWnd);
}

void MACRO_CloseWindow(LPCSTR lpszWindow)
{
    WINHELP_WINDOW *win;

    WINE_TRACE("(\"%s\")\n", lpszWindow);

    if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

    for (win = Globals.win_list; win; win = win->next)
        if (win->lpszName && !lstrcmpi(win->lpszName, lpszWindow))
            DestroyWindow(win->hMainWnd);
}

void MACRO_Compare(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_Contents(void)
{
    WINE_TRACE("()\n");

    if (Globals.active_win->page)
        MACRO_JumpContents(Globals.active_win->page->file->lpszPath, NULL);
}

void MACRO_ControlPanel(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

void MACRO_CopyDialog(void)
{
    WINE_FIXME("()\n");
}

void MACRO_CopyTopic(void)
{
    WINE_FIXME("()\n");
}

void MACRO_CreateButton(LPCSTR id, LPCSTR name, LPCSTR macro)
{
    WINHELP_WINDOW *win = Globals.active_win;
    WINHELP_BUTTON *button, **b;
    LONG            size;
    LPSTR           ptr;

    WINE_TRACE("(\"%s\", \"%s\", %s)\n", id, name, macro);

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
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_DeleteMark(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_DestroyButton(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_DisableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    WINE_FIXME("(\"%s\")\n", id);

    b = MACRO_LookupButton(Globals.active_win, id);
    if (!*b) {WINE_FIXME("Couldn't find button '%s'\n", id); return;}

    EnableWindow((*b)->hWnd, FALSE);
}

void MACRO_DisableItem(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_EnableButton(LPCSTR id)
{
    WINHELP_BUTTON**    b;

    WINE_TRACE("(\"%s\")\n", id);

    b = MACRO_LookupButton(Globals.active_win, id);
    if (!*b) {WINE_FIXME("Couldn't find button '%s'\n", id); return;}

    EnableWindow((*b)->hWnd, TRUE);
}

void MACRO_EnableItem(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_EndMPrint(void)
{
    WINE_FIXME("()\n");
}

void MACRO_ExecFile(LPCSTR str1, LPCSTR str2, LONG u, LPCSTR str3)
{
    WINE_FIXME("(\"%s\", \"%s\", %lu, \"%s\")\n", str1, str2, u, str3);
}

void MACRO_ExecProgram(LPCSTR str, LONG u)
{
    WINE_FIXME("(\"%s\", %lu)\n", str, u);
}

void MACRO_Exit(void)
{
    WINE_TRACE("()\n");

    while (Globals.win_list)
        DestroyWindow(Globals.win_list->hMainWnd);
}

void MACRO_ExtAbleItem(LPCSTR str, LONG u)
{
    WINE_FIXME("(\"%s\", %lu)\n", str, u);
}

void MACRO_ExtInsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u1, LONG u2)
{
    WINE_FIXME("(\"%s\", \"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, str4, u1, u2);
}

void MACRO_ExtInsertMenu(LPCSTR str1, LPCSTR str2, LPCSTR str3, LONG u1, LONG u2)
{
    WINE_FIXME("(\"%s\", \"%s\", \"%s\", %lu, %lu)\n", str1, str2, str3, u1, u2);
}

BOOL MACRO_FileExist(LPCSTR str)
{
    WINE_TRACE("(\"%s\")\n", str);
    return GetFileAttributes(str) != INVALID_FILE_ATTRIBUTES;
}

void MACRO_FileOpen(void)
{
    OPENFILENAME openfilename;
    CHAR szPath[MAX_PATHNAME_LEN];
    CHAR szDir[MAX_PATHNAME_LEN];
    CHAR szzFilter[2 * MAX_STRING_LEN + 100];
    LPSTR p = szzFilter;

    WINE_TRACE("()\n");

    LoadString(Globals.hInstance, STID_HELP_FILES_HLP, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.hlp");
    p += strlen(p) + 1;
    LoadString(Globals.hInstance, STID_ALL_FILES, p, MAX_STRING_LEN);
    p += strlen(p) + 1;
    lstrcpy(p, "*.*");
    p += strlen(p) + 1;
    *p = '\0';

    GetCurrentDirectory(sizeof(szDir), szDir);

    szPath[0]='\0';

    openfilename.lStructSize       = sizeof(OPENFILENAME);
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
    {
        HLPFILE*        hlpfile = WINHELP_LookupHelpFile(szPath);

        WINHELP_CreateHelpWindowByHash(hlpfile, 0, 
                                       WINHELP_GetWindowInfo(hlpfile, "main"), SW_SHOWNORMAL);
    }
}

void MACRO_Find(void)
{
    WINE_FIXME("()\n");
}

void MACRO_Finder(void)
{
    WINE_FIXME("()\n");
}

void MACRO_FloatingMenu(void)
{
    WINE_FIXME("()\n");
}

void MACRO_Flush(void)
{
    WINE_FIXME("()\n");
}

void MACRO_FocusWindow(LPCSTR lpszWindow)
{
    WINHELP_WINDOW *win;

    WINE_TRACE("(\"%s\")\n", lpszWindow);

    if (!lpszWindow || !lpszWindow[0]) lpszWindow = "main";

    for (win = Globals.win_list; win; win = win->next)
        if (win->lpszName && !lstrcmpi(win->lpszName, lpszWindow))
            SetFocus(win->hMainWnd);
}

void MACRO_Generate(LPCSTR str, LONG w, LONG l)
{
    WINE_FIXME("(\"%s\", %lx, %lx)\n", str, w, l);
}

void MACRO_GotoMark(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_HelpOn(void)
{
    WINE_TRACE("()\n");
    MACRO_JumpContents((Globals.wVersion > 4) ? "winhelp32.hlp" : "winhelp.hlp", NULL);
}

void MACRO_HelpOnTop(void)
{
    WINE_FIXME("()\n");
}

void MACRO_History(void)
{
    WINE_TRACE("()\n");

    if (Globals.active_win && !Globals.active_win->hHistoryWnd)
    {
        HWND hWnd = CreateWindow(HISTORY_WIN_CLASS_NAME, "History", WS_OVERLAPPEDWINDOW,
                                 0, 0, 0, 0, 0, 0, Globals.hInstance, Globals.active_win);
        ShowWindow(hWnd, SW_NORMAL);
    }
}

void MACRO_IfThen(BOOL b, LPCSTR t)
{
    if (b) MACRO_ExecuteMacro(t);
}

void MACRO_IfThenElse(BOOL b, LPCSTR t, LPCSTR f)
{
    if (b) MACRO_ExecuteMacro(t); else MACRO_ExecuteMacro(f);
}

BOOL MACRO_InitMPrint(void)
{
    WINE_FIXME("()\n");
    return FALSE;
}

void MACRO_InsertItem(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR str4, LONG u)
{
    WINE_FIXME("(\"%s\", \"%s\", \"%s\", \"%s\", %lu)\n", str1, str2, str3, str4, u);
}

void MACRO_InsertMenu(LPCSTR str1, LPCSTR str2, LONG u)
{
    WINE_FIXME("(\"%s\", \"%s\", %lu)\n", str1, str2, u);
}

BOOL MACRO_IsBook(void)
{
    WINE_TRACE("()\n");
    return Globals.isBook;
}

BOOL MACRO_IsMark(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
    return FALSE;
}

BOOL MACRO_IsNotMark(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
    return TRUE;
}

void MACRO_JumpContents(LPCSTR lpszPath, LPCSTR lpszWindow)
{
    HLPFILE*    hlpfile;

    WINE_TRACE("(\"%s\", \"%s\")\n", lpszPath, lpszWindow);
    hlpfile = WINHELP_LookupHelpFile(lpszPath);
    WINHELP_CreateHelpWindowByHash(hlpfile, 0, 
                                   WINHELP_GetWindowInfo(hlpfile, lpszWindow),
                                   SW_NORMAL);
}

void MACRO_JumpContext(LPCSTR lpszPath, LPCSTR lpszWindow, LONG context)
{
    WINE_FIXME("(\"%s\", \"%s\", %lu)\n", lpszPath, lpszWindow, context);
}

void MACRO_JumpHash(LPCSTR lpszPath, LPCSTR lpszWindow, LONG lHash)
{
    HLPFILE*    hlpfile;

    WINE_TRACE("(\"%s\", \"%s\", %lu)\n", lpszPath, lpszWindow, lHash);
    hlpfile = WINHELP_LookupHelpFile(lpszPath);
    WINHELP_CreateHelpWindowByHash(hlpfile, lHash, 
                                   WINHELP_GetWindowInfo(hlpfile, lpszWindow),
                                   SW_NORMAL);
}

void MACRO_JumpHelpOn(void)
{
    WINE_FIXME("()\n");
}

/* FIXME: those two macros are wrong
 * they should only contain 2 strings, path & window are coded as path>window
 */
void MACRO_JumpID(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR topic_id)
{
    WINE_TRACE("(\"%s\", \"%s\", \"%s\")\n", lpszPath, lpszWindow, topic_id);
    MACRO_JumpHash(lpszPath, lpszWindow, HLPFILE_Hash(topic_id));
}

void MACRO_JumpKeyword(LPCSTR lpszPath, LPCSTR lpszWindow, LPCSTR keyword)
{
    WINE_FIXME("(\"%s\", \"%s\", \"%s\")\n", lpszPath, lpszWindow, keyword);
}

void MACRO_KLink(LPCSTR str1, LONG u, LPCSTR str2, LPCSTR str3)
{
    WINE_FIXME("(\"%s\", %lu, \"%s\", \"%s\")\n", str1, u, str2, str3);
}

void MACRO_Menu(void)
{
    WINE_FIXME("()\n");
}

void MACRO_MPrintHash(LONG u)
{
    WINE_FIXME("(%lu)\n", u);
}

void MACRO_MPrintID(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_Next(void)
{
    HLPFILE_PAGE*   page;

    WINE_TRACE("()\n");
    page = Globals.active_win->page;
    page = HLPFILE_PageByOffset(page->file, page->browse_fwd);
    if (page)
    {
        page->file->wRefCount++;
        WINHELP_CreateHelpWindow(page, Globals.active_win->info, SW_NORMAL);
    }
}

void MACRO_NoShow(void)
{
    WINE_FIXME("()\n");
}

void MACRO_PopupContext(LPCSTR str, LONG u)
{
    WINE_FIXME("(\"%s\", %lu)\n", str, u);
}

void MACRO_PopupHash(LPCSTR str, LONG u)
{
    WINE_FIXME("(\"%s\", %lu)\n", str, u);
}

void MACRO_PopupId(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(\"%s\", \"%s\")\n", str1, str2);
}

void MACRO_PositionWindow(LONG i1, LONG i2, LONG u1, LONG u2, LONG u3, LPCSTR str)
{
    WINE_FIXME("(%li, %li, %lu, %lu, %lu, \"%s\")\n", i1, i2, u1, u2, u3, str);
}

void MACRO_Prev(void)
{
    HLPFILE_PAGE*   page;

    WINE_TRACE("()\n");
    page = Globals.active_win->page;
    page = HLPFILE_PageByOffset(page->file, page->browse_bwd);
    if (page)
    {
        page->file->wRefCount++;
        WINHELP_CreateHelpWindow(page, Globals.active_win->info, SW_NORMAL);
    }
}

void MACRO_Print(void)
{
    PRINTDLG printer;

    WINE_TRACE("()\n");

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
    WINE_FIXME("()\n");
}

void MACRO_RegisterRoutine(LPCSTR dll, LPCSTR proc, LPCSTR args)
{
    HANDLE      hLib;
    void        (*fn)();
    int         size;

    WINE_TRACE("(\"%s\", \"%s\", \"%s\")\n", dll, proc, args);

    if ((hLib = LoadLibrary(dll)) == NULL)
    {
        /* FIXME: internationalisation for error messages */
        WINE_FIXME("Cannot find dll %s\n", dll);
        fn = NULL;
    }
    else if (!(fn = (void (*)())GetProcAddress(hLib, proc)))
    {
        /* FIXME: internationalisation for error messages */
        WINE_FIXME("Cannot find proc %s in dll %s\n", dll, proc);
        fn = NULL;
    }

    /* FIXME: the library will not be unloaded until exit of program */
    size = ++MACRO_NumLoaded * sizeof(struct MacroDesc);
    if (!MACRO_Loaded) MACRO_Loaded = HeapAlloc(GetProcessHeap(), 0, size);
    else MACRO_Loaded = HeapReAlloc(GetProcessHeap(), 0, MACRO_Loaded, size);
    MACRO_Loaded[MACRO_NumLoaded - 1].name      = strdup(proc); /* FIXME */
    MACRO_Loaded[MACRO_NumLoaded - 1].alias     = NULL;
    MACRO_Loaded[MACRO_NumLoaded - 1].isBool    = 0;
    MACRO_Loaded[MACRO_NumLoaded - 1].arguments = strdup(args); /* FIXME */
    MACRO_Loaded[MACRO_NumLoaded - 1].fn        = fn;
}

void MACRO_RemoveAccelerator(LONG u1, LONG u2)
{
    WINE_FIXME("(%lu, %lu)\n", u1, u2);
}

void MACRO_ResetMenu(void)
{
    WINE_FIXME("()\n");
}

void MACRO_SaveMark(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_Search(void)
{
    WINE_FIXME("()\n");
}

void MACRO_SetContents(LPCSTR str, LONG u)
{
    WINE_FIXME("(\"%s\", %lu)\n", str, u);
}

void MACRO_SetHelpOnFile(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_SetPopupColor(LONG u1, LONG u2, LONG u3)
{
    WINE_FIXME("(%lu, %lu, %lu)\n", u1, u2, u3);
}

void MACRO_ShellExecute(LPCSTR str1, LPCSTR str2, LONG u1, LONG u2, LPCSTR str3, LPCSTR str4)
{
    WINE_FIXME("(\"%s\", \"%s\", %lu, %lu, \"%s\", \"%s\")\n", str1, str2, u1, u2, str3, str4);
}

void MACRO_ShortCut(LPCSTR str1, LPCSTR str2, LONG w, LONG l, LPCSTR str)
{
    WINE_FIXME("(\"%s\", \"%s\", %lx, %lx, \"%s\")\n", str1, str2, w, l, str);
}

void MACRO_TCard(LONG u)
{
    WINE_FIXME("(%lu)\n", u);
}

void MACRO_Test(LONG u)
{
    WINE_FIXME("(%lu)\n", u);
}

BOOL MACRO_TestALink(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
    return FALSE;
}

BOOL MACRO_TestKLink(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
    return FALSE;
}

void MACRO_UncheckItem(LPCSTR str)
{
    WINE_FIXME("(\"%s\")\n", str);
}

void MACRO_UpdateWindow(LPCSTR str1, LPCSTR str2)
{
    WINE_FIXME("(\"%s\", \"%s\")\n", str1, str2);
}
