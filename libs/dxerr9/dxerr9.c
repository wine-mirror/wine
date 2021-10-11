/*
 * DirectX 9 error routines
 *
 * Copyright 2004 Robert Reif
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "mmsystem.h"
#include "dmerror.h"
#include "ddraw.h"
#include "dinput.h"
#include "vfwmsgs.h"
#include "mmstream.h"
#include "dplay8.h"
#include "dxfile.h"
#include "d3d9.h"
#include "dsound.h"

#include "dxerr9.h"

static const struct {
    HRESULT      hr;
    const CHAR*  resultA;
    const WCHAR* resultW;
    const CHAR*  descriptionA;
    const WCHAR* descriptionW;
} info[] =
{
#define INFO(err,descr) { err, #err, L"" #err, descr, L"" descr }
    INFO( S_OK, "The function completed successfully" ),
    INFO( S_FALSE, "Call successful, but returned FALSE" ),
    INFO( ERROR_FILE_NOT_FOUND, "The system cannot find the file specified." ),
    INFO( ERROR_PATH_NOT_FOUND, "The system cannot find the path specified." ),
    INFO( ERROR_TOO_MANY_OPEN_FILES, "The system cannot open the file." ),
    INFO( ERROR_ACCESS_DENIED, "Access is denied." ),
    INFO( ERROR_INVALID_HANDLE, "The handle is invalid." ),
    INFO( ERROR_NOT_ENOUGH_MEMORY, "Not enough storage is available to process this command." ),
    INFO( ERROR_INVALID_BLOCK, "The storage control block address is invalid." ),
    INFO( ERROR_BAD_ENVIRONMENT, "The environment is incorrect." ),
    INFO( ERROR_BAD_FORMAT, "An attempt was made to load a program with an incorrect format." ),
    INFO( ERROR_OUTOFMEMORY, "The system cannot find the drive specified." ),
    INFO( VFW_S_NO_MORE_ITEMS, "The end of the list has been reached." ),
    INFO( VFW_S_DUPLICATE_NAME, "An attempt to add a filter with a duplicate name succeeded with a modified name." ),
    INFO( VFW_S_STATE_INTERMEDIATE, "The state transition has not completed." ),
    INFO( VFW_S_PARTIAL_RENDER, "Some of the streams in this movie are in an unsupported format." ),
    INFO( VFW_S_SOME_DATA_IGNORED, "The file contained some property settings that were not used." ),
    INFO( VFW_S_CONNECTIONS_DEFERRED, "Some connections have failed and have been deferred." ),
    INFO( VFW_S_RESOURCE_NOT_NEEDED, "The resource specified is no longer needed." ),
    INFO( VFW_S_MEDIA_TYPE_IGNORED, "A connection could not be made with the media type in the persistent graph, but has been made with a negotiated media type." ),
    INFO( VFW_S_VIDEO_NOT_RENDERED, "Cannot play back the video stream: no suitable decompressor could be found." ),
    INFO( VFW_S_AUDIO_NOT_RENDERED, "Cannot play back the audio stream: no audio hardware is available." ),
    INFO( VFW_S_RPZA, "Cannot play back the video stream: format 'RPZA' is not supported." ),
    INFO( VFW_S_ESTIMATED, "The value returned had to be estimated.  Its accuracy cannot be guaranteed." ),
    INFO( VFW_S_RESERVED, "This success code is reserved for internal purposes within ActiveMovie." ),
    INFO( VFW_S_STREAM_OFF, "The stream has been turned off." ),
    INFO( VFW_S_CANT_CUE, "The graph can't be cued because of lack of or corrupt data." ),
    INFO( VFW_S_NO_STOP_TIME, "The stop time for the sample was not set." ),
    INFO( VFW_S_NOPREVIEWPIN, "There was no preview pin available, so the capture pin output is being split to provide both capture and preview." ),
    INFO( VFW_S_DVD_NON_ONE_SEQUENTIAL, "The current title was not a sequential set of chapters (PGC) and the returned timing information might not be continuous." ),
    INFO( VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE, "The audio stream did not contain sufficient information to determine the contents of each channel." ),
    INFO( VFW_S_DVD_NOT_ACCURATE, "The seek into the movie was not frame accurate." ),
    INFO( D3DOK_NOAUTOGEN, "The call succeeded but there won't be any mipmaps generated" ),
    INFO( DS_NO_VIRTUALIZATION, "The call succeeded, but we had to substitute the 3D algorithm" ),
    INFO( DMUS_S_PARTIALLOAD, "The object could only load partially. This can happen if some components are not registered properly, such as embedded tracks and tools. This can also happen if some content is missing. For example, if a segment uses a DLS collection that is not in the loader's current search directory." ),
    INFO( DMUS_S_PARTIALDOWNLOAD, "Return value from IDirectMusicBand::Download() which indicates that some of the instruments safely downloaded, but others failed. This usually occurs when some instruments are on PChannels not supported by the performance or port." ),
    INFO( DMUS_S_REQUEUE, "Return value from IDirectMusicTool::ProcessPMsg() which indicates to the performance that it should cue the PMsg again automatically." ),
    INFO( DMUS_S_FREE, "Return value from IDirectMusicTool::ProcessPMsg() which indicates to the performance that it should free the PMsg automatically." ),
    INFO( DMUS_S_END, "Return value from IDirectMusicTrack::Play() which indicates to the segment that the track has no more data after mtEnd." ),
    INFO( DMUS_S_STRING_TRUNCATED, "Returned string has been truncated to fit the buffer size." ),
    INFO( DMUS_S_LAST_TOOL, "Returned from IDirectMusicGraph::StampPMsg() this indicates that the PMsg is already stamped with the last tool in the graph. The returned PMsg's tool pointer is now NULL." ),
    INFO( DMUS_S_OVER_CHORD, "Returned from IDirectMusicPerformance::MusicToMIDI() this indicates  that no note has been calculated because the music value has the note  at a position higher than the top note of the chord. This applies only to DMUS_PLAYMODE_NORMALCHORD play mode. This success code indicates that the caller should not do anything with the note. It is not meant to be played against this chord." ),
    INFO( DMUS_S_UP_OCTAVE, "Returned from IDirectMusicPerformance::MIDIToMusic()  and IDirectMusicPerformance::MusicToMIDI() this indicates that the note conversion generated a note value that is below 0, so it has been bumped up one or more octaves to be in the proper MIDI range of 0 through 127. Note that this is valid for MIDIToMusic() when using play modes DMUS_PLAYMODE_FIXEDTOCHORD and DMUS_PLAYMODE_FIXEDTOKEY, both of which store MIDI values in wMusicValue. With MusicToMIDI() it is valid for all play modes. Of course, DMUS_PLAYMODE_FIXED will never return this success code." ),
    INFO( DMUS_S_DOWN_OCTAVE, "Returned from IDirectMusicPerformance::MIDIToMusic()  and IDirectMusicPerformance::MusicToMIDI() this indicates  that the note conversion generated a note value that is above 127, so it has been bumped down one or more octaves to be in the proper MIDI range of 0 through 127.  Note that this is valid for MIDIToMusic() when using play modes DMUS_PLAYMODE_FIXEDTOCHORD and DMUS_PLAYMODE_FIXEDTOKEY, both of which store MIDI values in wMusicValue. With MusicToMIDI() it is valid for all play modes. Of course, DMUS_PLAYMODE_FIXED will never return this success code." ),
    INFO( DMUS_S_NOBUFFERCONTROL, "Although the audio output from the port will be routed to the same device as the given DirectSound buffer, buffer controls such as pan and volume will not affect the output." ),
    INFO( DMUS_S_GARBAGE_COLLECTED, "The requested operation was not performed because during CollectGarbage the loader determined that the object had been released." ),
    INFO( E_PENDING, "The data necessary to complete this operation is not yet available." ),
    INFO( E_NOTIMPL, "The function called is not supported at this time" ),
    INFO( E_NOINTERFACE, "The requested COM interface is not available" ),
    INFO( E_POINTER, "Invalid pointer" ),
    INFO( E_ABORT, "Operation aborted" ),
    INFO( E_FAIL, "An undetermined error occurred" ),
    INFO( E_UNEXPECTED, "Catastrophic failure" ),
    INFO( CLASSFACTORY_E_FIRST, "This object does not support aggregation" ),
    INFO( REGDB_E_CLASSNOTREG, "Class not registered" ),
    INFO( CO_E_NOTINITIALIZED, "CoInitialize has not been called." ),
    INFO( CO_E_ALREADYINITIALIZED, "CoInitialize has already been called." ),
    INFO( DIERR_INSUFFICIENTPRIVS, "& VFW_E_INVALIDMEDIATYPE Unable to IDirectInputJoyConfig_Acquire because the user does not have sufficient privileges to change the joystick configuration. & An invalid media type was specified" ),
    INFO( DIERR_DEVICEFULL, "& VFW_E_INVALIDSUBTYPE & DMO_E_INVALIDSTREAMINDEX The device is full. & An invalid media subtype was specified." ),
    INFO( DIERR_MOREDATA, "& VFW_E_NEED_OWNER & DMO_E_INVALIDTYPE Not all the requested information fit into the buffer. & This object can only be created as an aggregated object." ),
    INFO( DIERR_NOTDOWNLOADED, "& VFW_E_ENUM_OUT_OF_SYNC & DMO_E_TYPE_NOT_SET The effect is not downloaded. & The enumerator has become invalid." ),
    INFO( DIERR_HASEFFECTS, "& VFW_E_ALREADY_CONNECTED & DMO_E_NOTACCEPTING The device cannot be reinitialized because there are still effects attached to it. & At least one of the pins involved in the operation is already connected." ),
    INFO( DIERR_NOTEXCLUSIVEACQUIRED, "& VFW_E_FILTER_ACTIVE & DMO_E_TYPE_NOT_ACCEPTED The operation cannot be performed unless the device is acquired in DISCL_EXCLUSIVE mode. & This operation cannot be performed because the filter is active." ),
    INFO( DIERR_INCOMPLETEEFFECT, "& VFW_E_NO_TYPES & DMO_E_NO_MORE_ITEMS The effect could not be downloaded because essential information is missing.  For example, no axes have been associated with the effect, or no type-specific information has been created. & One of the specified pins supports no media types." ),
    INFO( DIERR_NOTBUFFERED, "& VFW_E_NO_ACCEPTABLE_TYPES Attempted to read buffered device data from a device that is not buffered. & There is no common media type between these pins." ),
    INFO( DIERR_EFFECTPLAYING, "& VFW_E_INVALID_DIRECTION An attempt was made to modify parameters of an effect while it is playing.  Not all hardware devices support altering the parameters of an effect while it is playing. & Two pins of the same direction cannot be connected together." ),
    INFO( DIERR_UNPLUGGED, "& VFW_E_NOT_CONNECTED    The operation could not be completed because the device is not plugged in. & The operation cannot be performed because the pins are not connected." ),
    INFO( DIERR_REPORTFULL, "& VFW_E_NO_ALLOCATOR    SendDeviceData failed because more information was requested to be sent than can be sent to the device.  Some devices have restrictions on how much data can be sent to them.  (For example, there might be a limit on the number of buttons that can be pressed at once.) & No sample buffer allocator is available." ),
    INFO( DIERR_MAPFILEFAIL, "& VFW_E_RUNTIME_ERROR  A mapper file function failed because reading or writing the user or IHV settings file failed. & A run-time error occurred." ),
    INFO( VFW_E_BUFFER_NOTSET, "No buffer space has been set" ),
    INFO( VFW_E_BUFFER_OVERFLOW, "The buffer is not big enough." ),
    INFO( VFW_E_BADALIGN, "An invalid alignment was specified." ),
    INFO( VFW_E_ALREADY_COMMITTED, "Cannot change allocated memory while the filter is active." ),
    INFO( VFW_E_BUFFERS_OUTSTANDING, "One or more buffers are still active." ),
    INFO( VFW_E_NOT_COMMITTED, "Cannot allocate a sample when the allocator is not active." ),
    INFO( VFW_E_SIZENOTSET, "Cannot allocate memory because no size has been set." ),
    INFO( VFW_E_NO_CLOCK, "Cannot lock for synchronization because no clock has been defined." ),
    INFO( VFW_E_NO_SINK, "Quality messages could not be sent because no quality sink has been defined." ),
    INFO( VFW_E_NO_INTERFACE, "A required interface has not been implemented." ),
    INFO( VFW_E_NOT_FOUND, "An object or name was not found." ),
    INFO( VFW_E_CANNOT_CONNECT, "No combination of intermediate filters could be found to make the connection." ),
    INFO( VFW_E_CANNOT_RENDER, "No combination of filters could be found to render the stream." ),
    INFO( VFW_E_CHANGING_FORMAT, "Could not change formats dynamically." ),
    INFO( VFW_E_NO_COLOR_KEY_SET, "No color key has been set." ),
    INFO( VFW_E_NOT_OVERLAY_CONNECTION, "Current pin connection is not using the IOverlay transport." ),
    INFO( VFW_E_NOT_SAMPLE_CONNECTION, "Current pin connection is not using the IMemInputPin transport." ),
    INFO( VFW_E_PALETTE_SET, "Setting a color key would conflict with the palette already set." ),
    INFO( VFW_E_COLOR_KEY_SET, "Setting a palette would conflict with the color key already set." ),
    INFO( VFW_E_NO_COLOR_KEY_FOUND, "No matching color key is available." ),
    INFO( VFW_E_NO_PALETTE_AVAILABLE, "No palette is available." ),
    INFO( VFW_E_NO_DISPLAY_PALETTE, "Display does not use a palette." ),
    INFO( VFW_E_TOO_MANY_COLORS, "Too many colors for the current display settings." ),
    INFO( VFW_E_STATE_CHANGED, "The state changed while waiting to process the sample." ),
    INFO( VFW_E_NOT_STOPPED, "The operation could not be performed because the filter is not stopped." ),
    INFO( VFW_E_NOT_PAUSED, "The operation could not be performed because the filter is not paused." ),
    INFO( VFW_E_NOT_RUNNING, "The operation could not be performed because the filter is not running." ),
    INFO( VFW_E_WRONG_STATE, "The operation could not be performed because the filter is in the wrong state." ),
    INFO( VFW_E_START_TIME_AFTER_END, "The sample start time is after the sample end time." ),
    INFO( VFW_E_INVALID_RECT, "The supplied rectangle is invalid." ),
    INFO( VFW_E_TYPE_NOT_ACCEPTED, "This pin cannot use the supplied media type." ),
    INFO( VFW_E_SAMPLE_REJECTED, "This sample cannot be rendered." ),
    INFO( VFW_E_SAMPLE_REJECTED_EOS, "This sample cannot be rendered because the end of the stream has been reached." ),
    INFO( VFW_E_DUPLICATE_NAME, "An attempt to add a filter with a duplicate name failed." ),
    INFO( VFW_E_TIMEOUT, "A time-out has expired." ),
    INFO( VFW_E_INVALID_FILE_FORMAT, "The file format is invalid." ),
    INFO( VFW_E_ENUM_OUT_OF_RANGE, "The list has already been exhausted." ),
    INFO( VFW_E_CIRCULAR_GRAPH, "The filter graph is circular." ),
    INFO( VFW_E_NOT_ALLOWED_TO_SAVE, "Updates are not allowed in this state." ),
    INFO( VFW_E_TIME_ALREADY_PASSED, "An attempt was made to queue a command for a time in the past." ),
    INFO( VFW_E_ALREADY_CANCELLED, "The queued command has already been canceled." ),
    INFO( VFW_E_CORRUPT_GRAPH_FILE, "Cannot render the file because it is corrupt." ),
    INFO( VFW_E_ADVISE_ALREADY_SET, "An overlay advise link already exists." ),
    INFO( VFW_E_NO_MODEX_AVAILABLE, "No full-screen modes are available." ),
    INFO( VFW_E_NO_ADVISE_SET, "This Advise cannot be canceled because it was not successfully set." ),
    INFO( VFW_E_NO_FULLSCREEN, "A full-screen mode is not available." ),
    INFO( VFW_E_IN_FULLSCREEN_MODE, "Cannot call IVideoWindow methods while in full-screen mode." ),
    INFO( VFW_E_UNKNOWN_FILE_TYPE, "The media type of this file is not recognized." ),
    INFO( VFW_E_CANNOT_LOAD_SOURCE_FILTER, "The source filter for this file could not be loaded." ),
    INFO( VFW_E_FILE_TOO_SHORT, "A file appeared to be incomplete." ),
    INFO( VFW_E_INVALID_FILE_VERSION, "The version number of the file is invalid." ),
    INFO( VFW_E_INVALID_CLSID, "This file is corrupt: it contains an invalid class identifier." ),
    INFO( VFW_E_INVALID_MEDIA_TYPE, "This file is corrupt: it contains an invalid media type." ),
    INFO( VFW_E_SAMPLE_TIME_NOT_SET, "No time stamp has been set for this sample." ),
    INFO( VFW_E_MEDIA_TIME_NOT_SET, "No media time stamp has been set for this sample." ),
    INFO( VFW_E_NO_TIME_FORMAT_SET, "No media time format has been selected." ),
    INFO( VFW_E_MONO_AUDIO_HW, "Cannot change balance because audio device is mono only." ),
    INFO( VFW_E_NO_DECOMPRESSOR, "Cannot play back the video stream: no suitable decompressor could be found." ),
    INFO( VFW_E_NO_AUDIO_HARDWARE, "Cannot play back the audio stream: no audio hardware is available, or the hardware is not responding." ),
    INFO( VFW_E_RPZA, "Cannot play back the video stream: format 'RPZA' is not supported." ),
    INFO( VFW_E_PROCESSOR_NOT_SUITABLE, "ActiveMovie cannot play MPEG movies on this processor." ),
    INFO( VFW_E_UNSUPPORTED_AUDIO, "Cannot play back the audio stream: the audio format is not supported." ),
    INFO( VFW_E_UNSUPPORTED_VIDEO, "Cannot play back the video stream: the video format is not supported." ),
    INFO( VFW_E_MPEG_NOT_CONSTRAINED, "ActiveMovie cannot play this video stream because it falls outside the constrained standard." ),
    INFO( VFW_E_NOT_IN_GRAPH, "Cannot perform the requested function on an object that is not in the filter graph." ),
    INFO( VFW_E_NO_TIME_FORMAT, "Cannot get or set time related information on an object that is using a time format of TIME_FORMAT_NONE." ),
    INFO( VFW_E_READ_ONLY, "The connection cannot be made because the stream is read only and the filter alters the data." ),
    INFO( VFW_E_BUFFER_UNDERFLOW, "The buffer is not full enough." ),
    INFO( VFW_E_UNSUPPORTED_STREAM, "Cannot play back the file.  The format is not supported." ),
    INFO( VFW_E_NO_TRANSPORT, "Pins cannot connect due to not supporting the same transport." ),
    INFO( VFW_E_BAD_VIDEOCD, "The Video CD can't be read correctly by the device or is the data is corrupt." ),
    INFO( VFW_E_OUT_OF_VIDEO_MEMORY, "There is not enough Video Memory at this display resolution and number of colors. Reducing resolution might help." ),
    INFO( VFW_E_VP_NEGOTIATION_FAILED, "The VideoPort connection negotiation process has failed." ),
    INFO( VFW_E_DDRAW_CAPS_NOT_SUITABLE, "Either DirectDraw has not been installed or the Video Card capabilities are not suitable. Make sure the display is not in 16 color mode." ),
    INFO( VFW_E_NO_VP_HARDWARE, "No VideoPort hardware is available, or the hardware is not responding." ),
    INFO( VFW_E_NO_CAPTURE_HARDWARE, "No Capture hardware is available, or the hardware is not responding." ),
    INFO( VFW_E_DVD_OPERATION_INHIBITED, "This User Operation is inhibited by DVD Content at this time." ),
    INFO( VFW_E_DVD_INVALIDDOMAIN, "This Operation is not permitted in the current domain." ),
    INFO( VFW_E_DVD_NO_BUTTON, "The specified button is invalid or is not present at the current time, or there is no button present at the specified location." ),
    INFO( VFW_E_DVD_GRAPHNOTREADY, "DVD-Video playback graph has not been built yet." ),
    INFO( VFW_E_DVD_RENDERFAIL, "DVD-Video playback graph building failed." ),
    INFO( VFW_E_DVD_DECNOTENOUGH, "DVD-Video playback graph could not be built due to insufficient decoders." ),
    INFO( VFW_E_DDRAW_VERSION_NOT_SUITABLE, "Version number of DirectDraw not suitable. Make sure to install dx5 or higher version." ),
    INFO( VFW_E_COPYPROT_FAILED, "Copy protection cannot be enabled. Please make sure any other copy protected content is not being shown now." ),
    INFO( VFW_E_TIME_EXPIRED, "This object cannot be used anymore as its time has expired." ),
    INFO( VFW_E_DVD_WRONG_SPEED, "The operation cannot be performed at the current playback speed." ),
    INFO( VFW_E_DVD_MENU_DOES_NOT_EXIST, "The specified menu doesn't exist." ),
    INFO( VFW_E_DVD_CMD_CANCELLED, "The specified command was either cancelled or no longer exists." ),
    INFO( VFW_E_DVD_STATE_WRONG_VERSION, "The data did not contain a recognized version." ),
    INFO( VFW_E_DVD_STATE_CORRUPT, "The state data was corrupt." ),
    INFO( VFW_E_DVD_STATE_WRONG_DISC, "The state data is from a different disc." ),
    INFO( VFW_E_DVD_INCOMPATIBLE_REGION, "The region was not compatible with the current drive." ),
    INFO( VFW_E_DVD_NO_ATTRIBUTES, "The requested DVD stream attribute does not exist." ),
    INFO( VFW_E_DVD_NO_GOUP_PGC, "Currently there is no GoUp (Annex J user function) program chain (PGC)." ),
    INFO( VFW_E_DVD_LOW_PARENTAL_LEVEL, "The current parental level was too low." ),
    INFO( VFW_E_DVD_NOT_IN_KARAOKE_MODE, "The current audio is not karaoke content." ),
    INFO( VFW_E_FRAME_STEP_UNSUPPORTED, "Frame step is not supported on this configuration." ),
    INFO( VFW_E_DVD_STREAM_DISABLED, "The specified stream is disabled and cannot be selected." ),
    INFO( VFW_E_DVD_TITLE_UNKNOWN, "The operation depends on the current title number, however the navigator has not yet entered the VTSM or the title domains, so the 'current' title index is unknown." ),
    INFO( VFW_E_DVD_INVALID_DISC, "The specified path does not point to a valid DVD disc." ),
    INFO( VFW_E_DVD_NO_RESUME_INFORMATION, "There is currently no resume information." ),
    INFO( VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD, "This thread has already blocked this output pin.  There is no need to call IPinFlowControl::Block() again." ),
    INFO( VFW_E_PIN_ALREADY_BLOCKED, "IPinFlowControl::Block() has been called on another thread.  The current thread cannot make any assumptions about this pin's block state." ),
    INFO( VFW_E_CERTIFICATION_FAILURE, "An operation failed due to a certification failure." ),
    INFO( VFW_E_BAD_KEY, "A registry entry is corrupt." ),
    INFO( MS_E_NOSTREAM, "No stream can be found with the specified attributes." ),
    INFO( MS_E_NOSEEKING, "Seeking not supported for this object." ),
    INFO( MS_E_INCOMPATIBLE, "The stream formats are not compatible." ),
    INFO( MS_E_BUSY, "The sample is busy." ),
    INFO( MS_E_NOTINIT, "The object can't accept the call because its initialize function or equivalent has not been called." ),
    INFO( MS_E_SOURCEALREADYDEFINED, "MS_E_SOURCEALREADYDEFINED" ),
    INFO( MS_E_INVALIDSTREAMTYPE, "The stream type is not valid for this operation." ),
    INFO( MS_E_NOTRUNNING, "The object is not in running state." ),
    INFO( ERROR_FILE_NOT_FOUND, "The system cannot find the file specified." ),
    INFO( ERROR_PATH_NOT_FOUND, "The system cannot find the path specified." ),
    INFO( ERROR_TOO_MANY_OPEN_FILES, "The system cannot open the file." ),
    INFO( E_ACCESSDENIED, "Access is denied" ),
    INFO( E_HANDLE, "Invalid handle" ),
    INFO( ERROR_NOT_ENOUGH_MEMORY, "Not enough storage is available to process this command." ),
    INFO( ERROR_INVALID_BLOCK, "The storage control block address is invalid." ),
    INFO( ERROR_BAD_ENVIRONMENT, "The environment is incorrect." ),
    INFO( ERROR_BAD_FORMAT, "An attempt was made to load a program with an incorrect format." ),
    INFO( ERROR_INVALID_ACCESS, "& DIERR_NOTACQUIRED The operation cannot be performed unless the device is acquired." ),
    INFO( E_OUTOFMEMORY, "Ran out of memory" ),
    INFO( ERROR_NOT_READY, "& DIERR_NOTINITIALIZED   This object has not been initialized" ),
    INFO( ERROR_READ_FAULT, "& DIERR_INPUTLOST       Access to the device has been lost.  It must be re-acquired." ),
    INFO( E_INVALIDARG, "An invalid parameter was passed to the returning function" ),
    INFO( ERROR_BAD_DRIVER_LEVEL, "& DIERR_BADDRIVERVER The object could not be created due to an incompatible driver version or mismatched or incomplete driver components." ),
    INFO( ERROR_BUSY, "& DIERR_ACQUIRED              The operation cannot be performed while the device is acquired." ),
    INFO( ERROR_OLD_WIN_VERSION, "& DIERR_OLDDIRECTINPUTVERSION The application requires a newer version of DirectInput." ),
    INFO( ERROR_RMODE_APP, "& DIERR_BETADIRECTINPUTVERSION The application was written for an unsupported prerelease version of DirectInput." ),
    INFO( ERROR_NOT_FOUND, "& E_PROP_ID_UNSUPPORTED  The specified property ID is not supported for the specified property set." ),
    INFO( ERROR_SET_NOT_FOUND, "& E_PROP_SET_UNSUPPORTED The specified property set is not supported." ),
    INFO( ERROR_ALREADY_INITIALIZED, "& DIERR_ALREADYINITIALIZED This object is already initialized" ),
    INFO( DPNERR_ABORTED, "Aborted" ),
    INFO( DPNERR_ADDRESSING, "Addressing" ),
    INFO( DPNERR_ALREADYCLOSING, "Already closing" ),
    INFO( DPNERR_ALREADYCONNECTED, "Already connected" ),
    INFO( DPNERR_ALREADYDISCONNECTING, "Already disconnecting" ),
    INFO( DPNERR_ALREADYINITIALIZED, "Already initialized" ),
    INFO( DPNERR_ALREADYREGISTERED, "Already registered" ),
    INFO( DPNERR_BUFFERTOOSMALL, "Buffer too small" ),
    INFO( DPNERR_CANNOTCANCEL, "Cannot cancel" ),
    INFO( DPNERR_CANTCREATEGROUP, "Cannot create group" ),
    INFO( DPNERR_CANTCREATEPLAYER, "Cannot create player" ),
    INFO( DPNERR_CANTLAUNCHAPPLICATION, "Cannot launch application" ),
    INFO( DPNERR_CONNECTING, "Connecting" ),
    INFO( DPNERR_CONNECTIONLOST, "Connection lost" ),
    INFO( DPNERR_CONVERSION, "Conversion" ),
    INFO( DPNERR_DATATOOLARGE, "Data too large" ),
    INFO( DPNERR_DOESNOTEXIST, "Does not exist" ),
    INFO( DPNERR_DPNSVRNOTAVAILABLE, "dpnsvr not available" ),
    INFO( DPNERR_DUPLICATECOMMAND, "Duplicate command" ),
    INFO( DPNERR_ENDPOINTNOTRECEIVING, "End point not receiving" ),
    INFO( DPNERR_ENUMQUERYTOOLARGE, "Enum query too large" ),
    INFO( DPNERR_ENUMRESPONSETOOLARGE, "Enum response too large" ),
    INFO( DPNERR_EXCEPTION, "Exception" ),
    INFO( DPNERR_GROUPNOTEMPTY, "Group not empty" ),
    INFO( DPNERR_HOSTING, "Hosting" ),
    INFO( DPNERR_HOSTREJECTEDCONNECTION, "Host rejected connection" ),
    INFO( DPNERR_HOSTTERMINATEDSESSION, "Host terminated session" ),
    INFO( DPNERR_INCOMPLETEADDRESS, "Incomplete address" ),
    INFO( DPNERR_INVALIDADDRESSFORMAT, "Invalid address format" ),
    INFO( DPNERR_INVALIDAPPLICATION, "Invalid application" ),
    INFO( DPNERR_INVALIDCOMMAND, "Invalid command" ),
    INFO( DPNERR_INVALIDDEVICEADDRESS, "Invalid device address" ),
    INFO( DPNERR_INVALIDENDPOINT, "Invalid end point" ),
    INFO( DPNERR_INVALIDFLAGS, "Invalid flags" ),
    INFO( DPNERR_INVALIDGROUP, "Invalid group" ),
    INFO( DPNERR_INVALIDHANDLE, "Invalid handle" ),
    INFO( DPNERR_INVALIDHOSTADDRESS, "Invalid host address" ),
    INFO( DPNERR_INVALIDINSTANCE, "Invalid instance" ),
    INFO( DPNERR_INVALIDINTERFACE, "Invalid interface" ),
    INFO( DPNERR_INVALIDOBJECT, "Invalid object" ),
    INFO( DPNERR_INVALIDPASSWORD, "Invalid password" ),
    INFO( DPNERR_INVALIDPLAYER, "Invalid player" ),
    INFO( DPNERR_INVALIDPRIORITY, "Invalid priority" ),
    INFO( DPNERR_INVALIDSTRING, "Invalid string" ),
    INFO( DPNERR_INVALIDURL, "Invalid url" ),
    INFO( DPNERR_INVALIDVERSION, "Invalid version" ),
    INFO( DPNERR_NOCAPS, "No caps" ),
    INFO( DPNERR_NOCONNECTION, "No connection" ),
    INFO( DPNERR_NOHOSTPLAYER, "No host player" ),
    INFO( DPNERR_NOMOREADDRESSCOMPONENTS, "No more address components" ),
    INFO( DPNERR_NORESPONSE, "No response" ),
    INFO( DPNERR_NOTALLOWED, "Not allowed" ),
    INFO( DPNERR_NOTHOST, "Not host" ),
    INFO( DPNERR_NOTREADY, "Not ready" ),
    INFO( DPNERR_NOTREGISTERED, "Not registered" ),
    INFO( DPNERR_PLAYERALREADYINGROUP, "Player already in group" ),
    INFO( DPNERR_PLAYERLOST, "Player lost" ),
    INFO( DPNERR_PLAYERNOTINGROUP, "Player not in group" ),
    INFO( DPNERR_PLAYERNOTREACHABLE, "Player not reachable" ),
    INFO( DPNERR_SENDTOOLARGE, "Send too large" ),
    INFO( DPNERR_SESSIONFULL, "Session full" ),
    INFO( DPNERR_TABLEFULL, "Table full" ),
    INFO( DPNERR_TIMEDOUT, "Timed out" ),
    INFO( DPNERR_UNINITIALIZED, "Uninitialized" ),
    INFO( DPNERR_USERCANCEL, "User cancel" ),
    INFO( DDERR_ALREADYINITIALIZED, "This object is already initialized" ),
    INFO( DDERR_CANNOTATTACHSURFACE, "This surface cannot be attached to the requested surface." ),
    INFO( DDERR_CANNOTDETACHSURFACE, "This surface cannot be detached from the requested surface." ),
    INFO( DDERR_CURRENTLYNOTAVAIL, "Support is currently not available." ),
    INFO( DDERR_EXCEPTION, "An exception was encountered while performing the requested operation" ),
    INFO( DDERR_HEIGHTALIGN, "Height of rectangle provided is not a multiple of reqd alignment" ),
    INFO( DDERR_INCOMPATIBLEPRIMARY, "Unable to match primary surface creation request with existing primary surface." ),
    INFO( DDERR_INVALIDCAPS, "One or more of the caps bits passed to the callback are incorrect." ),
    INFO( DDERR_INVALIDCLIPLIST, "DirectDraw does not support provided Cliplist." ),
    INFO( DDERR_INVALIDMODE, "DirectDraw does not support the requested mode" ),
    INFO( DDERR_INVALIDOBJECT, "DirectDraw received a pointer that was an invalid DIRECTDRAW object." ),
    INFO( DDERR_INVALIDPIXELFORMAT, "pixel format was invalid as specified" ),
    INFO( DDERR_INVALIDRECT, "Rectangle provided was invalid." ),
    INFO( DDERR_LOCKEDSURFACES, "Operation could not be carried out because one or more surfaces are locked" ),
    INFO( DDERR_NO3D, "There is no 3D present." ),
    INFO( DDERR_NOALPHAHW, "Operation could not be carried out because there is no alpha acceleration hardware present or available." ),
    INFO( DDERR_NOSTEREOHARDWARE, "Operation could not be carried out because there is no stereo hardware present or available." ),
    INFO( DDERR_NOSURFACELEFT, "Operation could not be carried out because there is no hardware present which supports stereo surfaces" ),
    INFO( DDERR_NOCLIPLIST, "no clip list available" ),
    INFO( DDERR_NOCOLORCONVHW, "Operation could not be carried out because there is no color conversion hardware present or available." ),
    INFO( DDERR_NOCOOPERATIVELEVELSET, "Create function called without DirectDraw object method SetCooperativeLevel being called." ),
    INFO( DDERR_NOCOLORKEY, "Surface doesn't currently have a color key" ),
    INFO( DDERR_NOCOLORKEYHW, "Operation could not be carried out because there is no hardware support of the dest color key." ),
    INFO( DDERR_NODIRECTDRAWSUPPORT, "No DirectDraw support possible with current display driver" ),
    INFO( DDERR_NOEXCLUSIVEMODE, "Operation requires the application to have exclusive mode but the application does not have exclusive mode." ),
    INFO( DDERR_NOFLIPHW, "Flipping visible surfaces is not supported." ),
    INFO( DDERR_NOGDI, "There is no GDI present." ),
    INFO( DDERR_NOMIRRORHW, "Operation could not be carried out because there is no hardware present or available." ),
    INFO( DDERR_NOTFOUND, "Requested item was not found" ),
    INFO( DDERR_NOOVERLAYHW, "Operation could not be carried out because there is no overlay hardware present or available." ),
    INFO( DDERR_OVERLAPPINGRECTS, "Operation could not be carried out because the source and destination rectangles are on the same surface and overlap each other." ),
    INFO( DDERR_NORASTEROPHW, "Operation could not be carried out because there is no appropriate raster op hardware present or available." ),
    INFO( DDERR_NOROTATIONHW, "Operation could not be carried out because there is no rotation hardware present or available." ),
    INFO( DDERR_NOSTRETCHHW, "Operation could not be carried out because there is no hardware support for stretching" ),
    INFO( DDERR_NOT4BITCOLOR, "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette." ),
    INFO( DDERR_NOT4BITCOLORINDEX, "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette." ),
    INFO( DDERR_NOT8BITCOLOR, "DirectDraw Surface is not in 8 bit color mode and the requested operation requires 8 bit color." ),
    INFO( DDERR_NOTEXTUREHW, "Operation could not be carried out because there is no texture mapping hardware present or available." ),
    INFO( DDERR_NOVSYNCHW, "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations." ),
    INFO( DDERR_NOZBUFFERHW, "Operation could not be carried out because there is no hardware support for zbuffer blitting." ),
    INFO( DDERR_NOZOVERLAYHW, "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays." ),
    INFO( DDERR_OUTOFCAPS, "The hardware needed for the requested operation has already been allocated." ),
    INFO( D3DERR_OUTOFVIDEOMEMORY, "Out of video memory" ),
    INFO( DDERR_OVERLAYCANTCLIP, "hardware does not support clipped overlays" ),
    INFO( DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "Can only have one color key active at one time for overlays" ),
    INFO( DDERR_PALETTEBUSY, "Access to this palette is being refused because the palette is already locked by another thread." ),
    INFO( DDERR_COLORKEYNOTSET, "No src color key specified for this operation." ),
    INFO( DDERR_SURFACEALREADYATTACHED, "This surface is already attached to the surface it is being attached to." ),
    INFO( DDERR_SURFACEALREADYDEPENDENT, "This surface is already a dependency of the surface it is being made a dependency of." ),
    INFO( DDERR_SURFACEBUSY, "Access to this surface is being refused because the surface is already locked by another thread." ),
    INFO( DDERR_CANTLOCKSURFACE, "Access to this surface is being refused because no driver exists which can supply a pointer to the surface. This is most likely to happen when attempting to lock the primary surface when no DCI provider is present. Will also happen on attempts to lock an optimized surface." ),
    INFO( DDERR_SURFACEISOBSCURED, "Access to Surface refused because Surface is obscured." ),
    INFO( DDERR_SURFACELOST, "Access to this surface is being refused because the surface is gone. The DIRECTDRAWSURFACE object representing this surface should have Restore called on it." ),
    INFO( DDERR_SURFACENOTATTACHED, "The requested surface is not attached." ),
    INFO( DDERR_TOOBIGHEIGHT, "Height requested by DirectDraw is too large." ),
    INFO( DDERR_TOOBIGSIZE, "Size requested by DirectDraw is too large --  The individual height and width are OK." ),
    INFO( DDERR_TOOBIGWIDTH, "Width requested by DirectDraw is too large." ),
    INFO( DDERR_UNSUPPORTEDFORMAT, "Pixel format requested is unsupported by DirectDraw" ),
    INFO( DDERR_UNSUPPORTEDMASK, "Bitmask in the pixel format requested is unsupported by DirectDraw" ),
    INFO( DDERR_INVALIDSTREAM, "The specified stream contains invalid data" ),
    INFO( DDERR_VERTICALBLANKINPROGRESS, "vertical blank is in progress" ),
    INFO( DDERR_WASSTILLDRAWING, "Was still drawing" ),
    INFO( DDERR_DDSCAPSCOMPLEXREQUIRED, "The specified surface type requires specification of the COMPLEX flag" ),
    INFO( DDERR_XALIGN, "Rectangle provided was not horizontally aligned on reqd. boundary" ),
    INFO( DDERR_INVALIDDIRECTDRAWGUID, "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier." ),
    INFO( DDERR_DIRECTDRAWALREADYCREATED, "A DirectDraw object representing this driver has already been created for this process." ),
    INFO( DDERR_NODIRECTDRAWHW, "A hardware only DirectDraw object creation was attempted but the driver did not support any hardware." ),
    INFO( DDERR_PRIMARYSURFACEALREADYEXISTS, "this process already has created a primary surface" ),
    INFO( DDERR_NOEMULATION, "software emulation not available." ),
    INFO( DDERR_REGIONTOOSMALL, "region passed to Clipper::GetClipList is too small." ),
    INFO( DDERR_CLIPPERISUSINGHWND, "an attempt was made to set a clip list for a clipper object that is already monitoring an hwnd." ),
    INFO( DDERR_NOCLIPPERATTACHED, "No clipper object attached to surface object" ),
    INFO( DDERR_NOHWND, "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND." ),
    INFO( DDERR_HWNDSUBCLASSED, "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state." ),
    INFO( DDERR_HWNDALREADYSET, "The CooperativeLevel HWND has already been set. It cannot be reset while the process has surfaces or palettes created." ),
    INFO( DDERR_NOPALETTEATTACHED, "No palette object attached to this surface." ),
    INFO( DDERR_NOPALETTEHW, "No hardware support for 16 or 256 color palettes." ),
    INFO( DDERR_BLTFASTCANTCLIP, "If a clipper object is attached to the source surface passed into a BltFast call." ),
    INFO( DDERR_NOBLTHW, "No blter." ),
    INFO( DDERR_NODDROPSHW, "No DirectDraw ROP hardware." ),
    INFO( DDERR_OVERLAYNOTVISIBLE, "returned when GetOverlayPosition is called on a hidden overlay" ),
    INFO( DDERR_NOOVERLAYDEST, "returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination." ),
    INFO( DDERR_INVALIDPOSITION, "returned when the position of the overlay on the destination is no longer legal for that destination." ),
    INFO( DDERR_NOTAOVERLAYSURFACE, "returned when an overlay member is called for a non-overlay surface" ),
    INFO( DDERR_EXCLUSIVEMODEALREADYSET, "An attempt was made to set the cooperative level when it was already set to exclusive." ),
    INFO( DDERR_NOTFLIPPABLE, "An attempt has been made to flip a surface that is not flippable." ),
    INFO( DDERR_CANTDUPLICATE, "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created." ),
    INFO( DDERR_NOTLOCKED, "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted." ),
    INFO( DDERR_CANTCREATEDC, "Windows cannot create any more DCs, or a DC was requested for a palette-indexed surface when the surface had no palette AND the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC)" ),
    INFO( DDERR_NODC, "No DC was ever created for this surface." ),
    INFO( DDERR_WRONGMODE, "This surface cannot be restored because it was created in a different mode." ),
    INFO( DDERR_IMPLICITLYCREATED, "This surface cannot be restored because it is an implicitly created surface." ),
    INFO( DDERR_NOTPALETTIZED, "The surface being used is not a palette-based surface" ),
    INFO( DDERR_UNSUPPORTEDMODE, "The display is currently in an unsupported mode" ),
    INFO( DDERR_NOMIPMAPHW, "Operation could not be carried out because there is no mip-map texture mapping hardware present or available." ),
    INFO( DDERR_INVALIDSURFACETYPE, "The requested action could not be performed because the surface was of the wrong type." ),
    INFO( DDERR_NOOPTIMIZEHW, "Device does not support optimized surfaces, therefore no video memory optimized surfaces" ),
    INFO( DDERR_NOTLOADED, "Surface is an optimized surface, but has not yet been allocated any memory" ),
    INFO( DDERR_NOFOCUSWINDOW, "Attempt was made to create or set a device window without first setting the focus window" ),
    INFO( DDERR_NOTONMIPMAPSUBLEVEL, "Attempt was made to set a palette on a mipmap sublevel" ),
    INFO( DDERR_DCALREADYCREATED, "A DC has already been returned for this surface. Only one DC can be retrieved per surface." ),
    INFO( DDERR_NONONLOCALVIDMEM, "An attempt was made to allocate non-local video memory from a device that does not support non-local video memory." ),
    INFO( DDERR_CANTPAGELOCK, "The attempt to page lock a surface failed." ),
    INFO( DDERR_CANTPAGEUNLOCK, "The attempt to page unlock a surface failed." ),
    INFO( DDERR_NOTPAGELOCKED, "An attempt was made to page unlock a surface with no outstanding page locks." ),
    INFO( DDERR_MOREDATA, "There is more data available than the specified buffer size could hold" ),
    INFO( DDERR_EXPIRED, "The data has expired and is therefore no longer valid." ),
    INFO( DDERR_TESTFINISHED, "The mode test has finished executing." ),
    INFO( DDERR_NEWMODE, "The mode test has switched to a new mode." ),
    INFO( DDERR_D3DNOTINITIALIZED, "D3D has not yet been initialized." ),
    INFO( DDERR_VIDEONOTACTIVE, "The video port is not active" ),
    INFO( DDERR_NOMONITORINFORMATION, "The monitor does not have EDID data." ),
    INFO( DDERR_NODRIVERSUPPORT, "The driver does not enumerate display mode refresh rates." ),
    INFO( DDERR_DEVICEDOESNTOWNSURFACE, "Surfaces created by one direct draw device cannot be used directly by another direct draw device." ),
    INFO( DXFILEERR_BADOBJECT, "Bad object" ),
    INFO( DXFILEERR_BADVALUE, "Bad value" ),
    INFO( DXFILEERR_BADTYPE, "Bad type" ),
    INFO( DXFILEERR_BADSTREAMHANDLE, "Bad stream handle" ),
    INFO( DXFILEERR_BADALLOC, "Bad alloc" ),
    INFO( DXFILEERR_NOTFOUND, "Not found" ),
    INFO( DXFILEERR_NOTDONEYET, "Not done yet" ),
    INFO( DXFILEERR_FILENOTFOUND, "File not found" ),
    INFO( DXFILEERR_RESOURCENOTFOUND, "Resource not found" ),
    INFO( DXFILEERR_URLNOTFOUND, "Url not found" ),
    INFO( DXFILEERR_BADRESOURCE, "Bad resource" ),
    INFO( DXFILEERR_BADFILETYPE, "Bad file type" ),
    INFO( DXFILEERR_BADFILEVERSION, "Bad file version" ),
    INFO( DXFILEERR_BADFILEFLOATSIZE, "Bad file float size" ),
    INFO( DXFILEERR_BADFILECOMPRESSIONTYPE, "Bad file compression type" ),
    INFO( DXFILEERR_BADFILE, "Bad file" ),
    INFO( DXFILEERR_PARSEERROR, "Parse error" ),
    INFO( DXFILEERR_NOTEMPLATE, "No template" ),
    INFO( DXFILEERR_BADARRAYSIZE, "Bad array size" ),
    INFO( DXFILEERR_BADDATAREFERENCE, "Bad data reference" ),
    INFO( DXFILEERR_INTERNALERROR, "Internal error" ),
    INFO( DXFILEERR_NOMOREOBJECTS, "No more objects" ),
    INFO( DXFILEERR_BADINTRINSICS, "Bad intrinsics" ),
    INFO( DXFILEERR_NOMORESTREAMHANDLES, "No more stream handles" ),
    INFO( DXFILEERR_NOMOREDATA, "No more data" ),
    INFO( DXFILEERR_BADCACHEFILE, "Bad cache file" ),
    INFO( DXFILEERR_NOINTERNET, "No internet" ),
    INFO( D3DERR_WRONGTEXTUREFORMAT, "Wrong texture format" ),
    INFO( D3DERR_UNSUPPORTEDCOLOROPERATION, "Unsupported color operation" ),
    INFO( D3DERR_UNSUPPORTEDCOLORARG, "Unsupported color arg" ),
    INFO( D3DERR_UNSUPPORTEDALPHAOPERATION, "Unsupported alpha operation" ),
    INFO( D3DERR_UNSUPPORTEDALPHAARG, "Unsupported alpha arg" ),
    INFO( D3DERR_TOOMANYOPERATIONS, "Too many operations" ),
    INFO( D3DERR_CONFLICTINGTEXTUREFILTER, "Conflicting texture filter" ),
    INFO( D3DERR_UNSUPPORTEDFACTORVALUE, "Unsupported factor value" ),
    INFO( D3DERR_CONFLICTINGRENDERSTATE, "Conflicting render state" ),
    INFO( D3DERR_UNSUPPORTEDTEXTUREFILTER, "Unsupported texture filter" ),
    INFO( D3DERR_CONFLICTINGTEXTUREPALETTE, "Conflicting texture palette" ),
    INFO( D3DERR_DRIVERINTERNALERROR, "Driver internal error" ),
    INFO( D3DERR_NOTFOUND, "Not found" ),
    INFO( D3DERR_MOREDATA, "More data" ),
    INFO( D3DERR_DEVICELOST, "Device lost" ),
    INFO( D3DERR_DEVICENOTRESET, "Device not reset" ),
    INFO( D3DERR_NOTAVAILABLE, "Not available" ),
    INFO( D3DERR_INVALIDDEVICE, "Invalid device" ),
    INFO( D3DERR_INVALIDCALL, "Invalid call" ),
    INFO( D3DERR_DRIVERINVALIDCALL, "Driver invalid call" ),
    INFO( DSERR_ALLOCATED, "The call failed because resources (such as a priority level) were already being used by another caller" ),
    INFO( DSERR_CONTROLUNAVAIL, "The control (vol, pan, etc.) requested by the caller is not available" ),
    INFO( DSERR_INVALIDCALL, "This call is not valid for the current state of this object" ),
    INFO( DSERR_PRIOLEVELNEEDED, "The caller does not have the priority level required for the function to succeed" ),
    INFO( DSERR_BADFORMAT, "The specified WAVE format is not supported" ),
    INFO( DSERR_NODRIVER, "No sound driver is available for use" ),
    INFO( DSERR_ALREADYINITIALIZED, "This object is already initialized" ),
    INFO( DSERR_BUFFERLOST, "The buffer memory has been lost, and must be restored" ),
    INFO( DSERR_OTHERAPPHASPRIO, "Another app has a higher priority level, preventing this call from succeeding" ),
    INFO( DSERR_UNINITIALIZED, "This object has not been initialized" ),
    INFO( DSERR_BUFFERTOOSMALL, "Tried to create a DSBCAPS_CTRLFX buffer shorter than DSBSIZE_FX_MIN milliseconds" ),
    INFO( DSERR_DS8_REQUIRED, "Attempt to use DirectSound 8 functionality on an older DirectSound object" ),
    INFO( DSERR_SENDLOOP, "A circular loop of send effects was detected" ),
    INFO( DSERR_BADSENDBUFFERGUID, "The GUID specified in an audiopath file does not match a valid MIXIN buffer" ),
    INFO( DMUS_E_DRIVER_FAILED, "An unexpected error was returned from a device driver, indicating possible failure of the driver or hardware." ),
    INFO( DMUS_E_PORTS_OPEN, "The requested operation cannot be performed while there are  instantiated ports in any process in the system." ),
    INFO( DMUS_E_DEVICE_IN_USE, "The requested device is already in use (possibly by a non-DirectMusic client) and cannot be opened again." ),
    INFO( DMUS_E_INSUFFICIENTBUFFER, "Buffer is not large enough for requested operation." ),
    INFO( DMUS_E_BUFFERNOTSET, "No buffer was prepared for the download data." ),
    INFO( DMUS_E_BUFFERNOTAVAILABLE, "Download failed due to inability to access or create download buffer." ),
    INFO( DMUS_E_NOTADLSCOL, "Error parsing DLS collection. File is corrupt." ),
    INFO( DMUS_E_INVALIDOFFSET, "Wave chunks in DLS collection file are at incorrect offsets." ),
    INFO( DMUS_E_ALREADY_LOADED, "Second attempt to load a DLS collection that is currently open. " ),
    INFO( DMUS_E_INVALIDPOS, "Error reading wave data from DLS collection. Indicates bad file." ),
    INFO( DMUS_E_INVALIDPATCH, "There is no instrument in the collection that matches patch number." ),
    INFO( DMUS_E_CANNOTSEEK, "The IStream* doesn't support Seek()." ),
    INFO( DMUS_E_CANNOTWRITE, "The IStream* doesn't support Write()." ),
    INFO( DMUS_E_CHUNKNOTFOUND, "The RIFF parser doesn't contain a required chunk while parsing file." ),
    INFO( DMUS_E_INVALID_DOWNLOADID, "Invalid download id was used in the process of creating a download buffer." ),
    INFO( DMUS_E_NOT_DOWNLOADED_TO_PORT, "Tried to unload an object that was not downloaded or previously unloaded." ),
    INFO( DMUS_E_ALREADY_DOWNLOADED, "Buffer was already downloaded to synth." ),
    INFO( DMUS_E_UNKNOWN_PROPERTY, "The specified property item was not recognized by the target object." ),
    INFO( DMUS_E_SET_UNSUPPORTED, "The specified property item may not be set on the target object." ),
    INFO( DMUS_E_GET_UNSUPPORTED, "* The specified property item may not be retrieved from the target object." ),
    INFO( DMUS_E_NOTMONO, "Wave chunk has more than one interleaved channel. DLS format requires MONO." ),
    INFO( DMUS_E_BADARTICULATION, "Invalid articulation chunk in DLS collection." ),
    INFO( DMUS_E_BADINSTRUMENT, "Invalid instrument chunk in DLS collection." ),
    INFO( DMUS_E_BADWAVELINK, "Wavelink chunk in DLS collection points to invalid wave." ),
    INFO( DMUS_E_NOARTICULATION, "Articulation missing from instrument in DLS collection." ),
    INFO( DMUS_E_NOTPCM, "Downoaded DLS wave is not in PCM format. " ),
    INFO( DMUS_E_BADWAVE, "Bad wave chunk in DLS collection" ),
    INFO( DMUS_E_BADOFFSETTABLE, "Offset Table for download buffer has errors. " ),
    INFO( DMUS_E_UNKNOWNDOWNLOAD, "Attempted to download unknown data type." ),
    INFO( DMUS_E_NOSYNTHSINK, "The operation could not be completed because no sink was connected to the synthesizer." ),
    INFO( DMUS_E_ALREADYOPEN, "An attempt was made to open the software synthesizer while it was already  open." ),
    INFO( DMUS_E_ALREADYCLOSED, "An attempt was made to close the software synthesizer while it was already  open." ),
    INFO( DMUS_E_SYNTHNOTCONFIGURED, "The operation could not be completed because the software synth has not  yet been fully configured." ),
    INFO( DMUS_E_SYNTHACTIVE, "The operation cannot be carried out while the synthesizer is active." ),
    INFO( DMUS_E_CANNOTREAD, "An error occurred while attempting to read from the IStream* object." ),
    INFO( DMUS_E_DMUSIC_RELEASED, "The operation cannot be performed because the final instance of the DirectMusic object was released. Ports cannot be used after final  release of the DirectMusic object." ),
    INFO( DMUS_E_BUFFER_EMPTY, "There was no data in the referenced buffer." ),
    INFO( DMUS_E_BUFFER_FULL, "There is insufficient space to insert the given event into the buffer." ),
    INFO( DMUS_E_PORT_NOT_CAPTURE, "The given operation could not be carried out because the port is a capture port." ),
    INFO( DMUS_E_PORT_NOT_RENDER, "The given operation could not be carried out because the port is a render port." ),
    INFO( DMUS_E_DSOUND_NOT_SET, "The port could not be created because no DirectSound has been specified. Specify a DirectSound interface via the IDirectMusic::SetDirectSound method; pass NULL to have DirectMusic manage usage of DirectSound." ),
    INFO( DMUS_E_ALREADY_ACTIVATED, "The operation cannot be carried out while the port is active." ),
    INFO( DMUS_E_INVALIDBUFFER, "Invalid DirectSound buffer was handed to port. " ),
    INFO( DMUS_E_WAVEFORMATNOTSUPPORTED, "Invalid buffer format was handed to the synth sink." ),
    INFO( DMUS_E_SYNTHINACTIVE, "The operation cannot be carried out while the synthesizer is inactive." ),
    INFO( DMUS_E_DSOUND_ALREADY_SET, "IDirectMusic::SetDirectSound has already been called. It may not be changed while in use." ),
    INFO( DMUS_E_INVALID_EVENT, "The given event is invalid (either it is not a valid MIDI message or it makes use of running status). The event cannot be packed into the buffer." ),
    INFO( DMUS_E_UNSUPPORTED_STREAM, "The IStream* object does not contain data supported by the loading object." ),
    INFO( DMUS_E_ALREADY_INITED, "The object has already been initialized." ),
    INFO( DMUS_E_INVALID_BAND, "The file does not contain a valid band." ),
    INFO( DMUS_E_TRACK_HDR_NOT_FIRST_CK, "The IStream* object's data does not have a track header as the first chunk, and therefore cannot be read by the segment object." ),
    INFO( DMUS_E_TOOL_HDR_NOT_FIRST_CK, "The IStream* object's data does not have a tool header as the first chunk, and therefore cannot be read by the graph object." ),
    INFO( DMUS_E_INVALID_TRACK_HDR, "The IStream* object's data contains an invalid track header (ckid is 0 and fccType is NULL,) and therefore cannot be read by the segment object." ),
    INFO( DMUS_E_INVALID_TOOL_HDR, "The IStream* object's data contains an invalid tool header (ckid is 0 and fccType is NULL,) and therefore cannot be read by the graph object." ),
    INFO( DMUS_E_ALL_TOOLS_FAILED, "The graph object was unable to load all tools from the IStream* object data. This may be due to errors in the stream, or the tools being incorrectly registered on the client." ),
    INFO( DMUS_E_ALL_TRACKS_FAILED, "The segment object was unable to load all tracks from the IStream* object data. This may be due to errors in the stream, or the tracks being incorrectly registered on the client." ),
    INFO( DSERR_OBJECTNOTFOUND, "The object requested was not found (numerically equal to DMUS_E_NOT_FOUND)" ),
    INFO( DMUS_E_NOT_INIT, "A required object is not initialized or failed to initialize." ),
    INFO( DMUS_E_TYPE_DISABLED, "The requested parameter type is currently disabled. Parameter types may be enabled and disabled by certain calls to SetParam()." ),
    INFO( DMUS_E_TYPE_UNSUPPORTED, "The requested parameter type is not supported on the object." ),
    INFO( DMUS_E_TIME_PAST, "The time is in the past, and the operation cannot succeed." ),
    INFO( DMUS_E_TRACK_NOT_FOUND, "The requested track is not contained by the segment." ),
    INFO( DMUS_E_TRACK_NO_CLOCKTIME_SUPPORT, "The track does not support clock time playback or getparam." ),
    INFO( DMUS_E_NO_MASTER_CLOCK, "There is no master clock in the performance. Be sure to call IDirectMusicPerformance::Init()." ),
    INFO( DMUS_E_LOADER_NOCLASSID, "The class id field is required and missing in the DMUS_OBJECTDESC." ),
    INFO( DMUS_E_LOADER_BADPATH, "The requested file path is invalid." ),
    INFO( DMUS_E_LOADER_FAILEDOPEN, "File open failed - either file doesn't exist or is locked." ),
    INFO( DMUS_E_LOADER_FORMATNOTSUPPORTED, "Search data type is not supported." ),
    INFO( DMUS_E_LOADER_FAILEDCREATE, "Unable to find or create object." ),
    INFO( DMUS_E_LOADER_OBJECTNOTFOUND, "Object was not found." ),
    INFO( DMUS_E_LOADER_NOFILENAME, "The file name is missing from the DMUS_OBJECTDESC." ),
    INFO( DMUS_E_INVALIDFILE, "The file requested is not a valid file." ),
    INFO( DMUS_E_ALREADY_EXISTS, "The tool is already contained in the graph. Create a new instance." ),
    INFO( DMUS_E_OUT_OF_RANGE, "Value is out of range, for instance the requested length is longer than the segment." ),
    INFO( DMUS_E_SEGMENT_INIT_FAILED, "Segment initialization failed, most likely due to a critical memory situation." ),
    INFO( DMUS_E_ALREADY_SENT, "The DMUS_PMSG has already been sent to the performance object via IDirectMusicPerformance::SendPMsg()." ),
    INFO( DMUS_E_CANNOT_FREE, "The DMUS_PMSG was either not allocated by the performance via IDirectMusicPerformance::AllocPMsg() or it was already freed via IDirectMusicPerformance::FreePMsg()." ),
    INFO( DMUS_E_CANNOT_OPEN_PORT, "The default system port could not be opened." ),
    INFO( DMUS_E_CANNOT_CONVERT, "A call to MIDIToMusic() or MusicToMIDI() resulted in an error because the requested conversion could not happen. This usually occurs when the provided DMUS_CHORD_KEY structure has an invalid chord or scale pattern." ),
    INFO( DMUS_E_DESCEND_CHUNK_FAIL, "DMUS_E_DESCEND_CHUNK_FAIL is returned when the end of the file  was reached before the desired chunk was found." ),
    INFO( DMUS_E_NOT_LOADED, "An attempt to use this object failed because it first needs to be loaded." ),
    INFO( DMUS_E_SCRIPT_LANGUAGE_INCOMPATIBLE, "The activeX scripting engine for the script's language is not compatible with DirectMusic." ),
    INFO( DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE, "A variant was used that had a type that is not supported by DirectMusic." ),
    INFO( DMUS_E_SCRIPT_ERROR_IN_SCRIPT, "An error was encountered while parsing or executing the script. The pErrorInfo parameter (if supplied) was filled with information about the error." ),
    INFO( DMUS_E_SCRIPT_CANTLOAD_OLEAUT32, "Loading of oleaut32.dll failed.  VBScript and other activeX scripting languages require use of oleaut32.dll.  On platforms where oleaut32.dll is not present, only the DirectMusicScript language, which doesn't require oleaut32.dll can be used." ),
    INFO( DMUS_E_SCRIPT_LOADSCRIPT_ERROR, "An error occurred while parsing a script loaded using LoadScript.  The script that was loaded contains an error." ),
    INFO( DMUS_E_SCRIPT_INVALID_FILE, "The script file is invalid." ),
    INFO( DMUS_E_INVALID_SCRIPTTRACK, "The file contains an invalid script track." ),
    INFO( DMUS_E_SCRIPT_VARIABLE_NOT_FOUND, "The script does not contain a variable with the specified name." ),
    INFO( DMUS_E_SCRIPT_ROUTINE_NOT_FOUND, "The script does not contain a routine with the specified name." ),
    INFO( DMUS_E_SCRIPT_CONTENT_READONLY, "Scripts variables for content referenced or embedded in a script cannot be set." ),
    INFO( DMUS_E_SCRIPT_NOT_A_REFERENCE, "Attempt was made to set a script's variable by reference to a value that was not an object type." ),
    INFO( DMUS_E_SCRIPT_VALUE_NOT_SUPPORTED, "Attempt was made to set a script's variable by value to an object that does not support a default value property." ),
    INFO( DMUS_E_INVALID_SEGMENTTRIGGERTRACK, "The file contains an invalid segment trigger track." ),
    INFO( DMUS_E_INVALID_LYRICSTRACK, "The file contains an invalid lyrics track." ),
    INFO( DMUS_E_INVALID_PARAMCONTROLTRACK, "The file contains an invalid parameter control track." ),
    INFO( DMUS_E_AUDIOVBSCRIPT_SYNTAXERROR, "A script written in AudioVBScript could not be read because it contained a statement that is not allowed by the AudioVBScript language." ),
    INFO( DMUS_E_AUDIOVBSCRIPT_RUNTIMEERROR, "A script routine written in AudioVBScript failed because an invalid operation occurred.  For example, adding the number 3 to a segment object would produce this error.  So would attempting to call a routine that doesn't exist." ),
    INFO( DMUS_E_AUDIOVBSCRIPT_OPERATIONFAILURE, "A script routine written in AudioVBScript failed because a function outside of a script failed to complete. For example, a call to PlaySegment that fails to play because of low memory would return this error." ),
    INFO( DMUS_E_AUDIOPATHS_NOT_VALID, "The Performance has set up some PChannels using the AssignPChannel command, which makes it not capable of supporting audio paths." ),
    INFO( DMUS_E_AUDIOPATHS_IN_USE, "This is the inverse of the previous error. The Performance has set up some audio paths, which makes is incompatible with the calls to allocate pchannels, etc. " ),
    INFO( DMUS_E_NO_AUDIOPATH_CONFIG, "A segment or song was asked for its embedded audio path configuration, but there isn't any. " ),
    INFO( DMUS_E_AUDIOPATH_INACTIVE, "An audiopath is inactive, perhaps because closedown was called." ),
    INFO( DMUS_E_AUDIOPATH_NOBUFFER, "An audiopath failed to create because a requested buffer could not be created." ),
    INFO( DMUS_E_AUDIOPATH_NOPORT, "An audiopath could not be used for playback because it lacked port assignments." ),
    INFO( DMUS_E_NO_AUDIOPATH, "Attempt was made to play segment in audiopath mode and there was no audiopath." ),
    INFO( DMUS_E_INVALIDCHUNK, "Invalid data was found in a RIFF file chunk." ),
    INFO( DMUS_E_AUDIOPATH_NOGLOBALFXBUFFER, "Attempt was made to create an audiopath that sends to a global effects buffer which did not exist." ),
    INFO( DMUS_E_INVALID_CONTAINER_OBJECT, "The file does not contain a valid container object." ),
#undef INFO
};

const char * WINAPI DXGetErrorString9A(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = ARRAY_SIZE(info); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].resultA;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return "Unknown";
}

const WCHAR * WINAPI DXGetErrorString9W(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = ARRAY_SIZE(info); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].resultW;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return L"Unknown";
}

const char * WINAPI DXGetErrorDescription9A(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = ARRAY_SIZE(info); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].descriptionA;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return "n/a";
}

const WCHAR * WINAPI DXGetErrorDescription9W(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = ARRAY_SIZE(info); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].descriptionW;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return L"n/a";
}

HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char*  strMsg, BOOL bPopMsgBox)
{
    char msg[1024];

    if (bPopMsgBox) {
        wsprintfA(msg, "File: %s\nLine: %d\nError Code: %s (0x%08x)\nCalling: %s",
            strFile, dwLine, DXGetErrorString9A(hr), hr, strMsg);
        MessageBoxA(0, msg, "Unexpected error encountered", MB_OK|MB_ICONERROR);
    } else {
        wsprintfA(msg, "%s(%d): %s (hr=%s (0x%08x))", strFile,
            dwLine, strMsg, DXGetErrorString9A(hr), hr);
        OutputDebugStringA(msg);
    }

    return hr;
}

HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox)
{
    WCHAR msg[1024];

    if (bPopMsgBox) {
        wsprintfW(msg, L"File: %s\nLine: %d\nError Code: %s (0x%08x)\nCalling: %s",
                  strFile, dwLine, DXGetErrorString9W(hr), hr, strMsg);
        MessageBoxW(0, msg, L"Unexpected error encountered", MB_OK|MB_ICONERROR);
    } else {
        wsprintfW(msg, L"%s(%d): %s (hr=%s (0x%08x))", strFile,
                  dwLine, strMsg, DXGetErrorString9W(hr), hr);
        OutputDebugStringW(msg);
    }

    return hr;
}
