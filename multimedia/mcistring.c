/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MCI stringinterface
 *
 * Copyright 1995 Marcus Meissner
 */
/* FIXME:
 * 	+ special commands of device drivers should be handled by those drivers
 *      + this current implementation does not allow commands like 
 * 	  "capability <filename> can play" which is allowed by the MCI standard.
 *	+ return value and their conversion to strings (including error codes)
 *	  is not coherent with MCI standard.
 *	+ digital video interface has not been throughoutfully tested
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "multimedia.h"
#include "digitalv.h"
#include "heap.h"
#include "ldt.h"
#include "user.h"
#include "driver.h"
#include "callback.h"
#include "debugtools.h"
#include "xmalloc.h"

DEFAULT_DEBUG_CHANNEL(mci)

/* The reason why I just don't lowercase the keywords array in 
 * mciSendString is left as an exercise to the reader.
 * (EP: For the blinds, like me, it's linked to file name handling 
 * which is case sensitive).
 */
#define STRCMP(x, y) lstrcmpiA(x, y)

/* standard function parameters for all functions */
#define _MCISTR_PROTO_ 				       		\
	WORD wDevID, WORD uDevTyp, LPSTR lpstrReturnString,	\
        UINT16 uReturnLength, LPCSTR dev, LPSTR* keywords,	\
	UINT16 nrofkeywords, DWORD dwFlags, HWND16 hwndCallback

/* copy string to return pointer including necessary checks 
 * for use in mciSendString()
 */
#define _MCI_STR(s) 						\
do {								\
    TRACE("-> returns '%s'\n", s);				\
    if (lpstrReturnString) {					\
	lstrcpynA(lpstrReturnString, s, uReturnLength);		\
	TRACE("--> '%s'\n", lpstrReturnString);		\
    }								\
} while(0)

static LPSTR _MCISTR_Unquote(LPSTR str)
{
    if (str) {
	int 	i, len = strlen(str);
	
	if (len > 1 && str[0] == '"' && str[len-1] == '"') {
	    for (i = 1; i < len-1; i++) {
		str[i - 1] = str[i];
	    }
	    str[len-2] = 0;
	}
    }
    return str;
}

/* print a DWORD in the specified timeformat */
static void
_MCISTR_printtf(char *buf, UINT16 uDevType, DWORD timef, DWORD val) 
{
    *buf = '\0';
    switch (timef) {
    case MCI_FORMAT_MILLISECONDS:
    case MCI_FORMAT_FRAMES:
    case MCI_FORMAT_BYTES:
    case MCI_FORMAT_SAMPLES:
    case MCI_VD_FORMAT_TRACK:
	/*case MCI_SEQ_FORMAT_SONGPTR: sameas MCI_VD_FORMAT_TRACK */
	sprintf(buf, "%ld",val);
	break;
    case MCI_FORMAT_HMS:
	/* well, the macros have the same content*/
	/*FALLTRHOUGH*/
    case MCI_FORMAT_MSF:
	sprintf(buf, "%d:%d:%d", 
		MCI_HMS_HOUR(val),
		MCI_HMS_MINUTE(val),
		MCI_HMS_SECOND(val));
	break;
    case MCI_FORMAT_TMSF:
	sprintf(buf, "%d:%d:%d:%d",
		MCI_TMSF_TRACK(val),
		MCI_TMSF_MINUTE(val),
		MCI_TMSF_SECOND(val),
		MCI_TMSF_FRAME(val));
	break;
    default:
	FIXME("missing timeformat for %ld, report.\n", timef);
	strcpy(buf, "0"); /* hmm */
	break;
    }
    return;
}
/* possible different return types */
#define _MCISTR_int	1
#define _MCISTR_time	2
#define _MCISTR_bool	3
#define _MCISTR_tfname	4
#define _MCISTR_mode	5
#define _MCISTR_divtype	6
#define _MCISTR_seqtype	7
#define _MCISTR_vdmtype	8
#define _MCISTR_devtype	9

static void
_MCISTR_convreturn(int type, DWORD dwReturn, LPSTR lpstrReturnString,
		   WORD uReturnLength, WORD uDevTyp, int timef) 
{
    switch (type) {
    case _MCISTR_vdmtype:
	switch (dwReturn) {
	case MCI_VD_MEDIA_CLV:	_MCI_STR("CLV");	break;
	case MCI_VD_MEDIA_CAV:	_MCI_STR("CAV");	break;
	default:
	case MCI_VD_MEDIA_OTHER:_MCI_STR("other");	break;
	}
	break;
    case _MCISTR_seqtype:
	switch (dwReturn) {
	case MCI_SEQ_NONE:	_MCI_STR("none");	break;
	case MCI_SEQ_SMPTE:	_MCI_STR("smpte");	break;
	case MCI_SEQ_FILE:	_MCI_STR("file");	break;
	case MCI_SEQ_MIDI:	_MCI_STR("midi");	break;
	default:FIXME("missing sequencer mode %ld\n", dwReturn);
	}
	break;
    case _MCISTR_mode:
	switch (dwReturn) {
	case MCI_MODE_NOT_READY:_MCI_STR("not ready");	break;
	case MCI_MODE_STOP:	_MCI_STR("stopped");	break;
	case MCI_MODE_PLAY:	_MCI_STR("playing");	break;
	case MCI_MODE_RECORD:	_MCI_STR("recording");	break;
	case MCI_MODE_SEEK:	_MCI_STR("seeking");	break;
	case MCI_MODE_PAUSE:	_MCI_STR("paused");	break;
	case MCI_MODE_OPEN:	_MCI_STR("open");	break;
	default:break;
	}
	break;
    case _MCISTR_bool:
	if (dwReturn)
	    _MCI_STR("true");
	else
	    _MCI_STR("false");
	break;
    case _MCISTR_int:{
	char	buf[16];
	sprintf(buf, "%ld", dwReturn);
	_MCI_STR(buf);
	break;
    }
    case _MCISTR_time: {	
	char	buf[100];
	_MCISTR_printtf(buf, uDevTyp, timef, dwReturn);
	_MCI_STR(buf);
	break;
    }
    case _MCISTR_tfname:
	switch (timef) {
	case MCI_FORMAT_MILLISECONDS:	_MCI_STR("milliseconds");	break;
	case MCI_FORMAT_FRAMES:		_MCI_STR("frames");		break;
	case MCI_FORMAT_BYTES:		_MCI_STR("bytes");		break;
	case MCI_FORMAT_SAMPLES:	_MCI_STR("samples");		break;
	case MCI_FORMAT_HMS:		_MCI_STR("hms");		break;
	case MCI_FORMAT_MSF:		_MCI_STR("msf");		break;
	case MCI_FORMAT_TMSF:		_MCI_STR("tmsf");		break;
	default:
	    FIXME("missing timefmt for %d, report.\n", timef);
	    break;
	}
	break;
    case _MCISTR_divtype:
	switch (dwReturn) {
	case MCI_SEQ_DIV_PPQN:		_MCI_STR("PPQN");		break;
	case MCI_SEQ_DIV_SMPTE_24:	_MCI_STR("SMPTE 24 frame");	break;
	case MCI_SEQ_DIV_SMPTE_25:	_MCI_STR("SMPTE 25 frame");	break;
	case MCI_SEQ_DIV_SMPTE_30:	_MCI_STR("SMPTE 30 frame");	break;
	case MCI_SEQ_DIV_SMPTE_30DROP:	_MCI_STR("SMPTE 30 frame drop");break;
	}
    case _MCISTR_devtype:
	switch (dwReturn) {
	case MCI_DEVTYPE_VCR:		_MCI_STR("vcr");		break;
	case MCI_DEVTYPE_VIDEODISC:	_MCI_STR("videodisc");		break;
	case MCI_DEVTYPE_CD_AUDIO:	_MCI_STR("cd audio");		break;
	case MCI_DEVTYPE_OVERLAY:	_MCI_STR("overlay");		break;
	case MCI_DEVTYPE_DAT:		_MCI_STR("dat");		break;
	case MCI_DEVTYPE_SCANNER:	_MCI_STR("scanner");		break;
	case MCI_DEVTYPE_ANIMATION:	_MCI_STR("animation");		break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:	_MCI_STR("digital video");	break;
	case MCI_DEVTYPE_OTHER:		_MCI_STR("other");		break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:_MCI_STR("waveform audio");	break;
	case MCI_DEVTYPE_SEQUENCER:	_MCI_STR("sequencer");		break;
	default:FIXME("unknown device type %ld, report.\n",
		      dwReturn);break;
	}
	break;
    default:
	FIXME("unknown resulttype %d, report.\n", type);
	break;
    }
}

#define FLAG1(str, flag) 			\
	if (!STRCMP(keywords[i], str)) {	\
		dwFlags |= flag;		\
		i++;				\
		continue;			\
	}

#define FLAG2(str1, str2, flag) 		\
	if (!STRCMP(keywords[i], str1) &&	\
	    (i+1 < nrofkeywords) && 		\
            !STRCMP(keywords[i+1], str2)) {	\
		dwFlags |= flag;		\
		i += 2;				\
		continue;			\
	}

/* All known subcommands are implemented in single functions to avoid
 * bloat and a xxxx lines long mciSendString(). All commands are of the
 * format MCISTR_Cmd(_MCISTR_PROTO_) where _MCISTR_PROTO_ is the above
 * defined line of arguments. (This is just for easy enhanceability.)
 * All functions return the MCIERR_ errorvalue as DWORD. Returnvalues
 * for the calls are in lpstrReturnString (If I mention return values
 * in function headers, I mean returnvalues in lpstrReturnString.)
 * Integers are sprintf("%d")ed integers. Boolean values are 
 * "true" and "false". 
 * timeformat depending values are "%d" "%d:%d" "%d:%d:%d" "%d:%d:%d:%d"
 * FIXME: is above line correct?
 *
 * Preceding every function is a list of implemented/known arguments.
 * Feel free to add missing arguments.
 *
 */

/*
 * Opens the specified MCI driver. 
 * Arguments: <name> 
 * Optional:
 *	"shareable"
 *	"alias <aliasname>"
 * 	"element <elementname>"
 * Additional:
 * waveform audio:
 *	"buffer <nrBytesPerSec>"
 * Animation:
 *	"nostatic"	increaste nr of nonstatic colours
 *	"parent <windowhandle>"
 *	"style <mask>"	bitmask of WS_xxxxx (see windows.h)
 *	"style child"	WS_CHILD
 *	"style overlap"	WS_OVERLAPPED
 *	"style popup"	WS_POPUP
 * Overlay:
 *	"parent <windowhandle>"
 *	"style <mask>"	bitmask of WS_xxxxx (see windows.h)
 *	"style child"	WS_CHILD
 *	"style overlap"	WS_OVERLAPPED
 *	"style popup"	WS_POPUP
 * Returns nothing.
 */
static DWORD
MCISTR_Open(_MCISTR_PROTO_) 
{
    int			res, i;
    char*		s;
    union U {
	MCI_OPEN_PARMSA		openParams;
	MCI_WAVE_OPEN_PARMSA	waveopenParams;
	MCI_ANIM_OPEN_PARMSA	animopenParams;
	MCI_OVLY_OPEN_PARMSA	ovlyopenParams;
	MCI_DGV_OPEN_PARMSA	dgvopenParams;
    };
    union U*		pU = xmalloc(sizeof(union U));

    pU->openParams.lpstrElementName = NULL;
    s = strchr(dev, '!');
    if (s != NULL) {
	*s++ = '\0';
	pU->openParams.lpstrElementName = strdup(s);
	dwFlags |= MCI_OPEN_ELEMENT;
    }
    for (i = 0; i < nrofkeywords; ) {
	if ((!STRCMP(keywords[i], "type")) && (i < nrofkeywords-1)) {
	    pU->openParams.lpstrElementName = strdup(dev);
	    dwFlags |= MCI_OPEN_ELEMENT;
	    dev = keywords[i+1];
	    /* FIXME: isn't there a memory leak here ? keyword+i ? */
	    memcpy(keywords+i, keywords+(i+2), (nrofkeywords-i-2) * sizeof(char*));
	    nrofkeywords -= 2;
	    i += 2;
	    continue;
	}
	i++;
    }
    CharUpperA((char*)dev);
    uDevTyp = MCI_GetDevTypeFromString(dev);
    if (uDevTyp == 0) {
	free(pU->openParams.lpstrElementName);
	free(pU);
	return MCIERR_INVALID_DEVICE_NAME;
    }

    pU->openParams.dwCallback	= hwndCallback;
    pU->openParams.wDeviceID	= wDevID;
    pU->ovlyopenParams.dwStyle	= 0; 
    pU->animopenParams.dwStyle	= 0; 
    pU->openParams.lpstrDeviceType	= strdup(dev);
    pU->openParams.lpstrAlias	= NULL;
    dwFlags |= MCI_OPEN_TYPE;
    for (i = 0; i < nrofkeywords; ) {
	FLAG1("shareable", MCI_OPEN_SHAREABLE);
	if (!STRCMP(keywords[i], "alias") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_OPEN_ALIAS;
	    pU->openParams.lpstrAlias = strdup(keywords[i+1]);
	    i += 2;
	    continue;
	}
	if (!STRCMP(keywords[i], "element") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_OPEN_ELEMENT;
	    assert(pU->openParams.lpstrElementName == NULL);
	    pU->openParams.lpstrElementName = strdup(keywords[i+1]);
	    i += 2;
	    continue;
	}
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    FLAG1("nostatic", MCI_ANIM_OPEN_NOSTATIC);
	    if (!STRCMP(keywords[i], "parent") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_ANIM_OPEN_PARENT;
		sscanf(keywords[i+1], "%u", &(pU->animopenParams.hWndParent));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "style") && (i+1 < nrofkeywords)) {
		DWORD	st;
		
		dwFlags |= MCI_ANIM_OPEN_WS;
		if (!STRCMP(keywords[i+1], "popup")) {
		    pU->animopenParams.dwStyle |= WS_POPUP; 
		} else if (!STRCMP(keywords[i+1], "overlap")) {
		    pU->animopenParams.dwStyle |= WS_OVERLAPPED; 
		} else if (!STRCMP(keywords[i+1], "child")) {
		    pU->animopenParams.dwStyle |= WS_CHILD; 
		} else if (sscanf(keywords[i+1], "%ld", &st)) {
		    pU->animopenParams.dwStyle |= st; 
		} else
		    FIXME("unknown 'style' keyword %s, please report.\n", keywords[i+1]);
		i += 2;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	    FLAG1("nostatic", MCI_ANIM_OPEN_NOSTATIC);
	    if (!STRCMP(keywords[i], "parent") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_DGV_OPEN_PARENT;
		sscanf(keywords[i+1], "%u", &(pU->dgvopenParams.hWndParent));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "style") && (i+1 < nrofkeywords)) {
		DWORD	st;
		
		dwFlags |= MCI_DGV_OPEN_WS;
		if (!STRCMP(keywords[i+1], "popup")) {
		    pU->dgvopenParams.dwStyle |= WS_POPUP; 
		} else if (!STRCMP(keywords[i+1], "overlap")) {
		    pU->dgvopenParams.dwStyle |= WS_OVERLAPPED; 
		} else if (!STRCMP(keywords[i+1], "child")) {
		    pU->dgvopenParams.dwStyle |= WS_CHILD; 
		} else if (sscanf(keywords[i+1], "%ld", &st)) {
		    pU->dgvopenParams.dwStyle |= st; 
		} else
		    FIXME("unknown 'style' keyword %s, please report.\n", keywords[i+1]);
		i += 2;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:
	    if (!STRCMP(keywords[i], "buffer") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_WAVE_OPEN_BUFFER;
		sscanf(keywords[i+1], "%ld", &(pU->waveopenParams.dwBufferSeconds));
	    }
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    /* looks just like anim, but without NOSTATIC */
	    if (!STRCMP(keywords[i], "parent") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_OVLY_OPEN_PARENT;
		sscanf(keywords[i+1], "%u", &(pU->ovlyopenParams.hWndParent));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "style") && (i+1 < nrofkeywords)) {
		DWORD	st;
		
		dwFlags |= MCI_OVLY_OPEN_WS;
		if (!STRCMP(keywords[i+1], "popup")) {
		    pU->ovlyopenParams.dwStyle |= WS_POPUP; 
		} else if (!STRCMP(keywords[i+1], "overlap")) {
		    pU->ovlyopenParams.dwStyle |= WS_OVERLAPPED; 
		} else if (!STRCMP(keywords[i+1], "child")) {
		    pU->ovlyopenParams.dwStyle |= WS_CHILD; 
		} else if (sscanf(keywords[i+1], "%ld", &st)) {
		    pU->ovlyopenParams.dwStyle |= st; 
		} else
		    FIXME("unknown 'style' keyword %s, please report.\n", keywords[i+1]);
		i += 2;
		continue;
	    }
	    break;
	}
	FIXME("unknown parameter passed %s, please report.\n",
	      keywords[i]);
	i++;
    }

    _MCISTR_Unquote(pU->openParams.lpstrElementName);

    res = mciSendCommandA(0, MCI_OPEN, dwFlags, (DWORD)pU);

    free(pU->openParams.lpstrElementName);
    free(pU->openParams.lpstrDeviceType);
    free(pU->openParams.lpstrAlias);
    free(pU);
    return res;
}

