/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 *
static char Copyright[] = "Copyright  Martin Ayotte, 1993";
*/
#ifndef WINELIB

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "ldt.h"
#include "callback.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "stddebug.h"
/* #define DEBUG_MCI    */
/* #undef  DEBUG_MCI    */
/* #define DEBUG_MMTIME */
/* #undef  DEBUG_MMTIME */
/* #define DEBUG_MMIO   */
/* #undef  DEBUG_MMIO   */
#include "debug.h"

static int	InstalledCount;
static int	InstalledListLen;
static LPSTR	lpInstallNames = NULL;

static BOOL		mmTimeStarted = FALSE;
static MMTIME	mmSysTimeMS;
static MMTIME	mmSysTimeSMPTE;

typedef struct tagTIMERENTRY {
	WORD		wDelay;
	WORD		wResol;
	FARPROC		lpFunc;
	DWORD		dwUser;
	WORD		wFlags;
	WORD		wTimerID;
	WORD		wCurTime;
	struct tagTIMERENTRY	*Next;
	struct tagTIMERENTRY	*Prev;
	} TIMERENTRY;
typedef TIMERENTRY *LPTIMERENTRY;

static LPTIMERENTRY lpTimerList = NULL;

static MCI_OPEN_DRIVER_PARMS	mciDrv[MAXMCIDRIVERS];

UINT WINAPI midiGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT WINAPI waveGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
LRESULT DrvDefDriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2);

LRESULT WAVE_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2);
LRESULT MIDI_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2);
LRESULT CDAUDIO_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2);
LRESULT ANIM_DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
							DWORD dwParam1, DWORD dwParam2);

/**************************************************************************
* 				MMSYSTEM_WEP		[MMSYSTEM.1]
*/
int MMSYSTEM_WEP(HANDLE hInstance, WORD wDataSeg,
		 WORD cbHeapSize, LPSTR lpCmdLine)
{
	printf("MMSYSTEM DLL INIT ... hInst=%04X \n", hInstance);
	return(TRUE);
}

/**************************************************************************
* 				sndPlaySound		[MMSYSTEM.2]
*/
BOOL WINAPI sndPlaySound(LPCSTR lpszSoundName, UINT uFlags)
{
	HMMIO			hmmio;
	MMCKINFO		mmckInfo;
	MMCKINFO		ckMainRIFF;
	HANDLE			hFormat;
	PCMWAVEFORMAT 	pcmWaveFormat;
	int				count;
	int				bufsize;
	HANDLE			hDesc;
	LPWAVEOPENDESC 	lpWaveDesc;
	HANDLE			hWaveHdr;
	LPWAVEHDR		lpWaveHdr;
	LPWAVEHDR		lp16WaveHdr;
	HANDLE			hData;
	DWORD			dwRet;
	char			str[128];
	LPSTR			ptr;
	printf("sndPlaySound // SoundName='%s' uFlags=%04X !\n", 
									lpszSoundName, uFlags);
	if (lpszSoundName == NULL) {
		printf("sndPlaySound // Stop !\n");
		return FALSE;
		}
	hmmio = mmioOpen((LPSTR)lpszSoundName, NULL, 
		MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
	if (hmmio == 0) {
		printf("sndPlaySound // searching in SystemSound List !\n");
		GetProfileString("Sounds", (LPSTR)lpszSoundName, "", str, sizeof(str));
		if (strlen(str) == 0) return FALSE;
		if ( (ptr = (LPSTR)strchr(str, ',')) != NULL) *ptr = '\0';
		hmmio = mmioOpen(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
		if (hmmio == 0) {
			printf("sndPlaySound // can't find SystemSound='%s' !\n", str);
			return FALSE;
			}
		}
	if (mmioDescend(hmmio, &ckMainRIFF, NULL, 0) != 0) {
ErrSND:	if (hmmio != 0)   mmioClose(hmmio, 0);
		return FALSE;
		}
	printf("sndPlaySound // ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType,
				ckMainRIFF.cksize);
	if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
	    (ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E'))) goto ErrSND;
	mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) != 0) goto ErrSND;
	printf("sndPlaySound // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
			(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType,
			mmckInfo.cksize);
	if (mmioRead(hmmio, (HPSTR) &pcmWaveFormat,
		(long) sizeof(PCMWAVEFORMAT)) != (long) sizeof(PCMWAVEFORMAT)) goto ErrSND;

	printf("sndPlaySound // wFormatTag=%04X !\n", pcmWaveFormat.wf.wFormatTag);
	printf("sndPlaySound // nChannels=%d \n", pcmWaveFormat.wf.nChannels);
	printf("sndPlaySound // nSamplesPerSec=%ld\n", pcmWaveFormat.wf.nSamplesPerSec);
	printf("sndPlaySound // nAvgBytesPerSec=%ld\n", pcmWaveFormat.wf.nAvgBytesPerSec);
	printf("sndPlaySound // nBlockAlign=%d \n", pcmWaveFormat.wf.nBlockAlign);
	printf("sndPlaySound // wBitsPerSample=%u !\n", pcmWaveFormat.wBitsPerSample);

	mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) != 0) goto ErrSND;
	printf("sndPlaySound // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
			(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType,
			mmckInfo.cksize);
	hDesc = USER_HEAP_ALLOC(sizeof(WAVEOPENDESC));
	lpWaveDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hDesc);
	lpWaveDesc->hWave = 0;
	pcmWaveFormat.wf.nAvgBytesPerSec = 
		pcmWaveFormat.wf.nSamplesPerSec * pcmWaveFormat.wf.nBlockAlign;
	hFormat = USER_HEAP_ALLOC(sizeof(PCMWAVEFORMAT));
	lpWaveDesc->lpFormat = (LPWAVEFORMAT) USER_HEAP_LIN_ADDR(hFormat);
	memcpy(lpWaveDesc->lpFormat, &pcmWaveFormat, sizeof(PCMWAVEFORMAT));
	lpWaveDesc = (LPWAVEOPENDESC) USER_HEAP_SEG_ADDR(hDesc);
	dwRet = wodMessage(0, WODM_OPEN, 0, (DWORD)lpWaveDesc, CALLBACK_NULL);
	if (dwRet != MMSYSERR_NOERROR) {
		printf("sndPlaySound // can't open WaveOut device !\n");
		goto ErrSND;
		}
	USER_HEAP_FREE(hFormat);
	hWaveHdr = USER_HEAP_ALLOC(sizeof(WAVEHDR));
	lpWaveHdr = (LPWAVEHDR) USER_HEAP_LIN_ADDR(hWaveHdr);
	lp16WaveHdr = (LPWAVEHDR) USER_HEAP_SEG_ADDR(hWaveHdr);
	bufsize = 64000;
	hData = GlobalAlloc(GMEM_MOVEABLE, bufsize);
	lpWaveHdr->lpData = (LPSTR) WIN16_GlobalLock(hData);
	lpWaveHdr->dwBufferLength = bufsize;
	lpWaveHdr->dwUser = 0L;
	lpWaveHdr->dwFlags = 0L;
	lpWaveHdr->dwLoops = 0L;
	dwRet = wodMessage(0, WODM_PREPARE, 0, (DWORD)lp16WaveHdr, sizeof(WAVEHDR));
	if (dwRet != MMSYSERR_NOERROR) {
		printf("sndPlaySound // can't prepare WaveOut device !\n");
		GlobalUnlock(hData);
		GlobalFree(hData);
		USER_HEAP_FREE(hDesc);
		USER_HEAP_FREE(hWaveHdr);
		goto ErrSND;
		}
	while(TRUE) {
		count = mmioRead(hmmio, PTR_SEG_TO_LIN(lpWaveHdr->lpData), bufsize);
		if (count < 1) break;
		lpWaveHdr->dwBufferLength = count;
/*		lpWaveHdr->dwBytesRecorded = count; */
		wodMessage(0, WODM_WRITE, 0, (DWORD)lp16WaveHdr, sizeof(WAVEHDR));
		}
	wodMessage(0, WODM_UNPREPARE, 0, (DWORD)lp16WaveHdr, sizeof(WAVEHDR));
	wodMessage(0, WODM_CLOSE, 0, 0L, 0L);
	GlobalUnlock(hData);
	GlobalFree(hData);
	USER_HEAP_FREE(hDesc);
	USER_HEAP_FREE(hWaveHdr);
	if (hmmio != 0)   mmioClose(hmmio, 0);
	return TRUE;
}

/**************************************************************************
* 				mmsystemGetVersion	[MMSYSTEM.5]
*/
WORD WINAPI mmsystemGetVersion()
{
	printf("mmsystemGetVersion // 0.4.0 ...?... :-) !\n");
	return(0x0040);
}

/**************************************************************************
* 				DriverProc	[MMSYSTEM.6]
*/
LRESULT DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	return DrvDefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
* 				OutputDebugStr		[MMSYSTEM.30]
*/
void WINAPI OutputDebugStr(LPCSTR str)
{
	printf("EMPTY STUB !!! OutputDebugStr('%s');\n", str);
}

/**************************************************************************
* 				DriverCallback	[MMSYSTEM.31]
*/
BOOL DriverCallback(DWORD dwCallBack, UINT uFlags, HANDLE hDev, 
		WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
	printf("DriverCallback(%08lX, %04X, %04X, %04X, %08lX, %08lX, %08lX); !\n",
		dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);
	switch(uFlags & DCB_TYPEMASK) {
		case DCB_NULL:
			printf("DriverCallback() // CALLBACK_NULL !\n");
			break;
		case DCB_WINDOW:
			printf("DriverCallback() // CALLBACK_WINDOW !\n");
			break;
		case DCB_TASK:
			printf("DriverCallback() // CALLBACK_TASK !\n");
			break;
		case DCB_FUNCTION:
			printf("DriverCallback() // CALLBACK_FUNCTION !\n");
			break;
		}
	return TRUE;
}

/**************************************************************************
* 				JoyGetNumDevs		[MMSYSTEM.101]
*/
WORD JoyGetNumDevs()
{
	printf("EMPTY STUB !!! JoyGetNumDevs();\n");
	return 0;
}

