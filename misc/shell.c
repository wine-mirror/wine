/*
 * 				Shell Library Functions
 *
 *  1998 Marcus Meissner
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
#include "commctrl.h"

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

static const char*	lpstrMsgWndCreated = "OTHERWINDOWCREATED";
static const char*	lpstrMsgWndDestroyed = "OTHERWINDOWDESTROYED";
static const char*	lpstrMsgShellActivate = "ACTIVATESHELLWINDOW";

static HWND16	SHELL_hWnd = 0;
static HHOOK	SHELL_hHook = 0;
static UINT16	uMsgWndCreated = 0;
static UINT16	uMsgWndDestroyed = 0;
static UINT16	uMsgShellActivate = 0;

/*************************************************************************
 *				DragAcceptFiles32		[SHELL32.54]
 */
void WINAPI DragAcceptFiles32(HWND32 hWnd, BOOL32 b)
{
  WND* wnd = WIN_FindWndPtr(hWnd);
  
  if( wnd )
    wnd->dwExStyle = b? wnd->dwExStyle | WS_EX_ACCEPTFILES
                      : wnd->dwExStyle & ~WS_EX_ACCEPTFILES;
}

/*************************************************************************
 *				DragAcceptFiles16		[SHELL.9]
 */
void WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b)
{
  DragAcceptFiles32(hWnd, b);
}

/*************************************************************************
 *				SHELL_DragQueryFile	[internal]
 * 
 */
static UINT32 SHELL_DragQueryFile(LPSTR lpDrop, LPWSTR lpwDrop, UINT32 lFile, 
				  LPSTR lpszFile, LPWSTR lpszwFile, UINT32 lLength)
{
    UINT32 i;

    i = 0;
    if (lpDrop) {
    while (i++ < lFile) {
	while (*lpDrop++); /* skip filename */
	if (!*lpDrop) 
	  return (lFile == 0xFFFFFFFF) ? i : 0;  
      }
    }
    if (lpwDrop) {
      while (i++ < lFile) {
	while (*lpwDrop++); /* skip filename */
	if (!*lpwDrop) 
	  return (lFile == 0xFFFFFFFF) ? i : 0;  
      }
    }
    
    if (lpDrop)  i = lstrlen32A(lpDrop);
    if (lpwDrop) i = lstrlen32W(lpwDrop);
    i++;
    if (!lpszFile && !lpszwFile) {
      return i;   /* needed buffer size */
    }
    i = (lLength > i) ? i : lLength;
    if (lpszFile) {
      if (lpDrop) lstrcpyn32A (lpszFile,  lpDrop,  i);
      else        lstrcpynWtoA(lpszFile,  lpwDrop, i);
    } else {
      if (lpDrop) lstrcpynAtoW(lpszwFile, lpDrop,  i);
      else        lstrcpyn32W (lpszwFile, lpwDrop, i);
    }
    return i;
}

/*************************************************************************
 *				DragQueryFile32A	[SHELL32.81] [shell32.82]
 */
UINT32 WINAPI DragQueryFile32A(HDROP32 hDrop, UINT32 lFile, LPSTR lpszFile,
			      UINT32 lLength)
{ /* hDrop is a global memory block allocated with GMEM_SHARE 
     * with DROPFILESTRUCT as a header and filenames following
     * it, zero length filename is in the end */       
    
    LPDROPFILESTRUCT32 lpDropFileStruct;
    LPSTR lpCurrent;
    UINT32 i;
    
    TRACE(shell,"(%08x, %x, %p, %u)\n",	hDrop,lFile,lpszFile,lLength);
    
    lpDropFileStruct = (LPDROPFILESTRUCT32) GlobalLock32(hDrop); 
    if(!lpDropFileStruct)
      return 0;

    lpCurrent = (LPSTR) lpDropFileStruct + lpDropFileStruct->lSize;
    i = SHELL_DragQueryFile(lpCurrent, NULL, lFile, lpszFile, NULL, lLength);
    GlobalUnlock32(hDrop);
    return i;
}

/*************************************************************************
 *				DragQueryFile32W	[shell32.133]
 */
