/*
 * 				Shell basics
 *
 *  1998 Marcus Meissner
 *  1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "winerror.h"
#include "file.h"
#include "shell.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "graphics.h"
#include "cursoricon.h"
#include "interfaces.h"
#include "sysmetrics.h"
#include "shlobj.h"
#include "debug.h"
#include "winreg.h"
#include "imagelist.h"
#include "sysmetrics.h"
#include "commctrl.h"
#include "authors.h"
#include "pidl.h"
#include "shell32_main.h"

/*************************************************************************
 *				CommandLineToArgvW	[SHELL32.7]
 */
LPWSTR* WINAPI CommandLineToArgvW(LPWSTR cmdline,LPDWORD numargs)
{ LPWSTR  *argv,s,t;
	int	i;
  TRACE(shell,"\n");

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
 *				Control_RunDLL		[SHELL32.12]
 *
 * Wild speculation in the following!
 *
 * http://premium.microsoft.com/msdn/library/techart/msdn193.htm
 */

void WINAPI Control_RunDLL( HWND32 hwnd, LPCVOID code, LPCSTR cmd, DWORD arg4 )
{
    FIXME(shell, "(0x%08x, %p, %s, 0x%08lx): stub\n", hwnd, code,
          debugstr_a(cmd), arg4);
}

/*************************************************************************
 *  SHGetFileInfoA		[SHELL32.254]
 *
 * FIXME
 *   
 */

DWORD WINAPI SHGetFileInfo32A(LPCSTR path,DWORD dwFileAttributes,
                              SHFILEINFO32A *psfi, UINT32 sizeofpsfi,
                              UINT32 flags )
{	CHAR szTemp[MAX_PATH];
	LPPIDLDATA 	pData;
	DWORD ret=0;
  
	TRACE(shell,"(%s,0x%lx,%p,0x%x,0x%x)\n",
	      path,dwFileAttributes,psfi,sizeofpsfi,flags);

	/* translate the pidl to a path*/
	if (flags & SHGFI_PIDL)
	{ SHGetPathFromIDList32A ((LPCITEMIDLIST)path,szTemp);
	  TRACE(shell,"pidl=%p is %s\n",path,szTemp);
	}
	else
	{ TRACE(shell,"path=%p\n",path);
	}

	if (flags & SHGFI_ATTRIBUTES)
	{ if (flags & SHGFI_PIDL)
	  { pData = _ILGetDataPointer((LPCITEMIDLIST)path);
	    psfi->dwAttributes = pData->u.generic.dwSFGAO; /* fixme: no direct access*/
	    ret=TRUE;
	  }
	  else
	  { psfi->dwAttributes=SFGAO_FILESYSTEM;
	    ret=TRUE;
	  }
	  FIXME(shell,"file attributes, stub\n");	    
	}

	if (flags & SHGFI_DISPLAYNAME)
	{ if (flags & SHGFI_PIDL)
	  { strcpy(psfi->szDisplayName,szTemp);
	  }
	  else
	  { strcpy(psfi->szDisplayName,path);
	  }
	  TRACE(shell,"displayname=%s\n", psfi->szDisplayName);	  
	  ret=TRUE;
	}
  
	if (flags & SHGFI_TYPENAME)
 	{ FIXME(shell,"get the file type, stub\n");
	  strcpy(psfi->szTypeName,"FIXME: Type");
	  ret=TRUE;
	}
  
  if (flags & SHGFI_ICONLOCATION)
  { FIXME(shell,"location of icon, stub\n");
    strcpy(psfi->szDisplayName,"");
    ret=TRUE;
  }

  if (flags & SHGFI_EXETYPE)
    FIXME(shell,"type of executable, stub\n");

  if (flags & SHGFI_LINKOVERLAY)
    FIXME(shell,"set icon to link, stub\n");

  if (flags & SHGFI_OPENICON)
    FIXME(shell,"set to open icon, stub\n");

  if (flags & SHGFI_SELECTED)
    FIXME(shell,"set icon to selected, stub\n");

  if (flags & SHGFI_SHELLICONSIZE)
    FIXME(shell,"set icon to shell size, stub\n");

  if (flags & SHGFI_USEFILEATTRIBUTES)
    FIXME(shell,"use the dwFileAttributes, stub\n");
 
  if (flags & SHGFI_ICON)
  { FIXME(shell,"icon handle\n");
    if (flags & SHGFI_SMALLICON)
     { TRACE(shell,"set to small icon\n"); 
       psfi->hIcon=pImageList_GetIcon(ShellSmallIconList,32,ILD_NORMAL);
       ret = (DWORD) ShellSmallIconList;
     }
     else
     { TRACE(shell,"set to big icon\n");
       psfi->hIcon=pImageList_GetIcon(ShellBigIconList,32,ILD_NORMAL);
       ret = (DWORD) ShellBigIconList;
     }      
  }

  if (flags & SHGFI_SYSICONINDEX)
  {  FIXME(shell,"get the SYSICONINDEX\n");
     psfi->iIcon=32;
     if (flags & SHGFI_SMALLICON)
     { TRACE(shell,"set to small icon\n"); 
       ret = (DWORD) ShellSmallIconList;
     }
     else        
     { TRACE(shell,"set to big icon\n");
       ret = (DWORD) ShellBigIconList;
     }
  }

 
  return ret;
}

/*************************************************************************
 *  SHGetFileInfo32W		[SHELL32.255]
 *
 * FIXME
 *   
 */

DWORD WINAPI SHGetFileInfo32W(LPCWSTR path,DWORD dwFileAttributes,
                              SHFILEINFO32W *psfi, UINT32 sizeofpsfi,
                              UINT32 flags )
{	FIXME(shell,"(%s,0x%lx,%p,0x%x,0x%x)\n",
	      debugstr_w(path),dwFileAttributes,psfi,sizeofpsfi,flags);
	return 0;
}

/*************************************************************************
 *             ExtractIcon32A   (SHELL32.133)
 */
HICON32 WINAPI ExtractIcon32A( HINSTANCE32 hInstance, LPCSTR lpszExeFileName,
	UINT32 nIconIndex )
{   HGLOBAL16 handle = InternalExtractIcon(hInstance,lpszExeFileName,nIconIndex, 1);
    TRACE(shell,"\n");
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
 *             ExtractIcon32W   (SHELL32.180)
 */
HICON32 WINAPI ExtractIcon32W( HINSTANCE32 hInstance, LPCWSTR lpszExeFileName,
	UINT32 nIconIndex )
{ LPSTR  exefn;
  HICON32  ret;
  TRACE(shell,"\n");

  exefn = HEAP_strdupWtoA(GetProcessHeap(),0,lpszExeFileName);
  ret = ExtractIcon32A(hInstance,exefn,nIconIndex);

	HeapFree(GetProcessHeap(),0,exefn);
	return ret;
}

/*************************************************************************
 *             FindExecutable32A   (SHELL32.184)
 */
HINSTANCE32 WINAPI FindExecutable32A( LPCSTR lpFile, LPCSTR lpDirectory,
                                      LPSTR lpResult )
{ HINSTANCE32 retval=31;    /* default - 'No association was found' */
    char old_dir[1024];

  TRACE(shell, "File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ))
  { /* FIXME - should throw a warning, perhaps! */
	return 2; /* File not found. Close enough, I guess. */
    }

    if (lpDirectory)
  { GetCurrentDirectory32A( sizeof(old_dir), old_dir );
        SetCurrentDirectory32A( lpDirectory );
    }

    retval = SHELL_FindExecutable( lpFile, "open", lpResult );

  TRACE(shell, "returning %s\n", lpResult);
  if (lpDirectory)
    SetCurrentDirectory32A( old_dir );
    return retval;
}

typedef struct
{ LPCSTR  szApp;
    LPCSTR  szOtherStuff;
    HICON32 hIcon;
} ABOUT_INFO;

#define		IDC_STATIC_TEXT		100
#define		IDC_LISTBOX		99
#define		IDC_WINE_TEXT		98

#define		DROP_FIELD_TOP		(-15)
#define		DROP_FIELD_HEIGHT	15

extern HICON32 hIconTitleFont;

static BOOL32 __get_dropline( HWND32 hWnd, LPRECT32 lprect )
{ HWND32 hWndCtl = GetDlgItem32(hWnd, IDC_WINE_TEXT);
    if( hWndCtl )
  { GetWindowRect32( hWndCtl, lprect );
	MapWindowPoints32( 0, hWnd, (LPPOINT32)lprect, 2 );
	lprect->bottom = (lprect->top += DROP_FIELD_TOP);
	return TRUE;
    }
    return FALSE;
}

/*************************************************************************
 *				SHAppBarMessage32	[SHELL32.207]
 */
UINT32 WINAPI SHAppBarMessage32(DWORD msg, PAPPBARDATA data)
{ FIXME(shell,"(0x%08lx,%p): stub\n", msg, data);
#if 0
  switch (msg)
  { case ABM_ACTIVATE:
        case ABM_GETAUTOHIDEBAR:
        case ABM_GETSTATE:
        case ABM_GETTASKBARPOS:
        case ABM_NEW:
        case ABM_QUERYPOS:
        case ABM_REMOVE:
        case ABM_SETAUTOHIDEBAR:
        case ABM_SETPOS:
        case ABM_WINDOWPOSCHANGED:
	    ;
    }
#endif
    return 0;
}

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.209]
 *
 */
LPITEMIDLIST WINAPI SHBrowseForFolder32A (LPBROWSEINFO32A lpbi)
{ FIXME (shell, "(%lx,%s) empty stub!\n", (DWORD)lpbi, lpbi->lpszTitle);
  return NULL;
}

/*************************************************************************
 *  SHGetDesktopFolder		[SHELL32.216]
 * 
 *  SDK header win95/shlobj.h: This is equivalent to call CoCreateInstance with
 *  CLSID_ShellDesktop
 *  CoCreateInstance(CLSID_Desktop, NULL, CLSCTX_INPROC, IID_IShellFolder, &pshf);
 *
 * RETURNS
 *   the interface to the shell desktop folder.
 *
 * FIXME
 *   the pdesktopfolder has to be released at the end (at dll unloading???)
 */
LPSHELLFOLDER pdesktopfolder=NULL;

DWORD WINAPI SHGetDesktopFolder(LPSHELLFOLDER *shellfolder)
{ HRESULT	hres = E_OUTOFMEMORY;
  LPCLASSFACTORY lpclf;
	TRACE(shell,"%p->(%p)\n",shellfolder,*shellfolder);

  if (pdesktopfolder)
	{	hres = NOERROR;
	}
	else
  { lpclf = IClassFactory_Constructor();
    /* fixme: the buildin IClassFactory_Constructor is at the moment only 
 		for rclsid=CLSID_ShellDesktop, so we get the right Interface (jsch)*/
    if(lpclf)
    { hres = lpclf->lpvtbl->fnCreateInstance(lpclf,NULL,(REFIID)&IID_IShellFolder, (void*)&pdesktopfolder);
	 	  lpclf->lpvtbl->fnRelease(lpclf);
	  }  
  }
	
  if (pdesktopfolder)
	{ *shellfolder = pdesktopfolder;
    pdesktopfolder->lpvtbl->fnAddRef(pdesktopfolder);
	}
  else
	{ *shellfolder=NULL;
	}	

  TRACE(shell,"-- %p->(%p)\n",shellfolder, *shellfolder);
	return hres;
}
/*************************************************************************
 *			 SHGetPathFromIDList		[SHELL32.221][NT 4.0: SHELL32.219]
 */
BOOL32 WINAPI SHGetPathFromIDList32(LPCITEMIDLIST pidl,LPSTR pszPath)     
{ TRACE(shell,"(pidl=%p,%p)\n",pidl,pszPath);
  return SHGetPathFromIDList32A(pidl,pszPath);
}

/*************************************************************************
 *			 SHGetSpecialFolderLocation	[SHELL32.223]
 * gets the folder locations from the registry and creates a pidl
 * creates missing reg keys and directorys
 * 
 * PARAMS
 *   hwndOwner [I]
 *   nFolder   [I] CSIDL_xxxxx
 *   ppidl     [O] PIDL of a special folder
 *
 * RETURNS
 *    HResult
 *
 * FIXME
 *   - look for "User Shell Folder" first
 *
 */
HRESULT WINAPI SHGetSpecialFolderLocation(HWND32 hwndOwner, INT32 nFolder, LPITEMIDLIST * ppidl)
{ LPSHELLFOLDER shellfolder;
  DWORD pchEaten,tpathlen=MAX_PATH,type,dwdisp,res;
  CHAR pszTemp[256],buffer[256],tpath[MAX_PATH],npath[MAX_PATH];
  LPWSTR lpszDisplayName = (LPWSTR)&pszTemp[0];
  HKEY key;

	enum 
	{ FT_UNKNOWN= 0x00000000,
	  FT_DIR=     0x00000001, 
	  FT_DESKTOP= 0x00000002,
	  FT_SPECIAL= 0x00000003
	} tFolder; 

	TRACE(shell,"(%04x,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

	strcpy(buffer,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\");

	res=RegCreateKeyEx32A(HKEY_CURRENT_USER,buffer,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&dwdisp);
	if (res)
	{ ERR(shell,"Could not create key %s %08lx \n",buffer,res);
	  return E_OUTOFMEMORY;
	}

	tFolder=FT_DIR;	
	switch (nFolder)
	{ case CSIDL_BITBUCKET:
	    strcpy (buffer,"xxx");			/*not in the registry*/
	    TRACE (shell,"looking for Recycler\n");
	    tFolder=FT_UNKNOWN;
	    break;
	  case CSIDL_CONTROLS:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE (shell,"looking for Control\n");
	    tFolder=FT_UNKNOWN;
	    break;
	  case CSIDL_DESKTOP:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE (shell,"looking for Desktop\n");
	    tFolder=FT_DESKTOP;			
	    break;
	  case CSIDL_DESKTOPDIRECTORY:
	  case CSIDL_COMMON_DESKTOPDIRECTORY:
	    strcpy (buffer,"Desktop");
	    break;
	  case CSIDL_DRIVES:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE (shell,"looking for Drives\n");
	    tFolder=FT_SPECIAL;
	    break;
	  case CSIDL_FONTS:
	    strcpy (buffer,"Fonts");			
	    break;
	  case CSIDL_NETHOOD:
	    strcpy (buffer,"NetHood");			
	    break;
	  case CSIDL_PRINTHOOD:
	    strcpy (buffer,"PrintHood");			
	    break;
	  case CSIDL_NETWORK:
	    strcpy (buffer,"xxx");				/*virtual folder*/
	    TRACE (shell,"looking for Network\n");
	    tFolder=FT_UNKNOWN;
	    break;
	  case CSIDL_APPDATA:
	    strcpy (buffer,"Appdata");			
	    break;
	  case CSIDL_PERSONAL:
	    strcpy (buffer,"Personal");			
	    break;
	  case CSIDL_FAVORITES:
	    strcpy (buffer,"Favorites");			
	    break;
	  case CSIDL_PRINTERS:
	    strcpy (buffer,"PrintHood");
	    break;
	  case CSIDL_COMMON_PROGRAMS:
	  case CSIDL_PROGRAMS:
	    strcpy (buffer,"Programs");			
	    break;
	  case CSIDL_RECENT:
	    strcpy (buffer,"Recent");
	    break;
	  case CSIDL_SENDTO:
	    strcpy (buffer,"SendTo");
	    break;
	  case CSIDL_COMMON_STARTMENU:
	  case CSIDL_STARTMENU:
	    strcpy (buffer,"Start Menu");
	    break;
	  case CSIDL_COMMON_STARTUP:  
	  case CSIDL_STARTUP:
	    strcpy (buffer,"Startup");			
	    break;
	  case CSIDL_TEMPLATES:
	    strcpy (buffer,"Templates");			
	    break;
	  default:
	    ERR (shell,"unknown CSIDL 0x%08x\n", nFolder);
	    tFolder=FT_UNKNOWN;			
	    break;
	}

	TRACE(shell,"Key=%s\n",buffer);

	type=REG_SZ;

	switch (tFolder)
	{ case FT_DIR:
	    /* Directory: get the value from the registry, if its not there 
			create it and the directory*/
	    if (RegQueryValueEx32A(key,buffer,NULL,&type,(LPBYTE)tpath,&tpathlen))
  	    { GetWindowsDirectory32A(npath,MAX_PATH);
	      PathAddBackslash32A(npath);
	      switch (nFolder)
	      { case CSIDL_DESKTOPDIRECTORY:
	        case CSIDL_COMMON_DESKTOPDIRECTORY:
      		  strcat (npath,"Desktop");
         	  break;
      		case CSIDL_FONTS:
         	  strcat (npath,"Fonts");			
         	  break;
      		case CSIDL_NETHOOD:
         	  strcat (npath,"NetHood");			
         	  break;
		case CSIDL_PRINTHOOD:
         	  strcat (npath,"PrintHood");			
         	  break;
	        case CSIDL_APPDATA:
         	  strcat (npath,"Appdata");			
         	  break;
	        case CSIDL_PERSONAL:
         	  strcpy (npath,"C:\\Personal");			
         	  break;
      		case CSIDL_FAVORITES:
         	  strcat (npath,"Favorites");			
         	  break;
	        case CSIDL_PRINTERS:
         	  strcat (npath,"PrintHood");			
         	  break;
	        case CSIDL_COMMON_PROGRAMS:
      		case CSIDL_PROGRAMS:
         	  strcat (npath,"Start Menu");			
         	  CreateDirectory32A(npath,NULL);
         	  strcat (npath,"\\Programs");			
         	  break;
      		case CSIDL_RECENT:
         	  strcat (npath,"Recent");
         	  break;
      		case CSIDL_SENDTO:
         	  strcat (npath,"SendTo");
         	  break;
	        case CSIDL_COMMON_STARTMENU:
      		case CSIDL_STARTMENU:
         	  strcat (npath,"Start Menu");
         	  break;
	        case CSIDL_COMMON_STARTUP:  
      		case CSIDL_STARTUP:
         	  strcat (npath,"Start Menu");			
         	  CreateDirectory32A(npath,NULL);
         	  strcat (npath,"\\Startup");			
         	  break;
      		case CSIDL_TEMPLATES:
         	  strcat (npath,"Templates");			
         	  break;
         	default:
         	  RegCloseKey(key);
        	  return E_OUTOFMEMORY;
	      }
	      if (RegSetValueEx32A(key,buffer,0,REG_SZ,(LPBYTE)npath,sizeof(npath)+1))
	      { ERR(shell,"could not create value %s\n",buffer);
	        RegCloseKey(key);
	        return E_OUTOFMEMORY;
	      }
	      TRACE(shell,"value %s=%s created\n",buffer,npath);
	      CreateDirectory32A(npath,NULL);
	      strcpy(tpath,npath);
	    }
	    break;
	  case FT_DESKTOP:
	    strcpy (tpath,"Desktop");
	    break;
	  case FT_SPECIAL:
	    if (nFolder==CSIDL_DRIVES)
	    strcpy (tpath,"My Computer");
	    break;
	  default:
	    RegCloseKey(key);
	    return E_OUTOFMEMORY;
	}

	RegCloseKey(key);

	TRACE(shell,"Value=%s\n",tpath);
	LocalToWideChar32(lpszDisplayName, tpath, 256);
  
	if (SHGetDesktopFolder(&shellfolder)==S_OK)
	{ shellfolder->lpvtbl->fnParseDisplayName(shellfolder,hwndOwner, NULL,lpszDisplayName,&pchEaten,ppidl,NULL);
	  shellfolder->lpvtbl->fnRelease(shellfolder);
	}

	TRACE(shell, "-- (new pidl %p)\n",*ppidl);
	return NOERROR;
}
/*************************************************************************
 * SHHelpShortcuts_RunDLL [SHELL32.224]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLL (DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{ FIXME (exec, "(%lx, %lx, %lx, %lx) empty stub!\n",
	dwArg1, dwArg2, dwArg3, dwArg4);

  return 0;
}

/*************************************************************************
 * SHLoadInProc [SHELL32.225]
 *
 */

DWORD WINAPI SHLoadInProc (DWORD dwArg1)
{ FIXME (shell, "(%lx) empty stub!\n", dwArg1);
    return 0;
}

/*************************************************************************
 *             ShellExecute32A   (SHELL32.245)
 */
HINSTANCE32 WINAPI ShellExecute32A( HWND32 hWnd, LPCSTR lpOperation,
                                    LPCSTR lpFile, LPCSTR lpParameters,
                                    LPCSTR lpDirectory, INT32 iShowCmd )
{   TRACE(shell,"\n");
    return ShellExecute16( hWnd, lpOperation, lpFile, lpParameters,
                           lpDirectory, iShowCmd );
}


/*************************************************************************
 *             AboutDlgProc32  (not an exported API function)
 */
LRESULT WINAPI AboutDlgProc32( HWND32 hWnd, UINT32 msg, WPARAM32 wParam,
                               LPARAM lParam )
{   HWND32 hWndCtl;
    char Template[512], AppTitle[512];

    TRACE(shell,"\n");

    switch(msg)
    { case WM_INITDIALOG:
      { ABOUT_INFO *info = (ABOUT_INFO *)lParam;
            if (info)
        { const char* const *pstr = SHELL_People;
                SendDlgItemMessage32A(hWnd, stc1, STM_SETICON32,info->hIcon, 0);
                GetWindowText32A( hWnd, Template, sizeof(Template) );
                sprintf( AppTitle, Template, info->szApp );
                SetWindowText32A( hWnd, AppTitle );
                SetWindowText32A( GetDlgItem32(hWnd, IDC_STATIC_TEXT),
                                  info->szOtherStuff );
                hWndCtl = GetDlgItem32(hWnd, IDC_LISTBOX);
                SendMessage32A( hWndCtl, WM_SETREDRAW, 0, 0 );
                SendMessage32A( hWndCtl, WM_SETFONT, hIconTitleFont, 0 );
                while (*pstr)
          { SendMessage32A( hWndCtl, LB_ADDSTRING32, (WPARAM32)-1, (LPARAM)*pstr );
                    pstr++;
                }
                SendMessage32A( hWndCtl, WM_SETREDRAW, 1, 0 );
            }
        }
        return 1;

    case WM_PAINT:
      { RECT32 rect;
	    PAINTSTRUCT32 ps;
	    HDC32 hDC = BeginPaint32( hWnd, &ps );

	    if( __get_dropline( hWnd, &rect ) )
		GRAPH_DrawLines( hDC, (LPPOINT32)&rect, 1, GetStockObject32( BLACK_PEN ) );
	    EndPaint32( hWnd, &ps );
	}
	break;

    case WM_LBTRACKPOINT:
	hWndCtl = GetDlgItem32(hWnd, IDC_LISTBOX);
	if( (INT16)GetKeyState16( VK_CONTROL ) < 0 )
      { if( DragDetect32( hWndCtl, *((LPPOINT32)&lParam) ) )
        { INT32 idx = SendMessage32A( hWndCtl, LB_GETCURSEL32, 0, 0 );
		if( idx != -1 )
          { INT32 length = SendMessage32A( hWndCtl, LB_GETTEXTLEN32, (WPARAM32)idx, 0 );
		    HGLOBAL16 hMemObj = GlobalAlloc16( GMEM_MOVEABLE, length + 1 );
		    char* pstr = (char*)GlobalLock16( hMemObj );

		    if( pstr )
            { HCURSOR16 hCursor = LoadCursor16( 0, MAKEINTRESOURCE16(OCR_DRAGOBJECT) );
			SendMessage32A( hWndCtl, LB_GETTEXT32, (WPARAM32)idx, (LPARAM)pstr );
			SendMessage32A( hWndCtl, LB_DELETESTRING32, (WPARAM32)idx, 0 );
			UpdateWindow32( hWndCtl );
			if( !DragObject16((HWND16)hWnd, (HWND16)hWnd, DRAGOBJ_DATA, 0, (WORD)hMemObj, hCursor) )
			    SendMessage32A( hWndCtl, LB_ADDSTRING32, (WPARAM32)-1, (LPARAM)pstr );
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
        { RECT32 rect;
		if( __get_dropline( hWnd, &rect ) )
          { POINT32 pt;
	    pt.x=lpDragInfo->pt.x;
	    pt.x=lpDragInfo->pt.y;
		    rect.bottom += DROP_FIELD_HEIGHT;
		    if( PtInRect32( &rect, pt ) )
            { SetWindowLong32A( hWnd, DWL_MSGRESULT, 1 );
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

		    hWndCtl = GetDlgItem32( hWnd, IDC_WINE_TEXT );
		    SendMessage32A( hWndCtl, WM_GETTEXT, 512, (LPARAM)Template );
		    if( !lstrncmp32A( Template, "WINE", 4 ) )
			SetWindowText32A( GetDlgItem32(hWnd, IDC_STATIC_TEXT), Template );
		    else
          { char* pch = Template + strlen(Template) - strlen(__appendix_str);
			*pch = '\0';
			SendMessage32A( GetDlgItem32(hWnd, IDC_LISTBOX), LB_ADDSTRING32, 
					(WPARAM32)-1, (LPARAM)Template );
		    }

		    lstrcpy32A( Template, pstr );
		    lstrcat32A( Template, __appendix_str );
		    SetWindowText32A( hWndCtl, Template );
		    SetWindowLong32A( hWnd, DWL_MSGRESULT, 1 );
		    return TRUE;
		}
	    }
	}
	break;

    case WM_COMMAND:
        if (wParam == IDOK)
    {  EndDialog32(hWnd, TRUE);
            return TRUE;
        }
        break;
    }
    return 0;
}


/*************************************************************************
 *             ShellAbout32A   (SHELL32.243)
 */
BOOL32 WINAPI ShellAbout32A( HWND32 hWnd, LPCSTR szApp, LPCSTR szOtherStuff,
                             HICON32 hIcon )
{   ABOUT_INFO info;
    TRACE(shell,"\n");
    info.szApp        = szApp;
    info.szOtherStuff = szOtherStuff;
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    return DialogBoxIndirectParam32A( WIN_GetWindowInstance( hWnd ),
                         SYSRES_GetResPtr( SYSRES_DIALOG_SHELL_ABOUT_MSGBOX ),
                                      hWnd, AboutDlgProc32, (LPARAM)&info );
}


/*************************************************************************
 *             ShellAbout32W   (SHELL32.244)
 */
BOOL32 WINAPI ShellAbout32W( HWND32 hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff,
                             HICON32 hIcon )
{   BOOL32 ret;
    ABOUT_INFO info;

    TRACE(shell,"\n");
    
    info.szApp        = HEAP_strdupWtoA( GetProcessHeap(), 0, szApp );
    info.szOtherStuff = HEAP_strdupWtoA( GetProcessHeap(), 0, szOtherStuff );
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    ret = DialogBoxIndirectParam32A( WIN_GetWindowInstance( hWnd ),
                         SYSRES_GetResPtr( SYSRES_DIALOG_SHELL_ABOUT_MSGBOX ),
                                      hWnd, AboutDlgProc32, (LPARAM)&info );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szApp );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szOtherStuff );
    return ret;
}

/*************************************************************************
 *				Shell_NotifyIcon	[SHELL32.296]
 *	FIXME
 *	This function is supposed to deal with the systray.
 *	Any ideas on how this is to be implimented?
 */
BOOL32 WINAPI Shell_NotifyIcon(	DWORD dwMessage, PNOTIFYICONDATA pnid )
{   TRACE(shell,"\n");
    return FALSE;
}

/*************************************************************************
 *				Shell_NotifyIcon	[SHELL32.297]
 *	FIXME
 *	This function is supposed to deal with the systray.
 *	Any ideas on how this is to be implimented?
 */
BOOL32 WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATA pnid )
{   TRACE(shell,"\n");
    return FALSE;
}

/*************************************************************************
 * FreeIconList
 */
void WINAPI FreeIconList( DWORD dw )
{ FIXME(shell, "(%lx): stub\n",dw);
}

/*************************************************************************
 * SHGetPathFromIDList32A        [SHELL32.261][NT 4.0: SHELL32.220]
 *
 * PARAMETERS
 *  pidl,   [IN] pidl 
 *  pszPath [OUT] path
 *
 * RETURNS 
 *  path from a passed PIDL.
 *
 * NOTES
 *     exported by name
 *
 * FIXME
 *  fnGetDisplayNameOf can return different types of OLEString
 */
DWORD WINAPI SHGetPathFromIDList32A (LPCITEMIDLIST pidl,LPSTR pszPath)
{	STRRET lpName;
	LPSHELLFOLDER shellfolder;
	CHAR  buffer[MAX_PATH],tpath[MAX_PATH];
	DWORD type,tpathlen=MAX_PATH,dwdisp;
	HKEY  key;

	TRACE(shell,"(pidl=%p,%p)\n",pidl,pszPath);

  if (!pidl)
  {  strcpy(buffer,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\");

     if (RegCreateKeyEx32A(HKEY_CURRENT_USER,buffer,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&dwdisp))
     { return E_OUTOFMEMORY;
     }
     type=REG_SZ;    
     strcpy (buffer,"Desktop");					/*registry name*/
     if ( RegQueryValueEx32A(key,buffer,NULL,&type,(LPBYTE)tpath,&tpathlen))
     { GetWindowsDirectory32A(tpath,MAX_PATH);
       PathAddBackslash32A(tpath);
       strcat (tpath,"Desktop");				/*folder name*/
       RegSetValueEx32A(key,buffer,0,REG_SZ,(LPBYTE)tpath,tpathlen);
       CreateDirectory32A(tpath,NULL);
     }
     RegCloseKey(key);
     strcpy(pszPath,tpath);
  }
  else
  { if (SHGetDesktopFolder(&shellfolder)==S_OK)
	{ shellfolder->lpvtbl->fnGetDisplayNameOf(shellfolder,pidl,SHGDN_FORPARSING,&lpName);
	  shellfolder->lpvtbl->fnRelease(shellfolder);
	}
  /*WideCharToLocal32(pszPath, lpName.u.pOleStr, MAX_PATH);*/
	strcpy(pszPath,lpName.u.cStr);
	/* fixme free the olestring*/
  }
	TRACE(shell,"-- (%s)\n",pszPath);
	return NOERROR;
}
/*************************************************************************
 * SHGetPathFromIDList32W [SHELL32.262]
 */
DWORD WINAPI SHGetPathFromIDList32W (LPCITEMIDLIST pidl,LPWSTR pszPath)
{	char sTemp[MAX_PATH];

	TRACE (shell,"(pidl=%p)\n", pidl);

	SHGetPathFromIDList32A (pidl, sTemp);
	lstrcpyAtoW(pszPath, sTemp);

	TRACE(shell,"-- (%s)\n",debugstr_w(pszPath));

	return NOERROR;
}


void	(CALLBACK* pDLLInitComctl)(void);
INT32	(CALLBACK* pImageList_AddIcon) (HIMAGELIST himl, HICON32 hIcon);
INT32	(CALLBACK* pImageList_ReplaceIcon) (HIMAGELIST, INT32, HICON32);
HIMAGELIST (CALLBACK * pImageList_Create) (INT32,INT32,UINT32,INT32,INT32);
HICON32	(CALLBACK * pImageList_GetIcon) (HIMAGELIST, INT32, UINT32);
INT32	(CALLBACK* pImageList_GetImageCount)(HIMAGELIST);

HDPA	(CALLBACK* pDPA_Create) (INT32);  
INT32	(CALLBACK* pDPA_InsertPtr) (const HDPA, INT32, LPVOID); 
BOOL32	(CALLBACK* pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM); 
LPVOID	(CALLBACK* pDPA_GetPtr) (const HDPA, INT32);   
BOOL32	(CALLBACK* pDPA_Destroy) (const HDPA); 
INT32	(CALLBACK *pDPA_Search) (const HDPA, LPVOID, INT32, PFNDPACOMPARE, LPARAM, UINT32);

/*************************************************************************
 * SHELL32 LibMain
 *
 * FIXME
 *  at the moment the icons are extracted from shell32.dll
 *  free the imagelists
 */
HINSTANCE32 shell32_hInstance; 

BOOL32 WINAPI Shell32LibMain(HINSTANCE32 hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{ HINSTANCE32 hComctl32;

  TRACE(shell,"0x%x 0x%lx %p\n", hinstDLL, fdwReason, lpvReserved);

  shell32_hInstance = hinstDLL;
  
  if (fdwReason==DLL_PROCESS_ATTACH)
  { hComctl32 = LoadLibrary32A("COMCTL32.DLL");	
    if (hComctl32)
    { pDLLInitComctl=GetProcAddress32(hComctl32,"InitCommonControlsEx");
      if (pDLLInitComctl)
      { pDLLInitComctl();
      }
      pImageList_Create=GetProcAddress32(hComctl32,"ImageList_Create");
      pImageList_AddIcon=GetProcAddress32(hComctl32,"ImageList_AddIcon");
      pImageList_ReplaceIcon=GetProcAddress32(hComctl32,"ImageList_ReplaceIcon");
      pImageList_GetIcon=GetProcAddress32(hComctl32,"ImageList_GetIcon");
      pImageList_GetImageCount=GetProcAddress32(hComctl32,"ImageList_GetImageCount");

      /* imports by ordinal, pray that it works*/
      pDPA_Create=GetProcAddress32(hComctl32, (LPCSTR)328L);
      pDPA_Destroy=GetProcAddress32(hComctl32, (LPCSTR)329L);
      pDPA_GetPtr=GetProcAddress32(hComctl32, (LPCSTR)332L);
      pDPA_InsertPtr=GetProcAddress32(hComctl32, (LPCSTR)334L);
      pDPA_Sort=GetProcAddress32(hComctl32, (LPCSTR)338L);
      pDPA_Search=GetProcAddress32(hComctl32, (LPCSTR)339L);

      FreeLibrary32(hComctl32);
    }
    else
    { /* panic, imediately exit wine*/
      ERR(shell,"P A N I C error getting functionpointers\n");
      exit (1);
    }
    SIC_Initialize();
  }
  return TRUE;
}
