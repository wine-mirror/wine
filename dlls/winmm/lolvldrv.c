/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM low level drivers handling functions
 *
 * Copyright 1999 Eric Pouech
 */

#include <string.h>
#include <assert.h>
#include "heap.h"
#include "user.h"	/* should be removed asap; used in MMDRV_(Get|Alloc|Free) */
#include "selectors.h"
#include "driver.h"
#include "winver.h"
#include "module.h"
#include "winemm.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmsys)

typedef	DWORD	CALLBACK (*WINEMM_msgFunc16)(UINT16, WORD, DWORD, DWORD, DWORD);
typedef	DWORD	CALLBACK (*WINEMM_msgFunc32)(UINT  , UINT, DWORD, DWORD, DWORD);

/* for each loaded driver and each known type of driver, this structure contains
 * the information needed to access it
 */
typedef struct tagWINE_MM_DRIVER_PART {
    int				nIDMin;		/* lower bound of global indexes for this type */
    int				nIDMax;		/* hhigher bound of global indexes for this type */
    union {
	WINEMM_msgFunc32	fnMessage32;	/* pointer to fonction */
	WINEMM_msgFunc16	fnMessage16;
    } u;
} WINE_MM_DRIVER_PART;

/* each low-level .drv will be associated with an instance of this structure */
typedef struct tagWINE_MM_DRIVER {
    HDRVR			hDrvr;		/* handle of loader driver */
    LPSTR			name;		/* name of the driver */
    BOOL			bIs32 : 1,	/* TRUE if 32 bit driver, FALSE for 16 */
	                        bIsMapper : 1;	/* TRUE if mapper */
    WINE_MM_DRIVER_PART		parts[MMDRV_MAX];/* Information for all known types */
} WINE_MM_DRIVER, *LPWINE_MM_DRIVER;

typedef enum {
    MMDRV_MAP_NOMEM, 	/* ko, memory problem */
    MMDRV_MAP_MSGERROR, /* ko, unknown message */
    MMDRV_MAP_OK, 	/* ok, no memory allocated. to be sent to the proc. */
    MMDRV_MAP_OKMEM, 	/* ok, some memory allocated, need to call UnMapMsg. to be sent to the proc. */
    MMDRV_MAP_PASS	/* not handled (no memory allocated) to be sent to the driver */
} MMDRV_MapType;

typedef	MMDRV_MapType	(*MMDRV_MAPFUNC)(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2);

/* each known type of driver has an instance of this structure */
typedef struct tagWINE_LLTYPE {
    /* those attributes depend on the specification of the type */    
    LPSTR		name;		/* name (for debugging) */
    BOOL		bSupportMapper;	/* if type is allowed to support mapper */
    MMDRV_MAPFUNC	Map16To32A;	/* those are function pointers to handle */
    MMDRV_MAPFUNC	UnMap16To32A;	/*   the parameter conversion (16 vs 32 bit) */
    MMDRV_MAPFUNC	Map32ATo16; 	/*   when hi-func (in mmsystem or winmm) and */
    MMDRV_MAPFUNC	UnMap32ATo16;	/*   low-func (in .drv) do not match */
    LPDRVCALLBACK	Callback;       /* handles callback for a specified type */
    /* those attributes reflect the loaded/current situation for the type */
    UINT		wMaxId;		/* number of loaded devices (sum across all loaded drivers */
    LPWINE_MLD		lpMlds;		/* "static" mlds to access the part though device IDs */
    int			nMapper;	/* index to mapper */
} WINE_LLTYPE;

static WINE_MM_DRIVER	MMDrvs[3];

/* ### start build ### */
extern WORD CALLBACK MMDRV_CallTo16_word_wwlll(FARPROC16,WORD,WORD,LONG,LONG,LONG);
/* ### stop build ### */

/**************************************************************************
 * 				MMDRV_GetDescription16		[internal]
 */
static	BOOL	MMDRV_GetDescription16(const char* fname, char* buf, int buflen)
{
    OFSTRUCT   	ofs;
    HFILE	hFile;
    WORD 	w;
    DWORD	dw;
    BOOL	ret = FALSE;

    if ((hFile = OpenFile(fname, &ofs, OF_READ | OF_SHARE_DENY_WRITE)) == HFILE_ERROR) {
	ERR("Can't open file %s\n", fname);
	return FALSE;
    }

#define E(_x)	do {TRACE _x;goto theEnd;} while(0)

    if (_lread(hFile, &w, 2) != 2)			E(("Can't read sig\n"));
    if (w != ('Z' * 256 + 'M')) 			E(("Bad sig %04x\n", w));
    if (_llseek(hFile, 0x3C, SEEK_SET) < 0) 		E(("Can't seek to ext header offset\n"));
    if (_lread(hFile, &dw, 4) != 4)			E(("Can't read ext header offset\n"));
    if (_llseek(hFile, dw + 0x2C, SEEK_SET) < 0) 	E(("Can't seek to ext header.nr table %lu\n", dw+0x2C));
    if (_lread(hFile, &dw, 4) != 4)			E(("Can't read nr table offset\n"));
    if (_llseek(hFile, dw, SEEK_SET) < 0) 		E(("Can't seek to nr table %lu\n", dw));
    if (_lread(hFile, buf, 1) != 1)			E(("Can't read descr length\n"));
    buflen = min((BYTE)buf[0], buflen - 1);
    if (_lread(hFile, buf, buflen) != buflen)		E(("Can't read descr (%d)\n", buflen));
    buf[buflen] = '\0';
    ret = TRUE;
    TRACE("Got '%s' [%d]\n", buf, buflen);
theEnd:
    CloseHandle(hFile);
    return ret;
}

/**************************************************************************
 * 				MMDRV_GetDescription32		[internal]
 */
static	BOOL	MMDRV_GetDescription32(const char* fname, char* buf, int buflen)
{
    OFSTRUCT   	ofs;
    DWORD	h;
    LPVOID	ptr = 0;
    LPVOID	val;
    DWORD	dw;
    BOOL	ret = FALSE;
    UINT	u;
    FARPROC pGetFileVersionInfoSizeA;
    FARPROC pGetFileVersionInfoA;
    FARPROC pVerQueryValueA;
    HMODULE hmodule = 0;

#define E(_x)	do {TRACE _x;goto theEnd;} while(0)

    if (OpenFile(fname, &ofs, OF_EXIST)==HFILE_ERROR)		E(("Can't find file %s\n", fname));

    if (!(hmodule = LoadLibraryA( "version.dll" ))) goto theEnd;
    if (!(pGetFileVersionInfoSizeA = GetProcAddress( hmodule, "GetFileVersionInfoSizeA" )))
        goto theEnd;
    if (!(pGetFileVersionInfoA = GetProcAddress( hmodule, "GetFileVersionInfoA" )))
        goto theEnd;
    if (!(pVerQueryValueA = GetProcAddress( hmodule, "pVerQueryValueA" )))
        goto theEnd;

    if (!(dw = pGetFileVersionInfoSizeA(ofs.szPathName, &h)))	E(("Can't get FVIS\n"));
    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, dw)))		E(("OOM\n"));
    if (!pGetFileVersionInfoA(ofs.szPathName, h, dw, ptr))	E(("Can't get FVI\n"));
    
#define	A(_x) if (pVerQueryValueA(ptr, "\\StringFileInfo\\040904B0\\" #_x, &val, &u)) \
                  TRACE(#_x " => %s\n", (LPSTR)val); else TRACE(#_x " @\n")
    
    A(CompanyName);
    A(FileDescription);
    A(FileVersion);
    A(InternalName);
    A(LegalCopyright);
    A(OriginalFilename);
    A(ProductName);
    A(ProductVersion);
    A(Comments);
    A(LegalTrademarks);
    A(PrivateBuild);
    A(SpecialBuild); 
#undef A

    if (!pVerQueryValueA(ptr, "\\StringFileInfo\\040904B0\\ProductName", &val, &u)) E(("Can't get product name\n"));
    lstrcpynA(buf, val, buflen);

#undef E    
    ret = TRUE;
theEnd:
    HeapFree(GetProcessHeap(), 0, ptr);
    if (hmodule) FreeLibrary( hmodule );
    return ret;
}

/**************************************************************************
 * 				MMDRV_Callback			[internal]
 */
static void	MMDRV_Callback(LPWINE_MLD mld, HDRVR hDev, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("CB (*%08lx)(%08x %08x %08lx %08lx %08lx\n",
	  mld->dwCallback, hDev, uMsg, mld->dwClientInstance, dwParam1, dwParam2);

    if (!mld->bFrom32 && (mld->dwFlags & DCB_TYPEMASK) == DCB_FUNCTION) {
	/* 16 bit func, call it */
	TRACE("Function (16 bit) !\n");
	MMDRV_CallTo16_word_wwlll((FARPROC16)mld->dwCallback, hDev, uMsg, 
				  mld->dwClientInstance, dwParam1, dwParam2);
    } else {
	DriverCallback(mld->dwCallback, mld->dwFlags, hDev, uMsg, 
		       mld->dwClientInstance, dwParam1, dwParam2);
    }
}