/* A helper function for a lot of others ... 
 * for instance status/play/record/seek etc.
 */
DWORD
_MCISTR_determine_timeformat(LPCSTR dev, WORD wDevID, WORD uDevTyp, int *timef)
{
    int			res;
    DWORD 		dwFlags = MCI_STATUS_ITEM;
    LPMCI_STATUS_PARMS	statusParams = xmalloc(sizeof(MCI_STATUS_PARMS));
    
    if (!statusParams) return 0;
    statusParams->dwItem	= MCI_STATUS_TIME_FORMAT;
    statusParams->dwReturn	= 0;
    res = mciSendCommandA(wDevID, MCI_STATUS, dwFlags, (DWORD)statusParams);

    if (res == 0) *timef = statusParams->dwReturn;
    free(statusParams);
    return res;
}

/* query status of MCI drivers
 * Arguments:
 * Required: 
 *	"mode"	- returns "not ready" "paused" "playing" "stopped" "open" 
 *		  "parked" "recording" "seeking" ....
 * Basics:
 *	"current track"	- returns current track as integer
 *	"length [track <nr>]"	- returns length [of track <nr>] in current 
 *				timeformat
 *	"number of tracks" - returns number of tracks as integer
 *	"position [track <nr>]" - returns position [in track <nr>] in current 
 *				timeformat
 *	"ready"			- checks if device is ready to play, -> bool
 *	"start position"	- returns start position in timeformat
 *	"time format"		- returns timeformat (list of possible values:
 * 				"ms" "msf" "milliseconds" "hmsf" "tmsf" "frames"
 *				"bytes" "samples" "hms")
 *	"media present"		- returns if media is present as bool
 * Animation:
 *	"forward"		- returns "true" if device is playing forwards
 *	"speed"			- returns speed for device
 *	"palette handle"	- returns palette handle
 *	"window handle"		- returns window handle
 * 	"stretch"		- returns stretch bool
 * MIDI sequencer:
 *	"division type"		- ? returns "PPQN" "SMPTE 24 frame" 
 * 			"SMPTE 25 frame" "SMPTE 30 frame" "SMPTE 30 drop frame"
 *	"tempo"			- current tempo in (PPQN? speed in frames, SMPTE*? speed in hsmf)
 *	"offset"		- offset in dito.
 *	"port"			- midi port as integer
 * 	"slave"			- slave device ("midi", "file", "none", "smpte")
 *	"master"		- masterdevice (dito.)
 * Overlay:
 *	"window handle"		- see animation
 *	"stretch"		- dito
 * Video Disc:
 *	"speed"			- speed as integer
 *	"forward"		- returns bool (when playing forward)
 *	"side"			- returns 1 or 2
 *	"media type"		- returns "CAV" "CLV" "other"
 *	"disc size"		- returns "8" or "12"
 * WAVEFORM audio:
 *	"input"			- base queries on input set
 *	"output"		- base queries on output set
 *	"format tag"		- return integer format tag
 *	"channels"		- return integer nr of channels
 *	"bytespersec"		- return average nr of bytes/sec
 *	"samplespersec"		- return nr of samples per sec
 *	"bitspersample"		- return bitspersample
 *	"alignment"		- return block alignment
 *	"level"			- return level?
 * Digitalvideo:
 *	"audio"
 *	"audio alignment"
 *	"audio bitspersample"
 *	"audio breaks"
 *	"audio bytespersec"
 *	"audio input"
 *	"audio record"
 *	"audio source"
 *	"audio samplespersec"
 *	"audio stream"
 *	"bass"
 *	"bitsperpel"
 *	"brightness"
 *	"color"
 *	"contrast"
 *	"disk space drive"
 *	"file completion"
 *	"file format "
 *	"file mode"
 *	"forward"
 *	"frames skipped"
 *	"gamma"
 *	"input"
 *	"left volume"
 *	"media present"
 *	"mode"
 *	"monitor"
 *	"monitor method"
 *	"nominal"
 *	"nominal frame rate"
 *	"nominal record frame rate"
 *	"output"
 *	"palette handle"
 *	"pause mode"
 *	"play speed"
 *	"record frame rate"
 *	"reference frame"
 *	"reserved size"
 *	"right volume"
 *	"seek exactly"
 *	"sharpness"
 *	"smpte"
 *	"speed"
 *	"still file format"
 *	"tint"
 *	"treble"
 *	"unsaved"
 *	"video"
 *	"video key index"
 *	"video key color"
 *	"video record"
 *	"video source"
 *	"video source number"
 *	"video stream"
 *	"volume"
 *	"window handle"
 *	"window visible"
 *	"window minimized"
 *	"window maximized"
 */

#define ITEM1(str, item, xtype) 		\
	if (!STRCMP(keywords[i], str)) {	\
		pU->statusParams.dwItem = item;	\
		type = xtype;			\
		i++;				\
		continue;			\
	}
#define ITEM2(str1, str2, item, xtype) 		\
	if (	!STRCMP(keywords[i], str1) &&	\
		(i+1 < nrofkeywords) &&		\
		!STRCMP(keywords[i+1], str2)) {	\
		pU->statusParams.dwItem = item;	\
		type = xtype;			\
		i += 2;				\
		continue;			\
	}
#define ITEM3(str1, str2, str3, item, xtype) 	\
	if (	!STRCMP(keywords[i], str1) &&	\
		(i+2 < nrofkeywords) &&		\
		!STRCMP(keywords[i+1], str2) &&	\
		!STRCMP(keywords[i+2], str3)) {	\
		pU->statusParams.dwItem = item;	\
		type = xtype;			\
		i += 3;				\
		continue;			\
	}
#define ITEM4(str1, str2, str3, str4, item, xtype) \
	if (	!STRCMP(keywords[i], str1) &&	\
		(i+3 < nrofkeywords) &&		\
		!STRCMP(keywords[i+1], str2) &&	\
		!STRCMP(keywords[i+2], str3) &&	\
		!STRCMP(keywords[i+3], str4)) {	\
		pU->statusParams.dwItem = item;	\
		type = xtype;			\
		i += 4;				\
		continue;			\
	}

static DWORD
MCISTR_Status(_MCISTR_PROTO_) {
    union U {
	MCI_STATUS_PARMS	statusParams;
	MCI_DGV_STATUS_PARMSA	dgvstatusParams;
    };
    union U*		pU = xmalloc(sizeof(union U));
    int			type = 0, i, res, timef;
    
    pU->statusParams.dwCallback = hwndCallback;
    dwFlags	|= MCI_STATUS_ITEM;
    res = _MCISTR_determine_timeformat(dev, wDevID, uDevTyp, &timef);
    if (res) return res;
    
    pU->statusParams.dwReturn	= 0;
    pU->statusParams.dwItem	= 0;
    
    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "track") && (i+1 < nrofkeywords)) {
	    sscanf(keywords[i+1], "%ld", &(pU->statusParams.dwTrack));
	    dwFlags |= MCI_TRACK;
	    i += 2;
	    continue;
	}
	FLAG1("start", MCI_STATUS_START);
	/* generic things */
	ITEM2("current", "track", MCI_STATUS_CURRENT_TRACK, _MCISTR_time);
	ITEM2("time", "format", MCI_STATUS_TIME_FORMAT, _MCISTR_tfname);
	ITEM1("ready", MCI_STATUS_READY, _MCISTR_bool);
	ITEM1("mode", MCI_STATUS_MODE, _MCISTR_mode);
	ITEM3("number", "of", "tracks", MCI_STATUS_NUMBER_OF_TRACKS, _MCISTR_int);
	ITEM1("length", MCI_STATUS_LENGTH, _MCISTR_time);
	ITEM1("position", MCI_STATUS_POSITION, _MCISTR_time);
	ITEM2("media", "present", MCI_STATUS_MEDIA_PRESENT, _MCISTR_bool);
	
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    ITEM2("palette", "handle", MCI_ANIM_STATUS_HPAL, _MCISTR_int);
	    ITEM2("window", "handle", MCI_ANIM_STATUS_HWND, _MCISTR_int);
	    ITEM1("stretch", MCI_ANIM_STATUS_STRETCH, _MCISTR_bool);
	    ITEM1("speed", MCI_ANIM_STATUS_SPEED, _MCISTR_int);
	    ITEM1("forward", MCI_ANIM_STATUS_FORWARD, _MCISTR_bool);
	    break;
	case MCI_DEVTYPE_SEQUENCER:
	    /* just completing the list, not working correctly */
	    ITEM2("division", "type", MCI_SEQ_STATUS_DIVTYPE, _MCISTR_divtype);
	    /* tempo ... PPQN in frames/second, SMPTE in hmsf */
	    ITEM1("tempo", MCI_SEQ_STATUS_TEMPO, _MCISTR_int);
	    ITEM1("port", MCI_SEQ_STATUS_PORT, _MCISTR_int);
	    ITEM1("slave", MCI_SEQ_STATUS_SLAVE, _MCISTR_seqtype);
	    ITEM1("master", MCI_SEQ_STATUS_SLAVE, _MCISTR_seqtype);
	    /* offset ... PPQN in frames/second, SMPTE in hmsf */
	    ITEM1("offset", MCI_SEQ_STATUS_SLAVE, _MCISTR_time);
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    ITEM2("window", "handle", MCI_OVLY_STATUS_HWND, _MCISTR_int);
	    ITEM1("stretch", MCI_OVLY_STATUS_STRETCH, _MCISTR_bool);
	    break;
	case MCI_DEVTYPE_VIDEODISC:
	    ITEM1("speed", MCI_VD_STATUS_SPEED, _MCISTR_int);
	    ITEM1("forward", MCI_VD_STATUS_FORWARD, _MCISTR_bool);
	    ITEM1("side", MCI_VD_STATUS_SIDE, _MCISTR_int);
	    ITEM2("media", "type", MCI_VD_STATUS_SIDE, _MCISTR_vdmtype);
	    /* returns 8 or 12 */
	    ITEM2("disc", "size", MCI_VD_STATUS_DISC_SIZE, _MCISTR_int);
	    break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:
	    /* I am not quite sure if foll. 2 lines are right. */
	    FLAG1("input", MCI_WAVE_INPUT);
	    FLAG1("output", MCI_WAVE_OUTPUT);
	    
	    ITEM2("format", "tag", MCI_WAVE_STATUS_FORMATTAG, _MCISTR_int);
	    ITEM1("channels", MCI_WAVE_STATUS_CHANNELS, _MCISTR_int);
	    ITEM1("bytespersec", MCI_WAVE_STATUS_AVGBYTESPERSEC, _MCISTR_int);
	    ITEM1("samplespersec", MCI_WAVE_STATUS_SAMPLESPERSEC, _MCISTR_int);
	    ITEM1("bitspersample", MCI_WAVE_STATUS_BITSPERSAMPLE, _MCISTR_int);
	    ITEM1("alignment", MCI_WAVE_STATUS_BLOCKALIGN, _MCISTR_int);
	    ITEM1("level", MCI_WAVE_STATUS_LEVEL, _MCISTR_int);
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	    ITEM1("audio", MCI_DGV_STATUS_AUDIO, _MCISTR_bool);	 /* FIXME? doc says MCI_ON/MCI_OFF */
	    ITEM2("audio", "alignment", MCI_DGV_STATUS_BLOCKALIGN, _MCISTR_int);
	    ITEM2("audio", "bitspersample", MCI_DGV_STATUS_BITSPERSAMPLE, _MCISTR_int);
