/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004 Hans Leidekker
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

#ifdef HAVE_LCMS_LCMS_H
#define HAVE_LCMS_H 1
#endif

#ifdef HAVE_LCMS_H

/*  These basic Windows types are defined in lcms.h when compiling on
 *  a non-Windows platforms (why?), so they would normally not conflict
 *  with anything included earlier. But since we are building Wine they
 *  most certainly will have been defined before we include lcms.h.
 *  The preprocessor comes to the rescue.
 */

#define BYTE    LCMS_BYTE
#define LPBYTE  LCMS_LPBYTE
#define WORD    LCMS_WORD
#define LPWORD  LCMS_LPWORD
#define DWORD   LCMS_DWORD
#define LPDWORD LCMS_LPDWORD
#define BOOL    LCMS_BOOL
#define LPSTR   LCMS_LPSTR
#define LPVOID  LCMS_LPVOID

#undef cdecl
#undef FAR

#undef ZeroMemory
#undef CopyMemory

#undef LOWORD
#undef MAX_PATH

#ifdef HAVE_LCMS_LCMS_H
#include <lcms/lcms.h>
#else
#include <lcms.h>
#endif

/*  Funny thing is lcms.h defines DWORD as an 'unsigned int' whereas Wine
 *  defines it as an 'unsigned long'. To avoid compiler warnings we use a
 *  preprocessor define for DWORD and LPDWORD to get back Wine's orginal
 *  (typedef) definitions.
 */

#undef DWORD
#undef LPDWORD

#define DWORD   DWORD
#define LPDWORD LPDWORD

extern HPROFILE MSCMS_cmsprofile2hprofile( cmsHPROFILE cmsprofile );
extern cmsHPROFILE MSCMS_hprofile2cmsprofile( HPROFILE profile );
extern HANDLE MSCMS_hprofile2handle( HPROFILE profile );

extern HPROFILE MSCMS_create_hprofile_handle( HANDLE file, cmsHPROFILE cmsprofile );
extern void MSCMS_destroy_hprofile_handle( HPROFILE profile );

#endif /* HAVE_LCMS_H */