UINT32 WINAPI DragQueryFile32W(HDROP32 hDrop, UINT32 lFile, LPWSTR lpszwFile,
			      UINT32 lLength)
{
    LPDROPFILESTRUCT32 lpDropFileStruct;
    LPWSTR lpwCurrent;
    UINT32 i;
    
    TRACE(shell,"(%08x, %x, %p, %u)\n",	hDrop,lFile,lpszwFile,lLength);
    
    lpDropFileStruct = (LPDROPFILESTRUCT32) GlobalLock32(hDrop); 
    if(!lpDropFileStruct)
      return 0;

    lpwCurrent = (LPWSTR) lpDropFileStruct + lpDropFileStruct->lSize;
    i = SHELL_DragQueryFile(NULL, lpwCurrent, lFile, NULL, lpszwFile,lLength);
    GlobalUnlock32(hDrop);
    return i;
}
/*************************************************************************
 *				DragQueryFile16		[SHELL.11]
 */
UINT16 WINAPI DragQueryFile16(HDROP16 hDrop, WORD wFile, LPSTR lpszFile,
                            WORD wLength)
{ /* hDrop is a global memory block allocated with GMEM_SHARE 
     * with DROPFILESTRUCT as a header and filenames following
     * it, zero length filename is in the end */       
    
    LPDROPFILESTRUCT16 lpDropFileStruct;
    LPSTR lpCurrent;
    WORD  i;
    
    TRACE(shell,"(%04x, %x, %p, %u)\n", hDrop,wFile,lpszFile,wLength);
    
    lpDropFileStruct = (LPDROPFILESTRUCT16) GlobalLock16(hDrop); 
    if(!lpDropFileStruct)
      return 0;
    
    lpCurrent = (LPSTR) lpDropFileStruct + lpDropFileStruct->wSize;

    i = (WORD)SHELL_DragQueryFile(lpCurrent, NULL, wFile==0xffff?0xffffffff:wFile,
				  lpszFile, NULL, wLength);
    GlobalUnlock16(hDrop);
    return i;
}



/*************************************************************************
 *				DragFinish32		[SHELL32.80]
 */
void WINAPI DragFinish32(HDROP32 h)
{ TRACE(shell,"\n");
    GlobalFree32((HGLOBAL32)h);
}

/*************************************************************************
 *				DragFinish16		[SHELL.12]
 */
void WINAPI DragFinish16(HDROP16 h)
{ TRACE(shell,"\n");
    GlobalFree16((HGLOBAL16)h);
}


/*************************************************************************
 *				DragQueryPoint32		[SHELL32.135]
 */
BOOL32 WINAPI DragQueryPoint32(HDROP32 hDrop, POINT32 *p)
{
  LPDROPFILESTRUCT32 lpDropFileStruct;  
  BOOL32             bRet;
  TRACE(shell,"\n");
  lpDropFileStruct = (LPDROPFILESTRUCT32) GlobalLock32(hDrop);
  
  memcpy(p,&lpDropFileStruct->ptMousePos,sizeof(POINT32));
  bRet = lpDropFileStruct->fInNonClientArea;
  
  GlobalUnlock32(hDrop);
  return bRet;
}

/*************************************************************************
 *				DragQueryPoint16		[SHELL.13]
 */
