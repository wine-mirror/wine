/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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
#include "winnls.h"
#include "wingdi.h"
#include "msi.h"
#include "msipriv.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


const WCHAR szMsiDialogClass[] = {
    'M','s','i','D','i','a','l','o','g','C','l','o','s','e','C','l','a','s','s',0
};

struct tag_dialog_info
{
    MSIPACKAGE *package;
    msi_dialog_event_handler event_handler;
    INT scale;
    HWND hwnd;
    WCHAR name[1];
};

typedef UINT (*msi_dialog_control_func)( dialog_info *dialog, MSIRECORD *rec );
struct control_handler 
{
    LPCWSTR control_type;
    msi_dialog_control_func func;
};

static UINT msi_dialog_text_control( dialog_info *dialog, MSIRECORD *rec )
{
    DWORD x, y, width, height;
    LPCWSTR text;
    const static WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };
    HWND hwnd;

    TRACE("%p %p\n", dialog, rec);

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );
    text = MSI_RecordGetString( rec, 10 );

    x = (dialog->scale * x) / 10;
    y = (dialog->scale * y) / 10;
    width = (dialog->scale * width) / 10;
    height = (dialog->scale * height) / 10;

    hwnd = CreateWindowW( szStatic, text, WS_CHILD | WS_VISIBLE |WS_GROUP,
                          x, y, width, height, dialog->hwnd, NULL, NULL, NULL );
    if (!hwnd)
        ERR("Failed to create hwnd\n");

    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_control( dialog_info *dialog, MSIRECORD *rec )
{
    const static WCHAR szButton[] = { 'B','U','T','T','O','N', 0 };
    DWORD x, y, width, height;
    LPCWSTR text;
    HWND hwnd;

    TRACE("%p %p\n", dialog, rec);

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );
    text = MSI_RecordGetString( rec, 10 );

    x = (dialog->scale * x) / 10;
    y = (dialog->scale * y) / 10;
    width = (dialog->scale * width) / 10;
    height = (dialog->scale * height) / 10;

    hwnd = CreateWindowW( szButton, text, WS_CHILD | WS_VISIBLE |WS_GROUP,
                          x, y, width, height, dialog->hwnd, NULL, NULL, NULL );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_line_control( dialog_info *dialog, MSIRECORD *rec )
{
    DWORD x, y, width, height;
    LPCWSTR text;
    const static WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };
    HWND hwnd;

    TRACE("%p %p\n", dialog, rec);

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );
    text = MSI_RecordGetString( rec, 10 );

    x = (dialog->scale * x) / 10;
    y = (dialog->scale * y) / 10;
    width = (dialog->scale * width) / 10;
    height = (dialog->scale * height) / 10;

    hwnd = CreateWindowW( szStatic, text, WS_CHILD | WS_VISIBLE |WS_GROUP |
                          SS_ETCHEDHORZ |SS_SUNKEN,
                          x, y, width, height, dialog->hwnd, NULL, NULL, NULL );
    if (!hwnd)
        ERR("Failed to create hwnd\n");

    return ERROR_SUCCESS;
}

#if 0
static UINT msi_load_picture( MSIDATABASE *db, LPCWSTR name, HBITMAP *hbm )
{
    IPicture *pic = NULL;
    IPersistFile *pf = NULL;
    IStream *stm = NULL;
    HRESULT r;

    r = CoCreateObject( &CLSID_Picture, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IPicture, (LPVOID*)&pic );
    if (FAILED(r))
        return ERROR_FUNCTION_FAILED;

    r = IPicture_QueryInterface( pic, &IID_IPersistFile, (LPVOID*) &pf );
    if (SUCCEEDED(r) )
    {
        r = IPersistFile_Load( pf, stm );
        IPersistFile_Release( pf );
    }

    if (SUCCEEDED(r) )
        IPicture_get_Handle( pic, hbm );
    IPicture_Release( pic );

    return r;
}
#endif

