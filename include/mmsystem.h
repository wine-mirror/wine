/* 
 * MMSYSTEM - Multimedia Wine Extension ... :-)
 */

#ifndef MMSYSTEM_H
#define MMSYSTEM_H

typedef LPSTR		    HPSTR;          /* a huge version of LPSTR */
typedef LPCSTR			HPCSTR;         /* a huge version of LPCSTR */

#define MAXWAVEDRIVERS 10
#define MAXMIDIDRIVERS 10
#define MAXAUXDRIVERS 10
#define MAXMCIDRIVERS 32

#define MAXPNAMELEN      32     /* max product name length (including NULL) */
#define MAXERRORLENGTH   128    /* max error text length (including NULL) */

typedef WORD    VERSION;        /* major (high byte), minor (low byte) */

typedef struct {
    UINT    wType;              /* indicates the contents of the union */
    union {
        DWORD ms;               /* milliseconds */
        DWORD sample;           /* samples */
        DWORD cb;               /* byte count */
        struct {                /* SMPTE */
            BYTE hour;          /* hours */
            BYTE min;           /* minutes */
            BYTE sec;           /* seconds */
            BYTE frame;         /* frames  */
            BYTE fps;           /* frames per second */
            BYTE dummy;         /* pad */
            } smpte;
        struct {                /* MIDI */
            DWORD songptrpos;   /* song pointer position */
            } midi;
        } u;
} MMTIME,  *LPMMTIME;

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

typedef void (*LPDRVCALLBACK) (HDRVR16 h, UINT uMessage, DWORD dwUser, DWORD dw1, DWORD dw2);

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


WORD mmsystemGetVersion(void);
void OutputDebugStr(LPCSTR);

BOOL sndPlaySound(LPCSTR lpszSoundName, UINT uFlags);

#define SND_SYNC            0x0000  /* play synchronously (default) */
#define SND_ASYNC           0x0001  /* play asynchronously */
#define SND_NODEFAULT       0x0002  /* don't use default sound */
#define SND_MEMORY          0x0004  /* lpszSoundName points to a memory file */
#define SND_LOOP            0x0008  /* loop the sound until next sndPlaySound */
#define SND_NOSTOP          0x0010  /* don't stop any currently playing sound */

/* waveform audio error return values */
#define WAVERR_BADFORMAT      (WAVERR_BASE + 0)    /* unsupported wave format */
#define WAVERR_STILLPLAYING   (WAVERR_BASE + 1)    /* still something playing */
#define WAVERR_UNPREPARED     (WAVERR_BASE + 2)    /* header not prepared */
#define WAVERR_SYNC           (WAVERR_BASE + 3)    /* device is synchronous */
#define WAVERR_LASTERROR      (WAVERR_BASE + 3)    /* last error in range */

typedef HWAVEIN16 *LPHWAVEIN16;
typedef HWAVEOUT16 *LPHWAVEOUT16;
typedef LPDRVCALLBACK LPWAVECALLBACK;

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
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag *lpNext;         /* reserved for driver */
    DWORD       reserved;               /* reserved for driver */
} WAVEHDR, *LPWAVEHDR;

#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
#define WHDR_ENDLOOP    0x00000008  /* loop end block */
#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

