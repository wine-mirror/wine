/* 
 * WINSPOOL functions
 * 
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "winspool.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(winspool)

CRITICAL_SECTION PRINT32_RegistryBlocker;

#define NUM_PRINTER_MAX 10

typedef struct _OPENEDPRINTERA
{
    LPSTR lpsPrinterName;
    HANDLE hPrinter;
    LPPRINTER_DEFAULTSA lpDefault;
} OPENEDPRINTERA, *LPOPENEDPRINTERA;


/* Initialize the structure OpenedPrinter Table */
static OPENEDPRINTERA OpenedPrinterTableA[NUM_PRINTER_MAX] = 
{ 
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL},
    {NULL, -1, NULL}
};

static char Printers[] =
 "System\\CurrentControlSet\\control\\Print\\Printers\\";
static char Drivers[] =
 "System\\CurrentControlSet\\control\\Print\\Environments\\Wine\\Drivers\\";


/******************************************************************
 *  WINSPOOL_GetOpenedPrinterEntryA
 *  Get the first place empty in the opened printer table
 */
static LPOPENEDPRINTERA WINSPOOL_GetOpenedPrinterEntryA()
{
    int i;
    for( i=0; i< NUM_PRINTER_MAX; i++)
	if (OpenedPrinterTableA[i].hPrinter == -1)
	{
	    OpenedPrinterTableA[i].hPrinter = i + 1;
	    return &OpenedPrinterTableA[i];
	}
    return NULL;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterA
 *  Get the pointer to the opened printer referred by the handle
 */
static LPOPENEDPRINTERA WINSPOOL_GetOpenedPrinterA(int printerHandle)
{
    if((printerHandle <=0) || (printerHandle > (NUM_PRINTER_MAX + 1)))
	return NULL;
    return &OpenedPrinterTableA[printerHandle -1];
}

/******************************************************************
 *              DeviceCapabilities32A    [WINSPOOL.151]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR pDeivce,LPCSTR pPort, WORD cap,
			       LPSTR pOutput, LPDEVMODEA lpdm)
{
    return GDI_CallDeviceCapabilities16(pDeivce, pPort, cap, pOutput, lpdm);

}


/*****************************************************************************
 *          DeviceCapabilities32W 
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    FIXME("(%p,%p,%d,%p,%p): stub\n",
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
    LPOPENEDPRINTERA lpOpenedPrinter;
    LPSTR lpName = pDeviceName;

    TRACE("(%d,%d,%s,%p,%p,%ld)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );

    if(!pDeviceName) {
        lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
	if(!lpOpenedPrinter) {
	    SetLastError(ERROR_INVALID_HANDLE);
	    return -1;
	}
	lpName = lpOpenedPrinter->lpsPrinterName;
    }

    return GDI_CallExtDeviceMode16(hWnd, pDevModeOutput, lpName, NULL,
				   pDevModeInput, NULL, fMode);

}


/*****************************************************************************
 *          DocumentProperties32W 
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
                                  LPWSTR pDeviceName,
                                  LPDEVMODEW pDevModeOutput,
                                  LPDEVMODEW pDevModeInput, DWORD fMode)
{
    FIXME("(%d,%d,%s,%p,%p,%ld): stub\n",
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
  /* Not implemented: use the DesiredAccess of pDefault to set 
     the access rights to the printer */

    LPOPENEDPRINTERA lpOpenedPrinter;

    TRACE("(printerName: %s, pDefault %p\n", lpPrinterName, pDefault);

    /* Get a place in the opened printer buffer*/
    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterEntryA();

    if((lpOpenedPrinter != NULL) && (lpPrinterName !=NULL) && 
       (phPrinter != NULL))
    {
	/* Get the name of the printer */
	lpOpenedPrinter->lpsPrinterName = 
	  HeapAlloc(GetProcessHeap(), 0, lstrlenA(lpPrinterName));
	lstrcpyA(lpOpenedPrinter->lpsPrinterName, lpPrinterName);

	/* Get the unique handle of the printer*/
	*phPrinter = lpOpenedPrinter->hPrinter;

	if (pDefault != NULL)
	{
	    /* Allocate enough memory for the lpDefault structure */
	    lpOpenedPrinter->lpDefault = 
	      HeapAlloc(GetProcessHeap(), 0, sizeof(PRINTER_DEFAULTSA));
	    lpOpenedPrinter->lpDefault->pDevMode =
	      HeapAlloc(GetProcessHeap(), 0, sizeof(DEVMODEA));
	    lpOpenedPrinter->lpDefault->pDatatype = 
	      HeapAlloc(GetProcessHeap(), 0, lstrlenA(pDefault->pDatatype));

	    /*Copy the information from incoming parameter*/
	    memcpy(lpOpenedPrinter->lpDefault->pDevMode, pDefault->pDevMode,
		   sizeof(DEVMODEA));
	    lstrcpyA(lpOpenedPrinter->lpDefault->pDatatype,
		     pDefault->pDatatype);
	    lpOpenedPrinter->lpDefault->DesiredAccess =
	      pDefault->DesiredAccess;
	}

	return TRUE;
    }

    if(lpOpenedPrinter == NULL)
	FIXME("Reach the OpenedPrinterTable maximum, augment this max.\n");
     return FALSE;
}

