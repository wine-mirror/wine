/*
 * Copyright 2020 Ziqing Hui
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

#define COBJMACROS

#include "windef.h"
#include "wincodec.h"
#include "wine/test.h"

/* 4x4 compressed(DXT1) DDS image */
static BYTE test_dds_image[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7
};

/* 4x4 uncompressed(BGR565) DDS image */
static BYTE test_dds_bgr565[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00,
    0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x4C, 0x7B, 0x08, 0x69, 0xF5, 0xA7, 0xF5, 0xA7, 0xF5, 0xA7, 0x4C, 0x7B,
    0x4C, 0x7B, 0x4C, 0x7B, 0x4C, 0x7B, 0xB1, 0x95, 0x4C, 0x7B, 0x08, 0x69, 0x08, 0x69, 0x4C, 0x7B
};

/* 4x4 compressed(DXT1) DDS image with mip maps, mipMapCount=3 */
static BYTE test_dds_mipmaps[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0xB1, 0x95, 0x6D, 0x7B, 0xFC, 0x55, 0x5D, 0x5D,
    0x2E, 0x8C, 0x4E, 0x7C, 0xAA, 0xAB, 0xAB, 0xAB
};

/* 4x4 compressed(DXT1) cube map */
static BYTE test_dds_cube[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00
};

/* 4x4 compressed(DXT1) volume texture, depth=4, mipMapCount=3 */
static BYTE test_dds_volume[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x8A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xD5, 0xA7, 0x2C, 0x7B, 0xE0, 0x00, 0x55, 0x55, 0xD5, 0xA7, 0x49, 0x69, 0x57, 0x00, 0xFF, 0x55,
    0xD5, 0xA7, 0x48, 0x69, 0xFD, 0x80, 0xFF, 0x55, 0x30, 0x8D, 0x89, 0x71, 0x55, 0xA8, 0x00, 0xFF,
    0x32, 0x96, 0x6D, 0x83, 0xA8, 0x55, 0x5D, 0x5D, 0x0E, 0x84, 0x6D, 0x7B, 0xA8, 0xA9, 0xAD, 0xAD,
    0x2E, 0x8C, 0x2E, 0x7C, 0xAA, 0xAB, 0xAB, 0xAB
};

/* 4x4 compressed(DXT1) texture array, arraySize=3, mipMapCount=3 */
static BYTE test_dds_array[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69,
    0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00
};

/* 12x12 compressed(DXT3) texture array, arraySize=2, mipMapCount=4 */
static BYTE test_dds_dxt3[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x0C, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4A, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB5, 0xA7, 0xAD, 0x83,
    0x60, 0x60, 0xE0, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0x6B, 0x72,
    0xD5, 0xD5, 0xAF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x8D, 0x89, 0x69,
    0x57, 0x5F, 0x5E, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB4, 0xA7, 0xAD, 0x83,
    0x00, 0xAA, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x9E, 0x6D, 0x83,
    0x00, 0x00, 0xAA, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0xCD, 0x83,
    0x5C, 0xF8, 0xAA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x7A, 0xC9, 0x71,
    0x80, 0x60, 0x60, 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4A, 0x72, 0xA8, 0x68,
    0x28, 0xBE, 0xD7, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x8C, 0xEA, 0x71,
    0x0B, 0xAB, 0xAD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x9F, 0xCC, 0x7A,
    0x5C, 0xA8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x8D, 0x49, 0x69,
    0x77, 0xEE, 0x88, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x7B, 0x08, 0x69,
    0xF8, 0x58, 0xF8, 0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4E, 0x84, 0x6B, 0x72,
    0x33, 0x99, 0x33, 0x99, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x14, 0x9F, 0x0A, 0x72,
    0xDC, 0xAA, 0x75, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB5, 0xA7, 0xAD, 0x83,
    0x60, 0x60, 0xE0, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0x6B, 0x72,
    0xD5, 0xD5, 0xAF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x8D, 0x89, 0x69,
    0x57, 0x5F, 0x5E, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB4, 0xA7, 0xAD, 0x83,
    0x00, 0xAA, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x9E, 0x6D, 0x83,
    0x00, 0x00, 0xAA, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0xCD, 0x83,
    0x5C, 0xF8, 0xAA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x7A, 0xC9, 0x71,
    0x80, 0x60, 0x60, 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4A, 0x72, 0xA8, 0x68,
    0x28, 0xBE, 0xD7, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x8C, 0xEA, 0x71,
    0x0B, 0xAB, 0xAD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x9F, 0xCC, 0x7A,
    0x5C, 0xA8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x8D, 0x49, 0x69,
    0x77, 0xEE, 0x88, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x7B, 0x08, 0x69,
    0xF8, 0x58, 0xF8, 0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4E, 0x84, 0x6B, 0x72,
    0x33, 0x99, 0x33, 0x99, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x14, 0x9F, 0x0A, 0x72,
    0xDC, 0xAA, 0x75, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00
};

