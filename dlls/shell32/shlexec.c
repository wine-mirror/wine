/*
 * 				Shell Library Functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2002 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "heap.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "ddeml.h"

#include "wine/winbase16.h"
#include "shell32_main.h"
#include "undocshell.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(exec);

/***********************************************************************
 * this function is supposed to expand the escape sequences found in the registry
 * some diving reported that the following were used:
 * + %1, %2...  seem to report to parameter of index N in ShellExecute pmts
 *	%1 file
 *	%2 printer
 *	%3 driver
 *	%4 port
 * %I adress of a global item ID (explorer switch /idlist)
 * %L seems to be %1 as long filename followed by the 8+3 variation
 * %S ???
 * %* all following parameters (see batfile)
 */
static BOOL argify(char* res, int len, const char* fmt, const char* lpFile)
{
    char        xlpFile[1024];
    BOOL        done = FALSE;

    while (*fmt)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
            case '\0':
            case '%':
                *res++ = '%';
                break;
            case '1':
            case '*':
                if (!done || (*fmt == '1'))
                {
                    if (SearchPathA(NULL, lpFile, ".exe", sizeof(xlpFile), xlpFile, NULL))
                    {
                        strcpy(res, xlpFile);
                        res += strlen(xlpFile);
                    }
                    else
                    {
                        strcpy(res, lpFile);
                        res += strlen(lpFile);
                    }
                }
                break;
            default: FIXME("Unknown escape sequence %%%c\n", *fmt);
            }
            fmt++;
            done = TRUE;
        }
        else
            *res++ = *fmt++;
    }
    *res = '\0';
    return done;
}

/*************************************************************************
 *	SHELL_ExecuteA [Internal]
 *
 */
