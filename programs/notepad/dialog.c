/*
 *  Notepad (dialog.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *  Copyright 2007 Rolf Kalbermatter
 *  Copyright 2010 Vitaly Perov
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

#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <winternl.h>

#include "main.h"
#include "dialog.h"

#define SPACES_IN_TAB 8
#define PRINT_LEN_MAX 500

static INT_PTR WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* Swap bytes of WCHAR buffer (big-endian <-> little-endian). */
static inline void byteswap_wide_string(LPWSTR str, UINT num)
{
    UINT i;
    for (i = 0; i < num; i++) str[i] = RtlUshortByteSwap(str[i]);
}

static void load_encoding_name(ENCODING enc, WCHAR* buffer, int length)
{
    switch (enc)
    {
        case ENCODING_UTF16LE:
            LoadStringW(Globals.hInstance, STRING_UNICODE_LE, buffer, length);
            break;

        case ENCODING_UTF16BE:
            LoadStringW(Globals.hInstance, STRING_UNICODE_BE, buffer, length);
            break;

        case ENCODING_UTF8:
            LoadStringW(Globals.hInstance, STRING_UTF8, buffer, length);
            break;

        case ENCODING_ANSI:
        {
            CPINFOEXW cpi;
            GetCPInfoExW(CP_ACP, 0, &cpi);
            lstrcpynW(buffer, cpi.CodePageName, length);
            break;
        }

        default:
            assert(0 && "bad encoding in load_encoding_name");
            break;
    }
}

VOID ShowLastError(void)
{
    DWORD error = GetLastError();
    if (error != NO_ERROR)
    {
        LPWSTR lpMsgBuf;
        WCHAR szTitle[MAX_STRING_LEN];

        LoadStringW(Globals.hInstance, STRING_ERROR, szTitle, ARRAY_SIZE(szTitle));
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0, (LPWSTR)&lpMsgBuf, 0, NULL);
        MessageBoxW(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
}

/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    Untitled - Notepad        if no file is open
 *    filename - Notepad        if a file is given
 */
void UpdateWindowCaption(void)
{
  WCHAR szNotepad[64];
  WCHAR szCaption[ARRAY_SIZE(Globals.szFileTitle) + ARRAY_SIZE(L" - ") + ARRAY_SIZE(szNotepad)];

  if (Globals.szFileTitle[0] != '\0')
      lstrcpyW(szCaption, Globals.szFileTitle);
  else
      LoadStringW(Globals.hInstance, STRING_UNTITLED, szCaption, ARRAY_SIZE(szCaption));

  LoadStringW(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));
  lstrcatW(szCaption, L" - ");
  lstrcatW(szCaption, szNotepad);

  SetWindowTextW(Globals.hMainWnd, szCaption);
}

int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCWSTR szString, DWORD dwFlags)
{
   WCHAR szMessage[MAX_STRING_LEN];
   WCHAR szResource[MAX_STRING_LEN];

   /* Load and format szMessage */
   LoadStringW(Globals.hInstance, formatId, szResource, ARRAY_SIZE(szResource));
   wnsprintfW(szMessage, ARRAY_SIZE(szMessage), szResource, szString);

   /* Load szCaption */
   if ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION)
     LoadStringW(Globals.hInstance, STRING_ERROR,  szResource, ARRAY_SIZE(szResource));
   else
     LoadStringW(Globals.hInstance, STRING_NOTEPAD,  szResource, ARRAY_SIZE(szResource));

   /* Display Modal Dialog */
   if (hParent == NULL)
     hParent = Globals.hMainWnd;
   return MessageBoxW(hParent, szMessage, szResource, dwFlags);
}

static void AlertFileNotFound(LPCWSTR szFileName)
{
   DIALOG_StringMsgBox(NULL, STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION|MB_OK);
}

static int AlertFileNotSaved(LPCWSTR szFileName)
{
   WCHAR szUntitled[MAX_STRING_LEN];

   LoadStringW(Globals.hInstance, STRING_UNTITLED, szUntitled, ARRAY_SIZE(szUntitled));
   return DIALOG_StringMsgBox(NULL, STRING_NOTSAVED, szFileName[0] ? szFileName : szUntitled,
     MB_ICONQUESTION|MB_YESNOCANCEL);
}

static int AlertUnicodeCharactersLost(LPCWSTR szFileName)
{
    WCHAR szCaption[MAX_STRING_LEN];
    WCHAR szMsgFormat[MAX_STRING_LEN];
    WCHAR szEnc[MAX_STRING_LEN];
    WCHAR* szMsg;
    DWORD_PTR args[2];
    int rc;

    LoadStringW(Globals.hInstance, STRING_NOTEPAD, szCaption,
                ARRAY_SIZE(szCaption));
    LoadStringW(Globals.hInstance, STRING_LOSS_OF_UNICODE_CHARACTERS,
                szMsgFormat, ARRAY_SIZE(szMsgFormat));
    load_encoding_name(ENCODING_ANSI, szEnc, ARRAY_SIZE(szEnc));
    args[0] = (DWORD_PTR)szFileName;
    args[1] = (DWORD_PTR)szEnc;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY, szMsgFormat, 0, 0, (LPWSTR)&szMsg, 0, (va_list *)args);
    rc = MessageBoxW(Globals.hMainWnd, szMsg, szCaption,
                     MB_OKCANCEL|MB_ICONEXCLAMATION);
    LocalFree(szMsg);
    return rc;
}

