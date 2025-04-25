/*
 * Copyright (C) 2020 Vijay Kiran Kamuju
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

#ifndef __WINE_XACT3_H
#define __WINE_XACT3_H

#include <windows.h>
#include <objbase.h>
#include <float.h>
#include <limits.h>
#include <xact3wb.h>
#include <xaudio2.h>

#ifdef __cplusplus
extern "C" {
#endif

#if XACT3_VER == 0x0300
    DEFINE_GUID(CLSID_XACTEngine, 0x3b80ee2a, 0xb0f5, 0x4780, 0x9e, 0x30, 0x90, 0xcb, 0x39, 0x68, 0x5b, 0x03);
#elif XACT3_VER == 0x0301
    DEFINE_GUID(CLSID_XACTEngine, 0x962f5027, 0x99be, 0x4692, 0xa4, 0x68, 0x85, 0x80, 0x2c, 0xf8, 0xde, 0x61);
#elif XACT3_VER == 0x0302
    DEFINE_GUID(CLSID_XACTEngine, 0xd3332f02, 0x3dd0, 0x4de9, 0x9a, 0xec, 0x20, 0xd8, 0x5c, 0x41, 0x11, 0xb6);
#elif XACT3_VER == 0x0303
    DEFINE_GUID(CLSID_XACTEngine, 0x94c1affa, 0x66e7, 0x4961, 0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f);
#elif XACT3_VER == 0x0304
    DEFINE_GUID(CLSID_XACTEngine, 0x0977d092, 0x2d95, 0x4e43, 0x8d, 0x42, 0x9d, 0xdc, 0xc2, 0x54, 0x5e, 0xd5);
#elif XACT3_VER == 0x0305
    DEFINE_GUID(CLSID_XACTEngine, 0x074b110f, 0x7f58, 0x4743, 0xae, 0xa5, 0x12, 0xf1, 0x5b, 0x50, 0x74, 0xed);
#elif XACT3_VER == 0x0306
    DEFINE_GUID(CLSID_XACTEngine, 0x248d8a3b, 0x6256, 0x44d3, 0xa0, 0x18, 0x2a, 0xc9, 0x6c, 0x45, 0x9f, 0x47);
#else /* XACT3_VER == 0x0307 or not defined */
    DEFINE_GUID(CLSID_XACTEngine, 0xbcc782bc, 0x6492, 0x4c22, 0x8c, 0x35, 0xf5, 0xd7, 0x2f, 0xe7, 0x3c, 0x6e);
#endif

DEFINE_GUID(CLSID_XACTAuditionEngine, 0x9ecdd80d, 0x0e81, 0x40d8, 0x89, 0x03, 0x2b, 0xf7, 0xb1, 0x31, 0xac, 0x43);
DEFINE_GUID(CLSID_XACTDebugEngine,    0x02860630, 0xbf3b, 0x42a8, 0xb1, 0x4e, 0x91, 0xed, 0xa2, 0xf5, 0x1e, 0xa5);

#if XACT3_VER == 0x0300
    DEFINE_GUID(IID_IXACT3Engine, 0x9e33f661, 0x2d07, 0x43ec, 0x97, 0x04, 0xbb, 0xcb, 0x71, 0xa5, 0x49, 0x72);
#elif XACT3_VER >= 0x0301 && XACT3_VER <= 0x304
    DEFINE_GUID(IID_IXACT3Engine, 0xe72c1b9a, 0xd717, 0x41c0, 0x81, 0xa6, 0x50, 0xeb, 0x56, 0xe8, 0x06, 0x49);
#else
    DEFINE_GUID(IID_IXACT3Engine, 0xb1ee676a, 0xd9cd, 0x4d2a, 0x89, 0xa8, 0xfa, 0x53, 0xeb, 0x9e, 0x48, 0x0b);
#endif

typedef struct IXACT3SoundBank IXACT3SoundBank;
typedef struct IXACT3WaveBank  IXACT3WaveBank;
typedef struct IXACT3Cue       IXACT3Cue;
typedef struct IXACT3Wave      IXACT3Wave;
typedef struct IXACT3Engine    IXACT3Engine;

typedef WORD  XACTCATEGORY;
typedef BYTE  XACTCHANNEL;
typedef WORD  XACTINDEX;
typedef BYTE  XACTINSTANCELIMIT;
typedef BYTE  XACTLOOPCOUNT;
typedef BYTE  XACTNOTIFICATIONTYPE;
typedef SHORT XACTPITCH;
typedef LONG  XACTTIME;
typedef WORD  XACTVARIABLEINDEX;
typedef FLOAT XACTVARIABLEVALUE;
typedef BYTE  XACTVARIATIONWEIGHT;
typedef FLOAT XACTVOLUME;