/**************************************************************************
* 				JoyGetDevCaps		[MMSYSTEM.102]
*/
WORD JoyGetDevCaps(WORD wID, LPJOYCAPS lpCaps, WORD wSize)
{
	printf("EMPTY STUB !!! JoyGetDevCaps(%04X, %p, %d);\n", 
										wID, lpCaps, wSize);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoyGetPos			[MMSYSTEM.103]
*/
WORD JoyGetPos(WORD wID, LPJOYINFO lpInfo)
{
	printf("EMPTY STUB !!! JoyGetPos(%04X, %p);\n", wID, lpInfo);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoyGetThreshold		[MMSYSTEM.104]
*/
WORD JoyGetThreshold(WORD wID, LPWORD lpThreshold)
{
	printf("EMPTY STUB !!! JoyGetThreshold(%04X, %p);\n", wID, lpThreshold);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoyReleaseCapture	[MMSYSTEM.105]
*/
WORD JoyReleaseCapture(WORD wID)
{
	printf("EMPTY STUB !!! JoyReleaseCapture(%04X);\n", wID);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoySetCapture		[MMSYSTEM.106]
*/
WORD JoySetCapture(HWND hWnd, WORD wID, WORD wPeriod, BOOL bChanged)
{
	printf("EMPTY STUB !!! JoySetCapture(%04X, %04X, %d, %d);\n", 
							hWnd, wID, wPeriod, bChanged);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoySetThreshold		[MMSYSTEM.107]
*/
WORD JoySetThreshold(WORD wID, WORD wThreshold)
{
	printf("EMPTY STUB !!! JoySetThreshold(%04X, %d);\n", wID, wThreshold);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoySetCalibration	[MMSYSTEM.109]
*/
WORD JoySetCalibration(WORD wID)
{
	printf("EMPTY STUB !!! JoySetCalibration(%04X);\n", wID);
	return MMSYSERR_NODRIVER;
}


/**************************************************************************
* 				auxGetNumDevs		[MMSYSTEM.350]
*/
UINT WINAPI auxGetNumDevs()
{
	UINT	count = 0;
	printf("auxGetNumDevs !\n");
	count += auxMessage(0, AUXDM_GETNUMDEVS, 0L, 0L, 0L);
	printf("auxGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				auxGetDevCaps		[MMSYSTEM.351]
*/
UINT WINAPI auxGetDevCaps(UINT uDeviceID, AUXCAPS FAR* lpCaps, UINT uSize)
{
	printf("auxGetDevCaps(%04X, %p, %d) !\n", 
					uDeviceID, lpCaps, uSize);
	return auxMessage(uDeviceID, AUXDM_GETDEVCAPS, 
				0L, (DWORD)lpCaps, (DWORD)uSize);
}

/**************************************************************************
* 				auxGetVolume		[MMSYSTEM.352]
*/
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("auxGetVolume(%04X, %p) !\n", uDeviceID, lpdwVolume);
	return auxMessage(uDeviceID, AUXDM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
* 				auxSetVolume		[MMSYSTEM.353]
*/
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("auxSetVolume(%04X, %08lX) !\n", uDeviceID, dwVolume);
	return auxMessage(uDeviceID, AUXDM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
* 				auxOutMessage		[MMSYSTEM.354]
*/
DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2)
{
	printf("auxOutMessage(%04X, %04X, %08lX, %08lX)\n", 
				uDeviceID, uMessage, dw1, dw2);
	return auxMessage(uDeviceID, uMessage, 0L, dw1, dw2);
}

/**************************************************************************
* 				mciGetErrorString		[MMSYSTEM.706]
*/
BOOL mciGetErrorString (DWORD wError, LPSTR lpstrBuffer, UINT uLength)
{
	LPSTR	msgptr;
	int		maxbuf;
	printf("mciGetErrorString(%08lX, %p, %d);\n", wError, lpstrBuffer, uLength);
	if ((lpstrBuffer == NULL) || (uLength < 1)) return(FALSE);
	lpstrBuffer[0] = '\0';
	switch(wError) {
		case MCIERR_INVALID_DEVICE_ID:
			msgptr = "Invalid MCI device ID. Use the ID returned when opening the MCI device.";
			break;
		case MCIERR_UNRECOGNIZED_KEYWORD:
			msgptr = "The driver cannot recognize the specified command parameter.";
			break;
		case MCIERR_UNRECOGNIZED_COMMAND:
			msgptr = "The driver cannot recognize the specified command.";
			break;
		case MCIERR_HARDWARE:
			msgptr = "There is a problem with your media device. Make sure it is working correctly or contact the device manufacturer.";
			break;
		case MCIERR_INVALID_DEVICE_NAME:
			msgptr = "The specified device is not open or is not recognized by MCI.";
			break;
		case MCIERR_OUT_OF_MEMORY:
			msgptr = "Not enough memory available for this task. \nQuit one or more applications to increase available memory, and then try again.";
			break;
		case MCIERR_DEVICE_OPEN:
			msgptr = "The device name is already being used as an alias by this application. Use a unique alias.";
			break;
		case MCIERR_CANNOT_LOAD_DRIVER:
			msgptr = "There is an undetectable problem in loading the specified device driver.";
			break;
		case MCIERR_MISSING_COMMAND_STRING:
			msgptr = "No command was specified.";
			break;
		case MCIERR_PARAM_OVERFLOW:
			msgptr = "The output string was to large to fit in the return buffer. Increase the size of the buffer.";
			break;
		case MCIERR_MISSING_STRING_ARGUMENT:
			msgptr = "The specified command requires a character-string parameter. Please provide one.";
			break;
		case MCIERR_BAD_INTEGER:
			msgptr = "The specified integer is invalid for this command.";
			break;
		case MCIERR_PARSER_INTERNAL:
			msgptr = "The device driver returned an invalid return type. Check with the device manufacturer about obtaining a new driver.";
			break;
		case MCIERR_DRIVER_INTERNAL:
			msgptr = "There is a problem with the device driver. Check with the device manufacturer about obtaining a new driver.";
			break;
		case MCIERR_MISSING_PARAMETER:
			msgptr = "The specified command requires a parameter. Please supply one.";
			break;
		case MCIERR_UNSUPPORTED_FUNCTION:
			msgptr = "The MCI device you are using does not support the specified command.";
			break;
		case MCIERR_FILE_NOT_FOUND:
			msgptr = "Cannot find the specified file. Make sure the path and filename are correct.";
			break;
		case MCIERR_DEVICE_NOT_READY:
			msgptr = "The device driver is not ready.";
			break;
		case MCIERR_INTERNAL:
			msgptr = "A problem occurred in initializing MCI. Try restarting Windows.";
			break;
		case MCIERR_DRIVER:
			msgptr = "There is a problem with the device driver. The driver has closed. Cannot access error.";
			break;
		case MCIERR_CANNOT_USE_ALL:
			msgptr = "Cannot use 'all' as the device name with the specified command.";
			break;
		case MCIERR_MULTIPLE:
			msgptr = "Errors occurred in more than one device. Specify each command and device separately to determine which devices caused the error";
			break;
		case MCIERR_EXTENSION_NOT_FOUND:
			msgptr = "Cannot determine the device type from the given filename extension.";
			break;
		case MCIERR_OUTOFRANGE:
			msgptr = "The specified parameter is out of range for the specified command.";
			break;
		case MCIERR_FLAGS_NOT_COMPATIBLE:
			msgptr = "The specified parameters cannot be used together.";
			break;
		case MCIERR_FILE_NOT_SAVED:
			msgptr = "Cannot save the specified file. Make sure you have enough disk space or are still connected to the network.";
			break;
		case MCIERR_DEVICE_TYPE_REQUIRED:
			msgptr = "Cannot find the specified device. Make sure it is installed or that the device name is spelled correctly.";
			break;
		case MCIERR_DEVICE_LOCKED:
			msgptr = "The specified device is now being closed. Wait a few seconds, and then try again.";
			break;
		case MCIERR_DUPLICATE_ALIAS:
			msgptr = "The specified alias is already being used in this application. Use a unique alias.";
			break;
		case MCIERR_BAD_CONSTANT:
			msgptr = "The specified parameter is invalid for this command.";
			break;
		case MCIERR_MUST_USE_SHAREABLE:
			msgptr = "The device driver is already in use. To share it, use the 'shareable' parameter with each 'open' command.";
			break;
		case MCIERR_MISSING_DEVICE_NAME:
			msgptr = "The specified command requires an alias, file, driver, or device name. Please supply one.";
			break;
		case MCIERR_BAD_TIME_FORMAT:
			msgptr = "The specified value for the time format is invalid. Refer to the MCI documentation for valid formats.";
			break;
		case MCIERR_NO_CLOSING_QUOTE:
			msgptr = "A closing double-quotation mark is missing from the parameter value. Please supply one.";
			break;
		case MCIERR_DUPLICATE_FLAGS:
			msgptr = "A parameter or value was specified twice. Only specify it once.";
			break;
		case MCIERR_INVALID_FILE:
			msgptr = "The specified file cannot be played on the specified MCI device. The file may be corrupt, or not in the correct format.";
			break;
		case MCIERR_NULL_PARAMETER_BLOCK:
			msgptr = "A null parameter block was passed to MCI.";
			break;
		case MCIERR_UNNAMED_RESOURCE:
			msgptr = "Cannot save an unnamed file. Supply a filename.";
			break;
		case MCIERR_NEW_REQUIRES_ALIAS:
			msgptr = "You must specify an alias when using the 'new' parameter.";
			break;
		case MCIERR_NOTIFY_ON_AUTO_OPEN:
			msgptr = "Cannot use the 'notify' flag with auto-opened devices.";
			break;
		case MCIERR_NO_ELEMENT_ALLOWED:
			msgptr = "Cannot use a filename with the specified device.";
			break;
		case MCIERR_NONAPPLICABLE_FUNCTION:
			msgptr = "Cannot carry out the commands in the order specified. Correct the command sequence, and then try again.";
			break;
		case MCIERR_ILLEGAL_FOR_AUTO_OPEN:
			msgptr = "Cannot carry out the specified command on an auto-opened device. Wait until the device is closed, and then try again.";
			break;
		case MCIERR_FILENAME_REQUIRED:
			msgptr = "The filename is invalid. Make sure the filename is not longer than 8 characters, followed by a period and an extension.";
			break;
		case MCIERR_EXTRA_CHARACTERS:
			msgptr = "Cannot specify extra characters after a string enclosed in quotation marks.";
			break;
		case MCIERR_DEVICE_NOT_INSTALLED:
			msgptr = "The specified device is not installed on the system. Use the Drivers option in Control Panel to install the device.";
			break;
		case MCIERR_GET_CD:
			msgptr = "Cannot access the specified file or MCI device. Try changing directories or restarting your computer.";
			break;
		case MCIERR_SET_CD:
			msgptr = "Cannot access the specified file or MCI device because the application cannot change directories.";
			break;
		case MCIERR_SET_DRIVE:
			msgptr = "Cannot access specified file or MCI device because the application cannot change drives.";
			break;
		case MCIERR_DEVICE_LENGTH:
			msgptr = "Specify a device or driver name that is less than 79 characters.";
			break;
		case MCIERR_DEVICE_ORD_LENGTH:
			msgptr = "Specify a device or driver name that is less than 69 characters.";
			break;
		case MCIERR_NO_INTEGER:
			msgptr = "The specified command requires an integer parameter. Please provide one.";
			break;
		case MCIERR_WAVE_OUTPUTSINUSE:
			msgptr = "All wave devices that can play files in the current format are in use. Wait until a wave device is free, and then try again.";
			break;
		case MCIERR_WAVE_SETOUTPUTINUSE:
			msgptr = "Cannot set the current wave device for play back because it is in use. Wait until the device is free, and then try again.";
			break;
		case MCIERR_WAVE_INPUTSINUSE:
			msgptr = "All wave devices that can record files in the current format are in use. Wait until a wave device is free, and then try again.";
			break;
		case MCIERR_WAVE_SETINPUTINUSE:
			msgptr = "Cannot set the current wave device for recording because it is in use. Wait until the device is free, and then try again.";
			break;
		case MCIERR_WAVE_OUTPUTUNSPECIFIED:
			msgptr = "Any compatible waveform playback device may be used.";
			break;
		case MCIERR_WAVE_INPUTUNSPECIFIED:
			msgptr = "Any compatible waveform recording device may be used.";
			break;
		case MCIERR_WAVE_OUTPUTSUNSUITABLE:
			msgptr = "No wave device that can play files in the current format is installed. Use the Drivers option to install the wave device.";
			break;
		case MCIERR_WAVE_SETOUTPUTUNSUITABLE:
			msgptr = "The device you are trying to play to cannot recognize the current file format.";
			break;
		case MCIERR_WAVE_INPUTSUNSUITABLE:
			msgptr = "No wave device that can record files in the current format is installed. Use the Drivers option to install the wave device.";
			break;
		case MCIERR_WAVE_SETINPUTUNSUITABLE:
			msgptr = "The device you are trying to record from cannot recognize the current file format.";
			break;
		case MCIERR_NO_WINDOW:
			msgptr = "There is no display window.";
			break;
		case MCIERR_CREATEWINDOW:
			msgptr = "Could not create or use window.";
			break;
		case MCIERR_FILE_READ:
			msgptr = "Cannot read the specified file. Make sure the file is still present, or check your disk or network connection.";
			break;
		case MCIERR_FILE_WRITE:
			msgptr = "Cannot write to the specified file. Make sure you have enough disk space or are still connected to the network.";
			break;

/* 
#define MCIERR_SEQ_DIV_INCOMPATIBLE     (MCIERR_BASE + 80)
#define MCIERR_SEQ_PORT_INUSE           (MCIERR_BASE + 81)
#define MCIERR_SEQ_PORT_NONEXISTENT     (MCIERR_BASE + 82)
#define MCIERR_SEQ_PORT_MAPNODEVICE     (MCIERR_BASE + 83)
#define MCIERR_SEQ_PORT_MISCERROR       (MCIERR_BASE + 84)
#define MCIERR_SEQ_TIMER                (MCIERR_BASE + 85)
#define MCIERR_SEQ_PORTUNSPECIFIED      (MCIERR_BASE + 86)
#define MCIERR_SEQ_NOMIDIPRESENT        (MCIERR_BASE + 87)

msg# 513 : vcr
msg# 514 : videodisc
msg# 515 : overlay
msg# 516 : cdaudio
msg# 517 : dat
msg# 518 : scanner
msg# 519 : animation
msg# 520 : digitalvideo
msg# 521 : other
msg# 522 : waveaudio
msg# 523 : sequencer
msg# 524 : not ready
msg# 525 : stopped
msg# 526 : playing
msg# 527 : recording
msg# 528 : seeking
msg# 529 : paused
msg# 530 : open
msg# 531 : false
msg# 532 : true
msg# 533 : milliseconds
msg# 534 : hms
msg# 535 : msf
msg# 536 : frames
msg# 537 : smpte 24
msg# 538 : smpte 25
msg# 539 : smpte 30
msg# 540 : smpte 30 drop
msg# 541 : bytes
msg# 542 : samples
msg# 543 : tmsf
*/
		default:
			msgptr = "Unknown MCI Error !\n";
			break;
		}
	maxbuf = min(uLength - 1, strlen(msgptr));
	if (maxbuf > 0) strncpy(lpstrBuffer, msgptr, maxbuf);
	lpstrBuffer[maxbuf + 1] = '\0';
	return(TRUE);
}


/**************************************************************************
* 				mciDriverNotify			[MMSYSTEM.711]
*/
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, UINT wDevID, UINT wStatus)
{
	printf("mciDriverNotify(%04X, %u, %04X)\n", hWndCallBack, wDevID, wStatus);
	if (!IsWindow(hWndCallBack)) return FALSE;
	printf("mciDriverNotify // before PostMessage\n");
	PostMessage(hWndCallBack, MM_MCINOTIFY, wStatus, 
			MAKELONG(mciDrv[wDevID].wDeviceID, 0));
	return TRUE;
}

/**************************************************************************
* 				mciOpen					[internal]
*/
DWORD mciOpen(DWORD dwParam, LPMCI_OPEN_PARMS lp16Parms)
{
	char	str[128];
	LPMCI_OPEN_PARMS lpParms;
	UINT	uDevTyp = 0;
	UINT	wDevID = 0;
	lpParms = PTR_SEG_TO_LIN(lp16Parms);
	printf("mciOpen(%08lX, %p (%p))\n", dwParam, lp16Parms, lpParms);
	if (lp16Parms == NULL) return MCIERR_INTERNAL;
	while(mciDrv[wDevID].wType != 0) {
		if (++wDevID >= MAXMCIDRIVERS) {
			printf("MCI_OPEN // MAXMCIDRIVERS reached !\n");
			return MCIERR_INTERNAL;
			}
		}
	printf("mciOpen // wDevID=%d \n", wDevID);
	if (dwParam & MCI_OPEN_ALIAS) {
		printf("MCI_OPEN // Alias='%s' !\n",
			(char*)PTR_SEG_TO_LIN(lpParms->lpstrAlias));
		uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
 		}
	if (dwParam & MCI_OPEN_TYPE) {
		if (dwParam & MCI_OPEN_TYPE_ID) {
			printf("MCI_OPEN // Dev=%p !\n", lpParms->lpstrDeviceType);
			uDevTyp = LOWORD((DWORD)lpParms->lpstrDeviceType);
			}
		else {
			if (lpParms->lpstrDeviceType == NULL) return MCIERR_INTERNAL;
			printf("MCI_OPEN // Dev='%s' !\n",
                              (char*)PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
			strcpy(str, PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
			AnsiUpper(str);
			if (strcmp(str, "CDAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_CD_AUDIO;
				}
			else
			if (strcmp(str, "WAVEAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
				}
			else
			if (strcmp(str, "SEQUENCER") == 0)	{
				uDevTyp = MCI_DEVTYPE_SEQUENCER;
				}
			else
			if (strcmp(str, "ANIMATION1") == 0) {
				uDevTyp = MCI_DEVTYPE_ANIMATION;
				}
			else
			if (strcmp(str, "AVIVIDEO") == 0) {
				uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
				}
			}
		}
	mciDrv[wDevID].wType = uDevTyp;
	mciDrv[wDevID].wDeviceID = wDevID;
	lpParms->wDeviceID = wDevID;
	printf("MCI_OPEN // mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n", 
				wDevID, uDevTyp, lpParms->wDeviceID);
	switch(uDevTyp) {
		case MCI_DEVTYPE_CD_AUDIO:
#ifdef WINELIB
			WINELIB_UNIMP ("CDAUDIO_DriverProc");
#else
			return CDAUDIO_DriverProc(0, 0, MCI_OPEN_DRIVER,
							dwParam, (DWORD)lp16Parms);
#endif
		case MCI_DEVTYPE_WAVEFORM_AUDIO:
			return WAVE_DriverProc(0, 0, MCI_OPEN_DRIVER, 
								dwParam, (DWORD)lp16Parms);
		case MCI_DEVTYPE_SEQUENCER:
			return MIDI_DriverProc(0, 0, MCI_OPEN_DRIVER, 
								dwParam, (DWORD)lp16Parms);
		case MCI_DEVTYPE_ANIMATION:
			return ANIM_DriverProc(0, 0, MCI_OPEN_DRIVER, 
								dwParam, (DWORD)lp16Parms);
		case MCI_DEVTYPE_DIGITAL_VIDEO:
			printf("MCI_OPEN // No DIGITAL_VIDEO yet !\n");
			return MCIERR_DEVICE_NOT_INSTALLED;
		default:
			printf("MCI_OPEN // Invalid Device Name '%p' !\n", lpParms->lpstrDeviceType);
			return MCIERR_INVALID_DEVICE_NAME;
		}
	return MCIERR_INTERNAL;
}


/**************************************************************************
* 				mciClose				[internal]
*/
DWORD mciClose(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
	DWORD	dwRet = MCIERR_INTERNAL;
	printf("mciClose(%u, %08lX, %p)\n", wDevID, dwParam, lpParms);
	switch(mciDrv[wDevID].wType) {
		case MCI_DEVTYPE_CD_AUDIO:
#ifndef WINELIB
			dwRet = CDAUDIO_DriverProc(mciDrv[wDevID].wDeviceID, 0, 
						MCI_CLOSE, dwParam, (DWORD)lpParms);
#endif
			break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO:
			dwRet = WAVE_DriverProc(mciDrv[wDevID].wDeviceID, 0, 
						MCI_CLOSE, dwParam, (DWORD)lpParms);
			break;
		case MCI_DEVTYPE_SEQUENCER:
			dwRet = MIDI_DriverProc(mciDrv[wDevID].wDeviceID, 0, 
						MCI_CLOSE, dwParam, (DWORD)lpParms);
			break;
		case MCI_DEVTYPE_ANIMATION:
			dwRet = ANIM_DriverProc(mciDrv[wDevID].wDeviceID, 0, 
						MCI_CLOSE, dwParam, (DWORD)lpParms);
			break;
		default:
			printf("mciClose() // unknown device type=%04X !\n", mciDrv[wDevID].wType);
		}
	mciDrv[wDevID].wType = 0;
	return dwRet;
}


/**************************************************************************
* 				mciSound				[internal]
*/
DWORD mciSysInfo(DWORD dwFlags, LPMCI_SYSINFO_PARMS lpParms)
{
	int	len;
	LPSTR	ptr;
	LPSTR	lpstrReturn;
	DWORD	*lpdwRet;
	LPSTR	SysFile = "SYSTEM.INI";
	dprintf_mci(stddeb, "mciSysInfo(%08lX, %08lX)\n", dwFlags, (DWORD)lpParms);
	lpstrReturn = PTR_SEG_TO_LIN(lpParms->lpstrReturn);
	switch(dwFlags) {
		case MCI_SYSINFO_QUANTITY:
			dprintf_mci(stddeb, "mciSysInfo // MCI_SYSINFO_QUANTITY \n");
			lpdwRet = (DWORD *)lpstrReturn;
			*(lpdwRet) = InstalledCount;		
			return 0;
		case MCI_SYSINFO_INSTALLNAME:
			dprintf_mci(stddeb, "mciSysInfo // MCI_SYSINFO_INSTALLNAME \n");
			if (lpInstallNames == NULL) {
				InstalledCount = 0;
				InstalledListLen = 0;
				ptr = lpInstallNames = malloc(2048);
				GetPrivateProfileString("mci", NULL, "", lpInstallNames, 2000, SysFile);
				while(strlen(ptr) > 0) {
					printf("---> '%s' \n", ptr);
					len = strlen(ptr) + 1;
					ptr += len;
					InstalledListLen += len;
					InstalledCount++;
					}
				}
			if (lpParms->dwRetSize < InstalledListLen) {
				strncpy(lpstrReturn, lpInstallNames, lpParms->dwRetSize - 2);
				lpstrReturn[lpParms->dwRetSize - 1] = '\0';
				}
			else
				strcpy(lpstrReturn, lpInstallNames);
			return 0;
		case MCI_SYSINFO_NAME:
			dprintf_mci(stddeb, "mciSysInfo // MCI_SYSINFO_NAME \n");
			return 0;
		case MCI_SYSINFO_OPEN:
			dprintf_mci(stddeb, "mciSysInfo // MCI_SYSINFO_OPEN \n");
			return 0;
		}
	return MMSYSERR_INVALPARAM;
}

/**************************************************************************
* 				mciSound				[internal]
*/
DWORD mciSound(UINT wDevID, DWORD dwParam, LPMCI_SOUND_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwParam & MCI_SOUND_NAME)
		printf("MCI_SOUND // file='%s' !\n", lpParms->lpstrSoundName);
	return MCIERR_INVALID_DEVICE_ID;
}



/**************************************************************************
* 				mciSendCommand			[MMSYSTEM.701]
*/
DWORD mciSendCommand(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
	HDRVR	hDrv = 0;
	dprintf_mci(stddeb, "mciSendCommand(%04X, %04X, %08lX, %08lX)\n", 
					wDevID, wMsg, dwParam1, dwParam2);
	switch(wMsg) {
		case MCI_OPEN:
			return mciOpen(dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
		case MCI_CLOSE:
			return mciClose(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
		case MCI_SYSINFO:
			return mciSysInfo(dwParam1, (LPMCI_SYSINFO_PARMS)PTR_SEG_TO_LIN(dwParam2));
		default:
			switch(mciDrv[wDevID].wType) {
				case MCI_DEVTYPE_CD_AUDIO:
#ifndef WINELIB
					return CDAUDIO_DriverProc(mciDrv[wDevID].wDeviceID, hDrv, 
											wMsg, dwParam1, dwParam2);
#endif
					
				case MCI_DEVTYPE_WAVEFORM_AUDIO:
					return WAVE_DriverProc(mciDrv[wDevID].wDeviceID, hDrv, 
											wMsg, dwParam1, dwParam2);
				case MCI_DEVTYPE_SEQUENCER:
					return MIDI_DriverProc(mciDrv[wDevID].wDeviceID, hDrv, 
											wMsg, dwParam1, dwParam2);
				case MCI_DEVTYPE_ANIMATION:
					return ANIM_DriverProc(mciDrv[wDevID].wDeviceID, hDrv, 
											wMsg, dwParam1, dwParam2);
				default:
					printf("mciSendCommand() // unknown device type=%04X !\n", 
											mciDrv[wDevID].wType);
				}
		}
	return MMSYSERR_INVALPARAM;
}

/**************************************************************************
* 				mciGetDeviceID			[MMSYSTEM.703]
*/
UINT mciGetDeviceID (LPCSTR lpstrName)
{
	char	str[128];
	printf("mciGetDeviceID(%s)\n", lpstrName);
	if (lpstrName != NULL) {
		strcpy(str, lpstrName);
		AnsiUpper(str);
		if (strcmp(str, "ALL") == 0) return MCI_ALL_DEVICE_ID;
		}
	return 0;
}

/**************************************************************************
* 				mciSendString			[MMSYSTEM.702]
*/
DWORD WINAPI mciSendString (LPCSTR lpstrCommand,
    LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
{
	printf("mciSendString('%s', %p, %u, %X)\n", 
			lpstrCommand, lpstrReturnString, 
			uReturnLength, hwndCallback);
	return MCIERR_MISSING_COMMAND_STRING;
}

/**************************************************************************
* 				mciSetYieldProc		[MMSYSTEM.714]
*/
BOOL WINAPI mciSetYieldProc (UINT uDeviceID, 
		YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    return FALSE;
}

/**************************************************************************
* 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
*/
UINT WINAPI mciGetDeviceIDFromElementID(DWORD dwElementID, LPCSTR lpstrType)
{
    return 0;
}

/**************************************************************************
* 				mciGetYieldProc		[MMSYSTEM.716]
*/
YIELDPROC WINAPI mciGetYieldProc(UINT uDeviceID, DWORD FAR* lpdwYieldData)
{
    return NULL;
}

/**************************************************************************
* 				mciGetCreatorTask	[MMSYSTEM.717]
*/
HTASK WINAPI mciGetCreatorTask(UINT uDeviceID)
{
    return 0;
}

/**************************************************************************
* 				midiOutGetNumDevs	[MMSYSTEM.201]
*/
UINT WINAPI midiOutGetNumDevs(void)
{
	UINT	count = 0;
	printf("midiOutGetNumDevs\n");
	count += modMessage(0, MODM_GETNUMDEVS, 0L, 0L, 0L);
	printf("midiOutGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				midiOutGetDevCaps	[MMSYSTEM.202]
*/
UINT WINAPI midiOutGetDevCaps(UINT uDeviceID,
    MIDIOUTCAPS FAR* lpCaps, UINT uSize)
{
	printf("midiOutGetDevCaps\n");
	return 0;
}

/**************************************************************************
* 				midiOutGetErrorText 	[MMSYSTEM.203]
*/
UINT WINAPI midiOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	printf("midiOutGetErrorText\n");
	return(midiGetErrorText(uError, lpText, uSize));
}


/**************************************************************************
* 				midiGetErrorText 		[internal]
*/
UINT WINAPI midiGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	LPSTR	msgptr;
	int		maxbuf;
	if ((lpText == NULL) || (uSize < 1)) return(FALSE);
	lpText[0] = '\0';
	switch(uError) {
		case MIDIERR_UNPREPARED:
			msgptr = "The MIDI header was not prepared. Use the Prepare function to prepare the header, and then try again.";
			break;
		case MIDIERR_STILLPLAYING:
			msgptr = "Cannot perform this operation while media data is still playing. Reset the device, or wait until the data is finished playing.";
			break;
		case MIDIERR_NOMAP:
			msgptr = "A MIDI map was not found. There may be a problem with the driver, or the MIDIMAP.CFG file may be corrupt or missing.";
			break;
		case MIDIERR_NOTREADY:
			msgptr = "The port is transmitting data to the device. Wait until the data has been transmitted, and then try again.";
			break;
		case MIDIERR_NODEVICE:
			msgptr = "The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use MIDI Mapper to edit the setup.";
			break;
		case MIDIERR_INVALIDSETUP:
			msgptr = "The current MIDI setup is damaged. Copy the original MIDIMAP.CFG file to the Windows SYSTEM directory, and then try again.";
			break;
/*
msg# 336 : Cannot use the song-pointer time format and the SMPTE time-format together.
msg# 337 : The specified MIDI device is already in use. Wait until it is free, and then try again.
msg# 338 : The specified MIDI device is not installed on the system. Use the Drivers option in Control Panel to install the driver.
msg# 339 : The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use MIDI Mapper to edit the setup.
msg# 340 : An error occurred using the specified port.
msg# 341 : All multimedia timers are being used by other applications. Quit one of these applications, and then try again.
msg# 342 : There is no current MIDI port.
msg# 343 : There are no MIDI devices installed on the system. Use the Drivers option in Control Panel to install the driver.
*/
		default:
			msgptr = "Unknown MIDI Error !\n";
			break;
		}
	maxbuf = min(uSize - 1, strlen(msgptr));
	if (maxbuf > 0) strncpy(lpText, msgptr, maxbuf);
	lpText[maxbuf + 1] = '\0';
	return(TRUE);
}

/**************************************************************************
* 				midiOutOpen			[MMSYSTEM.204]
*/
UINT WINAPI midiOutOpen(HMIDIOUT FAR* lphMidiOut, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HMIDI	hMidiOut;
	LPMIDIOPENDESC	lpDesc;
	LPMIDIOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL	bMapperFlg = FALSE;
	if (lphMidiOut != NULL) *lphMidiOut = 0;
	printf("midiOutOpen(%p, %d, %08lX, %08lX, %08lX);\n", 
		lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);
	if (uDeviceID == (UINT)MIDI_MAPPER) {
		printf("midiOutOpen	// MIDI_MAPPER mode requested !\n");
		bMapperFlg = TRUE;
		uDeviceID = 0;
		}
	hMidiOut = USER_HEAP_ALLOC(sizeof(MIDIOPENDESC));
	if (lphMidiOut != NULL) *lphMidiOut = hMidiOut;
	lp16Desc = (LPMIDIOPENDESC) USER_HEAP_SEG_ADDR(hMidiOut);
	lpDesc = (LPMIDIOPENDESC) PTR_SEG_TO_LIN(lp16Desc);
	if (lpDesc == NULL) return MMSYSERR_NOMEM;
	lpDesc->hMidi = hMidiOut;
	lpDesc->dwCallback = dwCallback;
	lpDesc->dwInstance = dwInstance;
	while(uDeviceID < MAXMIDIDRIVERS) {
		dwRet = modMessage(uDeviceID, MODM_OPEN, 
			lpDesc->dwInstance, (DWORD)lp16Desc, 0L);
		if (dwRet == MMSYSERR_NOERROR) break;
		if (!bMapperFlg) break;
		uDeviceID++;
		printf("midiOutOpen	// MIDI_MAPPER mode ! try next driver...\n");
		}
	return dwRet;
}

/**************************************************************************
* 				midiOutClose		[MMSYSTEM.205]
*/
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutClose(%04X)\n", hMidiOut);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiOutPrepareHeader	[MMSYSTEM.206]
*/
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutPrepareHeader(%04X, %p, %d)\n", 
					hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_PREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutUnprepareHeader	[MMSYSTEM.207]
*/
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutUnprepareHeader(%04X, %p, %d)\n", 
					hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_UNPREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutShortMsg		[MMSYSTEM.208]
*/
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutShortMsg(%04X, %08lX)\n", hMidiOut, dwMsg);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_DATA, lpDesc->dwInstance, dwMsg, 0L);
}

/**************************************************************************
* 				midiOutLongMsg		[MMSYSTEM.209]
*/
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutLongMsg(%04X, %p, %d)\n", 
				hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_LONGDATA, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutReset		[MMSYSTEM.210]
*/
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutReset(%04X)\n", hMidiOut);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiOutGetVolume	[MMSYSTEM.211]
*/
UINT WINAPI midiOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("midiOutGetVolume(%04X, %p);\n", uDeviceID, lpdwVolume);
	return modMessage(uDeviceID, MODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
	return 0;
}

/**************************************************************************
* 				midiOutSetVolume	[MMSYSTEM.212]
*/
UINT WINAPI midiOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("midiOutSetVolume(%04X, %08lX);\n", uDeviceID, dwVolume);
	return modMessage(uDeviceID, MODM_SETVOLUME, 0L, dwVolume, 0L);
	return 0;
}

/**************************************************************************
* 				midiOutCachePatches		[MMSYSTEM.213]
*/
UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut,
    UINT uBank, WORD FAR* lpwPatchArray, UINT uFlags)
{
	printf("midiOutCachePatches\n");
	return 0;
}

/**************************************************************************
* 				midiOutCacheDrumPatches	[MMSYSTEM.214]
*/
UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut,
    UINT uPatch, WORD FAR* lpwKeyArray, UINT uFlags)
{
	printf("midiOutCacheDrumPatches\n");
	return 0;
}

/**************************************************************************
* 				midiOutGetID		[MMSYSTEM.215]
*/
UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT FAR* lpuDeviceID)
{
	printf("midiOutGetID\n");
	return 0;
}

/**************************************************************************
* 				midiOutMessage		[MMSYSTEM.216]
*/
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage, 
						DWORD dwParam1, DWORD dwParam2)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiOutMessage(%04X, %04X, %08lX, %08lX)\n", 
			hMidiOut, uMessage, dwParam1, dwParam2);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
	return 0;
}

/**************************************************************************
* 				midiInGetNumDevs	[MMSYSTEM.301]
*/
UINT WINAPI midiInGetNumDevs(void)
{
	UINT	count = 0;
	printf("midiInGetNumDevs\n");
	count += midMessage(0, MIDM_GETNUMDEVS, 0L, 0L, 0L);
	printf("midiInGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				midiInGetDevCaps	[MMSYSTEM.302]
*/
UINT WINAPI midiInGetDevCaps(UINT uDeviceID,
    LPMIDIINCAPS lpCaps, UINT uSize)
{
	printf("midiInGetDevCaps\n");
	return 0;
}

/**************************************************************************
* 				midiInGetErrorText 		[MMSYSTEM.303]
*/
UINT WINAPI midiInGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	printf("midiInGetErrorText\n");
	return (midiGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
* 				midiInOpen		[MMSYSTEM.304]
*/
UINT WINAPI midiInOpen(HMIDIIN FAR* lphMidiIn, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HMIDI	hMidiIn;
	LPMIDIOPENDESC	lpDesc;
	LPMIDIOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL	bMapperFlg = FALSE;
	if (lphMidiIn != NULL) *lphMidiIn = 0;
	printf("midiInOpen(%p, %d, %08lX, %08lX, %08lX);\n", 
		lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);
	if (uDeviceID == (UINT)MIDI_MAPPER) {
		printf("midiInOpen	// MIDI_MAPPER mode requested !\n");
		bMapperFlg = TRUE;
		uDeviceID = 0;
		}
	hMidiIn = USER_HEAP_ALLOC(sizeof(MIDIOPENDESC));
	if (lphMidiIn != NULL) *lphMidiIn = hMidiIn;
	lp16Desc = (LPMIDIOPENDESC) USER_HEAP_SEG_ADDR(hMidiIn);
	lpDesc = (LPMIDIOPENDESC) PTR_SEG_TO_LIN(lp16Desc);
	if (lpDesc == NULL) return MMSYSERR_NOMEM;
	lpDesc->hMidi = hMidiIn;
	lpDesc->dwCallback = dwCallback;
	lpDesc->dwInstance = dwInstance;
	while(uDeviceID < MAXMIDIDRIVERS) {
		dwRet = midMessage(uDeviceID, MIDM_OPEN, 
			lpDesc->dwInstance, (DWORD)lpDesc, 0L);
		if (dwRet == MMSYSERR_NOERROR) break;
		if (!bMapperFlg) break;
		uDeviceID++;
		printf("midiInOpen	// MIDI_MAPPER mode ! try next driver...\n");
		}
	return dwRet;
}

/**************************************************************************
* 				midiInClose		[MMSYSTEM.305]
*/
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiInClose(%04X)\n", hMidiIn);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiInPrepareHeader	[MMSYSTEM.306]
*/
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiInPrepareHeader(%04X, %p, %d)\n", 
					hMidiIn, lpMidiInHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_PREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiInUnprepareHeader	[MMSYSTEM.307]
*/
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiInUnprepareHeader(%04X, %p, %d)\n", 
					hMidiIn, lpMidiInHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_UNPREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiInAddBuffer		[MMSYSTEM.308]
*/
UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInAddBuffer\n");
	return 0;
}

/**************************************************************************
* 				midiInStart			[MMSYSTEM.309]
*/
UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
	printf("midiInStart\n");
	return 0;
}

/**************************************************************************
* 				midiInStop			[MMSYSTEM.310]
*/
UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
	printf("midiInStop\n");
	return 0;
}

/**************************************************************************
* 				midiInReset			[MMSYSTEM.311]
*/
UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
	printf("midiInReset\n");
	return 0;
}

/**************************************************************************
* 				midiInGetID			[MMSYSTEM.312]
*/
UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT FAR* lpuDeviceID)
{
	printf("midiInGetID\n");
	return 0;
}

/**************************************************************************
* 				midiInMessage		[MMSYSTEM.313]
*/
DWORD WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage, 
							DWORD dwParam1, DWORD dwParam2)
{
	LPMIDIOPENDESC	lpDesc;
	printf("midiInMessage(%04X, %04X, %08lX, %08lX)\n", 
			hMidiIn, uMessage, dwParam1, dwParam2);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}


/**************************************************************************
* 				waveOutGetNumDevs		[MMSYSTEM.401]
*/
UINT WINAPI waveOutGetNumDevs()
{
	UINT	count = 0;
	printf("waveOutGetNumDevs\n");
	count += wodMessage(0, WODM_GETNUMDEVS, 0L, 0L, 0L);
	printf("waveOutGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				waveOutGetDevCaps		[MMSYSTEM.402]
*/
UINT WINAPI waveOutGetDevCaps(UINT uDeviceID, WAVEOUTCAPS FAR* lpCaps, UINT uSize)
{
	printf("waveOutGetDevCaps\n");
	return wodMessage(uDeviceID, WODM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}

/**************************************************************************
* 				waveOutGetErrorText 	[MMSYSTEM.403]
*/
UINT WINAPI waveOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
   printf("waveOutGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


/**************************************************************************
* 				waveGetErrorText 		[internal]
*/
UINT WINAPI waveGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	LPSTR	msgptr;
	int		maxbuf;
	printf("waveGetErrorText(%04X, %p, %d);\n", uError, lpText, uSize);
	if ((lpText == NULL) || (uSize < 1)) return(FALSE);
	lpText[0] = '\0';
	switch(uError) {
		case MMSYSERR_NOERROR:
			msgptr = "The specified command was carried out.";
			break;
		case MMSYSERR_ERROR:
			msgptr = "Undefined external error.";
			break;
		case MMSYSERR_BADDEVICEID:
			msgptr = "A device ID has been used that is out of range for your system.";
			break;
		case MMSYSERR_NOTENABLED:
			msgptr = "The driver was not enabled.";
			break;
		case MMSYSERR_ALLOCATED:
			msgptr = "The specified device is already in use. Wait until it is free, and then try again.";
			break;
		case MMSYSERR_INVALHANDLE:
			msgptr = "The specified device handle is invalid.";
			break;
		case MMSYSERR_NODRIVER:
			msgptr = "There is no driver installed on your system !\n";
			break;
		case MMSYSERR_NOMEM:
			msgptr = "Not enough memory available for this task. Quit one or more applications to increase available memory, and then try again.";
			break;
		case MMSYSERR_NOTSUPPORTED:
			msgptr = "This function is not supported. Use the Capabilities function to determine which functions and messages the driver supports.";
			break;
		case MMSYSERR_BADERRNUM:
			msgptr = "An error number was specified that is not defined in the system.";
			break;
		case MMSYSERR_INVALFLAG:
			msgptr = "An invalid flag was passed to a system function.";
			break;
		case MMSYSERR_INVALPARAM:
			msgptr = "An invalid parameter was passed to a system function.";
			break;
		case WAVERR_BADFORMAT:
			msgptr = "The specified format is not supported or cannot be translated. Use the Capabilities function to determine the supported formats";
			break;
		case WAVERR_STILLPLAYING:
			msgptr = "Cannot perform this operation while media data is still playing. Reset the device, or wait until the data is finished playing.";
			break;
		case WAVERR_UNPREPARED:
			msgptr = "The wave header was not prepared. Use the Prepare function to prepare the header, and then try again.";
			break;
		case WAVERR_SYNC:
			msgptr = "Cannot open the device without using the WAVE_ALLOWSYNC flag. Use the flag, and then try again.";
			break;
		default:
			msgptr = "Unknown MMSYSTEM Error !\n";
			break;
		}
	maxbuf = min(uSize - 1, strlen(msgptr));
	if (maxbuf > 0) strncpy(lpText, msgptr, maxbuf);
	lpText[maxbuf + 1] = '\0';
	return(TRUE);
}

/**************************************************************************
* 				waveOutOpen			[MMSYSTEM.404]
*/
UINT WINAPI waveOutOpen(HWAVEOUT FAR* lphWaveOut, UINT uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HWAVE	hWaveOut;
	LPWAVEOPENDESC	lpDesc;
	LPWAVEOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL	bMapperFlg = FALSE;
	printf("waveOutOpen(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
		lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveOutOpen	// WAVE_FORMAT_QUERY requested !\n");
		}
	if (uDeviceID == (UINT)WAVE_MAPPER) {
		printf("waveOutOpen	// WAVE_MAPPER mode requested !\n");
		bMapperFlg = TRUE;
		uDeviceID = 0;
		}
	if (lpFormat == NULL) return WAVERR_BADFORMAT;
	hWaveOut = USER_HEAP_ALLOC(sizeof(WAVEOPENDESC));
	if (lphWaveOut != NULL) *lphWaveOut = hWaveOut;
	lp16Desc = (LPWAVEOPENDESC) USER_HEAP_SEG_ADDR(hWaveOut);
	lpDesc = (LPWAVEOPENDESC) PTR_SEG_TO_LIN(lp16Desc);
	if (lpDesc == NULL) return MMSYSERR_NOMEM;
	lpDesc->hWave = hWaveOut;
	lpDesc->lpFormat = lpFormat;
	lpDesc->dwCallBack = dwCallback;
	lpDesc->dwInstance = dwInstance;
	while(uDeviceID < MAXWAVEDRIVERS) {
		dwRet = wodMessage(uDeviceID, WODM_OPEN, 
			lpDesc->dwInstance, (DWORD)lp16Desc, 0L);
		if (dwRet == MMSYSERR_NOERROR) break;
		if (!bMapperFlg) break;
		uDeviceID++;
		printf("waveOutOpen	// WAVE_MAPPER mode ! try next driver...\n");
		}
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveOutOpen	// End of WAVE_FORMAT_QUERY !\n");
		waveOutClose(hWaveOut);
		}
	return dwRet;
}

/**************************************************************************
* 				waveOutClose		[MMSYSTEM.405]
*/
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutClose(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutPrepareHeader	[MMSYSTEM.406]
*/
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
     WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutPrepareHeader(%04X, %p, %u);\n", 
					hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_PREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutUnprepareHeader	[MMSYSTEM.407]
*/
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
    WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutUnprepareHeader(%04X, %p, %u);\n", 
						hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_UNPREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutWrite		[MMSYSTEM.408]
*/
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, WAVEHDR FAR* lpWaveOutHdr,  UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutWrite(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_WRITE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutPause		[MMSYSTEM.409]
*/
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutPause(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_PAUSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutRestart		[MMSYSTEM.410]
*/
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutRestart(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_RESTART, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutReset		[MMSYSTEM.411]
*/
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutReset(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutGetPosition	[MMSYSTEM.412]
*/
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, MMTIME FAR* lpTime, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutGetPosition(%04X, %p, %u);\n", hWaveOut, lpTime, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_GETPOS, lpDesc->dwInstance, 
							(DWORD)lpTime, (DWORD)uSize);
}

/**************************************************************************
* 				waveOutGetPitch		[MMSYSTEM.413]
*/
UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, DWORD FAR* lpdwPitch)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutGetPitch(%04X, %p);\n", hWaveOut, lpdwPitch);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_GETPITCH, lpDesc->dwInstance, 
								(DWORD)lpdwPitch, 0L);
}

/**************************************************************************
* 				waveOutSetPitch		[MMSYSTEM.414]
*/
UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dwPitch)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutSetPitch(%04X, %08lX);\n", hWaveOut, dwPitch);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_SETPITCH, lpDesc->dwInstance, (DWORD)dwPitch, 0L);
}

/**************************************************************************
* 				waveOutGetVolume	[MMSYSTEM.415]
*/
UINT WINAPI waveOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("waveOutGetVolume(%04X, %p);\n", uDeviceID, lpdwVolume);
	return wodMessage(uDeviceID, WODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
* 				waveOutSetVolume	[MMSYSTEM.416]
*/
UINT WINAPI waveOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("waveOutSetVolume(%04X, %08lX);\n", uDeviceID, dwVolume);
	return wodMessage(uDeviceID, WODM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
* 				waveOutGetPlaybackRate	[MMSYSTEM.417]
*/
UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, DWORD FAR* lpdwRate)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutGetPlaybackRate(%04X, %p);\n", hWaveOut, lpdwRate);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_GETPLAYBACKRATE, lpDesc->dwInstance, 
								(DWORD)lpdwRate, 0L);
}

/**************************************************************************
* 				waveOutSetPlaybackRate	[MMSYSTEM.418]
*/
UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dwRate)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutSetPlaybackRate(%04X, %08lX);\n", hWaveOut, dwRate);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, WODM_SETPLAYBACKRATE, 
		lpDesc->dwInstance, (DWORD)dwRate, 0L);
}

