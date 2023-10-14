/*
 * Copyright 2020 Derek Lesho
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

#ifndef __CODECAPI_H
#define __CODECAPI_H

enum eAVEncH264VProfile
{
    eAVEncH264VProfile_unknown  = 0,
    eAVEncH264VProfile_Simple = 66,
    eAVEncH264VProfile_Base = 66,
    eAVEncH264VProfile_Main = 77,
    eAVEncH264VProfile_High = 100,
    eAVEncH264VProfile_422 = 122,
    eAVEncH264VProfile_High10 = 110,
    eAVEncH264VProfile_444 = 244,
    eAVEncH264VProfile_Extended = 88,
    eAVEncH264VProfile_ScalableBase =  83,
    eAVEncH264VProfile_ScalableHigh =  86,
    eAVEncH264VProfile_MultiviewHigh = 118,
    eAVEncH264VProfile_StereoHigh = 128,
    eAVEncH264VProfile_ConstrainedBase = 256,
    eAVEncH264VProfile_UCConstrainedHigh = 257,
    eAVEncH264VProfile_UCScalableConstrainedBase = 258,
    eAVEncH264VProfile_UCScalableConstrainedHigh = 259
};

enum eAVEncH264VLevel
{
    eAVEncH264VLevel1 = 10,
    eAVEncH264VLevel1_b = 11,
    eAVEncH264VLevel1_1 = 11,
    eAVEncH264VLevel1_2 = 12,
    eAVEncH264VLevel1_3 = 13,
    eAVEncH264VLevel2 = 20,
    eAVEncH264VLevel2_1 = 21,
    eAVEncH264VLevel2_2 = 22,
    eAVEncH264VLevel3 = 30,
    eAVEncH264VLevel3_1 = 31,
    eAVEncH264VLevel3_2 = 32,
    eAVEncH264VLevel4 = 40,
    eAVEncH264VLevel4_1 = 41,
    eAVEncH264VLevel4_2 = 42,
    eAVEncH264VLevel5 = 50,
    eAVEncH264VLevel5_1 = 51,
    eAVEncH264VLevel5_2 = 52
};

DEFINE_GUID(AVDecVideoAcceleration_H264, 0xf7db8a2f, 0x4f48, 0x4ee8, 0xae, 0x31, 0x8b, 0x6e, 0xbe, 0x55, 0x8a, 0xe2);

#endif /* __CODECAPI_H */
