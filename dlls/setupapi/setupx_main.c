/*
 *      SETUPX library
 *
 *      Copyright 1998,2000  Andreas Mohr
 *
 * FIXME: Rather non-functional functions for now.
 *
 * See:
 * http://www.geocities.com/SiliconValley/Network/5317/drivers.html
 * http://willemer.de/informatik/windows/inf_info.htm (German)
 * http://www.microsoft.com/ddk/ddkdocs/win98ddk/devinst_12uw.htm
 * DDK: setupx.h
 * http://mmatrix.tripod.com/customsystemfolder/infsysntaxfull.html
 * http://www.rdrop.com/~cary/html/inf_faq.html
 * http://support.microsoft.com/support/kb/articles/q194/6/40.asp
 *
 * Stuff tested with:
 * - rs405deu.exe (German Acroread 4.05 setup)
 * - ie5setup.exe
 * - Netmeeting
 *
 * FIXME:
 * - string handling is... weird ;) (buflen etc.)
 * - memory leaks ?
 * - separate that mess (but probably only when it's done completely)
 *
 * SETUPX consists of several parts with the following acronyms/prefixes:
 * Di	device installer (devinst.c ?)
 * Gen	generic installer (geninst.c ?)
 * Ip	.INF parsing (infparse.c)
 * LDD	logical device descriptor (ldd.c ?)
 * LDID	logical device ID 
 * SU   setup (setup.c ?)
 * Tp	text processing (textproc.c ?)
 * Vcp	virtual copy module (vcp.c ?)
 * ...
 *
 * The SETUPX DLL is NOT thread-safe. That's why many installers urge you to
 * "close all open applications".
 * All in all the design of it seems to be a bit weak.
 * Not sure whether my implementation of it is better, though ;-)
 */

#include <stdlib.h>
#include <stdio.h>
#include "winreg.h"
#include "wine/winuser16.h"
#include "setupx16.h"
#include "setupx_private.h"
#include "winerror.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(setupx);

/***********************************************************************
 *		SURegOpenKey
 */
