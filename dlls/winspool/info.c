/*
 * WINSPOOL functions
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 * Copyright 2001 Marcus Meissner
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#ifdef HAVE_CUPS_CUPS_H
# include <cups/cups.h>
# ifndef SONAME_LIBCUPS
#  define SONAME_LIBCUPS "libcups.so"
# endif
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/library.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winspool.h"
#include "winternl.h"
#include "wine/windef16.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "heap.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

static LPWSTR *printer_array;
static int nb_printers;

static DWORD (WINAPI *GDI_CallDeviceCapabilities16)( LPCSTR lpszDevice, LPCSTR lpszPort,
                                                     WORD fwCapability, LPSTR lpszOutput,
                                                     LPDEVMODEA lpdm );
static INT (WINAPI *GDI_CallExtDeviceMode16)( HWND hwnd, LPDEVMODEA lpdmOutput,
                                              LPSTR lpszDevice, LPSTR lpszPort,
                                              LPDEVMODEA lpdmInput, LPSTR lpszProfile,
                                              DWORD fwMode );

static char Printers[] =
"System\\CurrentControlSet\\control\\Print\\Printers\\";
static char Drivers[] =
"System\\CurrentControlSet\\control\\Print\\Environments\\%s\\Drivers\\";

static WCHAR DefaultEnvironmentW[] = {'W','i','n','e',0};

static WCHAR Configuration_FileW[] = {'C','o','n','f','i','g','u','r','a','t',
				      'i','o','n',' ','F','i','l','e',0};
static WCHAR DatatypeW[] = {'D','a','t','a','t','y','p','e',0};
static WCHAR Data_FileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static WCHAR Default_DevModeW[] = {'D','e','f','a','u','l','t',' ','D','e','v',
				   'M','o','d','e',0};
static WCHAR Dependent_FilesW[] = {'D','e','p','e','n','d','e','n','t',' ','F',
				   'i','l','e','s',0};
static WCHAR DescriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static WCHAR DriverW[] = {'D','r','i','v','e','r',0};
static WCHAR Help_FileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static WCHAR LocationW[] = {'L','o','c','a','t','i','o','n',0};
static WCHAR MonitorW[] = {'M','o','n','i','t','o','r',0};
static WCHAR NameW[] = {'N','a','m','e',0};
static WCHAR ParametersW[] = {'P','a','r','a','m','e','t','e','r','s',0};
static WCHAR PortW[] = {'P','o','r','t',0};
static WCHAR Print_ProcessorW[] = {'P','r','i','n','t',' ','P','r','o','c','e',
				   's','s','o','r',0};
static WCHAR Printer_DriverW[] = {'P','r','i','n','t','e','r',' ','D','r','i',
				  'v','e','r',0};
static WCHAR PrinterDriverDataW[] = {'P','r','i','n','t','e','r','D','r','i',
				     'v','e','r','D','a','t','a',0};
static WCHAR Separator_FileW[] = {'S','e','p','a','r','a','t','o','r',' ','F',
				  'i','l','e',0};
static WCHAR Share_NameW[] = {'S','h','a','r','e',' ','N','a','m','e',0};
static WCHAR WinPrintW[] = {'W','i','n','P','r','i','n','t',0};

static HKEY WINSPOOL_OpenDriverReg( LPVOID pEnvironment, BOOL unicode);
static BOOL WINSPOOL_GetPrinterDriver(HANDLE hPrinter, LPWSTR pEnvironment,
				      DWORD Level, LPBYTE pDriverInfo,
				      DWORD cbBuf, LPDWORD pcbNeeded,
				      BOOL unicode);

/* RtlCreateUnicodeStringFromAsciiz will return an empty string in the buffer
   if passed a NULL string. This returns NULLs to the result. 
*/
static inline PWSTR asciitounicode( UNICODE_STRING * usBufferPtr, LPCSTR src )
{
    if ( (src) )
    {
        RtlCreateUnicodeStringFromAsciiz(usBufferPtr, src);
        return usBufferPtr->Buffer;
    }
    usBufferPtr->Buffer = NULL; /* so that RtlFreeUnicodeString won't barf */
    return NULL;
}
            
static void
WINSPOOL_SetDefaultPrinter(const char *devname, const char *name,BOOL force) {
    char qbuf[200];

    /* If forcing, or no profile string entry for device yet, set the entry
     *
     * The always change entry if not WINEPS yet is discussable.
     */
    if (force								||
	!GetProfileStringA("windows","device","*",qbuf,sizeof(qbuf))	||
	!strcmp(qbuf,"*")						||
	!strstr(qbuf,"WINEPS")
    ) {
    	char *buf = HeapAlloc(GetProcessHeap(),0,strlen(name)+strlen(devname)+strlen(",WINEPS,LPR:")+1);

	sprintf(buf,"%s,WINEPS,LPR:%s",devname,name);
	WriteProfileStringA("windows","device",buf);
	HeapFree(GetProcessHeap(),0,buf);
    }
}

#ifdef HAVE_CUPS_CUPS_H
BOOL
CUPS_LoadPrinters(void) {
    typeof(cupsGetPrinters) *pcupsGetPrinters = NULL;
    typeof(cupsGetDefault) *pcupsGetDefault = NULL;
    typeof(cupsGetPPD) *pcupsGetPPD = NULL;
    char		**printers;
    int			i,nrofdests,hadprinter = FALSE;
    PRINTER_INFO_2A	pinfo2a;
    const char*		def;
    void *cupshandle = NULL;
    const  char *ppd;
    char   *port,*devline;
    UNICODE_STRING  lpszNameW;
    PWSTR pwstrNameW;
    HKEY hkeyPrinters, hkeyPrinter;

    cupshandle = wine_dlopen(SONAME_LIBCUPS, RTLD_NOW, NULL, 0);
    if (!cupshandle) 
	return FALSE;

#define DYNCUPS(x) 					\
    	p##x = wine_dlsym(cupshandle, #x, NULL,0);	\
	if (!p##x) return FALSE;

    DYNCUPS(cupsGetDefault);
    DYNCUPS(cupsGetPPD);
    DYNCUPS(cupsGetPrinters);
#undef DYNCUPS

    def = pcupsGetDefault();

    if (def && !strcmp(def,"none")) /* CUPS has "none" for no default printer */
    	def = NULL;

    nrofdests = pcupsGetPrinters(&printers);
    for (i=0;i<nrofdests;i++) {
        /* First check that the printer doesn't exist already */
        pwstrNameW = asciitounicode(&lpszNameW, printers[i]);
        if (RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) ==
            ERROR_SUCCESS) {
             if (RegOpenKeyW(hkeyPrinters, pwstrNameW, &hkeyPrinter) ==
                 ERROR_SUCCESS) {
                  /* We know this printer already */
                  RegCloseKey(hkeyPrinter);
                  RegCloseKey(hkeyPrinters);
                  RtlFreeUnicodeString(&lpszNameW);
                  TRACE("Printer %s already known. Skipping detection\n", printers[i]);
                  continue;
             }
             RegCloseKey(hkeyPrinters);
        }
        RtlFreeUnicodeString(&lpszNameW);

        /* OK, we haven't seen this one yet. Request PPD for it */
	ppd = pcupsGetPPD(printers[i]);
	if (!ppd) {
	    WARN("No ppd file for %s.\n",printers[i]);
	    /* If this was going to be the default printer,
	     * forget it and use another one.
	     */
	    if (def && !strcmp(def,printers[i]))
	        def = NULL;
	    continue;
	}
	unlink(ppd);

	hadprinter = TRUE;

	if (def && !strcmp(def,printers[i]))
	        WINSPOOL_SetDefaultPrinter(printers[i],printers[i],FALSE);

	/* The default printer has no PPD file, just use the first one
	 * which has one.
	 */
	if (!def) {
	        WINSPOOL_SetDefaultPrinter(printers[i],printers[i],FALSE);
		def = printers[i];
	}
	memset(&pinfo2a,0,sizeof(pinfo2a));
	pinfo2a.pPrinterName	= printers[i];
	pinfo2a.pDatatype	= "RAW";
	pinfo2a.pPrintProcessor	= "WinPrint";
	pinfo2a.pDriverName	= "PS Driver";
	pinfo2a.pComment	= "WINEPS Printer using CUPS";
	pinfo2a.pLocation	= "<physical location of printer>";
	port = HeapAlloc(GetProcessHeap(),0,strlen("LPR:")+strlen(printers[i])+1);
	sprintf(port,"LPR:%s",printers[i]);
	pinfo2a.pPortName	= port;
	pinfo2a.pParameters	= "<parameters?>";
	pinfo2a.pShareName	= "<share name?>";
	pinfo2a.pSepFile	= "<sep file?>";

	devline=HeapAlloc(GetProcessHeap(),0,strlen("WINEPS,")+strlen(port)+1);
	sprintf(devline,"WINEPS,%s",port);
	WriteProfileStringA("devices",printers[i],devline);
	HeapFree(GetProcessHeap(),0,devline);

	if (!AddPrinterA(NULL,2,(LPBYTE)&pinfo2a)) {
	    if (GetLastError()!=ERROR_PRINTER_ALREADY_EXISTS)
	        ERR("printer '%s' not added by AddPrinterA (error %ld)\n",printers[i],GetLastError());
	}
	HeapFree(GetProcessHeap(),0,port);
    }
    wine_dlclose(cupshandle, NULL, 0);
    return hadprinter;
}
#endif

static BOOL
PRINTCAP_ParseEntry(char *pent,BOOL isfirst) {
    PRINTER_INFO_2A	pinfo2a;
    char		*s,*name,*prettyname,*devname;
    BOOL		isps = FALSE;
    char		*port,*devline;

    while (isspace(*pent)) pent++;
    s = strchr(pent,':');
    if (!s) return FALSE;
    *s='\0';
    name = pent;
    pent = s+1;
    TRACE("%s\n",name);

    /* Determine whether this is a postscript printer. */

    /* 1. Check if name or aliases contain trigger phrases like 'ps' */
    if (strstr(name,"ps")		||
	strstr(name,"pd")		||	/* postscript double page */
	strstr(name,"postscript")	||
	strstr(name,"PostScript")
    ) {
	TRACE("%s has 'ps' style name, assuming postscript.\n",name);
	isps = TRUE;
    }
    /* 2. Check if this is a remote printer. These usually are postscript
     *    capable
     */
    if (strstr(pent,":rm")) {
	isps = TRUE;
	TRACE("%s is remote, assuming postscript.\n",name);
    }
    /* 3. Check if we have an input filter program. If we have one, it
     *    most likely is one capable of converting postscript.
     *    (Could probably check for occurrence of 'gs' or 'ghostscript'
     *     in the if file itself.)
     */
    if (strstr(pent,":if=/")) {
	isps = TRUE;
	TRACE("%s has inputfilter program, assuming postscript.\n",name);
    }

    /* If it is not a postscript printer, we cannot use it. */
    if (!isps)
	return FALSE;

    prettyname = name;
    /* Get longest name, usually the one at the right for later display. */
    while ((s=strchr(prettyname,'|'))) prettyname = s+1;
    s=strchr(name,'|');if (s) *s='\0';

    /* prettyname must fit into the dmDeviceName member of DEVMODE struct,
     * if it is too long, we use it as comment below. */
    devname = prettyname;
    if (strlen(devname)>=CCHDEVICENAME-1)
	 devname = name;
    if (strlen(devname)>=CCHDEVICENAME-1)
	return FALSE;

    if (isfirst) /* set first entry as default */
	    WINSPOOL_SetDefaultPrinter(devname,name,FALSE);

    memset(&pinfo2a,0,sizeof(pinfo2a));
    pinfo2a.pPrinterName	= devname;
    pinfo2a.pDatatype		= "RAW";
    pinfo2a.pPrintProcessor	= "WinPrint";
    pinfo2a.pDriverName		= "PS Driver";
    pinfo2a.pComment		= "WINEPS Printer using LPR";
    pinfo2a.pLocation		= prettyname;
    port = HeapAlloc(GetProcessHeap(),0,strlen("LPR:")+strlen(name)+1);
    sprintf(port,"LPR:%s",name);
    pinfo2a.pPortName		= port;
    pinfo2a.pParameters		= "<parameters?>";
    pinfo2a.pShareName		= "<share name?>";
    pinfo2a.pSepFile		= "<sep file?>";

    devline=HeapAlloc(GetProcessHeap(),0,strlen("WINEPS,")+strlen(port)+1);
    sprintf(devline,"WINEPS,%s",port);
    WriteProfileStringA("devices",devname,devline);
    HeapFree(GetProcessHeap(),0,devline);

    if (!AddPrinterA(NULL,2,(LPBYTE)&pinfo2a)) {
	if (GetLastError()!=ERROR_PRINTER_ALREADY_EXISTS)
	    ERR("%s not added by AddPrinterA (%ld)\n",name,GetLastError());
    }
    HeapFree(GetProcessHeap(),0,port);
    return TRUE;
}

