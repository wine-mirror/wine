/* Control Panel management */
/* Eric Pouech 2001 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "debugtools.h"
#include "cpl.h"

DEFAULT_DEBUG_CHANNEL(shlctrl);

typedef struct CPlApplet {
    struct CPlApplet*   next;		/* linked list */
    HWND		hWnd;
    unsigned		count;		/* number of subprograms */
    HMODULE     	hModule;	/* module of loaded applet */
    APPLET_PROC		proc;		/* entry point address */
    NEWCPLINFOA		info[1];	/* array of count information. 
					 * dwSize field is 0 if entry is invalid */
} CPlApplet;

typedef struct CPanel {
    CPlApplet*		first;		/* linked list */
    HWND		hWnd;
    unsigned            status;
    CPlApplet*		clkApplet;
    unsigned            clkSP;
} CPanel;

static	CPlApplet*	Control_UnloadApplet(CPlApplet* applet)
{
    unsigned	i;
    CPlApplet*	next;

    for (i = 0; i < applet->count; i++) {
        if (!applet->info[i].dwSize) continue;
        applet->proc(applet->hWnd, CPL_STOP, i, applet->info[i].lData);
    }
    if (applet->proc) applet->proc(applet->hWnd, CPL_EXIT, 0L, 0L);
    FreeLibrary(applet->hModule);
    next = applet->next;
    HeapFree(GetProcessHeap(), 0, applet);
    return next;
}

static CPlApplet*	Control_LoadApplet(HWND hWnd, LPCSTR cmd, CPanel* panel)
{
    CPlApplet*	applet;
    unsigned 	i;
    CPLINFO	info;

    if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet))))
       return applet;

    applet->hWnd = hWnd;

    if (!(applet->hModule = LoadLibraryA(cmd))) {
        WARN("Cannot load control panel applet %s\n", cmd);
	goto theError;
    }
    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "CPlApplet"))) {
        WARN("Not a valid control panel applet %s\n", cmd);
	goto theError;
    }
    if (!applet->proc(hWnd, CPL_INIT, 0L, 0L)) {
        WARN("Init of applet has failed\n");
	goto theError;
    }
    if ((applet->count = applet->proc(hWnd, CPL_GETCOUNT, 0L, 0L)) == 0) {
        WARN("No subprogram in applet\n");
	goto theError;
    }

    applet = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, applet, 
			 sizeof(*applet) + (applet->count - 1) * sizeof(NEWCPLINFOA));
    
    for (i = 0; i < applet->count; i++) {
       applet->info[i].dwSize = sizeof(NEWCPLINFOA);
       /* proc is supposed to return a null value upon success for
	* CPL_INQUIRE and CPL_NEWINQUIRE
	* However, real drivers don't seem to behave like this
	* So, use introspection rather than return value
	*/
       applet->proc(hWnd, CPL_NEWINQUIRE, i, (LPARAM)&applet->info[i]);
       if (applet->info[i].hIcon == 0) {
	   applet->proc(hWnd, CPL_INQUIRE, i, (LPARAM)&info);
	   if (info.idIcon == 0 || info.idName == 0) {
	       WARN("Couldn't get info from sp %u\n", i);
	       applet->info[i].dwSize = 0;
	   } else {
	       /* convert the old data into the new structure */
	       applet->info[i].dwFlags = 0;
	       applet->info[i].dwHelpContext = 0;
	       applet->info[i].lData = info.lData;
	       applet->info[i].hIcon = LoadIconA(applet->hModule, 
						 MAKEINTRESOURCEA(info.idIcon));
	       LoadStringA(applet->hModule, info.idName, 
			   applet->info[i].szName, sizeof(applet->info[i].szName));
	       LoadStringA(applet->hModule, info.idInfo, 
			   applet->info[i].szInfo, sizeof(applet->info[i].szInfo));
	       applet->info[i].szHelpFile[0] = '\0';
	   }
       }
    }

    applet->next = panel->first;
    panel->first = applet;

    return applet;

 theError:
    Control_UnloadApplet(applet);
    return NULL;
}

