/*
 * 				Shell basics
 *
 *  1998 Marcus Meissner
 *  1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "heap.h"
#include "dlgs.h"
#include "ldt.h"
#include "sysmetrics.h"
#include "debugtools.h"
#include "winreg.h"
#include "authors.h"
#include "winversion.h"

#include "shellapi.h"
#include "pidl.h"

#include "shell32_main.h"
#include "wine/undocshell.h"
#include "shlobj.h"
#include "shlguid.h"
#include "shlwapi.h"

DEFAULT_DEBUG_CHANNEL(shell);

#define MORE_DEBUG 1
/*************************************************************************
 * CommandLineToArgvW			[SHELL32.7]
 */
LPWSTR* WINAPI CommandLineToArgvW(LPWSTR cmdline,LPDWORD numargs)
{	LPWSTR  *argv,s,t;
	int	i;
	TRACE("\n");

	/* to get writeable copy */
	cmdline = HEAP_strdupW( GetProcessHeap(), 0, cmdline);
	s=cmdline;i=0;
	while (*s)
	{ /* space */
	  if (*s==0x0020) 
	  { i++;
	    s++;
	    while (*s && *s==0x0020)
	      s++;
	    continue;
	  }
	  s++;
	}
	argv=(LPWSTR*)HeapAlloc( GetProcessHeap(), 0, sizeof(LPWSTR)*(i+1) );
	s=t=cmdline;
	i=0;
	while (*s)
	{ if (*s==0x0020)
	  { *s=0;
	    argv[i++]=HEAP_strdupW( GetProcessHeap(), 0, t );
	    *s=0x0020;
	    while (*s && *s==0x0020)
	      s++;
	    if (*s)
	      t=s+1;
	    else
	      t=s;
	    continue;
	  }
	  s++;
	}
	if (*t)
	  argv[i++]=(LPWSTR)HEAP_strdupW( GetProcessHeap(), 0, t );

	HeapFree( GetProcessHeap(), 0, cmdline );
	argv[i]=NULL;
	*numargs=i;
	return argv;
}

/*************************************************************************
 * Control_RunDLL			[SHELL32.12]
 *
 * Wild speculation in the following!
 *
 * http://premium.microsoft.com/msdn/library/techart/msdn193.htm
 */

void WINAPI Control_RunDLL( HWND hwnd, LPCVOID code, LPCSTR cmd, DWORD arg4 )
{
    FIXME("(0x%08x, %p, %s, 0x%08lx): stub\n", hwnd, code,
          debugstr_a(cmd), arg4);
}

/*************************************************************************
 * SHGetFileInfoA			[SHELL32.@]
 *
 */

