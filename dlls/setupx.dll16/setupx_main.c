/*
 *      SETUPX library
 *
 *      Copyright 1998,2000  Andreas Mohr
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: Rather non-functional functions for now.
 *
 * See:
 * http://www.geocities.com/SiliconValley/Network/5317/drivers.html
 * http://willemer.de/informatik/windows/inf_info.htm (German)
 * DDK: setupx.h
 * http://mmatrix.tripod.com/customsystemfolder/infsysntaxfull.html
 * http://www.rdrop.com/~cary/html/inf_faq.html
 *
 * Stuff tested with:
 * - rs405deu.exe (German Acroread 4.05 setup)
 * - ie5setup.exe
 * - NetMeeting
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "wownt32.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "setupx16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		SURegOpenKey (SETUPX.47)
 */
DWORD WINAPI SURegOpenKey( HKEY hkey, LPCSTR lpszSubKey, PHKEY retkey )
{
    FIXME("(%p,%s,%p), semi-stub.\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegOpenKeyA( hkey, lpszSubKey, retkey );
}

/***********************************************************************
 *		SURegQueryValueEx (SETUPX.50)
 */
DWORD WINAPI SURegQueryValueEx( HKEY hkey, LPSTR lpszValueName,
                                LPDWORD lpdwReserved, LPDWORD lpdwType,
                                LPBYTE lpbData, LPDWORD lpcbData )
{
    FIXME("(%p,%s,%p,%p,%p,%ld), semi-stub.\n",hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData?*lpcbData:0);
    return RegQueryValueExA( hkey, lpszValueName, lpdwReserved, lpdwType,
                               lpbData, lpcbData );
}

/***********************************************************************
 *		TPWriteProfileString (SETUPX.62)
 */
BOOL WINAPI TPWriteProfileString16( LPCSTR section, LPCSTR entry, LPCSTR string )
{
    FIXME( "%s %s %s: stub\n", debugstr_a(section), debugstr_a(entry), debugstr_a(string) );
    return TRUE;
}


/***********************************************************************
 *		suErrorToIds  (SETUPX.61)
 */
DWORD WINAPI suErrorToIds16( WORD w1, WORD w2 )
{
    FIXME( "%x %x: stub\n", w1, w2 );
    return 0;
}

/***********************************************************************
 *		InstallHinfSection (SETUPX.527)
 *
 * hwnd = parent window
 * hinst = instance of SETUPX.DLL
 * lpszCmdLine = e.g. "DefaultInstall 132 C:\MYINSTALL\MYDEV.INF"
 * Here "DefaultInstall" is the .inf file section to be installed (optional).
 * The 132 value is made of the HOW_xxx flags and sometimes 128 (-> setupx16.h).
 *
 * nCmdShow = nCmdShow of CreateProcess
 */
RETERR16 WINAPI InstallHinfSection16( HWND16 hwnd, HINSTANCE16 hinst, LPCSTR lpszCmdLine, INT16 nCmdShow)
{
    InstallHinfSectionA( HWND_32(hwnd), 0, lpszCmdLine, nCmdShow );
    return OK;
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
 * ldd.c.
 * At the moment I don't know what the hell these functions are really doing.
 * That's why I added reporting stubs.
 * The only thing I do know is that I need them for the LDD/LDID infrastructure.
 * That's why I implemented them in a way that's suitable for my purpose.
 */
static LDD_LIST *pFirstLDD = NULL;

static BOOL std_LDDs_done = FALSE;

static void SETUPX_CreateStandardLDDs(void)
{
    HKEY hKey = 0;
    WORD n;
    DWORD type, len;
    LOGDISKDESC_S ldd;
    char buffer[MAX_PATH];

    /* has to be here, otherwise loop */
    std_LDDs_done = TRUE;

    RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", &hKey);

    for (n=0; n < ARRAY_SIZE(LDID_Data); n++)
    {
	buffer[0] = '\0';

	len = MAX_PATH;
	if ( (hKey) && (LDID_Data[n].RegValName)
	&&   (RegQueryValueExA(hKey, LDID_Data[n].RegValName,
		NULL, &type, (LPBYTE)buffer, &len) == ERROR_SUCCESS)
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
static RETERR16 SETUPX_DelLdd(LOGDISKID16 ldid)
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
    FIXME("(%d); - please report this!\n", ldid);
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
    LDD_LIST *pCurr;

    TRACE("(%p)\n", pldd);

    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (pldd->cbSize != sizeof(LOGDISKDESC_S))
        return ERR_VCP_LDDINVALID;

    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (pldd->ldid > pCurr->pldd->ldid))
	pCurr = pCurr->next;
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
    if (!pCurr || pldd->ldid != pCurr->pldd->ldid)
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
        HeapFree(heap, 0, pCurrLDD->pszPath);
        HeapFree(heap, 0, pCurrLDD->pszVolLabel);
        HeapFree(heap, 0, pCurrLDD->pszDiskName);
    }

    *pCurrLDD = *pldd;

    if (pldd->pszPath)
    {
        pCurrLDD->pszPath = HeapAlloc( heap, 0, strlen(pldd->pszPath)+1 );
        strcpy( pCurrLDD->pszPath, pldd->pszPath );
    }
    if (pldd->pszVolLabel)
    {
        pCurrLDD->pszVolLabel = HeapAlloc( heap, 0, strlen(pldd->pszVolLabel)+1 );
        strcpy( pCurrLDD->pszVolLabel, pldd->pszVolLabel );
    }
    if (pldd->pszDiskName)
    {
        pCurrLDD->pszDiskName = HeapAlloc( heap, 0, strlen(pldd->pszDiskName)+1 );
        strcpy( pCurrLDD->pszDiskName, pldd->pszDiskName );
    }

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
    LDD_LIST *pCurr;

    if (!std_LDDs_done)
	SETUPX_CreateStandardLDDs();

    if (pldd->cbSize != sizeof(LOGDISKDESC_S))
        return ERR_VCP_LDDINVALID;

    pCurr = pFirstLDD;
    /* search until we find the appropriate LDD or hit the end */
    while ((pCurr != NULL) && (pldd->ldid > pCurr->pldd->ldid))
	pCurr = pCurr->next;
    if (pCurr == NULL) /* hit end of list */
	return ERR_VCP_LDDFIND; /* correct ? */

    memcpy(pldd, pCurr->pldd, pldd->cbSize);
    /* hmm, we probably ought to strcpy() the string ptrs here */

    return OK;
}

/**********************************************************************/

RETERR16 WINAPI CtlGetLdd16(LPLOGDISKDESC pldd)
{
    FIXME("(%p); - please report this!\n", pldd);
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
 *		DiInstallClass		(SETUPX.321)
 *
 * Install Class from an INF file (in [ClassInstall] section)
 */
RETERR16 WINAPI DiInstallClass16(LPCSTR lpszInfFileName, DWORD dwFlags)
{
    FIXME("(%s, %lu), stub.\n", lpszInfFileName, dwFlags);
    return 0;
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

    SetupSetDirectoryIdA( 0, ldid, szPath );
    INIT_LDD(ldd, ldid);
    ldd.pszPath = szPath;
    return CtlSetLdd16(&ldd);
}

/***********************************************************************
 *		DiBuildDriverIndex  (SETUPX.412)
 */
RETERR16 WINAPI DiBuildDriverIndex16( WORD w1 )
{
    FIXME( "%x: stub\n", w1 );
    return 0;
}
