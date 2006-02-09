/*
 * Copyright 2005 Jacek Caban
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_APPSTARTUPNOTIFIER_CONTRACTID "@mozilla.org/embedcomp/appstartup-notifier;1"
#define NS_WEBBROWSER_CONTRACTID "@mozilla.org/embedding/browser/nsWebBrowser;1"
#define NS_PROFILE_CONTRACTID "@mozilla.org/profile/manager;1"

#define APPSTARTUP_TOPIC "app-startup"

#define PR_UINT32_MAX 0xffffffff

struct nsStringContainer {
    void *v;
    void *d1;
    PRUint32 d2;
    void *d3;
};

static nsresult (*NS_InitXPCOM2)(nsIServiceManager**,void*,void*);
static nsresult (*NS_ShutdownXPCOM)(nsIServiceManager*);
static nsresult (*NS_GetComponentRegistrar)(nsIComponentRegistrar**);
static nsresult (*NS_StringContainerInit)(nsStringContainer*);
static nsresult (*NS_CStringContainerInit)(nsCStringContainer*);
static nsresult (*NS_StringContainerFinish)(nsStringContainer*);
static nsresult (*NS_CStringContainerFinish)(nsCStringContainer*);
static nsresult (*NS_StringSetData)(nsAString*,const PRUnichar*,PRUint32);
static nsresult (*NS_CStringSetData)(nsACString*,const char*,PRUint32);
static nsresult (*NS_NewLocalFile)(const nsAString*,PRBool,nsIFile**);
static PRUint32 (*NS_CStringGetData)(nsACString*,const char**,PRBool*);

static HINSTANCE hXPCOM = NULL;

static nsIServiceManager *pServMgr = NULL;
static nsIComponentManager *pCompMgr = NULL;

static const WCHAR wszNsContainer[] = {'N','s','C','o','n','t','a','i','n','e','r',0};

static ATOM nscontainer_class;

static LRESULT WINAPI nsembed_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HTMLDocument *This;
    nsresult nsres;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(HTMLDocument**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = (HTMLDocument*)GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
        case WM_SIZE:
            TRACE("(%p)->(WM_SIZE)\n", This);

            nsres = nsIBaseWindow_SetSize(This->nscontainer->window,
                    LOWORD(lParam), HIWORD(lParam), TRUE);
            if(NS_FAILED(nsres))
                WARN("SetSize failed: %08lx\n", nsres);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}


static void register_nscontainer_class(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        CS_DBLCLKS,
        nsembed_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszNsContainer,
        NULL,
    };
    wndclass.hInstance = hInst;
    nscontainer_class = RegisterClassExW(&wndclass);
}

static BOOL get_mozilla_path(PRUnichar *gre_path)
{
    DWORD res, type, i, size = MAX_PATH;
    HKEY mozilla_key, hkey;
    WCHAR key_name[100];
    BOOL ret = FALSE;

    static const WCHAR wszGreKey[] =
        {'S','o','f','t','w','a','r','e','\\',
            'm','o','z','i','l','l','a','.','o','r','g','\\',
                'G','R','E',0};

    static const WCHAR wszGreHome[] = {'G','r','e','H','o','m','e',0};

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, wszGreKey, &mozilla_key);
    if(res != ERROR_SUCCESS) {
        TRACE("Could not open key %s\n", debugstr_w(wszGreKey));
        return FALSE;
    }

    for(i=0; !ret && RegEnumKeyW(mozilla_key, i, key_name, sizeof(key_name)/sizeof(WCHAR)) == ERROR_SUCCESS; i++) {
        RegOpenKeyW(mozilla_key, key_name, &hkey);
        res = RegQueryValueExW(hkey, wszGreHome, NULL, &type, (LPBYTE)gre_path, &size);
        if(res == ERROR_SUCCESS)
            ret = TRUE;
        RegCloseKey(hkey);
    }

    RegCloseKey(mozilla_key);
    return ret;
}

static BOOL get_mozctl_path(PRUnichar *gre_path)
{
    HKEY hkey;
    DWORD res, type, size = MAX_PATH;

    static const WCHAR wszMozCtlKey[] =
        {'S','o','f','t','w','a','r','e','\\','M','o','z','i','l','l','a',0};
    static const WCHAR wszBinDirectoryPath[] =
        {'B','i','n','D','i','r','e','c','t','o','r','y','P','a','t','h',0};
    static const WCHAR wszMozCtlClsidKey[] =
        {'C','L','S','I','D','\\',
         '{','1','3','3','9','B','5','4','C','-','3','4','5','3','-','1','1','D','2',
         '-','9','3','B','9','-','0','0','0','0','0','0','0','0','0','0','0','0','}','\\',
         'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, wszMozCtlKey, &hkey);
    if(res == ERROR_SUCCESS) {
        res = RegQueryValueExW(hkey, wszBinDirectoryPath, NULL, &type, (LPBYTE)gre_path, &size);
        if(res == ERROR_SUCCESS)
            return TRUE;
        else
            ERR("Could not get value %s\n", debugstr_w(wszBinDirectoryPath));
    }

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, wszMozCtlClsidKey, &hkey);
    if(res == ERROR_SUCCESS) {
        res = RegQueryValueExW(hkey, NULL, NULL, &type, (LPBYTE)gre_path, &size);
        if(res == ERROR_SUCCESS) {
            WCHAR *ptr;
            if((ptr = strrchrW(gre_path, '\\')))
                ptr[1] = 0;
            return TRUE;
        }else {
            ERR("Could not get value of %s\n", debugstr_w(wszMozCtlClsidKey));
        }
    }

    TRACE("Could not find Mozilla ActiveX Control\n");

    return FALSE;
}

static BOOL get_wine_gecko_path(PRUnichar *gre_path)
{
    HKEY hkey;
    DWORD res, type, size = MAX_PATH;

    static const WCHAR wszMshtmlKey[] = {
        'S','o','f','t','w','a','r','e','\\','W','i','n','e',
        '\\','M','S','H','T','M','L',0};
    static const WCHAR wszGeckoPath[] =
        {'G','e','c','k','o','P','a','t','h',0};

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML */
    res = RegOpenKeyW(HKEY_CURRENT_USER, wszMshtmlKey, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, wszGeckoPath, NULL, &type, (LPBYTE)gre_path, &size);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    return TRUE;
}