DWORD WINAPI SURegOpenKey( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    FIXME("(%x,%s,%p), semi-stub.\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegOpenKeyA( hkey, lpszSubKey, retkey );
}

/***********************************************************************
 *		SURegQueryValueEx
 */
DWORD WINAPI SURegQueryValueEx( HKEY hkey, LPSTR lpszValueName,
                                LPDWORD lpdwReserved, LPDWORD lpdwType,
                                LPBYTE lpbData, LPDWORD lpcbData )
{
    FIXME("(%x,%s,%p,%p,%p,%ld), semi-stub.\n",hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData?*lpcbData:0);
    return RegQueryValueExA( hkey, lpszValueName, lpdwReserved, lpdwType,
                               lpbData, lpcbData );
}

/*
 * Returns pointer to a string list with the first entry being number
 * of strings.
 *
 * Hmm. Should this be InitSubstrData(), GetFirstSubstr() and GetNextSubstr()
 * instead?
 */
static LPSTR *SETUPX_GetSubStrings(LPSTR start, char delimiter)
{
    LPSTR p, q;
    LPSTR *res = NULL;
    DWORD count = 0;
    int len;

    p = start;

    while (1)
    {
	/* find beginning of real substring */
	while ( (*p == ' ') || (*p == '\t') || (*p == '"') ) p++;

	/* find end of real substring */
	q = p;
	while ( (*q)
	     &&	(*q != ' ') && (*q != '\t') && (*q != '"')
	     && (*q != ';') && (*q != delimiter) ) q++;
	if (q == p)
	    break;
	len = (int)q - (int)p;

	/* alloc entry for new substring in steps of 32 units and copy over */
	if (count % 32 == 0)
	{ /* 1 for count field + current count + 32 */
	    res = HeapReAlloc(GetProcessHeap(), 0, res, (1+count+32)*sizeof(LPSTR));
	}
	*(res+1+count) = HeapAlloc(GetProcessHeap(), 0, len+1);
	strncpy(*(res+1+count), p, len);
	(*(res+1+count))[len] = '\0';
	count++;

	/* we are still within last substring (before delimiter),
	 * so get out of it */
	while ((*q) && (*q != ';') && (*q != delimiter)) q++;
	if ((!*q) || (*q == ';'))
	    break;
	p = q+1;
    }
    
    /* put number of entries at beginning of list */
    *(DWORD *)res = count;
    return res;
}

static void SETUPX_FreeSubStrings(LPSTR *substr)
{
    DWORD count = *(DWORD *)substr;
    LPSTR *pStrings = substr+1;
    DWORD n;

    for (n=0; n < count; n++)
	HeapFree(GetProcessHeap(), 0, *pStrings++);

    HeapFree(GetProcessHeap(), 0, substr);
}

static void SETUPX_IsolateSubString(LPSTR *begin, LPSTR *end)
{
    LPSTR p, q;

    p = *begin;
    q = *end;

    while ((p < q) && ((*p == ' ') || (*p == '\t'))) p++;
    while ((p < q) && (*p == '"')) p++;

    while ((q-1 >= p) && ((*(q-1) == ' ') || (*(q-1) == '\t'))) q--;
    while ((q-1 >= p) && (*(q-1) == '"')) q--;

    *begin = p;
    *end = q;
}

/*
 * Example: HKLM,"Software\Microsoft\Windows\CurrentVersion","ProgramFilesDir",,"C:\"
 * FIXME: use SETUPX_GetSubStrings() instead.
 *        Hmm, but on the other hand SETUPX_GetSubStrings() will probably
 *        soon be replaced by InitSubstrData() etc. anyway.
 * 
 */
static BOOL SETUPX_LookupRegistryString(LPSTR regstr, LPSTR buffer, DWORD buflen)
{
    HANDLE heap = GetProcessHeap();
    LPSTR items[5];
    LPSTR p, q, next;
    int len, n;
    HKEY hkey, hsubkey;
    DWORD dwType;

    TRACE("retrieving '%s'\n", regstr);

    p = regstr;

    /* isolate root key, subkey, value, flag, defval */
    for (n=0; n < 5; n++)
    {
	q = strchr(p, ',');
	if (!q)
	{
	    if (n == 4)
		q = p+strlen(p);
	    else
		return FALSE;
	}
	next = q+1;
	if (q < regstr)
            return FALSE;
        SETUPX_IsolateSubString(&p, &q);
        len = (int)q - (int)p;
        items[n] = HeapAlloc(heap, 0, len+1);
        strncpy(items[n], p, len);
        items[n][len] = '\0';
	p = next;
    }
    TRACE("got '%s','%s','%s','%s','%s'\n",
			items[0], items[1], items[2], items[3], items[4]);
    
    /* check root key */
    if (!strcasecmp(items[0], "HKCR"))
	hkey = HKEY_CLASSES_ROOT;
    else
    if (!strcasecmp(items[0], "HKCU"))
	hkey = HKEY_CURRENT_USER;
    else
    if (!strcasecmp(items[0], "HKLM"))
	hkey = HKEY_LOCAL_MACHINE;
    else
    if (!strcasecmp(items[0], "HKU"))
	hkey = HKEY_USERS;
    else
    { /* HKR ? -> relative to key passed to GenInstallEx */
	FIXME("unsupported regkey '%s'\n", items[0]);
        goto regfailed;
    }

    if (RegOpenKeyA(hkey, items[1], &hsubkey) != ERROR_SUCCESS)
        goto regfailed;

    if (RegQueryValueExA(hsubkey, items[2], NULL, &dwType, buffer, &buflen)
		!= ERROR_SUCCESS)
        goto regfailed;
    goto done;

regfailed:
    if (buffer) strcpy(buffer, items[4]); /* I don't care about buflen */
done:
    for (n=0; n < 5; n++)
        HeapFree(heap, 0, items[n]);
    if (buffer)
	TRACE("return '%s'\n", buffer);
    return TRUE;
}

static LPSTR SETUPX_GetSections(LPCSTR filename)
{
    LPSTR buf = NULL;
    DWORD len = 1024, res;

    do {
	buf = HeapReAlloc(GetProcessHeap(), 0, buf, len);
	res = GetPrivateProfileStringA(NULL, NULL, NULL, buf, len, filename);
	len *= 2;
    } while ((!res) && (len < 1048576));
    if (!res)
    {
	HeapFree(GetProcessHeap(), 0, buf);
	return NULL;
    }
    return buf;
}

static LPSTR SETUPX_GetSectionEntries(LPCSTR filename, LPCSTR section)
{
    LPSTR buf = NULL;
    DWORD len = 1024, res;

    do {
	buf = HeapReAlloc(GetProcessHeap(), 0, buf, len);
	res = GetPrivateProfileSectionA(section, buf, len, filename);
	len *= 2;
    } while ((!res) && (len < 1048576));
    if (!res)
    {
	HeapFree(GetProcessHeap(), 0, buf);
	return NULL;
    }
    return buf;
}


/***********************************************************************
 *		InstallHinfSection
 *
 * hwnd = parent window
 * hinst = instance of SETUPX.DLL
 * lpszCmdLine = e.g. "DefaultInstall 132 C:\MYINSTALL\MYDEV.INF"
 * Here "DefaultInstall" is the .inf file section to be installed (optional).
 * The 132 value is made of the HOW_xxx flags and sometimes 128 (-> setupx16.h).
 * 
 * nCmdShow = nCmdShow of CreateProcess
 */
typedef INT WINAPI (*MSGBOX_PROC)( HWND, LPCSTR, LPCSTR, UINT );
RETERR16 WINAPI InstallHinfSection16( HWND16 hwnd, HINSTANCE16 hinst, LPCSTR lpszCmdLine, INT16 nCmdShow)
{
    LPSTR *pSub;
    DWORD count;
    HINF16 hInf = 0;
    RETERR16 res = OK;
    WORD wFlags;
    BOOL reboot = FALSE;
    HMODULE hMod;
    MSGBOX_PROC pMessageBoxA;
    
    TRACE("(%04x, %04x, %s, %d);\n", hwnd, hinst, lpszCmdLine, nCmdShow);
    
    pSub = SETUPX_GetSubStrings((LPSTR)lpszCmdLine, ' ');

    count = *(DWORD *)pSub;
    if (count < 2) /* invalid number of arguments ? */
	goto end;
    if (IpOpen16(*(pSub+count), &hInf) != OK)
    {
	res = ERROR_FILE_NOT_FOUND; /* yes, correct */
	goto end;
    }
    if (GenInstall16(hInf, *(pSub+count-2), GENINSTALL_DO_ALL) != OK)
	goto end;
    wFlags = atoi(*(pSub+count-1)) & ~128;
    switch (wFlags)
    {
	case HOW_ALWAYS_SILENT_REBOOT:
	case HOW_SILENT_REBOOT:
	    reboot = TRUE;
	    break;
	case HOW_ALWAYS_PROMPT_REBOOT:
	case HOW_PROMPT_REBOOT:
	    if ((hMod = GetModuleHandleA("user32.dll")))
	    {
	      if ((pMessageBoxA = (MSGBOX_PROC)GetProcAddress( hMod, "MessageBoxA" )))
	      {
		 
	        if (pMessageBoxA(hwnd, "You must restart Wine before the new settings will take effect.\n\nDo you want to exit Wine now ?", "Systems Settings Change", MB_YESNO|MB_ICONQUESTION) == IDYES)
		  reboot = TRUE;
	      }
	    }
	    break;
	default:
	    ERR("invalid flags %d !\n", wFlags);
	    goto end;
    }
    
    res = OK;
end:
    IpClose16(hInf);
    SETUPX_FreeSubStrings(pSub);
    if (reboot)
    {
	/* FIXME: we should have a means of terminating all wine + wineserver */
	MESSAGE("Program or user told me to restart. Exiting Wine...\n");
	ExitProcess(1);
    }

    return res;
}

typedef struct
{
    LPCSTR RegValName;
    LPCSTR StdString; /* fallback string; sub dir of windows directory */
} LDID_DATA;

static const LDID_DATA LDID_Data[34] =
{
    { /* 0 (LDID_NULL) -- not defined */
	NULL,
	NULL
    },
    { /* 1 (LDID_SRCPATH) = source of installation. hmm, what to do here ? */
	"SourcePath", /* hmm, does SETUPX have to care about updating it ?? */
	NULL
    },
    { /* 2 (LDID_SETUPTEMP) = setup temp dir */
	"SetupTempDir",
	NULL
    },
    { /* 3 (LDID_UNINSTALL) = uninstall backup dir */
	"UninstallDir",
	NULL
    },
    { /* 4 (LDID_BACKUP) = backup dir */
	"BackupDir",
	NULL
    },
    { /* 5 (LDID_SETUPSCRATCH) = setup scratch dir */
	"SetupScratchDir",
	NULL
    },
    { /* 6 -- not defined */
	NULL,
	NULL
    },
    { /* 7 -- not defined */
	NULL,
	NULL
    },
    { /* 8 -- not defined */
	NULL,
	NULL
    },
    { /* 9 -- not defined */
	NULL,
	NULL
    },
    { /* 10 (LDID_WIN) = windows dir */
	"WinDir",
        ""
    },
    { /* 11 (LDID_SYS) = system dir */
	"SysDir",
	NULL /* call GetSystemDirectory() instead */
    },
    { /* 12 (LDID_IOS) = IOSubSys dir */
        NULL, /* FIXME: registry string ? */
	"SYSTEM\\IOSUBSYS"
    },
    { /* 13 (LDID_CMD) = COMMAND dir */
	NULL, /* FIXME: registry string ? */
	"COMMAND"
    },
    { /* 14 (LDID_CPL) = control panel dir */
	NULL,
	""
    },
    { /* 15 (LDID_PRINT) = windows printer dir */
	NULL,
	"SYSTEM" /* correct ?? */
    },
    { /* 16 (LDID_MAIL) = destination mail dir */
	NULL,
	""
    },
    { /* 17 (LDID_INF) = INF dir */
	"SetupScratchDir", /* correct ? */
	"INF"
    },
    { /* 18 (LDID_HELP) = HELP dir */
	NULL, /* ??? */
	"HELP"
    },
    { /* 19 (LDID_WINADMIN) = Admin dir */
	"WinAdminDir",
	""
    },
    { /* 20 (LDID_FONTS) = Fonts dir */
	NULL, /* ??? */
	"FONTS"
    },
    { /* 21 (LDID_VIEWERS) = Viewers */
	NULL, /* ??? */
	"SYSTEM\\VIEWERS"
    },
    { /* 22 (LDID_VMM32) = VMM32 dir */
	NULL, /* ??? */
	"SYSTEM\\VMM32"
    },
    { /* 23 (LDID_COLOR) = ICM dir */
	"ICMPath",
	"SYSTEM\\COLOR"
    },
    { /* 24 (LDID_APPS) = root of boot drive ? */
	"AppsDir",
	"C:\\"
    },
    { /* 25 (LDID_SHARED) = shared dir */
	"SharedDir",
	""
    },
    { /* 26 (LDID_WINBOOT) = Windows boot dir */
	"WinBootDir",
	""
    },
    { /* 27 (LDID_MACHINE) = machine specific files */
	"MachineDir",
	NULL
    },
    { /* 28 (LDID_HOST_WINBOOT) = Host Windows boot dir */
	"HostWinBootDir",
	NULL
    },
    { /* 29 -- not defined */
	NULL,
	NULL
    },
    { /* 30 (LDID_BOOT) = Root of boot drive */
	"BootDir",
	NULL
    },
    { /* 31 (LDID_BOOT_HOST) = Root of boot drive host */
	"BootHost",
	NULL
    },
    { /* 32 (LDID_OLD_WINBOOT) = subdir of root */
	"OldWinBootDir",
	NULL
    },
    { /* 33 (LDID_OLD_WIN) = old win dir */
	"OldWinDir",
	NULL
    }
    /* the rest (34-38) isn't too interesting, so I'll forget about it */
};

/* 
 * LDD  == Logical Device Descriptor
 * LDID == Logical Device ID
 *
 * The whole LDD/LDID business might go into a separate file named
 * ldd.c or logdevice.c.
 * At the moment I don't know what the hell these functions are really doing.
 * That's why I added reporting stubs.
 * The only thing I do know is that I need them for the LDD/LDID infrastructure.
 * That's why I implemented them in a way that's suitable for my purpose.
 */
static LDD_LIST *pFirstLDD = NULL;

static BOOL std_LDDs_done = FALSE;

void SETUPX_CreateStandardLDDs(void)
{
    HKEY hKey = 0;
    WORD n;
    DWORD type, len;
    LOGDISKDESC_S ldd;
    char buffer[MAX_PATH];

    /* has to be here, otherwise loop */
    std_LDDs_done = TRUE;

    RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", &hKey);

    for (n=0; n < sizeof(LDID_Data)/sizeof(LDID_DATA); n++)
    {
	buffer[0] = '\0';

	len = MAX_PATH;
	if ( (hKey) && (LDID_Data[n].RegValName)
	&&   (RegQueryValueExA(hKey, LDID_Data[n].RegValName,
		NULL, &type, buffer, &len) == ERROR_SUCCESS)
	&&   (type == REG_SZ) )
	{
	    TRACE("found value '%s' for LDID %d\n", buffer, n);
	}
	else
        switch(n)
	{
	    case LDID_SRCPATH:
		FIXME("LDID_SRCPATH: what exactly do we have to do here ?\n");
		strcpy(buffer, "X:\\FIXME");
		break;
	    case LDID_SYS:
	        GetSystemDirectoryA(buffer, MAX_PATH);
		break;
	    case LDID_APPS:
	    case LDID_MACHINE:
	    case LDID_HOST_WINBOOT:
	    case LDID_BOOT:
	    case LDID_BOOT_HOST:
		strcpy(buffer, "C:\\");
		break;
	    default:
		if (LDID_Data[n].StdString)
		{
		    DWORD len = GetWindowsDirectoryA(buffer, MAX_PATH);
		    LPSTR p;
		    p = buffer + len;
		    *p++ = '\\';
		    strcpy(p, LDID_Data[n].StdString);
		}
		break;
        }
	if (buffer[0])
	{
	    INIT_LDD(ldd, n);
	    ldd.pszPath = buffer;
	    TRACE("LDID %d -> '%s'\n", ldd.ldid, ldd.pszPath);
	    CtlSetLdd16(&ldd);
	}
    }
    if (hKey) RegCloseKey(hKey);
}
	
/***********************************************************************
 * CtlDelLdd		(SETUPX.37)
 *
 * RETURN
 *   ERR_VCP_LDDINVALID if ldid < LDID_ASSIGN_START.
 */
RETERR16 SETUPX_DelLdd(LOGDISKID16 ldid)
{
    LDD_LIST *pCurr, *pPrev = NULL;

    TRACE("(%d)\n", ldid);

    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (ldid < LDID_ASSIGN_START)
	return ERR_VCP_LDDINVALID;

    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (ldid > pCurr->pldd->ldid))
    {
	 pPrev = pCurr;
	 pCurr = pCurr->next;
    }
    if ( (pCurr == NULL) /* hit end of list */
      || (ldid != pCurr->pldd->ldid) )
	return ERR_VCP_LDDFIND; /* correct ? */

    /* ok, found our victim: eliminate it */

    if (pPrev)
	pPrev->next = pCurr->next;

    if (pCurr == pFirstLDD)
	pFirstLDD = NULL;
    HeapFree(GetProcessHeap(), 0, pCurr);
    
    return OK;
}

