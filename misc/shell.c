/*
 * 				Shell Library Functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "file.h"
#include "shell.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

extern HANDLE 	CURSORICON_LoadHandler( HANDLE, HINSTANCE, BOOL);
extern WORD 	GetIconID( HANDLE hResource, DWORD resType );

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void DragAcceptFiles(HWND hWnd, BOOL b)
{
    WND* wnd = WIN_FindWndPtr(hWnd);

    if( wnd )
	wnd->dwExStyle = b? wnd->dwExStyle | WS_EX_ACCEPTFILES
			  : wnd->dwExStyle & ~WS_EX_ACCEPTFILES;
}


/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
UINT DragQueryFile(HDROP16 hDrop, WORD wFile, LPSTR lpszFile, WORD wLength)
{
    /* hDrop is a global memory block allocated with GMEM_SHARE 
     * with DROPFILESTRUCT as a header and filenames following
     * it, zero length filename is in the end */       
    
    LPDROPFILESTRUCT lpDropFileStruct;
    LPSTR lpCurrent;
    WORD  i;
    
    dprintf_reg(stddeb,"DragQueryFile(%04x, %i, %p, %u)\n",
		hDrop,wFile,lpszFile,wLength);
    
    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock16(hDrop); 
    if(!lpDropFileStruct)
    {
	dprintf_reg(stddeb,"DragQueryFile: unable to lock handle!\n");
	return 0;
    } 
    lpCurrent = (LPSTR) lpDropFileStruct + lpDropFileStruct->wSize;
    
    i = 0;
    while (i++ < wFile)
    {
	while (*lpCurrent++);  /* skip filename */
	if (!*lpCurrent) 
	    return (wFile == 0xFFFF) ? i : 0;  
    }
    
    i = strlen(lpCurrent); 
    if (!lpszFile) return i+1;   /* needed buffer size */
    
    i = (wLength > i) ? i : wLength-1;
    strncpy(lpszFile, lpCurrent, i);
    lpszFile[i] = '\0';
    
    GlobalUnlock16(hDrop);
    return i;
}


/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void DragFinish(HDROP16 h)
{
    GlobalFree16((HGLOBAL16)h);
}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL DragQueryPoint(HDROP16 hDrop, POINT16 *p)
{
    LPDROPFILESTRUCT lpDropFileStruct;  
    BOOL             bRet;

    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock16(hDrop);

    memcpy(p,&lpDropFileStruct->ptMousePos,sizeof(POINT16));
    bRet = lpDropFileStruct->fInNonClientArea;

    GlobalUnlock16(hDrop);
    return bRet;
}

/*************************************************************************
 *				SHELL_FindExecutable
 * Utility for code sharing between FindExecutable and ShellExecute
 */
