/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MCI internal functions
 *
 * Copyright 1998/1999 Eric Pouech
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "heap.h"
#include "driver.h"
#include "winemm.h"
#include "selectors.h"
#include "digitalv.h"
#include "options.h"
#include "wine/winbase16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mci);

static	int			MCI_InstalledCount;
static	LPSTR			MCI_lpInstallNames = NULL;

/* First MCI valid device ID (0 means error) */
#define MCI_MAGIC 0x0001

/**************************************************************************
 * 				MCI_GetDriver			[internal]
 */
LPWINE_MCIDRIVER	MCI_GetDriver(UINT16 wDevID) 
{
    LPWINE_MCIDRIVER	wmd = 0;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();

    EnterCriticalSection(&iData->cs);
    for (wmd = iData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
	if (wmd->wDeviceID == wDevID)
	    break;
    }
    LeaveCriticalSection(&iData->cs);
    return wmd;
}

/**************************************************************************
 * 				MCI_GetDriverFromString		[internal]
 */
UINT	MCI_GetDriverFromString(LPCSTR lpstrName)
{
    LPWINE_MCIDRIVER	wmd;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();
    UINT		ret = 0;

    if (!lpstrName)
	return 0;
    
    if (!lstrcmpiA(lpstrName, "ALL"))
	return MCI_ALL_DEVICE_ID;
    
    EnterCriticalSection(&iData->cs);
    for (wmd = iData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
	if (wmd->lpstrElementName && strcmp(wmd->lpstrElementName, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	
	if (wmd->lpstrDeviceType && strcmp(wmd->lpstrDeviceType, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
	
	if (wmd->lpstrAlias && strcmp(wmd->lpstrAlias, lpstrName) == 0) {
	    ret = wmd->wDeviceID;
	    break;
	}
    }
    LeaveCriticalSection(&iData->cs);
    
    return ret;
}

/**************************************************************************
 * 			MCI_MessageToString			[internal]
 */
const char* MCI_MessageToString(UINT16 wMsg)
{
    static char buffer[100];
    
#define CASE(s) case (s): return #s
    
    switch (wMsg) {
	CASE(MCI_BREAK);
	CASE(MCI_CLOSE);
	CASE(MCI_CLOSE_DRIVER);
	CASE(MCI_COPY);
	CASE(MCI_CUE);
	CASE(MCI_CUT);
	CASE(MCI_DELETE);
	CASE(MCI_ESCAPE);
	CASE(MCI_FREEZE);
	CASE(MCI_PAUSE);
	CASE(MCI_PLAY);
	CASE(MCI_GETDEVCAPS);
	CASE(MCI_INFO);
	CASE(MCI_LOAD);
	CASE(MCI_OPEN);
	CASE(MCI_OPEN_DRIVER);
	CASE(MCI_PASTE);
	CASE(MCI_PUT);
	CASE(MCI_REALIZE);
	CASE(MCI_RECORD);
	CASE(MCI_RESUME);
	CASE(MCI_SAVE);
	CASE(MCI_SEEK);
	CASE(MCI_SET);
	CASE(MCI_SPIN);
	CASE(MCI_STATUS);
	CASE(MCI_STEP);
	CASE(MCI_STOP);
	CASE(MCI_SYSINFO);
	CASE(MCI_UNFREEZE);
	CASE(MCI_UPDATE);
	CASE(MCI_WHERE);
	CASE(MCI_WINDOW);
	/* constants for digital video */
	CASE(MCI_CAPTURE);
	CASE(MCI_MONITOR);
	CASE(MCI_RESERVE);
	CASE(MCI_SETAUDIO);
	CASE(MCI_SIGNAL);
	CASE(MCI_SETVIDEO);
	CASE(MCI_QUALITY);
	CASE(MCI_LIST);
	CASE(MCI_UNDO);
	CASE(MCI_CONFIGURE);
	CASE(MCI_RESTORE);
#undef CASE
    default:
	sprintf(buffer, "MCI_<<%04X>>", wMsg);
	return buffer;
    }
}

/**************************************************************************
 * 				MCI_GetDevTypeFromFileName	[internal]
 */
static	DWORD	MCI_GetDevTypeFromFileName(LPCSTR fileName, LPSTR buf, UINT len)
{
    LPSTR	tmp;

    if ((tmp = strrchr(fileName, '.'))) {
	GetProfileStringA("mci extensions", tmp + 1, "*", buf, len);
	if (strcmp(buf, "*") != 0) {
	    return 0;
	}
	TRACE("No [mci extensions] entry for '%s' found.\n", tmp);
    }
    return MCIERR_EXTENSION_NOT_FOUND;
}

#define	MAX_MCICMDTABLE			20
#define MCI_COMMAND_TABLE_NOT_LOADED	0xFFFE

typedef struct tagWINE_MCICMDTABLE {
    HANDLE		hMem;
    UINT		uDevType;
    LPCSTR		lpTable;
    UINT		nVerbs;		/* number of verbs in command table */
    LPCSTR*		aVerbs;		/* array of verbs to speed up the verb look up process */
} WINE_MCICMDTABLE, *LPWINE_MCICMDTABLE;
WINE_MCICMDTABLE	S_MciCmdTable[MAX_MCICMDTABLE];

/**************************************************************************
 * 				MCI_IsCommandTableValid		[internal]
 */
static	BOOL		MCI_IsCommandTableValid(UINT uTbl)
{
    LPCSTR	lmem, str;
    DWORD	flg;
    WORD	eid;
    int		idx = 0;
    BOOL	inCst = FALSE;

    TRACE("Dumping cmdTbl=%d [hMem=%08x devType=%d]\n", 
	  uTbl, S_MciCmdTable[uTbl].hMem, S_MciCmdTable[uTbl].uDevType);

    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].hMem || !S_MciCmdTable[uTbl].lpTable) 
	return FALSE;

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
	do {
	    str = lmem;
	    lmem += strlen(lmem) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    lmem += sizeof(DWORD) + sizeof(WORD);
	    idx ++;
	    /* EPP 	    TRACE("cmd='%s' %08lx %04x\n", str, flg, eid); */
	    switch (eid) {
	    case MCI_COMMAND_HEAD:	if (!*str || !flg) return FALSE; idx = 0;			break;	/* check unicity of str in table */
	    case MCI_STRING:            if (inCst) return FALSE;					break;
	    case MCI_INTEGER:           if (!*str) return FALSE;					break;
	    case MCI_END_COMMAND:       if (*str || flg || idx == 0) return FALSE; idx = 0;		break;
	    case MCI_RETURN:		if (*str || idx != 1) return FALSE;				break;
	    case MCI_FLAG:		if (!*str) return FALSE;					break;
	    case MCI_END_COMMAND_LIST:	if (*str || flg) return FALSE;	idx = 0;			break;
	    case MCI_RECT:		if (!*str || inCst) return FALSE;				break;
	    case MCI_CONSTANT:          if (inCst) return FALSE; inCst = TRUE;				break;
	    case MCI_END_CONSTANT:	if (*str || flg || !inCst) return FALSE; inCst = FALSE;		break;
	    default:			return FALSE;
	    }
	} while (eid != MCI_END_COMMAND_LIST);
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}

/**************************************************************************
 * 				MCI_DumpCommandTable		[internal]
 */
static	BOOL		MCI_DumpCommandTable(UINT uTbl)
{
    LPCSTR	lmem;
    LPCSTR	str;
    DWORD	flg;
    WORD	eid;
    
    if (!MCI_IsCommandTableValid(uTbl)) {
	ERR("Ooops: %d is not valid\n", uTbl);
	return FALSE;
    }

    lmem = S_MciCmdTable[uTbl].lpTable;
    do {
	do {
	    str = lmem;
	    lmem += strlen(lmem) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    TRACE("cmd='%s' %08lx %04x\n", str, flg, eid);
	    lmem += sizeof(DWORD) + sizeof(WORD);
	} while (eid != MCI_END_COMMAND && eid != MCI_END_COMMAND_LIST);
	TRACE(" => end of command%s\n", (eid == MCI_END_COMMAND_LIST) ? " list" : "");
    } while (eid != MCI_END_COMMAND_LIST);
    return TRUE;
}

static	UINT		MCI_SetCommandTable(LPWINE_MM_IDATA iData, HANDLE hMem, UINT uDevType);

/**************************************************************************
 * 				MCI_GetCommandTable		[internal]
 */
static	UINT		MCI_GetCommandTable(LPWINE_MM_IDATA iData, UINT uDevType)
{
    UINT	uTbl;
    char	buf[32];
    LPSTR	str = NULL;
    
    /* first look up existing for existing devType */
    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (S_MciCmdTable[uTbl].hMem && S_MciCmdTable[uTbl].uDevType == uDevType)
	    return uTbl;	
    }

    /* well try to load id */
    if (uDevType >= MCI_DEVTYPE_FIRST && uDevType <= MCI_DEVTYPE_LAST) {
	if (LoadStringA(iData->hWinMM32Instance, uDevType, buf, sizeof(buf))) {
	    str = buf;
	}
    } else if (uDevType == 0) {
	str = "CORE";
    }
    uTbl = MCI_NO_COMMAND_TABLE;
    if (str) {
	HRSRC 	hRsrc = FindResourceA(iData->hWinMM32Instance, str, (LPCSTR)RT_RCDATAA);
	HANDLE	hMem = 0;

	if (hRsrc) hMem = LoadResource(iData->hWinMM32Instance, hRsrc);
	if (hMem) {
	    uTbl = MCI_SetCommandTable(iData, hMem, uDevType);
	} else {
	    WARN("No command table found in resource %04x[%s]\n", 
		 iData->hWinMM32Instance, str);
	}
    }
    TRACE("=> %d\n", uTbl);
    return uTbl;
}

/**************************************************************************
 * 				MCI_SetCommandTable		[internal]
 */
static	UINT		MCI_SetCommandTable(LPWINE_MM_IDATA iData, HANDLE hMem, 
					    UINT uDevType)
{    
    int		        uTbl;
    static	BOOL	bInitDone = FALSE;

    /* <HACK>
     * The CORE command table must be loaded first, so that MCI_GetCommandTable()
     * can be called with 0 as a uDevType to retrieve it.
     * </HACK>
     */
    if (!bInitDone) {
	bInitDone = TRUE;
	for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	    S_MciCmdTable[uTbl].hMem = 0;
	}
	MCI_GetCommandTable(iData, 0);
    }

    for (uTbl = 0; uTbl < MAX_MCICMDTABLE; uTbl++) {
	if (S_MciCmdTable[uTbl].hMem == 0) {
	    LPCSTR 	lmem, str;
	    WORD	eid;
	    WORD	count;

	    S_MciCmdTable[uTbl].hMem = hMem;
	    S_MciCmdTable[uTbl].uDevType = uDevType;
	    S_MciCmdTable[uTbl].lpTable = LockResource(hMem);

	    if (TRACE_ON(mci)) {
		MCI_DumpCommandTable(uTbl);
	    }

	    /* create the verbs table */
	    /* get # of entries */
	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		lmem += strlen(lmem) + 1;
		eid = *(LPWORD)(lmem + sizeof(DWORD));
		lmem += sizeof(DWORD) + sizeof(WORD);
		if (eid == MCI_COMMAND_HEAD)
		    count++;
	    } while (eid != MCI_END_COMMAND_LIST);

	    S_MciCmdTable[uTbl].aVerbs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(LPCSTR));
	    S_MciCmdTable[uTbl].nVerbs = count;

	    lmem = S_MciCmdTable[uTbl].lpTable;
	    count = 0;
	    do {
		str = lmem;
		lmem += strlen(lmem) + 1;
		eid = *(LPWORD)(lmem + sizeof(DWORD));
		lmem += sizeof(DWORD) + sizeof(WORD);
		if (eid == MCI_COMMAND_HEAD)
		    S_MciCmdTable[uTbl].aVerbs[count++] = str;
	    } while (eid != MCI_END_COMMAND_LIST);
	    /* assert(count == S_MciCmdTable[uTbl].nVerbs); */
	    return uTbl;
	}
    }

    return MCI_NO_COMMAND_TABLE;
}

