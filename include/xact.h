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

#ifndef _XACT_H_
#define _XACT_H_

#include <windows.h>
#include <objbase.h>
#include <float.h>
#include <limits.h>
#include <xact2wb.h>

#if XACT3_VER == 0x0210
    DEFINE_GUID(CLSID_XACTEngine, 0x65d822a4, 0x4799, 0x42c6, 0x9b, 0x18, 0xd2, 0x6c, 0xf6, 0x6d, 0xd3, 0x20);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x03dd980d, 0xeca1, 0x4de0, 0x98, 0x22, 0xeb, 0x22, 0x44, 0xa0, 0xe2, 0x75);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x871dc2b3, 0xc947, 0x4aed, 0xbf, 0xad, 0xa1, 0x79, 0x18, 0xfb, 0xcf, 0x05);
    DEFINE_GUID(IID_IXACTEngine, 0x5ac3994b, 0xac77, 0x4c40, 0xb9, 0xfd, 0x7d, 0x5a, 0xfb, 0xe9, 0x64, 0xc5);
#elif XACT3_VER == 0x0209
    DEFINE_GUID(CLSID_XACTEngine, 0x343e68e6, 0x8f82, 0x4a8d, 0xa2, 0xda, 0x6e, 0x9a, 0x94, 0x4b, 0x37, 0x8c);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0xcedde475, 0x50b5, 0x47ef, 0x91, 0xa7, 0x3b, 0x49, 0xa0, 0xe8, 0xe5, 0x88);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x3cbb606b, 0x06f1, 0x473e, 0x9d, 0xd5, 0x0e, 0x4a, 0x3b, 0x47, 0x14, 0x13);
    DEFINE_GUID(IID_IXACTEngine, 0x893ff2e4, 0x8d03, 0x4d5f, 0xb0, 0xaa, 0x36, 0x3a, 0x9c, 0xbb, 0xf4, 0x37);
#elif XACT3_VER == 0x0208
    DEFINE_GUID(CLSID_XACTEngine, 0x77c56bf4, 0x18a1, 0x42b0, 0x88, 0xaf, 0x50, 0x72, 0xce, 0x81, 0x49, 0x49);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x46f7c2b1, 0x774b, 0x419e, 0x9a, 0xbe, 0xa4, 0x8f, 0xb0, 0x42, 0xa3, 0xb0);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x307473ef, 0xc3d4, 0x4d97, 0x9b, 0x9c, 0xce, 0x23, 0x92, 0x85, 0x21, 0xdb);
    DEFINE_GUID(IID_IXACTEngine, 0xf1fff4f0, 0xec75, 0x45cb, 0x98, 0x6d, 0xe6, 0x37, 0xf7, 0xe7, 0xcd, 0xd5);
#elif XACT3_VER == 0x0207
    DEFINE_GUID(CLSID_XACTEngine, 0xcd0d66ec, 0x8057, 0x43f5, 0xac, 0xbd, 0x66, 0xdf, 0xb3, 0x6f, 0xd7, 0x8c);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x9b94bf7a, 0xce0f, 0x4c68, 0x8b, 0x5e, 0xd0, 0x62, 0xcb, 0x37, 0x30, 0xc3);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x1bd54a4b, 0xa1dc, 0x4e4c, 0x92, 0xa2, 0x73, 0xed, 0x33, 0x55, 0x21, 0x48);
    DEFINE_GUID(IID_IXACTEngine, 0xc2f0af68, 0x1f6d, 0x40ed, 0x96, 0x4f, 0x26, 0x25, 0x68, 0x42, 0xed, 0xc4);
#elif XACT3_VER == 0x0206
    DEFINE_GUID(CLSID_XACTEngine, 0x3a2495ce, 0x31d0, 0x435b, 0x8c, 0xcf, 0xe9, 0xf0, 0x84, 0x3f, 0xd9, 0x60);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0xa17e147b, 0xc168, 0x45d4, 0x95, 0xf6, 0xb2, 0x15, 0x15, 0xec, 0x1e, 0x66);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0xfe7e064f, 0xf9ea, 0x49ee, 0xba, 0x64, 0x84, 0x5e, 0x42, 0x35, 0x59, 0xc5);
    DEFINE_GUID(IID_IXACTEngine, 0x5a5d41d0, 0x2161, 0x4a39, 0xaa, 0xdc, 0x11, 0x49, 0x30, 0x14, 0x7e, 0xaa);
#elif XACT3_VER == 0x0205
    DEFINE_GUID(CLSID_XACTEngine, 0x54b68bc7, 0x3a45, 0x416b, 0xa8, 0xc9, 0x19, 0xbf, 0x19, 0xec, 0x1d, 0xf5);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0xaeaf4809, 0x6e94, 0x4663, 0x8f, 0xf8, 0x1b, 0x4c, 0x7c, 0x0e, 0x6d, 0xfd);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x290d0a8c, 0xa131, 0x4cf4, 0x8b, 0xbd, 0x37, 0xd0, 0x2b, 0x59, 0xcc, 0x4a);
    DEFINE_GUID(IID_IXACTEngine, 0xf9df94ad, 0x6960, 0x4307, 0xbf, 0xad, 0x4e, 0x97, 0xac, 0x18, 0x94, 0xc6);
#elif XACT3_VER == 0x0204
    DEFINE_GUID(CLSID_XACTEngine, 0xbc3e0fc6, 0x2e0d, 0x4c45, 0xbc, 0x61, 0xd9, 0xc3, 0x28, 0x31, 0x9b, 0xd8);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x30bad9f7, 0x0018, 0x49e9, 0xbf, 0x94, 0x4a, 0xe8, 0x9c, 0xc5, 0x4d, 0x64);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x74ee14d5, 0xca1d, 0x44ac, 0x8b, 0xd3, 0xfa, 0x94, 0xf7, 0x34, 0x6e, 0x24);
    DEFINE_GUID(IID_IXACTEngine, 0x43a0d4a8, 0x9387, 0x4e06, 0x94, 0x33, 0x65, 0x41, 0x8f, 0xe7, 0x0a, 0x67);
