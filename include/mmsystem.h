/* 
 * MMSYSTEM - Multimedia Wine Extension ... :-)
 */

#ifndef __WINE_MMSYSTEM_H
#define __WINE_MMSYSTEM_H

typedef LPSTR		    HPSTR;          /* a huge version of LPSTR */
typedef LPCSTR			HPCSTR;         /* a huge version of LPCSTR */

#pragma pack(1)

#define MAXWAVEDRIVERS	10
#define MAXMIDIDRIVERS	10
#define MAXAUXDRIVERS	10
#define MAXMCIDRIVERS	32
#define MAXMIXERDRIVERS	10

#define MAXPNAMELEN      32     /* max product name length (including NULL) */
#define MAXERRORLENGTH   128    /* max error text length (including NULL) */
#define MAX_JOYSTICKOEMVXDNAME	260

typedef WORD    VERSION;        /* major (high byte), minor (low byte) */

typedef UINT16	MMVERSION16;
typedef UINT32	MMVERSION32;
DECL_WINELIB_TYPE(MMVERSION);
typedef UINT16	MCIDEVICEID16;
typedef UINT32	MCIDEVICEID32;
DECL_WINELIB_TYPE(MCIDEVICEID);
typedef	UINT16	MMRESULT16;
typedef	UINT32	MMRESULT32;
DECL_WINELIB_TYPE(MMRESULT);

typedef struct {
    UINT16    wType;		/* indicates the contents of the union */
    union {
	DWORD ms;		/* milliseconds */
	DWORD sample;		/* samples */
	DWORD cb;		/* byte count */
	struct {		/* SMPTE */
	    BYTE hour;		/* hours */
	    BYTE min;		/* minutes */
	    BYTE sec;		/* seconds */
	    BYTE frame;		/* frames  */
	    BYTE fps;		/* frames per second */
	    BYTE dummy;		/* pad */
	} smpte;
	struct {		/* MIDI */
	    DWORD songptrpos;	/* song pointer position */
	} midi;
    } u;
} MMTIME16,  *LPMMTIME16;

typedef struct {
    UINT32    wType;
    union {
	DWORD ms;
	DWORD sample;
	DWORD cb;
	struct {
	    BYTE hour;
	    BYTE min;
	    BYTE sec;
	    BYTE frame;
	    BYTE fps;
	    BYTE dummy;
	    BYTE pad[2];
	} smpte;
	struct {
	    DWORD songptrpos;
	} midi;
    } u;
} MMTIME32,  *LPMMTIME32;
DECL_WINELIB_TYPE(MMTIME);
DECL_WINELIB_TYPE(LPMMTIME);

#define TIME_MS         0x0001  /* time in milliseconds */
#define TIME_SAMPLES    0x0002  /* number of wave samples */
#define TIME_BYTES      0x0004  /* current byte offset */
#define TIME_SMPTE      0x0008  /* SMPTE time */
#define TIME_MIDI       0x0010  /* MIDI time */

#define MM_JOY1MOVE         0x3A0           /* joystick */
#define MM_JOY2MOVE         0x3A1
#define MM_JOY1ZMOVE        0x3A2
#define MM_JOY2ZMOVE        0x3A3
#define MM_JOY1BUTTONDOWN   0x3B5
#define MM_JOY2BUTTONDOWN   0x3B6
#define MM_JOY1BUTTONUP     0x3B7
#define MM_JOY2BUTTONUP     0x3B8

#define MM_MCINOTIFY        0x3B9           /* MCI */

#define MM_WOM_OPEN         0x3BB           /* waveform output */
#define MM_WOM_CLOSE        0x3BC
#define MM_WOM_DONE         0x3BD

#define MM_WIM_OPEN         0x3BE           /* waveform input */
#define MM_WIM_CLOSE        0x3BF
#define MM_WIM_DATA         0x3C0

#define MM_MIM_OPEN         0x3C1           /* MIDI input */
#define MM_MIM_CLOSE        0x3C2
#define MM_MIM_DATA         0x3C3
#define MM_MIM_LONGDATA     0x3C4
#define MM_MIM_ERROR        0x3C5
#define MM_MIM_LONGERROR    0x3C6

#define MM_MOM_OPEN         0x3C7           /* MIDI output */
#define MM_MOM_CLOSE        0x3C8
#define MM_MOM_DONE         0x3C9


#define MMSYSERR_BASE          0
#define WAVERR_BASE            32
#define MIDIERR_BASE           64
#define TIMERR_BASE            96
#define JOYERR_BASE            160
#define MCIERR_BASE            256

#define MCI_STRING_OFFSET      512
#define MCI_VD_OFFSET          1024
#define MCI_CD_OFFSET          1088
#define MCI_WAVE_OFFSET        1152
#define MCI_SEQ_OFFSET         1216

#define MMSYSERR_NOERROR      0                    /* no error */
#define MMSYSERR_ERROR        (MMSYSERR_BASE + 1)  /* unspecified error */
#define MMSYSERR_BADDEVICEID  (MMSYSERR_BASE + 2)  /* device ID out of range */
#define MMSYSERR_NOTENABLED   (MMSYSERR_BASE + 3)  /* driver failed enable */
#define MMSYSERR_ALLOCATED    (MMSYSERR_BASE + 4)  /* device already allocated */
#define MMSYSERR_INVALHANDLE  (MMSYSERR_BASE + 5)  /* device handle is invalid */
#define MMSYSERR_NODRIVER     (MMSYSERR_BASE + 6)  /* no device driver present */
#define MMSYSERR_NOMEM        (MMSYSERR_BASE + 7)  /* memory allocation error */
#define MMSYSERR_NOTSUPPORTED (MMSYSERR_BASE + 8)  /* function isn't supported */
#define MMSYSERR_BADERRNUM    (MMSYSERR_BASE + 9)  /* error value out of range */
#define MMSYSERR_INVALFLAG    (MMSYSERR_BASE + 10) /* invalid flag passed */
#define MMSYSERR_INVALPARAM   (MMSYSERR_BASE + 11) /* invalid parameter passed */
#define MMSYSERR_LASTERROR    (MMSYSERR_BASE + 11) /* last error in range */

#define CALLBACK_TYPEMASK   0x00070000l    /* callback type mask */
#define CALLBACK_NULL       0x00000000l    /* no callback */
#define CALLBACK_WINDOW     0x00010000l    /* dwCallback is a HWND */
#define CALLBACK_TASK       0x00020000l    /* dwCallback is a HTASK */
#define CALLBACK_FUNCTION   0x00030000l    /* dwCallback is a FARPROC */

typedef void (CALLBACK *LPDRVCALLBACK) (HDRVR16 h, UINT16 uMessage, DWORD dwUser, DWORD dw1, DWORD dw2);

#define MM_MICROSOFT            1       /* Microsoft Corp. */

#define MM_MIDI_MAPPER          1       /* MIDI Mapper */
#define MM_WAVE_MAPPER          2       /* Wave Mapper */

#define MM_SNDBLST_MIDIOUT      3       /* Sound Blaster MIDI output port */
#define MM_SNDBLST_MIDIIN       4       /* Sound Blaster MIDI input port  */
#define MM_SNDBLST_SYNTH        5       /* Sound Blaster internal synthesizer */
#define MM_SNDBLST_WAVEOUT      6       /* Sound Blaster waveform output */
#define MM_SNDBLST_WAVEIN       7       /* Sound Blaster waveform input */

#define MM_ADLIB                9       /* Ad Lib-compatible synthesizer */

#define MM_MPU401_MIDIOUT       10      /* MPU401-compatible MIDI output port */
#define MM_MPU401_MIDIIN        11      /* MPU401-compatible MIDI input port */

#define MM_PC_JOYSTICK          12      /* Joystick adapter */


UINT16 WINAPI mmsystemGetVersion16(void);
UINT32 WINAPI mmsystemGetVersion32(void);
#define mmsystemGetVersion WINELIB_NAME(mmsystemGetVersion)
BOOL16 WINAPI sndPlaySound(LPCSTR lpszSoundName, UINT16 uFlags);
BOOL32 WINAPI PlaySound32A(LPCSTR pszSound, HMODULE32 hmod, DWORD fdwSound);
BOOL32 WINAPI PlaySound32W(LPCWSTR pszSound, HMODULE32 hmod, DWORD fdwSound);
#define PlaySound WINELIB_NAME_AW(PlaySound)

#define SND_SYNC            0x0000  /* play synchronously (default) */
#define SND_ASYNC           0x0001  /* play asynchronously */
#define SND_NODEFAULT       0x0002  /* don't use default sound */
#define SND_MEMORY          0x0004  /* lpszSoundName points to a memory file */
#define SND_LOOP            0x0008  /* loop the sound until next sndPlaySound */
#define SND_NOSTOP          0x0010  /* don't stop any currently playing sound */

#define SND_NOWAIT	0x00002000L /* don't wait if the driver is busy */
#define SND_ALIAS       0x00010000L /* name is a registry alias */
#define SND_ALIAS_ID	0x00110000L /* alias is a predefined ID */
#define SND_FILENAME    0x00020000L /* name is file name */
#define SND_RESOURCE    0x00040004L /* name is resource name or atom */

/* waveform audio error return values */
#define WAVERR_BADFORMAT      (WAVERR_BASE + 0)    /* unsupported wave format */
#define WAVERR_STILLPLAYING   (WAVERR_BASE + 1)    /* still something playing */
#define WAVERR_UNPREPARED     (WAVERR_BASE + 2)    /* header not prepared */
#define WAVERR_SYNC           (WAVERR_BASE + 3)    /* device is synchronous */
#define WAVERR_LASTERROR      (WAVERR_BASE + 3)    /* last error in range */

typedef HWAVEIN16 *LPHWAVEIN16;
typedef HWAVEOUT16 *LPHWAVEOUT16;
typedef LPDRVCALLBACK LPWAVECALLBACK;
typedef HMIXER16 *LPHMIXER16;
typedef HMIXER32 *LPHMIXER32;

#define WOM_OPEN        MM_WOM_OPEN
#define WOM_CLOSE       MM_WOM_CLOSE
#define WOM_DONE        MM_WOM_DONE
#define WIM_OPEN        MM_WIM_OPEN
#define WIM_CLOSE       MM_WIM_CLOSE
#define WIM_DATA        MM_WIM_DATA

#define WAVE_MAPPER     (-1)

#define  WAVE_FORMAT_QUERY     0x0001
#define  WAVE_ALLOWSYNC        0x0002

typedef struct wavehdr_tag {
    LPSTR       lpData;		/* pointer to locked data buffer */
    DWORD       dwBufferLength;	/* length of data buffer */
    DWORD       dwBytesRecorded;/* used for input only */
    DWORD       dwUser;		/* for client's use */
    DWORD       dwFlags;	/* assorted flags (see defines) */
    DWORD       dwLoops;	/* loop control counter */

    struct wavehdr_tag *lpNext;	/* reserved for driver */
    DWORD       reserved;	/* reserved for driver */
} WAVEHDR, *LPWAVEHDR;

#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
#define WHDR_ENDLOOP    0x00000008  /* loop end block */
#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of sources supported */
    DWORD	dwSupport;		/* functionality supported by driver */
} WAVEOUTCAPS16, *LPWAVEOUTCAPS16;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of sources supported */
    WORD	wReserved1;		/* padding */
    DWORD	dwSupport;		/* functionality supported by driver */
} WAVEOUTCAPS32A, *LPWAVEOUTCAPS32A;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    WCHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of sources supported */
    WORD	wReserved1;		/* padding */
    DWORD	dwSupport;		/* functionality supported by driver */
} WAVEOUTCAPS32W, *LPWAVEOUTCAPS32W;
DECL_WINELIB_TYPE_AW(WAVEOUTCAPS);
DECL_WINELIB_TYPE_AW(LPWAVEOUTCAPS);

#define WAVECAPS_PITCH          0x0001   /* supports pitch control */
#define WAVECAPS_PLAYBACKRATE   0x0002   /* supports playback rate control */
#define WAVECAPS_VOLUME         0x0004   /* supports volume control */
#define WAVECAPS_LRVOLUME       0x0008   /* separate left-right volume control */
#define WAVECAPS_SYNC           0x0010

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of channels supported */
} WAVEINCAPS16, *LPWAVEINCAPS16;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of channels supported */
    WORD	wReserved1;
} WAVEINCAPS32A, *LPWAVEINCAPS32A;
typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    WCHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of channels supported */
    WORD	wReserved1;
} WAVEINCAPS32W, *LPWAVEINCAPS32W;
DECL_WINELIB_TYPE_AW(WAVEINCAPS);
DECL_WINELIB_TYPE_AW(LPWAVEINCAPS);

#define WAVE_INVALIDFORMAT     0x00000000       /* invalid format */
#define WAVE_FORMAT_1M08       0x00000001       /* 11.025 kHz, Mono,   8-bit  */
#define WAVE_FORMAT_1S08       0x00000002       /* 11.025 kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_1M16       0x00000004       /* 11.025 kHz, Mono,   16-bit */
#define WAVE_FORMAT_1S16       0x00000008       /* 11.025 kHz, Stereo, 16-bit */
#define WAVE_FORMAT_2M08       0x00000010       /* 22.05  kHz, Mono,   8-bit  */
#define WAVE_FORMAT_2S08       0x00000020       /* 22.05  kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_2M16       0x00000040       /* 22.05  kHz, Mono,   16-bit */
#define WAVE_FORMAT_2S16       0x00000080       /* 22.05  kHz, Stereo, 16-bit */
#define WAVE_FORMAT_4M08       0x00000100       /* 44.1   kHz, Mono,   8-bit  */
#define WAVE_FORMAT_4S08       0x00000200       /* 44.1   kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_4M16       0x00000400       /* 44.1   kHz, Mono,   16-bit */
#define WAVE_FORMAT_4S16       0x00000800       /* 44.1   kHz, Stereo, 16-bit */

/* General format structure common to all formats, same for Win16 and Win32 */
typedef struct {
    WORD	wFormatTag;	/* format type */
    WORD	nChannels;	/* number of channels */
    DWORD	nSamplesPerSec;	/* sample rate */
    DWORD	nAvgBytesPerSec;/* for buffer estimation */
    WORD	nBlockAlign; 	/* block size of data */
} WAVEFORMAT, *LPWAVEFORMAT;

#define WAVE_FORMAT_PCM     1

typedef struct {
    WAVEFORMAT	wf;
    WORD	wBitsPerSample;
} PCMWAVEFORMAT, *LPPCMWAVEFORMAT;

/* dito same for Win16 / Win32 */
typedef struct {
    WORD	wFormatTag;	/* format type */
    WORD	nChannels;	/* number of channels (i.e. mono, stereo...) */
    DWORD	nSamplesPerSec;	/* sample rate */
    DWORD	nAvgBytesPerSec;/* for buffer estimation */
    WORD	nBlockAlign;	/* block size of data */
    WORD	wBitsPerSample;	/* number of bits per sample of mono data */
    WORD	cbSize;		/* the count in bytes of the size of */
				/* extra information (after cbSize) */
} WAVEFORMATEX,*LPWAVEFORMATEX;