static void set_profile(void)
{
    nsIProfile *profile;
    PRBool exists = FALSE;
    nsresult nsres;

    static const WCHAR wszMSHTML[] = {'M','S','H','T','M','L',0};

    nsres = nsIServiceManager_GetServiceByContactID(pServMgr, NS_PROFILE_CONTRACTID,
                                         &IID_nsIProfile, (void**)&profile);
    if(NS_FAILED(nsres)) {
        ERR("Could not get profile service: %08lx\n", nsres);
        return;
    }

    nsres = nsIProfile_ProfileExists(profile, wszMSHTML, &exists);
    if(!exists) {
        nsres = nsIProfile_CreateNewProfile(profile, wszMSHTML, NULL, NULL, FALSE);
        if(NS_FAILED(nsres))
            ERR("CreateNewProfile failed: %08lx\n", nsres);
    }

    nsres = nsIProfile_SetCurrentProfile(profile, wszMSHTML);
    if(NS_FAILED(nsres))
        ERR("SetCurrentProfile failed: %08lx\n", nsres);

    nsIProfile_Release(profile);
}

static BOOL load_gecko(void)
{
    nsresult nsres;
    nsIObserver *pStartNotif;
    nsIComponentRegistrar *registrar = NULL;
    nsAString path;
    nsIFile *gre_dir;
    PRUnichar gre_path[MAX_PATH];
    WCHAR path_env[MAX_PATH];
    int len;

    static BOOL tried_load = FALSE;
    static const WCHAR wszPATH[] = {'P','A','T','H',0};
    static const WCHAR strXPCOM[] = {'x','p','c','o','m','.','d','l','l',0};

    TRACE("()\n");

    if(tried_load)
        return pCompMgr != NULL;
    tried_load = TRUE;

    if(!get_wine_gecko_path(gre_path) && !get_mozctl_path(gre_path)
       && !get_mozilla_path(gre_path)) {
        MESSAGE("Could not load Mozilla. HTML rendering will be disabled.\n");
        return FALSE;
    }

    TRACE("found path %s\n", debugstr_w(gre_path));

    /* We have to modify PATH as XPCOM loads other DLLs from this directory. */
    GetEnvironmentVariableW(wszPATH, path_env, sizeof(path_env)/sizeof(WCHAR));
    len = strlenW(path_env);
    path_env[len++] = ';';
    strcpyW(path_env+len, gre_path);
    SetEnvironmentVariableW(wszPATH, path_env);

    hXPCOM = LoadLibraryW(strXPCOM);
    if(!hXPCOM) {
        ERR("Could not load XPCOM: %ld\n", GetLastError());
        return FALSE;
    }

#define NS_DLSYM(func) \
    func = (typeof(func))GetProcAddress(hXPCOM, #func); \
    if(!func) \
        ERR("Could not GetProcAddress(" #func ") failed\n")

    NS_DLSYM(NS_InitXPCOM2);
    NS_DLSYM(NS_ShutdownXPCOM);
    NS_DLSYM(NS_GetComponentRegistrar);
    NS_DLSYM(NS_StringContainerInit);
    NS_DLSYM(NS_CStringContainerInit);
    NS_DLSYM(NS_StringContainerFinish);
    NS_DLSYM(NS_CStringContainerFinish);
    NS_DLSYM(NS_StringSetData);
    NS_DLSYM(NS_CStringSetData);
    NS_DLSYM(NS_NewLocalFile);
    NS_DLSYM(NS_CStringGetData);

#undef NS_DLSYM

    NS_StringContainerInit(&path);
    NS_StringSetData(&path, gre_path, PR_UINT32_MAX);
    nsres = NS_NewLocalFile(&path, FALSE, &gre_dir);
    NS_StringContainerFinish(&path);
    if(NS_FAILED(nsres)) {
        ERR("NS_NewLocalFile failed: %08lx\n", nsres);
        FreeLibrary(hXPCOM);
        return FALSE;
    }

    nsres = NS_InitXPCOM2(&pServMgr, gre_dir, NULL);
    if(NS_FAILED(nsres)) {
        ERR("NS_InitXPCOM2 failed: %08lx\n", nsres);
        FreeLibrary(hXPCOM);
        return FALSE;
    }

    nsres = nsIServiceManager_QueryInterface(pServMgr, &IID_nsIComponentManager, (void**)&pCompMgr);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIComponentManager: %08lx\n", nsres);

    nsres = NS_GetComponentRegistrar(&registrar);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIComponentRegistrar_AutoRegister(registrar, NULL);
        if(NS_FAILED(nsres))
            ERR("AutoRegister(NULL) failed: %08lx\n", nsres);

        nsres = nsIComponentRegistrar_AutoRegister(registrar, gre_dir);
        if(NS_FAILED(nsres))
            ERR("AutoRegister(gre_dir) failed: %08lx\n", nsres);
    }else {
        ERR("NS_GetComponentRegistrar failed: %08lx\n", nsres);
    }

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_APPSTARTUPNOTIFIER_CONTRACTID,
            NULL, &IID_nsIObserver, (void**)&pStartNotif);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIObserver_Observe(pStartNotif, NULL, APPSTARTUP_TOPIC, NULL);
        if(NS_FAILED(nsres))
            ERR("Observe failed: %08lx\n", nsres);

        nsIObserver_Release(pStartNotif);
    }else {
        ERR("could not get appstartup-notifier: %08lx\n", nsres);
    }

    set_profile();

    if(registrar) {
        register_nsservice(registrar);
        init_nsio(pCompMgr, registrar);
        nsIComponentRegistrar_Release(registrar);
    }

    return TRUE;
}