/**************************************************************************
 * 				MCI_DeleteCommandTable		[internal]
 */
static	BOOL	MCI_DeleteCommandTable(UINT uTbl)
{
    if (uTbl >= MAX_MCICMDTABLE || !S_MciCmdTable[uTbl].hMem)
	return FALSE;

    FreeResource16(S_MciCmdTable[uTbl].hMem);
    S_MciCmdTable[uTbl].hMem = 0;
    if (S_MciCmdTable[uTbl].aVerbs) {
	HeapFree(GetProcessHeap(), 0, S_MciCmdTable[uTbl].aVerbs);
	S_MciCmdTable[uTbl].aVerbs = 0;
    }
    return TRUE;
}

/**************************************************************************
 * 				MCI_UnLoadMciDriver		[internal]
 */
static	BOOL	MCI_UnLoadMciDriver(LPWINE_MM_IDATA iData, LPWINE_MCIDRIVER wmd)
{
    LPWINE_MCIDRIVER*		tmp;

    if (!wmd)
	return TRUE;

    if (wmd->hDrv) 
	CloseDriver(wmd->hDrv, 0, 0);

    if (wmd->dwPrivate != 0)
	WARN("Unloading mci driver with non nul dwPrivate field\n");

    EnterCriticalSection(&iData->cs);
    for (tmp = &iData->lpMciDrvs; *tmp; tmp = &(*tmp)->lpNext) {
	if (*tmp == wmd) {
	    *tmp = wmd->lpNext;
	    break;
	}
    }
    LeaveCriticalSection(&iData->cs);

    HeapFree(GetProcessHeap(), 0, wmd->lpstrDeviceType);
    HeapFree(GetProcessHeap(), 0, wmd->lpstrAlias);
    HeapFree(GetProcessHeap(), 0, wmd->lpstrElementName);

    HeapFree(GetProcessHeap(), 0, wmd);
    return TRUE;
}

/**************************************************************************
 * 				MCI_LoadMciDriver		[internal]
 */
static	DWORD	MCI_LoadMciDriver(LPWINE_MM_IDATA iData, LPCSTR _strDevTyp, 
				  LPWINE_MCIDRIVER* lpwmd)
{
    LPSTR			strDevTyp = CharUpperA(HEAP_strdupA(GetProcessHeap(), 0, _strDevTyp));
    LPWINE_MCIDRIVER		wmd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wmd));
    MCI_OPEN_DRIVER_PARMSA	modp;
    DWORD			dwRet = 0;
    HDRVR			hDrv = 0;

    if (!wmd || !strDevTyp) {
	dwRet = MCIERR_OUT_OF_MEMORY;
	goto errCleanUp;
    }

    EnterCriticalSection(&iData->cs);
    /* wmd must be inserted in list before sending opening the driver, coz' it
     * may want to lookup at wDevID
     */
    wmd->lpNext = iData->lpMciDrvs;
    iData->lpMciDrvs = wmd;

    for (modp.wDeviceID = MCI_MAGIC; 
	 MCI_GetDriver(modp.wDeviceID) != 0; 
	 modp.wDeviceID++);

    wmd->wDeviceID = modp.wDeviceID;
    wmd->lpfnYieldProc = MCI_DefYieldProc;
    wmd->dwYieldData = VK_CANCEL;
    wmd->hCreatorTask = GetCurrentTask();

    LeaveCriticalSection(&iData->cs);

    TRACE("wDevID=%04X \n", modp.wDeviceID);

    modp.lpstrParams = NULL;
    
    hDrv = OpenDriverA(strDevTyp, "mci", (LPARAM)&modp);
    
    if (!hDrv) {
	FIXME("Couldn't load driver for type %s.\n"
	      "If you don't have a windows installation accessible from Wine,\n"
	      "you perhaps forgot to create a [mci] section in system.ini\n",
	      strDevTyp);
	dwRet = MCIERR_DEVICE_NOT_INSTALLED;
	goto errCleanUp;
    }				

    /* FIXME: should also check that module's description is of the form
     * MODULENAME:[MCI] comment
     */

    wmd->hDrv = hDrv;
    /* some drivers will return 0x0000FFFF, some others 0xFFFFFFFF */
    wmd->uSpecificCmdTable = LOWORD(modp.wCustomCommandTable);
    wmd->uTypeCmdTable = MCI_COMMAND_TABLE_NOT_LOADED;

    TRACE("Loaded driver %x (%s), type is %d, cmdTable=%08x\n", 
	  hDrv, strDevTyp, modp.wType, modp.wCustomCommandTable);
    
    wmd->lpstrDeviceType = strDevTyp;
    wmd->wType = modp.wType;

    TRACE("mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n", 
	  modp.wDeviceID, modp.wType, modp.wDeviceID);
    *lpwmd = wmd;
    return 0;
errCleanUp:
    MCI_UnLoadMciDriver(iData, wmd);
    HeapFree(GetProcessHeap(), 0, strDevTyp);
    *lpwmd = 0;
    return dwRet;
}
    
/**************************************************************************
 * 			MCI_FinishOpen				[internal]
 */
static	DWORD	MCI_FinishOpen(LPWINE_MCIDRIVER wmd, LPMCI_OPEN_PARMSA lpParms, 
			       DWORD dwParam)
{
    if (dwParam & MCI_OPEN_ELEMENT)
	wmd->lpstrElementName = HEAP_strdupA(GetProcessHeap(), 0, 
					     lpParms->lpstrElementName);

    if (dwParam & MCI_OPEN_ALIAS)
	wmd->lpstrAlias = HEAP_strdupA(GetProcessHeap(), 0, lpParms->lpstrAlias);

    lpParms->wDeviceID = wmd->wDeviceID;

    return MCI_SendCommandFrom32(wmd->wDeviceID, MCI_OPEN_DRIVER, dwParam, 
				 (DWORD)lpParms);
}

/**************************************************************************
 * 				MCI_FindCommand		[internal]
 */
static	LPCSTR		MCI_FindCommand(UINT uTbl, LPCSTR verb)
{
    UINT	idx;

    if (uTbl >= MAX_MCICMDTABLE || S_MciCmdTable[uTbl].hMem == 0)
	return NULL;

    /* another improvement would be to have the aVerbs array sorted,
     * so that we could use a dichotomic search on it, rather than this dumb
     * array look up
     */
    for (idx = 0; idx < S_MciCmdTable[uTbl].nVerbs; idx++) {
	if (strcmp(S_MciCmdTable[uTbl].aVerbs[idx], verb) == 0)
	    return S_MciCmdTable[uTbl].aVerbs[idx];
    }

    return NULL;
}

/**************************************************************************
 * 				MCI_GetReturnType		[internal]
 */
static	DWORD		MCI_GetReturnType(LPCSTR lpCmd)
{
    lpCmd += strlen(lpCmd) + 1 + sizeof(DWORD) + sizeof(WORD);
    if (*lpCmd == '\0' && *(LPWORD)(lpCmd + 1 + sizeof(DWORD)) == MCI_RETURN) {
	return *(LPDWORD)(lpCmd + 1);
    }
    return 0L;
}

/**************************************************************************
 * 				MCI_GetMessage			[internal]
 */
static	WORD		MCI_GetMessage(LPCSTR lpCmd)
{
    return (WORD)*(LPDWORD)(lpCmd + strlen(lpCmd) + 1);
}

/**************************************************************************
 * 				MCI_GetDWord			[internal]
 */
static	BOOL		MCI_GetDWord(LPDWORD data, LPSTR* ptr)
{
    DWORD	val;
    LPSTR	ret;

    val = strtoul(*ptr, &ret, 0);

    switch (*ret) {
    case '\0':	break;
    case ' ':	ret++; break;
    default:	return FALSE;
    }

    *data |= val;
    *ptr = ret;
    return TRUE;
}

