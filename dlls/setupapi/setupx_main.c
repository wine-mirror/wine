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
 *
 * Stuff tested with:
 * - rs405deu.exe (German Acroread 4.05 setup)
 * - ie5setup.exe
 * - Netmeeting
 *
 * FIXME:
 * - check buflen
 */

#include <stdlib.h>
#include <stdio.h>
#include "winreg.h"
#include "wine/winuser16.h"
#include "setupx16.h"
#include "winerror.h"
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

/***********************************************************************
 *		InstallHinfSection
 *
 * hwnd = parent window
 * hinst = instance of SETUPX.DLL
 * lpszCmdLine = e.g. "DefaultInstall 132 C:\MYINSTALL\MYDEV.INF"
 * Here "DefaultInstall" is the .inf file section to be installed (optional).
 * 132 is the standard parameter, it seems.
 * 133 means don't prompt user for reboot.
 * 
 * nCmdShow = nCmdShow of CreateProcess
 * FIXME: is the return type correct ?
 */
DWORD WINAPI InstallHinfSection16( HWND16 hwnd, HINSTANCE16 hinst, LPCSTR lpszCmdLine, INT16 nCmdShow)
{
    FIXME("(%04x, %04x, %s, %d), stub.\n", hwnd, hinst, lpszCmdLine, nCmdShow);
    return 0;
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

    if (RegQueryValueExA(hsubkey, items[2], 0, &dwType, buffer, &buflen)
		!= ERROR_SUCCESS)
        goto regfailed;
    goto done;

regfailed:
    strcpy(buffer, items[4]); /* I don't care about buflen */
done:
    for (n=0; n < 5; n++)
        HeapFree(heap, 0, items[n]);
    TRACE("return '%s'\n", buffer);
    return TRUE;
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
 */
static BOOL SETUPX_TranslateCustomLDID(int ldid, LPSTR buffer, WORD buflen, INT16 hInf)
{
    char ldidstr[6], sectionbuf[0xffff], entrybuf[0xffff], section[256];
    LPCSTR filename;
    LPSTR pSec, pEnt, pEqual, p, pEnd;
    BOOL ret = FALSE;

    sprintf(ldidstr, "%d", ldid);
    filename = IP_GetFileName(hInf);
    if (!GetPrivateProfileStringA(NULL, NULL, NULL,
			    	sectionbuf, sizeof(sectionbuf), filename))
    {
	ERR("section buffer too small ?\n");
	return FALSE;
    }
    for (pSec=sectionbuf; *pSec; pSec += strlen(pSec)+1)
    {
	if (!GetPrivateProfileSectionA(pSec,
				entrybuf, sizeof(entrybuf), filename))
	{
	    ERR("entry buffer too small ?\n");
	    return FALSE;
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
		    while ((*p == ' ') || (*p == '\t')) p++;
		    goto found;
		}
	    }
	}
next_section:
    }
    return FALSE;
found:
    TRACE("found entry '%s'\n", p);
    /* strip off any flags we get
     * FIXME: what are these flags used for ?? */
    pEnd = strchr(p, ',');
    strncpy(section, p, (int)pEnd - (int)p);
    section[(int)pEnd - (int)p] = '\0';

    /* get the location of the registry key from that section */
    if (!GetPrivateProfileSectionA(section, entrybuf, sizeof(entrybuf), filename))
    {
	ERR("entrybuf too small ?\n");
	return FALSE;
    }
    GenFormStrWithoutPlaceHolders16(sectionbuf, entrybuf, hInf);
    ret = SETUPX_LookupRegistryString(sectionbuf, buffer, buflen);
    TRACE("return '%s'\n", buffer);
    return ret;
}

/*
 * Translate a logical disk identifier (LDID) into its string representation
 */
