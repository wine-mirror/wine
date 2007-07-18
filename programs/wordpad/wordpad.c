/*
 * Wordpad implementation
 *
 * Copyright 2004 by Krzysztof Foltman
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

#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0400

#define MAX_STRING_LEN 255

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <commdlg.h>

#include "resource.h"

/* use LoadString */
static const WCHAR xszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};
static const WCHAR xszMainMenu[] = {'M','A','I','N','M','E','N','U',0};

static const WCHAR wszRichEditClass[] = {'R','I','C','H','E','D','I','T','2','0','W',0};
static const WCHAR wszMainWndClass[] = {'W','O','R','D','P','A','D','T','O','P',0};
static const WCHAR wszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};

static HWND hMainWnd;
static HWND hEditorWnd;
static HWND hFindWnd;

static UINT ID_FINDMSGSTRING;

static WCHAR wszFilter[MAX_STRING_LEN];
static WCHAR wszDefaultFileName[MAX_STRING_LEN];
static WCHAR wszSaveChanges[MAX_STRING_LEN];

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam );

/* Load string resources */
static void DoLoadStrings(void)
{
    LPWSTR p = wszFilter;
    static const WCHAR files_rtf[] = {'*','.','r','t','f','\0'};
    static const WCHAR files_txt[] = {'*','.','t','x','t','\0'};
    static const WCHAR files_all[] = {'*','.','*','\0'};
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);

    LoadStringW(hInstance, STRING_RICHTEXT_FILES_RTF, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_rtf);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_txt);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_all);
    p += lstrlenW(p) + 1;
    *p = '\0';

    p = wszDefaultFileName;
    LoadStringW(hInstance, STRING_DEFAULT_FILENAME, p, MAX_STRING_LEN);

    p = wszSaveChanges;
    LoadStringW(hInstance, STRING_PROMPT_SAVE_CHANGES, p, MAX_STRING_LEN);
}

static void AddButton(HWND hwndToolBar, int nImage, int nCommand)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = nImage;
    button.idCommand = nCommand;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = TBSTYLE_BUTTON;
    button.dwData = 0;
    button.iString = -1;
    SendMessageW(hwndToolBar, TB_ADDBUTTONSW, 1, (LPARAM)&button);
}

static void AddSeparator(HWND hwndToolBar)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = -1;
    button.idCommand = 0;
    button.fsState = 0;
    button.fsStyle = TBSTYLE_SEP;
    button.dwData = 0;
    button.iString = -1;
    SendMessageW(hwndToolBar, TB_ADDBUTTONSW, 1, (LPARAM)&button);
}

static DWORD CALLBACK stream_in(DWORD_PTR cookie, LPBYTE buffer, LONG cb, LONG *pcb)
{
    HANDLE hFile = (HANDLE)cookie;
    DWORD read;

    if(!ReadFile(hFile, buffer, cb, &read, 0))
        return 1;

    *pcb = read;

    return 0;
}

static DWORD CALLBACK stream_out(DWORD_PTR cookie, LPBYTE buffer, LONG cb, LONG *pcb)
{
    DWORD written;
    int ret;
    HANDLE hFile = (HANDLE)cookie;

    ret = WriteFile(hFile, buffer, cb, &written, 0);

    if(!ret || (cb != written))
        return 1;

    *pcb = cb;

    return 0;
}

static WCHAR wszFileName[MAX_PATH];

static void set_caption(LPCWSTR wszNewFileName)
{
    static const WCHAR wszSeparator[] = {' ','-',' '};
    WCHAR *wszCaption;
    SIZE_T length = 0;

    if(!wszNewFileName)
        wszNewFileName = wszDefaultFileName;

    wszCaption = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                lstrlenW(wszNewFileName)*sizeof(WCHAR)+sizeof(wszSeparator)+sizeof(wszAppTitle));

    if(!wszCaption)
        return;

    memcpy(wszCaption, wszNewFileName, lstrlenW(wszNewFileName)*sizeof(WCHAR));
    length += lstrlenW(wszNewFileName);
    memcpy(wszCaption + length, wszSeparator, sizeof(wszSeparator));
    length += sizeof(wszSeparator) / sizeof(WCHAR);
    memcpy(wszCaption + length, wszAppTitle, sizeof(wszAppTitle));

    SetWindowTextW(hMainWnd, wszCaption);

    HeapFree(GetProcessHeap(), 0, wszCaption);
}