/******************************************************************
 *              OpenPrinter32W        [WINSPOOL.197]
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter,
			     LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%s,%p,%p):stub\n",debugstr_w(lpPrinterName), phPrinter,
          pDefault);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}




/******************************************************************
 *              ENUMPRINTERS_GetDWORDFromRegistryA    internal
 *
 * Reads a DWORD from registry KeyName 
 *
 * RETURNS
 *    value on OK or NULL on error
 */
DWORD ENUMPRINTERS_GetDWORDFromRegistryA(
				HKEY  hPrinterSettings,  /* handle to registry key */
				LPSTR KeyName			 /* name key to retrieve string from*/
){                   
 DWORD DataSize=8;
 DWORD DataType;
 BYTE  Data[8];
 DWORD Result=684;

 if (RegQueryValueExA(hPrinterSettings, KeyName, NULL, &DataType,
  					Data, &DataSize)!=ERROR_SUCCESS)
	FIXME("Query of register didn't succeed?\n");                     
 if (DataType == REG_DWORD_LITTLE_ENDIAN)
 	Result = Data[0] + (Data[1]<<8) + (Data[2]<<16) + (Data[3]<<24);
 if (DataType == REG_DWORD_BIG_ENDIAN)
 	Result = Data[3] + (Data[2]<<8) + (Data[1]<<16) + (Data[0]<<24);
 return(Result);
}


/******************************************************************
 *              ENUMPRINTERS_AddStringFromRegistryA    internal
 *
 * Reads a string from registry KeyName and writes it at
 * lpbPrinters[dwNextStringPos]. Store reference to string in Dest.
 *
 * RETURNS
 *    FALSE if there is still space left in the buffer.
 */
BOOL ENUMPRINTERS_AddStringFromRegistryA(
		HKEY  hPrinterSettings, /* handle to registry key */
		LPSTR KeyName,	        /* name key to retrieve string from*/
                LPSTR* Dest,	        /* pointer to write string addres to */
                LPBYTE lpbPrinters,     /* buffer which receives info*/
                LPDWORD dwNextStringPos,/* pos in buffer for next string */
	        DWORD  dwBufSize,       /* max size of buffer in bytes */
                BOOL   bCalcSpaceOnly   /* TRUE if out-of-space in buffer */
){                   
 DWORD DataSize=34;
 DWORD DataType;
 LPSTR Data = (LPSTR) malloc(DataSize*sizeof(char));

 while(RegQueryValueExA(hPrinterSettings, KeyName, NULL, &DataType,
  					Data, &DataSize)==ERROR_MORE_DATA)
    {
     Data = (LPSTR) realloc(Data, DataSize+2);
    }

 if (DataType == REG_SZ)
 	{                   
	 if (bCalcSpaceOnly==FALSE)
	 *Dest = &lpbPrinters[*dwNextStringPos];
	 *dwNextStringPos += DataSize+1;
	 if (*dwNextStringPos > dwBufSize)
	 	bCalcSpaceOnly=TRUE;
	 if (bCalcSpaceOnly==FALSE)
        {
         if (DataSize==0)		/* DataSize = 0 means empty string, even though*/
         	*Dest[0]=0;			/* the data itself needs not to be empty */
         else
	         strcpy(*Dest, Data);
        }
 	}
 else
 	WARN("Expected string setting, got something else from registry");
    
 if (Data)
    free(Data);
 return(bCalcSpaceOnly);
}



