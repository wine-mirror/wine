/*
 * 				Shell Library Functions
 *
 *  1998 Marcus Meissner
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "wine/shell16.h"
#include "winerror.h"
#include "dlgs.h"
#include "shellapi.h"
#include "shlobj.h"
#include "debugtools.h"
#include "winreg.h"
#include "shlwapi.h"

DEFAULT_DEBUG_CHANNEL(shell);
DECLARE_DEBUG_CHANNEL(exec);


typedef struct {     /* structure for dropped files */
 WORD     wSize;
 POINT16  ptMousePos;
 BOOL16   fInNonClientArea;
 /* memory block with filenames follows */
} DROPFILESTRUCT16, *LPDROPFILESTRUCT16;

static const char*	lpstrMsgWndCreated = "OTHERWINDOWCREATED";
static const char*	lpstrMsgWndDestroyed = "OTHERWINDOWDESTROYED";
static const char*	lpstrMsgShellActivate = "ACTIVATESHELLWINDOW";

static HWND16	SHELL_hWnd = 0;
static HHOOK	SHELL_hHook = 0;
static UINT16	uMsgWndCreated = 0;
static UINT16	uMsgWndDestroyed = 0;
static UINT16	uMsgShellActivate = 0;
HINSTANCE16	SHELL_hInstance = 0;
HINSTANCE SHELL_hInstance32;
static int SHELL_Attach = 0;

/***********************************************************************
 * DllEntryPoint [SHELL.101]
 *
 * Initialization code for shell.dll. Automatically loads the
 * 32-bit shell32.dll to allow thunking up to 32-bit code.
 *
 * RETURNS:
 */
BOOL WINAPI SHELL_DllEntryPoint(DWORD Reason, HINSTANCE16 hInst,
				WORD ds, WORD HeapSize, DWORD res1, WORD res2)
{
    TRACE("(%08lx, %04x, %04x, %04x, %08lx, %04x)\n",
          Reason, hInst, ds, HeapSize, res1, res2);

    switch(Reason)
    {
    case DLL_PROCESS_ATTACH:
	if (SHELL_Attach++) break;
	SHELL_hInstance = hInst;
	if(!SHELL_hInstance32)
	{
	    if(!(SHELL_hInstance32 = LoadLibraryA("shell32.dll")))
	    {
		ERR("Could not load sibling shell32.dll\n");
		return FALSE;
	    }
	}
	break;

    case DLL_PROCESS_DETACH:
	if(!--SHELL_Attach)
	{
	    SHELL_hInstance = 0;
	    if(SHELL_hInstance32)
		FreeLibrary(SHELL_hInstance32);
	}
	break;
    }
    return TRUE;
}

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b)
{
  DragAcceptFiles(hWnd, b);
}

/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
UINT16 WINAPI DragQueryFile16(
	HDROP16 hDrop,
	WORD wFile,
	LPSTR lpszFile,
	WORD wLength)
{
 	LPSTR lpDrop;
	UINT i = 0;
	LPDROPFILESTRUCT16 lpDropFileStruct = (LPDROPFILESTRUCT16) GlobalLock16(hDrop); 
   
	TRACE("(%04x, %x, %p, %u)\n", hDrop,wFile,lpszFile,wLength);
    
	if(!lpDropFileStruct) goto end;
    
	lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->wSize;
	wFile = (wFile==0xffff) ? 0xffffffff : wFile;

	while (i++ < wFile)
	{
	  while (*lpDrop++); /* skip filename */
	  if (!*lpDrop) 
	  {
	    i = (wFile == 0xFFFFFFFF) ? i : 0; 
	    goto end;
	  }
	}
    
	i = strlen(lpDrop);
	i++;
	if (!lpszFile ) goto end;   /* needed buffer size */
	i = (wLength > i) ? i : wLength;
	lstrcpynA (lpszFile,  lpDrop,  i);
end:
	GlobalUnlock16(hDrop);
	return i;
}