DWORD WINAPI SHGetFileInfoA(LPCSTR path,DWORD dwFileAttributes,
                              SHFILEINFOA *psfi, UINT sizeofpsfi,
                              UINT flags )
{
	char szLoaction[MAX_PATH];
	int iIndex;
	DWORD ret = TRUE, dwAttributes = 0;
	IShellFolder * psfParent = NULL;
	IExtractIconA * pei = NULL;
	LPITEMIDLIST	pidlLast = NULL, pidl = NULL;
	HRESULT hr = S_OK;

	TRACE("(%s fattr=0x%lx sfi=%p(attr=0x%08lx) size=0x%x flags=0x%x)\n", 
	  (flags & SHGFI_PIDL)? "pidl" : path, dwFileAttributes, psfi, psfi->dwAttributes, sizeofpsfi, flags);

	if ((flags & SHGFI_USEFILEATTRIBUTES) && (flags & (SHGFI_ATTRIBUTES|SHGFI_EXETYPE|SHGFI_PIDL)))
	  return FALSE;
	
	/* windows initializes this values regardless of the flags */
	psfi->szDisplayName[0] = '\0';
	psfi->szTypeName[0] = '\0';
	psfi->iIcon = 0;
	
	/* translate the path into a pidl only when SHGFI_USEFILEATTRIBUTES in not specified 
	   the pidl functions fail on not existing file names */
	if (flags & SHGFI_PIDL)
	{
	  pidl = (LPCITEMIDLIST) path;
	  if (!pidl )
	  {
	    ERR("pidl is null!\n");
	    return FALSE;
	  }
	}
	else if (!(flags & SHGFI_USEFILEATTRIBUTES))
	{
	  hr = SHILCreateFromPathA ( path, &pidl, &dwAttributes);
	  /* note: the attributes in ISF::ParseDisplayName are not implemented */
	}
	
	/* get the parent shellfolder */
	if (pidl)
	{
	  hr = SHBindToParent( pidl, &IID_IShellFolder, (LPVOID*)&psfParent, &pidlLast);
	}
	
	/* get the attributes of the child */
	if (SUCCEEDED(hr) && (flags & SHGFI_ATTRIBUTES))
	{
	  if (!(flags & SHGFI_ATTR_SPECIFIED))
	  {
	    psfi->dwAttributes = 0xffffffff;
	  }
	  IShellFolder_GetAttributesOf(psfParent, 1 , &pidlLast, &(psfi->dwAttributes));
	}

	/* get the displayname */
	if (SUCCEEDED(hr) && (flags & SHGFI_DISPLAYNAME))
	{ 
	  if (flags & SHGFI_USEFILEATTRIBUTES)
	  {
	    strcpy (psfi->szDisplayName, PathFindFileNameA(path));
	  }
	  else
	  {
	    STRRET str;
	    hr = IShellFolder_GetDisplayNameOf(psfParent, pidlLast, SHGDN_INFOLDER, &str);
	    StrRetToStrNA (psfi->szDisplayName, MAX_PATH, &str, pidlLast);
	  }
	}

	/* get the type name */
	if (SUCCEEDED(hr) && (flags & SHGFI_TYPENAME))
	{
	  _ILGetFileType(pidlLast, psfi->szTypeName, 80);
	}

	/* ### icons ###*/
	if (flags & SHGFI_LINKOVERLAY)
	  FIXME("set icon to link, stub\n");

	if (flags & SHGFI_SELECTED)
	  FIXME("set icon to selected, stub\n");

	if (flags & SHGFI_SHELLICONSIZE)
	  FIXME("set icon to shell size, stub\n");

	/* get the iconlocation */
	if (SUCCEEDED(hr) && (flags & SHGFI_ICONLOCATION ))
	{
	  UINT uDummy,uFlags;
	  hr = IShellFolder_GetUIObjectOf(psfParent, 0, 1, &pidlLast, &IID_IExtractIconA, &uDummy, (LPVOID*)&pei);

	  if (SUCCEEDED(hr))
	  {
	    hr = IExtractIconA_GetIconLocation(pei, (flags & SHGFI_OPENICON)? GIL_OPENICON : 0,szLoaction, MAX_PATH, &iIndex, &uFlags);
	    /* fixme what to do with the index? */

	    if(uFlags != GIL_NOTFILENAME)
	      strcpy (psfi->szDisplayName, szLoaction);
	    else
	      ret = FALSE;
	      
	    IExtractIconA_Release(pei);
	  }
	}

	/* get icon index (or load icon)*/
	if (SUCCEEDED(hr) && (flags & (SHGFI_ICON | SHGFI_SYSICONINDEX)))
	{
	  if (flags & SHGFI_USEFILEATTRIBUTES)
	  {
	    char sTemp [MAX_PATH];
	    char * szExt;
	    DWORD dwNr=0;

	    lstrcpynA(sTemp, path, MAX_PATH);
	    szExt = (LPSTR) PathFindExtensionA(sTemp);
	    if( szExt && HCR_MapTypeToValue(szExt, sTemp, MAX_PATH, TRUE)
              && HCR_GetDefaultIcon(sTemp, sTemp, MAX_PATH, &dwNr))
            {
              if (!strcmp("%1",sTemp))            /* icon is in the file */
              {
                strcpy(sTemp, path);
              }
	      /* FIXME: if sTemp contains a valid filename, get the icon 
	         from there, index is in dwNr
	      */
              psfi->iIcon = 2;
            }
            else                                  /* default icon */
            {
              psfi->iIcon = 0;
            }          
	  }
	  else
	  {
	    if (!(PidlToSicIndex(psfParent, pidlLast, (flags & SHGFI_LARGEICON), 
	      (flags & SHGFI_OPENICON)? GIL_OPENICON : 0, &(psfi->iIcon))))
	    {
	      ret = FALSE;
	    }
	  }
	  if (ret) 
	  {
	    ret = (DWORD) ((flags & SHGFI_LARGEICON) ? ShellBigIconList : ShellSmallIconList);
	  }
	}

	/* icon handle */
	if (SUCCEEDED(hr) && (flags & SHGFI_ICON))
	  psfi->hIcon = pImageList_GetIcon((flags & SHGFI_LARGEICON) ? ShellBigIconList:ShellSmallIconList, psfi->iIcon, ILD_NORMAL);


	if (flags & SHGFI_EXETYPE)
	  FIXME("type of executable, stub\n");

	if (flags & (SHGFI_UNKNOWN1 | SHGFI_UNKNOWN2 | SHGFI_UNKNOWN3))
	  FIXME("unknown attribute!\n");

	if (psfParent)
	  IShellFolder_Release(psfParent);

	if (hr != S_OK)
	  ret = FALSE;

	if(pidlLast) SHFree(pidlLast);
#ifdef MORE_DEBUG
	TRACE ("icon=0x%08x index=0x%08x attr=0x%08lx name=%s type=%s ret=0x%08lx\n", 
		psfi->hIcon, psfi->iIcon, psfi->dwAttributes, psfi->szDisplayName, psfi->szTypeName, ret);
#endif
	return ret;
}

