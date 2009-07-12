/*
 * Unit test suite for comdlg32 API functions: file dialogs
 *
 * Copyright 2007 Google (Lei Zhang)
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
 *
 */

#include <windows.h>
#include <wine/test.h>

#include "initguid.h"
#include "shlguid.h"
#define COBJMACROS
#include "shobjidl.h"

/* ##### */

static UINT_PTR CALLBACK OFNHookProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR nmh;

    if( msg == WM_NOTIFY)
    {
        nmh = (LPNMHDR) lParam;
        if( nmh->code == CDN_INITDONE)
        {
            PostMessage( GetParent(hDlg), WM_COMMAND, IDCANCEL, FALSE);
        } else if (nmh->code == CDN_FOLDERCHANGE )
        {
            char buf[1024];
            int ret;

            memset(buf, 0x66, sizeof(buf));
            ret = SendMessage( GetParent(hDlg), CDM_GETFOLDERIDLIST, 5, (LPARAM)buf);
            ok(ret > 0, "CMD_GETFOLDERIDLIST not implemented\n");
            if (ret > 5)
                ok(buf[0] == 0x66 && buf[1] == 0x66, "CMD_GETFOLDERIDLIST: The buffer was touched on failure\n");
        }
    }

    return 0;
}

/* bug 6829 */
static void test_DialogCancel(void)
{
    OPENFILENAMEA ofn;
    BOOL result;
    char szFileName[MAX_PATH] = "";
    char szInitialDir[MAX_PATH];

    GetWindowsDirectory(szInitialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = "txt";
    ofn.lpfnHook = OFNHookProc;
    ofn.lpstrInitialDir = szInitialDir;

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
       CDERR_INITIALIZATION, CommDlgExtendedError());

    result = GetOpenFileNameA(&ofn);
    ok(0 == result, "expected %d, got %d\n", 0, result);
    ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    result = GetSaveFileNameA(&ofn);
    ok(0 == result, "expected %d, got %d\n", 0, result);
    ok(0 == CommDlgExtendedError(), "expected %d, got %d\n", 0,
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    /* Before passing the ofn to Unicode functions, remove the ANSI strings */
    ofn.lpstrFilter = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = NULL;

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(), "expected %d, got %d\n",
              CDERR_INITIALIZATION, CommDlgExtendedError());

    SetLastError(0xdeadbeef);
    result = GetOpenFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetOpenFileNameW is not implemented\n");
    else
    {
        ok(0 == result, "expected %d, got %d\n", 0, result);
        ok(0 == CommDlgExtendedError() ||
           CDERR_INITIALIZATION == CommDlgExtendedError(), /* win9x */
           "expected %d or %d, got %d\n", 0, CDERR_INITIALIZATION,
           CommDlgExtendedError());
    }

    SetLastError(0xdeadbeef);
    result = GetSaveFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetSaveFileNameW is not implemented\n");
    else
    {
        ok(0 == result, "expected %d, got %d\n", 0, result);
        ok(0 == CommDlgExtendedError() ||
           CDERR_INITIALIZATION == CommDlgExtendedError(), /* win9x */
           "expected %d or %d, got %d\n", 0, CDERR_INITIALIZATION,
           CommDlgExtendedError());
    }
}