static void DoOpenFile(LPCWSTR szOpenFileName)
{
    HANDLE hFile;
    EDITSTREAM es;

    hFile = CreateFileW(szOpenFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = stream_in;

    /* FIXME: Handle different file formats */
    SendMessageW(hEditorWnd, EM_STREAMIN, SF_RTF, (LPARAM)&es);

    CloseHandle(hFile);

    SetFocus(hEditorWnd);

    set_caption(szOpenFileName);

    lstrcpyW(wszFileName, szOpenFileName);
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
}

static void DoSaveFile(LPCWSTR wszSaveFileName)
{
    HANDLE hFile;
    EDITSTREAM stream;
    LRESULT ret;

    hFile = CreateFileW(wszSaveFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
        return;

    stream.dwCookie = (DWORD_PTR)hFile;
    stream.pfnCallback = stream_out;

    /* FIXME: Handle different formats */
    ret = SendMessageW(hEditorWnd, EM_STREAMOUT, SF_RTF, (LPARAM)&stream);

    CloseHandle(hFile);

    SetFocus(hEditorWnd);

    if(!ret)
        return;

    lstrcpyW(wszFileName, wszSaveFileName);
    set_caption(wszFileName);
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
}

static void DialogSaveFile(void)
{
    OPENFILENAMEW sfn;

    WCHAR wszFile[MAX_PATH] = {'\0'};
    static const WCHAR wszDefExt[] = {'r','t','f','\0'};

    ZeroMemory(&sfn, sizeof(sfn));

    sfn.lStructSize = sizeof(sfn);
    sfn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    sfn.hwndOwner = hMainWnd;
    sfn.lpstrFilter = wszFilter;
    sfn.lpstrFile = wszFile;
    sfn.nMaxFile = MAX_PATH;
    sfn.lpstrDefExt = wszDefExt;

    if(!GetSaveFileNameW(&sfn))
        return;

    DoSaveFile(sfn.lpstrFile);
}

static BOOL prompt_save_changes(void)
{
    if(!wszFileName[0])
    {
        GETTEXTLENGTHEX gt;
        gt.flags = GTL_NUMCHARS;
        gt.codepage = 1200;
        if(!SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0))
            return TRUE;
    }

    if(!SendMessageW(hEditorWnd, EM_GETMODIFY, 0, 0))
    {
        return TRUE;
    } else
    {
        LPWSTR displayFileName;
        WCHAR *text;
        int ret;

        if(!wszFileName[0])
            displayFileName = wszDefaultFileName;
        else
            displayFileName = wszFileName;

        text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                         (lstrlenW(displayFileName)+lstrlenW(wszSaveChanges))*sizeof(WCHAR));

        if(!text)
            return FALSE;

        wsprintfW(text, wszSaveChanges, displayFileName);

        ret = MessageBoxW(hMainWnd, text, wszAppTitle, MB_YESNOCANCEL | MB_ICONEXCLAMATION);

        HeapFree(GetProcessHeap(), 0, text);

        switch(ret)
        {
            case IDNO:
                return TRUE;

            case IDYES:
                if(wszFileName[0])
                    DoSaveFile(wszFileName);
                else
                    DialogSaveFile();
                return TRUE;

            default:
                return FALSE;
        }
    }
}

static void DialogOpenFile(void)
{
    OPENFILENAMEW ofn;

    WCHAR wszFile[MAX_PATH] = {'\0'};
    static const WCHAR wszDefExt[] = {'r','t','f','\0'};

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = wszFilter;
    ofn.lpstrFile = wszFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = wszDefExt;

    if(GetOpenFileNameW(&ofn))
    {
        prompt_save_changes();
        DoOpenFile(ofn.lpstrFile);
    }
}

static void HandleCommandLine(LPWSTR cmdline)
{
    WCHAR delimiter;
    int opt_print = 0;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;
    while (*cmdline && *cmdline != delimiter) cmdline++;
    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ' || *cmdline == '-' || *cmdline == '/')
    {
        WCHAR option;

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == ' ') cmdline++;

        switch (option)
        {
            case 'p':
            case 'P':
                opt_print = 1;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed on the command line */
        if (cmdline[0] == '"')
        {
            cmdline++;
            cmdline[lstrlenW(cmdline) - 1] = 0;
        }
        DoOpenFile(cmdline);
        InvalidateRect(hMainWnd, NULL, FALSE);
    }

    if (opt_print)
        MessageBox(hMainWnd, "Printing not implemented", "WordPad", MB_OK);
}