nsACString *nsACString_Create(void)
{
    nsACString *ret;
    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(nsACString));
    NS_CStringContainerInit(ret);
    return ret;
}

void nsACString_SetData(nsACString *str, const char *data)
{
    NS_CStringSetData(str, data, PR_UINT32_MAX);
}

static PRUint32 nsACString_GetData(nsACString *str, const char **data, PRBool *termited)
{
    return NS_CStringGetData(str, data, termited);
}

void nsACString_Destroy(nsACString *str)
{
    NS_CStringContainerFinish(str);
    HeapFree(GetProcessHeap(), 0, str);
}

void close_gecko()
{
    TRACE("()\n");

    if(pCompMgr)
        nsIComponentManager_Release(pCompMgr);

    if(pServMgr)
        nsIServiceManager_Release(pServMgr);

    if(hXPCOM)
        FreeLibrary(hXPCOM);
}

/**********************************************************
 *      nsIWebBrowserChrome interface
 */

#define NSWBCHROME_THIS(iface) DEFINE_THIS(NSContainer, WebBrowserChrome, iface)

static nsresult NSAPI nsWebBrowserChrome_QueryInterface(nsIWebBrowserChrome *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

    *result = NULL;
    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = NSWBCHROME(This);
    }else if(IsEqualGUID(&IID_nsIWebBrowserChrome, riid)) {
        TRACE("(%p)->(IID_nsIWebBrowserChrome, %p)\n", This, result);
        *result = NSWBCHROME(This);
    }else if(IsEqualGUID(&IID_nsIContextMenuListener, riid)) {
        TRACE("(%p)->(IID_nsIContextMenuListener, %p)\n", This, result);
        *result = NSCML(This);
    }else if(IsEqualGUID(&IID_nsIURIContentListener, riid)) {
        TRACE("(%p)->(IID_nsIURIContentListener %p)\n", This, result);
        *result = NSURICL(This);
    }else if(IsEqualGUID(&IID_nsIEmbeddingSiteWindow, riid)) {
        TRACE("(%p)->(IIS_nsIEmbeddingSiteWindow %p)\n", This, result);
        *result = NSEMBWNDS(This);
    }

    if(*result)
        return NS_OK;

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsWebBrowserChrome_AddRef(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    TRACE("(%p)\n", This);
    return 2;  /* Should we implement ref conunting here? */
}