/***********************************************************************
 *		CtlDelLdd (SETUPX.37)
 */
RETERR16 WINAPI CtlDelLdd16(LOGDISKID16 ldid)
{
    FIXME("(%d); - please report to a.mohr@mailto.de !!!\n", ldid);
    return SETUPX_DelLdd(ldid);
}

/***********************************************************************
 * CtlFindLdd		(SETUPX.35)
 *
 * doesn't check pldd ptr validity: crash (W98SE)
 *
 * RETURN
 *   ERR_VCP_LDDINVALID if pldd->cbSize != structsize
 *   1 in all other cases ??
 * 
 */
RETERR16 WINAPI CtlFindLdd16(LPLOGDISKDESC pldd)
{
    LDD_LIST *pCurr, *pPrev = NULL;

    TRACE("(%p)\n", pldd);
   
    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (pldd->cbSize != sizeof(LOGDISKDESC_S))
        return ERR_VCP_LDDINVALID;

    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (pldd->ldid > pCurr->pldd->ldid))
    {
	pPrev = pCurr;
	pCurr = pCurr->next;
    }
    if ( (pCurr == NULL) /* hit end of list */
      || (pldd->ldid != pCurr->pldd->ldid) )
	return ERR_VCP_LDDFIND; /* correct ? */

    memcpy(pldd, pCurr->pldd, pldd->cbSize);
    /* hmm, we probably ought to strcpy() the string ptrs here */
    
    return 1; /* what is this ?? */
}

