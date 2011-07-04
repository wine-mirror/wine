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
#include <commoncontrols.h>
#include <commctrl.h>

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define EXPLORER_INFO_INDEX 0

#define NAV_TOOLBAR_HEIGHT 30
#define PATHBOX_HEIGHT 24

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480


static const WCHAR EXPLORER_CLASS[] = {'W','I','N','E','_','E','X','P','L','O','R','E','R','\0'};
static const WCHAR PATH_BOX_NAME[] = {'\0'};

HINSTANCE explorer_hInstance;

typedef struct parametersTAG {
    BOOL    explorer_mode;
    WCHAR   root[MAX_PATH];
    WCHAR   selection[MAX_PATH];
} parameters_struct;

typedef struct
{
    IExplorerBrowser *browser;
    HWND main_window,path_box;
    INT rebar_height;
    LPCITEMIDLIST pidl;
} explorer_info;

enum
{
    BACK_BUTTON,FORWARD_BUTTON,UP_BUTTON
};

typedef struct
{
    IExplorerBrowserEvents IExplorerBrowserEvents_iface;
    explorer_info* info;
    LONG ref;
} IExplorerBrowserEventsImpl;

static IExplorerBrowserEventsImpl *impl_from_IExplorerBrowserEvents(IExplorerBrowserEvents *iface)
{
    return CONTAINING_RECORD(iface, IExplorerBrowserEventsImpl, IExplorerBrowserEvents_iface);
}

static HRESULT WINAPI IExplorerBrowserEventsImpl_fnQueryInterface(IExplorerBrowserEvents *iface, REFIID riid, void **ppvObject)
{
    return E_NOINTERFACE;
}

static ULONG WINAPI IExplorerBrowserEventsImpl_fnAddRef(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IExplorerBrowserEventsImpl_fnRelease(IExplorerBrowserEvents *iface)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if(!ref)
        HeapFree(GetProcessHeap(),0,This);
    return ref;
}

static void update_path_box(explorer_info *info)
{
    WCHAR path[MAX_PATH];
    SHGetPathFromIDListW(info->pidl,path);
    SetWindowTextW(info->path_box,path);
}

