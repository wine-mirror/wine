/*
 * GUI support
 *
 * Copyright 2004 Ferenc Wagner
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
 *
 */

#include <windows.h>
#include <commctrl.h>

#include "guires.h"
#include "winetest.h"

/* Event object to signal successful window creation to main thread.
 */
HANDLE initEvent;

/* Dialog handle
 */
HWND dialog;

/* Progress data for the text* functions and for scaling.
 */
unsigned int progressMax, progressCurr;
double progressScale;

/* Progress group counter for the gui* functions.
 */
int progressGroup = -1;

char *
renderString (va_list ap)
{
    const char *fmt = va_arg (ap, char*);
    static char buffer[128];

    vsnprintf (buffer, sizeof buffer, fmt, ap);
    return buffer;
}

/* report (R_STATUS, fmt, ...) */
int
textStatus (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    fputs (str, stderr);
    fputc ('\n', stderr);
    free (str);
    return 0;
}

int
guiStatus (va_list ap)
{
    size_t len;
    char *str = vstrmake (&len, ap);

    if (len > 128) str[129] = 0;
    SetDlgItemText (dialog, IDC_SB, str);
    free (str);
    return 0;
}

/* report (R_PROGRESS, steps) */
int
textProgress (va_list ap)
{
    progressMax = va_arg (ap, int);
    progressCurr = 0;
    return 0;
}

int
guiProgress (va_list ap)
{
    unsigned int max = va_arg (ap, int);
    HWND pb = GetDlgItem (dialog, IDC_PB0 + ++progressGroup * 2);

    progressMax = max;
    progressCurr = 0;
    if (max > 0xffff) {
        progressScale = (double)0xffff / max;
        max = 0xffff;
    }
    else progressScale = 1;
    SendMessage (pb, PBM_SETRANGE, 0, MAKELPARAM (0, max));
    SendMessage (pb, PBM_SETSTEP, (WPARAM)1, 0);
    return 0;
}

/* report (R_STEP, fmt, ...) */
int
textStep (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    progressCurr++;
    fputs (str, stderr);
    fprintf (stderr, " (%d of %d)\n", progressCurr, progressMax);
    free (str);
    return 0;
}

int
guiStep (va_list ap)
{
    const int pgID = IDC_ST0 + progressGroup * 2;
    char *str = vstrmake (NULL, ap);
    
    progressCurr++;
    SetDlgItemText (dialog, pgID, str);
    SendDlgItemMessage (dialog, pgID+1, PBM_SETPOS,
                        (WPARAM)(progressScale * progressCurr), 0);
    free (str);
    return 0;
}

/* report (R_DELTA, inc, fmt, ...) */
int
textDelta (va_list ap)
{
    const int inc = va_arg (ap, int);
    char *str = vstrmake (NULL, ap);

    progressCurr += inc;
    fputs (str, stderr);
    fprintf (stderr, " (%d of %d)\n", progressCurr, progressMax);
    free (str);
    return 0;
}

int
guiDelta (va_list ap)
{
    const int inc = va_arg (ap, int);
    const int pgID = IDC_ST0 + progressGroup * 2;
    char *str = vstrmake (NULL, ap);

    progressCurr += inc;
    SetDlgItemText (dialog, pgID, str);
    SendDlgItemMessage (dialog, pgID+1, PBM_SETPOS,
                        (WPARAM)(progressScale * progressCurr), 0);
    free (str);
    return 0;
}

/* report (R_DIR, fmt, ...) */
int
textDir (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    fputs ("Temporary directory: ", stderr);
    fputs (str, stderr);
    fputc ('\n', stderr);
    free (str);
    return 0;
}

int
guiDir (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    SetDlgItemText (dialog, IDC_DIR, str);
    free (str);
    return 0;
}

/* report (R_OUT, fmt, ...) */
int
textOut (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    fputs ("Log file: ", stderr);
    fputs (str, stderr);
    fputc ('\n', stderr);
    free (str);
    return 0;
}

int
guiOut (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    SetDlgItemText (dialog, IDC_OUT, str);
    free (str);
    return 0;
}

/* report (R_FATAL, fmt, ...) */
int
textFatal (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    fputs ("Fatal error: ", stderr);
    fputs (str, stderr);
    fputc ('\n', stderr);
    free (str);
    exit (1);
}

int
guiFatal (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    MessageBox (dialog, str, "Fatal Error", MB_ICONERROR | MB_OK);
    free (str);
    exit (1);
}

/* report (R_WARNING, fmt, ...) */
int
textWarning (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    fputs ("Warning: ", stderr);
    fputs (str, stderr);
    fputc ('\n', stderr);
    free (str);
    return 0;
}

