/*
 * 				Shell Library Functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "shell.h"
#include "neexe.h"
#include "selectors.h"
#include "../rc/sysres.h"
#include "dlgs.h"
#include "dialog.h"
#include "stddebug.h"
/* #define DEBUG_REG */
#include "debug.h"

LPKEYSTRUCT	lphRootKey = NULL,lphTopKey = NULL;

static char RootKeyName[]=".classes", TopKeyName[] = "(null)";

/*************************************************************************
 *                        SHELL_RegCheckForRoot()     internal use only
 */
static LONG SHELL_RegCheckForRoot()
{
    HKEY hNewKey;

    if (lphRootKey == NULL){
      hNewKey = GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
      lphRootKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
      if (lphRootKey == NULL) {
        printf("SHELL_RegCheckForRoot: Couldn't allocate root key!\n");
        return ERROR_OUTOFMEMORY;
      }
      lphRootKey->hKey = 1;
      lphRootKey->lpSubKey = RootKeyName;
      lphRootKey->dwType = 0;
      lphRootKey->lpValue = NULL;
      lphRootKey->lpSubLvl = lphRootKey->lpNextKey = lphRootKey->lpPrevKey = NULL;

      hNewKey = GlobalAlloc(GMEM_MOVEABLE,sizeof(KEYSTRUCT));
      lphTopKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
      if (lphTopKey == NULL) {
        printf("SHELL_RegCheckForRoot: Couldn't allocate top key!\n");
        return ERROR_OUTOFMEMORY;
      }
      lphTopKey->hKey = 0;
      lphTopKey->lpSubKey = TopKeyName;
      lphTopKey->dwType = 0;
      lphTopKey->lpValue = NULL;
      lphTopKey->lpSubLvl = lphRootKey;
      lphTopKey->lpNextKey = lphTopKey->lpPrevKey = NULL;

      dprintf_reg(stddeb,"SHELL_RegCheckForRoot: Root/Top created\n");
    }
    return ERROR_SUCCESS;
}

/*************************************************************************
 *				RegOpenKey		[SHELL.1]
 */