#elif XACT3_VER == 0x0203
    DEFINE_GUID(CLSID_XACTEngine, 0x1138472b, 0xd187, 0x44e9, 0x81, 0xf2, 0xae, 0x1b, 0x0e, 0x77, 0x85, 0xf1);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x07fb2b69, 0x0ee4, 0x4eee, 0xbb, 0xe8, 0xc7, 0x62, 0x89, 0x79, 0x87, 0x17);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x9ebf716a, 0x4c83, 0x4db0, 0x9a, 0x5a, 0xc7, 0x27, 0xde, 0x92, 0xb2, 0x73);
    DEFINE_GUID(IID_IXACTEngine, 0xb28629f1, 0x0cb0, 0x47bc, 0xb4, 0x6a, 0xa2, 0xa1, 0xa7, 0x29, 0x6f, 0x02);
#elif XACT3_VER == 0x0202
    DEFINE_GUID(CLSID_XACTEngine, 0xc60fae90, 0x4183, 0x4a3f, 0xb2, 0xf7, 0xac, 0x1d, 0xc4, 0x9b, 0x0e, 0x5c);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x4cb2112c, 0x62e9, 0x43fc, 0x8d, 0x9d, 0x6c, 0xa0, 0x8c, 0xed, 0x45, 0xf1);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x3ec76fdc, 0xb626, 0x43fb, 0xbe, 0x22, 0x9b, 0x43, 0x1d, 0x06, 0xb9, 0x68);
    DEFINE_GUID(IID_IXACTEngine, 0x9c454686, 0xb827, 0x4e5e, 0x88, 0xd9, 0x5b, 0x99, 0xd6, 0x6b, 0x02, 0x2f);
#elif XACT3_VER == 0x0201
    DEFINE_GUID(CLSID_XACTEngine, 0x1f1b577e, 0x5e5a, 0x4e8a, 0xba, 0x73, 0xc6, 0x57, 0xea, 0x8e, 0x85, 0x98);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0xfcecc8be, 0xb09a, 0x48cb, 0x92, 0x08, 0x95, 0xa7, 0xed, 0x45, 0x82, 0xa6);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x2b4a46bb, 0xae7a, 0x4072, 0xae, 0x18, 0x11, 0x28, 0x15, 0x4f, 0xba, 0x97);
    DEFINE_GUID(IID_IXACTEngine, 0x7cdd1894, 0x643b, 0x4168, 0x83, 0x6f, 0xd1, 0x9d, 0x59, 0xd0, 0xce, 0x53);
#else
    DEFINE_GUID(CLSID_XACTEngine, 0x0aa000aa, 0xf404, 0x11d9, 0xbd, 0x7a, 0x00, 0x10, 0xdc, 0x4f, 0x8f, 0x81);
    DEFINE_GUID(CLSID_XACTAuditionEngine, 0x0aa000ab, 0xf404, 0x11d9, 0xbd, 0x7a, 0x00, 0x10, 0xdc, 0x4f, 0x8f, 0x81);
    DEFINE_GUID(CLSID_XACTDebugEngine, 0x0aa000ac, 0xf404, 0x11d9, 0xbd, 0x7a, 0x00, 0x10, 0xdc, 0x4f, 0x8f, 0x81);
    DEFINE_GUID(IID_IXACTEngine, 0x0aa000a0, 0xf404, 0x11d9, 0xbd, 0x7a, 0x00, 0x10, 0xdc, 0x4f, 0x8f, 0x81);
#endif

typedef struct IXACTSoundBank IXACTSoundBank;
typedef struct IXACTWaveBank IXACTWaveBank;
typedef struct IXACTCue IXACTCue;
typedef struct IXACTWave IXACTWave;
typedef struct IXACTEngine IXACTEngine;

typedef WORD XACTCATEGORY;
typedef BYTE XACTCHANNEL;
typedef WORD XACTINDEX;
typedef BYTE XACTINSTANCELIMIT;
typedef BYTE XACTLOOPCOUNT;
typedef BYTE XACTNOTIFICATIONTYPE;
typedef SHORT XACTPITCH;
typedef BYTE XACTPRIORITY;
typedef LONG XACTTIME;
typedef WORD XACTVARIABLEINDEX;
typedef FLOAT XACTVARIABLEVALUE;
typedef BYTE XACTVARIATIONWEIGHT;
typedef FLOAT XACTVOLUME;

static const XACTCATEGORY XACTCATEGORY_MIN = 0x0;
static const XACTCATEGORY XACTCATEGORY_MAX = 0xfffe;
static const XACTCATEGORY XACTCATEGORY_INVALID = 0xffff;
static const XACTCHANNEL XACTCHANNEL_MIN = 0;
static const XACTCHANNEL XACTCHANNEL_MAX = 0xff;
static const XACTINDEX XACTINDEX_MIN = 0x0;
static const XACTINDEX XACTINDEX_MAX = 0xfffe;
static const XACTINDEX XACTINDEX_INVALID = 0xffff;
static const XACTINSTANCELIMIT XACTINSTANCELIMIT_INFINITE = 0xff;
static const XACTINSTANCELIMIT XACTINSTANCELIMIT_MIN = 0x00;
static const XACTINSTANCELIMIT XACTINSTANCELIMIT_MAX = 0xfe;
static const XACTLOOPCOUNT XACTLOOPCOUNT_MIN = 0x0;
static const XACTLOOPCOUNT XACTLOOPCOUNT_MAX = 0xfe;
static const XACTLOOPCOUNT XACTLOOPCOUNT_INFINITE = 0xff;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MIN = 0x00;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MAX = 0xff;
static const XACTPITCH XACTPITCH_MIN = -1200;
static const XACTPITCH XACTPITCH_MAX = 1200;
static const XACTPITCH XACTPITCH_MIN_TOTAL = -2400;
static const XACTPITCH XACTPITCH_MAX_TOTAL = 2400;
static const XACTTIME XACTTIME_MIN = INT_MIN;
static const XACTTIME XACTTIME_MAX = INT_MAX;
static const XACTTIME XACTTIME_INFINITE = INT_MAX;
static const XACTVARIABLEINDEX XACTVARIABLEINDEX_MIN = 0x0000;
static const XACTVARIABLEINDEX XACTVARIABLEINDEX_MAX = 0xfffe;
static const XACTVARIABLEINDEX XACTVARIABLEINDEX_INVALID = 0xffff;
static const XACTVARIABLEVALUE XACTPARAMETERVALUE_MIN = -FLT_MAX;
static const XACTVARIABLEVALUE XACTPARAMETERVALUE_MAX = FLT_MAX;
static const XACTVARIABLEVALUE XACTVARIABLEVALUE_MIN = -FLT_MAX;
static const XACTVARIABLEVALUE XACTVARIABLEVALUE_MAX = FLT_MAX;
static const XACTVOLUME XACTVOLUME_MIN = 0.0f;
static const XACTVOLUME XACTVOLUME_MAX = FLT_MAX;
static const DWORD XACTWAVEALIGNMENT_MIN = 2048;