static HINSTANCE SHELL_FindExecutable( LPCSTR lpFile, 
				      LPCSTR lpDirectory,
				      LPCSTR lpOperation,
				      LPSTR lpResult)
{
    char *extension = NULL; /* pointer to file extension */
    char tmpext[5];         /* local copy to mung as we please */
    char filetype[256];     /* registry name for this filetype */
    LONG filetypelen=256;   /* length of above */
    char command[256];      /* command from registry */
    LONG commandlen=256;    /* This is the most DOS can handle :) */
    char buffer[256];       /* Used to GetProfileString */
    HINSTANCE retval=31;    /* default - 'No association was found' */
    char *tok;              /* token pointer */
    int i;                  /* random counter */

    dprintf_exec(stddeb, "SHELL_FindExecutable: File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpDirectory == NULL ) || 
	( lpResult == NULL ) || ( lpOperation == NULL ))
    {
	/* FIXME - should throw a warning, perhaps! */
	return 2; /* File not found. Close enough, I guess. */
    }

    /* First thing we need is the file's extension */
    extension = strrchr( lpFile, '.' ); /* Assume last "." is the one; */
					/* File->Run in progman uses */
					/* .\FILE.EXE :( */
    if ((extension == NULL) || (extension == &lpFile[strlen(lpFile)]))
    {
	return 31; /* no association */
    }

    /* Make local copy & lowercase it for reg & 'programs=' lookup */
    strncpy( tmpext, extension, 5 );
    if (strlen(extension)<=4)
	tmpext[strlen(extension)]='\0';
    else
	tmpext[4]='\0';
    for (i=0;i<strlen(tmpext);i++) tmpext[i]=tolower(tmpext[i]);
    dprintf_exec(stddeb, "SHELL_FindExecutable: %s file\n", tmpext);
    
    /* Three places to check: */
    /* 1. win.ini, [windows], programs (NB no leading '.') */
    /* 2. Registry, HKEY_CLASS_ROOT\<filetype>\shell\open\command */
    /* 3. win.ini, [extensions], extension (NB no leading '.' */
    /* All I know of the order is that registry is checked before */
    /* extensions; however, it'd make sense to check the programs */
    /* section first, so that's what happens here. */

    /* See if it's a program */
    GetProfileString("windows", "programs", "exe pif bat com",
		      buffer, sizeof(buffer)); /* FIXME check return code! */

    for (i=0;i<strlen(buffer); i++) buffer[i]=tolower(buffer[i]);

    tok = strtok(buffer, " \t"); /* ? */
    while( tok!= NULL)
    {
	if (strcmp(tok, &tmpext[1])==0) /* have to skip the leading "." */
	{
	    strcpy(lpResult, lpFile); /* Need to perhaps check that */
				      /* the file has a path attached */
	    dprintf_exec(stddeb, "SHELL_FindExecutable: found %s\n",
			 lpResult);
	    return 33; /* Greater than 32 to indicate success FIXME */
		       /* According to the docs, I should be returning */
		       /* a handle for the executable. Does this mean */
		       /* I'm supposed to open the executable file or */
		       /* something? More RTFM, I guess... */
	}
	tok=strtok(NULL, " \t");
    }

    /* Check registry */
    if (RegQueryValue16( (HKEY)HKEY_CLASSES_ROOT, tmpext, filetype,
                         &filetypelen ) == SHELL_ERROR_SUCCESS )
    {
	filetype[filetypelen]='\0';
	dprintf_exec(stddeb, "SHELL_FindExecutable: File type: %s\n",
		     filetype);

	/* Looking for ...buffer\shell\lpOperation\command */
	strcat( filetype, "\\shell\\" );
	strcat( filetype, lpOperation );
	strcat( filetype, "\\command" );
	
	if (RegQueryValue16( (HKEY)HKEY_CLASSES_ROOT, filetype, command,
                             &commandlen ) == SHELL_ERROR_SUCCESS )
	{
	    /* Is there a replace() function anywhere? */
	    command[commandlen]='\0';
	    strcpy( lpResult, command );
	    tok=strstr( lpResult, "%1" );
	    if (tok != NULL)
	    {
		tok[0]='\0'; /* truncate string at the percent */
		strcat( lpResult, lpFile ); /* what if no dir in lpFile? */
		tok=strstr( command, "%1" );
		if ((tok!=NULL) && (strlen(tok)>2))
		{
		    strcat( lpResult, &tok[2] );
		}
	    }
	    retval=33;
	}
    }
    else /* Check win.ini */
    {
	/* Toss the leading dot */
	extension++;
	GetProfileString( "extensions", extension, "", command,
			 sizeof(command));
	if (strlen(command)!=0)
	{
	    strcpy( lpResult, command );
	    tok=strstr( lpResult, "^" ); /* should be ^.extension? */
	    if (tok != NULL)
	    {
		tok[0]='\0';
		strcat( lpResult, lpFile ); /* what if no dir in lpFile? */
		tok=strstr( command, "^" ); /* see above */
		if ((tok != NULL) && (strlen(tok)>5))
		{
		    strcat( lpResult, &tok[5]);
		}
	    }
	    retval=33;
	}
    }

    dprintf_exec(stddeb, "SHELL_FindExecutable: returning %s\n", lpResult);
    return retval;
}