static struct test_data {
    BYTE *data;
    UINT size;
    UINT expected_frame_count;
    UINT expected_bytes_per_block;
    const GUID *expected_pixel_format;
    WICDdsParameters expected_parameters;
    BOOL wine_init;
} test_data[] = {
    { test_dds_bgr565,   sizeof(test_dds_bgr565),  1, 2,  &GUID_WICPixelFormat32bppBGRA,
      { 4,  4,  1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,  WICDdsTexture2D, WICDdsAlphaModeUnknown }, TRUE },
    { test_dds_image,    sizeof(test_dds_image),   1, 8,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 1, 1, DXGI_FORMAT_BC1_UNORM,       WICDdsTexture2D, WICDdsAlphaModePremultiplied } },
    { test_dds_mipmaps,  sizeof(test_dds_mipmaps), 3, 8,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 3, 1, DXGI_FORMAT_BC1_UNORM,       WICDdsTexture2D, WICDdsAlphaModePremultiplied } },
    { test_dds_volume,   sizeof(test_dds_volume),  7, 8,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  4, 3, 1, DXGI_FORMAT_BC1_UNORM,       WICDdsTexture3D, WICDdsAlphaModePremultiplied } },
    { test_dds_array,    sizeof(test_dds_array),   9, 8,  &GUID_WICPixelFormat32bppBGRA,
      { 4,  4,  1, 3, 3, DXGI_FORMAT_BC1_UNORM,       WICDdsTexture2D, WICDdsAlphaModeUnknown } },
    { test_dds_dxt3,     sizeof(test_dds_dxt3),    8, 16, &GUID_WICPixelFormat32bppBGRA,
      { 12, 12, 1, 4, 2, DXGI_FORMAT_BC2_UNORM,       WICDdsTexture2D, WICDdsAlphaModeUnknown } },
};

static IWICImagingFactory *factory = NULL;

static IWICStream *create_stream(const void *image_data, UINT image_size)
{
    HRESULT hr;
    IWICStream *stream = NULL;

    hr = IWICImagingFactory_CreateStream(factory, &stream);
    ok(hr == S_OK, "CreateStream failed, hr %#x\n", hr);
    if (hr != S_OK) goto fail;

    hr = IWICStream_InitializeFromMemory(stream, (BYTE *)image_data, image_size);
    ok(hr == S_OK, "InitializeFromMemory failed, hr %#x\n", hr);
    if (hr != S_OK) goto fail;

    return stream;

fail:
    if (stream) IWICStream_Release(stream);
    return NULL;
}

static IWICBitmapDecoder *create_decoder(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    GUID guidresult;

    hr = CoCreateInstance(&CLSID_WICDdsDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    if (hr != S_OK) {
        win_skip("Dds decoder is not supported\n");
        return NULL;
    }

    memset(&guidresult, 0, sizeof(guidresult));
    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
    ok(hr == S_OK, "GetContainerFormat failed, hr %#x\n", hr);
    ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatDds),
       "Got unexpected container format %s\n", debugstr_guid(&GUID_ContainerFormatDds));

    return decoder;
}