/******************************************************************
 *              ENUMPRINTERS_AddInfo2A        internal
 *
 *    Creates a PRINTER_INFO_2A structure at:  lpbPrinters[dwNextStructPos]
 *    for printer PrinterNameKey.
 *    Note that there is no check whether the information really fits!
 *
 * RETURNS
 *    FALSE if there is still space left in the buffer.
 *
 * BUGS:
 *    This function should not only read the registry but also ask the driver
 *    for information.
 */
BOOL ENUMPRINTERS_AddInfo2A(
                   LPSTR lpszPrinterName,/* name of printer to fill struct for*/
                   LPBYTE lpbPrinters,   /* buffer which receives info*/
                   DWORD  dwNextStructPos,  /* pos in buffer for struct */
                   LPDWORD dwNextStringPos, /* pos in buffer for next string */
				   DWORD  dwBufSize,        /* max size of buffer in bytes */
                   BOOL   bCalcSpaceOnly    /* TRUE if out-of-space in buffer */
){                   
 HKEY  hPrinterSettings;
 DWORD DevSize=0;
 DWORD DataType;
 LPSTR lpszPrinterSettings = (LPSTR) malloc(strlen(Printers)+
  										   	   strlen(lpszPrinterName)+2);
 LPPRINTER_INFO_2A lpPInfo2 = (LPPRINTER_INFO_2A) &lpbPrinters[dwNextStructPos];

 /* open the registry to find the attributes, etc of the printer */
 if (lpszPrinterSettings!=NULL)
 	{
     strcpy(lpszPrinterSettings,Printers);
     strcat(lpszPrinterSettings,lpszPrinterName);
    }
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszPrinterSettings, 0, 
 						KEY_READ, &hPrinterSettings) != ERROR_SUCCESS)
	{
     WARN("The registry did not contain my printer anymore?\n");
    }
 else
    {
     if (bCalcSpaceOnly==FALSE)
     lpPInfo2->pServerName = NULL;
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Name", &(lpPInfo2->pPrinterName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Share Name", &(lpPInfo2->pShareName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Port", &(lpPInfo2->pPortName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Printer Driver", &(lpPInfo2->pDriverName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Description", &(lpPInfo2->pComment), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Location", &(lpPInfo2->pLocation), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);

     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Separator File", &(lpPInfo2->pSepFile), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Print Processor", &(lpPInfo2->pPrintProcessor), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Datatype", &(lpPInfo2->pDatatype), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Parameters", &(lpPInfo2->pParameters), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     if (bCalcSpaceOnly == FALSE)
     	{                             
     lpPInfo2->pSecurityDescriptor = NULL; /* EnumPrinters doesn't return this*/

     					  /* FIXME: Attributes gets LOCAL as no REMOTE exists*/
	 lpPInfo2->Attributes = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"Attributes") +PRINTER_ATTRIBUTE_LOCAL; 
	 lpPInfo2->Priority   = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"Priority"); 
	 lpPInfo2->DefaultPriority = ENUMPRINTERS_GetDWORDFromRegistryA(
     								hPrinterSettings, "Default Priority"); 
	 lpPInfo2->StartTime  = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"StartTime"); 
	 lpPInfo2->UntilTime  = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"UntilTime"); 
	 lpPInfo2->Status     = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"Status"); 
	 lpPInfo2->cJobs      = 0;    /* FIXME: according to MSDN, this does not 
     							   * reflect the TotalJobs Key ??? */
	 lpPInfo2->AveragePPM = 0;    /* FIXME: according to MSDN, this does not 
     							   * reflect the TotalPages Key ??? */

     /* and read the devModes structure... */
      RegQueryValueExA(hPrinterSettings, "pDevMode", NULL, &DataType,
  					NULL, &DevSize); /* should return ERROR_MORE_DATA */
	      lpPInfo2->pDevMode = (LPDEVMODEA) &lpbPrinters[*dwNextStringPos];
      *dwNextStringPos += DevSize + 1;      
        } 
 if (*dwNextStringPos > dwBufSize)
 	bCalcSpaceOnly=TRUE;
 if (bCalcSpaceOnly==FALSE)
	      RegQueryValueExA(hPrinterSettings, "pDevMode", NULL, &DataType,
  					(LPBYTE)lpPInfo2->pDevMode, &DevSize); 
	}                                 

 if (lpszPrinterSettings)
	 free(lpszPrinterSettings);

 return(bCalcSpaceOnly);     
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
 LPSTR lpszPrinterSettings = (LPSTR) malloc(strlen(Printers)+
  										   	   strlen(lpszPrinterName)+2);
 LPPRINTER_INFO_4A lpPInfo4 = (LPPRINTER_INFO_4A) &lpbPrinters[dwNextStructPos];
 
 /* open the registry to find the attributes of the printer */
 if (lpszPrinterSettings!=NULL)
 	{
     strcpy(lpszPrinterSettings,Printers);
     strcat(lpszPrinterSettings,lpszPrinterName);
    }
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszPrinterSettings, 0, 
 						KEY_READ, &hPrinterSettings) != ERROR_SUCCESS)
	{
     WARN("The registry did not contain my printer anymore?\n");
    }
 else
    {
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Name", &(lpPInfo4->pPrinterName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     					  /* FIXME: Attributes gets LOCAL as no REMOTE exists*/
     if (bCalcSpaceOnly==FALSE)	                     
	 lpPInfo4->Attributes = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"Attributes") +PRINTER_ATTRIBUTE_LOCAL; 
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
 LPSTR lpszPrinterSettings = (LPSTR) malloc(strlen(Printers)+
  										   	   strlen(lpszPrinterName)+2);
 LPPRINTER_INFO_5A lpPInfo5 = (LPPRINTER_INFO_5A) &lpbPrinters[dwNextStructPos];

 /* open the registry to find the attributes, etc of the printer */
 if (lpszPrinterSettings!=NULL)
 	{
     strcpy(lpszPrinterSettings,Printers);
     strcat(lpszPrinterSettings,lpszPrinterName);
    }
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszPrinterSettings, 0, 
 						KEY_READ, &hPrinterSettings) != ERROR_SUCCESS)
	{
     WARN("The registry did not contain my printer anymore?\n");
    }
 else
    {
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Name", &(lpPInfo5->pPrinterName), 
                                  lpbPrinters, dwNextStringPos, 
                                  dwBufSize, bCalcSpaceOnly);
     bCalcSpaceOnly = ENUMPRINTERS_AddStringFromRegistryA(hPrinterSettings, 
     			                  "Port", &(lpPInfo5->pPortName), lpbPrinters,
                                  dwNextStringPos, dwBufSize, bCalcSpaceOnly);
     					  /* FIXME: Attributes gets LOCAL as no REMOTE exists*/
     if (bCalcSpaceOnly == FALSE)
   	   {
	 lpPInfo5->Attributes = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"Attributes") +PRINTER_ATTRIBUTE_LOCAL; 
	 lpPInfo5->DeviceNotSelectedTimeOut 
     					  = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"txTimeout"); 
	 lpPInfo5->TransmissionRetryTimeout
     					  = ENUMPRINTERS_GetDWORDFromRegistryA(hPrinterSettings,
     								"dnsTimeout"); 
    }
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
 *	    returns zero (faillure!)
 *      
 *    Returns nonzero (TRUE) on succes, or zero on faillure, use GetLastError
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
BOOL  WINAPI EnumPrintersA(
					DWORD dwType,      /* Types of print objects to enumerate */
                    LPSTR lpszName,    /* name of objects to enumerate */
			        DWORD dwLevel,     /* type of printer info structure */
                    LPBYTE lpbPrinters,/* buffer which receives info*/		        DWORD cbBuf,       /* max size of buffer in bytes */
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
 
 TRACE("entered.\n");

 /* test whether we're requested to really fill in. If so,
  * zero out the data area, and initialise some returns to zero,
  * to prevent problems 
  */
 if (lpbPrinters==NULL || cbBuf==0)
 	 bCalcSpaceOnly=TRUE;
 else
 {
  int i;
  for (i=0; i<cbBuf; i++)
  	  lpbPrinters[i]=0;
 }
 *lpdwReturned=0;
 *lpdwNeeded = 0;

 /* check for valid Flags */
 if (dwType != PRINTER_ENUM_LOCAL && dwType != PRINTER_ENUM_NAME)
 	{
     SetLastError(ERROR_INVALID_FLAGS);
     return(0);
    }
 switch(dwLevel)
 	{
     case 1:
	     return(TRUE);
     case 2:
     case 4:
     case 5:
     	 break;
     default:
     SetLastError(ERROR_INVALID_PARAMETER);
	     return(FALSE);
    } 	

 /* Enter critical section to prevent AddPrinters() et al. to
  * modify whilst we're reading in the registry
  */
 InitializeCriticalSection(&PRINT32_RegistryBlocker);
 EnterCriticalSection(&PRINT32_RegistryBlocker);
 
 /* get a pointer to a list of all printer names in the registry */ 
 if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Printers, 0, KEY_READ,
 				  &hPrinterListKey) !=ERROR_SUCCESS)
 	{
     /* Oh no! An empty list of printers!
      * (which is a valid configuration anyway)
      */
     TRACE("No entries in the Printers part of the registry\n");
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
            bCalcSpaceOnly = ENUMPRINTERS_AddInfo2A(PrinterName, lpbPrinters,
            					dwIndex*dwStructPrinterInfoSize,
                                &dwNextStringPos, cbBuf, bCalcSpaceOnly);
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
    if  (lpbPrinters!=NULL)
 		{
	  int i;
	  for (i=0; i<cbBuf; i++)
	  	  lpbPrinters[i]=0;
	    } 
     *lpdwReturned=0;    
    } 
 LeaveCriticalSection(&PRINT32_RegistryBlocker); 
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
    FIXME("Nearly empty stub\n");
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
    FIXME("(%s,%lx,%p):stub!\n", pName, Level, pMonitors);
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
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
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
    FIXME("(%s,%s,%s):stub\n",debugstr_a(pName),debugstr_a(pEnvironment),
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
    FIXME("(%s,0x%08x,%s):stub\n",debugstr_a(pName),hWnd,
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

       FIXME("():stub\n");
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

       FIXME("():stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
       return FALSE;
}

/*****************************************************************************
 *          AddForm32A  [WINSPOOL.103]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddForm32W  [WINSPOOL.104]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%d,%ld,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJob32A  [WINSPOOL.105]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData,
                        DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddJob32W  [WINSPOOL.106]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf,
                        LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pData, cbBuf,
          pcbNeeded);
    return 1;
}

/*****************************************************************************
 *          AddPrinter32A  [WINSPOOL.117]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2A *pi = (PRINTER_INFO_2A *) pPrinter;

    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;

    TRACE("(%s,%ld,%p)\n", pName, Level, pPrinter);
    
    if(pName != NULL) {
        FIXME("pName = `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(Level != 2) {
        WARN("Level = %ld\n", Level);
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
    if(RegOpenKeyA(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) ==
       ERROR_SUCCESS) {
        SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	RegCloseKey(hkeyPrinter);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegOpenKeyA(hkeyDrivers, pi->pDriverName, &hkeyDriver) != 
       ERROR_SUCCESS) {
        WARN("Can't find driver `%s'\n", pi->pDriverName);
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);
    if(strcasecmp(pi->pPrintProcessor, "WinPrint")) {  /* FIXME */
        WARN("Can't find processor `%s'\n", pi->pPrintProcessor);
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegCreateKeyA(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) !=
       ERROR_SUCCESS) {
        WARN("Can't create printer `%s'\n", pi->pPrinterName);
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    RegSetValueExA(hkeyPrinter, "Attributes", 0, REG_DWORD,
		   (LPSTR)&pi->Attributes, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Default DevMode", 0, REG_BINARY,
		   (LPSTR)&pi->pDevMode,
		   pi->pDevMode ? pi->pDevMode->dmSize : 0);
    RegSetValueExA(hkeyPrinter, "Description", 0, REG_SZ, pi->pComment, 0);
    RegSetValueExA(hkeyPrinter, "Location", 0, REG_SZ, pi->pLocation, 0);
    RegSetValueExA(hkeyPrinter, "Name", 0, REG_SZ, pi->pPrinterName, 0);
    RegSetValueExA(hkeyPrinter, "Parameters", 0, REG_SZ, pi->pParameters, 0);
    RegSetValueExA(hkeyPrinter, "Port", 0, REG_SZ, pi->pPortName, 0);
    RegSetValueExA(hkeyPrinter, "Print Processor", 0, REG_SZ,
		   pi->pPrintProcessor, 0);
    RegSetValueExA(hkeyPrinter, "Printer Driver", 0, REG_SZ, pi->pDriverName,
		   0);
    RegSetValueExA(hkeyPrinter, "Priority", 0, REG_DWORD,
		   (LPSTR)&pi->Priority, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Separator File", 0, REG_SZ, pi->pSepFile, 0);
    RegSetValueExA(hkeyPrinter, "Share Name", 0, REG_SZ, pi->pShareName, 0);
    RegSetValueExA(hkeyPrinter, "Start Time", 0, REG_DWORD,
		   (LPSTR)&pi->StartTime, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Status", 0, REG_DWORD,
		   (LPSTR)&pi->Status, sizeof(DWORD));
    RegSetValueExA(hkeyPrinter, "Until Time", 0, REG_DWORD,
		   (LPSTR)&pi->UntilTime, sizeof(DWORD));

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(!OpenPrinterA(pi->pPrinterName, &retval, NULL)) {
        ERR("OpenPrinter failing\n");
	return 0;
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinter32W  [WINSPOOL.122]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    FIXME("(%p,%ld,%p): stub\n", pName, Level, pPrinter);
    return 0;
}


/*****************************************************************************
 *          ClosePrinter32  [WINSPOOL.126]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    LPOPENEDPRINTERA lpOpenedPrinter;

    if ((hPrinter != -1) && (hPrinter < NUM_PRINTER_MAX + 1))
    {
	lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
	HeapFree(GetProcessHeap(), 0, lpOpenedPrinter->lpsPrinterName);
	lpOpenedPrinter->lpsPrinterName = NULL;
	
	/* Free the memory of lpDefault if it has been initialized*/
	if(lpOpenedPrinter->lpDefault != NULL)
	{
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault->pDevMode);
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault->pDatatype);
	    HeapFree(GetProcessHeap(), 0,
		     lpOpenedPrinter->lpDefault);
	    lpOpenedPrinter->lpDefault = NULL;
	}
	
	lpOpenedPrinter->hPrinter = -1;
	
	return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *          DeleteForm32A  [WINSPOOL.133]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteForm32W  [WINSPOOL.134]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%d,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter32  [WINSPOOL.143]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    FIXME("(%d): stub\n", hPrinter);
    return 1;
}