/***********************************************************************
 * CtlSetLdd			(SETUPX.33)
 *
 * Set an LDD entry.
 *
 * RETURN
 *   ERR_VCP_LDDINVALID if pldd.cbSize != sizeof(LOGDISKDESC_S)
 *
 */
RETERR16 WINAPI CtlSetLdd16(LPLOGDISKDESC pldd)
{
    LDD_LIST *pCurr, *pPrev = NULL;
    LPLOGDISKDESC pCurrLDD;
    HANDLE heap;
    BOOL is_new = FALSE;

    TRACE("(%p)\n", pldd);

    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (pldd->cbSize != sizeof(LOGDISKDESC_S))
	return ERR_VCP_LDDINVALID;

    heap = GetProcessHeap();
    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (pldd->ldid > pCurr->pldd->ldid))
    {
	 pPrev = pCurr;
	 pCurr = pCurr->next;
    }
    if (pCurr == NULL) /* hit end of list */
    {
	is_new = TRUE;
        pCurr = HeapAlloc(heap, 0, sizeof(LDD_LIST));
        pCurr->pldd = HeapAlloc(heap, 0, sizeof(LOGDISKDESC_S));
        pCurr->next = NULL;
        pCurrLDD = pCurr->pldd;
    }
    else
    {
        pCurrLDD = pCurr->pldd;
	if (pCurrLDD->pszPath)		HeapFree(heap, 0, pCurrLDD->pszPath);
	if (pCurrLDD->pszVolLabel)	HeapFree(heap, 0, pCurrLDD->pszVolLabel);
	if (pCurrLDD->pszDiskName)	HeapFree(heap, 0, pCurrLDD->pszDiskName);
    }

    memcpy(pCurrLDD, pldd, sizeof(LOGDISKDESC_S));

    if (pldd->pszPath)
        pCurrLDD->pszPath	= HEAP_strdupA(heap, 0, pldd->pszPath);
    if (pldd->pszVolLabel)
	pCurrLDD->pszVolLabel	= HEAP_strdupA(heap, 0, pldd->pszVolLabel);
    if (pldd->pszDiskName)
	pCurrLDD->pszDiskName	= HEAP_strdupA(heap, 0, pldd->pszDiskName);

    if (is_new) /* link into list */
    {
        if (pPrev)
	{
	    pCurr->next = pPrev->next;
            pPrev->next = pCurr;
	}
	if (!pFirstLDD)
	    pFirstLDD = pCurr;
    }
    
    return OK;
}


