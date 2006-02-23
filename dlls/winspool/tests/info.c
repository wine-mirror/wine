/*
 * Copyright (C) 2003, 2004 Stefan Leichter
 * Copyright (C) 2005 Detlef Riekenberg
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

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winreg.h"
#include "winspool.h"

#define MAGIC_DEAD  0x00dead00
#define DEFAULT_PRINTER_SIZE 1000

static char env_x86[] = "Windows NT x86";
static char env_win9x_case[] = "windowS 4.0";

static HANDLE  hwinspool;
static FARPROC pGetDefaultPrinterA;
static FARPROC pSetDefaultPrinterA;

/* report common behavior only once */
static DWORD report_deactivated_spooler = 1;
#define RETURN_ON_DEACTIVATED_SPOOLER(res) \
    if((res == 0) && (GetLastError() == RPC_S_SERVER_UNAVAILABLE)) \
    { \
        if(report_deactivated_spooler > 0) { \
            report_deactivated_spooler--; \
            trace("The Service 'Spooler' is required for many test\n"); \
        } \
        return; \
    }


static LPSTR find_default_printer(VOID)
{
    static  LPSTR   default_printer = NULL;
    static  char    buffer[DEFAULT_PRINTER_SIZE];
    DWORD   needed;
    DWORD   res;
    LPSTR   ptr;

    if ((default_printer == NULL) && (pGetDefaultPrinterA))
    {
        /* w2k and above */
        needed = sizeof(buffer);
        res = pGetDefaultPrinterA(buffer, &needed);
        if(res)  default_printer = buffer;
        trace("default_printer: '%s'\n", default_printer);
    }
    if (default_printer == NULL)
    {
        HKEY hwindows;
        DWORD   type;
        /* NT 3.x and above */
        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                        "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                        0, KEY_QUERY_VALUE, &hwindows) == NO_ERROR) {

            needed = sizeof(buffer);
            if (RegQueryValueEx(hwindows, "device", NULL, 
                                &type, (LPBYTE)buffer, &needed) == NO_ERROR) {

                ptr = strchr(buffer, ',');
                if (ptr) {
                    ptr[0] = '\0';
                    default_printer = buffer;
                }
            }
            RegCloseKey(hwindows);
        }
        trace("default_printer: '%s'\n", default_printer);
    }
    if (default_printer == NULL)
    {
        /* win9x */
        needed = sizeof(buffer);
        res = GetProfileStringA("windows", "device", "*", buffer, needed);
        if(res) {
            ptr = strchr(buffer, ',');
            if (ptr) {
                ptr[0] = '\0';
                default_printer = buffer;
            }
        }
        trace("default_printer: '%s'\n", default_printer);
    }
    return default_printer;
}

/* ########################### */