static BOOL
PRINTCAP_LoadPrinters(void) {
    BOOL		hadprinter = FALSE, isfirst = TRUE;
    char		buf[200];
    FILE		*f;

    f = fopen("/etc/printcap","r");
    if (!f)
	return FALSE;

    while (fgets(buf,sizeof(buf),f)) {
	char	*pent = NULL;
	do {
	    char	*s;
	    s=strchr(buf,'\n'); if (s) *s='\0';
	    if ((buf[0]=='#') || (buf[0]=='\0'))
		continue;

	    if (pent) {
		pent=HeapReAlloc(GetProcessHeap(),0,pent,strlen(pent)+strlen(buf)+2);
		strcat(pent,buf);
	    } else {
		pent=HeapAlloc(GetProcessHeap(),0,strlen(buf)+1);
		strcpy(pent,buf);
	    }

	    if (strlen(pent) && (pent[strlen(pent)-1] == '\\'))
		pent[strlen(pent)-1] = '\0';
	    else
		break;
	} while (fgets(buf,sizeof(buf),f));
	if (pent)
	    hadprinter |= PRINTCAP_ParseEntry(pent,isfirst);
	isfirst = FALSE;
	if (pent) HeapFree(GetProcessHeap(),0,pent);
	pent = NULL;
	if (feof(f)) break;
    }
    fclose(f);
    return hadprinter;
}

static inline DWORD set_reg_szW(HKEY hkey, WCHAR *keyname, WCHAR *value)
{
    return RegSetValueExW(hkey, keyname, 0, REG_SZ, (LPBYTE)value,
                   lstrlenW(value) * sizeof(WCHAR));
}

void
WINSPOOL_LoadSystemPrinters() {
    HKEY    	    	hkPPD;
    DRIVER_INFO_3A	di3a;
    di3a.cVersion = 0x400;
    di3a.pName = "PS Driver";
    di3a.pEnvironment = NULL;	/* NULL means auto */
    di3a.pDriverPath = "wineps16";
    di3a.pDataFile = "<datafile?>";
    di3a.pConfigFile = "wineps16";
    di3a.pHelpFile = "<helpfile?>";
    di3a.pDependentFiles = "<dependend files?>";
    di3a.pMonitorName = "<monitor name?>";
    di3a.pDefaultDataType = "RAW";

    if (!AddPrinterDriverA(NULL,3,(LPBYTE)&di3a)) {
	ERR("Failed adding PS Driver (%ld)\n",GetLastError());
        return;
    }
#ifdef HAVE_CUPS_CUPS_H
    /* If we have any CUPS based printers, skip looking for printcap printers */
    if (CUPS_LoadPrinters())
	return;
#endif

    /* Check for [ppd] section in config file before parsing /etc/printcap */

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\ppd",
    	    &hkPPD) == ERROR_SUCCESS)
    {
    	RegCloseKey(hkPPD);
    	PRINTCAP_LoadPrinters();
    }
}


/******************************************************************
 *  WINSPOOL_GetOpenedPrinterEntry
 *  Get the first place empty in the opened printer table
 */
static HANDLE WINSPOOL_GetOpenedPrinterEntry( LPCWSTR name )
{
    int i;

    for (i = 0; i < nb_printers; i++) if (!printer_array[i]) break;

    if (i >= nb_printers)
    {
        LPWSTR *new_array = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, printer_array,
                                         (nb_printers + 16) * sizeof(*new_array) );
        if (!new_array) return 0;
        printer_array = new_array;
        nb_printers += 16;
    }

    if ((printer_array[i] = HeapAlloc( GetProcessHeap(), 0, (strlenW(name)+1)*sizeof(WCHAR) )))
    {
        strcpyW( printer_array[i], name );
        return (HANDLE)(i + 1);
    }
    return 0;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinter
 *  Get the pointer to the opened printer referred by the handle
 */
static LPCWSTR WINSPOOL_GetOpenedPrinter(HANDLE printerHandle)
{
    int idx = (int)printerHandle;
    if ((idx <= 0) || (idx > nb_printers))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return printer_array[idx - 1];
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterRegKey
 *
 */
static DWORD WINSPOOL_GetOpenedPrinterRegKey(HANDLE hPrinter, HKEY *phkey)
{
    LPCWSTR name = WINSPOOL_GetOpenedPrinter(hPrinter);
    DWORD ret;
    HKEY hkeyPrinters;

    if(!name) return ERROR_INVALID_HANDLE;

    if((ret = RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters)) !=
       ERROR_SUCCESS)
        return ret;

    if(RegOpenKeyW(hkeyPrinters, name, phkey) != ERROR_SUCCESS)
    {
        ERR("Can't find opened printer %s in registry\n",
	    debugstr_w(name));
	RegCloseKey(hkeyPrinters);
        return ERROR_INVALID_PRINTER_NAME; /* ? */
    }
    RegCloseKey(hkeyPrinters);
    return ERROR_SUCCESS;
}

/***********************************************************
 *      DEVMODEcpyAtoW
 */
static LPDEVMODEW DEVMODEcpyAtoW(DEVMODEW *dmW, const DEVMODEA *dmA)
{
    BOOL Formname;
    ptrdiff_t off_formname = (char *)dmA->dmFormName - (char *)dmA;
    DWORD size;

    Formname = (dmA->dmSize > off_formname);
    size = dmA->dmSize + CCHDEVICENAME + (Formname ? CCHFORMNAME : 0);
    MultiByteToWideChar(CP_ACP, 0, dmA->dmDeviceName, -1, dmW->dmDeviceName,
			CCHDEVICENAME);
    if(!Formname) {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     dmA->dmSize - CCHDEVICENAME);
    } else {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     off_formname - CCHDEVICENAME);
      MultiByteToWideChar(CP_ACP, 0, dmA->dmFormName, -1, dmW->dmFormName,
			  CCHFORMNAME);
      memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA->dmSize -
	     (off_formname + CCHFORMNAME));
    }
    dmW->dmSize = size;
    memcpy((char *)dmW + dmW->dmSize, (char *)dmA + dmA->dmSize,
	   dmA->dmDriverExtra);
    return dmW;
}

/***********************************************************
 *      DEVMODEdupAtoW
 * Creates a unicode copy of supplied devmode on heap
 */
static LPDEVMODEW DEVMODEdupAtoW(HANDLE heap, const DEVMODEA *dmA)
{
    LPDEVMODEW dmW;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname;

    TRACE("\n");
    if(!dmA) return NULL;

    off_formname = (char *)dmA->dmFormName - (char *)dmA;
    Formname = (dmA->dmSize > off_formname);
    size = dmA->dmSize + CCHDEVICENAME + (Formname ? CCHFORMNAME : 0);
    dmW = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmA->dmDriverExtra);
    return DEVMODEcpyAtoW(dmW, dmA);
}

/***********************************************************
 *      DEVMODEdupWtoA
 * Creates an ascii copy of supplied devmode on heap
 */
static LPDEVMODEA DEVMODEdupWtoA(HANDLE heap, const DEVMODEW *dmW)
{
    LPDEVMODEA dmA;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname = (char *)dmW->dmFormName - (char *)dmW;

    if(!dmW) return NULL;
    Formname = (dmW->dmSize > off_formname);
    size = dmW->dmSize - CCHDEVICENAME - (Formname ? CCHFORMNAME : 0);
    dmA = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmW->dmDriverExtra);
    WideCharToMultiByte(CP_ACP, 0, dmW->dmDeviceName, -1, dmA->dmDeviceName,
			CCHDEVICENAME, NULL, NULL);
    if(!Formname) {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     dmW->dmSize - CCHDEVICENAME * sizeof(WCHAR));
    } else {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     off_formname - CCHDEVICENAME * sizeof(WCHAR));
      WideCharToMultiByte(CP_ACP, 0, dmW->dmFormName, -1, dmA->dmFormName,
			  CCHFORMNAME, NULL, NULL);
      memcpy(&dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize -
	     (off_formname + CCHFORMNAME * sizeof(WCHAR)));
    }
    dmA->dmSize = size;
    memcpy((char *)dmA + dmA->dmSize, (char *)dmW + dmW->dmSize,
	   dmW->dmDriverExtra);
    return dmA;
}

/***********************************************************
 *             PRINTER_INFO_2AtoW
 * Creates a unicode copy of PRINTER_INFO_2A on heap
 */
static LPPRINTER_INFO_2W PRINTER_INFO_2AtoW(HANDLE heap, LPPRINTER_INFO_2A piA)
{
    LPPRINTER_INFO_2W piW;
    UNICODE_STRING usBuffer;

    if(!piA) return NULL;
    piW = HeapAlloc(heap, 0, sizeof(*piW));
    memcpy(piW, piA, sizeof(*piW)); /* copy everything first */
    
    piW->pServerName = asciitounicode(&usBuffer,piA->pServerName);
    piW->pPrinterName = asciitounicode(&usBuffer,piA->pPrinterName);
    piW->pShareName = asciitounicode(&usBuffer,piA->pShareName);
    piW->pPortName = asciitounicode(&usBuffer,piA->pPortName);
    piW->pDriverName = asciitounicode(&usBuffer,piA->pDriverName);
    piW->pComment = asciitounicode(&usBuffer,piA->pComment);
    piW->pLocation = asciitounicode(&usBuffer,piA->pLocation);
    piW->pDevMode = DEVMODEdupAtoW(heap, piA->pDevMode);
    piW->pSepFile = asciitounicode(&usBuffer,piA->pSepFile);
    piW->pPrintProcessor = asciitounicode(&usBuffer,piA->pPrintProcessor);
    piW->pDatatype = asciitounicode(&usBuffer,piA->pDatatype);
    piW->pParameters = asciitounicode(&usBuffer,piA->pParameters);
    return piW;
}

/***********************************************************
 *       FREE_PRINTER_INFO_2W
 * Free PRINTER_INFO_2W and all strings
 */
static void FREE_PRINTER_INFO_2W(HANDLE heap, LPPRINTER_INFO_2W piW)
{
    if(!piW) return;

    HeapFree(heap,0,piW->pServerName);
    HeapFree(heap,0,piW->pPrinterName);
    HeapFree(heap,0,piW->pShareName);
    HeapFree(heap,0,piW->pPortName);
    HeapFree(heap,0,piW->pDriverName);
    HeapFree(heap,0,piW->pComment);
    HeapFree(heap,0,piW->pLocation);
    HeapFree(heap,0,piW->pDevMode);
    HeapFree(heap,0,piW->pSepFile);
    HeapFree(heap,0,piW->pPrintProcessor);
    HeapFree(heap,0,piW->pDatatype);
    HeapFree(heap,0,piW->pParameters);
    HeapFree(heap,0,piW);
    return;
}

/******************************************************************
 *              DeviceCapabilities     [WINSPOOL.@]
 *              DeviceCapabilitiesA    [WINSPOOL.@]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR pDevice,LPCSTR pPort, WORD cap,
			       LPSTR pOutput, LPDEVMODEA lpdm)
{
    INT ret;

    if (!GDI_CallDeviceCapabilities16)
    {
        GDI_CallDeviceCapabilities16 = (void*)GetProcAddress( GetModuleHandleA("gdi32"),
                                                              (LPCSTR)104 );
        if (!GDI_CallDeviceCapabilities16) return -1;
    }
    ret = GDI_CallDeviceCapabilities16(pDevice, pPort, cap, pOutput, lpdm);

    /* If DC_PAPERSIZE map POINT16s to POINTs */
    if(ret != -1 && cap == DC_PAPERSIZE && pOutput) {
        POINT16 *tmp = HeapAlloc( GetProcessHeap(), 0, ret * sizeof(POINT16) );
        POINT *pt = (POINT *)pOutput;
	INT i;
	memcpy(tmp, pOutput, ret * sizeof(POINT16));
	for(i = 0; i < ret; i++, pt++)
        {
            pt->x = tmp[i].x;
            pt->y = tmp[i].y;
        }
	HeapFree( GetProcessHeap(), 0, tmp );
    }
    return ret;
}