static void 	 Control_WndProc_Create(HWND hWnd, const CREATESTRUCTA* cs)
{
   CPanel*	panel = (CPanel*)cs->lpCreateParams;

   SetWindowLongA(hWnd, 0, (LPARAM)panel);
   panel->status = 0;
   panel->hWnd = hWnd;
}

#define	XICON	32
#define XSTEP	128
#define	YICON	32
#define YSTEP	64

static BOOL	Control_Localize(const CPanel* panel, unsigned cx, unsigned cy,
				 CPlApplet** papplet, unsigned* psp)
{
    unsigned	i, x = (XSTEP-XICON)/2, y = 0;
    CPlApplet*	applet;
    RECT	rc;
    
    GetClientRect(panel->hWnd, &rc);
    for (applet = panel->first; applet; applet = applet = applet->next) {
        for (i = 0; i < applet->count; i++) {
	    if (!applet->info[i].dwSize) continue;
	    if (x + XSTEP >= rc.right - rc.left) {
	        x = (XSTEP-XICON)/2;
		y += YSTEP;
	    }
	    if (cx >= x && cx < x + XICON && cy >= y && cy < y + YSTEP) {
	        *papplet = applet;
	        *psp = i;
		return TRUE;
	    }
	    x += XSTEP;
	}
    }
    return FALSE;
}

static LRESULT Control_WndProc_Paint(const CPanel* panel, WPARAM wParam)
{
    HDC		hdc;
    PAINTSTRUCT	ps;
    RECT	rc, txtRect;
    unsigned	i, x = 0, y = 0;
    CPlApplet*	applet;
    HGDIOBJ	hOldFont;

    hdc = (wParam) ? (HDC)wParam : BeginPaint(panel->hWnd, &ps);
    hOldFont = SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    GetClientRect(panel->hWnd, &rc);
    for (applet = panel->first; applet; applet = applet = applet->next) {
        for (i = 0; i < applet->count; i++) {
	    if (x + XSTEP >= rc.right - rc.left) {
	        x = 0;
		y += YSTEP;
	    }
	    if (!applet->info[i].dwSize) continue;
	    DrawIcon(hdc, x + (XSTEP-XICON)/2, y, applet->info[i].hIcon);
	    txtRect.left = x;
	    txtRect.right = x + XSTEP;
	    txtRect.top = y + YICON;
	    txtRect.bottom = y + YSTEP;
	    DrawTextA(hdc, applet->info[i].szName, -1, &txtRect, 
		      DT_CENTER | DT_VCENTER);
	    x += XSTEP;
	}
    }
    SelectObject(hdc, hOldFont);
    if (!wParam) EndPaint(panel->hWnd, &ps);
    return 0;
}

static LRESULT Control_WndProc_LButton(CPanel* panel, LPARAM lParam, BOOL up)
{
    unsigned	i;
    CPlApplet*	applet;
    
    if (Control_Localize(panel, LOWORD(lParam), HIWORD(lParam), &applet, &i)) {
       if (up) {
	   if (panel->clkApplet == applet && panel->clkSP == i) {
	       applet->proc(applet->hWnd, CPL_DBLCLK, i, applet->info[i].lData);
	   }
       } else {
	   panel->clkApplet = applet;
	   panel->clkSP = i;
       }
    }
    return 0;
}

static LRESULT WINAPI	Control_WndProc(HWND hWnd, UINT wMsg, 
					WPARAM lParam1, LPARAM lParam2)
{
   CPanel*	panel = (CPanel*)GetWindowLongA(hWnd, 0);

   if (panel || wMsg == WM_CREATE) {
      switch (wMsg) {
      case WM_CREATE:
	 Control_WndProc_Create(hWnd, (CREATESTRUCTA*)lParam2);
	 return 0;
      case WM_DESTROY:
	 while ((panel->first = Control_UnloadApplet(panel->first)));
	 break;
      case WM_PAINT:
	 return Control_WndProc_Paint(panel, lParam1);
      case WM_LBUTTONUP:
	 return Control_WndProc_LButton(panel, lParam2, TRUE);
      case WM_LBUTTONDOWN:
	 return Control_WndProc_LButton(panel, lParam2, FALSE);
/* EPP       case WM_COMMAND: */
/* EPP 	 return Control_WndProc_Command(mwi, lParam1, lParam2); */
      }
   }

   return DefWindowProcA(hWnd, wMsg, lParam1, lParam2);
}