static HRESULT init_decoder(IWICBitmapDecoder *decoder, IWICStream *stream, HRESULT expected, int index, BOOL wine_init)
{
    HRESULT hr;
    IWICWineDecoder *wine_decoder;

    hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
    if (index == -1) {
        ok(hr == S_OK || wine_init, "Decoder initialize failed, hr %#x\n", hr);
    } else {
        ok(hr == expected, "Test %u: Expected hr %#x, got %#x\n", index, expected, hr);
    }

    if (hr != S_OK && wine_init) {
        hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICWineDecoder, (void **)&wine_decoder);
        if (index == -1) {
            ok(hr == S_OK || broken(hr != S_OK), "QueryInterface failed, hr %#x\n", hr);
        } else {
            ok(hr == S_OK || broken(hr != S_OK), "Test %u: QueryInterface failed, hr %#x\n", index, hr);
        }
        if (hr == S_OK) {
            hr = IWICWineDecoder_Initialize(wine_decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
            if (index == -1)  {
                ok(hr == S_OK, "Initialize failed, hr %#x\n", hr);
            } else {
                ok(hr == S_OK, "Test %u: Initialize failed, hr %#x\n", index, hr);
            }

        }
    }

    return hr;
}

static BOOL has_extended_header(const BYTE *data)
{
    return data[84] == 'D' && data[85] == 'X' && data[86] == '1' && data[87] == '0';
}

static void test_dds_decoder_initialize(void)
{
    static BYTE test_dds_bad_magic[sizeof(test_dds_image)];
    static BYTE test_dds_bad_header[sizeof(test_dds_image)];
    static BYTE byte = 0;
    static DWORD dword = 0;
    static BYTE qword1[8] = { 0 };
    static BYTE qword2[8] = "DDS ";

    static struct test_data {
        void *data;
        UINT size;
        HRESULT expected;
        BOOL wine_init;
    } test_data[] = {
        { test_dds_image,        sizeof(test_dds_image),        S_OK },
        { test_dds_mipmaps,      sizeof(test_dds_mipmaps),      S_OK },
        { test_dds_volume,       sizeof(test_dds_volume),       S_OK },
        { test_dds_array,        sizeof(test_dds_array),        S_OK },
        { test_dds_dxt3,         sizeof(test_dds_dxt3),         S_OK },
        { test_dds_bgr565,       sizeof(test_dds_bgr565),       WINCODEC_ERR_BADHEADER, TRUE },
        { test_dds_cube,         sizeof(test_dds_cube),         WINCODEC_ERR_BADHEADER, TRUE },
        { test_dds_bad_magic,  sizeof(test_dds_bad_magic),  WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { test_dds_bad_header, sizeof(test_dds_bad_header), WINCODEC_ERR_BADHEADER },
        { &byte,   sizeof(byte),   WINCODEC_ERR_STREAMREAD },
        { &dword,  sizeof(dword),  WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { &qword1, sizeof(qword1), WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { &qword2, sizeof(qword2), WINCODEC_ERR_STREAMREAD },
    };

    int i;

    memcpy(test_dds_bad_magic, test_dds_image, sizeof(test_dds_image));
    memcpy(test_dds_bad_header, test_dds_image, sizeof(test_dds_image));
    test_dds_bad_magic[0] = 0;
    test_dds_bad_header[4] = 0;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;

        decoder = create_decoder();
        if (!decoder) goto next;

        init_decoder(decoder, stream, test_data[i].expected, i, test_data[i].wine_init);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
    }
}

static void test_dds_decoder_global_properties(IWICBitmapDecoder *decoder)
{
    HRESULT hr;
    IWICPalette *palette = NULL;
    IWICMetadataQueryReader *metadata_reader = NULL;
    IWICBitmapSource *preview = NULL, *thumnail = NULL;
    IWICColorContext *color_context = NULL;
    UINT count;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette failed, hr %#x\n", hr);
    if (hr == S_OK) {
        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "CopyPalette got unexpected hr %#x\n", hr);
        hr = IWICBitmapDecoder_CopyPalette(decoder, NULL);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "CopyPalette got unexpected hr %#x\n", hr);
    }

    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader);
    todo_wine ok (hr == S_OK, "GetMetadataQueryReader got unexpected hr %#x\n", hr);
    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataQueryReader got unexpected hr %#x\n", hr);

    hr = IWICBitmapDecoder_GetPreview(decoder, &preview);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetPreview got unexpected hr %#x\n", hr);
    hr = IWICBitmapDecoder_GetPreview(decoder, NULL);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetPreview got unexpected hr %#x\n", hr);

    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, &color_context, &count);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetColorContexts got unexpected hr %#x\n", hr);
    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, NULL, NULL);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetColorContexts got unexpected hr %#x\n", hr);

    hr = IWICBitmapDecoder_GetThumbnail(decoder, &thumnail);
    ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "GetThumbnail got unexpected hr %#x\n", hr);
    hr = IWICBitmapDecoder_GetThumbnail(decoder, NULL);
    ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "GetThumbnail got unexpected hr %#x\n", hr);

    if (palette) IWICPalette_Release(palette);
    if (metadata_reader) IWICMetadataQueryReader_Release(metadata_reader);
    if (preview) IWICBitmapSource_Release(preview);
    if (color_context) IWICColorContext_Release(color_context);
    if (thumnail) IWICBitmapSource_Release(thumnail);
}