/**************************************************************************
 * 				MCI_GetString		[internal]
 */
static	DWORD	MCI_GetString(LPSTR* str, LPSTR* args)
{
    LPSTR	ptr = *args;

    /* see if we have a quoted string */
    if (*ptr == '"') {
	ptr = strchr(*str = ptr + 1, '"');
	if (!ptr) return MCIERR_NO_CLOSING_QUOTE;
	/* FIXME: shall we escape \" from string ?? */
	if (ptr[-1] == '\\') TRACE("Ooops: un-escaped \"\n");
	*ptr++ = '\0'; /* remove trailing " */
	if (*ptr != ' ' && *ptr != '\0') return MCIERR_EXTRA_CHARACTERS;
	*ptr++ = '\0';
    } else {
	ptr = strchr(ptr, ' ');

	if (ptr) {
	    *ptr++ = '\0';
	} else {
	    ptr = *args + strlen(*args);
	}
	*str = *args;
    }

    *args = ptr;
    return 0;
}

#define	MCI_DATA_SIZE	16

/**************************************************************************
 * 				MCI_ParseOptArgs		[internal]
 */
static	DWORD	MCI_ParseOptArgs(LPDWORD data, int _offset, LPCSTR lpCmd, 
				    LPSTR args, LPDWORD dwFlags)
{
    int		len, offset;
    LPCSTR	lmem, str;
    DWORD	dwRet, flg, cflg = 0;
    WORD	eid;
    BOOL	inCst, found;

    /* loop on arguments */
    while (*args) {
	lmem = lpCmd;
	found = inCst = FALSE;
	offset = _offset;
	TRACE("args='%s' offset=%d\n", args, offset);
	    
	do { /* loop on options for command table for the requested verb */
	    str = lmem;
	    lmem += (len = strlen(lmem)) + 1;
	    flg = *(LPDWORD)lmem;
	    eid = *(LPWORD)(lmem + sizeof(DWORD));
	    lmem += sizeof(DWORD) + sizeof(WORD);
/* EPP 	    TRACE("\tcmd='%s' inCst=%s\n", str, inCst ? "Y" : "N"); */
	    
	    switch (eid) {
	    case MCI_CONSTANT:		
		inCst = TRUE;	cflg = flg;	break;
	    case MCI_END_CONSTANT:	
		/* there may be additional integral values after flag in constant */
		if (inCst && MCI_GetDWord(&(data[offset]), &args)) {
		    *dwFlags |= cflg;
		}
		inCst = FALSE;	cflg = 0;	
		break;
	    }

	    if (strncasecmp(args, str, len) == 0 && 
		(args[len] == 0 || args[len] == ' ')) {
		/* store good values into data[] */
		args += len;
		if (*args == ' ') args++;
		found = TRUE;

		switch (eid) {
		case MCI_COMMAND_HEAD:
		case MCI_RETURN:
		case MCI_END_COMMAND:
		case MCI_END_COMMAND_LIST:
		case MCI_CONSTANT: 	/* done above */
		case MCI_END_CONSTANT:  /* done above */
		    break;
		case MCI_FLAG:			
		    *dwFlags |= flg;
		    break;
		case MCI_INTEGER:
		    if (inCst) {
			data[offset] |= flg;
			*dwFlags |= cflg;
			inCst = FALSE;
		    } else {
			*dwFlags |= flg;
			if (!MCI_GetDWord(&(data[offset]), &args)) {
			    return MCIERR_BAD_INTEGER;
			}
		    }
		    break;
		case MCI_RECT:			
		    /* store rect in data (offset...offset+3) */
		    *dwFlags |= flg;
		    if (!MCI_GetDWord(&(data[offset+0]), &args) ||
			!MCI_GetDWord(&(data[offset+1]), &args) ||
			!MCI_GetDWord(&(data[offset+2]), &args) ||
			!MCI_GetDWord(&(data[offset+3]), &args)) {
			ERR("Bad rect '%s'\n", args);
			return MCIERR_BAD_INTEGER;
		    }
		    break;
		case MCI_STRING:
		    *dwFlags |= flg;
		    if ((dwRet = MCI_GetString((LPSTR*)&data[offset], &args)))
			return dwRet;
		    break;
		default:	ERR("oops");
		}
		/* exit inside while loop */
		eid = MCI_END_COMMAND;
	    } else {
		/* have offset incremented if needed */
		switch (eid) {
		case MCI_COMMAND_HEAD:
		case MCI_RETURN:
		case MCI_END_COMMAND:
		case MCI_END_COMMAND_LIST:
		case MCI_CONSTANT:
		case MCI_FLAG:			break;
		case MCI_INTEGER:		if (!inCst) offset++;	break;
		case MCI_END_CONSTANT:		
		case MCI_STRING:		offset++; break;
		case MCI_RECT:			offset += 4; break;
		default:			ERR("oops");
		}
	    }
	} while (eid != MCI_END_COMMAND);
	if (!found) {
	    TRACE("Optarg '%s' not found\n", args);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
	if (offset == MCI_DATA_SIZE) {
	    ERR("Internal data[] buffer overflow\n");
	    return MCIERR_PARSER_INTERNAL;
	}
    }
    return 0;
}

/**************************************************************************
 * 				MCI_HandleReturnValues	[internal]
 */
static	DWORD	MCI_HandleReturnValues(LPWINE_MM_IDATA iData, DWORD dwRet, 
				       LPWINE_MCIDRIVER wmd, LPCSTR lpCmd, LPDWORD data, 
				       LPSTR lpstrRet, UINT uRetLen)
{
    if (lpstrRet) {
	switch (MCI_GetReturnType(lpCmd)) {
	case 0: /* nothing to return */
	    break;
	case MCI_INTEGER:	
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
	    case MCI_INTEGER_RETURNED:
		wsnprintfA(lpstrRet, uRetLen, "%ld", data[1]);
		break;
	    case MCI_RESOURCE_RETURNED:
		/* return string which ID is HIWORD(data[1]), 
		 * string is loaded from mmsystem.dll */
		LoadStringA(iData->hWinMM32Instance, HIWORD(data[1]), 
			    lpstrRet, uRetLen);
		break;
	    case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
		/* return string which ID is HIWORD(data[1]), 
		 * string is loaded from driver */
		LoadStringA(wmd->hDrv, HIWORD(data[1]), lpstrRet, uRetLen);
		break;
	    case MCI_COLONIZED3_RETURN:
		wsnprintfA(lpstrRet, uRetLen, "%d:%d:%d", 
			 LOBYTE(LOWORD(data[1])), HIBYTE(LOWORD(data[1])), 
			 LOBYTE(HIWORD(data[1])));
		break;
	    case MCI_COLONIZED4_RETURN:
		wsnprintfA(lpstrRet, uRetLen, "%d:%d:%d:%d", 
			 LOBYTE(LOWORD(data[1])), HIBYTE(LOWORD(data[1])), 
			 LOBYTE(HIWORD(data[1])), HIBYTE(HIWORD(data[1])));
		break;
	    default:	ERR("Ooops (%04X)\n", HIWORD(dwRet));
	    }
	    break;
	case MCI_STRING:	
	    switch (dwRet & 0xFFFF0000ul) {
	    case 0:
		/* nothing to do data[1] == lpstrRet */
		break;
	    case MCI_INTEGER_RETURNED:
		data[1] = *(LPDWORD)lpstrRet;
		wsnprintfA(lpstrRet, uRetLen, "%ld", data[1]);
		break;
	    default:
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
		break;
	    }
	    break;
	case MCI_RECT:	
	    if (dwRet & 0xFFFF0000ul)	
		WARN("Oooch. MCI_STRING and HIWORD(dwRet)=%04x\n", HIWORD(dwRet));
	    wsnprintfA(lpstrRet, uRetLen, "%ld %ld %ld %ld", 
		       data[1], data[2], data[3], data[4]);	
	    break;
	default:		ERR("oops\n");
	}
    }
    return LOWORD(dwRet);
}

/**************************************************************************
 * 				mciSendStringA		[MMSYSTEM.702][WINMM.51]
 */