/* =================================
 *       A U X    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_Aux_Map16To32A		[internal]
 */
static	MMDRV_MapType	MMDRV_Aux_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_UnMap16To32A		[internal]
 */
static	MMDRV_MapType	MMDRV_Aux_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_Map32ATo16		[internal]
 */
static	MMDRV_MapType	MMDRV_Aux_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_UnMap32ATo16		[internal]
 */
static	MMDRV_MapType	MMDRV_Aux_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
#if 0
 case AUXDM_GETDEVCAPS:
    lpCaps->wMid = ac16.wMid;
    lpCaps->wPid = ac16.wPid;
    lpCaps->vDriverVersion = ac16.vDriverVersion;
    strcpy(lpCaps->szPname, ac16.szPname);
    lpCaps->wTechnology = ac16.wTechnology;
    lpCaps->dwSupport = ac16.dwSupport;
#endif
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_Callback		[internal]
 */
static void	CALLBACK MMDRV_Aux_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    FIXME("NIY\n");
    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *     M I X E R  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				xMMDRV_Mixer_Map16To32A		[internal]
 */
static	MMDRV_MapType	MMDRV_Mixer_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_UnMap16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_Mixer_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
#if 0
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));
    
    if (ret == MMSYSERR_NOERROR) {
	mixcaps->wMid           = micA.wMid;
	mixcaps->wPid           = micA.wPid;
	mixcaps->vDriverVersion = micA.vDriverVersion;
	strcpy(mixcaps->szPname, micA.szPname);
	mixcaps->fdwSupport     = micA.fdwSupport;
	mixcaps->cDestinations  = micA.cDestinations;
    }
    return ret;
#endif
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_Map32ATo16		[internal]
 */
static	MMDRV_MapType	MMDRV_Mixer_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_UnMap32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_Mixer_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_Callback		[internal]
 */
static void	CALLBACK MMDRV_Mixer_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    FIXME("NIY\n");
    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   M I D I  I N    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_MidiIn_Map16To32A		[internal]
 */
static	MMDRV_MapType	MMDRV_MidiIn_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_UnMap16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiIn_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_Map32ATo16		[internal]
 */
static	MMDRV_MapType	MMDRV_MidiIn_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_UnMap32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiIn_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return MMDRV_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_Callback		[internal]
 */
static void	CALLBACK MMDRV_MidiIn_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case MIM_OPEN:
    case MIM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */

    case MIM_DATA:	
    case MIM_MOREDATA:	
    case MIM_ERROR:	
	/* dwParam1 & dwParam2 are are data, nothing to do */
	break;
    case MIM_LONGDATA:
    case MIM_LONGERROR:
	/* dwParam1 points to a MidiHdr, work to be done !!! */
	if (mld->bFrom32 && !MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 32 => 16 */
	    LPMIDIHDR		mh16 = (LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)((LPSTR)mh16 - sizeof(LPMIDIHDR));
	    
	    dwParam1 = (DWORD)mh32;
	    mh32->dwFlags = mh16->dwFlags;
	    mh32->dwBytesRecorded = mh16->dwBytesRecorded;
	    if (mh32->reserved >= sizeof(MIDIHDR))
		mh32->dwOffset = mh16->dwOffset;
	} else if (!mld->bFrom32 && MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 16 => 32 */
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(dwParam1);
	    LPMIDIHDR		segmh16 = *(LPMIDIHDR*)((LPSTR)mh32 - sizeof(LPMIDIHDR));
	    LPMIDIHDR		mh16 = PTR_SEG_TO_LIN(segmh16);
	    
	    dwParam1 = (DWORD)segmh16;
	    mh16->dwFlags = mh32->dwFlags;
	    mh16->dwBytesRecorded = mh32->dwBytesRecorded;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;
	}	
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    /* case MOM_POSITIONCB: */
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   M I D I  O U T  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_MidiOut_Map16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiOut_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
	
    case MODM_OPEN:
    case MODM_CLOSE:
    case MODM_GETVOLUME:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA	moc32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMIDIOUTCAPS16) + sizeof(MIDIOUTCAPSA));
	    LPMIDIOUTCAPS16	moc16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (moc32) {
		*(LPMIDIOUTCAPS16*)moc32 = moc16;
		moc32 = (LPMIDIOUTCAPSA)((LPSTR)moc32 + sizeof(LPMIDIOUTCAPS16));
		*lpParam1 = (DWORD)moc32;
		*lpParam2 = sizeof(MIDIOUTCAPSA);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case MODM_PREPARE:
	{
	    LPMIDIHDR		mh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMIDIHDR) + sizeof(MIDIHDR));
	    LPMIDIHDR		mh16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (mh32) {
		*(LPMIDIHDR*)mh32 = (LPMIDIHDR)*lpParam1;
		mh32 = (LPMIDIHDR)((LPSTR)mh32 + sizeof(LPMIDIHDR));
		mh32->lpData = PTR_SEG_TO_LIN(mh16->lpData);
		mh32->dwBufferLength = mh16->dwBufferLength;
		mh32->dwBytesRecorded = mh16->dwBytesRecorded;
		mh32->dwUser = mh16->dwUser;
		mh32->dwFlags = mh16->dwFlags;
		/* FIXME: nothing on mh32->lpNext */
		/* could link the mh32->lpNext at this level for memory house keeping */
		mh32->dwOffset = (*lpParam2 >= sizeof(MIDIHDR)) ? ((LPMIDIHDR)mh16)->dwOffset : 0;
		mh16->lpNext = mh32; /* for reuse in unprepare and write */
		/* store size of passed MIDIHDR?? structure to know if dwOffset is available or not */
		mh16->reserved = *lpParam2;
		*lpParam1 = (DWORD)mh32;
		*lpParam2 = sizeof(MIDIHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh16 = PTR_SEG_TO_LIN(*lpParam1);
	    LPMIDIHDR		mh32 = (LPMIDIHDR)mh16->lpNext;

	    *lpParam1 = (DWORD)mh32;
	    *lpParam2 = sizeof(MIDIHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (mh32->dwBufferLength < mh16->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    mh32->dwBufferLength, mh16->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    mh32->dwBufferLength = mh16->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;

    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_UnMap16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiOut_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
	
    case MODM_OPEN:
    case MODM_CLOSE:
    case MODM_GETVOLUME:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA		moc32 = (LPMIDIOUTCAPSA)(*lpParam1);
	    LPMIDIOUTCAPS16		moc16 = *(LPMIDIOUTCAPS16*)((LPSTR)moc32 - sizeof(LPMIDIOUTCAPS16));

	    moc16->wMid			= moc32->wMid;
	    moc16->wPid			= moc32->wPid;
	    moc16->vDriverVersion	= moc32->vDriverVersion;
	    strcpy(moc16->szPname, moc32->szPname);
	    moc16->wTechnology		= moc32->wTechnology;
	    moc16->wVoices		= moc32->wVoices;
	    moc16->wNotes		= moc32->wNotes;
	    moc16->wChannelMask		= moc32->wChannelMask;
	    moc16->dwSupport		= moc32->dwSupport;
	    HeapFree(GetProcessHeap(), 0, (LPSTR)moc32 - sizeof(LPMIDIOUTCAPS16));
	    ret = MMDRV_MAP_OK;
	}
	break;
    case MODM_PREPARE:
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(*lpParam1);
	    LPMIDIHDR		mh16 = PTR_SEG_TO_LIN(*(LPMIDIHDR*)((LPSTR)mh32 - sizeof(LPMIDIHDR)));

	    assert(mh16->lpNext == mh32);
	    mh16->dwBufferLength = mh32->dwBufferLength;
	    mh16->dwBytesRecorded = mh32->dwBytesRecorded;
	    mh16->dwUser = mh32->dwUser;
	    mh16->dwFlags = mh32->dwFlags;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;

	    if (wMsg == MODM_UNPREPARE) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)mh32 - sizeof(LPMIDIHDR));
		mh16->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;

    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_Map32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiOut_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_CLOSE:
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA		moc32 = (LPMIDIOUTCAPSA)*lpParam1;
	    LPSTR			ptr = SEGPTR_ALLOC(sizeof(LPMIDIOUTCAPSA) + sizeof(MIDIOUTCAPS16));

	    if (ptr) {
		*(LPMIDIOUTCAPSA*)ptr = moc32;
		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIOUTCAPSA);
	    *lpParam2 = sizeof(MIDIOUTCAPS16);
	}
	break;
    case MODM_PREPARE:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)*lpParam1;
	    LPMIDIHDR		mh16; 
	    LPVOID		ptr = SEGPTR_ALLOC(sizeof(LPMIDIHDR) + sizeof(MIDIHDR) + mh32->dwBufferLength);

	    if (ptr) {
		*(LPMIDIHDR*)ptr = mh32;
		mh16 = (LPMIDIHDR)((LPSTR)ptr + sizeof(LPMIDIHDR));
		mh16->lpData = (LPSTR)SEGPTR_GET(ptr) + sizeof(LPMIDIHDR) + sizeof(MIDIHDR);
		/* data will be copied on WODM_WRITE */
		mh16->dwBufferLength = mh32->dwBufferLength;
		mh16->dwBytesRecorded = mh32->dwBytesRecorded;
		mh16->dwUser = mh32->dwUser;
		mh16->dwFlags = mh32->dwFlags;
		/* FIXME: nothing on mh32->lpNext */
		/* could link the mh32->lpNext at this level for memory house keeping */
		mh16->dwOffset = (*lpParam2 >= sizeof(MIDIHDR)) ? mh32->dwOffset : 0;
		    
		mh32->lpNext = (LPMIDIHDR)mh16; /* for reuse in unprepare and write */
		mh32->reserved = *lpParam2;

		TRACE("mh16=%08lx mh16->lpData=%08lx mh32->buflen=%lu mh32->lpData=%08lx\n", 
		      (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIHDR), (DWORD)mh16->lpData,
		      mh32->dwBufferLength, (DWORD)mh32->lpData);
		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIHDR);
		*lpParam2 = sizeof(MIDIHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(*lpParam1);
	    LPMIDIHDR		mh16 = (LPMIDIHDR)mh32->lpNext;
	    LPSTR		ptr = (LPSTR)mh16 - sizeof(LPMIDIHDR);

	    assert(*(LPMIDIHDR*)ptr == mh32);
	    
	    TRACE("mh16=%08lx mh16->lpData=%08lx mh32->buflen=%lu mh32->lpData=%08lx\n", 
		  (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIHDR), (DWORD)mh16->lpData,
		  mh32->dwBufferLength, (DWORD)mh32->lpData);

	    if (wMsg == MODM_LONGDATA)
		memcpy((LPSTR)mh16 + sizeof(MIDIHDR), mh32->lpData, mh32->dwBufferLength);

	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIHDR);
	    *lpParam2 = sizeof(MIDIHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (mh16->dwBufferLength < mh32->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    mh16->dwBufferLength, mh32->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    mh16->dwBufferLength = mh32->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;
    case MODM_OPEN:
	{
            LPMIDIOPENDESC		mod32 = (LPMIDIOPENDESC)*lpParam1;
	    LPVOID			ptr;
	    LPMIDIOPENDESC16		mod16;

	    /* allocated data are mapped as follows:
	       LPMIDIOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       MIDIOPENDESC16	mod16: openDesc passed to driver
	       MIDIOPENSTRMID	cIds 
	    */
	    ptr = SEGPTR_ALLOC(sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD) + sizeof(MIDIOPENDESC16) + 
			       mod32->cIds ? (mod32->cIds - 1) * sizeof(MIDIOPENSTRMID) : 0);

	    if (ptr) {
		*(LPMIDIOPENDESC*)ptr = mod32;
		*(LPDWORD)(ptr + sizeof(LPMIDIOPENDESC)) = *lpdwUser;
		mod16 = (LPMIDIOPENDESC16)((LPSTR)ptr + sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD));

		mod16->hMidi = mod32->hMidi;
		mod16->dwCallback = mod32->dwCallback;
		mod16->dwInstance = mod32->dwInstance;
		mod16->dnDevNode = mod32->dnDevNode;
		mod16->cIds = mod32->cIds;
		memcpy(&mod16->rgIds, &mod32->rgIds, mod32->cIds * sizeof(MIDIOPENSTRMID));

		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMIDIOPENDESC) + sizeof(DWORD);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case MODM_GETVOLUME:
    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_UnMap32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_MidiOut_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_CLOSE:
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
    case MODM_GETDEVCAPS:
	{
	    LPMIDIOUTCAPS16		moc16 = (LPMIDIOUTCAPS16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)moc16 - sizeof(LPMIDIOUTCAPSA);
            LPMIDIOUTCAPSA		moc32 = *(LPMIDIOUTCAPSA*)ptr;

	    moc32->wMid			= moc16->wMid;
	    moc32->wPid			= moc16->wPid;
	    moc32->vDriverVersion	= moc16->vDriverVersion;
	    strcpy(moc32->szPname, moc16->szPname);
	    moc32->wTechnology		= moc16->wTechnology;
	    moc32->wVoices		= moc16->wVoices;
	    moc32->wNotes		= moc16->wNotes;
	    moc32->wChannelMask		= moc16->wChannelMask;
	    moc32->dwSupport		= moc16->dwSupport;

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);
	    ret = MMDRV_MAP_OK;
	}
	break;
    case MODM_PREPARE:
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh16 = (LPMIDIHDR)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR		ptr = (LPSTR)mh16 - sizeof(LPMIDIHDR);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)ptr;

	    assert(mh32->lpNext == (LPMIDIHDR)mh16);
	    mh32->dwBytesRecorded = mh16->dwBytesRecorded;
	    mh32->dwUser = mh16->dwUser;
	    mh32->dwFlags = mh16->dwFlags;

	    if (wMsg == MODM_UNPREPARE) {
		if (!SEGPTR_FREE(ptr))
		    FIXME("bad free line=%d\n", __LINE__);
		mh32->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;
    case MODM_OPEN:
	{
	    LPMIDIOPENDESC16		mod16 = (LPMIDIOPENDESC16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)mod16 - sizeof(LPMIDIOPENDESC) - 2*sizeof(DWORD);

	    **(DWORD**)(ptr + sizeof(LPMIDIOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPMIDIOPENDESC) + sizeof(DWORD));

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);

	    ret = MMDRV_MAP_OK;
	}
	break;
    case MODM_GETVOLUME:
    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_Callback		[internal]
 */
