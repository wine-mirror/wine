/* 
 * Implementation of some printer driver bits
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "winspool.h"
#include "ldt.h"
#include "winerror.h"
#include "winreg.h"
#include "debug.h"

static char PrinterModel[]	= "Printer Model";
static char DefaultDevMode[]	= "Default DevMode";
static char PrinterDriverData[] = "PrinterDriverData";
static char Printers[]		= "System\\CurrentControlSet\\Control\\Print\\Printers\\";

/******************************************************************
 *                  StartDoc16  [GDI.377]
 *
 */
INT16 WINAPI StartDoc16( HDC16 hdc, const DOCINFO16 *lpdoc )
{
  INT16 retVal;
  TRACE(print,"(%p)\n", lpdoc );
  TRACE(print,"%d 0x%lx:0x%p 0x%lx:0x%p\n",lpdoc->cbSize,
	lpdoc->lpszDocName,PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	lpdoc->lpszOutput,PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  TRACE(print, "%d %s %s\n",lpdoc->cbSize,
	(LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszDocName),
	(LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszOutput));
  retVal =  Escape16(hdc, STARTDOC,
    strlen((LPSTR)PTR_SEG_TO_LIN(lpdoc->lpszDocName)), lpdoc->lpszDocName, 0);
  TRACE(print,"Escape16 returned %d\n",retVal);
  return retVal;
}

/******************************************************************
 *                  EndPage16  [GDI.380]
 *
 */
INT16 WINAPI EndPage16( HDC16 hdc )
{
  INT16 retVal;
  retVal =  Escape16(hdc, NEWFRAME, 0, 0, 0);
  TRACE(print,"Escape16 returned %d\n",retVal);
  return retVal;
}

/******************************************************************
 *                  StartDoc32A  [GDI32.347]
 *
 */
INT WINAPI StartDocA(HDC hdc ,const DOCINFOA* doc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/*************************************************************************
 *                  StartDoc32W [GDI32.348]
 * 
 */
INT WINAPI StartDocW(HDC hdc, const DOCINFOW* doc) {
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  StartPage32  [GDI32.349]
 *
 */
INT WINAPI StartPage(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  EndPage32  [GDI32.77]
 *
 */
INT WINAPI EndPage(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************
 *                  EndDoc16  [GDI.378]
 *
 */
INT16 WINAPI EndDoc16(HDC16 hdc)
{
  return  Escape16(hdc, ENDDOC, 0, 0, 0);
}

/******************************************************************
 *                  EndDoc32  [GDI32.76]
 *
 */
INT WINAPI EndDoc(HDC hdc)
{
  FIXME(gdi,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); 
  return 0; /* failure*/
}

/******************************************************************************
 *                 AbortDoc16  [GDI.382]
 */
INT16 WINAPI AbortDoc16(HDC16 hdc)
{
  return Escape16(hdc, ABORTDOC, 0, 0, 0);
}

/******************************************************************************
 *                 AbortDoc32  [GDI32.0]
 */
INT WINAPI AbortDoc(HDC hdc)
{
    FIXME(gdi, "(%d): stub\n", hdc);
    return 1;
}

/******************************************************************
 *                  DrvGetPrinterDataInternal
 *
 * Helper for DrvGetPrinterData
 */
static DWORD DrvGetPrinterDataInternal(LPSTR RegStr_Printer,
LPBYTE lpPrinterData, int cbData, int what)
{
    DWORD res = -1;
    HKEY hkey;
    DWORD dwType, cbQueryData;

    if (!(RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey))) {
        if (what == INT_PD_DEFAULT_DEVMODE) { /* "Default DevMode" */
            if (!(RegQueryValueExA(hkey, DefaultDevMode, 0, &dwType, 0, &cbQueryData))) {
                if (!lpPrinterData)
		    res = cbQueryData;
		else if ((cbQueryData) && (cbQueryData <= cbData)) {
		    cbQueryData = cbData;
		    if (RegQueryValueExA(hkey, DefaultDevMode, 0,
				&dwType, lpPrinterData, &cbQueryData))
		        res = cbQueryData;
		}
	    }
	} else { /* "Printer Driver" */
	    cbQueryData = 32;
	    RegQueryValueExA(hkey, "Printer Driver", 0,
			&dwType, lpPrinterData, &cbQueryData);
	    res = cbQueryData;
	}
    }
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************
 *                DrvGetPrinterData     [GDI.282]
 *
 */
DWORD WINAPI DrvGetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               LPDWORD lpType, LPBYTE lpPrinterData,
                               int cbData, LPDWORD lpNeeded)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0, hkey2 = 0;
    DWORD res = 0;
    DWORD dwType, PrinterAttr, cbPrinterAttr, SetData, size;

    if (HIWORD(lpPrinter))
            TRACE(print,"printer %s\n",lpPrinter);
    else
            TRACE(print,"printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE(print,"profile %s\n",lpProfile);
    else
            TRACE(print,"profile %p\n",lpProfile);
    TRACE(print,"lpType %p\n",lpType);

    if ((!lpPrinter) || (!lpProfile) || (!lpNeeded))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
                               strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if (((DWORD)lpProfile == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_DEVMODE);
	if (size+1) {
	    *lpNeeded = size;
	    if ((lpPrinterData) && (*lpNeeded > cbData))
		res = ERROR_MORE_DATA;
	}
	else res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    if (((DWORD)lpProfile == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel)))) {
	*lpNeeded = 32;
	if (!lpPrinterData) goto failed;
	if (cbData < 32) {
	    res = ERROR_MORE_DATA;
	    goto failed;
	}
	size = DrvGetPrinterDataInternal(RegStr_Printer, lpPrinterData, cbData,
					 INT_PD_DEFAULT_MODEL);
	if ((size+1) && (lpType))
	    *lpType = REG_SZ;
	else
	    res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	if ((res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)))
	    goto failed;
        cbPrinterAttr = 4;
        if ((res = RegQueryValueExA(hkey, "Attributes", 0,
                        &dwType, (LPBYTE)&PrinterAttr, &cbPrinterAttr)))
	    goto failed;
	if ((res = RegOpenKeyA(hkey, PrinterDriverData, &hkey2)))
	    goto failed;
        *lpNeeded = cbData;
        res = RegQueryValueExA(hkey2, lpProfile, 0,
                lpType, lpPrinterData, lpNeeded);
        if ((res != ERROR_CANTREAD) &&
         ((PrinterAttr &
        (PRINTER_ATTRIBUTE_ENABLE_BIDI|PRINTER_ATTRIBUTE_NETWORK))
        == PRINTER_ATTRIBUTE_NETWORK))
        {
	    if (!(res) && (*lpType == REG_DWORD) && (*(LPDWORD)lpPrinterData == -1))
	        res = ERROR_INVALID_DATA;
	}
	else
        {
	    SetData = -1;
	    RegSetValueExA(hkey2, lpProfile, 0, REG_DWORD, (LPBYTE)&SetData, 4); /* no result returned */
	}
    }
	
failed:
    if (hkey2) RegCloseKey(hkey2);
    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}


/******************************************************************
 *                 DrvSetPrinterData     [GDI.281]
 *
 */
DWORD WINAPI DrvSetPrinterData16(LPSTR lpPrinter, LPSTR lpProfile,
                               DWORD lpType, LPBYTE lpPrinterData,
                               DWORD dwSize)
{
    LPSTR RegStr_Printer;
    HKEY hkey = 0;
    DWORD res = 0;

    if (HIWORD(lpPrinter))
            TRACE(print,"printer %s\n",lpPrinter);
    else
            TRACE(print,"printer %p\n",lpPrinter);
    if (HIWORD(lpProfile))
            TRACE(print,"profile %s\n",lpProfile);
    else
            TRACE(print,"profile %p\n",lpProfile);
    TRACE(print,"lpType %08lx\n",lpType);

    if ((!lpPrinter) || (!lpProfile) ||
    ((DWORD)lpProfile == INT_PD_DEFAULT_MODEL) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, PrinterModel))))
	return ERROR_INVALID_PARAMETER;

    RegStr_Printer = HeapAlloc(GetProcessHeap(), 0,
			strlen(Printers) + strlen(lpPrinter) + 2);
    strcpy(RegStr_Printer, Printers);
    strcat(RegStr_Printer, lpPrinter);

    if (((DWORD)lpProfile == INT_PD_DEFAULT_DEVMODE) || (HIWORD(lpProfile) &&
    (!strcmp(lpProfile, DefaultDevMode)))) {
	if ( RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey) 
	     != ERROR_SUCCESS ||
	     RegSetValueExA(hkey, DefaultDevMode, 0, REG_BINARY, 
			      lpPrinterData, dwSize) != ERROR_SUCCESS )
	        res = ERROR_INVALID_PRINTER_NAME;
    }
    else
    {
	strcat(RegStr_Printer, "\\");

	if( (res = RegOpenKeyA(HKEY_LOCAL_MACHINE, RegStr_Printer, &hkey)) ==
	    ERROR_SUCCESS ) {

	    if (!lpPrinterData) 
	        res = RegDeleteValueA(hkey, lpProfile);
	    else
                res = RegSetValueExA(hkey, lpProfile, 0, lpType,
				       lpPrinterData, dwSize);
	}
    }

    if (hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, RegStr_Printer);
    return res;
}