static void test_EnumMonitors(void)
{
    DWORD   res;
    LPBYTE  buffer;
    DWORD   cbBuf;
    DWORD   pcbNeeded;
    DWORD   pcReturned;
    DWORD   level;

    /* valid levels are 1 and 2 */
    for(level = 0; level < 4; level++) {
        cbBuf = MAGIC_DEAD;
        pcReturned = MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = EnumMonitorsA(NULL, level, NULL, 0, &cbBuf, &pcReturned);

        RETURN_ON_DEACTIVATED_SPOOLER(res)

        /* not implemented yet in wine */
        if (!res && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) continue;


        /* use only a short test, when we test with an invalid level */
        if(!level || (level > 2)) {
            ok( (!res && (GetLastError() == ERROR_INVALID_LEVEL)) ||
                (res && (pcReturned == 0)),
                "(%ld) returned %ld with %ld and 0x%08lx (expected '0' with " \
                "ERROR_INVALID_LEVEL or '!=0' and 0x0)\n",
                level, res, GetLastError(), pcReturned);
            continue;
        }        

        /* Level 2 is not supported on win9x */
        if (!res && (GetLastError() == ERROR_INVALID_LEVEL)) {
            trace("Level %ld not supported, skipping tests\n", level);
            continue;
        }

        ok((!res) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%ld) returned %ld with %ld (expected '0' with " \
            "ERROR_INSUFFICIENT_BUFFER)\n", level, res, GetLastError());

        if (!cbBuf) {
            trace("no valid buffer size returned, skipping tests\n");
            continue;
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, cbBuf *2);
        if (buffer == NULL) continue;

        SetLastError(MAGIC_DEAD);
        pcbNeeded = MAGIC_DEAD;
        res = EnumMonitorsA(NULL, level, buffer, cbBuf, &pcbNeeded, &pcReturned);
        ok(res, "(%ld) returned %ld with %ld (expected '!=0')\n",
                level, res, GetLastError());
        ok(pcbNeeded == cbBuf, "(%ld) returned %ld (expected %ld)\n",
                level, pcbNeeded, cbBuf);
        /* We can validate the returned Data with the Registry here */


        SetLastError(MAGIC_DEAD);
        pcReturned = MAGIC_DEAD;
        pcbNeeded = MAGIC_DEAD;
        res = EnumMonitorsA(NULL, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok(res, "(%ld) returned %ld with %ld (expected '!=0')\n", level,
                res, GetLastError());
        ok(pcbNeeded == cbBuf, "(%ld) returned %ld (expected %ld)\n", level,
                pcbNeeded, cbBuf);

        SetLastError(MAGIC_DEAD);
        pcbNeeded = MAGIC_DEAD;
        res = EnumMonitorsA(NULL, level, buffer, cbBuf-1, &pcbNeeded, &pcReturned);
        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%ld) returned %ld with %ld (expected '0' with " \
            "ERROR_INSUFFICIENT_BUFFER)\n", level, res, GetLastError());

        ok(pcbNeeded == cbBuf, "(%ld) returned %ld (expected %ld)\n", level,
                pcbNeeded, cbBuf);

/*
      Do not add the next test:
      w2k+:  RPC_X_NULL_REF_POINTER 
      NT3.5: ERROR_INVALID_USER_BUFFER
      win9x: crash in winspool.drv

      res = EnumMonitorsA(NULL, level, NULL, cbBuf, &pcbNeeded, &pcReturned);
*/

        SetLastError(MAGIC_DEAD);
        pcbNeeded = MAGIC_DEAD;
        pcReturned = MAGIC_DEAD;
        res = EnumMonitorsA(NULL, level, buffer, cbBuf, NULL, &pcReturned);
        ok( res || (!res && (GetLastError() == RPC_X_NULL_REF_POINTER)) ,
            "(%ld) returned %ld with %ld (expected '!=0' or '0' with "\
            "RPC_X_NULL_REF_POINTER)\n", level, res, GetLastError());

        pcbNeeded = MAGIC_DEAD;
        pcReturned = MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = EnumMonitorsA(NULL, level, buffer, cbBuf, &pcbNeeded, NULL);
        ok( res || (!res && (GetLastError() == RPC_X_NULL_REF_POINTER)) ,
            "(%ld) returned %ld with %ld (expected '!=0' or '0' with "\
            "RPC_X_NULL_REF_POINTER)\n", level, res, GetLastError());

        HeapFree(GetProcessHeap(), 0, buffer);
    } /* for(level ... */
}