BOOL16 WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p)
{
  LPDROPFILESTRUCT16 lpDropFileStruct;  
  BOOL16           bRet;
  TRACE(shell,"\n");
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
HINSTANCE32 SHELL_FindExecutable( LPCSTR lpFile, 
                                         LPCSTR lpOperation,
                                         LPSTR lpResult)
{ char *extension = NULL; /* pointer to file extension */
    char tmpext[5];         /* local copy to mung as we please */
    char filetype[256];     /* registry name for this filetype */
    LONG filetypelen=256;   /* length of above */
    char command[256];      /* command from registry */
    LONG commandlen=256;    /* This is the most DOS can handle :) */
    char buffer[256];       /* Used to GetProfileString */
    HINSTANCE32 retval=31;  /* default - 'No association was found' */
    char *tok;              /* token pointer */
    int i;                  /* random counter */
    char xlpFile[256];      /* result of SearchPath */

  TRACE(shell, "%s\n", (lpFile != NULL?lpFile:"-") );

    lpResult[0]='\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if (( lpFile == NULL ) || ( lpResult == NULL ) || ( lpOperation == NULL ))
  { WARN(exec, "(lpFile=%s,lpResult=%s,lpOperation=%s): NULL parameter\n",
           lpFile, lpOperation, lpResult);
        return 2; /* File not found. Close enough, I guess. */
    }

    if (SearchPath32A( NULL, lpFile,".exe",sizeof(xlpFile),xlpFile,NULL))
  { TRACE(shell, "SearchPath32A returned non-zero\n");
        lpFile = xlpFile;
    }

    /* First thing we need is the file's extension */
    extension = strrchr( xlpFile, '.' ); /* Assume last "." is the one; */
					/* File->Run in progman uses */
					/* .\FILE.EXE :( */
  TRACE(shell, "xlpFile=%s,extension=%s\n", xlpFile, extension);

    if ((extension == NULL) || (extension == &xlpFile[strlen(xlpFile)]))
  { WARN(shell, "Returning 31 - No association\n");
        return 31; /* no association */
    }

    /* Make local copy & lowercase it for reg & 'programs=' lookup */
    lstrcpyn32A( tmpext, extension, 5 );
    CharLower32A( tmpext );
  TRACE(shell, "%s file\n", tmpext);
    
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
  { for (i=0;i<strlen(buffer); i++) buffer[i]=tolower(buffer[i]);

		tok = strtok(buffer, " \t"); /* ? */
		while( tok!= NULL)
		  {
			if (strcmp(tok, &tmpext[1])==0) /* have to skip the leading "." */
			  {
				strcpy(lpResult, xlpFile);
				/* Need to perhaps check that the file has a path
				 * attached */
        TRACE(shell, "found %s\n", lpResult);
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
  TRACE(shell, "File type: %s\n", filetype);

	/* Looking for ...buffer\shell\lpOperation\command */
	strcat( filetype, "\\shell\\" );
	strcat( filetype, lpOperation );
	strcat( filetype, "\\command" );
	
	if (RegQueryValue16( HKEY_CLASSES_ROOT, filetype, command,
                             &commandlen ) == ERROR_SUCCESS )
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

    TRACE(shell, "returning %s\n", lpResult);
    return retval;
}

/*************************************************************************
 *				ShellExecute16		[SHELL.20]
 */
HINSTANCE16 WINAPI ShellExecute16( HWND16 hWnd, LPCSTR lpOperation,
                                   LPCSTR lpFile, LPCSTR lpParameters,
                                   LPCSTR lpDirectory, INT16 iShowCmd )
{   HINSTANCE16 retval=31;
    char old_dir[1024];
    char cmd[256];

    TRACE(shell, "(%04x,'%s','%s','%s','%s',%x)\n",
		hWnd, lpOperation ? lpOperation:"<null>", lpFile ? lpFile:"<null>",
		lpParameters ? lpParameters : "<null>", 
		lpDirectory ? lpDirectory : "<null>", iShowCmd);

    if (lpFile==NULL) return 0; /* should not happen */
    if (lpOperation==NULL) /* default is open */
      lpOperation="open";

    if (lpDirectory)
    { GetCurrentDirectory32A( sizeof(old_dir), old_dir );
        SetCurrentDirectory32A( lpDirectory );
    }

    retval = SHELL_FindExecutable( lpFile, lpOperation, cmd );

    if (retval > 32)  /* Found */
    { if (lpParameters)
      { strcat(cmd," ");
            strcat(cmd,lpParameters);
        }

      TRACE(shell,"starting %s\n",cmd);
        retval = WinExec32( cmd, iShowCmd );
    }
    if (lpDirectory)
      SetCurrentDirectory32A( old_dir );
    return retval;
}

/*************************************************************************
 *             FindExecutable16   (SHELL.21)
 */
HINSTANCE16 WINAPI FindExecutable16( LPCSTR lpFile, LPCSTR lpDirectory,
                                     LPSTR lpResult )
{ return (HINSTANCE16)FindExecutable32A( lpFile, lpDirectory, lpResult );
}


/*************************************************************************
 *             AboutDlgProc16   (SHELL.33)
 */
LRESULT WINAPI AboutDlgProc16( HWND16 hWnd, UINT16 msg, WPARAM16 wParam,
                               LPARAM lParam )
{ return AboutDlgProc32( hWnd, msg, wParam, lParam );
}


/*************************************************************************
 *             ShellAbout16   (SHELL.22)
 */
BOOL16 WINAPI ShellAbout16( HWND16 hWnd, LPCSTR szApp, LPCSTR szOtherStuff,
                            HICON16 hIcon )
{ return ShellAbout32A( hWnd, szApp, szOtherStuff, hIcon );
}

/*************************************************************************
 *				SHELL_GetResourceTable
 */
static DWORD SHELL_GetResourceTable(HFILE32 hFile,LPBYTE *retptr)
{
  IMAGE_DOS_HEADER	mz_header;
  char			magic[4];
  int			size;
  TRACE(shell,"\n");  
  *retptr = NULL;
  _llseek32( hFile, 0, SEEK_SET );
  if (	(_lread32(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
  	(mz_header.e_magic != IMAGE_DOS_SIGNATURE)
  ) { /* .ICO file ? */
        if (mz_header.e_cblp == 1) { /* ICONHEADER.idType, must be 1 */
	    *retptr = (LPBYTE)-1;
  	    return 1;
	}
	else
	    return 0; /* failed */
  }
  _llseek32( hFile, mz_header.e_lfanew, SEEK_SET );
  if (_lread32( hFile, magic, sizeof(magic) ) != sizeof(magic))
	return 0;
  _llseek32( hFile, mz_header.e_lfanew, SEEK_SET);

  if (*(DWORD*)magic  == IMAGE_NT_SIGNATURE)
	return IMAGE_NT_SIGNATURE;
  if (*(WORD*)magic == IMAGE_OS2_SIGNATURE) {
  	IMAGE_OS2_HEADER	ne_header;
  	LPBYTE			pTypeInfo = (LPBYTE)-1;

  	if (_lread32(hFile,&ne_header,sizeof(ne_header))!=sizeof(ne_header))
		return 0;

	if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE) return 0;
	size = ne_header.rname_tab_offset - ne_header.resource_tab_offset;
	if( size > sizeof(NE_TYPEINFO) )
	{
	    pTypeInfo = (BYTE*)HeapAlloc( GetProcessHeap(), 0, size);
	    if( pTypeInfo ) {
		_llseek32(hFile, mz_header.e_lfanew+ne_header.resource_tab_offset, SEEK_SET);
		if( _lread32( hFile, (char*)pTypeInfo, size) != size ) { 
		    HeapFree( GetProcessHeap(), 0, pTypeInfo); 
		    pTypeInfo = NULL;
		}
	    }
	}
  	*retptr = pTypeInfo;
        return IMAGE_OS2_SIGNATURE;
  } else
  	return 0; /* failed */
}

/*************************************************************************
 *			SHELL_LoadResource
 */
static HGLOBAL16 SHELL_LoadResource(HINSTANCE16 hInst, HFILE32 hFile, NE_NAMEINFO* pNInfo, WORD sizeShift)
{ BYTE*  ptr;
 HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, (DWORD)pNInfo->length << sizeShift);
  TRACE(shell,"\n");
 if( (ptr = (BYTE*)GlobalLock16( handle )) )
  { _llseek32( hFile, (DWORD)pNInfo->offset << sizeShift, SEEK_SET);
     _lread32( hFile, (char*)ptr, pNInfo->length << sizeShift);
     return handle;
   }
 return 0;
}

/*************************************************************************
 *                      ICO_LoadIcon
 */
static HGLOBAL16 ICO_LoadIcon(HINSTANCE16 hInst, HFILE32 hFile, LPicoICONDIRENTRY lpiIDE)
{ BYTE*  ptr;
 HGLOBAL16 handle = DirectResAlloc( hInst, 0x10, lpiIDE->dwBytesInRes);
  TRACE(shell,"\n");
 if( (ptr = (BYTE*)GlobalLock16( handle )) )
  { _llseek32( hFile, lpiIDE->dwImageOffset, SEEK_SET);
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
static HGLOBAL16 ICO_GetIconDirectory(HINSTANCE16 hInst, HFILE32 hFile, LPicoICONDIR* lplpiID ) 
{ WORD    id[3];  /* idReserved, idType, idCount */
  LPicoICONDIR	lpiID;
  int		i;
 
  TRACE(shell,"\n"); 
  _llseek32( hFile, 0, SEEK_SET );
  if( _lread32(hFile,(char*)id,sizeof(id)) != sizeof(id) ) return 0;

  /* check .ICO header 
   *
   * - see http://www.microsoft.com/win32dev/ui/icons.htm
   */

  if( id[0] || id[1] != 1 || !id[2] ) return 0;

  i = id[2]*sizeof(icoICONDIRENTRY) + sizeof(id);

  lpiID = (LPicoICONDIR)HeapAlloc( GetProcessHeap(), 0, i);

  if( _lread32(hFile,(char*)lpiID->idEntries,i) == i )
  { HGLOBAL16 handle = DirectResAlloc( hInst, 0x10,
                                     id[2]*sizeof(ICONDIRENTRY) + sizeof(id) );
     if( handle ) 
    { CURSORICONDIR*     lpID = (CURSORICONDIR*)GlobalLock16( handle );
       lpID->idReserved = lpiID->idReserved = id[0];
       lpID->idType = lpiID->idType = id[1];
       lpID->idCount = lpiID->idCount = id[2];
       for( i=0; i < lpiID->idCount; i++ )
      { memcpy((void*)(lpID->idEntries + i), 
		   (void*)(lpiID->idEntries + i), sizeof(ICONDIRENTRY) - 2);
	    lpID->idEntries[i].icon.wResId = i;
         }
      *lplpiID = lpiID;
       return handle;
     }
  }
  /* fail */

  HeapFree( GetProcessHeap(), 0, lpiID);
  return 0;
}

/*************************************************************************
 *			InternalExtractIcon		[SHELL.39]
 *
 * This abortion is called directly by Progman
 */
HGLOBAL16 WINAPI InternalExtractIcon(HINSTANCE16 hInstance,
                                     LPCSTR lpszExeFileName, UINT16 nIconIndex,
                                     WORD n )
{
  HGLOBAL16 	hRet = 0;
  HGLOBAL16*	RetPtr = NULL;
  LPBYTE  	pData;
  OFSTRUCT 	ofs;
  DWORD		sig;
  HFILE32 	hFile = OpenFile32( lpszExeFileName, &ofs, OF_READ );
  UINT16	iconDirCount = 0,iconCount = 0;
  
  TRACE(shell,"(%04x,file %s,start %d,extract %d\n", 
		       hInstance, lpszExeFileName, nIconIndex, n);

  if( hFile == HFILE_ERROR32 || !n )
    return 0;

  hRet = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, sizeof(HICON16)*n);
  RetPtr = (HICON16*)GlobalLock16(hRet);

  *RetPtr = (n == 0xFFFF)? 0: 1;	/* error return values */

  sig = SHELL_GetResourceTable(hFile,&pData);

  if((sig == IMAGE_OS2_SIGNATURE)
  || (sig == 1)) /* .ICO file */
  {
    HICON16	 hIcon = 0;
    NE_TYPEINFO* pTInfo = (NE_TYPEINFO*)(pData + 2);
    NE_NAMEINFO* pIconStorage = NULL;
    NE_NAMEINFO* pIconDir = NULL;
    LPicoICONDIR lpiID = NULL;
 
    if( pData == (BYTE*)-1 )
    {
	/* check for .ICO file */

	hIcon = ICO_GetIconDirectory(hInstance, hFile, &lpiID);
	if( hIcon ) { iconDirCount = 1; iconCount = lpiID->idCount; }
    }
    else while( pTInfo->type_id && !(pIconStorage && pIconDir) )
    {
	/* find icon directory and icon repository */

	if( pTInfo->type_id == NE_RSCTYPE_GROUP_ICON ) 
	  {
	     iconDirCount = pTInfo->count;
	     pIconDir = ((NE_NAMEINFO*)(pTInfo + 1));
       TRACE(shell,"\tfound directory - %i icon families\n", iconDirCount);
	  }
	if( pTInfo->type_id == NE_RSCTYPE_ICON ) 
	  { 
	     iconCount = pTInfo->count;
	     pIconStorage = ((NE_NAMEINFO*)(pTInfo + 1));
       TRACE(shell,"\ttotal icons - %i\n", iconCount);
	  }
  	pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1)+pTInfo->count*sizeof(NE_NAMEINFO));
    }

    /* load resources and create icons */

    if( (pIconStorage && pIconDir) || lpiID )
      if( nIconIndex == (UINT16)-1 ) RetPtr[0] = iconDirCount;
      else if( nIconIndex < iconDirCount )
      {
	  UINT16   i, icon;

	  if( n > iconDirCount - nIconIndex ) n = iconDirCount - nIconIndex;

	  for( i = nIconIndex; i < nIconIndex + n; i++ ) 
	  {
	      /* .ICO files have only one icon directory */

	      if( lpiID == NULL )
	           hIcon = SHELL_LoadResource( hInstance, hFile, pIconDir + i, 
							      *(WORD*)pData );
	      RetPtr[i-nIconIndex] = GetIconID( hIcon, 3 );
	      GlobalFree16(hIcon); 
          }

	  for( icon = nIconIndex; icon < nIconIndex + n; icon++ )
	  {
	      hIcon = 0;
	      if( lpiID )
		   hIcon = ICO_LoadIcon( hInstance, hFile, 
					 lpiID->idEntries + RetPtr[icon-nIconIndex]);
	      else
	         for( i = 0; i < iconCount; i++ )
		   if( pIconStorage[i].id == (RetPtr[icon-nIconIndex] | 0x8000) )
		     hIcon = SHELL_LoadResource( hInstance, hFile, pIconStorage + i,
								    *(WORD*)pData );
	      if( hIcon )
	      {
		  RetPtr[icon-nIconIndex] = LoadIconHandler( hIcon, TRUE ); 
		  FarSetOwner( RetPtr[icon-nIconIndex], GetExePtr(hInstance) );
	      }
	      else
		  RetPtr[icon-nIconIndex] = 0;
	  }
      }
    if( lpiID ) HeapFree( GetProcessHeap(), 0, lpiID);
    else HeapFree( GetProcessHeap(), 0, pData);
  } 
  if( sig == IMAGE_NT_SIGNATURE)
  {
  	LPBYTE			peimage,idata,igdata;
	PIMAGE_DOS_HEADER	dheader;
	PIMAGE_NT_HEADERS	pe_header;
	PIMAGE_SECTION_HEADER	pe_sections;
	PIMAGE_RESOURCE_DIRECTORY	rootresdir,iconresdir,icongroupresdir;
	PIMAGE_RESOURCE_DATA_ENTRY	idataent,igdataent;
	HANDLE32		fmapping;
	int			i,j;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY	xresent;
	CURSORICONDIR		**cids;
	
	fmapping = CreateFileMapping32A(hFile,NULL,PAGE_READONLY|SEC_COMMIT,0,0,NULL);
	if (fmapping == 0) { /* FIXME, INVALID_HANDLE_VALUE? */
    WARN(shell,"failed to create filemap.\n");
		_lclose32( hFile);
		return 0;
	}
	peimage = MapViewOfFile(fmapping,FILE_MAP_READ,0,0,0);
	if (!peimage) {
    WARN(shell,"failed to mmap filemap.\n");
		CloseHandle(fmapping);
		_lclose32( hFile);
		return 0;
	}
	dheader = (PIMAGE_DOS_HEADER)peimage;
	/* it is a pe header, SHELL_GetResourceTable checked that */
	pe_header = (PIMAGE_NT_HEADERS)(peimage+dheader->e_lfanew);
	/* probably makes problems with short PE headers... but I haven't seen 
	 * one yet... 
	 */
	pe_sections = (PIMAGE_SECTION_HEADER)(((char*)pe_header)+sizeof(*pe_header));
	rootresdir = NULL;
	for (i=0;i<pe_header->FileHeader.NumberOfSections;i++) {
		if (pe_sections[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			continue;
		/* FIXME: doesn't work when the resources are not in a seperate section */
		if (pe_sections[i].VirtualAddress == pe_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress) {
			rootresdir = (PIMAGE_RESOURCE_DIRECTORY)((char*)peimage+pe_sections[i].PointerToRawData);
			break;
		}
	}

	if (!rootresdir) {
    WARN(shell,"haven't found section for resource directory.\n");
		UnmapViewOfFile(peimage);
		CloseHandle(fmapping);
		_lclose32( hFile);
		return 0;
	}
	icongroupresdir = GetResDirEntryW(rootresdir,RT_GROUP_ICON32W,
                                          (DWORD)rootresdir,FALSE);
	if (!icongroupresdir) {
    WARN(shell,"No Icongroupresourcedirectory!\n");
		UnmapViewOfFile(peimage);
		CloseHandle(fmapping);
		_lclose32( hFile);
		return 0;
	}

	iconDirCount = icongroupresdir->NumberOfNamedEntries+icongroupresdir->NumberOfIdEntries;
	if( nIconIndex == (UINT16)-1 ) {
		RetPtr[0] = iconDirCount;
		UnmapViewOfFile(peimage);
		CloseHandle(fmapping);
		_lclose32( hFile);
		return hRet;
	}

	if (nIconIndex >= iconDirCount) {
    WARN(shell,"nIconIndex %d is larger than iconDirCount %d\n",
			    nIconIndex,iconDirCount);
		UnmapViewOfFile(peimage);
		CloseHandle(fmapping);
		_lclose32( hFile);
		GlobalFree16(hRet);
		return 0;
	}
	cids = (CURSORICONDIR**)HeapAlloc(GetProcessHeap(),0,n*sizeof(CURSORICONDIR*));
		
	/* caller just wanted the number of entries */

	xresent = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(icongroupresdir+1);
	/* assure we don't get too much ... */
	if( n > iconDirCount - nIconIndex ) n = iconDirCount - nIconIndex;

	/* starting from specified index ... */
	xresent = xresent+nIconIndex;

	for (i=0;i<n;i++,xresent++) {
		CURSORICONDIR	*cid;
		PIMAGE_RESOURCE_DIRECTORY	resdir;

		/* go down this resource entry, name */
		resdir = (PIMAGE_RESOURCE_DIRECTORY)((DWORD)rootresdir+(xresent->u2.s.OffsetToDirectory));
		/* default language (0) */
		resdir = GetResDirEntryW(resdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);
		igdataent = (PIMAGE_RESOURCE_DATA_ENTRY)resdir;

		/* lookup address in mapped image for virtual address */
		igdata = NULL;
		for (j=0;j<pe_header->FileHeader.NumberOfSections;j++) {
			if (igdataent->OffsetToData < pe_sections[j].VirtualAddress)
				continue;
			if (igdataent->OffsetToData+igdataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
				continue;
			igdata = peimage+(igdataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
		}
		if (!igdata) {
      WARN(shell,"no matching real address for icongroup!\n");
			UnmapViewOfFile(peimage);
			CloseHandle(fmapping);
			_lclose32( hFile);
			return 0;
		}
		/* found */
		cid = (CURSORICONDIR*)igdata;
		cids[i] = cid;
		RetPtr[i] = LookupIconIdFromDirectoryEx32(igdata,TRUE,SYSMETRICS_CXICON,SYSMETRICS_CYICON,0);
	}
	iconresdir=GetResDirEntryW(rootresdir,RT_ICON32W,
                                   (DWORD)rootresdir,FALSE);
	if (!iconresdir) {
      WARN(shell,"No Iconresourcedirectory!\n");
	    UnmapViewOfFile(peimage);
	    CloseHandle(fmapping);
	    _lclose32( hFile);
	    return 0;
	}
	for (i=0;i<n;i++) {
	    PIMAGE_RESOURCE_DIRECTORY	xresdir;

	    xresdir = GetResDirEntryW(iconresdir,(LPWSTR)RetPtr[i],(DWORD)rootresdir,FALSE);
	    xresdir = GetResDirEntryW(xresdir,(LPWSTR)0,(DWORD)rootresdir,TRUE);

	    idataent = (PIMAGE_RESOURCE_DATA_ENTRY)xresdir;

	    idata = NULL;
	    /* map virtual to address in image */
	    for (j=0;j<pe_header->FileHeader.NumberOfSections;j++) {
		if (idataent->OffsetToData < pe_sections[j].VirtualAddress)
		    continue;
		if (idataent->OffsetToData+idataent->Size > pe_sections[j].VirtualAddress+pe_sections[j].SizeOfRawData)
		    continue;
		idata = peimage+(idataent->OffsetToData-pe_sections[j].VirtualAddress+pe_sections[j].PointerToRawData);
	    }
	    if (!idata) {
    WARN(shell,"no matching real address found for icondata!\n");
		RetPtr[i]=0;
		continue;
	    }
	    RetPtr[i] = CreateIconFromResourceEx32(idata,idataent->Size,TRUE,0x00030000,SYSMETRICS_CXICON,SYSMETRICS_CYICON,0);
	}
	UnmapViewOfFile(peimage);
	CloseHandle(fmapping);
	_lclose32( hFile);
	return hRet;
  }
  _lclose32( hFile );
  /* return array with icon handles */
  return hRet;

}

/*************************************************************************
 *             ExtractIcon16   (SHELL.34)
 */
HICON16 WINAPI ExtractIcon16( HINSTANCE16 hInstance, LPCSTR lpszExeFileName,
	UINT16 nIconIndex )
{   TRACE(shell,"\n");
    return ExtractIcon32A( hInstance, lpszExeFileName, nIconIndex );
}



/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 * 
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON32 WINAPI ExtractAssociatedIcon32A(HINSTANCE32 hInst,LPSTR lpIconPath,
	LPWORD lpiIcon)
{ TRACE(shell,"\n");
	return ExtractAssociatedIcon16(hInst,lpIconPath,lpiIcon);
}

HICON16 WINAPI ExtractAssociatedIcon16(HINSTANCE16 hInst,LPSTR lpIconPath,
	LPWORD lpiIcon)
{ HICON16 hIcon;

  TRACE(shell,"\n");

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
	hIcon = LoadIcon16( hInst, MAKEINTRESOURCE16(*lpiIcon));
    }
    return hIcon;
}

/*************************************************************************
 *				FindEnvironmentString	[SHELL.38]
 *
 * Returns a pointer into the DOS environment... Ugh.
 */
LPSTR SHELL_FindString(LPSTR lpEnv, LPCSTR entry)
{ UINT16 l;

  TRACE(shell,"\n");

  l = strlen(entry); 
  for( ; *lpEnv ; lpEnv+=strlen(lpEnv)+1 )
  { if( lstrncmpi32A(lpEnv, entry, l) ) 
      continue;
	if( !*(lpEnv+l) )
	    return (lpEnv + l); 		/* empty entry */
	else if ( *(lpEnv+l)== '=' )
	    return (lpEnv + l + 1);
    }
    return NULL;
}

SEGPTR WINAPI FindEnvironmentString(LPSTR str)
{ SEGPTR  spEnv;
  LPSTR lpEnv,lpString;
  TRACE(shell,"\n");
    
  spEnv = GetDOSEnvironment();

  lpEnv = (LPSTR)PTR_SEG_TO_LIN(spEnv);
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
DWORD WINAPI DoEnvironmentSubst(LPSTR str,WORD length)
{
  LPSTR   lpEnv = (LPSTR)PTR_SEG_TO_LIN(GetDOSEnvironment());
  LPSTR   lpBuffer = (LPSTR)HeapAlloc( GetProcessHeap(), 0, length);
  LPSTR   lpstr = str;
  LPSTR   lpbstr = lpBuffer;

  CharToOem32A(str,str);

  TRACE(shell,"accept %s\n", str);

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
           WARN(shell,"-- Env subst aborted - string too short\n");
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

  TRACE(shell,"-- return %s\n", str);

  OemToChar32A(str,str);
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
LRESULT WINAPI ShellHookProc(INT16 code, WPARAM16 wParam, LPARAM lParam)
{
    TRACE(shell,"%i, %04x, %08x\n", code, wParam, 
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
	PostMessage16( SHELL_hWnd, uMsg, wParam, 0 );
    }
    return CallNextHookEx16( WH_SHELL, code, wParam, lParam );
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
BOOL32 WINAPI RegisterShellHook(HWND16 hWnd, UINT16 uAction)
{ TRACE(shell,"%04x [%u]\n", hWnd, uAction );

    switch( uAction )
  { case 2:  /* register hWnd as a shell window */
	     if( !SHELL_hHook )
      { HMODULE16 hShell = GetModuleHandle16( "SHELL" );
        SHELL_hHook = SetWindowsHookEx16( WH_SHELL, ShellHookProc, hShell, 0 );
		if( SHELL_hHook )
        { uMsgWndCreated = RegisterWindowMessage32A( lpstrMsgWndCreated );
		    uMsgWndDestroyed = RegisterWindowMessage32A( lpstrMsgWndDestroyed );
		    uMsgShellActivate = RegisterWindowMessage32A( lpstrMsgShellActivate );
		} 
        else 
          WARN(shell,"-- unable to install ShellHookProc()!\n");
	     }

      if( SHELL_hHook )
        return ((SHELL_hWnd = hWnd) != 0);
	     break;

	default:
    WARN(shell, "-- unknown code %i\n", uAction );
	     /* just in case */
	     SHELL_hWnd = 0;
    }
    return FALSE;
}