typedef struct {
    UINT    wMid;                  /* manufacturer ID */
    UINT    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    UINT    wChannels;             /* number of sources supported */
    DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPS, *LPWAVEOUTCAPS;

#define WAVECAPS_PITCH          0x0001   /* supports pitch control */
#define WAVECAPS_PLAYBACKRATE   0x0002   /* supports playback rate control */
#define WAVECAPS_VOLUME         0x0004   /* supports volume control */
#define WAVECAPS_LRVOLUME       0x0008   /* separate left-right volume control */
#define WAVECAPS_SYNC           0x0010

typedef struct {
    UINT    wMid;                    /* manufacturer ID */
    UINT    wPid;                    /* product ID */
    VERSION vDriverVersion;          /* version of the driver */
    char    szPname[MAXPNAMELEN];    /* product name (NULL terminated string) */
    DWORD   dwFormats;               /* formats supported */
    UINT    wChannels;               /* number of channels supported */
} WAVEINCAPS, *LPWAVEINCAPS;

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

/* general format structure common to all formats */
typedef struct {
    WORD    wFormatTag;						/* format type */
    WORD    nChannels; 						/* number of channels */
    DWORD   nSamplesPerSec WINE_PACKED;		/* sample rate */
    DWORD   nAvgBytesPerSec WINE_PACKED;	/* for buffer estimation */
    WORD    nBlockAlign; 					/* block size of data */
} WAVEFORMAT, *LPWAVEFORMAT;

#define WAVE_FORMAT_PCM     1

typedef struct {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PCMWAVEFORMAT, *LPPCMWAVEFORMAT;

UINT waveOutGetNumDevs(void);
UINT waveOutGetDevCaps(UINT uDeviceID, WAVEOUTCAPS * lpCaps,
    UINT uSize);
UINT waveOutGetVolume(UINT uDeviceID, DWORD * lpdwVolume);
UINT waveOutSetVolume(UINT uDeviceID, DWORD dwVolume);
UINT waveOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT waveGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT waveOutOpen(HWAVEOUT16 * lphWaveOut, UINT uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags);
UINT waveOutClose(HWAVEOUT16 hWaveOut);
UINT waveOutPrepareHeader(HWAVEOUT16 hWaveOut,
     WAVEHDR * lpWaveOutHdr, UINT uSize);
UINT waveOutUnprepareHeader(HWAVEOUT16 hWaveOut,
    WAVEHDR * lpWaveOutHdr, UINT uSize);
UINT waveOutWrite(HWAVEOUT16 hWaveOut, WAVEHDR * lpWaveOutHdr,
    UINT uSize);
UINT waveOutPause(HWAVEOUT16 hWaveOut);
UINT waveOutRestart(HWAVEOUT16 hWaveOut);
UINT waveOutReset(HWAVEOUT16 hWaveOut);
UINT waveOutBreakLoop(HWAVEOUT16 hWaveOut);
UINT waveOutGetPosition(HWAVEOUT16 hWaveOut, MMTIME * lpInfo,
    UINT uSize);
UINT waveOutGetPitch(HWAVEOUT16 hWaveOut, DWORD * lpdwPitch);
UINT waveOutSetPitch(HWAVEOUT16 hWaveOut, DWORD dwPitch);
UINT waveOutGetPlaybackRate(HWAVEOUT16 hWaveOut, DWORD * lpdwRate);
UINT waveOutSetPlaybackRate(HWAVEOUT16 hWaveOut, DWORD dwRate);
UINT waveOutGetID(HWAVEOUT16 hWaveOut, UINT * lpuDeviceID);

DWORD waveOutMessage(HWAVEOUT16 hWaveOut, UINT uMessage, DWORD dw1, DWORD dw2);

UINT waveInGetNumDevs(void);
UINT waveInGetDevCaps(UINT uDeviceID, WAVEINCAPS * lpCaps,
    UINT uSize);
UINT waveInGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT waveInOpen(HWAVEIN16 * lphWaveIn, UINT uDeviceID,
    const LPWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags);
UINT waveInClose(HWAVEIN16 hWaveIn);
UINT waveInPrepareHeader(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT uSize);
UINT waveInUnprepareHeader(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT uSize);
UINT waveInAddBuffer(HWAVEIN16 hWaveIn,
    WAVEHDR * lpWaveInHdr, UINT uSize);
UINT waveInStart(HWAVEIN16 hWaveIn);
UINT waveInStop(HWAVEIN16 hWaveIn);
UINT waveInReset(HWAVEIN16 hWaveIn);
UINT waveInGetPosition(HWAVEIN16 hWaveIn, MMTIME * lpInfo,
    UINT uSize);
UINT waveInGetID(HWAVEIN16 hWaveIn, UINT * lpuDeviceID);

DWORD waveInMessage(HWAVEIN16 hWaveIn, UINT uMessage, DWORD dw1, DWORD dw2);

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
    UINT    wMid;                  /* manufacturer ID */
    UINT    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    UINT    wTechnology;           /* type of device */
    UINT    wVoices;               /* # of voices (internal synth only) */
    UINT    wNotes;                /* max # of notes (internal synth only) */
    UINT    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPS, *LPMIDIOUTCAPS;

#define MOD_MIDIPORT    1  /* output port */
#define MOD_SYNTH       2  /* generic internal synth */
#define MOD_SQSYNTH     3  /* square wave internal synth */
#define MOD_FMSYNTH     4  /* FM internal synth */
#define MOD_MAPPER      5  /* MIDI mapper */

#define MIDICAPS_VOLUME          0x0001  /* supports volume control */
#define MIDICAPS_LRVOLUME        0x0002  /* separate left-right volume control */
#define MIDICAPS_CACHE           0x0004

typedef struct {
    UINT    wMid;                  /* manufacturer ID */
    UINT    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
} MIDIINCAPS, *LPMIDIINCAPS;

typedef struct {
    LPSTR       lpData;               /* pointer to locked data block */
    DWORD       dwBufferLength;       /* length of data in data block */
    DWORD       dwBytesRecorded;      /* used for input only */
    DWORD       dwUser;               /* for client's use */
    DWORD       dwFlags;              /* assorted flags (see defines) */
    struct midihdr_tag *lpNext;       /* reserved for driver */
    DWORD       reserved;             /* reserved for driver */
} MIDIHDR, *LPMIDIHDR;

#define MHDR_DONE       0x00000001       /* done bit */
#define MHDR_PREPARED   0x00000002       /* set if header prepared */
#define MHDR_INQUEUE    0x00000004       /* reserved for driver */

UINT midiOutGetNumDevs(void);
UINT midiOutGetDevCaps(UINT uDeviceID,
    MIDIOUTCAPS * lpCaps, UINT uSize);
UINT midiOutGetVolume(UINT uDeviceID, DWORD * lpdwVolume);
UINT midiOutSetVolume(UINT uDeviceID, DWORD dwVolume);
UINT midiOutGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT midiGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT midiOutOpen(HMIDIOUT16 * lphMidiOut, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags);
UINT midiOutClose(HMIDIOUT16 hMidiOut);
UINT midiOutPrepareHeader(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT uSize);
UINT midiOutUnprepareHeader(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT uSize);
UINT midiOutShortMsg(HMIDIOUT16 hMidiOut, DWORD dwMsg);
UINT midiOutLongMsg(HMIDIOUT16 hMidiOut,
    MIDIHDR * lpMidiOutHdr, UINT uSize);
UINT midiOutReset(HMIDIOUT16 hMidiOut);
UINT midiOutCachePatches(HMIDIOUT16 hMidiOut,
    UINT uBank, WORD * lpwPatchArray, UINT uFlags);
UINT midiOutCacheDrumPatches(HMIDIOUT16 hMidiOut,
    UINT uPatch, WORD * lpwKeyArray, UINT uFlags);
UINT midiOutGetID(HMIDIOUT16 hMidiOut, UINT * lpuDeviceID);

DWORD midiOutMessage(HMIDIOUT16 hMidiOut, UINT uMessage, DWORD dw1, DWORD dw2);

UINT midiInGetNumDevs(void);
UINT midiInGetDevCaps(UINT uDeviceID,
    LPMIDIINCAPS lpCaps, UINT uSize);
UINT midiInGetErrorText(UINT uError, LPSTR lpText, UINT uSize);
UINT midiInOpen(HMIDIIN16 * lphMidiIn, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD dwFlags);
UINT midiInClose(HMIDIIN16 hMidiIn);
UINT midiInPrepareHeader(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT uSize);
UINT midiInUnprepareHeader(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT uSize);
UINT midiInAddBuffer(HMIDIIN16 hMidiIn,
    MIDIHDR * lpMidiInHdr, UINT uSize);
UINT midiInStart(HMIDIIN16 hMidiIn);
UINT midiInStop(HMIDIIN16 hMidiIn);
UINT midiInReset(HMIDIIN16 hMidiIn);
UINT midiInGetID(HMIDIIN16 hMidiIn, UINT * lpuDeviceID);

DWORD midiInMessage(HMIDIIN16 hMidiIn, UINT uMessage, DWORD dw1, DWORD dw2);

#define AUX_MAPPER     (-1)

typedef struct {
    UINT    wMid;                  /* manufacturer ID */
    UINT    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    UINT    wTechnology;           /* type of device */
    DWORD   dwSupport;             /* functionality supported by driver */
} AUXCAPS, *LPAUXCAPS;

#define AUXCAPS_CDAUDIO    1       /* audio from internal CD-ROM drive */
#define AUXCAPS_AUXIN      2       /* audio from auxiliary input jacks */

#define AUXCAPS_VOLUME          0x0001  /* supports volume control */
#define AUXCAPS_LRVOLUME        0x0002  /* separate left-right volume control */

UINT auxGetNumDevs(void);
UINT auxGetDevCaps(UINT uDeviceID, AUXCAPS * lpCaps, UINT uSize);
UINT auxSetVolume(UINT uDeviceID, DWORD dwVolume);
UINT auxGetVolume(UINT uDeviceID, DWORD * lpdwVolume);

DWORD auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2);

#define TIMERR_NOERROR        (0)                  /* no error */
#define TIMERR_NOCANDO        (TIMERR_BASE+1)      /* request not completed */
#define TIMERR_STRUCT         (TIMERR_BASE+33)     /* time struct size */

typedef void (*LPTIMECALLBACK) (UINT uTimerID, UINT uMessage, DWORD dwUser, DWORD dw1, DWORD dw2);

#define TIME_ONESHOT    0   /* program timer for single event */
#define TIME_PERIODIC   1   /* program for continuous periodic event */

typedef struct {
    UINT    wPeriodMin;     /* minimum period supported  */
    UINT    wPeriodMax;     /* maximum period supported  */
} TIMECAPS, *LPTIMECAPS;

UINT timeGetSystemTime(MMTIME * lpTime, UINT uSize);
DWORD timeGetTime(void);
UINT timeSetEvent(UINT uDelay, UINT uResolution,
    LPTIMECALLBACK lpFunction, DWORD dwUser, UINT uFlags);
UINT timeKillEvent(UINT uTimerID);
UINT timeGetDevCaps(TIMECAPS * lpTimeCaps, UINT uSize);
UINT timeBeginPeriod(UINT uPeriod);
UINT timeEndPeriod(UINT uPeriod);

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
    UINT wMid;                  /* manufacturer ID */
    UINT wPid;                  /* product ID */
    char szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    UINT wXmin;                 /* minimum x position value */
    UINT wXmax;                 /* maximum x position value */
    UINT wYmin;                 /* minimum y position value */
    UINT wYmax;                 /* maximum y position value */
    UINT wZmin;                 /* minimum z position value */
    UINT wZmax;                 /* maximum z position value */
    UINT wNumButtons;           /* number of buttons */
    UINT wPeriodMin;            /* minimum message period when captured */
    UINT wPeriodMax;            /* maximum message period when captured */
} JOYCAPS, *LPJOYCAPS;