static LRESULT handle_findmsg(LPFINDREPLACEW pFr)
{
    if(pFr->Flags & FR_DIALOGTERM)
    {
        hFindWnd = 0;
        pFr->Flags = FR_FINDNEXT;
        return 0;
    } else if(pFr->Flags & FR_FINDNEXT)
    {
        DWORD flags = FR_DOWN;
        FINDTEXTW ft;
        static CHARRANGE cr;
        LRESULT end, ret;
        GETTEXTLENGTHEX gt;
        LRESULT length;
        int startPos;
        HMENU hMenu = GetMenu(hMainWnd);
        MENUITEMINFOW mi;

        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_DATA;
        mi.dwItemData = 1;
        SetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

        gt.flags = GTL_NUMCHARS;
        gt.codepage = 1200;

        length = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        if(pFr->lCustData == -1)
        {
            SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&end);
            cr.cpMin = startPos;
            pFr->lCustData = startPos;
            cr.cpMax = length;
            if(cr.cpMin == length)
                cr.cpMin = 0;
        } else
        {
            startPos = pFr->lCustData;
        }

        if(cr.cpMax > length)
        {
            startPos = 0;
            cr.cpMin = 0;
            cr.cpMax = length;
        }

        ft.chrg = cr;
        ft.lpstrText = pFr->lpstrFindWhat;

        if(pFr->Flags & FR_MATCHCASE)
            flags |= FR_MATCHCASE;
        if(pFr->Flags & FR_WHOLEWORD)
            flags |= FR_WHOLEWORD;

        ret = SendMessageW(hEditorWnd, EM_FINDTEXTW, (WPARAM)flags, (LPARAM)&ft);

        if(ret == -1)
        {
            if(cr.cpMax == length && cr.cpMax != startPos)
            {
                ft.chrg.cpMin = cr.cpMin = 0;
                ft.chrg.cpMax = cr.cpMax = startPos;

                ret = SendMessageW(hEditorWnd, EM_FINDTEXTW, (WPARAM)flags, (LPARAM)&ft);
            }
        }

        if(ret == -1)
        {
            pFr->lCustData = -1;
            MessageBoxW(hMainWnd, MAKEINTRESOURCEW(STRING_SEARCH_FINISHED), wszAppTitle,
                        MB_OK | MB_ICONASTERISK);
        } else
        {
            end = ret + lstrlenW(pFr->lpstrFindWhat);
            cr.cpMin = end;
            SendMessageW(hEditorWnd, EM_SETSEL, (WPARAM)ret, (LPARAM)end);
            SendMessageW(hEditorWnd, EM_SCROLLCARET, 0, 0);
        }
    }

    return 0;
}

static void dialog_find(LPFINDREPLACEW fr)
{
    static WCHAR findBuffer[MAX_STRING_LEN];

    ZeroMemory(fr, sizeof(FINDREPLACEW));
    fr->lStructSize = sizeof(FINDREPLACEW);
    fr->hwndOwner = hMainWnd;
    fr->Flags = FR_HIDEUPDOWN;
    fr->lpstrFindWhat = findBuffer;
    fr->lCustData = -1;
    fr->wFindWhatLen = MAX_STRING_LEN*sizeof(WCHAR);

    hFindWnd = FindTextW(fr);
}


static void DoDefaultFont(void)
{
    static const WCHAR szFaceName[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0};
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));

    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    fmt.dwEffects = 0;

    lstrcpyW(fmt.szFaceName, szFaceName);

    SendMessageW(hEditorWnd, EM_SETCHARFORMAT,  SCF_DEFAULT, (LPARAM)&fmt);
}

static void update_window(void)
{
    RECT rect;

    GetWindowRect(hMainWnd, &rect);

    (void) OnSize(hMainWnd, SIZE_RESTORED, MAKELONG(rect.bottom, rect.right));
}

static void toggle_toolbar(int bandId)
{
    HWND hwndReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    REBARBANDINFOW rbbinfo;
    BOOL hide = TRUE;

    if(!hwndReBar)
        return;

    rbbinfo.cbSize = sizeof(rbbinfo);
    rbbinfo.fMask = RBBIM_STYLE | RBBIM_SIZE;

    SendMessageW(hwndReBar, RB_GETBANDINFO, bandId, (LPARAM)&rbbinfo);

    if(rbbinfo.fStyle & RBBS_HIDDEN)
        hide = FALSE;

    SendMessageW(hwndReBar, RB_SHOWBAND, bandId, hide ? 0 : 1);

    if(bandId == BANDID_TOOLBAR)
    {
        rbbinfo.fMask ^= RBBIM_SIZE;

        SendMessageW(hwndReBar, RB_GETBANDINFO, BANDID_FORMATBAR, (LPARAM)&rbbinfo);

        if(hide)
            rbbinfo.fStyle ^= RBBS_BREAK;
        else
            rbbinfo.fStyle |= RBBS_BREAK;

        SendMessageW(hwndReBar, RB_SETBANDINFO, BANDID_FORMATBAR, (LPARAM)&rbbinfo);
    }
}