int
guiWarning (va_list ap)
{
    char *str = vstrmake (NULL, ap);

    MessageBox (dialog, str, "Warning", MB_ICONWARNING | MB_OK);
    free (str);
    return 0;
}

/* report (R_ASK, type, fmt, ...) */
int
textAsk (va_list ap)
{
    int uType = va_arg (ap, int);
    char *str = vstrmake (NULL, ap);

    fprintf (stderr, "Question of type %d: %s\n!FIXME, stub\n",
             uType, str);
    free (str);
    return 0;
}

int
guiAsk (va_list ap)
{
    int uType = va_arg (ap, int);
    char *str = vstrmake (NULL, ap);
    int ret = MessageBox (dialog, str, "Question",
                          MB_ICONQUESTION | uType);

    free (str);
    return ret;
}

BOOL CALLBACK
AboutProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDCANCEL:
            EndDialog (hwnd, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CALLBACK
DlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SendMessage (hwnd, WM_SETICON, ICON_SMALL,
                     (LPARAM)LoadIcon (GetModuleHandle (NULL),
                                       MAKEINTRESOURCE (IDI_WINE)));
        SendMessage (hwnd, WM_SETICON, ICON_BIG,
                     (LPARAM)LoadIcon (GetModuleHandle (NULL),
                                       MAKEINTRESOURCE (IDI_WINE)));
        dialog = hwnd;
        if (!SetEvent (initEvent)) {
            report (R_STATUS, "Can't signal main thread: %d",
                    GetLastError ());
            EndDialog (hwnd, 2);
        }
        return TRUE;
    case WM_CLOSE:
        EndDialog (hwnd, 3);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDHELP:
            DialogBox (GetModuleHandle (NULL),
                       MAKEINTRESOURCE (IDD_ABOUT), hwnd, AboutProc);
            return TRUE;
        case IDABORT:
            report (R_WARNING, "Not implemented");
            return TRUE;
        }
    }
    return FALSE;
}

DWORD WINAPI
DlgThreadProc ()
{
    int ret;

    InitCommonControls ();
    ret = DialogBox (GetModuleHandle (NULL),
                     MAKEINTRESOURCE (IDD_STATUS),
                     NULL, DlgProc);
    switch (ret) {
    case 0:
        report (R_WARNING, "Invalid parent handle");
        break;
    case 1:
        report (R_WARNING, "DialogBox failed: %d",
                GetLastError ());
        break;
    case 3:
        exit (0);
    default:
        report (R_STATUS, "Dialog exited: %d", ret);
    }
    return 0;
}

int
report (enum report_type t, ...)
{
    typedef int r_fun_t (va_list);

    va_list ap;
    int ret = 0;
    static r_fun_t * const text_funcs[] =
        {textStatus, textProgress, textStep, textDelta,
         textDir, textOut, textFatal, textWarning, textAsk};
    static r_fun_t * const GUI_funcs[] =
        {guiStatus, guiProgress, guiStep, guiDelta,
         guiDir, guiOut, guiFatal, guiWarning, guiAsk};
    static r_fun_t * const * funcs = NULL;

    if (!funcs) {
        HANDLE DlgThread;
        DWORD DlgThreadID;

        funcs = text_funcs;
        initEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
        if (!initEvent)
            report (R_STATUS, "Can't create event object: %d",
                    GetLastError ());
        else {
            DlgThread = CreateThread (NULL, 0, DlgThreadProc,
                                      NULL, 0, &DlgThreadID);
            if (!DlgThread)
                report (R_STATUS, "Can't create GUI thread: %d",
                        GetLastError ());
            else {
                DWORD ret = WaitForSingleObject (initEvent, INFINITE);
                switch (ret) {
                case WAIT_OBJECT_0:
                    funcs = GUI_funcs;
                    break;
                case WAIT_TIMEOUT:
                    report (R_STATUS, "GUI creation timed out");
                    break;
                case WAIT_FAILED:
                    report (R_STATUS, "Wait for GUI failed: %d",
                            GetLastError ());
                    break;
                default:
                    report (R_STATUS, "Wait returned %d",
                            ret);
                    break;
                }
            }
        }
    }
        
    va_start (ap, t);
    if (t < sizeof text_funcs / sizeof text_funcs[0] &&
        t < sizeof GUI_funcs / sizeof GUI_funcs[0] &&
        t >= 0) ret = funcs[t](ap);
    else report (R_WARNING, "unimplemented report type: %d", t);
    va_end (ap);
    return ret;
}
