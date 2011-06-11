/*
 * explorer.exe
 *
 * Copyright 2004 CodeWeavers, Mike Hearn
 * Copyright 2005,2006 CodeWeavers, Aric Stewart
 * Copyright 2011 Jay Yang
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

#define COBJMACROS

#include "wine/unicode.h"
#include "wine/debug.h"
#include "explorer_private.h"
#include "resource.h"

#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>


WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define EXPLORER_INFO_INDEX 0

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480


static const WCHAR EXPLORER_CLASS[] = {'W','I','N','E','_','E','X','P','L','O','R','E','R','\0'};

HINSTANCE explorer_hInstance;

typedef struct parametersTAG {
    BOOL    explorer_mode;
    WCHAR   root[MAX_PATH];
    WCHAR   selection[MAX_PATH];
} parameters_struct;

typedef struct
{
    IExplorerBrowser *browser;
} explorer_info;

static void make_explorer_window(IShellFolder* startFolder)
{
    RECT explorerRect;
    HWND window;
    FOLDERSETTINGS fs;
    explorer_info *info;
    HRESULT hres;
    WCHAR explorer_title[100];
    LoadStringW(explorer_hInstance,IDS_EXPLORER_TITLE,explorer_title,
                sizeof(explorer_title)/sizeof(WCHAR));
    info = HeapAlloc(GetProcessHeap(),0,sizeof(explorer_info));
    if(!info)
    {
        WINE_ERR("Could not allocate a explorer_info struct\n");
        return;
    }
    hres = CoCreateInstance(&CLSID_ExplorerBrowser,NULL,CLSCTX_INPROC_SERVER,
                            &IID_IExplorerBrowser,(LPVOID*)&info->browser);
    if(!SUCCEEDED(hres))
    {
        WINE_ERR("Could not obtain an instance of IExplorerBrowser\n");
        HeapFree(GetProcessHeap(),0,info);
        return;
    }

    window = CreateWindowW(EXPLORER_CLASS,explorer_title,WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,CW_USEDEFAULT,DEFAULT_WIDTH,
                           DEFAULT_HEIGHT,NULL,NULL,explorer_hInstance,NULL);
    fs.ViewMode = FVM_DETAILS;
    fs.fFlags = FWF_AUTOARRANGE;
    explorerRect.left = 0;
    explorerRect.top = 0;
    explorerRect.right = DEFAULT_WIDTH;
    explorerRect.bottom = DEFAULT_HEIGHT;

    IExplorerBrowser_Initialize(info->browser,window,&explorerRect,&fs);
    IExplorerBrowser_SetOptions(info->browser,EBO_SHOWFRAMES);
    SetWindowLongPtrW(window,EXPLORER_INFO_INDEX,(LONG_PTR)info);
    IExplorerBrowser_BrowseToObject(info->browser,(IUnknown*)startFolder,
                                    SBSP_ABSOLUTE);
    ShowWindow(window,SW_SHOWDEFAULT);
    UpdateWindow(window);
}

static void update_window_size(explorer_info *info, int height, int width)
{
    RECT new_rect;
    new_rect.left = 0;
    new_rect.top = 0;
    new_rect.right = width;
    new_rect.bottom = height;
    IExplorerBrowser_SetRect(info->browser,NULL,new_rect);
}

static void do_exit(int code)
{
    OleUninitialize();
    ExitProcess(code);
}

LRESULT CALLBACK explorer_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    explorer_info *info
        = (explorer_info*)GetWindowLongPtrW(hwnd,EXPLORER_INFO_INDEX);
    IExplorerBrowser *browser = NULL;

    if(info)
        browser = info->browser;
    switch(uMsg)
    {
    case WM_DESTROY:
        IExplorerBrowser_Release(browser);
        HeapFree(GetProcessHeap(),0,info);
        PostQuitMessage(0);
        break;
    case WM_QUIT:
        do_exit(wParam);
    case WM_SIZE:
        update_window_size(info,HIWORD(lParam),LOWORD(lParam));
        break;
    default:
        return DefWindowProcW(hwnd,uMsg,wParam,lParam);
    }
    return 0;
}

static void register_explorer_window_class(void)
{
    WNDCLASSEXW window_class;
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = 0;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = sizeof(LONG_PTR);
    window_class.lpfnWndProc = explorer_wnd_proc;
    window_class.hInstance = explorer_hInstance;
    window_class.hIcon = NULL;
    window_class.hCursor = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = EXPLORER_CLASS;
    window_class.hIconSm = NULL;
    RegisterClassExW(&window_class);
}

static IShellFolder* get_starting_shell_folder(parameters_struct* params)
{
    IShellFolder* desktop,*folder;
    LPITEMIDLIST root_pidl;
    HRESULT hres;

    SHGetDesktopFolder(&desktop);
    if(!params->root || (strlenW(params->root)==0))
    {
        return desktop;
    }
    hres = IShellFolder_ParseDisplayName(desktop,NULL,NULL,
                                         params->root,NULL,
                                         &root_pidl,NULL);

    if(FAILED(hres))
    {
        return desktop;
    }
    hres = IShellFolder_BindToObject(desktop,root_pidl,NULL,
                                     &IID_IShellFolder,
                                     (void**)&folder);
    if(FAILED(hres))
    {
        return desktop;
    }
    IShellFolder_Release(desktop);
    return folder;
}

static int copy_path_string(LPWSTR target, LPWSTR source)
{
    INT i = 0;

    while (isspaceW(*source)) source++;

    if (*source == '\"')
    {
        source ++;
        while (*source != '\"') target[i++] = *source++;
        target[i] = 0;
        source ++;
        i+=2;
    }
    else
    {
        while (*source && !isspaceW(*source)) target[i++] = *source++;
        target[i] = 0;
    }
    return i;
}


static void copy_path_root(LPWSTR root, LPWSTR path)
{
    LPWSTR p,p2;
    INT i = 0;

    p = path;
    while (*p!=0)
        p++;

    while (*p!='\\' && p > path)
        p--;

    if (p == path)
        return;

    p2 = path;
    while (p2 != p)
    {
        root[i] = *p2;
        i++;
        p2++;
    }
    root[i] = 0;
}

/*
 * Command Line parameters are:
 * [/n]  Opens in single-paned view for each selected items. This is default
 * [/e,] Uses Windows Explorer View
 * [/root,object] Specifies the root level of the view
 * [/select,object] parent folder is opened and specified object is selected
 */