/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPCWSTR szFilename)
{
   WIN32_FIND_DATAW entry;
   HANDLE hFile;

   hFile = FindFirstFileW(szFilename, &entry);
   FindClose(hFile);

   return (hFile != INVALID_HANDLE_VALUE);
}

static inline BOOL is_conversion_to_ansi_lossy(LPCWSTR textW, int lenW)
{
    BOOL ret = FALSE;
    WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, textW, lenW, NULL, 0,
                        NULL, &ret);
    return ret;
}

typedef enum
{
    SAVED_OK,
    SAVE_FAILED,
    SHOW_SAVEAS_DIALOG
} SAVE_STATUS;

/* szFileName is the filename to save under; enc is the encoding to use.
 *
 * If the function succeeds, it returns SAVED_OK.
 * If the function fails, it returns SAVE_FAILED.
 * If Unicode data could be lost due to conversion to a non-Unicode character
 * set, a warning is displayed. The user can continue (and the function carries
 * on), or cancel (and the function returns SHOW_SAVEAS_DIALOG).
 */
static SAVE_STATUS DoSaveFile(LPCWSTR szFileName, ENCODING enc)
{
    int lenW;
    WCHAR* textW;
    HANDLE hFile;
    DWORD dwNumWrite;
    PVOID pBytes;
    DWORD size;

    /* lenW includes the byte-order mark, but not the \0. */
    lenW = GetWindowTextLengthW(Globals.hEdit) + 1;
    textW = HeapAlloc(GetProcessHeap(), 0, (lenW+1) * sizeof(WCHAR));
    if (!textW)
    {
        ShowLastError();
        return SAVE_FAILED;
    }
    textW[0] = (WCHAR) 0xfeff;
    lenW = GetWindowTextW(Globals.hEdit, textW+1, lenW) + 1;

    switch (enc)
    {
    case ENCODING_UTF16BE:
        byteswap_wide_string(textW, lenW);
        /* fall through */

    case ENCODING_UTF16LE:
        size = lenW * sizeof(WCHAR);
        pBytes = textW;
        break;

    case ENCODING_UTF8:
        size = WideCharToMultiByte(CP_UTF8, 0, textW, lenW, NULL, 0, NULL, NULL);
        pBytes = HeapAlloc(GetProcessHeap(), 0, size);
        if (!pBytes)
        {
            ShowLastError();
            HeapFree(GetProcessHeap(), 0, textW);
            return SAVE_FAILED;
        }
        WideCharToMultiByte(CP_UTF8, 0, textW, lenW, pBytes, size, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, textW);
        break;

    default:
        if (is_conversion_to_ansi_lossy(textW+1, lenW-1)
            && AlertUnicodeCharactersLost(szFileName) == IDCANCEL)
        {
            HeapFree(GetProcessHeap(), 0, textW);
            return SHOW_SAVEAS_DIALOG;
        }

        size = WideCharToMultiByte(CP_ACP, 0, textW+1, lenW-1, NULL, 0, NULL, NULL);
        pBytes = HeapAlloc(GetProcessHeap(), 0, size);
        if (!pBytes)
        {
            ShowLastError();
            HeapFree(GetProcessHeap(), 0, textW);
            return SAVE_FAILED;
        }
        WideCharToMultiByte(CP_ACP, 0, textW+1, lenW-1, pBytes, size, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, textW);
        break;
    }

    hFile = CreateFileW(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        HeapFree(GetProcessHeap(), 0, pBytes);
        return SAVE_FAILED;
    }
    if (!WriteFile(hFile, pBytes, size, &dwNumWrite, NULL))
    {
        ShowLastError();
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, pBytes);
        return SAVE_FAILED;
    }
    SetEndOfFile(hFile);
    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, pBytes);

    SendMessageW(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
    return SAVED_OK;
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(void)
{
    int nResult;

    nResult=GetWindowTextLengthW(Globals.hEdit);
    if (SendMessageW(Globals.hEdit, EM_GETMODIFY, 0, 0) &&
        (nResult || Globals.szFileName[0]))
    {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult) {
            case IDYES:     return DIALOG_FileSave();

            case IDNO:      break;

            case IDCANCEL:  return(FALSE);

            default:        return(FALSE);
        } /* switch */
    } /* if */

    SetFileNameAndEncoding(L"", ENCODING_ANSI);

    UpdateWindowCaption();
    return(TRUE);
}