static const XACTTIME             XACTTIME_MIN               = INT_MIN;
static const XACTTIME             XACTTIME_MAX               = INT_MAX;
static const XACTTIME             XACTTIME_INFINITE          = INT_MAX;
static const XACTINSTANCELIMIT    XACTINSTANCELIMIT_INFINITE = 0xff;
static const XACTINSTANCELIMIT    XACTINSTANCELIMIT_MIN      = 0x00;
static const XACTINSTANCELIMIT    XACTINSTANCELIMIT_MAX      = 0xfe;
static const XACTINDEX            XACTINDEX_MIN              = 0x0;
static const XACTINDEX            XACTINDEX_MAX              = 0xfffe;
static const XACTINDEX            XACTINDEX_INVALID          = 0xffff;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MIN   = 0x00;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MAX   = 0xff;
static const XACTVARIABLEVALUE    XACTVARIABLEVALUE_MIN      = -FLT_MAX;
static const XACTVARIABLEVALUE    XACTVARIABLEVALUE_MAX      = FLT_MAX;
static const XACTVARIABLEINDEX    XACTVARIABLEINDEX_MIN      = 0x0000;
static const XACTVARIABLEINDEX    XACTVARIABLEINDEX_MAX      = 0xfffe;
static const XACTVARIABLEINDEX    XACTVARIABLEINDEX_INVALID  = 0xffff;
static const XACTCATEGORY         XACTCATEGORY_MIN           = 0x0;
static const XACTCATEGORY         XACTCATEGORY_MAX           = 0xfffe;
static const XACTCATEGORY         XACTCATEGORY_INVALID       = 0xffff;
static const XACTCHANNEL          XACTCHANNEL_MIN            = 0;
static const XACTCHANNEL          XACTCHANNEL_MAX            = 0xFF;
static const XACTPITCH            XACTPITCH_MIN              = -1200;
static const XACTPITCH            XACTPITCH_MAX              = 1200;
static const XACTPITCH            XACTPITCH_MIN_TOTAL        = -2400;
static const XACTPITCH            XACTPITCH_MAX_TOTAL        = 2400;
static const XACTVOLUME           XACTVOLUME_MIN             = 0.0f;
static const XACTVOLUME           XACTVOLUME_MAX             = 16777216.0f;
static const XACTVARIABLEVALUE    XACTPARAMETERVALUE_MIN     = -FLT_MAX;
static const XACTVARIABLEVALUE    XACTPARAMETERVALUE_MAX     = FLT_MAX;
static const XACTLOOPCOUNT        XACTLOOPCOUNT_MIN          = 0x0;
static const XACTLOOPCOUNT        XACTLOOPCOUNT_MAX          = 0xfe;
static const XACTLOOPCOUNT        XACTLOOPCOUNT_INFINITE     = 0xff;
static const DWORD                XACTWAVEALIGNMENT_MIN      = 2048;

#define FACILITY_XACTENGINE 0xAC7
#define XACTENGINEERROR(n) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XACTENGINE, n)

#define XACTENGINE_E_OUTOFMEMORY               E_OUTOFMEMORY
#define XACTENGINE_E_INVALIDARG                E_INVALIDARG
#define XACTENGINE_E_NOTIMPL                   E_NOTIMPL
#define XACTENGINE_E_FAIL                      E_FAIL

#define XACTENGINE_E_ALREADYINITIALIZED        XACTENGINEERROR(0x001)
#define XACTENGINE_E_NOTINITIALIZED            XACTENGINEERROR(0x002)
#define XACTENGINE_E_EXPIRED                   XACTENGINEERROR(0x003)
#define XACTENGINE_E_NONOTIFICATIONCALLBACK    XACTENGINEERROR(0x004)
#define XACTENGINE_E_NOTIFICATIONREGISTERED    XACTENGINEERROR(0x005)
#define XACTENGINE_E_INVALIDUSAGE              XACTENGINEERROR(0x006)
#define XACTENGINE_E_INVALIDDATA               XACTENGINEERROR(0x007)
#define XACTENGINE_E_INSTANCELIMITFAILTOPLAY   XACTENGINEERROR(0x008)
#define XACTENGINE_E_NOGLOBALSETTINGS          XACTENGINEERROR(0x009)
#define XACTENGINE_E_INVALIDVARIABLEINDEX      XACTENGINEERROR(0x00a)
#define XACTENGINE_E_INVALIDCATEGORY           XACTENGINEERROR(0x00b)
#define XACTENGINE_E_INVALIDCUEINDEX           XACTENGINEERROR(0x00c)
#define XACTENGINE_E_INVALIDWAVEINDEX          XACTENGINEERROR(0x00d)
#define XACTENGINE_E_INVALIDTRACKINDEX         XACTENGINEERROR(0x00e)
#define XACTENGINE_E_INVALIDSOUNDOFFSETORINDEX XACTENGINEERROR(0x00f)
#define XACTENGINE_E_READFILE                  XACTENGINEERROR(0x010)
#define XACTENGINE_E_UNKNOWNEVENT              XACTENGINEERROR(0x011)
#define XACTENGINE_E_INCALLBACK                XACTENGINEERROR(0x012)
#define XACTENGINE_E_NOWAVEBANK                XACTENGINEERROR(0x013)
#define XACTENGINE_E_SELECTVARIATION           XACTENGINEERROR(0x014)
#define XACTENGINE_E_MULTIPLEAUDITIONENGINES   XACTENGINEERROR(0x015)
#define XACTENGINE_E_WAVEBANKNOTPREPARED       XACTENGINEERROR(0x016)
#define XACTENGINE_E_NORENDERER                XACTENGINEERROR(0x017)
#define XACTENGINE_E_INVALIDENTRYCOUNT         XACTENGINEERROR(0x018)
#define XACTENGINE_E_SEEKTIMEBEYONDCUEEND      XACTENGINEERROR(0x019)
#define XACTENGINE_E_SEEKTIMEBEYONDWAVEEND     XACTENGINEERROR(0x01a)
#define XACTENGINE_E_NOFRIENDLYNAMES           XACTENGINEERROR(0x01b)
#define XACTENGINE_E_AUDITION_WRITEFILE        XACTENGINEERROR(0x101)
#define XACTENGINE_E_AUDITION_NOSOUNDBANK      XACTENGINEERROR(0x102)
#define XACTENGINE_E_AUDITION_INVALIDRPCINDEX  XACTENGINEERROR(0x103)
#define XACTENGINE_E_AUDITION_MISSINGDATA      XACTENGINEERROR(0x104)
#define XACTENGINE_E_AUDITION_UNKNOWNCOMMAND   XACTENGINEERROR(0x105)
#define XACTENGINE_E_AUDITION_INVALIDDSPINDEX  XACTENGINEERROR(0x106)
#define XACTENGINE_E_AUDITION_MISSINGWAVE      XACTENGINEERROR(0x107)
#define XACTENGINE_E_AUDITION_CREATEDIRECTORYFAILED XACTENGINEERROR(0x108)
#define XACTENGINE_E_AUDITION_INVALIDSESSION   XACTENGINEERROR(0x109)

static const DWORD XACT_FLAG_STOP_RELEASE    = 0x00000000;
static const DWORD XACT_FLAG_STOP_IMMEDIATE  = 0x00000001;

