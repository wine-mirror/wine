/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */

static char Copyright[] = "Copyright  Martin Ayotte, 1993";

#include "stdio.h"
#include "win.h"
#include "driver.h"
#include "mmsystem.h"

static WORD		mciActiveDev = 0;


UINT WINAPI midiGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT WINAPI waveGetErrorText(UINT uError, LPSTR lpText, UINT uSize);


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
	printf("EMPTY STUB !!! sndPlaySound // SoundName='%s' uFlags=%04X !\n", 
										lpszSoundName, uFlags);
	return 0;
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
	printf("EMPTY STUB !!! DriverCallback() !\n");
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
	printf("EMPTY STUB !!! JoyGetDevCaps(%04X, %08X, %d);\n", 
										wID, lpCaps, wSize);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoyGetPos			[MMSYSTEM.103]
*/
WORD JoyGetPos(WORD wID, LPJOYINFO lpInfo)
{
	printf("EMPTY STUB !!! JoyGetPos(%04X, %08X);\n", wID, lpInfo);
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
* 				JoyGetThreshold		[MMSYSTEM.104]
*/
WORD JoyGetThreshold(WORD wID, LPWORD lpThreshold)
{
	printf("EMPTY STUB !!! JoyGetThreshold(%04X, %08X);\n", wID, lpThreshold);
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
	printf("auxGetNumDevs !\n");
	return 0;
}

/**************************************************************************
* 				auxGetDevCaps		[MMSYSTEM.351]
*/
UINT WINAPI auxGetDevCaps(UINT uDeviceID, AUXCAPS FAR* lpCaps, UINT uSize)
{
	printf("auxGetDevCaps !\n");
	return 0;
}

/**************************************************************************
* 				auxGetVolume		[MMSYSTEM.352]
*/
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("auxGetVolume !\n");
	return 0;
}

/**************************************************************************
* 				auxSetVolume		[MMSYSTEM.353]
*/
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("auxSetVolume !\n");
	return 0;
}

/**************************************************************************
* 				auxOutMessage		[MMSYSTEM.354]
*/
DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2)
{
	printf("auxOutMessage !\n");
	return 0L;
}

/**************************************************************************
* 				mciGetErrorString		[MMSYSTEM.706]
*/
BOOL mciGetErrorString (DWORD wError, LPSTR lpstrBuffer, UINT uLength)
{
	LPSTR	msgptr;
	int		maxbuf;
	printf("mciGetErrorString(%04X, %08X, %d);\n", wError, lpstrBuffer, uLength);
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
			msgptr = "Unkown MCI Error !\n";
			break;
		}
	maxbuf = min(uLength - 1, strlen(msgptr));
	if (maxbuf > 0) strncpy(lpstrBuffer, msgptr, maxbuf);
	lpstrBuffer[maxbuf + 1] = '\0';
	return(TRUE);
}


/**************************************************************************
* 				mciWaveOpen					[internal]
*/
DWORD mciWaveOpen(UINT wDevID, DWORD dwParam, LPMCI_WAVE_OPEN_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	printf("mciWaveOpen(%04X, %08X, %08X)\n", wDevID, dwParam, lpParms);
	return MCIERR_INTERNAL;
}