static void test_GetDefaultPrinter(void)
{
    BOOL    retval;
    DWORD   exact = DEFAULT_PRINTER_SIZE;
    DWORD   size;
    char    buffer[DEFAULT_PRINTER_SIZE];

    if (!pGetDefaultPrinterA)  return;
	/* only supported on NT like OSes starting with win2k */

    SetLastError(ERROR_SUCCESS);
    retval = pGetDefaultPrinterA(buffer, &exact);
    if (!retval || !exact || !strlen(buffer) ||
	(ERROR_SUCCESS != GetLastError())) {
	if ((ERROR_FILE_NOT_FOUND == GetLastError()) ||
	    (ERROR_INVALID_NAME == GetLastError()))
	    trace("this test requires a default printer to be set\n");
	else {
		ok( 0, "function call GetDefaultPrinterA failed unexpected!\n"
		"function returned %s\n"
		"last error 0x%08lx\n"
		"returned buffer size 0x%08lx\n"
		"returned buffer content %s\n",
		retval ? "true" : "false", GetLastError(), exact, buffer);
	}
	return;
    }
    SetLastError(ERROR_SUCCESS);
    retval = pGetDefaultPrinterA(NULL, NULL); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INVALID_PARAMETER == GetLastError(),
	"Last error wrong! ERROR_INVALID_PARAMETER expected, got 0x%08lx\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = pGetDefaultPrinterA(buffer, NULL); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INVALID_PARAMETER == GetLastError(),
	"Last error wrong! ERROR_INVALID_PARAMETER expected, got 0x%08lx\n",
	GetLastError());

    SetLastError(ERROR_SUCCESS);
    size = 0;
    retval = pGetDefaultPrinterA(NULL, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    SetLastError(ERROR_SUCCESS);
    size = DEFAULT_PRINTER_SIZE;
    retval = pGetDefaultPrinterA(NULL, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    size = 0;
    retval = pGetDefaultPrinterA(buffer, &size); 
    ok( !retval, "function result wrong! False expected\n");
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
	"Last error wrong! ERROR_INSUFFICIENT_BUFFER expected, got 0x%08lx\n",
	GetLastError());
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);

    size = exact;
    retval = pGetDefaultPrinterA(buffer, &size); 
    ok( retval, "function result wrong! True expected\n");
    ok( size == exact, "Parameter size wrong! %ld expected got %ld\n",
	exact, size);
}

static void test_GetPrinterDriverDirectory(void)
{
    LPBYTE buffer = NULL;
    DWORD  cbBuf = 0, pcbNeeded = 0;
    BOOL   res;

    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, 0, &cbBuf);
    trace("first call returned 0x%04x, with %ld: buffer size 0x%08lx\n",
    res, GetLastError(), cbBuf);

    RETURN_ON_DEACTIVATED_SPOOLER(res)
    ok((res == 0) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "returned %d with lasterror=%ld (expected '0' with " \
        "ERROR_INSUFFICIENT_BUFFER)\n", res, GetLastError());

    if (!cbBuf) {
        trace("no valid buffer size returned, skipping tests\n");
        return;
    }

    buffer = HeapAlloc( GetProcessHeap(), 0, cbBuf*2);
    if (buffer == NULL)  return ;

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);

    res = GetPrinterDriverDirectoryA(NULL, NULL, 1, buffer, cbBuf*2, &pcbNeeded);
    ok( res, "expected result != 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
 
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf-1, &pcbNeeded);
    ok( !res , "expected result == 0, got %d\n", res);
    ok( cbBuf == pcbNeeded, "pcbNeeded set to %ld instead of %ld\n",
                            pcbNeeded, cbBuf);
    
    ok( ERROR_INSUFFICIENT_BUFFER == GetLastError(),
        "last error set to %ld instead of ERROR_INSUFFICIENT_BUFFER\n",
        GetLastError());

