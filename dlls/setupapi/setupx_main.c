/*
 *      SETUPX library
 *
 *      Copyright 1998,2000  Andreas Mohr
 *
 * FIXME: Rather non-functional functions for now.
 *
 * See:
 * http://www.geocities.com/SiliconValley/Network/5317/drivers.html
 * http://www.microsoft.com/ddk/ddkdocs/win98ddk/devinst_12uw.htm
 * DDK: setupx.h
 * http://mmatrix.tripod.com/customsystemfolder/infsysntaxfull.html
 * http://www.rdrop.com/~cary/html/inf_faq.html
 *
 * Stuff tested with rs405deu.exe (German Acroread 4.05 setup)
 */

#include "winreg.h"
#include "wine/winuser16.h"
#include "setupx16.h"
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


/*
 * GenFormStrWithoutPlaceholders
 *
 * Any real docu ?
 */
void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR szDst, LPCSTR szSrc, HINF16 hInf)
{
    FIXME("(%p, '%s', %04x), stub.\n", szDst, szSrc, hInf);
    strcpy(szDst, szSrc);
}

RETERR16 WINAPI CtlGetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    FIXME("(%04x, %p), stub.\n", ldid, szPath);
    strcpy(szPath, "FIXME_BogusLddPath");
    return OK;
}

RETERR16 WINAPI CtlSetLddPath16(LOGDISKID16 ldid, LPSTR szPath)
{
    FIXME("(%04x, '%s'), stub.\n", ldid, szPath);
    return OK;
}

RETERR16 WINAPI vcpOpen16(LPWORD p1, LPWORD p2)
{
    FIXME("(%p, %p), stub.\n", p1, p2);
    return OK;
}

RETERR16 WINAPI vcpClose16(WORD w1, WORD w2, WORD w3)
{
    FIXME("(%04x, %04x %04x), stub.\n", w1, w2, w3);
    return OK;
}

RETERR16 WINAPI GenInstall16(HINF16 hInfFile, LPCSTR szInstallSection, WORD wFlags)
{
    FIXME("(%04x, '%s', %04x), stub. This doesn't install anything yet !\n", hInfFile, szInstallSection, wFlags);
    return OK;
}