static void	CALLBACK MMDRV_MidiOut_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case MOM_DONE:
	if (mld->bFrom32 && !MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 32 => 16 */
	    LPMIDIHDR		mh16 = (LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)((LPSTR)mh16 - sizeof(LPMIDIHDR));
	    
	    dwParam1 = (DWORD)mh32;
	    mh32->dwFlags = mh16->dwFlags;
	    mh32->dwOffset = mh16->dwOffset;
	    if (mh32->reserved >= sizeof(MIDIHDR))
		mh32->dwOffset = mh16->dwOffset;
	} else if (!mld->bFrom32 && MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 16 => 32 */
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(dwParam1);
	    LPMIDIHDR		segmh16 = *(LPMIDIHDR*)((LPSTR)mh32 - sizeof(LPMIDIHDR));
	    LPMIDIHDR		mh16 = PTR_SEG_TO_LIN(segmh16);
	    
	    dwParam1 = (DWORD)segmh16;
	    mh16->dwFlags = mh32->dwFlags;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;
	}	
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    /* case MOM_POSITIONCB: */
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   W A V E  I N    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_WaveIn_Map16To32A		[internal]
 */
static	MMDRV_MapType	MMDRV_WaveIn_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = MMDRV_MAP_OK;
	break;
    case WIDM_OPEN:
    case WIDM_CLOSE:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;
    case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA	wic32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEINCAPS16) + sizeof(WAVEINCAPSA));
	    LPWAVEINCAPS16	wic16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (wic32) {
		*(LPWAVEINCAPS16*)wic32 = wic16;
		wic32 = (LPWAVEINCAPSA)((LPSTR)wic32 + sizeof(LPWAVEINCAPS16));
		*lpParam1 = (DWORD)wic32;
		*lpParam2 = sizeof(WAVEINCAPSA);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_GETPOS:
	{
            LPMMTIME		mmt32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMMTIME16) + sizeof(MMTIME));
	    LPMMTIME16		mmt16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (mmt32) {
		*(LPMMTIME16*)mmt32 = mmt16;
		mmt32 = (LPMMTIME)((LPSTR)mmt32 + sizeof(LPMMTIME16));

		mmt32->wType = mmt16->wType;
		*lpParam1 = (DWORD)mmt32;
		*lpParam2 = sizeof(MMTIME);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_PREPARE:
	{
	    LPWAVEHDR		wh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEHDR) + sizeof(WAVEHDR));
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (wh32) {
		*(LPWAVEHDR*)wh32 = (LPWAVEHDR)*lpParam1;
		wh32 = (LPWAVEHDR)((LPSTR)wh32 + sizeof(LPWAVEHDR));
		wh32->lpData = PTR_SEG_TO_LIN(wh16->lpData);
		wh32->dwBufferLength = wh16->dwBufferLength;
		wh32->dwBytesRecorded = wh16->dwBytesRecorded;
		wh32->dwUser = wh16->dwUser;
		wh32->dwFlags = wh16->dwFlags;
		wh32->dwLoops = wh16->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh16->lpNext = wh32; /* for reuse in unprepare and write */
		*lpParam1 = (DWORD)wh32;
		*lpParam2 = sizeof(WAVEHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*lpParam1);
	    LPWAVEHDR		wh32 = (LPWAVEHDR)wh16->lpNext;

	    *lpParam1 = (DWORD)wh32;
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wh32->dwBufferLength < wh16->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    wh32->dwBufferLength, wh16->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    wh32->dwBufferLength = wh16->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_UnMap16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveIn_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = MMDRV_MAP_OK;
	break;
    case WIDM_OPEN:
    case WIDM_CLOSE:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;
    case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA		wic32 = (LPWAVEINCAPSA)(*lpParam1);
	    LPWAVEINCAPS16		wic16 = *(LPWAVEINCAPS16*)((LPSTR)wic32 - sizeof(LPWAVEINCAPS16));

	    wic16->wMid = wic32->wMid;
	    wic16->wPid = wic32->wPid; 
	    wic16->vDriverVersion = wic32->vDriverVersion; 
	    strcpy(wic16->szPname, wic32->szPname);
	    wic16->dwFormats = wic32->dwFormats; 
	    wic16->wChannels = wic32->wChannels; 
	    HeapFree(GetProcessHeap(), 0, (LPSTR)wic32 - sizeof(LPWAVEINCAPS16));
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WIDM_GETPOS:
	{
            LPMMTIME		mmt32 = (LPMMTIME)(*lpParam1);
	    LPMMTIME16		mmt16 = *(LPMMTIME16*)((LPSTR)mmt32 - sizeof(LPMMTIME16));

	    MMSYSTEM_MMTIME32to16(mmt16, mmt32);
	    HeapFree(GetProcessHeap(), 0, (LPSTR)mmt32 - sizeof(LPMMTIME16));
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_PREPARE:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*(LPWAVEHDR*)((LPSTR)wh32 - sizeof(LPWAVEHDR)));

	    assert(wh16->lpNext == wh32);
	    wh16->dwBufferLength = wh32->dwBufferLength;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	    wh16->dwUser = wh32->dwUser;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwLoops = wh32->dwLoops;

	    if (wMsg == WIDM_UNPREPARE) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)wh32 - sizeof(LPWAVEHDR));
		wh16->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_Map32ATo16		[internal]
 */
