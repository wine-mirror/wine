/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */
/* FIXME: I think there are some segmented vs. linear pointer weirdnesses 
 *        and long term pointers to 16 bit space in here
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "ldt.h"
#include "user.h"
#include "driver.h"
#include "file.h"
#include "mmsystem.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "callback.h"

static int	InstalledCount;
static int	InstalledListLen;
static LPSTR	lpInstallNames = NULL;

MCI_OPEN_DRIVER_PARMS	mciDrv[MAXMCIDRIVERS];
/* struct below is to remember alias/devicenames for mcistring.c 
 * FIXME: should use some internal struct ... 
 */
MCI_OPEN_PARMS		mciOpenDrv[MAXMCIDRIVERS];

UINT16 midiGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
UINT16 waveGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
LONG DrvDefDriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		      DWORD dwParam1, DWORD dwParam2);

LONG WAVE_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2);
LONG MIDI_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2);
LONG CDAUDIO_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
			DWORD dwParam1, DWORD dwParam2);
LONG ANIM_DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
		     DWORD dwParam1, DWORD dwParam2);


#define GetDrv(wDevID) (&mciDrv[MMSYSTEM_DevIDToIndex(wDevID)])
#define GetOpenDrv(wDevID) (&mciOpenDrv[MMSYSTEM_DevIDToIndex(wDevID)])

/* The wDevID's returned by wine were originally in the range 
 * 0 - (MAXMCIDRIVERS - 1) and used directly as array indices.
 * Unfortunately, ms-windows uses wDevID of zero to indicate
 * errors.  Now, multimedia drivers must pass the wDevID through
 * MMSYSTEM_DevIDToIndex to get an index in that range.  An
 * aribtrary value, MMSYSTEM_MAGIC is added to the wDevID seen
 * by the windows programs.
 */

#define MMSYSTEM_MAGIC 0x0F00

/**************************************************************************
* 				MMSYSTEM_DevIDToIndex	[internal]
*/
int MMSYSTEM_DevIDToIndex(UINT16 wDevID) {
	return wDevID - MMSYSTEM_MAGIC;
}

/**************************************************************************
* 				MMSYSTEM_FirstDevId	[internal]
*/
UINT16 MMSYSTEM_FirstDevID(void)
{
	return MMSYSTEM_MAGIC;
}

/**************************************************************************
* 				MMSYSTEM_NextDevId	[internal]
*/
UINT16 MMSYSTEM_NextDevID(UINT16 wDevID) {
	return wDevID + 1;
}

/**************************************************************************
* 				MMSYSTEM_DevIdValid	[internal]
*/
BOOL32 MMSYSTEM_DevIDValid(UINT16 wDevID) {
	return wDevID >= 0x0F00 && wDevID < (0x0F00 + MAXMCIDRIVERS);
}

/**************************************************************************
* 				MMSYSTEM_WEP		[MMSYSTEM.1]
*/
int MMSYSTEM_WEP(HINSTANCE16 hInstance, WORD wDataSeg,
		 WORD cbHeapSize, LPSTR lpCmdLine)
{
	fprintf(stderr, "STUB: Unloading MMSystem DLL ... hInst=%04X \n", hInstance);
	return(TRUE);
}