static void test_dds_decoder_image_parameters(void)
{
    int i;
    HRESULT hr;
    WICDdsParameters parameters;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        UINT frame_count;
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;
        IWICDdsDecoder *dds_decoder = NULL;

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;

        decoder = create_decoder();
        if (!decoder) goto next;

        hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICDdsDecoder, (void **)&dds_decoder);
        ok(hr == S_OK, "QueryInterface failed, hr %#x\n", hr);
        if (hr != S_OK) goto next;

        hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        ok(hr == WINCODEC_ERR_WRONGSTATE, "Test %u: GetFrameCount got unexpected hr %#x\n", i, hr);
        hr = IWICBitmapDecoder_GetFrameCount(decoder, NULL);
        ok(hr == E_INVALIDARG, "Test %u: GetFrameCount got unexpected hr %#x\n", i, hr);

        hr = IWICDdsDecoder_GetParameters(dds_decoder, &parameters);
        ok(hr == WINCODEC_ERR_WRONGSTATE, "Test %u: GetParameters got unexpected hr %#x\n", i, hr);
        hr = IWICDdsDecoder_GetParameters(dds_decoder, NULL);
        ok(hr == E_INVALIDARG, "Test %u: GetParameters got unexpected hr %#x\n", i, hr);

        hr = init_decoder(decoder, stream, S_OK, -1, test_data[i].wine_init);
        if (hr != S_OK) {
            win_skip("uncompressed DDS image is not supported\n");
            goto next;
        }

        hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        ok(hr == S_OK, "Test %u: GetFrameCount failed, hr %#x\n", i, hr);
        if (hr == S_OK) {
            ok(frame_count == test_data[i].expected_frame_count, "Test %u: Expected frame count %u, got %u\n",
               i, test_data[i].expected_frame_count, frame_count);
        }
        hr = IWICBitmapDecoder_GetFrameCount(decoder, NULL);
        ok(hr == E_INVALIDARG, "Test %u: GetParameters got unexpected hr %#x\n", i, hr);

        hr = IWICDdsDecoder_GetParameters(dds_decoder, &parameters);
        ok(hr == S_OK, "Test %u: GetParameters failed, hr %#x\n", i, hr);
        if (hr == S_OK) {
            ok(parameters.Width == test_data[i].expected_parameters.Width,
               "Test %u: Expected Width %u, got %u\n", i, test_data[i].expected_parameters.Width, parameters.Width);
            ok(parameters.Height == test_data[i].expected_parameters.Height,
               "Test %u: Expected Height %u, got %u\n", i, test_data[i].expected_parameters.Height, parameters.Height);
            ok(parameters.Depth == test_data[i].expected_parameters.Depth,
               "Test %u: Expected Depth %u, got %u\n", i, test_data[i].expected_parameters.Depth, parameters.Depth);
            ok(parameters.MipLevels == test_data[i].expected_parameters.MipLevels,
               "Test %u: Expected MipLevels %u, got %u\n", i, test_data[i].expected_parameters.MipLevels, parameters.MipLevels);
            ok(parameters.ArraySize == test_data[i].expected_parameters.ArraySize,
               "Test %u: Expected ArraySize %u, got %u\n", i, test_data[i].expected_parameters.ArraySize, parameters.ArraySize);
            ok(parameters.DxgiFormat == test_data[i].expected_parameters.DxgiFormat,
               "Test %u: Expected DxgiFormat %#x, got %#x\n", i, test_data[i].expected_parameters.DxgiFormat, parameters.DxgiFormat);
            ok(parameters.Dimension == test_data[i].expected_parameters.Dimension,
               "Test %u: Expected Dimension %#x, got %#x\n", i, test_data[i].expected_parameters.Dimension, parameters.Dimension);
            ok(parameters.AlphaMode == test_data[i].expected_parameters.AlphaMode,
               "Test %u: Expected AlphaMode %#x, got %#x\n", i, test_data[i].expected_parameters.AlphaMode, parameters.AlphaMode);
        }
        hr = IWICDdsDecoder_GetParameters(dds_decoder, NULL);
        ok(hr == E_INVALIDARG, "Test %u: GetParameters got unexpected hr %#x\n", i, hr);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
        if (dds_decoder) IWICDdsDecoder_Release(dds_decoder);

    }
}