typedef struct {
    UINT wXpos;                 /* x position */
    UINT wYpos;                 /* y position */
    UINT wZpos;                 /* z position */
    UINT wButtons;              /* button states */
} JOYINFO, *LPJOYINFO;

UINT joyGetDevCaps(UINT uJoyID, JOYCAPS * lpCaps, UINT uSize);
UINT joyGetNumDevs(void);
UINT joyGetPos(UINT uJoyID, JOYINFO * lpInfo);
UINT joyGetThreshold(UINT uJoyID, UINT * lpuThreshold);
UINT joyReleaseCapture(UINT uJoyID);
UINT joySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod,
    BOOL bChanged);
UINT joySetThreshold(UINT uJoyID, UINT uThreshold);

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
typedef LONG (*LPMMIOPROC)(LPSTR lpmmioinfo, UINT uMessage,
            LPARAM lParam1, LPARAM lParam2);

typedef struct {
        DWORD           dwFlags;        /* general status flags */
        FOURCC          fccIOProc;      /* pointer to I/O procedure */
        LPMMIOPROC      pIOProc;        /* pointer to I/O procedure */
        UINT            wErrorRet;      /* place for error to be returned */
        HTASK           htask;          /* alternate local task */
        /* fields maintained by MMIO functions during buffered I/O */
        LONG            cchBuffer;      /* size of I/O buffer (or 0L) */
        HPSTR           pchBuffer;      /* start of I/O buffer (or NULL) */
        HPSTR           pchNext;        /* pointer to next byte to read/write */
        HPSTR           pchEndRead;     /* pointer to last valid byte to read */
        HPSTR           pchEndWrite;    /* pointer to last byte to write */
        LONG            lBufOffset;     /* disk offset of start of buffer */
        /* fields maintained by I/O procedure */
        LONG            lDiskOffset;    /* disk offset of next read or write */
        DWORD           adwInfo[3];     /* data specific to type of MMIOPROC */
        /* other fields maintained by MMIO */
        DWORD           dwReserved1;    /* reserved for MMIO use */
        DWORD           dwReserved2;    /* reserved for MMIO use */
        HMMIO16         hmmio;          /* handle to open file */
} MMIOINFO, *LPMMIOINFO;

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