static inline ENCODING detect_encoding_of_buffer(const void* buffer, int size)
{
    static const char bom_utf8[] = { 0xef, 0xbb, 0xbf };
    if (size >= sizeof(bom_utf8) && !memcmp(buffer, bom_utf8, sizeof(bom_utf8)))
        return ENCODING_UTF8;
    else
    {
        int flags = IS_TEXT_UNICODE_SIGNATURE |
                    IS_TEXT_UNICODE_REVERSE_SIGNATURE |
                    IS_TEXT_UNICODE_ODD_LENGTH;
        IsTextUnicode(buffer, size, &flags);
        if (flags & IS_TEXT_UNICODE_SIGNATURE)
            return ENCODING_UTF16LE;
        else if (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
            return ENCODING_UTF16BE;
        else
            return ENCODING_ANSI;
    }
}

void DoOpenFile(LPCWSTR szFileName, ENCODING enc)
{
    HANDLE hFile;
    LPSTR pTemp;
    DWORD size;
    DWORD dwNumRead;
    int lenW;
    WCHAR* textW;
    int i;
    WCHAR log[5];

    /* Close any files and prompt to save changes */
    if (!DoCloseFile())
	return;

    hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
	AlertFileNotFound(szFileName);
	return;
    }

    size = GetFileSize(hFile, NULL);
    if (size == INVALID_FILE_SIZE)
    {
	CloseHandle(hFile);
	ShowLastError();
	return;
    }

    /* Extra memory for (WCHAR)'\0'-termination. */
    pTemp = HeapAlloc(GetProcessHeap(), 0, size+2);
    if (!pTemp)
    {
	CloseHandle(hFile);
	ShowLastError();
	return;
    }

    if (!ReadFile(hFile, pTemp, size, &dwNumRead, NULL))
    {
	CloseHandle(hFile);
	HeapFree(GetProcessHeap(), 0, pTemp);
	ShowLastError();
	return;
    }

    CloseHandle(hFile);

    size = dwNumRead;

    if (enc == ENCODING_AUTO)
        enc = detect_encoding_of_buffer(pTemp, size);
    else if (size >= 2 && (enc==ENCODING_UTF16LE || enc==ENCODING_UTF16BE))
    {
        /* If UTF-16 (BE or LE) is selected, and there is a UTF-16 BOM,
         * override the selection (like native Notepad).
         */
        if ((BYTE)pTemp[0] == 0xff && (BYTE)pTemp[1] == 0xfe)
            enc = ENCODING_UTF16LE;
        else if ((BYTE)pTemp[0] == 0xfe && (BYTE)pTemp[1] == 0xff)
            enc = ENCODING_UTF16BE;
    }

    switch (enc)
    {
    case ENCODING_UTF16BE:
        byteswap_wide_string((WCHAR*) pTemp, size/sizeof(WCHAR));
        /* Forget whether the file is BE or LE, like native Notepad. */
        enc = ENCODING_UTF16LE;

        /* fall through */

    case ENCODING_UTF16LE:
        textW = (LPWSTR)pTemp;
        lenW  = size/sizeof(WCHAR);
        break;

    default:
        {
            int cp = (enc==ENCODING_UTF8) ? CP_UTF8 : CP_ACP;
            lenW = MultiByteToWideChar(cp, 0, pTemp, size, NULL, 0);
            textW = HeapAlloc(GetProcessHeap(), 0, (lenW+1) * sizeof(WCHAR));
            if (!textW)
            {
                ShowLastError();
                HeapFree(GetProcessHeap(), 0, pTemp);
                return;
            }
            MultiByteToWideChar(cp, 0, pTemp, size, textW, lenW);
            HeapFree(GetProcessHeap(), 0, pTemp);
            break;
        }
    }

    /* Replace '\0's with spaces. Other than creating a custom control that
     * can deal with '\0' characters, it's the best that can be done.
     */
    for (i = 0; i < lenW; i++)
        if (textW[i] == '\0')
            textW[i] = ' ';
    textW[lenW] = '\0';

    if (lenW >= 1 && textW[0] == 0xfeff)
        SetWindowTextW(Globals.hEdit, textW+1);
    else
        SetWindowTextW(Globals.hEdit, textW);

    HeapFree(GetProcessHeap(), 0, textW);

    SendMessageW(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
    SendMessageW(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    SetFocus(Globals.hEdit);
    
    /*  If the file starts with .LOG, add a time/date at the end and set cursor after */
    if (GetWindowTextW(Globals.hEdit, log, ARRAY_SIZE(log)) && !lstrcmpW(log, L".LOG"))
    {
        SendMessageW(Globals.hEdit, EM_SETSEL, GetWindowTextLengthW(Globals.hEdit), -1);
        SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
	DIALOG_EditTimeDate();
        SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
    }

    SetFileNameAndEncoding(szFileName, enc);
    UpdateWindowCaption();
}

VOID DIALOG_FileNew(VOID)
{
    /* Close any files and prompt to save changes */
    if (DoCloseFile()) {
        SetWindowTextW(Globals.hEdit, L"");
        SendMessageW(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        SetFocus(Globals.hEdit);
    }
}

/* Used to detect encoding of files selected in Open dialog.
 * Returns ENCODING_AUTO if file can't be read, etc.
 */
static ENCODING detect_encoding_of_file(LPCWSTR szFileName)
{
    DWORD size;
    HANDLE hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return ENCODING_AUTO;
    size = GetFileSize(hFile, NULL);
    if (size == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return ENCODING_AUTO;
    }
    else
    {
        DWORD dwNumRead;
        BYTE buffer[MAX_STRING_LEN];
        if (!ReadFile(hFile, buffer, min(size, sizeof(buffer)), &dwNumRead, NULL))
        {
            CloseHandle(hFile);
            return ENCODING_AUTO;
        }
        CloseHandle(hFile);
        return detect_encoding_of_buffer(buffer, dwNumRead);
    }
}

static LPWSTR dialog_print_to_file(HWND hMainWnd)
{
    OPENFILENAMEW ofn;
    static WCHAR file[MAX_PATH] = L"output.prn";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"prn";

    if(GetSaveFileNameW(&ofn))
        return file;
    else
        return FALSE;
}
static UINT_PTR CALLBACK OfnHookProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hEncCombo;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ENCODING enc;
            hEncCombo = GetDlgItem(hdlg, IDC_OFN_ENCCOMBO);
            for (enc = MIN_ENCODING; enc <= MAX_ENCODING; enc++)
            {
                WCHAR szEnc[MAX_STRING_LEN];
                load_encoding_name(enc, szEnc, ARRAY_SIZE(szEnc));
                SendMessageW(hEncCombo, CB_ADDSTRING, 0, (LPARAM)szEnc);
            }
            SendMessageW(hEncCombo, CB_SETCURSEL, (WPARAM)Globals.encOfnCombo, 0);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_OFN_ENCCOMBO &&
            HIWORD(wParam) == CBN_SELCHANGE)
        {
            int index = SendMessageW(hEncCombo, CB_GETCURSEL, 0, 0);
            Globals.encOfnCombo = index==CB_ERR ? ENCODING_ANSI : (ENCODING)index;
        }

        break;

    case WM_NOTIFY:
        switch (((OFNOTIFYW*)lParam)->hdr.code)
        {
            case CDN_SELCHANGE:
                if (Globals.bOfnIsOpenDialog)
                {
                    /* Check the start of the selected file for a BOM. */
                    ENCODING enc;
                    WCHAR szFileName[MAX_PATH];
                    SendMessageW(GetParent(hdlg), CDM_GETFILEPATH,
                                 ARRAY_SIZE(szFileName), (LPARAM)szFileName);
                    enc = detect_encoding_of_file(szFileName);
                    if (enc != ENCODING_AUTO)
                    {
                        Globals.encOfnCombo = enc;
                        SendMessageW(hEncCombo, CB_SETCURSEL, (WPARAM)enc, 0);
                    }
                }
                break;

            default:
                break;
        }
        break;

    default:
        break;
    }
    return 0;
}