/**************************************************************************
* 				mciOpen					[internal]
*/
DWORD mciOpen(UINT wDevID, DWORD dwParam, LPMCI_OPEN_PARMS lpParms)
{
	char	str[128];
	DWORD	dwDevTyp = 0;
	if (lpParms == NULL) return MCIERR_INTERNAL;
	printf("mciOpen(%04X, %08X, %08X)\n", wDevID, dwParam, lpParms);
	if (dwParam & MCI_OPEN_TYPE) {
		if (lpParms->lpstrDeviceType == NULL) return MCIERR_INTERNAL;
		if (dwParam & MCI_OPEN_TYPE_ID) {
			printf("MCI_OPEN // Dev=%08X !\n", lpParms->lpstrDeviceType);
			dwDevTyp = (DWORD)lpParms->lpstrDeviceType;
			}
		else {
			printf("MCI_OPEN // Dev='%s' !\n", lpParms->lpstrDeviceType);
			strcpy(str, lpParms->lpstrDeviceType);
			AnsiUpper(str);
			if (strcmp(str, "CDAUDIO") == 0) {
				dwDevTyp = MCI_DEVTYPE_CD_AUDIO;
				}
			else
			if (strcmp(str, "WAVEAUDIO") == 0) {
				dwDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
				}
			else
			if (strcmp(str, "SEQUENCER") == 0)	{
				dwDevTyp = MCI_DEVTYPE_SEQUENCER;
				}
			else
			if (strcmp(str, "ANIMATION1") == 0) {
				dwDevTyp = MCI_DEVTYPE_ANIMATION;
				}
			}
		switch(dwDevTyp) {
			case MCI_DEVTYPE_CD_AUDIO:
				printf("MCI_OPEN // No SEQUENCER yet !\n");
				return MCIERR_DEVICE_NOT_INSTALLED;
			case MCI_DEVTYPE_WAVEFORM_AUDIO:
				printf("MCI_OPEN // No WAVEAUDIO yet !\n");
				return MCIERR_DEVICE_NOT_INSTALLED;
			case MCI_DEVTYPE_SEQUENCER:
				printf("MCI_OPEN // No SEQUENCER yet !\n");
				return MCIERR_DEVICE_NOT_INSTALLED;
			case MCI_DEVTYPE_ANIMATION:
				printf("MCI_OPEN // No ANIMATION yet !\n");
				return MCIERR_DEVICE_NOT_INSTALLED;
			case MCI_DEVTYPE_DIGITAL_VIDEO:
				printf("MCI_OPEN // No DIGITAL_VIDEO yet !\n");
				return MCIERR_DEVICE_NOT_INSTALLED;
			default:
				printf("MCI_OPEN // Invalid Device Name '%08X' !\n", lpParms->lpstrDeviceType);
				return MCIERR_INVALID_DEVICE_NAME;
			}
		lpParms->wDeviceID = ++mciActiveDev;
		printf("MCI_OPEN // wDeviceID=%04X !\n", lpParms->wDeviceID);
		return 0;
		}
	if (dwParam & MCI_OPEN_ELEMENT) {
		printf("MCI_OPEN // Element !\n");
		printf("MCI_OPEN // Elem=%s' !\n", lpParms->lpstrElementName);
		}
	return MCIERR_INTERNAL;
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
* 				mciGetDevCaps			[internal]
*/
DWORD mciGetDevCaps(UINT wDevID, DWORD dwParam, LPMCI_GETDEVCAPS_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->dwReturn = 0;
	return 0;
}


/**************************************************************************
* 				mciInfo					[internal]
*/
DWORD mciInfo(UINT wDevID, DWORD dwParam, LPMCI_INFO_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	lpParms->lpstrReturn = NULL;
	lpParms->dwRetSize = 0;
	return 0;
}


/**************************************************************************
* 				mciStatus				[internal]
*/
DWORD mciStatus(UINT wDevID, DWORD dwParam, LPMCI_STATUS_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
}


/**************************************************************************
* 				mciPlay					[internal]
*/
DWORD mciPlay(UINT wDevID, DWORD dwParam, LPMCI_PLAY_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
}


/**************************************************************************
* 				mciRecord				[internal]
*/
DWORD mciRecord(UINT wDevID, DWORD dwParam, LPMCI_RECORD_PARMS lpParms)
{
	if (lpParms == NULL) return MCIERR_INTERNAL;
	return 0;
}


/**************************************************************************
* 				mciClose				[internal]
*/
DWORD mciClose(UINT wDevID)
{
	return 0;
}


/**************************************************************************
* 				mciSendCommand			[MMSYSTEM.701]
*/
DWORD mciSendCommand(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
	printf("mciSendCommand(%04X, %04X, %08X, %08X)\n", 
					wDevID, wMsg, dwParam1, dwParam2);
	switch(wMsg) {
		case MCI_OPEN:
			printf("mciSendCommand // MCI_OPEN !\n");
			if (dwParam1 & MCI_WAVE_OPEN_BUFFER)
				return mciWaveOpen(wDevID, dwParam1, 
					(LPMCI_WAVE_OPEN_PARMS)dwParam2);
			else
				return mciOpen(wDevID, dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
		case MCI_PLAY:
			printf("mciSendCommand // MCI_PLAY !\n");
			return mciPlay(wDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
		case MCI_RECORD:
			printf("mciSendCommand // MCI_RECORD !\n");
			return mciRecord(wDevID, dwParam1, (LPMCI_RECORD_PARMS)dwParam2);
		case MCI_CLOSE:
			printf("mciSendCommand // MCI_CLOSE !\n");
			return mciClose(wDevID);
		case MCI_SOUND:
			printf("mciSendCommand // MCI_SOUND !\n");
			return mciSound(wDevID, dwParam1, (LPMCI_SOUND_PARMS)dwParam2);
		case MCI_STATUS:
			printf("mciSendCommand // MCI_STATUS !\n");
			return mciStatus(wDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
		case MCI_INFO:
			printf("mciSendCommand // MCI_INFO !\n");
			return mciInfo(wDevID, dwParam1, (LPMCI_INFO_PARMS)dwParam2);
		case MCI_GETDEVCAPS:
			printf("mciSendCommand // MCI_GETDEVCAPS !\n");
			return mciGetDevCaps(wDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
		}
	return MCIERR_DEVICE_NOT_INSTALLED;
}

/**************************************************************************
* 				mciGetDeviceID			[MMSYSTEM.703]
*/
UINT mciGetDeviceID (LPCSTR lpstrName)
{
	printf("mciGetDeviceID(%s)\n", lpstrName);
	return 0;
}

/**************************************************************************
* 				mciSendString			[MMSYSTEM.702]
*/
DWORD WINAPI mciSendString (LPCSTR lpstrCommand,
    LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
{
	printf("mciSendString('%s', %lX, %u, %X)\n", 
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
}

/**************************************************************************
* 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
*/
UINT WINAPI mciGetDeviceIDFromElementID(DWORD dwElementID, LPCSTR lpstrType)
{
}

/**************************************************************************
* 				mciGetYieldProc		[MMSYSTEM.716]
*/
YIELDPROC WINAPI mciGetYieldProc(UINT uDeviceID, DWORD FAR* lpdwYieldData)
{
}

/**************************************************************************
* 				mciGetCreatorTask	[MMSYSTEM.717]
*/
HTASK WINAPI mciGetCreatorTask(UINT uDeviceID)
{
}

/**************************************************************************
* 				midiOutGetNumDevs	[MMSYSTEM.201]
*/
UINT WINAPI midiOutGetNumDevs(void)
{
	printf("midiOutGetNumDevs\n");
	return 0;
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
			msgptr = "Unkown MIDI Error !\n";
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
	printf("midiOutOpen\n");
	if (lphMidiOut != NULL) *lphMidiOut = 0;
	return 0;
}

/**************************************************************************
* 				midiOutClose		[MMSYSTEM.205]
*/
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
	printf("midiOutClose\n");
	return 0;
}

/**************************************************************************
* 				midiOutPrepareHeader	[MMSYSTEM.206]
*/
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutPrepareHeader\n");
	return 0;
}

/**************************************************************************
* 				midiOutUnprepareHeader	[MMSYSTEM.207]
*/
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutUnprepareHeader\n");
	return 0;
}

/**************************************************************************
* 				midiOutShortMsg		[MMSYSTEM.208]
*/
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
	printf("midiOutShortMsg\n");
	return 0;
}

/**************************************************************************
* 				midiOutLongMsg		[MMSYSTEM.209]
*/
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutLongMsg\n");
	return 0;
}

/**************************************************************************
* 				midiOutReset		[MMSYSTEM.210]
*/
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
	printf("midiOutReset\n");
	return 0;
}

/**************************************************************************
* 				midiOutGetVolume	[MMSYSTEM.211]
*/
UINT WINAPI midiOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("midiOutGetVolume\n");
	return 0;
}

