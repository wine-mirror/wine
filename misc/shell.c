/*
 * 				Shell Library Functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "shell.h"
#include "neexe.h"
#include "selectors.h"
#include "alias.h"
#include "relay32.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

LPKEYSTRUCT	lphRootKey = NULL,lphTopKey = NULL;

static char RootKeyName[]=".classes", TopKeyName[] = "[top-null]";

/*************************************************************************
 *                        SHELL_Init()
 */
BOOL SHELL_Init()
{
    HKEY hNewKey;
    
    hNewKey = GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
    lphRootKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
    if (lphRootKey == NULL) {
        printf("SHELL_RegCheckForRoot: Couldn't allocate root key!\n");
        return FALSE;
    }
    lphRootKey->hKey = (HKEY)1;
    lphRootKey->lpSubKey = RootKeyName;
    lphRootKey->dwType = 0;
    lphRootKey->lpValue = NULL;
    lphRootKey->lpSubLvl = lphRootKey->lpNextKey = lphRootKey->lpPrevKey = NULL;
    
    hNewKey = GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
    lphTopKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
    if (lphTopKey == NULL) {
        printf("SHELL_RegCheckForRoot: Couldn't allocate top key!\n");
        return FALSE;
    }
    lphTopKey->hKey = 0;
    lphTopKey->lpSubKey = TopKeyName;
    lphTopKey->dwType = 0;
    lphTopKey->lpValue = NULL;
    lphTopKey->lpSubLvl = lphRootKey;
    lphTopKey->lpNextKey = lphTopKey->lpPrevKey = NULL;

    dprintf_reg(stddeb,"SHELL_RegCheckForRoot: Root/Top created\n");

    return TRUE;
}

/* FIXME: the loading and saving of the registry database is rather messy.
 * bad input (while reading) may crash wine.
 */
void
_DumpLevel(FILE *f,LPKEYSTRUCT lpTKey,int tabs)
{
	LPKEYSTRUCT	lpKey;

	lpKey=lpTKey->lpSubLvl;
	while (lpKey) {
		int	i;
		for (i=0;i<tabs;i++) fprintf(f,"\t");
		/* implement different dwTypes ... */
		if (lpKey->lpValue)
			fprintf(f,"%s=%s\n",lpKey->lpSubKey,lpKey->lpValue);
		else
			fprintf(f,"%s\n",lpKey->lpSubKey);

		if (lpKey->lpSubLvl)
			_DumpLevel(f,lpKey,tabs+1);
		lpKey=lpKey->lpNextKey;
	}
}

static void
_SaveKey(HKEY hKey,char *where)
{
	FILE		*f;
	LPKEYSTRUCT	lpKey;

	f=fopen(where,"w");
	if (f==NULL) {
		perror("registry-fopen");
		return;
	}
	switch ((DWORD)hKey) {
	case HKEY_CLASSES_ROOT:
		lpKey=lphRootKey;
		break;
	default:return;
	}
	_DumpLevel(f,lpKey,0);
	fclose(f);
}

void
SHELL_SaveRegistry(void)
{
	/* FIXME: 
	 * -implement win95 additional keytypes here
	 * (HKEY_LOCAL_MACHINE,HKEY_CURRENT_USER or whatever)
	 * -choose better filename(s)
	 */
	_SaveKey((HKEY)HKEY_CLASSES_ROOT,"/tmp/winereg");
}