/*****************************************************************************
 *          SetPrinter32A  [WINSPOOL.211]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME("(%d,%ld,%p,%ld): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32A  [WINSPOOL.209]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJob32W  [WINSPOOL.210]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                       LPBYTE pJob, DWORD Command)
{
    FIXME("(%d,%ld,%ld,%p,%ld): stub\n",hPrinter,JobId,Level,pJob,
         Command);
    return FALSE;
}

/*****************************************************************************
 *          GetForm32A  [WINSPOOL.181]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded); 
    return FALSE;
}

/*****************************************************************************
 *          GetForm32W  [WINSPOOL.182]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%s,%ld,%p,%ld,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32A  [WINSPOOL.207]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetForm32W  [WINSPOOL.208]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter32  [WINSPOOL.202]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME("(%d,%p,%ld,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32A  [WINSPOOL.203]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinter32W  [WINSPOOL.204]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%d, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}


/*****************************************************************************
 *    WINSPOOL_GetStringFromRegA
 *
 * Get ValueName from hkey storing result in str.  buflen is space left in str
 */ 
static BOOL WINSPOOL_GetStringFromRegA(HKEY hkey, LPCSTR ValueName, LPSTR ptr,
				       DWORD buflen, DWORD *needed)
{
    DWORD sz = buflen, type;
    LONG ret;

    ret = RegQueryValueExA(hkey, ValueName, 0, &type, ptr, &sz);

    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        ERR("Got ret = %ld\n", ret);
	return FALSE;
    }
    *needed = sz;
    return TRUE;
}