/**************************************************************************
* 				midiOutSetVolume	[MMSYSTEM.212]
*/
UINT WINAPI midiOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("midiOutSetVolume\n");
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
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage, DWORD dw1, DWORD dw2)
{
	printf("midiOutMessage\n");
	return 0;
}

/**************************************************************************
* 				midiInGetNumDevs	[MMSYSTEM.301]
*/
UINT WINAPI midiInGetNumDevs(void)
{
	printf("midiInGetNumDevs\n");
	return 0;
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
	printf("midiInOpen\n");
	if (lphMidiIn != NULL) *lphMidiIn = 0;
	return 0;
}

/**************************************************************************
* 				midiInClose		[MMSYSTEM.305]
*/
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
	printf("midiInClose\n");
	return 0;
}

/**************************************************************************
* 				midiInPrepareHeader	[MMSYSTEM.306]
*/
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInPrepareHeader\n");
	return 0;
}

/**************************************************************************
* 				midiInUnprepareHeader	[MMSYSTEM.307]
*/
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInUnprepareHeader\n");
	return 0;
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
	printf("midiInMessage\n");
	return 0;
}


/**************************************************************************
* 				waveOutGetNumDevs		[MMSYSTEM.401]
*/
UINT WINAPI waveOutGetNumDevs()
{
	printf("waveOutGetNumDevs\n");
	return 1;
}