#define FACILITY_XACTENGINE 0xac7
#define XACTENGINEERROR(n) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XACTENGINE, n)

#define XACTENGINE_E_OUTOFMEMORY               E_OUTOFMEMORY
#define XACTENGINE_E_INVALIDARG                E_INVALIDARG
#define XACTENGINE_E_NOTIMPL                   E_NOTIMPL
#define XACTENGINE_E_FAIL                      E_FAIL

#define XACTENGINE_E_ALREADYINITIALIZED             XACTENGINEERROR(0x001)
#define XACTENGINE_E_NOTINITIALIZED                 XACTENGINEERROR(0x002)
#define XACTENGINE_E_EXPIRED                        XACTENGINEERROR(0x003)
#define XACTENGINE_E_NONOTIFICATIONCALLBACK         XACTENGINEERROR(0x004)
#define XACTENGINE_E_NOTIFICATIONREGISTERED         XACTENGINEERROR(0x005)
#define XACTENGINE_E_INVALIDUSAGE                   XACTENGINEERROR(0x006)
#define XACTENGINE_E_INVALIDDATA                    XACTENGINEERROR(0x007)
#define XACTENGINE_E_INSTANCELIMITFAILTOPLAY        XACTENGINEERROR(0x008)
#define XACTENGINE_E_NOGLOBALSETTINGS               XACTENGINEERROR(0x009)
#define XACTENGINE_E_INVALIDVARIABLEINDEX           XACTENGINEERROR(0x00a)
#define XACTENGINE_E_INVALIDCATEGORY                XACTENGINEERROR(0x00b)
#define XACTENGINE_E_INVALIDCUEINDEX                XACTENGINEERROR(0x00c)
#define XACTENGINE_E_INVALIDWAVEINDEX               XACTENGINEERROR(0x00d)
#define XACTENGINE_E_INVALIDTRACKINDEX              XACTENGINEERROR(0x00e)
#define XACTENGINE_E_INVALIDSOUNDOFFSETORINDEX      XACTENGINEERROR(0x00f)
#define XACTENGINE_E_READFILE                       XACTENGINEERROR(0x010)
#define XACTENGINE_E_UNKNOWNEVENT                   XACTENGINEERROR(0x011)
#define XACTENGINE_E_INCALLBACK                     XACTENGINEERROR(0x012)
#define XACTENGINE_E_NOWAVEBANK                     XACTENGINEERROR(0x013)
#define XACTENGINE_E_SELECTVARIATION                XACTENGINEERROR(0x014)
#define XACTENGINE_E_MULTIPLEAUDITIONENGINES        XACTENGINEERROR(0x015)
#define XACTENGINE_E_WAVEBANKNOTPREPARED            XACTENGINEERROR(0x016)
#define XACTENGINE_E_NORENDERER                     XACTENGINEERROR(0x017)
#define XACTENGINE_E_INVALIDENTRYCOUNT              XACTENGINEERROR(0x018)
#define XACTENGINE_E_SEEKTIMEBEYONDCUEEND           XACTENGINEERROR(0x019)
#define XACTENGINE_E_SEEKTIMEBEYONDWAVEEND          XACTENGINEERROR(0x019)
#define XACTENGINE_E_NOFRIENDLYNAMES                XACTENGINEERROR(0x01a)
#define XACTENGINE_E_AUDITION_WRITEFILE             XACTENGINEERROR(0x101)
#define XACTENGINE_E_AUDITION_NOSOUNDBANK           XACTENGINEERROR(0x102)
#define XACTENGINE_E_AUDITION_INVALIDRPCINDEX       XACTENGINEERROR(0x103)
#define XACTENGINE_E_AUDITION_MISSINGDATA           XACTENGINEERROR(0x104)
#define XACTENGINE_E_AUDITION_UNKNOWNCOMMAND        XACTENGINEERROR(0x105)
#define XACTENGINE_E_AUDITION_INVALIDDSPINDEX       XACTENGINEERROR(0x106)
#define XACTENGINE_E_AUDITION_MISSINGWAVE           XACTENGINEERROR(0x107)
#define XACTENGINE_E_AUDITION_CREATEDIRECTORYFAILED XACTENGINEERROR(0x108)
#define XACTENGINE_E_AUDITION_INVALIDSESSION        XACTENGINEERROR(0x109)

static const DWORD XACT_FLAG_STOP_RELEASE       = 0x00000000;
static const DWORD XACT_FLAG_STOP_IMMEDIATE     = 0x00000001;
static const DWORD XACT_FLAG_MANAGEDATA         = 0x00000001;
static const DWORD XACT_FLAG_BACKGROUND_MUSIC   = 0x00000002;
static const DWORD XACT_FLAG_UNITS_MS           = 0x00000004;
static const DWORD XACT_FLAG_UNITS_SAMPLES      = 0x00000008;
#define XACT_FLAG_CUE_STOP_RELEASE              XACT_FLAG_STOP_RELEASE
#define XACT_FLAG_CUE_STOP_IMMEDIATE            XACT_FLAG_STOP_IMMEDIATE
#define XACT_FLAG_GLOBAL_SETTINGS_MANAGEDATA    XACT_FLAG_MANAGEDATA
#define XACT_FLAG_SOUNDBANK_STOP_IMMEDIATE      XACT_FLAG_STOP_IMMEDIATE
#define XACT_FLAG_ENGINE_CREATE_MANAGEDATA      XACT_FLAG_MANAGEDATA
#define XACT_FLAG_ENGINE_STOP_IMMEDIATE         XACT_FLAG_STOP_IMMEDIATE