static UINT SHELL_ExecuteA(char *lpCmd, LPSHELLEXECUTEINFOA sei, BOOL shWait)
{
    STARTUPINFOA  startup;
    PROCESS_INFORMATION info;
    UINT retval = 31;

    TRACE("Execute %s from directory %s\n", lpCmd, sei->lpDirectory);
    ZeroMemory(&startup,sizeof(STARTUPINFOA));
    startup.cb = sizeof(STARTUPINFOA);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = sei->nShow;
    if (CreateProcessA(NULL, lpCmd, NULL, NULL, FALSE, 0,
                       NULL, sei->lpDirectory, &startup, &info))
    {
        /* Give 30 seconds to the app to come up, if desired. Probably only needed
           when starting app immediately before making a DDE connection. */
        if (shWait)
            if (WaitForInputIdle( info.hProcess, 30000 ) == -1)
                WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        retval = 33;
        if(sei->fMask & SEE_MASK_NOCLOSEPROCESS)
            sei->hProcess = info.hProcess;
        else
            CloseHandle( info.hProcess );
        CloseHandle( info.hThread );
    }
    else if ((retval = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", retval);
        retval = ERROR_BAD_FORMAT;
    }

    sei->hInstApp = (HINSTANCE)retval;
    return retval;
}

/***********************************************************************
 *           SHELL_TryAppPath
 *
 * Helper function for SHELL_FindExecutable
 * @param lpResult - pointer to a buffer of size MAX_PATH
 * On entry: szName is a filename (probably without path separators).
 * On exit: if szName found in "App Path", place full path in lpResult, and return true
 */
static BOOL SHELL_TryAppPath( LPCSTR szName, LPSTR lpResult)
{
    HKEY hkApp = 0;
    char szAppKey[256];
    LONG len;
    LONG res; 
    BOOL found = FALSE;

    sprintf(szAppKey, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s", szName);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szAppKey, 0, KEY_READ, &hkApp);
    if (res) {
        /*TRACE("RegOpenKeyExA(HKEY_LOCAL_MACHINE, %s,) returns %ld\n", szAppKey, res);*/
        goto end;
    }

    len = MAX_PATH;
    res = RegQueryValueA(hkApp, NULL, lpResult, &len);
    if (res) {
        /*TRACE("RegQueryValueA(hkApp, NULL,) returns %ld\n", res);*/
        goto end;
    }
    /*TRACE("%s -> %s\n", szName, lpResult);*/
    found = TRUE;

end:
    if (hkApp) RegCloseKey(hkApp);
    return found;
}

/*************************************************************************
 *	SHELL_FindExecutable [Internal]
 *
 * Utility for code sharing between FindExecutable and ShellExecute
 * in:
 *      lpFile the name of a file
 *      lpOperation the operation on it (open)
 * out:
 *      lpResult a buffer, big enough :-(, to store the command to do the
 *              operation on the file
 *      key a buffer, big enough, to get the key name to do actually the
 *              command (it'll be used afterwards for more information
 *              on the operation)
 */
static UINT SHELL_FindExecutable(LPCSTR lpPath, LPCSTR lpFile, LPCSTR lpOperation,
                                 LPSTR lpResult, LPSTR key)
{
    char *extension = NULL; /* pointer to file extension */
    char tmpext[5];         /* local copy to mung as we please */
    char filetype[256];     /* registry name for this filetype */
    LONG filetypelen = 256; /* length of above */
    char command[256];      /* command from registry */
    LONG commandlen = 256;  /* This is the most DOS can handle :) */
    char buffer[256];       /* Used to GetProfileString */
    UINT retval = 31;  /* default - 'No association was found' */
    char *tok;              /* token pointer */
    char xlpFile[256] = ""; /* result of SearchPath */

    TRACE("%s\n", (lpFile != NULL) ? lpFile : "-");

    lpResult[0] = '\0'; /* Start off with an empty return string */
    if (key) *key = '\0';

    /* trap NULL parameters on entry */
    if ((lpFile == NULL) || (lpResult == NULL) || (lpOperation == NULL))
    {
        WARN("(lpFile=%s,lpResult=%s,lpOperation=%s): NULL parameter\n",
             lpFile, lpOperation, lpResult);
        return 2; /* File not found. Close enough, I guess. */
    }

    if (SHELL_TryAppPath( lpFile, lpResult ))
    {
        TRACE("found %s via App Paths\n", lpResult);
        return 33;
    }

    if (SearchPathA(lpPath, lpFile, ".exe", sizeof(xlpFile), xlpFile, NULL))
    {
        TRACE("SearchPathA returned non-zero\n");
        lpFile = xlpFile;
        /* Hey, isn't this value ignored?  Why make this call?  Shouldn't we return here?  --dank*/
    }

    /* First thing we need is the file's extension */
    extension = strrchr(xlpFile, '.'); /* Assume last "." is the one; */
                                       /* File->Run in progman uses */
                                       /* .\FILE.EXE :( */
    TRACE("xlpFile=%s,extension=%s\n", xlpFile, extension);

    if ((extension == NULL) || (extension == &xlpFile[strlen(xlpFile)]))
    {
        WARN("Returning 31 - No association\n");
        return 31; /* no association */
    }

    /* Make local copy & lowercase it for reg & 'programs=' lookup */
    lstrcpynA(tmpext, extension, 5);
    CharLowerA(tmpext);
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
    if (GetProfileStringA("windows", "programs", "exe pif bat com",
                          buffer, sizeof(buffer)) > 0)
    {
        UINT i;

        for (i = 0;i<strlen(buffer); i++) buffer[i] = tolower(buffer[i]);

        tok = strtok(buffer, " \t"); /* ? */
        while (tok!= NULL)
        {
            if (strcmp(tok, &tmpext[1]) == 0) /* have to skip the leading "." */
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
            tok = strtok(NULL, " \t");
        }
    }

    /* Check registry */
    if (RegQueryValueA(HKEY_CLASSES_ROOT, tmpext, filetype,
                       &filetypelen) == ERROR_SUCCESS)
    {
	filetype[filetypelen] = '\0';
	TRACE("File type: %s\n", filetype);

	/* Looking for ...buffer\shell\lpOperation\command */
	strcat(filetype, "\\shell\\");
	strcat(filetype, lpOperation);
	strcat(filetype, "\\command");

	if (RegQueryValueA(HKEY_CLASSES_ROOT, filetype, command,
                           &commandlen) == ERROR_SUCCESS)
	{
            if (key) strcpy(key, filetype);
#if 0
            LPSTR tmp;
            char param[256];
	    LONG paramlen = 256;

            /* FIXME: it seems all Windows version don't behave the same here.
             * the doc states that this ddeexec information can be found after
             * the exec names.
             * on Win98, it doesn't appear, but I think it does on Win2k
             */
	    /* Get the parameters needed by the application
	       from the associated ddeexec key */
	    tmp = strstr(filetype, "command");
	    tmp[0] = '\0';
	    strcat(filetype, "ddeexec");

	    if (RegQueryValueA(HKEY_CLASSES_ROOT, filetype, param, &paramlen) == ERROR_SUCCESS)
	    {
                strcat(command, " ");
                strcat(command, param);
                commandlen += paramlen;
	    }
#endif
	    command[commandlen] = '\0';
            argify(lpResult, sizeof(lpResult), command, xlpFile);
	    retval = 33; /* FIXME see above */
	}
    }
    else /* Check win.ini */
    {
	/* Toss the leading dot */
	extension++;
	if (GetProfileStringA("extensions", extension, "", command,
                              sizeof(command)) > 0)
        {
            if (strlen(command) != 0)
            {
                strcpy(lpResult, command);
                tok = strstr(lpResult, "^"); /* should be ^.extension? */
                if (tok != NULL)
                {
                    tok[0] = '\0';
                    strcat(lpResult, xlpFile); /* what if no dir in xlpFile? */
                    tok = strstr(command, "^"); /* see above */
                    if ((tok != NULL) && (strlen(tok)>5))
                    {
                        strcat(lpResult, &tok[5]);
                    }
                }
                retval = 33; /* FIXME - see above */
            }
        }
    }

    TRACE("returning %s\n", lpResult);
    return retval;
}

/******************************************************************
 *		dde_cb
 *
 * callback for the DDE connection. not really usefull
 */
static HDDEDATA CALLBACK dde_cb(UINT uType, UINT uFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2,
                                HDDEDATA hData, DWORD dwData1, DWORD dwData2)
{
    return NULL;
}

/******************************************************************
 *		dde_connect
 *
 * ShellExecute helper. Used to do an operation with a DDE connection
 *
 * Handles both the direct connection (try #1), and if it fails,
 * launching an application and trying (#2) to connect to it
 *
 */
static unsigned dde_connect(char* key, char* start, char* ddeexec,
                            const char* lpFile,
                            LPSHELLEXECUTEINFOA sei, SHELL_ExecuteA1632 execfunc)
{
    char*       endkey = key + strlen(key);
    char        app[256], topic[256], ifexec[256], res[256];
    LONG        applen, topiclen, ifexeclen;
    char*       exec;
    DWORD       ddeInst = 0;
    DWORD       tid;
    HSZ         hszApp, hszTopic;
    HCONV       hConv;
    unsigned    ret = 31;

    strcpy(endkey, "\\application");
    applen = sizeof(app);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, app, &applen) != ERROR_SUCCESS)
    {
        FIXME("default app name NIY %s\n", key);
        return 2;
    }

    strcpy(endkey, "\\topic");
    topiclen = sizeof(topic);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, topic, &topiclen) != ERROR_SUCCESS)
    {
        strcpy(topic, "System");
    }

    if (DdeInitializeA(&ddeInst, dde_cb, APPCMD_CLIENTONLY, 0L) != DMLERR_NO_ERROR)
    {
        return 2;
    }

    hszApp = DdeCreateStringHandleA(ddeInst, app, CP_WINANSI);
    hszTopic = DdeCreateStringHandleA(ddeInst, topic, CP_WINANSI);

    hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
    exec = ddeexec;
    if (!hConv)
    {
        TRACE("Launching '%s'\n", start);
        ret = execfunc(start, sei, TRUE);
        if (ret < 32)
        {
            TRACE("Couldn't launch\n");
            goto error;
        }
        hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
        if (!hConv)
        {
            TRACE("Couldn't connect. ret=%d\n", ret);
            ret = 30; /* whatever */
            goto error;
        }
        strcpy(endkey, "\\ifexec");
        ifexeclen = sizeof(ifexec);
        if (RegQueryValueA(HKEY_CLASSES_ROOT, key, ifexec, &ifexeclen) == ERROR_SUCCESS)
        {
            exec = ifexec;
        }
    }

    argify(res, sizeof(res), exec, lpFile);
    TRACE("%s %s => %s\n", exec, lpFile, res);

    ret = (DdeClientTransaction(res, strlen(res) + 1, hConv, 0L, 0,
                                XTYP_EXECUTE, 10000, &tid) != DMLERR_NO_ERROR) ? 31 : 33;
    DdeDisconnect(hConv);
 error:
    DdeUninitialize(ddeInst);
    return ret;
}

