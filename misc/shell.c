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
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

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