static const DWORD XACT_STATE_CREATED           = 0x00000001;
static const DWORD XACT_STATE_PREPARING         = 0x00000002;
static const DWORD XACT_STATE_PREPARED          = 0x00000004;
static const DWORD XACT_STATE_PLAYING           = 0x00000008;
static const DWORD XACT_STATE_STOPPING          = 0x00000010;
static const DWORD XACT_STATE_STOPPED           = 0x00000020;
static const DWORD XACT_STATE_PAUSED            = 0x00000040;
static const DWORD XACT_STATE_INUSE             = 0x00000080;
static const DWORD XACT_STATE_PREPAREFAILED     = 0x80000000;
#define XACT_CUESTATE_CREATED               XACT_STATE_CREATED
#define XACT_CUESTATE_PREPARING             XACT_STATE_PREPARING
#define XACT_CUESTATE_PREPARED              XACT_STATE_PREPARED
#define XACT_CUESTATE_PLAYING               XACT_STATE_PLAYING
#define XACT_CUESTATE_STOPPING              XACT_STATE_STOPPING
#define XACT_CUESTATE_STOPPED               XACT_STATE_STOPPED
#define XACT_CUESTATE_PAUSED                XACT_STATE_PAUSED
#define XACT_SOUNDBANKSTATE_INUSE           XACT_STATE_INUSE
#define XACT_WAVEBANKSTATE_INUSE            XACT_STATE_INUSE
#define XACT_WAVEBANKSTATE_PREPARED         XACT_STATE_PREPARED
#define XACT_WAVEBANKSTATE_PREPAREFAILED    XACT_STATE_PREPAREFAILED

#define XACT_RENDERER_ID_LENGTH    0xff
#define XACT_RENDERER_NAME_LENGTH  0xff
#define XACT_CUE_NAME_LENGTH       0xff

#define XACT_ENGINE_LOOKAHEAD_DEFAULT 250

static const DWORD XACT_FLAG_API_AUDITION_MODE = 0x00000001;
static const DWORD XACT_FLAG_API_DEBUG_MODE    = 0x00000002;

#define XACT_DEBUGENGINE_REGISTRY_KEY   TEXT("Software\\Microsoft\\XACT")
#define XACT_DEBUGENGINE_REGISTRY_VALUE TEXT("DebugEngine")

typedef struct XACT_RENDERER_DETAILS
{
    WCHAR rendererID[XACT_RENDERER_ID_LENGTH];
    WCHAR displayName[XACT_RENDERER_NAME_LENGTH];
    BOOL defaultDevice;
} XACT_RENDERER_DETAILS, *LPXACT_RENDERER_DETAILS;

typedef BOOL (__stdcall *XACT_READFILE_CALLBACK)(HANDLE file, void *buffer, DWORD size, DWORD *ret_size, OVERLAPPED *overlapped);
typedef BOOL (__stdcall *XACT_GETOVERLAPPEDRESULT_CALLBACK)(HANDLE file, OVERLAPPED *overlapped, DWORD *ret_size, BOOL wait);

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
typedef const XACT_STREAMING_PARAMETERS *LPCXACT_STREAMING_PARAMETERS, *LPCXACT_WAVEBANK_STREAMING_PARAMETERS;

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
    XACTCHANNELMAPENTRY *paEntries;
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
    XACTCHANNELVOLUMEENTRY *paEntries;
} XACTCHANNELVOLUME, *LPXACTCHANNELVOLUME;
typedef const XACTCHANNELVOLUME *LPCXACTCHANNELVOLUME;

static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPREPARED                      = 1;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPLAY                          = 2;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUESTOP                          = 3;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEDESTROYED                     = 4;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MARKER                           = 5;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED               = 6;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED                = 7;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED             = 8;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED            = 9;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUICONNECTED                     = 10;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUIDISCONNECTED                  = 11;
#if XACT3_VER >= 0x0205
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPREPARED                     = 12;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPLAY                         = 13;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVESTOP                         = 14;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVELOOPED                       = 15;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEDESTROYED                    = 16;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKPREPARED                 = 17;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT = 18;
#else
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPLAY                         = 12;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVESTOP                         = 13;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKPREPARED                 = 14;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT = 15;
#endif

static const BYTE XACT_FLAG_NOTIFICATION_PERSIST = 0x01;

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE 0xfffe
#endif