static UINT msi_dialog_bitmap_control( dialog_info *dialog, MSIRECORD *rec )
{
    DWORD x, y, width, height;
    LPCWSTR text;
    const static WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };
    HWND hwnd;

    TRACE("%p %p\n", dialog, rec);

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );
    text = MSI_RecordGetString( rec, 10 );

    x = (dialog->scale * x) / 10;
    y = (dialog->scale * y) / 10;
    width = (dialog->scale * width) / 10;
    height = (dialog->scale * height) / 10;

    hwnd = CreateWindowW( szStatic, text, WS_CHILD | WS_VISIBLE |WS_GROUP | WS_DISABLED |
                          SS_BITMAP | SS_LEFT | SS_CENTERIMAGE,
                          x, y, width, height, dialog->hwnd, NULL, NULL, NULL );
    if (!hwnd)
        ERR("Failed to create hwnd\n");

    return ERROR_SUCCESS;
}

static const WCHAR szText[] = { 'T','e','x','t',0 };
static const WCHAR szButton[] = { 'P','u','s','h','B','u','t','t','o','n',0 };
static const WCHAR szLine[] = { 'L','i','n','e',0 };
static const WCHAR szBitmap[] = { 'B','i','t','m','a','p',0 };

struct control_handler msi_dialog_handler[] =
{
    { szText, msi_dialog_text_control },
    { szButton, msi_dialog_button_control },
    { szLine, msi_dialog_line_control },
    { szBitmap, msi_dialog_bitmap_control },
};

#define NUM_CONTROL_TYPES (sizeof msi_dialog_handler/sizeof msi_dialog_handler[0])

typedef UINT (*record_func)( MSIRECORD *rec, LPVOID param );

static UINT msi_iterate_records( MSIQUERY *view, record_func func, LPVOID param )
{
    MSIRECORD *rec = NULL;
    UINT r;

    r = MSI_ViewExecute( view, NULL );
    if( r != ERROR_SUCCESS )
        return r;

    /* iterate a query */
    while( 1 )
    {
        r = MSI_ViewFetch( view, &rec );
        if( r != ERROR_SUCCESS )
            break;
        r = func( rec, param );
        msiobj_release( &rec->hdr );
        if( r != ERROR_SUCCESS )
            break;
    }

    MSI_ViewClose( view );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_create_controls( MSIRECORD *rec, LPVOID param )
{
    dialog_info *dialog = param;
    LPCWSTR control_type;
    UINT i;

    /* find and call the function that can create this type of control */
    control_type = MSI_RecordGetString( rec, 3 );
    for( i=0; i<NUM_CONTROL_TYPES; i++ )
        if (!strcmpiW( msi_dialog_handler[i].control_type, control_type ))
            break;
    if( i != NUM_CONTROL_TYPES )
        msi_dialog_handler[i].func( dialog, rec );
    else
        ERR("no handler for element type %s\n", debugstr_w(control_type));

    return ERROR_SUCCESS;
}

static UINT msi_dialog_fill_controls( dialog_info *dialog, LPCWSTR name )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','C','o','n','t','r','o','l',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(name));
        return ERROR_INVALID_PARAMETER;
    }

    r = msi_iterate_records( view, msi_dialog_create_controls, dialog );
    msiobj_release( &view->hdr );

    return r;
}