UINT16 WINAPI waveOutGetNumDevs16();
UINT32 WINAPI waveOutGetNumDevs32();
#define waveOutGetNumDevs WINELIB_NAME(waveOutGetNumDevs)
UINT16 WINAPI waveOutGetDevCaps16(UINT16,LPWAVEOUTCAPS16,UINT16);
UINT32 WINAPI waveOutGetDevCaps32A(UINT32,LPWAVEOUTCAPS32A,UINT32);
UINT32 WINAPI waveOutGetDevCaps32W(UINT32,LPWAVEOUTCAPS32W,UINT32);
#define waveOutGetDevCaps WINELIB_NAME_AW(waveOutGetDevCaps)
UINT16 WINAPI waveOutGetVolume16(UINT16,DWORD*);
UINT32 WINAPI waveOutGetVolume32(UINT32,DWORD*);
#define waveOutGetVolume WINELIB_NAME(waveOutGetVolume)
UINT16 WINAPI waveOutSetVolume16(UINT16,DWORD);
UINT32 WINAPI waveOutSetVolume32(UINT32,DWORD);
#define waveOutSetVolume WINELIB_NAME(waveOutSetVolume)
UINT16 WINAPI waveOutGetErrorText16(UINT16,LPSTR,UINT16);
UINT32 WINAPI waveOutGetErrorText32A(UINT32,LPSTR,UINT32);
UINT32 WINAPI waveOutGetErrorText32W(UINT32,LPWSTR,UINT32);
#define waveOutGetErrorText WINELIB_NAME_AW(waveOutGetErrorText)
UINT16 WINAPI waveOutOpen16(HWAVEOUT16*,UINT16,const LPWAVEFORMATEX,DWORD,DWORD,DWORD);
UINT32 WINAPI waveOutOpen32(HWAVEOUT32*,UINT32,const LPWAVEFORMATEX,DWORD,DWORD,DWORD);
#define waveOutOpen WINELIB_NAME(waveOutOpen)
UINT16 WINAPI waveOutClose16(HWAVEOUT16);
UINT32 WINAPI waveOutClose32(HWAVEOUT32);
#define waveOutClose WINELIB_NAME(waveOutClose)
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16,WAVEHDR*,UINT16);
UINT32 WINAPI waveOutPrepareHeader32(HWAVEOUT32,WAVEHDR*,UINT32);
#define waveOutPrepareHeader WINELIB_NAME(waveOutPrepareHeader)
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16,WAVEHDR*,UINT16);
UINT32 WINAPI waveOutUnprepareHeader32(HWAVEOUT32,WAVEHDR*,UINT32);
#define waveOutUnprepareHeader WINELIB_NAME(waveOutUnprepareHeader)
UINT16 WINAPI waveOutWrite16(HWAVEOUT16,WAVEHDR*,UINT16);
UINT32 WINAPI waveOutWrite32(HWAVEOUT32,WAVEHDR*,UINT32);
#define waveOutWrite WINELIB_NAME(waveOutWrite)
UINT16 WINAPI waveOutPause16(HWAVEOUT16);
UINT32 WINAPI waveOutPause32(HWAVEOUT32);
#define waveOutPause WINELIB_NAME(waveOutPause)
UINT16 WINAPI waveOutRestart16(HWAVEOUT16);
UINT32 WINAPI waveOutRestart32(HWAVEOUT32);
#define waveOutRestart WINELIB_NAME(waveOutRestart)
UINT16 WINAPI waveOutReset16(HWAVEOUT16);
UINT32 WINAPI waveOutReset32(HWAVEOUT32);
#define waveOutReset WINELIB_NAME(waveOutReset)
UINT16 WINAPI waveOutBreakLoop16(HWAVEOUT16);
UINT32 WINAPI waveOutBreakLoop32(HWAVEOUT32);
#define waveOutBreakLoop WINELIB_NAME(waveOutBreakLoop)
UINT16 WINAPI waveOutGetPosition16(HWAVEOUT16,LPMMTIME16,UINT16);
UINT32 WINAPI waveOutGetPosition32(HWAVEOUT32,LPMMTIME32,UINT32);
#define waveOutGetPosition WINELIB_NAME(waveOutGetPosition)
UINT16 WINAPI waveOutGetPitch16(HWAVEOUT16,DWORD*);
UINT32 WINAPI waveOutGetPitch32(HWAVEOUT32,DWORD*);
#define waveOutGetPitch WINELIB_NAME(waveOutGetPitch)
UINT16 WINAPI waveOutSetPitch16(HWAVEOUT16,DWORD);
UINT32 WINAPI waveOutSetPitch32(HWAVEOUT32,DWORD);
#define waveOutSetPitch WINELIB_NAME(waveOutSetPitch)
UINT16 WINAPI waveOutGetPlaybackRate16(HWAVEOUT16,DWORD*);
UINT32 WINAPI waveOutGetPlaybackRate32(HWAVEOUT32,DWORD*);
#define waveOutGetPlaybackRate WINELIB_NAME(waveOutGetPlaybackRate)
UINT16 WINAPI waveOutSetPlaybackRate16(HWAVEOUT16,DWORD);
UINT32 WINAPI waveOutSetPlaybackRate32(HWAVEOUT32,DWORD);
#define waveOutSetPlaybackRate WINELIB_NAME(waveOutSetPlaybackRate)
UINT16 WINAPI waveOutGetID16(HWAVEOUT16,UINT16*);
UINT32 WINAPI waveOutGetID32(HWAVEOUT32,UINT32*);
#define waveOutGetID WINELIB_NAME(waveOutGetID)
DWORD WINAPI waveOutMessage16(HWAVEOUT16,UINT16,DWORD,DWORD);
DWORD WINAPI waveOutMessage32(HWAVEOUT32,UINT32,DWORD,DWORD);
#define waveOutMessage WINELIB_NAME(waveOutMessage)

UINT16 WINAPI waveInGetNumDevs16();
UINT32 WINAPI waveInGetNumDevs32();
#define waveInGetNumDevs WINELIB_NAME(waveInGetNumDevs)
UINT16 WINAPI waveInGetDevCaps16(UINT16,LPWAVEINCAPS16,UINT16);
UINT32 WINAPI waveInGetDevCaps32A(UINT32,LPWAVEINCAPS32A,UINT32);
UINT32 WINAPI waveInGetDevCaps32W(UINT32,LPWAVEINCAPS32W,UINT32);
#define waveInGetDevCaps WINELIB_NAME_AW(waveInGetDevCaps)
UINT16 WINAPI waveInGetErrorText16(UINT16,LPSTR,UINT16);
UINT32 WINAPI waveInGetErrorText32A(UINT32,LPSTR,UINT32);
UINT32 WINAPI waveInGetErrorText32W(UINT32,LPWSTR,UINT32);
#define waveInGetErrorText WINELIB_NAME_AW(waveInGetErrorText)
UINT16 WINAPI waveInOpen16(HWAVEIN16*,UINT16,const LPWAVEFORMAT,DWORD,DWORD,DWORD);
UINT32 WINAPI waveInOpen32(HWAVEIN32*,UINT32,const LPWAVEFORMAT,DWORD,DWORD,DWORD);
#define waveInOpen WINELIB_NAME(waveInOpen)
UINT16 WINAPI waveInClose16(HWAVEIN16);
UINT32 WINAPI waveInClose32(HWAVEIN32);
#define waveInClose WINELIB_TYPE(waveInClose)
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16,WAVEHDR*,UINT16);
UINT32 WINAPI waveInPrepareHeader32(HWAVEIN32,WAVEHDR*,UINT32);
#define waveInPrepareHeader WINELIB_NAME(waveInPrepareHeader)
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16,WAVEHDR*,UINT16);
UINT32 WINAPI waveInUnprepareHeader32(HWAVEIN32,WAVEHDR*,UINT32);
#define waveInUnprepareHeader WINELIB_NAME(waveInUnprepareHeader)
UINT16 WINAPI waveInAddBuffer16(HWAVEIN16,WAVEHDR*,UINT16);
UINT32 WINAPI waveInAddBuffer32(HWAVEIN32,WAVEHDR*,UINT32);
#define waveInAddBuffer WINELIB_NAME(waveInAddBuffer)
UINT16 WINAPI waveInStart16(HWAVEIN16);
UINT32 WINAPI waveInStart32(HWAVEIN32);
#define waveInStart WINELIB_NAME(waveInStart)
UINT16 WINAPI waveInStop16(HWAVEIN16);
UINT32 WINAPI waveInStop32(HWAVEIN32);
#define waveInStop WINELIB_NAME(waveInStop)
UINT16 WINAPI waveInReset16(HWAVEIN16);
UINT32 WINAPI waveInReset32(HWAVEIN32);
#define waveInReset WINELIB_NAME(waveInReset)
UINT16 WINAPI waveInGetPosition16(HWAVEIN16,LPMMTIME16,UINT16);
UINT32 WINAPI waveInGetPosition32(HWAVEIN32,LPMMTIME32,UINT32);
#define waveInGetPosition WINELIB_NAME(waveInGetPosition)
UINT16 WINAPI waveInGetID16(HWAVEIN16,UINT16*);
UINT32 WINAPI waveInGetID32(HWAVEIN32,UINT32*);
#define waveInGetID WINELIB_NAME(waveInGetID)

DWORD WINAPI waveInMessage16(HWAVEIN16,UINT16,DWORD,DWORD);
DWORD WINAPI waveInMessage32(HWAVEIN32,UINT32,DWORD,DWORD);
#define waveInMessage WINELIB_NAME(waveInMessage)

#define MIDIERR_UNPREPARED    (MIDIERR_BASE + 0)   /* header not prepared */
#define MIDIERR_STILLPLAYING  (MIDIERR_BASE + 1)   /* still something playing */
#define MIDIERR_NOMAP         (MIDIERR_BASE + 2)   /* no current map */
#define MIDIERR_NOTREADY      (MIDIERR_BASE + 3)   /* hardware is still busy */
#define MIDIERR_NODEVICE      (MIDIERR_BASE + 4)   /* port no longer connected */
#define MIDIERR_INVALIDSETUP  (MIDIERR_BASE + 5)   /* invalid setup */
#define MIDIERR_LASTERROR     (MIDIERR_BASE + 5)   /* last error in range */

typedef HMIDIIN16  *LPHMIDIIN16;
typedef HMIDIOUT16  *LPHMIDIOUT16;
typedef LPDRVCALLBACK LPMIDICALLBACK;
#define MIDIPATCHSIZE   128
typedef WORD PATCHARRAY[MIDIPATCHSIZE];
typedef WORD *LPPATCHARRAY;
typedef WORD KEYARRAY[MIDIPATCHSIZE];
typedef WORD *LPKEYARRAY;

#define MIM_OPEN        MM_MIM_OPEN
#define MIM_CLOSE       MM_MIM_CLOSE
#define MIM_DATA        MM_MIM_DATA
#define MIM_LONGDATA    MM_MIM_LONGDATA
#define MIM_ERROR       MM_MIM_ERROR
#define MIM_LONGERROR   MM_MIM_LONGERROR
#define MOM_OPEN        MM_MOM_OPEN
#define MOM_CLOSE       MM_MOM_CLOSE
#define MOM_DONE        MM_MOM_DONE

#define MIDIMAPPER     (-1)
#define MIDI_MAPPER    (-1)

/* flags for wFlags parm of 
	midiOutCachePatches(), 
	midiOutCacheDrumPatches() */
#define MIDI_CACHE_ALL      1
#define MIDI_CACHE_BESTFIT  2
#define MIDI_CACHE_QUERY    3
#define MIDI_UNCACHE        4

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION16	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD	wTechnology;	/* type of device */
    WORD	wVoices;	/* # of voices (internal synth only) */
    WORD	wNotes;		/* max # of notes (internal synth only) */
    WORD	wChannelMask;	/* channels used (internal synth only) */
    DWORD	dwSupport;	/* functionality supported by driver */
} MIDIOUTCAPS16, *LPMIDIOUTCAPS16;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION32	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD	wTechnology;	/* type of device */
    WORD	wVoices;	/* # of voices (internal synth only) */
    WORD	wNotes;		/* max # of notes (internal synth only) */
    WORD	wChannelMask;	/* channels used (internal synth only) */
    DWORD	dwSupport;	/* functionality supported by driver */
} MIDIOUTCAPS32A, *LPMIDIOUTCAPS32A;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION32	vDriverVersion;	/* version of the driver */
    WCHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD	wTechnology;	/* type of device */
    WORD	wVoices;	/* # of voices (internal synth only) */
    WORD	wNotes;		/* max # of notes (internal synth only) */
    WORD	wChannelMask;	/* channels used (internal synth only) */
    DWORD	dwSupport;	/* functionality supported by driver */
} MIDIOUTCAPS32W, *LPMIDIOUTCAPS32W;

DECL_WINELIB_TYPE_AW(MIDIOUTCAPS);
DECL_WINELIB_TYPE_AW(LPMIDIOUTCAPS);

#define MOD_MIDIPORT    1  /* output port */
#define MOD_SYNTH       2  /* generic internal synth */
#define MOD_SQSYNTH     3  /* square wave internal synth */
#define MOD_FMSYNTH     4  /* FM internal synth */
#define MOD_MAPPER      5  /* MIDI mapper */

#define MIDICAPS_VOLUME		0x0001  /* supports volume control */
#define MIDICAPS_LRVOLUME	0x0002  /* separate left-right volume control */
#define MIDICAPS_CACHE		0x0004

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION16	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    DWORD	dwSupport;	/* included in win95 and higher */
} MIDIINCAPS16, *LPMIDIINCAPS16;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION32	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    DWORD	dwSupport;	/* included in win95 and higher */
} MIDIINCAPS32A, *LPMIDIINCAPS32A;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION32	vDriverVersion;	/* version of the driver */
    WCHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    DWORD	dwSupport;	/* included in win95 and higher */
} MIDIINCAPS32W, *LPMIDIINCAPS32W;

DECL_WINELIB_TYPE_AW(MIDIINCAPS);
DECL_WINELIB_TYPE_AW(LPMIDIINCAPS);

typedef struct {
    LPSTR	lpData;		/* pointer to locked data block */
    DWORD	dwBufferLength;	/* length of data in data block */
    DWORD	dwBytesRecorded;/* used for input only */
    DWORD	dwUser;		/* for client's use */
    DWORD	dwFlags;	/* assorted flags (see defines) */
    struct midihdr_tag *lpNext;	/* reserved for driver */
    DWORD	reserved;	/* reserved for driver */
} MIDIHDR, *LPMIDIHDR;

#define MHDR_DONE       0x00000001       /* done bit */
#define MHDR_PREPARED   0x00000002       /* set if header prepared */
#define MHDR_INQUEUE    0x00000004       /* reserved for driver */

UINT16 WINAPI midiOutGetNumDevs16();
UINT32 WINAPI midiOutGetNumDevs32();
#define midiOutGetNumDevs WINELIB_NAME(midiOutGetNumDevs)
UINT16 WINAPI midiOutGetDevCaps16(UINT16,LPMIDIOUTCAPS16,UINT16);
UINT32 WINAPI midiOutGetDevCaps32A(UINT32,LPMIDIOUTCAPS32A,UINT32);
UINT32 WINAPI midiOutGetDevCaps32W(UINT32,LPMIDIOUTCAPS32W,UINT32);
#define midiOutGetDevCaps WINELIB_NAME_AW(midiOutGetDevCaps)
UINT16 WINAPI midiOutGetVolume16(UINT16,DWORD*);
UINT32 WINAPI midiOutGetVolume32(UINT32,DWORD*);
#define midiOutGetVolume WINELIB_NAME(midiOutGetVolume)
UINT16 WINAPI midiOutSetVolume16(UINT16,DWORD);
UINT32 WINAPI midiOutSetVolume32(UINT32,DWORD);
#define midiOutSetVolume WINELIB_NAME(midiOutSetVolume)
UINT16 WINAPI midiOutGetErrorText16(UINT16,LPSTR,UINT16);
UINT32 WINAPI midiOutGetErrorText32A(UINT32,LPSTR,UINT32);
UINT32 WINAPI midiOutGetErrorText32W(UINT32,LPWSTR,UINT32);
#define midiOutGetErrorText WINELIB_NAME_AW(midiOutGetErrorText)
UINT16 WINAPI midiGetErrorText(UINT16,LPSTR,UINT16);
UINT16 WINAPI midiOutOpen16(HMIDIOUT16*,UINT16,DWORD,DWORD,DWORD);
UINT32 WINAPI midiOutOpen32(HMIDIOUT32*,UINT32,DWORD,DWORD,DWORD);
#define midiOutOpen WINELIB_NAME(midiOutOpen)
UINT16 WINAPI midiOutClose16(HMIDIOUT16);
UINT32 WINAPI midiOutClose32(HMIDIOUT32);
#define midiOutClose WINELIB_NAME(midiOutClose)
UINT16 WINAPI midiOutPrepareHeader16(HMIDIOUT16,MIDIHDR*,UINT16);
UINT32 WINAPI midiOutPrepareHeader32(HMIDIOUT32,MIDIHDR*,UINT32);
#define midiOutPrepareHeader WINELIB_NAME(midiOutPrepareHeader)
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16,MIDIHDR*,UINT16);
UINT32 WINAPI midiOutUnprepareHeader32(HMIDIOUT32,MIDIHDR*,UINT32);
#define midiOutUnprepareHeader WINELIB_NAME(midiOutUnprepareHeader)
UINT16 WINAPI midiOutShortMsg16(HMIDIOUT16,DWORD);
UINT32 WINAPI midiOutShortMsg32(HMIDIOUT32,DWORD);
#define midiOutShortMsg WINELIB_NAME(midiOutShortMsg)
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16,MIDIHDR*,UINT16);
UINT32 WINAPI midiOutLongMsg32(HMIDIOUT32,MIDIHDR*,UINT32);
#define midiOutLongMsg WINELIB_NAME(midiOutLongMsg)
UINT16 WINAPI midiOutReset16(HMIDIOUT16);
UINT32 WINAPI midiOutReset32(HMIDIOUT32);
#define midiOutReset WINELIB_NAME(midiOutReset)
UINT16 WINAPI midiOutCachePatches16(HMIDIOUT16,UINT16,WORD*,UINT16);
UINT32 WINAPI midiOutCachePatches32(HMIDIOUT32,UINT32,WORD*,UINT32);
#define midiOutCachePatches WINELIB_NAME(midiOutCachePatches)
UINT16 WINAPI midiOutCacheDrumPatches16(HMIDIOUT16,UINT16,WORD*,UINT16);
UINT32 WINAPI midiOutCacheDrumPatches32(HMIDIOUT32,UINT32,WORD*,UINT32);
#define midiOutCacheDrumPatches WINELIB_NAME(midiOutCacheDrumPatches)
UINT16 WINAPI midiOutGetID16(HMIDIOUT16,UINT16*);
UINT32 WINAPI midiOutGetID32(HMIDIOUT32,UINT32*);
#define midiOutGetID WINELIB_NAME(midiOutGetID)