DWORD WINAPI mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrRet, 
			    UINT uRetLen, HWND hwndCallback)
{
    LPSTR		verb, dev, args;
    LPWINE_MCIDRIVER	wmd = 0;
    DWORD		dwFlags = 0, dwRet = 0;
    int			offset = 0;
    DWORD		data[MCI_DATA_SIZE];
    LPCSTR		lpCmd = 0;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();

    TRACE("('%s', %p, %d, %X)\n", lpstrCommand, lpstrRet, uRetLen, hwndCallback);

    /* format is <command> <device> <optargs> */
    if (!(verb = HEAP_strdupA(GetProcessHeap(), 0, lpstrCommand)))
	return MCIERR_OUT_OF_MEMORY;

    memset(data, 0, sizeof(data));

    if (!(args = strchr(verb, ' '))) {
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }
    *args++ = '\0';
    if ((dwRet = MCI_GetString(&dev, &args))) {
	goto errCleanUp;
    }

    /* case dev == 'new' has to be handled */
    if (!strcasecmp(dev, "new")) {
	FIXME("'new': NIY as device name\n");
	dwRet = MCIERR_MISSING_DEVICE_NAME;
	goto errCleanUp;
    }

    /* otherwise, try to grab devType from open */
    if (!strcmp(verb, "open")) {
	LPSTR	devType, tmp;

	if ((devType = strchr(dev, '!')) != NULL) {
	    *devType++ = '\0';	    
	    tmp = devType; devType = dev; dev = tmp;

	    dwFlags |= MCI_OPEN_TYPE;
	    data[2] = (DWORD)devType;
	    devType = CharUpperA(HEAP_strdupA(GetProcessHeap(), 0, devType));
	} else {
	    if ((devType = strstr(args, "type ")) != NULL) {
		devType += 5;
		tmp = strchr(devType, ' ');
		if (tmp) *tmp = '\0';
		devType = CharUpperA(HEAP_strdupA(GetProcessHeap(), 0, devType));
		if (tmp) *tmp = ' ';
		/* dwFlags and data[2] will be correctly set in ParseOpt loop */
	    } else {
		char	buf[32];
		if ((dwRet = MCI_GetDevTypeFromFileName(dev, buf, sizeof(buf))))
		    goto errCleanUp;

		devType = CharUpperA(HEAP_strdupA(GetProcessHeap(), 0, buf));
	    }
	    dwFlags |= MCI_OPEN_ELEMENT;
	    data[3] = (DWORD)dev;
	}
	dwRet = MCI_LoadMciDriver(iData, devType, &wmd);
	HeapFree(GetProcessHeap(), 0, devType);
	if (dwRet) {
	    MCI_UnLoadMciDriver(iData, wmd);
	    goto errCleanUp;
	}
    } else {
	wmd = MCI_GetDriver(mciGetDeviceIDA(dev));
	if (!wmd) {
	    FIXME("Oooch: couldn't find driver for '%s' in '%s'; automatic open is NIY\n", 
		  dev, lpstrCommand);
	    dwRet = MCIERR_INVALID_DEVICE_ID;
	    goto errCleanUp;
	}
    }

    /* get the verb in the different command tables */
    /* try the device specific command table */
    if (wmd) lpCmd = MCI_FindCommand(wmd->uSpecificCmdTable, verb);
    /* try the type specific command table */
    if (!lpCmd) {
	if (wmd && wmd->uTypeCmdTable == MCI_COMMAND_TABLE_NOT_LOADED)
	    wmd->uTypeCmdTable = MCI_GetCommandTable(iData, wmd->wType);
	if (wmd && wmd->uTypeCmdTable != MCI_NO_COMMAND_TABLE)
	    lpCmd = MCI_FindCommand(wmd->uTypeCmdTable, verb);
    }
    /* try core command table */
    if (!lpCmd) lpCmd = MCI_FindCommand(MCI_GetCommandTable(iData, 0), verb);

    if (!lpCmd) {
	TRACE("Command '%s' not found!\n", verb);
	dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	goto errCleanUp;
    }

    /* set up call back */
    if (hwndCallback != 0) {
	dwFlags |= MCI_NOTIFY;
	data[0] = (DWORD)hwndCallback;
    }

    /* set return information */
    switch (MCI_GetReturnType(lpCmd)) {
    case 0:		offset = 1;	break;
    case MCI_INTEGER:	offset = 2;	break;
    case MCI_STRING:	data[1] = (DWORD)lpstrRet; data[2] = uRetLen; offset = 3; break;
    case MCI_RECT:	offset = 5;	break;
    default:	ERR("oops\n");
    }

    TRACE("verb='%s' on dev='%s'; offset=%d\n", verb, dev, offset);

    if ((dwRet = MCI_ParseOptArgs(data, offset, lpCmd, args, &dwFlags)))
	goto errCleanUp;

    if (lpstrRet && uRetLen) *lpstrRet = '\0';

#define	STR_OF(_x) (IsBadReadPtr((char*)_x,1)?"?":(char*)(_x))
    TRACE("[%d, %s, %08lx, %08lx/%s %08lx/%s %08lx/%s %08lx/%s %08lx/%s %08lx/%s]\n",
	  wmd->wDeviceID, MCI_MessageToString(MCI_GetMessage(lpCmd)), dwFlags, 
	  data[0], STR_OF(data[0]), data[1], STR_OF(data[1]),
	  data[2], STR_OF(data[2]), data[3], STR_OF(data[3]),
	  data[4], STR_OF(data[4]), data[5], STR_OF(data[5]));
#undef STR_OF

    if (strcmp(verb, "open") == 0) {
	if ((dwRet = MCI_FinishOpen(wmd, (LPMCI_OPEN_PARMSA)data, dwFlags)))
	    MCI_UnLoadMciDriver(iData, wmd);
	/* FIXME: notification is not properly shared across two opens */
    } else {
	dwRet = MCI_SendCommand(wmd->wDeviceID, MCI_GetMessage(lpCmd), dwFlags, (DWORD)data, TRUE);
    }
    TRACE("=> 1/ %lx (%s)\n", dwRet, lpstrRet);
    if (dwRet)	goto errCleanUp;
    
    dwRet = MCI_HandleReturnValues(iData, dwRet, wmd, lpCmd, data, lpstrRet, uRetLen);
    TRACE("=> 2/ %lx (%s)\n", dwRet, lpstrRet);

errCleanUp:
    HeapFree(GetProcessHeap(), 0, verb);
    return dwRet;
}

/**************************************************************************
 * 				mciSendStringW			[WINMM.52]
 */
DWORD WINAPI mciSendStringW(LPCWSTR lpwstrCommand, LPSTR lpstrRet, 
			    UINT uRetLen, HWND hwndCallback)
{
    LPSTR 	lpstrCommand;
    UINT	ret;

    /* FIXME: is there something to do with lpstrReturnString ? */
    lpstrCommand = HEAP_strdupWtoA(GetProcessHeap(), 0, lpwstrCommand);
    ret = mciSendStringA(lpstrCommand, lpstrRet, uRetLen, hwndCallback);
    HeapFree(GetProcessHeap(), 0, lpstrCommand);
    return ret;
}

/**************************************************************************
 * 				mciSendString16			[MMSYSTEM.702]
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrRet, 
			     UINT16 uRetLen, HWND16 hwndCallback)
{
    return mciSendStringA(lpstrCommand, lpstrRet, uRetLen, hwndCallback);
}

/**************************************************************************
 * 				mciExecute			[WINMM.38]
 */
DWORD WINAPI mciExecute(LPCSTR lpstrCommand)
{
    char	strRet[256];
    DWORD	ret;

    TRACE("(%s)!\n", lpstrCommand);

    ret = mciSendString16(lpstrCommand, strRet, sizeof(strRet), 0);
    if (ret != 0) {
	if (!mciGetErrorStringA(ret, strRet, sizeof(strRet))) {
	    sprintf(strRet, "Unknown MCI error (%ld)", ret);
	}
	MessageBoxA(0, strRet, "Error in mciExecute()", MB_OK); 
    }
    /* FIXME: what shall I return ? */
    return 0;
}

/**************************************************************************
 *                    	mciLoadCommandResource			[MMSYSTEM.705]
 */
UINT16 WINAPI mciLoadCommandResource16(HANDLE16 hInst, LPCSTR resname, UINT16 type)
{
    HRSRC	        hRsrc = 0;
    HGLOBAL      	hMem;
    UINT16		ret = MCI_NO_COMMAND_TABLE;
    LPWINE_MM_IDATA 	iData = MULTIMEDIA_GetIData();

    TRACE("(%04x, %s, %d)!\n", hInst, resname, type);

    /* if file exists "resname.mci", then load resource "resname" from it
     * otherwise directly from driver
     * We don't support it (who uses this feature ?), but we check anyway
     */
    if (!type) {
	char		buf[128];
	OFSTRUCT       	ofs;

	strcat(strcpy(buf, resname), ".mci");
	if (OpenFile(buf, &ofs, OF_EXIST) != HFILE_ERROR) {
	    FIXME("NIY: command table to be loaded from '%s'\n", ofs.szPathName);
	}
    }
    if (!(hRsrc = FindResourceA(hInst, resname, (LPCSTR)RT_RCDATAA))) {
	WARN("No command table found in resource\n");
    } else if ((hMem = LoadResource(hInst, hRsrc))) {
	ret = MCI_SetCommandTable(iData, hMem, type);
    } else {
	WARN("Couldn't load resource.\n");
    }
    TRACE("=> %04x\n", ret);
    return ret;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[MMSYSTEM.713]
 */
BOOL16 WINAPI mciFreeCommandResource16(UINT16 uTable)
{
    TRACE("(%04x)!\n", uTable);

    return MCI_DeleteCommandTable(uTable);
}
 
/**************************************************************************
 *                    	mciLoadCommandResource  		[WINMM.48]
 *
 * Strangely, this function only exists as an UNICODE one.
 */
UINT WINAPI mciLoadCommandResource(HINSTANCE hinst, LPCWSTR resNameW, UINT type)
{
    LPSTR 	resNameA;
    UINT	ret;

    TRACE("(%04x, %s, %d)!\n", hinst, debugstr_w(resNameW), type);

    resNameA = HEAP_strdupWtoA(GetProcessHeap(), 0, resNameW);
    ret = mciLoadCommandResource16(hinst, resNameA, type);
    HeapFree(GetProcessHeap(), 0, resNameA);
    return ret;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[WINMM.39]
 */
BOOL WINAPI mciFreeCommandResource(UINT uTable)
{
    TRACE("(%08x)!\n", uTable);

    return mciFreeCommandResource16(uTable);
}

typedef enum {
    MCI_MAP_NOMEM, 	/* ko, memory problem */
    MCI_MAP_MSGERROR, 	/* ko, unknown message */
    MCI_MAP_OK, 	/* ok, no memory allocated. to be sent to the proc. */
    MCI_MAP_OKMEM, 	/* ok, some memory allocated, need to call UnMapMsg. to be sent to the proc. */
    MCI_MAP_PASS	/* not handled (no memory allocated) to be sent to the driver */
} MCI_MapType;

/**************************************************************************
 * 			MCI_MapMsg16To32A			[internal]
 */
static	MCI_MapType	MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    if (*lParam == 0)
	return MCI_MAP_OK;
    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:		
    case MCI_CLOSE_DRIVER:	
	/* case MCI_CONFIGURE:*/
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:		
    case MCI_PLAY:		
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:		
    case MCI_RESUME:		
    case MCI_SEEK:		
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:		/* FIXME: is wrong for digital video */
    case MCI_STEP:
    case MCI_STOP:		
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	*lParam = (DWORD)PTR_SEG_TO_LIN(*lParam);
	return MCI_MAP_OK;
    case MCI_WINDOW:
	/* in fact, I would also need the dwFlags... to see 
	 * which members of lParam are effectively used 
	 */
	*lParam = (DWORD)PTR_SEG_TO_LIN(*lParam);
	FIXME("Current mapping may be wrong\n");
	break;
    case MCI_BREAK:
	{
            LPMCI_BREAK_PARMS		mbp32 = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_BREAK_PARMS));
	    LPMCI_BREAK_PARMS16		mbp16 = PTR_SEG_TO_LIN(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->nVirtKey = mbp16->nVirtKey;
		mbp32->hwndBreak = mbp16->hwndBreak;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return MCI_MAP_OKMEM;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSA	mvep32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_VD_ESCAPE_PARMSA));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = PTR_SEG_TO_LIN(*lParam);

	    if (mvep32a) {
		mvep32a->dwCallback       = mvep16->dwCallback;
		mvep32a->lpstrCommand     = PTR_SEG_TO_LIN(mvep16->lpstrCommand);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mvep32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_INFO_PARMSA));
	    LPMCI_INFO_PARMS16	mip16  = PTR_SEG_TO_LIN(*lParam);

	    /* FIXME this is wrong if device is of type 
	     * MCI_DEVTYPE_DIGITAL_VIDEO, some members are not mapped
	     */
	    if (mip32a) {
		mip32a->dwCallback  = mip16->dwCallback;
		mip32a->lpstrReturn = PTR_SEG_TO_LIN(mip16->lpstrReturn);
		mip32a->dwRetSize   = mip16->dwRetSize;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mip32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	{
            LPMCI_OPEN_PARMSA	mop32a = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_OPEN_PARMS16) + sizeof(MCI_OPEN_PARMSA) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16  = PTR_SEG_TO_LIN(*lParam);

	    if (mop32a) {
		*(LPMCI_OPEN_PARMS16*)(mop32a) = mop16;
		mop32a = (LPMCI_OPEN_PARMSA)((char*)mop32a + sizeof(LPMCI_OPEN_PARMS16));
		mop32a->dwCallback       = mop16->dwCallback;
		mop32a->wDeviceID        = mop16->wDeviceID;
		mop32a->lpstrDeviceType  = PTR_SEG_TO_LIN(mop16->lpstrDeviceType);
		mop32a->lpstrElementName = PTR_SEG_TO_LIN(mop16->lpstrElementName);
		mop32a->lpstrAlias       = PTR_SEG_TO_LIN(mop16->lpstrAlias);
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and 
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the 
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop32a + 1, mop16 + 1, 2 * sizeof(DWORD));
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mop32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_SYSINFO_PARMSA));
	    LPMCI_SYSINFO_PARMS16	msip16  = PTR_SEG_TO_LIN(*lParam);

	    if (msip32a) {
		msip32a->dwCallback       = msip16->dwCallback;
		msip32a->lpstrReturn      = PTR_SEG_TO_LIN(msip16->lpstrReturn);
		msip32a->dwRetSize        = msip16->dwRetSize;
		msip32a->dwNumber         = msip16->dwNumber;
		msip32a->wDeviceType      = msip16->wDeviceType;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)msip32a;
	}
	return MCI_MAP_OKMEM;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return MCI_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
    }
    return MCI_MAP_MSGERROR;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32A			[internal]
 */