/*************************************************************************
 *	execute_from_key [Internal]
 */
static UINT execute_from_key(LPSTR key, LPCSTR lpFile, LPSHELLEXECUTEINFOA sei, SHELL_ExecuteA1632 execfunc)
{
    char cmd[1024] = "";
    LONG cmdlen = sizeof(cmd);
    UINT retval = 31;

    /* Get the application for the registry */
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, cmd, &cmdlen) == ERROR_SUCCESS)
    {
        LPSTR tmp;
        char param[256] = "";
        LONG paramlen = 256;

        /* Get the parameters needed by the application
           from the associated ddeexec key */
        tmp = strstr(key, "command");
        assert(tmp);
        strcpy(tmp, "ddeexec");

        if (RegQueryValueA(HKEY_CLASSES_ROOT, key, param, &paramlen) == ERROR_SUCCESS)
        {
            TRACE("Got ddeexec %s => %s\n", key, param);
            retval = dde_connect(key, cmd, param, lpFile, sei, execfunc);
        }
        else
        {
            /* Is there a replace() function anywhere? */
            cmd[cmdlen] = '\0';
            argify(param, sizeof(param), cmd, lpFile);
            retval = execfunc(param, sei, FALSE);
        }
    }
    else TRACE("ooch\n");

    return retval;
}