static void    Control_DoInterface(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    WNDCLASSA	wc;
    MSG		msg;

    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = Control_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(CPlApplet*);
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "Shell_Control_WndClass";

    if (!RegisterClassA(&wc)) return;

    CreateWindowExA(0, wc.lpszClassName, "Wine Control Panel", 
		    WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
		    CW_USEDEFAULT, CW_USEDEFAULT, 
		    CW_USEDEFAULT, CW_USEDEFAULT, 
		    hWnd, (HMENU)0, hInst, panel);
    if (!panel->hWnd) return;
    while (GetMessageA(&msg, panel->hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
	if (!panel->first) break;
    }
}

static	void	Control_DoWindow(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    HANDLE		h;
    WIN32_FIND_DATAA	fd;
    char		buffer[MAX_PATH];

    /* FIXME: should grab path somewhere from configuration */
    if ((h = FindFirstFileA("c:\\windows\\system\\*.cpl", &fd)) != 0) {
        do {
	   sprintf(buffer, "c:\\windows\\system\\%s", fd.cFileName);
	   Control_LoadApplet(hWnd, buffer, panel);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
    }

    if (panel->first) Control_DoInterface(panel, hWnd, hInst);
}

static	void	Control_DoLaunch(CPanel* panel, HWND hWnd, LPCSTR cmd)
   /* forms to parse:
    *	foo.cpl,@sp,str
    *	foo.cpl,@sp
    *	foo.cpl,,str
    *	foo.cpl @sp
    *	foo.cpl str
    */
{
    char*	buffer;
    char*	beg = NULL;
    char*	end;
    char	ch;
    unsigned 	sp = 0;
    char*	extraPmts = NULL;

    buffer = HeapAlloc(GetProcessHeap(), 0, strlen(cmd) + 1);
    if (!buffer) return;

    end = strcpy(buffer, cmd);

    for (;;) {
	ch = *end;
	if (ch == ' ' || ch == ',' || ch == '\0') {
	    *end = '\0';
	    if (beg) {
	        if (*beg == '@') {
		    sp = atoi(beg + 1);
		} else if (*beg == '\0') {
		    sp = 0;
		} else {
		    extraPmts = beg;
		}
	    }
	    if (ch == '\0') break;
	    beg = end + 1;
	    if (ch == ' ') while (end[1] == ' ') end++;
	}
	end++;
    }
    Control_LoadApplet(hWnd, buffer, panel);

    if (panel->first) {
       CPlApplet* applet = panel->first;

       assert(applet && applet->next == NULL);
       if (sp >= applet->count) {
	  WARN("Out of bounds (%u >= %u), setting to 0\n", sp, applet->count);
	  sp = 0;
       }
       if (applet->info[sp].dwSize) {
	  if (!applet->proc(applet->hWnd, CPL_STARTWPARMSA, sp, (LPARAM)extraPmts))
	     applet->proc(applet->hWnd, CPL_DBLCLK, sp, applet->info[sp].lData);
       }
       Control_UnloadApplet(applet);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
}

/*************************************************************************
 * Control_RunDLL			[SHELL32.12]
 *
 */
void WINAPI Control_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    CPanel	panel;

    TRACE("(0x%08x, 0x%08lx, %s, 0x%08lx)\n", 
	  hWnd, (DWORD)hInst, debugstr_a(cmd), nCmdShow);

    memset(&panel, 0, sizeof(panel));

    if (!cmd || !*cmd) {
        Control_DoWindow(&panel, hWnd, hInst);
    } else {
        Control_DoLaunch(&panel, hWnd, cmd);
    }
}

/*************************************************************************
 * Control_FillCache_RunDLL			[SHELL32.8]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLL(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    FIXME("0x%04x 0x%04x 0x%04lx 0x%04lx stub\n",hWnd, hModule, w, x);
    return 0;
}

/*************************************************************************
 * RunDLL_CallEntry16				[SHELL32.122]
 * the name is propably wrong
 */
HRESULT WINAPI RunDLL_CallEntry16(DWORD v, DWORD w, DWORD x, DWORD y, DWORD z)
{
    FIXME("0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",v,w,x,y,z);
    return 0;
}