DWORD WINAPI midiOutMessage16(HMIDIOUT16,UINT16,DWORD,DWORD);
DWORD WINAPI midiOutMessage32(HMIDIOUT32,UINT32,DWORD,DWORD);
#define midiOutMessage WINELIB_NAME(midiOutMessage)

UINT16 WINAPI midiInGetNumDevs16(void);
UINT32 WINAPI midiInGetNumDevs32(void);
#define midiInGetNumDevs WINELIB_NAME(midiInGetNumDevs)
UINT16 WINAPI midiInGetDevCaps16(UINT16,LPMIDIINCAPS16,UINT16);
UINT32 WINAPI midiInGetDevCaps32A(UINT32,LPMIDIINCAPS32A,UINT32);
UINT32 WINAPI midiInGetDevCaps32W(UINT32,LPMIDIINCAPS32W,UINT32);
#define midiInGetDevCaps WINELIB_NAME_AW(midiInGetDevCaps)
UINT16 WINAPI midiInGetErrorText16(UINT16,LPSTR,UINT16);
UINT32 WINAPI midiInGetErrorText32A(UINT32,LPSTR,UINT32);
UINT32 WINAPI midiInGetErrorText32W(UINT32,LPWSTR,UINT32);
#define midiInGetErrorText WINELIB_NAME_AW(midiInGetErrorText)
UINT16 WINAPI midiInOpen16(HMIDIIN16*,UINT16,DWORD,DWORD,DWORD);
UINT32 WINAPI midiInOpen32(HMIDIIN32*,UINT32,DWORD,DWORD,DWORD);
#define midiInOpen WINELIB_NAME(midiInOpen)
UINT16 WINAPI midiInClose16(HMIDIIN16);
UINT32 WINAPI midiInClose32(HMIDIIN32);
#define midiInClose WINELIB_NAME(midiInClose)
UINT16 WINAPI midiInPrepareHeader16(HMIDIIN16,MIDIHDR*,UINT16);
UINT32 WINAPI midiInPrepareHeader32(HMIDIIN32,MIDIHDR*,UINT32);
#define midiInPrepareHeader WINELIB_NAME(midiInPrepareHeader)
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16,MIDIHDR*,UINT16);
UINT32 WINAPI midiInUnprepareHeader32(HMIDIIN32,MIDIHDR*,UINT32);
#define midiInUnprepareHeader WINELIB_NAME(midiInUnprepareHeader)
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16,MIDIHDR*,UINT16);
UINT32 WINAPI midiInAddBuffer32(HMIDIIN32,MIDIHDR*,UINT32);
#define midiInAddBuffer WINELIB_NAME(midiInAddBuffer)
UINT16 WINAPI midiInStart16(HMIDIIN16);
UINT32 WINAPI midiInStart32(HMIDIIN32);
#define midiInStart WINELIB_NAME(midiInStart)
UINT16 WINAPI midiInStop16(HMIDIIN16);
UINT32 WINAPI midiInStop32(HMIDIIN32);
#define midiInStop WINELIB_NAME(midiInStop)
UINT16 WINAPI midiInReset16(HMIDIIN16);
UINT32 WINAPI midiInReset32(HMIDIIN32);
#define midiInReset WINELIB_NAME(midiInReset)
UINT16 WINAPI midiInGetID16(HMIDIIN16,UINT16*);
UINT32 WINAPI midiInGetID32(HMIDIIN32,UINT32*);
#define midiInGetID WINELIB_NAME(midiInGetID)
DWORD WINAPI midiInMessage16(HMIDIIN16,UINT16,DWORD,DWORD);
DWORD WINAPI midiInMessage32(HMIDIIN32,UINT32,DWORD,DWORD);
#define midiInMessage WINELIB_NAME(midiInMessage)

#define AUX_MAPPER     (-1)

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (NULL terminated string) */
    WORD	wTechnology;		/* type of device */
    DWORD	dwSupport;		/* functionality supported by driver */
} AUXCAPS16, *LPAUXCAPS16;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (NULL terminated string) */
    WORD	wTechnology;		/* type of device */
    WORD	wReserved1;		/* padding */
    DWORD	dwSupport;		/* functionality supported by driver */
} AUXCAPS32A, *LPAUXCAPS32A;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION32	vDriverVersion;		/* version of the driver */
    WCHAR	szPname[MAXPNAMELEN];	/* product name (NULL terminated string) */
    WORD	wTechnology;		/* type of device */
    WORD	wReserved1;		/* padding */
    DWORD	dwSupport;		/* functionality supported by driver */
} AUXCAPS32W, *LPAUXCAPS32W;

#define AUXCAPS_CDAUDIO    1       /* audio from internal CD-ROM drive */
#define AUXCAPS_AUXIN      2       /* audio from auxiliary input jacks */

#define AUXCAPS_VOLUME          0x0001  /* supports volume control */
#define AUXCAPS_LRVOLUME        0x0002  /* separate left-right volume control */

UINT16 WINAPI auxGetNumDevs16();
UINT32 WINAPI auxGetNumDevs32();
#define auxGetNumDevs WINELIB_NAME(auxGetNumDevs)
UINT16 WINAPI auxGetDevCaps16 (UINT16,LPAUXCAPS16,UINT16);
UINT32 WINAPI auxGetDevCaps32A(UINT32,LPAUXCAPS32A,UINT32);
UINT32 WINAPI auxGetDevCaps32W(UINT32,LPAUXCAPS32W,UINT32);
#define auxGetDevCaps WINELIB_NAME_AW(auxGetDevCaps)
UINT16 WINAPI auxSetVolume16(UINT16,DWORD);
UINT32 WINAPI auxSetVolume32(UINT32,DWORD);
#define auxSetVolume WINELIB_NAME(auxSetVolume)

UINT16 WINAPI auxGetVolume16(UINT16,LPDWORD);
UINT32 WINAPI auxGetVolume32(UINT32,LPDWORD);
#define auxGetVolume WINELIB_NAME(auxGetVolume)

DWORD WINAPI auxOutMessage16(UINT16,UINT16,DWORD,DWORD);
DWORD WINAPI auxOutMessage32(UINT32,UINT32,DWORD,DWORD);
#define auxOutMessage WINELIB_NAME(auxOutMessage)

#define TIMERR_NOERROR        (0)                  /* no error */
#define TIMERR_NOCANDO        (TIMERR_BASE+1)      /* request not completed */
#define TIMERR_STRUCT         (TIMERR_BASE+33)     /* time struct size */

typedef void (CALLBACK *LPTIMECALLBACK16)(UINT16 uTimerID, UINT16 uMessage, DWORD dwUser, DWORD dw1, DWORD dw2);
typedef void (CALLBACK *LPTIMECALLBACK32)(UINT32 uTimerID, UINT32 uMessage, DWORD dwUser, DWORD dw1, DWORD dw2);
DECL_WINELIB_TYPE(LPTIMECALLBACK);

#define TIME_ONESHOT    0   /* program timer for single event */
#define TIME_PERIODIC   1   /* program for continuous periodic event */

typedef struct {
    UINT16	wPeriodMin;	/* minimum period supported  */
    UINT16	wPeriodMax;	/* maximum period supported  */
} TIMECAPS16,*LPTIMECAPS16;

typedef struct {
    UINT32	wPeriodMin;
    UINT32	wPeriodMax;
} TIMECAPS32, *LPTIMECAPS32;

DECL_WINELIB_TYPE(TIMECAPS);
DECL_WINELIB_TYPE(LPTIMECAPS);

MMRESULT16 WINAPI timeGetSystemTime16(LPMMTIME16,UINT16);
MMRESULT32 WINAPI timeGetSystemTime32(LPMMTIME32,UINT32);
#define timeGetSystemTime WINELIB_NAME(timeGetSystemTime)
DWORD WINAPI timeGetTime();	/* same for win32/win16 */
MMRESULT16 WINAPI timeSetEvent16(UINT16,UINT16,LPTIMECALLBACK16,DWORD,UINT16);
MMRESULT32 WINAPI timeSetEvent32(UINT32,UINT32,LPTIMECALLBACK32,DWORD,UINT32);
#define timeSetEvent WINELIB_NAME(timeSetEvent)
MMRESULT16 WINAPI timeKillEvent16(UINT16);
MMRESULT32 WINAPI timeKillEvent32(UINT32);
#define timeKillEvent WINELIB_NAME(timeKillEvent)
MMRESULT16 WINAPI timeGetDevCaps16(LPTIMECAPS16,UINT16);
MMRESULT32 WINAPI timeGetDevCaps32(LPTIMECAPS32,UINT32);
#define timeGetDevCaps WINELIB_NAME(timeGetDevCaps)
MMRESULT16 WINAPI timeBeginPeriod16(UINT16);
MMRESULT32 WINAPI timeBeginPeriod32(UINT32);
#define timeBeginPeriod WINELIB_NAME(timeBeginPeriod)
MMRESULT16 WINAPI timeEndPeriod16(UINT16);
MMRESULT32 WINAPI timeEndPeriod32(UINT32);
#define timeEndPeriod WINELIB_NAME(timeEndPeriod)

#define JOYERR_NOERROR        (0)                  /* no error */
#define JOYERR_PARMS          (JOYERR_BASE+5)      /* bad parameters */
#define JOYERR_NOCANDO        (JOYERR_BASE+6)      /* request not completed */
#define JOYERR_UNPLUGGED      (JOYERR_BASE+7)      /* joystick is unplugged */

#define JOY_BUTTON1         0x0001
#define JOY_BUTTON2         0x0002
#define JOY_BUTTON3         0x0004
#define JOY_BUTTON4         0x0008
#define JOY_BUTTON1CHG      0x0100
#define JOY_BUTTON2CHG      0x0200
#define JOY_BUTTON3CHG      0x0400
#define JOY_BUTTON4CHG      0x0800

#define JOYSTICKID1         0
#define JOYSTICKID2         1

typedef struct {
    WORD wMid;                  /* manufacturer ID */
    WORD wPid;                  /* product ID */
    char szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    UINT16 wXmin;                 /* minimum x position value */
    UINT16 wXmax;                 /* maximum x position value */
    UINT16 wYmin;                 /* minimum y position value */
    UINT16 wYmax;                 /* maximum y position value */
    UINT16 wZmin;                 /* minimum z position value */
    UINT16 wZmax;                 /* maximum z position value */
    UINT16 wNumButtons;           /* number of buttons */
    UINT16 wPeriodMin;            /* minimum message period when captured */
    UINT16 wPeriodMax;            /* maximum message period when captured */
    /* win95,nt4 additions: */
    UINT16 wRmin;		/* minimum r position value */
    UINT16 wRmax;		/* maximum r position value */
    UINT16 wUmin;		/* minimum u (5th axis) position value */
    UINT16 wUmax;		/* maximum u (5th axis) position value */
    UINT16 wVmin;		/* minimum v (6th axis) position value */
    UINT16 wVmax;		/* maximum v (6th axis) position value */
    UINT16 wCaps;		/* joystick capabilites */
    UINT16 wMaxAxes;		/* maximum number of axes supported */
    UINT16 wNumAxes;		/* number of axes in use */
    UINT16 wMaxButtons;		/* maximum number of buttons supported */
    CHAR szRegKey[MAXPNAMELEN]; /* registry key */
    CHAR szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
} JOYCAPS16, *LPJOYCAPS16;

typedef struct {
    WORD wMid;
    WORD wPid;
    CHAR szPname[MAXPNAMELEN];
    UINT32 wXmin;
    UINT32 wXmax;
    UINT32 wYmin;
    UINT32 wYmax;
    UINT32 wZmin;
    UINT32 wZmax;
    UINT32 wNumButtons;
    UINT32 wPeriodMin;
    UINT32 wPeriodMax;
    UINT32 wRmin;
    UINT32 wRmax;
    UINT32 wUmin;
    UINT32 wUmax;
    UINT32 wVmin;
    UINT32 wVmax;
    UINT32 wCaps;
    UINT32 wMaxAxes;
    UINT32 wNumAxes;
    UINT32 wMaxButtons;
    CHAR szRegKey[MAXPNAMELEN];
    CHAR szOEMVxD[MAX_JOYSTICKOEMVXDNAME];
} JOYCAPS32A, *LPJOYCAPS32A;

typedef struct {
    WORD wMid;
    WORD wPid;
    WCHAR szPname[MAXPNAMELEN];
    UINT32 wXmin;
    UINT32 wXmax;
    UINT32 wYmin;
    UINT32 wYmax;
    UINT32 wZmin;
    UINT32 wZmax;
    UINT32 wNumButtons;
    UINT32 wPeriodMin;
    UINT32 wPeriodMax;
    UINT32 wRmin;
    UINT32 wRmax;
    UINT32 wUmin;
    UINT32 wUmax;
    UINT32 wVmin;
    UINT32 wVmax;
    UINT32 wCaps;
    UINT32 wMaxAxes;
    UINT32 wNumAxes;
    UINT32 wMaxButtons;
    WCHAR szRegKey[MAXPNAMELEN];
    WCHAR szOEMVxD[MAX_JOYSTICKOEMVXDNAME];
} JOYCAPS32W, *LPJOYCAPS32W;
DECL_WINELIB_TYPE_AW(JOYCAPS)
DECL_WINELIB_TYPE_AW(LPJOYCAPS)

typedef struct {
    UINT16 wXpos;                 /* x position */
    UINT16 wYpos;                 /* y position */
    UINT16 wZpos;                 /* z position */
    UINT16 wButtons;              /* button states */
} JOYINFO16, *LPJOYINFO16;

typedef struct {
    UINT32 wXpos;
    UINT32 wYpos;
    UINT32 wZpos;
    UINT32 wButtons;
} JOYINFO32, *LPJOYINFO32;

DECL_WINELIB_TYPE(JOYINFO)
DECL_WINELIB_TYPE(LPJOYINFO)

MMRESULT16 WINAPI joyGetDevCaps16 (UINT16,LPJOYCAPS16 ,UINT16);
MMRESULT32 WINAPI joyGetDevCaps32A(UINT32,LPJOYCAPS32A,UINT32);
MMRESULT32 WINAPI joyGetDevCaps32W(UINT32,LPJOYCAPS32W,UINT32);
#define joyGetDevCaps WINELIB_NAME_AW(joyGetDevCaps)
UINT16 WINAPI joyGetNumDevs16(void);
UINT32 WINAPI joyGetNumDevs32(void);
#define joyGetNumDevs WINELIB_NAME(joyGetNumDevs)
MMRESULT16 WINAPI joyGetPos16(UINT16,LPJOYINFO16);
MMRESULT32 WINAPI joyGetPos32(UINT32,LPJOYINFO32);
#define joyGetPos WINELIB_NAME(joyGetPos)
MMRESULT16 WINAPI joyGetThreshold16(UINT16,UINT16*);
MMRESULT32 WINAPI joyGetThreshold32(UINT32,UINT32*);
#define joyGetThreshold WINELIB_NAME(joyGetThreshold)
MMRESULT16 WINAPI joyReleaseCapture16(UINT16);
MMRESULT32 WINAPI joyReleaseCapture32(UINT32);
#define joyReleaseCapture WINELIB_NAME(joyReleaseCapture)
MMRESULT16 WINAPI joySetCapture16(HWND16,UINT16,UINT16,BOOL16);
MMRESULT32 WINAPI joySetCapture32(HWND32,UINT32,UINT32,BOOL32);
#define joySetCapture WINELIB_NAME(joySetCapture)
MMRESULT16 WINAPI joySetThreshold16(UINT16,UINT16);
MMRESULT32 WINAPI joySetThreshold32(UINT32,UINT32);
#define joySetThreshold WINELIB_NAME(joySetThreshold)

typedef struct {
	WORD		wMid;		/* manufacturer id */
	WORD		wPid;		/* product id */
	MMVERSION16	vDriverVersion;		/* version of the driver */
	CHAR		szPname[MAXPNAMELEN];	/* product name */
	DWORD		fdwSupport;	/* misc. support bits */
	DWORD		cDestinations;	/* count of destinations */
} MIXERCAPS16,*LPMIXERCAPS16;