/* EPP 	    ITEM2("audio", "breaks", MCI_AVI_STATUS_AUDIO_BREAKS, _MCISTR_int); */
	    ITEM2("audio", "bytespersec", MCI_DGV_STATUS_AVGBYTESPERSEC, _MCISTR_int);
	    ITEM2("audio", "input", MCI_DGV_STATUS_AUDIO_INPUT, _MCISTR_int);
	    ITEM2("audio", "record", MCI_DGV_STATUS_AUDIO_RECORD, _MCISTR_bool); /* FIXME? doc says MCI_ON/MCI_OFF */
	    ITEM3("audio", "source", "number", MCI_DGV_STATUS_AUDIO_SOURCE, _MCISTR_int);
	    /* FIXME: ITEM2("audio", "source", MCI_DGV_STATUS_AUDIO_SOURCE, _MCISTR_dgvaudiosource); */
/* EPP 	    ITEM2("audio", "samplespersec", MCI_DGV_STATUS_SAMPLESPERSECOND, _MCISTR_int); */
	    ITEM2("audio", "stream", MCI_DGV_STATUS_AUDIO_STREAM, _MCISTR_int);
	    ITEM1("bass", MCI_DGV_STATUS_BASS, _MCISTR_int);
	    ITEM1("bitsperpel", MCI_DGV_STATUS_BITSPERPEL, _MCISTR_int);
	    ITEM1("brightness", MCI_DGV_STATUS_BRIGHTNESS, _MCISTR_int);
	    ITEM1("color", MCI_DGV_STATUS_COLOR, _MCISTR_int);
	    ITEM1("contrast", MCI_DGV_STATUS_CONTRAST, _MCISTR_int);
/* EPP  *	"disk space drive" FIXME */
	    ITEM2("file", "completion", MCI_DGV_STATUS_FILE_COMPLETION, _MCISTR_int);
/* EPP 	    ITEM2("file", "format", MCI_DGV_STATUS_FILEFORMAT, _MCISTR_???); */
/* EPP 	    ITEM2("file", "mode", MCI_DGV_STATUS_FILE_MODE, _MCISTR_gdvfileformat); */
	    ITEM1("forward", MCI_DGV_STATUS_FORWARD, _MCISTR_bool);
/* EPP 	    ITEM2("frames", "skipped", MCI_AVI_STATUS_FRAMES_SKIPPED, _MCISTR_int); */
	    ITEM1("gamma", MCI_DGV_STATUS_GAMMA, _MCISTR_int);
	    ITEM2("input", "bass", MCI_DGV_STATUS_BASS|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "brightness", MCI_DGV_STATUS_BRIGHTNESS|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "color", MCI_DGV_STATUS_COLOR|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "contrast", MCI_DGV_STATUS_CONTRAST|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "gamma", MCI_DGV_STATUS_GAMMA|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "sharpness", MCI_DGV_STATUS_SHARPNESS|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "tint", MCI_DGV_STATUS_TINT|MCI_DGV_STATUS_INPUT, _MCISTR_int);
	    ITEM2("input", "treble", MCI_DGV_STATUS_TREBLE|MCI_DGV_STATUS_INPUT, _MCISTR_int);

	    ITEM2("left", "volume", MCI_DGV_STATUS_VOLUME|MCI_DGV_STATUS_LEFT, _MCISTR_int);
	    ITEM2("media", "present", MCI_STATUS_MEDIA_PRESENT, _MCISTR_bool);