/*****************************************************************************
 *          GetPrinter32A  [WINSPOOL.187]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
			DWORD cbBuf, LPDWORD pcbNeeded)
{
    OPENEDPRINTERA *lpOpenedPrinter;
    DWORD size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters;
    
    TRACE("(%d,%ld,%p,%ld,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer `%s' in registry\n",
	    lpOpenedPrinter->lpsPrinterName);
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 2:
      {
        PRINTER_INFO_2A *pi2 = (PRINTER_INFO_2A *)pPrinter;

        size = sizeof(PRINTER_INFO_2A);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else
	    cbBuf = 0;
	needed = size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Name", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    pi2->pPrinterName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Port", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    pi2->pPortName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Printer Driver", ptr, cbBuf,
				   &size);
	if(size <= cbBuf) {
	    pi2->pDriverName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Default DevMode", ptr, cbBuf,
				   &size);
	if(size <= cbBuf) {
	    pi2->pDevMode = (LPDEVMODEA)ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Print Processor", ptr, cbBuf,
				   &size);
	if(size <= cbBuf) {
	    pi2->pPrintProcessor = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	break;
      }
      
    case 5:
      {
        PRINTER_INFO_5A *pi5 = (PRINTER_INFO_5A *)pPrinter;

        size = sizeof(PRINTER_INFO_5A);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else
	    cbBuf = 0;
	needed = size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Name", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    pi5->pPrinterName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyPrinter, "Port", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    pi5->pPortName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
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

    if(pcbNeeded) *pcbNeeded = needed;
    if(cbBuf) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}


/*****************************************************************************
 *          GetPrinter32W  [WINSPOOL.194]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%ld,%p,%ld,%p): stub\n", hPrinter, Level, pPrinter,
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
    OPENEDPRINTERA *lpOpenedPrinter;
    char DriverName[100];
    DWORD ret, type, size, dw, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    
    TRACE("(%d,%s,%ld,%p,%ld,%p)\n",hPrinter,pEnvironment,
         Level,pDriverInfo,cbBuf, pcbNeeded);

    lpOpenedPrinter = WINSPOOL_GetOpenedPrinterA(hPrinter);
    if(!lpOpenedPrinter) {
        SetLastError(ERROR_INVALID_HANDLE);
	return FALSE;
    }
    if(pEnvironment) {
        FIXME("pEnvironment = `%s'\n", pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(Level < 1 || Level > 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Printers, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyPrinters, lpOpenedPrinter->lpsPrinterName, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer `%s' in registry\n",
	    lpOpenedPrinter->lpsPrinterName);
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }
    size = sizeof(DriverName);
    ret = RegQueryValueExA(hkeyPrinter, "Printer Driver", 0, &type, DriverName,
			   &size);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer `%s'\n",
	    lpOpenedPrinter->lpsPrinterName);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }
    if(RegOpenKeyA(hkeyDrivers, DriverName, &hkeyDriver)
       != ERROR_SUCCESS) {
        ERR("Can't find driver `%s' in registry\n", DriverName);
	RegCloseKey(hkeyDrivers);
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 1:
        size = sizeof(DRIVER_INFO_1A);
	break;
    case 2:
        size = sizeof(DRIVER_INFO_2A);
	break;
    case 3:
        size = sizeof(DRIVER_INFO_3A);
	break;
    default:
        ERR("Invalid level\n");
	return FALSE;
    }

    if(size <= cbBuf) {
        ptr = pDriverInfo + size;
	cbBuf -= size;
    } else
        cbBuf = 0;
    needed = size;

    size = strlen(DriverName) + 1;
    if(size <= cbBuf) {
        cbBuf -= size;
        strcpy(ptr, DriverName);
        if(Level == 1)
	    ((DRIVER_INFO_1A *)pDriverInfo)->pName = ptr;
	else
	    ((DRIVER_INFO_2A *)pDriverInfo)->pName = ptr;    
	ptr += size;
    }
    needed += size;

    if(Level > 1) {
        DRIVER_INFO_2A *di2 = (DRIVER_INFO_2A *)pDriverInfo;

	size = sizeof(dw);
        if(RegQueryValueExA(hkeyDriver, "Version", 0, &type, (PBYTE)&dw,
			    &size) !=
	   ERROR_SUCCESS)
	    WARN("Can't get Version\n");
	else if(cbBuf)
	    di2->cVersion = dw;

	size = strlen("Wine") + 1;  /* FIXME */
	if(size <= cbBuf) {
	    cbBuf -= size;
	    strcpy(ptr, "Wine");
	    di2->pEnvironment = ptr;
	    ptr += size;
	} else 
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Driver", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    di2->pDriverPath = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Data File", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    di2->pDataFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Configuration File", ptr,
				   cbBuf, &size);
	if(size <= cbBuf) {
	    di2->pConfigFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;
    }

    if(Level > 2) {
        DRIVER_INFO_3A *di3 = (DRIVER_INFO_3A *)pDriverInfo;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Help File", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    di3->pHelpFile = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Dependent Files", ptr, cbBuf,
				   &size);
	if(size <= cbBuf) {
	    di3->pDependentFiles = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "Monitor", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    di3->pMonitorName = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;

	WINSPOOL_GetStringFromRegA(hkeyDriver, "DataType", ptr, cbBuf, &size);
	if(size <= cbBuf) {
	    di3->pDefaultDataType = ptr;
	    ptr += size;
	} else
	    cbBuf = 0;
	needed += size;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = needed;
    if(cbBuf) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriver32W  [WINSPOOL.193]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo, 
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%d,%p,%ld,%p,%ld,%p): stub\n",hPrinter,pEnvironment,
          Level,pDriverInfo,cbBuf, pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *       GetPrinterDriverDirectoryA  [WINSPOOL.191]
 */
