/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */

static char Copyright[] = "Copyright  Martin Ayotte, 1993";

#include "stdio.h"
#include "win.h"
#include "mmsystem.h"


int MCI_LibMain(HANDLE hInstance, WORD wDataSeg,
		 WORD cbHeapSize, LPSTR lpCmdLine)
{
	printf("MMSYSTEM DLL INIT ... hInst=%04X \n", hInstance);
	return(TRUE);
}


BOOL WINAPI sndPlaySound(LPCSTR lpszSoundName, UINT uFlags)
{
	printf("sndPlaySound // lpszSoundName='%s' uFlags=%04X !\n", 
										lpszSoundName, uFlags);
	return 0;
}



WORD WINAPI mmsystemGetVersion()
{
	printf("mmsystemGetVersion // 0.4.0 ...?... :-) !\n");
	return(0x0040);
}


void WINAPI OutputDebugStr(LPCSTR str)
{
	printf("OutputDebugStr('%s');\n", str);
}


UINT WINAPI auxGetNumDevs()
{
	printf("auxGetNumDevs !\n");
	return 0;
}


UINT WINAPI auxGetDevCaps(UINT uDeviceID, AUXCAPS FAR* lpCaps, UINT uSize)
{
	printf("auxGetDevCaps !\n");
	return 0;
}


UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("auxGetVolume !\n");
	return 0;
}



UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("auxSetVolume !\n");
	return 0;
}


DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2)
{
	printf("auxOutMessage !\n");
	return 0L;
}



BOOL mciGetErrorString (DWORD wError, LPSTR lpstrBuffer, UINT uLength)
{
	LPSTR	msgptr;
	int		maxbuf;
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
	strncpy(lpstrBuffer, msgptr, maxbuf);
	lpstrBuffer[maxbuf + 1] = '\0';
	return(TRUE);
}



DWORD mciSendCommand(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
	printf("mciSendCommand(%04X, %04X, %08X, %08X)\n", 
					wDevID, wMsg, dwParam1, dwParam2);
	return MCIERR_DEVICE_NOT_INSTALLED;
}




UINT mciGetDeviceID (LPCSTR lpstrName)
{
	printf("mciGetDeviceID(%s)\n", lpstrName);
	return 0;
}



DWORD WINAPI mciSendString (LPCSTR lpstrCommand,
    LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback)
{
	printf("mciSendString('%s', %lX, %u, %X)\n", 
			lpstrCommand, lpstrReturnString, 
			uReturnLength, hwndCallback);
	return MCIERR_MISSING_COMMAND_STRING;
}



HMMIO WINAPI mmioOpen(LPSTR szFileName, MMIOINFO FAR* lpmmioinfo, DWORD dwOpenFlags)
{
	printf("mmioOpen('%s', %08X, %08X);\n", szFileName, lpmmioinfo, dwOpenFlags);
	return 0;
}


    
UINT WINAPI mmioClose(HMMIO hmmio, UINT uFlags)
{
	printf("mmioClose(%04X, %04X);\n", hmmio, uFlags);
	return 0;
}



LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	printf("mmioRead\n");
	return 0;
}



LONG WINAPI mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch)
{
	printf("mmioWrite\n");
	return 0;
}


LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
	printf("mmioSeek\n");
	return 0;
}



UINT WINAPI midiOutGetNumDevs(void)
{
	printf("midiOutGetNumDevs\n");
	return 0;
}



UINT WINAPI midiOutGetDevCaps(UINT uDeviceID,
    MIDIOUTCAPS FAR* lpCaps, UINT uSize)
{
	printf("midiOutGetDevCaps\n");
	return 0;
}


UINT WINAPI midiOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
	printf("midiOutGetVolume\n");
	return 0;
}


UINT WINAPI midiOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
	printf("midiOutSetVolume\n");
	return 0;
}


UINT WINAPI midiOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	printf("midiOutGetErrorText\n");
	return(midiGetErrorText(uError, lpText, uSize));
}


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
	strncpy(lpText, msgptr, maxbuf);
	lpText[maxbuf + 1] = '\0';
	return(TRUE);
}


UINT WINAPI midiOutOpen(HMIDIOUT FAR* lphMidiOut, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	printf("midiOutOpen\n");
	return 0;
}


UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
	printf("midiOutClose\n");
	return 0;
}


UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutPrepareHeader\n");
	return 0;
}


UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutUnprepareHeader\n");
	return 0;
}


UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
	printf("midiOutShortMsg\n");
	return 0;
}


UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
    MIDIHDR FAR* lpMidiOutHdr, UINT uSize)
{
	printf("midiOutLongMsg\n");
	return 0;
}


UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
	printf("midiOutReset\n");
	return 0;
}


UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut,
    UINT uBank, WORD FAR* lpwPatchArray, UINT uFlags)
{
	printf("midiOutCachePatches\n");
	return 0;
}


UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut,
    UINT uPatch, WORD FAR* lpwKeyArray, UINT uFlags)
{
	printf("midiOutCacheDrumPatches\n");
	return 0;
}


UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT FAR* lpuDeviceID)
{
	printf("midiOutGetID\n");
	return 0;
}


DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage, DWORD dw1, DWORD dw2)
{
	printf("midiOutMessage\n");
	return 0;
}




UINT WINAPI midiInGetNumDevs(void)
{
	printf("midiInGetNumDevs\n");
	return 0;
}



UINT WINAPI midiInGetDevCaps(UINT uDeviceID,
    LPMIDIINCAPS lpCaps, UINT uSize)
{
	printf("midiInGetDevCaps\n");
	return 0;
}



UINT WINAPI midiInGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
	printf("midiInGetErrorText\n");
	return (midiGetErrorText(uError, lpText, uSize));
}



UINT WINAPI midiInOpen(HMIDIIN FAR* lphMidiIn, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
	printf("midiInOpen\n");
	return 0;
}



UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
	printf("midiInClose\n");
	return 0;
}




UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT FAR* lpuDeviceID)
{
	printf("midiInGetID\n");
	return 0;
}



UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInPrepareHeader\n");
	return 0;
}



UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInUnprepareHeader\n");
	return 0;
}



UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
    MIDIHDR FAR* lpMidiInHdr, UINT uSize)
{
	printf("midiInAddBuffer\n");
	return 0;
}


UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
	printf("midiInStart\n");
	return 0;
}


UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
printf("midiInStop\n");
return 0;
}



UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
printf("midiInReset\n");
return 0;
}



UINT WINAPI waveOutGetNumDevs()
{
printf("waveOutGetNumDevs\n");
return 0;
}



UINT WINAPI waveOutGetDevCaps(UINT uDeviceID, WAVEOUTCAPS FAR* lpCaps, UINT uSize)
{
printf("waveOutGetDevCaps\n");
return 0;
}


UINT WINAPI waveOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
   printf("waveOutGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


UINT WINAPI waveGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
   if ((lpText == NULL) || (uSize < 1)) return(FALSE);
   lpText[0] = '\0';
   switch(uError)
      {
      case MMSYSERR_NOERROR:
         sprintf(lpText, "The specified command was carried out.");
         break;
      case MMSYSERR_ERROR:
         sprintf(lpText, "Undefined external error.");
         break;
      case MMSYSERR_BADDEVICEID:
         sprintf(lpText, "A device ID has been used that is out of range for your system.");
         break;
      case MMSYSERR_NOTENABLED:
         sprintf(lpText, "The driver was not enabled.");
         break;
      case MMSYSERR_ALLOCATED:
         sprintf(lpText, "The specified device is already in use. Wait until it is free, and then try again.");
         break;
      case MMSYSERR_INVALHANDLE:
         sprintf(lpText, "The specified device handle is invalid.");
         break;
      case MMSYSERR_NODRIVER:
         sprintf(lpText, "There is no driver installed on your system !\n");
         break;
      case MMSYSERR_NOMEM:
         sprintf(lpText, "Not enough memory available for this task. Quit one or more applications to increase available memory, and then try again.");
         break;
      case MMSYSERR_NOTSUPPORTED:
         sprintf(lpText, "This function is not supported. Use the Capabilities function to determine which functions and messages the driver supports.");
         break;
      case MMSYSERR_BADERRNUM:
         sprintf(lpText, "An error number was specified that is not defined in the system.");
         break;
      case MMSYSERR_INVALFLAG:
         sprintf(lpText, "An invalid flag was passed to a system function.");
         break;
      case MMSYSERR_INVALPARAM:
         sprintf(lpText, "An invalid parameter was passed to a system function.");
         break;
      case WAVERR_BADFORMAT:
         sprintf(lpText, "The specified format is not supported or cannot be translated. Use the Capabilities function to determine the supported formats");
         break;
      case WAVERR_STILLPLAYING:
         sprintf(lpText, "Cannot perform this operation while media data is still playing. Reset the device, or wait until the data is finished playing.");
         break;
      case WAVERR_UNPREPARED:
         sprintf(lpText, "The wave header was not prepared. Use the Prepare function to prepare the header, and then try again.");
         break;
      case WAVERR_SYNC:
         sprintf(lpText, "Cannot open the device without using the WAVE_ALLOWSYNC flag. Use the flag, and then try again.");
         break;
         
      default:
         sprintf(lpText, "Unkown MMSYSTEM Error !\n");
         break;
      }
   lpText[uSize - 1] = '\0';
   return(TRUE);
}


UINT WINAPI waveOutOpen(HWAVEOUT FAR* lphWaveOut, UINT uDeviceID,
    const WAVEFORMAT FAR* lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
printf("waveOutOpen\n");
return 0;
}



UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
printf("waveOutClose\n");
return 0;
}



UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
     WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
printf("waveOutPrepareHeader\n");
return 0;
}



UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
    WAVEHDR FAR* lpWaveOutHdr, UINT uSize)
{
printf("waveOutUnprepareHeader\n");
return 0;
}



UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, WAVEHDR FAR* lpWaveOutHdr,  UINT uSize)
{
printf("waveOutWrite\n");
return 0;
}


UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
printf("waveOutPause\n");
return 0;
}


UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
printf("waveOutRestart\n");
return 0;
}


UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
printf("waveOutReset\n");
return 0;
}



UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, MMTIME FAR* lpInfo, UINT uSize)
{
printf("waveOutGetPosition\n");
return 0;
}



UINT WINAPI waveOutGetVolume(UINT uDeviceID, DWORD FAR* lpdwVolume)
{
printf("waveOutGetVolume\n");
return 0;
}


UINT WINAPI waveOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
printf("waveOutSetVolume\n");
return 0;
}



UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT FAR* lpuDeviceID)
{
printf("waveOutGetID\n");
return 0;
}



UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, DWORD FAR* lpdwPitch)
{
printf("waveOutGetPitch\n");
return 0;
}



UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dwPitch)
{
printf("waveOutSetPitch\n");
return 0;
}


UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, DWORD FAR* lpdwRate)
{
printf("waveOutGetPlaybackRate\n");
return 0;
}



UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dwRate)
{
printf("waveOutSetPlaybackRate\n");
return 0;
}




UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
printf("waveOutBreakLoop\n");
return 0;
}



UINT WINAPI waveInGetNumDevs()
{
printf("waveInGetNumDevs\n");
return 0;
}


UINT WINAPI waveInGetDevCaps(UINT uDeviceID, WAVEINCAPS FAR* lpCaps, UINT uSize)
{
printf("waveInGetDevCaps\n");
return 0;
}


UINT WINAPI waveInGetErrorText(UINT uError, LPSTR lpText, UINT uSize)
{
   printf("waveInGetErrorText\n");
   return(waveGetErrorText(uError, lpText, uSize));
}


UINT WINAPI waveInOpen(HWAVEIN FAR* lphWaveIn, UINT uDeviceID,
    const WAVEFORMAT FAR* lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
printf("waveInOpen\n");
return 0;
}


UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
printf("waveInClose\n");
return 0;
}


UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
printf("waveInPrepareHeader\n");
return 0;
}


UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
printf("waveInUnprepareHeader\n");
return 0;
}



UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize)
{
printf("waveInAddBuffer\n");
return 0;
}

UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
printf("waveInReset\n");
return 0;
}


UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
printf("waveInStart\n");
return 0;
}


UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
printf("waveInStop\n");
return 0;
}



UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, MMTIME FAR* lpInfo, UINT uSize)
{
printf("waveInGetPosition\n");
return 0;
}


UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT FAR* lpuDeviceID)
{
printf("waveInGetID\n");
return 0;
}


/*
UINT WINAPI mciGetDeviceIDFromElementID (DWORD dwElementID,
    LPCSTR lpstrType);

BOOL WINAPI mciSetYieldProc (UINT uDeviceID, YIELDPROC fpYieldProc,
    DWORD dwYieldData);

HTASK WINAPI mciGetCreatorTask(UINT uDeviceID);
YIELDPROC WINAPI mciGetYieldProc (UINT uDeviceID, DWORD FAR* lpdwYieldData);


FOURCC WINAPI mmioStringToFOURCC(LPCSTR sz, UINT uFlags);
LPMMIOPROC WINAPI mmioInstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc,
    DWORD dwFlags);
UINT WINAPI mmioGetInfo(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags);
UINT WINAPI mmioSetInfo(HMMIO hmmio, const MMIOINFO FAR* lpmmioinfo, UINT uFlags);
UINT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer,
    UINT uFlags);
UINT WINAPI mmioFlush(HMMIO hmmio, UINT uFlags);
UINT WINAPI mmioAdvance(HMMIO hmmio, MMIOINFO FAR* lpmmioinfo, UINT uFlags);
LRESULT WINAPI mmioSendMessage(HMMIO hmmio, UINT uMessage,
    LPARAM lParam1, LPARAM lParam2);
UINT WINAPI mmioDescend(HMMIO hmmio, MMCKINFO FAR* lpck,
    const MMCKINFO FAR* lpckParent, UINT uFlags);
UINT WINAPI mmioAscend(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags);
UINT WINAPI mmioCreateChunk(HMMIO hmmio, MMCKINFO FAR* lpck, UINT uFlags);

DWORD WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage, DWORD dw1, DWORD dw2);


DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage, DWORD dw1, DWORD dw2);

UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
    WAVEHDR FAR* lpWaveInHdr, UINT uSize);
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage, DWORD dw1, DWORD dw2);

*/