/* EPP 	    ITEM2("monitor", "method", MCI_DGV_STATUS_MONITOR_METHOD, _MCISTR_monitor); */
/* EPP 	    ITEM1("monitor", MCI_DGV_STATUS_MONITOR, _MCISTR_monitor2); */
	    ITEM2("nominal", "bass", MCI_DGV_STATUS_BASS|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "brightness", MCI_DGV_STATUS_BRIGHTNESS|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "color", MCI_DGV_STATUS_COLOR|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "contrast", MCI_DGV_STATUS_CONTRAST|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "gamma", MCI_DGV_STATUS_GAMMA|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "sharpness", MCI_DGV_STATUS_SHARPNESS|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "tint", MCI_DGV_STATUS_TINT|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("nominal", "treble", MCI_DGV_STATUS_TREBLE|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM3("nominal", "frame", "rate", MCI_DGV_STATUS_FRAME_RATE|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM4("nominal", "record", "frame", "rate", MCI_DGV_STATUS_FRAME_RATE|MCI_DGV_STATUS_RECORD|MCI_DGV_STATUS_NOMINAL, _MCISTR_int);
	    ITEM2("output", "bass", MCI_DGV_STATUS_BASS|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "brightness", MCI_DGV_STATUS_BRIGHTNESS|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "color", MCI_DGV_STATUS_COLOR|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "contrast", MCI_DGV_STATUS_CONTRAST|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "gamma", MCI_DGV_STATUS_GAMMA|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "sharpness", MCI_DGV_STATUS_SHARPNESS|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "tint", MCI_DGV_STATUS_TINT|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("output", "treble", MCI_DGV_STATUS_TREBLE|MCI_DGV_STATUS_OUTPUT, _MCISTR_int);
	    ITEM2("palette", "handle", MCI_DGV_STATUS_HPAL, _MCISTR_int); /* FIXME? */
	    ITEM2("pause", "mode", MCI_DGV_STATUS_PAUSE_MODE, _MCISTR_bool); /* FIXME */
/* EPP 	    ITEM2("play", "speed", MCI_AVI_STATUS_LAST_PLAY_SPEED, _MCISTR_int); */
	    ITEM3("record", "frame", "rate", MCI_DGV_STATUS_FRAME_RATE|MCI_DGV_STATUS_RECORD, _MCISTR_int);
	    ITEM2("reference", "frame", MCI_DGV_STATUS_FRAME_RATE, _MCISTR_int); /* FIXME */
	    ITEM2("reserved", "size", MCI_DGV_STATUS_SIZE, _MCISTR_int);
	    ITEM2("right", "volume", MCI_DGV_STATUS_VOLUME|MCI_DGV_STATUS_RIGHT, _MCISTR_int);
	    ITEM2("seek", "exactly", MCI_DGV_STATUS_SEEK_EXACTLY, _MCISTR_bool);
	    ITEM1("sharpness", MCI_DGV_STATUS_SHARPNESS, _MCISTR_int);
/* EPP 	    ITEM1("smpte", MCI_DGV_STATUS_SMPTE, _MCISTR_smpte); */
/* EPP 	    ITEM1("speed", MCI_DGV_STATUS_SPEED, _MCISTR_speed);  */
/* EPP 	    ITEM3("still", "file", "format", MCI_DGV_STATUS_STILL_FILEFORMAT, _MCISTR_???); */
	    ITEM1("tint", MCI_DGV_STATUS_TINT, _MCISTR_int);
	    ITEM1("treble", MCI_DGV_STATUS_TREBLE, _MCISTR_int);
	    ITEM1("unsaved", MCI_DGV_STATUS_UNSAVED, _MCISTR_bool);
	    ITEM3("video", "key", "index", MCI_DGV_STATUS_KEY_INDEX, _MCISTR_int);
	    ITEM3("video", "key", "color", MCI_DGV_STATUS_KEY_COLOR, _MCISTR_bool);
	    ITEM2("video", "record", MCI_DGV_STATUS_VIDEO_RECORD, _MCISTR_bool);  /* FIXME MCI_ON/OFF */
	    ITEM3("video", "source", "number", MCI_DGV_STATUS_VIDEO_SOURCE, _MCISTR_int);
/* EPP 	    ITEM2("video", "source", MCI_DGV_STATUS_VIDEO_SOURCE, _MCISTR_videosrctype); */
	    ITEM2("video", "stream", MCI_DGV_STATUS_VIDEO_STREAM, _MCISTR_int);
	    ITEM1("video", MCI_DGV_STATUS_VIDEO, _MCISTR_bool); /* FIXME MCI_ON/OFF */
	    ITEM1("volume", MCI_DGV_STATUS_VOLUME, _MCISTR_int);
	    ITEM2("window", "handle", MCI_DGV_STATUS_HWND, _MCISTR_int);
	    ITEM2("window", "visible", MCI_DGV_STATUS_WINDOW_VISIBLE, _MCISTR_bool);
	    ITEM2("window", "minimized", MCI_DGV_STATUS_WINDOW_MINIMIZED, _MCISTR_bool);
	    ITEM2("window", "maximized",MCI_DGV_STATUS_WINDOW_MAXIMIZED , _MCISTR_bool);
	    break;
	}
	FIXME("unknown keyword '%s'\n", keywords[i]);
	i++;
    }
    if (!pU->statusParams.dwItem) 
	return MCIERR_MISSING_STRING_ARGUMENT;
    
    res = mciSendCommandA(wDevID, MCI_STATUS, dwFlags, (DWORD)pU);

    if (res == 0)
	_MCISTR_convreturn(type, pU->statusParams.dwReturn, lpstrReturnString, uReturnLength, uDevTyp, timef);
    free(pU);
    return res;
}
#undef ITEM1
#undef ITEM2
#undef ITEM3
#undef ITEM4

/* set specified parameters in respective MCI drivers
 * Arguments:
 *	"door open"	eject media or somesuch
 *	"door close"	load media
 *	"time format <timeformatname>"	"ms" "milliseconds" "msf" "hmsf" 
 *					"tmsf" "SMPTE 24" "SMPTE 25" "SMPTE 30"
 *					"SMPTE drop 30"
 *	"audio [all|left|right] [on|off]" sets specified audiochannel on or off
 *	"video [on|off]"		sets video on/off
 * Waveform audio:
 *	"formattag pcm"		sets format to pcm
 *	"formattag <nr>"	sets integer formattag value
 *	"any input"		accept input from any known source
 *	"any output"		output to any known destination
 *	"input <nr>"		input from source <nr>
 *	"output <nr>"		output to destination <nr>
 *	"channels <nr>"		sets nr of channels 
 *	"bytespersec <nr>"	sets average bytes per second
 *	"samplespersec <nr>"	sets average samples per second (1 sample can
 *				be 2 bytes!) 
 *	"alignment <nr>"	sets the blockalignment to <nr>
 *	"bitspersample <nr>"	sets the nr of bits per sample
 * Sequencer:
 *	"master [midi|file|smpte|none]" sets the midi master device
 *	"slave [midi|file|smpte|none]" sets the midi master device
 *	"port mapper"		midioutput to portmapper
 *	"port <nr>"		midioutput to specified port
 *	"tempo <nr>"		tempo of track (depends on timeformat/divtype)
 *	"offset <nr>"		start offset?
 */
static DWORD
MCISTR_Set(_MCISTR_PROTO_) {
    union U {
	MCI_SET_PARMS		setParams;
	MCI_WAVE_SET_PARMS	wavesetParams;
	MCI_SEQ_SET_PARMS	seqsetParams;
    };
    union U*	pU = xmalloc(sizeof(union U));
    int		i, res;
    
    pU->setParams.dwCallback = hwndCallback;

    for (i = 0; i < nrofkeywords; ) {
	FLAG2("door", "open", MCI_SET_DOOR_OPEN);
	FLAG2("door", "closed", MCI_SET_DOOR_CLOSED);
	
	if (!STRCMP(keywords[i], "time") && (i+2 < nrofkeywords) && !STRCMP(keywords[i+1], "format")) {
	    dwFlags |= MCI_SET_TIME_FORMAT;
	    
	    /* FIXME:is this a shortcut for milliseconds or
	     *	 minutes:seconds? */
	    if (!STRCMP(keywords[i+2], "ms"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
	    
	    if (!STRCMP(keywords[i+2], "milliseconds"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
	    if (!STRCMP(keywords[i+2], "msf"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_MSF;
	    if (!STRCMP(keywords[i+2], "hms"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_HMS;
	    if (!STRCMP(keywords[i+2], "frames"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_FRAMES;
	    if (!STRCMP(keywords[i+2], "track"))
		pU->setParams.dwTimeFormat = MCI_VD_FORMAT_TRACK;
	    if (!STRCMP(keywords[i+2], "bytes"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_BYTES;
	    if (!STRCMP(keywords[i+2], "samples"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_SAMPLES;
	    if (!STRCMP(keywords[i+2], "tmsf"))
		pU->setParams.dwTimeFormat = MCI_FORMAT_TMSF;
	    if (	!STRCMP(keywords[i+2], "song") && 
			(i+3 < nrofkeywords) &&
			!STRCMP(keywords[i+3], "pointer")
			)
		pU->setParams.dwTimeFormat = MCI_SEQ_FORMAT_SONGPTR;
	    if (!STRCMP(keywords[i+2], "smpte") && (i+3 < nrofkeywords)) {
		if (!STRCMP(keywords[i+3], "24"))
		    pU->setParams.dwTimeFormat = MCI_FORMAT_SMPTE_24;
		if (!STRCMP(keywords[i+3], "25"))
		    pU->setParams.dwTimeFormat = MCI_FORMAT_SMPTE_25;
		if (!STRCMP(keywords[i+3], "30"))
		    pU->setParams.dwTimeFormat = MCI_FORMAT_SMPTE_30;
		if (!STRCMP(keywords[i+3], "drop") && (i+4 < nrofkeywords) && !STRCMP(keywords[i+4], "30")) {
		    pU->setParams.dwTimeFormat = MCI_FORMAT_SMPTE_30DROP;
		    i++;
		}
		i++;
				/*FALLTHROUGH*/
	    }
	    i += 3;
	    continue;
	}
	if (!STRCMP(keywords[i], "audio") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_SET_AUDIO;
	    if (!STRCMP(keywords[i+1], "all"))
		pU->setParams.dwAudio = MCI_SET_AUDIO_ALL;
	    if (!STRCMP(keywords[i+1], "left"))
		pU->setParams.dwAudio = MCI_SET_AUDIO_LEFT;
	    if (!STRCMP(keywords[i+1], "right"))
		pU->setParams.dwAudio = MCI_SET_AUDIO_RIGHT;
	    i += 2;
	    continue;
	}
	FLAG1("video", MCI_SET_VIDEO);
	FLAG1("on", MCI_SET_ON);
	FLAG1("off", MCI_SET_OFF);
	switch (uDevTyp) {
	case MCI_DEVTYPE_WAVEFORM_AUDIO:
	    FLAG2("any", "input", MCI_WAVE_SET_ANYINPUT);
	    FLAG2("any", "output", MCI_WAVE_SET_ANYOUTPUT);
	    
	    if (	!STRCMP(keywords[i], "formattag") && 
			(i+1 < nrofkeywords) &&
			!STRCMP(keywords[i+1], "pcm")
			) {
		dwFlags |= MCI_WAVE_SET_FORMATTAG;
		pU->wavesetParams.wFormatTag = WAVE_FORMAT_PCM;
		i += 2;
		continue;
	    }
	    
	    /* <keyword> <integer> */
#define WII(str,flag,fmt,element) 		\
    if (!STRCMP(keywords[i], str) &&		\
	(i+1 < nrofkeywords)) {			\
        sscanf(keywords[i+1], fmt,		\
	       &(pU->wavesetParams. element));	\
	dwFlags |= flag;			\
	i += 2;					\
	continue;				\
    }
	    WII("formattag", MCI_WAVE_SET_FORMATTAG, "%u", wFormatTag);
	    WII("channels", MCI_WAVE_SET_CHANNELS, "%u", nChannels);
	    WII("bytespersec", MCI_WAVE_SET_AVGBYTESPERSEC, "%lu", nAvgBytesPerSec);
	    WII("samplespersec", MCI_WAVE_SET_SAMPLESPERSEC, "%lu", nSamplesPerSec);
	    WII("alignment", MCI_WAVE_SET_BLOCKALIGN, "%u", nBlockAlign);
	    WII("bitspersample", MCI_WAVE_SET_BITSPERSAMPLE, "%u", wBitsPerSample);
	    WII("input", MCI_WAVE_INPUT, "%u", wInput);
	    WII("output", MCI_WAVE_OUTPUT, "%u", wOutput);
#undef WII
	    break;
	case MCI_DEVTYPE_SEQUENCER:
	    if (!STRCMP(keywords[i], "master") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_SEQ_SET_MASTER;
		if (!STRCMP(keywords[i+1], "midi"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_MIDI;
		if (!STRCMP(keywords[i+1], "file"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_FILE;
		if (!STRCMP(keywords[i+1], "smpte"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_SMPTE;
		if (!STRCMP(keywords[i+1], "none"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_NONE;
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "slave") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_SEQ_SET_SLAVE;
		if (!STRCMP(keywords[i+1], "midi"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_MIDI;
		if (!STRCMP(keywords[i+1], "file"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_FILE;
		if (!STRCMP(keywords[i+1], "smpte"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_SMPTE;
		if (!STRCMP(keywords[i+1], "none"))
		    pU->seqsetParams.dwMaster = MCI_SEQ_NONE;
		i += 2;
		continue;
	    }
	    if (	!STRCMP(keywords[i], "port") && 
			(i+1 < nrofkeywords) &&
			!STRCMP(keywords[i+1], "mapper")
			) {
		pU->seqsetParams.dwPort=-1;/* FIXME:not sure*/
		dwFlags |= MCI_SEQ_SET_PORT;
		i += 2;
		continue;
	    }
#define SII(str,flag,element) 			\
	if (!STRCMP(keywords[i], str) && 	\
             (i+1 < nrofkeywords)) {		\
	    sscanf(keywords[i+1], "%ld",	\
		   &(pU->seqsetParams.element));\
	    dwFlags |= flag;			\
	    i += 2;				\
	    continue;				\
	}
	    SII("tempo", MCI_SEQ_SET_TEMPO, dwTempo);
	    SII("port", MCI_SEQ_SET_PORT, dwPort);
	    SII("offset", MCI_SEQ_SET_PORT, dwOffset);
#undef SII
	}
	i++;
    }
    if (!dwFlags)
	return MCIERR_MISSING_STRING_ARGUMENT;
    res = mciSendCommandA(wDevID, MCI_SET, dwFlags, (DWORD)pU);
    free(pU);
    return res;
}

/* specify break key
 * Arguments: 
 *	"off"		disable break
 *	"on <keyid>"	enable break on key with keyid
 * (I strongly suspect, that there is another parameter:
 *	"window <handle>"	
 * but I don't see it mentioned in my documentation.
 * Returns nothing.
 */
static DWORD
MCISTR_Break(_MCISTR_PROTO_)
{
    LPMCI_BREAK_PARMS	breakParams = xmalloc(sizeof(MCI_BREAK_PARMS));
    int 		res, i;
    
    if (!breakParams) return 0;

    breakParams->dwCallback = hwndCallback;
    /*breakParams.hwndBreak ? */
    for (i = 0; i < nrofkeywords; i++) {
	FLAG1("off", MCI_BREAK_OFF);
	if (!STRCMP(keywords[i], "on") && (nrofkeywords > i+1)) {
	    dwFlags &= ~MCI_BREAK_OFF;
	    dwFlags |= MCI_BREAK_KEY;
	    sscanf(keywords[i+1], "%d", &(breakParams->nVirtKey));
	    i += 2;
	    continue;
	}
    }
    res = mciSendCommandA(wDevID, MCI_BREAK, dwFlags, (DWORD)breakParams);
    free(breakParams);
    return res;
}

#define ITEM1(str, item, xtype) 		\
	if (!STRCMP(keywords[i], str)) {	\
		gdcParams->dwItem = item;	\
		type = xtype;			\
		i++;				\
		continue;			\
	}
#define ITEM2(str1, str2, item, xtype) 		\
	if (	!STRCMP(keywords[i], str1) &&	\
		(i+1 < nrofkeywords) &&		\
		!STRCMP(keywords[i+1], str2)) {	\
		gdcParams->dwItem = item;	\
		type = xtype;			\
		i += 2;				\
		continue;			\
	}
#define ITEM3(str1, str2, str3, item, xtype) 	\
	if (	!STRCMP(keywords[i], str1) &&	\
		(i+2 < nrofkeywords) &&		\
		!STRCMP(keywords[i+1], str2) &&	\
		!STRCMP(keywords[i+2], str3)) {	\
		gdcParams->dwItem = item;	\
		type = xtype;			\
		i += 3;				\
		continue;			\
	}

/* get device capabilities of MCI drivers
 * Arguments:
 * Generic:
 *	"device type"		returns device name as string
 *	"has audio"		returns bool
 *	"has video"		returns bool
 *	"uses files"		returns bool
 *	"compound device"	returns bool
 *	"can record"		returns bool
 *	"can play"		returns bool
 *	"can eject"		returns bool
 *	"can save"		returns bool
 * Animation:
 *	"palettes"		returns nr of available palette entries
 *	"windows"		returns nr of available windows
 *	"can reverse"		returns bool
 *	"can stretch"		returns bool
 *	"slow play rate"	returns the slow playrate
 *	"fast play rate"	returns the fast playrate
 *	"normal play rate"	returns the normal playrate
 * Digital video
 *	"can freeze"		returns bool
 *	"can lock"		returns bool
 *	"can reverse"		returns bool
 *	"can stretch"		returns bool
 *	"can stretch input"	returns bool
 *	"can test"		returns bool
 *	"has still"		returns bool
 *	"maximum play rate"	returns the maximum play rate, in fps
 *	"minimum play rate"	returns the minimum play rate, in fps
 *	"uses files"		returns bool
 *	"uses palettes"		returns bool
 *	"windows"		returns nr of available windows
 * Overlay:
 *	"windows"		returns nr of available windows
 *	"can stretch"		returns bool
 *	"can freeze"		returns bool
 * Videodisc:
 *	"cav"			assume CAV discs (default if no disk inserted)
 *	"clv"			assume CLV discs 
 *	"can reverse"		returns bool
 *	"slow play rate"	returns the slow playrate
 *	"fast play rate"	returns the fast playrate
 *	"normal play rate"	returns the normal playrate
 * Waveform audio:
 *	"inputs"		returns nr of inputdevices
 *	"outputs"		returns nr of outputdevices
 */
static DWORD
MCISTR_Capability(_MCISTR_PROTO_) {
    MCI_GETDEVCAPS_PARMS *gdcParams = xmalloc(sizeof(MCI_GETDEVCAPS_PARMS));
    int	type=0, i, res;
    
    gdcParams->dwCallback = hwndCallback;
    if (!nrofkeywords)
	return MCIERR_MISSING_STRING_ARGUMENT;
    /* well , thats default */
    dwFlags |= MCI_GETDEVCAPS_ITEM;
    gdcParams->dwItem = 0;

    for (i = 0; i < nrofkeywords; i++) {
	ITEM2("device", "type", MCI_GETDEVCAPS_DEVICE_TYPE, _MCISTR_devtype);
	ITEM2("has", "audio", MCI_GETDEVCAPS_HAS_AUDIO, _MCISTR_bool);
	ITEM2("has", "video", MCI_GETDEVCAPS_HAS_VIDEO, _MCISTR_bool);
	ITEM2("uses", "files", MCI_GETDEVCAPS_USES_FILES, _MCISTR_bool);
	ITEM2("compound", "device", MCI_GETDEVCAPS_COMPOUND_DEVICE, _MCISTR_bool);
	ITEM2("can", "record", MCI_GETDEVCAPS_CAN_RECORD, _MCISTR_bool);
	ITEM2("can", "play", MCI_GETDEVCAPS_CAN_PLAY, _MCISTR_bool);
	ITEM2("can", "eject", MCI_GETDEVCAPS_CAN_EJECT, _MCISTR_bool);
	ITEM2("can", "save", MCI_GETDEVCAPS_CAN_SAVE, _MCISTR_bool);
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    ITEM1("palettes", MCI_ANIM_GETDEVCAPS_PALETTES, _MCISTR_int);
	    ITEM1("windows", MCI_ANIM_GETDEVCAPS_MAX_WINDOWS, _MCISTR_int);
	    ITEM2("can", "reverse", MCI_ANIM_GETDEVCAPS_CAN_REVERSE, _MCISTR_bool);
	    ITEM2("can", "stretch", MCI_ANIM_GETDEVCAPS_CAN_STRETCH, _MCISTR_bool);
	    ITEM3("slow", "play", "rate", MCI_ANIM_GETDEVCAPS_SLOW_RATE, _MCISTR_int);
	    ITEM3("fast", "play", "rate", MCI_ANIM_GETDEVCAPS_FAST_RATE, _MCISTR_int);
	    ITEM3("normal", "play", "rate", MCI_ANIM_GETDEVCAPS_NORMAL_RATE, _MCISTR_int);
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	    ITEM2("can", "freeze", MCI_DGV_GETDEVCAPS_CAN_FREEZE, _MCISTR_bool);
	    ITEM2("can", "lock", MCI_DGV_GETDEVCAPS_CAN_LOCK, _MCISTR_bool);
	    ITEM2("can", "reverse", MCI_DGV_GETDEVCAPS_CAN_REVERSE, _MCISTR_bool);
	    ITEM3("can", "stretch", "input", MCI_DGV_GETDEVCAPS_CAN_STR_IN, _MCISTR_bool);
	    ITEM2("can", "stretch", MCI_DGV_GETDEVCAPS_CAN_STRETCH, _MCISTR_bool);
	    ITEM2("can", "test", MCI_DGV_GETDEVCAPS_CAN_TEST, _MCISTR_bool);
	    ITEM2("has", "still", MCI_DGV_GETDEVCAPS_HAS_STILL, _MCISTR_bool);
	    ITEM3("maximum", "play", "rate", MCI_DGV_GETDEVCAPS_MAXIMUM_RATE, _MCISTR_int);
	    ITEM3("minimum", "play", "rate", MCI_DGV_GETDEVCAPS_MINIMUM_RATE, _MCISTR_int);
	    ITEM2("uses", "files", MCI_GETDEVCAPS_USES_FILES, _MCISTR_bool);
	    ITEM2("uses", "palettes", MCI_DGV_GETDEVCAPS_PALETTES, _MCISTR_bool);
	    ITEM1("windows", MCI_DGV_GETDEVCAPS_MAX_WINDOWS, _MCISTR_int);
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    ITEM1("windows", MCI_OVLY_GETDEVCAPS_MAX_WINDOWS, _MCISTR_int);
	    ITEM2("can", "freeze", MCI_OVLY_GETDEVCAPS_CAN_FREEZE, _MCISTR_bool);
	    ITEM2("can", "stretch", MCI_OVLY_GETDEVCAPS_CAN_STRETCH, _MCISTR_bool);
	    break;
	case MCI_DEVTYPE_VIDEODISC:
	    FLAG1("cav", MCI_VD_GETDEVCAPS_CAV);
	    FLAG1("clv", MCI_VD_GETDEVCAPS_CLV);
	    ITEM2("can", "reverse", MCI_VD_GETDEVCAPS_CAN_REVERSE, _MCISTR_bool);
	    ITEM3("slow", "play", "rate", MCI_VD_GETDEVCAPS_SLOW_RATE, _MCISTR_int);
	    ITEM3("fast", "play", "rate", MCI_VD_GETDEVCAPS_FAST_RATE, _MCISTR_int);
	    ITEM3("normal", "play", "rate", MCI_VD_GETDEVCAPS_NORMAL_RATE, _MCISTR_int);
	    break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:
	    ITEM1("inputs", MCI_WAVE_GETDEVCAPS_INPUTS, _MCISTR_int);
	    ITEM1("outputs", MCI_WAVE_GETDEVCAPS_OUTPUTS, _MCISTR_int);
	    break;
	}
    }
    res = mciSendCommandA(wDevID, MCI_GETDEVCAPS, dwFlags, (DWORD)gdcParams);

    /* no timeformat needed */
    if (res == 0)
	_MCISTR_convreturn(type, gdcParams->dwReturn, lpstrReturnString,
			    uReturnLength, uDevTyp, 0);
    free(gdcParams);
    return res;
}
#undef ITEM1
#undef ITEM2
#undef ITEM3
/* resumes operation of device. no arguments, no return values */
static DWORD
MCISTR_Resume(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS*	genParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int			res;

    genParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_RESUME, dwFlags, (DWORD)genParams);
    free(genParams);
    return res;
}

/* pauses operation of device. no arguments, no return values */
static DWORD
MCISTR_Pause(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS*	genParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int 		res;

    genParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_PAUSE, dwFlags, (DWORD)genParams);
    free(genParams);
    return res;
}

/* stops operation of device. no arguments, no return values */
static DWORD
MCISTR_Stop(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS*	genParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int 		res;

    genParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_STOP, dwFlags, (DWORD)genParams);
    free(genParams);
    return res;
}

/* starts recording.
 * Arguments:
 *	"overwrite"	overwrite existing things
 *	"insert"	insert at current position
 *	"to <time>"	record up to <time> (specified in timeformat)
 *	"from <time>"	record from <time> (specified in timeformat)
 */
static DWORD
MCISTR_Record(_MCISTR_PROTO_) {
    int			i, res, timef, nrargs, j, k, a[4];
    char			*parsestr;
    MCI_RECORD_PARMS	*recordParams = xmalloc(sizeof(MCI_RECORD_PARMS));
    
    res = _MCISTR_determine_timeformat(dev, wDevID, uDevTyp, &timef);
    if (res) return res;
    
    switch (timef) {
    case MCI_FORMAT_MILLISECONDS:
    case MCI_FORMAT_FRAMES:
    case MCI_FORMAT_BYTES:
    case MCI_FORMAT_SAMPLES:
	nrargs = 1;
	parsestr = "%d";
	break;
    case MCI_FORMAT_HMS:
    case MCI_FORMAT_MSF:
	parsestr="%d:%d:%d";
	nrargs=3;
	break;
    case MCI_FORMAT_TMSF:
	parsestr="%d:%d:%d:%d";
	nrargs=4;
	break;
    default:FIXME("unknown timeformat %d, please report.\n", timef);
	parsestr="%d";
	nrargs=1;
	break;
    }
    recordParams->dwCallback = hwndCallback;
    for (i = 0; i < nrofkeywords; ) {
	if (!strcmp(keywords[i], "to") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_TO;
	    a[0] = a[1] = a[2] = a[3] = 0;
	    j=sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* add up all integers we got, if we have more 
	     * shift them. (Well I should use the macros in 
	     * mmsystem.h, right).
	     */
	    recordParams->dwTo = 0;
	    for (k = 0; k < j; k++)
		recordParams->dwTo += a[k] << (8*(nrargs-k));
	    i += 2;
	    continue;
	}
	if (!strcmp(keywords[i], "from") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_FROM;
	    a[0] = a[1] = a[2] = a[3] = 0;
	    j = sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* dito. */
	    recordParams->dwFrom = 0;
	    for (k = 0; k < j; k++)
		recordParams->dwFrom += a[k]<<(8*(nrargs-k));
	    i += 2;
	    continue;
	}
	FLAG1("insert", MCI_RECORD_INSERT);
	FLAG1("overwrite", MCI_RECORD_OVERWRITE);
	i++;
    }
    res = mciSendCommandA(wDevID, MCI_RECORD, dwFlags, (DWORD)recordParams);
    free(recordParams);
    return res;
}

/* play media
 * Arguments:
 * 	"to <time>"	play up to <time> (specified in set timeformat)
 * 	"from <time>"	play from <time> (specified in set timeformat)
 * Animation:
 *	"slow"		play slow
 *	"fast"		play fast 
 *	"scan"		play as fast as possible (with audio disabled perhaps)
 *	"reverse"	play reverse
 *	"speed <fps>"	play with specified frames per second
 * Videodisc:
 *	"slow"		play slow
 *	"fast"		play fast 
 *	"scan"		play as fast as possible (with audio disabled perhaps)
 *	"reverse"	play reverse
 *	"speed <fps>"	play with specified frames per second
 * Digitalvideo:
 *	"fullscreen"
 *	"repeat"
 *	"reverse"
 *	"window"
 */
static DWORD
MCISTR_Play(_MCISTR_PROTO_) {
    int			i, res, timef, nrargs, j, k, a[4];
    char			*parsestr;
    union U {
	MCI_PLAY_PARMS		playParams;
	MCI_VD_PLAY_PARMS	vdplayParams;
	MCI_ANIM_PLAY_PARMS	animplayParams;
	MCI_DGV_PLAY_PARMS	dgvplayParams;
    };
    union U *pU = xmalloc(sizeof(union U));
    
    res = _MCISTR_determine_timeformat(dev, wDevID, uDevTyp, &timef);
    if (res) return res;
    switch (timef) {
    case MCI_FORMAT_MILLISECONDS:
    case MCI_FORMAT_FRAMES:
    case MCI_FORMAT_BYTES:
    case MCI_FORMAT_SAMPLES:
	nrargs=1;
	parsestr="%d";
	break;
    case MCI_FORMAT_HMS:
    case MCI_FORMAT_MSF:
	parsestr="%d:%d:%d";
	nrargs=3;
	break;
    case MCI_FORMAT_TMSF:
	parsestr="%d:%d:%d:%d";
	nrargs=4;
	break;
    default:FIXME("unknown timeformat %d, please report.\n", timef);
	parsestr="%d";
	nrargs=1;
	break;
    }
    pU->playParams.dwCallback = hwndCallback;
    for (i = 0; i < nrofkeywords; ) {
	if (!strcmp(keywords[i], "to") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_TO;
	    a[0]=a[1]=a[2]=a[3]=0;
	    j=sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* add up all integers we got, if we have more 
	     * shift them. (Well I should use the macros in 
	     * mmsystem.h, right).
	     */
	    pU->playParams.dwTo=0;
	    for (k = 0; k < j; k++)
		pU->playParams.dwTo += a[k] << (8*(nrargs-k));
	    i += 2;
	    continue;
	}
	if (!strcmp(keywords[i], "from") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_FROM;
	    a[0]=a[1]=a[2]=a[3]=0;
	    j=sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* dito. */
	    pU->playParams.dwFrom=0;
	    for (k = 0; k < j; k++)
		pU->playParams.dwFrom += a[k]<<(8*(nrargs-k));
	    i += 2;
	    continue;
	}
	switch (uDevTyp) {
	case MCI_DEVTYPE_VIDEODISC:
	    FLAG1("slow", MCI_VD_PLAY_SLOW);
	    FLAG1("fast", MCI_VD_PLAY_FAST);
	    FLAG1("scan", MCI_VD_PLAY_SCAN);
	    FLAG1("reverse", MCI_VD_PLAY_REVERSE);
	    if (!STRCMP(keywords[i], "speed") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_VD_PLAY_SPEED;
		sscanf(keywords[i+1], "%ld", &(pU->vdplayParams.dwSpeed));
		i += 2;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_ANIMATION:
	    FLAG1("slow", MCI_ANIM_PLAY_SLOW);
	    FLAG1("fast", MCI_ANIM_PLAY_FAST);
	    FLAG1("scan", MCI_ANIM_PLAY_SCAN);
	    FLAG1("reverse", MCI_ANIM_PLAY_REVERSE);
	    if (!STRCMP(keywords[i], "speed") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_ANIM_PLAY_SPEED;
		sscanf(keywords[i+1], "%ld", &(pU->animplayParams.dwSpeed));
		i += 2;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
/* EPP 	    FLAG1("fullscreen", MCI_MCIAVI_PLAY_FULLSCREEN); */
	    FLAG1("repeat", MCI_DGV_PLAY_REPEAT);
	    FLAG1("reverse", MCI_DGV_PLAY_REVERSE);
/* EPP 	    FLAG1("window", MCI_MCIAVI_PLAY_WINDOW); */
	    break;
	}
	i++;
    }
    res = mciSendCommandA(wDevID, MCI_PLAY, dwFlags, (DWORD)pU);
    free(pU);
    return res;
}

/* seek to a specified position
 * Arguments:
 *	"to start"	seek to start of medium
 *	"to end"	seek to end of medium
 * 	"to <time>"	seek to <time> specified in current timeformat
 */
static DWORD
MCISTR_Seek(_MCISTR_PROTO_) {
    int			i, res, timef, nrargs, j, k, a[4];
    char		*parsestr;
    MCI_SEEK_PARMS	*seekParams = xmalloc(sizeof(MCI_SEEK_PARMS));
    
    res = _MCISTR_determine_timeformat(dev, wDevID, uDevTyp, &timef);
    if (res) return res;
    switch (timef) {
    case MCI_FORMAT_MILLISECONDS:
    case MCI_FORMAT_FRAMES:
    case MCI_FORMAT_BYTES:
    case MCI_FORMAT_SAMPLES:
	nrargs=1;
	parsestr="%d";
	break;
    case MCI_FORMAT_HMS:
    case MCI_FORMAT_MSF:
	parsestr="%d:%d:%d";
	nrargs=3;
	break;
    case MCI_FORMAT_TMSF:
	parsestr="%d:%d:%d:%d";
	nrargs=4;
	break;
    default:
	FIXME("unknown timeformat %d, please report.\n", timef);
	parsestr="%d";
	nrargs=1;
	break;
    }
    seekParams->dwCallback = hwndCallback;
    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "to") && (i+1 < nrofkeywords)) {
	    if (!STRCMP(keywords[i+1], "start")) {
		dwFlags |= MCI_SEEK_TO_START;
		seekParams->dwTo = 0;
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i+1], "end")) {
		dwFlags |= MCI_SEEK_TO_END;
		seekParams->dwTo = 0;
		i += 2;
		continue;
	    }
	    dwFlags |= MCI_TO;
	    i += 2;
	    a[0] = a[1] = a[2] = a[3] = 0;
	    j = sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    seekParams->dwTo = 0;
	    for (k = 0; k < j; k++)
		seekParams->dwTo += a[k] << (8*(nrargs-k));
	    continue;
	}
	switch (uDevTyp) {
	case MCI_DEVTYPE_VIDEODISC:
	    FLAG1("reverse", MCI_VD_SEEK_REVERSE);
	    break;
	}
	i++;
    }
    res = mciSendCommandA(wDevID, MCI_SEEK, dwFlags, (DWORD)seekParams);
    free(seekParams);
    return res;
}

/* close media/driver */
static DWORD
MCISTR_Close(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS*	closeParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int 		res;
    
    closeParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_CLOSE, dwFlags, (DWORD)closeParams);
    free(closeParams);
    return res;
}

/* return information.
 * Arguments:
 *	"product"	return product name (human readable)
 *	"file"		return filename
 * Animation:
 *	"text"		returns text?
 * Overlay:
 *	"text"		returns text?
 * Digitalvideo
 *	"audio algorithm"
 *	"audio quality"
 *	"still algorithm"
 *	"still quality"
 *	"usage"
 *	"version"
 *	"video algorithm"
 *	"video quality"
 *	"window text"
 */
static DWORD
MCISTR_Info(_MCISTR_PROTO_)
{
    union U {
	MCI_INFO_PARMSA		infoParams;
	MCI_DGV_INFO_PARMSA	dgvinfoParams;
    };
    union U*		pU = xmalloc(sizeof(union U));
    DWORD		sflags;
    int			i, res;
    
    sflags = dwFlags;
    for (i = 0; i < nrofkeywords; i++) {
	FLAG1("product", MCI_INFO_PRODUCT);
	FLAG1("file", MCI_INFO_FILE);
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    FLAG2("window", "text", MCI_ANIM_INFO_TEXT);
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    FLAG2("window", "text", MCI_OVLY_INFO_TEXT);
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
#define	MI1(str, flag) 					\
	if (!STRCMP(keywords[i], str)) {		\
		dwFlags |= MCI_DGV_INFO_ITEM;		\
                pU->dgvinfoParams.dwItem |= flag;	\
		i++;					\
		continue;				\
	}
#define MI2(str1, str2, flag) 				\
	if (!STRCMP(keywords[i], str1) &&		\
	    (i+1 < nrofkeywords) && 			\
            !STRCMP(keywords[i+1], str2)) {		\
		dwFlags |= MCI_DGV_INFO_ITEM;		\
                pU->dgvinfoParams.dwItem |= flag;	\
		i += 2;					\
		continue;				\
	}
	    MI2("audio", "algorithm", MCI_DGV_INFO_AUDIO_ALG);
	    MI2("audio", "quality", MCI_DGV_INFO_AUDIO_QUALITY);
	    MI2("still", "algorithm", MCI_DGV_INFO_STILL_ALG);
	    MI2("still", "quality", MCI_DGV_INFO_STILL_QUALITY);
	    MI1("usage", MCI_DGV_INFO_USAGE);
	    MI1("version", MCI_INFO_VERSION );
	    MI2("video", "algorithm", MCI_DGV_INFO_VIDEO_ALG);
	    MI2("video", "quality", MCI_DGV_INFO_VIDEO_QUALITY);
	    MI2("window", "text", MCI_DGV_INFO_TEXT);
#undef MI1
#undef MI2
	}
    }
    if (dwFlags == sflags)
	return MCIERR_MISSING_STRING_ARGUMENT;
    pU->infoParams.dwCallback = hwndCallback;

    /* MCI driver will fill in lpstrReturn, dwRetSize.
     * FIXME: I don't know if this is correct behaviour
     */
    res = mciSendCommandA(wDevID, MCI_INFO, dwFlags, (DWORD)pU);
    if (res == 0)
	_MCI_STR(pU->infoParams.lpstrReturn);
    free(pU);
    return res;
}

/* query MCI driver itself for information
 * Arguments:
 *	"installname"	return install name of <device> (system.ini)
 *	"quantity"	return nr of installed drivers
 *	"open"		open drivers only (additional flag)
 *	"name <nr>"	return nr of devices with <devicetyp>
 *	"name all"	return nr of all devices
 *
 */
static DWORD
MCISTR_Sysinfo(_MCISTR_PROTO_) {
    MCI_SYSINFO_PARMSA	sysinfoParams;
    int			i, res;
    
    sysinfoParams.lpstrReturn	= lpstrReturnString;
    sysinfoParams.dwRetSize	= uReturnLength;
    sysinfoParams.wDeviceType	= uDevTyp;
    sysinfoParams.dwCallback    = hwndCallback;

    for (i = 0; i < nrofkeywords; i++) {
	FLAG1("installname", MCI_SYSINFO_INSTALLNAME);
	FLAG1("quantity", MCI_SYSINFO_INSTALLNAME);
	FLAG1("open", MCI_SYSINFO_OPEN);
	if (!strcmp(keywords[i], "name") && (i+1 < nrofkeywords)) {
	    sscanf(keywords[i+1], "%ld", &(sysinfoParams.dwNumber));
	    dwFlags |= MCI_SYSINFO_NAME;
	    i++;
	}
    }
    res = mciSendCommandA(0, MCI_SYSINFO, dwFlags, (DWORD)&sysinfoParams);

    if (dwFlags & MCI_SYSINFO_QUANTITY) {
	char	buf[100];
	
	sprintf(buf, "%ld", *(long*)lpstrReturnString);
	_MCI_STR(buf);
    }
    /* no need to copy anything back, mciSysInfo did it for us */
    return res;
}

/* load file
 * Argument: "<filename>"
 * Overlay: "at <left> <top> <right> <bottom>" additional
 */
static DWORD
MCISTR_Load(_MCISTR_PROTO_) {
    union U {
	MCI_LOAD_PARMSA		loadParams;
	MCI_OVLY_LOAD_PARMSA	ovlyloadParams;
    };
    union 	U *pU = xmalloc(sizeof(union U));
    int		i, len = 0, res;
    char	*s;
    
    for (i = 0; i < nrofkeywords; ) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_OVERLAY:
	    if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
		dwFlags |= MCI_OVLY_RECT;
		sscanf(keywords[i+1], "%d", &(pU->ovlyloadParams.rc.left));
		sscanf(keywords[i+2], "%d", &(pU->ovlyloadParams.rc.top));
		sscanf(keywords[i+3], "%d", &(pU->ovlyloadParams.rc.right));
		sscanf(keywords[i+4], "%d", &(pU->ovlyloadParams.rc.bottom));
		memcpy(keywords+i, keywords+(i+5), nrofkeywords-(i+5));
		continue;
	    }
	    break;
	}
	len += strlen(keywords[i])+1;
	i++;
    }
    s=(char*)xmalloc(len);
    *s='\0';
    while (i < nrofkeywords) {
	strcat(s, keywords[i]);
	i++;
	if (i < nrofkeywords) strcat(s, " ");
    }
    pU->loadParams.lpfilename = s;
    pU->loadParams.dwCallback = hwndCallback;
    dwFlags |= MCI_LOAD_FILE;
    res = mciSendCommandA(wDevID, MCI_LOAD, dwFlags, (DWORD)pU);
    free(s);
    free(pU);
    return res;
}

/* save to file
 * Argument: "<filename>"
 * Overlay: "at <left> <top> <right> <bottom>" additional
 */
static DWORD
MCISTR_Save(_MCISTR_PROTO_) {
    union U {
	MCI_SAVE_PARMS		saveParams;
	MCI_OVLY_SAVE_PARMSA	ovlysaveParams;
    };
    union U*	pU = xmalloc(sizeof(union U));
    int		i, len = 0, res;
    char*	s;
    
    for (i = 0; i < nrofkeywords; ) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_OVERLAY:
	    if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
		dwFlags |= MCI_OVLY_RECT;
		sscanf(keywords[i+1], "%d", &(pU->ovlysaveParams.rc.left));
		sscanf(keywords[i+2], "%d", &(pU->ovlysaveParams.rc.top));
		sscanf(keywords[i+3], "%d", &(pU->ovlysaveParams.rc.right));
		sscanf(keywords[i+4], "%d", &(pU->ovlysaveParams.rc.bottom));
		memcpy(keywords+i, keywords+(i+5), nrofkeywords-(i+5));
		continue;
	    }
	    break;
	}
	len += strlen(keywords[i])+1;
	i++;
    }
    s = (char*)xmalloc(len);
    *s = '\0';
    while (i < nrofkeywords) {
	strcat(s, keywords[i]);
	i++;
	if (i < nrofkeywords) strcat(s, " ");
    }
    pU->saveParams.lpfilename = s;
    pU->saveParams.dwCallback = hwndCallback;
    dwFlags |= MCI_LOAD_FILE;
    res = mciSendCommandA(wDevID, MCI_SAVE, dwFlags, (DWORD)pU);
    free(s);
    free(pU);
    return res;
}

/* prepare device for input/output
 * (only applyable to waveform audio)
 */
static DWORD
MCISTR_Cue(_MCISTR_PROTO_) {
    MCI_GENERIC_PARMS	*cueParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int			i, res;
    
    for (i = 0; i < nrofkeywords; i++) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_WAVEFORM_AUDIO:
	    FLAG1("input", MCI_WAVE_INPUT);
	    FLAG1("output", MCI_WAVE_OUTPUT);
	    break;
	}
    }

    cueParams->dwCallback = hwndCallback;

    res = mciSendCommandA(wDevID, MCI_CUE, dwFlags, (DWORD)cueParams);
    free(cueParams);
    return res;
}

/* delete information */
static DWORD
MCISTR_Delete(_MCISTR_PROTO_) {
    int	timef, nrargs, i, j, k, a[4], res;
    char	*parsestr;
    MCI_WAVE_DELETE_PARMS *deleteParams = xmalloc(sizeof(MCI_WAVE_DELETE_PARMS));
    
    /* only implemented for waveform audio */
    if (uDevTyp != MCI_DEVTYPE_WAVEFORM_AUDIO)
	return MCIERR_UNSUPPORTED_FUNCTION; /* well it fits */
    res = _MCISTR_determine_timeformat(dev, wDevID, uDevTyp, &timef);
    if (res) return res;
    switch (timef) {
    case MCI_FORMAT_MILLISECONDS:
    case MCI_FORMAT_FRAMES:
    case MCI_FORMAT_BYTES:
    case MCI_FORMAT_SAMPLES:
	nrargs=1;
	parsestr="%d";
	break;
    case MCI_FORMAT_HMS:
    case MCI_FORMAT_MSF:
	parsestr="%d:%d:%d";
	nrargs=3;
	break;
    case MCI_FORMAT_TMSF:
	parsestr="%d:%d:%d:%d";
	nrargs=4;
	break;
    default:FIXME("unknown timeformat %d, please report.\n", timef);
	parsestr="%d";
	nrargs=1;
	break;
    }
    for (i = 0; i < nrofkeywords; ) {
	if (!strcmp(keywords[i], "to") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_TO;
	    a[0] = a[1] = a[2] = a[3] = 0;
	    j = sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* add up all integers we got, if we have more 
	     * shift them. (Well I should use the macros in 
	     * mmsystem.h, right).
	     */
	    deleteParams->dwTo = 0;
	    for (k = 0;k < j;k++)
		deleteParams->dwTo += a[k]<<(8*(nrargs-k));
	    i += 2;
	    continue;
	}
	if (!strcmp(keywords[i], "from") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_FROM;
	    a[0] = a[1] = a[2] = a[3] = 0;
	    j = sscanf(keywords[i+1], parsestr, &a[0], &a[1], &a[2], &a[3]);
	    /* dito. */
	    deleteParams->dwFrom = 0;
	    for (k = 0; k < j; k++)
		deleteParams->dwFrom += a[k]<<(8*(nrargs-k));
	    i += 2;
	    continue;
	}
	i++;
    }
    deleteParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_DELETE, dwFlags, (DWORD)deleteParams);
    free(deleteParams);
    return res;
}

/* send command to device. only applies to videodisc */
static DWORD
MCISTR_Escape(_MCISTR_PROTO_)
{
    LPMCI_VD_ESCAPE_PARMSA	escapeParams = xmalloc(sizeof(MCI_VD_ESCAPE_PARMSA));
    int			i, len = 0, res;
    char		*s;
    
    if (uDevTyp != MCI_DEVTYPE_VIDEODISC)
	return MCIERR_UNSUPPORTED_FUNCTION;

    for (i = 0; i < nrofkeywords; i++) {
	len += strlen(keywords[i]) + 1;
    }
    s = (char*)malloc(len);
    *s = '\0';
    for (i = 0; i < nrofkeywords; ) {
	strcat(s, keywords[i]);
	i++;
	if (i < nrofkeywords) strcat(s, " ");
    }
    escapeParams->lpstrCommand = s;
    escapeParams->dwCallback = hwndCallback;
    dwFlags |= MCI_VD_ESCAPE_STRING;
    res = mciSendCommandA(wDevID, MCI_ESCAPE, dwFlags, (DWORD)escapeParams);
    free(s);
    free(escapeParams);
    return res;
}

/* unfreeze [part of] the overlayed video 
 * only applyable to Overlay devices
 */
static DWORD
MCISTR_Unfreeze(_MCISTR_PROTO_)
{
    LPMCI_OVLY_RECT_PARMS	unfreezeParams = xmalloc(sizeof(MCI_OVLY_RECT_PARMS));
    int				i, res;
    
    if (uDevTyp != MCI_DEVTYPE_OVERLAY)
	return MCIERR_UNSUPPORTED_FUNCTION;
    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
	    sscanf(keywords[i+1], "%d", &(unfreezeParams->rc.left));
	    sscanf(keywords[i+2], "%d", &(unfreezeParams->rc.top));
	    sscanf(keywords[i+3], "%d", &(unfreezeParams->rc.right));
	    sscanf(keywords[i+4], "%d", &(unfreezeParams->rc.bottom));
	    dwFlags |= MCI_OVLY_RECT;
	    i += 5;
	    continue;
	}
	i++;
    }
    unfreezeParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_UNFREEZE, dwFlags, (DWORD)unfreezeParams);
    free(unfreezeParams);
    return res;
}
/* freeze [part of] the overlayed video 
 * only applyable to Overlay devices
 */
static DWORD
MCISTR_Freeze(_MCISTR_PROTO_)
{
    LPMCI_OVLY_RECT_PARMS	freezeParams = xmalloc(sizeof(MCI_OVLY_RECT_PARMS));
    int				i, res;
    
    if (uDevTyp != MCI_DEVTYPE_OVERLAY)
	return MCIERR_UNSUPPORTED_FUNCTION;
    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
	    sscanf(keywords[i+1], "%d", &(freezeParams->rc.left));
	    sscanf(keywords[i+2], "%d", &(freezeParams->rc.top));
	    sscanf(keywords[i+3], "%d", &(freezeParams->rc.right));
	    sscanf(keywords[i+4], "%d", &(freezeParams->rc.bottom));
	    dwFlags |= MCI_OVLY_RECT;
	    i += 5;
	    continue;
	}
	i++;
    }
    freezeParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_FREEZE, dwFlags, (DWORD)freezeParams);
    free(freezeParams);
    return res;
}

/* copy parts of image to somewhere else 
 * "source [at <left> <top> <right> <bottom>]"	source is framebuffer [or rect]
 * "destination [at <left> <top> <right> <bottom>]"	destination is framebuffer [or rect]
 * Overlay:
 * "frame [at <left> <top> <right> <bottom>]"	frame is framebuffer [or rect]
 * 						where the video input is placed
 * "video [at <left> <top> <right> <bottom>]"	video is whole video [or rect]
 *						(defining part of input to
 *						be displayed)
 * FIXME: This whole junk is passing multiple rectangles.
 *	I don't know how to do that with the present interface.
 *	(Means code below is broken)
 */
static DWORD
MCISTR_Put(_MCISTR_PROTO_) {
    union U {
	MCI_OVLY_RECT_PARMS	ovlyputParams;
	MCI_ANIM_RECT_PARMS	animputParams;
	MCI_DGV_RECT_PARMS	dgvputParams;
    };
    union U*	pU = xmalloc(sizeof(union U));
    int		i, res;
    
    for (i = 0; i < nrofkeywords; ) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    FLAG1("source", MCI_ANIM_PUT_SOURCE);
	    FLAG1("destination", MCI_ANIM_PUT_DESTINATION);
	    if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
		sscanf(keywords[i+1], "%d", &(pU->animputParams.rc.left));
		sscanf(keywords[i+2], "%d", &(pU->animputParams.rc.top));
		sscanf(keywords[i+3], "%d", &(pU->animputParams.rc.right));
		sscanf(keywords[i+4], "%d", &(pU->animputParams.rc.bottom));
		dwFlags |= MCI_ANIM_RECT;
		i += 5;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    FLAG1("source", MCI_OVLY_PUT_SOURCE);
	    FLAG1("destination", MCI_OVLY_PUT_DESTINATION);
	    FLAG1("video", MCI_OVLY_PUT_VIDEO);
	    FLAG1("frame", MCI_OVLY_PUT_FRAME);
	    if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
		sscanf(keywords[i+1], "%d", &(pU->ovlyputParams.rc.left));
		sscanf(keywords[i+2], "%d", &(pU->ovlyputParams.rc.top));
		sscanf(keywords[i+3], "%d", &(pU->ovlyputParams.rc.right));
		sscanf(keywords[i+4], "%d", &(pU->ovlyputParams.rc.bottom));
		dwFlags |= MCI_OVLY_RECT;
		i += 5;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	    FLAG1("source", MCI_DGV_PUT_SOURCE);
	    FLAG1("destination", MCI_DGV_PUT_DESTINATION);
	    FLAG1("video", MCI_DGV_PUT_VIDEO);
	    FLAG1("frame", MCI_DGV_PUT_FRAME);
	    if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
		sscanf(keywords[i+1], "%d", &(pU->dgvputParams.rc.left));
		sscanf(keywords[i+2], "%d", &(pU->dgvputParams.rc.top));
		sscanf(keywords[i+3], "%d", &(pU->dgvputParams.rc.right));
		sscanf(keywords[i+4], "%d", &(pU->dgvputParams.rc.bottom));
		dwFlags |= MCI_DGV_RECT;
		i += 5;
		continue;
	    }
	    break;
	    break;
	}
	i++;
    }
    pU->dgvputParams.dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_PUT, dwFlags, (DWORD)pU);
    free(pU);
    return res;
}

