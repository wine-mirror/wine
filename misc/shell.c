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
UINT DragQueryFile(HDROP hDrop, WORD wFile, LPSTR lpszFile, WORD wLength)
{
    /* hDrop is a global memory block allocated with GMEM_SHARE 
     * with DROPFILESTRUCT as a header and filenames following
     * it, zero length filename is in the end */       
    
    LPDROPFILESTRUCT lpDropFileStruct;
    LPSTR lpCurrent;
    WORD  i;
    
    dprintf_reg(stddeb,"DragQueryFile(%04x, %i, %p, %u)\n",
		hDrop,wFile,lpszFile,wLength);
    
    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock(hDrop); 
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
    
    GlobalUnlock(hDrop);
    return i;
}


/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void DragFinish(HDROP h)
{
    GlobalFree((HGLOBAL)h);
}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL DragQueryPoint(HDROP hDrop, POINT FAR *p)
{
    LPDROPFILESTRUCT lpDropFileStruct;  
    BOOL             bRet;

    lpDropFileStruct = (LPDROPFILESTRUCT) GlobalLock(hDrop);

    memcpy(p,&lpDropFileStruct->ptMousePos,sizeof(POINT));
    bRet = lpDropFileStruct->fInNonClientArea;

    GlobalUnlock(hDrop);
    return bRet;
}


/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile, LPSTR lpParameters, LPCSTR lpDirectory, INT iShowCmd)
{
    char cmd[400];
    char *p,*x;
    long len;
    char subclass[200];

    /* OK. We are supposed to lookup the program associated with lpFile,
     * then to execute it using that program. If lpFile is a program,
     * we have to pass the parameters. If an instance is already running,
     * we might have to send DDE commands.
     *
     * FIXME: Should also look up WIN.INI [Extensions] section?
     */

    dprintf_exec(stddeb, "ShellExecute(%04x,'%s','%s','%s','%s',%x)\n",
		hWnd, lpOperation ? lpOperation:"<null>", lpFile ? lpFile:"<null>",
		lpParameters ? lpParameters : "<null>", 
		lpDirectory ? lpDirectory : "<null>", iShowCmd);

    if (lpFile==NULL) return 0; /* should not happen */
    if (lpOperation==NULL) /* default is open */
      lpOperation="open";
    p=strrchr(lpFile,'.');
    if (p!=NULL) {
      x=p; /* the suffixes in the register database are lowercased */
      while (*x) {*x=tolower(*x);x++;}
    }
    if (p==NULL || !strcmp(p,".exe")) {
      p=".exe";
      if (lpParameters) {
        sprintf(cmd,"%s %s",lpFile,lpParameters);
      } else {
        strcpy(cmd,lpFile);
      }
    } else {
      len=200;
      if (RegQueryValue((HKEY)HKEY_CLASSES_ROOT,p,subclass,&len)==SHELL_ERROR_SUCCESS) {
	if (len>20)
	  fprintf(stddeb,"ShellExecute:subclass with len %ld? (%s), please report.\n",len,subclass);
	subclass[len]='\0';
	strcat(subclass,"\\shell\\");
	strcat(subclass,lpOperation);
	strcat(subclass,"\\command");
	dprintf_exec(stddeb,"ShellExecute:looking for %s.\n",subclass);
	len=400;
	if (RegQueryValue((HKEY)HKEY_CLASSES_ROOT,subclass,cmd,&len)==SHELL_ERROR_SUCCESS) {
	  char *t;
	  dprintf_exec(stddeb,"ShellExecute:...got %s\n",cmd);
	  cmd[len]='\0';
	  t=strstr(cmd,"%1");
	  if (t==NULL) {
	    strcat(cmd," ");
	    strcat(cmd,lpFile);
	  } else {
	    char *s;
	    s=xmalloc(len+strlen(lpFile)+10);
	    strncpy(s,cmd,t-cmd);
	    s[t-cmd]='\0';
	    strcat(s,lpFile);
	    strcat(s,t+2);
	    strcpy(cmd,s);
	    free(s);
	  }
	  /* does this use %x magic too? */
	  if (lpParameters) {
	    strcat(cmd," ");
	    strcat(cmd,lpParameters);
	  }
	} else {
	  fprintf(stddeb,"ShellExecute: No %s\\shell\\%s\\command found for \"%s\" suffix.\n",subclass,lpOperation,p);
	  return (HINSTANCE)14; /* unknown type */
	}
      } else {
	fprintf(stddeb,"ShellExecute: No operation found for \"%s\" suffix.\n",p);
	return (HINSTANCE)14; /* file not found */
      }
    }
    dprintf_exec(stddeb,"ShellExecute:starting %s\n",cmd);
    return WinExec(cmd,iShowCmd);
}


/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	fprintf(stdnimp, "FindExecutable: someone has to fix me and this is YOUR turn! :-)\n");

	lpResult[0]='\0';
        return 31;		/* no association */
}

static char AppName[128], AppMisc[906];

/*************************************************************************
 *				AboutDlgProc		[SHELL.33]
 */
LRESULT AboutDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  char Template[512], AppTitle[512];
 
  switch(msg) {
   case WM_INITDIALOG:
#ifdef WINELIB32
    SendDlgItemMessage(hWnd,stc1,STM_SETICON,lParam,0);
#else
    SendDlgItemMessage(hWnd,stc1,STM_SETICON,LOWORD(lParam),0);
#endif
    GetWindowText(hWnd, Template, 511);
    sprintf(AppTitle, Template, AppName);
    SetWindowText(hWnd, AppTitle);
    SetWindowText(GetDlgItem(hWnd,100), AppMisc);
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
    bRet = DialogBoxIndirectParam( WIN_GetWindowInstance( hWnd ),
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

 if( (ptr = (BYTE*)GlobalLock( handle )) )
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

  hRet = GlobalAlloc( GMEM_MOVEABLE, sizeof(HICON)*n);
  RetPtr = (HICON*)GlobalLock(hRet);

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
		       hIcon = SHELL_LoadResource( hInstance, hFile, pIconDir + (i - nIconIndex), 
										 *(WORD*)pData );
		       RetPtr[i-nIconIndex] = GetIconID( hIcon, 3 );
		       GlobalFree(hIcon); 
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
      HICON* ptr = (HICON*)GlobalLock(handle);
      HICON  hIcon = *ptr;

      GlobalFree(handle);
      return hIcon;
    }
  return 0;
}

/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 */
HICON ExtractAssociatedIcon(HINSTANCE hInst,LPSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hIcon = ExtractIcon(hInst, lpIconPath, *lpiIcon);

    /* MAKEINTRESOURCE(2) seems to be "default" icon according to Progman 
     *
     * For data files it probably should call FindExecutable and load
     * icon from there. As of now FindExecutable is empty stub.
     */

    if( hIcon < 2 ) hIcon = LoadIcon( hInst, MAKEINTRESOURCE(2));

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
       if( strncasecmp(lpEnv, entry, l) ) continue;
       
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