#pragma pack(push,1)

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct _WAVEFORMATEX
{
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
#endif

#ifndef _WAVEFORMATEXTENSIBLE_
#define _WAVEFORMATEXTENSIBLE_
typedef struct
{
    WAVEFORMATEX Format;
    union
    {
        WORD wValidBitsPerSample;
        WORD wSamplesPerBlock;
        WORD wReserved;
    } Samples;
    DWORD dwChannelMask;
    GUID SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#endif

typedef struct XACT_NOTIFICATION_CUE
{
    XACTINDEX cueIndex;
    IXACTSoundBank *pSoundBank;
    IXACTCue *pCue;
} XACT_NOTIFICATION_CUE, *LPXACT_NOTIFICATION_CUE;
typedef const XACT_NOTIFICATION_CUE *LPCXACT_NOTIFICATION_CUE;

typedef struct XACT_NOTIFICATION_DESCRIPTION
{
    XACTNOTIFICATIONTYPE type;
    BYTE flags;
    IXACTSoundBank *pSoundBank;
    IXACTWaveBank *pWaveBank;
    IXACTCue *pCue;
#if XACT3_VER >= 0x0205
    IXACTWave *pWave;
#endif
    XACTINDEX cueIndex;
#if XACT3_VER >= 0x0205
    XACTINDEX waveIndex;
#endif
    void *pvContext;
} XACT_NOTIFICATION_DESCRIPTION, *LPXACT_NOTIFICATION_DESCRIPTION;
typedef const XACT_NOTIFICATION_DESCRIPTION *LPCXACT_NOTIFICATION_DESCRIPTION;

typedef struct XACT_NOTIFICATION_GUI
{
    DWORD reserved;
} XACT_NOTIFICATION_GUI, *LPXACT_NOTIFICATION_GUI;
typedef const XACT_NOTIFICATION_GUI *LPCXACT_NOTIFICATION_GUI;

typedef struct XACT_NOTIFICATION_MARKER
{
    XACTINDEX cueIndex;
    IXACTSoundBank *pSoundBank;
    IXACTCue *pCue;
    DWORD marker;
} XACT_NOTIFICATION_MARKER, *LPXACT_NOTIFICATION_MARKER;
typedef const XACT_NOTIFICATION_MARKER *LPCXACT_NOTIFICATION_MARKER;

typedef struct XACT_NOTIFICATION_SOUNDBANK
{
    IXACTSoundBank *pSoundBank;
} XACT_NOTIFICATION_SOUNDBANK, *LPXACT_NOTIFICATION_SOUNDBANK;
typedef const XACT_NOTIFICATION_SOUNDBANK *LPCXACT_NOTIFICATION_SOUNDBANK;

typedef struct XACT_NOTIFICATION_VARIABLE
{
    XACTINDEX cueIndex;
    IXACTSoundBank *pSoundBank;
    IXACTCue *pCue;
    XACTVARIABLEINDEX variableIndex;
    XACTVARIABLEVALUE variableValue;
    BOOL local;
} XACT_NOTIFICATION_VARIABLE, *LPXACT_NOTIFICATION_VARIABLE;
typedef const XACT_NOTIFICATION_VARIABLE *LPCXACT_NOTIFICATION_VARIABLE;

typedef struct XACT_NOTIFICATION_WAVE
{
    IXACTWaveBank *pWaveBank;
    XACTINDEX waveIndex;
    XACTINDEX cueIndex;
    IXACTSoundBank *pSoundBank;
    IXACTCue *pCue;
#if XACT3_VER >= 0x0205
    IXACTWave *pWave;
#endif
} XACT_NOTIFICATION_WAVE, *LPXACT_NOTIFICATION_WAVE;
typedef const XACT_NOTIFICATION_WAVE *LPCXACT_NOTIFICATION_NAME;

typedef struct XACT_NOTIFICATION_WAVEBANK
{
    IXACTWaveBank *pWaveBank;
} XACT_NOTIFICATION_WAVEBANK, *LPXACT_NOTIFICATION_WAVEBANK;
typedef const XACT_NOTIFICATION_WAVEBANK *LPCXACT_NOTIFICATION_WAVEBANK;

typedef struct XACT_NOTIFICATION
{
    XACTNOTIFICATIONTYPE type;
    LONG timeStamp;
    void *pvContext;
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

typedef void (__stdcall *XACT_NOTIFICATION_CALLBACK)(const XACT_NOTIFICATION *notification);

typedef struct XACT_RUNTIME_PARAMETERS
{
    DWORD lookAheadTime;
    void *pGlobalSettingsBuffer;
    DWORD globalSettingsBufferSize;
    DWORD globalSettingsFlags;
    DWORD globalSettingsAllocAttributes;
    XACT_FILEIO_CALLBACKS fileIOCallbacks;
    XACT_NOTIFICATION_CALLBACK fnNotificationCallback;
    const WCHAR *pRendererID;
} XACT_RUNTIME_PARAMETERS, *LPXACT_RUNTIME_PARAMETERS;
typedef const XACT_RUNTIME_PARAMETERS *LPCXACT_RUNTIME_PARAMETERS;

#define INTERFACE IXACTCue
DECLARE_INTERFACE(IXACTCue)
{
    STDMETHOD(Play)(THIS) PURE;
    STDMETHOD(Stop)(THIS_ DWORD flags) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *state) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetChannelMap)(THIS_ XACTCHANNELMAP *map, DWORD buffer_size, DWORD *needed_size) PURE;
    STDMETHOD(SetChannelMap)(THIS_ XACTCHANNELMAP *map) PURE;
    STDMETHOD(GetChannelVolume)(THIS_ XACTCHANNELVOLUME *volume) PURE;
    STDMETHOD(SetChannelVolume)(THIS_ XACTCHANNELVOLUME *volume) PURE;
    STDMETHOD(SetMatrixCoefficients)(THIS_ UINT32 src_count, UINT32 dst_count, float *coefficients) PURE;
    STDMETHOD_(XACTVARIABLEINDEX,GetVariableIndex)(THIS_ const char *name) PURE;
    STDMETHOD(SetVariable)(THIS_ XACTVARIABLEINDEX index, XACTVARIABLEVALUE value) PURE;
    STDMETHOD(GetVariable)(THIS_ XACTVARIABLEINDEX index, XACTVARIABLEVALUE *value) PURE;
    STDMETHOD(Pause)(THIS_ BOOL pause) PURE;
#if XACT3_VER >= 0x0205
    STDMETHOD(GetProperties)(THIS_ XACT_CUE_INSTANCE_PROPERTIES **properties) PURE;
#endif
};
#undef INTERFACE

#ifndef __cplusplus
#define IXACTCue_Play(p)                        (p)->lpVtbl->Destroy(p)
#define IXACTCue_Stop(p,a)                      (p)->lpVtbl->Stop(p,a)
#define IXACTCue_GetState(p,a)                  (p)->lpVtbl->GetState(p,a)
#define IXACTCue_Destroy(p)                     (p)->lpVtbl->Destroy(p)
#define IXACTCue_GetChannelMap(p,a,b,c)         (p)->lpVtbl->GetChannelMap(p,a,b,c)
#define IXACTCue_SetChannelMap(p,a)             (p)->lpVtbl->SetChannelMap(p,a)
#define IXACTCue_GetChannelVolume(p,a)          (p)->lpVtbl->GetChannelVolume(p,a)
#define IXACTCue_SetChannelVolume(p,a)          (p)->lpVtbl->SetChannelVolume(p,a)
#define IXACTCue_SetMatrixCoefficients(p,a,b,c) (p)->lpVtbl->SetMatrixCoefficients(p,a,b,c)
#define IXACTCue_GetVariableIndex(p,a)          (p)->lpVtbl->GetVariableIndex(p,a)
#define IXACTCue_SetVariable(p,a,b)             (p)->lpVtbl->SetVariable(p,a,b)
#define IXACTCue_GetVariable(p,a,b)             (p)->lpVtbl->GetVariable(p,a,b)
#define IXACTCue_Pause(p,a)                     (p)->lpVtbl->Pause(p,a)
#if XACT3_VER >= 0x0205
#define IXACTCue_GetProperties(p,a)             (p)->lpVtbl->GetProperties(p,a)
#endif
#else
#define IXACTCue_Play(p)                        (p)->Destroy()
#define IXACTCue_Stop(p,a)                      (p)->Stop(a)
#define IXACTCue_GetState(p,a)                  (p)->Stop(a)
#define IXACTCue_Destroy(p)                     (p)->Destroy()
#define IXACTCue_GetChannelMap(p,a,b,c)         (p)->GetChannelMap(a,b,c)
#define IXACTCue_SetChannelMap(p,a)             (p)->SetChannelMap(a)
#define IXACTCue_GetChannelVolume(p,a)          (p)->GetChannelVolume(a)
#define IXACTCue_SetChannelVolume(p,a)          (p)->SetChannelVolume(a)
#define IXACTCue_SetMatrixCoefficients(p,a,b,c) (p)->SetMatrixCoefficients(a,b,c)
#define IXACTCue_GetVariableIndex(p,a)          (p)->GetVariableIndex(a)
#define IXACTCue_SetVariable(p,a,b)             (p)->SetVariable(a,b)
#define IXACTCue_GetVariable(p,a,b)             (p)->GetVariable(a,b)
#define IXACTCue_Pause(p,a)                     (p)->Pause(a)
#if XACT3_VER >= 0x0205
#define IXACTCue_GetProperties(p,a)             (p)->GetProperties(a)
#endif
#endif

#define INTERFACE IXACTWave
DECLARE_INTERFACE(IXACTWave)
{
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(Play)(THIS) PURE;
    STDMETHOD(Stop)(THIS_ DWORD flags) PURE;
    STDMETHOD(Pause)(THIS_ BOOL pause) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *state) PURE;
    STDMETHOD(SetPitch)(THIS_ XACTPITCH pitch) PURE;
    STDMETHOD(SetVolume)(THIS_ XACTVOLUME volume) PURE;
    STDMETHOD(SetMatrixCoefficients)(THIS_ UINT32 src_count, UINT32 dst_count, float *coefficients) PURE;
    STDMETHOD(GetProperties)(THIS_ XACT_WAVE_INSTANCE_PROPERTIES *properties) PURE;
};
#undef INTERFACE