static void test_dds_decoder_frame_properties(IWICBitmapFrameDecode *frame_decode, IWICDdsFrameDecode *dds_frame,
                                              UINT frame_count, WICDdsParameters *params, int i, int frame_index)
{
    HRESULT hr;
    UINT width, height ,expected_width, expected_height, slice_index, depth;
    UINT width_in_blocks, height_in_blocks, expected_width_in_blocks, expected_height_in_blocks;
    UINT expected_block_width, expected_block_height;
    WICDdsFormatInfo format_info;
    GUID pixel_format;

    /* frame size tests */

    hr = IWICBitmapFrameDecode_GetSize(frame_decode, NULL, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSize got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, NULL, &height);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSize got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, &width, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSize got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, &width, &height);
    ok(hr == S_OK, "Test %u, frame %u: GetSize failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;

    depth = params->Depth;
    expected_width = params->Width;
    expected_height = params->Height;
    slice_index = frame_index % (frame_count / params->ArraySize);
    while (slice_index >= depth)
    {
        if (expected_width > 1) expected_width /= 2;
        if (expected_height > 1) expected_height /= 2;
        slice_index -= depth;
        if (depth > 1) depth /= 2;
    }
    ok(width == expected_width, "Test %u, frame %u: Expected width %u, got %u\n", i, expected_width, frame_index, width);
    ok(height == expected_height, "Test %u, frame %u: Expected height %u, got %u\n", i, expected_height, frame_index, height);

    /* frame format information tests */

    if (test_data[i].expected_parameters.DxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) {
        expected_block_width = 1;
        expected_block_height = 1;
    } else {
        expected_block_width = 4;
        expected_block_height = 4;
    }

    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetFormatInfo got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, &format_info);
    ok(hr == S_OK, "Test %u, frame %u: GetFormatInfo failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;

    ok(format_info.DxgiFormat == test_data[i].expected_parameters.DxgiFormat,
       "Test %u, frame %u: Expected DXGI format %#x, got %#x\n",
       i, frame_index, test_data[i].expected_parameters.DxgiFormat, format_info.DxgiFormat);
    ok(format_info.BytesPerBlock == test_data[i].expected_bytes_per_block,
       "Test %u, frame %u: Expected bytes per block %u, got %u\n",
       i, frame_index, test_data[i].expected_bytes_per_block, format_info.BytesPerBlock);
    ok(format_info.BlockWidth == expected_block_width,
       "Test %u, frame %u: Expected block width %u, got %u\n",
       i, frame_index, expected_block_width, format_info.BlockWidth);
    ok(format_info.BlockHeight == expected_block_height,
       "Test %u, frame %u: Expected block height %u, got %u\n",
       i, frame_index, expected_block_height, format_info.BlockHeight);

    /* size in blocks tests */

    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, NULL, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSizeInBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, NULL, &height_in_blocks);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSizeInBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetSizeInBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, &height_in_blocks);
    ok(hr == S_OK, "Test %u, frame %u: GetSizeInBlocks failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;

    expected_width_in_blocks = (expected_width + expected_block_width - 1) / expected_block_width;
    expected_height_in_blocks = (expected_height + expected_block_height - 1) / expected_block_height;
    ok(width_in_blocks == expected_width_in_blocks,
       "Test %u, frame %u: Expected width in blocks %u, got %u\n", i, frame_index, expected_width_in_blocks, width_in_blocks);
    ok(height_in_blocks == expected_height_in_blocks,
       "Test %u, frame %u: Expected height in blocks %u, got %u\n", i, frame_index, expected_height_in_blocks, height_in_blocks);

    /* pixel format tests */

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame_decode, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: GetPixelFormat got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame_decode, &pixel_format);
    ok(hr == S_OK, "Test %u, frame %u: GetPixelFormat failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    ok(IsEqualGUID(&pixel_format, test_data[i].expected_pixel_format),
       "Test %u, frame %u: Expected pixel format %s, got %s\n",
       i, frame_index, debugstr_guid(test_data[i].expected_pixel_format), debugstr_guid(&pixel_format));
}