BOOL WINAPI GetPrinterDriverDirectoryA(LPSTR pName, LPSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", pName, pEnvironment, Level,
	  pDriverDirectory, cbBuf, pcbNeeded);
    if(pName != NULL) {
        FIXME("pName = `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(pEnvironment != NULL) {
        FIXME("pEnvironment = `%s' - unsupported\n", pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(Level != 1)  /* win95 ignores this so we just carry on */
        WARN("Level = %ld - assuming 1\n", Level);
    
    /* FIXME should read from registry */
    needed = GetSystemDirectoryA(pDriverDirectory, cbBuf);
    needed++;
    if(pcbNeeded)
        *pcbNeeded = needed;
    if(needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    return TRUE;
}


/*****************************************************************************
 *       GetPrinterDriverDirectoryW  [WINSPOOL.192]
 */
BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR pName, LPWSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    LPSTR pNameA = NULL, pEnvironmentA = NULL;
    BOOL ret;

    if(pName)
        pNameA = HEAP_strdupWtoA( GetProcessHeap(), 0, pName );
    if(pEnvironment)
        pEnvironmentA = HEAP_strdupWtoA( GetProcessHeap(), 0, pEnvironment );
    ret = GetPrinterDriverDirectoryA( pNameA, pEnvironmentA, Level,
				      pDriverDirectory, cbBuf, pcbNeeded );
    if(pNameA)
        HeapFree( GetProcessHeap(), 0, pNameA );
    if(pEnvironmentA)
        HeapFree( GetProcessHeap(), 0, pEnvironment );

    return ret;
}

/*****************************************************************************
 *          AddPrinterDriver32A  [WINSPOOL.120]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    DRIVER_INFO_3A di3;
    HKEY hkeyDrivers, hkeyName;

    TRACE("(%s,%ld,%p)\n",pName,level,pDriverInfo);

    if(level != 2 && level != 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(pName != NULL) {
        FIXME("pName= `%s' - unsupported\n", pName);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!pDriverInfo) {
        WARN("pDriverInfo == NULL");
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

    if(di3.pEnvironment) {
        FIXME("pEnvironment = `%s'\n", di3.pEnvironment);
	SetLastError(ERROR_INVALID_ENVIRONMENT);
	return FALSE;
    }
    if(RegCreateKeyA(HKEY_LOCAL_MACHINE, Drivers, &hkeyDrivers) !=
       ERROR_SUCCESS) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    if(level == 2) { /* apparently can't overwrite with level2 */
        if(RegOpenKeyA(hkeyDrivers, di3.pName, &hkeyName) == ERROR_SUCCESS) {
	    RegCloseKey(hkeyName);
	    RegCloseKey(hkeyDrivers);
	    WARN("Trying to create existing printer driver `%s'\n", di3.pName);
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
 *          AddPrinterDriver32W  [WINSPOOL.121]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level, 
				   LPBYTE pDriverInfo)
{
    FIXME("(%s,%ld,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}


/*****************************************************************************
 *          PrinterProperties  [WINSPOOL.201]
 *
 *     Displays a dialog to set the properties of the printer.
 *
 * RETURNS 
 *     nonzero on succes or zero on faillure
 *
 * BUGS
 *	   implemented as stub only
 */
BOOL WINAPI PrinterProperties(HWND hWnd,      /* handle to parent window */
                              HANDLE hPrinter /* handle to printer object */
){
    FIXME("(%d,%d): stub\n", hWnd, hPrinter);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