static const DWORD XACT_FLAG_MANAGEDATA      = 0x00000001;
static const DWORD XACT_FLAG_BACKGROUND_MUSIC = 0x00000002;
static const DWORD XACT_FLAG_UNITS_MS        = 0x00000004;
static const DWORD XACT_FLAG_UNITS_SAMPLES   = 0x00000008;

static const DWORD XACT_STATE_CREATED        = 0x00000001;
static const DWORD XACT_STATE_PREPARING      = 0x00000002;
static const DWORD XACT_STATE_PREPARED       = 0x00000004;
static const DWORD XACT_STATE_PLAYING        = 0x00000008;
static const DWORD XACT_STATE_STOPPING       = 0x00000010;
static const DWORD XACT_STATE_STOPPED        = 0x00000020;
static const DWORD XACT_STATE_PAUSED         = 0x00000040;
static const DWORD XACT_STATE_INUSE          = 0x00000080;
static const DWORD XACT_STATE_PREPAREFAILED  = 0x80000000;

#define XACT_FLAG_GLOBAL_SETTINGS_MANAGEDATA XACT_FLAG_MANAGEDATA

#define XACT_RENDERER_ID_LENGTH    0xff
#define XACT_RENDERER_NAME_LENGTH  0xff
#define XACT_CUE_NAME_LENGTH       0xff

#define XACT_CONTENT_VERSION 46

#define XACT_ENGINE_LOOKAHEAD_DEFAULT 250

static const DWORD XACT_FLAG_API_AUDITION_MODE = 0x00000001;
static const DWORD XACT_FLAG_API_DEBUG_MODE    = 0x00000002;

#define XACT_DEBUGENGINE_REGISTRY_KEY   L"Software\\Microsoft\\XACT"
#define XACT_DEBUGENGINE_REGISTRY_VALUE L"DebugEngine"

typedef struct XACT_RENDERER_DETAILS
{
    WCHAR rendererID[XACT_RENDERER_ID_LENGTH];
    WCHAR displayName[XACT_RENDERER_NAME_LENGTH];
    BOOL defaultDevice;
} XACT_RENDERER_DETAILS, *LPXACT_RENDERER_DETAILS;

typedef BOOL (__stdcall *XACT_READFILE_CALLBACK)(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
);
typedef BOOL (__stdcall *XACT_GETOVERLAPPEDRESULT_CALLBACK)(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
);

typedef struct XACT_FILEIO_CALLBACKS
{
    XACT_READFILE_CALLBACK readFileCallback;
    XACT_GETOVERLAPPEDRESULT_CALLBACK getOverlappedResultCallback;
} XACT_FILEIO_CALLBACKS, *PXACT_FILEIO_CALLBACKS;
typedef const XACT_FILEIO_CALLBACKS *PCXACT_FILEIO_CALLBACKS;

typedef struct XACT_STREAMING_PARAMETERS
{
    HANDLE file;
    DWORD offset;
    DWORD flags;
    WORD packetSize;
} XACT_STREAMING_PARAMETERS, *LPXACT_STREAMING_PARAMETERS, XACT_WAVEBANK_STREAMING_PARAMETERS, *LPXACT_WAVEBANK_STREAMING_PARAMETERS;
typedef const XACT_STREAMING_PARAMETERS *LPCXACT_STREAMING_PARAMETERS;
typedef const XACT_WAVEBANK_STREAMING_PARAMETERS *LPCXACT_WAVEBANK_STREAMING_PARAMETERS;

typedef struct XACT_CUE_PROPERTIES
{
    char friendlyName[XACT_CUE_NAME_LENGTH];
    BOOL interactive;
    XACTINDEX iaVariableIndex;
    XACTINDEX numVariations;
    XACTINSTANCELIMIT maxInstances;
    XACTINSTANCELIMIT currentInstances;
} XACT_CUE_PROPERTIES, *LPXACT_CUE_PROPERTIES;

typedef struct XACT_TRACK_PROPERTIES
{
    XACTTIME duration;
    XACTINDEX numVariations;
    XACTCHANNEL numChannels;
    XACTINDEX waveVariation;
    XACTLOOPCOUNT loopCount;
} XACT_TRACK_PROPERTIES, *LPXACT_TRACK_PROPERTIES;

typedef struct XACT_VARIATION_PROPERTIES
{
    XACTINDEX index;
    XACTVARIATIONWEIGHT weight;
    XACTVARIABLEVALUE iaVariableMin;
    XACTVARIABLEVALUE iaVariableMax;
    BOOL linger;
} XACT_VARIATION_PROPERTIES, *LPXACT_VARIATION_PROPERTIES;

typedef struct XACT_SOUND_PROPERTIES
{
    XACTCATEGORY category;
    BYTE priority;
    XACTPITCH pitch;
    XACTVOLUME volume;
    XACTINDEX numTracks;
    XACT_TRACK_PROPERTIES arrTrackProperties[1];
} XACT_SOUND_PROPERTIES, *LPXACT_SOUND_PROPERTIES;

typedef struct XACT_SOUND_VARIATION_PROPERTIES
{
    XACT_VARIATION_PROPERTIES variationProperties;
    XACT_SOUND_PROPERTIES soundProperties;
} XACT_SOUND_VARIATION_PROPERTIES, *LPXACT_SOUND_VARIATION_PROPERTIES;

typedef struct XACT_CUE_INSTANCE_PROPERTIES
{
    DWORD allocAttributes;
    XACT_CUE_PROPERTIES cueProperties;
    XACT_SOUND_VARIATION_PROPERTIES activeVariationProperties;
} XACT_CUE_INSTANCE_PROPERTIES, *LPXACT_CUE_INSTANCE_PROPERTIES;

typedef struct XACTCHANNELMAPENTRY
{
    XACTCHANNEL InputChannel;
    XACTCHANNEL OutputChannel;
    XACTVOLUME Volume;
} XACTCHANNELMAPENTRY, *LPXACTCHANNELMAPENTRY;
typedef const XACTCHANNELMAPENTRY *LPCXACTCHANNELMAPENTRY;