/**************************************************************************
* 				waveOutBreakLoop 	[MMSYSTEM.419]
*/
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
	printf("waveOutBreakLoop(%04X)\n", hWaveOut);
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetID	 	[MMSYSTEM.420]
*/
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT FAR* lpuDeviceID)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutGetID(%04X, %p);\n", hWaveOut, lpuDeviceID);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
/*
	*lpuDeviceID = lpParms->wDeviceID; 
*/
	return 0;
}

/**************************************************************************
* 				waveOutMessage 		[MMSYSTEM.421]
*/
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage, 
							DWORD dwParam1, DWORD dwParam2)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveOutMessage(%04X, %04X, %08lX, %08lX)\n", 
			hWaveOut, uMessage, dwParam1, dwParam2);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
* 				waveInGetNumDevs 		[MMSYSTEM.501]
*/
UINT WINAPI waveInGetNumDevs()
{
	UINT	count = 0;
	printf("waveInGetNumDevs\n");
	count += widMessage(0, WIDM_GETNUMDEVS, 0L, 0L, 0L);
	printf("waveInGetNumDevs return %u \n", count);
	return count;
}


/**************************************************************************
* 				waveInGetDevCaps 		[MMSYSTEM.502]
*/
UINT WINAPI waveInGetDevCaps(UINT uDeviceID, WAVEINCAPS FAR* lpCaps, UINT uSize)
{
	printf("waveInGetDevCaps\n");
	return widMessage(uDeviceID, WIDM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}


/**************************************************************************
* 				waveInGetErrorText 	[MMSYSTEM.503]
*/
UINT WINAPI waveInGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
   printf("waveInGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


/**************************************************************************
* 				waveInOpen			[MMSYSTEM.504]
*/
UINT WINAPI waveInOpen(HWAVEIN FAR* lphWaveIn, UINT uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HWAVE	hWaveIn;
	LPWAVEOPENDESC	lpDesc;
	LPWAVEOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL	bMapperFlg = FALSE;
	printf("waveInOpen(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
		lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveInOpen // WAVE_FORMAT_QUERY requested !\n");
		}
	if (uDeviceID == (UINT)WAVE_MAPPER) {
		printf("waveInOpen	// WAVE_MAPPER mode requested !\n");
		bMapperFlg = TRUE;
		uDeviceID = 0;
		}
	if (lpFormat == NULL) return WAVERR_BADFORMAT;
	hWaveIn = USER_HEAP_ALLOC(sizeof(WAVEOPENDESC));
	if (lphWaveIn != NULL) *lphWaveIn = hWaveIn;
	lp16Desc = (LPWAVEOPENDESC) USER_HEAP_SEG_ADDR(hWaveIn);
	lpDesc = (LPWAVEOPENDESC) PTR_SEG_TO_LIN(lp16Desc);
	if (lpDesc == NULL) return MMSYSERR_NOMEM;
	lpDesc->hWave = hWaveIn;
	lpDesc->lpFormat = lpFormat;
	lpDesc->dwCallBack = dwCallback;
	lpDesc->dwInstance = dwInstance;
	while(uDeviceID < MAXWAVEDRIVERS) {
		dwRet = widMessage(uDeviceID, WIDM_OPEN, 
			lpDesc->dwInstance, (DWORD)lp16Desc, 0L);
		if (dwRet == MMSYSERR_NOERROR) break;
		if (!bMapperFlg) break;
		uDeviceID++;
		printf("waveInOpen	// WAVE_MAPPER mode ! try next driver...\n");
		}
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveInOpen	// End of WAVE_FORMAT_QUERY !\n");
		waveInClose(hWaveIn);
		}
	return dwRet;
}


/**************************************************************************
* 				waveInClose			[MMSYSTEM.505]
*/
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInClose(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, WIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInPrepareHeader		[MMSYSTEM.506]
*/
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;
	printf("waveInPrepareHeader(%04X, %p, %u);\n", 
					hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	lp32WaveInHdr->lpNext = NULL;
    lp32WaveInHdr->dwBytesRecorded = 0;
	printf("waveInPrepareHeader // lpData=%p size=%lu \n", 
		lp32WaveInHdr->lpData, lp32WaveInHdr->dwBufferLength);
	return widMessage(0, WIDM_PREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInUnprepareHeader	[MMSYSTEM.507]
*/
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;
	printf("waveInUnprepareHeader(%04X, %p, %u);\n", 
						hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	USER_HEAP_FREE(HIWORD((DWORD)lp32WaveInHdr->lpData));
	lp32WaveInHdr->lpData = NULL;
	lp32WaveInHdr->lpNext = NULL;
	return widMessage(0, WIDM_UNPREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInAddBuffer		[MMSYSTEM.508]
*/
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;
	printf("waveInAddBuffer(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	lp32WaveInHdr->lpNext = NULL;
    lp32WaveInHdr->dwBytesRecorded = 0;
	printf("waveInAddBuffer // lpData=%p size=%lu \n", 
		lp32WaveInHdr->lpData, lp32WaveInHdr->dwBufferLength);
	return widMessage(0, WIDM_ADDBUFFER, lpDesc->dwInstance,
								(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInStart			[MMSYSTEM.509]
*/
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInStart(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, WIDM_START, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInStop			[MMSYSTEM.510]
*/
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInStop(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, WIDM_STOP, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInReset			[MMSYSTEM.511]
*/
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInReset(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, WIDM_RESET, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInGetPosition	[MMSYSTEM.512]
*/
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, MMTIME FAR* lpTime, UINT uSize)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInGetPosition(%04X, %p, %u);\n", hWaveIn, lpTime, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, WIDM_GETPOS, lpDesc->dwInstance,
							(DWORD)lpTime, (DWORD)uSize);
}


/**************************************************************************
* 				waveInGetID			[MMSYSTEM.513]
*/
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT FAR* lpuDeviceID)
{
	printf("waveInGetID\n");
	if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
	return 0;
}


/**************************************************************************
* 				waveInMessage 		[MMSYSTEM.514]
*/
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
							DWORD dwParam1, DWORD dwParam2)
{
	LPWAVEOPENDESC	lpDesc;
	printf("waveInMessage(%04X, %04X, %08lX, %08lX)\n", 
			hWaveIn, uMessage, dwParam1, dwParam2);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}


/**************************************************************************
* 				MMSysTimeCallback	[internal]
*/
WORD FAR PASCAL MMSysTimeCallback(HWND hWnd, WORD wMsg, int nID, DWORD dwTime)
{
	LPTIMERENTRY	lpTimer = lpTimerList;
	mmSysTimeMS.u.ms += 33;
	mmSysTimeSMPTE.u.smpte.frame++;
	while (lpTimer != NULL) {
		lpTimer->wCurTime--;
		if (lpTimer->wCurTime == 0) {
			lpTimer->wCurTime = lpTimer->wDelay;
			if (lpTimer->lpFunc != (FARPROC)NULL) {
				dprintf_mmtime(stddeb,"MMSysTimeCallback // before CallBack16 !\n");
                                CallTimeFuncProc( lpTimer->lpFunc,
                                                  lpTimer->wTimerID, 0,
                                                  lpTimer->dwUser, 0, 0 );
				dprintf_mmtime(stddeb, "MMSysTimeCallback // after CallBack16 !\n");
				fflush(stdout);
				}
			if (lpTimer->wFlags & TIME_ONESHOT)
				timeKillEvent(lpTimer->wTimerID);
			}
		lpTimer = lpTimer->Next;
		}
	return 0;
}

/**************************************************************************
* 				StartMMTime			[internal]
*/
void StartMMTime()
{
	if (!mmTimeStarted) {
		mmTimeStarted = TRUE;
		mmSysTimeMS.wType = TIME_MS;
		mmSysTimeMS.u.ms = 0;
		mmSysTimeSMPTE.wType = TIME_SMPTE;
		mmSysTimeSMPTE.u.smpte.hour = 0;
		mmSysTimeSMPTE.u.smpte.min = 0;
		mmSysTimeSMPTE.u.smpte.sec = 0;
		mmSysTimeSMPTE.u.smpte.frame = 0;
		mmSysTimeSMPTE.u.smpte.fps = 0;
		mmSysTimeSMPTE.u.smpte.dummy = 0;
		SetTimer(0, 1, 33, (FARPROC)MMSysTimeCallback);
		}
}

/**************************************************************************
* 				timeGetSystemTime	[MMSYSTEM.601]
*/
WORD timeGetSystemTime(LPMMTIME lpTime, WORD wSize)
{
	printf("timeGetSystemTime(%p, %u);\n", lpTime, wSize);
	if (!mmTimeStarted) StartMMTime();
	return 0;
}

/**************************************************************************
* 				timeSetEvent		[MMSYSTEM.602]
*/
WORD timeSetEvent(WORD wDelay, WORD wResol, 
				LPTIMECALLBACK lpFunc, 
				DWORD dwUser, WORD wFlags)
{
	WORD			wNewID = 0;
	LPTIMERENTRY	lpNewTimer;
	LPTIMERENTRY	lpTimer = lpTimerList;
	printf("timeSetEvent(%u, %u, %p, %08lX, %04X);\n",
			wDelay, wResol, lpFunc, dwUser, wFlags);
	if (!mmTimeStarted) StartMMTime();
	lpNewTimer = (LPTIMERENTRY) malloc(sizeof(TIMERENTRY));
	if (lpNewTimer == NULL)	return 0;
	while (lpTimer != NULL) {
		wNewID = max(wNewID, lpTimer->wTimerID);
		if (lpTimer->Next == NULL) break;
		lpTimer = lpTimer->Next;
		}
	if (lpTimerList == NULL) {
		lpTimerList = lpNewTimer;
		lpNewTimer->Prev = NULL;
		}
	else {
		lpTimer->Next = lpNewTimer;
		lpNewTimer->Prev = lpTimer;
		}
	lpNewTimer->Next = NULL;
	lpNewTimer->wTimerID = wNewID + 1;
	lpNewTimer->wCurTime = wDelay;
	lpNewTimer->wDelay = wDelay;
	lpNewTimer->wResol = wResol;
	lpNewTimer->lpFunc = (FARPROC)lpFunc;
	lpNewTimer->dwUser = dwUser;
	lpNewTimer->wFlags = wFlags;
	return lpNewTimer->wTimerID;
}

/**************************************************************************
* 				timeKillEvent		[MMSYSTEM.603]
*/
WORD timeKillEvent(WORD wID)
{
	LPTIMERENTRY	lpTimer = lpTimerList;
	while (lpTimer != NULL) {
		if (wID == lpTimer->wTimerID) {
			if (lpTimer->Prev != NULL) lpTimer->Prev->Next = lpTimer->Next;
			if (lpTimer->Next != NULL) lpTimer->Next->Prev = lpTimer->Prev;
			free(lpTimer);
			return TRUE;
			}
		lpTimer = lpTimer->Next;
		}
	return 0;
}

/**************************************************************************
* 				timeGetDevCaps		[MMSYSTEM.604]
*/
WORD timeGetDevCaps(LPTIMECAPS lpCaps, WORD wSize)
{
	printf("timeGetDevCaps(%p, %u) !\n", lpCaps, wSize);
	return 0;
}

/**************************************************************************
* 				timeBeginPeriod		[MMSYSTEM.605]
*/
WORD timeBeginPeriod(WORD wPeriod)
{
	printf("timeBeginPeriod(%u) !\n", wPeriod);
	if (!mmTimeStarted) StartMMTime();
	return 0;
}

/**************************************************************************
* 				timeEndPeriod		[MMSYSTEM.606]
*/
WORD timeEndPeriod(WORD wPeriod)
{
	printf("timeEndPeriod(%u) !\n", wPeriod);
	return 0;
}

/**************************************************************************
* 				timeGetTime			[MMSYSTEM.607]
*/
DWORD timeGetTime()
{
	printf("timeGetTime(); !\n");
	if (!mmTimeStarted) StartMMTime();
	return 0;
}


/**************************************************************************
* 				mmioOpen			[MMSYSTEM.1210]
*/
HMMIO WINAPI mmioOpen(LPSTR szFileName, MMIOINFO FAR* lpmmioinfo, DWORD dwOpenFlags)
{
	int		hFile;
	HANDLE		hmmio;
	OFSTRUCT	ofs;
	LPMMIOINFO	lpmminfo;
	printf("mmioOpen('%s', %p, %08lX);\n", szFileName, lpmmioinfo, dwOpenFlags);
	hFile = OpenFile(szFileName, &ofs, dwOpenFlags);
	if (hFile == -1) return 0;
	hmmio = GlobalAlloc(GMEM_MOVEABLE, sizeof(MMIOINFO));
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	memset(lpmminfo, 0, sizeof(MMIOINFO));
	lpmminfo->hmmio = hmmio;
	lpmminfo->dwReserved2 = MAKELONG(hFile, 0);
	GlobalUnlock(hmmio);
	printf("mmioOpen // return hmmio=%04X\n", hmmio);
	return (HMMIO)hmmio;
}


    
/**************************************************************************
* 				mmioClose			[MMSYSTEM.1211]
*/
UINT WINAPI mmioClose(HMMIO hmmio, UINT uFlags)
{
	LPMMIOINFO	lpmminfo;
	printf("mmioClose(%04X, %04X);\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	_lclose(LOWORD(lpmminfo->dwReserved2));
	GlobalUnlock(hmmio);
	GlobalFree(hmmio);
	return 0;
}



/**************************************************************************
* 				mmioRead			[MMSYSTEM.1212]
*/
LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	LONG		count;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioRead(%04X, %p, %ld);\n", hmmio, pch, cch);
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	count = read(LOWORD(lpmminfo->dwReserved2), pch, cch);
	GlobalUnlock(hmmio);
	dprintf_mmio(stddeb, "mmioRead // count=%ld\n", count);
	return count;
}



/**************************************************************************
* 				mmioWrite			[MMSYSTEM.1213]
*/
LONG WINAPI mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch)
{
	LONG		count;
	LPMMIOINFO	lpmminfo;
	printf("mmioWrite(%04X, %p, %ld);\n", hmmio, pch, cch);
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	count = _lwrite(LOWORD(lpmminfo->dwReserved2), (LPSTR)pch, cch);
	GlobalUnlock(hmmio);
	return count;
}

/**************************************************************************
* 				mmioSeek			[MMSYSTEM.1214]
*/
LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
	int		count;
	LPMMIOINFO	lpmminfo;
	printf("mmioSeek(%04X, %08lX, %d);\n", hmmio, lOffset, iOrigin);
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) {
		printf("mmioSeek // can't lock hmmio=%04X !\n", hmmio);
		return 0;
		}
	count = _llseek(LOWORD(lpmminfo->dwReserved2), lOffset, iOrigin);
	GlobalUnlock(hmmio);
	return count;
}

/**************************************************************************
* 				mmioGetInfo			[MMSYSTEM.1215]
*/
UINT WINAPI mmioGetInfo(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	LPMMIOINFO	lpmminfo;
	printf("mmioGetInfo\n");
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	memcpy(lpmmioinfo, lpmminfo, sizeof(MMIOINFO));
	GlobalUnlock(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetInfo			[MMSYSTEM.1216]
*/
UINT WINAPI mmioSetInfo(HMMIO hmmio, const MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	LPMMIOINFO	lpmminfo;
	printf("mmioSetInfo\n");
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	GlobalUnlock(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetBuffer		[MMSYSTEM.1217]
*/
UINT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, 
						LONG cchBuffer, UINT uFlags)
{
	printf("mmioSetBuffer // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioFlush			[MMSYSTEM.1218]
*/
UINT WINAPI mmioFlush(HMMIO hmmio, UINT uFlags)
{
	LPMMIOINFO	lpmminfo;
	printf("mmioFlush(%04X, %04X)\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	GlobalUnlock(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioAdvance			[MMSYSTEM.1219]
*/
UINT WINAPI mmioAdvance(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	int		count = 0;
	LPMMIOINFO	lpmminfo;
	printf("mmioAdvance\n");
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	if (uFlags == MMIO_READ) {
		count = read(LOWORD(lpmminfo->dwReserved2), 
			lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer);
		}
	if (uFlags == MMIO_WRITE) {
		count = _lwrite(LOWORD(lpmminfo->dwReserved2),
			lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer);
		}
	lpmmioinfo->pchNext	+= count;
	GlobalUnlock(hmmio);
	lpmminfo->lDiskOffset = _llseek(LOWORD(lpmminfo->dwReserved2), 0, SEEK_CUR);
	return 0;
}

/**************************************************************************
* 				mmioStringToFOURCC	[MMSYSTEM.1220]
*/
FOURCC WINAPI mmioStringToFOURCC(LPCSTR sz, UINT uFlags)
{
	printf("mmioStringToFOURCC // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioInstallIOProc	[MMSYSTEM.1221]
*/
LPMMIOPROC WINAPI mmioInstallIOProc(FOURCC fccIOProc, 
				LPMMIOPROC pIOProc, DWORD dwFlags)
{
	printf("mmioInstallIOProc // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioSendMessage		[MMSYSTEM.1222]
*/
LRESULT WINAPI mmioSendMessage(HMMIO hmmio, UINT uMessage,
					    LPARAM lParam1, LPARAM lParam2)
{
	printf("mmioSendMessage // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioDescend			[MMSYSTEM.1223]
*/
UINT WINAPI mmioDescend(HMMIO hmmio, MMCKINFO FAR* lpck,
		    const MMCKINFO FAR* lpckParent, UINT uFlags)
{
	DWORD	dwfcc, dwOldPos;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioDescend(%04X, %p, %p, %04X);\n", 
				hmmio, lpck, lpckParent, uFlags);
	if (lpck == NULL) return 0;
	lpmminfo = (LPMMIOINFO)GlobalLock(hmmio);
	if (lpmminfo == NULL) return 0;
	dwfcc = lpck->ckid;
	dprintf_mmio(stddeb, "mmioDescend // dwfcc=%08lX\n", dwfcc);
	dwOldPos = _llseek(LOWORD(lpmminfo->dwReserved2), 0, SEEK_CUR);
	dprintf_mmio(stddeb, "mmioDescend // dwOldPos=%ld\n", dwOldPos);
	if (lpckParent != NULL) {
		dprintf_mmio(stddeb, "mmioDescend // seek inside parent at %ld !\n", lpckParent->dwDataOffset);
		dwOldPos = _llseek(LOWORD(lpmminfo->dwReserved2), 
					lpckParent->dwDataOffset, SEEK_SET);
		}
	if ((uFlags & MMIO_FINDCHUNK) || (uFlags & MMIO_FINDRIFF) || 
		(uFlags & MMIO_FINDLIST)) {
		dprintf_mmio(stddeb, "mmioDescend // MMIO_FINDxxxx dwfcc=%08lX !\n", dwfcc);
		while (TRUE) {
			if (read(LOWORD(lpmminfo->dwReserved2), (LPSTR)lpck, 
					sizeof(MMCKINFO)) < sizeof(MMCKINFO)) {
				_llseek(LOWORD(lpmminfo->dwReserved2), dwOldPos, SEEK_SET);
				GlobalUnlock(hmmio);
				return MMIOERR_CHUNKNOTFOUND;
				}
			dprintf_mmio(stddeb, "mmioDescend // dwfcc=%08lX ckid=%08lX cksize=%08lX !\n", 
									dwfcc, lpck->ckid, lpck->cksize);
			if (dwfcc == lpck->ckid) break;
			dwOldPos += lpck->cksize + 2 * sizeof(DWORD);
			if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
				dwOldPos += sizeof(DWORD);
			_llseek(LOWORD(lpmminfo->dwReserved2), dwOldPos, SEEK_SET);
			}
		}
	else {
		if (read(LOWORD(lpmminfo->dwReserved2), (LPSTR)lpck, 
				sizeof(MMCKINFO)) < sizeof(MMCKINFO)) {
			_llseek(LOWORD(lpmminfo->dwReserved2), dwOldPos, SEEK_SET);
			GlobalUnlock(hmmio);
			return MMIOERR_CHUNKNOTFOUND;
			}
		}
	lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
		lpck->dwDataOffset += sizeof(DWORD);
	lpmminfo->lDiskOffset = _llseek(LOWORD(lpmminfo->dwReserved2), 
									lpck->dwDataOffset, SEEK_SET);
	GlobalUnlock(hmmio);
	dprintf_mmio(stddeb, "mmioDescend // lpck->ckid=%08lX lpck->cksize=%ld !\n", 
								lpck->ckid, lpck->cksize);
	printf("mmioDescend // lpck->fccType=%08lX !\n", lpck->fccType);
	return 0;
}

/**************************************************************************
* 				mmioAscend			[MMSYSTEM.1224]
*/
UINT WINAPI mmioAscend(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags)
{
	printf("mmioAscend // empty stub !\n");
	return 0;
}

/**************************************************************************
* 				mmioCreateChunk		[MMSYSTEM.1225]
*/
UINT WINAPI mmioCreateChunk(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags)
{
	printf("mmioCreateChunk // empty stub \n");
	return 0;
}


/**************************************************************************
* 				mmioRename			[MMSYSTEM.1226]
*/
UINT WINAPI mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
     MMIOINFO FAR* lpmmioinfo, DWORD dwRenameFlags)
{
	printf("mmioRename('%s', '%s', %p, %08lX); // empty stub \n",
			szFileName, szNewFileName, lpmmioinfo, dwRenameFlags);
	return 0;
}

/**************************************************************************
* 				DrvOpen				[MMSYSTEM.1100]
*/
HDRVR DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
	printf("DrvOpen('%s', '%s', %08lX);\n",
		lpDriverName, lpSectionName, lParam);
	return OpenDriver(lpDriverName, lpSectionName, lParam);
}


/**************************************************************************
* 				DrvClose			[MMSYSTEM.1101]
*/
LRESULT DrvClose(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
	printf("DrvClose(%04X, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
	return CloseDriver(hDrvr, lParam1, lParam2);
}


/**************************************************************************
* 				DrvSendMessage		[MMSYSTEM.1102]
*/
LRESULT WINAPI DrvSendMessage(HDRVR hDriver, WORD msg, LPARAM lParam1, LPARAM lParam2)
{
	DWORD 	dwDevID = 0;
	printf("DrvSendMessage(%04X, %04X, %08lX, %08lX);\n",
					hDriver, msg, lParam1, lParam2);
#ifndef WINELIB
	return CDAUDIO_DriverProc(dwDevID, hDriver, msg, lParam1, lParam2);
#endif
}

/**************************************************************************
* 				DrvGetModuleHandle	[MMSYSTEM.1103]
*/
HANDLE DrvGetModuleHandle(HDRVR hDrvr)
{
	printf("DrvGetModuleHandle(%04X);\n", hDrvr);
        return 0;
}


/**************************************************************************
* 				DrvDefDriverProc	[MMSYSTEM.1104]
*/
LRESULT DrvDefDriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
}


#endif /* #ifdef WINELIB */