static	MCI_MapType	MCI_UnMapMsg16To32A(WORD uDevType, WORD wMsg, DWORD lParam)
{
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:	
	/* case MCI_CONFIGURE:*/
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:		
    case MCI_PLAY:		
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:		
    case MCI_RESUME:		
    case MCI_SEEK:		
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:		
    case MCI_STEP:
    case MCI_STOP:		
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	return MCI_MAP_OK;

    case MCI_WINDOW:
	/* FIXME ?? see Map function */
	return MCI_MAP_OK;

    case MCI_BREAK:
    case MCI_ESCAPE:
    case MCI_INFO:
    case MCI_SYSINFO:
	HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
	return MCI_MAP_OK;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)lParam;
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)((char*)mop32a - sizeof(LPMCI_OPEN_PARMS16));
	    
	    mop16->wDeviceID = mop32a->wDeviceID;
	    if (!HeapFree(GetProcessHeap(), 0, (LPVOID)(lParam - sizeof(LPMCI_OPEN_PARMS16))))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return MCI_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
    }
    return MCI_MAP_MSGERROR;
}

/*
 * 0000 stop
 * 0001 squeeze   signed 4 bytes to 2 bytes     *( LPINT16)D = ( INT16)*( LPINT16)S; D += 2;     S += 4
 * 0010 squeeze unsigned 4 bytes to 2 bytes     *(LPUINT16)D = (UINT16)*(LPUINT16)S; D += 2;     S += 4
 * 0100
 * 0101 
 * 0110 zero 4 bytes                            *(DWORD)D = 0                        D += 4;     S += 4
 * 0111 copy string                             *(LPSTR*)D = seg dup(*(LPSTR*)S)     D += 4;     S += 4
 * 1xxx copy xxx + 1 bytes                      memcpy(D, S, xxx + 1);               D += xxx+1; S += xxx+1
 */

/**************************************************************************
 * 			MCI_MsgMapper32To16_Create		[internal]
 *
 * Helper for MCI_MapMsg32ATo16. 
 * Maps the 32 bit pointer (*ptr), of size bytes, to an allocated 16 bit 
 * segmented pointer.
 * map contains a list of action to be performed for the mapping (see list
 * above)
 * if keep is TRUE, keeps track of in 32 bit ptr in allocated 16 bit area.
 */
static	MCI_MapType	MCI_MsgMapper32To16_Create(void** ptr, int size16, DWORD map, BOOLEAN keep)
{
    void*	lp = SEGPTR_ALLOC((keep ? sizeof(void**) : 0) + size16);
    LPBYTE	p16, p32;

    if (!lp) {
	return MCI_MAP_NOMEM;
    }
    p32 = (LPBYTE)(*ptr);
    if (keep) {
	*(void**)lp = *ptr;
	p16 = (LPBYTE)lp + sizeof(void**);
	*ptr = (char*)SEGPTR_GET(lp) + sizeof(void**);
    } else {
	p16 = lp;
	*ptr = (void*)SEGPTR_GET(lp);
    }
    
    if (map == 0) {
	memcpy(p16, p32, size16);
    } else {
	unsigned	nibble;
	unsigned	sz;

	while (map & 0xF) {
	    nibble = map & 0xF;
	    if (nibble & 0x8) {
		sz = (nibble & 7) + 1;
		memcpy(p16, p32, sz);
		p16 += sz;
		p32 += sz;
		size16 -= sz;	/* DEBUG only */
	    } else {
		switch (nibble) {
		case 0x1:	*( LPINT16)p16 = ( INT16)*( LPINT16)p32;		p16 += 2;	p32 += 4;	size16 -= 2;	break;
		case 0x2:	*(LPUINT16)p16 = (UINT16)*(LPUINT16)p32;		p16 += 2;     	p32 += 4;	size16 -= 2;	break;
		case 0x6:	*(LPDWORD)p16 = 0;					p16 += 4;     	p32 += 4;	size16 -= 4;	break;
		case 0x7:	*(LPDWORD)p16 = SEGPTR_GET(SEGPTR_STRDUP(*(LPSTR*)p32));p16 += 4;	p32 += 4;	size16 -= 4;	break;
		default:	FIXME("Unknown nibble for mapping (%x)\n", nibble);
		}
	    }
	    map >>= 4;
	}
	if (size16 != 0) /* DEBUG only */
	    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
    }
    return MCI_MAP_OKMEM;
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Destroy		[internal]
 *
 * Helper for MCI_UnMapMsg32ATo16. 
 */
static	MCI_MapType	MCI_MsgMapper32To16_Destroy(void* ptr, int size16, DWORD map, BOOLEAN kept)
{
    if (ptr) {
	void*		msg16 = PTR_SEG_TO_LIN(ptr);
	void*		alloc;
	LPBYTE		p32, p16;
	unsigned	nibble;

	if (kept) {
	    alloc = (char*)msg16 - sizeof(void**);
	    p32 = *(void**)alloc;
	    p16 = msg16;

	    if (map == 0) {
		memcpy(p32, p16, size16);
	    } else {
		while (map & 0xF) {
		    nibble = map & 0xF;
		    if (nibble & 0x8) {
			memcpy(p32, p16, (nibble & 7) + 1);
			p16 += (nibble & 7) + 1;
			p32 += (nibble & 7) + 1;
			size16 -= (nibble & 7) + 1;
		    } else {
			switch (nibble) {
			case 0x1:	*( LPINT)p32 = *( LPINT16)p16;				p16 += 2;	p32 += 4;	size16 -= 2;	break;
			case 0x2:	*(LPUINT)p32 = *(LPUINT16)p16;				p16 += 2;     	p32 += 4;	size16 -= 2;	break;
			case 0x6:								p16 += 4;     	p32 += 4;	size16 -= 4;	break;
			case 0x7:	strcpy(*(LPSTR*)p32, PTR_SEG_TO_LIN(*(DWORD*)p16));
			    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(*(DWORD*)p16))) {
				FIXME("bad free line=%d\n", __LINE__);
			    }
			    p16 += 4;	p32 += 4;	size16 -= 4;	break;
			default:	FIXME("Unknown nibble for mapping (%x)\n", nibble);
			}
		    }
		    map >>= 4;
		}
		if (size16 != 0) /* DEBUG only */
		    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
	    }
	} else {
	    alloc = msg16;
	}

	if (!SEGPTR_FREE(alloc)) {
	    FIXME("bad free line=%d\n", __LINE__);
	} 
    }	
    return MCI_MAP_OK;
}