/*************************************************************************
 * FindExecutableA			[SHELL32.@]
 */
HINSTANCE WINAPI FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    UINT retval = 31;    /* default - 'No association was found' */
    char old_dir[1024];

    TRACE("File %s, Dir %s\n",
          (lpFile != NULL ? lpFile : "-"), (lpDirectory != NULL ? lpDirectory : "-"));

    lpResult[0] = '\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if ((lpFile == NULL) || (lpResult == NULL))
    {
        /* FIXME - should throw a warning, perhaps! */
	return (HINSTANCE)2; /* File not found. Close enough, I guess. */
    }

    if (lpDirectory)
    {
        GetCurrentDirectoryA(sizeof(old_dir), old_dir);
        SetCurrentDirectoryA(lpDirectory);
    }

    retval = SHELL_FindExecutable(lpDirectory, lpFile, "open", lpResult, NULL);

    TRACE("returning %s\n", lpResult);
    if (lpDirectory)
        SetCurrentDirectoryA(old_dir);
    return (HINSTANCE)retval;
}

/*************************************************************************
 * FindExecutableW			[SHELL32.@]
 */
HINSTANCE WINAPI FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult)
{
    FIXME("(%p,%p,%p): stub\n", lpFile, lpDirectory, lpResult);
    return (HINSTANCE)31;    /* default - 'No association was found' */
}

/*************************************************************************
 *	ShellExecuteExA32 [Internal]
 */