/***********************************************************************
 * CtlAddLdd		(SETUPX.36)
 *
 * doesn't check pldd ptr validity: crash (W98SE)
 *
 */
static LOGDISKID16 ldid_to_add = LDID_ASSIGN_START;
RETERR16 WINAPI CtlAddLdd16(LPLOGDISKDESC pldd)
{
    pldd->ldid = ldid_to_add++;
    return CtlSetLdd16(pldd);
}

/***********************************************************************
 * CtlGetLdd		(SETUPX.34)
 *
 * doesn't check pldd ptr validity: crash (W98SE)
 * What the !@#$%&*( is the difference between CtlFindLdd() and CtlGetLdd() ??
 *
 * RETURN
 *   ERR_VCP_LDDINVALID if pldd->cbSize != structsize
 * 
 */
static RETERR16 SETUPX_GetLdd(LPLOGDISKDESC pldd)
{
    LDD_LIST *pCurr, *pPrev = NULL;

    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (pldd->cbSize != sizeof(LOGDISKDESC_S))
        return ERR_VCP_LDDINVALID;

    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (pldd->ldid > pCurr->pldd->ldid))
    {
	 pPrev = pCurr;
	 pCurr = pCurr->next;
    }
    if (pCurr == NULL) /* hit end of list */
	return ERR_VCP_LDDFIND; /* correct ? */

    memcpy(pldd, pCurr->pldd, pldd->cbSize);
    /* hmm, we probably ought to strcpy() the string ptrs here */

    return OK;
}

/**********************************************************************/

RETERR16 WINAPI CtlGetLdd16(LPLOGDISKDESC pldd)
{
    FIXME("(%p); - please report to a.mohr@mailto.de !!!\n", pldd);
    return SETUPX_GetLdd(pldd);
}