/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void WINAPI DragFinish16(HDROP16 h)
{
    TRACE("\n");
    GlobalFree16((HGLOBAL16)h);
}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL16 WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p)
{
  LPDROPFILESTRUCT16 lpDropFileStruct;  
  BOOL16           bRet;
  TRACE("\n");
  lpDropFileStruct = (LPDROPFILESTRUCT16) GlobalLock16(hDrop);
  
  memcpy(p,&lpDropFileStruct->ptMousePos,sizeof(POINT16));
  bRet = lpDropFileStruct->fInNonClientArea;
  
  GlobalUnlock16(hDrop);
  return bRet;
}

/*************************************************************************
 *	SHELL_FindExecutable [Internal]
 *
 * Utility for code sharing between FindExecutable and ShellExecute
 */
HINSTANCE SHELL_FindExecutable( LPCSTR lpFile, 
                                         LPCSTR lpOperation,
                                         LPSTR lpResult)
{ char *extension = NULL; /* pointer to file extension */
    char tmpext[5];         /* local copy to mung as we please */
    char filetype[256];     /* registry name for this filetype */
    LONG filetypelen=256;   /* length of above */
    char command[256];      /* command from registry */
    LONG commandlen=256;    /* This is the most DOS can handle :) */
    char buffer[256];       /* Used to GetProfileString */
    HINSTANCE retval=31;  /* default - 'No association was found' */
    char *tok;              /* token pointer */
    int i;                  /* random counter */
    char xlpFile[256] = ""; /* result of SearchPath */

  TRACE("%s\n", (lpFile != NULL?lpFile:"-") );

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ) || ( lpOperation == NULL ))
  { WARN_(exec)("(lpFile=%s,lpResult=%s,lpOperation=%s): NULL parameter\n",
           lpFile, lpOperation, lpResult);
        return 2; /* File not found. Close enough, I guess. */
    }

    if (SearchPathA( NULL, lpFile,".exe",sizeof(xlpFile),xlpFile,NULL))
  { TRACE("SearchPathA returned non-zero\n");
        lpFile = xlpFile;
    }

    /* First thing we need is the file's extension */
    extension = strrchr( xlpFile, '.' ); /* Assume last "." is the one; */
					/* File->Run in progman uses */
					/* .\FILE.EXE :( */
  TRACE("xlpFile=%s,extension=%s\n", xlpFile, extension);

    if ((extension == NULL) || (extension == &xlpFile[strlen(xlpFile)]))
  { WARN("Returning 31 - No association\n");
        return 31; /* no association */
    }

    /* Make local copy & lowercase it for reg & 'programs=' lookup */
    lstrcpynA( tmpext, extension, 5 );
    CharLowerA( tmpext );
  TRACE("%s file\n", tmpext);
    
    /* Three places to check: */
    /* 1. win.ini, [windows], programs (NB no leading '.') */
    /* 2. Registry, HKEY_CLASS_ROOT\<filetype>\shell\open\command */
    /* 3. win.ini, [extensions], extension (NB no leading '.' */
    /* All I know of the order is that registry is checked before */
    /* extensions; however, it'd make sense to check the programs */
    /* section first, so that's what happens here. */

    /* See if it's a program - if GetProfileString fails, we skip this
     * section. Actually, if GetProfileString fails, we've probably
     * got a lot more to worry about than running a program... */
    if ( GetProfileStringA("windows", "programs", "exe pif bat com",
						  buffer, sizeof(buffer)) > 0 )
  { for (i=0;i<strlen(buffer); i++) buffer[i]=tolower(buffer[i]);

		tok = strtok(buffer, " \t"); /* ? */
		while( tok!= NULL)
		  {
			if (strcmp(tok, &tmpext[1])==0) /* have to skip the leading "." */
			  {
				strcpy(lpResult, xlpFile);
				/* Need to perhaps check that the file has a path
				 * attached */
        TRACE("found %s\n", lpResult);
        return 33;

		/* Greater than 32 to indicate success FIXME According to the
		 * docs, I should be returning a handle for the
		 * executable. Does this mean I'm supposed to open the
		 * executable file or something? More RTFM, I guess... */
			  }
			tok=strtok(NULL, " \t");
		  }
	  }

    /* Check registry */
    if (RegQueryValue16( HKEY_CLASSES_ROOT, tmpext, filetype,
                         &filetypelen ) == ERROR_SUCCESS )
    {
	filetype[filetypelen]='\0';
	TRACE("File type: %s\n", filetype);

	/* Looking for ...buffer\shell\lpOperation\command */
	strcat( filetype, "\\shell\\" );
	strcat( filetype, lpOperation );
	strcat( filetype, "\\command" );
	
	if (RegQueryValue16( HKEY_CLASSES_ROOT, filetype, command,
                             &commandlen ) == ERROR_SUCCESS )
	{
            LPSTR tmp;
            char param[256];
	    LONG paramlen = 256;


	    /* Get the parameters needed by the application 
	       from the associated ddeexec key */ 
	    tmp = strstr(filetype,"command");
	    tmp[0] = '\0';
	    strcat(filetype,"ddeexec");

	    if(RegQueryValue16( HKEY_CLASSES_ROOT, filetype, param,&paramlen ) == ERROR_SUCCESS)
	    {
	      strcat(command," ");
	      strcat(command,param);
	      commandlen += paramlen;
	    }

	    /* Is there a replace() function anywhere? */
	    command[commandlen]='\0';
	    strcpy( lpResult, command );
	    tok=strstr( lpResult, "%1" );
	    if (tok != NULL)
	    {
		tok[0]='\0'; /* truncate string at the percent */
		strcat( lpResult, xlpFile ); /* what if no dir in xlpFile? */
		tok=strstr( command, "%1" );
		if ((tok!=NULL) && (strlen(tok)>2))
		{
		    strcat( lpResult, &tok[2] );
		}
	    }
	    retval=33; /* FIXME see above */
	}
    }
    else /* Check win.ini */
    {
	/* Toss the leading dot */
	extension++;
	if ( GetProfileStringA( "extensions", extension, "", command,
                                  sizeof(command)) > 0)
	  {
		if (strlen(command)!=0)
		  {
			strcpy( lpResult, command );
			tok=strstr( lpResult, "^" ); /* should be ^.extension? */
			if (tok != NULL)
			  {
				tok[0]='\0';
				strcat( lpResult, xlpFile ); /* what if no dir in xlpFile? */
				tok=strstr( command, "^" ); /* see above */
				if ((tok != NULL) && (strlen(tok)>5))
				  {
					strcat( lpResult, &tok[5]);
				  }
			  }
			retval=33; /* FIXME - see above */
		  }
	  }
	}

    TRACE("returning %s\n", lpResult);
    return retval;
}