BOOL CALLBACK datetime_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                WCHAR buffer[MAX_STRING_LEN];
                SYSTEMTIME st;
                HWND hListWnd = GetDlgItem(hWnd, IDC_DATETIME);
                GetLocalTime(&st);

                GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, 0, (LPWSTR)&buffer,
                               MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, 0, (LPWSTR)&buffer,
                               MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, 0, (LPWSTR)&buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);

                SendMessageW(hListWnd, LB_SETSEL, TRUE, 0);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    {
                        LRESULT index;
                        HWND hListWnd = GetDlgItem(hWnd, IDC_DATETIME);

                        index = SendMessageW(hListWnd, LB_GETCURSEL, 0, 0);

                        if(index != LB_ERR)
                        {
                            WCHAR buffer[MAX_STRING_LEN];
                            SendMessageW(hListWnd, LB_GETTEXT, index, (LPARAM)&buffer);
                            SendMessageW(hEditorWnd, EM_REPLACESEL, TRUE, (LPARAM)&buffer);
                        }
                    }
                    /* Fall through */

                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}

static LRESULT OnCreate( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hToolBarWnd, hFormatBarWnd,  hReBarWnd;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    HANDLE hDLL;
    TBADDBITMAP ab;
    int nStdBitmaps = 0;
    REBARINFO rbi;
    REBARBANDINFOW rbb;
    static const WCHAR wszRichEditDll[] = {'R','I','C','H','E','D','2','0','.','D','L','L','\0'};
    static const WCHAR wszRichEditText[] = {'R','i','c','h','E','d','i','t',' ','t','e','x','t','\0'};

    CreateStatusWindowW(CCS_NODIVIDER|WS_CHILD|WS_VISIBLE, wszRichEditText, hWnd, IDC_STATUSBAR);

    hReBarWnd = CreateWindowExW(WS_EX_TOOLWINDOW, REBARCLASSNAMEW, NULL,
      CCS_NODIVIDER|WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_TOP,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hWnd, (HMENU)IDC_REBAR, hInstance, NULL);

    rbi.cbSize = sizeof(rbi);
    rbi.fMask = 0;
    rbi.himl = NULL;
    if(!SendMessageW(hReBarWnd, RB_SETBARINFO, 0, (LPARAM)&rbi))
        return -1;

    hToolBarWnd = CreateToolbarEx(hReBarWnd, CCS_NOPARENTALIGN|CCS_NOMOVEY|WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_BUTTON,
      IDC_TOOLBAR,
      1, hInstance, IDB_TOOLBAR,
      NULL, 0,
      24, 24, 16, 16, sizeof(TBBUTTON));

    ab.hInst = HINST_COMMCTRL;
    ab.nID = IDB_STD_SMALL_COLOR;
    nStdBitmaps = SendMessageW(hToolBarWnd, TB_ADDBITMAP, 0, (LPARAM)&ab);

    AddButton(hToolBarWnd, nStdBitmaps+STD_FILENEW, ID_FILE_NEW);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILEOPEN, ID_FILE_OPEN);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILESAVE, ID_FILE_SAVE);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PRINT, ID_PRINT);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PRINTPRE, ID_PREVIEW);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FIND, ID_FIND);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_CUT, ID_EDIT_CUT);
    AddButton(hToolBarWnd, nStdBitmaps+STD_COPY, ID_EDIT_COPY);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PASTE, ID_EDIT_PASTE);
    AddButton(hToolBarWnd, nStdBitmaps+STD_UNDO, ID_EDIT_UNDO);
    AddButton(hToolBarWnd, nStdBitmaps+STD_REDOW, ID_EDIT_REDO);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, 0, ID_DATETIME);

    SendMessageW(hToolBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.cbSize = sizeof(rbb);
    rbb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_STYLE;
    rbb.fStyle = RBBS_CHILDEDGE | RBBS_BREAK | RBBS_NOGRIPPER;
    rbb.cx = 0;
    rbb.hwndChild = hToolBarWnd;
    rbb.cxMinChild = 0;
    rbb.cyChild = rbb.cyMinChild = HIWORD(SendMessageW(hToolBarWnd, TB_GETBUTTONSIZE, 0, 0));

    SendMessageW(hReBarWnd, RB_INSERTBAND, BANDID_TOOLBAR, (LPARAM)&rbb);

    hFormatBarWnd = CreateToolbarEx(hReBarWnd,
         CCS_NOPARENTALIGN | CCS_NOMOVEY | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_BUTTON,
         IDC_FORMATBAR, 6, hInstance, IDB_FORMATBAR, NULL, 0, 16, 16, 16, 16, sizeof(TBBUTTON));

    AddButton(hFormatBarWnd, 0, ID_FORMAT_BOLD);
    AddButton(hFormatBarWnd, 1, ID_FORMAT_ITALIC);
    AddButton(hFormatBarWnd, 2, ID_FORMAT_UNDERLINE);
    AddSeparator(hFormatBarWnd);
    AddButton(hFormatBarWnd, 3, ID_ALIGN_LEFT);
    AddButton(hFormatBarWnd, 4, ID_ALIGN_CENTER);
    AddButton(hFormatBarWnd, 5, ID_ALIGN_RIGHT);

    SendMessageW(hFormatBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.hwndChild = hFormatBarWnd;

    SendMessageW(hReBarWnd, RB_INSERTBAND, BANDID_FORMATBAR, (LPARAM)&rbb);

    hDLL = LoadLibraryW(wszRichEditDll);
    if(!hDLL)
    {
        MessageBoxW(hWnd, MAKEINTRESOURCEW(STRING_LOAD_RICHED_FAILED), wszAppTitle,
                    MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }

    hEditorWnd = CreateWindowExW(WS_EX_CLIENTEDGE, wszRichEditClass, NULL,
      WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,
      0, 0, 1000, 100, hWnd, (HMENU)IDC_EDITOR, hInstance, NULL);

    if (!hEditorWnd)
    {
        fprintf(stderr, "Error code %u\n", GetLastError());
        return -1;
    }
    assert(hEditorWnd);

    SetFocus(hEditorWnd);
    SendMessageW(hEditorWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);

    DoDefaultFont();

    DoLoadStrings();
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);

    ID_FINDMSGSTRING = RegisterWindowMessageW(FINDMSGSTRINGW);

    return 0;
}

static LRESULT OnUser( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hwndToolBar = GetDlgItem(hwndReBar, IDC_TOOLBAR);
    HWND hwndFormatBar = GetDlgItem(hwndReBar, IDC_FORMATBAR);
    int from, to;
    CHARFORMAT2W fmt;
    PARAFORMAT2 pf;
    GETTEXTLENGTHEX gt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);

    gt.flags = GTL_NUMCHARS;
    gt.codepage = 1200;

    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_FIND,
                 SendMessageW(hwndEditor, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0) ? 1 : 0);

    SendMessageW(hwndEditor, EM_GETCHARFORMAT, TRUE, (LPARAM)&fmt);

    SendMessageW(hwndEditor, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_UNDO,
      SendMessageW(hwndEditor, EM_CANUNDO, 0, 0));
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_REDO,
      SendMessageW(hwndEditor, EM_CANREDO, 0, 0));
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_CUT, from == to ? 0 : 1);
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_COPY, from == to ? 0 : 1);

    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_BOLD, (fmt.dwMask & CFM_BOLD) &&
            (fmt.dwEffects & CFE_BOLD));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_BOLD, !(fmt.dwMask & CFM_BOLD));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_ITALIC, (fmt.dwMask & CFM_ITALIC) &&
            (fmt.dwEffects & CFE_ITALIC));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_ITALIC, !(fmt.dwMask & CFM_ITALIC));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_UNDERLINE, (fmt.dwMask & CFM_UNDERLINE) &&
            (fmt.dwEffects & CFE_UNDERLINE));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_UNDERLINE, !(fmt.dwMask & CFM_UNDERLINE));

    SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_LEFT, (pf.wAlignment == PFA_LEFT));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_CENTER, (pf.wAlignment == PFA_CENTER));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_RIGHT, (pf.wAlignment == PFA_RIGHT));

    return 0;
}