typedef struct {
	WORD		wMid;
	WORD		wPid;
	MMVERSION32	vDriverVersion;
	CHAR		szPname[MAXPNAMELEN];
	DWORD		fdwSupport;
	DWORD		cDestinations;
} MIXERCAPS32A,*LPMIXERCAPS32A;

typedef struct {
	WORD		wMid;
	WORD		wPid;
	MMVERSION32	vDriverVersion;
	WCHAR		szPname[MAXPNAMELEN];
	DWORD		fdwSupport;
	DWORD		cDestinations;
} MIXERCAPS32W,*LPMIXERCAPS32W;

DECL_WINELIB_TYPE_AW(MIXERCAPS);
DECL_WINELIB_TYPE_AW(LPMIXERCAPS);

#define MIXER_SHORT_NAME_CHARS	16
#define MIXER_LONG_NAME_CHARS	64

/*  MIXERLINE.fdwLine */
#define	MIXERLINE_LINEF_ACTIVE		0x00000001
#define	MIXERLINE_LINEF_DISCONNECTED	0x00008000
#define	MIXERLINE_LINEF_SOURCE		0x80000000

/*  MIXERLINE.dwComponentType */
/*  component types for destinations and sources */
#define	MIXERLINE_COMPONENTTYPE_DST_FIRST	0x00000000L
#define	MIXERLINE_COMPONENTTYPE_DST_UNDEFINED	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 0)
#define	MIXERLINE_COMPONENTTYPE_DST_DIGITAL	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 1)
#define	MIXERLINE_COMPONENTTYPE_DST_LINE	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 2)
#define	MIXERLINE_COMPONENTTYPE_DST_MONITOR	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 3)
#define	MIXERLINE_COMPONENTTYPE_DST_SPEAKERS	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 4)
#define	MIXERLINE_COMPONENTTYPE_DST_HEADPHONES	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 5)
#define	MIXERLINE_COMPONENTTYPE_DST_TELEPHONE	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 6)
#define	MIXERLINE_COMPONENTTYPE_DST_WAVEIN	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 7)
#define	MIXERLINE_COMPONENTTYPE_DST_VOICEIN	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)
#define	MIXERLINE_COMPONENTTYPE_DST_LAST	(MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)

#define	MIXERLINE_COMPONENTTYPE_SRC_FIRST	0x00001000L
#define	MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 0)
#define	MIXERLINE_COMPONENTTYPE_SRC_DIGITAL	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 1)
#define	MIXERLINE_COMPONENTTYPE_SRC_LINE	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define	MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 3)
#define	MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 4)
#define	MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define	MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 6)
#define	MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)
#define	MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8)
#define	MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 9)
#define	MIXERLINE_COMPONENTTYPE_SRC_ANALOG	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)
#define	MIXERLINE_COMPONENTTYPE_SRC_LAST	(MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)

/*  MIXERLINE.Target.dwType */
#define	MIXERLINE_TARGETTYPE_UNDEFINED	0
#define	MIXERLINE_TARGETTYPE_WAVEOUT	1
#define	MIXERLINE_TARGETTYPE_WAVEIN	2
#define	MIXERLINE_TARGETTYPE_MIDIOUT	3
#define	MIXERLINE_TARGETTYPE_MIDIIN	4
#define MIXERLINE_TARGETTYPE_AUX	5

typedef struct {
    DWORD	cbStruct;		/* size of MIXERLINE structure */
    DWORD	dwDestination;		/* zero based destination index */
    DWORD	dwSource;		/* zero based source index (if source) */
    DWORD	dwLineID;		/* unique line id for mixer device */
    DWORD	fdwLine;		/* state/information about line */
    DWORD	dwUser;			/* driver specific information */
    DWORD	dwComponentType;	/* component type line connects to */
    DWORD	cChannels;		/* number of channels line supports */
    DWORD	cConnections;		/* number of connections [possible] */
    DWORD	cControls;		/* number of controls at this line */
    CHAR	szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR	szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD	dwType;			/* MIXERLINE_TARGETTYPE_xxxx */
	DWORD	dwDeviceID;		/* target device ID of device type */
	WORD	wMid;			/* of target device */
	WORD	wPid;			/*      " */
	MMVERSION16	vDriverVersion;	/*      " */
	CHAR	szPname[MAXPNAMELEN];	/*      " */
    } Target;
} MIXERLINE16, *LPMIXERLINE16;

typedef struct {
    DWORD	cbStruct;
    DWORD	dwDestination;
    DWORD	dwSource;
    DWORD	dwLineID;
    DWORD	fdwLine;
    DWORD	dwUser;
    DWORD	dwComponentType;
    DWORD	cChannels;
    DWORD	cConnections;
    DWORD	cControls;
    CHAR	szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR	szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD	dwType;
	DWORD	dwDeviceID;
	WORD	wMid;
	WORD	wPid;
	MMVERSION32	vDriverVersion;
	CHAR	szPname[MAXPNAMELEN];
    } Target;
} MIXERLINE32A, *LPMIXERLINE32A;

typedef struct {
    DWORD	cbStruct;
    DWORD	dwDestination;
    DWORD	dwSource;
    DWORD	dwLineID;
    DWORD	fdwLine;
    DWORD	dwUser;
    DWORD	dwComponentType;
    DWORD	cChannels;
    DWORD	cConnections;
    DWORD	cControls;
    WCHAR	szShortName[MIXER_SHORT_NAME_CHARS];
    WCHAR	szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD	dwType;
	DWORD	dwDeviceID;
	WORD	wMid;
	WORD	wPid;
	MMVERSION32	vDriverVersion;
	WCHAR	szPname[MAXPNAMELEN];
    } Target;
} MIXERLINE32W, *LPMIXERLINE32W;

DECL_WINELIB_TYPE_AW(MIXERLINE);
DECL_WINELIB_TYPE_AW(LPMIXERLINE);

/*  MIXERCONTROL.fdwControl */
#define	MIXERCONTROL_CONTROLF_UNIFORM	0x00000001L
#define	MIXERCONTROL_CONTROLF_MULTIPLE	0x00000002L
#define	MIXERCONTROL_CONTROLF_DISABLED	0x80000000L

/*  MIXERCONTROL_CONTROLTYPE_xxx building block defines */
#define	MIXERCONTROL_CT_CLASS_MASK		0xF0000000L
#define	MIXERCONTROL_CT_CLASS_CUSTOM		0x00000000L
#define	MIXERCONTROL_CT_CLASS_METER		0x10000000L
#define	MIXERCONTROL_CT_CLASS_SWITCH		0x20000000L
#define	MIXERCONTROL_CT_CLASS_NUMBER		0x30000000L
#define	MIXERCONTROL_CT_CLASS_SLIDER		0x40000000L
#define	MIXERCONTROL_CT_CLASS_FADER		0x50000000L
#define	MIXERCONTROL_CT_CLASS_TIME		0x60000000L
#define	MIXERCONTROL_CT_CLASS_LIST		0x70000000L

#define	MIXERCONTROL_CT_SUBCLASS_MASK		0x0F000000L

#define	MIXERCONTROL_CT_SC_SWITCH_BOOLEAN	0x00000000L
#define	MIXERCONTROL_CT_SC_SWITCH_BUTTON	0x01000000L

#define	MIXERCONTROL_CT_SC_METER_POLLED		0x00000000L

#define	MIXERCONTROL_CT_SC_TIME_MICROSECS	0x00000000L
#define	MIXERCONTROL_CT_SC_TIME_MILLISECS	0x01000000L

#define	MIXERCONTROL_CT_SC_LIST_SINGLE		0x00000000L
#define	MIXERCONTROL_CT_SC_LIST_MULTIPLE	0x01000000L

#define	MIXERCONTROL_CT_UNITS_MASK		0x00FF0000L
#define	MIXERCONTROL_CT_UNITS_CUSTOM		0x00000000L
#define	MIXERCONTROL_CT_UNITS_BOOLEAN		0x00010000L
#define	MIXERCONTROL_CT_UNITS_SIGNED		0x00020000L
#define	MIXERCONTROL_CT_UNITS_UNSIGNED		0x00030000L
#define	MIXERCONTROL_CT_UNITS_DECIBELS		0x00040000L /* in 10ths */
#define	MIXERCONTROL_CT_UNITS_PERCENT		0x00050000L /* in 10ths */

/*  Commonly used control types for specifying MIXERCONTROL.dwControlType */
#define MIXERCONTROL_CONTROLTYPE_CUSTOM		(MIXERCONTROL_CT_CLASS_CUSTOM | MIXERCONTROL_CT_UNITS_CUSTOM)
#define MIXERCONTROL_CONTROLTYPE_BOOLEANMETER	(MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_SIGNEDMETER	(MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PEAKMETER	(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER + 1)
#define MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER	(MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_BOOLEAN	(MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BOOLEAN | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_ONOFF		(MIXERCONTROL_CONTROLTYPE_BOOLEAN + 1)
#define MIXERCONTROL_CONTROLTYPE_MUTE		(MIXERCONTROL_CONTROLTYPE_BOOLEAN + 2)
#define MIXERCONTROL_CONTROLTYPE_MONO		(MIXERCONTROL_CONTROLTYPE_BOOLEAN + 3)
#define MIXERCONTROL_CONTROLTYPE_LOUDNESS	(MIXERCONTROL_CONTROLTYPE_BOOLEAN + 4)
#define MIXERCONTROL_CONTROLTYPE_STEREOENH	(MIXERCONTROL_CONTROLTYPE_BOOLEAN + 5)
#define MIXERCONTROL_CONTROLTYPE_BUTTON		(MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BUTTON | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_DECIBELS	(MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_DECIBELS)
#define MIXERCONTROL_CONTROLTYPE_SIGNED		(MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_UNSIGNED	(MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_PERCENT	(MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_PERCENT)
#define MIXERCONTROL_CONTROLTYPE_SLIDER		(MIXERCONTROL_CT_CLASS_SLIDER | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PAN		(MIXERCONTROL_CONTROLTYPE_SLIDER + 1)
#define MIXERCONTROL_CONTROLTYPE_QSOUNDPAN	(MIXERCONTROL_CONTROLTYPE_SLIDER + 2)
#define MIXERCONTROL_CONTROLTYPE_FADER		(MIXERCONTROL_CT_CLASS_FADER | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_VOLUME		(MIXERCONTROL_CONTROLTYPE_FADER + 1)
#define MIXERCONTROL_CONTROLTYPE_BASS		(MIXERCONTROL_CONTROLTYPE_FADER + 2)
#define MIXERCONTROL_CONTROLTYPE_TREBLE		(MIXERCONTROL_CONTROLTYPE_FADER + 3)
#define MIXERCONTROL_CONTROLTYPE_EQUALIZER	(MIXERCONTROL_CONTROLTYPE_FADER + 4)
#define MIXERCONTROL_CONTROLTYPE_SINGLESELECT	(MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_SINGLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MUX		(MIXERCONTROL_CONTROLTYPE_SINGLESELECT + 1)
#define MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT	(MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_MULTIPLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MIXER		(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT + 1)
#define MIXERCONTROL_CONTROLTYPE_MICROTIME	(MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MICROSECS | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_MILLITIME	(MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MILLISECS | MIXERCONTROL_CT_UNITS_UNSIGNED)