/******************************************************************
 *              DeviceCapabilities32A    [WINSPOOL.151]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR printer,LPCSTR target,WORD z,
                                   LPSTR a,LPDEVMODEA b)
{
    FIXME(print,"(%s,%s,%d,%p,%p):stub.\n",printer,target,z,a,b);
    return 1;   	
}


/*****************************************************************************
 *          DeviceCapabilities32W 
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
                                   WORD fwCapability, LPWSTR pOutput,
                                   const DEVMODEW *pDevMode)
{
    FIXME(print,"(%p,%p,%d,%p,%p): stub\n",
          pDevice, pPort, fwCapability, pOutput, pDevMode);
    return -1;
}

/******************************************************************
 *              DocumentProperties32A   [WINSPOOL.155]
 *
 */
LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
                                  LPDEVMODEA pDevModeInput,DWORD fMode )
{
    FIXME(print,"(%d,%d,%s,%p,%p,%ld):stub.\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );
    return 1;
}


/*****************************************************************************
 *          DocumentProperties32W 
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODEW pDevModeOutput,
                                  LPDEVMODEW pDevModeInput, DWORD fMode)
{
    FIXME(print,"(%d,%d,%s,%p,%p,%ld): stub\n",
          hWnd,hPrinter,debugstr_w(pDeviceName),pDevModeOutput,pDevModeInput,
	  fMode);
    return -1;
}


/******************************************************************
 *              OpenPrinter32A        [WINSPOOL.196]
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSA pDefault)
{
    FIXME(print,"(%s,%p,%p):stub\n",debugstr_a(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              OpenPrinter32W        [WINSPOOL.197]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSW pDefault)
{
    FIXME(print,"(%s,%p,%p):stub\n",debugstr_w(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              ENUMPRINTERS_AddInfo4A        internal
 *
 *    Creates a PRINTER_INFO_4A structure at:  lpbPrinters[dwNextStructPos]
 *    for printer PrinterNameKey.
 *    Note that there is no check whether the information really fits!
 *
 * RETURNS
 *    FALSE if there is still space left in the buffer.
 *
 * BUGS:
 *    This function should not exist in Win95 mode, but does anyway.
 */
BOOL ENUMPRINTERS_AddInfo4A(
                   LPSTR lpszPrinterName,/* name of printer to fill struct for*/
                   LPBYTE lpbPrinters,   /* buffer which receives info*/
                   DWORD  dwNextStructPos,  /* pos in buffer for struct */
                   LPDWORD dwNextStringPos, /* pos in buffer for next string */
				   DWORD  dwBufSize,        /* max size of buffer in bytes */
                   BOOL   bCalcSpaceOnly    /* TRUE if out-of-space in buffer */
){                   
 HKEY  hPrinterSettings;
 DWORD DataType;
 DWORD DataSize=8;
 char  Data[8];
 DWORD* DataPointer= (DWORD*) Data;
 LPSTR lpszPrinterSettings = (LPSTR) malloc(strlen(Printers)+
  										   	   strlen(lpszPrinterName)+2);
 LPPRINTER_INFO_4A lpPInfo4 = (LPPRINTER_INFO_4A) &lpbPrinters[dwNextStructPos];

 lpPInfo4->pPrinterName = &lpbPrinters[*dwNextStringPos];
 *dwNextStringPos += strlen(lpszPrinterName)+1;
 if (*dwNextStringPos > dwBufSize)
 	bCalcSpaceOnly=TRUE;
 if (bCalcSpaceOnly==FALSE)
     strcpy(lpPInfo4->pPrinterName, lpszPrinterName);
 lpPInfo4->pServerName = NULL;
 
 /* open the registry to find the attributes of the printer */
 if (lpszPrinterSettings!=NULL)
 	{
     strcpy(lpszPrinterSettings,Printers);
     strcat(lpszPrinterSettings,lpszPrinterName);
    }
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszPrinterSettings, 0, 
 						KEY_READ, &hPrinterSettings) != ERROR_SUCCESS)
	{
     FIXME(print, "The registry did not contain my printer anymore?\n");
     lpPInfo4->Attributes = 0;
    }
 else
    {
	 RegQueryValueExA(hPrinterSettings, "Attributes", NULL, &DataType,
  					Data, &DataSize);
	 if (DataType==REG_DWORD) 
  	 lpPInfo4->Attributes = PRINTER_ATTRIBUTE_LOCAL + *DataPointer; 
    }
 if (lpszPrinterSettings)
	 free(lpszPrinterSettings);

 return(bCalcSpaceOnly);     
}