#ifndef __cplusplus
#define IXACTWave_Destroy(p)                        (p)->lpVtbl->Destroy(p)
#define IXACTWave_Play(p)                           (p)->lpVtbl->Destroy(p)
#define IXACTWave_Stop(p,a)                         (p)->lpVtbl->Stop(p,a)
#define IXACTWave_Pause(p,a)                        (p)->lpVtbl->Pause(p,a)
#define IXACTWave_GetState(p,a)                     (p)->lpVtbl->GetState(p,a)
#define IXACTWave_SetPitch(p,a)                     (p)->lpVtbl->SetPitch(p,a)
#define IXACTWave_SetVolume(p,a)                    (p)->lpVtbl->SetVolume(p,a)
#define IXACTWave_SetMatrixCoefficients(p,a,b,c)    (p)->lpVtbl->SetMatrixCoefficients(p,a,b,c)
#define IXACTWave_GetProperties(p,a)                (p)->lpVtbl->GetProperties(p,a)
#else
#define IXACTWave_Destroy(p)                        (p)->Destroy()
#define IXACTWave_Play(p)                           (p)->Destroy()
#define IXACTWave_Stop(p,a)                         (p)->Stop(a)
#define IXACTWave_Pause(p,a)                        (p)->Pause(a)
#define IXACTWave_GetState(p,a)                     (p)->Stop(a)
#define IXACTWave_SetPitch(p,a)                     (p)->SetVariable(a)
#define IXACTWave_SetVolume(p,a)                    (p)->SetVolume(a)
#define IXACTWave_SetMatrixCoefficients(p,a,b,c)    (p)->SetMatrixCoefficients(a,b,c)
#define IXACTWave_GetProperties(p,a)                (p)->GetProperties(a)
#endif

#define INTERFACE IXACTSoundBank
DECLARE_INTERFACE(IXACTSoundBank)
{
    STDMETHOD_(XACTINDEX,GetCueIndex)(THIS_ const char *name) PURE;
#if XACT3_VER >= 0x0205
    STDMETHOD(GetNumCues)(THIS_ XACTINDEX *count) PURE;
    STDMETHOD(GetCueProperties)(THIS_ XACTINDEX index, XACT_CUE_PROPERTIES *properties) PURE;
#endif
    STDMETHOD(Prepare)(THIS_ XACTINDEX index, DWORD flags, XACTTIME offset, IXACTCue **cue) PURE;
    STDMETHOD(Play)(THIS_ XACTINDEX index, DWORD flags, XACTTIME offset, IXACTCue **cue) PURE;
    STDMETHOD(Stop)(THIS_ XACTINDEX index, DWORD flags) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetState)(THIS_ DWORD *state) PURE;
};
#undef INTERFACE

#ifndef __cplusplus
#define IXACTSoundBank_GetCueIndex(p,a)         (p)->lpVtbl->GetCueIndex(p,a)
#if XACT3_VER >= 0x0205
#define IXACTSoundBank_GetNumCues(p,a)          (p)->lpVtbl->GetNumCues(p,a)
#define IXACTSoundBank_GetCueProperties(p,a,b)  (p)->lpVtbl->GetCueProperties(p,a,b)
#endif
#define IXACTSoundBank_Prepare(p,a,b,c,d)       (p)->lpVtbl->Prepare(p,a,b,c,d)
#define IXACTSoundBank_Play(p,a,b,c,d)          (p)->lpVtbl->Play(p,a,b,c,d)
#define IXACTSoundBank_Stop(p,a,b)              (p)->lpVtbl->Stop(p,a,b)
#define IXACTSoundBank_Destroy(p)               (p)->lpVtbl->Destroy(p)
#define IXACTSoundBank_GetState(p,a)            (p)->lpVtbl->GetState(p,a)
#else
#define IXACTSoundBank_GetCueIndex(p,a)         (p)->GetCueIndex(a)
#if XACT3_VER >= 0x0205
#define IXACTSoundBank_GetNumCues(p,a)          (p)->GetNumCues(a)
#define IXACTSoundBank_GetCueProperties(p,a,b)  (p)->GetCueProperties(a,b)
#endif
#define IXACTSoundBank_Prepare(p,a,b,c,d)       (p)->Prepare(a,b,c,d)
#define IXACTSoundBank_Play(p,a,b,c,d)          (p)->Play(a,b,c,d)
#define IXACTSoundBank_Stop(p,a,b)              (p)->Stop(a,b)
#define IXACTSoundBank_Destroy(p)               (p)->Destroy()
#define IXACTSoundBank_GetState(p,a)            (p)->GetState(a)
#endif