VOID DIALOG_FileOpen(VOID)
{
    OPENFILENAMEW openfilename;
    WCHAR szPath[MAX_PATH];

    ZeroMemory(&openfilename, sizeof(openfilename));

    lstrcpyW(szPath, L"*.txt");

    openfilename.lStructSize       = sizeof(openfilename);
    openfilename.hwndOwner         = Globals.hMainWnd;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = Globals.szFilter;
    openfilename.lpstrFile         = szPath;
    openfilename.nMaxFile          = ARRAY_SIZE(szPath);
    openfilename.Flags = OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER |
                         OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                         OFN_HIDEREADONLY | OFN_ENABLESIZING;
    openfilename.lpfnHook          = OfnHookProc;
    openfilename.lpTemplateName    = MAKEINTRESOURCEW(IDD_OFN_TEMPLATE);
    openfilename.lpstrDefExt       = L"txt";

    Globals.encOfnCombo = ENCODING_ANSI;
    Globals.bOfnIsOpenDialog = TRUE;

    if (GetOpenFileNameW(&openfilename))
        DoOpenFile(openfilename.lpstrFile, Globals.encOfnCombo);
}

/* Return FALSE to cancel close */
BOOL DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == '\0')
        return DIALOG_FileSaveAs();
    else
    {
        switch (DoSaveFile(Globals.szFileName, Globals.encFile))
        {
            case SAVED_OK:           return TRUE;
            case SHOW_SAVEAS_DIALOG: return DIALOG_FileSaveAs();
            default:                 return FALSE;
        }
    }
}

BOOL DIALOG_FileSaveAs(VOID)
{
    OPENFILENAMEW saveas;
    WCHAR szPath[MAX_PATH];

    ZeroMemory(&saveas, sizeof(saveas));

    lstrcpyW(szPath, L"*.txt");

    saveas.lStructSize       = sizeof(OPENFILENAMEW);
    saveas.hwndOwner         = Globals.hMainWnd;
    saveas.hInstance         = Globals.hInstance;
    saveas.lpstrFilter       = Globals.szFilter;
    saveas.lpstrFile         = szPath;
    saveas.nMaxFile          = ARRAY_SIZE(szPath);
    saveas.Flags          = OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER |
                            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY | OFN_ENABLESIZING;
    saveas.lpfnHook          = OfnHookProc;
    saveas.lpTemplateName    = MAKEINTRESOURCEW(IDD_OFN_TEMPLATE);
    saveas.lpstrDefExt       = L"txt";

    /* Preset encoding to what file was opened/saved last with. */
    Globals.encOfnCombo = Globals.encFile;
    Globals.bOfnIsOpenDialog = FALSE;

retry:
    if (!GetSaveFileNameW(&saveas))
        return FALSE;

    switch (DoSaveFile(szPath, Globals.encOfnCombo))
    {
        case SAVED_OK:
            SetFileNameAndEncoding(szPath, Globals.encOfnCombo);
            UpdateWindowCaption();
            return TRUE;

        case SHOW_SAVEAS_DIALOG:
            goto retry;

        default:
            return FALSE;
    }
}