/******************************************************************
 *              ENUMPRINTERS_AddInfo5A        internal
 *
 *    Creates a PRINTER_INFO_5A structure at:  lpbPrinters[dwNextStructPos]
 *    for printer PrinterNameKey.
 *    Settings are read from the registry.
 *    Note that there is no check whether the information really fits!
 * RETURNS
 *    FALSE if there is still space left in the buffer.
 */
BOOL ENUMPRINTERS_AddInfo5A(
                   LPSTR lpszPrinterName,/* name of printer to fill struct for*/
                   LPBYTE lpbPrinters,   /* buffer which receives info*/
                   DWORD  dwNextStructPos,  /* pos in buffer for struct */
                   LPDWORD dwNextStringPos, /* pos in buffer for next string */
				   DWORD  dwBufSize,        /* max size of buffer in bytes */
                   BOOL   bCalcSpaceOnly    /* TRUE if out-of-space in buffer */
){                   
 HKEY  hPrinterSettings;
 DWORD DataType;
 DWORD DataSize=255;
 char  Data[255];
 DWORD* DataPointer= (DWORD*) Data;
 LPSTR lpszPrinterSettings = (LPSTR) malloc(strlen(Printers)+
  										   	   strlen(lpszPrinterName)+2);
 LPPRINTER_INFO_5A lpPInfo5 = (LPPRINTER_INFO_5A) &lpbPrinters[dwNextStructPos];

 /* copy the PrinterName into the structure */
 lpPInfo5->pPrinterName = &lpbPrinters[*dwNextStringPos];
 *dwNextStringPos += strlen(lpszPrinterName)+1;
 if (*dwNextStringPos > dwBufSize)
 	bCalcSpaceOnly=TRUE;
 if (bCalcSpaceOnly==FALSE)
     strcpy(lpPInfo5->pPrinterName, lpszPrinterName);

 /* open the registry to find the attributes, etc of the printer */
 if (lpszPrinterSettings!=NULL)
 	{
     strcpy(lpszPrinterSettings,Printers);
     strcat(lpszPrinterSettings,lpszPrinterName);
    }
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszPrinterSettings, 0, 
 						KEY_READ, &hPrinterSettings) != ERROR_SUCCESS)
	{
     FIXME(print, "The registry did not contain my printer anymore?\n");
     lpPInfo5->Attributes = 0;
     lpPInfo5->pPortName = NULL;
     lpPInfo5->Attributes = 0;  
     lpPInfo5->DeviceNotSelectedTimeOut =0;
     lpPInfo5->TransmissionRetryTimeout = 0; 
    }
 else
    {
	 RegQueryValueExA(hPrinterSettings, "Attributes", NULL, &DataType,
  					Data, &DataSize);
	 if (DataType==REG_DWORD) 
  	 	lpPInfo5->Attributes = PRINTER_ATTRIBUTE_LOCAL + *DataPointer; 

     DataSize=255;
	 RegQueryValueExA(hPrinterSettings, "txTimeout", NULL, &DataType,
  					Data, &DataSize);
	 if (DataType==REG_DWORD) 
 		lpPInfo5->DeviceNotSelectedTimeOut = *DataPointer;

     DataSize=255;
	 RegQueryValueExA(hPrinterSettings, "dnsTimeout", NULL, &DataType,
  					Data, &DataSize);
	 if (DataType==REG_DWORD) 
		 lpPInfo5->TransmissionRetryTimeout = *DataPointer; 
         
     DataSize=255;
	 RegQueryValueExA(hPrinterSettings, "Port", NULL, &DataType,
  					Data, &DataSize);
 	 lpPInfo5->pPortName = &lpbPrinters[*dwNextStringPos];
     *dwNextStringPos += DataSize+1;
 	 if (*dwNextStringPos > dwBufSize)
	 	bCalcSpaceOnly=TRUE;
	 if (bCalcSpaceOnly==FALSE)
    	 strcpy(lpPInfo5->pPortName, Data);         
    }
    
 if (lpszPrinterSettings)
	 free(lpszPrinterSettings);

 return(bCalcSpaceOnly);     
}