static HRESULT WINAPI IExplorerBrowserEventsImpl_fnOnNavigationComplete(IExplorerBrowserEvents *iface, PCIDLIST_ABSOLUTE pidl)
{
    IExplorerBrowserEventsImpl *This = impl_from_IExplorerBrowserEvents(iface);
    This->info->pidl = pidl;
    update_path_box(This->info);
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEventsImpl_fnOnNavigationFailed(IExplorerBrowserEvents *iface, PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEventsImpl_fnOnNavigationPending(IExplorerBrowserEvents *iface, PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEventsImpl_fnOnViewCreated(IExplorerBrowserEvents *iface, IShellView *psv)
{
    return S_OK;
}

static IExplorerBrowserEventsVtbl vt_IExplorerBrowserEvents =
{
    IExplorerBrowserEventsImpl_fnQueryInterface,
    IExplorerBrowserEventsImpl_fnAddRef,
    IExplorerBrowserEventsImpl_fnRelease,
    IExplorerBrowserEventsImpl_fnOnNavigationPending,
    IExplorerBrowserEventsImpl_fnOnViewCreated,
    IExplorerBrowserEventsImpl_fnOnNavigationComplete,
    IExplorerBrowserEventsImpl_fnOnNavigationFailed
};

static IExplorerBrowserEvents *make_explorer_events(explorer_info *info)
{
    IExplorerBrowserEventsImpl *ret
        = HeapAlloc(GetProcessHeap(), 0, sizeof(IExplorerBrowserEventsImpl));
    ret->IExplorerBrowserEvents_iface.lpVtbl = &vt_IExplorerBrowserEvents;
    ret->info = info;
    IExplorerBrowserEvents_AddRef(&ret->IExplorerBrowserEvents_iface);
    return &ret->IExplorerBrowserEvents_iface;
}

static void make_explorer_window(IShellFolder* startFolder)
{
    RECT explorerRect;
    HWND rebar,nav_toolbar;
    FOLDERSETTINGS fs;
    IExplorerBrowserEvents *events;
    explorer_info *info;
    HRESULT hres;
    DWORD cookie;
    WCHAR explorer_title[100];
    WCHAR pathbox_label[50];
    TBADDBITMAP bitmap_info;
    TBBUTTON nav_buttons[3];
    int hist_offset,view_offset;
    REBARBANDINFOW band_info;
    memset(nav_buttons,0,sizeof(nav_buttons));
    LoadStringW(explorer_hInstance,IDS_EXPLORER_TITLE,explorer_title,
                sizeof(explorer_title)/sizeof(WCHAR));
    LoadStringW(explorer_hInstance,IDS_PATHBOX_LABEL,pathbox_label,
                sizeof(pathbox_label)/sizeof(WCHAR));
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
    info->rebar_height=0;
    info->main_window
        = CreateWindowW(EXPLORER_CLASS,explorer_title,WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,CW_USEDEFAULT,DEFAULT_WIDTH,
                        DEFAULT_HEIGHT,NULL,NULL,explorer_hInstance,NULL);

    fs.ViewMode = FVM_DETAILS;
    fs.fFlags = FWF_AUTOARRANGE;
    explorerRect.left = 0;
    explorerRect.top = 0;
    explorerRect.right = DEFAULT_WIDTH;
    explorerRect.bottom = DEFAULT_HEIGHT;

    IExplorerBrowser_Initialize(info->browser,info->main_window,&explorerRect,&fs);
    IExplorerBrowser_SetOptions(info->browser,EBO_SHOWFRAMES);
    SetWindowLongPtrW(info->main_window,EXPLORER_INFO_INDEX,(LONG)info);

    /*setup navbar*/
    rebar = CreateWindowExW(WS_EX_TOOLWINDOW,REBARCLASSNAMEW,NULL,
                            WS_CHILD|WS_VISIBLE|RBS_VARHEIGHT|CCS_TOP|CCS_NODIVIDER,
                            0,0,0,0,info->main_window,NULL,explorer_hInstance,NULL);
    nav_toolbar
        = CreateWindowExW(TBSTYLE_EX_MIXEDBUTTONS,TOOLBARCLASSNAMEW,NULL,
                          WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT,0,0,0,0,rebar,NULL,
                          explorer_hInstance,NULL);

    bitmap_info.hInst = HINST_COMMCTRL;
    bitmap_info.nID = IDB_HIST_LARGE_COLOR;
    hist_offset= SendMessageW(nav_toolbar,TB_ADDBITMAP,0,(LPARAM)&bitmap_info);
    bitmap_info.nID = IDB_VIEW_LARGE_COLOR;
    view_offset= SendMessageW(nav_toolbar,TB_ADDBITMAP,0,(LPARAM)&bitmap_info);

    nav_buttons[0].iBitmap=hist_offset+HIST_BACK;
    nav_buttons[0].idCommand=BACK_BUTTON;
    nav_buttons[0].fsState=TBSTATE_ENABLED;
    nav_buttons[0].fsStyle=BTNS_BUTTON|BTNS_AUTOSIZE;
    nav_buttons[1].iBitmap=hist_offset+HIST_FORWARD;
    nav_buttons[1].idCommand=FORWARD_BUTTON;
    nav_buttons[1].fsState=TBSTATE_ENABLED;
    nav_buttons[1].fsStyle=BTNS_BUTTON|BTNS_AUTOSIZE;
    nav_buttons[2].iBitmap=view_offset+VIEW_PARENTFOLDER;
    nav_buttons[2].idCommand=UP_BUTTON;
    nav_buttons[2].fsState=TBSTATE_ENABLED;
    nav_buttons[2].fsStyle=BTNS_BUTTON|BTNS_AUTOSIZE;
    SendMessageW(nav_toolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
    SendMessageW(nav_toolbar,TB_ADDBUTTONSW,sizeof(nav_buttons)/sizeof(TBBUTTON),(LPARAM)nav_buttons);

    band_info.cbSize = sizeof(band_info);
    band_info.fMask = RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
    band_info.hwndChild = nav_toolbar;
    band_info.fStyle=RBBS_GRIPPERALWAYS|RBBS_CHILDEDGE;
    band_info.cyChild=NAV_TOOLBAR_HEIGHT;
    band_info.cx=0;
    band_info.cyMinChild=NAV_TOOLBAR_HEIGHT;
    band_info.cxMinChild=0;
    SendMessageW(rebar,RB_INSERTBANDW,-1,(LPARAM)&band_info);
    info->path_box = CreateWindowW(WC_COMBOBOXEXW,PATH_BOX_NAME,
                                   WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                   0,0,DEFAULT_WIDTH,PATHBOX_HEIGHT,rebar,NULL,
                                   explorer_hInstance,NULL);
    band_info.cyChild=PATHBOX_HEIGHT;
    band_info.cx=0;
    band_info.cyMinChild=PATHBOX_HEIGHT;
    band_info.cxMinChild=0;
    band_info.fMask|=RBBIM_TEXT;
    band_info.lpText=pathbox_label;
    band_info.fStyle|=RBBS_BREAK;
    band_info.hwndChild=info->path_box;
    SendMessageW(rebar,RB_INSERTBANDW,-1,(LPARAM)&band_info);
    events = make_explorer_events(info);
    IExplorerBrowser_Advise(info->browser,events,&cookie);
    IExplorerBrowser_BrowseToObject(info->browser,(IUnknown*)startFolder,
                                    SBSP_ABSOLUTE);
    ShowWindow(info->main_window,SW_SHOWDEFAULT);
    UpdateWindow(info->main_window);
    IExplorerBrowserEvents_Release(events);
}

static void update_window_size(explorer_info *info, int height, int width)
{
    RECT new_rect;
    new_rect.left = 0;
    new_rect.top = info->rebar_height;
    new_rect.right = width;
    new_rect.bottom = height;
    IExplorerBrowser_SetRect(info->browser,NULL,new_rect);
}

static void do_exit(int code)
{
    OleUninitialize();
    ExitProcess(code);
}

static LRESULT explorer_on_end_edit(explorer_info *info,NMCBEENDEDITW *edit_info)
{
    LPITEMIDLIST pidl = NULL;

    WINE_TRACE("iWhy=%x\n",edit_info->iWhy);
    switch(edit_info->iWhy)
    {
    case CBENF_RETURN:
        {
            WCHAR path[MAX_PATH];
            HWND edit_ctrl = (HWND)SendMessageW(edit_info->hdr.hwndFrom,
                                                CBEM_GETEDITCONTROL,0,0);
            *((WORD*)path)=MAX_PATH;
            SendMessageW(edit_ctrl,EM_GETLINE,0,(LPARAM)path);
            pidl = ILCreateFromPathW(path);
            break;
        }
    case CBENF_ESCAPE:
        /*make sure the that the path box resets*/
        update_path_box(info);
        return 0;
    default:
        return 0;
    }
    if(pidl)
        IExplorerBrowser_BrowseToIDList(info->browser,pidl,SBSP_ABSOLUTE);
    if(edit_info->iWhy==CBENF_RETURN)
        ILFree(pidl);
    return 0;
}

static LRESULT update_rebar_size(explorer_info* info,NMRBAUTOSIZE *size_info)
{
    RECT new_rect;
    RECT window_rect;
    info->rebar_height = size_info->rcTarget.bottom-size_info->rcTarget.top;
    GetWindowRect(info->main_window,&window_rect);
    new_rect.left = 0;
    new_rect.top = info->rebar_height;
    new_rect.right = window_rect.right-window_rect.left;
    new_rect.bottom = window_rect.bottom-window_rect.top;
    IExplorerBrowser_SetRect(info->browser,NULL,new_rect);
    return 0;
}

static LRESULT explorer_on_notify(explorer_info* info,NMHDR* notification)
{
    WINE_TRACE("code=%i\n",notification->code);
    switch(notification->code)
    {
    case CBEN_ENDEDITW:
        return explorer_on_end_edit(info,(NMCBEENDEDITW*)notification);
    case RBN_AUTOSIZE:
        return update_rebar_size(info,(NMRBAUTOSIZE*)notification);
    default:
        break;
    }
    return 0;
}

static LRESULT CALLBACK explorer_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    explorer_info *info
        = (explorer_info*)GetWindowLongPtrW(hwnd,EXPLORER_INFO_INDEX);
    IExplorerBrowser *browser = NULL;

    WINE_TRACE("(hwnd=%p,uMsg=%u,wParam=%lx,lParam=%lx)\n",hwnd,uMsg,wParam,lParam);
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
    case WM_NOTIFY:
        return explorer_on_notify(info,(NMHDR*)lParam);
    case WM_COMMAND:
        if(HIWORD(wParam)==BN_CLICKED)
        {
            switch(LOWORD(wParam))
            {
            case BACK_BUTTON:
                IExplorerBrowser_BrowseToObject(browser,NULL,SBSP_NAVIGATEBACK);
                break;
            case FORWARD_BUTTON:
                IExplorerBrowser_BrowseToObject(browser,NULL,SBSP_NAVIGATEFORWARD);
                break;
            case UP_BUTTON:
                IExplorerBrowser_BrowseToObject(browser,NULL,SBSP_PARENT);
                break;
            }
        }
        break;
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
    window_class.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
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
    if (!params->root[0])
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
    INITCOMMONCONTROLSEX init_info;

    memset(&parameters,0,sizeof(parameters));
    explorer_hInstance = hinstance;
    parse_command_line(cmdline,&parameters);
    hres = OleInitialize(NULL);
    if(!SUCCEEDED(hres))
    {
        WINE_ERR("Could not initialize COM\n");
        ExitProcess(EXIT_FAILURE);
    }
    init_info.dwSize = sizeof(INITCOMMONCONTROLSEX);
    init_info.dwICC = ICC_USEREX_CLASSES | ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    if(!InitCommonControlsEx(&init_info))
    {
        WINE_ERR("Could not initialize Comctl\n");
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