/**************************************************************************
* 				sndPlaySound		[MMSYSTEM.2]
*/
BOOL16 sndPlaySound(LPCSTR lpszSoundName, UINT16 uFlags)
{
	BOOL16			bRet = FALSE;
	HMMIO16			hmmio;
	MMCKINFO                ckMainRIFF;
	char			str[128];
	LPSTR			ptr;

	dprintf_mmsys(stddeb, "sndPlaySound // SoundName='%s' uFlags=%04X !\n", 
									lpszSoundName, uFlags);
	if (lpszSoundName == NULL) {
		dprintf_mmsys(stddeb, "sndPlaySound // Stop !\n");
		return FALSE;
		}
	hmmio = mmioOpen((LPSTR)lpszSoundName, NULL, 
		MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);

	if (uFlags & SND_MEMORY) {
		dprintf_mmsys(stddeb, "sndPlaySound // SND_MEMORY flag not implemented!\n");
		return FALSE;
	}

	if (hmmio == 0) 
	{
		dprintf_mmsys(stddeb, "sndPlaySound // searching in SystemSound List !\n");
		GetProfileString32A("Sounds", (LPSTR)lpszSoundName, "", str, sizeof(str));
		if (strlen(str) == 0) return FALSE;
		if ( (ptr = (LPSTR)strchr(str, ',')) != NULL) *ptr = '\0';
		hmmio = mmioOpen(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
		if (hmmio == 0) 
		{
			dprintf_mmsys(stddeb, "sndPlaySound // can't find SystemSound='%s' !\n", str);
			return FALSE;
		}
	}

	if (mmioDescend(hmmio, &ckMainRIFF, NULL, 0) == 0) 
        {
	    dprintf_mmsys(stddeb, "sndPlaySound // ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);

	    if ((ckMainRIFF.ckid == FOURCC_RIFF) &&
	    	(ckMainRIFF.fccType == mmioFOURCC('W', 'A', 'V', 'E')))
	    {
		MMCKINFO        mmckInfo;

		mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');

		if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0)
		{
		    PCMWAVEFORMAT           pcmWaveFormat;

		    dprintf_mmsys(stddeb, "sndPlaySound // Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
				(LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

		    if (mmioRead(hmmio, (HPSTR) &pcmWaveFormat,
			        (long) sizeof(PCMWAVEFORMAT)) == (long) sizeof(PCMWAVEFORMAT))
		    {

			dprintf_mmsys(stddeb, "sndPlaySound // wFormatTag=%04X !\n", pcmWaveFormat.wf.wFormatTag);
			dprintf_mmsys(stddeb, "sndPlaySound // nChannels=%d \n", pcmWaveFormat.wf.nChannels);
			dprintf_mmsys(stddeb, "sndPlaySound // nSamplesPerSec=%ld\n", pcmWaveFormat.wf.nSamplesPerSec);
			dprintf_mmsys(stddeb, "sndPlaySound // nAvgBytesPerSec=%ld\n", pcmWaveFormat.wf.nAvgBytesPerSec);
			dprintf_mmsys(stddeb, "sndPlaySound // nBlockAlign=%d \n", pcmWaveFormat.wf.nBlockAlign);
			dprintf_mmsys(stddeb, "sndPlaySound // wBitsPerSample=%u !\n", pcmWaveFormat.wBitsPerSample);

			mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
			if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0)
			{
			    LPWAVEFORMAT     lpFormat	= (LPWAVEFORMAT) SEGPTR_ALLOC(sizeof(PCMWAVEFORMAT));
			    LPWAVEOPENDESC   lpWaveDesc = (LPWAVEOPENDESC) SEGPTR_ALLOC(sizeof(WAVEOPENDESC));
			    DWORD            dwRet;

			    dprintf_mmsys(stddeb, "sndPlaySound // Chunk Found \
 ckid=%.4s fccType=%.4s cksize=%08lX \n", (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

			    pcmWaveFormat.wf.nAvgBytesPerSec = pcmWaveFormat.wf.nSamplesPerSec * 
							   pcmWaveFormat.wf.nBlockAlign;
			    memcpy(lpFormat, &pcmWaveFormat, sizeof(PCMWAVEFORMAT));

			    lpWaveDesc->hWave    = 0;
			    lpWaveDesc->lpFormat = (LPWAVEFORMAT) SEGPTR_GET(lpFormat);

			    dwRet = wodMessage( 0, 
						WODM_OPEN, 0, (DWORD)SEGPTR_GET(lpWaveDesc), CALLBACK_NULL);
			    SEGPTR_FREE(lpFormat);
			    SEGPTR_FREE(lpWaveDesc);

			    if (dwRet == MMSYSERR_NOERROR) 
			    {
				LPWAVEHDR    lpWaveHdr = (LPWAVEHDR) SEGPTR_ALLOC(sizeof(WAVEHDR));
				SEGPTR       spWaveHdr = SEGPTR_GET(lpWaveHdr);
				HGLOBAL16    hData;
				INT32        count, bufsize;

				bufsize = 64000;
				hData = GlobalAlloc16(GMEM_MOVEABLE, bufsize);
				lpWaveHdr->lpData = (LPSTR) WIN16_GlobalLock16(hData);
				lpWaveHdr->dwBufferLength = bufsize;
				lpWaveHdr->dwUser = 0L;
				lpWaveHdr->dwFlags = 0L;
				lpWaveHdr->dwLoops = 0L;

				dwRet = wodMessage( 0,
						    WODM_PREPARE, 0, (DWORD)spWaveHdr, sizeof(WAVEHDR));
				if (dwRet == MMSYSERR_NOERROR) 
				{
				    while( TRUE )
				    {
					count = mmioRead(hmmio, PTR_SEG_TO_LIN(lpWaveHdr->lpData), bufsize);
					if (count < 1) break;
					lpWaveHdr->dwBufferLength = count;
				/*	lpWaveHdr->dwBytesRecorded = count; */
					wodMessage( 0, WODM_WRITE, 
						    0, (DWORD)spWaveHdr, sizeof(WAVEHDR));
				    }
				    wodMessage( 0, 
					        WODM_UNPREPARE, 0, (DWORD)spWaveHdr, sizeof(WAVEHDR));
				    wodMessage( 0,
						WODM_CLOSE, 0, 0L, 0L);

				    bRet = TRUE;
				}
				else dprintf_mmsys(stddeb, "sndPlaySound // can't prepare WaveOut device !\n");

				GlobalUnlock16(hData);
				GlobalFree16(hData);

				SEGPTR_FREE(lpWaveHdr);
			    }
			}
		    }
		}
	    }
	}

	if (hmmio != 0) mmioClose(hmmio, 0);
	return bRet;
}

/**************************************************************************
* 				mmsystemGetVersion	[MMSYSTEM.5]
*/
WORD mmsystemGetVersion()
{
	dprintf_mmsys(stddeb, "mmsystemGetVersion // 0.4.0 ...?... :-) !\n");
	return 0x0040;
}

/**************************************************************************
* 				DriverProc	[MMSYSTEM.6]
*/
LRESULT DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	return DrvDefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
* 				DriverCallback	[MMSYSTEM.31]
*/
BOOL16 DriverCallback(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev, 
		WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "DriverCallback(%08lX, %04X, %04X, %04X, %08lX, %08lX, %08lX); !\n",
		dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);
	switch(uFlags & DCB_TYPEMASK) {
		case DCB_NULL:
			dprintf_mmsys(stddeb, "DriverCallback() // CALLBACK_NULL !\n");
			break;
		case DCB_WINDOW:
			dprintf_mmsys(stddeb, "DriverCallback() // CALLBACK_WINDOW = %04lX handle = %04X!\n",dwCallBack,hDev);
			if (!IsWindow32(dwCallBack)) return FALSE;
			lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hDev);
			if (lpDesc == NULL) return FALSE;

			PostMessage16((HWND16)dwCallBack, wMsg, hDev, dwParam1);
			break;
		case DCB_TASK:
			dprintf_mmsys(stddeb, "DriverCallback() // CALLBACK_TASK !\n");
			return FALSE;
		case DCB_FUNCTION:
			dprintf_mmsys(stddeb, "DriverCallback() // CALLBACK_FUNCTION !\n");
			CallTo16_word_wwlll((FARPROC16)dwCallBack,hDev,wMsg,dwUser,dwParam1,dwParam2);
			break;
		}
	return TRUE;
}

/**************************************************************************
* 				auxGetNumDevs		[MMSYSTEM.350]
*/
UINT16 auxGetNumDevs()
{
	UINT16	count = 0;
	dprintf_mmsys(stddeb, "auxGetNumDevs !\n");
	count += auxMessage(0, AUXDM_GETNUMDEVS, 0L, 0L, 0L);
	dprintf_mmsys(stddeb, "auxGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				auxGetDevCaps		[MMSYSTEM.351]
*/
UINT16 auxGetDevCaps(UINT16 uDeviceID, AUXCAPS * lpCaps, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "auxGetDevCaps(%04X, %p, %d) !\n", 
					uDeviceID, lpCaps, uSize);
	return auxMessage(uDeviceID, AUXDM_GETDEVCAPS, 
				0L, (DWORD)lpCaps, (DWORD)uSize);
}

/**************************************************************************
* 				auxGetVolume		[MMSYSTEM.352]
*/
UINT16 auxGetVolume(UINT16 uDeviceID, DWORD * lpdwVolume)
{
	dprintf_mmsys(stddeb, "auxGetVolume(%04X, %p) !\n", uDeviceID, lpdwVolume);
	return auxMessage(uDeviceID, AUXDM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
* 				auxSetVolume		[MMSYSTEM.353]
*/
UINT16 auxSetVolume(UINT16 uDeviceID, DWORD dwVolume)
{
	dprintf_mmsys(stddeb, "auxSetVolume(%04X, %08lX) !\n", uDeviceID, dwVolume);
	return auxMessage(uDeviceID, AUXDM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
* 				auxOutMessage		[MMSYSTEM.354]
*/
DWORD auxOutMessage(UINT16 uDeviceID, UINT16 uMessage, DWORD dw1, DWORD dw2)
{
	dprintf_mmsys(stddeb, "auxOutMessage(%04X, %04X, %08lX, %08lX)\n", 
				uDeviceID, uMessage, dw1, dw2);
	return auxMessage(uDeviceID, uMessage, 0L, dw1, dw2);
}

/**************************************************************************
* 				mciGetErrorString		[MMSYSTEM.706]
*/
BOOL16 mciGetErrorString (DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
	LPSTR	msgptr;
	dprintf_mmsys(stddeb, "mciGetErrorString(%08lX, %p, %d);\n", wError, lpstrBuffer, uLength);
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
		case MCIERR_SEQ_DIV_INCOMPATIBLE:
			msgptr = "The time formats of the \"song pointer\" and SMPTE are mutually exclusive. You can't use them together.";
			break;
		case MCIERR_SEQ_NOMIDIPRESENT:
			msgptr = "The system has no installed MIDI devices. Use the Drivers option from the Control Panel to install a MIDI driver.";
			break;
		case MCIERR_SEQ_PORT_INUSE:
			msgptr = "The specified MIDI port is already in use. Wait until it is free; the try again.";
			break;
		case MCIERR_SEQ_PORT_MAPNODEVICE:
			msgptr = "The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use the MIDI Mapper option from the Control Panel to edit the setup.";
			break;
		case MCIERR_SEQ_PORT_MISCERROR:
			msgptr = "An error occurred with the specified port.";
			break;
		case MCIERR_SEQ_PORT_NONEXISTENT:
			msgptr = "The specified MIDI device is not installed on the system. Use the Drivers option from the Control Panel to install a MIDI device.";
			break;
		case MCIERR_SEQ_PORTUNSPECIFIED:
			msgptr = "The system doesnot have a current MIDI port specified.";
			break;
		case MCIERR_SEQ_TIMER:
			msgptr = "All multimedia timers are being used by other applications. Quit one of these applications; then, try again.";
			break;

/* 
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
        lstrcpyn32A(lpstrBuffer, msgptr, uLength);
	dprintf_mmsys(stddeb, "mciGetErrorString // msg = %s;\n", msgptr);
	return TRUE;
}


/**************************************************************************
* 				mciDriverNotify			[MMSYSTEM.711]
*/
BOOL16 mciDriverNotify(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
	dprintf_mmsys(stddeb, "mciDriverNotify(%04X, %u, %04X)\n", hWndCallBack, wDevID, wStatus);
	if (!IsWindow32(hWndCallBack)) return FALSE;
	dprintf_mmsys(stddeb, "mciDriverNotify // before PostMessage\n");
	PostMessage16( hWndCallBack, MM_MCINOTIFY, wStatus, 
                       MAKELONG(wDevID, 0));
	return TRUE;
}

/**************************************************************************
* 				mciOpen					[internal]
*/

DWORD mciOpen(DWORD dwParam, LPMCI_OPEN_PARMS lp16Parms)
{
	char	str[128];
	LPMCI_OPEN_PARMS lpParms;
	UINT16	uDevTyp = 0;
	UINT16	wDevID = MMSYSTEM_FirstDevID();
	DWORD dwret;

	lpParms = PTR_SEG_TO_LIN(lp16Parms);
	dprintf_mmsys(stddeb, "mciOpen(%08lX, %p (%p))\n", dwParam, lp16Parms, lpParms);
	if (lp16Parms == NULL) return MCIERR_INTERNAL;

	while(GetDrv(wDevID)->wType != 0) {
		wDevID = MMSYSTEM_NextDevID(wDevID);
		if (!MMSYSTEM_DevIDValid(wDevID)) {
			dprintf_mmsys(stddeb, "MCI_OPEN // MAXMCIDRIVERS reached !\n");
			return MCIERR_INTERNAL;
		}
	}
	dprintf_mmsys(stddeb, "mciOpen // wDevID=%04X \n", wDevID);
	memcpy(GetOpenDrv(wDevID),lpParms,sizeof(*lpParms));

	if (dwParam & MCI_OPEN_ELEMENT) {
		char	*s,*t;

		dprintf_mmsys(stddeb,"mciOpen // lpstrElementName='%s'\n",
			(char*)PTR_SEG_TO_LIN(lpParms->lpstrElementName)
		);
		s=(char*)PTR_SEG_TO_LIN(lpParms->lpstrElementName);
		t=strrchr(s,'.');
		if (t) {
			GetProfileString32A("mci extensions",t+1,"*",str,sizeof(str));
			CharUpper32A(str);
			dprintf_mmsys(stddeb, "mciOpen // str = %s \n", str);
			if (strcmp(str, "CDAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_CD_AUDIO;
			} else
			if (strcmp(str, "WAVEAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
			} else
			if (strcmp(str, "SEQUENCER") == 0)	{
				uDevTyp = MCI_DEVTYPE_SEQUENCER;
			} else
			if (strcmp(str, "ANIMATION1") == 0) {
				uDevTyp = MCI_DEVTYPE_ANIMATION;
			} else
			if (strcmp(str, "AVIVIDEO") == 0) {
				uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
			} else 
			if (strcmp(str,"*") == 0) {
				dprintf_mmsys(stddeb,"No [mci extensions] entry for %s found.\n",t);
				return MCIERR_EXTENSION_NOT_FOUND;
			} else  {
				dprintf_mmsys(stddeb,"[mci extensions] entry %s for %s not supported.\n",str,t);
			}
		} else
			return MCIERR_EXTENSION_NOT_FOUND;
	}

	if (dwParam & MCI_OPEN_ALIAS) {
		dprintf_mmsys(stddeb, "MCI_OPEN // Alias='%s' !\n",
			(char*)PTR_SEG_TO_LIN(lpParms->lpstrAlias));
                GetOpenDrv(wDevID)->lpstrAlias = SEGPTR_GET(
                    SEGPTR_STRDUP((char*)PTR_SEG_TO_LIN(lpParms->lpstrAlias)));
		/* mplayer does allocate alias to CDAUDIO */
	}
	if (dwParam & MCI_OPEN_TYPE) {
		if (dwParam & MCI_OPEN_TYPE_ID) {
			dprintf_mmsys(stddeb, "MCI_OPEN // Dev=%08lx !\n", lpParms->lpstrDeviceType);
			uDevTyp = LOWORD((DWORD)lpParms->lpstrDeviceType);
 			GetOpenDrv(wDevID)->lpstrDeviceType=lpParms->lpstrDeviceType;
		} else {
			if (lpParms->lpstrDeviceType == (SEGPTR)NULL) return MCIERR_INTERNAL;
			dprintf_mmsys(stddeb, "MCI_OPEN // Dev='%s' !\n",
                              (char*)PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
                        GetOpenDrv(wDevID)->lpstrDeviceType = SEGPTR_GET(
              SEGPTR_STRDUP((char*)PTR_SEG_TO_LIN(lpParms->lpstrDeviceType)));
			strcpy(str, PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
			CharUpper32A(str);
			if (strcmp(str, "CDAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_CD_AUDIO;
			} else
			if (strcmp(str, "WAVEAUDIO") == 0) {
				uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
			} else
			if (strcmp(str, "SEQUENCER") == 0)	{
				uDevTyp = MCI_DEVTYPE_SEQUENCER;
			} else
			if (strcmp(str, "ANIMATION1") == 0) {
				uDevTyp = MCI_DEVTYPE_ANIMATION;
			} else
			if (strcmp(str, "AVIVIDEO") == 0) {
				uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
			}
		}
	}
	GetDrv(wDevID)->wType = uDevTyp;
	GetDrv(wDevID)->wDeviceID = 0;  /* FIXME? for multiple devices */
	lpParms->wDeviceID = wDevID;
	dprintf_mmsys(stddeb, "MCI_OPEN // mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n", 
				wDevID, uDevTyp, lpParms->wDeviceID);
	switch(uDevTyp)
        {
        case MCI_DEVTYPE_CD_AUDIO:
	  dwret = CDAUDIO_DriverProc( 0, 0, MCI_OPEN_DRIVER,
				     dwParam, (DWORD)lp16Parms);
	  break;
        case MCI_DEVTYPE_WAVEFORM_AUDIO:
	  dwret =  WAVE_DriverProc( 0, 0, MCI_OPEN_DRIVER, 
				   dwParam, (DWORD)lp16Parms);
	  break;
        case MCI_DEVTYPE_SEQUENCER:
	  dwret = MIDI_DriverProc( 0, 0, MCI_OPEN_DRIVER, 
				  dwParam, (DWORD)lp16Parms);
	  break;
        case MCI_DEVTYPE_ANIMATION:
	  dwret = ANIM_DriverProc( 0, 0, MCI_OPEN_DRIVER, 
				  dwParam, (DWORD)lp16Parms);
	  break;
        case MCI_DEVTYPE_DIGITAL_VIDEO:
	  dprintf_mmsys(stddeb, "MCI_OPEN // No DIGITAL_VIDEO yet !\n");
	  return MCIERR_DEVICE_NOT_INSTALLED;
        default:
	  dprintf_mmsys(stddeb, "MCI_OPEN // Invalid Device Name '%08lx' !\n", lpParms->lpstrDeviceType);
	  return MCIERR_INVALID_DEVICE_NAME;
        }


	if (dwParam&MCI_NOTIFY)
	  mciDriverNotify(lpParms->dwCallback,wDevID,
			  (dwret==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));

	/* only handled devices fall through */
	dprintf_mmsys(stddeb, "MCI_OPEN // wDevID = %04X wDeviceID = %d dwret = %ld\n",wDevID, lpParms->wDeviceID, dwret);
	return dwret;
}


/**************************************************************************
* 				mciClose				[internal]
*/
DWORD mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
	DWORD	dwRet = MCIERR_INTERNAL;

	dprintf_mmsys(stddeb, "mciClose(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);
	switch(GetDrv(wDevID)->wType) {
		case MCI_DEVTYPE_CD_AUDIO:
			dwRet = CDAUDIO_DriverProc(GetDrv(wDevID)->wDeviceID,0,
                                           MCI_CLOSE, dwParam, (DWORD)lpParms);
			break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO:
			dwRet = WAVE_DriverProc(GetDrv(wDevID)->wDeviceID, 0, 
						MCI_CLOSE, dwParam,
                                                (DWORD)lpParms);
			break;
		case MCI_DEVTYPE_SEQUENCER:
			dwRet = MIDI_DriverProc(GetDrv(wDevID)->wDeviceID, 0, 
						MCI_CLOSE, dwParam,
                                                (DWORD)lpParms);
			break;
		case MCI_DEVTYPE_ANIMATION:
			dwRet = ANIM_DriverProc(GetDrv(wDevID)->wDeviceID, 0, 
						MCI_CLOSE, dwParam,
                                                (DWORD)lpParms);
			break;
		default:
			dprintf_mmsys(stddeb, "mciClose() // unknown device type=%04X !\n", GetDrv(wDevID)->wType);
			dwRet = MCIERR_DEVICE_NOT_INSTALLED;
		}
	GetDrv(wDevID)->wType = 0;

	if (dwParam&MCI_NOTIFY)
	  mciDriverNotify(lpParms->dwCallback,wDevID,
			  (dwRet==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));

	dprintf_mmsys(stddeb, "mciClose() // returns %ld\n",dwRet);
	return dwRet;
}


/**************************************************************************
* 				mciSysinfo				[internal]
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
				ptr = lpInstallNames = xmalloc(2048);
				GetPrivateProfileString32A("mci", NULL, "", lpInstallNames, 2000, SysFile);
				while(strlen(ptr) > 0) {
					dprintf_mci(stddeb, "---> '%s' \n", ptr);
					len = strlen(ptr) + 1;
					ptr += len;
					InstalledListLen += len;
					InstalledCount++;
				}
			}
			if (lpParms->dwRetSize < InstalledListLen)
				lstrcpyn32A(lpstrReturn, lpInstallNames, lpParms->dwRetSize - 1);
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
*  not used anymore ??

DWORD mciSound(UINT16 wDevID, DWORD dwParam, LPMCI_SOUND_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	if (dwParam & MCI_SOUND_NAME)
		dprintf_mci(stddeb, "MCI_SOUND // file='%s' !\n", lpParms->lpstrSoundName);
	return MCIERR_INVALID_DEVICE_ID;
}
*
*/

static const char *_mciCommandToString(UINT16 wMsg)
{
	static char buffer[100];

#define CASE(s) case (s): return #s

	switch (wMsg) {
		CASE(MCI_OPEN);
		CASE(MCI_CLOSE);
		CASE(MCI_ESCAPE);
		CASE(MCI_PLAY);
		CASE(MCI_SEEK);
		CASE(MCI_STOP);
		CASE(MCI_PAUSE);
		CASE(MCI_INFO);
		CASE(MCI_GETDEVCAPS);
		CASE(MCI_SPIN);
		CASE(MCI_SET);
		CASE(MCI_STEP);
		CASE(MCI_RECORD);
		CASE(MCI_SYSINFO);
		CASE(MCI_BREAK);
		CASE(MCI_SAVE);
		CASE(MCI_STATUS);
		CASE(MCI_CUE);
		CASE(MCI_REALIZE);
		CASE(MCI_WINDOW);
		CASE(MCI_PUT);
		CASE(MCI_WHERE);
		CASE(MCI_FREEZE);
		CASE(MCI_UNFREEZE);
		CASE(MCI_LOAD);
		CASE(MCI_CUT);
		CASE(MCI_COPY);
		CASE(MCI_PASTE);
		CASE(MCI_UPDATE);
		CASE(MCI_RESUME);
		CASE(MCI_DELETE);
		default:
			sprintf(buffer, "%04X", wMsg);
			return buffer;

	}
}

/**************************************************************************
* 				mciSendCommand			[MMSYSTEM.701]
*/
DWORD mciSendCommand(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    HDRVR16 hDrv = 0;
    dprintf_mci(stddeb, "mciSendCommand(%04X, %s, %08lX, %08lX)\n", 
                wDevID, _mciCommandToString(wMsg), dwParam1, dwParam2);
    switch(wMsg)
    {
    case MCI_OPEN:
        return mciOpen(dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
    case MCI_CLOSE:
        return mciClose( wDevID, dwParam1,
                         (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SYSINFO:
        return mciSysInfo( dwParam1,
                           (LPMCI_SYSINFO_PARMS)PTR_SEG_TO_LIN(dwParam2));
    default:
        switch(GetDrv(wDevID)->wType)
        {
        case MCI_DEVTYPE_CD_AUDIO:
            return CDAUDIO_DriverProc(GetDrv(wDevID)->wDeviceID, hDrv, 
                                      wMsg, dwParam1, dwParam2);
        case MCI_DEVTYPE_WAVEFORM_AUDIO:
            return WAVE_DriverProc(GetDrv(wDevID)->wDeviceID, hDrv, 
                                   wMsg, dwParam1, dwParam2);
        case MCI_DEVTYPE_SEQUENCER:
            return MIDI_DriverProc(GetDrv(wDevID)->wDeviceID, hDrv, 
                                   wMsg, dwParam1, dwParam2);
        case MCI_DEVTYPE_ANIMATION:
            return ANIM_DriverProc(GetDrv(wDevID)->wDeviceID, hDrv, 
                                   wMsg, dwParam1, dwParam2);
        default:
            dprintf_mci(stddeb,
                        "mciSendCommand() // unknown device type=%04X !\n", 
                        GetDrv(wDevID)->wType);
        }
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
* 				mciGetDeviceID	       	[MMSYSTEM.703]
*/
UINT16 mciGetDeviceID (LPCSTR lpstrName)
{
    UINT16 wDevID;

    dprintf_mci(stddeb, "mciGetDeviceID(\"%s\")\n", lpstrName);
    if (lpstrName && !lstrcmpi32A(lpstrName, "ALL"))
        return MCI_ALL_DEVICE_ID;

    if (!lpstrName)
	return 0;

    wDevID = MMSYSTEM_FirstDevID();
    while(MMSYSTEM_DevIDValid(wDevID) && GetDrv(wDevID)->wType) {
	if (GetOpenDrv(wDevID)->lpstrDeviceType && 
            strcmp(PTR_SEG_TO_LIN(GetOpenDrv(wDevID)->lpstrDeviceType), lpstrName) == 0)
	    return wDevID;

	if (GetOpenDrv(wDevID)->lpstrAlias && 
            strcmp(PTR_SEG_TO_LIN(GetOpenDrv(wDevID)->lpstrAlias), lpstrName) == 0)
	    return wDevID;

	wDevID = MMSYSTEM_NextDevID(wDevID);
    }

    return 0;
}

/**************************************************************************
* 				mciSetYieldProc		[MMSYSTEM.714]
*/
BOOL16 mciSetYieldProc (UINT16 uDeviceID, 
		YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    return FALSE;
}

/**************************************************************************
* 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
*/
UINT16 mciGetDeviceIDFromElementID(DWORD dwElementID, LPCSTR lpstrType)
{
    return 0;
}

/**************************************************************************
* 				mciGetYieldProc		[MMSYSTEM.716]
*/
YIELDPROC mciGetYieldProc(UINT16 uDeviceID, DWORD * lpdwYieldData)
{
    return NULL;
}

/**************************************************************************
* 				mciGetCreatorTask	[MMSYSTEM.717]
*/
HTASK16 mciGetCreatorTask(UINT16 uDeviceID)
{
    return 0;
}

/**************************************************************************
* 				midiOutGetNumDevs	[MMSYSTEM.201]
*/
UINT16 midiOutGetNumDevs(void)
{
	UINT16	count = 0;
	dprintf_mmsys(stddeb, "midiOutGetNumDevs\n");
	count += modMessage(0, MODM_GETNUMDEVS, 0L, 0L, 0L);
	dprintf_mmsys(stddeb, "midiOutGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				midiOutGetDevCaps	[MMSYSTEM.202]
*/
UINT16 midiOutGetDevCaps(UINT16 uDeviceID, MIDIOUTCAPS * lpCaps, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "midiOutGetDevCaps\n");
	return modMessage(uDeviceID,MODM_GETDEVCAPS,0,(DWORD)lpCaps,uSize);
}

/**************************************************************************
* 				midiOutGetErrorText 	[MMSYSTEM.203]
*/
UINT16 midiOutGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "midiOutGetErrorText\n");
	return midiGetErrorText(uError, lpText, uSize);
}


/**************************************************************************
* 				midiGetErrorText       	[internal]
*/
UINT16 midiGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
	LPSTR	msgptr;
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
	lstrcpyn32A(lpText, msgptr, uSize);
	return TRUE;
}

/**************************************************************************
* 				midiOutOpen    		[MMSYSTEM.204]
*/
UINT16 midiOutOpen(HMIDIOUT16 * lphMidiOut, UINT16 uDeviceID,
		 DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HMIDI16	hMidiOut;
	LPMIDIOPENDESC	lpDesc;
	LPMIDIOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL32	bMapperFlg = FALSE;
	if (lphMidiOut != NULL) *lphMidiOut = 0;
	dprintf_mmsys(stddeb, "midiOutOpen(%p, %d, %08lX, %08lX, %08lX);\n", 
		lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);
	if (uDeviceID == (UINT16)MIDI_MAPPER) {
		dprintf_mmsys(stddeb, "midiOutOpen	// MIDI_MAPPER mode requested !\n");
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
		dprintf_mmsys(stddeb, "midiOutOpen	// MIDI_MAPPER mode ! try next driver...\n");
		}
	return dwRet;
}

/**************************************************************************
* 				midiOutClose		[MMSYSTEM.205]
*/
UINT16 midiOutClose(HMIDIOUT16 hMidiOut)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutClose(%04X)\n", hMidiOut);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiOutPrepareHeader	[MMSYSTEM.206]
*/
UINT16 midiOutPrepareHeader(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutPrepareHeader(%04X, %p, %d)\n", 
					hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_PREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutUnprepareHeader	[MMSYSTEM.207]
*/
UINT16 midiOutUnprepareHeader(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutUnprepareHeader(%04X, %p, %d)\n", 
					hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_UNPREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutShortMsg		[MMSYSTEM.208]
*/
UINT16 midiOutShortMsg(HMIDIOUT16 hMidiOut, DWORD dwMsg)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutShortMsg(%04X, %08lX)\n", hMidiOut, dwMsg);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_DATA, lpDesc->dwInstance, dwMsg, 0L);
}

/**************************************************************************
* 				midiOutLongMsg		[MMSYSTEM.209]
*/
UINT16 midiOutLongMsg(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutLongMsg(%04X, %p, %d)\n", 
				hMidiOut, lpMidiOutHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_LONGDATA, lpDesc->dwInstance, 
						(DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiOutReset		[MMSYSTEM.210]
*/
UINT16 midiOutReset(HMIDIOUT16 hMidiOut)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutReset(%04X)\n", hMidiOut);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, MODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiOutGetVolume	[MMSYSTEM.211]
*/
UINT16 midiOutGetVolume(UINT16 uDeviceID, DWORD * lpdwVolume)
{
	dprintf_mmsys(stddeb, "midiOutGetVolume(%04X, %p);\n", uDeviceID, lpdwVolume);
	return modMessage(uDeviceID, MODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
	return 0;
}

/**************************************************************************
* 				midiOutSetVolume	[MMSYSTEM.212]
*/
UINT16 midiOutSetVolume(UINT16 uDeviceID, DWORD dwVolume)
{
	dprintf_mmsys(stddeb, "midiOutSetVolume(%04X, %08lX);\n", uDeviceID, dwVolume);
	return modMessage(uDeviceID, MODM_SETVOLUME, 0L, dwVolume, 0L);
	return 0;
}

/**************************************************************************
* 				midiOutCachePatches		[MMSYSTEM.213]
*/
UINT16 midiOutCachePatches(HMIDIOUT16 hMidiOut,
    UINT16 uBank, WORD * lpwPatchArray, UINT16 uFlags)
{
        /* not really necessary to support this */
	fprintf(stdnimp, "midiOutCachePatches: not supported yet\n");
	return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
* 				midiOutCacheDrumPatches	[MMSYSTEM.214]
*/
UINT16 midiOutCacheDrumPatches(HMIDIOUT16 hMidiOut,
    UINT16 uPatch, WORD * lpwKeyArray, UINT16 uFlags)
{
	fprintf(stdnimp, "midiOutCacheDrumPatchesi: not supported yet\n");
	return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
* 				midiOutGetID		[MMSYSTEM.215]
*/
UINT16 midiOutGetID(HMIDIOUT16 hMidiOut, UINT16 * lpuDeviceID)
{
	dprintf_mmsys(stddeb, "midiOutGetID\n");
	return 0;
}

/**************************************************************************
* 				midiOutMessage		[MMSYSTEM.216]
*/
DWORD midiOutMessage(HMIDIOUT16 hMidiOut, UINT16 uMessage, 
						DWORD dwParam1, DWORD dwParam2)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiOutMessage(%04X, %04X, %08lX, %08lX)\n", 
			hMidiOut, uMessage, dwParam1, dwParam2);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return modMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
	return 0;
}

/**************************************************************************
* 				midiInGetNumDevs	[MMSYSTEM.301]
*/
UINT16 midiInGetNumDevs(void)
{
	UINT16	count = 0;
	dprintf_mmsys(stddeb, "midiInGetNumDevs\n");
	count += midMessage(0, MIDM_GETNUMDEVS, 0L, 0L, 0L);
	dprintf_mmsys(stddeb, "midiInGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				midiInGetDevCaps	[MMSYSTEM.302]
*/
UINT16 midiInGetDevCaps(UINT16 uDeviceID,
    LPMIDIINCAPS lpCaps, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "midiInGetDevCaps\n");
	return midMessage(uDeviceID,MIDM_GETDEVCAPS,0,(DWORD)lpCaps,uSize);;
}

/**************************************************************************
* 				midiInGetErrorText 		[MMSYSTEM.303]
*/
UINT16 midiInGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "midiInGetErrorText\n");
	return (midiGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
* 				midiInOpen		[MMSYSTEM.304]
*/
UINT16 midiInOpen(HMIDIIN16 * lphMidiIn, UINT16 uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HMIDI16	hMidiIn;
	LPMIDIOPENDESC	lpDesc;
	LPMIDIOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL32	bMapperFlg = FALSE;
	if (lphMidiIn != NULL) *lphMidiIn = 0;
	dprintf_mmsys(stddeb, "midiInOpen(%p, %d, %08lX, %08lX, %08lX);\n", 
		lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);
	if (uDeviceID == (UINT16)MIDI_MAPPER) {
		dprintf_mmsys(stddeb, "midiInOpen	// MIDI_MAPPER mode requested !\n");
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
		dprintf_mmsys(stddeb, "midiInOpen	// MIDI_MAPPER mode ! try next driver...\n");
		}
	return dwRet;
}

/**************************************************************************
* 				midiInClose		[MMSYSTEM.305]
*/
UINT16 midiInClose(HMIDIIN16 hMidiIn)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiInClose(%04X)\n", hMidiIn);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				midiInPrepareHeader	[MMSYSTEM.306]
*/
UINT16 midiInPrepareHeader(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT16 uSize)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiInPrepareHeader(%04X, %p, %d)\n", 
					hMidiIn, lpMidiInHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_PREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiInUnprepareHeader	[MMSYSTEM.307]
*/
UINT16 midiInUnprepareHeader(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT16 uSize)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiInUnprepareHeader(%04X, %p, %d)\n", 
					hMidiIn, lpMidiInHdr, uSize);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, MIDM_UNPREPARE, lpDesc->dwInstance, 
						(DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
* 				midiInAddBuffer		[MMSYSTEM.308]
*/
UINT16 midiInAddBuffer(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "midiInAddBuffer\n");
	return 0;
}

/**************************************************************************
* 				midiInStart			[MMSYSTEM.309]
*/
UINT16 midiInStart(HMIDIIN16 hMidiIn)
{
	dprintf_mmsys(stddeb, "midiInStart\n");
	return 0;
}

/**************************************************************************
* 				midiInStop			[MMSYSTEM.310]
*/
UINT16 midiInStop(HMIDIIN16 hMidiIn)
{
	dprintf_mmsys(stddeb, "midiInStop\n");
	return 0;
}

/**************************************************************************
* 				midiInReset			[MMSYSTEM.311]
*/
UINT16 midiInReset(HMIDIIN16 hMidiIn)
{
	dprintf_mmsys(stddeb, "midiInReset\n");
	return 0;
}

/**************************************************************************
* 				midiInGetID			[MMSYSTEM.312]
*/
UINT16 midiInGetID(HMIDIIN16 hMidiIn, UINT16 * lpuDeviceID)
{
	dprintf_mmsys(stddeb, "midiInGetID\n");
	return 0;
}

/**************************************************************************
* 				midiInMessage		[MMSYSTEM.313]
*/
DWORD midiInMessage(HMIDIIN16 hMidiIn, UINT16 uMessage, 
							DWORD dwParam1, DWORD dwParam2)
{
	LPMIDIOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "midiInMessage(%04X, %04X, %08lX, %08lX)\n", 
			hMidiIn, uMessage, dwParam1, dwParam2);
	lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return midMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}


/**************************************************************************
* 				waveOutGetNumDevs		[MMSYSTEM.401]
*/
UINT16 waveOutGetNumDevs()
{
	UINT16	count = 0;
	dprintf_mmsys(stddeb, "waveOutGetNumDevs\n");
	count += wodMessage( MMSYSTEM_FirstDevID(), WODM_GETNUMDEVS, 0L, 0L, 0L);
	dprintf_mmsys(stddeb, "waveOutGetNumDevs return %u \n", count);
	return count;
}

/**************************************************************************
* 				waveOutGetDevCaps		[MMSYSTEM.402]
*/
UINT16 waveOutGetDevCaps(UINT16 uDeviceID, WAVEOUTCAPS * lpCaps, UINT16 uSize)
{
	if (uDeviceID > waveOutGetNumDevs() - 1) return MMSYSERR_BADDEVICEID;
	if (uDeviceID == (UINT16)WAVE_MAPPER) return MMSYSERR_BADDEVICEID; /* FIXME: do we have a wave mapper ? */
	dprintf_mmsys(stddeb, "waveOutGetDevCaps\n");
	return wodMessage(uDeviceID, WODM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}

/**************************************************************************
* 				waveOutGetErrorText 	[MMSYSTEM.403]
*/
UINT16 waveOutGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
   dprintf_mmsys(stddeb, "waveOutGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


/**************************************************************************
* 				waveGetErrorText 		[internal]
*/
UINT16 waveGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
	LPSTR	msgptr;
	dprintf_mmsys(stddeb, "waveGetErrorText(%04X, %p, %d);\n", uError, lpText, uSize);
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
	lstrcpyn32A(lpText, msgptr, uSize);
	return TRUE;
}

/**************************************************************************
* 				waveOutOpen			[MMSYSTEM.404]
*/
UINT16 waveOutOpen(HWAVEOUT16 * lphWaveOut, UINT16 uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HWAVEOUT16 hWaveOut;
	LPWAVEOPENDESC	lpDesc;
	LPWAVEOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL32	bMapperFlg = FALSE;

	dprintf_mmsys(stddeb, "waveOutOpen(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
		lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		dprintf_mmsys(stddeb, "waveOutOpen	// WAVE_FORMAT_QUERY requested !\n");
		}
	if (uDeviceID == (UINT16)WAVE_MAPPER) {
		dprintf_mmsys(stddeb, "waveOutOpen	// WAVE_MAPPER mode requested !\n");
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
	lpDesc->lpFormat = lpFormat;  /* should the struct be copied iso pointer? */
	lpDesc->dwCallBack = dwCallback;
	lpDesc->dwInstance = dwInstance;
	if (uDeviceID >= MAXWAVEDRIVERS) uDeviceID = 0;
	while(uDeviceID < MAXWAVEDRIVERS) {
		dwRet = wodMessage(uDeviceID, WODM_OPEN, 
			lpDesc->dwInstance, (DWORD)lp16Desc, dwFlags);
		if (dwRet == MMSYSERR_NOERROR) break;
		if (!bMapperFlg) break;
		uDeviceID++;
		dprintf_mmsys(stddeb, "waveOutOpen	// WAVE_MAPPER mode ! try next driver...\n");
		}
	lpDesc->uDeviceID = uDeviceID;  /* save physical Device ID */
	if (dwFlags & WAVE_FORMAT_QUERY) {
		dprintf_mmsys(stddeb, "waveOutOpen	// End of WAVE_FORMAT_QUERY !\n");
		dwRet = waveOutClose(hWaveOut);
		}
	return dwRet;
}

/**************************************************************************
* 				waveOutClose		[MMSYSTEM.405]
*/
UINT16 waveOutClose(HWAVEOUT16 hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutClose(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutPrepareHeader	[MMSYSTEM.406]
*/
UINT16 waveOutPrepareHeader(HWAVEOUT16 hWaveOut,
     WAVEHDR * lpWaveOutHdr, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutPrepareHeader(%04X, %p, %u);\n", 
					hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_PREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutUnprepareHeader	[MMSYSTEM.407]
*/
UINT16 waveOutUnprepareHeader(HWAVEOUT16 hWaveOut,
    WAVEHDR * lpWaveOutHdr, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutUnprepareHeader(%04X, %p, %u);\n", 
						hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_UNPREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutWrite		[MMSYSTEM.408]
*/
UINT16 waveOutWrite(HWAVEOUT16 hWaveOut, WAVEHDR * lpWaveOutHdr,  UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutWrite(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_WRITE, lpDesc->dwInstance, 
							(DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
* 				waveOutPause		[MMSYSTEM.409]
*/
UINT16 waveOutPause(HWAVEOUT16 hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutPause(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_PAUSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutRestart		[MMSYSTEM.410]
*/
UINT16 waveOutRestart(HWAVEOUT16 hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveOutRestart(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_RESTART, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutReset		[MMSYSTEM.411]
*/
UINT16 waveOutReset(HWAVEOUT16 hWaveOut)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutReset(%04X)\n", hWaveOut);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
* 				waveOutGetPosition	[MMSYSTEM.412]
*/
UINT16 waveOutGetPosition(HWAVEOUT16 hWaveOut, MMTIME * lpTime, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutGetPosition(%04X, %p, %u);\n", hWaveOut, lpTime, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_GETPOS, lpDesc->dwInstance, 
							(DWORD)lpTime, (DWORD)uSize);
}

/**************************************************************************
* 				waveOutGetPitch		[MMSYSTEM.413]
*/
UINT16 waveOutGetPitch(HWAVEOUT16 hWaveOut, DWORD * lpdwPitch)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutGetPitch(%04X, %p);\n", hWaveOut, lpdwPitch);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_GETPITCH, lpDesc->dwInstance, 
								(DWORD)lpdwPitch, 0L);
}

/**************************************************************************
* 				waveOutSetPitch		[MMSYSTEM.414]
*/
UINT16 waveOutSetPitch(HWAVEOUT16 hWaveOut, DWORD dwPitch)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutSetPitch(%04X, %08lX);\n", hWaveOut, dwPitch);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_SETPITCH, lpDesc->dwInstance, (DWORD)dwPitch, 0L);
}

/**************************************************************************
* 				waveOutGetVolume	[MMSYSTEM.415]
*/
UINT16 waveOutGetVolume(UINT16 uDeviceID, DWORD * lpdwVolume)
{
	dprintf_mmsys(stddeb, "waveOutGetVolume(%04X, %p);\n", uDeviceID, lpdwVolume);
	return wodMessage(uDeviceID, WODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
* 				waveOutSetVolume	[MMSYSTEM.416]
*/
UINT16 waveOutSetVolume(UINT16 uDeviceID, DWORD dwVolume)
{
	dprintf_mmsys(stddeb, "waveOutSetVolume(%04X, %08lX);\n", uDeviceID, dwVolume);
	return wodMessage(uDeviceID, WODM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
* 				waveOutGetPlaybackRate	[MMSYSTEM.417]
*/
UINT16 waveOutGetPlaybackRate(HWAVEOUT16 hWaveOut, DWORD * lpdwRate)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutGetPlaybackRate(%04X, %p);\n", hWaveOut, lpdwRate);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_GETPLAYBACKRATE, lpDesc->dwInstance, 
								(DWORD)lpdwRate, 0L);
}

/**************************************************************************
* 				waveOutSetPlaybackRate	[MMSYSTEM.418]
*/
UINT16 waveOutSetPlaybackRate(HWAVEOUT16 hWaveOut, DWORD dwRate)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutSetPlaybackRate(%04X, %08lX);\n", hWaveOut, dwRate);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, WODM_SETPLAYBACKRATE, 
		lpDesc->dwInstance, (DWORD)dwRate, 0L);
}

/**************************************************************************
* 				waveOutBreakLoop 	[MMSYSTEM.419]
*/
UINT16 waveOutBreakLoop(HWAVEOUT16 hWaveOut)
{
	dprintf_mmsys(stddeb, "waveOutBreakLoop(%04X)\n", hWaveOut);
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetID	 	[MMSYSTEM.420]
*/
UINT16 waveOutGetID(HWAVEOUT16 hWaveOut, UINT16 * lpuDeviceID)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutGetID(%04X, %p);\n", hWaveOut, lpuDeviceID);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
	*lpuDeviceID = lpDesc->uDeviceID;
        return 0;
}

/**************************************************************************
* 				waveOutMessage 		[MMSYSTEM.421]
*/
DWORD waveOutMessage(HWAVEOUT16 hWaveOut, UINT16 uMessage, 
							DWORD dwParam1, DWORD dwParam2)
{
	LPWAVEOPENDESC	lpDesc;
	dprintf_mmsys(stddeb, "waveOutMessage(%04X, %04X, %08lX, %08lX)\n", 
			hWaveOut, uMessage, dwParam1, dwParam2);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return wodMessage( lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
* 				waveInGetNumDevs 		[MMSYSTEM.501]
*/
UINT16 waveInGetNumDevs()
{
	UINT16	count = 0;
	dprintf_mmsys(stddeb, "waveInGetNumDevs\n");
	count += widMessage(0, WIDM_GETNUMDEVS, 0L, 0L, 0L);
	dprintf_mmsys(stddeb, "waveInGetNumDevs return %u \n", count);
	return count;
}


/**************************************************************************
* 				waveInGetDevCaps 		[MMSYSTEM.502]
*/
UINT16 waveInGetDevCaps(UINT16 uDeviceID, WAVEINCAPS * lpCaps, UINT16 uSize)
{
	dprintf_mmsys(stddeb, "waveInGetDevCaps\n");
	return widMessage(uDeviceID, WIDM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}


/**************************************************************************
* 				waveInGetErrorText 	[MMSYSTEM.503]
*/
UINT16 waveInGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
   dprintf_mmsys(stddeb, "waveInGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


/**************************************************************************
* 				waveInOpen			[MMSYSTEM.504]
*/
UINT16 waveInOpen(HWAVEIN16 * lphWaveIn, UINT16 uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	HWAVEIN16 hWaveIn;
	LPWAVEOPENDESC	lpDesc;
	LPWAVEOPENDESC	lp16Desc;
	DWORD	dwRet = 0;
	BOOL32	bMapperFlg = FALSE;
	dprintf_mmsys(stddeb, "waveInOpen(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
		lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		dprintf_mmsys(stddeb, "waveInOpen // WAVE_FORMAT_QUERY requested !\n");
		}
	if (uDeviceID == (UINT16)WAVE_MAPPER) {
		dprintf_mmsys(stddeb, "waveInOpen	// WAVE_MAPPER mode requested !\n");
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
		dprintf_mmsys(stddeb, "waveInOpen	// WAVE_MAPPER mode ! try next driver...\n");
		}
	lpDesc->uDeviceID = uDeviceID;
	if (dwFlags & WAVE_FORMAT_QUERY) {
		dprintf_mmsys(stddeb, "waveInOpen	// End of WAVE_FORMAT_QUERY !\n");
		dwRet = waveInClose(hWaveIn);
		}
	return dwRet;
}


/**************************************************************************
* 				waveInClose			[MMSYSTEM.505]
*/
UINT16 waveInClose(HWAVEIN16 hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInClose(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, WIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInPrepareHeader		[MMSYSTEM.506]
*/
UINT16 waveInPrepareHeader(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;

	dprintf_mmsys(stddeb, "waveInPrepareHeader(%04X, %p, %u);\n", 
					hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	lp32WaveInHdr->lp16Next = (SEGPTR)NULL;
    lp32WaveInHdr->dwBytesRecorded = 0;
	dprintf_mmsys(stddeb, "waveInPrepareHeader // lpData=%p size=%lu \n", 
		lp32WaveInHdr->lpData, lp32WaveInHdr->dwBufferLength);
	return widMessage(lpDesc->uDeviceID, WIDM_PREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInUnprepareHeader	[MMSYSTEM.507]
*/
UINT16 waveInUnprepareHeader(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;

	dprintf_mmsys(stddeb, "waveInUnprepareHeader(%04X, %p, %u);\n", 
						hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	USER_HEAP_FREE(HIWORD((DWORD)lp32WaveInHdr->lpData));
	lp32WaveInHdr->lpData = NULL;
	lp32WaveInHdr->lp16Next = (SEGPTR)NULL;
	return widMessage(lpDesc->uDeviceID, WIDM_UNPREPARE, lpDesc->dwInstance, 
							(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInAddBuffer		[MMSYSTEM.508]
*/
UINT16 waveInAddBuffer(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;
	LPWAVEHDR 		lp32WaveInHdr;

	dprintf_mmsys(stddeb, "waveInAddBuffer(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
	lp32WaveInHdr = PTR_SEG_TO_LIN(lpWaveInHdr);
	lp32WaveInHdr->lp16Next = (SEGPTR)NULL;
    lp32WaveInHdr->dwBytesRecorded = 0;
	dprintf_mmsys(stddeb, "waveInAddBuffer // lpData=%p size=%lu \n", 
		lp32WaveInHdr->lpData, lp32WaveInHdr->dwBufferLength);
	return widMessage(lpDesc->uDeviceID, WIDM_ADDBUFFER, lpDesc->dwInstance,
								(DWORD)lpWaveInHdr, uSize);
}


/**************************************************************************
* 				waveInStart			[MMSYSTEM.509]
*/
UINT16 waveInStart(HWAVEIN16 hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInStart(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, WIDM_START, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInStop			[MMSYSTEM.510]
*/
UINT16 waveInStop(HWAVEIN16 hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInStop(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, WIDM_STOP, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInReset			[MMSYSTEM.511]
*/
UINT16 waveInReset(HWAVEIN16 hWaveIn)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInReset(%04X)\n", hWaveIn);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, WIDM_RESET, lpDesc->dwInstance, 0L, 0L);
}


/**************************************************************************
* 				waveInGetPosition	[MMSYSTEM.512]
*/
UINT16 waveInGetPosition(HWAVEIN16 hWaveIn, MMTIME * lpTime, UINT16 uSize)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInGetPosition(%04X, %p, %u);\n", hWaveIn, lpTime, uSize);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, WIDM_GETPOS, lpDesc->dwInstance,
			  (DWORD)lpTime, (DWORD)uSize);
}


/**************************************************************************
* 				waveInGetID			[MMSYSTEM.513]
*/
UINT16 waveInGetID(HWAVEIN16 hWaveIn, UINT16 * lpuDeviceID)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInGetID\n");
	if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	*lpuDeviceID = lpDesc->uDeviceID;
	return 0;
}


/**************************************************************************
* 				waveInMessage 		[MMSYSTEM.514]
*/
DWORD waveInMessage(HWAVEIN16 hWaveIn, UINT16 uMessage,
		    DWORD dwParam1, DWORD dwParam2)
{
	LPWAVEOPENDESC	lpDesc;

	dprintf_mmsys(stddeb, "waveInMessage(%04X, %04X, %08lX, %08lX)\n", 
			hWaveIn, uMessage, dwParam1, dwParam2);
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
	return widMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}


/**************************************************************************
* 				mmioOpen       		[MMSYSTEM.1210]
*/
HMMIO16 mmioOpen(LPSTR szFileName, MMIOINFO * lpmmioinfo, DWORD dwOpenFlags)
{
        HFILE32 hFile;
	HMMIO16		hmmio;
	OFSTRUCT	ofs;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioOpen('%s', %p, %08lX);\n", szFileName, lpmmioinfo, dwOpenFlags);
        if (!szFileName)
        {
            /* FIXME: should load memory file if szFileName == NULL */
            fprintf(stderr, "WARNING: mmioOpen(): szFileName == NULL (memory file ???)\n");
            return 0;
 	}
	hFile = OpenFile32(szFileName, &ofs, dwOpenFlags);
	if (hFile == -1) return 0;
	hmmio = GlobalAlloc16(GMEM_MOVEABLE, sizeof(MMIOINFO));
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	memset(lpmminfo, 0, sizeof(MMIOINFO));
	lpmminfo->hmmio = hmmio;
	lpmminfo->dwReserved2 = hFile;
	GlobalUnlock16(hmmio);
	dprintf_mmio(stddeb, "mmioOpen // return hmmio=%04X\n", hmmio);
	return hmmio;
}

    
/**************************************************************************
* 				mmioClose      		[MMSYSTEM.1211]
*/
UINT16 mmioClose(HMMIO16 hmmio, UINT16 uFlags)
{
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioClose(%04X, %04X);\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	_lclose32((HFILE32)lpmminfo->dwReserved2);
	GlobalUnlock16(hmmio);
	GlobalFree16(hmmio);
	return 0;
}



/**************************************************************************
* 				mmioRead	       	[MMSYSTEM.1212]
*/
LONG mmioRead(HMMIO16 hmmio, HPSTR pch, LONG cch)
{
	LONG		count;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioRead(%04X, %p, %ld);\n", hmmio, pch, cch);
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	count = _lread32(LOWORD(lpmminfo->dwReserved2), pch, cch);
	GlobalUnlock16(hmmio);
	dprintf_mmio(stddeb, "mmioRead // count=%ld\n", count);
	return count;
}



/**************************************************************************
* 				mmioWrite      		[MMSYSTEM.1213]
*/
LONG mmioWrite(HMMIO16 hmmio, HPCSTR pch, LONG cch)
{
	LONG		count;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioWrite(%04X, %p, %ld);\n", hmmio, pch, cch);
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	count = _lwrite32(LOWORD(lpmminfo->dwReserved2), (LPSTR)pch, cch);
	GlobalUnlock16(hmmio);
	return count;
}

/**************************************************************************
* 				mmioSeek       		[MMSYSTEM.1214]
*/
LONG mmioSeek(HMMIO16 hmmio, LONG lOffset, int iOrigin)
{
	int		count;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioSeek(%04X, %08lX, %d);\n", hmmio, lOffset, iOrigin);
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) {
		dprintf_mmio(stddeb, "mmioSeek // can't lock hmmio=%04X !\n", hmmio);
		return 0;
		}
	count = _llseek32((HFILE32)lpmminfo->dwReserved2, lOffset, iOrigin);
	GlobalUnlock16(hmmio);
	return count;
}

/**************************************************************************
* 				mmioGetInfo	       	[MMSYSTEM.1215]
*/
UINT16 mmioGetInfo(HMMIO16 hmmio, MMIOINFO * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioGetInfo\n");
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	memcpy(lpmmioinfo, lpmminfo, sizeof(MMIOINFO));
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetInfo    		[MMSYSTEM.1216]
*/
UINT16 mmioSetInfo(HMMIO16 hmmio, const MMIOINFO * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioSetInfo\n");
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetBuffer		[MMSYSTEM.1217]
*/
UINT16 mmioSetBuffer(HMMIO16 hmmio, LPSTR pchBuffer, 
						LONG cchBuffer, UINT16 uFlags)
{
	dprintf_mmio(stddeb, "mmioSetBuffer // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioFlush      		[MMSYSTEM.1218]
*/
UINT16 mmioFlush(HMMIO16 hmmio, UINT16 uFlags)
{
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioFlush(%04X, %04X)\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioAdvance    		[MMSYSTEM.1219]
*/
UINT16 mmioAdvance(HMMIO16 hmmio, MMIOINFO * lpmmioinfo, UINT16 uFlags)
{
	int		count = 0;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioAdvance\n");
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if (uFlags == MMIO_READ) {
		count = _lread32(LOWORD(lpmminfo->dwReserved2), 
			lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer);
		}
	if (uFlags == MMIO_WRITE) {
		count = _lwrite32(LOWORD(lpmminfo->dwReserved2),
			lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer);
		}
	lpmmioinfo->pchNext	+= count;
	GlobalUnlock16(hmmio);
	lpmminfo->lDiskOffset = _llseek32((HFILE32)lpmminfo->dwReserved2, 0, SEEK_CUR);
	return 0;
}

/**************************************************************************
* 				mmioStringToFOURCC	[MMSYSTEM.1220]
*/
FOURCC mmioStringToFOURCC(LPCSTR sz, UINT16 uFlags)
{
	dprintf_mmio(stddeb, "mmioStringToFOURCC // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioInstallIOProc	[MMSYSTEM.1221]
*/
LPMMIOPROC mmioInstallIOProc(FOURCC fccIOProc, 
				LPMMIOPROC pIOProc, DWORD dwFlags)
{
	dprintf_mmio(stddeb, "mmioInstallIOProc // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioSendMessage		[MMSYSTEM.1222]
*/
LRESULT mmioSendMessage(HMMIO16 hmmio, UINT16 uMessage,
					    LPARAM lParam1, LPARAM lParam2)
{
	dprintf_mmio(stddeb, "mmioSendMessage // empty stub \n");
	return 0;
}

/**************************************************************************
* 				mmioDescend	       	[MMSYSTEM.1223]
*/
UINT16 mmioDescend(HMMIO16 hmmio, MMCKINFO * lpck,
		    const MMCKINFO * lpckParent, UINT16 uFlags)
{
	DWORD	dwfcc, dwOldPos;
	LPMMIOINFO	lpmminfo;
	dprintf_mmio(stddeb, "mmioDescend(%04X, %p, %p, %04X);\n", 
				hmmio, lpck, lpckParent, uFlags);
	if (lpck == NULL) return 0;
	lpmminfo = (LPMMIOINFO)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	dwfcc = lpck->ckid;
	dprintf_mmio(stddeb, "mmioDescend // dwfcc=%08lX\n", dwfcc);
	dprintf_mmio(stddeb, "mmioDescend // hfile = %ld\n", lpmminfo->dwReserved2);
	dwOldPos = _llseek32((HFILE32)lpmminfo->dwReserved2, 0, SEEK_CUR);
	dprintf_mmio(stddeb, "mmioDescend // dwOldPos=%ld\n", dwOldPos);
	if (lpckParent != NULL) {
		dprintf_mmio(stddeb, "mmioDescend // seek inside parent at %ld !\n", lpckParent->dwDataOffset);
		dwOldPos = _llseek32((HFILE32)lpmminfo->dwReserved2,
					lpckParent->dwDataOffset, SEEK_SET);
		}
/*

   It seems to be that FINDRIFF should not be treated the same as the 
   other FINDxxx so I treat it as a MMIO_FINDxxx

	if ((uFlags & MMIO_FINDCHUNK) || (uFlags & MMIO_FINDRIFF) || 
		(uFlags & MMIO_FINDLIST)) {
*/
	if ((uFlags & MMIO_FINDCHUNK) || (uFlags & MMIO_FINDLIST)) {
		dprintf_mmio(stddeb, "mmioDescend // MMIO_FINDxxxx dwfcc=%08lX !\n", dwfcc);
		while (TRUE) {
		        size_t ix;

			ix =_lread32((HFILE32)lpmminfo->dwReserved2, (LPSTR)lpck, sizeof(MMCKINFO));
			dprintf_mmio(stddeb, "mmioDescend // after _lread32 ix = %d req = %d, errno = %d\n",ix,sizeof(MMCKINFO),errno);
			if (ix < sizeof(MMCKINFO)) {

				_llseek32((HFILE32)lpmminfo->dwReserved2, dwOldPos, SEEK_SET);
				GlobalUnlock16(hmmio);
				dprintf_mmio(stddeb, "mmioDescend // return ChunkNotFound\n");
				return MMIOERR_CHUNKNOTFOUND;
				}
			dprintf_mmio(stddeb, "mmioDescend // dwfcc=%08lX ckid=%08lX cksize=%08lX !\n", 
									dwfcc, lpck->ckid, lpck->cksize);
			if (dwfcc == lpck->ckid) break;
			dwOldPos += lpck->cksize + 2 * sizeof(DWORD);
			if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
				dwOldPos += sizeof(DWORD);
			_llseek32((HFILE32)lpmminfo->dwReserved2, dwOldPos, SEEK_SET);
			}
		}
	else {
		if (_lread32(LOWORD(lpmminfo->dwReserved2), (LPSTR)lpck, 
				sizeof(MMCKINFO)) < sizeof(MMCKINFO)) {
                    _llseek32((HFILE32)lpmminfo->dwReserved2, dwOldPos, SEEK_SET);
			GlobalUnlock16(hmmio);
 		        dprintf_mmio(stddeb, "mmioDescend // return ChunkNotFound 2nd\n");
			return MMIOERR_CHUNKNOTFOUND;
			}
		}
	lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
		lpck->dwDataOffset += sizeof(DWORD);
	lpmminfo->lDiskOffset = _llseek32((HFILE32)lpmminfo->dwReserved2, 
                                          lpck->dwDataOffset, SEEK_SET);
	GlobalUnlock16(hmmio);
	dprintf_mmio(stddeb, "mmioDescend // lpck->ckid=%08lX lpck->cksize=%ld !\n", 
								lpck->ckid, lpck->cksize);
	dprintf_mmio(stddeb, "mmioDescend // lpck->fccType=%08lX !\n", lpck->fccType);
	return 0;
}

/**************************************************************************
* 				mmioAscend     		[MMSYSTEM.1224]
*/
UINT16 mmioAscend(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	dprintf_mmio(stddeb, "mmioAscend // empty stub !\n");
	return 0;
}

/**************************************************************************
* 				mmioCreateChunk		[MMSYSTEM.1225]
*/
UINT16 mmioCreateChunk(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	dprintf_mmio(stddeb, "mmioCreateChunk // empty stub \n");
	return 0;
}


/**************************************************************************
* 				mmioRename     		[MMSYSTEM.1226]
*/
UINT16 mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
     MMIOINFO * lpmmioinfo, DWORD dwRenameFlags)
{
	dprintf_mmio(stddeb, "mmioRename('%s', '%s', %p, %08lX); // empty stub \n",
			szFileName, szNewFileName, lpmmioinfo, dwRenameFlags);
	return 0;
}

/**************************************************************************
* 				DrvOpen	       		[MMSYSTEM.1100]
*/
HDRVR16 DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
	dprintf_mmsys(stddeb, "DrvOpen('%s', '%s', %08lX);\n",
		lpDriverName, lpSectionName, lParam);
	return OpenDriver(lpDriverName, lpSectionName, lParam);
}


/**************************************************************************
* 				DrvClose       		[MMSYSTEM.1101]
*/
LRESULT DrvClose(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
	dprintf_mmsys(stddeb, "DrvClose(%04X, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
	return CloseDriver(hDrvr, lParam1, lParam2);
}


/**************************************************************************
* 				DrvSendMessage		[MMSYSTEM.1102]
*/
LRESULT DrvSendMessage(HDRVR16 hDriver, WORD msg, LPARAM lParam1, LPARAM lParam2)
{
	DWORD 	dwDriverID = 0;
	dprintf_mmsys(stddeb, "DrvSendMessage(%04X, %04X, %08lX, %08lX);\n",
					hDriver, msg, lParam1, lParam2);
	return CDAUDIO_DriverProc(dwDriverID, hDriver, msg, lParam1, lParam2);
}

/**************************************************************************
* 				DrvGetModuleHandle	[MMSYSTEM.1103]
*/
HANDLE16 DrvGetModuleHandle(HDRVR16 hDrvr)
{
	dprintf_mmsys(stddeb, "DrvGetModuleHandle(%04X);\n", hDrvr);
        return 0;
}


/**************************************************************************
* 				DrvDefDriverProc	[MMSYSTEM.1104]
*/
LRESULT DrvDefDriverProc(DWORD dwDriverID, HDRVR16 hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	return DefDriverProc(dwDriverID, hDriv, wMsg, dwParam1, dwParam2);
}