/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile,
		       LPSTR lpParameters, LPCSTR lpDirectory,
		       INT iShowCmd)
{
    HINSTANCE retval=31;
    char cmd[256];

    dprintf_exec(stddeb, "ShellExecute(%04x,'%s','%s','%s','%s',%x)\n",
		hWnd, lpOperation ? lpOperation:"<null>", lpFile ? lpFile:"<null>",
		lpParameters ? lpParameters : "<null>", 
		lpDirectory ? lpDirectory : "<null>", iShowCmd);

    if (lpFile==NULL) return 0; /* should not happen */
    if (lpOperation==NULL) /* default is open */
      lpOperation="open";

    retval = SHELL_FindExecutable( lpFile, lpDirectory, lpOperation, cmd );

    if ( retval <= 32 )
    {
	return retval;
    }

    if (lpParameters)
    {
	strcat(cmd," ");
	strcat(cmd,lpParameters);
    }

    dprintf_exec(stddeb,"ShellExecute:starting %s\n",cmd);
    return WinExec(cmd,iShowCmd);
}

/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    HINSTANCE retval=31;    /* default - 'No association was found' */

    dprintf_exec(stddeb, "FindExecutable: File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpDirectory == NULL ) || 
	( lpResult == NULL ))
    {
	/* FIXME - should throw a warning, perhaps! */
	return 2; /* File not found. Close enough, I guess. */
    }

    retval = SHELL_FindExecutable( lpFile, lpDirectory, "open",
				  lpResult );

    dprintf_exec(stddeb, "FindExecutable: returning %s\n", lpResult);
    return retval;
}

static char AppName[128], AppMisc[1536];

/*************************************************************************
 *				AboutDlgProc		[SHELL.33]
 */
LRESULT AboutDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  char Template[512], AppTitle[512];
 
  switch(msg) {
   case WM_INITDIALOG:
    SendDlgItemMessage32A(hWnd,stc1,STM_SETICON,lParam,0);
    GetWindowText32A(hWnd, Template, sizeof(Template));
    sprintf(AppTitle, Template, AppName);
    SetWindowText32A(hWnd, AppTitle);
    SetWindowText32A(GetDlgItem(hWnd,100), AppMisc);
    return 1;
    
   case WM_COMMAND:
    switch (wParam) {
     case IDOK:
      EndDialog(hWnd, TRUE);
      return TRUE;
    }
    break;
  }
  return FALSE;
}

/*************************************************************************
 *				ShellAbout		[SHELL.22]
 */
INT ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon)
{
    HANDLE handle;
    BOOL bRet;

    if (szApp) strncpy(AppName, szApp, sizeof(AppName));
    else *AppName = 0;
    AppName[sizeof(AppName)-1]=0;

    if (szOtherStuff) strncpy(AppMisc, szOtherStuff, sizeof(AppMisc));
    else *AppMisc = 0;
    AppMisc[sizeof(AppMisc)-1]=0;

    if (!hIcon) hIcon = LoadIcon(0,MAKEINTRESOURCE(OIC_WINEICON));
    handle = SYSRES_LoadResource( SYSRES_DIALOG_SHELL_ABOUT_MSGBOX );
    if (!handle) return FALSE;
    bRet = DialogBoxIndirectParam16( WIN_GetWindowInstance( hWnd ),
                                   handle, hWnd,
                                   MODULE_GetWndProcEntry16("AboutDlgProc"), 
				   (LONG)hIcon );
    SYSRES_FreeResource( handle );
    return bRet;
}

/*************************************************************************
 *				SHELL_GetResourceTable
 *
 * FIXME: Implement GetPEResourceTable in w32sys.c and call it here.
 */