static UINT_PTR CALLBACK create_view_window2_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NOTIFY)
    {
        if (((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE)
        {
            IShellBrowser *shell_browser = (IShellBrowser *)SendMessage(GetParent(dlg), WM_USER + 7 /* WM_GETISHELLBROWSER */, 0, 0);
            IShellView *shell_view = NULL;
            IShellView2 *shell_view2 = NULL;
            SV2CVW2_PARAMS view_params;
            FOLDERSETTINGS folder_settings;
            HRESULT hr;
            RECT rect = {0, 0, 0, 0};

            hr = IShellBrowser_QueryActiveShellView(shell_browser, &shell_view);
            ok(SUCCEEDED(hr), "QueryActiveShellView returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView_QueryInterface(shell_view, &IID_IShellView2, (void **)&shell_view2);
            if (hr == E_NOINTERFACE)
            {
                win_skip("IShellView2 not supported\n");
                goto cleanup;
            }
            ok(SUCCEEDED(hr), "QueryInterface returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_DestroyViewWindow(shell_view2);
            ok(SUCCEEDED(hr), "DestroyViewWindow returned %#x\n", hr);

            folder_settings.ViewMode = FVM_LIST;
            folder_settings.fFlags = 0;

            view_params.cbSize = sizeof(view_params);
            view_params.psvPrev = NULL;
            view_params.pfs = &folder_settings;
            view_params.psbOwner = shell_browser;
            view_params.prcView = &rect;
            view_params.pvid = NULL;
            view_params.hwndView = NULL;

            hr = IShellView2_CreateViewWindow2(shell_view2, &view_params);
            ok(SUCCEEDED(hr), "CreateViewWindow2 returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_GetCurrentInfo(shell_view2, &folder_settings);
            ok(SUCCEEDED(hr), "GetCurrentInfo returned %#x\n", hr);
            ok(folder_settings.ViewMode == FVM_LIST, "view mode is %d, expected %d\n", folder_settings.ViewMode, FVM_LIST);

            hr = IShellView2_DestroyViewWindow(shell_view2);
            ok(SUCCEEDED(hr), "DestroyViewWindow returned %#x\n", hr);

            /* XP and W2K3 need this. On Win9x and W2K the call to DestroyWindow() fails and has
             * no side effects. NT4 doesn't get here. (FIXME: Vista doesn't get here yet).
             */
            DestroyWindow(view_params.hwndView);

            view_params.pvid = &VID_Details;
            hr = IShellView2_CreateViewWindow2(shell_view2, &view_params);
            ok(SUCCEEDED(hr), "CreateViewWindow2 returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_GetCurrentInfo(shell_view2, &folder_settings);
            ok(SUCCEEDED(hr), "GetCurrentInfo returned %#x\n", hr);
            ok(folder_settings.ViewMode == FVM_DETAILS ||
               broken(folder_settings.ViewMode == FVM_LIST), /* Win9x */
               "view mode is %d, expected %d\n", folder_settings.ViewMode, FVM_DETAILS);

cleanup:
            if (shell_view2) IShellView2_Release(shell_view2);
            if (shell_view) IShellView_Release(shell_view);
            PostMessage(GetParent(dlg), WM_COMMAND, IDCANCEL, 0);
        }
    }
    return 0;
}

static LONG_PTR WINAPI template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        HWND p,cb;
        INT sel;
        p = GetParent(dlg);
        ok(p!=NULL, "Failed to get parent of template\n");
        cb = GetDlgItem(p,0x470);
        ok(cb!=NULL, "Failed to get filter combobox\n");
        sel = SendMessage(cb, CB_GETCURSEL, 0, 0);
        ok (sel != -1, "Failed to get selection from filter listbox\n");
    }
    if (msg == WM_NOTIFY)
    {
        if (((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE)
            PostMessage(GetParent(dlg), WM_COMMAND, IDCANCEL, 0);
    }
    return 0;
}

static void test_create_view_window2(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = {0};
    DWORD ret;

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = create_view_window2_hook;
    ofn.Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ret = GetOpenFileNameA(&ofn);
    ok(!ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
}

static void test_create_view_template(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = {0};
    DWORD ret;

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = (LPOFNHOOKPROC)template_hook;
    ofn.Flags = OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE;
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.lpTemplateName = "template1";
    ofn.lpstrFilter="text\0*.txt\0All\0*\0\0";
    ret = GetOpenFileNameA(&ofn);
    ok(!ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
}

/* test cases for resizing of the file dialog */
struct {
    DWORD flags;
    int resize_folderchange;/* change in CDN_FOLDERCHANGE handler */
    int resize_timer1;      /* change in first WM_TIMER handler */
    int resize_check;       /* expected change (in second  WM_TIMER handler) */
    BOOL todo;              /* mark that test todo_wine */
    BOOL testcontrols;      /* test resizing and moving of the controls */
} resize_testcases[] = {
    { 0                , 10, 10, 20,FALSE,FALSE},   /* 0 */
    { 0                ,-10,-10,-20,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0,  0,  0,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0,-10,  0,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0, 10, 10,FALSE, TRUE},
    { OFN_ENABLESIZING ,-10,  0, 10,FALSE,FALSE},   /* 5 */
    { OFN_ENABLESIZING , 10,  0, 10,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0, 10, 20,FALSE,FALSE},
    /* mark the end */
    { 0xffffffff }
};

static LONG_PTR WINAPI resize_template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static RECT initrc, rc;
    static int index, count;
    static int gotSWP_bottom, gotShowWindow;
    HWND parent = GetParent( dlg);
    int resize;
#define MAXNRCTRLS 30
    static RECT ctrlrcs[MAXNRCTRLS];
    static int ctrlids[MAXNRCTRLS];
    static HWND ctrls[MAXNRCTRLS];
    static int nrctrls;

    switch( msg)
    {
        case WM_INITDIALOG:
        {
            DWORD style;

            index = ((OPENFILENAME*)lParam)->lCustData;
            count = 0;
            gotSWP_bottom = gotShowWindow = 0;
            /* test style */
            style = GetWindowLong( parent, GWL_STYLE);
            if( resize_testcases[index].flags & OFN_ENABLESIZING)
                if( !(style & WS_SIZEBOX)) {
                    win_skip( "OFN_ENABLESIZING flag not supported.\n");
                    PostMessage( parent, WM_COMMAND, IDCANCEL, 0);
                } else
                    ok( style & WS_SIZEBOX,
                            "testid %d: dialog should have a WS_SIZEBOX style.\n", index);
            else
                ok( !(style & WS_SIZEBOX),
                        "testid %d: dialog should not have a WS_SIZEBOX style.\n", index);
            break;
        }
        case WM_NOTIFY:
        {
            if(( (LPNMHDR)lParam)->code == CDN_FOLDERCHANGE){
                GetWindowRect( parent, &initrc);
                if( (resize  = resize_testcases[index].resize_folderchange)){
                    MoveWindow( parent, initrc.left,initrc.top, initrc.right - initrc.left + resize,
                            initrc.bottom - initrc.top + resize, TRUE);
                }
                SetTimer( dlg, 0, 100, 0);
            }
            break;
        }
        case WM_TIMER:
        {
            if( count == 0){
                /* store the control rectangles */
                if( resize_testcases[index].testcontrols) {
                    HWND ctrl;
                    int i;
                    for( i = 0, ctrl = GetWindow( parent, GW_CHILD);
                            i < MAXNRCTRLS && ctrl;
                            i++, ctrl = GetWindow( ctrl, GW_HWNDNEXT)) {
                        ctrlids[i] = GetDlgCtrlID( ctrl);
                        GetWindowRect( ctrl, &ctrlrcs[i]);
                        MapWindowPoints( NULL, parent, (LPPOINT) &ctrlrcs[i], 2);
                        ctrls[i] = ctrl;
                    }
                    nrctrls = i;
                }
                if( (resize  = resize_testcases[index].resize_timer1)){
                    GetWindowRect( parent, &rc);
                    MoveWindow( parent, rc.left,rc.top, rc.right - rc.left + resize,
                            rc.bottom - rc.top + resize, TRUE);
                }
            } else if( count == 1){
                resize  = resize_testcases[index].resize_check;
                GetWindowRect( parent, &rc);
                if( resize_testcases[index].todo){
                    todo_wine {
                        ok( resize == rc.right - rc.left - initrc.right + initrc.left,
                            "testid %d size-x change %d expected %d\n", index,
                            rc.right - rc.left - initrc.right + initrc.left, resize);
                        ok( resize == rc.bottom - rc.top - initrc.bottom + initrc.top,
                            "testid %d size-y change %d expected %d\n", index,
                            rc.bottom - rc.top - initrc.bottom + initrc.top, resize);
                    }
                }else{
                    ok( resize == rc.right - rc.left - initrc.right + initrc.left,
                        "testid %d size-x change %d expected %d\n", index,
                        rc.right - rc.left - initrc.right + initrc.left, resize);
                    ok( resize == rc.bottom - rc.top - initrc.bottom + initrc.top,
                        "testid %d size-y change %d expected %d\n", index,
                        rc.bottom - rc.top - initrc.bottom + initrc.top, resize);
                }
                if( resize_testcases[index].testcontrols) {
                    int i;
                    RECT rc;
                    for( i = 0; i < nrctrls; i++) {
                        GetWindowRect( ctrls[i], &rc);
                        MapWindowPoints( NULL, parent, (LPPOINT) &rc, 2);
                        switch( ctrlids[i]){

/* test if RECT R1, moved and sized result in R2 */
#define TESTRECTS( R1, R2, Mx, My, Sx, Sy) \
         ((R1).left + (Mx) ==(R2).left \
        &&(R1).top + (My) ==(R2).top \
        &&(R1).right + (Mx) + (Sx) == (R2).right \
        &&(R1).bottom + (My) + (Sy) ==(R2).bottom)

                            /* sized horizontal and moved vertical */
                            case cmb1:
                            case edt1:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 10, 10, 0) ||
                                    broken(TESTRECTS( ctrlrcs[i], rc, 0, 10, 0, 0)),/*win98*/
                                    "control id %03x should have sized horizontally and moved vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* sized horizontal and vertical */
                            case lst2:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 0, 10, 10),
                                    "control id %03x should have sized horizontally and vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* moved horizontal and vertical */
                            case IDCANCEL:
                            case pshHelp:
                                ok( TESTRECTS( ctrlrcs[i], rc, 10, 10, 0, 0) ||
                                    broken(TESTRECTS( ctrlrcs[i], rc, 0, 10, 0, 0)),/*win98*/
                                    "control id %03x should have moved horizontally and vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* moved vertically */
                            case chx1:
                            case stc2:
                            case stc3:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 10, 0, 0),
                                    "control id %03x should have moved vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* resized horizontal */
                            case cmb2: /* aka IDC_LOOKIN */
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 0, 10, 0)||
                                        TESTRECTS( ctrlrcs[i], rc, 0, 0, 0, 0), /* Vista and higher */
                                    "control id %03x should have resized horizontally, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* non moving non sizing controls */
                            case stc4:
                                ok( TESTRECTS( rc, ctrlrcs[i], 0, 0, 0, 0),
                                    "control id %03x was moved/resized, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* todo_wine: non moving non sizing controls */
                            case lst1:
todo_wine
                                ok( TESTRECTS( rc, ctrlrcs[i], 0, 0, 0, 0),
                                    "control id %03x was moved/resized, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* don't test: id is not unique */
                            case IDOK:
                            case stc1:
                            case 0:
                            case  -1:
                                break;
                            default:
                                trace("untested control id %03x before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
#undef TESTRECTS
#undef MAXNRCTRLS
                        }
                    }
                }
                KillTimer( dlg, 0);
                PostMessage( parent, WM_COMMAND, IDCANCEL, 0);
            }
            count++;
        }
        break;
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            if(  !index && pwp->hwndInsertAfter == HWND_BOTTOM){
                gotSWP_bottom = 1;
                ok( gotShowWindow == 0, "The WM_WINDOWPOSCHANGING message came after a WM_SHOWWINDOW message\n");
            }
        }
        break;
        case WM_SHOWWINDOW:
        {
            if(  !index){
                gotShowWindow = 1;
                ok( gotSWP_bottom == 1, "No WM_WINDOWPOSCHANGING message came before a WM_SHOWWINDOW message\n");
            }
        }
        break;
    }
    return 0;
}

static void test_resize(void)
{
    OPENFILENAME ofn = { sizeof(OPENFILENAME)};
    char filename[1024] = {0};
    DWORD ret;
    int i;

    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = (LPOFNHOOKPROC) resize_template_hook;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpTemplateName = "template_sz";
    for( i = 0; resize_testcases[i].flags != 0xffffffff; i++) {
        ofn.lCustData = i;
        ofn.Flags = resize_testcases[i].flags |
            OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE | OFN_SHOWHELP ;
        ret = GetOpenFileName(&ofn);
        ok(!ret, "GetOpenFileName returned %#x\n", ret);
        ret = CommDlgExtendedError();
        ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    }
}

/* test cases for control message IDOK */
/* Show case for bug #19079 */
static struct {
    int  retval;        /* return code of the message handler */
    BOOL setmsgresult;  /* set the result in the DWLP_MSGRESULT */
    BOOL usemsgokstr;   /* use the FILEOKSTRING message instead of WM_NOTIFY:CDN_FILEOK */
    BOOL do_subclass;   /* subclass the dialog hook procedure */
    BOOL expclose;      /* is the dialog expected to close ? */
    BOOL actclose;      /* has the dialog actually closed ? */
} ok_testcases[] = {
    { 0,        FALSE,  FALSE,  FALSE,  TRUE},
    { 0,         TRUE,  FALSE,  FALSE,  TRUE},
    { 0,        FALSE,  FALSE,   TRUE,  TRUE},
    { 0,         TRUE,  FALSE,   TRUE,  TRUE},
    { 1,        FALSE,  FALSE,  FALSE,  TRUE},
    { 1,         TRUE,  FALSE,  FALSE, FALSE},
    { 1,        FALSE,  FALSE,   TRUE, FALSE},
    { 1,         TRUE,  FALSE,   TRUE, FALSE},
    /* FILEOKSTRING tests */
    { 1,         TRUE,   TRUE,  FALSE, FALSE},
    { 1,        FALSE,   TRUE,   TRUE, FALSE},
    /* mark the end */
    { -1 }
};

/* test_ok_wndproc can be used as hook procedure or a subclass
 * window proc for the file dialog */
static LONG_PTR WINAPI test_ok_wndproc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND parent = GetParent( dlg);
    static int index;
    static UINT msgFILEOKSTRING;
    if (msg == WM_INITDIALOG)
    {
        index = ((OPENFILENAME*)lParam)->lCustData;
        ok_testcases[index].actclose = TRUE;
        msgFILEOKSTRING = RegisterWindowMessageA( FILEOKSTRING);
    }
    if( msg == WM_NOTIFY) {
        if(((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE) {
            SetTimer( dlg, 0, 100, 0);
            PostMessage( parent, WM_COMMAND, IDOK, 0);
            return FALSE;
        } else if(((LPNMHDR)lParam)->code == CDN_FILEOK) {
            if( ok_testcases[index].usemsgokstr)
                return FALSE;
            if( ok_testcases[index].setmsgresult)
                SetWindowLongPtrA( dlg, DWLP_MSGRESULT, ok_testcases[index].retval);
            return ok_testcases[index].retval;
        }
    }
    if( msg == msgFILEOKSTRING) {
        if( !ok_testcases[index].usemsgokstr)
            return FALSE;
        if( ok_testcases[index].setmsgresult)
            SetWindowLongPtrA( dlg, DWLP_MSGRESULT, ok_testcases[index].retval);
        return ok_testcases[index].retval;
    }
    if( msg == WM_TIMER) {
        /* the dialog did not close automatically */
        ok_testcases[index].actclose = FALSE;
        KillTimer( dlg, 0);
        PostMessage( parent, WM_COMMAND, IDCANCEL, 0);
        return FALSE;
    }
    if( ok_testcases[index].do_subclass)
        return DefWindowProc( dlg, msg, wParam, lParam);
    return FALSE;
}

static LONG_PTR WINAPI ok_template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SETFONT)
        SetWindowLongPtrA( dlg, GWLP_WNDPROC, (LONG_PTR) test_ok_wndproc);
    return FALSE;
}

static void test_ok(void)
{
    OPENFILENAME ofn = { sizeof(OPENFILENAME)};
    char filename[1024] = {0};
    char tmpfilename[ MAX_PATH];
    int i;
    DWORD ret;

    if (!GetTempFileNameA(".", "txt", 0, tmpfilename)) {
        skip("Failed to create a temporary file name\n");
        return;
    }
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpTemplateName = "template1";
    ofn.Flags =  OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE ;
    for( i = 0; ok_testcases[i].retval != -1; i++) {
        strcpy( filename, tmpfilename);
        ofn.lCustData = i;
        ofn.lpfnHook = ok_testcases[i].do_subclass
            ? (LPOFNHOOKPROC) ok_template_hook
            : (LPOFNHOOKPROC) test_ok_wndproc;
        ret = GetOpenFileNameA(&ofn);
        ok( ok_testcases[i].expclose == ok_testcases[i].actclose,
                "testid %d: Open File dialog should %shave closed.\n", i,
                ok_testcases[i].expclose ? "" : "NOT ");
        ok(ret == ok_testcases[i].expclose, "testid %d: GetOpenFileName returned %#x\n", i, ret);
        ret = CommDlgExtendedError();
        ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    }
    ret =  DeleteFileA( tmpfilename);
    ok( ret, "Failed to delete temporary file %s err %d\n", tmpfilename, GetLastError());
}

START_TEST(filedlg)
{
    test_DialogCancel();
    test_create_view_window2();
    test_create_view_template();
    test_resize();
    test_ok();
}