/**************************************************************************
 * 			MCI_MapMsg32ATo16			[internal]
 *
 * Map a 32-A bit MCI message to a 16 bit MCI message.
 */
static	MCI_MapType	MCI_MapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD* lParam)
{
    int		size;
    BOOLEAN     keep = FALSE;
    DWORD	map = 0;

    if (*lParam == 0)
	return MCI_MAP_OK;

    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
    case MCI_BREAK:
	size = sizeof(MCI_BREAK_PARMS);
	break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:	
	size = sizeof(MCI_GENERIC_PARMS);	
	break;
	/* case MCI_CONFIGURE:*/
	/* case MCI_COPY: */
    case MCI_CUE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_CUE_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_CUE_PARMS);	break;*/	FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_CUT:*/
    case MCI_DELETE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_DELETE_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_DELETE_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_FREEZE_PARMS);	map = 0x0001111B; 	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_GETDEVCAPS:
	keep = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = (LPMCI_INFO_PARMSA)(*lParam);
	    char*		ptr;
	    LPMCI_INFO_PARMS16	mip16;
	    
	    switch (uDevType) {
	    case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_INFO_PARMS16);	break;
	    default:				size = sizeof(MCI_INFO_PARMS16);	break;
	    }
	    ptr = SEGPTR_ALLOC(sizeof(LPMCI_INFO_PARMSA) + size);

	    if (ptr) {
		*(LPMCI_INFO_PARMSA*)ptr = mip32a;
		mip16 = (LPMCI_INFO_PARMS16)(ptr + sizeof(LPMCI_INFO_PARMSA));
		mip16->dwCallback  = mip32a->dwCallback;
		mip16->lpstrReturn = (LPSTR)SEGPTR_GET(SEGPTR_ALLOC(mip32a->dwRetSize));
		mip16->dwRetSize   = mip32a->dwRetSize;
		if (uDevType == MCI_DEVTYPE_DIGITAL_VIDEO) {
		    ((LPMCI_DGV_INFO_PARMS16)mip16)->dwItem = ((LPMCI_DGV_INFO_PARMSA)mip32a)->dwItem;
		}
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_INFO_PARMSA);
	}
	return MCI_MAP_OKMEM;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	{
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)(*lParam);
	    char*		ptr    = SEGPTR_ALLOC(sizeof(LPMCI_OPEN_PARMSA) + sizeof(MCI_OPEN_PARMS16) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16;


	    if (ptr) {
		*(LPMCI_OPEN_PARMSA*)(ptr) = mop32a;
		mop16 = (LPMCI_OPEN_PARMS16)(ptr + sizeof(LPMCI_OPEN_PARMSA));
		mop16->dwCallback       = mop32a->dwCallback;
		mop16->wDeviceID        = mop32a->wDeviceID;
		if (dwFlags & MCI_OPEN_TYPE) {
		    if (dwFlags & MCI_OPEN_TYPE_ID) {
			/* dword "transparent" value */
			mop16->lpstrDeviceType = mop32a->lpstrDeviceType;
		    } else {
			/* string */
			mop16->lpstrDeviceType = mop32a->lpstrDeviceType ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrDeviceType)) : 0;
		    }
		} else {
		    /* nuthin' */
		    mop16->lpstrDeviceType = 0;
		}
		if (dwFlags & MCI_OPEN_ELEMENT) {
		    if (dwFlags & MCI_OPEN_ELEMENT_ID) {
			mop16->lpstrElementName = mop32a->lpstrElementName;
		    } else {
			mop16->lpstrElementName = mop32a->lpstrElementName ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrElementName)) : 0;
		    }
		} else {
		    mop16->lpstrElementName = 0;
		}
		if (dwFlags & MCI_OPEN_ALIAS) {
		    mop16->lpstrAlias = mop32a->lpstrAlias ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrAlias)) : 0;
		} else {
		    mop16->lpstrAlias = 0;
		}
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and 
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the 
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop16 + 1, mop32a + 1, 2 * sizeof(DWORD));
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_OPEN_PARMSA);
	}
	return MCI_MAP_OKMEM;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_PLAY:
	size = sizeof(MCI_PLAY_PARMS);
	break;
    case MCI_PUT:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_REALIZE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_RECORD:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECORD_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_RECORD_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_RECORD_PARMS);	break;
	}
	break;
    case MCI_RESUME:		
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_SEEK:
	switch (uDevType) {
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SEEK_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_SEEK_PARMS);		break;
	}
	break;
    case MCI_SET:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SET_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SET_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	case MCI_DEVTYPE_SEQUENCER:	size = sizeof(MCI_SEQ_SET_PARMS);	break;
        /* FIXME: normally the 16 and 32 bit structures are byte by byte aligned, 
	 * so not doing anything should work...
	 */
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_SET_PARMS);	break;
	default:			size = sizeof(MCI_SET_PARMS);		break;
	}
	break;	
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	size = sizeof(MCI_SET_PARMS);		
	break;
    case MCI_STATUS:
	keep = TRUE;
	switch (uDevType) {
	/* FIXME:
	 * don't know if buffer for value is the one passed thru lpstrDevice 
	 * or is provided by MCI driver.
	 * Assuming solution 2: provided by MCI driver, so zeroing on entry
	 */
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	map = 0x0B6FF;		break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STEP_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STEP_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	case MCI_DEVTYPE_VIDEODISC:	size = sizeof(MCI_VD_STEP_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;	
    case MCI_STOP:		
	size = sizeof(MCI_SET_PARMS);		
	break;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = (LPMCI_SYSINFO_PARMSA)(*lParam);
	    char*		      	ptr     = SEGPTR_ALLOC(sizeof(LPMCI_SYSINFO_PARMSA) + sizeof(MCI_SYSINFO_PARMS16));
	    LPMCI_SYSINFO_PARMS16	msip16;

	    if (ptr) {
		*(LPMCI_SYSINFO_PARMSA*)(ptr) = msip32a;
		msip16 = (LPMCI_SYSINFO_PARMS16)(ptr + sizeof(LPMCI_SYSINFO_PARMSA));

		msip16->dwCallback       = msip32a->dwCallback;
		msip16->lpstrReturn      = (LPSTR)SEGPTR_GET(SEGPTR_ALLOC(msip32a->dwRetSize));
		msip16->dwRetSize        = msip32a->dwRetSize;
		msip16->dwNumber         = msip32a->dwNumber;
		msip16->wDeviceType      = msip32a->wDeviceType;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_SYSINFO_PARMSA);
	}
	return MCI_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_UPDATE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_UPDATE_PARMS16);	map = 0x000B1111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7FB;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7FB;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	return MCI_MAP_PASS;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
	return MCI_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Create((void**)lParam, size, map, keep);
}

/**************************************************************************
 * 			MCI_UnMapMsg32ATo16			[internal]
 */