FOURCC mmioStringToFOURCC(LPCSTR sz, UINT uFlags);
LPMMIOPROC mmioInstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc,
    DWORD dwFlags);
HMMIO16 mmioOpen(LPSTR szFileName, MMIOINFO * lpmmioinfo,
    DWORD dwOpenFlags);

UINT mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
     MMIOINFO * lpmmioinfo, DWORD dwRenameFlags);

UINT mmioClose(HMMIO16 hmmio, UINT uFlags);
LONG mmioRead(HMMIO16 hmmio, HPSTR pch, LONG cch);
LONG mmioWrite(HMMIO16 hmmio, HPCSTR pch, LONG cch);
LONG mmioSeek(HMMIO16 hmmio, LONG lOffset, int iOrigin);
UINT mmioGetInfo(HMMIO16 hmmio, MMIOINFO * lpmmioinfo, UINT uFlags);
UINT mmioSetInfo(HMMIO16 hmmio, const MMIOINFO * lpmmioinfo, UINT uFlags);
UINT mmioSetBuffer(HMMIO16 hmmio, LPSTR pchBuffer, LONG cchBuffer,
    UINT uFlags);
UINT mmioFlush(HMMIO16 hmmio, UINT uFlags);
UINT mmioAdvance(HMMIO16 hmmio, MMIOINFO * lpmmioinfo, UINT uFlags);
LONG mmioSendMessage(HMMIO16 hmmio, UINT uMessage,
    LPARAM lParam1, LPARAM lParam2);