/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE16 WINAPI ShellExecute16( HWND16 hWnd, LPCSTR lpOperation,
                                   LPCSTR lpFile, LPCSTR lpParameters,
                                   LPCSTR lpDirectory, INT16 iShowCmd )
{   HINSTANCE16 retval=31;
    char old_dir[1024];
    char cmd[1024] = "";

    TRACE("(%04x,'%s','%s','%s','%s',%x)\n",
		hWnd, lpOperation ? lpOperation:"<null>", lpFile ? lpFile:"<null>",
		lpParameters ? lpParameters : "<null>", 
		lpDirectory ? lpDirectory : "<null>", iShowCmd);

    if (lpFile==NULL) return 0; /* should not happen */
    if (lpOperation==NULL) /* default is open */
      lpOperation="open";

    if (lpDirectory)
    { GetCurrentDirectoryA( sizeof(old_dir), old_dir );
        SetCurrentDirectoryA( lpDirectory );
    }

    /* First try to execute lpFile with lpParameters directly */ 
    strcpy(cmd,lpFile);
    strcat(cmd,lpParameters ? lpParameters : "");

    retval = WinExec16( cmd, iShowCmd );

    /* Unable to execute lpFile directly
       Check if we can match an application to lpFile */
    if(retval < 32)
    { 
      cmd[0] = '\0';
      retval = SHELL_FindExecutable( lpFile, lpOperation, cmd );

      if (retval > 32)  /* Found */
      {
        if (lpParameters)
        {
            strcat(cmd," ");
            strcat(cmd,lpParameters);
        }
        retval = WinExec16( cmd, iShowCmd );
      }
      else if(PathIsURLA((LPSTR)lpFile))    /* File not found, check for URL */
      {
        char lpstrProtocol[256];
        LONG cmdlen = 512;
        LPSTR lpstrRes;
        INT iSize;
      
	lpstrRes = strchr(lpFile,':');
        iSize = lpstrRes - lpFile;
	
        /* Looking for ...protocol\shell\lpOperation\command */
	strncpy(lpstrProtocol,lpFile,iSize);
	lpstrProtocol[iSize]='\0';
        strcat( lpstrProtocol, "\\shell\\" );
	strcat( lpstrProtocol, lpOperation );
	strcat( lpstrProtocol, "\\command" );
	
	/* Remove File Protocol from lpFile */
	/* In the case file://path/file     */
	if(!strncasecmp(lpFile,"file",iSize))
	{
	  lpFile += iSize;
	  while(*lpFile == ':') lpFile++;
	}
	

	/* Get the application for the protocol and execute it */
	if (RegQueryValue16( HKEY_CLASSES_ROOT, lpstrProtocol, cmd,
                             &cmdlen ) == ERROR_SUCCESS )
	{
	    LPSTR tok;
            LPSTR tmp;
            char param[256] = "";
	    LONG paramlen = 256;

	    /* Get the parameters needed by the application 
	       from the associated ddeexec key */ 
	    tmp = strstr(lpstrProtocol,"command");
	    tmp[0] = '\0';
	    strcat(lpstrProtocol,"ddeexec");

	    if(RegQueryValue16( HKEY_CLASSES_ROOT, lpstrProtocol, param,&paramlen ) == ERROR_SUCCESS)
	    {
	      strcat(cmd," ");
	      strcat(cmd,param);
	      cmdlen += paramlen;
	    }
	    
	    /* Is there a replace() function anywhere? */
	    cmd[cmdlen]='\0';

	    tok=strstr( cmd, "%1" );
	    if (tok != NULL)
	    {
		tok[0]='\0'; /* truncate string at the percent */
		strcat( cmd, lpFile ); /* what if no dir in xlpFile? */
		tok=strstr( cmd, "%1" );
		if ((tok!=NULL) && (strlen(tok)>2))
		{
		    strcat( cmd, &tok[2] );
		}
	    }
 
	    retval = WinExec16( cmd, iShowCmd );
	}
      }
    /* Check if file specified is in the form www.??????.*** */
      else if(!strncasecmp(lpFile,"www",3))
      {
	/* if so, append lpFile http:// and call ShellExecute */ 
        char lpstrTmpFile[256] = "http://" ;
	strcat(lpstrTmpFile,lpFile);
	retval = ShellExecuteA(hWnd,lpOperation,lpstrTmpFile,NULL,NULL,0);
      }
    }
    if (lpDirectory)
      SetCurrentDirectoryA( old_dir );
    return retval;
}