typedef struct {
    DWORD		cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD		dwControlID;        /* unique control id for mixer device */
    DWORD		dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD		fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD		cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    CHAR		szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR		szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG	lMinimum;	/* signed minimum for this control */
	    LONG	lMaximum;	/* signed maximum for this control */
	} l;
	struct {
	    DWORD	dwMinimum;	/* unsigned minimum for this control */
	    DWORD	dwMaximum;	/* unsigned maximum for this control */
	} dw;
	DWORD       	dwReserved[6];
    } Bounds;
    union {
	DWORD		cSteps;		/* # of steps between min & max */
	DWORD		cbCustomData;	/* size in bytes of custom data */
	DWORD		dwReserved[6];	/* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROL16, *LPMIXERCONTROL16;

typedef struct {
    DWORD		cbStruct;
    DWORD		dwControlID;
    DWORD		dwControlType;
    DWORD		fdwControl;
    DWORD		cMultipleItems;
    CHAR		szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR		szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG	lMinimum;
	    LONG	lMaximum;
	} l;
	struct {
	    DWORD	dwMinimum;
	    DWORD	dwMaximum;
	} dw;
	DWORD       	dwReserved[6];
    } Bounds;
    union {
	DWORD		cSteps;
	DWORD		cbCustomData;
	DWORD		dwReserved[6];
    } Metrics;
} MIXERCONTROL32A, *LPMIXERCONTROL32A;

typedef struct {
    DWORD		cbStruct;
    DWORD		dwControlID;
    DWORD		dwControlType;
    DWORD		fdwControl;
    DWORD		cMultipleItems;
    WCHAR		szShortName[MIXER_SHORT_NAME_CHARS];
    WCHAR		szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG	lMinimum;
	    LONG	lMaximum;
	} l;
	struct {
	    DWORD	dwMinimum;
	    DWORD	dwMaximum;
	} dw;
	DWORD       	dwReserved[6];
    } Bounds;
    union {
	DWORD		cSteps;
	DWORD		cbCustomData;
	DWORD		dwReserved[6];
    } Metrics;
} MIXERCONTROL32W, *LPMIXERCONTROL32W;

DECL_WINELIB_TYPE_AW(MIXERCONTROL);
DECL_WINELIB_TYPE_AW(LPMIXERCONTROL);

typedef struct {
    DWORD	cbStruct;	/* size in bytes of MIXERLINECONTROLS */
    DWORD	dwLineID;	/* line id (from MIXERLINE.dwLineID) */
    union {
	DWORD	dwControlID;	/* MIXER_GETLINECONTROLSF_ONEBYID */
	DWORD	dwControlType;	/* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    } u;
    DWORD	cControls;	/* count of controls pmxctrl points to */
    DWORD	cbmxctrl;	/* size in bytes of _one_ MIXERCONTROL */
    LPMIXERCONTROL16	pamxctrl;/* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLS16, *LPMIXERLINECONTROLS16;

typedef struct {
    DWORD	cbStruct;
    DWORD	dwLineID;
    union {
	DWORD	dwControlID;
	DWORD	dwControlType;
    } u;
    DWORD	cControls;
    DWORD	cbmxctrl;
    LPMIXERCONTROL32A	pamxctrl;
} MIXERLINECONTROLS32A, *LPMIXERLINECONTROLS32A;

typedef struct {
    DWORD	cbStruct;
    DWORD	dwLineID;
    union {
	DWORD	dwControlID;
	DWORD	dwControlType;
    } u;
    DWORD	cControls;
    DWORD	cbmxctrl;
    LPMIXERCONTROL32W	pamxctrl;
} MIXERLINECONTROLS32W, *LPMIXERLINECONTROLS32W;

DECL_WINELIB_TYPE_AW(MIXERLINECONTROLS);
DECL_WINELIB_TYPE_AW(LPMIXERLINECONTROLS);

typedef struct {
    DWORD	cbStruct;	/* size in bytes of MIXERCONTROLDETAILS */
    DWORD	dwControlID;	/* control id to get/set details on */
    DWORD	cChannels;	/* number of channels in paDetails array */
    union {
        HWND16	hwndOwner;	/* for MIXER_SETCONTROLDETAILSF_CUSTOM */
        DWORD	cMultipleItems;	/* if _MULTIPLE, the number of items per channel */
    } u;
    DWORD	cbDetails;	/* size of _one_ details_XX struct */
    LPVOID	paDetails;	/* pointer to array of details_XX structs */
} MIXERCONTROLDETAILS16,*LPMIXERCONTROLDETAILS16;

typedef struct {
    DWORD	cbStruct;
    DWORD	dwControlID;
    DWORD	cChannels;
    union {
        HWND32	hwndOwner;
        DWORD	cMultipleItems;
    } u;
    DWORD	cbDetails;
    LPVOID	paDetails;
} MIXERCONTROLDETAILS32,*LPMIXERCONTROLDETAILS32;

DECL_WINELIB_TYPE(MIXERCONTROLDETAILS);
DECL_WINELIB_TYPE(LPMIXERCONTROLDETAILS);

typedef struct {
    DWORD	dwParam1;
    DWORD	dwParam2;
    CHAR	szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT16,*LPMIXERCONTROLDETAILS_LISTTEXT16;

typedef struct {
    DWORD	dwParam1;
    DWORD	dwParam2;
    CHAR	szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT32A,*LPMIXERCONTROLDETAILS_LISTTEXT32A;

typedef struct {
    DWORD	dwParam1;
    DWORD	dwParam2;
    WCHAR	szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT32W,*LPMIXERCONTROLDETAILS_LISTTEXT32W;

DECL_WINELIB_TYPE_AW(MIXERCONTROLDETAILS_LISTTEXT);
DECL_WINELIB_TYPE_AW(LPMIXERCONTROLDETAILS_LISTTEXT);

/*  MIXER_GETCONTROLDETAILSF_VALUE */
typedef struct {
	LONG	fValue;
} MIXERCONTROLDETAILS_BOOLEAN,*LPMIXERCONTROLDETAILS_BOOLEAN;

typedef struct {
	LONG	lValue;
} MIXERCONTROLDETAILS_SIGNED,*LPMIXERCONTROLDETAILS_SIGNED;

typedef struct {
	DWORD	dwValue;
} MIXERCONTROLDETAILS_UNSIGNED,*LPMIXERCONTROLDETAILS_UNSIGNED;

/* bits passed to mixerGetLineInfo.fdwInfo */
#define MIXER_GETLINEINFOF_DESTINATION		0x00000000L
#define MIXER_GETLINEINFOF_SOURCE		0x00000001L
#define MIXER_GETLINEINFOF_LINEID		0x00000002L
#define MIXER_GETLINEINFOF_COMPONENTTYPE	0x00000003L
#define MIXER_GETLINEINFOF_TARGETTYPE		0x00000004L
#define MIXER_GETLINEINFOF_QUERYMASK		0x0000000FL

/* bitmask passed to mixerGetLineControl */
#define MIXER_GETLINECONTROLSF_ALL		0x00000000L
#define MIXER_GETLINECONTROLSF_ONEBYID		0x00000001L
#define MIXER_GETLINECONTROLSF_ONEBYTYPE	0x00000002L
#define MIXER_GETLINECONTROLSF_QUERYMASK	0x0000000FL

/* bitmask passed to mixerGetControlDetails */
#define MIXER_GETCONTROLDETAILSF_VALUE		0x00000000L
#define MIXER_GETCONTROLDETAILSF_LISTTEXT	0x00000001L
#define MIXER_GETCONTROLDETAILSF_QUERYMASK	0x0000000FL

/* bitmask passed to mixerSetControlDetails */
#define	MIXER_SETCONTROLDETAILSF_VALUE		0x00000000L
#define	MIXER_SETCONTROLDETAILSF_CUSTOM		0x00000001L
#define	MIXER_SETCONTROLDETAILSF_QUERYMASK	0x0000000FL

UINT16 WINAPI mixerGetNumDevs16();
UINT32 WINAPI mixerGetNumDevs32();
#define mixerGetNumDevs WINELIB_NAME(mixerGetNumDevs)
UINT16 WINAPI mixerOpen16(LPHMIXER16,UINT16,DWORD,DWORD,DWORD);
UINT32 WINAPI mixerOpen32(LPHMIXER32,UINT32,DWORD,DWORD,DWORD);
#define mixerOpen WINELIB_NAME(mixerOpen)
UINT16 WINAPI mixerClose16(HMIXER16);
UINT32 WINAPI mixerClose32(HMIXER32);
#define mixerClose WINELIB_NAME(mixerClose)
UINT16 WINAPI mixerMessage16(HMIXER16,UINT16,DWORD,DWORD);
UINT32 WINAPI mixerMessage32(HMIXER32,UINT32,DWORD,DWORD);
#define mixerMessage WINELIB_NAME(mixerMessage)
UINT16 WINAPI mixerGetDevCaps16(UINT16,LPMIXERCAPS16,UINT16);
UINT32 WINAPI mixerGetDevCaps32A(UINT32,LPMIXERCAPS32A,UINT32);
UINT32 WINAPI mixerGetDevCaps32W(UINT32,LPMIXERCAPS32W,UINT32);
#define mixerGetDevCaps WINELIB_NAME_AW(mixerGetDevCaps)
UINT16 WINAPI mixerGetLineInfo16(HMIXEROBJ16,LPMIXERLINE16,DWORD);
UINT32 WINAPI mixerGetLineInfo32A(HMIXEROBJ32,LPMIXERLINE32A,DWORD);
UINT32 WINAPI mixerGetLineInfo32W(HMIXEROBJ32,LPMIXERLINE32W,DWORD);
#define mixerGetLineInfo WINELIB_NAME_AW(mixerGetLineInfo)
UINT16 WINAPI mixerGetID16(HMIXEROBJ16,LPUINT16,DWORD);
UINT32 WINAPI mixerGetID32(HMIXEROBJ32,LPUINT32,DWORD);
#define mixerGetID WINELIB_NAME(mixerGetID)
UINT16 WINAPI mixerGetLineControls16(HMIXEROBJ16,LPMIXERLINECONTROLS16,DWORD);
UINT32 WINAPI mixerGetLineControls32A(HMIXEROBJ32,LPMIXERLINECONTROLS32A,DWORD);
UINT32 WINAPI mixerGetLineControls32W(HMIXEROBJ32,LPMIXERLINECONTROLS32W,DWORD);
#define mixerGetLineControl WINELIB_NAME_AW(mixerGetLineControl)
UINT16 WINAPI mixerGetControlDetails16(HMIXEROBJ16,LPMIXERCONTROLDETAILS16,DWORD);
UINT32 WINAPI mixerGetControlDetails32A(HMIXEROBJ32,LPMIXERCONTROLDETAILS32,DWORD);
UINT32 WINAPI mixerGetControlDetails32W(HMIXEROBJ32,LPMIXERCONTROLDETAILS32,DWORD);
#define mixerGetControlDetails WINELIB_NAME_AW(mixerGetControlDetails)
UINT16 WINAPI mixerSetControlDetails16(HMIXEROBJ16,LPMIXERCONTROLDETAILS16,DWORD);
UINT32 WINAPI mixerSetControlDetails32A(HMIXEROBJ16,LPMIXERCONTROLDETAILS32,DWORD);
UINT32 WINAPI mixerSetControlDetails32W(HMIXEROBJ16,LPMIXERCONTROLDETAILS32,DWORD);
#define mixerSetControlDetails WINELIB_NAME_AW(mixerSetControlDetails)

#define MMIOERR_BASE            256
#define MMIOERR_FILENOTFOUND    (MMIOERR_BASE + 1)  /* file not found */
#define MMIOERR_OUTOFMEMORY     (MMIOERR_BASE + 2)  /* out of memory */
#define MMIOERR_CANNOTOPEN      (MMIOERR_BASE + 3)  /* cannot open */
#define MMIOERR_CANNOTCLOSE     (MMIOERR_BASE + 4)  /* cannot close */
#define MMIOERR_CANNOTREAD      (MMIOERR_BASE + 5)  /* cannot read */
#define MMIOERR_CANNOTWRITE     (MMIOERR_BASE + 6)  /* cannot write */
#define MMIOERR_CANNOTSEEK      (MMIOERR_BASE + 7)  /* cannot seek */
#define MMIOERR_CANNOTEXPAND    (MMIOERR_BASE + 8)  /* cannot expand file */
#define MMIOERR_CHUNKNOTFOUND   (MMIOERR_BASE + 9)  /* chunk not found */
#define MMIOERR_UNBUFFERED      (MMIOERR_BASE + 10) /* file is unbuffered */

#define CFSEPCHAR       '+'             /* compound file name separator char. */

typedef DWORD           FOURCC;         /* a four character code */
typedef LONG (CALLBACK *LPMMIOPROC16)(LPSTR lpmmioinfo, UINT16 uMessage,
                                      LPARAM lParam1, LPARAM lParam2);
typedef LONG (CALLBACK *LPMMIOPROC32)(LPSTR lpmmioinfo, UINT32 uMessage,
                                      LPARAM lParam1, LPARAM lParam2);
DECL_WINELIB_TYPE(LPMMIOPROC);

typedef struct {
        DWORD		dwFlags;	/* general status flags */
        FOURCC		fccIOProc;	/* pointer to I/O procedure */
        LPMMIOPROC16	pIOProc;	/* pointer to I/O procedure */
        UINT16		wErrorRet;	/* place for error to be returned */
        HTASK16		htask;		/* alternate local task */
        /* fields maintained by MMIO functions during buffered I/O */
        LONG		cchBuffer;	/* size of I/O buffer (or 0L) */
        HPSTR		pchBuffer;	/* start of I/O buffer (or NULL) */
        HPSTR		pchNext;	/* pointer to next byte to read/write */
        HPSTR		pchEndRead;	/* pointer to last valid byte to read */
        HPSTR		pchEndWrite;	/* pointer to last byte to write */
        LONG		lBufOffset;	/* disk offset of start of buffer */
        /* fields maintained by I/O procedure */
        LONG		lDiskOffset;	/* disk offset of next read or write */
        DWORD		adwInfo[3];	/* data specific to type of MMIOPROC */
        /* other fields maintained by MMIO */
        DWORD		dwReserved1;	/* reserved for MMIO use */
        DWORD		dwReserved2;	/* reserved for MMIO use */
        HMMIO16		hmmio;		/* handle to open file */
} MMIOINFO16, *LPMMIOINFO16;

typedef struct {
        DWORD		dwFlags;
        FOURCC		fccIOProc;
        LPMMIOPROC32	pIOProc;
        UINT32		wErrorRet;
        HTASK32		htask;
        /* fields maintained by MMIO functions during buffered I/O */
        LONG		cchBuffer;
        HPSTR		pchBuffer;
        HPSTR		pchNext;
        HPSTR		pchEndRead;
        HPSTR		pchEndWrite;
        LONG		lBufOffset;
        /* fields maintained by I/O procedure */
        LONG		lDiskOffset;
        DWORD		adwInfo[3];
        /* other fields maintained by MMIO */
        DWORD		dwReserved1;
        DWORD		dwReserved2;
        HMMIO32		hmmio;
} MMIOINFO32, *LPMMIOINFO32;

typedef struct _MMCKINFO
{
        FOURCC          ckid;           /* chunk ID */
        DWORD           cksize;         /* chunk size */
        FOURCC          fccType;        /* form type or list type */
        DWORD           dwDataOffset;   /* offset of data portion of chunk */
        DWORD           dwFlags;        /* flags used by MMIO functions */
} MMCKINFO, *LPMMCKINFO;

#define MMIO_RWMODE     0x00000003      /* open file for reading/writing/both */
#define MMIO_SHAREMODE  0x00000070      /* file sharing mode number */

#define MMIO_CREATE     0x00001000      /* create new file (or truncate file) */
#define MMIO_PARSE      0x00000100      /* parse new file returning path */
#define MMIO_DELETE     0x00000200      /* create new file (or truncate file) */
#define MMIO_EXIST      0x00004000      /* checks for existence of file */
#define MMIO_ALLOCBUF   0x00010000      /* mmioOpen() should allocate a buffer */
#define MMIO_GETTEMP    0x00020000      /* mmioOpen() should retrieve temp name */

#define MMIO_DIRTY      0x10000000      /* I/O buffer is dirty */

#define MMIO_READ       0x00000000      /* open file for reading only */
#define MMIO_WRITE      0x00000001      /* open file for writing only */
#define MMIO_READWRITE  0x00000002      /* open file for reading and writing */

#define MMIO_COMPAT     0x00000000      /* compatibility mode */
#define MMIO_EXCLUSIVE  0x00000010      /* exclusive-access mode */
#define MMIO_DENYWRITE  0x00000020      /* deny writing to other processes */
#define MMIO_DENYREAD   0x00000030      /* deny reading to other processes */
#define MMIO_DENYNONE   0x00000040      /* deny nothing to other processes */

#define MMIO_FHOPEN             0x0010  /* mmioClose: keep file handle open */
#define MMIO_EMPTYBUF           0x0010  /* mmioFlush: empty the I/O buffer */
#define MMIO_TOUPPER            0x0010  /* mmioStringToFOURCC: to u-case */
#define MMIO_INSTALLPROC    0x00010000  /* mmioInstallIOProc: install MMIOProc */
#define MMIO_GLOBALPROC     0x10000000  /* mmioInstallIOProc: install globally */
#define MMIO_REMOVEPROC     0x00020000  /* mmioInstallIOProc: remove MMIOProc */
#define MMIO_FINDPROC       0x00040000  /* mmioInstallIOProc: find an MMIOProc */
#define MMIO_FINDCHUNK          0x0010  /* mmioDescend: find a chunk by ID */
#define MMIO_FINDRIFF           0x0020  /* mmioDescend: find a LIST chunk */
#define MMIO_FINDLIST           0x0040  /* mmioDescend: find a RIFF chunk */
#define MMIO_CREATERIFF         0x0020  /* mmioCreateChunk: make a LIST chunk */
#define MMIO_CREATELIST         0x0040  /* mmioCreateChunk: make a RIFF chunk */

#define SEEK_SET   0
#define SEEK_CUR   1
#define SEEK_END   2

#define MMIOM_READ      MMIO_READ       /* read */
#define MMIOM_WRITE    MMIO_WRITE       /* write */
#define MMIOM_SEEK              2       /* seek to a new position in file */
#define MMIOM_OPEN              3       /* open file */
#define MMIOM_CLOSE             4       /* close file */
#define MMIOM_WRITEFLUSH        5       /* write and flush */

#define MMIOM_RENAME            6       /* rename specified file */

#define MMIOM_USER         0x8000       /* beginning of user-defined messages */

#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST     mmioFOURCC('L', 'I', 'S', 'T')

#define FOURCC_DOS      mmioFOURCC('D', 'O', 'S', ' ')
#define FOURCC_MEM      mmioFOURCC('M', 'E', 'M', ' ')

#define MMIO_DEFAULTBUFFER      8192    /* default buffer size */

#define mmioFOURCC( ch0, ch1, ch2, ch3 )                                \
                ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
                ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

LPMMIOPROC16 WINAPI mmioInstallIOProc16(FOURCC,LPMMIOPROC16,DWORD);
LPMMIOPROC32 WINAPI mmioInstallIOProc32A(FOURCC,LPMMIOPROC32,DWORD);
LPMMIOPROC32 WINAPI mmioInstallIOProc32W(FOURCC,LPMMIOPROC32,DWORD);
#define      mmioInstallIOPro WINELIB_NAME_AW(mmioInstallIOProc)

FOURCC WINAPI mmioStringToFOURCC16(LPCSTR sz, UINT16 uFlags);
FOURCC WINAPI mmioStringToFOURCC32A(LPCSTR sz, UINT32 uFlags);
FOURCC WINAPI mmioStringToFOURCC32W(LPCWSTR sz, UINT32 uFlags);
#define mmioStringToFOURCC WINELIB_NAME_AW(mmioStringToFOURCC)
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16 * lpmmioinfo, DWORD dwOpenFlags);
HMMIO32 WINAPI mmioOpen32A(LPSTR szFileName, MMIOINFO32 * lpmmioinfo, DWORD dwOpenFlags);
HMMIO32 WINAPI mmioOpen32W(LPWSTR szFileName, MMIOINFO32 * lpmmioinfo, DWORD dwOpenFlags);
#define mmioOpen WINELIB_NAME_AW(mmioOpen)

UINT16 WINAPI mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
     MMIOINFO16 * lpmmioinfo, DWORD dwRenameFlags);

UINT16 WINAPI mmioClose(HMMIO16 hmmio, UINT16 uFlags);
LONG WINAPI mmioRead(HMMIO16 hmmio, HPSTR pch, LONG cch);
LONG WINAPI mmioWrite(HMMIO16 hmmio, HPCSTR pch, LONG cch);
LONG WINAPI mmioSeek(HMMIO16 hmmio, LONG lOffset, int iOrigin);
UINT16 WINAPI mmioGetInfo(HMMIO16 hmmio, MMIOINFO16 * lpmmioinfo, UINT16 uFlags);
UINT16 WINAPI mmioSetInfo(HMMIO16 hmmio, const MMIOINFO16 * lpmmioinfo, UINT16 uFlags);
UINT16 WINAPI mmioSetBuffer(HMMIO16 hmmio, LPSTR pchBuffer, LONG cchBuffer,
    UINT16 uFlags);
UINT16 WINAPI mmioFlush(HMMIO16 hmmio, UINT16 uFlags);
UINT16 WINAPI mmioAdvance(HMMIO16 hmmio, MMIOINFO16 * lpmmioinfo, UINT16 uFlags);
LONG WINAPI mmioSendMessage(HMMIO16 hmmio, UINT16 uMessage,
    LPARAM lParam1, LPARAM lParam2);
UINT16 WINAPI mmioDescend(HMMIO16 hmmio, MMCKINFO * lpck,
    const MMCKINFO * lpckParent, UINT16 uFlags);
UINT16 WINAPI mmioAscend(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags);
UINT16 WINAPI mmioCreateChunk(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags);

typedef UINT16 (CALLBACK *YIELDPROC) (UINT16 uDeviceID, DWORD dwYieldData);

DWORD WINAPI mciSendCommand (UINT16 uDeviceID, UINT16 uMessage,
                             DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI mciSendString (LPCSTR lpstrCommand,
                            LPSTR lpstrReturnString, UINT16 uReturnLength,
                            HWND16 hwndCallback);
UINT16 WINAPI mciGetDeviceID (LPCSTR lpstrName);
UINT16 WINAPI mciGetDeviceIDFromElementID (DWORD dwElementID,
                                           LPCSTR lpstrType);
BOOL16 WINAPI mciGetErrorString16 (DWORD,LPSTR,UINT16);
BOOL32 WINAPI mciGetErrorString32A(DWORD,LPSTR,UINT32);
BOOL32 WINAPI mciGetErrorString32W(DWORD,LPWSTR,UINT32);
#define mciGetErrorString WINELIB_NAME_AW(mciGetErrorString)
BOOL16 WINAPI mciSetYieldProc (UINT16 uDeviceID, YIELDPROC fpYieldProc,
                               DWORD dwYieldData);

HTASK16 WINAPI mciGetCreatorTask(UINT16 uDeviceID);
YIELDPROC WINAPI mciGetYieldProc (UINT16 uDeviceID, DWORD * lpdwYieldData);

#define MCIERR_INVALID_DEVICE_ID        (MCIERR_BASE + 1)
#define MCIERR_UNRECOGNIZED_KEYWORD     (MCIERR_BASE + 3)
#define MCIERR_UNRECOGNIZED_COMMAND     (MCIERR_BASE + 5)
#define MCIERR_HARDWARE                 (MCIERR_BASE + 6)
#define MCIERR_INVALID_DEVICE_NAME      (MCIERR_BASE + 7)
#define MCIERR_OUT_OF_MEMORY            (MCIERR_BASE + 8)
#define MCIERR_DEVICE_OPEN              (MCIERR_BASE + 9)
#define MCIERR_CANNOT_LOAD_DRIVER       (MCIERR_BASE + 10)
#define MCIERR_MISSING_COMMAND_STRING   (MCIERR_BASE + 11)
#define MCIERR_PARAM_OVERFLOW           (MCIERR_BASE + 12)
#define MCIERR_MISSING_STRING_ARGUMENT  (MCIERR_BASE + 13)
#define MCIERR_BAD_INTEGER              (MCIERR_BASE + 14)
#define MCIERR_PARSER_INTERNAL          (MCIERR_BASE + 15)
#define MCIERR_DRIVER_INTERNAL          (MCIERR_BASE + 16)
#define MCIERR_MISSING_PARAMETER        (MCIERR_BASE + 17)
#define MCIERR_UNSUPPORTED_FUNCTION     (MCIERR_BASE + 18)
#define MCIERR_FILE_NOT_FOUND           (MCIERR_BASE + 19)
#define MCIERR_DEVICE_NOT_READY         (MCIERR_BASE + 20)
#define MCIERR_INTERNAL                 (MCIERR_BASE + 21)
#define MCIERR_DRIVER                   (MCIERR_BASE + 22)
#define MCIERR_CANNOT_USE_ALL           (MCIERR_BASE + 23)
#define MCIERR_MULTIPLE                 (MCIERR_BASE + 24)
#define MCIERR_EXTENSION_NOT_FOUND      (MCIERR_BASE + 25)
#define MCIERR_OUTOFRANGE               (MCIERR_BASE + 26)
#define MCIERR_FLAGS_NOT_COMPATIBLE     (MCIERR_BASE + 28)
#define MCIERR_FILE_NOT_SAVED           (MCIERR_BASE + 30)
#define MCIERR_DEVICE_TYPE_REQUIRED     (MCIERR_BASE + 31)
#define MCIERR_DEVICE_LOCKED            (MCIERR_BASE + 32)
#define MCIERR_DUPLICATE_ALIAS          (MCIERR_BASE + 33)
#define MCIERR_BAD_CONSTANT             (MCIERR_BASE + 34)
#define MCIERR_MUST_USE_SHAREABLE       (MCIERR_BASE + 35)
#define MCIERR_MISSING_DEVICE_NAME      (MCIERR_BASE + 36)
#define MCIERR_BAD_TIME_FORMAT          (MCIERR_BASE + 37)
#define MCIERR_NO_CLOSING_QUOTE         (MCIERR_BASE + 38)
#define MCIERR_DUPLICATE_FLAGS          (MCIERR_BASE + 39)
#define MCIERR_INVALID_FILE             (MCIERR_BASE + 40)
#define MCIERR_NULL_PARAMETER_BLOCK     (MCIERR_BASE + 41)
#define MCIERR_UNNAMED_RESOURCE         (MCIERR_BASE + 42)
#define MCIERR_NEW_REQUIRES_ALIAS       (MCIERR_BASE + 43)
#define MCIERR_NOTIFY_ON_AUTO_OPEN      (MCIERR_BASE + 44)
#define MCIERR_NO_ELEMENT_ALLOWED       (MCIERR_BASE + 45)
#define MCIERR_NONAPPLICABLE_FUNCTION   (MCIERR_BASE + 46)
#define MCIERR_ILLEGAL_FOR_AUTO_OPEN    (MCIERR_BASE + 47)
#define MCIERR_FILENAME_REQUIRED        (MCIERR_BASE + 48)
#define MCIERR_EXTRA_CHARACTERS         (MCIERR_BASE + 49)
#define MCIERR_DEVICE_NOT_INSTALLED     (MCIERR_BASE + 50)
#define MCIERR_GET_CD                   (MCIERR_BASE + 51)
#define MCIERR_SET_CD                   (MCIERR_BASE + 52)
#define MCIERR_SET_DRIVE                (MCIERR_BASE + 53)
#define MCIERR_DEVICE_LENGTH            (MCIERR_BASE + 54)
#define MCIERR_DEVICE_ORD_LENGTH        (MCIERR_BASE + 55)
#define MCIERR_NO_INTEGER               (MCIERR_BASE + 56)

#define MCIERR_WAVE_OUTPUTSINUSE        (MCIERR_BASE + 64)
#define MCIERR_WAVE_SETOUTPUTINUSE      (MCIERR_BASE + 65)
#define MCIERR_WAVE_INPUTSINUSE         (MCIERR_BASE + 66)
#define MCIERR_WAVE_SETINPUTINUSE       (MCIERR_BASE + 67)
#define MCIERR_WAVE_OUTPUTUNSPECIFIED   (MCIERR_BASE + 68)
#define MCIERR_WAVE_INPUTUNSPECIFIED    (MCIERR_BASE + 69)
#define MCIERR_WAVE_OUTPUTSUNSUITABLE   (MCIERR_BASE + 70)
#define MCIERR_WAVE_SETOUTPUTUNSUITABLE (MCIERR_BASE + 71)
#define MCIERR_WAVE_INPUTSUNSUITABLE    (MCIERR_BASE + 72)
#define MCIERR_WAVE_SETINPUTUNSUITABLE  (MCIERR_BASE + 73)

#define MCIERR_SEQ_DIV_INCOMPATIBLE     (MCIERR_BASE + 80)
#define MCIERR_SEQ_PORT_INUSE           (MCIERR_BASE + 81)
#define MCIERR_SEQ_PORT_NONEXISTENT     (MCIERR_BASE + 82)
#define MCIERR_SEQ_PORT_MAPNODEVICE     (MCIERR_BASE + 83)
#define MCIERR_SEQ_PORT_MISCERROR       (MCIERR_BASE + 84)
#define MCIERR_SEQ_TIMER                (MCIERR_BASE + 85)
#define MCIERR_SEQ_PORTUNSPECIFIED      (MCIERR_BASE + 86)
#define MCIERR_SEQ_NOMIDIPRESENT        (MCIERR_BASE + 87)

#define MCIERR_NO_WINDOW                (MCIERR_BASE + 90)
#define MCIERR_CREATEWINDOW             (MCIERR_BASE + 91)
#define MCIERR_FILE_READ                (MCIERR_BASE + 92)
#define MCIERR_FILE_WRITE               (MCIERR_BASE + 93)

#define MCIERR_CUSTOM_DRIVER_BASE       (MCIERR_BASE + 256)

#define MCI_OPEN_DRIVER					0x0801
#define MCI_CLOSE_DRIVER				0x0802
#define MCI_OPEN						0x0803
#define MCI_CLOSE						0x0804
#define MCI_ESCAPE                      0x0805
#define MCI_PLAY                        0x0806
#define MCI_SEEK                        0x0807
#define MCI_STOP                        0x0808
#define MCI_PAUSE                       0x0809
#define MCI_INFO                        0x080A
#define MCI_GETDEVCAPS                  0x080B
#define MCI_SPIN                        0x080C
#define MCI_SET                         0x080D
#define MCI_STEP                        0x080E
#define MCI_RECORD                      0x080F
#define MCI_SYSINFO                     0x0810
#define MCI_BREAK                       0x0811
#define MCI_SOUND                       0x0812
#define MCI_SAVE                        0x0813
#define MCI_STATUS                      0x0814
#define MCI_CUE                         0x0830
#define MCI_REALIZE                     0x0840
#define MCI_WINDOW                      0x0841
#define MCI_PUT                         0x0842
#define MCI_WHERE                       0x0843
#define MCI_FREEZE                      0x0844
#define MCI_UNFREEZE                    0x0845
#define MCI_LOAD                        0x0850
#define MCI_CUT                         0x0851
#define MCI_COPY                        0x0852
#define MCI_PASTE                       0x0853
#define MCI_UPDATE                      0x0854
#define MCI_RESUME                      0x0855
#define MCI_DELETE                      0x0856

#define MCI_USER_MESSAGES               (0x400 + DRV_MCI_FIRST)

#define MCI_ALL_DEVICE_ID               0xFFFF

#define MCI_DEVTYPE_VCR                 (MCI_STRING_OFFSET + 1)
#define MCI_DEVTYPE_VIDEODISC           (MCI_STRING_OFFSET + 2)
#define MCI_DEVTYPE_OVERLAY             (MCI_STRING_OFFSET + 3)
#define MCI_DEVTYPE_CD_AUDIO            (MCI_STRING_OFFSET + 4)
#define MCI_DEVTYPE_DAT                 (MCI_STRING_OFFSET + 5)
#define MCI_DEVTYPE_SCANNER             (MCI_STRING_OFFSET + 6)
#define MCI_DEVTYPE_ANIMATION           (MCI_STRING_OFFSET + 7)
#define MCI_DEVTYPE_DIGITAL_VIDEO       (MCI_STRING_OFFSET + 8)
#define MCI_DEVTYPE_OTHER               (MCI_STRING_OFFSET + 9)
#define MCI_DEVTYPE_WAVEFORM_AUDIO      (MCI_STRING_OFFSET + 10)
#define MCI_DEVTYPE_SEQUENCER           (MCI_STRING_OFFSET + 11)

#define MCI_DEVTYPE_FIRST               MCI_DEVTYPE_VCR
#define MCI_DEVTYPE_LAST                MCI_DEVTYPE_SEQUENCER

#define MCI_MODE_NOT_READY              (MCI_STRING_OFFSET + 12)
#define MCI_MODE_STOP                   (MCI_STRING_OFFSET + 13)
#define MCI_MODE_PLAY                   (MCI_STRING_OFFSET + 14)
#define MCI_MODE_RECORD                 (MCI_STRING_OFFSET + 15)
#define MCI_MODE_SEEK                   (MCI_STRING_OFFSET + 16)
#define MCI_MODE_PAUSE                  (MCI_STRING_OFFSET + 17)
#define MCI_MODE_OPEN                   (MCI_STRING_OFFSET + 18)

#define MCI_FORMAT_MILLISECONDS         0
#define MCI_FORMAT_HMS                  1
#define MCI_FORMAT_MSF                  2
#define MCI_FORMAT_FRAMES               3
#define MCI_FORMAT_SMPTE_24             4
#define MCI_FORMAT_SMPTE_25             5
#define MCI_FORMAT_SMPTE_30             6
#define MCI_FORMAT_SMPTE_30DROP         7
#define MCI_FORMAT_BYTES                8
#define MCI_FORMAT_SAMPLES              9
#define MCI_FORMAT_TMSF                 10

#define MCI_MSF_MINUTE(msf)             ((BYTE)(msf))
#define MCI_MSF_SECOND(msf)             ((BYTE)(((WORD)(msf)) >> 8))
#define MCI_MSF_FRAME(msf)              ((BYTE)((msf)>>16))

#define MCI_MAKE_MSF(m, s, f)           ((DWORD)(((BYTE)(m) | \
                                                  ((WORD)(s)<<8)) | \
                                                 (((DWORD)(BYTE)(f))<<16)))