/***********************************************************************
 *		CtlGetLddPath		(SETUPX.38)
 *
 * Gets the path of an LDD.
 * No crash if szPath == NULL.
 * szPath has to be at least MAX_PATH_LEN bytes long.
 * RETURN
 *   ERR_VCP_LDDUNINIT if LDD for LDID not found.
 */
RETERR16 WINAPI CtlGetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    TRACE("(%d, %p);\n", ldid, szPath);

    if (szPath)
    {
	LOGDISKDESC_S ldd;
	INIT_LDD(ldd, ldid);
	if (CtlFindLdd16(&ldd) == ERR_VCP_LDDFIND)
	    return ERR_VCP_LDDUNINIT;
	SETUPX_GetLdd(&ldd);
        strcpy(szPath, ldd.pszPath);
	TRACE("ret '%s' for LDID %d\n", szPath, ldid);
    }
    return OK;
}

/***********************************************************************
 *		CtlSetLddPath		(SETUPX.508)
 *
 * Sets the path of an LDD.
 * Creates LDD for LDID if not existing yet.
 */
RETERR16 WINAPI CtlSetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    LOGDISKDESC_S ldd;
    TRACE("(%d, '%s');\n", ldid, szPath);
    
    INIT_LDD(ldd, ldid);
    ldd.pszPath = szPath;
    return CtlSetLdd16(&ldd);
}

/*
 * Find the value of a custom LDID in a .inf file
 * e.g. for 49301:
 * 49300,49301=ProgramFilesDir,5
 * -- profile section lookup -->
 * [ProgramFilesDir]
 * HKLM,"Software\Microsoft\Windows\CurrentVersion","ProgramFilesDir",,"%24%"
 * -- GenFormStrWithoutPlaceHolders16 -->
 * HKLM,"Software\Microsoft\Windows\CurrentVersion","ProgramFilesDir",,"C:\"
 * -- registry lookup -->
 * C:\Program Files (or C:\ if not found in registry)
 * 
 * FIXME:
 * - maybe we ought to add a caching array for speed ? - I don't care :)
 * - not sure whether the processing is correct - sometimes there are equal
 *   LDIDs for both install and removal sections.
 * - probably the whole function can be removed as installers add that on their
 *   own 
 */
static BOOL SETUPX_AddCustomLDID(int ldid, INT16 hInf)
{
    char ldidstr[6];
    LPSTR sectionbuf = NULL, entrybuf = NULL, regsectionbuf = NULL;
    LPCSTR filename;
    LPSTR pSec, pEnt, pEqual, p, *pSub = NULL;
    BOOL ret = FALSE;
    char buffer[MAX_PATH];
    LOGDISKDESC_S ldd;

    sprintf(ldidstr, "%d", ldid);
    filename = IP_GetFileName(hInf);
    if (!(sectionbuf = SETUPX_GetSections(filename)))
    {
	ERR("couldn't get sections !\n");
	return FALSE;
    }
    for (pSec=sectionbuf; *pSec; pSec += strlen(pSec)+1)
    {
	if (!(entrybuf = SETUPX_GetSectionEntries(filename, pSec)))
	{
	    ERR("couldn't get section entries !\n");
	    goto end;
	}
	for (pEnt=entrybuf; *pEnt; pEnt += strlen(pEnt)+1)
	{
	    if (strstr(pEnt, ldidstr))
	    {
		pEqual = strchr(pEnt, '=');
		if (!pEqual) /* crippled entry ?? */
		    continue;

		/* make sure we found the LDID on left side of the equation */
		if (pEnt+strlen(ldidstr) <= pEqual)
		{ /* found */

		    /* but we don't want entries in the strings section */
		    if (!strcasecmp(pSec, "Strings"))
			goto next_section;
		    p = pEqual+1;
		    goto found;
		}
	    }
	}
next_section:
    }
    goto end;
found:
    TRACE("found entry '%s'\n", p);
    pSub = SETUPX_GetSubStrings(p, ',');
    if (*(DWORD *)pSub > 2)
    {
	ERR("malformed entry '%s' ?\n", p);
	goto end;
    }
    TRACE("found section '%s'\n", *(pSub+1));
    /* FIXME: what are the optional flags at the end of an entry used for ?? */

    /* get the location of the registry key from that section */
    if (!(regsectionbuf = SETUPX_GetSectionEntries(filename, *(pSub+1))))
    {
	ERR("couldn't get registry section entries !\n");
	goto end;
    }
    /* sectionbuf is > 1024 bytes anyway, so use it */
    GenFormStrWithoutPlaceHolders16(sectionbuf, regsectionbuf, hInf);
    ret = SETUPX_LookupRegistryString(sectionbuf, buffer, MAX_PATH);
    TRACE("return '%s'\n", buffer);
    INIT_LDD(ldd, ldid);
    ldd.pszPath = buffer;
    CtlSetLdd16(&ldd);
end:
    SETUPX_FreeSubStrings(pSub);
    if (sectionbuf)	HeapFree(GetProcessHeap(), 0, sectionbuf);
    if (entrybuf)	HeapFree(GetProcessHeap(), 0, entrybuf);
    if (regsectionbuf)	HeapFree(GetProcessHeap(), 0, regsectionbuf);
    return ret;
}

