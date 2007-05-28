/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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

#ifdef HAVE_LCMS

/*  These basic Windows types are defined in lcms.h when compiling on
 *  a non-Windows platform (why?), so they would normally not conflict
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

/*  Funny thing is lcms.h defines DWORD as an 'unsigned long' whereas Wine
 *  defines it as an 'unsigned int'. To avoid compiler warnings we use a
 *  preprocessor define for DWORD and LPDWORD to get back Wine's original
 *  (typedef) definitions.
 */

#undef BOOL
#undef DWORD
#undef LPDWORD

#define BOOL    BOOL
#define DWORD   DWORD
#define LPDWORD LPDWORD

extern DWORD MSCMS_hprofile2access( HPROFILE );
extern HPROFILE MSCMS_handle2hprofile( HANDLE file );
extern HPROFILE MSCMS_cmsprofile2hprofile( cmsHPROFILE cmsprofile );
extern HPROFILE MSCMS_iccprofile2hprofile( const icProfile *iccprofile );
extern HANDLE MSCMS_hprofile2handle( HPROFILE profile );
extern cmsHPROFILE MSCMS_hprofile2cmsprofile( HPROFILE profile );
extern icProfile *MSCMS_hprofile2iccprofile( HPROFILE profile );

extern HPROFILE MSCMS_create_hprofile_handle( HANDLE file, icProfile *iccprofile,
                                              cmsHPROFILE cmsprofile, DWORD access );
extern void MSCMS_destroy_hprofile_handle( HPROFILE profile );

extern cmsHTRANSFORM MSCMS_htransform2cmstransform( HTRANSFORM transform );
extern HTRANSFORM MSCMS_create_htransform_handle( cmsHTRANSFORM cmstransform );
extern void MSCMS_destroy_htransform_handle( HTRANSFORM transform );

extern DWORD MSCMS_get_tag_count( const icProfile *iccprofile );
extern void MSCMS_get_tag_by_index( icProfile *iccprofile, DWORD index, icTag *tag );
extern void MSCMS_get_tag_data( const icProfile *iccprofile, const icTag *tag, DWORD offset, void *buffer );
extern void MSCMS_set_tag_data( icProfile *iccprofile, const icTag *tag, DWORD offset, const void *buffer );
extern void MSCMS_get_profile_header( const icProfile *iccprofile, PROFILEHEADER *header );
extern void MSCMS_set_profile_header( icProfile *iccprofile, const PROFILEHEADER *header );
extern DWORD MSCMS_get_profile_size( const icProfile *iccprofile );

#endif /* HAVE_LCMS */