/* palette behaviour changing
 * (Animation only)
 * 	"normal"	realize the palette normally
 * 	"background"	realize the palette as background palette
 */
static DWORD
MCISTR_Realize(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS	*realizeParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int			i, res;
    
    if (uDevTyp != MCI_DEVTYPE_ANIMATION)
	return MCIERR_UNSUPPORTED_FUNCTION;
    for (i = 0; i < nrofkeywords; i++) {
	FLAG1("background", MCI_ANIM_REALIZE_BKGD);
	FLAG1("normal", MCI_ANIM_REALIZE_NORM);
    }
    realizeParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_REALIZE, dwFlags, (DWORD)realizeParams);
    free(realizeParams);
    return res;
}

/* Digitalvideo:
 *	"algorithm <algorithm>"
 *	"alignment to <integer>"
 *	"bass to <factor>"
 *	"bitspersample to <bit_count>"
 *	"bytespersec to <integer>"
 *	"clocktime"
 *	"input"
 *	"left off"
 *	"left on"
 *	"left volume to <factor>"
 *	"off"
 *	"on"
 *	"output"
 *	"over <duration>"
 *	"quality <descripto>r"
 *	"record off"
 *	"record on"
 *	"right off"
 *	"right on"
 *	"right volume to <factor>"
 *	"samplespersec to <integer>"
 *	"source to <sourcename>"
 *	"stream to <number>"
 *	"treble to <factor>"
 *	"volume to <factor>"
 * Vcr:
 *	"off"
 *	"on"
 *	"monitor to type number number"
 *	"record off"
 *	"record track track_number off"
 *	"record on "
 *	"record track track_number on "
 *	"source to type number number"
 *	"track track_number off"
 *	"track track_number on"
 */