static void test_dds_decoder_frame_data(IWICDdsFrameDecode *dds_frame, UINT frame_count, WICDdsParameters *params,
                                        int i, int frame_index)
{
    HRESULT hr;
    WICDdsFormatInfo format_info;
    WICRect rect = { 0, 0, 1, 1 }, rect_test_a = { 0, 0, 0, 0 }, rect_test_b = { 0, 0, 0xdeadbeaf, 0xdeadbeaf };
    WICRect rect_test_c = { -0xdeadbeaf, -0xdeadbeaf, 1, 1 }, rect_test_d = { 0xdeadbeaf, 0xdeadbeaf, 1, 1 };
    BYTE buffer[256];
    UINT stride, frame_stride, frame_size, width_in_blocks, height_in_blocks;
    UINT width, height, depth, array_index;
    UINT block_offset;
    int slice_index;

    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, &format_info);
    ok(hr == S_OK, "Test %u, frame %u: GetFormatInfo failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, &height_in_blocks);
    ok(hr == S_OK, "Test %u, frame %u: GetSizeInBlocks failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    stride = rect.Width * format_info.BytesPerBlock;
    frame_stride = width_in_blocks * format_info.BytesPerBlock;
    frame_size = frame_stride * height_in_blocks;

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, 0, 0, NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_a, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_b, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_c, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_d, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, frame_stride * height_in_blocks - 1, buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, frame_stride * height_in_blocks, buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, 0, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, 0, buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, 1, buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, stride * rect.Height - 1, buffer);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, stride * rect.Height, buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, sizeof(buffer), NULL);
    ok(hr == E_INVALIDARG, "Test %u, frame %u: CopyBlocks got unexpected hr %#x\n", i, frame_index, hr);

    block_offset = 128; /* DDS magic and header */
    if (has_extended_header(test_data[i].data)) block_offset += 20; /* DDS extended header */
    width = params->Width;
    height = params->Height;
    depth = params->Depth;
    slice_index = frame_index % (frame_count / params->ArraySize);
    array_index = frame_index / (frame_count / params->ArraySize);
    block_offset += (test_data[i].size - block_offset) / params->ArraySize * array_index;
    while (slice_index >= 0)
    {
        width_in_blocks = (width + format_info.BlockWidth - 1) / format_info.BlockWidth;
        height_in_blocks = (width + format_info.BlockWidth - 1) / format_info.BlockWidth;
        block_offset += (slice_index >= depth) ?
                        (width_in_blocks * height_in_blocks * format_info.BytesPerBlock * depth) :
                        (width_in_blocks * height_in_blocks * format_info.BytesPerBlock * slice_index);
        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
        slice_index -= depth;
        if (depth > 1) depth /= 2;
    }

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    ok(!strncmp((const char *)test_data[i].data + block_offset, (const char *)buffer, format_info.BytesPerBlock),
       "Test %u, frame %u: Block data mismatch\n", i, frame_index);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    ok(!strncmp((const char *)test_data[i].data + block_offset, (const char *)buffer, frame_size),
       "Test %u, frame %u: Block data mismatch\n", i, frame_index);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "Test %u, frame %u: CopyBlocks failed, hr %#x\n", i, frame_index, hr);
    if (hr != S_OK) return;
    ok(!strncmp((const char *)test_data[i].data + block_offset, (const char *)buffer, frame_size),
       "Test %u, frame %u: Block data mismatch\n", i, frame_index);
}