#define BUFSIZE	256
void
_LoadLevel(FILE *f,LPKEYSTRUCT lpKey,int tabsexp,char *buf)
{
	int		i;
	char		*s,*t;
	HKEY		hNewKey;
	LPKEYSTRUCT	lpNewKey;

	while (1) {
		if (NULL==fgets(buf,BUFSIZE,f)) {
			buf[0]=0;
			return;
		}
		for (i=0;buf[i]=='\t';i++) /*empty*/;
		s=buf+i;
		if (NULL!=(t=strchr(s,'\n'))) *t='\0';
		if (NULL!=(t=strchr(s,'\r'))) *t='\0';

		if (i<tabsexp) return;

		if (i>tabsexp) {
			hNewKey=GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
			lpNewKey=lpKey->lpSubLvl=(LPKEYSTRUCT)GlobalLock(hNewKey);
			lpNewKey->hKey		= hNewKey;
			lpNewKey->dwType	= 0;
			lpNewKey->lpSubKey	= NULL;
			lpNewKey->lpValue	= NULL;
			lpNewKey->lpSubLvl	= NULL;
			lpNewKey->lpNextKey	= NULL;
			lpNewKey->lpPrevKey	= NULL;
			if (NULL!=(t=strchr(s,'='))) {
				*t='\0';t++;
				lpNewKey->dwType	= REG_SZ;
				lpNewKey->lpSubKey	= xstrdup(s);
				lpNewKey->lpValue	= xstrdup(t);
			} else {
				lpNewKey->dwType	= REG_SZ;
				lpNewKey->lpSubKey	= xstrdup(s);
			}
			_LoadLevel(f,lpNewKey,tabsexp+1,buf);
		}
		for (i=0;buf[i]=='\t';i++) /*empty*/;
		s=buf+i;
		if (i<tabsexp) return;
		if (buf[0]=='\0') break; /* marks end of file */
		/* we have a buf now. even when returning from _LoadLevel */
		hNewKey		= GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
		lpNewKey	= lpKey->lpNextKey=(LPKEYSTRUCT)GlobalLock(hNewKey);
		lpNewKey->lpPrevKey	= lpKey;
		lpNewKey->hKey		= hNewKey;
		lpNewKey->dwType	= 0;
		lpNewKey->lpSubKey	= NULL;
		lpNewKey->lpValue	= NULL;
		lpNewKey->lpSubLvl	= NULL;
		lpNewKey->lpNextKey	= NULL;
		if (NULL!=(t=strchr(s,'='))) {
			*t='\0';t++;
			lpNewKey->dwType	= REG_SZ;
			lpNewKey->lpSubKey	= xstrdup(s);
			lpNewKey->lpValue	= xstrdup(t);
		} else {
			lpNewKey->dwType	= REG_SZ;
			lpNewKey->lpSubKey	= xstrdup(s);
		}
		lpKey=lpNewKey;
	}
}

void
_LoadKey(HKEY hKey,char *from) 
{
	FILE		*f;
	LPKEYSTRUCT	lpKey;
	char		buf[BUFSIZE]; /* FIXME: long enough? */

	f=fopen(from,"r");
	if (f==NULL) {
            dprintf_reg(stddeb,"fopen-registry-read");
            return;
	}
	switch ((DWORD)hKey) {
	case HKEY_CLASSES_ROOT:
		lpKey=lphRootKey;
		break;
	default:return;
	}
	_LoadLevel(f,lpKey,-1,buf);
}

void
SHELL_LoadRegistry(void) 
{
	_LoadKey((HKEY)HKEY_CLASSES_ROOT,"/tmp/winereg");
}

/*************************************************************************
 *				RegOpenKey		[SHELL.1]
 */