static	MCI_MapType	MCI_UnMapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD lParam)
{
    int		size = 0;
    BOOLEAN     kept = FALSE;	/* there is no need to compute size when kept is FALSE */
    DWORD 	map = 0;

    switch (wMsg) {
    case MCI_BREAK:
        break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:	
    case MCI_CLOSE_DRIVER:	
	break;
	/* case MCI_CONFIGURE:*/
	/* case MCI_COPY: */
    case MCI_CUE:	
	break;
	/* case MCI_CUT: */
    case MCI_DELETE:
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	break;
    case MCI_GETDEVCAPS:
	kept = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMS16	mip16  = (LPMCI_INFO_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_INFO_PARMSA	mip32a = *(LPMCI_INFO_PARMSA*)((char*)mip16 - sizeof(LPMCI_INFO_PARMSA));

	    memcpy(mip32a->lpstrReturn, PTR_SEG_TO_LIN(mip16->lpstrReturn), mip32a->dwRetSize);

	    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(mip16->lpstrReturn)))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE((char*)mip16 - sizeof(LPMCI_INFO_PARMSA)))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMS16	mop16  = (LPMCI_OPEN_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_OPEN_PARMSA	mop32a = *(LPMCI_OPEN_PARMSA*)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA));
	    
	    mop32a->wDeviceID = mop16->wDeviceID;
	    if ((dwFlags & MCI_OPEN_TYPE) && ! 
		(dwFlags & MCI_OPEN_TYPE_ID) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrDeviceType)))
		FIXME("bad free line=%d\n", __LINE__);
	    if ((dwFlags & MCI_OPEN_ELEMENT) && 
		!(dwFlags & MCI_OPEN_ELEMENT_ID) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrElementName)))
		FIXME("bad free line=%d\n", __LINE__);
	    if ((dwFlags & MCI_OPEN_ALIAS) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrAlias)))
		FIXME("bad free line=%d\n", __LINE__);

	    if (!SEGPTR_FREE((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA)))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	break;
    case MCI_PLAY:		
	break;
    case MCI_PUT:
	break;
    case MCI_REALIZE:
	break;
    case MCI_RECORD:
	break;
    case MCI_RESUME:
	break;
    case MCI_SEEK:
	break;
    case MCI_SET:
	break;
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	break;
    case MCI_STATUS:
	kept = TRUE;
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	
	if (lParam) {
            LPMCI_DGV_STATUS_PARMS16	mdsp16  = (LPMCI_DGV_STATUS_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_DGV_STATUS_PARMSA	mdsp32a = *(LPMCI_DGV_STATUS_PARMSA*)((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA));

	    if (mdsp16) {
		mdsp32a->dwReturn = mdsp16->dwReturn;
		if (dwFlags & MCI_DGV_STATUS_DISKSPACE) {
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%p\n", mdsp16->lpstrDrive);
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%s\n", (LPSTR)PTR_SEG_TO_LIN(mdsp16->lpstrDrive));

		    /* FIXME: see map function */
		    strcpy(mdsp32a->lpstrDrive, (LPSTR)PTR_SEG_TO_LIN(mdsp16->lpstrDrive));
		}

		if (!SEGPTR_FREE((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA)))
		    FIXME("bad free line=%d\n", __LINE__);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	}
	return MCI_MAP_OKMEM;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	break;
    case MCI_STOP:
	break;
    case MCI_SYSINFO:
	if (lParam) {
            LPMCI_SYSINFO_PARMS16	msip16  = (LPMCI_SYSINFO_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_SYSINFO_PARMSA	msip32a = *(LPMCI_SYSINFO_PARMSA*)((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA));

	    if (msip16) {
		msip16->dwCallback       = msip32a->dwCallback;
		memcpy(msip32a->lpstrReturn, PTR_SEG_TO_LIN(msip16->lpstrReturn), msip32a->dwRetSize);
		if (!SEGPTR_FREE(PTR_SEG_TO_LIN(msip16->lpstrReturn)))
		    FIXME("bad free line=%d\n", __LINE__);

		if (!SEGPTR_FREE((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA)))
		    FIXME("bad free line=%d\n", __LINE__);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	}
	return MCI_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	break;
    case MCI_UPDATE:
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	default:			break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7666;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7666;	break;
	default:			break;
	}
	/* FIXME: see map function */
	break;

    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return MCI_MAP_PASS;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
	return MCI_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Destroy((void*)lParam, size, map, kept);
}

/**************************************************************************
 * 			MCI_SendCommandFrom32			[internal]
 */
DWORD MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet = MCIERR_DEVICE_NOT_INSTALLED;
    LPWINE_MCIDRIVER	wmd = MCI_GetDriver(wDevID);

    if (!wmd) {
	dwRet = MCIERR_INVALID_DEVICE_ID;
    } else {
	switch (GetDriverFlags(wmd->hDrv) & (WINE_GDF_EXIST|WINE_GDF_16BIT)) {
	case WINE_GDF_EXIST|WINE_GDF_16BIT:
	    {
		MCI_MapType	res;
		
		switch (res = MCI_MapMsg32ATo16(wmd->wType, wMsg, dwParam1, &dwParam2)) {
		case MCI_MAP_MSGERROR:
		    TRACE("Not handled yet (%s)\n", MCI_MessageToString(wMsg));
		    dwRet = MCIERR_DRIVER_INTERNAL;
		    break;
		case MCI_MAP_NOMEM:
		    TRACE("Problem mapping msg=%s from 32a to 16\n", MCI_MessageToString(wMsg));
		    dwRet = MCIERR_OUT_OF_MEMORY;
		    break;
		case MCI_MAP_OK:
		case MCI_MAP_OKMEM:
		    dwRet = SendDriverMessage16(wmd->hDrv, wMsg, dwParam1, dwParam2);
		    if (res == MCI_MAP_OKMEM)
			MCI_UnMapMsg32ATo16(wmd->wType, wMsg, dwParam1, dwParam2);
		    break;
		case MCI_MAP_PASS:
		    dwRet = SendDriverMessage(wmd->hDrv, wMsg, dwParam1, dwParam2);
		    break;
		}
	    }
	    break;
	case WINE_GDF_EXIST:
	    dwRet = SendDriverMessage(wmd->hDrv, wMsg, dwParam1, dwParam2);
	    break;
	default:
	    WARN("Unknown driver %u\n", wmd->hDrv);
	    dwRet = MCIERR_DRIVER_INTERNAL;
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_SendCommandFrom16			[internal]
 */
DWORD MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet = MCIERR_DEVICE_NOT_INSTALLED;
    LPWINE_MCIDRIVER	wmd = MCI_GetDriver(wDevID);

    if (!wmd) {
	dwRet = MCIERR_INVALID_DEVICE_ID;
    } else {
	MCI_MapType		res;
	
	switch (GetDriverFlags(wmd->hDrv) & (WINE_GDF_EXIST|WINE_GDF_16BIT)) {
	case WINE_GDF_EXIST|WINE_GDF_16BIT:		
	    dwRet = SendDriverMessage16(wmd->hDrv, wMsg, dwParam1, dwParam2);
	    break;
	case WINE_GDF_EXIST:
	    switch (res = MCI_MapMsg16To32A(wmd->wType, wMsg, &dwParam2)) {
	    case MCI_MAP_MSGERROR:
		TRACE("Not handled yet (%s)\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_DRIVER_INTERNAL;
		break;
	    case MCI_MAP_NOMEM:
		TRACE("Problem mapping msg=%s from 16 to 32a\n", MCI_MessageToString(wMsg));
		dwRet = MCIERR_OUT_OF_MEMORY;
		break;
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = SendDriverMessage(wmd->hDrv, wMsg, dwParam1, dwParam2);
		if (res == MCI_MAP_OKMEM)
		    MCI_UnMapMsg16To32A(wmd->wType, wMsg, dwParam2);
		break;
	    case MCI_MAP_PASS:
		dwRet = SendDriverMessage16(wmd->hDrv, wMsg, dwParam1, dwParam2);
		break;
	    }
	    break;
	default:
	    WARN("Unknown driver %u\n", wmd->hDrv);
	    dwRet = MCIERR_DRIVER_INTERNAL;
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_Open				[internal]
 */
static	DWORD MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSA lpParms)
{
    char			strDevTyp[128];
    DWORD 			dwRet; 
    LPWINE_MCIDRIVER		wmd;
    LPWINE_MM_IDATA		iData = MULTIMEDIA_GetIData();

    TRACE("(%08lX, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    /* only two low bytes are generic, the other ones are dev type specific */
#define WINE_MCIDRIVER_SUPP	(0xFFFF0000|MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT| \
                         MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID| \
                         MCI_NOTIFY|MCI_WAIT)
    if ((dwParam & ~WINE_MCIDRIVER_SUPP) != 0) {
	FIXME("Unsupported yet dwFlags=%08lX\n", dwParam & ~WINE_MCIDRIVER_SUPP);
    }
#undef WINE_MCIDRIVER_SUPP

    strDevTyp[0] = 0;

    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
	    WORD uDevType = LOWORD((DWORD)lpParms->lpstrDeviceType);

	    if (uDevType < MCI_DEVTYPE_FIRST ||
		uDevType > MCI_DEVTYPE_LAST ||
		!LoadStringA(iData->hWinMM32Instance, uDevType, strDevTyp, sizeof(strDevTyp))) {
		dwRet = MCIERR_BAD_INTEGER;
		goto errCleanUp;
	    }
	} else {
	    LPSTR	ptr;
	    if (lpParms->lpstrDeviceType == NULL) {
		dwRet = MCIERR_NULL_PARAMETER_BLOCK;
		goto errCleanUp;
	    }
	    strcpy(strDevTyp, lpParms->lpstrDeviceType);
	    ptr = strchr(strDevTyp, '!');
	    if (ptr) {
		/* this behavior is not documented in windows. However, since, in
		 * some occasions, MCI_OPEN handling is translated by WinMM into
		 * a call to mciSendString("open <type>"); this code shall be correct
		 */
		if (dwParam & MCI_OPEN_ELEMENT) {
		    ERR("Both MCI_OPEN_ELEMENT(%s) and %s are used\n",
			lpParms->lpstrElementName, strDevTyp);
		    dwRet = MCIERR_UNRECOGNIZED_KEYWORD;
		    goto errCleanUp;
		}
		dwParam |= MCI_OPEN_ELEMENT;
		*ptr++ = 0;
		/* FIXME: not a good idea to write in user supplied buffer */
		lpParms->lpstrElementName = ptr;
	    }
		
	}
	TRACE("devType='%s' !\n", strDevTyp);
    }

    if (dwParam & MCI_OPEN_ELEMENT) {
	TRACE("lpstrElementName='%s'\n", lpParms->lpstrElementName);

	if (dwParam & MCI_OPEN_ELEMENT_ID) {
	    FIXME("Unsupported yet flag MCI_OPEN_ELEMENT_ID\n");
	    dwRet = MCIERR_UNRECOGNIZED_KEYWORD;
	    goto errCleanUp;
	}

	if (!lpParms->lpstrElementName) {
	    dwRet = MCIERR_NULL_PARAMETER_BLOCK;
	    goto errCleanUp;
	}

	/* type, if given as a parameter, supersedes file extension */
	if (!strDevTyp[0] && 
	    MCI_GetDevTypeFromFileName(lpParms->lpstrElementName, 
				       strDevTyp, sizeof(strDevTyp))) {
	    if (GetDriveTypeA(lpParms->lpstrElementName) != DRIVE_CDROM) {
		dwRet = MCIERR_EXTENSION_NOT_FOUND;
		goto errCleanUp;
	    }
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    strcpy(strDevTyp, "CDAUDIO");
	}
    }
    
    if (strDevTyp[0] == 0) {
	FIXME("Couldn't load driver\n");
	dwRet = MCIERR_INVALID_DEVICE_NAME;
	goto errCleanUp;
    }

    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE("Alias='%s' !\n", lpParms->lpstrAlias);
	if (!lpParms->lpstrAlias) {
	    dwRet = MCIERR_NULL_PARAMETER_BLOCK;
	    goto errCleanUp;
	}
    }

    if ((dwRet = MCI_LoadMciDriver(iData, strDevTyp, &wmd))) {
	goto errCleanUp;
    }

    if ((dwRet = MCI_FinishOpen(wmd, lpParms, dwParam))) {
	TRACE("Failed to open driver (MCI_OPEN_DRIVER) [%08lx], closing\n", dwRet);
	/* FIXME: is dwRet the correct ret code ? */
	goto errCleanUp;
    }

    /* only handled devices fall through */
    TRACE("wDevID=%04X wDeviceID=%d dwRet=%ld\n", wmd->wDeviceID, lpParms->wDeviceID, dwRet);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, wmd->wDeviceID, MCI_NOTIFY_SUCCESSFUL);
    
    return 0;
errCleanUp:
    MCI_UnLoadMciDriver(iData, wmd);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, 0, MCI_NOTIFY_FAILURE);
    return dwRet;
}

/**************************************************************************
 * 			MCI_Close				[internal]
 */
static	DWORD MCI_Close(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet;
    LPWINE_MCIDRIVER	wmd;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if (wDevID == MCI_ALL_DEVICE_ID) {
	LPWINE_MCIDRIVER	next;

	EnterCriticalSection(&iData->cs);
	/* FIXME: shall I notify once after all is done, or for 
	 * each of the open drivers ? if the latest, which notif
	 * to return when only one fails ?
	 */
	for (wmd = iData->lpMciDrvs; wmd; ) {
	    next = wmd->lpNext;
	    MCI_Close(wmd->wDeviceID, dwParam, lpParms);
	    wmd = next;
	}	
	LeaveCriticalSection(&iData->cs);
	return 0;
    }

    if (!(wmd = MCI_GetDriver(wDevID))) {
	return MCIERR_INVALID_DEVICE_ID;
    }

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD)lpParms);

    MCI_UnLoadMciDriver(iData, wmd);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, wDevID,
			  (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    
    return dwRet;
}