static DWORD
MCISTR_SetAudio(_MCISTR_PROTO_)
{
    MCI_DGV_SETAUDIO_PARMSA	setaudioParams;
    int				i, res;

    if (uDevTyp != MCI_DEVTYPE_DIGITAL_VIDEO)
	return MCIERR_UNSUPPORTED_FUNCTION;

    setaudioParams.dwCallback =  hwndCallback;

    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "algorithm") && (i+1 < nrofkeywords)) {
	    setaudioParams.lpstrAlgorithm = strdup(keywords[i+1]);
	    dwFlags |= MCI_DGV_SETAUDIO_ALG;
	    i += 2;
	    continue;
	}
#define MSAI2(str1, str2, flag)							\
	if (  !STRCMP(keywords[i], str1) && (i+2 < nrofkeywords) && 		\
              !STRCMP(keywords[i+1], str2)) {					\
	    dwFlags |= MCI_DGV_SETAUDIO_ITEM;					\
	    setaudioParams.dwItem = flag;					\
	    sscanf(keywords[i+2], "%lu", &setaudioParams.dwValue);		\
	    i += 3;								\
	    continue;								\
	}
#define MSAI3(str1, str2, str3, flag)						\
	if (  !STRCMP(keywords[i], str1) && (i+3 < nrofkeywords) && 		\
              !STRCMP(keywords[i+1], str2) && !STRCMP(keywords[i+2], str3)) {	\
	    dwFlags |= MCI_DGV_SETAUDIO_ITEM;					\
	    setaudioParams.dwItem = flag;					\
	    sscanf(keywords[i+3], "%lu", &setaudioParams.dwValue);		\
	    i += 4;								\
	    continue;								\
	}
	MSAI2("alignment", "to", MCI_DGV_SETAUDIO_BLOCKALIGN);
	MSAI2("bass", "to", MCI_DGV_SETAUDIO_BASS);
	MSAI2("bitspersample", "to", MCI_DGV_SETAUDIO_BITSPERSAMPLE);
	MSAI2("bytespersec", "to", MCI_DGV_SETAUDIO_AVGBYTESPERSEC);
	MSAI2("samplespersec", "to", MCI_DGV_SETAUDIO_SAMPLESPERSEC);
	MSAI2("stream", "to", MCI_DGV_SETAUDIO_STREAM);
	MSAI2("treble", "to", MCI_DGV_SETAUDIO_TREBLE);
	MSAI2("volume", "to", MCI_DGV_SETAUDIO_VOLUME);
	MSAI3("input", "bass", "to", MCI_DGV_SETAUDIO_BASS|MCI_DGV_SETAUDIO_INPUT);
	MSAI3("input", "treble", "to", MCI_DGV_SETAUDIO_TREBLE|MCI_DGV_SETAUDIO_INPUT);
	MSAI3("input", "volume", "to", MCI_DGV_SETAUDIO_VOLUME|MCI_DGV_SETAUDIO_INPUT);
	MSAI3("output", "bass", "to", MCI_DGV_SETAUDIO_BASS|MCI_DGV_SETAUDIO_OUTPUT);
	MSAI3("output", "treble", "to", MCI_DGV_SETAUDIO_TREBLE|MCI_DGV_SETAUDIO_OUTPUT);
	MSAI3("output", "volume", "to", MCI_DGV_SETAUDIO_VOLUME|MCI_DGV_SETAUDIO_OUTPUT);

	FLAG1("clocktime", MCI_DGV_SETAUDIO_CLOCKTIME);
	FLAG2("left", "off", MCI_DGV_SETAUDIO_LEFT|MCI_SET_OFF);
	FLAG2("left", "on", MCI_DGV_SETAUDIO_LEFT|MCI_SET_ON);
	if (!STRCMP(keywords[i], "left") && (i+3 < nrofkeywords) && 
	    !STRCMP(keywords[i+1], "volume") && !STRCMP(keywords[i+2], "to")) {
	    dwFlags |= MCI_DGV_SETAUDIO_ITEM | MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_LEFT;
	    setaudioParams.dwItem = MCI_DGV_SETAUDIO_VOLUME;
	    sscanf(keywords[i+3], "%lu", &setaudioParams.dwValue);
	    i += 4;
	    continue;
	}
	FLAG1("off", MCI_SET_OFF);
	FLAG1("on", MCI_SET_ON);
	if (!STRCMP(keywords[i], "over") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_DGV_SETAUDIO_OVER;
	    sscanf(keywords[i+3], "%lu", &setaudioParams.dwOver);
	    i += 2;
	    continue;
	}
	if (!STRCMP(keywords[i], "quality") && (i+1 < nrofkeywords)) {
	    setaudioParams.lpstrQuality = strdup(keywords[i+1]);
	    dwFlags |= MCI_DGV_SETAUDIO_QUALITY;
	    i += 2;
	    continue;
	}
	FLAG2("record", "off", MCI_DGV_SETAUDIO_RECORD|MCI_SET_OFF);
	FLAG2("record", "on", MCI_DGV_SETAUDIO_RECORD|MCI_SET_ON);
	FLAG2("right", "off", MCI_DGV_SETAUDIO_RIGHT|MCI_SET_OFF);
	FLAG2("right", "on", MCI_DGV_SETAUDIO_RIGHT|MCI_SET_ON);
	if (!STRCMP(keywords[i], "right") && (i+3 < nrofkeywords) && 
	    !STRCMP(keywords[i+1], "volume") && !STRCMP(keywords[i+2], "to")) {
	    dwFlags |= MCI_DGV_SETAUDIO_ITEM | MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_RIGHT;
	    setaudioParams.dwItem = MCI_DGV_SETAUDIO_VOLUME;
	    sscanf(keywords[i+3], "%lu", &setaudioParams.dwValue);
	    i += 4;
	    continue;
	}
	if (!STRCMP(keywords[i], "source") && (i+2 < nrofkeywords) && !STRCMP(keywords[i+1], "to")) {
	    dwFlags |= MCI_DGV_SETAUDIO_ITEM;
	    setaudioParams.dwItem |= MCI_DGV_SETAUDIO_SOURCE;
	    if (!STRCMP(keywords[i+2], "left")) {
		setaudioParams.dwValue |= MCI_DGV_SETAUDIO_SOURCE_LEFT;
	    } else if (!STRCMP(keywords[i+2], "right")) {
		setaudioParams.dwValue |= MCI_DGV_SETAUDIO_SOURCE_RIGHT;
	    } else if (!STRCMP(keywords[i+2], "average")) {
		setaudioParams.dwValue |= MCI_DGV_SETAUDIO_SOURCE_AVERAGE;
	    } else if (!STRCMP(keywords[i+2], "stereo")) {
		setaudioParams.dwValue |= MCI_DGV_SETAUDIO_SOURCE_STEREO;
	    } else {
		res = MCIERR_UNSUPPORTED_FUNCTION;
		return res;
	    }
	    i += 3;
	    continue;
	}
    }
    res = mciSendCommandA(wDevID, MCI_SETAUDIO, dwFlags, (DWORD)&setaudioParams);
    free(setaudioParams.lpstrAlgorithm);
    free(setaudioParams.lpstrQuality);
    return res;
}