/*************************************************************************
 * SHGetFileInfoW			[SHELL32.@]
 */

DWORD WINAPI SHGetFileInfoW(LPCWSTR path,DWORD dwFileAttributes,
                              SHFILEINFOW *psfi, UINT sizeofpsfi,
                              UINT flags )
{	FIXME("(%s,0x%lx,%p,0x%x,0x%x)\n",
	      debugstr_w(path),dwFileAttributes,psfi,sizeofpsfi,flags);
	return 0;
}

/*************************************************************************
 * SHGetFileInfoAW			[SHELL32.@]
 */
DWORD WINAPI SHGetFileInfoAW(
	LPCVOID path,
	DWORD dwFileAttributes,
	LPVOID psfi,
	UINT sizeofpsfi,
	UINT flags)
{
	if(VERSION_OsIsUnicode())
	  return SHGetFileInfoW(path, dwFileAttributes, psfi, sizeofpsfi, flags );
	return SHGetFileInfoA(path, dwFileAttributes, psfi, sizeofpsfi, flags );
}

/*************************************************************************
 * ExtractIconA				[SHELL32.133]
 *
 * fixme
 *  is the filename is not a file return 1
 */
HICON WINAPI ExtractIconA( HINSTANCE hInstance, LPCSTR lpszExeFileName,
	UINT nIconIndex )
{   HGLOBAL16 handle = InternalExtractIcon16(hInstance,lpszExeFileName,nIconIndex, 1);
    TRACE("\n");
    if( handle )
    {
	HICON16* ptr = (HICON16*)GlobalLock16(handle);
	HICON16  hIcon = *ptr;

	GlobalFree16(handle);
	return hIcon;
    }
    return 0;
}

/*************************************************************************
 * ExtractIconW				[SHELL32.180]
 *
 * fixme
 *  is the filename is not a file return 1
 */
HICON WINAPI ExtractIconW( HINSTANCE hInstance, LPCWSTR lpszExeFileName,
	UINT nIconIndex )
{ LPSTR  exefn;
  HICON  ret;
  TRACE("\n");

  exefn = HEAP_strdupWtoA(GetProcessHeap(),0,lpszExeFileName);
  ret = ExtractIconA(hInstance,exefn,nIconIndex);

	HeapFree(GetProcessHeap(),0,exefn);
	return ret;
}

/*************************************************************************
 * FindExecutableA			[SHELL32.184]
 */
