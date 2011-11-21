/*
 * Copyright (c) 2009 Andrew Nguyen
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

#ifndef __WINE_T2EMBAPI_H
#define __WINE_T2EMBAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Possible return values. */
#define E_NONE                              __MSABI_LONG(0x0000)
#define E_API_NOTIMPL                       __MSABI_LONG(0x0001)

typedef ULONG (WINAPIV * READEMBEDPROC)(void*,void*,ULONG);
typedef ULONG (WINAPIV * WRITEEMBEDPROC)(void*,void*,ULONG);

typedef struct
{
    unsigned short usStructSize;
    unsigned short usRefStrSize;
    unsigned short *pusRefStr;
} TTLOADINFO;

typedef struct
{
    unsigned short usStructSize;
    unsigned short usRootStrSize;
    unsigned short *pusRootStr;
} TTEMBEDINFO;

LONG WINAPI TTLoadEmbeddedFont(HANDLE*,ULONG,ULONG*,ULONG,ULONG*,READEMBEDPROC,
                               LPVOID,LPWSTR,LPSTR,TTLOADINFO*);

/* embedding privileges */
#define EMBED_PREVIEWPRINT  1
#define EMBED_EDITABLE      2
#define EMBED_INSTALLABLE   3
#define EMBED_NOEMBEDDING   4

LONG WINAPI TTGetEmbeddingType(HDC, ULONG*);

#ifdef __cplusplus
}
#endif

#endif