static LRESULT OnNotify( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    NMHDR *pHdr = (NMHDR *)lParam;

    if (pHdr->hwndFrom != hwndEditor)
        return 0;

    if (pHdr->code == EN_SELCHANGE)
    {
        SELCHANGE *pSC = (SELCHANGE *)lParam;
        char buf[128];

        sprintf( buf,"selection = %d..%d, line count=%ld",
                 pSC->chrg.cpMin, pSC->chrg.cpMax,
        SendMessage(hwndEditor, EM_GETLINECOUNT, 0, 0));
        SetWindowText(GetDlgItem(hWnd, IDC_STATUSBAR), buf);
        SendMessage(hWnd, WM_USER, 0, 0);
        return 1;
    }
    return 0;
}

static LRESULT OnCommand( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatus = GetDlgItem(hWnd, IDC_STATUSBAR);
    static FINDREPLACEW findreplace;

    if ((HWND)lParam == hwndEditor)
        return 0;

    switch(LOWORD(wParam))
    {

    case ID_FILE_EXIT:
        PostMessageW(hWnd, WM_CLOSE, 0, 0);
        break;

    case ID_FILE_NEW:
        if(prompt_save_changes())
        {
            set_caption(NULL);
            wszFileName[0] = '\0';
            SetWindowTextW(hwndEditor, wszFileName);
            SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
            /* FIXME: set default format too */
        }
        break;

    case ID_FILE_OPEN:
        DialogOpenFile();
        break;

    case ID_FILE_SAVE:
        if(wszFileName[0])
        {
            DoSaveFile(wszFileName);
            break;
        }
        /* Fall through */

    case ID_FILE_SAVEAS:
        DialogSaveFile();
        break;

    case ID_FIND:
        dialog_find(&findreplace);
        break;

    case ID_FIND_NEXT:
        handle_findmsg(&findreplace);
        break;

    case ID_PRINT:
    case ID_PREVIEW:
        {
            static const WCHAR wszNotImplemented[] = {'N','o','t',' ',
                                                      'i','m','p','l','e','m','e','n','t','e','d','\0'};
            MessageBoxW(hWnd, wszNotImplemented, wszAppTitle, MB_OK);
        }
        break;

    case ID_FORMAT_BOLD:
    case ID_FORMAT_ITALIC:
    case ID_FORMAT_UNDERLINE:
        {
        CHARFORMAT2W fmt;
        int mask = CFM_BOLD;
        if (LOWORD(wParam) == ID_FORMAT_ITALIC) mask = CFM_ITALIC;
        if (LOWORD(wParam) == ID_FORMAT_UNDERLINE) mask = CFM_UNDERLINE;

        ZeroMemory(&fmt, sizeof(fmt));
        fmt.cbSize = sizeof(fmt);
        SendMessageW(hwndEditor, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        if (!(fmt.dwMask&mask))
            fmt.dwEffects |= mask;
        else
            fmt.dwEffects ^= mask;
        fmt.dwMask = mask;
        SendMessageW(hwndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        break;
        }

    case ID_EDIT_CUT:
        PostMessageW(hwndEditor, WM_CUT, 0, 0);
        break;

    case ID_EDIT_COPY:
        PostMessageW(hwndEditor, WM_COPY, 0, 0);
        break;

    case ID_EDIT_PASTE:
        PostMessageW(hwndEditor, WM_PASTE, 0, 0);
        break;

    case ID_EDIT_CLEAR:
        PostMessageW(hwndEditor, WM_CLEAR, 0, 0);
        break;

    case ID_EDIT_SELECTALL:
        {
        CHARRANGE range = {0, -1};
        SendMessageW(hwndEditor, EM_EXSETSEL, 0, (LPARAM)&range);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_GETTEXT:
        {
        int nLen = GetWindowTextLengthW(hwndEditor);
        LPWSTR data = HeapAlloc( GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR) );
        TEXTRANGEW tr;

        GetWindowTextW(hwndEditor, data, nLen+1);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);

        HeapFree( GetProcessHeap(), 0, data);
        data = HeapAlloc(GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR));
        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = nLen;
        tr.lpstrText = data;
        SendMessage (hwndEditor, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data );

        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_CHARFORMAT:
    case ID_EDIT_DEFCHARFORMAT:
        {
        CHARFORMAT2W cf;
        LRESULT i;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = 0;
        i = SendMessageW(hwndEditor, EM_GETCHARFORMAT,
                        LOWORD(wParam) == ID_EDIT_CHARFORMAT, (LPARAM)&cf);
        return 0;
        }

    case ID_EDIT_PARAFORMAT:
        {
        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(pf));
        pf.cbSize = sizeof(pf);
        SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        return 0;
        }

    case ID_EDIT_SELECTIONINFO:
        {
        CHARRANGE range = {0, -1};
        char buf[128];
        WCHAR *data = NULL;

        SendMessage(hwndEditor, EM_EXGETSEL, 0, (LPARAM)&range);
        data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * (range.cpMax-range.cpMin+1));
        SendMessage(hwndEditor, EM_GETSELTEXT, 0, (LPARAM)data);
        sprintf(buf, "Start = %d, End = %d", range.cpMin, range.cpMax);
        MessageBoxA(hWnd, buf, "Editor", MB_OK);
        MessageBoxW(hWnd, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_READONLY:
        {
        long nStyle = GetWindowLong(hwndEditor, GWL_STYLE);
        if (nStyle & ES_READONLY)
            SendMessageW(hwndEditor, EM_SETREADONLY, 0, 0);
        else
            SendMessageW(hwndEditor, EM_SETREADONLY, 1, 0);
        return 0;
        }

    case ID_EDIT_MODIFIED:
        if (SendMessageW(hwndEditor, EM_GETMODIFY, 0, 0))
            SendMessageW(hwndEditor, EM_SETMODIFY, 0, 0);
        else
            SendMessageW(hwndEditor, EM_SETMODIFY, 1, 0);
        return 0;

    case ID_EDIT_UNDO:
        SendMessageW(hwndEditor, EM_UNDO, 0, 0);
        return 0;

    case ID_EDIT_REDO:
        SendMessageW(hwndEditor, EM_REDO, 0, 0);
        return 0;

    case ID_ALIGN_LEFT:
    case ID_ALIGN_CENTER:
    case ID_ALIGN_RIGHT:
        {
        PARAFORMAT2 pf;

        pf.cbSize = sizeof(pf);
        pf.dwMask = PFM_ALIGNMENT;
        switch(LOWORD(wParam)) {
        case ID_ALIGN_LEFT: pf.wAlignment = PFA_LEFT; break;
        case ID_ALIGN_CENTER: pf.wAlignment = PFA_CENTER; break;
        case ID_ALIGN_RIGHT: pf.wAlignment = PFA_RIGHT; break;
        }
        SendMessageW(hwndEditor, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
        break;
        }

    case ID_BACK_1:
        SendMessageW(hwndEditor, EM_SETBKGNDCOLOR, 1, 0);
        break;

    case ID_BACK_2:
        SendMessageW(hwndEditor, EM_SETBKGNDCOLOR, 0, RGB(255,255,192));
        break;

    case ID_TOGGLE_TOOLBAR:
        toggle_toolbar(BANDID_TOOLBAR);
        update_window();
        break;

    case ID_TOGGLE_FORMATBAR:
        toggle_toolbar(BANDID_FORMATBAR);
        update_window();
        break;

    case ID_TOGGLE_STATUSBAR:
        ShowWindow(hwndStatus, IsWindowVisible(hwndStatus) ? SW_HIDE : SW_SHOW);
        update_window();
        break;

    case ID_DATETIME:
        {
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_DATETIME), hWnd, (DLGPROC)datetime_proc);
        break;
        }

    default:
        SendMessageW(hwndEditor, WM_COMMAND, wParam, lParam);
        break;
    }
    return 0;
}

