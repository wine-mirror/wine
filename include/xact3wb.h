/*
 * Copyright (c) 2020 Alistair Leslie-Hughes
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
} WAVEBANKMINIWAVEFORMAT;

typedef struct WAVEBANKREGION
{
    DWORD dwOffset;
    DWORD dwLength;
} WAVEBANKREGION;

typedef struct WAVEBANKSAMPLEREGION
{
    DWORD dwStartSample;
    DWORD dwTotalSamples;
} WAVEBANKSAMPLEREGION;

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
    } DUMMYUNIONNAME;

    WAVEBANKMINIWAVEFORMAT Format;
    WAVEBANKREGION PlayRegion;
    WAVEBANKSAMPLEREGION LoopRegion;
} WAVEBANKENTRY;

#endif /* __XACT3WB_H__ */
