/*
 * Copyright 2004 (C) Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_ICM_H
#define __WINE_ICM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE HPROFILE;
typedef HPROFILE *PHPROFILE;
typedef HANDLE HTRANSFORM;

typedef DWORD TAGTYPE, *PTAGTYPE, *LPTAGTYPE;

typedef enum {
    BM_x555RGB     = 0x00,
    BM_565RGB      = 0x01,
    BM_RGBTRIPLETS = 0x02,
    BM_BGRTRIPLETS = 0x04,
    BM_xRGBQUADS   = 0x08,
    BM_10b_RGB     = 0x09,
    BM_16b_RGB     = 0x0a,
    BM_xBGRQUADS   = 0x10,
    BM_CMYKQUADS   = 0x20,
    BM_x555XYZ     = 0x101,
    BM_x555Yxz,
    BM_x555Lab,
    BM_x555G3CH,
    BM_XYZTRIPLETS = 0x201,
    BM_YxyTRIPLETS,
    BM_LabTRIPLETS,
    BM_G3CHTRIPLETS,
    BM_5CHANNEL,
    BM_6CHANNEL,
    BM_7CHANNEL,
    BM_8CHANNEL,
    BM_GRAY,
    BM_xXYZQUADS   = 0x301,
    BM_xYxyQUADS,
    BM_xLabQUADS,
    BM_xG3CHQUADS,
    BM_KYMCQUADS,
    BM_10b_XYZ     = 0x401,
    BM_10b_Yxy,
    BM_10b_Lab,
    BM_10b_G3CH,
    BM_NAMED_INDEX,
    BM_16b_XYZ     = 0x501,
    BM_16b_Yxy,
    BM_16b_Lab,
    BM_16b_G3CH,
    BM_16b_GRAY,
} BMFORMAT, *PBMFORMAT, *LPBMFORMAT;

typedef BOOL (CALLBACK *PBMCALLBACKFN)(ULONG,ULONG,LPARAM);
typedef PBMCALLBACKFN LPPBMCALLBACKFN;

typedef struct tagPROFILEHEADER {
    DWORD phSize;
    DWORD phCMMType;
    DWORD phVersion;
    DWORD phClass;
    DWORD phDataColorSpace;
    DWORD phConnectionSpace;
    DWORD phDateTime[3];
    DWORD phSignature;
    DWORD phPlatform;
    DWORD phProfileFlags;
    DWORD phManufacturer;
    DWORD phModel;
    DWORD phAttributes;
    DWORD phRenderingIntent;
    CIEXYZ phIlluminant;
    DWORD phCreator;
    BYTE phReserved[44];
} PROFILEHEADER, *PPROFILEHEADER, *LPPROFILEHEADER;

typedef struct tagPROFILE {
    DWORD dwType;
    PVOID pProfileData;
    DWORD cbDataSize;
} PROFILE, *PPROFILE, *LPPROFILE;

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ICM_H */