static BOOL SETUPX_TranslateLDID(int ldid, LPSTR buffer, WORD buflen, HINF16 hInf)
{
    BOOL handled = FALSE;

    if ((ldid >= LDID_SRCPATH) && (ldid <= LDID_OLD_WIN))
    {
	if (LDID_Data[ldid].RegValName)
	{
	    HKEY hKey;

	    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", &hKey) == ERROR_SUCCESS)
	    {
	        DWORD type, len = buflen;

		if ( (RegQueryValueExA(hKey, LDID_Data[ldid].RegValName,
			NULL, &type, buffer, &len) == ERROR_SUCCESS)
		&&   (type == REG_SZ) )
		{
		    TRACE("found value '%s' for LDID %d\n", buffer, ldid);
		    handled = TRUE;
		}

		RegCloseKey(hKey);
	    }
	}
    }
    if (!handled)
    {
        switch(ldid)
	{
	    case LDID_SRCPATH:
		FIXME("LDID_SRCPATH: what exactly do we have to do here ?\n");
		break;
	    case LDID_SYS:
	        GetSystemDirectoryA(buffer, buflen);
		handled = TRUE;
	    	break;
	    case LDID_APPS:
	    case LDID_MACHINE:
	    case LDID_HOST_WINBOOT:
	    case LDID_BOOT:
	    case LDID_BOOT_HOST:
		strncpy(buffer, "C:\\", buflen);
		buffer[buflen-1] = '\0';
		handled = TRUE;
		break;
	    default:
		if ( (ldid >= LDID_NULL) && (ldid <= LDID_OLD_WIN)
		  && (LDID_Data[ldid].StdString) )
		{
		    UINT len = GetWindowsDirectoryA(buffer, buflen);
		    if (len <= buflen-1)
		    {
			buffer += len;
			buflen -= len;
			*buffer++ = '\\';
			buflen--;
		        strncpy(buffer, LDID_Data[ldid].StdString, buflen);
		        buffer[buflen-1] = '\0';
		    }
		    handled = TRUE;
		}
		break;
        }
    }

    if (!handled)
	handled = SETUPX_TranslateCustomLDID(ldid, buffer, buflen, hInf);

    if (!handled)
        FIXME("unimplemented LDID %d\n", ldid);

    return handled;
}

/***********************************************************************
 *		GenFormStrWithoutPlaceHolders
 */
void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR szDst, LPCSTR szSrc, HINF16 hInf)
{
    LPCSTR pSrc = szSrc, pSrcEnd = szSrc + strlen(szSrc);
    LPSTR pDst = szDst, p, pPHBegin;
    int count;
    
    FIXME("(%p, '%s', %04x), semi stub.\n", szDst, szSrc, hInf);
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
		    done = SETUPX_TranslateLDID(ldid, pDst, 256, hInf);
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
 *		CtlGetLddPath
 */
RETERR16 WINAPI CtlGetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    FIXME("(%04x, %p), stub.\n", ldid, szPath);
    strcpy(szPath, "FIXME_BogusLddPath");
    return OK;
}

/***********************************************************************
 *		CtlSetLddPath
 */
RETERR16 WINAPI CtlSetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    FIXME("(%04x, '%s'), stub.\n", ldid, szPath);
    return OK;
}

/***********************************************************************
 *		vcpOpen
 *
 * p2 is "\001" for Netmeeting.
 */
RETERR16 WINAPI vcpOpen16(LPWORD p1, LPWORD p2)
{
    FIXME("(%p, %p), stub.\n", p1, p2);
    return OK;
}

/***********************************************************************
 *		vcpClose
 */
RETERR16 WINAPI vcpClose16(WORD w1, WORD w2, WORD w3)
{
    FIXME("(%04x, %04x %04x), stub.\n", w1, w2, w3);
    return OK;
}

/***********************************************************************
 *		GenInstall
 */
RETERR16 WINAPI GenInstall16(HINF16 hInfFile, LPCSTR szInstallSection, WORD wFlags)
{
    FIXME("(%04x, '%s', %04x), stub. This doesn't install anything yet ! Use native SETUPX.DLL instead !!\n", hInfFile, szInstallSection, wFlags);
    return OK;
}