HINSTANCE WINAPI FindExecutableA( LPCSTR lpFile, LPCSTR lpDirectory,
                                      LPSTR lpResult )
{ HINSTANCE retval=31;    /* default - 'No association was found' */
    char old_dir[1024];

  TRACE("File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ))
  { /* FIXME - should throw a warning, perhaps! */
	return 2; /* File not found. Close enough, I guess. */
    }

    if (lpDirectory)
  { GetCurrentDirectoryA( sizeof(old_dir), old_dir );
        SetCurrentDirectoryA( lpDirectory );
    }

    retval = SHELL_FindExecutable( lpFile, "open", lpResult );

  TRACE("returning %s\n", lpResult);
  if (lpDirectory)
    SetCurrentDirectoryA( old_dir );
    return retval;
}

/*************************************************************************
 * FindExecutableW			[SHELL32.219]
 */
HINSTANCE WINAPI FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory,
                                     LPWSTR lpResult)
{
  FIXME("(%p,%p,%p): stub\n", lpFile, lpDirectory, lpResult);
  return 31;    /* default - 'No association was found' */
}

typedef struct
{ LPCSTR  szApp;
    LPCSTR  szOtherStuff;
    HICON hIcon;
} ABOUT_INFO;

#define		IDC_STATIC_TEXT		100
#define		IDC_LISTBOX		99
#define		IDC_WINE_TEXT		98

#define		DROP_FIELD_TOP		(-15)
#define		DROP_FIELD_HEIGHT	15

extern HICON hIconTitleFont;