typedef struct {
    LPWSTR mptr;
    LPWSTR mend;
    LPWSTR lptr;
    DWORD len;
} TEXTINFO, *LPTEXTINFO;

static int notepad_print_header(HDC hdc, RECT *rc, BOOL dopage, BOOL header, int page, LPWSTR text)
{
    SIZE szMetric;

    if (*text)
    {
        /* Write the header or footer */
        GetTextExtentPoint32W(hdc, text, lstrlenW(text), &szMetric);
        if (dopage)
            ExtTextOutW(hdc, (rc->left + rc->right - szMetric.cx) / 2,
                        header ? rc->top : rc->bottom - szMetric.cy,
                        ETO_CLIPPED, rc, text, lstrlenW(text), NULL);
        return 1;
    }
    return 0;
}

static WCHAR *expand_header_vars(WCHAR *pattern, int page)
{
    int length = 0;
    int i;
    BOOL inside = FALSE;
    WCHAR *buffer = NULL;

    for (i = 0; pattern[i]; i++)
    {
        if (inside)
        {
            if (pattern[i] == '&')
                length++;
            else if (pattern[i] == 'p')
                length += 11;
            inside = FALSE;
        }
        else if (pattern[i] == '&')
            inside = TRUE;
        else
            length++;
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR));
    if (buffer)
    {
        int j = 0;
        inside = FALSE;
        for (i = 0; pattern[i]; i++)
        {
            if (inside)
            {
                if (pattern[i] == '&')
                    buffer[j++] = '&';
                else if (pattern[i] == 'p')
                    j += wnsprintfW(&buffer[j], 11, L"%d", page);
                inside = FALSE;
            }
            else if (pattern[i] == '&')
                inside = TRUE;
            else
                buffer[j++] = pattern[i];
        }
        buffer[j++] = 0;
    }
    return buffer;
}

static BOOL notepad_print_page(HDC hdc, RECT *rc, BOOL dopage, int page, LPTEXTINFO tInfo)
{
    int b, y;
    TEXTMETRICW tm;
    SIZE szMetrics;
    WCHAR *footer_text = NULL;

    footer_text = expand_header_vars(Globals.szFooter, page);
    if (footer_text == NULL)
        return FALSE;

    if (dopage)
    {
        if (StartPage(hdc) <= 0)
        {
            MessageBoxW(Globals.hMainWnd, L"StartPage failed", L"Print Error", MB_ICONEXCLAMATION);
            HeapFree(GetProcessHeap(), 0, footer_text);
            return FALSE;
        }
    }

    GetTextMetricsW(hdc, &tm);
    y = rc->top + notepad_print_header(hdc, rc, dopage, TRUE, page, Globals.szFileName) * tm.tmHeight;
    b = rc->bottom - 2 * notepad_print_header(hdc, rc, FALSE, FALSE, page, footer_text) * tm.tmHeight;

    do {
        INT m, n;

        if (!tInfo->len)
        {
            /* find the end of the line */
            while (tInfo->mptr < tInfo->mend && *tInfo->mptr != '\n' && *tInfo->mptr != '\r')
            {
                if (*tInfo->mptr == '\t')
                {
                    /* replace tabs with spaces */
                    for (m = 0; m < SPACES_IN_TAB; m++)
                    {
                        if (tInfo->len < PRINT_LEN_MAX)
                            tInfo->lptr[tInfo->len++] = ' ';
                        else if (Globals.bWrapLongLines)
                            break;
                    }
                }
                else if (tInfo->len < PRINT_LEN_MAX)
                    tInfo->lptr[tInfo->len++] = *tInfo->mptr;

                if (tInfo->len >= PRINT_LEN_MAX && Globals.bWrapLongLines)
                     break;

                tInfo->mptr++;
            }
        }

        /* Find out how much we should print if line wrapping is enabled */
        if (Globals.bWrapLongLines)
        {
            GetTextExtentExPointW(hdc, tInfo->lptr, tInfo->len, rc->right - rc->left, &n, NULL, &szMetrics);
            if (n < tInfo->len && tInfo->lptr[n] != ' ')
            {
                m = n;
                /* Don't wrap words unless it's a single word over the entire line */
                while (m  && tInfo->lptr[m] != ' ') m--;
                if (m > 0) n = m + 1;
            }
        }
        else
            n = tInfo->len;

        if (dopage)
            ExtTextOutW(hdc, rc->left, y, ETO_CLIPPED, rc, tInfo->lptr, n, NULL);

        tInfo->len -= n;

        if (tInfo->len)
        {
            memcpy(tInfo->lptr, tInfo->lptr + n, tInfo->len * sizeof(WCHAR));
            y += tm.tmHeight + tm.tmExternalLeading;
        }
        else
        {
            /* find the next line */
            while (tInfo->mptr < tInfo->mend && y < b && (*tInfo->mptr == '\n' || *tInfo->mptr == '\r'))
            {
                if (*tInfo->mptr == '\n')
                    y += tm.tmHeight + tm.tmExternalLeading;
                tInfo->mptr++;
            }
        }
    } while (tInfo->mptr < tInfo->mend && y < b);

    notepad_print_header(hdc, rc, dopage, FALSE, page, footer_text);
    if (dopage)
    {
        EndPage(hdc);
    }
    HeapFree(GetProcessHeap(), 0, footer_text);
    return TRUE;
}