static LRESULT OnInitPopupMenu( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    HMENU hMenu = (HMENU)wParam;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hwndStatus = GetDlgItem(hWnd, IDC_STATUSBAR);
    PARAFORMAT pf;
    int nAlignment = -1;
    REBARBANDINFOW rbbinfo;
    int selFrom, selTo;
    GETTEXTLENGTHEX gt;
    LRESULT textLength;
    MENUITEMINFOW mi;

    SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&selFrom, (LPARAM)&selTo);
    EnableMenuItem(hMenu, ID_EDIT_COPY, MF_BYCOMMAND|(selFrom == selTo) ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_EDIT_CUT, MF_BYCOMMAND|(selFrom == selTo) ? MF_GRAYED : MF_ENABLED);

    pf.cbSize = sizeof(PARAFORMAT);
    SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    CheckMenuItem(hMenu, ID_EDIT_READONLY,
      MF_BYCOMMAND|(GetWindowLong(hwndEditor, GWL_STYLE)&ES_READONLY ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_EDIT_MODIFIED,
      MF_BYCOMMAND|(SendMessage(hwndEditor, EM_GETMODIFY, 0, 0) ? MF_CHECKED : MF_UNCHECKED));
    if (pf.dwMask & PFM_ALIGNMENT)
        nAlignment = pf.wAlignment;
    CheckMenuItem(hMenu, ID_ALIGN_LEFT, MF_BYCOMMAND|(nAlignment == PFA_LEFT) ?
            MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_CENTER, MF_BYCOMMAND|(nAlignment == PFA_CENTER) ?
            MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_RIGHT, MF_BYCOMMAND|(nAlignment == PFA_RIGHT) ?
            MF_CHECKED : MF_UNCHECKED);
    EnableMenuItem(hMenu, ID_EDIT_UNDO, MF_BYCOMMAND|(SendMessageW(hwndEditor, EM_CANUNDO, 0, 0)) ?
            MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, ID_EDIT_REDO, MF_BYCOMMAND|(SendMessageW(hwndEditor, EM_CANREDO, 0, 0)) ?
            MF_ENABLED : MF_GRAYED);

    rbbinfo.cbSize = sizeof(rbbinfo);
    rbbinfo.fMask = RBBIM_STYLE;
    SendMessageW(hwndReBar, RB_GETBANDINFO, BANDID_TOOLBAR, (LPARAM)&rbbinfo);

    CheckMenuItem(hMenu, ID_TOGGLE_TOOLBAR, MF_BYCOMMAND|(rbbinfo.fStyle & RBBS_HIDDEN) ?
            MF_UNCHECKED : MF_CHECKED);

    SendMessageW(hwndReBar, RB_GETBANDINFO, BANDID_FORMATBAR, (LPARAM)&rbbinfo);

    CheckMenuItem(hMenu, ID_TOGGLE_FORMATBAR, MF_BYCOMMAND|(rbbinfo.fStyle & RBBS_HIDDEN) ?
            MF_UNCHECKED : MF_CHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_STATUSBAR, MF_BYCOMMAND|IsWindowVisible(hwndStatus) ?
            MF_CHECKED : MF_UNCHECKED);

    gt.flags = GTL_NUMCHARS;
    gt.codepage = 1200;
    textLength = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);
    EnableMenuItem(hMenu, ID_FIND, MF_BYCOMMAND|(textLength ? MF_ENABLED : MF_GRAYED));

    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_DATA;

    GetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

    EnableMenuItem(hMenu, ID_FIND_NEXT, MF_BYCOMMAND|((textLength && mi.dwItemData) ?
                   MF_ENABLED : MF_GRAYED));
    return 0;
}

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    int nStatusSize = 0;
    RECT rc;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    int rebarHeight = 0;
    REBARBANDINFOW rbbinfo;
    int rebarRows = 2;

    if (hwndStatusBar)
    {
        SendMessageW(hwndStatusBar, WM_SIZE, 0, 0);
        if (IsWindowVisible(hwndStatusBar))
        {
            GetClientRect(hwndStatusBar, &rc);
            nStatusSize = rc.bottom - rc.top;
        } else
        {
            nStatusSize = 0;
        }
    }
    if (hwndReBar)
    {
        rbbinfo.cbSize = sizeof(rbbinfo);
        rbbinfo.fMask = RBBIM_STYLE;

        SendMessageW(hwndReBar, RB_GETBANDINFO, BANDID_TOOLBAR, (LPARAM)&rbbinfo);
        if(rbbinfo.fStyle & RBBS_HIDDEN)
            rebarRows--;

        SendMessageW(hwndReBar, RB_GETBANDINFO, BANDID_FORMATBAR, (LPARAM)&rbbinfo);
        if(rbbinfo.fStyle & RBBS_HIDDEN)
            rebarRows--;

        rebarHeight = rebarRows ? SendMessageW(hwndReBar, RB_GETBARHEIGHT, 0, 0) : 0;

        rc.top = rc.left = 0;
        rc.bottom = rebarHeight;
        rc.right = LOWORD(lParam);
        SendMessageW(hwndReBar, RB_SIZETORECT, 0, (LPARAM)&rc);
    }
    if (hwndEditor)
    {
        GetClientRect(hWnd, &rc);
        MoveWindow(hwndEditor, 0, rebarHeight, rc.right, rc.bottom-nStatusSize-rebarHeight, TRUE);
    }

    return DefWindowProcW(hWnd, WM_SIZE, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == ID_FINDMSGSTRING)
        return handle_findmsg((LPFINDREPLACEW)lParam);

    switch(msg)
    {
    case WM_CREATE:
        return OnCreate( hWnd, wParam, lParam );

    case WM_USER:
        return OnUser( hWnd, wParam, lParam );

    case WM_NOTIFY:
        return OnNotify( hWnd, wParam, lParam );

    case WM_COMMAND:
        return OnCommand( hWnd, wParam, lParam );

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
        if(prompt_save_changes())
            PostQuitMessage(0);
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            SetFocus(GetDlgItem(hWnd, IDC_EDITOR));
        return 0;

    case WM_INITMENUPOPUP:
        return OnInitPopupMenu( hWnd, wParam, lParam );

    case WM_SIZE:
        return OnSize( hWnd, wParam, lParam );

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPSTR szCmdParagraph, int res)
{
    INITCOMMONCONTROLSEX classes = {8, ICC_BAR_CLASSES|ICC_COOL_CLASSES};
    HACCEL hAccel;
    WNDCLASSW wc;
    MSG msg;
    static const WCHAR wszAccelTable[] = {'M','A','I','N','A','C','C','E','L',
                                          'T','A','B','L','E','\0'};

    InitCommonControlsEx(&classes);

    hAccel = LoadAcceleratorsW(hInstance, wszAccelTable);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WORDPAD));
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = xszMainMenu;
    wc.lpszClassName = wszMainWndClass;
    RegisterClassW(&wc);

    hMainWnd = CreateWindowExW(0, wszMainWndClass, wszAppTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, hInstance, NULL);
    ShowWindow(hMainWnd, SW_SHOWDEFAULT);

    set_caption(NULL);

    HandleCommandLine(GetCommandLineW());

    while(GetMessageW(&msg,0,0,0))
    {
        if (IsDialogMessage(hFindWnd, &msg))
            continue;

        if (TranslateAcceleratorW(hMainWnd, hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!PeekMessageW(&msg, 0, 0, 0, PM_NOREMOVE))
            SendMessageW(hMainWnd, WM_USER, 0, 0);
    }

    return 0;
}