/* videodisc spinning
 *	"up"
 *	"down"
 */
static DWORD
MCISTR_Spin(_MCISTR_PROTO_)
{
    MCI_GENERIC_PARMS	*spinParams = xmalloc(sizeof(MCI_GENERIC_PARMS));
    int			i, res;
    
    if (uDevTyp != MCI_DEVTYPE_VIDEODISC)
	return MCIERR_UNSUPPORTED_FUNCTION;
    for (i = 0; i < nrofkeywords; i++) {
	FLAG1("up", MCI_VD_SPIN_UP);
	FLAG1("down", MCI_VD_SPIN_UP);
    }
    spinParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_SPIN, dwFlags, (DWORD)spinParams);
    free(spinParams);
    return res;
}

/* step single frames
 * 	"reverse"	optional flag
 *	"by <nr>"	for <nr> frames
 */
static DWORD
MCISTR_Step(_MCISTR_PROTO_) {
    union U {
	MCI_ANIM_STEP_PARMS	animstepParams;
	MCI_VD_STEP_PARMS	vdstepParams;
    };
    union U *pU = xmalloc(sizeof(union U));
    int	i, res;
    
    for (i = 0; i < nrofkeywords; ) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    FLAG1("reverse", MCI_ANIM_STEP_REVERSE);
	    if (!STRCMP(keywords[i], "by") && (i+1 < nrofkeywords)) {
		sscanf(keywords[i+1], "%ld", &(pU->animstepParams.dwFrames));
		dwFlags |= MCI_ANIM_STEP_FRAMES;
		i += 2;
		continue;
	    }
	    break;
	case MCI_DEVTYPE_VIDEODISC:
	    FLAG1("reverse", MCI_VD_STEP_REVERSE);
	    if (!STRCMP(keywords[i], "by") && (i+1 < nrofkeywords)) {
		sscanf(keywords[i+1], "%ld", &(pU->vdstepParams.dwFrames));
		dwFlags |= MCI_VD_STEP_FRAMES;
		i += 2;
		continue;
	    }
	    break;
	}
	i++;
    }
    pU->animstepParams.dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_STEP, dwFlags, (DWORD)pU);
    free(pU);
    return res;
}

/* update animation window
 * Arguments:
 *	"at <left> <top> <right> <bottom>" only in this rectangle
 *	"hdc"		device context
 */
static DWORD
MCISTR_Update(_MCISTR_PROTO_) {
    int		i, res;
    LPMCI_ANIM_UPDATE_PARMS	updateParams = xmalloc(sizeof(MCI_ANIM_UPDATE_PARMS));
    
    for (i = 0; i < nrofkeywords; ) {
	if (!STRCMP(keywords[i], "at") && (i+4 < nrofkeywords)) {
	    sscanf(keywords[i+1], "%d", &(updateParams->rc.left));
	    sscanf(keywords[i+2], "%d", &(updateParams->rc.top));
	    sscanf(keywords[i+3], "%d", &(updateParams->rc.right));
	    sscanf(keywords[i+4], "%d", &(updateParams->rc.bottom));
	    dwFlags |= MCI_ANIM_RECT;
	    i += 5;
	    continue;
	}
	if (!STRCMP(keywords[i], "hdc") && (i+1 < nrofkeywords)) {
	    dwFlags |= MCI_ANIM_UPDATE_HDC;
	    sscanf(keywords[i+1], "%d", &(updateParams->hDC));
	    i += 2;
	    continue;
	}
	i++;
    }
    updateParams->dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_UPDATE, dwFlags, (DWORD)updateParams);
    free(updateParams);
    return res;
}

/* where command for animation and overlay drivers.
 * just returns the specified rectangle as a string
 * Arguments:
 *	"source"
 *	"destination"
 * Overlay special:
 *	"video"
 *	"frame"
 */
static DWORD
MCISTR_Where(_MCISTR_PROTO_) {
    union U {
	MCI_ANIM_RECT_PARMS	animwhereParams;
	MCI_OVLY_RECT_PARMS	ovlywhereParams;
    };
    union U*	pU = xmalloc(sizeof(union U));
    int		i, res;
    
    for (i = 0; i < nrofkeywords; i++) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    FLAG1("source", MCI_ANIM_WHERE_SOURCE);
	    FLAG1("destination", MCI_ANIM_WHERE_DESTINATION);
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    FLAG1("source", MCI_OVLY_WHERE_SOURCE);
	    FLAG1("destination", MCI_OVLY_WHERE_DESTINATION);
	    FLAG1("video", MCI_OVLY_WHERE_VIDEO);
	    FLAG1("frame", MCI_OVLY_WHERE_FRAME);
	    break;
	}
    }
    pU->animwhereParams.dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_WHERE, dwFlags, (DWORD)pU);
    if (res == 0) {
	char	buf[100];
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    sprintf(buf, "%d %d %d %d",
		    pU->animwhereParams.rc.left,
		    pU->animwhereParams.rc.top,
		    pU->animwhereParams.rc.right,
		    pU->animwhereParams.rc.bottom
		   );
	    break;
	case MCI_DEVTYPE_OVERLAY:
	    sprintf(buf, "%d %d %d %d",
		    pU->ovlywhereParams.rc.left,
		    pU->ovlywhereParams.rc.top,
		    pU->ovlywhereParams.rc.right,
		    pU->ovlywhereParams.rc.bottom
		   );
	    break;
	default:strcpy(buf, "0 0 0 0");break;
	}
	_MCI_STR(buf);
    }
    free(pU);
    return	res;
}