/**************************************************************************
 * 			MCI_WriteString				[internal]
 */
DWORD	MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr)
{
    DWORD	ret = 0;

    if (lpSrcStr) {
	if (dstSize <= strlen(lpSrcStr)) {
	    lstrcpynA(lpDstStr, lpSrcStr, dstSize - 1);
	    ret = MCIERR_PARAM_OVERFLOW;
	} else {
	    strcpy(lpDstStr, lpSrcStr);
	}	
    } else {
	*lpDstStr = 0;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Sysinfo				[internal]
 */
static	DWORD MCI_SysInfo(UINT uDevID, DWORD dwFlags, LPMCI_SYSINFO_PARMSA lpParms)
{
    DWORD		ret = MCIERR_INVALID_DEVICE_ID;
    LPWINE_MCIDRIVER	wmd;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();

    if (lpParms == NULL)			return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("(%08x, %08lX, %08lX[num=%ld, wDevTyp=%u])\n", 
	  uDevID, dwFlags, (DWORD)lpParms, lpParms->dwNumber, lpParms->wDeviceType);
    
    switch (dwFlags & ~MCI_SYSINFO_OPEN) {
    case MCI_SYSINFO_QUANTITY:
	{
	    DWORD	cnt = 0;
	    
	    if (lpParms->wDeviceType < MCI_DEVTYPE_FIRST || 
		lpParms->wDeviceType > MCI_DEVTYPE_LAST) {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers\n");
		    EnterCriticalSection(&iData->cs);
		    for (wmd = iData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
			cnt++;
		    }
		    LeaveCriticalSection(&iData->cs);
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers\n");
		    cnt = MCI_InstalledCount;
		}
	    } else {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %u\n", 
			  lpParms->wDeviceType);
		    EnterCriticalSection(&iData->cs);
		    for (wmd = iData->lpMciDrvs; wmd; wmd = wmd->lpNext) {
			if (wmd->wType == lpParms->wDeviceType)
			    cnt++;
		    }
		    LeaveCriticalSection(&iData->cs);
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %u\n", 
			  lpParms->wDeviceType);
		    FIXME("Don't know how to get # of MCI devices of a given type\n");
		    cnt = 1;
		}
	    }
	    *(DWORD*)lpParms->lpstrReturn = cnt;
	}
	TRACE("(%ld) => '%ld'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = MCI_INTEGER_RETURNED;
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE("MCI_SYSINFO_INSTALLNAME \n");
	if ((wmd = MCI_GetDriver(uDevID))) {
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, 
				  wmd->lpstrDeviceType);
	} else {
	    *lpParms->lpstrReturn = 0;
	    ret = MCIERR_INVALID_DEVICE_ID;
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    case MCI_SYSINFO_NAME:
	TRACE("MCI_SYSINFO_NAME\n");
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    FIXME("Don't handle MCI_SYSINFO_NAME|MCI_SYSINFO_OPEN (yet)\n");
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	} else if (lpParms->dwNumber > MCI_InstalledCount) {
	    ret = MCIERR_OUTOFRANGE;
	} else {
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = MCI_lpInstallNames;

	    while (--count > 0) ptr += strlen(ptr) + 1;
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, ptr);
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    default:
	TRACE("Unsupported flag value=%08lx\n", dwFlags);
	ret = MCIERR_UNRECOGNIZED_COMMAND;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Break				[internal]
 */
static	DWORD MCI_Break(UINT wDevID, DWORD dwFlags, LPMCI_BREAK_PARMS lpParms)
{
    DWORD	dwRet = 0;
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, wDevID,
			  (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);

    return dwRet;
}
    
/**************************************************************************
 * 			MCI_SendCommand				[internal]
 */
DWORD	MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1, 
			DWORD dwParam2, BOOL bFrom32)
{
    DWORD		dwRet = MCIERR_UNRECOGNIZED_COMMAND;

    switch (wMsg) {
    case MCI_OPEN:
	if (bFrom32) {
	    dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
	} else {
	    switch (MCI_MapMsg16To32A(0, wMsg, &dwParam2)) {
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
		MCI_UnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_CLOSE:
	if (bFrom32) {
	    dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
	} else {
	    switch (MCI_MapMsg16To32A(0, wMsg, &dwParam2)) {
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
		MCI_UnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_SYSINFO:
	if (bFrom32) {
	    dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
	} else {
	    switch (MCI_MapMsg16To32A(0, wMsg, &dwParam2)) {
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
		MCI_UnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc doesnot  bark */
	    }
	}
	break;
    case MCI_BREAK:
	if (bFrom32) {
	    dwRet = MCI_Break(wDevID, dwParam1, (LPMCI_BREAK_PARMS)dwParam2);
	} else {
	    switch (MCI_MapMsg16To32A(0, wMsg, &dwParam2)) {
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = MCI_Break(wDevID, dwParam1, (LPMCI_BREAK_PARMS)dwParam2);
		MCI_UnMapMsg16To32A(0, wMsg, dwParam2);
		break;
	    default: break; /* so that gcc does not bark */
	    }
	}
	break;
    case MCI_SOUND:
	/* FIXME: it seems that MCI_SOUND needs the same handling as MCI_BREAK
	 * but I couldn't get any doc on this MCI message
	 */
	break;
    default:
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    dwRet = (bFrom32) ?
		MCI_SendCommandFrom32(wDevID, wMsg, dwParam1, dwParam2) :
		MCI_SendCommandFrom16(wDevID, wMsg, dwParam1, dwParam2);
	}	    
	break;
    }
    return dwRet;
}

/**************************************************************************
 * 				MCI_CleanUp			[internal]
 *
 * Some MCI commands need to be cleaned-up (when not called from 
 * mciSendString), because MCI drivers return extra information for string
 * transformation. This function gets rid of them.
 */
LRESULT		MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2, BOOL bIs32)
{
    if (LOWORD(dwRet)) 
	return LOWORD(dwRet);

    switch (wMsg) {
    case MCI_GETDEVCAPS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	    {
		LPMCI_GETDEVCAPS_PARMS	lmgp;

		lmgp = (LPMCI_GETDEVCAPS_PARMS)(bIs32 ? (void*)dwParam2 : PTR_SEG_TO_LIN(dwParam2));
		TRACE("Changing %08lx to %08lx\n", lmgp->dwReturn, (DWORD)LOWORD(lmgp->dwReturn));
		lmgp->dwReturn = LOWORD(lmgp->dwReturn);
	    } 
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc(%s)\n", 
		  HIWORD(dwRet), MCI_MessageToString(wMsg));
	}
	break;
    case MCI_STATUS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	    {
		LPMCI_STATUS_PARMS	lsp;

		lsp = (LPMCI_STATUS_PARMS)(bIs32 ? (void*)dwParam2 : PTR_SEG_TO_LIN(dwParam2));
		TRACE("Changing %08lx to %08lx\n", lsp->dwReturn, (DWORD)LOWORD(lsp->dwReturn));
		lsp->dwReturn = LOWORD(lsp->dwReturn);
	    }
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc(%s)\n", 
		  HIWORD(dwRet), MCI_MessageToString(wMsg));
	}
	break;
    case MCI_SYSINFO:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	case MCI_INTEGER_RETURNED:
	    /* nothing to do */
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x)\n", HIWORD(dwRet));	
	}
	break;
    default:
	if (HIWORD(dwRet)) {
	    FIXME("Got non null hiword for dwRet=0x%08lx for command %s\n", 
		  dwRet, MCI_MessageToString(wMsg));
	}
	break;
    }
    return LOWORD(dwRet);
}

/**************************************************************************
 * 			MULTIMEDIA_MciInit			[internal]
 *
 * Initializes the MCI internal variables.
 *
 */
BOOL MULTIMEDIA_MciInit(void)
{
    LPSTR	ptr1, ptr2;

    MCI_InstalledCount = 0;
    ptr1 = MCI_lpInstallNames = HeapAlloc(GetProcessHeap(), 0, 2048);

    if (!MCI_lpInstallNames)
	return FALSE;

    /* FIXME: should do also some registry diving here ? */
    if (PROFILE_GetWineIniString("options", "mci", "", MCI_lpInstallNames, 2048) > 0) {
	TRACE("Wine => '%s' \n", ptr1);
	while ((ptr2 = strchr(ptr1, ':')) != 0) {
	    *ptr2++ = 0;
	    TRACE("---> '%s' \n", ptr1);
	    MCI_InstalledCount++;
	    ptr1 = ptr2;
	}
	MCI_InstalledCount++;
	TRACE("---> '%s' \n", ptr1);
	ptr1 += strlen(ptr1) + 1;
    } else {
	GetPrivateProfileStringA("mci", NULL, "", MCI_lpInstallNames, 2048, "SYSTEM.INI");
	while (strlen(ptr1) > 0) {
	    TRACE("---> '%s' \n", ptr1);
	    ptr1 += strlen(ptr1) + 1;
	    MCI_InstalledCount++;
	}
    }
    return TRUE;
}

