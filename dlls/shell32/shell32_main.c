/*
 * 				Shell basics
 *
 *  1998 Marcus Meissner
 *  1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
 */
#include <stdlib.h>
#include <string.h>

#include "wine/winuser16.h"
#include "winerror.h"
#include "heap.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "sysmetrics.h"
#include "debugtools.h"
#include "winreg.h"
#include "authors.h"
#include "winversion.h"

#include "shellapi.h"
#include "pidl.h"
#include "shlobj.h"
#include "shell32_main.h"

#include "shlguid.h"

DECLARE_DEBUG_CHANNEL(exec)
DECLARE_DEBUG_CHANNEL(shell)

/*************************************************************************
 * CommandLineToArgvW			[SHELL32.7]
 */
LPWSTR* WINAPI CommandLineToArgvW(LPWSTR cmdline,LPDWORD numargs)
{	LPWSTR  *argv,s,t;
	int	i;
	TRACE_(shell)("\n");

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
    FIXME_(shell)("(0x%08x, %p, %s, 0x%08lx): stub\n", hwnd, code,
          debugstr_a(cmd), arg4);
}

/*************************************************************************
 * SHGetFileInfoA			[SHELL32.254]
 */

DWORD WINAPI SHGetFileInfoA(LPCSTR path,DWORD dwFileAttributes,
                              SHFILEINFOA *psfi, UINT sizeofpsfi,
                              UINT flags )
{	CHAR		szTemp[MAX_PATH];
	LPPIDLDATA 	pData;
	LPITEMIDLIST	pPidlTemp = NULL;
	DWORD		ret=0, dwfa = dwFileAttributes;

	TRACE_(shell)("(%s,0x%lx,%p,0x%x,0x%x)\n",
	      path,dwFileAttributes,psfi,sizeofpsfi,flags);

	/* translate the pidl to a path*/
	if (flags & SHGFI_PIDL)
	{ pPidlTemp = (LPCITEMIDLIST)path;
	  SHGetPathFromIDListA (pPidlTemp, szTemp);
	  TRACE_(shell)("pidl=%p is %s\n", path, szTemp);
	}
	else
	{ strcpy(szTemp,path);
	  TRACE_(shell)("path=%s\n", szTemp);
	}

	if (flags & SHGFI_ATTRIBUTES)
	{ if (flags & SHGFI_PIDL)
	  {
	    /*
	     * We have to test for the desktop folder first because ILGetDataPointer returns
	     * NULL on the desktop folder.
	     */
	    if (_ILIsDesktop((LPCITEMIDLIST)path))
	    { psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANLINK;
	      ret = TRUE;
	    }
	    else
	    { pData = _ILGetDataPointer((LPCITEMIDLIST)path);
	      	
	      switch (pData->type)
	      { case PT_DESKTOP:
		  psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANLINK;
	        case PT_MYCOMP:
		  psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
		     SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK ;
		case PT_SPECIAL:
		  psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_CAPABILITYMASK;
		case PT_DRIVE:
		  psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM  | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
		    SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANLINK;
		case PT_FOLDER:
		  psfi->dwAttributes = SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_CAPABILITYMASK;
		case PT_VALUE:
		  psfi->dwAttributes = SFGAO_FILESYSTEM | SFGAO_CAPABILITYMASK;
	      }
	      ret=TRUE;
	    }
	  }
	  else
	  { if (! (flags & SHGFI_USEFILEATTRIBUTES))
	      dwfa = GetFileAttributesA (szTemp);

	    psfi->dwAttributes = SFGAO_FILESYSTEM;
	    if (dwfa == FILE_ATTRIBUTE_DIRECTORY) 
	      psfi->dwAttributes |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
	    ret=TRUE;
	  }
	  WARN_(shell)("file attributes, semi-stub\n");
	}

	if (flags & SHGFI_DISPLAYNAME)
	{ if (flags & SHGFI_PIDL)
	  { strcpy(psfi->szDisplayName,szTemp);
	  }
	  else
	  { strcpy(psfi->szDisplayName,path);
	  }
	  TRACE_(shell)("displayname=%s\n", psfi->szDisplayName);
	  ret=TRUE;
	}

	if (flags & SHGFI_TYPENAME)
	{ FIXME_(shell)("get the file type, stub\n");
	  strcpy(psfi->szTypeName,"FIXME: Type");
	  ret=TRUE;
	}

	if (flags & SHGFI_ICONLOCATION)
	{ FIXME_(shell)("location of icon, stub\n");
	  strcpy(psfi->szDisplayName,"");
	  ret=TRUE;
	}

	if (flags & SHGFI_EXETYPE)
	  FIXME_(shell)("type of executable, stub\n");

	if (flags & SHGFI_LINKOVERLAY)
	  FIXME_(shell)("set icon to link, stub\n");

	if (flags & SHGFI_OPENICON)
	  FIXME_(shell)("set to open icon, stub\n");

	if (flags & SHGFI_SELECTED)
	  FIXME_(shell)("set icon to selected, stub\n");

	if (flags & SHGFI_SHELLICONSIZE)
	  FIXME_(shell)("set icon to shell size, stub\n");

	if (flags & SHGFI_USEFILEATTRIBUTES)
	  FIXME_(shell)("use the dwFileAttributes, stub\n");

	if (flags & (SHGFI_UNKNOWN1 | SHGFI_UNKNOWN2 | SHGFI_UNKNOWN3))
	  FIXME_(shell)("unknown attribute!\n");
	
	if (flags & SHGFI_ICON)
	{ FIXME_(shell)("icon handle\n");
	  if (flags & SHGFI_SMALLICON)
	  { TRACE_(shell)("set to small icon\n"); 
	    psfi->hIcon=pImageList_GetIcon(ShellSmallIconList,32,ILD_NORMAL);
	    ret = (DWORD) ShellSmallIconList;
	  }
	  else
	  { TRACE_(shell)("set to big icon\n");
	    psfi->hIcon=pImageList_GetIcon(ShellBigIconList,32,ILD_NORMAL);
	    ret = (DWORD) ShellBigIconList;
	  }      
	}

	if (flags & SHGFI_SYSICONINDEX)
	{ IShellFolder * sf;
	  if (!pPidlTemp)
	  { pPidlTemp = ILCreateFromPathA (szTemp);
	  }
	  if (SUCCEEDED (SHGetDesktopFolder (&sf)))
	  { psfi->iIcon = SHMapPIDLToSystemImageListIndex (sf, pPidlTemp, 0);
	    IShellFolder_Release(sf);
	  }
	  TRACE_(shell)("-- SYSICONINDEX %i\n", psfi->iIcon);

	  if (flags & SHGFI_SMALLICON)
	  { TRACE_(shell)("set to small icon\n"); 
	    ret = (DWORD) ShellSmallIconList;
	  }
	  else        
	  { TRACE_(shell)("set to big icon\n");
	    ret = (DWORD) ShellBigIconList;
	  }
	}

	return ret;
}