static DWORD
MCISTR_Window(_MCISTR_PROTO_) {
    union U {
	MCI_ANIM_WINDOW_PARMSA	animwindowParams;
	MCI_OVLY_WINDOW_PARMSA	ovlywindowParams;
	MCI_DGV_WINDOW_PARMSA	dgvwindowParams;
    };
    union U*	pU = xmalloc(sizeof(union U));
    int		i, res;
    char*	s;
    
    s = NULL;
    for (i = 0; i < nrofkeywords; ) {
	switch (uDevTyp) {
	case MCI_DEVTYPE_ANIMATION:
	    if (!STRCMP(keywords[i], "handle") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_ANIM_WINDOW_HWND;
		if (!STRCMP(keywords[i+1], "default")) 
		    pU->animwindowParams.hWnd = MCI_OVLY_WINDOW_DEFAULT;
		else
		    sscanf(keywords[i+1], "%d", &(pU->animwindowParams.hWnd));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "state") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_ANIM_WINDOW_STATE;
		if (!STRCMP(keywords[i+1], "hide"))
		    pU->animwindowParams.nCmdShow = SW_HIDE;
		if (!STRCMP(keywords[i+1], "iconic"))
		    pU->animwindowParams.nCmdShow = SW_SHOWMINNOACTIVE; /* correct? */
		if (!STRCMP(keywords[i+1], "minimized"))
		    pU->animwindowParams.nCmdShow = SW_SHOWMINIMIZED;
		if (!STRCMP(keywords[i+1], "maximized"))
		    pU->animwindowParams.nCmdShow = SW_SHOWMAXIMIZED;
		if (!STRCMP(keywords[i+1], "minimize"))
		    pU->animwindowParams.nCmdShow = SW_MINIMIZE;
		if (!STRCMP(keywords[i+1], "normal"))
		    pU->animwindowParams.nCmdShow = SW_NORMAL;
		if (!STRCMP(keywords[i+1], "restore"))
		    pU->animwindowParams.nCmdShow = SW_NORMAL;
		if (!STRCMP(keywords[i+1], "show"))
		    pU->animwindowParams.nCmdShow = SW_SHOW;
		if (!STRCMP(keywords[i+1], "no") && (i+2 < nrofkeywords)) {
		    if (!STRCMP(keywords[i+2], "active"))
			pU->animwindowParams.nCmdShow = SW_SHOWNOACTIVATE;
		    if (!STRCMP(keywords[i+2], "action"))
			pU->animwindowParams.nCmdShow = SW_SHOWNA;/* correct?*/
		    i++;
		}
		i += 2;
		continue;
	    }
	    /* text is enclosed in " ... " as it seems */
	    if (!STRCMP(keywords[i], "text")) {
		if (keywords[i+1][0] != '"') {
		    i++;
		    continue;
		}
		dwFlags |= MCI_ANIM_WINDOW_TEXT;
		pU->animwindowParams.lpstrText = _MCISTR_Unquote(keywords[i+1]);
		i += 2;
		continue;
	    }
	    FLAG1("stretch", MCI_ANIM_WINDOW_ENABLE_STRETCH);
	    break;
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	    if (!STRCMP(keywords[i], "handle") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_DGV_WINDOW_HWND;
		if (!STRCMP(keywords[i+1], "default")) 
		    pU->dgvwindowParams.hWnd = MCI_OVLY_WINDOW_DEFAULT;
		else
		    sscanf(keywords[i+1], "%d", &(pU->dgvwindowParams.hWnd));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "state") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_DGV_WINDOW_STATE;
		if (!STRCMP(keywords[i+1], "hide"))
		    pU->dgvwindowParams.nCmdShow = SW_HIDE;
		if (!STRCMP(keywords[i+1], "iconic"))
		    pU->dgvwindowParams.nCmdShow = SW_SHOWMINNOACTIVE; /* correct? */
		if (!STRCMP(keywords[i+1], "minimized"))
		    pU->dgvwindowParams.nCmdShow = SW_SHOWMINIMIZED;
		if (!STRCMP(keywords[i+1], "maximized"))
		    pU->dgvwindowParams.nCmdShow = SW_SHOWMAXIMIZED;
		if (!STRCMP(keywords[i+1], "minimize"))
		    pU->dgvwindowParams.nCmdShow = SW_MINIMIZE;
		if (!STRCMP(keywords[i+1], "normal"))
		    pU->dgvwindowParams.nCmdShow = SW_NORMAL;
		if (!STRCMP(keywords[i+1], "restore"))
		    pU->dgvwindowParams.nCmdShow = SW_NORMAL;
		if (!STRCMP(keywords[i+1], "show"))
		    pU->dgvwindowParams.nCmdShow = SW_SHOW;
		if (!STRCMP(keywords[i+1], "no") && (i+2 < nrofkeywords)) {
		    if (!STRCMP(keywords[i+2], "active"))
			pU->dgvwindowParams.nCmdShow = SW_SHOWNOACTIVATE;
		    if (!STRCMP(keywords[i+2], "action"))
			pU->dgvwindowParams.nCmdShow = SW_SHOWNA;/* correct?*/
		    i++;
		}
		i += 2;
		continue;
	    }
	    /* text is enclosed in " ... " as it seems */
	    if (!STRCMP(keywords[i], "text")) {
		if (keywords[i+1][0] != '"') {
		    i++;
		    continue;
		}
		dwFlags |= MCI_DGV_WINDOW_TEXT;
		pU->dgvwindowParams.lpstrText = _MCISTR_Unquote(keywords[i+1]);
		i += 2;
		continue;
	    }
	    break;

	case MCI_DEVTYPE_OVERLAY:
	    if (!STRCMP(keywords[i], "handle") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_OVLY_WINDOW_HWND;
		if (!STRCMP(keywords[i+1], "default")) 
		    pU->ovlywindowParams.hWnd = MCI_OVLY_WINDOW_DEFAULT;
		else
		    sscanf(keywords[i+1], "%d", &(pU->ovlywindowParams.hWnd));
		i += 2;
		continue;
	    }
	    if (!STRCMP(keywords[i], "state") && (i+1 < nrofkeywords)) {
		dwFlags |= MCI_OVLY_WINDOW_STATE;
		if (!STRCMP(keywords[i+1], "hide"))
		    pU->ovlywindowParams.nCmdShow = SW_HIDE;
		if (!STRCMP(keywords[i+1], "iconic"))
		    pU->ovlywindowParams.nCmdShow = SW_SHOWMINNOACTIVE; /* correct? */
		if (!STRCMP(keywords[i+1], "minimized"))
		    pU->ovlywindowParams.nCmdShow = SW_SHOWMINIMIZED;
		if (!STRCMP(keywords[i+1], "maximized"))
		    pU->ovlywindowParams.nCmdShow = SW_SHOWMAXIMIZED;
		if (!STRCMP(keywords[i+1], "minimize"))
		    pU->ovlywindowParams.nCmdShow = SW_MINIMIZE;
		if (!STRCMP(keywords[i+1], "normal"))
		    pU->ovlywindowParams.nCmdShow = SW_NORMAL;
		if (!STRCMP(keywords[i+1], "show"))
		    pU->ovlywindowParams.nCmdShow = SW_SHOW;
		if (!STRCMP(keywords[i+1], "no") && (i+2 < nrofkeywords)) {
		    if (!STRCMP(keywords[i+2], "active"))
			pU->ovlywindowParams.nCmdShow = SW_SHOWNOACTIVATE;
		    if (!STRCMP(keywords[i+2], "action"))
			pU->ovlywindowParams.nCmdShow = SW_SHOWNA;/* correct?*/
		    i++;
		}
		i += 2;
		continue;
	    }
	    /* text is enclosed in " ... " as it seems */
	    if (!STRCMP(keywords[i], "text")) {
		if (keywords[i+1][0] != '"') {
		    i++;
		    continue;
		}
		dwFlags |= MCI_OVLY_WINDOW_TEXT;
		pU->ovlywindowParams.lpstrText = _MCISTR_Unquote(keywords[i+1]);
		i += 2;
		continue;
	    }
	    FLAG1("stretch", MCI_OVLY_WINDOW_ENABLE_STRETCH);
	    break;
	}
	i++;
    }
    pU->animwindowParams.dwCallback = hwndCallback;
    res = mciSendCommandA(wDevID, MCI_WINDOW, dwFlags, (DWORD)pU);
    if (s) free(s);
    free(pU);
    return res;
}

struct	_MCISTR_cmdtable {
    char	*cmd;
    DWORD	(*fun)(_MCISTR_PROTO_);
} MCISTR_cmdtable[]={
    {"break",		MCISTR_Break},
    {"capability",	MCISTR_Capability},
    {"close",		MCISTR_Close},
    {"cue",		MCISTR_Cue},
    {"delete",		MCISTR_Delete},
    {"escape",		MCISTR_Escape},
    {"freeze",		MCISTR_Freeze},
    {"info",		MCISTR_Info},
    {"load",		MCISTR_Load},
    {"open",		MCISTR_Open},
    {"pause",		MCISTR_Pause},
    {"play",		MCISTR_Play},
    {"put",		MCISTR_Put},
    {"realize",		MCISTR_Realize},
    {"record",		MCISTR_Record},
    {"resume",		MCISTR_Resume},
    {"save",		MCISTR_Save},
    {"seek",		MCISTR_Seek},
    {"set",		MCISTR_Set},
    {"setaudio",	MCISTR_SetAudio},
    {"spin",		MCISTR_Spin},
    {"status",		MCISTR_Status},
    {"step",		MCISTR_Step},
    {"stop",		MCISTR_Stop},
    {"sysinfo",		MCISTR_Sysinfo},
    {"unfreeze",	MCISTR_Unfreeze},
    {"update",		MCISTR_Update},
    {"where",		MCISTR_Where},
    {"window",		MCISTR_Window},
    {NULL,		NULL}
};

/**************************************************************************
 * 				mciSendString16			[MMSYSTEM.702]
 */
/* The usercode sends a string with a command (and flags) expressed in 
 * words in it... We do our best to call appropriate drivers,
 * and return a errorcode AND a readable string (if lpstrRS != NULL)
 * Info gathered by watching cool134.exe and from Borland's mcistrwh.hlp
 */
/* FIXME: "all" is a valid devicetype and we should access all devices if
 * it is used. (imagine "close all"). Not implemented yet.
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrReturnString, 
			     UINT16 uReturnLength, HWND16 hwndCallback)
{
    char	*cmd, *dev, *args, **keywords;
    WORD	uDevTyp = 0, wDevID = 0;
    DWORD	dwFlags;
    int		res = 0, i, nrofkeywords;
    
    TRACE("('%s', %p, %d, %X)\n", 
	  lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);

    /* format is <command> <device> <optargs> */
    cmd = strdup(lpstrCommand);
    dev = strchr(cmd, ' ');
    if (dev == NULL) {
	free(cmd);
	return MCIERR_MISSING_DEVICE_NAME;
    }
    *dev++ = '\0';
    args = strchr(dev, ' ');

    if (args != NULL) {
	char	*s;

	*args = '\0';
	while (*++args == ' ');

	i = 1;/* nrofkeywords = nrofspaces+1 */
	s = args;
	while ((s = strchr(s, ' ')) != NULL) {
	    i++;
	    while (*++s == ' ');
	    /* see if we have a quoted string */
	    if (*s == '"') {
		s = strchr(s+1, '"');
		if (!s || s[1] != ' ') {
		    /* missed closing '"'*/
		    free(cmd);
		    return MCIERR_NO_CLOSING_QUOTE;
		}
	    }
	}
	keywords = (char**)xmalloc(sizeof(char*) * (i + 2));
	nrofkeywords = i;
	s = args; 
	i = 0;
	while (s && i < nrofkeywords) {
	    keywords[i++] = s;
	    if (*s == '"') {
		if ((s = strchr(s+1, '"')) != NULL) s++;
	    } else {
		s = strchr(s, ' ');
	    }
	    if (s) {
		*s = '\0';
		while (*++s == ' ');
	    }
	    TRACE("[%d] => '%s'\n", i-1, keywords[i-1]);
	}
	keywords[i] = NULL;
    } else {
	nrofkeywords = 0;
	keywords = (char**)xmalloc(sizeof(char*));
	keywords[0] = NULL;
    }
    dwFlags = 0; /* default flags */
    for (i = 0; i < nrofkeywords;) {
	if (!STRCMP(keywords[i], "wait")) {
	    dwFlags |= MCI_WAIT;
	    memcpy(keywords+i, keywords+(i+1), (nrofkeywords-i-1) * sizeof(char*));
	    nrofkeywords--;
	    continue;
	}
	if (!STRCMP(keywords[i], "notify")) {
	    dwFlags |= MCI_NOTIFY;
	    memcpy(keywords+i, keywords+(i+1), (nrofkeywords-i-1) * sizeof(char*));
	    nrofkeywords--;
	    continue;
	}
	i++;
    }

    /* determine wDevID and uDevTyp for all commands except "open" */
    if (STRCMP(cmd, "open") != 0) {
	wDevID = mciGetDeviceIDA(dev);
	if (wDevID == 0) {
	    TRACE("Device '%s' not found!\n", dev);
	    res = MCIERR_INVALID_DEVICE_NAME;
	    goto the_end;
	}
	uDevTyp = MCI_GetDrv(wDevID)->modp.wType;
    }

    if (lpstrReturnString && uReturnLength > 0)
	*lpstrReturnString = 0;

    for (i = 0; MCISTR_cmdtable[i].cmd != NULL; i++) {
	if (!STRCMP(MCISTR_cmdtable[i].cmd, cmd)) {
	    res = MCISTR_cmdtable[i].fun(wDevID, uDevTyp, lpstrReturnString,
					 uReturnLength, dev, (LPSTR*)keywords, 
					 nrofkeywords, dwFlags, hwndCallback);
	    break;
	}
    }
    if (MCISTR_cmdtable[i].cmd == NULL) {
	FIXME("('%s', %p, %u, %X): unimplemented, please report.\n", 
	      lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
	res = MCIERR_MISSING_COMMAND_STRING;
    }
the_end:
    free(keywords); free(cmd);
    return res;
}

/**************************************************************************
 * 				mciSendStringA		[MMSYSTEM.702][WINMM.51]
 */
DWORD WINAPI mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrReturnString, 
			    UINT uReturnLength, HWND hwndCallback)
{
    return mciSendString16(lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
}

/**************************************************************************
 * 				mciSendStringW			[WINMM.52]
 */
DWORD WINAPI mciSendStringW(LPCWSTR lpwstrCommand, LPSTR lpstrReturnString, 
			    UINT uReturnLength, HWND hwndCallback)
{
    LPSTR 	lpstrCommand;
    UINT	ret;

    /* FIXME: is there something to do with lpstrReturnString ? */
    lpstrCommand = HEAP_strdupWtoA(GetProcessHeap(), 0, lpwstrCommand);
    ret = mciSendString16(lpstrCommand, lpstrReturnString, uReturnLength, hwndCallback);
    HeapFree(GetProcessHeap(), 0, lpstrCommand);
    return ret;
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
	if (!mciGetErrorString16(ret, strRet, sizeof(strRet))) {
	    sprintf(strRet, "Unknown MCI error (%ld)", ret);
	}
	MessageBoxA(0, strRet, "Error in mciExecute()", MB_OK); 
    }
    /* FIXME: what shall I return ? */
    return 0;
}