static BOOL __get_dropline( HWND hWnd, LPRECT lprect )
{ HWND hWndCtl = GetDlgItem(hWnd, IDC_WINE_TEXT);
    if( hWndCtl )
  { GetWindowRect( hWndCtl, lprect );
	MapWindowPoints( 0, hWnd, (LPPOINT)lprect, 2 );
	lprect->bottom = (lprect->top += DROP_FIELD_TOP);
	return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 * SHAppBarMessage			[SHELL32.207]
 */
UINT WINAPI SHAppBarMessage(DWORD msg, PAPPBARDATA data)
{
        int width=data->rc.right - data->rc.left;
        int height=data->rc.bottom - data->rc.top;
        RECT rec=data->rc;
        switch (msg)
        { case ABM_GETSTATE:
               return ABS_ALWAYSONTOP | ABS_AUTOHIDE;
          case ABM_GETTASKBARPOS:
               GetWindowRect(data->hWnd, &rec);
               data->rc=rec;
               return TRUE;
          case ABM_ACTIVATE:
               SetActiveWindow(data->hWnd);
               return TRUE;
          case ABM_GETAUTOHIDEBAR:
               data->hWnd=GetActiveWindow();
               return TRUE;
          case ABM_NEW:
               SetWindowPos(data->hWnd,HWND_TOP,rec.left,rec.top,
                                        width,height,SWP_SHOWWINDOW);
               return TRUE;
          case ABM_QUERYPOS:
               GetWindowRect(data->hWnd, &(data->rc));
               return TRUE;
          case ABM_REMOVE:
               CloseHandle(data->hWnd);
               return TRUE;
          case ABM_SETAUTOHIDEBAR:
               SetWindowPos(data->hWnd,HWND_TOP,rec.left+1000,rec.top,
                                       width,height,SWP_SHOWWINDOW);          
               return TRUE;
          case ABM_SETPOS:
               data->uEdge=(ABE_RIGHT | ABE_LEFT);
               SetWindowPos(data->hWnd,HWND_TOP,data->rc.left,data->rc.top,
                                  width,height,SWP_SHOWWINDOW);
               return TRUE;
          case ABM_WINDOWPOSCHANGED:
               SetWindowPos(data->hWnd,HWND_TOP,rec.left,rec.top,
                                        width,height,SWP_SHOWWINDOW);
               return TRUE;
          }
      return FALSE;
}

/*************************************************************************
 * SHHelpShortcuts_RunDLL		[SHELL32.224]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLL (DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{ FIXME("(%lx, %lx, %lx, %lx) empty stub!\n",
	dwArg1, dwArg2, dwArg3, dwArg4);

  return 0;
}

/*************************************************************************
 * SHLoadInProc				[SHELL32.225]
 * Create an instance of specified object class from within 
 * the shell process and release it immediately
 */

DWORD WINAPI SHLoadInProc (REFCLSID rclsid)
{
	IUnknown * pUnk = NULL;
	TRACE("%s\n", debugstr_guid(rclsid));

	CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown,(LPVOID*)pUnk);
	if(pUnk)
	{
	  IUnknown_Release(pUnk);
          return NOERROR;
	}
	return DISP_E_MEMBERNOTFOUND;
}

/*************************************************************************
 * ShellExecuteA			[SHELL32.245]
 */
HINSTANCE WINAPI ShellExecuteA( HWND hWnd, LPCSTR lpOperation,
                                    LPCSTR lpFile, LPCSTR lpParameters,
                                    LPCSTR lpDirectory, INT iShowCmd )
{   TRACE("\n");
    return ShellExecute16( hWnd, lpOperation, lpFile, lpParameters,
                           lpDirectory, iShowCmd );
}

/*************************************************************************
 * ShellExecuteW			[SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, 
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);   
 */
HINSTANCE WINAPI 
ShellExecuteW(
       HWND hwnd, 
       LPCWSTR lpOperation, 
       LPCWSTR lpFile, 
       LPCWSTR lpParameters, 
       LPCWSTR lpDirectory, 
       INT nShowCmd) {

       FIXME(": stub\n");
       return 0;
}

/*************************************************************************
 * AboutDlgProc			(internal)
 */
BOOL WINAPI AboutDlgProc( HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam )
{   HWND hWndCtl;
    char Template[512], AppTitle[512];

    TRACE("\n");

    switch(msg)
    { case WM_INITDIALOG:
      { ABOUT_INFO *info = (ABOUT_INFO *)lParam;
            if (info)
        { const char* const *pstr = SHELL_People;
                SendDlgItemMessageA(hWnd, stc1, STM_SETICON,info->hIcon, 0);
                GetWindowTextA( hWnd, Template, sizeof(Template) );
                sprintf( AppTitle, Template, info->szApp );
                SetWindowTextA( hWnd, AppTitle );
                SetWindowTextA( GetDlgItem(hWnd, IDC_STATIC_TEXT),
                                  info->szOtherStuff );
                hWndCtl = GetDlgItem(hWnd, IDC_LISTBOX);
                SendMessageA( hWndCtl, WM_SETREDRAW, 0, 0 );
                SendMessageA( hWndCtl, WM_SETFONT, hIconTitleFont, 0 );
                while (*pstr)
          { SendMessageA( hWndCtl, LB_ADDSTRING, (WPARAM)-1, (LPARAM)*pstr );
                    pstr++;
                }
                SendMessageA( hWndCtl, WM_SETREDRAW, 1, 0 );
            }
        }
        return 1;

    case WM_PAINT:
      { RECT rect;
	    PAINTSTRUCT ps;
	    HDC hDC = BeginPaint( hWnd, &ps );

	    if( __get_dropline( hWnd, &rect ) ) {
	        SelectObject( hDC, GetStockObject( BLACK_PEN ) );
	        MoveToEx( hDC, rect.left, rect.top, NULL );
		LineTo( hDC, rect.right, rect.bottom );
	    }
	    EndPaint( hWnd, &ps );
	}
	break;

    case WM_LBTRACKPOINT:
	hWndCtl = GetDlgItem(hWnd, IDC_LISTBOX);
	if( (INT16)GetKeyState16( VK_CONTROL ) < 0 )
      { if( DragDetect( hWndCtl, *((LPPOINT)&lParam) ) )
        { INT idx = SendMessageA( hWndCtl, LB_GETCURSEL, 0, 0 );
		if( idx != -1 )
          { INT length = SendMessageA( hWndCtl, LB_GETTEXTLEN, (WPARAM)idx, 0 );
		    HGLOBAL16 hMemObj = GlobalAlloc16( GMEM_MOVEABLE, length + 1 );
		    char* pstr = (char*)GlobalLock16( hMemObj );

		    if( pstr )
            { HCURSOR16 hCursor = LoadCursor16( 0, MAKEINTRESOURCE16(OCR_DRAGOBJECT) );
			SendMessageA( hWndCtl, LB_GETTEXT, (WPARAM)idx, (LPARAM)pstr );
			SendMessageA( hWndCtl, LB_DELETESTRING, (WPARAM)idx, 0 );
			UpdateWindow( hWndCtl );
			if( !DragObject16((HWND16)hWnd, (HWND16)hWnd, DRAGOBJ_DATA, 0, (WORD)hMemObj, hCursor) )
			    SendMessageA( hWndCtl, LB_ADDSTRING, (WPARAM)-1, (LPARAM)pstr );
		    }
            if( hMemObj )
              GlobalFree16( hMemObj );
		}
	    }
	}
	break;

    case WM_QUERYDROPOBJECT:
	if( wParam == 0 )
      { LPDRAGINFO lpDragInfo = (LPDRAGINFO)PTR_SEG_TO_LIN((SEGPTR)lParam);
	    if( lpDragInfo && lpDragInfo->wFlags == DRAGOBJ_DATA )
        { RECT rect;
		if( __get_dropline( hWnd, &rect ) )
          { POINT pt;
	    pt.x=lpDragInfo->pt.x;
	    pt.x=lpDragInfo->pt.y;
		    rect.bottom += DROP_FIELD_HEIGHT;
		    if( PtInRect( &rect, pt ) )
            { SetWindowLongA( hWnd, DWL_MSGRESULT, 1 );
			return TRUE;
		    }
		}
	    }
	}
	break;

    case WM_DROPOBJECT:
	if( wParam == hWnd )
      { LPDRAGINFO lpDragInfo = (LPDRAGINFO)PTR_SEG_TO_LIN((SEGPTR)lParam);
	    if( lpDragInfo && lpDragInfo->wFlags == DRAGOBJ_DATA && lpDragInfo->hList )
        { char* pstr = (char*)GlobalLock16( (HGLOBAL16)(lpDragInfo->hList) );
		if( pstr )
          { static char __appendix_str[] = " with";

		    hWndCtl = GetDlgItem( hWnd, IDC_WINE_TEXT );
		    SendMessageA( hWndCtl, WM_GETTEXT, 512, (LPARAM)Template );
		    if( !strncmp( Template, "WINE", 4 ) )
			SetWindowTextA( GetDlgItem(hWnd, IDC_STATIC_TEXT), Template );
		    else
          { char* pch = Template + strlen(Template) - strlen(__appendix_str);
			*pch = '\0';
			SendMessageA( GetDlgItem(hWnd, IDC_LISTBOX), LB_ADDSTRING, 
					(WPARAM)-1, (LPARAM)Template );
		    }

		    strcpy( Template, pstr );
		    strcat( Template, __appendix_str );
		    SetWindowTextA( hWndCtl, Template );
		    SetWindowLongA( hWnd, DWL_MSGRESULT, 1 );
		    return TRUE;
		}
	    }
	}
	break;

    case WM_COMMAND:
        if (wParam == IDOK)
    {  EndDialog(hWnd, TRUE);
            return TRUE;
        }
        break;
    case WM_CLOSE:
      EndDialog(hWnd, TRUE);
      break;
    }

    return 0;
}


/*************************************************************************
 * ShellAboutA				[SHELL32.243]
 */
BOOL WINAPI ShellAboutA( HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff,
                             HICON hIcon )
{   ABOUT_INFO info;
    HRSRC hRes;
    LPVOID template;
    TRACE("\n");

    if(!(hRes = FindResourceA(shell32_hInstance, "SHELL_ABOUT_MSGBOX", RT_DIALOGA)))
        return FALSE;
    if(!(template = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        return FALSE;

    info.szApp        = szApp;
    info.szOtherStuff = szOtherStuff;
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    return DialogBoxIndirectParamA( GetWindowLongA( hWnd, GWL_HINSTANCE ),
                                      template, hWnd, AboutDlgProc, (LPARAM)&info );
}


/*************************************************************************
 * ShellAboutW				[SHELL32.244]
 */
BOOL WINAPI ShellAboutW( HWND hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff,
                             HICON hIcon )
{   BOOL ret;
    ABOUT_INFO info;
    HRSRC hRes;
    LPVOID template;

    TRACE("\n");
    
    if(!(hRes = FindResourceA(shell32_hInstance, "SHELL_ABOUT_MSGBOX", RT_DIALOGA)))
        return FALSE;
    if(!(template = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        return FALSE;

    info.szApp        = HEAP_strdupWtoA( GetProcessHeap(), 0, szApp );
    info.szOtherStuff = HEAP_strdupWtoA( GetProcessHeap(), 0, szOtherStuff );
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    ret = DialogBoxIndirectParamA( GetWindowLongA( hWnd, GWL_HINSTANCE ),
                                   template, hWnd, AboutDlgProc, (LPARAM)&info );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szApp );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szOtherStuff );
    return ret;
}

/*************************************************************************
 * FreeIconList
 */
void WINAPI FreeIconList( DWORD dw )
{ FIXME("(%lx): stub\n",dw);
}

/***********************************************************************
 * DllGetVersion [SHELL32]
 *
 * Retrieves version information of the 'SHELL32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a shell32.dll from IE4.01 SP1.
 */

HRESULT WINAPI SHELL32_DllGetVersion (DLLVERSIONINFO *pdvi)
{
	if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) 
	{
	  WARN("wrong DLLVERSIONINFO size from app");
	  return E_INVALIDARG;
	}

	pdvi->dwMajorVersion = 4;
	pdvi->dwMinorVersion = 72;
	pdvi->dwBuildNumber = 3110;
	pdvi->dwPlatformID = 1;

	TRACE("%lu.%lu.%lu.%lu\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

	return S_OK;
}
/*************************************************************************
 * global variables of the shell32.dll
 * all are once per process
 *
 */
void	(WINAPI* pDLLInitComctl)(LPVOID);
INT	(WINAPI* pImageList_AddIcon) (HIMAGELIST himl, HICON hIcon);
INT	(WINAPI* pImageList_ReplaceIcon) (HIMAGELIST, INT, HICON);
HIMAGELIST (WINAPI * pImageList_Create) (INT,INT,UINT,INT,INT);
BOOL	(WINAPI* pImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
HICON	(WINAPI * pImageList_GetIcon) (HIMAGELIST, INT, UINT);
INT	(WINAPI* pImageList_GetImageCount)(HIMAGELIST);
COLORREF (WINAPI *pImageList_SetBkColor)(HIMAGELIST, COLORREF);

LPVOID	(WINAPI* pCOMCTL32_Alloc) (INT);  
BOOL	(WINAPI* pCOMCTL32_Free) (LPVOID);  

HDPA	(WINAPI* pDPA_Create) (INT);  
INT	(WINAPI* pDPA_InsertPtr) (const HDPA, INT, LPVOID); 
BOOL	(WINAPI* pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM); 
LPVOID	(WINAPI* pDPA_GetPtr) (const HDPA, INT);   
BOOL	(WINAPI* pDPA_Destroy) (const HDPA); 
INT	(WINAPI *pDPA_Search) (const HDPA, LPVOID, INT, PFNDPACOMPARE, LPARAM, UINT);
LPVOID	(WINAPI *pDPA_DeletePtr) (const HDPA hdpa, INT i);

/* user32 */
HICON (WINAPI *pLookupIconIdFromDirectoryEx)(LPBYTE dir, BOOL bIcon, INT width, INT height, UINT cFlag);
HICON (WINAPI *pCreateIconFromResourceEx)(LPBYTE bits,UINT cbSize, BOOL bIcon, DWORD dwVersion, INT width, INT height,UINT cFlag);

static HINSTANCE	hComctl32;
static INT		shell32_RefCount = 0;

LONG		shell32_ObjCount = 0;
HINSTANCE	shell32_hInstance = 0; 
HMODULE		huser32 = 0;
HIMAGELIST	ShellSmallIconList = 0;
HIMAGELIST	ShellBigIconList = 0;


/*************************************************************************
 * SHELL32 LibMain
 *
 * NOTES
 *  calling oleinitialize here breaks sone apps.
 */

BOOL WINAPI Shell32LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
	    shell32_RefCount++;
	    if (shell32_hInstance) return TRUE;

	    shell32_hInstance = hinstDLL;
	    hComctl32 = GetModuleHandleA("COMCTL32.DLL");	
	    if(!huser32) huser32 = GetModuleHandleA("USER32.DLL");
	    DisableThreadLibraryCalls(shell32_hInstance);

	    if (!hComctl32 || !huser32)
	    {
	      ERR("P A N I C SHELL32 loading failed\n");
	      return FALSE;
	    }

	    /* comctl32 */
	    pDLLInitComctl=(void*)GetProcAddress(hComctl32,"InitCommonControlsEx");
	    pImageList_Create=(void*)GetProcAddress(hComctl32,"ImageList_Create");
	    pImageList_AddIcon=(void*)GetProcAddress(hComctl32,"ImageList_AddIcon");
	    pImageList_ReplaceIcon=(void*)GetProcAddress(hComctl32,"ImageList_ReplaceIcon");
	    pImageList_GetIcon=(void*)GetProcAddress(hComctl32,"ImageList_GetIcon");
	    pImageList_GetImageCount=(void*)GetProcAddress(hComctl32,"ImageList_GetImageCount");
	    pImageList_Draw=(void*)GetProcAddress(hComctl32,"ImageList_Draw");
	    pImageList_SetBkColor=(void*)GetProcAddress(hComctl32,"ImageList_SetBkColor");
	    pCOMCTL32_Alloc=(void*)GetProcAddress(hComctl32, (LPCSTR)71L);
	    pCOMCTL32_Free=(void*)GetProcAddress(hComctl32, (LPCSTR)73L);
	    pDPA_Create=(void*)GetProcAddress(hComctl32, (LPCSTR)328L);
	    pDPA_Destroy=(void*)GetProcAddress(hComctl32, (LPCSTR)329L);
	    pDPA_GetPtr=(void*)GetProcAddress(hComctl32, (LPCSTR)332L);
	    pDPA_InsertPtr=(void*)GetProcAddress(hComctl32, (LPCSTR)334L);
	    pDPA_DeletePtr=(void*)GetProcAddress(hComctl32, (LPCSTR)336L);
	    pDPA_Sort=(void*)GetProcAddress(hComctl32, (LPCSTR)338L);
	    pDPA_Search=(void*)GetProcAddress(hComctl32, (LPCSTR)339L);
	    /* user32 */
	    pLookupIconIdFromDirectoryEx=(void*)GetProcAddress(huser32,"LookupIconIdFromDirectoryEx");
	    pCreateIconFromResourceEx=(void*)GetProcAddress(huser32,"CreateIconFromResourceEx");

	    /* initialize the common controls */
	    if (pDLLInitComctl)
	    {
	      pDLLInitComctl(NULL);
	    }

	    SIC_Initialize();
	    SYSTRAY_Init();
	    InitChangeNotifications();
	    SHInitRestricted(NULL, NULL);
	    break;

	  case DLL_THREAD_ATTACH:
	    shell32_RefCount++;
	    break;

	  case DLL_THREAD_DETACH:
	    shell32_RefCount--;
	    break;

	  case DLL_PROCESS_DETACH:
	    shell32_RefCount--;

	    if ( !shell32_RefCount )
	    { 
	      shell32_hInstance = 0;

	      if (pdesktopfolder) 
	      {
	        IShellFolder_Release(pdesktopfolder);
	        pdesktopfolder = NULL;
	      }

	      SIC_Destroy();
	      FreeChangeNotifications();
	      
	      /* this one is here to check if AddRef/Release is balanced */
	      if (shell32_ObjCount)
	      {
	        WARN("leaving with %lu objects left (memory leak)\n", shell32_ObjCount);
	      }
	    }

	    TRACE("refcount=%u objcount=%lu \n", shell32_RefCount, shell32_ObjCount);
	    break;
	}
	return TRUE;
}

/*************************************************************************
 * DllInstall         [SHELL32.202]
 *
 * PARAMETERS
 *   
 *    BOOL bInstall - TRUE for install, FALSE for uninstall
 *    LPCWSTR pszCmdLine - command line (unused by shell32?)
 */

HRESULT WINAPI SHELL32_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;		/* indicate success */
}

/***********************************************************************
 *              DllCanUnloadNow (SHELL32.@)
 */
HRESULT WINAPI SHELL32_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}