/*************************************************************************
 *             FindExecutable   (SHELL.21)
 */
HINSTANCE16 WINAPI FindExecutable16( LPCSTR lpFile, LPCSTR lpDirectory,
                                     LPSTR lpResult )
{ return (HINSTANCE16)FindExecutableA( lpFile, lpDirectory, lpResult );
}


/*************************************************************************
 *             AboutDlgProc   (SHELL.33)
 */
BOOL16 WINAPI AboutDlgProc16( HWND16 hWnd, UINT16 msg, WPARAM16 wParam,
                               LPARAM lParam )
{ return AboutDlgProc( hWnd, msg, wParam, lParam );
}


/*************************************************************************
 *             ShellAbout   (SHELL.22)
 */
BOOL16 WINAPI ShellAbout16( HWND16 hWnd, LPCSTR szApp, LPCSTR szOtherStuff,
                            HICON16 hIcon )
{ return ShellAboutA( hWnd, szApp, szOtherStuff, hIcon );
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 */
HGLOBAL16 WINAPI InternalExtractIcon16(HINSTANCE16 hInstance,
                                     LPCSTR lpszExeFileName, UINT16 nIconIndex, WORD n )
{
    HGLOBAL16 hRet = 0;
    HICON16 *RetPtr = NULL;
    OFSTRUCT ofs;
    HFILE hFile;

	TRACE("(%04x,file %s,start %d,extract %d\n", 
		       hInstance, lpszExeFileName, nIconIndex, n);

	if( !n )
	  return 0;

	hFile = OpenFile( lpszExeFileName, &ofs, OF_READ );

	hRet = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, sizeof(HICON16)*n);
	RetPtr = (HICON16*)GlobalLock16(hRet);

	if (hFile == HFILE_ERROR)
	{ /* not found - load from builtin module if available */
	  HINSTANCE hInst = (HINSTANCE)LoadLibrary16(lpszExeFileName);

	  if (hInst < 32) /* hmm, no Win16 module - try Win32 :-) */
	    hInst = LoadLibraryA(lpszExeFileName);
	  if (hInst)
	  {
	    int i;
	    for (i=nIconIndex; i < nIconIndex + n; i++)
	      RetPtr[i-nIconIndex] =
		      (HICON16)LoadIconA(hInst, (LPCSTR)(DWORD)i);
	    FreeLibrary(hInst);
	    return hRet;
	  }
          GlobalFree16( hRet );
	  return 0;
	}

        if (nIconIndex == (UINT16)-1)  /* get number of icons */
        {
            RetPtr[0] = PrivateExtractIconsA( ofs.szPathName, -1, 0, 0, NULL, 0, 0, 0 );
        }
        else
        {
            HRESULT res;
            HICON *icons;
            icons = HeapAlloc( GetProcessHeap(), 0, n * sizeof(*icons) );
            res = PrivateExtractIconsA( ofs.szPathName, nIconIndex,
                                        GetSystemMetrics(SM_CXICON),
                                        GetSystemMetrics(SM_CYICON),
                                        icons, 0, n, 0 );
            if (!res)
            {
                int i;
                for (i = 0; i < n; i++) RetPtr[i] = (HICON16)icons[i];
            }
            else
            {
                GlobalFree16( hRet );
                hRet = 0;
            }
            HeapFree( GetProcessHeap(), 0, icons );
        }
        return hRet;
}