/*****************************************************************************
 *          DeviceCapabilitiesW        [WINSPOOL.@]
 *
 * Call DeviceCapabilitiesA since we later call 16bit stuff anyway
 *
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    LPDEVMODEA dmA = DEVMODEdupWtoA(GetProcessHeap(), pDevMode);
    LPSTR pDeviceA = HEAP_strdupWtoA(GetProcessHeap(),0,pDevice);
    LPSTR pPortA = HEAP_strdupWtoA(GetProcessHeap(),0,pPort);
    INT ret;

    if(pOutput && (fwCapability == DC_BINNAMES ||
		   fwCapability == DC_FILEDEPENDENCIES ||
		   fwCapability == DC_PAPERNAMES)) {
      /* These need A -> W translation */
        INT size = 0, i;
	LPSTR pOutputA;
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, NULL,
				  dmA);
	if(ret == -1)
	    return ret;
	switch(fwCapability) {
	case DC_BINNAMES:
	    size = 24;
	    break;
	case DC_PAPERNAMES:
	case DC_FILEDEPENDENCIES:
	    size = 64;
	    break;
	}
	pOutputA = HeapAlloc(GetProcessHeap(), 0, size * ret);
	ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, pOutputA,
				  dmA);
	for(i = 0; i < ret; i++)
	    MultiByteToWideChar(CP_ACP, 0, pOutputA + (i * size), -1,
				pOutput + (i * size), size);
	HeapFree(GetProcessHeap(), 0, pOutputA);
    } else {
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability,
				  (LPSTR)pOutput, dmA);
    }
    HeapFree(GetProcessHeap(),0,pPortA);
    HeapFree(GetProcessHeap(),0,pDeviceA);
    HeapFree(GetProcessHeap(),0,dmA);
    return ret;
}

/******************************************************************
 *              DocumentPropertiesA   [WINSPOOL.@]
 *
 */
LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
				LPDEVMODEA pDevModeInput,DWORD fMode )
{
    LPSTR lpName = pDeviceName;
    LONG ret;

    TRACE("(%p,%p,%s,%p,%p,%ld)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );

    if(!pDeviceName) {
        LPCWSTR lpNameW = WINSPOOL_GetOpenedPrinter(hPrinter);
        if(!lpNameW) {
		ERR("no name from hPrinter?\n");
		return -1;
	}
	lpName = HEAP_strdupWtoA(GetProcessHeap(),0,lpNameW);
    }

    if (!GDI_CallExtDeviceMode16)
    {
        GDI_CallExtDeviceMode16 = (void*)GetProcAddress( GetModuleHandleA("gdi32"),
                                                         (LPCSTR)102 );
        if (!GDI_CallExtDeviceMode16) {
		ERR("No CallExtDeviceMode16?\n");
		return -1;
	}
    }
    ret = GDI_CallExtDeviceMode16(hWnd, pDevModeOutput, lpName, "LPT1:",
				  pDevModeInput, NULL, fMode);

    if(!pDeviceName)
        HeapFree(GetProcessHeap(),0,lpName);
    return ret;
}


/*****************************************************************************
 *          DocumentPropertiesW (WINSPOOL.@)
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
				LPWSTR pDeviceName,
				LPDEVMODEW pDevModeOutput,
				LPDEVMODEW pDevModeInput, DWORD fMode)
{

    LPSTR pDeviceNameA = HEAP_strdupWtoA(GetProcessHeap(),0,pDeviceName);
    LPDEVMODEA pDevModeInputA = DEVMODEdupWtoA(GetProcessHeap(),pDevModeInput);
    LPDEVMODEA pDevModeOutputA = NULL;
    LONG ret;

    TRACE("(%p,%p,%s,%p,%p,%ld)\n",
          hWnd,hPrinter,debugstr_w(pDeviceName),pDevModeOutput,pDevModeInput,
	  fMode);
    if(pDevModeOutput) {
        ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, NULL, NULL, 0);
	if(ret < 0) return ret;
	pDevModeOutputA = HeapAlloc(GetProcessHeap(), 0, ret);
    }
    ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, pDevModeOutputA,
			      pDevModeInputA, fMode);
    if(pDevModeOutput) {
        DEVMODEcpyAtoW(pDevModeOutput, pDevModeOutputA);
	HeapFree(GetProcessHeap(),0,pDevModeOutputA);
    }
    if(fMode == 0 && ret > 0)
        ret += (CCHDEVICENAME + CCHFORMNAME);
    HeapFree(GetProcessHeap(),0,pDevModeInputA);
    HeapFree(GetProcessHeap(),0,pDeviceNameA);
    return ret;
}

/******************************************************************
 *              OpenPrinterA        [WINSPOOL.@]
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			 LPPRINTER_DEFAULTSA pDefault)
{
    UNICODE_STRING lpPrinterNameW;
    UNICODE_STRING usBuffer;
    PRINTER_DEFAULTSW DefaultW, *pDefaultW = NULL;
    PWSTR pwstrPrinterNameW;
    BOOL ret;

    pwstrPrinterNameW = asciitounicode(&lpPrinterNameW,lpPrinterName);

    if(pDefault) {
        DefaultW.pDatatype = asciitounicode(&usBuffer,pDefault->pDatatype);
	DefaultW.pDevMode = DEVMODEdupAtoW(GetProcessHeap(),
					   pDefault->pDevMode);
	DefaultW.DesiredAccess = pDefault->DesiredAccess;
	pDefaultW = &DefaultW;
    }
    ret = OpenPrinterW(pwstrPrinterNameW, phPrinter, pDefaultW);
    if(pDefault) {
        RtlFreeUnicodeString(&usBuffer);
	HeapFree(GetProcessHeap(), 0, DefaultW.pDevMode);
    }
    RtlFreeUnicodeString(&lpPrinterNameW);
    return ret;
}

/******************************************************************
 *              OpenPrinterW        [WINSPOOL.@]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			 LPPRINTER_DEFAULTSW pDefault)
{
    HKEY hkeyPrinters, hkeyPrinter;

    if (!lpPrinterName) {
       FIXME("(printerName: NULL, pDefault %p Ret: False\n", pDefault);
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    TRACE("(printerName: %s, pDefault %p)\n", debugstr_w(lpPrinterName),
	  pDefault);

    /* Check Printer exists */
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	SetLastError(ERROR_FILE_NOT_FOUND); /* ?? */
	return FALSE;
    }

    if(lpPrinterName[0] == '\0' || /* explicitly exclude "" */
       RegOpenKeyW(hkeyPrinters, lpPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        TRACE("Can't find printer %s in registry\n",
	      debugstr_w(lpPrinterName));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME);
	return FALSE;
    }
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    if(!phPrinter) /* This seems to be what win95 does anyway */
        return TRUE;

    /* Get the unique handle of the printer*/
    *phPrinter = WINSPOOL_GetOpenedPrinterEntry( lpPrinterName );

    if (pDefault != NULL)
        FIXME("Not handling pDefault\n");

    return TRUE;
}

/******************************************************************
 *              AddMonitorA        [WINSPOOL.@]
 *
 */