/*
    Do not add the next test:
    XPsp2: crash in this app, when the spooler is not running 
    NT3.5: ERROR_INVALID_USER_BUFFER
    win9x: ERROR_INVALID_PARAMETER

    pcbNeeded = MAGIC_DEAD;
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, &pcbNeeded);
*/

    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, buffer, cbBuf, NULL);
    ok( (!res && RPC_X_NULL_REF_POINTER == GetLastError()) || res,
         "expected either result == 0 and "
         "last error == RPC_X_NULL_REF_POINTER or result != 0 "
         "got result %d and last error == %ld\n", res, GetLastError());

    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA( NULL, NULL, 1, NULL, cbBuf, NULL);
    ok(res || (GetLastError() == RPC_X_NULL_REF_POINTER),
        "returned %d with %ld (expected '!=0' or '0' with " \
        "RPC_X_NULL_REF_POINTER)\n", res, GetLastError());
 
 
    /* with a valid buffer, but level is too large */
    buffer[0] = '\0';
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA(NULL, NULL, 2, buffer, cbBuf, &pcbNeeded);

    /* Level not checked in win9x and wine:*/
    if((res != FALSE) && buffer[0])
    {
        trace("Level '2' not checked '%s'\n", buffer);
    }
    else
    {
        ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "returned %d with lasterror=%ld (expected '0' with " \
        "ERROR_INVALID_LEVEL)\n", res, GetLastError());
    }

    /* printing environments are case insensitive */
    /* "Windows 4.0" is valid for win9x and NT */
    buffer[0] = '\0';
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA(NULL, env_win9x_case, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        cbBuf = pcbNeeded;
        buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, cbBuf*2);
        if (buffer == NULL)  return ;

        SetLastError(MAGIC_DEAD);
        res = GetPrinterDriverDirectoryA(NULL, env_win9x_case, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);
    }

    ok(res && buffer[0], "returned %d with " \
        "lasterror=%ld and len=%d (expected '1' with 'len > 0')\n", 
        res, GetLastError(), lstrlenA((char *)buffer));

    buffer[0] = '\0';
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA(NULL, env_x86, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        cbBuf = pcbNeeded;
        buffer = HeapReAlloc(GetProcessHeap(), 0, buffer, cbBuf*2);
        if (buffer == NULL)  return ;

        buffer[0] = '\0';
        SetLastError(MAGIC_DEAD);
        res = GetPrinterDriverDirectoryA(NULL, env_x86, 1, 
                                        buffer, cbBuf*2, &pcbNeeded);
    }

    /* "Windows NT x86" is invalid for win9x */
    ok( (res && buffer[0]) ||
        (!res && (GetLastError() == ERROR_INVALID_ENVIRONMENT)), 
        "returned %d with lasterror=%ld and len=%d (expected '!= 0' with " \
        "'len > 0' or '0' with ERROR_INVALID_ENVIRONMENT)\n",
        res, GetLastError(), lstrlenA((char *)buffer));

    /* A Setup-Programm (PDFCreator_0.8.0) use empty strings */
    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA("", "", 1, buffer, cbBuf*2, &pcbNeeded);
    ok(res, "returned %d with %ld (expected '!=0')\n", res, GetLastError() );

    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA(NULL, "", 1, buffer, cbBuf*2, &pcbNeeded);
    ok(res, "returned %d with %ld (expected '!=0')\n", res, GetLastError() );

    SetLastError(MAGIC_DEAD);
    res = GetPrinterDriverDirectoryA("", NULL, 1, buffer, cbBuf*2, &pcbNeeded);
    ok(res, "returned %d with %ld (expected '!=0')\n", res, GetLastError() );

    HeapFree( GetProcessHeap(), 0, buffer);
}