static	MMDRV_MapType	MMDRV_WaveIn_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_CLOSE:
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = MMDRV_MAP_OK;
	break;

    case WIDM_OPEN:
	{
            LPWAVEOPENDESC		wod32 = (LPWAVEOPENDESC)*lpParam1;
	    int				sz = sizeof(WAVEFORMATEX);
	    LPVOID			ptr;
	    LPWAVEOPENDESC16		wod16;

	    /* allocated data are mapped as follows:
	       LPWAVEOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       WAVEOPENDESC16	wod16: openDesc passed to driver
	       WAVEFORMATEX	openDesc->lpFormat passed to driver
	       xxx		extra bytes to WAVEFORMATEX
	    */
	    if (wod32->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		TRACE("Allocating %u extra bytes (%d)\n", ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize, wod32->lpFormat->wFormatTag);
		sz += ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize;
	    }

	    ptr = SEGPTR_ALLOC(sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16) + sz);

	    if (ptr) {
		*(LPWAVEOPENDESC*)ptr = wod32;
		*(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC)) = *lpdwUser;
		wod16 = (LPWAVEOPENDESC16)((LPSTR)ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD));

		wod16->hWave = wod32->hWave;
		wod16->lpFormat = (LPWAVEFORMATEX)((DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16));
		memcpy(wod16 + 1, wod32->lpFormat, sz);
		
		wod16->dwCallback = wod32->dwCallback;
		wod16->dwInstance = wod32->dwInstance;
		wod16->uMappedDeviceID = wod32->uMappedDeviceID;
		wod16->dnDevNode = wod32->dnDevNode;
		
		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + sizeof(DWORD);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_PREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)*lpParam1;
	    LPWAVEHDR		wh16; 
	    LPVOID		ptr = SEGPTR_ALLOC(sizeof(LPWAVEHDR) + sizeof(WAVEHDR) + wh32->dwBufferLength);

	    if (ptr) {
		*(LPWAVEHDR*)ptr = wh32;
		wh16 = (LPWAVEHDR)((LPSTR)ptr + sizeof(LPWAVEHDR));
		wh16->lpData = (LPSTR)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR) + sizeof(WAVEHDR);
		/* data will be copied on WODM_WRITE */
		wh16->dwBufferLength = wh32->dwBufferLength;
		wh16->dwBytesRecorded = wh32->dwBytesRecorded;
		wh16->dwUser = wh32->dwUser;
		wh16->dwFlags = wh32->dwFlags;
		wh16->dwLoops = wh32->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh32->lpNext = wh16; /* for reuse in unprepare and write */
		TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n", 
		      (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		      wh32->dwBufferLength, (DWORD)wh32->lpData);
		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR);
		*lpParam2 = sizeof(WAVEHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_UNPREPARE:
 	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = wh32->lpNext;
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);

	    assert(*(LPWAVEHDR*)ptr == wh32);
	    
	    TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n", 
		  (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		  wh32->dwBufferLength, (DWORD)wh32->lpData);

	    if (wMsg == WIDM_ADDBUFFER)
		memcpy((LPSTR)wh16 + sizeof(WAVEHDR), wh32->lpData, wh32->dwBufferLength);

	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR);
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wh32->dwBufferLength < wh16->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    wh32->dwBufferLength, wh16->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    wh32->dwBufferLength = wh16->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;
   case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA		wic32 = (LPWAVEINCAPSA)*lpParam1;
	    LPSTR			ptr = SEGPTR_ALLOC(sizeof(LPWAVEINCAPSA) + sizeof(WAVEINCAPS16));

	    if (ptr) {
		*(LPWAVEINCAPSA*)ptr = wic32;
		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEINCAPSA);
	    *lpParam2 = sizeof(WAVEINCAPS16);
	}
	break;
    case WIDM_GETPOS:
 	{
            LPMMTIME		mmt32 = (LPMMTIME)*lpParam1;
	    LPSTR		ptr = SEGPTR_ALLOC(sizeof(LPMMTIME) + sizeof(MMTIME16));
	    LPMMTIME16		mmt16 = (LPMMTIME16)(ptr + sizeof(LPMMTIME));

	    if (ptr) {
		*(LPMMTIME*)ptr = mmt32;
		mmt16->wType = mmt32->wType;
		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMMTIME);
	    *lpParam2 = sizeof(MMTIME16);
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_UnMap32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveIn_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_CLOSE:
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = MMDRV_MAP_OK;
	break;

    case WIDM_OPEN:
	{
	    LPWAVEOPENDESC16		wod16 = (LPWAVEOPENDESC16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wod16 - sizeof(LPWAVEOPENDESC) - 2*sizeof(DWORD);
            LPWAVEOPENDESC		wod32 = *(LPWAVEOPENDESC*)ptr;

	    wod32->uMappedDeviceID = wod16->uMappedDeviceID;
	    **(DWORD**)(ptr + sizeof(LPWAVEOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD));

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);

	    ret = MMDRV_MAP_OK;
	}
	break;

    case WIDM_ADDBUFFER:
    case WIDM_PREPARE:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh16 = (LPWAVEHDR)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)ptr;

	    assert(wh32->lpNext == wh16);
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	    wh32->dwUser = wh16->dwUser;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwLoops = wh16->dwLoops;

	    if (wMsg == WIDM_UNPREPARE) {
		if (!SEGPTR_FREE(ptr))
		    FIXME("bad free line=%d\n", __LINE__);
		wh32->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;
     case WIDM_GETDEVCAPS:
	{
	    LPWAVEINCAPS16		wic16 = (LPWAVEINCAPS16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wic16 - sizeof(LPWAVEINCAPSA);
            LPWAVEINCAPSA		wic32 = *(LPWAVEINCAPSA*)ptr;

	    wic32->wMid = wic16->wMid;
	    wic32->wPid = wic16->wPid;
	    wic32->vDriverVersion = wic16->vDriverVersion;
	    strcpy(wic32->szPname, wic16->szPname);
	    wic32->dwFormats = wic16->dwFormats;
	    wic32->wChannels = wic16->wChannels;
	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WIDM_GETPOS:
	{
	    LPMMTIME16		mmt16 = (LPMMTIME16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR		ptr   = (LPSTR)mmt16 - sizeof(LPMMTIME);
            LPMMTIME		mmt32 = *(LPMMTIME*)ptr;

	    MMSYSTEM_MMTIME16to32(mmt32, mmt16);

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);

	    ret = MMDRV_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_Callback		[internal]
 */
static void	CALLBACK MMDRV_WaveIn_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WIM_DATA:
	if (mld->bFrom32 && !MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 32 => 16 */
	    LPWAVEHDR		wh16 = (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)((LPSTR)wh16 - sizeof(LPWAVEHDR));
	    
	    dwParam1 = (DWORD)wh32;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	} else if (!mld->bFrom32 && MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 16 => 32 */
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(dwParam1);
	    LPWAVEHDR		segwh16 = *(LPWAVEHDR*)((LPSTR)wh32 - sizeof(LPWAVEHDR));
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(segwh16);
	    
	    dwParam1 = (DWORD)segwh16;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	}	
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   W A V E  O U T  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_WaveOut_Map16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveOut_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    /* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;

    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
    case WODM_GETVOLUME:
    case WODM_OPEN:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case WODM_GETDEVCAPS: 
	{
            LPWAVEOUTCAPSA		woc32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEOUTCAPS16) + sizeof(WAVEOUTCAPSA));
	    LPWAVEOUTCAPS16		woc16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (woc32) {
		*(LPWAVEOUTCAPS16*)woc32 = woc16;
		woc32 = (LPWAVEOUTCAPSA)((LPSTR)woc32 + sizeof(LPWAVEOUTCAPS16));
		*lpParam1 = (DWORD)woc32;
		*lpParam2 = sizeof(WAVEOUTCAPSA);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WODM_GETPOS:
	{
            LPMMTIME		mmt32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMMTIME16) + sizeof(MMTIME));
	    LPMMTIME16		mmt16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (mmt32) {
		*(LPMMTIME16*)mmt32 = mmt16;
		mmt32 = (LPMMTIME)((LPSTR)mmt32 + sizeof(LPMMTIME16));

		mmt32->wType = mmt16->wType;
		*lpParam1 = (DWORD)mmt32;
		*lpParam2 = sizeof(MMTIME);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WODM_PREPARE:
	{
	    LPWAVEHDR		wh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEHDR) + sizeof(WAVEHDR));
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*lpParam1);

	    if (wh32) {
		*(LPWAVEHDR*)wh32 = (LPWAVEHDR)*lpParam1;
		wh32 = (LPWAVEHDR)((LPSTR)wh32 + sizeof(LPWAVEHDR));
		wh32->lpData = PTR_SEG_TO_LIN(wh16->lpData);
		wh32->dwBufferLength = wh16->dwBufferLength;
		wh32->dwBytesRecorded = wh16->dwBytesRecorded;
		wh32->dwUser = wh16->dwUser;
		wh32->dwFlags = wh16->dwFlags;
		wh32->dwLoops = wh16->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh16->lpNext = wh32; /* for reuse in unprepare and write */
		*lpParam1 = (DWORD)wh32;
		*lpParam2 = sizeof(WAVEHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*lpParam1);
	    LPWAVEHDR		wh32 = (LPWAVEHDR)wh16->lpNext;

	    *lpParam1 = (DWORD)wh32;
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wh32->dwBufferLength < wh16->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    wh32->dwBufferLength, wh16->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    wh32->dwBufferLength = wh16->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_UnMap16To32A	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveOut_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret = MMDRV_MAP_MSGERROR;

    switch (wMsg) {
    /* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;

    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
    case WODM_GETVOLUME:
    case WODM_OPEN:
	FIXME("Shouldn't be used: those 16 bit functions use the 32 bit interface\n");
	break;
	
    case WODM_GETDEVCAPS: 
	{
            LPWAVEOUTCAPSA		woc32 = (LPWAVEOUTCAPSA)(*lpParam1);
	    LPWAVEOUTCAPS16		woc16 = *(LPWAVEOUTCAPS16*)((LPSTR)woc32 - sizeof(LPWAVEOUTCAPS16));

	    woc16->wMid = woc32->wMid;
	    woc16->wPid = woc32->wPid; 
	    woc16->vDriverVersion = woc32->vDriverVersion; 
	    strcpy(woc16->szPname, woc32->szPname);
	    woc16->dwFormats = woc32->dwFormats; 
	    woc16->wChannels = woc32->wChannels; 
	    woc16->dwSupport = woc32->dwSupport; 
	    HeapFree(GetProcessHeap(), 0, (LPSTR)woc32 - sizeof(LPWAVEOUTCAPS16));
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_GETPOS:
	{
            LPMMTIME		mmt32 = (LPMMTIME)(*lpParam1);
	    LPMMTIME16		mmt16 = *(LPMMTIME16*)((LPSTR)mmt32 - sizeof(LPMMTIME16));

	    MMSYSTEM_MMTIME32to16(mmt16, mmt32);
	    HeapFree(GetProcessHeap(), 0, (LPSTR)mmt32 - sizeof(LPMMTIME16));
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_PREPARE:
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(*(LPWAVEHDR*)((LPSTR)wh32 - sizeof(LPWAVEHDR)));

	    assert(wh16->lpNext == wh32);
	    wh16->dwBufferLength = wh32->dwBufferLength;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	    wh16->dwUser = wh32->dwUser;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwLoops = wh32->dwLoops;

	    if (wMsg == WODM_UNPREPARE) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)wh32 - sizeof(LPWAVEHDR));
		wh16->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_Map32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveOut_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret;

    switch (wMsg) {
	/* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
	
    case WODM_GETDEVCAPS:
	{
            LPWAVEOUTCAPSA		woc32 = (LPWAVEOUTCAPSA)*lpParam1;
	    LPSTR			ptr = SEGPTR_ALLOC(sizeof(LPWAVEOUTCAPSA) + sizeof(WAVEOUTCAPS16));

	    if (ptr) {
		*(LPWAVEOUTCAPSA*)ptr = woc32;
		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOUTCAPSA);
	    *lpParam2 = sizeof(WAVEOUTCAPS16);
	}
	break;
    case WODM_GETPITCH:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    case WODM_GETPLAYBACKRATE:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    case WODM_GETPOS:
	{
            LPMMTIME		mmt32 = (LPMMTIME)*lpParam1;
	    LPSTR		ptr = SEGPTR_ALLOC(sizeof(LPMMTIME) + sizeof(MMTIME16));
	    LPMMTIME16		mmt16 = (LPMMTIME16)(ptr + sizeof(LPMMTIME));

	    if (ptr) {
		*(LPMMTIME*)ptr = mmt32;
		mmt16->wType = mmt32->wType;
		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPMMTIME);
	    *lpParam2 = sizeof(MMTIME16);
	}
	break;
    case WODM_GETVOLUME:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    case WODM_OPEN:
	{
            LPWAVEOPENDESC		wod32 = (LPWAVEOPENDESC)*lpParam1;
	    int				sz = sizeof(WAVEFORMATEX);
	    LPVOID			ptr;
	    LPWAVEOPENDESC16		wod16;

	    /* allocated data are mapped as follows:
	       LPWAVEOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       WAVEOPENDESC16	wod16: openDesc passed to driver
	       WAVEFORMATEX	openDesc->lpFormat passed to driver
	       xxx		extra bytes to WAVEFORMATEX
	    */
	    if (wod32->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		TRACE("Allocating %u extra bytes (%d)\n", ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize, wod32->lpFormat->wFormatTag);
		sz += ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize;
	    }

	    ptr = SEGPTR_ALLOC(sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16) + sz);

	    if (ptr) {
		*(LPWAVEOPENDESC*)ptr = wod32;
		*(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC)) = *lpdwUser;
		wod16 = (LPWAVEOPENDESC16)((LPSTR)ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD));

		wod16->hWave = wod32->hWave;
		wod16->lpFormat = (LPWAVEFORMATEX)((DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16));
		memcpy(wod16 + 1, wod32->lpFormat, sz);
		
		wod16->dwCallback = wod32->dwCallback;
		wod16->dwInstance = wod32->dwInstance;
		wod16->uMappedDeviceID = wod32->uMappedDeviceID;
		wod16->dnDevNode = wod32->dnDevNode;
		
		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEOPENDESC) + sizeof(DWORD);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WODM_PREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)*lpParam1;
	    LPWAVEHDR		wh16; 
	    LPVOID		ptr = SEGPTR_ALLOC(sizeof(LPWAVEHDR) + sizeof(WAVEHDR) + wh32->dwBufferLength);

	    if (ptr) {
		*(LPWAVEHDR*)ptr = wh32;
		wh16 = (LPWAVEHDR)((LPSTR)ptr + sizeof(LPWAVEHDR));
		wh16->lpData = (LPSTR)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR) + sizeof(WAVEHDR);
		/* data will be copied on WODM_WRITE */
		wh16->dwBufferLength = wh32->dwBufferLength;
		wh16->dwBytesRecorded = wh32->dwBytesRecorded;
		wh16->dwUser = wh32->dwUser;
		wh16->dwFlags = wh32->dwFlags;
		wh16->dwLoops = wh32->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh32->lpNext = wh16; /* for reuse in unprepare and write */
		TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n", 
		      (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		      wh32->dwBufferLength, (DWORD)wh32->lpData);
		*lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR);
		*lpParam2 = sizeof(WAVEHDR);

		ret = MMDRV_MAP_OKMEM;
	    } else {
		ret = MMDRV_MAP_NOMEM;
	    }
	}
	break;
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = wh32->lpNext;
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);

	    assert(*(LPWAVEHDR*)ptr == wh32);
	    
	    TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n", 
		  (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		  wh32->dwBufferLength, (DWORD)wh32->lpData);

	    if (wMsg == WODM_WRITE)
		memcpy((LPSTR)wh16 + sizeof(WAVEHDR), wh32->lpData, wh32->dwBufferLength);

	    *lpParam1 = (DWORD)SEGPTR_GET(ptr) + sizeof(LPWAVEHDR);
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wh16->dwBufferLength < wh32->dwBufferLength) {
		ERR("Size of buffer has been increased (%ld, %ld)\n",
		    wh16->dwBufferLength, wh32->dwBufferLength);
		return MMDRV_MAP_MSGERROR;
	    }
	    wh16->dwBufferLength = wh32->dwBufferLength;
	    ret = MMDRV_MAP_OKMEM;
	}
	break;
    default:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_UnMap32ATo16	[internal]
 */