UINT mmioDescend(HMMIO16 hmmio, MMCKINFO * lpck,
    const MMCKINFO * lpckParent, UINT uFlags);
UINT mmioAscend(HMMIO16 hmmio, MMCKINFO * lpck, UINT uFlags);
UINT mmioCreateChunk(HMMIO16 hmmio, MMCKINFO * lpck, UINT uFlags);

typedef UINT (*YIELDPROC) (UINT uDeviceID, DWORD dwYieldData);

DWORD mciSendCommand (UINT uDeviceID, UINT uMessage,
    DWORD dwParam1, DWORD dwParam2);
DWORD mciSendString (LPCSTR lpstrCommand,
    LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
UINT mciGetDeviceID (LPCSTR lpstrName);
UINT mciGetDeviceIDFromElementID (DWORD dwElementID,
    LPCSTR lpstrType);
BOOL mciGetErrorString (DWORD wError, LPSTR lpstrBuffer,
    UINT uLength);
BOOL mciSetYieldProc (UINT uDeviceID, YIELDPROC fpYieldProc,
    DWORD dwYieldData);

HTASK mciGetCreatorTask(UINT uDeviceID);
YIELDPROC mciGetYieldProc (UINT uDeviceID, DWORD * lpdwYieldData);

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
	DWORD   dwCallback;
	UINT    wDeviceID;
	UINT    wReserved0;
	LPCSTR  lpstrDeviceType;
	LPCSTR  lpstrElementName;
	LPCSTR  lpstrAlias;
} MCI_OPEN_PARMS, *LPMCI_OPEN_PARMS;

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
} MCI_INFO_PARMS, *LPMCI_INFO_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwReturn;
	DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS, *LPMCI_GETDEVCAPS_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPSTR   lpstrReturn;
	DWORD   dwRetSize;
	DWORD   dwNumber;
	UINT    wDeviceType;
	UINT    wReserved0;
} MCI_SYSINFO_PARMS, *LPMCI_SYSINFO_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwTimeFormat;
	DWORD   dwAudio;
} MCI_SET_PARMS, *LPMCI_SET_PARMS;