typedef struct XACTCHANNELMAP
{
    XACTCHANNEL EntryCount;
    XACTCHANNELMAPENTRY* paEntries;
} XACTCHANNELMAP, *LPXACTCHANNELMAP;
typedef const XACTCHANNELMAP *LPCXACTCHANNELMAP;

typedef struct XACTCHANNELVOLUMEENTRY
{
    XACTCHANNEL EntryIndex;
    XACTVOLUME Volume;
} XACTCHANNELVOLUMEENTRY, *LPXACTCHANNELVOLUMEENTRY;
typedef const XACTCHANNELVOLUMEENTRY *LPCXACTCHANNELVOLUMEENTRY;

typedef struct XACTCHANNELVOLUME
{
    XACTCHANNEL EntryCount;
    XACTCHANNELVOLUMEENTRY* paEntries;
} XACTCHANNELVOLUME, *LPXACTCHANNELVOLUME;
typedef const XACTCHANNELVOLUME *LPCXACTCHANNELVOLUME;

static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPREPARED                     = 1;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPLAY                         = 2;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUESTOP                         = 3;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEDESTROYED                    = 4;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MARKER                          = 5;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED              = 6;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED               = 7;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED            = 8;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED           = 9;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUICONNECTED                    = 10;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUIDISCONNECTED                 = 11;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPREPARED                    = 12;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPLAY                        = 13;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVESTOP                        = 14;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVELOOPED                      = 15;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEDESTROYED                   = 16;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKPREPARED                 = 17;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT = 18;

static const BYTE XACT_FLAG_NOTIFICATION_PERSIST = 0x01;

#pragma pack(push,1)

typedef struct XACT_NOTIFICATION_DESCRIPTION
{
    XACTNOTIFICATIONTYPE type;
    BYTE flags;
    IXACT3SoundBank *pSoundBank;
    IXACT3WaveBank *pWaveBank;
    IXACT3Cue *pCue;
    IXACT3Wave *pWave;
    XACTINDEX cueIndex;
    XACTINDEX waveIndex;
    void* pvContext;
} XACT_NOTIFICATION_DESCRIPTION, *LPXACT_NOTIFICATION_DESCRIPTION;
typedef const XACT_NOTIFICATION_DESCRIPTION *LPCXACT_NOTIFICATION_DESCRIPTION;

typedef struct XACT_NOTIFICATION_CUE
{
    XACTINDEX cueIndex;
    IXACT3SoundBank *pSoundBank;
    IXACT3Cue *pCue;
} XACT_NOTIFICATION_CUE, *LPXACT_NOTIFICATION_CUE;
typedef const XACT_NOTIFICATION_CUE *LPCXACT_NOTIFICATION_CUE;

typedef struct XACT_NOTIFICATION_MARKER
{
    XACTINDEX cueIndex;
    IXACT3SoundBank *pSoundBank;
    IXACT3Cue *pCue;
    DWORD marker;
} XACT_NOTIFICATION_MARKER, *LPXACT_NOTIFICATION_MARKER;
typedef const XACT_NOTIFICATION_MARKER *LPCXACT_NOTIFICATION_MARKER;

typedef struct XACT_NOTIFICATION_SOUNDBANK
{
    IXACT3SoundBank *pSoundBank;
} XACT_NOTIFICATION_SOUNDBANK, *LPXACT_NOTIFICATION_SOUNDBANK;
typedef const XACT_NOTIFICATION_SOUNDBANK *LPCXACT_NOTIFICATION_SOUNDBANK;

typedef struct XACT_NOTIFICATION_WAVEBANK
{
    IXACT3WaveBank *pWaveBank;
} XACT_NOTIFICATION_WAVEBANK, *LPXACT_NOTIFICATION_WAVEBANK;
typedef const XACT_NOTIFICATION_WAVEBANK *LPCXACT_NOTIFICATION_WAVEBANK;

typedef struct XACT_NOTIFICATION_VARIABLE
{
    XACTINDEX cueIndex;
    IXACT3SoundBank *pSoundBank;
    IXACT3Cue *pCue;
    XACTVARIABLEINDEX variableIndex;
    XACTVARIABLEVALUE variableValue;
    BOOL local;
} XACT_NOTIFICATION_VARIABLE, *LPXACT_NOTIFICATION_VARIABLE;
typedef const XACT_NOTIFICATION_VARIABLE *LPCXACT_NOTIFICATION_VARIABLE;

typedef struct XACT_NOTIFICATION_GUI
{
    DWORD reserved;
} XACT_NOTIFICATION_GUI, *LPXACT_NOTIFICATION_GUI;
typedef const XACT_NOTIFICATION_GUI *LPCXACT_NOTIFICATION_GUI;

typedef struct XACT_NOTIFICATION_WAVE
{
    IXACT3WaveBank *pWaveBank;
    XACTINDEX waveIndex;
    XACTINDEX cueIndex;
    IXACT3SoundBank *pSoundBank;
    IXACT3Cue *pCue;
    IXACT3Wave *pWave;
} XACT_NOTIFICATION_WAVE, *LPXACT_NOTIFICATION_WAVE;
typedef const XACT_NOTIFICATION_WAVE *LPCXACT_NOTIFICATION_NAME;

typedef struct XACT_NOTIFICATION
{
    XACTNOTIFICATIONTYPE type;
    LONG timeStamp;
    PVOID pvContext;
    union
    {
        XACT_NOTIFICATION_CUE cue;
        XACT_NOTIFICATION_MARKER marker;
        XACT_NOTIFICATION_SOUNDBANK soundBank;
        XACT_NOTIFICATION_WAVEBANK waveBank;
        XACT_NOTIFICATION_VARIABLE variable;
        XACT_NOTIFICATION_GUI gui;
        XACT_NOTIFICATION_WAVE wave;
    };
} XACT_NOTIFICATION, *LPXACT_NOTIFICATION;
typedef const XACT_NOTIFICATION *LPCXACT_NOTIFICATION;

typedef void (__stdcall *XACT_NOTIFICATION_CALLBACK)(
    const XACT_NOTIFICATION *pNotification
);

#pragma pack(pop)