LONG RegOpenKey(HKEY hKey, LPCSTR lpSubKey, HKEY FAR *lphKey)
{
	LPKEYSTRUCT	lpKey;
	LPCSTR		ptr;
	char		str[128];
	LONG            dwRet;

        dwRet = SHELL_RegCheckForRoot();
        if (dwRet != ERROR_SUCCESS) return dwRet;
	dprintf_reg(stddeb, "RegOpenKey(%08lX, %p='%s', %p)\n",
						hKey, lpSubKey, lpSubKey, lphKey);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
        switch(hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegOpenKey // specific key = %08lX !\n", hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }
        if (!*lpSubKey)  { *lphKey = hKey; return ERROR_SUCCESS; }
        while(*lpSubKey) {
          ptr = strchr(lpSubKey,'\\');
          if (!ptr) ptr = lpSubKey + strlen(lpSubKey);
          strncpy(str,lpSubKey,ptr-lpSubKey);
          str[ptr-lpSubKey] = 0;
          lpSubKey = ptr; 
          if (*lpSubKey) lpSubKey++;
	  
	  lpKey = lpKey->lpSubLvl;
          while(lpKey != NULL && strcmp(lpKey->lpSubKey, str) != 0) { lpKey = lpKey->lpNextKey; }
          if (lpKey == NULL) {
	    dprintf_reg(stddeb,"RegOpenKey: key %s not found!\n",str);
	    return ERROR_BADKEY;
	  }	    
	}
        *lphKey = lpKey->hKey;
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCreateKey		[SHELL.2]
 */
LONG RegCreateKey(HKEY hKey, LPCSTR lpSubKey, HKEY FAR *lphKey)
{
	HKEY		hNewKey;
	LPKEYSTRUCT	lpNewKey;
	LPKEYSTRUCT	lpKey;
	LPKEYSTRUCT	lpPrevKey;
	LONG		dwRet;
	LPCSTR		ptr;
	char		str[128];

	dwRet = SHELL_RegCheckForRoot();
        if (dwRet != ERROR_SUCCESS) return dwRet;
	dprintf_reg(stddeb, "RegCreateKey(%08lX, '%s', %p)\n",	hKey, lpSubKey, lphKey);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
        switch(hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegCreateKey // specific key = %08lX !\n", hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }
        if (!*lpSubKey)  { *lphKey = hKey; return ERROR_SUCCESS; }
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
	      return ERROR_OUTOFMEMORY;
	    }
	    lpNewKey->hKey = hNewKey;
	    lpNewKey->lpSubKey = malloc(strlen(str) + 1);
	    if (lpNewKey->lpSubKey == NULL) {
	      printf("RegCreateKey // Can't alloc key string !\n");
	      return ERROR_OUTOFMEMORY;
	    }
	    strcpy(lpNewKey->lpSubKey, str);
	    lpNewKey->lpNextKey = lpPrevKey->lpSubLvl;
	    lpNewKey->lpPrevKey = NULL;
            lpPrevKey->lpSubLvl = lpNewKey;

	    lpNewKey->dwType = 0;
	    lpNewKey->lpValue = NULL;
	    lpNewKey->lpSubLvl = NULL;
	    *lphKey = hNewKey;
	    dprintf_reg(stddeb,"RegCreateKey // successful '%s' key=%08lX !\n", str, hNewKey);
	    lpKey = lpNewKey;
	  } else {
            *lphKey = lpKey->hKey;
	    dprintf_reg(stddeb,"RegCreateKey // found '%s', key=%08lX\n", str, *lphKey);
	  }
	}
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCloseKey		[SHELL.3]
 */
LONG RegCloseKey(HKEY hKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegCloseKey(%08lX);\n", hKey);
	return ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				RegDeleteKey		[SHELL.4]
 */
LONG RegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegDeleteKey(%08lX, '%s');\n", 
												hKey, lpSubKey);
	return ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				RegSetValue		[SHELL.5]
 */
LONG RegSetValue(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, 
					LPCSTR lpVal, DWORD dwIgnored)
{
	HKEY		hRetKey;
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	dprintf_reg(stddeb, "RegSetValue(%08lX, '%s', %08lX, '%s', %08lX);\n",
						hKey, lpSubKey, dwType, lpVal, dwIgnored);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lpVal == NULL) return ERROR_INVALID_PARAMETER;
	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
	        dprintf_reg(stddeb, "RegSetValue // key not found ... so create it !\n");
		if ((dwRet = RegCreateKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
			fprintf(stderr, "RegSetValue // key creation error %08lX !\n", dwRet);
			return dwRet;
			}
		}
	lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
	if (lpKey == NULL) return ERROR_BADKEY;
	if (lpKey->lpValue != NULL) free(lpKey->lpValue);
	lpKey->lpValue = malloc(strlen(lpVal) + 1);
	strcpy(lpKey->lpValue, lpVal);
	dprintf_reg(stddeb,"RegSetValue // successful key='%s' val='%s' !\n", lpSubKey, lpKey->lpValue);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegQueryValue		[SHELL.6]
 */
LONG RegQueryValue(HKEY hKey, LPCSTR lpSubKey, LPSTR lpVal, LONG FAR *lpcb)
{
	HKEY		hRetKey;
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	int			size;
	dprintf_reg(stddeb, "RegQueryValue(%08lX, '%s', %p, %p);\n",
							hKey, lpSubKey, lpVal, lpcb);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lpVal == NULL) return ERROR_INVALID_PARAMETER;
	if (lpcb == NULL) return ERROR_INVALID_PARAMETER;
        if (!*lpcb) return ERROR_INVALID_PARAMETER;

	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
		fprintf(stderr, "RegQueryValue // key not found !\n");
		return dwRet;
		}
	lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
	if (lpKey == NULL) return ERROR_BADKEY;
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
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegEnumKey		[SHELL.7]
 */