/*
 * Translate a logical disk identifier (LDID) into its string representation
 * I'm afraid this can be totally replaced by CtlGetLddPath().
 */
static BOOL SETUPX_IP_TranslateLDID(int ldid, LPSTR *p, HINF16 hInf)
{
    BOOL handled = FALSE;
    LOGDISKDESC_S ldd;

    ldd.cbSize = sizeof(LOGDISKDESC_S);
    ldd.ldid = ldid;
    if (CtlFindLdd16(&ldd) == ERR_VCP_LDDFIND)
    {
	/* hmm, it seems the installers already do the work for us
	 * (by calling CtlSetLddPath) that SETUPX_AddCustomLDID
	 * is supposed to do. Grmbl ;-)
	 * Well, I'll leave it here anyway, but print error... */
	ERR("hmm, LDID %d not registered yet !?\n", ldid);
	handled = SETUPX_AddCustomLDID(ldid, hInf);
    }
    else
	handled = TRUE;
    
    SETUPX_GetLdd(&ldd);
    
    if (!handled)
    {
        FIXME("What is LDID %d ??\n", ldid);
	*p = "LDID_FIXME";
    }
    else
	*p = ldd.pszPath;

    return handled;
}

/***********************************************************************
 *		GenFormStrWithoutPlaceHolders
 *
 * ought to be pretty much implemented, I guess...
 */
void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR szDst, LPCSTR szSrc, HINF16 hInf)
{
    LPCSTR pSrc = szSrc, pSrcEnd = szSrc + strlen(szSrc);
    LPSTR pDst = szDst, p, pPHBegin;
    int count;
    
    TRACE("(%p, '%s', %04x);\n", szDst, szSrc, hInf);
    while (pSrc < pSrcEnd)
    {
	p = strchr(pSrc, '%');
	if (p)
	{
	    count = (int)p - (int)pSrc;
	    strncpy(pDst, pSrc, count);
	    pSrc += count;
	    pDst += count;
	    pPHBegin = p+1;
	    p = strchr(pPHBegin, '%');
	    if (p)
	    {
		char placeholder[80]; /* that really ought to be enough ;) */
		int ldid;
		BOOL done = TRUE;
		count = (int)p - (int)pPHBegin;
		strncpy(placeholder, pPHBegin, count);
		placeholder[count] = '\0';
		ldid = atoi(placeholder);
		if (ldid)
		{
		    LPSTR p;
		    done = SETUPX_IP_TranslateLDID(ldid, &p, hInf);
		    strcpy(pDst, p);
		    if (done)
			pDst += strlen(pDst);
		}
		else
		{ /* hmm, string placeholder. Need to look up
		     in the [strings] section of the hInf */
		    DWORD ret;
		    char buf[256]; /* long enough ? */
		    
		    ret = GetPrivateProfileStringA("strings", placeholder, "",
		    			buf, 256, IP_GetFileName(hInf));
		    if (ret)
		    {
			strcpy(pDst, buf);
			pDst += strlen(buf);
		    }
		    else
		    {
		        ERR("placeholder string '%s' not found !\n", placeholder);
			done = FALSE;
		    }
		}
		if (!done)
		{ /* copy raw placeholder string over */
		    count = (int)p - (int)pPHBegin + 2;
		    strncpy(pDst, pPHBegin-1, count);
		    pDst += count;
		    
		}
		pSrc = p+1;
		continue;
	    }
	}

	/* copy the remaining source string over */
	strncpy(pDst, pSrc, (int)pSrcEnd - (int)pSrc + 1);
	break;
    }
    TRACE("ret '%s'\n", szDst);
}

/***********************************************************************
 *		VcpOpen
 *
 * No idea what to do here.
 */
RETERR16 WINAPI VcpOpen16(VIFPROC vifproc, LPARAM lparamMsgRef)
{
    FIXME("(%p, %08lx), stub.\n", vifproc, lparamMsgRef);
    return OK;
}

/***********************************************************************
 *		VcpClose
 *
 * Is fl related to VCPDISKINFO.fl ?
 */
RETERR16 WINAPI VcpClose16(WORD fl, LPCSTR lpszBackupDest)
{
    FIXME("(%04x, '%s'), stub.\n", fl, lpszBackupDest);
    return OK;
}

/*
 * Copy all items in a CopyFiles entry over to the destination
 *
 * - VNLP_xxx is what is given as flags for a .INF CopyFiles section
 */