VOID DIALOG_FilePrint(VOID)
{
    DOCINFOW di;
    PRINTDLGW printer;
    int page, dopage, copy;
    LOGFONTW lfFont;
    HFONT hTextFont, old_font = 0;
    DWORD size;
    BOOL ret = FALSE;
    RECT rc;
    LPWSTR pTemp;
    TEXTINFO tInfo;
    WCHAR cTemp[PRINT_LEN_MAX];

    /* Get Current Settings */
    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize           = sizeof(printer);
    printer.hwndOwner             = Globals.hMainWnd;
    printer.hDevMode              = Globals.hDevMode;
    printer.hDevNames             = Globals.hDevNames;
    printer.hInstance             = Globals.hInstance;

    /* Set some default flags */
    printer.Flags                 = PD_RETURNDC | PD_NOSELECTION;
    printer.nFromPage             = 0;
    printer.nMinPage              = 1;
    /* we really need to calculate number of pages to set nMaxPage and nToPage */
    printer.nToPage               = 0;
    printer.nMaxPage              = -1;
    /* Let commdlg manage copy settings */
    printer.nCopies               = (WORD)PD_USEDEVMODECOPIES;

    if (!PrintDlgW(&printer)) return;

    Globals.hDevMode = printer.hDevMode;
    Globals.hDevNames = printer.hDevNames;

    SetMapMode(printer.hDC, MM_TEXT);

    /* initialize DOCINFO */
    di.cbSize = sizeof(DOCINFOW);
    di.lpszDocName = Globals.szFileTitle;
    di.lpszOutput = NULL;
    di.lpszDatatype = NULL;
    di.fwType = 0; 

    if(printer.Flags & PD_PRINTTOFILE)
    {
        di.lpszOutput = dialog_print_to_file(printer.hwndOwner);
        if(!di.lpszOutput)
            return;
    }

    /* Get the file text */
    size = GetWindowTextLengthW(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!pTemp)
    {
       DeleteDC(printer.hDC);
       ShowLastError();
       return;
    }
    size = GetWindowTextW(Globals.hEdit, pTemp, size);

    if (StartDocW(printer.hDC, &di) > 0)
    {
        /* Get the page margins in pixels. */
        rc.top =    MulDiv(Globals.iMarginTop, GetDeviceCaps(printer.hDC, LOGPIXELSY), 2540) -
                    GetDeviceCaps(printer.hDC, PHYSICALOFFSETY);
        rc.bottom = GetDeviceCaps(printer.hDC, PHYSICALHEIGHT) -
                    MulDiv(Globals.iMarginBottom, GetDeviceCaps(printer.hDC, LOGPIXELSY), 2540);
        rc.left =   MulDiv(Globals.iMarginLeft, GetDeviceCaps(printer.hDC, LOGPIXELSX), 2540) -
                    GetDeviceCaps(printer.hDC, PHYSICALOFFSETX);
        rc.right =  GetDeviceCaps(printer.hDC, PHYSICALWIDTH) -
                    MulDiv(Globals.iMarginRight, GetDeviceCaps(printer.hDC, LOGPIXELSX), 2540);

        /* Create a font for the printer resolution */
        lfFont = Globals.lfFont;
        lfFont.lfHeight = MulDiv(lfFont.lfHeight, GetDeviceCaps(printer.hDC, LOGPIXELSY), GetDpiForWindow(Globals.hMainWnd));
        /* Make the font a bit lighter */
        lfFont.lfWeight -= 100;
        hTextFont = CreateFontIndirectW(&lfFont);
        old_font = SelectObject(printer.hDC, hTextFont);

        for (copy = 1; copy <= printer.nCopies; copy++)
        {
            page = 1;

            tInfo.mptr = pTemp;
            tInfo.mend = pTemp + size;
            tInfo.lptr = cTemp;
            tInfo.len = 0;

            do {
                if (printer.Flags & PD_PAGENUMS)
                {
                    /* a specific range of pages is selected, so
                     * skip pages that are not to be printed
                     */
                    if (page > printer.nToPage)
                        break;
                    else if (page >= printer.nFromPage)
                        dopage = 1;
                    else
                        dopage = 0;
                }
                else
                    dopage = 1;

                ret = notepad_print_page(printer.hDC, &rc, dopage, page, &tInfo);
                page++;
            } while (ret && tInfo.mptr < tInfo.mend);

            if (!ret) break;
        }
        EndDoc(printer.hDC);
        SelectObject(printer.hDC, old_font);
        DeleteObject(hTextFont);
    }
    DeleteDC(printer.hDC);
    HeapFree(GetProcessHeap(), 0, pTemp);
}