BOOL WINAPI AddMonitorA(LPSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME("(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              DeletePrinterDriverA        [WINSPOOL.@]
 *
 */
BOOL WINAPI
DeletePrinterDriverA (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
{
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pDriverName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeleteMonitorA        [WINSPOOL.@]
 *
 */
BOOL WINAPI
DeleteMonitorA (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
{
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pMonitorName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeletePortA        [WINSPOOL.@]
 *
 */
BOOL WINAPI
DeletePortA (LPSTR pName, HWND hWnd, LPSTR pPortName)
{
    FIXME("(%s,%p,%s):stub\n",debugstr_a(pName),hWnd,
          debugstr_a(pPortName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    SetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI
SetPrinterW(
  HANDLE  hPrinter,
  DWORD     Level,
  LPBYTE    pPrinter,
  DWORD     Command) {

    FIXME("():stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    WritePrinter  [WINSPOOL.@]
 */
BOOL WINAPI
WritePrinter(
  HANDLE  hPrinter,
  LPVOID  pBuf,
  DWORD   cbBuf,
  LPDWORD pcWritten) {

    FIXME("():stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *          AddFormA  [WINSPOOL.@]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddFormW  [WINSPOOL.@]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJobA  [WINSPOOL.@]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%p,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJobW  [WINSPOOL.@]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME("(%p,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          GetPrintProcessorDirectoryA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrintProcessorDirectoryA(LPSTR server, LPSTR environ,
                                        DWORD level,  LPBYTE Info,
                                        DWORD cbBuf, LPDWORD needed)
{
    FIXME("(%s,%s,%ld,%p,0x%08lx): stub\n", server, environ, level, Info,
          cbBuf);
    return 0;
}

/*****************************************************************************
 *          WINSPOOL_OpenDriverReg [internal]
 *
 * opens the registry for the printer drivers depending on the given input
 * variable pEnvironment
 *
 * RETURNS:
 *    the opened hkey on success
 *    NULL on error
 */
static HKEY WINSPOOL_OpenDriverReg( LPVOID pEnvironment, BOOL unicode)
{   HKEY  retval;
    LPSTR lpKey, p = NULL;

    TRACE("%s\n",
	  (unicode) ? debugstr_w(pEnvironment) : debugstr_a(pEnvironment));

    if(pEnvironment)
        p = (unicode) ? HEAP_strdupWtoA( GetProcessHeap(), 0, pEnvironment) :
                        pEnvironment;
    else {
        OSVERSIONINFOA ver;
        ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

        if(!GetVersionExA( &ver))
            return 0;

        switch (ver.dwPlatformId) {
             case VER_PLATFORM_WIN32s:
                  ERR("win32 style printing used with 16 bits app, try specifying 'win95' Windows version\n");
                  return 0;

             case VER_PLATFORM_WIN32_NT:
                  p = "Windows NT x86";
                  break;
             default:
                  p = "Windows 4.0";
                  break;
        }
        TRACE("set environment to %s\n", p);
    }

    lpKey = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       strlen(p) + strlen(Drivers));
    sprintf( lpKey, Drivers, p);

    TRACE("%s\n", lpKey);

    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, lpKey, &retval) !=
       ERROR_SUCCESS)
       retval = 0;

    if(pEnvironment && unicode)
       HeapFree( GetProcessHeap(), 0, p);
    HeapFree( GetProcessHeap(), 0, lpKey);

    return retval;
}

/*****************************************************************************
 *          AddPrinterW  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2W *pi = (PRINTER_INFO_2W *) pPrinter;
    LPDEVMODEA dmA;
    LPDEVMODEW dmW;
    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    LONG size;

    TRACE("(%s,%ld,%p)\n", debugstr_w(pName), Level, pPrinter);

    if(pName != NULL) {
        ERR("pName = %s - unsupported\n", debugstr_w(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(Level != 2) {
        ERR("Level = %ld, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    if (strlenW(pi->pPrinterName) >= CCHDEVICENAME) {
	ERR("Printername %s must not exceed length of DEVMODE.dmDeviceName !\n",
		debugstr_w(pi->pPrinterName)
	);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    if(!pPrinter) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return 0;
    }
    if(!RegOpenKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter)) {
	if (!RegQueryValueA(hkeyPrinter,"Attributes",NULL,NULL)) {
	    SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return 0;
	}
	RegCloseKey(hkeyPrinter);
    }
    hkeyDrivers = WINSPOOL_OpenDriverReg( NULL, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegOpenKeyW(hkeyDrivers, pi->pDriverName, &hkeyDriver) !=
       ERROR_SUCCESS) {
        WARN("Can't find driver %s\n", debugstr_w(pi->pDriverName));
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(lstrcmpiW(pi->pPrintProcessor, WinPrintW)) {  /* FIXME */
        FIXME("Can't find processor %s\n", debugstr_w(pi->pPrintProcessor));
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey(hkeyPrinters);
	return 0;
    }

    if(RegCreateKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) !=
       ERROR_SUCCESS) {
        FIXME("Can't create printer %s\n", debugstr_w(pi->pPrinterName));
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    RegSetValueExA(hkeyPrinter, "Attributes", 0, REG_DWORD,
		   (LPBYTE)&pi->Attributes, sizeof(DWORD));
    set_reg_szW(hkeyPrinter, DatatypeW, pi->pDatatype);

    /* See if we can load the driver.  We may need the devmode structure anyway
     *
     * FIXME:
     * Note that DocumentPropertiesW will briefly try to open the printer we
     * just create to find a DEVMODEA struct (it will use the WINEPS default
     * one in case it is not there, so we are ok).
     */
    size = DocumentPropertiesW(0, 0, pi->pPrinterName, NULL, NULL, 0);
    if(size < 0) {
        FIXME("DocumentPropertiesW on printer '%s' fails\n", debugstr_w(pi->pPrinterName));
	size = sizeof(DEVMODEW);
    }
    if(pi->pDevMode)
        dmW = pi->pDevMode;
    else {
	dmW = HeapAlloc(GetProcessHeap(), 0, size);
	dmW->dmSize = size;
	if (0>DocumentPropertiesW(0,0,pi->pPrinterName,dmW,NULL,DM_OUT_BUFFER)) {
	    ERR("DocumentPropertiesW on printer '%s' failed!\n", debugstr_w(pi->pPrinterName));
	    SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	    return 0;
	}
	/* set devmode to printer name */
	strcpyW(dmW->dmDeviceName,pi->pPrinterName);
    }

    /* Write DEVMODEA not DEVMODEW into reg.  This is what win9x does
       and we support these drivers.  NT writes DEVMODEW so somehow
       we'll need to distinguish between these when we support NT
       drivers */
    dmA = DEVMODEdupWtoA(GetProcessHeap(), dmW);
    RegSetValueExA(hkeyPrinter, "Default DevMode", 0, REG_BINARY, (LPBYTE)dmA,
		   dmA->dmSize + dmA->dmDriverExtra);
    HeapFree(GetProcessHeap(), 0, dmA);
    if(!pi->pDevMode)
        HeapFree(GetProcessHeap(), 0, dmW);
    set_reg_szW(hkeyPrinter, DescriptionW, pi->pComment);
    set_reg_szW(hkeyPrinter, LocationW, pi->pLocation);
    set_reg_szW(hkeyPrinter, NameW, pi->pPrinterName);
    set_reg_szW(hkeyPrinter, ParametersW, pi->pParameters);

    set_reg_szW(hkeyPrinter, PortW, pi->pPortName);
    set_reg_szW(hkeyPrinter, Print_ProcessorW, pi->pPrintProcessor);
    set_reg_szW(hkeyPrinter, Printer_DriverW, pi->pDriverName);
    RegSetValueExA(hkeyPrinter, "Priority", 0, REG_DWORD,
		   (LPBYTE)&pi->Priority, sizeof(DWORD));
    set_reg_szW(hkeyPrinter, Separator_FileW, pi->pSepFile);
    set_reg_szW(hkeyPrinter, Share_NameW, pi->pShareName);
    RegSetValueExA(hkeyPrinter, "StartTime", 0, REG_DWORD,
		   (LPBYTE)&pi->StartTime, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Status", 0, REG_DWORD,
		   (LPBYTE)&pi->Status, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "UntilTime", 0, REG_DWORD,
		   (LPBYTE)&pi->UntilTime, sizeof(DWORD));

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(!OpenPrinterW(pi->pPrinterName, &retval, NULL)) {
        ERR("OpenPrinter failing\n");
	return 0;
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinterA  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    UNICODE_STRING pNameW;
    PWSTR pwstrNameW;
    PRINTER_INFO_2W *piW;
    PRINTER_INFO_2A *piA = (PRINTER_INFO_2A*)pPrinter;
    HANDLE ret;

    TRACE("(%s,%ld,%p): stub\n", debugstr_a(pName), Level, pPrinter);
    if(Level != 2) {
        ERR("Level = %ld, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    pwstrNameW = asciitounicode(&pNameW,pName);
    piW = PRINTER_INFO_2AtoW(GetProcessHeap(), piA);

    ret = AddPrinterW(pwstrNameW, Level, (LPBYTE)piW);

    FREE_PRINTER_INFO_2W(GetProcessHeap(), piW);
    RtlFreeUnicodeString(&pNameW);
    return ret;
}


/*****************************************************************************
 *          ClosePrinter  [WINSPOOL.@]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    int i = (int)hPrinter;

    TRACE("Handle %p\n", hPrinter);

    if ((i <= 0) || (i > nb_printers)) return FALSE;
    HeapFree( GetProcessHeap(), 0, printer_array[i - 1] );
    printer_array[i - 1] = NULL;
    return TRUE;
}

/*****************************************************************************
 *          DeleteFormA  [WINSPOOL.@]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME("(%p,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteFormW  [WINSPOOL.@]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%p,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter  [WINSPOOL.@]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    LPCWSTR lpNameW = WINSPOOL_GetOpenedPrinter(hPrinter);
    HKEY hkeyPrinters;

    if(!lpNameW) return FALSE;
    if(RegOpenKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't open Printers key\n");
	return 0;
    }

    /* This should use a recursive delete see Q142491 or SHDeleteKey */
    if(RegDeleteKeyW(hkeyPrinters, lpNameW) == ERROR_SUCCESS) {
        SetLastError(ERROR_PRINTER_NOT_FOUND); /* ?? */
	RegCloseKey(hkeyPrinters);
	return 0;
    }

    ClosePrinter(hPrinter);
    return TRUE;
}

/*****************************************************************************
 *          SetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME("(%p,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJobA  [WINSPOOL.@]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%p,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJobW  [WINSPOOL.@]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%p,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          EndDocPrinter  [WINSPOOL.@]
 */
BOOL WINAPI EndDocPrinter(HANDLE hPrinter)
{
    FIXME("(hPrinter=%p): stub\n", hPrinter);
    return FALSE;
}

/*****************************************************************************
 *          EndPagePrinter  [WINSPOOL.@]
 */
BOOL WINAPI EndPagePrinter(HANDLE hPrinter)
{
    FIXME("(hPrinter=%p): stub\n", hPrinter);
    return FALSE;
}

/*****************************************************************************
 *          StartDocPrinterA  [WINSPOOL.@]
 */
DWORD WINAPI StartDocPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    FIXME("(hPrinter=%p, Level=0x%lx, pDocInfo=%p): stub\n", hPrinter, Level, pDocInfo);
    return FALSE;
}

/*****************************************************************************
 *          StartDocPrinterW  [WINSPOOL.@]
 */
DWORD WINAPI StartDocPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    FIXME("(hPrinter=%p, Level=0x%lx, pDocInfo=%p): stub\n", hPrinter, Level, pDocInfo);
    return FALSE;
}

/*****************************************************************************
 *          StartPagePrinter  [WINSPOOL.@]
 */
BOOL WINAPI StartPagePrinter(HANDLE hPrinter)
{
    FIXME("(hPrinter=%p): stub\n", hPrinter);
    return FALSE;
}

/*****************************************************************************
 *          GetFormA  [WINSPOOL.@]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%p,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetFormW  [WINSPOOL.@]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%p,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetFormA  [WINSPOOL.@]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%p,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetFormW  [WINSPOOL.@]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%p,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter  [WINSPOOL.@]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME("(%p,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME("(%p, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%p, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *    WINSPOOL_GetDWORDFromReg
 *
 * Return DWORD associated with ValueName from hkey.
 */
static DWORD WINSPOOL_GetDWORDFromReg(HKEY hkey, LPCSTR ValueName)
{
    DWORD sz = sizeof(DWORD), type, value = 0;
    LONG ret;

    ret = RegQueryValueExA(hkey, ValueName, 0, &type, (LPBYTE)&value, &sz);

    if(ret != ERROR_SUCCESS) {
        WARN("Got ret = %ld on name %s\n", ret, ValueName);
	return 0;
    }
    if(type != REG_DWORD) {
        ERR("Got type %ld\n", type);
	return 0;
    }
    return value;
}

/*****************************************************************************
 *    WINSPOOL_GetStringFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * String is stored either as unicode or ascii.
 * Bit of a hack here to get the ValueName if we want ascii.
 */
static BOOL WINSPOOL_GetStringFromReg(HKEY hkey, LPCWSTR ValueName, LPBYTE ptr,
				      DWORD buflen, DWORD *needed,
				      BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    if(unicode)
        ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    else {
        LPSTR ValueNameA = HEAP_strdupWtoA(GetProcessHeap(),0,ValueName);
        ret = RegQueryValueExA(hkey, ValueNameA, 0, &type, ptr, &sz);
	HeapFree(GetProcessHeap(),0,ValueNameA);
    }
    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        WARN("Got ret = %ld\n", ret);
	*needed = 0;
	return FALSE;
    }
    *needed = sz;
    return TRUE;
}

/*****************************************************************************
 *    WINSPOOL_GetDefaultDevMode
 *
 * Get a default DevMode values for wineps.
 * FIXME - use ppd.
 */

static void WINSPOOL_GetDefaultDevMode(
	LPBYTE ptr,
	DWORD buflen, DWORD *needed,
	BOOL unicode)
{
    DEVMODEA	dm;

	/* fill default DEVMODE - should be read from ppd... */
	ZeroMemory( &dm, sizeof(dm) );
	strcpy(dm.dmDeviceName,"wineps");
	dm.dmSpecVersion = DM_SPECVERSION;
	dm.dmDriverVersion = 1;
	dm.dmSize = sizeof(DEVMODEA);
	dm.dmDriverExtra = 0;
	dm.dmFields =
		DM_ORIENTATION | DM_PAPERSIZE |
		DM_PAPERLENGTH | DM_PAPERWIDTH |
		DM_SCALE |
		DM_COPIES |
		DM_DEFAULTSOURCE | DM_PRINTQUALITY |
		DM_YRESOLUTION | DM_TTOPTION;

	dm.u1.s1.dmOrientation = DMORIENT_PORTRAIT;
	dm.u1.s1.dmPaperSize = DMPAPER_A4;
	dm.u1.s1.dmPaperLength = 2970;
	dm.u1.s1.dmPaperWidth = 2100;

	dm.dmScale = 100;
	dm.dmCopies = 1;
	dm.dmDefaultSource = DMBIN_AUTO;
	dm.dmPrintQuality = DMRES_MEDIUM;
	/* dm.dmColor */
	/* dm.dmDuplex */
	dm.dmYResolution = 300; /* 300dpi */
	dm.dmTTOption = DMTT_BITMAP;
	/* dm.dmCollate */
	/* dm.dmFormName */
	/* dm.dmLogPixels */
	/* dm.dmBitsPerPel */
	/* dm.dmPelsWidth */
	/* dm.dmPelsHeight */
	/* dm.dmDisplayFlags */
	/* dm.dmDisplayFrequency */
	/* dm.dmICMMethod */
	/* dm.dmICMIntent */
	/* dm.dmMediaType */
	/* dm.dmDitherType */
	/* dm.dmReserved1 */
	/* dm.dmReserved2 */
	/* dm.dmPanningWidth */
	/* dm.dmPanningHeight */

    if(unicode) {
	if(buflen >= sizeof(DEVMODEW)) {
	    DEVMODEW *pdmW = DEVMODEdupAtoW(GetProcessHeap(), &dm );
	    memcpy(ptr, pdmW, sizeof(DEVMODEW));
	    HeapFree(GetProcessHeap(),0,pdmW);
	}
	*needed = sizeof(DEVMODEW);
    }
    else
    {
	if(buflen >= sizeof(DEVMODEA)) {
	    memcpy(ptr, &dm, sizeof(DEVMODEA));
	}
	*needed = sizeof(DEVMODEA);
    }
}

/*****************************************************************************
 *    WINSPOOL_GetDevModeFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * DevMode is stored either as unicode or ascii.
 */
static BOOL WINSPOOL_GetDevModeFromReg(HKEY hkey, LPCWSTR ValueName,
				       LPBYTE ptr,
				       DWORD buflen, DWORD *needed,
				       BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    if (ptr && buflen>=sizeof(DEVMODEA)) memset(ptr, 0, sizeof(DEVMODEA));
    ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    if ((ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)) sz = 0;
    if (sz < sizeof(DEVMODEA))
    {
        ERR("corrupted registry for %s ( size %ld)\n",debugstr_w(ValueName),sz);
	return FALSE;
    }
    /* ensures that dmSize is not erratically bogus if registry is invalid */
    if (ptr && ((DEVMODEA*)ptr)->dmSize < sizeof(DEVMODEA))
        ((DEVMODEA*)ptr)->dmSize = sizeof(DEVMODEA);
    if(unicode) {
	sz += (CCHDEVICENAME + CCHFORMNAME);
	if(buflen >= sz) {
	    DEVMODEW *dmW = DEVMODEdupAtoW(GetProcessHeap(), (DEVMODEA*)ptr);
	    memcpy(ptr, dmW, sz);
	    HeapFree(GetProcessHeap(),0,dmW);
	}
    }
    *needed = sz;
    return TRUE;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_2
 *
 * Fills out a PRINTER_INFO_2A|W struct storing the strings in buf.
 * The strings are either stored as unicode or ascii.
 */
static BOOL WINSPOOL_GetPrinter_2(HKEY hkeyPrinter, PRINTER_INFO_2W *pi2,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Share_NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pShareName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Printer_DriverW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDriverName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, LocationW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pLocation = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetDevModeFromReg(hkeyPrinter, Default_DevModeW, ptr, left,
				  &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    else
    {
	MESSAGE( "no DevMode in registry. please setup your printer again.\n"
		 "use the default hard-coded DevMode(wineps/A4/300dpi).\n" );
	WINSPOOL_GetDefaultDevMode(ptr, left, &size, unicode);
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Separator_FileW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
            pi2->pSepFile = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Print_ProcessorW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pPrintProcessor = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DatatypeW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDatatype = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, ParametersW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pParameters = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi2) {
        pi2->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
        pi2->Priority = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Priority");
        pi2->DefaultPriority = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
							"Default Priority");
        pi2->StartTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "StartTime");
        pi2->UntilTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "UntilTime");
    }

    if(!space && pi2) /* zero out pi2 if we can't completely fill buf */
        memset(pi2, 0, sizeof(*pi2));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_4
 *
 * Fills out a PRINTER_INFO_4 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_4(HKEY hkeyPrinter, PRINTER_INFO_4W *pi4,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi4->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi4) {
        pi4->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
    }

    if(!space && pi4) /* zero out pi4 if we can't completely fill buf */
        memset(pi4, 0, sizeof(*pi4));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_5
 *
 * Fills out a PRINTER_INFO_5 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_5(HKEY hkeyPrinter, PRINTER_INFO_5W *pi5,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi5) {
        pi5->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
        pi5->DeviceNotSelectedTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								"dnsTimeout");
        pi5->TransmissionRetryTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								 "txTimeout");
    }

    if(!space && pi5) /* zero out pi5 if we can't completely fill buf */
        memset(pi5, 0, sizeof(*pi5));

    return space;
}

/*****************************************************************************
 *          WINSPOOL_GetPrinter
 *
 *    Implementation of GetPrinterA|W.  Relies on PRINTER_INFO_*W being
 *    essentially the same as PRINTER_INFO_*A. i.e. the structure itself is
 *    just a collection of pointers to strings.
 */
static BOOL WINSPOOL_GetPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
				DWORD cbBuf, LPDWORD pcbNeeded, BOOL unicode)
{
    LPCWSTR name;
    DWORD size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters;
    BOOL ret;

    TRACE("(%p,%ld,%p,%ld,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    if (!(name = WINSPOOL_GetOpenedPrinter(hPrinter))) return FALSE;

    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyPrinters, name, &hkeyPrinter) != ERROR_SUCCESS)
    {
        ERR("Can't find opened printer %s in registry\n", debugstr_w(name));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 2:
      {
        PRINTER_INFO_2W *pi2 = (PRINTER_INFO_2W *)pPrinter;

        size = sizeof(PRINTER_INFO_2W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi2 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_2(hkeyPrinter, pi2, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }

    case 4:
      {
	PRINTER_INFO_4W *pi4 = (PRINTER_INFO_4W *)pPrinter;

        size = sizeof(PRINTER_INFO_4W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi4 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_4(hkeyPrinter, pi4, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }


    case 5:
      {
        PRINTER_INFO_5W *pi5 = (PRINTER_INFO_5W *)pPrinter;

        size = sizeof(PRINTER_INFO_5W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi5 = NULL;
	    cbBuf = 0;
	}

	ret = WINSPOOL_GetPrinter_5(hkeyPrinter, pi5, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }

    default:
        FIXME("Unimplemented level %ld\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyPrinter);
	return FALSE;
    }

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    TRACE("returning %d needed = %ld\n", ret, needed);
    if(pcbNeeded) *pcbNeeded = needed;
    if(!ret)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return ret;
}

/*****************************************************************************
 *          GetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
			DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       TRUE);
}

/*****************************************************************************
 *          GetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       FALSE);
}

/*****************************************************************************
 *          WINSPOOL_EnumPrinters
 *
 *    Implementation of EnumPrintersA|W
 */
static BOOL WINSPOOL_EnumPrinters(DWORD dwType, LPWSTR lpszName,
				  DWORD dwLevel, LPBYTE lpbPrinters,
				  DWORD cbBuf, LPDWORD lpdwNeeded,
				  LPDWORD lpdwReturned, BOOL unicode)

{
    HKEY hkeyPrinters, hkeyPrinter;
    WCHAR PrinterName[255];
    DWORD needed = 0, number = 0;
    DWORD used, i, left;
    PBYTE pi, buf;

    if(lpbPrinters)
        memset(lpbPrinters, 0, cbBuf);
    if(lpdwReturned)
        *lpdwReturned = 0;
    if(lpdwNeeded)
        *lpdwNeeded = 0;

    /* PRINTER_ENUM_DEFAULT is only supported under win9x, we behave like NT */
    if(dwType == PRINTER_ENUM_DEFAULT)
	return TRUE;

    if (dwType & PRINTER_ENUM_CONNECTIONS) {
        FIXME("We dont handle PRINTER_ENUM_CONNECTIONS\n");
	dwType &= ~PRINTER_ENUM_CONNECTIONS; /* we dont handle that */
	dwType |= PRINTER_ENUM_LOCAL;
    }

    if (!((dwType & PRINTER_ENUM_LOCAL) || (dwType & PRINTER_ENUM_NAME))) {
        FIXME("dwType = %08lx\n", dwType);
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }

    if(RegQueryInfoKeyA(hkeyPrinters, NULL, NULL, NULL, &number, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyPrinters);
	ERR("Can't query Printers key\n");
	return FALSE;
    }
    TRACE("Found %ld printers\n", number);

    switch(dwLevel) {
    case 1:
        RegCloseKey(hkeyPrinters);
	if (lpdwReturned)
	    *lpdwReturned = number;
	return TRUE;

    case 2:
        used = number * sizeof(PRINTER_INFO_2W);
	break;
    case 4:
        used = number * sizeof(PRINTER_INFO_4W);
	break;
    case 5:
        used = number * sizeof(PRINTER_INFO_5W);
	break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	return FALSE;
    }
    pi = (used <= cbBuf) ? lpbPrinters : NULL;

    for(i = 0; i < number; i++) {
        if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)) !=
	   ERROR_SUCCESS) {
	    ERR("Can't enum key number %ld\n", i);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	TRACE("Printer %ld is %s\n", i, debugstr_w(PrinterName));
	if(RegOpenKeyW(hkeyPrinters, PrinterName, &hkeyPrinter) !=
	   ERROR_SUCCESS) {
	    ERR("Can't open key %s\n", debugstr_w(PrinterName));
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}

	if(cbBuf > used) {
	    buf = lpbPrinters + used;
	    left = cbBuf - used;
	} else {
	    buf = NULL;
	    left = 0;
	}

	switch(dwLevel) {
	case 2:
	    WINSPOOL_GetPrinter_2(hkeyPrinter, (PRINTER_INFO_2W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_2W);
	    break;
	case 4:
	    WINSPOOL_GetPrinter_4(hkeyPrinter, (PRINTER_INFO_4W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_4W);
	    break;
	case 5:
	    WINSPOOL_GetPrinter_5(hkeyPrinter, (PRINTER_INFO_5W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_5W);
	    break;
	default:
	    ERR("Shouldn't be here!\n");
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	RegCloseKey(hkeyPrinter);
    }
    RegCloseKey(hkeyPrinters);

    if(lpdwNeeded)
        *lpdwNeeded = used;

    if(used > cbBuf) {
        if(lpbPrinters)
	    memset(lpbPrinters, 0, cbBuf);
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    if(lpdwReturned)
        *lpdwReturned = number;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


/******************************************************************
 *              EnumPrintersW        [WINSPOOL.@]
 *
 *    Enumerates the available printers, print servers and print
 *    providers, depending on the specified flags, name and level.
 *
 * RETURNS:
 *
 *    If level is set to 1:
 *      Not implemented yet!
 *      Returns TRUE with an empty list.
 *
 *    If level is set to 2:
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Returns an array of PRINTER_INFO_2 data structures in the
 *      lpbPrinters buffer. Note that according to MSDN also an
 *      OpenPrinter should be performed on every remote printer.
 *
 *    If level is set to 4 (officially WinNT only):
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_4 data structures in the
 *      lpbPrinters buffer.
 *
 *    If level is set to 5 (officially WinNT4/Win9x only):
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_5 data structures in the
 *      lpbPrinters buffer.
 *
 *    If level set to 3 or 6+:
 *	    returns zero (failure!)
 *
 *    Returns nonzero (TRUE) on success, or zero on failure, use GetLastError
 *    for information.
 *
 * BUGS:
 *    - Only PRINTER_ENUM_LOCAL and PRINTER_ENUM_NAME are implemented.
 *    - Only levels 2, 4 and 5 are implemented at the moment.
 *    - 16-bit printer drivers are not enumerated.
 *    - Returned amount of bytes used/needed does not match the real Windoze
 *      implementation (as in this implementation, all strings are part
 *      of the buffer, whereas Win32 keeps them somewhere else)
 *    - At level 2, EnumPrinters should also call OpenPrinter for remote printers.
 *
 * NOTE:
 *    - In a regular Wine installation, no registry settings for printers
 *      exist, which makes this function return an empty list.
 */
BOOL  WINAPI EnumPrintersW(
		DWORD dwType,        /* [in] Types of print objects to enumerate */
                LPWSTR lpszName,     /* [in] name of objects to enumerate */
	        DWORD dwLevel,       /* [in] type of printer info structure */
                LPBYTE lpbPrinters,  /* [out] buffer which receives info */
		DWORD cbBuf,         /* [in] max size of buffer in bytes */
		LPDWORD lpdwNeeded,  /* [out] pointer to var: # bytes used/needed */
		LPDWORD lpdwReturned /* [out] number of entries returned */
		)
{
    return WINSPOOL_EnumPrinters(dwType, lpszName, dwLevel, lpbPrinters, cbBuf,
				 lpdwNeeded, lpdwReturned, TRUE);
}

/******************************************************************
 *              EnumPrintersA        [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintersA(DWORD dwType, LPSTR lpszName,
			  DWORD dwLevel, LPBYTE lpbPrinters,
			  DWORD cbBuf, LPDWORD lpdwNeeded,
			  LPDWORD lpdwReturned)
{
    BOOL ret;
    UNICODE_STRING lpszNameW;
    PWSTR pwstrNameW;
    
    pwstrNameW = asciitounicode(&lpszNameW,lpszName);
    ret = WINSPOOL_EnumPrinters(dwType, pwstrNameW, dwLevel, lpbPrinters, cbBuf,
				lpdwNeeded, lpdwReturned, FALSE);
    RtlFreeUnicodeString(&lpszNameW);
    return ret;
}

/*****************************************************************************
 *          WINSPOOL_GetDriverInfoFromReg [internal]
 *
 *    Enters the information from the registry into the DRIVER_INFO struct
 *
 * RETURNS
 *    zero if the printer driver does not exist in the registry
 *    (only if Level > 1) otherwise nonzero
 */
static BOOL WINSPOOL_GetDriverInfoFromReg(
                            HKEY    hkeyDrivers,
                            LPWSTR  DriverName,
                            LPWSTR  pEnvironment,
                            DWORD   Level,
                            LPBYTE  ptr,            /* DRIVER_INFO */
                            LPBYTE  pDriverStrings, /* strings buffer */
                            DWORD   cbBuf,          /* size of string buffer */
                            LPDWORD pcbNeeded,      /* space needed for str. */
                            BOOL    unicode)        /* type of strings */
{   DWORD  dw, size, tmp, type;
    HKEY   hkeyDriver;
    LPBYTE strPtr = pDriverStrings;

    TRACE("%s,%s,%ld,%p,%p,%ld,%d\n",
          debugstr_w(DriverName), debugstr_w(pEnvironment),
          Level, ptr, pDriverStrings, cbBuf, unicode);

    if(unicode) {
        *pcbNeeded = (lstrlenW(DriverName) + 1) * sizeof(WCHAR);
            if (*pcbNeeded <= cbBuf)
               strcpyW((LPWSTR)strPtr, DriverName);
    } else {
        *pcbNeeded = WideCharToMultiByte(CP_ACP, 0, DriverName, -1, NULL, 0,
                                          NULL, NULL);
        if(*pcbNeeded <= cbBuf)
            WideCharToMultiByte(CP_ACP, 0, DriverName, -1, strPtr, *pcbNeeded,
                                NULL, NULL);
    }
    if(Level == 1) {
       if(ptr)
          ((PDRIVER_INFO_1W) ptr)->pName = (LPWSTR) strPtr;
       return TRUE;
    } else {
       if(ptr)
          ((PDRIVER_INFO_3W) ptr)->pName = (LPWSTR) strPtr;
       strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(!DriverName[0] || RegOpenKeyW(hkeyDrivers, DriverName, &hkeyDriver) != ERROR_SUCCESS) {
        ERR("Can't find driver '%s' in registry\n", debugstr_w(DriverName));
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
        return FALSE;
    }

    size = sizeof(dw);
    if(RegQueryValueExA(hkeyDriver, "Version", 0, &type, (PBYTE)&dw, &size) !=
        ERROR_SUCCESS)
         WARN("Can't get Version\n");
    else if(ptr)
         ((PDRIVER_INFO_3A) ptr)->cVersion = dw;

    if(!pEnvironment)
        pEnvironment = DefaultEnvironmentW;
    if(unicode)
        size = (lstrlenW(pEnvironment) + 1) * sizeof(WCHAR);
    else
        size = WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1, NULL, 0,
			           NULL, NULL);
    *pcbNeeded += size;
    if(*pcbNeeded <= cbBuf) {
        if(unicode)
            strcpyW((LPWSTR)strPtr, pEnvironment);
        else
            WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1, strPtr, size,
                                NULL, NULL);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pEnvironment = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, DriverW, strPtr, 0, &size,
			         unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, DriverW, strPtr, size, &tmp,
                                      unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDriverPath = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Data_FileW, strPtr, 0, &size,
			         unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Data_FileW, strPtr, size,
                                      &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDataFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Configuration_FileW, strPtr,
                                 0, &size, unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Configuration_FileW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pConfigFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(Level == 2 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Help_FileW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Help_FileW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pHelpFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Dependent_FilesW, strPtr, 0,
			     &size, unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Dependent_FilesW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDependentFiles = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pMonitorName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDefaultDataType = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
    RegCloseKey(hkeyDriver);
    return TRUE;
}

/*****************************************************************************
 *          WINSPOOL_GetPrinterDriver
 */
static BOOL WINSPOOL_GetPrinterDriver(HANDLE hPrinter, LPWSTR pEnvironment,
				      DWORD Level, LPBYTE pDriverInfo,
				      DWORD cbBuf, LPDWORD pcbNeeded,
				      BOOL unicode)
{
    LPCWSTR name;
    WCHAR DriverName[100];
    DWORD ret, type, size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDrivers;

    TRACE("(%p,%s,%ld,%p,%ld,%p)\n",hPrinter,debugstr_w(pEnvironment),
	  Level,pDriverInfo,cbBuf, pcbNeeded);

    ZeroMemory(pDriverInfo, cbBuf);

    if (!(name = WINSPOOL_GetOpenedPrinter(hPrinter))) return FALSE;

    if(Level < 1 || Level > 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyPrinters, name, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer %s in registry\n", debugstr_w(name));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }
    size = sizeof(DriverName);
    DriverName[0] = 0;
    ret = RegQueryValueExW(hkeyPrinter, Printer_DriverW, 0, &type,
			   (LPBYTE)DriverName, &size);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer %s\n", debugstr_w(name));
	return FALSE;
    }

    hkeyDrivers = WINSPOOL_OpenDriverReg( pEnvironment, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    switch(Level) {
    case 1:
        size = sizeof(DRIVER_INFO_1W);
	break;
    case 2:
        size = sizeof(DRIVER_INFO_2W);
	break;
    case 3:
        size = sizeof(DRIVER_INFO_3W);
	break;
    default:
        ERR("Invalid level\n");
	return FALSE;
    }

    if(size <= cbBuf)
        ptr = pDriverInfo + size;

    if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverName,
                         pEnvironment, Level, pDriverInfo,
                         (cbBuf < size) ? NULL : ptr,
                         (cbBuf < size) ? 0 : cbBuf - size,
                         &needed, unicode)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
    }

    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = size + needed;
    TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
    if(cbBuf >= needed) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriverA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
			      DWORD Level, LPBYTE pDriverInfo,
			      DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL ret;
    UNICODE_STRING pEnvW;
    PWSTR pwstrEnvW;
    
    pwstrEnvW = asciitounicode(&pEnvW, pEnvironment);
    ret = WINSPOOL_GetPrinterDriver(hPrinter, pwstrEnvW, Level, pDriverInfo,
				    cbBuf, pcbNeeded, FALSE);
    RtlFreeUnicodeString(&pEnvW);
    return ret;
}
/*****************************************************************************
 *          GetPrinterDriverW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo,
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinterDriver(hPrinter, pEnvironment, Level,
				     pDriverInfo, cbBuf, pcbNeeded, TRUE);
}

/*****************************************************************************
 *       GetPrinterDriverDirectoryW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR pName, LPWSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(pName), 
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);
    if(pName != NULL) {
        FIXME("pName = `%s' - unsupported\n", debugstr_w(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(pEnvironment != NULL) {
        FIXME("pEnvironment = `%s' - unsupported\n", debugstr_w(pEnvironment));
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(Level != 1)  /* win95 ignores this so we just carry on */
        WARN("Level = %ld - assuming 1\n", Level);

    /* FIXME should read from registry */
    needed = GetSystemDirectoryW( (LPWSTR)pDriverDirectory, cbBuf/sizeof(WCHAR));
    /* GetSystemDirectoryW returns number of TCHAR without '\0' 
     * adjust this now
     */
    needed++;
    needed*=sizeof(WCHAR);

    if(pcbNeeded)
        *pcbNeeded = needed;
    TRACE("required <%08lx>\n", *pcbNeeded);
    if(needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    return TRUE;
}


/*****************************************************************************
 *       GetPrinterDriverDirectoryA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverDirectoryA(LPSTR pName, LPSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    UNICODE_STRING nameW, environmentW;
    BOOL ret;
    DWORD pcbNeededW;
    INT len = cbBuf * sizeof(WCHAR)/sizeof(CHAR);
    WCHAR *driverDirectoryW = NULL;

    if (len) driverDirectoryW = HeapAlloc( GetProcessHeap(), 0, len );

    if(pName) RtlCreateUnicodeStringFromAsciiz(&nameW, pName);
    else nameW.Buffer = NULL;
    if(pEnvironment) RtlCreateUnicodeStringFromAsciiz(&environmentW, pEnvironment);
    else environmentW.Buffer = NULL;

    ret = GetPrinterDriverDirectoryW( nameW.Buffer, environmentW.Buffer, Level,
				      (LPBYTE)driverDirectoryW, len, &pcbNeededW );
    if (ret) {
        DWORD needed;
        needed = 1 + WideCharToMultiByte( CP_ACP, 0, driverDirectoryW, -1, 
                                   pDriverDirectory, cbBuf, NULL, NULL);
        if(pcbNeeded)
            *pcbNeeded = needed;
        ret = (needed <= cbBuf) ? TRUE : FALSE;
    } else 
        if(pcbNeeded) *pcbNeeded = pcbNeededW * sizeof(CHAR)/sizeof(WCHAR);

    TRACE("provided<%ld> required <%ld>\n", cbBuf, *pcbNeeded);

    if(driverDirectoryW)
        HeapFree( GetProcessHeap(), 0, driverDirectoryW );
    RtlFreeUnicodeString(&environmentW);
    RtlFreeUnicodeString(&nameW);

    return ret;
}

/*****************************************************************************
 *          AddPrinterDriverA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    DRIVER_INFO_3A di3;
    HKEY hkeyDrivers, hkeyName;

    TRACE("(%s,%ld,%p)\n",debugstr_a(pName),level,pDriverInfo);

    if(level != 2 && level != 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(pName != NULL) {
        FIXME("pName= %s - unsupported\n", debugstr_a(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!pDriverInfo) {
        WARN("pDriverInfo == NULL\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if(level == 3)
        di3 = *(DRIVER_INFO_3A *)pDriverInfo;
    else {
        memset(&di3, 0, sizeof(di3));
        *(DRIVER_INFO_2A *)&di3 = *(DRIVER_INFO_2A *)pDriverInfo;
    }

    if(!di3.pName || !di3.pDriverPath || !di3.pConfigFile ||
       !di3.pDataFile) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!di3.pDefaultDataType) di3.pDefaultDataType = "";
    if(!di3.pDependentFiles) di3.pDependentFiles = "\0";
    if(!di3.pHelpFile) di3.pHelpFile = "";
    if(!di3.pMonitorName) di3.pMonitorName = "";

    hkeyDrivers = WINSPOOL_OpenDriverReg(di3.pEnvironment, FALSE);

    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    if(level == 2) { /* apparently can't overwrite with level2 */
        if(RegOpenKeyA(hkeyDrivers, di3.pName, &hkeyName) == ERROR_SUCCESS) {
	    RegCloseKey(hkeyName);
	    RegCloseKey(hkeyDrivers);
	    WARN("Trying to create existing printer driver %s\n", debugstr_a(di3.pName));
	    SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
	    return FALSE;
	}
    }
    if(RegCreateKeyA(hkeyDrivers, di3.pName, &hkeyName) != ERROR_SUCCESS) {
	RegCloseKey(hkeyDrivers);
	ERR("Can't create Name key\n");
	return FALSE;
    }
    RegSetValueExA(hkeyName, "Configuration File", 0, REG_SZ, di3.pConfigFile,
		   0);
    RegSetValueExA(hkeyName, "Data File", 0, REG_SZ, di3.pDataFile, 0);
    RegSetValueExA(hkeyName, "Driver", 0, REG_SZ, di3.pDriverPath, 0);
    RegSetValueExA(hkeyName, "Version", 0, REG_DWORD, (LPSTR)&di3.cVersion,
		   sizeof(DWORD));
    RegSetValueExA(hkeyName, "Datatype", 0, REG_SZ, di3.pDefaultDataType, 0);
    RegSetValueExA(hkeyName, "Dependent Files", 0, REG_MULTI_SZ,
		   di3.pDependentFiles, 0);
    RegSetValueExA(hkeyName, "Help File", 0, REG_SZ, di3.pHelpFile, 0);
    RegSetValueExA(hkeyName, "Monitor", 0, REG_SZ, di3.pMonitorName, 0);
    RegCloseKey(hkeyName);
    RegCloseKey(hkeyDrivers);

    return TRUE;
}
/*****************************************************************************
 *          AddPrinterDriverW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level,
				   LPBYTE pDriverInfo)
{
    FIXME("(%s,%ld,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}


/*****************************************************************************
 *          PrinterProperties  [WINSPOOL.@]
 *
 *     Displays a dialog to set the properties of the printer.
 *
 * RETURNS
 *     nonzero on success or zero on failure
 *
 * BUGS
 *	   implemented as stub only
 */
BOOL WINAPI PrinterProperties(HWND hWnd,      /* [in] handle to parent window */
                              HANDLE hPrinter /* [in] handle to printer object */
){
    FIXME("(%p,%p): stub\n", hWnd, hPrinter);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *          EnumJobsA [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("(%p,first=%ld,no=%ld,level=%ld,job=%p,cb=%ld,%p,%p), stub!\n",
	hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned
    );
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return FALSE;
}


/*****************************************************************************
 *          EnumJobsW [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("(%p,first=%ld,no=%ld,level=%ld,job=%p,cb=%ld,%p,%p), stub!\n",
	hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned
    );
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return FALSE;
}

/*****************************************************************************
 *          WINSPOOL_EnumPrinterDrivers [internal]
 *
 *    Delivers information about all printer drivers installed on the
 *    localhost or a given server
 *
 * RETURNS
 *    nonzero on success or zero on failure. If the buffer for the returned
 *    information is too small the function will return an error
 *
 * BUGS
 *    - only implemented for localhost, foreign hosts will return an error
 */
static BOOL WINSPOOL_EnumPrinterDrivers(LPWSTR pName, LPWSTR pEnvironment,
                                        DWORD Level, LPBYTE pDriverInfo,
                                        DWORD cbBuf, LPDWORD pcbNeeded,
                                        LPDWORD pcReturned, BOOL unicode)

{   HKEY  hkeyDrivers;
    DWORD i, needed, number = 0, size = 0;
    WCHAR DriverNameW[255];
    PBYTE ptr;

    TRACE("%s,%s,%ld,%p,%ld,%d\n",
          debugstr_w(pName), debugstr_w(pEnvironment),
          Level, pDriverInfo, cbBuf, unicode);

    /* check for local drivers */
    if(pName) {
        ERR("remote drivers unsupported! Current remote host is %s\n",
             debugstr_w(pName));
        return FALSE;
    }

    /* check input parameter */
    if((Level < 1) || (Level > 3)) {
        ERR("unsupported level %ld \n", Level);
        return FALSE;
    }

    /* initialize return values */
    if(pDriverInfo)
        memset( pDriverInfo, 0, cbBuf);
    *pcbNeeded  = 0;
    *pcReturned = 0;

    hkeyDrivers = WINSPOOL_OpenDriverReg(pEnvironment, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't open Drivers key\n");
        return FALSE;
    }

    if(RegQueryInfoKeyA(hkeyDrivers, NULL, NULL, NULL, &number, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyDrivers);
        ERR("Can't query Drivers key\n");
        return FALSE;
    }
    TRACE("Found %ld Drivers\n", number);

    /* get size of single struct
     * unicode and ascii structure have the same size
     */
    switch (Level) {
        case 1:
            size = sizeof(DRIVER_INFO_1A);
            break;
        case 2:
            size = sizeof(DRIVER_INFO_2A);
            break;
        case 3:
            size = sizeof(DRIVER_INFO_3A);
            break;
    }

    /* calculate required buffer size */
    *pcbNeeded = size * number;

    for( i = 0,  ptr = (pDriverInfo && (cbBuf >= size)) ? pDriverInfo : NULL ;
         i < number;
         i++, ptr = (ptr && (cbBuf >= size * i)) ? ptr + size : NULL) {
        if(RegEnumKeyW(hkeyDrivers, i, DriverNameW, sizeof(DriverNameW))
                       != ERROR_SUCCESS) {
            ERR("Can't enum key number %ld\n", i);
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }
        if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverNameW,
                         pEnvironment, Level, ptr,
                         (cbBuf < *pcbNeeded) ? NULL : pDriverInfo + *pcbNeeded,
                         (cbBuf < *pcbNeeded) ? 0 : cbBuf - *pcbNeeded,
                         &needed, unicode)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }
	(*pcbNeeded) += needed;
    }

    RegCloseKey(hkeyDrivers);

    if(cbBuf < *pcbNeeded){
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************
 *          EnumPrinterDriversW  [WINSPOOL.@]
 *
 *    see function EnumPrinterDrivers for RETURNS, BUGS
 */
BOOL WINAPI EnumPrinterDriversW(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
                                LPBYTE pDriverInfo, DWORD cbBuf,
                                LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return WINSPOOL_EnumPrinterDrivers(pName, pEnvironment, Level, pDriverInfo,
                                       cbBuf, pcbNeeded, pcReturned, TRUE);
}

/*****************************************************************************
 *          EnumPrinterDriversA  [WINSPOOL.@]
 *
 *    see function EnumPrinterDrivers for RETURNS, BUGS
 */
BOOL WINAPI EnumPrinterDriversA(LPSTR pName, LPSTR pEnvironment, DWORD Level,
                                LPBYTE pDriverInfo, DWORD cbBuf,
                                LPDWORD pcbNeeded, LPDWORD pcReturned)
{   BOOL ret;
    UNICODE_STRING pNameW, pEnvironmentW;
    PWSTR pwstrNameW, pwstrEnvironmentW;

    pwstrNameW = asciitounicode(&pNameW, pName);
    pwstrEnvironmentW = asciitounicode(&pEnvironmentW, pEnvironment);

    ret = WINSPOOL_EnumPrinterDrivers(pwstrNameW, pwstrEnvironmentW,
                                      Level, pDriverInfo, cbBuf, pcbNeeded,
                                      pcReturned, FALSE);
    RtlFreeUnicodeString(&pNameW);
    RtlFreeUnicodeString(&pEnvironmentW);

    return ret;
}

static CHAR PortMonitor[] = "Wine Port Monitor";
static CHAR PortDescription[] = "Wine Port";

/******************************************************************************
 *		EnumPortsA   (WINSPOOL.@)
 */
BOOL WINAPI EnumPortsA(LPSTR name,DWORD level,LPBYTE buffer,DWORD bufsize,
                       LPDWORD bufneeded,LPDWORD bufreturned)
{
    CHAR portname[10];
    DWORD info_size, ofs, i, printer_count, serial_count, count, n, r;
    const LPCSTR szSerialPortKey = "Software\\Wine\\Wine\\Config\\serialports";
    const LPCSTR szPrinterPortKey = "Software\\Wine\\Wine\\Config\\spooler";
    HKEY hkey_serial, hkey_printer;

    TRACE("(%s,%ld,%p,%ld,%p,%p)\n",
          name,level,buffer,bufsize,bufneeded,bufreturned);

    switch( level )
    {
    case 1:
        info_size = sizeof (PORT_INFO_1A);
        break;
    case 2:
        info_size = sizeof (PORT_INFO_2A);
        break;
    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }
    
    /* see how many exist */

    hkey_serial = 0;
    hkey_printer = 0;
    serial_count = 0;
    printer_count = 0;
    r = RegOpenKeyA( HKEY_LOCAL_MACHINE, szSerialPortKey, &hkey_serial);
    if (r == ERROR_SUCCESS)
    {
        RegQueryInfoKeyA ( hkey_serial, NULL, NULL, NULL, NULL, NULL, NULL,
	    &serial_count, NULL, NULL, NULL, NULL);
    }

    r = RegOpenKeyA( HKEY_LOCAL_MACHINE, szPrinterPortKey, &hkey_printer);
    if ( r == ERROR_SUCCESS )
    {
        RegQueryInfoKeyA( hkey_printer, NULL, NULL, NULL, NULL, NULL, NULL,
	    &printer_count, NULL, NULL, NULL, NULL);
    }
    count = serial_count + printer_count;

    /* then fill in the structure info structure once
       we know the offset to the first string */

    memset( buffer, 0, bufsize );
    n = 0;
    ofs = info_size*count; 
    for ( i=0; i<count; i++)
    {
        DWORD vallen = sizeof(portname) - 1;

        /* get the serial port values, then the printer values */
        if ( i < serial_count )
            r = RegEnumValueA( hkey_serial, i, 
                     portname, &vallen, NULL, NULL, NULL, 0 );
        else
            r = RegEnumValueA( hkey_printer, i-serial_count, 
                     portname, &vallen, NULL, NULL, NULL, 0 );

        if ( r )
            continue;

        /* add a colon if necessary, and make it upper case */
        CharUpperBuffA(portname,vallen);
        if (strcasecmp(portname,"nul")!=0)
            if (vallen && (portname[vallen-1] != ':') )
                lstrcatA(portname,":");

        /* add the port info structure if we can fit it */
        if ( info_size*(n+1) < bufsize )
        {
            if ( level == 1)
            {
                PORT_INFO_1A *info = (PORT_INFO_1A*) &buffer[info_size*n];
                info->pName = (LPSTR) &buffer[ofs];
            }
            else if ( level == 2)
            {
                PORT_INFO_2A *info = (PORT_INFO_2A*) &buffer[info_size*n];
                info->pPortName = (LPSTR) &buffer[ofs];
                /* FIXME: fill in more stuff here */
                info->pMonitorName = PortMonitor;
                info->pDescription = PortDescription;
                info->fPortType = PORT_TYPE_WRITE|PORT_TYPE_READ;
            }

            /* add the name of the port if we can fit it */
            if ( ofs < bufsize )
                lstrcpynA(&buffer[ofs],portname,bufsize - ofs);
        }

        ofs += lstrlenA(portname)+1;
        n++;
    }

    RegCloseKey(hkey_serial);
    RegCloseKey(hkey_printer);

    if(bufneeded)
        *bufneeded = ofs;

    if(bufreturned)
        *bufreturned = count;

    return TRUE;
}

/******************************************************************************
 *		GetDefaultPrinterA   (WINSPOOL.@)
 *
 * Based on PRINTDLG_GetDefaultPrinterName in dlls/commdlg/printdlg.c
 */
BOOL WINAPI GetDefaultPrinterA(LPSTR name, LPDWORD namesize)
{
   char *ptr;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   if (!GetProfileStringA ("windows", "device", "", name, *namesize))
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   if ((ptr = strchr (name, ',')) == NULL)
   {
      SetLastError (ERROR_FILE_NOT_FOUND);
      return FALSE;
   }

   *ptr = '\0';
   *namesize = strlen (name) + 1;
   return TRUE;
}


/******************************************************************************
 *		GetDefaultPrinterW   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterW(LPWSTR name, LPDWORD namesize)
{
   char *buf;
   BOOL  ret;

   if (*namesize < 1)
   {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return FALSE;
   }

   buf = HeapAlloc (GetProcessHeap (), 0, *namesize);
   ret = GetDefaultPrinterA (buf, namesize);
   if (ret)
   {
       DWORD len = MultiByteToWideChar (CP_ACP, 0, buf, -1, name, *namesize);
       if (!len)
       {
           SetLastError (ERROR_INSUFFICIENT_BUFFER);
           ret = FALSE;
       }
       else *namesize = len;
   }

   HeapFree (GetProcessHeap (), 0, buf);
   return ret;
}


/******************************************************************************
 *		SetPrinterDataExA   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataExA(HANDLE hPrinter, LPSTR pKeyName,
			       LPSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %08lx, %p, %08lx)\n", hPrinter, debugstr_a(pKeyName),
	  debugstr_a(pValueName), Type, pData, cbData);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegCreateKeyA(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        ERR("Can't create subkey %s\n", debugstr_a(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    ret = RegSetValueExA(hkeySubkey, pValueName, 0, Type, pData, cbData);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		SetPrinterDataExW   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataExW(HANDLE hPrinter, LPWSTR pKeyName,
			       LPWSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %08lx, %p, %08lx)\n", hPrinter, debugstr_w(pKeyName),
	  debugstr_w(pValueName), Type, pData, cbData);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegCreateKeyW(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        ERR("Can't create subkey %s\n", debugstr_w(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    ret = RegSetValueExW(hkeySubkey, pValueName, 0, Type, pData, cbData);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		SetPrinterDataA   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    return SetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, Type,
			     pData, cbData);
}

/******************************************************************************
 *		SetPrinterDataW   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, DWORD Type,
			     LPBYTE pData, DWORD cbData)
{
    return SetPrinterDataExW(hPrinter, PrinterDriverDataW, pValueName, Type,
			     pData, cbData);
}

/******************************************************************************
 *		GetPrinterDataExA   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExA(HANDLE hPrinter, LPSTR pKeyName,
			       LPSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %p, %p, %08lx, %p)\n", hPrinter,
	  debugstr_a(pKeyName), debugstr_a(pValueName), pType, pData, nSize,
	  pcbNeeded);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegOpenKeyA(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        WARN("Can't open subkey %s\n", debugstr_a(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExA(hkeySubkey, pValueName, 0, pType, pData, pcbNeeded);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		GetPrinterDataExW   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExW(HANDLE hPrinter, LPWSTR pKeyName,
			       LPWSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %p, %p, %08lx, %p)\n", hPrinter,
	  debugstr_w(pKeyName), debugstr_w(pValueName), pType, pData, nSize,
	  pcbNeeded);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegOpenKeyW(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        WARN("Can't open subkey %s\n", debugstr_w(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExW(hkeySubkey, pValueName, 0, pType, pData, pcbNeeded);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		GetPrinterDataA   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, LPDWORD pType,
			     LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, pType,
			     pData, nSize, pcbNeeded);
}

/******************************************************************************
 *		GetPrinterDataW   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, LPDWORD pType,
			     LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExW(hPrinter, PrinterDriverDataW, pValueName, pType,
			     pData, nSize, pcbNeeded);
}

/*******************************************************************************
 *		EnumPrinterDataExW	[WINSPOOL.@]
 */
DWORD WINAPI EnumPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName,
				LPBYTE pEnumValues, DWORD cbEnumValues,
				LPDWORD pcbEnumValues, LPDWORD pnEnumValues)
{
    HKEY		    hkPrinter, hkSubKey;
    DWORD		    r, ret, dwIndex, cValues, cbMaxValueNameLen,
			    cbValueNameLen, cbMaxValueLen, cbValueLen,
			    cbBufSize, dwType;
    LPWSTR		    lpValueName;
    HANDLE		    hHeap;
    PBYTE		    lpValue;
    PPRINTER_ENUM_VALUESW   ppev;

    TRACE ("%p %s\n", hPrinter, debugstr_w (pKeyName));

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    ret = WINSPOOL_GetOpenedPrinterRegKey (hPrinter, &hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	TRACE ("WINSPOOL_GetOpenedPrinterRegKey (%p) returned %li\n",
		hPrinter, ret);
	return ret;
    }

    ret = RegOpenKeyExW (hkPrinter, pKeyName, 0, KEY_READ, &hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkPrinter);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("RegOpenKeyExW (%p, %s) returned %li\n", hPrinter,
		debugstr_w (pKeyName), ret);
	return ret;
    }

    ret = RegCloseKey (hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %li\n", ret);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ret;
    }

    ret = RegQueryInfoKeyW (hkSubKey, NULL, NULL, NULL, NULL, NULL, NULL,
	    &cValues, &cbMaxValueNameLen, &cbMaxValueLen, NULL, NULL);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("RegQueryInfoKeyW (%p) returned %li\n", hkSubKey, ret);
	return ret;
    }

    TRACE ("RegQueryInfoKeyW returned cValues = %li, cbMaxValueNameLen = %li, "
	    "cbMaxValueLen = %li\n", cValues, cbMaxValueNameLen, cbMaxValueLen);

    if (cValues == 0)			/* empty key */
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	*pcbEnumValues = *pnEnumValues = 0;
	return ERROR_SUCCESS;
    }

    ++cbMaxValueNameLen;			/* allow for trailing '\0' */

    hHeap = GetProcessHeap ();
    if (hHeap == NULL)
    {
	ERR ("GetProcessHeap failed\n");
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ERROR_OUTOFMEMORY;
    }

    lpValueName = HeapAlloc (hHeap, 0, cbMaxValueNameLen * sizeof (WCHAR));
    if (lpValueName == NULL)
    {
	ERR ("Failed to allocate %li bytes from process heap\n",
		cbMaxValueNameLen * sizeof (WCHAR));
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ERROR_OUTOFMEMORY;
    }

    lpValue = HeapAlloc (hHeap, 0, cbMaxValueLen);
    if (lpValue == NULL)
    {
	ERR ("Failed to allocate %li bytes from process heap\n", cbMaxValueLen);
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ERROR_OUTOFMEMORY;
    }

    TRACE ("pass 1: calculating buffer required for all names and values\n");

    cbBufSize = cValues * sizeof (PRINTER_ENUM_VALUESW);

    TRACE ("%li bytes required for %li headers\n", cbBufSize, cValues);

    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
	cbValueNameLen = cbMaxValueNameLen; cbValueLen = cbMaxValueLen;
	ret = RegEnumValueW (hkSubKey, dwIndex, lpValueName, &cbValueNameLen,
		NULL, NULL, lpValue, &cbValueLen);
	if (ret != ERROR_SUCCESS)
	{
	    if (HeapFree (hHeap, 0, lpValue) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    if (HeapFree (hHeap, 0, lpValueName) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %li\n", r);
	    TRACE ("RegEnumValueW (%li) returned %li\n", dwIndex, ret);
	    return ret;
	}

	TRACE ("%s [%li]: name needs %li bytes, data needs %li bytes\n",
		debugstr_w (lpValueName), dwIndex,
		(cbValueNameLen + 1) * sizeof (WCHAR), cbValueLen);

	cbBufSize += (cbValueNameLen + 1) * sizeof (WCHAR);
	cbBufSize += cbValueLen;
    }

    TRACE ("%li bytes required for all %li values\n", cbBufSize, cValues);

    *pcbEnumValues = cbBufSize;
    *pnEnumValues = cValues;

    if (cbEnumValues < cbBufSize)	/* buffer too small */
    {
	if (HeapFree (hHeap, 0, lpValue) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("%li byte buffer is not large enough\n", cbEnumValues);
	return ERROR_MORE_DATA;
    }

    TRACE ("pass 2: copying all names and values to buffer\n");

    ppev = (PPRINTER_ENUM_VALUESW) pEnumValues;		/* array of structs */
    pEnumValues += cValues * sizeof (PRINTER_ENUM_VALUESW);

    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
	cbValueNameLen = cbMaxValueNameLen; cbValueLen = cbMaxValueLen;
	ret = RegEnumValueW (hkSubKey, dwIndex, lpValueName, &cbValueNameLen,
		NULL, &dwType, lpValue, &cbValueLen);
	if (ret != ERROR_SUCCESS)
	{
	    if (HeapFree (hHeap, 0, lpValue) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    if (HeapFree (hHeap, 0, lpValueName) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %li\n", r);
	    TRACE ("RegEnumValueW (%li) returned %li\n", dwIndex, ret);
	    return ret;
	}

	cbValueNameLen = (cbValueNameLen + 1) * sizeof (WCHAR);
	memcpy (pEnumValues, lpValueName, cbValueNameLen);
	ppev[dwIndex].pValueName = (LPWSTR) pEnumValues;
	pEnumValues += cbValueNameLen;

	/* return # of *bytes* (including trailing \0), not # of chars */
	ppev[dwIndex].cbValueName = cbValueNameLen;

	ppev[dwIndex].dwType = dwType;

	memcpy (pEnumValues, lpValue, cbValueLen);
	ppev[dwIndex].pData = pEnumValues;
	pEnumValues += cbValueLen;

	ppev[dwIndex].cbData = cbValueLen;

	TRACE ("%s [%li]: copied name (%li bytes) and data (%li bytes)\n",
		debugstr_w (lpValueName), dwIndex, cbValueNameLen, cbValueLen);
    }

    if (HeapFree (hHeap, 0, lpValue) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %li\n", ret);
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ret;
    }

    if (HeapFree (hHeap, 0, lpValueName) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %li\n", ret);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ret;
    }

    ret = RegCloseKey (hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %li\n", ret);
	return ret;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************************
 *		EnumPrinterDataExA	[WINSPOOL.@]
 *
 * This functions returns value names and REG_SZ, REG_EXPAND_SZ, and
 * REG_MULTI_SZ values as ASCII strings in Unicode-sized buffers.  This is
 * what Windows 2000 SP1 does.
 *
 */
DWORD WINAPI EnumPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName,
				LPBYTE pEnumValues, DWORD cbEnumValues,
				LPDWORD pcbEnumValues, LPDWORD pnEnumValues)
{
    INT	    len;
    LPWSTR  pKeyNameW;
    DWORD   ret, dwIndex, dwBufSize;
    HANDLE  hHeap;
    LPSTR   pBuffer;

    TRACE ("%p %s\n", hPrinter, pKeyName);

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    len = MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, NULL, 0);
    if (len == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %li\n", ret);
	return ret;
    }

    hHeap = GetProcessHeap ();
    if (hHeap == NULL)
    {
	ERR ("GetProcessHeap failed\n");
	return ERROR_OUTOFMEMORY;
    }

    pKeyNameW = HeapAlloc (hHeap, 0, len * sizeof (WCHAR));
    if (pKeyNameW == NULL)
    {
	ERR ("Failed to allocate %li bytes from process heap\n",
		(LONG) len * sizeof (WCHAR));
	return ERROR_OUTOFMEMORY;
    }

    if (MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, pKeyNameW, len) == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %li\n", ret);
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	return ret;
    }

    ret = EnumPrinterDataExW (hPrinter, pKeyNameW, pEnumValues, cbEnumValues,
	    pcbEnumValues, pnEnumValues);
    if (ret != ERROR_SUCCESS)
    {
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %li\n", GetLastError ());
	TRACE ("EnumPrinterDataExW returned %li\n", ret);
	return ret;
    }

    if (HeapFree (hHeap, 0, pKeyNameW) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %li\n", ret);
	return ret;
    }

    if (*pnEnumValues == 0)	/* empty key */
	return ERROR_SUCCESS;

    dwBufSize = 0;
    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev =
		&((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	if (dwBufSize < ppev->cbValueName)
	    dwBufSize = ppev->cbValueName;

	if (dwBufSize < ppev->cbData && (ppev->dwType == REG_SZ ||
		ppev->dwType == REG_EXPAND_SZ || ppev->dwType == REG_MULTI_SZ))
	    dwBufSize = ppev->cbData;
    }

    TRACE ("Largest Unicode name or value is %li bytes\n", dwBufSize);

    pBuffer = HeapAlloc (hHeap, 0, dwBufSize);
    if (pBuffer == NULL)
    {
	ERR ("Failed to allocate %li bytes from process heap\n", dwBufSize);
	return ERROR_OUTOFMEMORY;
    }

    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev =
		&((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	len = WideCharToMultiByte (CP_ACP, 0, ppev->pValueName,
		ppev->cbValueName / sizeof (WCHAR), pBuffer, dwBufSize, NULL,
		NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %li\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pValueName, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);

	if (ppev->dwType != REG_SZ && ppev->dwType != REG_EXPAND_SZ &&
		ppev->dwType != REG_MULTI_SZ)
	    continue;

	len = WideCharToMultiByte (CP_ACP, 0, (LPWSTR) ppev->pData,
		ppev->cbData / sizeof (WCHAR), pBuffer, dwBufSize, NULL, NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %li\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %li\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pData, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);
	TRACE ("  (only first string of REG_MULTI_SZ printed)\n");
    }

    if (HeapFree (hHeap, 0, pBuffer) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %li\n", ret);
	return ret;
    }

    return ERROR_SUCCESS;
}

/******************************************************************************
 *		AddPortA (WINSPOOL.@)
 */
BOOL WINAPI AddPortA(LPSTR pName ,HWND hWnd, LPSTR pMonitorName)
{
    FIXME("(%s, %p, %s\n), stub!\n",pName,hWnd,pMonitorName);
    return FALSE;
}
