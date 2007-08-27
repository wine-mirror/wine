/*
 * Copyright 2001 Andreas Mohr
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

#ifndef __SETUPAPI_PRIVATE_H
#define __SETUPAPI_PRIVATE_H

#define COPYFILEDLGORD	1000
#define SOURCESTRORD	500
#define DESTSTRORD	501
#define PROGRESSORD	502


#define REG_INSTALLEDFILES "System\\CurrentControlSet\\Control\\InstalledFiles"
#define REGPART_RENAME "\\Rename"
#define REG_VERSIONCONFLICT "Software\\Microsoft\\VersionConflictManager"

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

/* string substitutions */

struct inf_file;
extern const WCHAR *DIRID_get_string( int dirid );
extern unsigned int PARSER_string_substA( const struct inf_file *file, const WCHAR *text,
                                          char *buffer, unsigned int size );
extern unsigned int PARSER_string_substW( const struct inf_file *file, const WCHAR *text,
                                          WCHAR *buffer, unsigned int size );
extern const WCHAR *PARSER_get_inf_filename( HINF hinf );
extern WCHAR *PARSER_get_src_root( HINF hinf );
extern WCHAR *PARSER_get_dest_dir( INFCONTEXT *context );

/* support for Ascii queue callback functions */

struct callback_WtoA_context
{
    void               *orig_context;
    PSP_FILE_CALLBACK_A orig_handler;
};

UINT CALLBACK QUEUE_callback_WtoA( void *context, UINT notification, UINT_PTR, UINT_PTR );

/* from msvcrt/sys/stat.h */
#define _S_IWRITE 0x0080
#define _S_IREAD  0x0100

extern OSVERSIONINFOW OsVersionInfo;

extern BOOL create_fake_dll( const WCHAR *name, const WCHAR *source );

#endif /* __SETUPAPI_PRIVATE_H */
