/*
 * Copyright (c) 2020 Alistair Leslie-Hughes
 * Copyright (c) 2020 Vijay Kiran Kamuju
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
#ifndef __XACT3WB_H__
#define __XACT3WB_H__

#pragma pack(push,1)

typedef DWORD WAVEBANKOFFSET;

#define WAVEBANK_HEADER_SIGNATURE 0x444e4257 /* DNBW */
#define WAVEBANK_HEADER_VERSION   44
#define WAVEBANK_BANKNAME_LENGTH  64
#define WAVEBANK_ENTRYNAME_LENGTH 64

#define WAVEBANK_MAX_DATA_SEGMENT_SIZE     0xffffffff
#define WAVEBANK_COMPACT_DATA_SEGMENT_SIZE 0x001fffff

#define WAVEBANK_TYPE_BUFFER    0x00000000
#define WAVEBANK_TYPE_STREAMING 0x00000001
#define WAVEBANK_TYPE_MASK      0x00000001

#define WAVEBANK_FLAGS_ENTRYNAMES    0x00010000
#define WAVEBANK_FLAGS_COMPACT       0x00020000
#define WAVEBANK_FLAGS_SYNC_DISABLED 0x00040000
#define WAVEBANK_FLAGS_SEEKTABLES    0x00080000
#define WAVEBANK_FLAGS_MASK          0x000F0000

#define WAVEBANK_DVD_SECTOR_SIZE 2048
#define WAVEBANK_DVD_BLOCK_SIZE  (WAVEBANK_DVD_SECTOR_SIZE * 16)
#define WAVEBANK_ALIGNMENT_MIN   4
#define WAVEBANK_ALIGNMENT_DVD   WAVEBANK_DVD_SECTOR_SIZE

typedef enum WAVEBANKSEGIDX
{
    WAVEBANK_SEGIDX_BANKDATA = 0,
    WAVEBANK_SEGIDX_ENTRYMETADATA,
    WAVEBANK_SEGIDX_SEEKTABLES,
    WAVEBANK_SEGIDX_ENTRYNAMES,
    WAVEBANK_SEGIDX_ENTRYWAVEDATA,
    WAVEBANK_SEGIDX_COUNT
} WAVEBANKSEGIDX, *LPWAVEBANKSEGIDX;
typedef const WAVEBANKSEGIDX *LPCWAVEBANKSEGIDX;

#define WAVEBANKMINIFORMAT_TAG_PCM   0x0
#define WAVEBANKMINIFORMAT_TAG_XMA   0x1
#define WAVEBANKMINIFORMAT_TAG_ADPCM 0x2
#define WAVEBANKMINIFORMAT_TAG_WMA   0x3

#define WAVEBANKMINIFORMAT_BITDEPTH_8  0x0
#define WAVEBANKMINIFORMAT_BITDEPTH_16 0x1

typedef union WAVEBANKMINIWAVEFORMAT
{
    struct
    {
        DWORD wFormatTag     :  2;
        DWORD nChannels      :  3;
        DWORD nSamplesPerSec : 18;
        DWORD wBlockAlign    :  8;
        DWORD wBitsPerSample :  1;
    } DUMMYSTRUCTNAME;
    DWORD dwValue;
} WAVEBANKMINIWAVEFORMAT, *LPWAVEBANKMINIWAVEFORMAT;
typedef const WAVEBANKMINIWAVEFORMAT *LPCWAVEBANKMINIWAVEFORMAT;

typedef struct WAVEBANKREGION
{
    DWORD dwOffset;
    DWORD dwLength;
} WAVEBANKREGION, *LPWAVEBANKREGION;
typedef const WAVEBANKREGION *LPCWAVEBANKREGION;

typedef struct WAVEBANKSAMPLEREGION
{
    DWORD dwStartSample;
    DWORD dwTotalSamples;
} WAVEBANKSAMPLEREGION, *LPWAVEBANKSAMPLEREGION;
typedef const WAVEBANKSAMPLEREGION *LPCWAVEBANKSAMPLEREGION;

typedef struct WAVEBANKHEADER
{
    DWORD dwSignature;
    DWORD dwVersion;
    DWORD dwHeaderVersion;
    WAVEBANKREGION Segments[WAVEBANK_SEGIDX_COUNT];
} WAVEBANKHEADER, *LPWAVEBANKHEADER;
typedef const WAVEBANKHEADER *LPCWAVEBANKHEADER;

#define WAVEBANKENTRY_XMASTREAMS_MAX  3
#define WAVEBANKENTRY_XMACHANNELS_MAX 6

#define WAVEBANKENTRY_FLAGS_READAHEAD         0x00000001
#define WAVEBANKENTRY_FLAGS_LOOPCACHE         0x00000002
#define WAVEBANKENTRY_FLAGS_REMOVELOOPTAIL    0x00000004
#define WAVEBANKENTRY_FLAGS_IGNORELOOP        0x00000008
#define WAVEBANKENTRY_FLAGS_MASK              0x00000008

typedef struct WAVEBANKENTRY
{
    union
    {
        struct
        {
            DWORD dwFlags  :  4;
            DWORD Duration : 28;
        } DUMMYSTRUCTNAME;
        DWORD dwFlagsAndDuration;
    };

    WAVEBANKMINIWAVEFORMAT Format;
    WAVEBANKREGION PlayRegion;
    WAVEBANKSAMPLEREGION LoopRegion;
} WAVEBANKENTRY, *LPWAVEBANKENTRY;
typedef const WAVEBANKENTRY *LPCWAVEBANKENTRY;

typedef struct WAVEBANKENTRYCOMPACT
{
    DWORD dwOffset          : 21;
    DWORD dwLengthDeviation : 11;
} WAVEBANKENTRYCOMPACT, *LPWAVEBANKENTRYCOMPACT;
typedef const WAVEBANKENTRYCOMPACT *LPCWAVEBANKENTRYCOMPACT;

typedef struct WAVEBANKDATA
{
    DWORD dwFlags;
    DWORD dwEntryCount;
    CHAR  szBankName[WAVEBANK_BANKNAME_LENGTH];
    DWORD dwEntryMetaDataElementSize;
    DWORD dwEntryNameElementSize;
    DWORD dwAlignment;
    WAVEBANKMINIWAVEFORMAT CompactFormat;
    FILETIME BuildTime;
} WAVEBANKDATA, *LPWAVEBANKDATA;
typedef const WAVEBANKDATA *LPCWAVEBANKDATA;

#pragma pack(pop)

#endif /* __XACT3WB_H__ */