/**************************************************************************
* 				waveOutGetDevCaps		[MMSYSTEM.402]
*/
UINT WINAPI waveOutGetDevCaps(UINT uDeviceID, WAVEOUTCAPS FAR* lpCaps, UINT uSize)
{
	printf("waveOutGetDevCaps\n");
	return MMSYSERR_INVALHANDLE;
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
	printf("waveGetErrorText(%04X, %08X, %d);\n", uError, lpText, uSize);
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
			msgptr = "Unkown MMSYSTEM Error !\n";
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
	printf("waveOutOpen(%08X, %d, %08X, %08X, %08X, %08X);\n", 
		lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveOutOpen	// WAVE_FORMAT_QUERY !\n");
		if (uDeviceID == (UINT)WAVE_MAPPER) {
			printf("waveOutOpen	// No WAVE_MAPPER supported yet !\n");
			return MMSYSERR_BADDEVICEID;
			}
		}
	if (lpFormat == NULL) return WAVERR_BADFORMAT;
	if (lphWaveOut != NULL) *lphWaveOut = 0;
	if (lphWaveOut != NULL) *lphWaveOut = ++mciActiveDev;
	return 0;
/*	return MMSYSERR_BADDEVICEID;*/
}

/**************************************************************************
* 				waveOutClose		[MMSYSTEM.405]
*/
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
	printf("waveOutClose\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutPrepareHeader	[MMSYSTEM.406]
*/
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
     WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
	printf("waveOutPrepareHeader\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutUnprepareHeader	[MMSYSTEM.407]
*/
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
    WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
	printf("waveOutUnprepareHeader\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutWrite		[MMSYSTEM.408]
*/
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, WAVEHDR FAR* lpWaveOutHdr,  UINT uSize)
{
	printf("waveOutWrite\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutPause		[MMSYSTEM.409]
*/
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
	printf("waveOutPause\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutRestart		[MMSYSTEM.410]
*/
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
	printf("waveOutRestart\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutReset		[MMSYSTEM.411]
*/
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
	printf("waveOutReset\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetPosition	[MMSYSTEM.412]
*/
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, MMTIME FAR* lpInfo, UINT uSize)
{
	printf("waveOutGetPosition\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetPitch		[MMSYSTEM.413]
*/
UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, DWORD FAR* lpdwPitch)
{
	printf("waveOutGetPitch\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutSetPitch		[MMSYSTEM.414]
*/
UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dwPitch)
{
	printf("waveOutSetPitch\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetVolume	[MMSYSTEM.415]
*/
UINT WINAPI waveOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("waveOutGetVolume\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutSetVolume	[MMSYSTEM.416]
*/
UINT WINAPI waveOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("waveOutSetVolume\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetPlaybackRate	[MMSYSTEM.417]
*/
UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, DWORD FAR* lpdwRate)
{
	printf("waveOutGetPlaybackRate\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutSetPlaybackRate	[MMSYSTEM.418]
*/
UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dwRate)
{
	printf("waveOutSetPlaybackRate\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutBreakLoop 	[MMSYSTEM.419]
*/
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
	printf("waveOutBreakLoop\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutGetID	 	[MMSYSTEM.420]
*/
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT FAR* lpuDeviceID)
{
	printf("waveOutGetID\n");
	return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
* 				waveOutMessage 		[MMSYSTEM.421]
*/
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage, 
							DWORD dwParam1, DWORD dwParam2)
{
}

/**************************************************************************
* 				waveInGetNumDevs 		[MMSYSTEM.501]
*/
UINT WINAPI waveInGetNumDevs()
{
	printf("waveInGetNumDevs\n");
	return 0;
}


/**************************************************************************
* 				waveInGetDevCaps 		[MMSYSTEM.502]
*/
UINT WINAPI waveInGetDevCaps(UINT uDeviceID, WAVEINCAPS FAR* lpCaps, UINT uSize)
{
	printf("waveInGetDevCaps\n");
	return MMSYSERR_INVALHANDLE;
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
	printf("waveInOpen(%08X, %d, %08X, %08X, %08X, %08X);\n", 
		lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
	if (dwFlags & WAVE_FORMAT_QUERY) {
		printf("waveInOpen	// WAVE_FORMAT_QUERY !\n");
		if (uDeviceID == (UINT)WAVE_MAPPER) {
			printf("waveInOpen	// No WAVE_MAPPER supported yet !\n");
			return MMSYSERR_BADDEVICEID;
			}
		}
	if (lphWaveIn != NULL) *lphWaveIn = 0;
	if (lpFormat == NULL) return WAVERR_BADFORMAT;
	return MMSYSERR_BADDEVICEID;
}


/**************************************************************************
* 				waveInClose			[MMSYSTEM.505]
*/
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
	printf("waveInClose\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInPrepareHeader		[MMSYSTEM.506]
*/
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	printf("waveInPrepareHeader\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInUnprepareHeader	[MMSYSTEM.507]
*/
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	printf("waveInUnprepareHeader\n");
	return MMSYSERR_INVALHANDLE;
}



/**************************************************************************
* 				waveInAddBuffer		[MMSYSTEM.508]
*/
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
	printf("waveInAddBuffer\n");
	return 0;
}


/**************************************************************************
* 				waveInStart			[MMSYSTEM.509]
*/
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
	printf("waveInStart\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInStop			[MMSYSTEM.510]
*/
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
	printf("waveInStop\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInReset			[MMSYSTEM.511]
*/
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
	printf("waveInReset\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInGetPosition	[MMSYSTEM.512]
*/
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, MMTIME FAR* lpInfo, UINT uSize)
{
	printf("waveInGetPosition\n");
	return MMSYSERR_INVALHANDLE;
}


/**************************************************************************
* 				waveInGetID			[MMSYSTEM.513]
*/
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT FAR* lpuDeviceID)
{
	printf("waveInGetID\n");
	return 0;
}


/**************************************************************************
* 				waveInMessage 		[MMSYSTEM.514]
*/
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
							DWORD dwParam1, DWORD dwParam2)
{
}



/**************************************************************************
* 				mmioOpen			[MMSYSTEM.1210]
*/
HMMIO WINAPI mmioOpen(LPSTR szFileName, MMIOINFO FAR* lpmmioinfo, DWORD dwOpenFlags)
{
	printf("mmioOpen('%s', %08X, %08X);\n", szFileName, lpmmioinfo, dwOpenFlags);
	return 0;
}


    
/**************************************************************************
* 				mmioClose			[MMSYSTEM.1211]
*/
UINT WINAPI mmioClose(HMMIO hmmio, UINT uFlags)
{
	printf("mmioClose(%04X, %04X);\n", hmmio, uFlags);
	return 0;
}



/**************************************************************************
* 				mmioRead			[MMSYSTEM.1212]
*/
LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	printf("mmioRead\n");
	return 0;
}



/**************************************************************************
* 				mmioWrite			[MMSYSTEM.1213]
*/
LONG WINAPI mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch)
{
	printf("mmioWrite\n");
	return 0;
}

/**************************************************************************
* 				mmioSeek			[MMSYSTEM.1214]
*/
LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
	printf("mmioSeek\n");
	return 0;
}

/**************************************************************************
* 				mmioGetInfo			[MMSYSTEM.1215]
*/
UINT WINAPI mmioGetInfo(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	printf("mmioGetInfo\n");
	return 0;
}

/**************************************************************************
* 				mmioGetInfo			[MMSYSTEM.1216]
*/
UINT WINAPI mmioSetInfo(HMMIO hmmio, const MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	printf("mmioSetInfo\n");
	return 0;
}

/**************************************************************************
* 				mmioSetBuffer		[MMSYSTEM.1217]
*/
UINT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, 
						LONG cchBuffer, UINT uFlags)
{
	printf("mmioSetBuffer\n");
	return 0;
}

/**************************************************************************
* 				mmioFlush			[MMSYSTEM.1218]
*/
UINT WINAPI mmioFlush(HMMIO hmmio, UINT uFlags)
{
	printf("mmioFlush\n");
	return 0;
}

/**************************************************************************
* 				mmioAdvance			[MMSYSTEM.1219]
*/
UINT WINAPI mmioAdvance(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags)
{
	printf("mmioAdvance\n");
	return 0;
}

/**************************************************************************
* 				mmioStringToFOURCC	[MMSYSTEM.1220]
*/
FOURCC WINAPI mmioStringToFOURCC(LPCSTR sz, UINT uFlags)
{
	printf("mmioStringToFOURCC\n");
	return 0;
}

/**************************************************************************
* 				mmioInstallIOProc	[MMSYSTEM.1221]
*/
LPMMIOPROC WINAPI mmioInstallIOProc(FOURCC fccIOProc, 
				LPMMIOPROC pIOProc, DWORD dwFlags)
{
	printf("mmioInstallIOProc\n");
	return 0;
}

/**************************************************************************
* 				mmioSendMessage		[MMSYSTEM.1222]
*/
LRESULT WINAPI mmioSendMessage(HMMIO hmmio, UINT uMessage,
					    LPARAM lParam1, LPARAM lParam2)
{
	printf("mmioSendMessage\n");
	return 0;
}

/**************************************************************************
* 				mmioDescend			[MMSYSTEM.1223]
*/
UINT WINAPI mmioDescend(HMMIO hmmio, MMCKINFO FAR* lpck,
		    const MMCKINFO FAR* lpckParent, UINT uFlags)
{
	printf("mmioDescend\n");
	return 0;
}

/**************************************************************************
* 				mmioAscend			[MMSYSTEM.1224]
*/
UINT WINAPI mmioAscend(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags)
{
	printf("mmioAscend\n");
	return 0;
}

/**************************************************************************
* 				mmioCreateChunk		[MMSYSTEM.1225]
*/
UINT WINAPI mmioCreateChunk(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags)
{
	printf("mmioCreateChunk\n");
	return 0;
}


/**************************************************************************
* 				mmioRename			[MMSYSTEM.1226]
*/
UINT WINAPI mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
     MMIOINFO FAR* lpmmioinfo, DWORD dwRenameFlags)
{
	printf("mmioRename('%s', '%s', %08X, %08X);\n",
			szFileName, szNewFileName, lpmmioinfo, dwRenameFlags);
	return 0;
}

/**************************************************************************
* 				DrvOpen				[MMSYSTEM.1100]
*/
HDRVR DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
	printf("DrvOpen('%s', '%s', %08X);\n",
		lpDriverName, lpSectionName, lParam);
	return OpenDriver(lpDriverName, lpSectionName, lParam);
}


/**************************************************************************
* 				DrvClose			[MMSYSTEM.1101]
*/
LRESULT DrvClose(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
	printf("DrvClose(%04X, %08X, %08X);\n", hDrvr, lParam1, lParam2);
	return CloseDriver(hDrvr, lParam1, lParam2);
}


/**************************************************************************
* 				DrvSendMessage		[MMSYSTEM.1102]
*/
LRESULT WINAPI DrvSendMessage(HDRVR hDriver, WORD msg, LPARAM lParam1, LPARAM lParam2)
{
	printf("DrvSendMessage(%04X, %04X, %08X, %08X);\n",
					hDriver, msg, lParam1, lParam2);
}

/**************************************************************************
* 				DrvGetModuleHandle	[MMSYSTEM.1103]
*/
HANDLE DrvGetModuleHandle(HDRVR hDrvr)
{
	printf("DrvGetModuleHandle(%04X);\n", hDrvr);
}


/**************************************************************************
* 				DrvDefDriverProc	[MMSYSTEM.1104]
*/
LRESULT DrvDefDriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
}