#define MCI_TMSF_TRACK(tmsf)            ((BYTE)(tmsf))
#define MCI_TMSF_MINUTE(tmsf)           ((BYTE)(((WORD)(tmsf)) >> 8))
#define MCI_TMSF_SECOND(tmsf)           ((BYTE)((tmsf)>>16))
#define MCI_TMSF_FRAME(tmsf)            ((BYTE)((tmsf)>>24))

#define MCI_MAKE_TMSF(t, m, s, f)       ((DWORD)(((BYTE)(t) | \
                                                  ((WORD)(m)<<8)) | \
                                                 (((DWORD)(BYTE)(s) | \
                                                   ((WORD)(f)<<8))<<16)))

#define MCI_HMS_HOUR(hms)               ((BYTE)(hms))
#define MCI_HMS_MINUTE(hms)             ((BYTE)(((WORD)(hms)) >> 8))
#define MCI_HMS_SECOND(hms)             ((BYTE)((hms)>>16))

#define MCI_MAKE_HMS(h, m, s)           ((DWORD)(((BYTE)(h) | \
                                                  ((WORD)(m)<<8)) | \
                                                 (((DWORD)(BYTE)(s))<<16)))

#define MCI_NOTIFY_SUCCESSFUL           0x0001
#define MCI_NOTIFY_SUPERSEDED           0x0002
#define MCI_NOTIFY_ABORTED              0x0004
#define MCI_NOTIFY_FAILURE              0x0008

#define MCI_NOTIFY                      0x00000001L
#define MCI_WAIT                        0x00000002L
#define MCI_FROM                        0x00000004L
#define MCI_TO                          0x00000008L
#define MCI_TRACK                       0x00000010L

#define MCI_OPEN_SHAREABLE              0x00000100L
#define MCI_OPEN_ELEMENT                0x00000200L
#define MCI_OPEN_ALIAS                  0x00000400L
#define MCI_OPEN_ELEMENT_ID             0x00000800L
#define MCI_OPEN_TYPE_ID                0x00001000L
#define MCI_OPEN_TYPE                   0x00002000L

#define MCI_SEEK_TO_START               0x00000100L
#define MCI_SEEK_TO_END                 0x00000200L

#define MCI_STATUS_ITEM                 0x00000100L
#define MCI_STATUS_START                0x00000200L

#define MCI_STATUS_LENGTH               0x00000001L
#define MCI_STATUS_POSITION             0x00000002L
#define MCI_STATUS_NUMBER_OF_TRACKS     0x00000003L
#define MCI_STATUS_MODE                 0x00000004L
#define MCI_STATUS_MEDIA_PRESENT        0x00000005L
#define MCI_STATUS_TIME_FORMAT          0x00000006L
#define MCI_STATUS_READY                0x00000007L
#define MCI_STATUS_CURRENT_TRACK        0x00000008L

#define MCI_INFO_PRODUCT                0x00000100L
#define MCI_INFO_FILE                   0x00000200L

#define MCI_GETDEVCAPS_ITEM             0x00000100L

#define MCI_GETDEVCAPS_CAN_RECORD       0x00000001L
#define MCI_GETDEVCAPS_HAS_AUDIO        0x00000002L
#define MCI_GETDEVCAPS_HAS_VIDEO        0x00000003L
#define MCI_GETDEVCAPS_DEVICE_TYPE      0x00000004L
#define MCI_GETDEVCAPS_USES_FILES       0x00000005L
#define MCI_GETDEVCAPS_COMPOUND_DEVICE  0x00000006L
#define MCI_GETDEVCAPS_CAN_EJECT        0x00000007L
#define MCI_GETDEVCAPS_CAN_PLAY         0x00000008L
#define MCI_GETDEVCAPS_CAN_SAVE         0x00000009L

#define MCI_SYSINFO_QUANTITY            0x00000100L
#define MCI_SYSINFO_OPEN                0x00000200L
#define MCI_SYSINFO_NAME                0x00000400L
#define MCI_SYSINFO_INSTALLNAME         0x00000800L

