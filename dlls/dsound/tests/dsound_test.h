/*
 * Unit tests for dsound functions
 *
 * Copyright (c) 2004 Francois Gouget
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

static const unsigned int formats[][3]={
    { 8000,  8, 1},
    { 8000,  8, 2},
    { 8000, 16, 1},
    { 8000, 16, 2},
    {11025,  8, 1},
    {11025,  8, 2},
    {11025, 16, 1},
    {11025, 16, 2},
    {22050,  8, 1},
    {22050,  8, 2},
    {22050, 16, 1},
    {22050, 16, 2},
    {44100,  8, 1},
    {44100,  8, 2},
    {44100, 16, 1},
    {44100, 16, 2},
    {48000,  8, 1},
    {48000,  8, 2},
    {48000, 16, 1},
    {48000, 16, 2},
    {96000,  8, 1},
    {96000,  8, 2},
    {96000, 16, 1},
    {96000, 16, 2}
};
#define NB_FORMATS (sizeof(formats)/sizeof(*formats))

/* The time slice determines how often we will service the buffer */
#define TIME_SLICE    100
#define BUFFER_LEN    (4*TIME_SLICE)


extern char* wave_generate_la(WAVEFORMATEX*,double,DWORD*);
extern HWND get_hwnd();
extern void init_format(WAVEFORMATEX*,int,int,int,int);
extern void test_buffer(LPDIRECTSOUND,LPDIRECTSOUNDBUFFER,
                        BOOL,BOOL,LONG,BOOL,LONG,BOOL,double,BOOL,
                        LPDIRECTSOUND3DLISTENER,BOOL,BOOL);