/*************************************************************************
 *             ExtractIcon   (SHELL.34)
 */
HICON16 WINAPI ExtractIcon16( HINSTANCE16 hInstance, LPCSTR lpszExeFileName,
	UINT16 nIconIndex )
{   TRACE("\n");
    return ExtractIconA( hInstance, lpszExeFileName, nIconIndex );
}

/*************************************************************************
 *             ExtractIconEx   (SHELL.40)
 */
HICON16 WINAPI ExtractIconEx16(
	LPCSTR lpszFile, INT16 nIconIndex, HICON16 *phiconLarge,
	HICON16 *phiconSmall, UINT16 nIcons
) {
    HICON	*ilarge,*ismall;
    UINT16	ret;
    int		i;

    if (phiconLarge)
    	ilarge = (HICON*)HeapAlloc(GetProcessHeap(),0,nIcons*sizeof(HICON));
    else
    	ilarge = NULL;
    if (phiconSmall)
    	ismall = (HICON*)HeapAlloc(GetProcessHeap(),0,nIcons*sizeof(HICON));
    else
    	ismall = NULL;
    ret = ExtractIconExA(lpszFile,nIconIndex,ilarge,ismall,nIcons);
    if (ilarge) {
    	for (i=0;i<nIcons;i++)
	    phiconLarge[i]=ilarge[i];
	HeapFree(GetProcessHeap(),0,ilarge);
    }
    if (ismall) {
    	for (i=0;i<nIcons;i++)
	    phiconSmall[i]=ismall[i];
	HeapFree(GetProcessHeap(),0,ismall);
    }
    return ret;
}