static void parse_command_line(LPWSTR commandline,parameters_struct *parameters)
{
    static const WCHAR arg_n[] = {'/','n'};
    static const WCHAR arg_e[] = {'/','e',','};
    static const WCHAR arg_root[] = {'/','r','o','o','t',','};
    static const WCHAR arg_select[] = {'/','s','e','l','e','c','t',','};
    static const WCHAR arg_desktop[] = {'/','d','e','s','k','t','o','p'};

    LPWSTR p, p2;

    p2 = commandline;
    p = strchrW(commandline,'/');
    while(p)
    {
        if (strncmpW(p, arg_n, sizeof(arg_n)/sizeof(WCHAR))==0)
        {
            parameters->explorer_mode = FALSE;
            p += sizeof(arg_n)/sizeof(WCHAR);
        }
        else if (strncmpW(p, arg_e, sizeof(arg_e)/sizeof(WCHAR))==0)
        {
            parameters->explorer_mode = TRUE;
            p += sizeof(arg_e)/sizeof(WCHAR);
        }
        else if (strncmpW(p, arg_root, sizeof(arg_root)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_root)/sizeof(WCHAR);
            p+=copy_path_string(parameters->root,p);
        }
        else if (strncmpW(p, arg_select, sizeof(arg_select)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_select)/sizeof(WCHAR);
            p+=copy_path_string(parameters->selection,p);
            if (!parameters->root[0])
                copy_path_root(parameters->root,
                               parameters->selection);
        }
        else if (strncmpW(p, arg_desktop, sizeof(arg_desktop)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_desktop)/sizeof(WCHAR);
            manage_desktop( p );  /* the rest of the command line is handled by desktop mode */
        }
        else p++;

        p2 = p;
        p = strchrW(p,'/');
    }
    if (p2 && *p2)
    {
        /* left over command line is generally the path to be opened */
        copy_path_string(parameters->root,p2);
    }
}

int WINAPI wWinMain(HINSTANCE hinstance,
                    HINSTANCE previnstance,
                    LPWSTR cmdline,
                    int cmdshow)
{

    parameters_struct   parameters;
    HRESULT hres;
    MSG msg;
    IShellFolder *folder;

    memset(&parameters,0,sizeof(parameters));
    explorer_hInstance = hinstance;
    parse_command_line(cmdline,&parameters);
    hres = OleInitialize(NULL);
    if(!SUCCEEDED(hres))
    {
        WINE_ERR("Could not initialize COM\n");
        ExitProcess(EXIT_FAILURE);
    }
    register_explorer_window_class();
    folder = get_starting_shell_folder(&parameters);
    make_explorer_window(folder);
    IShellFolder_Release(folder);
    while(GetMessageW( &msg, NULL, 0, 0 ) != 0)
    {
	    TranslateMessage(&msg);
	    DispatchMessageW(&msg);
    }
    return 0;
}