/*************************************************************************
 * SHGetFileInfoW			[SHELL32.255]
 */

DWORD WINAPI SHGetFileInfoW(LPCWSTR path,DWORD dwFileAttributes,
                              SHFILEINFOW *psfi, UINT sizeofpsfi,
                              UINT flags )
{	FIXME_(shell)("(%s,0x%lx,%p,0x%x,0x%x)\n",
	      debugstr_w(path),dwFileAttributes,psfi,sizeofpsfi,flags);
	return 0;
}

/*************************************************************************
 * ExtractIconA				[SHELL32.133]
 */
HICON WINAPI ExtractIconA( HINSTANCE hInstance, LPCSTR lpszExeFileName,
	UINT nIconIndex )
{   HGLOBAL16 handle = InternalExtractIcon16(hInstance,lpszExeFileName,nIconIndex, 1);
    TRACE_(shell)("\n");
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
 */
HICON WINAPI ExtractIconW( HINSTANCE hInstance, LPCWSTR lpszExeFileName,
	UINT nIconIndex )
{ LPSTR  exefn;
  HICON  ret;
  TRACE_(shell)("\n");

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

  TRACE_(shell)("File %s, Dir %s\n", 
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

  TRACE_(shell)("returning %s\n", lpResult);
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
  FIXME_(shell)("(%p,%p,%p): stub\n", lpFile, lpDirectory, lpResult);
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
 * SHAppBarMessage32			[SHELL32.207]
 */
UINT WINAPI SHAppBarMessage(DWORD msg, PAPPBARDATA data)
{
	FIXME_(shell)("(0x%08lx,%p hwnd=0x%08x): stub\n", msg, data, data->hWnd);

	switch (msg)
	{ case ABM_GETSTATE:
		return ABS_ALWAYSONTOP | ABS_AUTOHIDE;
	  case ABM_GETTASKBARPOS:
		/* fake a taskbar on the bottom of the desktop */
		{ RECT rec;
		  GetWindowRect(GetDesktopWindow(), &rec);
		  rec.left = 0;
		  rec.top = rec.bottom - 2;
		}
		return TRUE;
	  case ABM_ACTIVATE:
	  case ABM_GETAUTOHIDEBAR:
	  case ABM_NEW:
	  case ABM_QUERYPOS:
	  case ABM_REMOVE:
	  case ABM_SETAUTOHIDEBAR:
	  case ABM_SETPOS:
	  case ABM_WINDOWPOSCHANGED:
		return FALSE;
	}
	return 0;
}


/*************************************************************************
 * SHGetDesktopFolder			[SHELL32.216]
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
{	HRESULT	hres = E_OUTOFMEMORY;
	LPCLASSFACTORY lpclf;
	TRACE_(shell)("%p->(%p)\n",shellfolder,*shellfolder);

	if (pdesktopfolder) 
	{ hres = NOERROR;
	}
	else 
	{ lpclf = IClassFactory_Constructor();
	  if(lpclf) 
	  { hres = IClassFactory_CreateInstance(lpclf,NULL,(REFIID)&IID_IShellFolder, (void*)&pdesktopfolder);
	    IClassFactory_Release(lpclf);
	  }  
	}
	
	if (pdesktopfolder) 
	{ *shellfolder = pdesktopfolder;
	  IShellFolder_AddRef(pdesktopfolder);
	} 
	else 
	{ *shellfolder=NULL;
	}

	TRACE_(shell)("-- %p->(%p)\n",shellfolder, *shellfolder);
	return hres;
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.223]
 *
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
HRESULT WINAPI SHGetSpecialFolderLocation(HWND hwndOwner, INT nFolder, LPITEMIDLIST * ppidl)
{	LPSHELLFOLDER shellfolder;
	DWORD	pchEaten, tpathlen=MAX_PATH, type, dwdisp, res, dwLastError;
	CHAR	pszTemp[256], buffer[256], tpath[MAX_PATH], npath[MAX_PATH];
	LPWSTR	lpszDisplayName = (LPWSTR)&pszTemp[0];
	HKEY	key;

	enum 
	{ FT_UNKNOWN= 0x00000000,
	  FT_DIR=     0x00000001, 
	  FT_DESKTOP= 0x00000002,
	  FT_SPECIAL= 0x00000003
	} tFolder; 

	TRACE_(shell)("(%04x,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

	strcpy(buffer,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\");

	res=RegCreateKeyExA(HKEY_CURRENT_USER,buffer,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&dwdisp);
	if (res)
	{ ERR_(shell)("Could not create key %s %08lx \n",buffer,res);
	  return E_OUTOFMEMORY;
	}

	tFolder=FT_DIR;	
	switch (nFolder)
	{ case CSIDL_BITBUCKET:
	    strcpy (buffer,"xxx");			/*not in the registry*/
	    TRACE_(shell)("looking for Recycler\n");
	    tFolder=FT_UNKNOWN;
	    break;
	  case CSIDL_CONTROLS:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE_(shell)("looking for Control\n");
	    tFolder=FT_UNKNOWN;
	    break;
	  case CSIDL_DESKTOP:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE_(shell)("looking for Desktop\n");
	    tFolder=FT_DESKTOP;			
	    break;
	  case CSIDL_DESKTOPDIRECTORY:
	  case CSIDL_COMMON_DESKTOPDIRECTORY:
	    strcpy (buffer,"Desktop");
	    break;
	  case CSIDL_DRIVES:
	    strcpy (buffer,"xxx");			/*virtual folder*/
	    TRACE_(shell)("looking for Drives\n");
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
	    TRACE_(shell)("looking for Network\n");
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
	  case CSIDL_INTERNET_CACHE:
	    strcpy (buffer,"Cache");
	    break;
	  case CSIDL_HISTORY:
	    strcpy (buffer,"History");
	    break;
	  case CSIDL_COOKIES:
	    strcpy(buffer,"Cookies");
	    break;
	  default:
	    ERR_(shell)("unknown CSIDL 0x%08x\n", nFolder);
	    tFolder=FT_UNKNOWN;			
	    break;
	}

	TRACE_(shell)("Key=%s\n",buffer);

	type=REG_SZ;

	switch (tFolder)
	{ case FT_DIR:
	    /* Directory: get the value from the registry, if its not there 
			create it and the directory*/
	    if (RegQueryValueExA(key,buffer,NULL,&type,(LPBYTE)tpath,&tpathlen))
  	    { GetWindowsDirectoryA(npath,MAX_PATH);
	      PathAddBackslashA(npath);
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
         	  CreateDirectoryA(npath,NULL);
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
         	  CreateDirectoryA(npath,NULL);
         	  strcat (npath,"\\Startup");			
         	  break;
      		case CSIDL_TEMPLATES:
         	  strcat (npath,"Templates");			
         	  break;
		case CSIDL_INTERNET_CACHE:
		  strcat(npath,"Temporary Internet Files");
		  break;
		case CSIDL_HISTORY:
		  strcat (npath,"History");
		  break;
		case CSIDL_COOKIES:
		  strcat (npath,"Cookies");
		  break;
         	default:
         	  RegCloseKey(key);
        	  return E_OUTOFMEMORY;
	      }
	      if (RegSetValueExA(key,buffer,0,REG_SZ,(LPBYTE)npath,sizeof(npath)+1))
	      { ERR_(shell)("could not create value %s\n",buffer);
	        RegCloseKey(key);
	        return E_OUTOFMEMORY;
	      }
	      TRACE_(shell)("value %s=%s created\n",buffer,npath);
	      dwLastError = GetLastError();
	      CreateDirectoryA(npath,NULL);
	      SetLastError (dwLastError);
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

	TRACE_(shell)("Value=%s\n",tpath);
	LocalToWideChar(lpszDisplayName, tpath, 256);
  
	if (SHGetDesktopFolder(&shellfolder)==S_OK)
	{ IShellFolder_ParseDisplayName(shellfolder,hwndOwner, NULL,lpszDisplayName,&pchEaten,ppidl,NULL);
	  IShellFolder_Release(shellfolder);
	}

	TRACE_(shell)("-- (new pidl %p)\n",*ppidl);
	return NOERROR;
}
/*************************************************************************
 * SHHelpShortcuts_RunDLL		[SHELL32.224]
 *
 */
DWORD WINAPI SHHelpShortcuts_RunDLL (DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{ FIXME_(exec)("(%lx, %lx, %lx, %lx) empty stub!\n",
	dwArg1, dwArg2, dwArg3, dwArg4);

  return 0;
}

/*************************************************************************
 * SHLoadInProc				[SHELL32.225]
 *
 */

DWORD WINAPI SHLoadInProc (DWORD dwArg1)
{ FIXME_(shell)("(%lx) empty stub!\n", dwArg1);
    return 0;
}

/*************************************************************************
 * ShellExecuteA			[SHELL32.245]
 */
HINSTANCE WINAPI ShellExecuteA( HWND hWnd, LPCSTR lpOperation,
                                    LPCSTR lpFile, LPCSTR lpParameters,
                                    LPCSTR lpDirectory, INT iShowCmd )
{   TRACE_(shell)("\n");
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

       FIXME_(shell)(": stub\n");
       return 0;
}

/*************************************************************************
 * AboutDlgProc32			(internal)
 */
BOOL WINAPI AboutDlgProc( HWND hWnd, UINT msg, WPARAM wParam,
                              LPARAM lParam )
{   HWND hWndCtl;
    char Template[512], AppTitle[512];

    TRACE_(shell)("\n");

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
		    if( !lstrncmpA( Template, "WINE", 4 ) )
			SetWindowTextA( GetDlgItem(hWnd, IDC_STATIC_TEXT), Template );
		    else
          { char* pch = Template + strlen(Template) - strlen(__appendix_str);
			*pch = '\0';
			SendMessageA( GetDlgItem(hWnd, IDC_LISTBOX), LB_ADDSTRING, 
					(WPARAM)-1, (LPARAM)Template );
		    }

		    lstrcpyA( Template, pstr );
		    lstrcatA( Template, __appendix_str );
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
    TRACE_(shell)("\n");

    if(!(hRes = FindResourceA(shell32_hInstance, "SHELL_ABOUT_MSGBOX", RT_DIALOGA)))
        return FALSE;
    if(!(template = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        return FALSE;

    info.szApp        = szApp;
    info.szOtherStuff = szOtherStuff;
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    return DialogBoxIndirectParamA( WIN_GetWindowInstance( hWnd ),
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

    TRACE_(shell)("\n");
    
    if(!(hRes = FindResourceA(shell32_hInstance, "SHELL_ABOUT_MSGBOX", RT_DIALOGA)))
        return FALSE;
    if(!(template = (LPVOID)LoadResource(shell32_hInstance, hRes)))
        return FALSE;

    info.szApp        = HEAP_strdupWtoA( GetProcessHeap(), 0, szApp );
    info.szOtherStuff = HEAP_strdupWtoA( GetProcessHeap(), 0, szOtherStuff );
    info.hIcon        = hIcon;
    if (!hIcon) info.hIcon = LoadIcon16( 0, MAKEINTRESOURCE16(OIC_WINEICON) );
    ret = DialogBoxIndirectParamA( WIN_GetWindowInstance( hWnd ),
                                   template, hWnd, AboutDlgProc, (LPARAM)&info );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szApp );
    HeapFree( GetProcessHeap(), 0, (LPSTR)info.szOtherStuff );
    return ret;
}

/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 *	FIXME
 *	This function is supposed to deal with the systray.
 *	Any ideas on how this is to be implimented?
 */
BOOL WINAPI Shell_NotifyIcon(	DWORD dwMessage, PNOTIFYICONDATAA pnid )
{   TRACE_(shell)("\n");
    return FALSE;
}

/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.297]
 *	FIXME
 *	This function is supposed to deal with the systray.
 *	Any ideas on how this is to be implimented?
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid )
{   TRACE_(shell)("\n");
    return FALSE;
}

/*************************************************************************
 * FreeIconList
 */
void WINAPI FreeIconList( DWORD dw )
{ FIXME_(shell)("(%lx): stub\n",dw);
}

/*************************************************************************
 * SHGetPathFromIDListA		[SHELL32.261][NT 4.0: SHELL32.220]
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
DWORD WINAPI SHGetPathFromIDListA (LPCITEMIDLIST pidl,LPSTR pszPath)
{	STRRET lpName;
	LPSHELLFOLDER shellfolder;
	CHAR  buffer[MAX_PATH],tpath[MAX_PATH];
	DWORD type,tpathlen=MAX_PATH,dwdisp;
	HKEY  key;

	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);

	if (!pidl)
	{  strcpy(buffer,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\");

	  if (RegCreateKeyExA(HKEY_CURRENT_USER,buffer,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&key,&dwdisp))
	  { return E_OUTOFMEMORY;
	  }
	  type=REG_SZ;    
	  strcpy (buffer,"Desktop");					/*registry name*/
	  if ( RegQueryValueExA(key,buffer,NULL,&type,(LPBYTE)tpath,&tpathlen))
	  { GetWindowsDirectoryA(tpath,MAX_PATH);
	    PathAddBackslashA(tpath);
	    strcat (tpath,"Desktop");				/*folder name*/
	    RegSetValueExA(key,buffer,0,REG_SZ,(LPBYTE)tpath,tpathlen);
	    CreateDirectoryA(tpath,NULL);
	  }
	  RegCloseKey(key);
	  strcpy(pszPath,tpath);
	}
	else
	{ if (SHGetDesktopFolder(&shellfolder)==S_OK)
	  { IShellFolder_GetDisplayNameOf(shellfolder,pidl,SHGDN_FORPARSING,&lpName);
	    IShellFolder_Release(shellfolder);
	  }
	  strcpy(pszPath,lpName.u.cStr);
	}
	TRACE_(shell)("-- (%s)\n",pszPath);

	return TRUE;
}
/*************************************************************************
 * SHGetPathFromIDListW 			[SHELL32.262]
 */
DWORD WINAPI SHGetPathFromIDListW (LPCITEMIDLIST pidl,LPWSTR pszPath)
{	char sTemp[MAX_PATH];

	TRACE_(shell)("(pidl=%p)\n", pidl);

	SHGetPathFromIDListA (pidl, sTemp);
	lstrcpyAtoW(pszPath, sTemp);

	TRACE_(shell)("-- (%s)\n",debugstr_w(pszPath));

	return TRUE;
}

/*************************************************************************
 * SHGetPathFromIDListAW		[SHELL32.221][NT 4.0: SHELL32.219]
 */
BOOL WINAPI SHGetPathFromIDListAW(LPCITEMIDLIST pidl,LPVOID pszPath)     
{
	TRACE_(shell)("(pidl=%p,%p)\n",pidl,pszPath);

	if (VERSION_OsIsUnicode())
	  return SHGetPathFromIDListW(pidl,pszPath);
	return SHGetPathFromIDListA(pidl,pszPath);
}

/***********************************************************************
 * DllGetVersion [COMCTL32.25]
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
	{ WARN_(shell)("wrong DLLVERSIONINFO size from app");
	  return E_INVALIDARG;
	}

	pdvi->dwMajorVersion = 4;
	pdvi->dwMinorVersion = 72;
	pdvi->dwBuildNumber = 3110;
	pdvi->dwPlatformID = 1;

	TRACE_(shell)("%lu.%lu.%lu.%lu\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

	return S_OK;
}
/*************************************************************************
 * global variables of the shell32.dll
 *
 */
void	(WINAPI* pDLLInitComctl)(LPVOID);
INT	(WINAPI* pImageList_AddIcon) (HIMAGELIST himl, HICON hIcon);
INT	(WINAPI* pImageList_ReplaceIcon) (HIMAGELIST, INT, HICON);
HIMAGELIST (WINAPI * pImageList_Create) (INT,INT,UINT,INT,INT);
BOOL	(WINAPI* pImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
HICON	(WINAPI * pImageList_GetIcon) (HIMAGELIST, INT, UINT);
INT	(WINAPI* pImageList_GetImageCount)(HIMAGELIST);

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

static BOOL		bShell32IsInitialized = 0;
static HINSTANCE	hComctl32;
static INT		shell32_RefCount = 0;

INT		shell32_ObjCount = 0;
HINSTANCE	shell32_hInstance; 
/*************************************************************************
 * SHELL32 LibMain
 *
 */

BOOL WINAPI Shell32LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{	HMODULE hUser32;

	TRACE_(shell)("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

	shell32_hInstance = hinstDLL;

	switch (fdwReason)
	{ case DLL_PROCESS_ATTACH:
	    if (!bShell32IsInitialized)
	    { hComctl32 = LoadLibraryA("COMCTL32.DLL");	
	      hUser32 = GetModuleHandleA("USER32");
	      if (hComctl32 && hUser32)
	      { pDLLInitComctl=(void*)GetProcAddress(hComctl32,"InitCommonControlsEx");
	        if (pDLLInitComctl)
	        { pDLLInitComctl(NULL);
	        }
	        pImageList_Create=(void*)GetProcAddress(hComctl32,"ImageList_Create");
	        pImageList_AddIcon=(void*)GetProcAddress(hComctl32,"ImageList_AddIcon");
	        pImageList_ReplaceIcon=(void*)GetProcAddress(hComctl32,"ImageList_ReplaceIcon");
	        pImageList_GetIcon=(void*)GetProcAddress(hComctl32,"ImageList_GetIcon");
	        pImageList_GetImageCount=(void*)GetProcAddress(hComctl32,"ImageList_GetImageCount");
	        pImageList_Draw=(void*)GetProcAddress(hComctl32,"ImageList_Draw");

	        /* imports by ordinal, pray that it works*/
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
	        pLookupIconIdFromDirectoryEx=(void*)GetProcAddress(hUser32,"LookupIconIdFromDirectoryEx");
	        pCreateIconFromResourceEx=(void*)GetProcAddress(hUser32,"CreateIconFromResourceEx");
	      }
	      else
	      { ERR_(shell)("P A N I C SHELL32 loading failed\n");
	        return FALSE;
	      }
	      SIC_Initialize();
	      bShell32IsInitialized = TRUE;
	    }
	    shell32_RefCount++;
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
	      bShell32IsInitialized = FALSE;

	      if (pdesktopfolder) 
	      { IShellFolder_Release(pdesktopfolder);
	      }

	      SIC_Destroy();
              FreeLibrary(hComctl32);

	      /* this one is here to check if AddRef/Release is balanced */
	      if (shell32_ObjCount)
	      { WARN_(shell)("leaving with %u objects left (memory leak)\n", shell32_ObjCount);
	      }
	    }
	    TRACE_(shell)("refcount=%u objcount=%u \n", shell32_RefCount, shell32_ObjCount);
	    break;	      
	}

	return TRUE;
}