/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 * 
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON16 WINAPI ExtractAssociatedIcon16(HINSTANCE16 hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{	HICON16 hIcon;
	WORD wDummyIcon = 0;

	TRACE("\n");

	if(lpiIcon == NULL)
	    lpiIcon = &wDummyIcon;

	hIcon = ExtractIcon16(hInst, lpIconPath, *lpiIcon);

	if( hIcon < 2 )
	{ if( hIcon == 1 ) /* no icons found in given file */
	  { char  tempPath[0x80];
	    UINT16  uRet = FindExecutable16(lpIconPath,NULL,tempPath);

	    if( uRet > 32 && tempPath[0] )
	    { strcpy(lpIconPath,tempPath);
	      hIcon = ExtractIcon16(hInst, lpIconPath, *lpiIcon);
	      if( hIcon > 2 ) 
	        return hIcon;
	    }
	    else hIcon = 0;
	  }

	  if( hIcon == 1 ) 
	    *lpiIcon = 2;   /* MSDOS icon - we found .exe but no icons in it */
	  else
	    *lpiIcon = 6;   /* generic icon - found nothing */

	  GetModuleFileName16(hInst, lpIconPath, 0x80);
	  hIcon = LoadIconA( hInst, MAKEINTRESOURCEA(*lpiIcon));
	}
	return hIcon;
}

/*************************************************************************
 *				ExtractAssociatedIconA (SHELL32.@)
 * 
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{	TRACE("\n");
	return ExtractAssociatedIcon16(hInst,lpIconPath,lpiIcon);
}

/*************************************************************************
 *				FindEnvironmentString	[SHELL.38]
 *
 * Returns a pointer into the DOS environment... Ugh.
 */
LPSTR SHELL_FindString(LPSTR lpEnv, LPCSTR entry)
{ UINT16 l;

  TRACE("\n");

  l = strlen(entry); 
  for( ; *lpEnv ; lpEnv+=strlen(lpEnv)+1 )
  { if( strncasecmp(lpEnv, entry, l) ) 
      continue;
	if( !*(lpEnv+l) )
	    return (lpEnv + l); 		/* empty entry */
	else if ( *(lpEnv+l)== '=' )
	    return (lpEnv + l + 1);
    }
    return NULL;
}

/**********************************************************************/

SEGPTR WINAPI FindEnvironmentString16(LPSTR str)
{ SEGPTR  spEnv;
  LPSTR lpEnv,lpString;
  TRACE("\n");
    
  spEnv = GetDOSEnvironment16();

  lpEnv = MapSL(spEnv);
  lpString = (spEnv)?SHELL_FindString(lpEnv, str):NULL; 

    if( lpString )		/*  offset should be small enough */
	return spEnv + (lpString - lpEnv);
    return (SEGPTR)NULL;
}

/*************************************************************************
 *              		DoEnvironmentSubst      [SHELL.37]
 *
 * Replace %KEYWORD% in the str with the value of variable KEYWORD
 * from "DOS" environment.
 */
DWORD WINAPI DoEnvironmentSubst16(LPSTR str,WORD length)
{
  LPSTR   lpEnv = MapSL(GetDOSEnvironment16());
  LPSTR   lpBuffer = (LPSTR)HeapAlloc( GetProcessHeap(), 0, length);
  LPSTR   lpstr = str;
  LPSTR   lpbstr = lpBuffer;

  CharToOemA(str,str);

  TRACE("accept %s\n", str);

  while( *lpstr && lpbstr - lpBuffer < length )
   {
     LPSTR lpend = lpstr;

     if( *lpstr == '%' )
       {
	  do { lpend++; } while( *lpend && *lpend != '%' );
	  if( *lpend == '%' && lpend - lpstr > 1 )	/* found key */
	    {
	       LPSTR lpKey;
	      *lpend = '\0';  
	       lpKey = SHELL_FindString(lpEnv, lpstr+1);
	       if( lpKey )				/* found key value */
		 {
		   int l = strlen(lpKey);

		   if( l > length - (lpbstr - lpBuffer) - 1 )
		     {
           WARN("-- Env subst aborted - string too short\n");
		      *lpend = '%';
		       break;
		     }
		   strcpy(lpbstr, lpKey);
		   lpbstr += l;
		 }
	       else break;
	      *lpend = '%';
	       lpstr = lpend + 1;
	    }
	  else break;					/* back off and whine */

	  continue;
       } 

     *lpbstr++ = *lpstr++;
   }

 *lpbstr = '\0';
  if( lpstr - str == strlen(str) )
    {
      strncpy(str, lpBuffer, length);
      length = 1;
    }
  else
      length = 0;

  TRACE("-- return %s\n", str);

  OemToCharA(str,str);
  HeapFree( GetProcessHeap(), 0, lpBuffer);

  /*  Return str length in the LOWORD
   *  and 1 in HIWORD if subst was successful.
   */
 return (DWORD)MAKELONG(strlen(str), length);
}

/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 * System-wide WH_SHELL hook.
 */
LRESULT WINAPI ShellHookProc16(INT16 code, WPARAM16 wParam, LPARAM lParam)
{
    TRACE("%i, %04x, %08x\n", code, wParam, 
						      (unsigned)lParam );
    if( SHELL_hHook && SHELL_hWnd )
    {
	UINT16	uMsg = 0;
        switch( code )
        {
	    case HSHELL_WINDOWCREATED:		uMsg = uMsgWndCreated;   break;
	    case HSHELL_WINDOWDESTROYED:	uMsg = uMsgWndDestroyed; break;
	    case HSHELL_ACTIVATESHELLWINDOW: 	uMsg = uMsgShellActivate;
        }
	PostMessageA( SHELL_hWnd, uMsg, wParam, 0 );
    }
    return CallNextHookEx16( WH_SHELL, code, wParam, lParam );
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
BOOL WINAPI RegisterShellHook16(HWND16 hWnd, UINT16 uAction)
{ 
    TRACE("%04x [%u]\n", hWnd, uAction );

    switch( uAction )
    { 
    case 2:  /* register hWnd as a shell window */
        if( !SHELL_hHook )
        { 
            HMODULE16 hShell = GetModuleHandle16( "SHELL" );
            HOOKPROC16 hookProc = (HOOKPROC16)GetProcAddress16( hShell, (LPCSTR)103 );
            SHELL_hHook = SetWindowsHookEx16( WH_SHELL, hookProc, hShell, 0 );
            if ( SHELL_hHook )
            { 
                uMsgWndCreated = RegisterWindowMessageA( lpstrMsgWndCreated );
                uMsgWndDestroyed = RegisterWindowMessageA( lpstrMsgWndDestroyed );
                uMsgShellActivate = RegisterWindowMessageA( lpstrMsgShellActivate );
            } 
            else 
                WARN("-- unable to install ShellHookProc()!\n");
        }

        if ( SHELL_hHook )
            return ((SHELL_hWnd = hWnd) != 0);
        break;

    default:
        WARN("-- unknown code %i\n", uAction );
        SHELL_hWnd = 0; /* just in case */
    }
    return FALSE;
}


/***********************************************************************
 *           DriveType   (SHELL.262)
 */
UINT16 WINAPI DriveType16( UINT16 drive )
{
    UINT ret;
    char path[] = "A:\\";
    path[0] += drive;
    ret = GetDriveTypeA(path);
    switch(ret)  /* some values are not supported in Win16 */
    {
    case DRIVE_CDROM:
        ret = DRIVE_REMOTE;
        break;
    case DRIVE_NO_ROOT_DIR:
        ret = DRIVE_UNKNOWN;
        break;
    }
    return ret;
}