typedef struct {
	DWORD   dwCallback;
	int     nVirtKey;
	UINT    wReserved0;
	HWND    hwndBreak;
	UINT    wReserved1;
} MCI_BREAK_PARMS, *LPMCI_BREAK_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpstrSoundName;
} MCI_SOUND_PARMS, *LPMCI_SOUND_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
} MCI_SAVE_PARMS, *LPMCI_SAVE_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
} MCI_LOAD_PARMS, *LPMCI_LOAD_PARMS;

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
	DWORD   dwCallback;
	LPCSTR  lpstrCommand;
} MCI_VD_ESCAPE_PARMS, *LPMCI_VD_ESCAPE_PARMS;

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
	DWORD   dwCallback;
	UINT    wDeviceID;
	UINT    wReserved0;
	LPCSTR  lpstrDeviceType;
	LPCSTR  lpstrElementName;
	LPCSTR  lpstrAlias;
	DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS, *LPMCI_WAVE_OPEN_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwFrom;
	DWORD   dwTo;
} MCI_WAVE_DELETE_PARMS, *LPMCI_WAVE_DELETE_PARMS;

typedef struct {
	DWORD   dwCallback;
	DWORD   dwTimeFormat;
	DWORD   dwAudio;
	UINT    wInput;
	UINT    wReserved0;
	UINT    wOutput;
	UINT    wReserved1;
	UINT    wFormatTag;
	UINT    wReserved2;
	UINT    nChannels;
	UINT    wReserved3;
	DWORD   nSamplesPerSec;
	DWORD   nAvgBytesPerSec;
	UINT    nBlockAlign;
	UINT    wReserved4;
	UINT    wBitsPerSample;
	UINT    wReserved5;
} MCI_WAVE_SET_PARMS, * LPMCI_WAVE_SET_PARMS;

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
	UINT    wDeviceID;
	UINT    wReserved0;
	LPCSTR  lpstrDeviceType;
	LPCSTR  lpstrElementName;
	LPCSTR  lpstrAlias;
	DWORD   dwStyle;
	HWND    hWndParent;
	UINT    wReserved1;
} MCI_ANIM_OPEN_PARMS, *LPMCI_ANIM_OPEN_PARMS;

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
	DWORD   dwCallback;
	HWND    hWnd;
	UINT    wReserved1;
	UINT    nCmdShow;
	UINT    wReserved2;
	LPCSTR  lpstrText;
} MCI_ANIM_WINDOW_PARMS, *LPMCI_ANIM_WINDOW_PARMS;