/******************************************************************
 *              EnumPrintersA        [WINSPOOL.174]
 *
 *    Enumerates the available printers, print servers and print
 *    providers, depending on the specified flags, name and level.
 *
 * RETURNS:
 *
 *    If level is set to 1:
 *    If level is set to 2:
 *      Not implemented yet! 
 *      Returns TRUE with an empty list.
 *
 *    If level is set to 4 (officially WinNT only):
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL).
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
 *	    returns zero (faillure!)
 *      
 *    Returns nonzero (TRUE) on succes, or zero on faillure, use GetLastError
 *    for information.
 *
 * BUGS:
 *    - Only PRINTER_ENUM_LOCAL and PRINTER_ENUM_NAME are implemented.
 *    - Only level 4 and 5 are implemented at the moment.
 *    - 16-bit printer drivers are not enumerated.
 *    - returned bytes used/needed do not match the real Windoze implementation
 *
 * NOTE:
 *    - In a regular Wine installation, no registry settings for printers
 *      exist, which makes this function return an empty list.
 */
BOOL  WINAPI EnumPrintersA(
					DWORD dwType,      /* Types of print objects to enumerate */
                    LPSTR lpszName,    /* name of objects to enumerate */
			        DWORD dwLevel,     /* type of printer info structure */
                    LPBYTE lpbPrinters,/* buffer which receives info*/
			        DWORD cbBuf,       /* max size of buffer in bytes */
                    LPDWORD lpdwNeeded,/* pointer to var: # bytes used/needed */
			        LPDWORD lpdwReturned/* number of entries returned */
                   )
{
 HKEY  hPrinterListKey;
 DWORD dwIndex=0;
 char  PrinterName[255];
 DWORD PrinterNameLength=255;
 FILETIME FileTime;
 DWORD dwNextStringPos;	  /* position of next space for a string in the buffer*/
 DWORD dwStructPrinterInfoSize;	/* size of a Printer_Info_X structure */
 BOOL  bCalcSpaceOnly=FALSE;/* if TRUE: don't store data, just calculate space*/
 
 TRACE(print, "EnumPrintersA\n");

 /* check for valid Flags */
 if (dwType != PRINTER_ENUM_LOCAL && dwType != PRINTER_ENUM_NAME)
 	{
     SetLastError(ERROR_INVALID_FLAGS);
     return(0);
    }
 if (dwLevel == 1 || dwLevel == 2)
     return(TRUE);
 if (dwLevel != 4 && dwLevel != 5)
 	{
     SetLastError(ERROR_INVALID_PARAMETER);
     return(0);
    } 	

 /* zero out the data area, and initialise some returns to zero,
  * to prevent problems 
 */
{
  int i;
  for (i=0; i<cbBuf; i++)
  	  lpbPrinters[i]=0;
 }
    *lpdwReturned=0;
    *lpdwNeeded = 0;
 
 /* get a pointer to a list of all printer names in the registry */ 
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Printers, 0, KEY_READ,
 				  &hPrinterListKey) !=ERROR_SUCCESS)
 	{
     /* Oh no! An empty list of printers!
      * (which is a valid configuration anyway)
      */
     TRACE(print, "No entries in the Printers part of the registry\n");
     return(TRUE);
    }

 /* count the number of entries and check if it fits in the buffer
  */
 while(RegEnumKeyExA(hPrinterListKey, dwIndex, PrinterName, &PrinterNameLength,
                   NULL, NULL, NULL, &FileTime)==ERROR_SUCCESS)
    {
     PrinterNameLength=255;
     dwIndex++;
    }
 *lpdwReturned = dwIndex;    
 switch(dwLevel)
 	{
     case 1:
     	dwStructPrinterInfoSize = sizeof(PRINTER_INFO_1A);
      	break;
     case 2:
     	dwStructPrinterInfoSize = sizeof(PRINTER_INFO_2A);
      	break;     
     case 4:
     	dwStructPrinterInfoSize = sizeof(PRINTER_INFO_4A);
      	break;
     case 5:
     	dwStructPrinterInfoSize = sizeof(PRINTER_INFO_5A);
      	break;
     default:
     	dwStructPrinterInfoSize = 0;
      	break;     
    } 
 if (dwIndex*dwStructPrinterInfoSize+1 > cbBuf)
 	bCalcSpaceOnly = TRUE;
    
 /* the strings which contain e.g. PrinterName, PortName, etc,
  * are also stored in lpbPrinters, but after the regular structs.
  * dwNextStringPos will always point to the next free place for a 
  * string.
  */  
 dwNextStringPos=(dwIndex+1)*dwStructPrinterInfoSize;    

 /* check each entry: if OK, add to list in corresponding INFO .
  */    
 for(dwIndex=0; dwIndex < *lpdwReturned; dwIndex++)
    {
     PrinterNameLength=255;
     if (RegEnumKeyExA(hPrinterListKey, dwIndex, PrinterName, &PrinterNameLength,
                   NULL, NULL, NULL, &FileTime)!=ERROR_SUCCESS)
     	break;	/* exit for loop*/
        
     /* check whether this printer is allowed in the list
      * by comparing name to lpszName 
      */
     if (dwType == PRINTER_ENUM_NAME)
        if (strcmp(PrinterName,lpszName)!=0)
        	continue;		

     switch(dwLevel)
     	{
         case 1:
         	/* FIXME: unimplemented */
            break;
         case 2:
            /* FIXME: unimplemented */
            break;
         case 4:
            bCalcSpaceOnly = ENUMPRINTERS_AddInfo4A(PrinterName, lpbPrinters,
            					dwIndex*dwStructPrinterInfoSize,
                                &dwNextStringPos, cbBuf, bCalcSpaceOnly);
            break;
         case 5:
            bCalcSpaceOnly = ENUMPRINTERS_AddInfo5A(PrinterName, lpbPrinters,
            					dwIndex*dwStructPrinterInfoSize,
                                &dwNextStringPos, cbBuf, bCalcSpaceOnly);
            break;
        }     	
    }
 RegCloseKey(hPrinterListKey);
 *lpdwNeeded = dwNextStringPos;
 
 if (bCalcSpaceOnly==TRUE)
 	{
	  int i;
	  for (i=0; i<cbBuf; i++)
	  	  lpbPrinters[i]=0;
     *lpdwReturned=0;    
    } 
 
 return(TRUE);
}