static nsrefcnt NSAPI nsWebBrowserChrome_Release(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    TRACE("(%p)\n", This);
    return 1;
}

static nsresult NSAPI nsWebBrowserChrome_SetStatus(nsIWebBrowserChrome *iface,
        PRUint32 statusType, const PRUnichar *status)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    TRACE("(%p)->(%ld %s)\n", This, statusType, debugstr_w(status));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_GetWebBrowser(nsIWebBrowserChrome *iface,
        nsIWebBrowser **aWebBrowser)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aWebBrowser);

    if(!aWebBrowser)
        return NS_ERROR_INVALID_ARG;

    *aWebBrowser = This->webbrowser;
    return S_OK;
}

static nsresult NSAPI nsWebBrowserChrome_SetWebBrowser(nsIWebBrowserChrome *iface,
        nsIWebBrowser *aWebBrowser)
{
    NSContainer *This = NSWBCHROME_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aWebBrowser);

    if(aWebBrowser != This->webbrowser)
        ERR("Wrong nsWebBrowser!\n");

    return NS_OK;
}

static nsresult NSAPI nsWebBrowserChrome_GetChromeFlags(nsIWebBrowserChrome *iface,
        PRUint32 *aChromeFlags)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%p)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SetChromeFlags(nsIWebBrowserChrome *iface,
        PRUint32 aChromeFlags)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%08lx)\n", This, aChromeFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_DestroyBrowserWindow(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    TRACE("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_SizeBrowserTo(nsIWebBrowserChrome *iface,
        PRInt32 aCX, PRInt32 aCY)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%ld %ld)\n", This, aCX, aCY);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ShowAsModal(nsIWebBrowserChrome *iface)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_IsWindowModal(nsIWebBrowserChrome *iface, PRBool *_retval)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%p)\n", This, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsWebBrowserChrome_ExitModalEventLoop(nsIWebBrowserChrome *iface,
        nsresult aStatus)
{
    NSContainer *This = NSWBCHROME_THIS(iface);
    WARN("(%p)->(%08lx)\n", This, aStatus);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSWBCHROME_THIS

static const nsIWebBrowserChromeVtbl nsWebBrowserChromeVtbl = {
    nsWebBrowserChrome_QueryInterface,
    nsWebBrowserChrome_AddRef,
    nsWebBrowserChrome_Release,
    nsWebBrowserChrome_SetStatus,
    nsWebBrowserChrome_GetWebBrowser,
    nsWebBrowserChrome_SetWebBrowser,
    nsWebBrowserChrome_GetChromeFlags,
    nsWebBrowserChrome_SetChromeFlags,
    nsWebBrowserChrome_DestroyBrowserWindow,
    nsWebBrowserChrome_SizeBrowserTo,
    nsWebBrowserChrome_ShowAsModal,
    nsWebBrowserChrome_IsWindowModal,
    nsWebBrowserChrome_ExitModalEventLoop
};

/**********************************************************
 *      nsIContextMenuListener interface
 */

#define NSCML_THIS(iface) DEFINE_THIS(NSContainer, ContextMenuListener, iface)

static nsresult NSAPI nsContextMenuListener_QueryInterface(nsIContextMenuListener *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsContextMenuListener_AddRef(nsIContextMenuListener *iface)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsContextMenuListener_Release(nsIContextMenuListener *iface)
{
    NSContainer *This = NSCML_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsContextMenuListener_OnShowContextMenu(nsIContextMenuListener *iface,
        PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    NSContainer *This = NSCML_THIS(iface);
    nsIDOMMouseEvent *event;
    POINT pt;
    DWORD dwID = CONTEXT_MENU_DEFAULT;
    nsresult nsres;

    TRACE("(%p)->(%08lx %p %p)\n", This, aContextFlags, aEvent, aNode);

    nsres = nsIDOMEvent_QueryInterface(aEvent, &IID_nsIDOMMouseEvent, (void**)&event);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMMouseEvent interface: %08lx\n", nsres);
        return nsres;
    }

    nsIDOMMouseEvent_GetScreenX(event, &pt.x);
    nsIDOMMouseEvent_GetScreenY(event, &pt.y);
    nsIDOMMouseEvent_Release(event);

    switch(aContextFlags) {
    case CONTEXT_NONE:
    case CONTEXT_DOCUMENT:
    case CONTEXT_TEXT:
        dwID = CONTEXT_MENU_DEFAULT;
        break;
    case CONTEXT_IMAGE:
    case CONTEXT_IMAGE|CONTEXT_LINK:
        dwID = CONTEXT_MENU_IMAGE;
        break;
    case CONTEXT_LINK:
        dwID = CONTEXT_MENU_ANCHOR;
        break;
    case CONTEXT_INPUT:
        dwID = CONTEXT_MENU_CONTROL;
        break;
    default:
        FIXME("aContextFlags=%08lx\n", aContextFlags);
    };

    HTMLDocument_ShowContextMenu(This->doc, dwID, &pt);

    return NS_OK;
}

#undef NSCML_THIS

static const nsIContextMenuListenerVtbl nsContextMenuListenerVtbl = {
    nsContextMenuListener_QueryInterface,
    nsContextMenuListener_AddRef,
    nsContextMenuListener_Release,
    nsContextMenuListener_OnShowContextMenu
};

/**********************************************************
 *      nsIURIContentListener interface
 */

#define NSURICL_THIS(iface) DEFINE_THIS(NSContainer, URIContentListener, iface)

static nsresult NSAPI nsURIContentListener_QueryInterface(nsIURIContentListener *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsURIContentListener_AddRef(nsIURIContentListener *iface)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsURIContentListener_Release(nsIURIContentListener *iface)
{
    NSContainer *This = NSURICL_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsURIContentListener_OnStartURIOpen(nsIURIContentListener *iface, nsIURI *aURI,
        PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);
    BOOL do_load = TRUE;
    nsresult nsres;
    nsACString *spec_str = nsACString_Create();

    TRACE("(%p)->(%p %p)\n", This, aURI, _retval);

    nsres = nsIURI_GetSpec(aURI, spec_str);
    if(NS_SUCCEEDED(nsres)) {
        const char *spec = NULL;
        LPWSTR specw;
        int len;

        nsACString_GetData(spec_str, &spec, NULL);

        len = MultiByteToWideChar(CP_ACP, 0, spec, -1, NULL, 0);
        specw = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, spec, -1, specw, -1);

        if(strcmpW(This->url, specw)) /* hack */
            do_load = HTMLDocument_OnLoad(This->doc, specw);

        HeapFree(GetProcessHeap(), 0, specw);
    }else {
        ERR("GetSpec failed: %08lx\n", nsres);
    }

    nsACString_Destroy(spec_str);

    if(!do_load) {
        *_retval = TRUE;
        return NS_OK;
    }

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_DoContent(nsIURIContentListener *iface,
        const char *aContentType, PRBool aIsContentPreferred, nsIRequest *aRequest,
        nsIStreamListener **aContentHandler, PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);
    TRACE("(%p)->(%s %x %p %p %p)\n", This, debugstr_a(aContentType), aIsContentPreferred,
            aRequest, aContentHandler, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_IsPreferred(nsIURIContentListener *iface,
        const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_a(aContentType), aDesiredContentType, _retval);

    /* FIXME: Should we do something here? */
    *_retval = TRUE; 
    return NS_OK;
}

static nsresult NSAPI nsURIContentListener_CanHandleContent(nsIURIContentListener *iface,
        const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType,
        PRBool *_retval)
{
    NSContainer *This = NSURICL_THIS(iface);
    TRACE("(%p)->(%s %x %p %p)\n", This, debugstr_a(aContentType), aIsContentPreferred,
            aDesiredContentType, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_GetLoadCookie(nsIURIContentListener *iface,
        nsISupports **aLoadCookie)
{
    NSContainer *This = NSURICL_THIS(iface);
    WARN("(%p)->(%p)\n", This, aLoadCookie);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_SetLoadCookie(nsIURIContentListener *iface,
        nsISupports *aLoadCookie)
{
    NSContainer *This = NSURICL_THIS(iface);
    WARN("(%p)->(%p)\n", This, aLoadCookie);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_GetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener **aParentContentListener)
{
    NSContainer *This = NSURICL_THIS(iface);
    WARN("(%p)->(%p)\n", This, aParentContentListener);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURIContentListener_SetParentContentListener(nsIURIContentListener *iface,
        nsIURIContentListener *aParentContentListener)
{
    NSContainer *This = NSURICL_THIS(iface);
    WARN("(%p)->(%p)\n", This, aParentContentListener);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSURICL_THIS

static const nsIURIContentListenerVtbl nsURIContentListenerVtbl = {
    nsURIContentListener_QueryInterface,
    nsURIContentListener_AddRef,
    nsURIContentListener_Release,
    nsURIContentListener_OnStartURIOpen,
    nsURIContentListener_DoContent,
    nsURIContentListener_IsPreferred,
    nsURIContentListener_CanHandleContent,
    nsURIContentListener_GetLoadCookie,
    nsURIContentListener_SetLoadCookie,
    nsURIContentListener_GetParentContentListener,
    nsURIContentListener_SetParentContentListener
};

/**********************************************************
 *      nsIEmbeddinSiteWindow interface
 */

#define NSEMBWNDS_THIS(iface) DEFINE_THIS(NSContainer, EmbeddingSiteWindow, iface)

static nsresult NSAPI nsEmbeddingSiteWindow_QueryInterface(nsIEmbeddingSiteWindow *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_QueryInterface(NSWBCHROME(This), riid, result);
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_AddRef(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsEmbeddingSiteWindow_Release(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetDimensions(nsIEmbeddingSiteWindow *iface,
        PRUint32 flags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%08lx %ld %ld %ld %ld)\n", This, flags, x, y, cx, cy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetDimensions(nsIEmbeddingSiteWindow *iface,
        PRUint32 flags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%08lx %p %p %p %p)\n", This, flags, x, y, cx, cy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetFocus(nsIEmbeddingSiteWindow *iface)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetVisibility(nsIEmbeddingSiteWindow *iface,
        PRBool *aVisibility)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%p)\n", This, aVisibility);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetVisibility(nsIEmbeddingSiteWindow *iface,
        PRBool aVisibility)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%x)\n", This, aVisibility);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetTitle(nsIEmbeddingSiteWindow *iface,
        PRUnichar **aTitle)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%p)\n", This, aTitle);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_SetTitle(nsIEmbeddingSiteWindow *iface,
        const PRUnichar *aTitle)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);
    WARN("(%p)->(%s)\n", This, debugstr_w(aTitle));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsEmbeddingSiteWindow_GetSiteWindow(nsIEmbeddingSiteWindow *iface,
        void **aSiteWindow)
{
    NSContainer *This = NSEMBWNDS_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSiteWindow);

    *aSiteWindow = This->hwnd;
    return NS_OK;
}

static const nsIEmbeddingSiteWindowVtbl nsEmbeddingSiteWindowVtbl = {
    nsEmbeddingSiteWindow_QueryInterface,
    nsEmbeddingSiteWindow_AddRef,
    nsEmbeddingSiteWindow_Release,
    nsEmbeddingSiteWindow_SetDimensions,
    nsEmbeddingSiteWindow_GetDimensions,
    nsEmbeddingSiteWindow_SetFocus,
    nsEmbeddingSiteWindow_GetVisibility,
    nsEmbeddingSiteWindow_SetVisibility,
    nsEmbeddingSiteWindow_GetTitle,
    nsEmbeddingSiteWindow_SetTitle,
    nsEmbeddingSiteWindow_GetSiteWindow
};

void HTMLDocument_NSContainer_Init(HTMLDocument *This)
{
    nsIWebBrowserSetup *wbsetup;
    nsresult nsres;

    This->nscontainer = NULL;

    if(!load_gecko())
        return;

    This->nscontainer = HeapAlloc(GetProcessHeap(), 0, sizeof(NSContainer));

    This->nscontainer->lpWebBrowserChromeVtbl    = &nsWebBrowserChromeVtbl;
    This->nscontainer->lpContextMenuListenerVtbl = &nsContextMenuListenerVtbl;
    This->nscontainer->lpURIContentListenerVtbl  = &nsURIContentListenerVtbl;
    This->nscontainer->lpEmbeddingSiteWindowVtbl = &nsEmbeddingSiteWindowVtbl;

    This->nscontainer->doc = This;

    nsres = nsIComponentManager_CreateInstanceByContractID(pCompMgr, NS_WEBBROWSER_CONTRACTID,
            NULL, &IID_nsIWebBrowser, (void**)&This->nscontainer->webbrowser);
    if(NS_FAILED(nsres))
        ERR("Creating WebBrowser failed: %08lx\n", nsres);

    nsres = nsIWebBrowser_SetContainerWindow(This->nscontainer->webbrowser,
            NSWBCHROME(This->nscontainer));
    if(NS_FAILED(nsres))
        ERR("SetContainerWindow failed: %08lx\n", nsres);

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser, &IID_nsIBaseWindow,
            (void**)&This->nscontainer->window);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIBaseWindow interface: %08lx\n", nsres);

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser,
            &IID_nsIWebBrowserSetup, (void**)&wbsetup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(wbsetup, SETUP_IS_CHROME_WRAPPER, TRUE);
        nsIWebBrowserSetup_Release(wbsetup);
        if(NS_FAILED(nsres))
            ERR("SetProperty(SETUP_IS_CHROME_WRAPPER) failed: %08lx\n", nsres);
    }else {
        ERR("Could not get nsIWebBrowserSetup interface\n");
    }

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser, &IID_nsIWebNavigation,
            (void**)&This->nscontainer->navigation);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIWebNavigation interface: %08lx\n", nsres);

    nsres = nsIWebBrowserFocus_QueryInterface(This->nscontainer->webbrowser, &IID_nsIWebBrowserFocus,
            (void**)&This->nscontainer->focus);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIWebBrowserFocus interface: %08lx\n", nsres);