typedef struct {
	DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT16 ptOffset;
	POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_ANIM_RECT_PARMS, *LPMCI_ANIM_RECT_PARMS;

typedef struct {
	DWORD   dwCallback;
	RECT16  rc;
	HDC     hDC;
} MCI_ANIM_UPDATE_PARMS, *LPMCI_ANIM_UPDATE_PARMS;

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
	DWORD   dwCallback;
	UINT    wDeviceID;
	UINT    wReserved0;
	LPCSTR  lpstrDeviceType;
	LPCSTR  lpstrElementName;
	LPCSTR  lpstrAlias;
	DWORD   dwStyle;
	HWND    hWndParent;
	UINT    wReserved1;
} MCI_OVLY_OPEN_PARMS, *LPMCI_OVLY_OPEN_PARMS;

typedef struct {
	DWORD   dwCallback;
	HWND    hWnd;
	UINT    wReserved1;
	UINT    nCmdShow;
	UINT    wReserved2;
	LPCSTR  lpstrText;
} MCI_OVLY_WINDOW_PARMS, *LPMCI_OVLY_WINDOW_PARMS;

typedef struct {
	DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
	POINT16 ptOffset;
	POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
	RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_OVLY_RECT_PARMS, *LPMCI_OVLY_RECT_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
	RECT16  rc;
} MCI_OVLY_SAVE_PARMS, *LPMCI_OVLY_SAVE_PARMS;

typedef struct {
	DWORD   dwCallback;
	LPCSTR  lpfilename;
	RECT16  rc;
} MCI_OVLY_LOAD_PARMS, *LPMCI_OVLY_LOAD_PARMS;


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

typedef struct {
	DWORD   	dwCallback;
	DWORD   	dwInstance;
	HMIDIOUT16	hMidi;
	DWORD   	dwFlags;
} PORTALLOC, *LPPORTALLOC;

typedef struct {
	HWAVE16			hWave;
	LPWAVEFORMAT	lpFormat;
	DWORD			dwCallBack;
	DWORD			dwInstance;
} WAVEOPENDESC, *LPWAVEOPENDESC;

typedef struct {
	HMIDI16			hMidi;
	DWORD			dwCallback;
	DWORD			dwInstance;
} MIDIOPENDESC, *LPMIDIOPENDESC;

typedef struct {
	UINT			wDelay;
	UINT			wResolution;
	LPTIMECALLBACK	lpFunction;
	DWORD			dwUser;
	UINT			wFlags;
} TIMEREVENT, *LPTIMEREVENT;

typedef struct {
	UINT    wDeviceID;				/* device ID */
	LPSTR 	lpstrParams;			/* parameter string for entry in SYSTEM.INI */
	UINT    wCustomCommandTable;	/* custom command table (0xFFFF if none) */
									/* filled in by the driver */
	UINT    wType;					/* driver type */
									/* filled in by the driver */
} MCI_OPEN_DRIVER_PARMS, * LPMCI_OPEN_DRIVER_PARMS;

DWORD mciGetDriverData(UINT uDeviceID);
BOOL  mciSetDriverData(UINT uDeviceID, DWORD dwData);
UINT  mciDriverYield(UINT uDeviceID);
BOOL  mciDriverNotify(HWND hwndCallback, UINT uDeviceID,
    UINT uStatus);
UINT  mciLoadCommandResource(HINSTANCE hInstance,
    LPCSTR lpResName, UINT uType);
BOOL  mciFreeCommandResource(UINT uTable);

#define DCB_NULL		0x0000
#define DCB_WINDOW		0x0001			/* dwCallback is a HWND */
#define DCB_TASK		0x0002			/* dwCallback is a HTASK */
#define DCB_FUNCTION	0x0003			/* dwCallback is a FARPROC */
#define DCB_TYPEMASK	0x0007
#define DCB_NOSWITCH	0x0008			/* don't switch stacks for callback */

BOOL DriverCallback(DWORD dwCallBack, UINT uFlags, HANDLE hDev, 
		WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
DWORD auxMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);
DWORD wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2);

#endif /* MMSYSTEM_H */