typedef struct XACT_WAVE_PROPERTIES
{
    char friendlyName[WAVEBANK_ENTRYNAME_LENGTH];
    WAVEBANKMINIWAVEFORMAT format;
    DWORD durationInSamples;
    WAVEBANKSAMPLEREGION loopRegion;
    BOOL streaming;
} XACT_WAVE_PROPERTIES, *LPXACT_WAVE_PROPERTIES;
typedef const XACT_WAVE_PROPERTIES *LPCXACT_WAVE_PROPERTIES;

typedef struct XACT_WAVE_INSTANCE_PROPERTIES
{
    XACT_WAVE_PROPERTIES properties;
    BOOL backgroundMusic;
} XACT_WAVE_INSTANCE_PROPERTIES, *LPXACT_WAVE_INSTANCE_PROPERTIES;
typedef const XACT_WAVE_INSTANCE_PROPERTIES *LPCXACT_WAVE_INSTANCE_PROPERTIES;

typedef struct XACT_RUNTIME_PARAMETERS
{
    DWORD lookAheadTime;
    void* pGlobalSettingsBuffer;
    DWORD globalSettingsBufferSize;
    DWORD globalSettingsFlags;
    DWORD globalSettingsAllocAttributes;
    XACT_FILEIO_CALLBACKS fileIOCallbacks;
    XACT_NOTIFICATION_CALLBACK fnNotificationCallback;
    LPCWSTR pRendererID;
    IXAudio2 *pXAudio2;
    IXAudio2MasteringVoice *pMasteringVoice;
} XACT_RUNTIME_PARAMETERS, *LPXACT_RUNTIME_PARAMETERS;
typedef const XACT_RUNTIME_PARAMETERS *LPCXACT_RUNTIME_PARAMETERS;

#define XACT_FLAG_CUE_STOP_RELEASE   XACT_FLAG_STOP_RELEASE
#define XACT_FLAG_CUE_STOP_IMMEDIATE XACT_FLAG_STOP_IMMEDIATE

#define XACT_CUESTATE_CREATED   XACT_STATE_CREATED
#define XACT_CUESTATE_PREPARING XACT_STATE_PREPARING
#define XACT_CUESTATE_PREPARED  XACT_STATE_PREPARED
#define XACT_CUESTATE_PLAYING   XACT_STATE_PLAYING
#define XACT_CUESTATE_STOPPING  XACT_STATE_STOPPING
#define XACT_CUESTATE_STOPPED   XACT_STATE_STOPPED
#define XACT_CUESTATE_PAUSED    XACT_STATE_PAUSED

/*****************************************************************************
 * IXACT3Cue interface
 */
