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

/*  A simple structure to tie together a pointer to an icc profile, an lcms
 *  color profile handle and a Windows file handle. If the profile is memory
 *  based the file handle field is set to INVALID_HANDLE_VALUE. The 'access'
 *  field records the access parameter supplied to an OpenColorProfile()
 *  call, i.e. PROFILE_READ or PROFILE_READWRITE.
 */

#include <lcms2.h>

enum object_type
{
    OBJECT_TYPE_PROFILE,
    OBJECT_TYPE_TRANSFORM,
};

struct object
{
    enum object_type type;
    LONG             refs;
    void           (*close)( struct object * );
};

struct profile
{
    struct object hdr;
    HANDLE        file;
    DWORD         access;
    char         *data;
    DWORD         size;
    cmsHPROFILE   cmsprofile;
};

struct transform
{
    struct object hdr;
    cmsHTRANSFORM cmstransform;
};

extern HANDLE alloc_handle( struct object *obj );
extern void free_handle( HANDLE );

struct object *grab_object( HANDLE, enum object_type );
void release_object( struct object * );

struct tag_entry
{
    DWORD sig;
    DWORD offset;
    DWORD size;
};

extern DWORD get_tag_count( const struct profile * );
extern BOOL get_tag_entry( const struct profile *, DWORD, struct tag_entry * );
extern BOOL get_adjusted_tag( const struct profile *, TAGTYPE, struct tag_entry * );
extern BOOL get_tag_data( const struct profile *, TAGTYPE, DWORD, void *, DWORD *, BOOL * );
extern BOOL set_tag_data( const struct profile *, TAGTYPE, DWORD, const void *, DWORD * );
extern void get_profile_header( const struct profile *, PROFILEHEADER * );
extern void set_profile_header( const struct profile *, const PROFILEHEADER * );