static void test_dds_decoder_frame(IWICBitmapDecoder *decoder, int i)
{
    HRESULT hr;
    IWICDdsDecoder *dds_decoder = NULL;
    UINT frame_count, j;
    WICDdsParameters params;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "Test %u: GetFrameCount failed, hr %#x\n", i, hr);
    if (hr != S_OK) return;
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICDdsDecoder, (void **)&dds_decoder);
    ok(hr == S_OK, "Test %u: QueryInterface failed, hr %#x\n", i, hr);
    if (hr != S_OK) goto end;
    hr = IWICDdsDecoder_GetParameters(dds_decoder, &params);
    ok(hr == S_OK, "Test %u: GetParameters failed, hr %#x\n", i, hr);
    if (hr != S_OK) goto end;

    for (j = 0; j < frame_count; j++)
    {
        IWICBitmapFrameDecode *frame_decode = NULL;
        IWICDdsFrameDecode *dds_frame = NULL;

        hr = IWICBitmapDecoder_GetFrame(decoder, j, &frame_decode);
        ok(hr == S_OK, "Test %u, frame %u: GetFrame failed, hr %#x\n", i, j, hr);
        if (hr != S_OK) goto next;
        hr = IWICBitmapFrameDecode_QueryInterface(frame_decode, &IID_IWICDdsFrameDecode, (void **)&dds_frame);
        ok(hr == S_OK, "Test %u, frame %u: QueryInterface failed, hr %#x\n", i, j, hr);
        if (hr != S_OK) goto next;

        test_dds_decoder_frame_properties(frame_decode, dds_frame, frame_count, &params, i, j);
        test_dds_decoder_frame_data(dds_frame, frame_count, &params, i, j);

    next:
        if (frame_decode) IWICBitmapFrameDecode_Release(frame_decode);
        if (dds_frame) IWICDdsFrameDecode_Release(dds_frame);
    }

end:
    if (dds_decoder) IWICDdsDecoder_Release(dds_decoder);
}

static void test_dds_decoder(void)
{
    int i;
    HRESULT hr;

    test_dds_decoder_initialize();
    test_dds_decoder_image_parameters();

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;
        decoder = create_decoder();
        if (!decoder) goto next;
        hr = init_decoder(decoder, stream, S_OK, -1, test_data[i].wine_init);
        if (hr != S_OK) {
            win_skip("uncompressed DDS image is not supported\n");
            goto next;
        }

        test_dds_decoder_global_properties(decoder);
        test_dds_decoder_frame(decoder, i);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
    }
}

START_TEST(ddsformat)
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr %#x\n", hr);
    if (hr != S_OK) goto end;

    test_dds_decoder();

end:
    if (factory) IWICImagingFactory_Release(factory);
    CoUninitialize();
}