VOID DIALOG_FilePrinterSetup(VOID)
{
    PRINTDLGW printer;

    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = Globals.hMainWnd;
    printer.hDevMode            = Globals.hDevMode;
    printer.hDevNames           = Globals.hDevNames;
    printer.hInstance           = Globals.hInstance;
    printer.Flags               = PD_PRINTSETUP;
    printer.nCopies             = 1;

    PrintDlgW(&printer);

    Globals.hDevMode = printer.hDevMode;
    Globals.hDevNames = printer.hDevNames;
}

VOID DIALOG_FileExit(VOID)
{
    PostMessageW(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

VOID DIALOG_EditUndo(VOID)
{
    SendMessageW(Globals.hEdit, EM_UNDO, 0, 0);
}

VOID DIALOG_EditCut(VOID)
{
    SendMessageW(Globals.hEdit, WM_CUT, 0, 0);
}

VOID DIALOG_EditCopy(VOID)
{
    SendMessageW(Globals.hEdit, WM_COPY, 0, 0);
}

VOID DIALOG_EditPaste(VOID)
{
    SendMessageW(Globals.hEdit, WM_PASTE, 0, 0);
}

VOID DIALOG_EditDelete(VOID)
{
    SendMessageW(Globals.hEdit, WM_CLEAR, 0, 0);
}

VOID DIALOG_EditSelectAll(VOID)
{
    SendMessageW(Globals.hEdit, EM_SETSEL, 0, -1);
}

VOID DIALOG_EditTimeDate(VOID)
{
    SYSTEMTIME   st;
    WCHAR        szDate[MAX_STRING_LEN];

    GetLocalTime(&st);

    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, szDate, MAX_STRING_LEN);
    SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);

    SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)L" ");

    GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, MAX_STRING_LEN);
    SendMessageW(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);
}