#define INTERFACE IXACT3Cue
DECLARE_INTERFACE(IXACT3Cue)
{
    /*** IXACT3Cue methods ***/
    STDMETHOD(Play)(THIS) PURE;
    STDMETHOD(Stop)(THIS_ DWORD pdwFlags) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *pdwState) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(SetMatrixCoefficients)(THIS_ UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float *pMatrixCoefficients) PURE;
    STDMETHOD_(XACTVARIABLEINDEX,GetVariableIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(SetVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue) PURE;
    STDMETHOD(GetVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE *nValue) PURE;
    STDMETHOD(Pause)(THIS_ BOOL fPause) PURE;
    STDMETHOD(GetProperties)(THIS_ LPXACT_CUE_INSTANCE_PROPERTIES *ppProperties) PURE;
#if XACT3_VER >= 0x0305
    STDMETHOD(SetOutputVoices)(THIS_ const XAUDIO2_VOICE_SENDS *pSendList) PURE;
    STDMETHOD(SetOutputVoiceMatrix)(THIS_ IXAudio2Voice *pDestinationVoice, UINT32 SourceChannels, UINT32 DestinationChannels, const float *pLevelMatrix) PURE;
#endif
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IXACT3Cue methods ***/
#define IXACT3Cue_Play(p)               (p)->lpVtbl->Destroy(p)
#define IXACT3Cue_Stop(p,a)             (p)->lpVtbl->Stop(p,a)
#define IXACT3Cue_GetState(p,a)         (p)->lpVtbl->GetState(p,a)
#define IXACT3Cue_Destroy(p)            (p)->lpVtbl->Destroy(p)
#define IXACT3Cue_SetMatrixCoefficients(p,a,b,c) (p)->lpVtbl->SetMatrixCoefficients(p,a,b,c)
#define IXACT3Cue_GetVariableIndex(p,a) (p)->lpVtbl->GetVariableIndex(p,a)
#define IXACT3Cue_SetVariable(p,a,b)    (p)->lpVtbl->SetVariable(p,a,b)
#define IXACT3Cue_GetVariable(p,a,b)    (p)->lpVtbl->GetVariable(p,a,b)
#define IXACT3Cue_Pause(p,a)            (p)->lpVtbl->Pause(p,a)
#define IXACT3Cue_GetProperties(p,a)    (p)->lpVtbl->GetProperties(p,a)
#define IXACT3Cue_SetOutputVoices(p,a)  (p)->lpVtbl->SetOutputVoices(p,a)
#define IXACT3Cue_SetOutputVoiceMatrix(p,a,b,c,d) (p)->lpVtbl->SetOutputVoiceMatrix(p,a,b,c,d)
#else
/*** IXACT3Cue methods ***/
#define IXACT3Cue_Play(p)               (p)->Destroy()
#define IXACT3Cue_Stop(p,a)             (p)->Stop(a)
#define IXACT3Cue_GetState(p,a)         (p)->Stop(a)
#define IXACT3Cue_Destroy(p)            (p)->Destroy()
#define IXACT3Cue_SetMatrixCoefficients(p,a,b,c) (p)->SetMatrixCoefficients(a,b,c)
#define IXACT3Cue_GetVariableIndex(p,a) (p)->GetVariableIndex(a)
#define IXACT3Cue_SetVariable(p,a,b)    (p)->SetVariable(a,b)
#define IXACT3Cue_GetVariable(p,a,b)    (p)->GetVariable(a,b)
#define IXACT3Cue_Pause(p,a)            (p)->Pause(a)
#define IXACT3Cue_GetProperties(p,a)    (p)->GetProperties(a)
#define IXACT3Cue_SetOutputVoices(p,a)  (p)->SetOutputVoices(a)
#define IXACT3Cue_SetOutputVoiceMatrix(p,a,b,c,d) (p)->SetOutputVoiceMatrix(a,b,c,d)
#endif

/*****************************************************************************
 * IXACT3Wave interface
 */
#define INTERFACE IXACT3Wave
DECLARE_INTERFACE(IXACT3Wave)
{
    /*** IXACT3Wave methods ***/
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(Play)(THIS) PURE;
    STDMETHOD(Stop)(THIS_ DWORD pdwFlags) PURE;
    STDMETHOD(Pause)(THIS_ BOOL fPause) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *pdwState) PURE;
    STDMETHOD(SetPitch)(THIS_ XACTPITCH pitch) PURE;
    STDMETHOD(SetVolume)(THIS_ XACTVOLUME volume) PURE;
    STDMETHOD(SetMatrixCoefficients)(THIS_ UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float *pMatrixCoefficients) PURE;
    STDMETHOD(GetProperties)(THIS_ LPXACT_WAVE_INSTANCE_PROPERTIES pProperties) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IXACT3Wave methods ***/
#define IXACT3Wave_Destroy(p)            (p)->lpVtbl->Destroy(p)
#define IXACT3Wave_Play(p)               (p)->lpVtbl->Destroy(p)
#define IXACT3Wave_Stop(p,a)             (p)->lpVtbl->Stop(p,a)
#define IXACT3Wave_Pause(p,a)            (p)->lpVtbl->Pause(p,a)
#define IXACT3Wave_GetState(p,a)         (p)->lpVtbl->GetState(p,a)
#define IXACT3Wave_SetPitch(p,a)         (p)->lpVtbl->SetPitch(p,a)
#define IXACT3Wave_SetVolume(p,a)        (p)->lpVtbl->SetVolume(p,a)
#define IXACT3Wave_SetMatrixCoefficients(p,a,b,c) (p)->lpVtbl->SetMatrixCoefficients(p,a,b,c)
#define IXACT3Wave_GetProperties(p,a)    (p)->lpVtbl->GetProperties(p,a)
#else
/*** IXACT3Wave methods ***/
#define IXACT3Wave_Destroy(p)            (p)->Destroy()
#define IXACT3Wave_Play(p)               (p)->Destroy()
#define IXACT3Wave_Stop(p,a)             (p)->Stop(a)
#define IXACT3Wave_Pause(p,a)            (p)->Pause(a)
#define IXACT3Wave_GetState(p,a)         (p)->Stop(a)
#define IXACT3Wave_SetPitch(p,a)         (p)->SetVariable(a)
#define IXACT3Wave_SetVolume(p,a)        (p)->SetVolume(a)
#define IXACT3Wave_SetMatrixCoefficients(p,a,b,c) (p)->SetMatrixCoefficients(a,b,c)
#define IXACT3Wave_GetProperties(p,a)    (p)->GetProperties(a)
#endif

#define XACT_FLAG_SOUNDBANK_STOP_IMMEDIATE XACT_FLAG_STOP_IMMEDIATE
#define XACT_SOUNDBANKSTATE_INUSE          XACT_STATE_INUSE

/*****************************************************************************
 * IXACT3SoundBank interface
 */
#define INTERFACE IXACT3SoundBank
DECLARE_INTERFACE(IXACT3SoundBank)
{
    /*** IXACT3SoundBank methods ***/
    STDMETHOD_(XACTINDEX,GetCueIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(GetNumCues)(THIS_ XACTINDEX *pnNumCues) PURE;
    STDMETHOD(GetCueProperties)(THIS_ XACTINDEX nCueIndex, LPXACT_CUE_PROPERTIES pProperties) PURE;
    STDMETHOD(Prepare)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACT3Cue **ppCue) PURE;
    STDMETHOD(Play)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACT3Cue **ppCue) PURE;
    STDMETHOD(Stop)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *pdwState) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IXACT3SoundBank methods ***/
#define IXACT3SoundBank_GetCueIndex(p,a)   (p)->lpVtbl->GetCueIndex(p,a)
#define IXACT3SoundBank_GetNumCues(p,a)    (p)->lpVtbl->GetNumCues(p,a)
#define IXACT3SoundBank_GetCueProperties(p,a,b) (p)->lpVtbl->GetCueProperties(p,a,b)
#define IXACT3SoundBank_Prepare(p,a,b,c,d) (p)->lpVtbl->Prepare(p,a,b,c,d)
#define IXACT3SoundBank_Play(p,a,b,c,d)    (p)->lpVtbl->Play(p,a,b,c,d)
#define IXACT3SoundBank_Stop(p,a,b)        (p)->lpVtbl->Stop(p,a,b)
#define IXACT3SoundBank_Destroy(p)         (p)->lpVtbl->Destroy(p)
#define IXACT3SoundBank_GetState(p,a)      (p)->lpVtbl->GetState(p,a)
#else
/*** IXACT3SoundBank methods ***/
#define IXACT3SoundBank_GetCueIndex(p,a)   (p)->GetCueIndex(a)
#define IXACT3SoundBank_GetNumCues(p,a)    (p)->GetNumCues(a)
#define IXACT3SoundBank_GetCueProperties(p,a,b) (p)->GetCueProperties(a,b)
#define IXACT3SoundBank_Prepare(p,a,b,c,d) (p)->Prepare(a,b,c,d)
#define IXACT3SoundBank_Play(p,a,b,c,d)    (p)->Play(a,b,c,d)
#define IXACT3SoundBank_Stop(p,a,b)        (p)->Stop(a,b)
#define IXACT3SoundBank_Destroy(p)         (p)->Destroy()
#define IXACT3SoundBank_GetState(p,a)      (p)->GetState(a)
#endif