static void test_OpenPrinter(void)
{
    PRINTER_DEFAULTSA defaults;
    HANDLE  hprinter;
    LPSTR   default_printer;
    DWORD   res;
    DWORD   size;
    CHAR    buffer[DEFAULT_PRINTER_SIZE];
    LPSTR   ptr;

    SetLastError(MAGIC_DEAD);
    res = OpenPrinter(NULL, NULL, NULL);    
    /* The deactivated Spooler is catched here on NT3.51 */
    RETURN_ON_DEACTIVATED_SPOOLER(res)
    ok(!res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %ld with %ld (expected '0' with ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());


    /* Get Handle for the local Printserver (NT only)*/
    hprinter = (HANDLE) MAGIC_DEAD;
    SetLastError(MAGIC_DEAD);
    res = OpenPrinter(NULL, &hprinter, NULL);
    /* The deactivated Spooler is catched here on XPsp2 */
    RETURN_ON_DEACTIVATED_SPOOLER(res)
    ok(res || (!res && GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %ld with %ld (expected '!=0' or '0' with ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());
    if(res) {
        ClosePrinter(hprinter);

        defaults.pDatatype=NULL;
        defaults.pDevMode=NULL;

        defaults.DesiredAccess=0;
        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(NULL, &hprinter, &defaults);
        ok(res, "returned %ld with %ld (expected '!=0')\n", res, GetLastError());
        if (res) ClosePrinter(hprinter);

        defaults.DesiredAccess=-1;
        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(NULL, &hprinter, &defaults);
        ok(!res && GetLastError() == ERROR_ACCESS_DENIED,
            "returned %ld with %ld (expected '0' with ERROR_ACCESS_DENIED)\n", 
            res, GetLastError());
        if (res) ClosePrinter(hprinter);

    }

    size = sizeof(buffer) - 3 ;
    ptr = buffer;
    ptr[0] = '\\';
    ptr++;
    ptr[0] = '\\';
    ptr++;
    if (GetComputerNameA(ptr, &size)) {

        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(buffer, &hprinter, NULL);
        todo_wine {
        ok(res || (!res && GetLastError() == ERROR_INVALID_PARAMETER),
            "returned %ld with %ld (expected '!=0' or '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        }
        if(res) ClosePrinter(hprinter);
    }

    /* Invalid Printername */
    hprinter = (HANDLE) MAGIC_DEAD;
    SetLastError(MAGIC_DEAD);
    res = OpenPrinter("illegal,name", &hprinter, NULL);
    ok(!res && ((GetLastError() == ERROR_INVALID_PRINTER_NAME) || 
                (GetLastError() == ERROR_INVALID_PARAMETER) ),
       "returned %ld with %ld (expected '0' with: ERROR_INVALID_PARAMETER or" \
       "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());
    if(res) ClosePrinter(hprinter);


    /* Get Handle for the default Printer */
    if ((default_printer = find_default_printer()))
    {
        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(default_printer, &hprinter, NULL);
        if((!res) && (GetLastError() == RPC_S_SERVER_UNAVAILABLE))
        {
                trace("The Service 'Spooler' is required for '%s'\n", default_printer);
            return;
        }
        ok(res, "returned %ld with %ld (expected '!=0')\n", res, GetLastError());
        if(res) ClosePrinter(hprinter);

        defaults.pDatatype=NULL;
        defaults.pDevMode=NULL;
        defaults.DesiredAccess=0;

        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(default_printer, &hprinter, &defaults);
        ok(res || GetLastError() == ERROR_ACCESS_DENIED,
            "returned %ld with %ld (expected '!=0' or '0' with " \
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
        if(res) ClosePrinter(hprinter);

        defaults.pDatatype="";

        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(default_printer, &hprinter, &defaults);
        /* stop here, when a remote Printserver has no RPC-Service running */
        RETURN_ON_DEACTIVATED_SPOOLER(res)
        ok(res || ((GetLastError() == ERROR_INVALID_DATATYPE) ||
                   (GetLastError() == ERROR_ACCESS_DENIED)),
            "returned %ld with %ld (expected '!=0' or '0' with: " \
            "ERROR_INVALID_DATATYPE or ERROR_ACCESS_DENIED)\n",
            res, GetLastError());
        if(res) ClosePrinter(hprinter);


        defaults.pDatatype=NULL;
        defaults.DesiredAccess=PRINTER_ACCESS_USE;

        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(default_printer, &hprinter, &defaults);
        ok(res || GetLastError() == ERROR_ACCESS_DENIED,
            "returned %ld with %ld (expected '!=0' or '0' with " \
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
        if(res) ClosePrinter(hprinter);


        defaults.DesiredAccess=PRINTER_ALL_ACCESS;
        hprinter = (HANDLE) MAGIC_DEAD;
        SetLastError(MAGIC_DEAD);
        res = OpenPrinter(default_printer, &hprinter, &defaults);
        ok(res || GetLastError() == ERROR_ACCESS_DENIED,
            "returned %ld with %ld (expected '!=0' or '0' with " \
            "ERROR_ACCESS_DENIED)\n", res, GetLastError());
        if(res) ClosePrinter(hprinter);
    }

}


static void test_SetDefaultPrinter(void)
{
    DWORD   res;
    LPSTR   default_printer;
    DWORD   size = DEFAULT_PRINTER_SIZE;
    CHAR    buffer[DEFAULT_PRINTER_SIZE];
    CHAR    org_value[DEFAULT_PRINTER_SIZE];


    if (!pSetDefaultPrinterA)  return;
	/* only supported on win2k and above */

    default_printer = find_default_printer();

    /* backup the original value */
    org_value[0] = '\0';
    SetLastError(MAGIC_DEAD);
    res = GetProfileStringA("windows", "device", NULL, org_value, size);

    /* first part: with the default Printer */
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA("no_printer_with_this_name");

    RETURN_ON_DEACTIVATED_SPOOLER(res)
    /* spooler is running or we have no spooler here*/

    /* Not implemented in wine */
    if (!res && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
        trace("SetDefaultPrinterA() not implemented yet.\n");
        return;
    }

    ok(!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME),
        "returned %ld with %ld (expected '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", org_value);
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA("");
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "returned %ld with %ld (expected '!=0' or '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", org_value);
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA(NULL);
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "returned %ld with %ld (expected '!=0' or '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", org_value);
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA(default_printer);
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "returned %ld with %ld (expected '!=0' or '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());


    /* second part: always without a default Printer */
    WriteProfileStringA("windows", "device", NULL);    
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA("no_printer_with_this_name");

    ok(!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME),
        "returned %ld with %ld (expected '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", NULL);    
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA("");
    /* we get ERROR_INVALID_PRINTER_NAME when no printer is installed */
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
         "returned %ld with %ld (expected '!=0' or '0' with " \
         "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", NULL);    
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA(NULL);
    /* we get ERROR_INVALID_PRINTER_NAME when no printer is installed */
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "returned %ld with %ld (expected '!=0' or '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    WriteProfileStringA("windows", "device", NULL);    
    SetLastError(MAGIC_DEAD);
    res = pSetDefaultPrinterA(default_printer);
    ok(res || (!res && (GetLastError() == ERROR_INVALID_PRINTER_NAME)),
        "returned %ld with %ld (expected '!=0' or '0' with " \
        "ERROR_INVALID_PRINTER_NAME)\n", res, GetLastError());

    /* restore the original value */
    res = pSetDefaultPrinterA(default_printer);          /* the nice way */
    WriteProfileStringA("windows", "device", org_value); /* the old way */

    buffer[0] = '\0';
    SetLastError(MAGIC_DEAD);
    res = GetProfileStringA("windows", "device", NULL, buffer, size);
    ok(!lstrcmpA(org_value, buffer), "'%s' (expected '%s')\n", buffer, org_value);

}


START_TEST(info)
{
    hwinspool = GetModuleHandleA("winspool.drv");
    pGetDefaultPrinterA = (void *) GetProcAddress(hwinspool, "GetDefaultPrinterA");
    pSetDefaultPrinterA = (void *) GetProcAddress(hwinspool, "SetDefaultPrinterA");

    find_default_printer();

    test_EnumMonitors(); 
    test_GetDefaultPrinter();
    test_GetPrinterDriverDirectory();
    test_OpenPrinter();
    test_SetDefaultPrinter();
}
