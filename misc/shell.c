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
#include "cursoricon.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

/* .ICO file ICONDIR definitions */

#pragma pack(1)

typedef struct
{
    BYTE        bWidth;          /* Width, in pixels, of the image	*/
    BYTE        bHeight;         /* Height, in pixels, of the image	*/
    BYTE        bColorCount;     /* Number of colors in image (0 if >=8bpp) */
    BYTE        bReserved;       /* Reserved ( must be 0)		*/
    WORD        wPlanes;         /* Color Planes			*/
    WORD        wBitCount;       /* Bits per pixel			*/
    DWORD       dwBytesInRes;    /* How many bytes in this resource?	*/
    DWORD       dwImageOffset;   /* Where in the file is this image?	*/
} icoICONDIRENTRY, *LPicoICONDIRENTRY;

typedef struct
{
    WORD            idReserved;   /* Reserved (must be 0)		*/
    WORD            idType;       /* Resource Type (1 for icons)	*/
    WORD            idCount;      /* How many images?			*/
    icoICONDIRENTRY idEntries[1]; /* An entry for each image (idCount of 'em) */
} icoICONDIR, *LPicoICONDIR;

#pragma pack(4)

extern HGLOBAL16 CURSORICON_LoadHandler( HGLOBAL16, HINSTANCE16, BOOL);
extern WORD 	GetIconID( HGLOBAL16 hResource, DWORD resType );

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
    if(!lpDropFileStruct) return 0;

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
    BOOL16           bRet;

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
static HINSTANCE16 SHELL_FindExecutable( LPCSTR lpFile, 
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
    HINSTANCE16 retval=31;  /* default - 'No association was found' */
    char *tok;              /* token pointer */
    int i;                  /* random counter */
    char xlpFile[256];      /* result of SearchPath */

    dprintf_exec(stddeb, "SHELL_FindExecutable: File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ) || ( lpOperation == NULL ))
    {
	/* FIXME - should throw a warning, perhaps! */
	return 2; /* File not found. Close enough, I guess. */
    }
    if (SearchPath32A(lpDirectory,lpFile,NULL,sizeof(xlpFile),xlpFile,NULL))
    	lpFile = xlpFile;
    else {
    	if (SearchPath32A(lpDirectory,lpFile,".exe",sizeof(xlpFile),xlpFile,NULL))
	    lpFile = xlpFile;
    }

    /* First thing we need is the file's extension */
    extension = strrchr( xlpFile, '.' ); /* Assume last "." is the one; */
					/* File->Run in progman uses */
					/* .\FILE.EXE :( */
    if ((extension == NULL) || (extension == &xlpFile[strlen(xlpFile)]))
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

    /* See if it's a program - if GetProfileString fails, we skip this
     * section. Actually, if GetProfileString fails, we've probably
     * got a lot more to worry about than running a program... */
    if ( GetProfileString32A("windows", "programs", "exe pif bat com",
						  buffer, sizeof(buffer)) > 0 )
	  {
		for (i=0;i<strlen(buffer); i++) buffer[i]=tolower(buffer[i]);

		tok = strtok(buffer, " \t"); /* ? */
		while( tok!= NULL)
		  {
			if (strcmp(tok, &tmpext[1])==0) /* have to skip the leading "." */
			  {
				strcpy(lpResult, xlpFile);
				/* Need to perhaps check that the file has a path
				 * attached */
				dprintf_exec(stddeb, "SHELL_FindExecutable: found %s\n",
							 lpResult);
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
	if ( GetProfileString32A( "extensions", extension, "", command,
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

    dprintf_exec(stddeb, "SHELL_FindExecutable: returning %s\n", lpResult);
    return retval;
}

/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE16 ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile,
                         LPSTR lpParameters, LPCSTR lpDirectory,
                         INT iShowCmd)
{
    HINSTANCE16 retval=31;
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
HINSTANCE16 FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    HINSTANCE16 retval=31;    /* default - 'No association was found' */

    dprintf_exec(stddeb, "FindExecutable: File %s, Dir %s\n", 
		 (lpFile != NULL?lpFile:"-"), 
		 (lpDirectory != NULL?lpDirectory:"-"));

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ))
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
LRESULT AboutDlgProc(HWND hWnd, UINT msg, WPARAM16 wParam, LPARAM lParam)
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
INT ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON16 hIcon)
{
    HGLOBAL16 handle;
    BOOL bRet;

    if (szApp) strncpy(AppName, szApp, sizeof(AppName));
    else *AppName = 0;
    AppName[sizeof(AppName)-1]=0;

    if (szOtherStuff) strncpy(AppMisc, szOtherStuff, sizeof(AppMisc));
    else *AppMisc = 0;
    AppMisc[sizeof(AppMisc)-1]=0;

    if (!hIcon) hIcon = LoadIcon16(0,MAKEINTRESOURCE(OIC_WINEICON));
    handle = SYSRES_LoadResource( SYSRES_DIALOG_SHELL_ABOUT_MSGBOX );
    if (!handle) return FALSE;
    bRet = DialogBoxIndirectParam16( WIN_GetWindowInstance( hWnd ),
                                     handle, hWnd,
                                     (DLGPROC16)MODULE_GetWndProcEntry16("AboutDlgProc"), 
                                     (LPARAM)hIcon );
    SYSRES_FreeResource( handle );
    return bRet;
}

/*************************************************************************
 *				SHELL_GetResourceTable
 *
 * FIXME: Implement GetPEResourceTable in w32sys.c and call it here.
 */
static BYTE* SHELL_GetResourceTable(HFILE hFile)
{
  struct mz_header_s mz_header;
  struct ne_header_s ne_header;
  int		size;
  
  _llseek( hFile, 0, SEEK_SET );
  if ((_lread32(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
      (mz_header.mz_magic != MZ_SIGNATURE)) return (BYTE*)-1;

  _llseek( hFile, mz_header.ne_offset, SEEK_SET );
  if (_lread32( hFile, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
      return NULL;

  if (ne_header.ne_magic == PE_SIGNATURE) 
     { fprintf(stdnimp,"Win32s FIXME: file %s line %i\n", __FILE__, __LINE__ );
       return NULL; }

  if (ne_header.ne_magic != NE_SIGNATURE) return NULL;

  size = ne_header.rname_tab_offset - ne_header.resource_tab_offset;

  if( size > sizeof(NE_TYPEINFO) )
    {
      BYTE* pTypeInfo = (BYTE*)xmalloc(size);

      if( !pTypeInfo ) return NULL;

      _llseek(hFile, mz_header.ne_offset+ne_header.resource_tab_offset, SEEK_SET);
      if( _lread32( hFile, (char*)pTypeInfo, size) != size )
	{ free(pTypeInfo); return NULL; }
      return pTypeInfo;
    }
  /* no resources */

  return NULL;
}

/*************************************************************************
 *			SHELL_LoadResource
 */
static HGLOBAL16 SHELL_LoadResource(HINSTANCE16 hInst, HFILE hFile, NE_NAMEINFO* pNInfo, WORD sizeShift)
{
 BYTE*	ptr;
 HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, (DWORD)pNInfo->length << sizeShift);

 if( (ptr = (BYTE*)GlobalLock16( handle )) )
   {
    _llseek( hFile, (DWORD)pNInfo->offset << sizeShift, SEEK_SET);
     _lread32( hFile, (char*)ptr, pNInfo->length << sizeShift);
     return handle;
   }
 return 0;
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static HGLOBAL16 ICO_LoadIcon(HINSTANCE16 hInst, HFILE hFile, LPicoICONDIRENTRY lpiIDE)
{
 BYTE*  ptr;
 HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, lpiIDE->dwBytesInRes);

 if( (ptr = (BYTE*)GlobalLock16( handle )) )
   {
    _llseek( hFile, lpiIDE->dwImageOffset, SEEK_SET);
     _lread32( hFile, (char*)ptr, lpiIDE->dwBytesInRes);
     return handle;
   }
 return 0;
}

/*************************************************************************
 *                      ICO_GetIconDirectory
 *
 *  Read .ico file and build phony ICONDIR struct for GetIconID
 */
static HGLOBAL16 ICO_GetIconDirectory(HINSTANCE16 hInst, HFILE hFile, LPicoICONDIR* lplpiID ) 
{
  WORD		id[3];	/* idReserved, idType, idCount */
  LPicoICONDIR	lpiID;
  int		i;
 
  _llseek( hFile, 0, SEEK_SET );
  if( _lread32(hFile,(char*)id,sizeof(id)) != sizeof(id) ) return 0;

  /* check .ICO header 
   *
   * - see http://www.microsoft.com/win32dev/ui/icons.htm
   */

  if( id[0] || id[1] != 1 || !id[2] ) return 0;

  i = id[2]*sizeof(icoICONDIRENTRY) + sizeof(id);

  lpiID = (LPicoICONDIR)xmalloc(i);

  if( _lread32(hFile,(char*)lpiID->idEntries,i) == i )
  {  
     HGLOBAL16 handle = DirectResAlloc( hInst, 0x10,
                                     id[2]*sizeof(ICONDIRENTRY) + sizeof(id) );
     if( handle ) 
     {
       CURSORICONDIR*     lpID = (CURSORICONDIR*)GlobalLock16( handle );
       lpID->idReserved = lpiID->idReserved = id[0];
       lpID->idType = lpiID->idType = id[1];
       lpID->idCount = lpiID->idCount = id[2];
       for( i=0; i < lpiID->idCount; i++ )
         {
	    memcpy((void*)(lpID->idEntries + i), 
		   (void*)(lpiID->idEntries + i), sizeof(ICONDIRENTRY) - 2);
	    lpID->idEntries[i].icon.wResId = i;
         }
      *lplpiID = lpiID;
       return handle;
     }
  }
  /* fail */

  free(lpiID);
  return 0;
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 */
HGLOBAL16 InternalExtractIcon(HINSTANCE16 hInstance, LPCSTR lpszExeFileName, UINT nIconIndex, WORD n )
{
  HGLOBAL16 	hRet = 0;
  HGLOBAL16*	RetPtr = NULL;
  BYTE*  	pData;
  OFSTRUCT 	ofs;
  HFILE 	hFile = OpenFile( lpszExeFileName, &ofs, OF_READ );
  
  dprintf_reg(stddeb, "InternalExtractIcon(%04x, file '%s', start from %d, extract %d\n", 
		       hInstance, lpszExeFileName, nIconIndex, n);

  if( hFile == HFILE_ERROR || !n ) return 0;

  hRet = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, sizeof(HICON16)*n);
  RetPtr = (HICON16*)GlobalLock16(hRet);

 *RetPtr = (n == 0xFFFF)? 0: 1;				/* error return values */

  pData = SHELL_GetResourceTable(hFile);
  if( pData ) 
  {
    HICON16	 hIcon = 0;
    BOOL	 icoFile = FALSE;
    UINT         iconDirCount = 0;
    UINT         iconCount = 0;
    NE_TYPEINFO* pTInfo = (NE_TYPEINFO*)(pData + 2);
    NE_NAMEINFO* pIconStorage = NULL;
    NE_NAMEINFO* pIconDir = NULL;
    LPicoICONDIR lpiID = NULL;
 
    if( pData == (BYTE*)-1 )
      {
	/* check for .ICO file */

	hIcon = ICO_GetIconDirectory(hInstance, hFile, &lpiID);
	if( hIcon )
	  { icoFile = TRUE; iconDirCount = 1; iconCount = lpiID->idCount; }
      }
    else while( pTInfo->type_id && !(pIconStorage && pIconDir) )
      {
	/* find icon directory and icon repository */

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

    if( (pIconStorage && pIconDir) || icoFile )
      if( nIconIndex == (UINT)-1 ) RetPtr[0] = iconDirCount;
      else if( nIconIndex < iconDirCount )
        {
	   UINT   i, icon;

	   if( n > iconDirCount - nIconIndex ) n = iconDirCount - nIconIndex;

	   for( i = nIconIndex; i < nIconIndex + n; i++ ) 
	     {
	       /* .ICO files have only one icon directory */

	       if( !icoFile )
	         hIcon = SHELL_LoadResource( hInstance, hFile, pIconDir + i, 
							    *(WORD*)pData );
	       RetPtr[i-nIconIndex] = GetIconID( hIcon, 3 );
	       GlobalFree16(hIcon); 
             }

	   for( icon = nIconIndex; icon < nIconIndex + n; icon++ )
	     {
	       hIcon = 0;
	       if( icoFile )
		 hIcon = ICO_LoadIcon( hInstance, hFile, lpiID->idEntries + RetPtr[icon-nIconIndex]);
	       else
	         for( i = 0; i < iconCount; i++ )
		    if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
		      hIcon = SHELL_LoadResource( hInstance, hFile, pIconStorage + i,
								     *(WORD*)pData );
	       RetPtr[icon-nIconIndex] = (hIcon)?CURSORICON_LoadHandler( hIcon, hInstance, FALSE ):0;
	     }
        }
    if( icoFile ) free(lpiID);
    else free(pData);
 } 
 _lclose( hFile );
 
  /* return array with icon handles */

  return hRet;
}

/*************************************************************************
 *				ExtractIcon 		[SHELL.34]
 */
HICON16 ExtractIcon(HINSTANCE16 hInstance, LPCSTR lpszExeFileName, WORD nIconIndex)
{
  HGLOBAL16 handle = InternalExtractIcon(hInstance,lpszExeFileName,nIconIndex, 1);

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
 *				ExtractAssociatedIcon	[SHELL.36]
 * 
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON16 ExtractAssociatedIcon(HINSTANCE16 hInst,LPSTR lpIconPath,LPWORD lpiIcon)
{
    HICON16 hIcon = ExtractIcon(hInst, lpIconPath, *lpiIcon);

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
	hIcon = LoadIcon16( hInst, MAKEINTRESOURCE(*lpiIcon));
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
	fprintf(stdnimp, "RegisterShellHook( %p ) : Empty Stub !!!\n", ptr);
	return 0;
}


/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 */
int ShellHookProc(void) 
{
	fprintf(stdnimp, "ShellHookProc : Empty Stub !!!\n");
        return 0;
}

/*************************************************************************
 *				SHGetFileInfoA		[SHELL32.54]
 */
DWORD
SHGetFileInfo32A(LPCSTR path,DWORD dwFileAttributes,SHFILEINFO32A *psfi,
	UINT32 sizeofpsfi,UINT32 flags
) {
	fprintf(stdnimp,"SHGetFileInfo32A(%s,0x%08lx,%p,%d,0x%08x)\n",
		path,dwFileAttributes,psfi,sizeofpsfi,flags
	);
	return TRUE;
}