#define INTERFACE IXACTWaveBank
DECLARE_INTERFACE(IXACTWaveBank)
{
    STDMETHOD(Destroy)(THIS) PURE;
#if XACT3_VER >= 0x0205
    STDMETHOD(GetNumWaves)(THIS_ XACTINDEX *count) PURE;
    STDMETHOD_(XACTINDEX,GetWaveIndex)(THIS_ const char *name) PURE;
    STDMETHOD(GetWaveProperties)(THIS_ XACTINDEX index, LPXACT_WAVE_PROPERTIES pWaveProperties) PURE;
    STDMETHOD(Prepare)(THIS_ XACTINDEX index, DWORD flags, DWORD offset, XACTLOOPCOUNT loop_count, IXACTWave **wave) PURE;
    STDMETHOD(Play)(THIS_ XACTINDEX index, DWORD flags, DWORD offset, XACTLOOPCOUNT loop_count, IXACTWave **wave) PURE;
    STDMETHOD(Stop)(THIS_ XACTINDEX index, DWORD flags) PURE;
#endif
    STDMETHOD(GetState)(THIS_ DWORD *state) PURE;
};
#undef INTERFACE

#ifndef __cplusplus
#define IXACTWaveBank_Destroy(p)                (p)->lpVtbl->Destroy(p)
#if XACT3_VER >= 0x0205
#define IXACTWaveBank_GetNumWaves(p,a)          (p)->lpVtbl->GetNumCues(p,a)
#define IXACTWaveBank_GetWaveIndex(p,a)         (p)->lpVtbl->GetWaveIndex(p,a)
#define IXACTWaveBank_GetWaveProperties(p,a,b)  (p)->lpVtbl->GetWaveProperties(p,a,b)
#define IXACTWaveBank_Prepare(p,a,b,c,d,e)      (p)->lpVtbl->Prepare(p,a,b,c,d,e)
#define IXACTWaveBank_Play(p,a,b,c,d,e)         (p)->lpVtbl->Play(p,a,b,c,d,e)
#define IXACTWaveBank_Stop(p,a,b)               (p)->lpVtbl->Stop(p,a,b)
#endif
#define IXACTWaveBank_GetState(p,a)             (p)->lpVtbl->GetState(p,a)
#else
#define IXACTWaveBank_Destroy(p)                (p)->Destroy()
#if XACT3_VER >= 0x0205
#define IXACTWaveBank_GetNumWaves(p,a)          (p)->GetNumWaves(a)
#define IXACTWaveBank_GetWaveIndex(p,a)         (p)->GetWaveIndex(a)
#define IXACTWaveBank_GetWaveProperties(p,a,b)  (p)->GetWaveProperties(a,b)
#define IXACTWaveBank_Prepare(p,a,b,c,d,e)      (p)->Prepare(a,b,c,d,e)
#define IXACTWaveBank_Play(p,a,b,c,d,e)         (p)->Play(a,b,c,d,e)
#define IXACTWaveBank_Stop(p,a,b)               (p)->Stop(a,b)
#endif
#define IXACTWaveBank_GetState(p,a)             (p)->GetState(a)
#endif

#define INTERFACE IXACTEngine
DECLARE_INTERFACE_(IXACTEngine,IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, void **out) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetRendererCount)(THIS_ XACTINDEX *count) PURE;
    STDMETHOD(GetRendererDetails)(THIS_ XACTINDEX index, XACT_RENDERER_DETAILS *details) PURE;
#if XACT3_VER >= 0x0205
    STDMETHOD(GetFinalMixFormat)(THIS_ WAVEFORMATEXTENSIBLE *format) PURE;
#endif
    STDMETHOD(Initialize)(THIS_ const XACT_RUNTIME_PARAMETERS *params) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
    STDMETHOD(DoWork)(THIS) PURE;
    STDMETHOD(CreateSoundBank)(THIS_ const void *buffer, DWORD size, DWORD flags, DWORD alloc_flags, IXACTSoundBank **bank) PURE;
    STDMETHOD(CreateInMemoryWaveBank)(THIS_ const void *buffer, DWORD size, DWORD flags, DWORD alloc_flags, IXACTWaveBank **bank) PURE;
    STDMETHOD(CreateStreamingWaveBank)(THIS_ const XACT_WAVEBANK_STREAMING_PARAMETERS *params, IXACTWaveBank **bank) PURE;
#if XACT3_VER >= 0x0205
    STDMETHOD(PrepareWave)(THIS_ DWORD flags, const char *path, WORD packet_size, DWORD alignment, DWORD offset, XACTLOOPCOUNT loop_count, IXACTWave **wave) PURE;
    STDMETHOD(PrepareInMemoryWave)(THIS_ DWORD flags, WAVEBANKENTRY entry, DWORD *seek_table, BYTE *data, DWORD offset, XACTLOOPCOUNT loop_count, IXACTWave **wave) PURE;
    STDMETHOD(PrepareStreamingWave)(THIS_ DWORD flags, WAVEBANKENTRY entry, XACT_STREAMING_PARAMETERS params, DWORD alignment, DWORD *seek_table, DWORD offset, XACTLOOPCOUNT loop_count, IXACTWave **wave) PURE;
#endif
    STDMETHOD(RegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION *desc) PURE;
    STDMETHOD(UnRegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION *desc) PURE;
    STDMETHOD_(XACTCATEGORY,GetCategory)(THIS_ const char *name) PURE;
    STDMETHOD(Stop)(THIS_ XACTCATEGORY category, DWORD flags) PURE;
    STDMETHOD(SetVolume)(THIS_ XACTCATEGORY category, XACTVOLUME volume) PURE;
    STDMETHOD(Pause)(THIS_ XACTCATEGORY category, BOOL pause) PURE;
    STDMETHOD_(XACTVARIABLEINDEX,GetGlobalVariableIndex)(THIS_ const char *name) PURE;
    STDMETHOD(SetGlobalVariable)(THIS_ XACTVARIABLEINDEX index, XACTVARIABLEVALUE value) PURE;
    STDMETHOD(GetGlobalVariable)(THIS_ XACTVARIABLEINDEX index, XACTVARIABLEVALUE *value) PURE;
};
#undef INTERFACE