VOID DIALOG_EditWrap(VOID)
{
    BOOL modify = FALSE;
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                    ES_AUTOVSCROLL | ES_MULTILINE;
    RECT rc;
    DWORD size;
    LPWSTR pTemp;

    size = GetWindowTextLengthW(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    GetWindowTextW(Globals.hEdit, pTemp, size);
    modify = SendMessageW(Globals.hEdit, EM_GETMODIFY, 0, 0);
    DestroyWindow(Globals.hEdit);
    GetClientRect(Globals.hMainWnd, &rc);
    if( Globals.bWrapLongLines ) dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    Globals.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"edit", NULL, dwStyle,
                         0, 0, rc.right, rc.bottom, Globals.hMainWnd,
                         NULL, Globals.hInstance, NULL);
    SendMessageW(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
    SetWindowTextW(Globals.hEdit, pTemp);
    SendMessageW(Globals.hEdit, EM_SETMODIFY, modify, 0);
    SetFocus(Globals.hEdit);
    HeapFree(GetProcessHeap(), 0, pTemp);
    
    Globals.bWrapLongLines = !Globals.bWrapLongLines;
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_WRAP,
        MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
    SetWindowSubclass(Globals.hEdit, EDIT_CallBackProc, 0, 0);
    updateWindowSize(rc.right, rc.bottom);
}

VOID DIALOG_SelectFont(VOID)
{
    CHOOSEFONTW cf;
    LOGFONTW lf=Globals.lfFont;

    ZeroMemory( &cf, sizeof(cf) );
    cf.lStructSize=sizeof(cf);
    cf.hwndOwner=Globals.hMainWnd;
    cf.lpLogFont=&lf;
    cf.Flags=CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;

    if( ChooseFontW(&cf) )
    {
        HFONT currfont=Globals.hFont;

        Globals.hFont=CreateFontIndirectW( &lf );
        Globals.lfFont=lf;
        SendMessageW( Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, TRUE );
        if( currfont!=NULL )
            DeleteObject( currfont );
    }
}

VOID DIALOG_Search(VOID)
{
        /* Allow only one search/replace dialog to open */
        if(Globals.hFindReplaceDlg != NULL)
        {
            SetActiveWindow(Globals.hFindReplaceDlg);
            return;
        }

        ZeroMemory(&Globals.find, sizeof(Globals.find));
        Globals.find.lStructSize      = sizeof(Globals.find);
        Globals.find.hwndOwner        = Globals.hMainWnd;
        Globals.find.hInstance        = Globals.hInstance;
        Globals.find.lpstrFindWhat    = Globals.szFindText;
        Globals.find.wFindWhatLen     = ARRAY_SIZE(Globals.szFindText);
        Globals.find.Flags            = FR_DOWN|FR_HIDEWHOLEWORD;

        /* We only need to create the modal FindReplace dialog which will */
        /* notify us of incoming events using hMainWnd Window Messages    */

        Globals.hFindReplaceDlg = FindTextW(&Globals.find);
        assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_SearchNext(VOID)
{
    if (Globals.lastFind.lpstrFindWhat == NULL)
        DIALOG_Search();
    else                /* use the last find data */
        NOTEPAD_DoFind(&Globals.lastFind);
}

VOID DIALOG_Replace(VOID)
{
        /* Allow only one search/replace dialog to open */
        if(Globals.hFindReplaceDlg != NULL)
        {
            SetActiveWindow(Globals.hFindReplaceDlg);
            return;
        }

        ZeroMemory(&Globals.find, sizeof(Globals.find));
        Globals.find.lStructSize      = sizeof(Globals.find);
        Globals.find.hwndOwner        = Globals.hMainWnd;
        Globals.find.hInstance        = Globals.hInstance;
        Globals.find.lpstrFindWhat    = Globals.szFindText;
        Globals.find.wFindWhatLen     = ARRAY_SIZE(Globals.szFindText);
        Globals.find.lpstrReplaceWith = Globals.szReplaceText;
        Globals.find.wReplaceWithLen  = ARRAY_SIZE(Globals.szReplaceText);
        Globals.find.Flags            = FR_DOWN|FR_HIDEWHOLEWORD;

        /* We only need to create the modal FindReplace dialog which will */
        /* notify us of incoming events using hMainWnd Window Messages    */

        Globals.hFindReplaceDlg = ReplaceTextW(&Globals.find);
        assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_HelpContents(VOID)
{
    WinHelpW(Globals.hMainWnd, L"notepad.hlp", HELP_INDEX, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID)
{
    WCHAR szNotepad[MAX_STRING_LEN];
    HICON icon = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_NOTEPAD),
                            IMAGE_ICON, 48, 48, LR_SHARED);

    LoadStringW(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));
    ShellAboutW(Globals.hMainWnd, szNotepad, L"Wine Notepad", icon);
}


/***********************************************************************
 *
 *           DIALOG_FilePageSetup
 */
VOID DIALOG_FilePageSetup(void)
{
    DialogBoxW(Globals.hInstance, MAKEINTRESOURCEW(DIALOG_PAGESETUP),
               Globals.hMainWnd, DIALOG_PAGESETUP_DlgProc);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static INT_PTR WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

   switch (msg)
    {
    case WM_COMMAND:
      switch (wParam)
        {
        case IDOK:
          /* save user input and close dialog */
          GetDlgItemTextW(hDlg, IDC_PAGESETUP_HEADERVALUE, Globals.szHeader, ARRAY_SIZE(Globals.szHeader));
          GetDlgItemTextW(hDlg, IDC_PAGESETUP_FOOTERVALUE, Globals.szFooter, ARRAY_SIZE(Globals.szFooter));

          Globals.iMarginTop = GetDlgItemInt(hDlg, IDC_PAGESETUP_TOPVALUE, NULL, FALSE) * 100;
          Globals.iMarginBottom = GetDlgItemInt(hDlg, IDC_PAGESETUP_BOTTOMVALUE, NULL, FALSE) * 100;
          Globals.iMarginLeft = GetDlgItemInt(hDlg, IDC_PAGESETUP_LEFTVALUE, NULL, FALSE) * 100;
          Globals.iMarginRight = GetDlgItemInt(hDlg, IDC_PAGESETUP_RIGHTVALUE, NULL, FALSE) * 100;
          EndDialog(hDlg, IDOK);
          return TRUE;

        case IDCANCEL:
          /* discard user input and close dialog */
          EndDialog(hDlg, IDCANCEL);
          return TRUE;

        case IDHELP:
        {
          /* FIXME: Bring this to work */
          MessageBoxW(Globals.hMainWnd, L"Sorry, no help available", L"Help", MB_ICONEXCLAMATION);
          return TRUE;
        }

	default:
	    break;
        }
      break;

    case WM_INITDIALOG:
       /* fetch last user input prior to display dialog */
       SetDlgItemTextW(hDlg, IDC_PAGESETUP_HEADERVALUE, Globals.szHeader);
       SetDlgItemTextW(hDlg, IDC_PAGESETUP_FOOTERVALUE, Globals.szFooter);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_TOPVALUE, Globals.iMarginTop / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_BOTTOMVALUE, Globals.iMarginBottom / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_LEFTVALUE, Globals.iMarginLeft / 100, FALSE);
       SetDlgItemInt(hDlg, IDC_PAGESETUP_RIGHTVALUE, Globals.iMarginRight / 100, FALSE);
       break;
    }

  return FALSE;
}

static INT_PTR WINAPI DIALOG_GOTO_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDOK:
                {
                    int lineValue = GetDlgItemInt(hDlg, IDC_GOTO_LINEVALUE, NULL, FALSE) - 1;
                    long lineIndex = SendMessageW(Globals.hEdit, EM_LINEINDEX, lineValue, 0);

                    SendMessageW(Globals.hEdit, EM_SETSEL, lineIndex, lineIndex);
                    UpdateStatusBar();
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case WM_INITDIALOG:
        {
            int currentLine = SendMessageW(Globals.hEdit, EM_LINEFROMCHAR, -1, 0) + 1;
            SetDlgItemInt(hDlg, IDC_GOTO_LINEVALUE, currentLine, FALSE);
            break;
        }
    }
    return FALSE;
}

void DIALOG_EditGoTo(void)
{
    DialogBoxW(Globals.hInstance, MAKEINTRESOURCEW(DIALOG_GOTO),
               Globals.hMainWnd, DIALOG_GOTO_DlgProc);
}