static	MMDRV_MapType	MMDRV_WaveOut_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    MMDRV_MapType	ret;

    switch (wMsg) {
	/* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = MMDRV_MAP_OK;
	break;
	
    case WODM_GETDEVCAPS:
	{
	    LPWAVEOUTCAPS16		woc16 = (LPWAVEOUTCAPS16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)woc16 - sizeof(LPWAVEOUTCAPSA);
            LPWAVEOUTCAPSA		woc32 = *(LPWAVEOUTCAPSA*)ptr;

	    woc32->wMid = woc16->wMid;
	    woc32->wPid = woc16->wPid;
	    woc32->vDriverVersion = woc16->vDriverVersion;
	    strcpy(woc32->szPname, woc16->szPname);
	    woc32->dwFormats = woc16->dwFormats;
	    woc32->wChannels = woc16->wChannels;
	    woc32->dwSupport = woc16->dwSupport;
	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_GETPITCH:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    case WODM_GETPLAYBACKRATE:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    case WODM_GETPOS:
	{
	    LPMMTIME16		mmt16 = (LPMMTIME16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR		ptr   = (LPSTR)mmt16 - sizeof(LPMMTIME);
            LPMMTIME		mmt32 = *(LPMMTIME*)ptr;

	    MMSYSTEM_MMTIME16to32(mmt32, mmt16);

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);

	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_OPEN:
	{
	    LPWAVEOPENDESC16		wod16 = (LPWAVEOPENDESC16)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wod16 - sizeof(LPWAVEOPENDESC) - 2*sizeof(DWORD);
            LPWAVEOPENDESC		wod32 = *(LPWAVEOPENDESC*)ptr;

	    wod32->uMappedDeviceID = wod16->uMappedDeviceID;
	    **(DWORD**)(ptr + sizeof(LPWAVEOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD));

	    if (!SEGPTR_FREE(ptr))
		FIXME("bad free line=%d\n", __LINE__);

	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_PREPARE:
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh16 = (LPWAVEHDR)PTR_SEG_TO_LIN(*lpParam1);
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)ptr;

	    assert(wh32->lpNext == wh16);
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	    wh32->dwUser = wh16->dwUser;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwLoops = wh16->dwLoops;

	    if (wMsg == WODM_UNPREPARE) {
		if (!SEGPTR_FREE(ptr))
		    FIXME("bad free line=%d\n", __LINE__);
		wh32->lpNext = 0;
	    }
	    ret = MMDRV_MAP_OK;
	}
	break;
    case WODM_GETVOLUME:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    default:
	FIXME("NIY: no conversion yet\n");
	ret = MMDRV_MAP_MSGERROR;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_Callback		[internal]
 */
static void	CALLBACK MMDRV_WaveOut_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WOM_DONE:
	if (mld->bFrom32 && !MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 32 => 16 */
	    LPWAVEHDR		wh16 = (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)((LPSTR)wh16 - sizeof(LPWAVEHDR));
	    
	    dwParam1 = (DWORD)wh32;
	    wh32->dwFlags = wh16->dwFlags;
	} else if (!mld->bFrom32 && MMDrvs[mld->mmdIndex].bIs32) {
	    /* initial map is: 16 => 32 */
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(dwParam1);
	    LPWAVEHDR		segwh16 = *(LPWAVEHDR*)((LPSTR)wh32 - sizeof(LPWAVEHDR));
	    LPWAVEHDR		wh16 = PTR_SEG_TO_LIN(segwh16);
	    
	    dwParam1 = (DWORD)segwh16;
	    wh16->dwFlags = wh32->dwFlags;
	}	
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

#define A(_x,_y) {#_y, _x, \
MMDRV_##_y##_Map16To32A, MMDRV_##_y##_UnMap16To32A, \
MMDRV_##_y##_Map32ATo16, MMDRV_##_y##_UnMap32ATo16, \
MMDRV_##_y##_Callback, 0, NULL, -1}

/* Note: the indices of this array must match the definitions
 *	 of the MMDRV_???? manifest constants
 */
static WINE_LLTYPE	llTypes[MMDRV_MAX] = {
    A(TRUE,  Aux),
    A(FALSE, Mixer),
    A(TRUE,  MidiIn),
    A(TRUE,  MidiOut),
    A(TRUE,  WaveIn),
    A(TRUE,  WaveOut),
};
#undef A

/**************************************************************************
 * 			MMDRV_GetNum				[internal]
 */
UINT	MMDRV_GetNum(UINT type)
{
    assert(type < MMDRV_MAX);
    return llTypes[type].wMaxId;
}

/**************************************************************************
 * 				WINE_Message			[internal]
 */
DWORD	MMDRV_Message(LPWINE_MLD mld, WORD wMsg, DWORD dwParam1, 
		      DWORD dwParam2, BOOL bFrom32)
{
    LPWINE_MM_DRIVER 		lpDrv;
    DWORD			ret;
    WINE_MM_DRIVER_PART*	part;
    WINE_LLTYPE*		llType = &llTypes[mld->type];
    MMDRV_MapType		map;
    int				devID;

    TRACE("(%s %u %u 0x%08lx 0x%08lx 0x%08lx %c)!\n", 
	  llTypes[mld->type].name, mld->uDeviceID, wMsg, 
	  mld->dwDriverInstance, dwParam1, dwParam2, bFrom32?'Y':'N');

    if (mld->uDeviceID == (UINT16)-1) {
	if (!llType->bSupportMapper) {
	    WARN("uDev=-1 requested on non-mappable ll type %s\n", 
		 llTypes[mld->type].name);
	    return MMSYSERR_BADDEVICEID;
	}
	devID = -1;
    } else {
	if (mld->uDeviceID >= llType->wMaxId) {
	    WARN("uDev(%u) requested >= max (%d)\n", mld->uDeviceID, llType->wMaxId);
	    return MMSYSERR_BADDEVICEID;
	}
	devID = mld->uDeviceID;
    }

    lpDrv = &MMDrvs[mld->mmdIndex];
    part = &lpDrv->parts[mld->type];

#if 0
    /* some sanity checks */
    if (!(part->nIDMin <= devID))
	ERR("!(part->nIDMin(%d) <= devID(%d))\n", part->nIDMin, devID);
    if (!(devID < part->nIDMax))
	ERR("!(devID(%d) < part->nIDMax(%d))\n", devID, part->nIDMax);
#endif

    if (lpDrv->bIs32) {
	assert(part->u.fnMessage32);
	
	if (bFrom32) {
	    TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx\n", 
		  mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
            ret = part->u.fnMessage32(mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
	    TRACE("=> %lu\n", ret);
	} else {
	    map = llType->Map16To32A(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
	    switch (map) {
	    case MMDRV_MAP_NOMEM:
		ret = MMSYSERR_NOMEM;
		break;
	    case MMDRV_MAP_MSGERROR:
		FIXME("NIY: no conversion yet 16->32 (%u)\n", wMsg);
		ret = MMSYSERR_ERROR;
		break;
	    case MMDRV_MAP_OK:
	    case MMDRV_MAP_OKMEM:
		TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx\n", 
		      mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
		ret = part->u.fnMessage32(mld->uDeviceID, wMsg, mld->dwDriverInstance, 
					  dwParam1, dwParam2);
		TRACE("=> %lu\n", ret);
		if (map == MMDRV_MAP_OKMEM)
		    llType->UnMap16To32A(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
		break;
	    default:
	    case MMDRV_MAP_PASS:
		FIXME("NIY: pass used ?\n");
		ret = MMSYSERR_NOTSUPPORTED;
		break;
	    }
	}
    } else {
	assert(part->u.fnMessage16);

	if (bFrom32) {
	    map = llType->Map32ATo16(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
	    switch (map) {
	    case MMDRV_MAP_NOMEM:
		ret = MMSYSERR_NOMEM;
		break;
	    case MMDRV_MAP_MSGERROR:
		FIXME("NIY: no conversion yet 32->16 (%u)\n", wMsg);
		ret = MMSYSERR_ERROR;
		break;
	    case MMDRV_MAP_OK:
	    case MMDRV_MAP_OKMEM:
		TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx\n", 
		      mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
		ret = MMDRV_CallTo16_word_wwlll((FARPROC16)part->u.fnMessage16, mld->uDeviceID, 
						wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
		TRACE("=> %lu\n", ret);
		if (map == MMDRV_MAP_OKMEM)
		    llType->UnMap32ATo16(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
		break;
	    default:
	    case MMDRV_MAP_PASS:
		FIXME("NIY: pass used ?\n");
		ret = MMSYSERR_NOTSUPPORTED;
		break;
	    }
	} else {
	    TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx\n", 
		  mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
            ret = MMDRV_CallTo16_word_wwlll((FARPROC16)part->u.fnMessage16, mld->uDeviceID, 
					    wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
	    TRACE("=> %lu\n", ret);
	}
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_Alloc			[internal]
 */
LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags, 
			    DWORD* dwCallback, DWORD* dwInstance, BOOL bFrom32)
{
    LPWINE_MLD	mld;

    if ((*hndl = USER_HEAP_ALLOC(size)) == 0) 
	return NULL;
    
    mld = (LPWINE_MLD) USER_HEAP_LIN_ADDR(*hndl);
    if (!mld)	return NULL;
    mld->type = type;
    if ((UINT)*hndl < MMDRV_GetNum(type) || HIWORD(*hndl) != 0) {
	/* FIXME: those conditions must be fulfilled so that:
	 * - we can distinguish between device IDs and handles
	 * - we can use handles as 16 or 32 bit entities
	 */
	ERR("Shouldn't happen. Bad allocation scheme\n");
    }

    mld->bFrom32 = bFrom32;
    mld->dwFlags = HIWORD(*dwFlags);
    mld->dwCallback = *dwCallback;
    mld->dwClientInstance = *dwInstance;

    *dwFlags = LOWORD(*dwFlags) | CALLBACK_FUNCTION;
    *dwCallback = (DWORD)llTypes[type].Callback;
    *dwInstance = (DWORD)mld; /* FIXME: wouldn't some 16 bit drivers only use the loword ? */

    return mld;
}

/**************************************************************************
 * 				MMDRV_Free			[internal]
 */
void	MMDRV_Free(HANDLE hndl, LPWINE_MLD mld)
{
    USER_HEAP_FREE(hndl);
}

/**************************************************************************
 * 				MMDRV_Open			[internal]
 */
DWORD	MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD dwParam1, DWORD dwFlags)
{
    DWORD		dwRet = MMSYSERR_BADDEVICEID;
    DWORD		dwInstance;
    WINE_LLTYPE*	llType = &llTypes[mld->type];

    mld->dwDriverInstance = (DWORD)&dwInstance;

    if (mld->uDeviceID == (UINT)-1 || mld->uDeviceID == (UINT16)-1) {
	TRACE("MAPPER mode requested !\n");
	/* check if mapper is supported by type */
	if (llType->bSupportMapper) {
	    if (llType->nMapper == -1) {
		/* no driver for mapper has been loaded, try a dumb implementation */
		TRACE("No mapper loaded, doing it by hand\n");
		for (mld->uDeviceID = 0; mld->uDeviceID < llType->wMaxId; mld->uDeviceID++) {
		    if ((dwRet = MMDRV_Open(mld, wMsg, dwParam1, dwFlags)) == MMSYSERR_NOERROR) {
			/* to share this function epilog */
			dwInstance = mld->dwDriverInstance;
			break;
		    }
		}
	    } else {
		mld->uDeviceID = (UINT16)-1;
		mld->mmdIndex = llType->lpMlds[-1].mmdIndex;
		TRACE("Setting mmdIndex to %u\n", mld->mmdIndex);
		dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags, TRUE);
	    }
	}
    } else {
	if (mld->uDeviceID < llType->wMaxId) {
	    mld->mmdIndex = llType->lpMlds[mld->uDeviceID].mmdIndex;
	    TRACE("Setting mmdIndex to %u\n", mld->mmdIndex);
	    dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags, TRUE);
	}
    }
    if (dwRet == MMSYSERR_NOERROR)
	mld->dwDriverInstance = dwInstance;
    return dwRet;
}

/**************************************************************************
 * 				MMDRV_Close			[internal]
 */
DWORD	MMDRV_Close(LPWINE_MLD mld, UINT wMsg)
{
    return MMDRV_Message(mld, wMsg, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				MMDRV_GetByID			[internal]
 */
LPWINE_MLD	MMDRV_GetByID(UINT uDevID, UINT type)
{
    if (uDevID < llTypes[type].wMaxId)
	return &llTypes[type].lpMlds[uDevID];
    if ((uDevID == (UINT16)-1 || uDevID == (UINT)-1) && llTypes[type].nMapper != -1)
	return &llTypes[type].lpMlds[-1];
    return NULL;
}

/**************************************************************************
 * 				MMDRV_Get			[internal]
 */
LPWINE_MLD	MMDRV_Get(HANDLE hndl, UINT type, BOOL bCanBeID)
{
    LPWINE_MLD	mld = NULL;

    assert(type < MMDRV_MAX);

    if ((UINT)hndl >= llTypes[type].wMaxId) {
	mld = (LPWINE_MLD)USER_HEAP_LIN_ADDR(hndl);
	
	if (!IsBadWritePtr(mld, sizeof(*mld)) && mld->type != type) mld = NULL;
    }
    if (mld == NULL && bCanBeID) {
	mld = MMDRV_GetByID((UINT)hndl, type);
    }
    return mld;
}

/**************************************************************************
 * 				MMDRV_GetRelated		[internal]
 */
LPWINE_MLD	MMDRV_GetRelated(HANDLE hndl, UINT srcType,
				 BOOL bSrcCanBeID, UINT dstType)
{
    LPWINE_MLD		mld;

    if ((mld = MMDRV_Get(hndl, srcType, bSrcCanBeID)) != NULL) {
	WINE_MM_DRIVER_PART*	part = &MMDrvs[mld->mmdIndex].parts[dstType];
	if (part->nIDMin < part->nIDMax)
	    return MMDRV_GetByID(part->nIDMin, dstType);
    } 
    return NULL;
}

/**************************************************************************
 * 				MMDRV_PhysicalFeatures		[internal]
 */
UINT	MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg, DWORD dwParam1, 
			       DWORD dwParam2)
{
    WINE_MM_DRIVER*	lpDrv = &MMDrvs[mld->mmdIndex];

    TRACE("(%p, %04x, %08lx, %08lx)\n", mld, uMsg, dwParam1, dwParam2);

    /* all those function calls are undocumented */
    switch (uMsg) {
    case 0x801: /* DRV_QUERYDRVENTRY */
	lstrcpynA((LPSTR)dwParam1, lpDrv->name, LOWORD(dwParam2));
	break;
    case 0x802: /* DRV_QUERYDEVNODE */
	*(LPDWORD)dwParam1 = 0L; /* should be DevNode */
	break;
    case 0x803:	/* DRV_QUERYNAME */
	WARN("NIY 0x803\n");
	break;
    case 0x804: /* DRV_QUERYDRIVERIDS */
	WARN("NIY call VxD\n");
	/* should call VxD MMDEVLDR with (DevNode, dwParam1 and dwParam2) as pmts
	 * dwParam1 is buffer and dwParam2 is sizeof buffer
	 * I don't know where the result is stored though
	 */
	break;
    case 0x805: /* DRV_QUERYMAPPABLE */
	return (lpDrv->bIsMapper) ? 2 : 0;
    default:
	WARN("Unknown call %04x\n", uMsg);
	return MMSYSERR_INVALPARAM;
    }
    return 0L;
}

/**************************************************************************
 * 				MMDRV_InitPerType		[internal]
 */
static  BOOL	MMDRV_InitPerType(LPWINE_MM_DRIVER lpDrv, UINT num, 
				  UINT type, UINT wMsg)
{
    WINE_MM_DRIVER_PART*	part = &lpDrv->parts[type];
    DWORD			ret;
    UINT			count = 0;
    int				i, k;

    part->nIDMin = part->nIDMax = 0;

    /* for DRVM_INIT and DRVM_ENABLE, dwParam2 should be PnP node */
    /* the DRVM_ENABLE is only required when the PnP node is non zero */

    if (lpDrv->bIs32 && part->u.fnMessage32) {
	ret = part->u.fnMessage32(0, DRVM_INIT, 0L, 0L, 0L);
	TRACE("DRVM_INIT => %08lx\n", ret);
#if 0
	ret = part->u.fnMessage32(0, DRVM_ENABLE, 0L, 0L, 0L);
	TRACE("DRVM_ENABLE => %08lx\n", ret);
#endif
        count = part->u.fnMessage32(0, wMsg, 0L, 0L, 0L);
    }

    if (!lpDrv->bIs32 && part->u.fnMessage16) {
        ret = MMDRV_CallTo16_word_wwlll((FARPROC16)part->u.fnMessage16, 
					0, DRVM_INIT, 0L, 0L, 0L);
	TRACE("DRVM_INIT => %08lx\n", ret);
#if 0
	ret = MMDRV_CallTo16_word_wwlll((FARPROC16)part->u.fnMessage16,
					0, DRVM_ENABLE, 0L, 0L, 0L);
	TRACE("DRVM_ENABLE => %08lx\n", ret);
#endif
        count = MMDRV_CallTo16_word_wwlll((FARPROC16)part->u.fnMessage16, 
					  0, wMsg, 0L, 0L, 0L);
    }

    TRACE("Got %u dev for (%s:%s)\n", count, lpDrv->name, llTypes[type].name);
    if (count == 0)
	return FALSE;

    /* got some drivers */
    if (lpDrv->bIsMapper) {
	if (llTypes[type].nMapper != -1)
	    ERR("Two mappers for type %s (%d, %s)\n", 
		llTypes[type].name, llTypes[type].nMapper, lpDrv->name);
	if (count > 1)
	    ERR("Strange: mapper with %d > 1 devices\n", count);
	llTypes[type].nMapper = num;
    } else {
	part->nIDMin = llTypes[type].wMaxId;
	llTypes[type].wMaxId += count;
	part->nIDMax = llTypes[type].wMaxId;
    }
    TRACE("Setting min=%d max=%d (ttop=%d) for (%s:%s)\n",
	  part->nIDMin, part->nIDMax, llTypes[type].wMaxId, 
	  lpDrv->name, llTypes[type].name);
    /* realloc translation table */
    llTypes[type].lpMlds = (LPWINE_MLD)
	HeapReAlloc(GetProcessHeap(), 0, (llTypes[type].lpMlds) ? llTypes[type].lpMlds - 1 : NULL,
		    sizeof(WINE_MLD) * (llTypes[type].wMaxId + 1)) + 1;
    /* re-build the translation table */
    if (llTypes[type].nMapper != -1) {
	TRACE("%s:Trans[%d] -> %s\n", llTypes[type].name, -1, MMDrvs[llTypes[type].nMapper].name);
	llTypes[type].lpMlds[-1].uDeviceID = (UINT16)-1;
	llTypes[type].lpMlds[-1].type = type;
	llTypes[type].lpMlds[-1].mmdIndex = llTypes[type].nMapper;
	llTypes[type].lpMlds[-1].dwDriverInstance = 0;
    }
    for (i = k = 0; i <= num; i++) {
	while (MMDrvs[i].parts[type].nIDMin <= k && k < MMDrvs[i].parts[type].nIDMax) {
	    TRACE("%s:Trans[%d] -> %s\n", llTypes[type].name, k, MMDrvs[i].name);
	    llTypes[type].lpMlds[k].uDeviceID = k;
	    llTypes[type].lpMlds[k].type = type;
	    llTypes[type].lpMlds[k].mmdIndex = i;
	    llTypes[type].lpMlds[k].dwDriverInstance = 0;
	    k++;
	}
    }
    return TRUE;
}

/**************************************************************************
 * 				MMDRV_Install			[internal]
 */
static	BOOL	MMDRV_Install(LPCSTR name, int num, BOOL bIsMapper)
{
    int			count = 0;
    char		buffer[128];
    HMODULE		hModule;
    LPWINE_MM_DRIVER	lpDrv = &MMDrvs[num];

    TRACE("('%s');\n", name);
    
    memset(lpDrv, 0, sizeof(*lpDrv));

    /* First load driver */
    if ((lpDrv->hDrvr = OpenDriverA(name, 0, 0)) == 0) {
	WARN("Couldn't open driver '%s'\n", name);
	return FALSE;
    }

    /* Then look for xxxMessage functions */
#define	AA(_w,_x,_y,_z)						\
    func = (WINEMM_msgFunc##_y) _z (hModule, #_x);		\
    if (func != NULL) 						\
        { lpDrv->parts[_w].u.fnMessage##_y = func; count++; 	\
          TRACE("Got %d bit func '%s'\n", _y, #_x);         }

    if ((GetDriverFlags(lpDrv->hDrvr) & (WINE_GDF_EXIST|WINE_GDF_16BIT)) == WINE_GDF_EXIST) {
	WINEMM_msgFunc32	func;

	lpDrv->bIs32 = TRUE;
	if ((hModule = GetDriverModuleHandle(lpDrv->hDrvr))) {
#define A(_x,_y)	AA(_x,_y,32,GetProcAddress)
	    A(MMDRV_AUX,	auxMessage);
	    A(MMDRV_MIXER,	mixMessage);
	    A(MMDRV_MIDIIN,	midMessage);
	    A(MMDRV_MIDIOUT, 	modMessage);
	    A(MMDRV_WAVEIN,	widMessage);
	    A(MMDRV_WAVEOUT,	wodMessage);
#undef A
	}
    } else {
	WINEMM_msgFunc16	func;

	/*
	 * DESCRIPTION 'wave,aux,mixer:Creative Labs Sound Blaster 16 Driver'
	 * The beginning of the module description indicates the driver supports 
	 * waveform, auxiliary, and mixer devices. Use one of the following 
	 * device-type names, followed by a colon (:) to indicate the type of
	 * device your driver supports. If the driver supports more than one
	 * type of device, separate each device-type name with a comma (,). 
	 *	
	 * wave for waveform audio devices 
	 * wavemapper for wave mappers
	 * midi for MIDI audio devices 
	 * midimapper for midi mappers
	 * aux for auxiliary audio devices 
	 * mixer for mixer devices 
	 */
	
	lpDrv->bIs32 = FALSE;
	if ((hModule = GetDriverModuleHandle16(lpDrv->hDrvr))) {
#define A(_x,_y)	AA(_x,_y,16,WIN32_GetProcAddress16)
	    A(MMDRV_AUX,	auxMessage);
	    A(MMDRV_MIXER,	mixMessage);
	    A(MMDRV_MIDIIN,	midMessage);
	    A(MMDRV_MIDIOUT, 	modMessage);
	    A(MMDRV_WAVEIN,	widMessage);
	    A(MMDRV_WAVEOUT,	wodMessage);
#undef A
	}
    }
#undef AA

    if (TRACE_ON(mmsys)) {
	if ((lpDrv->bIs32) ? MMDRV_GetDescription32(name, buffer, sizeof(buffer)) :
	                     MMDRV_GetDescription16(name, buffer, sizeof(buffer)))
	    TRACE("%s => %s\n", name, buffer);
	else
	    TRACE("%s => No description\n", name);
    }

    if (!count) {
	CloseDriver(lpDrv->hDrvr, 0, 0);
	WARN("No message functions found\n");
	return FALSE;
    }

    /* FIXME: being a mapper or not should be known by another way */
    /* it's known for NE drvs (the description is of the form '*mapper: *'
     * I don't have any clue for PE drvs
     * on Win 9x, the value is gotten from the key mappable under
     * HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\MediaResources\
     */
    lpDrv->bIsMapper = bIsMapper;
    lpDrv->name = HEAP_strdupA(GetProcessHeap(), 0, name);

    /* Finish init and get the count of the devices */
    MMDRV_InitPerType(lpDrv, num, MMDRV_AUX,		AUXDM_GETNUMDEVS);
    MMDRV_InitPerType(lpDrv, num, MMDRV_MIXER, 		MXDM_GETNUMDEVS);
    MMDRV_InitPerType(lpDrv, num, MMDRV_MIDIIN, 	MIDM_GETNUMDEVS);
    MMDRV_InitPerType(lpDrv, num, MMDRV_MIDIOUT, 	MODM_GETNUMDEVS);
    MMDRV_InitPerType(lpDrv, num, MMDRV_WAVEIN, 	WIDM_GETNUMDEVS);
    MMDRV_InitPerType(lpDrv, num, MMDRV_WAVEOUT, 	WODM_GETNUMDEVS);
    /* FIXME: if all those func calls return FALSE, then the driver must be unloaded */
    return TRUE;
}

/**************************************************************************
 * 				MMDRV_Init			[internal]
 */
BOOL	MMDRV_Init(void)
{
    int				num = 0;

    /* FIXME: this should be moved to init files;
     * 		- either .winerc/wine.conf
     *		- or made of registry keys
     * this is a temporary hack, shall be removed anytime now
     */
    /* first load hardware drivers */
    if (MMDRV_Install("wineoss.drv",	   num, FALSE)) num++;

    /* finish with mappers */
    if (MMDRV_Install("msacm.drv", 	   num, TRUE )) num++;
    if (MMDRV_Install("midimap.drv",	   num, TRUE )) num++;


    /* be sure that size of MMDrvs matches the max number of loadable drivers !! 
     * if not just increase size of MMDrvs */
    assert(num <= sizeof(MMDrvs)/sizeof(MMDrvs[0]));

    return TRUE;
}