#define XACT_WAVEBANKSTATE_INUSE         XACT_STATE_INUSE
#define XACT_WAVEBANKSTATE_PREPARED      XACT_STATE_PREPARED
#define XACT_WAVEBANKSTATE_PREPAREFAILED XACT_STATE_PREPAREFAILED

/*****************************************************************************
 * IXACT3WaveBank interface
 */
#define INTERFACE IXACT3WaveBank
DECLARE_INTERFACE(IXACT3WaveBank)
{
    /*** IXACT3WaveBank methods ***/
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetNumWaves)(THIS_ XACTINDEX *pnNumWaves) PURE;
    STDMETHOD_(XACTINDEX,GetWaveIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(GetWaveProperties)(THIS_ XACTINDEX nWaveIndex, LPXACT_WAVE_PROPERTIES pWaveProperties) PURE;
    STDMETHOD(Prepare)(THIS_ XACTINDEX nWaveIndex, DWORD dwFlags, DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount, IXACT3Wave **ppWave) PURE;
    STDMETHOD(Play)(THIS_ XACTINDEX nWaveIndex, DWORD dwFlags, DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount, IXACT3Wave **ppWave) PURE;
    STDMETHOD(Stop)(THIS_ XACTINDEX nWaveIndex, DWORD dwFlags) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *pdwState) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IXACT3WaveBank methods ***/
#define IXACT3WaveBank_Destroy(p)          (p)->lpVtbl->Destroy(p)
#define IXACT3WaveBank_GetNumWaves(p,a)    (p)->lpVtbl->GetNumCues(p,a)
#define IXACT3WaveBank_GetWaveIndex(p,a)   (p)->lpVtbl->GetWaveIndex(p,a)
#define IXACT3WaveBank_GetWaveProperties(p,a,b) (p)->lpVtbl->GetWaveProperties(p,a,b)
#define IXACT3WaveBank_Prepare(p,a,b,c,d,e)     (p)->lpVtbl->Prepare(p,a,b,c,d,e)
#define IXACT3WaveBank_Play(p,a,b,c,d,e)   (p)->lpVtbl->Play(p,a,b,c,d,e)
#define IXACT3WaveBank_Stop(p,a,b)         (p)->lpVtbl->Stop(p,a,b)
#define IXACT3WaveBank_GetState(p,a)       (p)->lpVtbl->GetState(p,a)
#else
/*** IXACT3WaveBank methods ***/
#define IXACT3WaveBank_Destroy(p)          (p)->Destroy()
#define IXACT3WaveBank_GetNumWaves(p,a)    (p)->GetNumWaves(a)
#define IXACT3WaveBank_GetWaveIndex(p,a)   (p)->GetWaveIndex(a)
#define IXACT3WaveBank_GetWaveProperties(p,a,b) (p)->GetWaveProperties(a,b)
#define IXACT3WaveBank_Prepare(p,a,b,c,d,e)     (p)->Prepare(a,b,c,d,e)
#define IXACT3WaveBank_Play(p,a,b,c,d,e)  (p)->Play(a,b,c,d,e)
#define IXACT3WaveBank_Stop(p,a,b)        (p)->Stop(a,b)
#define IXACT3WaveBank_GetState(p,a)      (p)->GetState(a)
#endif

#define XACT_FLAG_ENGINE_CREATE_MANAGEDATA XACT_FLAG_MANAGEDATA
#define XACT_FLAG_ENGINE_STOP_IMMEDIATE    XACT_FLAG_STOP_IMMEDIATE

/*****************************************************************************
 * IXACT3Engine interface
 */