LONG RegEnumKey(HKEY hKey, DWORD dwSubKey, LPSTR lpBuf, DWORD dwSize)
{
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	LONG            len;

	dwRet = SHELL_RegCheckForRoot();
        if (dwRet != ERROR_SUCCESS) return dwRet;
	dprintf_reg(stddeb, "RegEnumKey(%08lX, %ld)\n", hKey, dwSubKey);
	if (lpBuf == NULL) return ERROR_INVALID_PARAMETER;
        switch(hKey) {
	case 0: 
	  lpKey = lphTopKey; break;
        case HKEY_CLASSES_ROOT: /* == 1 */
          lpKey = lphRootKey; break;
        default: 
	  dprintf_reg(stddeb,"RegEnumKey // specific key = %08lX !\n", hKey);
	  lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
        }

        lpKey = lpKey->lpSubLvl;
        while(lpKey != NULL){
          if (!dwSubKey){
            len = min(dwSize-1,strlen(lpKey->lpSubKey));
	    strncpy(lpBuf,lpKey->lpSubKey,len);
	    lpBuf[len] = 0;
            dprintf_reg(stddeb, "RegEnumKey: found %s\n",lpBuf);
	    return ERROR_SUCCESS;
	  }
          dwSubKey--;
	  lpKey = lpKey->lpNextKey;
        }
	dprintf_reg(stddeb, "RegEnumKey: key not found!\n");
	return ERROR_INVALID_PARAMETER;
}

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void DragAcceptFiles(HWND hWnd, BOOL b)
{
	fprintf(stdnimp, "DragAcceptFiles : Empty Stub !!!\n");
}


/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
void DragQueryFile(HDROP h, UINT u, LPSTR u2, UINT u3)
{
	fprintf(stdnimp, "DragQueryFile : Empty Stub !!!\n");

}


/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void DragFinish(HDROP h)
{
	fprintf(stdnimp, "DragFinish : Empty Stub !!!\n");
}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL DragQueryPoint(HDROP h, POINT FAR *p)
{
	fprintf(stdnimp, "DragQueryPoint : Empty Stub !!!\n");
        return FALSE;
}


/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd)
{
        fprintf(stdnimp, "ShellExecute: empty stub\n");
        return 2;
	fprintf(stdnimp, "ShellExecute // hWnd=%04X\n", hWnd);  
	fprintf(stdnimp, "ShellExecute // lpOperation='%s'\n", lpOperation);
	fprintf(stdnimp, "ShellExecute // lpFile='%s'\n", lpFile);
	fprintf(stdnimp, "ShellExecute // lpParameters='%s'\n", lpParameters);
	fprintf(stdnimp, "ShellExecute // lpDirectory='%s'\n", lpDirectory);
	fprintf(stdnimp, "ShellExecute // iShowCmd=%04X\n", iShowCmd);
	return 2; /* file not found */
}


/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	fprintf(stdnimp, "FindExecutable : Empty Stub !!!\n");
        return 0;
}

static char AppName[512], AppMisc[512];

/*************************************************************************
 *				AboutDlgProc		[SHELL.33]
 */
INT AboutDlgProc(HWND hWnd, WORD msg, WORD wParam, LONG lParam)
{
  switch(msg) {
   case WM_INITDIALOG:
    SendDlgItemMessage(hWnd,stc1,STM_SETICON,LOWORD(lParam),0);
    SetWindowText(hWnd, AppName);
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
  if (szApp) {
    sprintf(AppName, "About %s", szApp);
  } else  {
    *AppName = 0;
  }
  if (szOtherStuff) {
    strcpy(AppMisc, szOtherStuff);
  } else {
    *AppMisc = 0;
  }
  if (!hIcon) {
    hIcon = LoadIcon(0,MAKEINTRESOURCE(OIC_WINEICON));
  }
  return DialogBoxIndirectParamPtr(GetWindowWord(hWnd, GWW_HINSTANCE),
				   sysres_DIALOG_SHELL_ABOUT_MSGBOX,
				   hWnd, GetWndProcEntry16("AboutDlgProc"),
				   hIcon);
}

/*************************************************************************
 *				ExtractIcon		[SHELL.34]
 */
HICON ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex)
{
	HICON	hIcon = 0;
	HINSTANCE hInst2 = hInst;
	dprintf_reg(stddeb, "ExtractIcon(%04X, '%s', %d\n", 
			hInst, lpszExeFileName, nIconIndex);
	if (lpszExeFileName != NULL) {
		hInst2 = LoadLibrary(lpszExeFileName);
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
 * I couldn't find any reference, so even the number of bytes on the
 * stack might be wrong
 */
WORD DoEnvironmentSubst(LPSTR a,WORD b,WORD c)
{
    printf(stderr, "DoEnvironmentSubst: Unknown argument count\n");
    dprintf_reg(stdnimp, "DoEnvironmentSubst %x %x %x\n",a,b,c);
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