#define MCI_SET_DOOR_OPEN               0x00000100L
#define MCI_SET_DOOR_CLOSED             0x00000200L
#define MCI_SET_TIME_FORMAT             0x00000400L
#define MCI_SET_AUDIO                   0x00000800L
#define MCI_SET_VIDEO                   0x00001000L
#define MCI_SET_ON                      0x00002000L
#define MCI_SET_OFF                     0x00004000L

#define MCI_SET_AUDIO_ALL               0x00000000L
#define MCI_SET_AUDIO_LEFT              0x00000001L
#define MCI_SET_AUDIO_RIGHT             0x00000002L

#define MCI_BREAK_KEY                   0x00000100L
#define MCI_BREAK_HWND                  0x00000200L
#define MCI_BREAK_OFF                   0x00000400L

#define MCI_RECORD_INSERT               0x00000100L
#define MCI_RECORD_OVERWRITE            0x00000200L

#define MCI_SOUND_NAME                  0x00000100L

#define MCI_SAVE_FILE                   0x00000100L

#define MCI_LOAD_FILE                   0x00000100L

typedef struct {
	DWORD   dwCallback;
} MCI_GENERIC_PARMS, *LPMCI_GENERIC_PARMS;

typedef struct {
	DWORD	dwCallback;
	WORD	wDeviceID;
	WORD	wReserved0;
	LPSTR	lpstrDeviceType;
	LPSTR	lpstrElementName;
	LPSTR	lpstrAlias;
} MCI_OPEN_PARMS16, *LPMCI_OPEN_PARMS16;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPSTR		lpstrDeviceType;
	LPSTR		lpstrElementName;
	LPSTR		lpstrAlias;
} MCI_OPEN_PARMS32A, *LPMCI_OPEN_PARMS32A;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPWSTR		lpstrDeviceType;
	LPWSTR		lpstrElementName;
	LPWSTR		lpstrAlias;
} MCI_OPEN_PARMS32W, *LPMCI_OPEN_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_OPEN_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_OPEN_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
} MCI_PLAY_PARMS, *LPMCI_PLAY_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwTo;
} MCI_SEEK_PARMS, *LPMCI_SEEK_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwReturn;
	DWORD   dwItem;
	DWORD   dwTrack;
} MCI_STATUS_PARMS, *LPMCI_STATUS_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPSTR   lpstrReturn;
	DWORD   dwRetSize;
} MCI_INFO_PARMS16, *LPMCI_INFO_PARMS16;

typedef struct {
	DWORD   dwCallback;
	LPSTR   lpstrReturn;
	DWORD   dwRetSize;
} MCI_INFO_PARMS32A, *LPMCI_INFO_PARMS32A;

typedef struct {
	DWORD   dwCallback;
	LPSTR   lpstrReturn;
	DWORD   dwRetSize;
} MCI_INFO_PARMS32W, *LPMCI_INFO_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_INFO_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_INFO_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwReturn;
	DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS, *LPMCI_GETDEVCAPS_PARMS;

typedef struct {
	DWORD	dwCallback;
	LPSTR	lpstrReturn;
	DWORD	dwRetSize;
	DWORD	dwNumber;
	WORD	wDeviceType;
	WORD	wReserved0;
} MCI_SYSINFO_PARMS16, *LPMCI_SYSINFO_PARMS16;

typedef struct {
	DWORD	dwCallback;
	LPSTR	lpstrReturn;
	DWORD	dwRetSize;
	DWORD	dwNumber;
	UINT32	wDeviceType;
} MCI_SYSINFO_PARMS32A, *LPMCI_SYSINFO_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	LPWSTR	lpstrReturn;
	DWORD	dwRetSize;
	DWORD	dwNumber;
	UINT32	wDeviceType;
} MCI_SYSINFO_PARMS32W, *LPMCI_SYSINFO_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_SYSINFO_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_SYSINFO_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwTimeFormat;
	DWORD   dwAudio;
} MCI_SET_PARMS, *LPMCI_SET_PARMS;

typedef struct {
	DWORD	dwCallback;
	UINT16	nVirtKey;
	WORD	wReserved0;
	HWND16	hwndBreak;
	WORD	wReserved1;
} MCI_BREAK_PARMS16, *LPMCI_BREAK_PARMS16;

typedef struct {
	DWORD	dwCallback;
	INT32	nVirtKey;
	HWND32	hwndBreak;
} MCI_BREAK_PARMS32, *LPMCI_BREAK_PARMS32;

DECL_WINELIB_TYPE(MCI_BREAK_PARMS);
DECL_WINELIB_TYPE(LPMCI_BREAK_PARMS);

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpstrSoundName;
} MCI_SOUND_PARMS, *LPMCI_SOUND_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
} MCI_SAVE_PARMS, *LPMCI_SAVE_PARMS;

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpfilename;
} MCI_LOAD_PARMS16, *LPMCI_LOAD_PARMS16;

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpfilename;
} MCI_LOAD_PARMS32A, *LPMCI_LOAD_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	LPCWSTR	lpfilename;
} MCI_LOAD_PARMS32W, *LPMCI_LOAD_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_LOAD_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_LOAD_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
} MCI_RECORD_PARMS, *LPMCI_RECORD_PARMS;

#define MCI_VD_MODE_PARK                (MCI_VD_OFFSET + 1)

#define MCI_VD_MEDIA_CLV                (MCI_VD_OFFSET + 2)
#define MCI_VD_MEDIA_CAV                (MCI_VD_OFFSET + 3)
#define MCI_VD_MEDIA_OTHER              (MCI_VD_OFFSET + 4)

#define MCI_VD_FORMAT_TRACK             0x4001

#define MCI_VD_PLAY_REVERSE             0x00010000L
#define MCI_VD_PLAY_FAST                0x00020000L
#define MCI_VD_PLAY_SPEED               0x00040000L
#define MCI_VD_PLAY_SCAN                0x00080000L
#define MCI_VD_PLAY_SLOW                0x00100000L

#define MCI_VD_SEEK_REVERSE             0x00010000L

#define MCI_VD_STATUS_SPEED             0x00004002L
#define MCI_VD_STATUS_FORWARD           0x00004003L
#define MCI_VD_STATUS_MEDIA_TYPE        0x00004004L
#define MCI_VD_STATUS_SIDE              0x00004005L
#define MCI_VD_STATUS_DISC_SIZE         0x00004006L

#define MCI_VD_GETDEVCAPS_CLV           0x00010000L
#define MCI_VD_GETDEVCAPS_CAV           0x00020000L

#define MCI_VD_SPIN_UP                  0x00010000L
#define MCI_VD_SPIN_DOWN                0x00020000L

#define MCI_VD_GETDEVCAPS_CAN_REVERSE   0x00004002L
#define MCI_VD_GETDEVCAPS_FAST_RATE     0x00004003L
#define MCI_VD_GETDEVCAPS_SLOW_RATE     0x00004004L
#define MCI_VD_GETDEVCAPS_NORMAL_RATE   0x00004005L

#define MCI_VD_STEP_FRAMES              0x00010000L
#define MCI_VD_STEP_REVERSE             0x00020000L

#define MCI_VD_ESCAPE_STRING            0x00000100L

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
	DWORD   dwSpeed;
} MCI_VD_PLAY_PARMS, *LPMCI_VD_PLAY_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrames;
} MCI_VD_STEP_PARMS, *LPMCI_VD_STEP_PARMS;

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpstrCommand;
} MCI_VD_ESCAPE_PARMS16, *LPMCI_VD_ESCAPE_PARMS16;

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpstrCommand;
} MCI_VD_ESCAPE_PARMS32A, *LPMCI_VD_ESCAPE_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	LPCWSTR	lpstrCommand;
} MCI_VD_ESCAPE_PARMS32W, *LPMCI_VD_ESCAPE_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_VD_ESCAPE_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_VD_ESCAPE_PARMS);

#define MCI_WAVE_OPEN_BUFFER            0x00010000L

#define MCI_WAVE_SET_FORMATTAG          0x00010000L
#define MCI_WAVE_SET_CHANNELS           0x00020000L
#define MCI_WAVE_SET_SAMPLESPERSEC      0x00040000L
#define MCI_WAVE_SET_AVGBYTESPERSEC     0x00080000L
#define MCI_WAVE_SET_BLOCKALIGN         0x00100000L
#define MCI_WAVE_SET_BITSPERSAMPLE      0x00200000L

#define MCI_WAVE_INPUT                  0x00400000L
#define MCI_WAVE_OUTPUT                 0x00800000L

#define MCI_WAVE_STATUS_FORMATTAG       0x00004001L
#define MCI_WAVE_STATUS_CHANNELS        0x00004002L
#define MCI_WAVE_STATUS_SAMPLESPERSEC   0x00004003L
#define MCI_WAVE_STATUS_AVGBYTESPERSEC  0x00004004L
#define MCI_WAVE_STATUS_BLOCKALIGN      0x00004005L
#define MCI_WAVE_STATUS_BITSPERSAMPLE   0x00004006L
#define MCI_WAVE_STATUS_LEVEL           0x00004007L

#define MCI_WAVE_SET_ANYINPUT           0x04000000L
#define MCI_WAVE_SET_ANYOUTPUT          0x08000000L

#define MCI_WAVE_GETDEVCAPS_INPUTS      0x00004001L
#define MCI_WAVE_GETDEVCAPS_OUTPUTS     0x00004002L

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID16	wDeviceID;
	WORD		wReserved0;
	SEGPTR		lpstrDeviceType;
	SEGPTR		lpstrElementName;
	SEGPTR		lpstrAlias;
	DWORD		dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS16, *LPMCI_WAVE_OPEN_PARMS16;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCSTR		lpstrDeviceType;
	LPCSTR		lpstrElementName;
	LPCSTR		lpstrAlias;
	DWORD   	dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS32A, *LPMCI_WAVE_OPEN_PARMS32A;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCWSTR		lpstrDeviceType;
	LPCWSTR		lpstrElementName;
	LPCWSTR		lpstrAlias;
	DWORD   	dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS32W, *LPMCI_WAVE_OPEN_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_WAVE_OPEN_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_WAVE_OPEN_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
} MCI_WAVE_DELETE_PARMS, *LPMCI_WAVE_DELETE_PARMS;

typedef struct {
	DWORD	dwCallback;
	DWORD	dwTimeFormat;
	DWORD	dwAudio;
	UINT16	wInput;
	UINT16	wReserved0;
	UINT16	wOutput;
	UINT16	wReserved1;
	UINT16	wFormatTag;
	UINT16	wReserved2;
	UINT16	nChannels;
	UINT16	wReserved3;
	DWORD	nSamplesPerSec;
	DWORD	nAvgBytesPerSec;
	UINT16	nBlockAlign;
	UINT16	wReserved4;
	UINT16	wBitsPerSample;
	UINT16	wReserved5;
} MCI_WAVE_SET_PARMS16, * LPMCI_WAVE_SET_PARMS16;

typedef struct {
	DWORD	dwCallback;
	DWORD	dwTimeFormat;
	DWORD	dwAudio;
	UINT32	wInput;
	UINT32	wOutput;
	UINT32	wFormatTag;
	UINT32	nChannels;
	DWORD	nSamplesPerSec;
	DWORD	nAvgBytesPerSec;
	UINT32	nBlockAlign;
	UINT32	wBitsPerSample;
} MCI_WAVE_SET_PARMS32, * LPMCI_WAVE_SET_PARMS32;

DECL_WINELIB_TYPE(MCI_WAVE_SET_PARMS);
DECL_WINELIB_TYPE(LPMCI_WAVE_SET_PARMS);

#define     MCI_SEQ_DIV_PPQN            (0 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_24        (1 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_25        (2 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30DROP    (3 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30        (4 + MCI_SEQ_OFFSET)

#define     MCI_SEQ_FORMAT_SONGPTR      0x4001
#define     MCI_SEQ_FILE                0x4002
#define     MCI_SEQ_MIDI                0x4003
#define     MCI_SEQ_SMPTE               0x4004
#define     MCI_SEQ_NONE                65533

#define MCI_SEQ_STATUS_TEMPO            0x00004002L
#define MCI_SEQ_STATUS_PORT             0x00004003L
#define MCI_SEQ_STATUS_SLAVE            0x00004007L
#define MCI_SEQ_STATUS_MASTER           0x00004008L
#define MCI_SEQ_STATUS_OFFSET           0x00004009L
#define MCI_SEQ_STATUS_DIVTYPE          0x0000400AL

#define MCI_SEQ_SET_TEMPO               0x00010000L
#define MCI_SEQ_SET_PORT                0x00020000L
#define MCI_SEQ_SET_SLAVE               0x00040000L
#define MCI_SEQ_SET_MASTER              0x00080000L
#define MCI_SEQ_SET_OFFSET              0x01000000L

typedef struct {
	DWORD   dwCallback;
	DWORD   dwTimeFormat;
	DWORD   dwAudio;
	DWORD   dwTempo;
	DWORD   dwPort;
	DWORD   dwSlave;
	DWORD   dwMaster;
	DWORD   dwOffset;
} MCI_SEQ_SET_PARMS, *LPMCI_SEQ_SET_PARMS;

#define MCI_ANIM_OPEN_WS                0x00010000L
#define MCI_ANIM_OPEN_PARENT            0x00020000L
#define MCI_ANIM_OPEN_NOSTATIC          0x00040000L

#define MCI_ANIM_PLAY_SPEED             0x00010000L
#define MCI_ANIM_PLAY_REVERSE           0x00020000L
#define MCI_ANIM_PLAY_FAST              0x00040000L
#define MCI_ANIM_PLAY_SLOW              0x00080000L
#define MCI_ANIM_PLAY_SCAN              0x00100000L

#define MCI_ANIM_STEP_REVERSE           0x00010000L
#define MCI_ANIM_STEP_FRAMES            0x00020000L

#define MCI_ANIM_STATUS_SPEED           0x00004001L
#define MCI_ANIM_STATUS_FORWARD         0x00004002L
#define MCI_ANIM_STATUS_HWND            0x00004003L
#define MCI_ANIM_STATUS_HPAL            0x00004004L
#define MCI_ANIM_STATUS_STRETCH         0x00004005L

#define MCI_ANIM_INFO_TEXT              0x00010000L

#define MCI_ANIM_GETDEVCAPS_CAN_REVERSE 0x00004001L
#define MCI_ANIM_GETDEVCAPS_FAST_RATE   0x00004002L
#define MCI_ANIM_GETDEVCAPS_SLOW_RATE   0x00004003L
#define MCI_ANIM_GETDEVCAPS_NORMAL_RATE 0x00004004L
#define MCI_ANIM_GETDEVCAPS_PALETTES    0x00004006L
#define MCI_ANIM_GETDEVCAPS_CAN_STRETCH 0x00004007L
#define MCI_ANIM_GETDEVCAPS_MAX_WINDOWS 0x00004008L

#define MCI_ANIM_REALIZE_NORM           0x00010000L
#define MCI_ANIM_REALIZE_BKGD           0x00020000L

#define MCI_ANIM_WINDOW_HWND            0x00010000L
#define MCI_ANIM_WINDOW_STATE           0x00040000L
#define MCI_ANIM_WINDOW_TEXT            0x00080000L
#define MCI_ANIM_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_ANIM_WINDOW_DISABLE_STRETCH 0x00200000L

#define MCI_ANIM_WINDOW_DEFAULT         0x00000000L

#define MCI_ANIM_RECT                   0x00010000L
#define MCI_ANIM_PUT_SOURCE             0x00020000L
#define MCI_ANIM_PUT_DESTINATION        0x00040000L

#define MCI_ANIM_WHERE_SOURCE           0x00020000L
#define MCI_ANIM_WHERE_DESTINATION      0x00040000L

#define MCI_ANIM_UPDATE_HDC             0x00020000L

typedef struct {
	DWORD   dwCallback;
	UINT16  wDeviceID;
	UINT16  wReserved0;
	SEGPTR  lpstrDeviceType;
	SEGPTR  lpstrElementName;
	SEGPTR  lpstrAlias;
	DWORD   dwStyle;
	HWND16  hWndParent;
	UINT16  wReserved1;
} MCI_ANIM_OPEN_PARMS16, *LPMCI_ANIM_OPEN_PARMS16;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCSTR		lpstrDeviceType;
	LPCSTR		lpstrElementName;
	LPCSTR		lpstrAlias;
	DWORD		dwStyle;
	HWND32		hWndParent;
} MCI_ANIM_OPEN_PARMS32A, *LPMCI_ANIM_OPEN_PARMS32A;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCSTR		lpstrDeviceType;
	LPCSTR		lpstrElementName;
	LPCSTR		lpstrAlias;
	DWORD		dwStyle;
	HWND32		hWndParent;
} MCI_ANIM_OPEN_PARMS32W, *LPMCI_ANIM_OPEN_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_ANIM_OPEN_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_ANIM_OPEN_PARMS);

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
	DWORD   dwSpeed;
} MCI_ANIM_PLAY_PARMS, *LPMCI_ANIM_PLAY_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrames;
} MCI_ANIM_STEP_PARMS, *LPMCI_ANIM_STEP_PARMS;