#define INTERFACE IXACT3Engine
DECLARE_INTERFACE_(IXACT3Engine,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IXACT3Engine methods ***/
    STDMETHOD(GetRendererCount)(THIS_ XACTINDEX *pnRendererCount) PURE;
    STDMETHOD(GetRendererDetails)(THIS_ XACTINDEX nRendererIndex, LPXACT_RENDERER_DETAILS pRendererDetails) PURE;
    STDMETHOD(GetFinalMixFormat)(THIS_ WAVEFORMATEXTENSIBLE *pFinalMixFormat) PURE;
    STDMETHOD(Initialize)(THIS_ const XACT_RUNTIME_PARAMETERS *pParams) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
    STDMETHOD(DoWork)(THIS) PURE;
    STDMETHOD(CreateSoundBank)(THIS_ const void *pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACT3SoundBank **ppSoundBank) PURE;
    STDMETHOD(CreateInMemoryWaveBank)(THIS_ const void *pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACT3WaveBank **ppWaveBank) PURE;
    STDMETHOD(CreateStreamingWaveBank)(THIS_ const XACT_WAVEBANK_STREAMING_PARAMETERS *pParams, IXACT3WaveBank **ppWaveBank) PURE;
    STDMETHOD(PrepareWave)(THIS_ DWORD dwFlags, PCSTR szWavePath, WORD wStreamingPacketSize, DWORD dwAlignment,
                           DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount, IXACT3Wave **ppWave) PURE;
    STDMETHOD(PrepareInMemoryWave)(THIS_ DWORD dwFlags, WAVEBANKENTRY entry, DWORD *pdwSeekTable, BYTE *pbWaveData,
                           DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount, IXACT3Wave **ppWave) PURE;
    STDMETHOD(PrepareStreamingWave)(THIS_ DWORD dwFlags, WAVEBANKENTRY entry, XACT_STREAMING_PARAMETERS streamingParams, DWORD dwAlignment, DWORD *pdwSeekTable,
                           DWORD dwPlayOffset, XACTLOOPCOUNT nLoopCount, IXACT3Wave **ppWave) PURE;
    STDMETHOD(RegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION *pNotificationDesc) PURE;
    STDMETHOD(UnRegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION *pNotificationDesc) PURE;
    STDMETHOD_(XACTCATEGORY,GetCategory)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(Stop)(THIS_ XACTCATEGORY nCategory, DWORD dwFlags) PURE;
    STDMETHOD(SetVolume)(THIS_ XACTCATEGORY nCategory, XACTVOLUME nVolume) PURE;
    STDMETHOD(Pause)(THIS_ XACTCATEGORY nCategory, BOOL fPause) PURE;
    STDMETHOD_(XACTVARIABLEINDEX,GetGlobalVariableIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(SetGlobalVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue) PURE;
    STDMETHOD(GetGlobalVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE *nValue) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IXACT3Engine_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IXACT3Engine_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IXACT3Engine_Release(p)            (p)->lpVtbl->Release(p)
/*** IXACT3Engine methods ***/
#define IXACT3Engine_GetRendererCount(p,a)         (p)->lpVtbl->GetRendererCount(p,a)
#define IXACT3Engine_GetRendererDetails(p,a,b)     (p)->lpVtbl->GetRendererDetails(p,a,b)
#define IXACT3Engine_GetFinalMixFormat(p,a)        (p)->lpVtbl->GetFinalMixFormat(p,a)
#define IXACT3Engine_Initialize(p,a)               (p)->lpVtbl->Initialize(p,a)
#define IXACT3Engine_Shutdown(p)                   (p)->lpVtbl->Shutdown(p)
#define IXACT3Engine_DoWork(p)                     (p)->lpVtbl->DoWork(p)
#define IXACT3Engine_CreateSoundBank(p,a,b,c,d,e)  (p)->lpVtbl->CreateSoundBank(p,a,b,c,d,e)
#define IXACT3Engine_CreateInMemoryWaveBank(p,a,b,c,d,e) (p)->lpVtbl->CreateInMemoryWaveBank(p,a,b,c,d,e)
#define IXACT3Engine_CreateStreamingWaveBank(p,a,b) (p)->lpVtbl->CreateStreamingWaveBank(p,a,b)
#define IXACT3Engine_PrepareWave(p,a,b,c,d,e,f,g)  (p)->lpVtbl->PrepareWave(p,a,b,c,d,e,f,g)
#define IXACT3Engine_PrepareInMemoryWave(p,a,b,c,d,e,f,g) (p)->lpVtbl->PrepareInMemoryWave(p,a,b,c,d,e,f,g)
#define IXACT3Engine_PrepareStreamingWave(p,a,b,c,d,e,f,g,h) (p)->lpVtbl->PrepareStreamingWave(p,a,b,c,d,e,f,g,h)
#define IXACT3Engine_RegisterNotification(p,a)     (p)->lpVtbl->RegisterNotification(p,a)
#define IXACT3Engine_UnRegisterNotification(p,a)   (p)->lpVtbl->UnRegisterNotification(p,a)
#define IXACT3Engine_GetCategory(p,a)              (p)->lpVtbl->GetCategory(p,a)
#define IXACT3Engine_Stop(p,a,b)                   (p)->lpVtbl->Stop(a,b)
#define IXACT3Engine_SetVolume(p,a,b)              (p)->lpVtbl->SetVolume(p,a,b)
#define IXACT3Engine_Pause(p,a,b)                  (p)->lpVtbl->Pause(a,b)
#define IXACT3Engine_GetGlobalVariableIndex(p,a)   (p)->lpVtbl->GetGlobalVariableIndex(p,a)
#define IXACT3Engine_SetGlobalVariable(p,a,b)      (p)->lpVtbl->SetGlobalVariable(p,a,b)
#define IXACT3Engine_GetGlobalVariable(p,a,b)      (p)->lpVtbl->GetGlobalVariable(p,a,b)
#else
/*** IUnknown methods ***/
#define IXACT3Engine_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IXACT3Engine_AddRef(p)             (p)->AddRef()
#define IXACT3Engine_Release(p)            (p)->Release()
/*** IXACT3Engine methods ***/
#define IXACT3Engine_GetRendererCount(p,a)         (p)->GetRendererCount(a)
#define IXACT3Engine_GetRendererDetails(p,a,b)     (p)->GetRendererDetails(a,b)
#define IXACT3Engine_GetFinalMixFormat(p,a)        (p)->GetFinalMixFormat(a)
#define IXACT3Engine_Initialize(p,a)               (p)->Initialize(a)
#define IXACT3Engine_Shutdown(p)                   (p)->Shutdown()
#define IXACT3Engine_DoWork(p)                     (p)->DoWork()
#define IXACT3Engine_CreateSoundBank(p,a,b,c,d,e)  (p)->CreateSoundBank(a,b,c,d,e)
#define IXACT3Engine_CreateInMemoryWaveBank(p,a,b,c,d,e) (p)->CreateInMemoryWaveBank(a,b,c,d,e)
#define IXACT3Engine_CreateStreamingWaveBank(p,a,b) (p)->CreateStreamingWaveBank(a,b)
#define IXACT3Engine_PrepareWave(p,a,b,c,d,e,f,g)  (p)->PrepareWave(a,b,c,d,e,f,g)
#define IXACT3Engine_PrepareInMemoryWave(p,a,b,c,d,e,f,g) (p)->PrepareInMemoryWave(a,b,c,d,e,f,g)
#define IXACT3Engine_PrepareStreamingWave(p,a,b,c,d,e,f,g,h) (p)->PrepareStreamingWave(a,b,c,d,e,f,g,h)
#define IXACT3Engine_RegisterNotification(p,a)     (p)->RegisterNotification(a)
#define IXACT3Engine_UnRegisterNotification(p,a)   (p)->UnRegisterNotification(a)
#define IXACT3Engine_GetCategory(p,a)              (p)->GetCategory(a)
#define IXACT3Engine_Stop(p,a,b)                   (p)->Stop(a,b)
#define IXACT3Engine_SetVolume(p,a,b)              (p)->SetVolume(p,a,b)
#define IXACT3Engine_Pause(p,a,b)                  (p)->Pause(a,b)
#define IXACT3Engine_GetGlobalVariableIndex(p,a)   (p)->GetGlobalVariableIndex(a)
#define IXACT3Engine_SetGlobalVariable(p,a,b)      (p)->SetGlobalVariable(a,b)
#define IXACT3Engine_GetGlobalVariable(p,a,b)      (p)->GetGlobalVariable(a,b)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __WINE_XACT3_H */