#ifndef __cplusplus
#define IXACTEngine_QueryInterface(p,a,b)                   (p)->lpVtbl->QueryInterface(p,a,b)
#define IXACTEngine_AddRef(p)                               (p)->lpVtbl->AddRef(p)
#define IXACTEngine_Release(p)                              (p)->lpVtbl->Release(p)
#define IXACTEngine_GetRendererCount(p,a)                   (p)->lpVtbl->GetRendererCount(p,a)
#define IXACTEngine_GetRendererDetails(p,a,b)               (p)->lpVtbl->GetRendererDetails(p,a,b)
#if XACT3_VER >= 0x0205
#define IXACTEngine_GetFinalMixFormat(p,a)                  (p)->lpVtbl->GetFinalMixFormat(p,a)
#endif
#define IXACTEngine_Initialize(p,a)                         (p)->lpVtbl->Initialize(p,a)
#define IXACTEngine_Shutdown(p)                             (p)->lpVtbl->Shutdown(p)
#define IXACTEngine_DoWork(p)                               (p)->lpVtbl->DoWork(p)
#define IXACTEngine_CreateSoundBank(p,a,b,c,d,e)            (p)->lpVtbl->CreateSoundBank(p,a,b,c,d,e)
#define IXACTEngine_CreateInMemoryWaveBank(p,a,b,c,d,e)     (p)->lpVtbl->CreateInMemoryWaveBank(p,a,b,c,d,e)
#define IXACTEngine_CreateStreamingWaveBank(p,a,b)          (p)->lpVtbl->CreateStreamingWaveBank(p,a,b)
#if XACT3_VER >= 0x0205
#define IXACTEngine_PrepareWave(p,a,b,c,d,e,f,g)            (p)->lpVtbl->PrepareWave(p,a,b,c,d,e,f,g)
#define IXACTEngine_PrepareInMemoryWave(p,a,b,c,d,e,f,g)    (p)->lpVtbl->PrepareInMemoryWave(p,a,b,c,d,e,f,g)
#define IXACTEngine_PrepareStreamingWave(p,a,b,c,d,e,f,g,h) (p)->lpVtbl->PrepareStreamingWave(p,a,b,c,d,e,f,g,h)
#endif
#define IXACTEngine_RegisterNotification(p,a)               (p)->lpVtbl->RegisterNotification(p,a)
#define IXACTEngine_UnRegisterNotification(p,a)             (p)->lpVtbl->UnRegisterNotification(p,a)
#define IXACTEngine_GetCategory(p,a)                        (p)->lpVtbl->GetCategory(p,a)
#define IXACTEngine_Stop(p,a,b)                             (p)->lpVtbl->Stop(a,b)
#define IXACTEngine_SetVolume(p,a,b)                        (p)->lpVtbl->SetVolume(p,a,b)
#define IXACTEngine_Pause(p,a,b)                            (p)->lpVtbl->Pause(a,b)
#define IXACTEngine_GetGlobalVariableIndex(p,a)             (p)->lpVtbl->GetGlobalVariableIndex(p,a)
#define IXACTEngine_SetGlobalVariable(p,a,b)                (p)->lpVtbl->SetGlobalVariable(p,a,b)
#define IXACTEngine_GetGlobalVariable(p,a,b)                (p)->lpVtbl->GetGlobalVariable(p,a,b)
#else
#define IXACTEngine_QueryInterface(p,a,b)                   (p)->QueryInterface(a,b)
#define IXACTEngine_AddRef(p)                               (p)->AddRef()
#define IXACTEngine_Release(p)                              (p)->Release()
#define IXACTEngine_GetRendererCount(p,a)                   (p)->GetRendererCount(a)
#define IXACTEngine_GetRendererDetails(p,a,b)               (p)->GetRendererDetails(a,b)
#if XACT3_VER >= 0x0205
#define IXACTEngine_GetFinalMixFormat(p,a)                  (p)->GetFinalMixFormat(a)
#endif
#define IXACTEngine_Initialize(p,a)                         (p)->Initialize(a)
#define IXACTEngine_Shutdown(p)                             (p)->Shutdown()
#define IXACTEngine_DoWork(p)                               (p)->DoWork()
#define IXACTEngine_CreateSoundBank(p,a,b,c,d,e)            (p)->CreateSoundBank(a,b,c,d,e)
#define IXACTEngine_CreateInMemoryWaveBank(p,a,b,c,d,e)     (p)->CreateInMemoryWaveBank(a,b,c,d,e)
#define IXACTEngine_CreateStreamingWaveBank(p,a,b)          (p)->CreateStreamingWaveBank(a,b)
#if XACT3_VER >= 0x0205
#define IXACTEngine_PrepareWave(p,a,b,c,d,e,f,g)            (p)->PrepareWave(a,b,c,d,e,f,g)
#define IXACTEngine_PrepareInMemoryWave(p,a,b,c,d,e,f,g)    (p)->PrepareInMemoryWave(a,b,c,d,e,f,g)
#define IXACTEngine_PrepareStreamingWave(p,a,b,c,d,e,f,g,h) (p)->PrepareStreamingWave(a,b,c,d,e,f,g,h)
#endif
#define IXACTEngine_RegisterNotification(p,a)               (p)->RegisterNotification(a)
#define IXACTEngine_UnRegisterNotification(p,a)             (p)->UnRegisterNotification(a)
#define IXACTEngine_GetCategory(p,a)                        (p)->GetCategory(a)
#define IXACTEngine_Stop(p,a,b)                             (p)->Stop(a,b)
#define IXACTEngine_SetVolume(p,a,b)                        (p)->SetVolume(a,b)
#define IXACTEngine_Pause(p,a,b)                            (p)->Pause(a,b)
#define IXACTEngine_GetGlobalVariableIndex(p,a)             (p)->GetGlobalVariableIndex(a)
#define IXACTEngine_SetGlobalVariable(p,a,b)                (p)->SetGlobalVariable(a,b)
#define IXACTEngine_GetGlobalVariable(p,a,b)                (p)->GetGlobalVariable(a,b)
#endif

#endif /* _XACT_H_ */