BYTE* SHELL_GetResourceTable(HFILE hFile)
{
  struct mz_header_s mz_header;
  struct ne_header_s ne_header;
  int		size;
  
  _llseek( hFile, 0, SEEK_SET );
  if ((FILE_Read(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
      (mz_header.mz_magic != MZ_SIGNATURE)) return (BYTE*)-1;

  _llseek( hFile, mz_header.ne_offset, SEEK_SET );
  if (FILE_Read( hFile, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
      return NULL;

  if (ne_header.ne_magic == PE_SIGNATURE) 
     { fprintf(stdnimp,"Win32 FIXME: file %s line %i\n", __FILE__, __LINE__ );
       return NULL; }

  if (ne_header.ne_magic != NE_SIGNATURE) return NULL;

  size = ne_header.rname_tab_offset - ne_header.resource_tab_offset;

  if( size > sizeof(NE_TYPEINFO) )
    {
      BYTE* pTypeInfo = (BYTE*)xmalloc(size);

      if( !pTypeInfo ) return NULL;

      _llseek(hFile, mz_header.ne_offset+ne_header.resource_tab_offset, SEEK_SET);
      if( FILE_Read( hFile, (char*)pTypeInfo, size) != size )
	{ free(pTypeInfo); return NULL; }
      return pTypeInfo;
    }
  /* no resources */

  return NULL;
}

/*************************************************************************
 *			SHELL_LoadResource
 */
HANDLE	SHELL_LoadResource(HINSTANCE hInst, HFILE hFile, NE_NAMEINFO* pNInfo, WORD sizeShift)
{
 BYTE*	ptr;
 HANDLE handle = DirectResAlloc( hInst, 0x10, (DWORD)pNInfo->length << sizeShift);

 if( (ptr = (BYTE*)GlobalLock16( handle )) )
   {
    _llseek( hFile, (DWORD)pNInfo->offset << sizeShift, SEEK_SET);
     FILE_Read( hFile, (char*)ptr, pNInfo->length << sizeShift);
     return handle;
   }
 return (HANDLE)0;
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 */
HICON InternalExtractIcon(HINSTANCE hInstance, LPCSTR lpszExeFileName, UINT nIconIndex, WORD n )
{
  HANDLE 	hRet = 0;
  HICON*	RetPtr = NULL;
  BYTE*  	pData;
  OFSTRUCT 	ofs;
  HFILE 	hFile = OpenFile( lpszExeFileName, &ofs, OF_READ );
  
  dprintf_reg(stddeb, "InternalExtractIcon(%04x, file '%s', start from %d, extract %d\n", 
		       hInstance, lpszExeFileName, nIconIndex, n);

  if( hFile == HFILE_ERROR || !n ) return 0;

  hRet = GlobalAlloc16( GMEM_FIXED, sizeof(HICON)*n);
  RetPtr = (HICON*)GlobalLock16(hRet);

 *RetPtr = (n == 0xFFFF)? 0: 1;				/* error return values */

  pData = SHELL_GetResourceTable(hFile);
  if( pData ) 
    if( pData == (BYTE*)-1 )
      {
	/* FIXME: possible .ICO file */

	fprintf(stddeb,"InternalExtractIcon: cannot handle file %s\n", lpszExeFileName);
      }
    else						/* got resource table */
      {
	UINT	     iconDirCount = 0;
	UINT	     iconCount = 0;
	NE_TYPEINFO* pTInfo = (NE_TYPEINFO*)(pData + 2);
	NE_NAMEINFO* pIconStorage = NULL;
	NE_NAMEINFO* pIconDir = NULL;

	/* find icon directory and icon repository */

        while( pTInfo->type_id && !(pIconStorage && pIconDir) )
	  {
	   if( pTInfo->type_id == NE_RSCTYPE_GROUP_ICON ) 
	       {
		 iconDirCount = pTInfo->count;
	         pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
		 dprintf_reg(stddeb,"\tfound directory - %i icon families\n", iconDirCount);
	       }
	   if( pTInfo->type_id == NE_RSCTYPE_ICON ) 
	       { 
		 iconCount = pTInfo->count;
		 pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
		 dprintf_reg(stddeb,"\ttotal icons - %i\n", iconCount);
	       }
  	   pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1)+pTInfo->count*sizeof(NE_NAMEINFO));
          }

	/* load resources and create icons */

        if( pIconStorage && pIconDir )

            if( nIconIndex == (UINT)-1 ) RetPtr[0] = iconDirCount;
	    else if( nIconIndex < iconDirCount )
	      {
		  HANDLE hIcon;
		  UINT   i, icon;

		  if( n > iconDirCount - nIconIndex ) n = iconDirCount - nIconIndex;

		  for( i = nIconIndex; i < nIconIndex + n; i++ ) 
		     {
		       hIcon = SHELL_LoadResource( hInstance, hFile, pIconDir + i, 
								  *(WORD*)pData );
		       RetPtr[i-nIconIndex] = GetIconID( hIcon, 3 );
		       GlobalFree16(hIcon); 
		     }

		  for( icon = nIconIndex; icon < nIconIndex + n; icon++ )
		     {
		       hIcon = 0;
		       for( i = 0; i < iconCount; i++ )
			  if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
			      hIcon = SHELL_LoadResource( hInstance, hFile, pIconStorage + i,
									     *(WORD*)pData );
	               RetPtr[icon-nIconIndex] = (hIcon)?CURSORICON_LoadHandler( hIcon, hInstance, FALSE ):0;
		     }
	      }
	free(pData);
      }

 _lclose( hFile );
 
  /* return array with icon handles */

  return hRet;
}

/*************************************************************************
 *				ExtractIcon 		[SHELL.34]
 */
HICON ExtractIcon(HINSTANCE hInstance, LPCSTR lpszExeFileName, WORD nIconIndex)
{
  HANDLE handle = InternalExtractIcon(hInstance,lpszExeFileName,nIconIndex, 1);

  if( handle )
    {
      HICON* ptr = (HICON*)GlobalLock16(handle);
      HICON  hIcon = *ptr;

      GlobalFree16(handle);
      return hIcon;
    }
  return 0;
}

/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 * 
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON ExtractAssociatedIcon(HINSTANCE hInst,LPSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hIcon = ExtractIcon(hInst, lpIconPath, *lpiIcon);

    if( hIcon < 2 )
      {

	if( hIcon == 1 ) /* no icons found in given file */
	  {
	    char  tempPath[0x80];
	    UINT  uRet = FindExecutable(lpIconPath,NULL,tempPath);

	    if( uRet > 32 && tempPath[0] )
	      {
		strcpy(lpIconPath,tempPath);
	        hIcon = ExtractIcon(hInst, lpIconPath, *lpiIcon);

		if( hIcon > 2 ) return hIcon;
	      }
	    else hIcon = 0;
	  }
	
	if( hIcon == 1 ) 
	  *lpiIcon = 2;   /* MSDOS icon - we found .exe but no icons in it */
	else
	  *lpiIcon = 6;   /* generic icon - found nothing */

        GetModuleFileName(hInst, lpIconPath, 0x80);
	hIcon = LoadIcon( hInst, MAKEINTRESOURCE(*lpiIcon));
      }

    return hIcon;
}

/*************************************************************************
 *				FindEnvironmentString	[SHELL.38]
 *
 * Returns a pointer into the DOS environment... Ugh.
 */
LPSTR SHELL_FindString(LPSTR lpEnv, LPCSTR entry)
{
  UINT 	l = strlen(entry); 
  for( ; *lpEnv ; lpEnv+=strlen(lpEnv)+1 )
     {
       if( lstrncmpi32A(lpEnv, entry, l) ) continue;
       
       if( !*(lpEnv+l) )
         return (lpEnv + l); 		/* empty entry */
       else if ( *(lpEnv+l)== '=' )
	 return (lpEnv + l + 1);
     }
  return NULL;
}

SEGPTR FindEnvironmentString(LPSTR str)
{
 SEGPTR  spEnv = GetDOSEnvironment();
 LPSTR  lpEnv = (LPSTR)PTR_SEG_TO_LIN(spEnv);
 
 LPSTR  lpString = (spEnv)?SHELL_FindString(lpEnv, str):NULL; 

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
DWORD DoEnvironmentSubst(LPSTR str,WORD length)
{
  LPSTR   lpEnv = (LPSTR)PTR_SEG_TO_LIN(GetDOSEnvironment());
  LPSTR   lpBuffer = (LPSTR)xmalloc(length);
  LPSTR   lpstr = str;
  LPSTR   lpbstr = lpBuffer;

  AnsiToOem(str,str);

  dprintf_reg(stddeb,"DoEnvSubst: accept %s", str);

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
		       fprintf(stdnimp,"File %s, line %i: Env subst aborted - string too short\n", 
					__FILE__, __LINE__);
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

  dprintf_reg(stddeb," return %s\n", str);

  OemToAnsi(str,str);
  free(lpBuffer);

  /*  Return str length in the LOWORD
   *  and 1 in HIWORD if subst was successful.
   */
 return (DWORD)MAKELONG(strlen(str), length);
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
int RegisterShellHook(void *ptr) 
{
	dprintf_reg(stdnimp, "RegisterShellHook : Empty Stub !!!\n");
	return 0;
}


/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 */
int ShellHookProc(void) 
{
	dprintf_reg(stdnimp, "ShellHookProc : Empty Stub !!!\n");
        return 0;
}