/******************************************************************
 *              EnumPrinters32W        [WINSPOOL.175]
 *
 */
BOOL  WINAPI EnumPrintersW(DWORD dwType, LPWSTR lpszName,
			       DWORD dwLevel, LPBYTE lpbPrinters,
			       DWORD cbBuf, LPDWORD lpdwNeeded,
			       LPDWORD lpdwReturned)
{
    FIXME(print,"Nearly empty stub\n");
    *lpdwReturned=0;
    *lpdwNeeded = 0;
    return TRUE;
}

/******************************************************************
 *              AddMonitor32A        [WINSPOOL.107]
 *
 */
BOOL WINAPI AddMonitorA(LPCSTR pName, DWORD Level, LPBYTE pMonitors)
{
    FIXME(print, "(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *              DeletePrinterDriver32A        [WINSPOOL.146]
 *
 */
BOOL WINAPI
DeletePrinterDriverA (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
{
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pDriverName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeleteMonitor32A        [WINSPOOL.135]
 *
 */
BOOL WINAPI
DeleteMonitorA (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
{
    FIXME(print,"(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
          debugstr_a(pMonitorName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************
 *              DeletePort32A        [WINSPOOL.137]
 *
 */
BOOL WINAPI
DeletePortA (LPSTR pName, HWND hWnd, LPSTR pPortName)
{
    FIXME(print,"(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
          debugstr_a(pPortName));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    SetPrinter32W  [WINSPOOL.214]
 */
BOOL WINAPI
SetPrinterW(
  HANDLE  hPrinter,
  DWORD     Level,
  LPBYTE    pPrinter,
  DWORD     Command) {

       FIXME(print,"():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/******************************************************************************
 *    WritePrinter32  [WINSPOOL.223]
 */
BOOL WINAPI
WritePrinter( 
  HANDLE  hPrinter,
  LPVOID  pBuf,
  DWORD   cbBuf,
  LPDWORD pcWritten) {

       FIXME(print,"():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/*****************************************************************************
 *          AddForm32A  [WINSPOOL.103]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddForm32W  [WINSPOOL.104]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME(print, "(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJob32A  [WINSPOOL.105]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJob32W  [WINSPOOL.106]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinter32A  [WINSPOOL.117]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%s,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}

/*****************************************************************************
 *          AddPrinter32W  [WINSPOOL.122]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME(print, "(%p,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}


/*****************************************************************************
 *          ClosePrinter32  [WINSPOOL.126]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32A  [WINSPOOL.133]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32W  [WINSPOOL.134]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME(print, "(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter32  [WINSPOOL.143]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    FIXME(print, "(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          SetPrinter32A  [WINSPOOL.211]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME(print, "(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32A  [WINSPOOL.209]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32W  [WINSPOOL.210]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME(print, "(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetForm32A  [WINSPOOL.181]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetForm32W  [WINSPOOL.182]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32A  [WINSPOOL.207]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32W  [WINSPOOL.208]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter32  [WINSPOOL.202]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME(print, "(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32A  [WINSPOOL.203]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32W  [WINSPOOL.204]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME(print, "(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32A  [WINSPOOL.187]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter, 
         cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinter32W  [WINSPOOL.194]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter,
          cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32A  [WINSPOOL.190]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
                                 DWORD Level, LPBYTE pDriverInfo,
                                 DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
         Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32W  [WINSPOOL.193]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME(print, "(%d,%p,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
          Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}
/*****************************************************************************
 *          AddPrinterDriver32A  [WINSPOOL.120]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME(print, "(%s,%ld,%p): stub\n",printerName,level,pDriverInfo);
    return FALSE;
}
/*****************************************************************************
 *          AddPrinterDriver32W  [WINSPOOL.121]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME(print, "(%s,%ld,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}