/* figure out the height of 10 point MS Sans Serif */
static INT msi_dialog_get_sans_serif_height( HWND hwnd )
{
    static const WCHAR szSansSerif[] = {
        'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0 };
    LOGFONTW lf;
    TEXTMETRICW tm;
    BOOL r;
    LONG height = 0;
    HFONT hFont, hOldFont;
    HDC hdc;

    hdc = GetDC( hwnd );
    if (hdc)
    {
        memset( &lf, 0, sizeof lf );
        lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        strcpyW( lf.lfFaceName, szSansSerif );
        hFont = CreateFontIndirectW(&lf);
        if (hFont)
        {
            hOldFont = SelectObject( hdc, hFont );
            r = GetTextMetricsW( hdc, &tm );
            if (r)
                height = tm.tmHeight;
            SelectObject( hdc, hOldFont );
            DeleteObject( hFont );
        }
        ReleaseDC( hwnd, hdc );
    }
    return height;
}

static LRESULT msi_dialog_oncreate( HWND hwnd, LPCREATESTRUCTW cs )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','D','i','a','l','o','g',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','`',' ','=',' ','\'','%','s','\'',0};
    dialog_info *dialog = (dialog_info*) cs->lpCreateParams;
    MSIPACKAGE *package = dialog->package;
    MSIQUERY *view = NULL;
    MSIRECORD *rec = NULL;
    DWORD width, height;
    LPCWSTR title;
    UINT r;

    TRACE("%p %p\n", dialog, package);

    dialog->hwnd = hwnd;
    SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR) dialog );

    /* fetch the associated record from the Dialog table */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return 1;
    }
    MSI_ViewExecute( view, NULL );
    MSI_ViewFetch( view, &rec );
    MSI_ViewClose( view );
    msiobj_release( &view->hdr );

    if( !rec )
    {
        ERR("No record found for dialog %s\n", debugstr_w(dialog->name));
        return 1;
    }

    dialog->scale = msi_dialog_get_sans_serif_height(dialog->hwnd);

    width = MSI_RecordGetInteger( rec, 4 );
    height = MSI_RecordGetInteger( rec, 5 );
    title = MSI_RecordGetString( rec, 7 );

    width = (dialog->scale * width) / 10;
    height = (dialog->scale * height) / 10;

    SetWindowTextW( hwnd, title );
    SetWindowPos( hwnd, 0, 0, 0, width, height,
                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW );

    msiobj_release( &rec->hdr );

    msi_dialog_fill_controls( dialog, dialog->name );

    return 0;
}

static LRESULT msi_dialog_handle_click( dialog_info *dialog,
                                     DWORD id, HWND handle )
{
    TRACE("BN_CLICKED %08lx %p\n", id, handle);

    return 0;
}

static LRESULT WINAPI MSIDialog_WndProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    dialog_info *dialog = (LPVOID) GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    switch (msg)
    {
    case WM_CREATE:
        return msi_dialog_oncreate( hwnd, (LPCREATESTRUCTW)lParam );

    case WM_COMMAND:
        if( HIWORD(wParam) == BN_CLICKED )
            return msi_dialog_handle_click( dialog, LOWORD(wParam), (HWND)lParam );
        break;

    case WM_DESTROY:
        dialog->hwnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* functions that interface to other modules within MSI */

dialog_info *msi_dialog_create( MSIPACKAGE* package, LPCWSTR szDialogName,
                                msi_dialog_event_handler event_handler )
{
    dialog_info *dialog;
    HWND hwnd;

    TRACE("%p %s\n", package, debugstr_w(szDialogName));

    /* allocate the structure for the dialog to use */
    dialog = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof *dialog + sizeof(WCHAR)*strlenW(szDialogName) );
    if( !dialog )
        return NULL;
    strcpyW( dialog->name, szDialogName );
    dialog->package = package;
    dialog->event_handler = event_handler;
    msiobj_addref( &package->hdr );

    /* create and show the dialog window */
    hwnd = CreateWindowW( szMsiDialogClass, szDialogName, WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     NULL, NULL, NULL, dialog );
    if( !hwnd )
    {
        msi_dialog_destroy( dialog );
        return NULL;
    }
    ShowWindow( hwnd, SW_SHOW );
    UpdateWindow( hwnd );

    return dialog;
}

void msi_dialog_destroy( dialog_info *dialog )
{
    if( dialog->hwnd )
        DestroyWindow( dialog->hwnd );
    msiobj_release( &dialog->package->hdr );
    dialog->package = NULL;
    HeapFree( GetProcessHeap(), 0, dialog );
}

void msi_dialog_register_class( void )
{
    WNDCLASSW cls;

    ZeroMemory( &cls, sizeof cls );
    cls.lpfnWndProc   = MSIDialog_WndProc;
    cls.hInstance     = NULL;
    cls.hIcon         = LoadIconW(0, (LPWSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    cls.lpszMenuName  = NULL;
    cls.lpszClassName = szMsiDialogClass;

    RegisterClassW( &cls );
}

void msi_dialog_unregister_class( void )
{
    UnregisterClassW( szMsiDialogClass, NULL );
}