typedef struct {
	DWORD	dwCallback;
	HWND16	hWnd;
	WORD	wReserved1;
	WORD	nCmdShow;
	WORD	wReserved2;
	LPCSTR	lpstrText;
} MCI_ANIM_WINDOW_PARMS16, *LPMCI_ANIM_WINDOW_PARMS16;

typedef struct {
	DWORD	dwCallback;
	HWND32	hWnd;
	UINT32	nCmdShow;
	LPCSTR	lpstrText;
} MCI_ANIM_WINDOW_PARMS32A, *LPMCI_ANIM_WINDOW_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	HWND32	hWnd;
	UINT32	nCmdShow;
	LPCWSTR	lpstrText;
} MCI_ANIM_WINDOW_PARMS32W, *LPMCI_ANIM_WINDOW_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_ANIM_WINDOW_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_ANIM_WINDOW_PARMS);

typedef struct {
	DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT16 ptOffset;
	POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_ANIM_RECT_PARMS16, *LPMCI_ANIM_RECT_PARMS16;

typedef struct {
	DWORD	dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT32	ptOffset;
	POINT32	ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT32	rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_ANIM_RECT_PARMS32, *LPMCI_ANIM_RECT_PARMS32;

DECL_WINELIB_TYPE(MCI_ANIM_RECT_PARMS);
DECL_WINELIB_TYPE(LPMCI_ANIM_RECT_PARMS);

typedef struct {
	DWORD   dwCallback;
	RECT16  rc;
	HDC16   hDC;
} MCI_ANIM_UPDATE_PARMS16, *LPMCI_ANIM_UPDATE_PARMS16;

typedef struct {
	DWORD   dwCallback;
	RECT32  rc;
	HDC32   hDC;
} MCI_ANIM_UPDATE_PARMS32, *LPMCI_ANIM_UPDATE_PARMS32;

DECL_WINELIB_TYPE(MCI_ANIM_UPDATE_PARMS);
DECL_WINELIB_TYPE(LPMCI_ANIM_UPDATE_PARMS);

#define MCI_OVLY_OPEN_WS                0x00010000L
#define MCI_OVLY_OPEN_PARENT            0x00020000L

#define MCI_OVLY_STATUS_HWND            0x00004001L
#define MCI_OVLY_STATUS_STRETCH         0x00004002L

#define MCI_OVLY_INFO_TEXT              0x00010000L

#define MCI_OVLY_GETDEVCAPS_CAN_STRETCH 0x00004001L
#define MCI_OVLY_GETDEVCAPS_CAN_FREEZE  0x00004002L
#define MCI_OVLY_GETDEVCAPS_MAX_WINDOWS 0x00004003L

#define MCI_OVLY_WINDOW_HWND            0x00010000L
#define MCI_OVLY_WINDOW_STATE           0x00040000L
#define MCI_OVLY_WINDOW_TEXT            0x00080000L
#define MCI_OVLY_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_OVLY_WINDOW_DISABLE_STRETCH 0x00200000L

#define MCI_OVLY_WINDOW_DEFAULT         0x00000000L

#define MCI_OVLY_RECT                   0x00010000L
#define MCI_OVLY_PUT_SOURCE             0x00020000L
#define MCI_OVLY_PUT_DESTINATION        0x00040000L
#define MCI_OVLY_PUT_FRAME              0x00080000L
#define MCI_OVLY_PUT_VIDEO              0x00100000L

#define MCI_OVLY_WHERE_SOURCE           0x00020000L
#define MCI_OVLY_WHERE_DESTINATION      0x00040000L
#define MCI_OVLY_WHERE_FRAME            0x00080000L
#define MCI_OVLY_WHERE_VIDEO            0x00100000L

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID16	wDeviceID;
	WORD		wReserved0;
	LPCSTR		lpstrDeviceType;
	LPCSTR		lpstrElementName;
	LPCSTR		lpstrAlias;
	DWORD		dwStyle;
	HWND16		hWndParent;
	WORD		wReserved1;
} MCI_OVLY_OPEN_PARMS16, *LPMCI_OVLY_OPEN_PARMS16;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCSTR		lpstrDeviceType;
	LPCSTR		lpstrElementName;
	LPCSTR		lpstrAlias;
	DWORD		dwStyle;
	HWND32		hWndParent;
} MCI_OVLY_OPEN_PARMS32A, *LPMCI_OVLY_OPEN_PARMS32A;

typedef struct {
	DWORD		dwCallback;
	MCIDEVICEID32	wDeviceID;
	LPCWSTR		lpstrDeviceType;
	LPCWSTR		lpstrElementName;
	LPCWSTR		lpstrAlias;
	DWORD		dwStyle;
	HWND32		hWndParent;
} MCI_OVLY_OPEN_PARMS32W, *LPMCI_OVLY_OPEN_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_OVLY_OPEN_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_OVLY_OPEN_PARMS);

typedef struct {
	DWORD	dwCallback;
	HWND16	hWnd;
	WORD	wReserved1;
	UINT16	nCmdShow;
	WORD	wReserved2;
	LPCSTR	lpstrText;
} MCI_OVLY_WINDOW_PARMS16, *LPMCI_OVLY_WINDOW_PARMS16;

typedef struct {
	DWORD	dwCallback;
	HWND32	hWnd;
	UINT32	nCmdShow;
	LPCSTR	lpstrText;
} MCI_OVLY_WINDOW_PARMS32A, *LPMCI_OVLY_WINDOW_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	HWND32	hWnd;
	UINT32	nCmdShow;
	LPCWSTR	lpstrText;
} MCI_OVLY_WINDOW_PARMS32W, *LPMCI_OVLY_WINDOW_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_OVLY_WINDOW_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_OVLY_WINDOW_PARMS);

typedef struct {
	DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT16 ptOffset;
	POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_OVLY_RECT_PARMS16, *LPMCI_OVLY_RECT_PARMS16;

typedef struct {
	DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT32 ptOffset;
	POINT32 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT32  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_OVLY_RECT_PARMS32, *LPMCI_OVLY_RECT_PARMS32;

DECL_WINELIB_TYPE(MCI_OVLY_RECT_PARMS);
DECL_WINELIB_TYPE(LPMCI_OVLY_RECT_PARMS);

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
	RECT16  rc;
} MCI_OVLY_SAVE_PARMS16, *LPMCI_OVLY_SAVE_PARMS16;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
	RECT32  rc;
} MCI_OVLY_SAVE_PARMS32A, *LPMCI_OVLY_SAVE_PARMS32A;

typedef struct {
	DWORD   dwCallback;
	LPCWSTR  lpfilename;
	RECT32  rc;
} MCI_OVLY_SAVE_PARMS32W, *LPMCI_OVLY_SAVE_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_OVLY_SAVE_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_OVLY_SAVE_PARMS);

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpfilename;
	RECT16	rc;
} MCI_OVLY_LOAD_PARMS16, *LPMCI_OVLY_LOAD_PARMS16;

typedef struct {
	DWORD	dwCallback;
	LPCSTR	lpfilename;
	RECT32	rc;
} MCI_OVLY_LOAD_PARMS32A, *LPMCI_OVLY_LOAD_PARMS32A;

typedef struct {
	DWORD	dwCallback;
	LPCWSTR	lpfilename;
	RECT32	rc;
} MCI_OVLY_LOAD_PARMS32W, *LPMCI_OVLY_LOAD_PARMS32W;

DECL_WINELIB_TYPE_AW(MCI_OVLY_LOAD_PARMS);
DECL_WINELIB_TYPE_AW(LPMCI_OVLY_LOAD_PARMS);

/**************************************************************
 * 		Linux MMSYSTEM Internals & Sample Audio Drivers
 */

#define DRVM_INIT             100
#define WODM_INIT             DRVM_INIT
#define WIDM_INIT             DRVM_INIT
#define MODM_INIT             DRVM_INIT
#define MIDM_INIT             DRVM_INIT
#define AUXM_INIT             DRVM_INIT

#define WODM_GETNUMDEVS       3
#define WODM_GETDEVCAPS       4
#define WODM_OPEN             5
#define WODM_CLOSE            6
#define WODM_PREPARE          7
#define WODM_UNPREPARE        8
#define WODM_WRITE            9
#define WODM_PAUSE            10
#define WODM_RESTART          11
#define WODM_RESET            12 
#define WODM_GETPOS           13
#define WODM_GETPITCH         14
#define WODM_SETPITCH         15
#define WODM_GETVOLUME        16
#define WODM_SETVOLUME        17
#define WODM_GETPLAYBACKRATE  18
#define WODM_SETPLAYBACKRATE  19
#define WODM_BREAKLOOP        20
#define WODM_STOP             21

#define WIDM_GETNUMDEVS  50
#define WIDM_GETDEVCAPS  51
#define WIDM_OPEN        52
#define WIDM_CLOSE       53
#define WIDM_PREPARE     54
#define WIDM_UNPREPARE   55
#define WIDM_ADDBUFFER   56
#define WIDM_START       57
#define WIDM_STOP        58
#define WIDM_RESET       59
#define WIDM_GETPOS      60
#define WIDM_PAUSE       61

#define MODM_GETNUMDEVS		1
#define MODM_GETDEVCAPS		2
#define MODM_OPEN			3
#define MODM_CLOSE			4
#define MODM_PREPARE		5
#define MODM_UNPREPARE		6
#define MODM_DATA			7
#define MODM_LONGDATA		8
#define MODM_RESET          9
#define MODM_GETVOLUME		10
#define MODM_SETVOLUME		11
#define MODM_CACHEPATCHES		12      
#define MODM_CACHEDRUMPATCHES	13     

#define MIDM_GETNUMDEVS  53
#define MIDM_GETDEVCAPS  54
#define MIDM_OPEN        55
#define MIDM_CLOSE       56
#define MIDM_PREPARE     57
#define MIDM_UNPREPARE   58
#define MIDM_ADDBUFFER   59
#define MIDM_START       60
#define MIDM_STOP        61
#define MIDM_RESET       62

#define AUXDM_GETNUMDEVS    3
#define AUXDM_GETDEVCAPS    4
#define AUXDM_GETVOLUME     5
#define AUXDM_SETVOLUME     6

#define	MXDM_GETNUMDEVS		1
#define	MXDM_GETDEVCAPS		2
#define	MXDM_OPEN		3
#define	MXDM_CLOSE		4
#define	MXDM_GETLINEINFO	5
#define	MXDM_GETLINECONTROLS	6
#define	MXDM_GETCONTROLDETAILS	7
#define	MXDM_SETCONTROLDETAILS	8

#define MCI_MAX_DEVICE_TYPE_LENGTH 80

#define MCI_FALSE                       (MCI_STRING_OFFSET + 19)
#define MCI_TRUE                        (MCI_STRING_OFFSET + 20)

#define MCI_FORMAT_RETURN_BASE          MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S       (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S                (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S                (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S             (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S           (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S           (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S           (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S       (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S              (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S            (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S               (MCI_STRING_OFFSET + 31)

#define MCI_VD_FORMAT_TRACK_S           (MCI_VD_OFFSET + 5)

#define WAVE_FORMAT_PCM_S               (MCI_WAVE_OFFSET + 0)
#define WAVE_MAPPER_S                   (MCI_WAVE_OFFSET + 1)

#define MCI_SEQ_MAPPER_S                (MCI_SEQ_OFFSET + 5)
#define MCI_SEQ_FILE_S                  (MCI_SEQ_OFFSET + 6)
#define MCI_SEQ_MIDI_S                  (MCI_SEQ_OFFSET + 7)
#define MCI_SEQ_SMPTE_S                 (MCI_SEQ_OFFSET + 8)
#define MCI_SEQ_FORMAT_SONGPTR_S        (MCI_SEQ_OFFSET + 9)
#define MCI_SEQ_NONE_S                  (MCI_SEQ_OFFSET + 10)
#define MIDIMAPPER_S                    (MCI_SEQ_OFFSET + 11)

#define MCI_RESOURCE_RETURNED       0x00010000  /* resource ID */
#define MCI_COLONIZED3_RETURN       0x00020000  /* colonized ID, 3 bytes data */
#define MCI_COLONIZED4_RETURN       0x00040000  /* colonized ID, 4 bytes data */
#define MCI_INTEGER_RETURNED        0x00080000  /* integer conversion needed */
#define MCI_RESOURCE_DRIVER         0x00100000  /* driver owns returned resource */

#define MCI_NO_COMMAND_TABLE    0xFFFF

#define MCI_COMMAND_HEAD        0
#define MCI_STRING              1
#define MCI_INTEGER             2
#define MCI_END_COMMAND         3
#define MCI_RETURN              4
#define MCI_FLAG                5
#define MCI_END_COMMAND_LIST    6
#define MCI_RECT                7
#define MCI_CONSTANT            8
#define MCI_END_CONSTANT        9

/* Mixer flags */
#define MIXER_OBJECTF_HANDLE	0x80000000L
#define MIXER_OBJECTF_MIXER	0x00000000L
#define MIXER_OBJECTF_HMIXER	(MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT	0x10000000L
#define MIXER_OBJECTF_HWAVEOUT	(MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN	0x20000000L
#define MIXER_OBJECTF_HWAVEIN	(MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT	0x30000000L
#define MIXER_OBJECTF_HMIDIOUT	(MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN	0x40000000L
#define MIXER_OBJECTF_HMIDIIN	(MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX	0x50000000L

#define MAKEMCIRESOURCE(wRet, wRes) MAKELRESULT((wRet), (wRes))

/* the 95 DDK defines those slightly different, but they are internal anyway */
typedef struct {
	DWORD   	dwCallback;
	DWORD   	dwInstance;
	HMIDIOUT16	hMidi;
	DWORD   	dwFlags;
} PORTALLOC, *LPPORTALLOC;

typedef struct {
	HWAVE16		hWave;
	LPWAVEFORMAT	lpFormat;
	DWORD		dwCallBack;
	DWORD		dwInstance;
	UINT16		uDeviceID;
} WAVEOPENDESC, *LPWAVEOPENDESC;

typedef struct {
	HMIDI16		hMidi;
	DWORD		dwCallback;
	DWORD		dwInstance;
} MIDIOPENDESC, *LPMIDIOPENDESC;

typedef struct {
	UINT16			wDelay;
	UINT16			wResolution;
	LPTIMECALLBACK16	lpFunction;
	DWORD			dwUser;
	UINT16			wFlags;
} TIMEREVENT, *LPTIMEREVENT;

typedef struct tMIXEROPENDESC
{
	HMIXEROBJ16	hmx;
	DWORD		dwCallback;
	DWORD		dwInstance;
	UINT16		uDeviceID;
} MIXEROPENDESC,*LPMIXEROPENDESC;

typedef struct {
	UINT16	wDeviceID;	/* device ID */
	LPSTR	lpstrParams;	/* parameter string for entry in SYSTEM.INI */
	UINT16	wCustomCommandTable;	/* custom command table (0xFFFF if none)
					 * filled in by the driver */
	UINT16	wType;		/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMS, *LPMCI_OPEN_DRIVER_PARMS;

DWORD  WINAPI mciGetDriverData(UINT16 uDeviceID);
BOOL16 WINAPI mciSetDriverData(UINT16 uDeviceID, DWORD dwData);
UINT16 WINAPI mciDriverYield(UINT16 uDeviceID);
BOOL16 WINAPI mciDriverNotify(HWND16 hwndCallback, UINT16 uDeviceID,
                              UINT16 uStatus);
UINT16 WINAPI mciLoadCommandResource(HINSTANCE16 hInstance,
                                     LPCSTR lpResName, UINT16 uType);
BOOL16 WINAPI mciFreeCommandResource(UINT16 uTable);

#define DCB_NULL		0x0000
#define DCB_WINDOW		0x0001			/* dwCallback is a HWND */
#define DCB_TASK		0x0002			/* dwCallback is a HTASK */
#define DCB_FUNCTION	0x0003			/* dwCallback is a FARPROC */
#define DCB_TYPEMASK	0x0007
#define DCB_NOSWITCH	0x0008			/* don't switch stacks for callback */

BOOL16 WINAPI DriverCallback(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev, 
                             WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI auxMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);

struct LINUX_MCIDRIVER {
	HDRVR16			hdrv;
	DRIVERPROC16		driverproc;
	MCI_OPEN_DRIVER_PARMS	modp;
	MCI_OPEN_PARMS16	mop;
	DWORD			private;
};

#pragma pack(4)
DWORD WINAPI mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD WINAPI wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
#pragma pack(4)

#endif /* __WINE_MMSYSTEM_H */