#if 0
    nsres = nsIWebBrowserStream_QueryInterface(This->nscontainer->webbrowser, &IID_nsIWebBrowserStream,
            (void**)&This->nscontainer->stream);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIWebBrowserStream interface: %08lx\n", nsres);
#else
    This->nscontainer->stream = NULL;
#endif

    if(!nscontainer_class)
        register_nscontainer_class();

    This->nscontainer->hwnd = CreateWindowExW(0, wszNsContainer, NULL,
            WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 100, 100,
            GetDesktopWindow(), NULL, hInst, This);

    nsres = nsIBaseWindow_InitWindow(This->nscontainer->window, This->nscontainer->hwnd, NULL,
            0, 0, 100, 100);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIBaseWindow_Create(This->nscontainer->window);
        if(NS_FAILED(nsres))
            WARN("Creating window failed: %08lx\n", nsres);

        nsIBaseWindow_SetVisibility(This->nscontainer->window, FALSE);
        nsIBaseWindow_SetEnabled(This->nscontainer->window, FALSE);
    }else {
        ERR("InitWindow failed: %08lx\n", nsres);
    }

    nsres = nsIWebBrowser_SetParentURIContentListener(This->nscontainer->webbrowser,
            NSURICL(This->nscontainer));
    if(NS_FAILED(nsres))
        ERR("SetParentURIContentListener failed: %08lx\n", nsres);

    This->nscontainer->url = NULL;
}

void HTMLDocument_NSContainer_Destroy(HTMLDocument *This)
{
    TRACE("(%p)\n", This);

    nsIWebBrowser_Release(This->nscontainer->webbrowser);
    nsIWebNavigation_Release(This->nscontainer->navigation);
    nsIBaseWindow_Release(This->nscontainer->window);

    if(This->nscontainer->stream)
        nsIWebBrowserStream_Release(This->nscontainer->stream);

    HeapFree(GetProcessHeap(), 0, This->nscontainer);

    if(This->nscontainer->url)
        CoTaskMemFree(This->nscontainer->url);
}