LONG RegOpenKey(HKEY hKey, LPCSTR lpSubKey, LPHKEY lphKey)
{
	LPKEYSTRUCT	lpKey,lpNextKey;
	LPCSTR		ptr;
	char		str[128];

	dprintf_reg(stddeb, "RegOpenKey(%08lX, %p='%s', %p)\n",
				       (DWORD)hKey, lpSubKey, lpSubKey, lphKey);
	if (lphKey == NULL) return SHELL_ERROR_INVALID_PARAMETER;
        switch((DWORD)hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
        case 0x80000000:
        case 0x80000001:
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegOpenKey // specific key = %08lX !\n", (DWORD)hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }
	if (lpSubKey == NULL || !*lpSubKey)  { 
	  *lphKey = hKey; 
	  return SHELL_ERROR_SUCCESS; 
	}
        while(*lpSubKey) {
          ptr = strchr(lpSubKey,'\\');
          if (!ptr) ptr = lpSubKey + strlen(lpSubKey);
          strncpy(str,lpSubKey,ptr-lpSubKey);
          str[ptr-lpSubKey] = 0;
          lpSubKey = ptr; 
          if (*lpSubKey) lpSubKey++;
	  
	  lpNextKey = lpKey->lpSubLvl;
          while(lpKey != NULL && strcmp(lpKey->lpSubKey, str) != 0) { 
          	lpKey = lpNextKey;
          	if (lpKey) lpNextKey = lpKey->lpNextKey;
          }
          if (lpKey == NULL) {
	    dprintf_reg(stddeb,"RegOpenKey: key %s not found!\n",str);
	    return SHELL_ERROR_BADKEY;
	  }	    
	}
        *lphKey = lpKey->hKey;
	return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCreateKey		[SHELL.2]
 */
LONG RegCreateKey(HKEY hKey, LPCSTR lpSubKey, LPHKEY lphKey)
{
	HKEY		hNewKey;
	LPKEYSTRUCT	lpNewKey;
	LPKEYSTRUCT	lpKey;
	LPKEYSTRUCT	lpPrevKey;
	LPCSTR		ptr;
	char		str[128];

	dprintf_reg(stddeb, "RegCreateKey(%08lX, '%s', %p)\n",	(DWORD)hKey, lpSubKey, lphKey);
	if (lphKey == NULL) return SHELL_ERROR_INVALID_PARAMETER;
        switch((DWORD)hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
        case 0x80000000:
        case 0x80000001:
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegCreateKey // specific key = %08lX !\n", (DWORD)hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }
	if (lpSubKey == NULL || !*lpSubKey)  { 
	  *lphKey = hKey; 
	  return SHELL_ERROR_SUCCESS;
	}
        while (*lpSubKey) {
          dprintf_reg(stddeb, "RegCreateKey: Looking for subkey %s\n", lpSubKey);
          ptr = strchr(lpSubKey,'\\');
          if (!ptr) ptr = lpSubKey + strlen(lpSubKey);
          strncpy(str,lpSubKey,ptr-lpSubKey);
          str[ptr-lpSubKey] = 0;
          lpSubKey = ptr; 
          if (*lpSubKey) lpSubKey++;
	  
	  lpPrevKey = lpKey;
	  lpKey = lpKey->lpSubLvl;
          while(lpKey != NULL && strcmp(lpKey->lpSubKey, str) != 0) { 
	    lpKey = lpKey->lpNextKey; 
	  }
          if (lpKey == NULL) {
	    hNewKey = GlobalAlloc(GMEM_MOVEABLE, sizeof(KEYSTRUCT));
	    lpNewKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
	    if (lpNewKey == NULL) {
	      printf("RegCreateKey // Can't alloc new key !\n");
	      return SHELL_ERROR_OUTOFMEMORY;
	    }
	    lpNewKey->hKey = hNewKey;
	    lpNewKey->lpSubKey = malloc(strlen(str) + 1);
	    if (lpNewKey->lpSubKey == NULL) {
	      printf("RegCreateKey // Can't alloc key string !\n");
	      return SHELL_ERROR_OUTOFMEMORY;
	    }
	    strcpy(lpNewKey->lpSubKey, str);
	    lpNewKey->lpNextKey = lpPrevKey->lpSubLvl;
	    lpNewKey->lpPrevKey = NULL;
            lpPrevKey->lpSubLvl = lpNewKey;

	    lpNewKey->dwType = 0;
	    lpNewKey->lpValue = NULL;
	    lpNewKey->lpSubLvl = NULL;
	    *lphKey = hNewKey;
	    dprintf_reg(stddeb,"RegCreateKey // successful '%s' key=%08lX !\n", str, (DWORD)hNewKey);
	    lpKey = lpNewKey;
	  } else {
            *lphKey = lpKey->hKey;
	    dprintf_reg(stddeb,"RegCreateKey // found '%s', key=%08lX\n", str, (DWORD)*lphKey);
	  }
	}
	return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCloseKey		[SHELL.3]
 */
LONG RegCloseKey(HKEY hKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegCloseKey(%08lX);\n", (DWORD)hKey);
	return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegDeleteKey		[SHELL.4]
 */
LONG RegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegDeleteKey(%08lX, '%s');\n",
                     (DWORD)hKey, lpSubKey);
	return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegSetValue		[SHELL.5]
 */
LONG RegSetValue(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, 
		 LPCSTR lpVal, DWORD dwIgnored)
{
    HKEY       	hRetKey;
    LPKEYSTRUCT	lpKey;
    LONG       	dwRet;
    dprintf_reg(stddeb, "RegSetValue(%08lX, '%s', %08lX, '%s', %08lX);\n",
		(DWORD)hKey, lpSubKey, dwType, lpVal, dwIgnored);
    /*if (lpSubKey == NULL) return SHELL_ERROR_INVALID_PARAMETER;*/
    if (lpVal == NULL) return SHELL_ERROR_INVALID_PARAMETER;
    if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != SHELL_ERROR_SUCCESS) {
	dprintf_reg(stddeb, "RegSetValue // key not found ... so create it !\n");
	if ((dwRet = RegCreateKey(hKey, lpSubKey, &hRetKey)) != SHELL_ERROR_SUCCESS) {
	    fprintf(stderr, "RegSetValue // key creation error %08lX !\n", dwRet);
	    return dwRet;
	}
    }
    lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
    if (lpKey == NULL) return SHELL_ERROR_BADKEY;
    if (lpKey->lpValue != NULL) free(lpKey->lpValue);
    lpKey->lpValue = xmalloc(strlen(lpVal) + 1);
    strcpy(lpKey->lpValue, lpVal);
    dprintf_reg(stddeb,"RegSetValue // successful key='%s' val='%s' !\n", lpSubKey, lpKey->lpValue);
    return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegQueryValue		[SHELL.6]
 */
LONG RegQueryValue(HKEY hKey, LPCSTR lpSubKey, LPSTR lpVal, LPLONG lpcb)
{
	HKEY		hRetKey;
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	int			size;
	dprintf_reg(stddeb, "RegQueryValue(%08lX, '%s', %p, %p);\n",
                    (DWORD)hKey, lpSubKey, lpVal, lpcb);
	/*if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;*/
	if (lpVal == NULL) return SHELL_ERROR_INVALID_PARAMETER;
	if (lpcb == NULL) return SHELL_ERROR_INVALID_PARAMETER;
        if (!*lpcb) return SHELL_ERROR_INVALID_PARAMETER;

	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != SHELL_ERROR_SUCCESS) {
		fprintf(stderr, "RegQueryValue // key not found !\n");
		return dwRet;
	}
	lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
	if (lpKey == NULL) return SHELL_ERROR_BADKEY;
	if (lpKey->lpValue != NULL) {
          if ((size = strlen(lpKey->lpValue)+1) > *lpcb){
            strncpy(lpVal,lpKey->lpValue,*lpcb-1);
            lpVal[*lpcb-1] = 0;
	  } else {
            strcpy(lpVal,lpKey->lpValue);
            *lpcb = size;
          }
	} else {
	  *lpVal = 0;
	  *lpcb = (LONG)1;
	}
	dprintf_reg(stddeb,"RegQueryValue // return '%s' !\n", lpVal);
	return SHELL_ERROR_SUCCESS;
}


/*************************************************************************
 *				RegEnumKey		[SHELL.7]
 */
LONG RegEnumKey(HKEY hKey, DWORD dwSubKey, LPSTR lpBuf, DWORD dwSize)
{
	LPKEYSTRUCT	lpKey;
	LONG            len;

	dprintf_reg(stddeb, "RegEnumKey(%08lX, %ld)\n", (DWORD)hKey, dwSubKey);
	if (lpBuf == NULL) return SHELL_ERROR_INVALID_PARAMETER;
        switch((DWORD)hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
        case 0x80000000:
        case 0x80000001:
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegEnumKey // specific key = %08lX !\n", (DWORD)hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }
        lpKey = lpKey->lpSubLvl;
        while(lpKey != NULL){
          if (!dwSubKey){
            len = MIN(dwSize-1,strlen(lpKey->lpSubKey));
	    strncpy(lpBuf,lpKey->lpSubKey,len);
	    lpBuf[len] = 0;
            dprintf_reg(stddeb, "RegEnumKey: found %s\n",lpBuf);
	    return SHELL_ERROR_SUCCESS;
	  }
          dwSubKey--;
          lpKey = lpKey->lpNextKey;
        }
	dprintf_reg(stddeb, "RegEnumKey: key not found!\n");
	return SHELL_ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void DragAcceptFiles(HWND hWnd, BOOL b)
{
    /* flips WS_EX_ACCEPTFILES bit according to the value of b */
    dprintf_reg(stddeb,"DragAcceptFiles(%04x, %u) old exStyle %08lx\n",
		hWnd,b,GetWindowLong(hWnd,GWL_EXSTYLE));

    SetWindowLong(hWnd,GWL_EXSTYLE,
		  GetWindowLong(hWnd,GWL_EXSTYLE) | b*(LONG)WS_EX_ACCEPTFILES);
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
	fprintf(stdnimp, "FindExecutable : Empty Stub !!!\n");
        return 0;
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
    DWORD WineProc,Win16Proc,Win32Proc;
    static int initialized=0;

    if (szApp) strncpy(AppName, szApp, sizeof(AppName));
    else *AppName = 0;
    AppName[sizeof(AppName)-1]=0;

    if (szOtherStuff) strncpy(AppMisc, szOtherStuff, sizeof(AppMisc));
    else *AppMisc = 0;
    AppMisc[sizeof(AppMisc)-1]=0;

    if (!hIcon) hIcon = LoadIcon(0,MAKEINTRESOURCE(OIC_WINEICON));
    
    if(!initialized)
    {
        WineProc=(DWORD)AboutDlgProc;
        Win16Proc=(DWORD)GetWndProcEntry16("AboutDlgProc");
        Win32Proc=(DWORD)RELAY32_GetEntryPoint(RELAY32_GetBuiltinDLL("WINPROCS32"),
                                               "AboutDlgProc",0);
        ALIAS_RegisterAlias(WineProc,Win16Proc,Win32Proc);
        initialized=1;
    }

    handle = SYSRES_LoadResource( SYSRES_DIALOG_SHELL_ABOUT_MSGBOX );
    if (!handle) return FALSE;
    bRet = DialogBoxIndirectParam( WIN_GetWindowInstance( hWnd ),
                                   handle, hWnd,
                                   GetWndProcEntry16("AboutDlgProc"), 
				   (LONG)hIcon );
    SYSRES_FreeResource( handle );
    return bRet;
}

/*************************************************************************
 *				ExtractIcon		[SHELL.34]
 */
HICON ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex)
{
	HICON	hIcon = 0;
	HINSTANCE hInst2 = hInst;
	dprintf_reg(stddeb, "ExtractIcon(%04x, '%s', %d\n", 
			hInst, lpszExeFileName, nIconIndex);
        return 0;
	if (lpszExeFileName != NULL) {
		hInst2 = LoadModule(lpszExeFileName,(LPVOID)-1);
	}
	if (hInst2 != 0 && nIconIndex == (UINT)-1) {
#if 0
		count = GetRsrcCount(hInst2, NE_RSCTYPE_GROUP_ICON);
		dprintf_reg(stddeb, "ExtractIcon // '%s' has %d icons !\n", lpszExeFileName, count);
		return (HICON)count;
#endif
	}
	if (hInst2 != hInst && hInst2 != 0) {
		FreeLibrary(hInst2);
	}
	return hIcon;
}


/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 */
HICON ExtractAssociatedIcon(HINSTANCE hInst,LPSTR lpIconPath, LPWORD lpiIcon)
{
    dprintf_reg(stdnimp, "ExtractAssociatedIcon : Empty Stub !!!\n");
    return 0;
}

/*************************************************************************
 *              DoEnvironmentSubst      [SHELL.37]
 */
DWORD DoEnvironmentSubst(LPSTR str,WORD len)
{
    dprintf_reg(stdnimp, "DoEnvironmentSubst(%s,%x): Empty Stub !!!\n",str,len);
    return 0;
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