BOOL WINAPI ShellExecuteExA32 (LPSHELLEXECUTEINFOA sei, SHELL_ExecuteA1632 execfunc)
{
    CHAR szApplicationName[MAX_PATH],szCommandline[MAX_PATH],szPidl[20],fileName[MAX_PATH];
    LPSTR pos;
    int gap, len;
    char lpstrProtocol[256];
    LPCSTR lpFile,lpOperation;
    UINT retval = 31;
    char cmd[1024];
    BOOL done;

    TRACE("mask=0x%08lx hwnd=%p verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s\n",
            sei->fMask, sei->hwnd, debugstr_a(sei->lpVerb),
            debugstr_a(sei->lpFile), debugstr_a(sei->lpParameters),
            debugstr_a(sei->lpDirectory), sei->nShow,
            (sei->fMask & SEE_MASK_CLASSNAME) ? debugstr_a(sei->lpClass) : "not used");

    sei->hProcess = (HANDLE)NULL;
    ZeroMemory(szApplicationName,MAX_PATH);
    if (sei->lpFile)
        strcpy(szApplicationName, sei->lpFile);

    ZeroMemory(szCommandline,MAX_PATH);
    if (sei->lpParameters)
        strcpy(szCommandline, sei->lpParameters);

    if (sei->fMask & ((SEE_MASK_CLASSKEY & ~SEE_MASK_CLASSNAME) |
        SEE_MASK_INVOKEIDLIST | SEE_MASK_ICON | SEE_MASK_HOTKEY |
        SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
        SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE |
        SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
    {
        FIXME("flags ignored: 0x%08lx\n", sei->fMask);
    }

    /* process the IDList */
    if ( (sei->fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST) /*0x0c*/
    {
        SHGetPathFromIDListA (sei->lpIDList,szApplicationName);
        TRACE("-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
    }
    else
    {
        if (sei->fMask & SEE_MASK_IDLIST )
        {
            pos = strstr(szCommandline, "%I");
            if (pos)
            {
                LPVOID pv;
                HGLOBAL hmem = SHAllocShared ( sei->lpIDList, ILGetSize(sei->lpIDList), 0);
                pv = SHLockShared(hmem,0);
                sprintf(szPidl,":%p",pv );
                SHUnlockShared(pv);

                gap = strlen(szPidl);
                len = strlen(pos)-2;
                memmove(pos+gap,pos+2,len);
                memcpy(pos,szPidl,gap);
            }
        }
    }

    if (sei->fMask & SEE_MASK_CLASSNAME)
    {
	/* launch a document by fileclass like 'WordPad.Document.1' */
        /* the Commandline contains 'c:\Path\wordpad.exe "%1"' */
        /* FIXME: szCommandline should not be of a fixed size. Plus MAX_PATH is way too short! */
        HCR_GetExecuteCommandA(sei->lpClass, (sei->lpVerb) ? sei->lpVerb : "open", szCommandline, sizeof(szCommandline));
        /* FIXME: get the extension of lpFile, check if it fits to the lpClass */
        TRACE("SEE_MASK_CLASSNAME->'%s', doc->'%s'\n", szCommandline, szApplicationName);

        cmd[0] = '\0';
        done = argify(cmd, sizeof(cmd), szCommandline, szApplicationName);
        if (!done && szApplicationName[0])
        {
            strcat(cmd, " ");
            strcat(cmd, szApplicationName);
        }
        retval = execfunc(cmd, sei, FALSE);
        if (retval > 32)
            return TRUE;
        else
            return FALSE;
    }

    /* We set the default to open, and that should generally work.
       But that is not really the way the MS docs say to do it. */
    if (sei->lpVerb == NULL)
        lpOperation = "open";
    else
        lpOperation = sei->lpVerb;

    /* Else, try to execute the filename */
    TRACE("execute:'%s','%s'\n",szApplicationName, szCommandline);

    strcpy(fileName, szApplicationName);
    lpFile = fileName;
    if (szCommandline[0]) {
        strcat(szApplicationName, " ");
        strcat(szApplicationName, szCommandline);
    }

    retval = execfunc(szApplicationName, sei, FALSE);
    if (retval > 32)
        return TRUE;

    /* Else, try to find the executable */
    cmd[0] = '\0';
    retval = SHELL_FindExecutable(sei->lpDirectory, lpFile, lpOperation, cmd, lpstrProtocol);
    if (retval > 32)  /* Found */
    {
        CHAR szQuotedCmd[MAX_PATH+2];
        /* Must quote to handle case where cmd contains spaces, 
         * else security hole if malicious user creates executable file "C:\\Program"
         */
        if (szCommandline[0])
            sprintf(szQuotedCmd, "\"%s\" %s", cmd, szCommandline);
        else
            sprintf(szQuotedCmd, "\"%s\"", cmd);
        TRACE("%s/%s => %s/%s\n", szApplicationName, lpOperation, szQuotedCmd, lpstrProtocol);
        if (*lpstrProtocol)
            retval = execute_from_key(lpstrProtocol, szApplicationName, sei, execfunc);
        else
            retval = execfunc(szQuotedCmd, sei, FALSE);
    }
    else if (PathIsURLA((LPSTR)lpFile))    /* File not found, check for URL */
    {
        LPSTR lpstrRes;
        INT iSize;

        lpstrRes = strchr(lpFile, ':');
        if (lpstrRes)
            iSize = lpstrRes - lpFile;
        else
            iSize = strlen(lpFile);

        TRACE("Got URL: %s\n", lpFile);
        /* Looking for ...protocol\shell\lpOperation\command */
        strncpy(lpstrProtocol, lpFile, iSize);
        lpstrProtocol[iSize] = '\0';
        strcat(lpstrProtocol, "\\shell\\");
        strcat(lpstrProtocol, lpOperation);
        strcat(lpstrProtocol, "\\command");

        /* Remove File Protocol from lpFile */
        /* In the case file://path/file     */
        if (!strncasecmp(lpFile, "file", iSize))
        {
            lpFile += iSize;
            while (*lpFile == ':') lpFile++;
        }
        retval = execute_from_key(lpstrProtocol, lpFile, sei, execfunc);
    }
    /* Check if file specified is in the form www.??????.*** */
    else if (!strncasecmp(lpFile, "www", 3))
    {
        /* if so, append lpFile http:// and call ShellExecute */
        char lpstrTmpFile[256] = "http://" ;
        strcat(lpstrTmpFile, lpFile);
        retval = (UINT)ShellExecuteA(sei->hwnd, lpOperation, lpstrTmpFile, NULL, NULL, 0);
    }

    if (retval <= 32)
    {
        sei->hInstApp = (HINSTANCE)retval;
        return FALSE;
    }

    sei->hInstApp = (HINSTANCE)33;
    return TRUE;
}

/*************************************************************************
 * ShellExecuteA			[SHELL32.290]
 */
HINSTANCE WINAPI ShellExecuteA(HWND hWnd, LPCSTR lpOperation,LPCSTR lpFile,
                               LPCSTR lpParameters,LPCSTR lpDirectory, INT iShowCmd)
{
    SHELLEXECUTEINFOA sei;
    HANDLE hProcess = 0;

    TRACE("\n");
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = hWnd;
    sei.lpVerb = lpOperation;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = iShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = hProcess;

    ShellExecuteExA32 (&sei, SHELL_ExecuteA);
    return sei.hInstApp;
}

/*************************************************************************
 * ShellExecuteEx				[SHELL32.291]
 *
 */
BOOL WINAPI ShellExecuteExAW (LPVOID sei)
{
    if (SHELL_OsIsUnicode())
	return ShellExecuteExW (sei);
    return ShellExecuteExA32 (sei, SHELL_ExecuteA);
}

/*************************************************************************
 * ShellExecuteExA				[SHELL32.292]
 *
 */
BOOL WINAPI ShellExecuteExA (LPSHELLEXECUTEINFOA sei)
{
    return  ShellExecuteExA32 (sei, SHELL_ExecuteA);
}

/*************************************************************************
 * ShellExecuteExW				[SHELL32.293]
 *
 */
BOOL WINAPI ShellExecuteExW (LPSHELLEXECUTEINFOW sei)
{
    SHELLEXECUTEINFOA seiA;
    DWORD ret;

    TRACE("%p\n", sei);

    memcpy(&seiA, sei, sizeof(SHELLEXECUTEINFOA));

    if (sei->lpVerb)
        seiA.lpVerb = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpVerb);

    if (sei->lpFile)
        seiA.lpFile = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpFile);

    if (sei->lpParameters)
        seiA.lpParameters = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpParameters);

    if (sei->lpDirectory)
        seiA.lpDirectory = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpDirectory);

    if ((sei->fMask & SEE_MASK_CLASSNAME) && sei->lpClass)
        seiA.lpClass = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpClass);
    else
        seiA.lpClass = NULL;

    ret = ShellExecuteExA(&seiA);

    if (seiA.lpVerb)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpVerb );
    if (seiA.lpFile)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpFile );
    if (seiA.lpParameters)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpParameters );
    if (seiA.lpDirectory)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpDirectory );
    if (seiA.lpClass)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpClass );

    return ret;
}

/*************************************************************************
 * ShellExecuteW			[SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpOperation,
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
 */
HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
    SHELLEXECUTEINFOW sei;
    HANDLE hProcess = 0;

    TRACE("\n");
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = hwnd;
    sei.lpVerb = lpOperation;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = nShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = hProcess;

    ShellExecuteExW (&sei);
    return sei.hInstApp;
}