static BOOL SETUPX_CopyFiles(LPSTR *pSub, HINF16 hInf)
{
    BOOL res = FALSE;
    unsigned int n;
    LPCSTR filename = IP_GetFileName(hInf);
    LPSTR pCopyEntry;
    char pDestStr[MAX_PATH];
    LPSTR pSrcDir, pDstDir;
    LPSTR pFileEntries, p;
    WORD ldid;
    LOGDISKDESC_S ldd;
    LPSTR *pSubFile;
    LPSTR pSrcFile, pDstFile;

    for (n=0; n < *(DWORD *)pSub; n++)
    {
	pCopyEntry = *(pSub+1+n);
	if (*pCopyEntry == '@')
	{
	    ERR("single file not handled yet !\n");
	    continue;
	}

	/* get source directory for that entry */
	INIT_LDD(ldd, LDID_SRCPATH);
	SETUPX_GetLdd(&ldd);
	pSrcDir = ldd.pszPath;
	
        /* get destination directory for that entry */
	if (!(GetPrivateProfileStringA("DestinationDirs", pCopyEntry, "",
					pDestStr, sizeof(pDestStr), filename)))
	    continue;

	/* translate destination dir if given as LDID */
	ldid = atoi(pDestStr);
	if (ldid)
	{
	    if (!(SETUPX_IP_TranslateLDID(ldid, &pDstDir, hInf)))
		continue;
	}
	else
	    pDstDir = pDestStr;
	
	/* now that we have the destination dir, iterate over files to copy */
	pFileEntries = SETUPX_GetSectionEntries(filename, pCopyEntry);
        for (p=pFileEntries; *p; p +=strlen(p)+1)
	{
	    pSubFile = SETUPX_GetSubStrings(p, ',');
	    pSrcFile = *(pSubFile+1);
	    pDstFile = (*(DWORD *)pSubFile > 1) ? *(pSubFile+2) : pSrcFile;
	    TRACE("copying file '%s\\%s' to '%s\\%s'\n", pSrcDir, pSrcFile, pDstDir, pDstFile);
	    if (*(DWORD *)pSubFile > 2)
	    {
		WORD flag;
		if ((flag = atoi(*(pSubFile+3)))) /* ah, flag */
		{
		    if (flag & 0x2c)
		    FIXME("VNLP_xxx flag %d not handled yet.\n", flag);
		}
		else
		    FIXME("temp file name '%s' given. Need to register in wininit.ini !\n", *(pSubFile+3)); /* strong guess that this is VcpQueueCopy() */
	    }
	    SETUPX_FreeSubStrings(pSubFile);
	    /* we don't copy ANYTHING yet ! (I'm too lazy and want to verify
	     * this first before destroying whole partitions ;-) */
	}
    }
	    
    return res;
}

/***********************************************************************
 *		GenInstall
 *
 * general install function for .INF file sections
 *
 * This is not perfect - patch whenever you can !
 * 
 * wFlags == GENINSTALL_DO_xxx
 * e.g. NetMeeting:
 * first call GENINSTALL_DO_REGSRCPATH | GENINSTALL_DO_FILES,
 * second call GENINSTALL_DO_LOGCONFIG | CFGAUTO | INI2REG | REG | INI
 */
RETERR16 WINAPI GenInstall16(HINF16 hInfFile, LPCSTR szInstallSection, WORD wFlags)
{
    LPCSTR filename = IP_GetFileName(hInfFile);
    LPSTR pEntries, p, pEnd;
    DWORD len;
    LPSTR *pSub;

    FIXME("(%04x, '%s', %04x), semi-stub. Please implement additional operations here !\n", hInfFile, szInstallSection, wFlags);
    pEntries = SETUPX_GetSectionEntries(filename, szInstallSection);
    if (!pEntries)
    {
	ERR("couldn't find entries for section '%s' !\n", szInstallSection);
	return ERR_IP_SECT_NOT_FOUND;
    }
    for (p=pEntries; *p; p +=strlen(p)+1)
    {
	pEnd = strchr(p, '=');
	if (!pEnd) continue;
	pSub = SETUPX_GetSubStrings(pEnd+1, ','); /* split entries after the '=' */
	SETUPX_IsolateSubString(&p, &pEnd);
	len = (int)pEnd - (int)p;

	if (wFlags & GENINSTALL_DO_FILES)
	{
	    if (!strncasecmp(p, "CopyFiles", len))
	    {
	        SETUPX_CopyFiles(pSub, hInfFile);
		continue;
	    }
#if IMPLEMENT_THAT
	    else
	    if (!strncasecmp(p, "DelFiles", len))
	    {
	        SETUPX_DelFiles(filename, szInstallSection, pSub);
		continue;
	    }
#endif
	}
	if (wFlags & GENINSTALL_DO_INI)
	{
#if IMPLEMENT_THAT
	    if (!strncasecmp(p, "UpdateInis", len))
	    {
	        SETUPX_UpdateInis(filename, szInstallSection, pSub);
		continue;
	    }
#endif
	}
	if (wFlags & GENINSTALL_DO_REG)
	{
#if IMPLEMENT_THAT
	    /* probably use SUReg*() functions here */
	    if (!strncasecmp(p, "AddReg", len))
	    {
	        SETUPX_AddReg(filename, szInstallSection, pSub);
		continue;
	    }
	    else
	    if (!strncasecmp(p, "DelReg", len))
	    {
	        SETUPX_DelReg(filename, szInstallSection, pSub);
		continue;
	    }
#endif
	}
	
	SETUPX_FreeSubStrings(pSub);
    }
    HeapFree(GetProcessHeap(), 0, pEntries);
    return OK;
}
