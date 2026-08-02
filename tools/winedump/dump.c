/*
 *	File dumping utility
 *
 * 	Copyright 2001,2007 Eric Pouech
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../tools.h"
#include "windef.h"
#include "winbase.h"
#include "winedump.h"

void *dump_base = NULL;
size_t dump_total_len = 0;

void dump_data_offset( const unsigned char *ptr, unsigned int size, unsigned int offset, const char *prefix )
{
    unsigned int i, j;

    printf( "%s%08x: ", prefix, offset );
    if (!ptr)
    {
        printf("NULL\n");
        return;
    }
    for (i = 0; i < size; i++)
    {
        printf( "%02x%c", ptr[i], (i % 16 == 7) ? '-' : ' ' );
        if ((i % 16) == 15)
        {
            printf( " " );
            for (j = 0; j < 16; j++)
                printf( "%c", isprint(ptr[i-15+j]) ? ptr[i-15+j] : '.' );
            if (i < size-1) printf( "\n%s%08x: ", prefix, offset + i + 1 );
        }
    }
    if (i % 16)
    {
        printf( "%*s ", 3 * (16-(i%16)), "" );
        for (j = 0; j < i % 16; j++)
            printf( "%c", isprint(ptr[i-(i%16)+j]) ? ptr[i-(i%16)+j] : '.' );
    }
    printf( "\n" );
}

void dump_data( const unsigned char *ptr, unsigned int size, const char *prefix )
{
    dump_data_offset( ptr, size, 0, prefix );
}

static char* dump_want_n(unsigned sz)
{
    static char         buffer[64 * 1024];
    static unsigned     idx;
    char*               ret;

    assert(sz < sizeof(buffer));
    if (idx + sz >= sizeof(buffer)) idx = 0;
    ret = &buffer[idx];
    idx += sz;
    return ret;
}

const char *get_time_str(unsigned long _t)
{
    const time_t    t = (const time_t)_t;
    const char      *str = ctime(&t);
    size_t          len;
    char*           buf;

    if (!str) return "not valid time";

    len = strlen(str);
    /* FIXME: I don't get the same values from MS' pedump running under Wine...
     * I wonder if Wine isn't broken wrt to GMT settings...
     */
    if (len && str[len-1] == '\n') len--;
    buf = dump_want_n(len + 1);
    if (buf)
    {
        memcpy( buf, str, len );
        buf[len] = 0;
    }
    return buf;
}

unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

void dump_unicode_str( const WCHAR *str, int len )
{
    if (len == -1) len = strlenW( str );
    printf( "L\"");
    while (len-- > 0 && *str)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': printf( "\\n" ); break;
        case '\r': printf( "\\r" ); break;
        case '\t': printf( "\\t" ); break;
        case '"':  printf( "\\\"" ); break;
        case '\\': printf( "\\\\" ); break;
        default:
            if (c >= ' ' && c <= 126) putchar(c);
            else printf( "\\u%04x",c);
        }
    }
    printf( "\"" );
}

const char *get_hexint64_str( DWORD64 l )
{
    char *buf = dump_want_n(2 + 16 + 1);
    if (sizeof(l) > sizeof(unsigned long) && l >> 32)
        sprintf(buf, "%#lx%08lx", (unsigned long)(l >> 32), (unsigned long)l);
    else
        sprintf(buf, "%#lx", (unsigned long)l);
    assert(strlen(buf) <= 18);
    return buf;
}

const char *get_uint64_str( DWORD64 l )
{
    char *buf = dump_want_n( 32 );
    char *ptr = buf + 31;
    *ptr = '\0';
    for ( ; l; l /= 10)
        *--ptr = '0' + (l % 10);
    if (ptr == buf + 31) *--ptr = '0';
    assert(ptr >= buf);
    return ptr;
}

const char* get_symbol_str(const char* symname)
{
    const char* ret = NULL;

    if (!symname) return "(nil)";
    if (globals.do_demangle) ret = demangle( symname );
    return ret ? ret : symname;
}

const char* get_guid_str(const GUID* guid)
{
    char* str;

    str = dump_want_n(39);
    if (str)
        sprintf(str, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                (unsigned int)guid->Data1, guid->Data2, guid->Data3,
                guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return str;
}

const char *get_unicode_str( const WCHAR *str, int len )
{
    char *buffer;
    int i = 0;

    if (!str) return "(null)";
    if (len == -1) len = strlenW( str );
    buffer = dump_want_n( len * 6 + 3);
    buffer[i++] = '"';
    while (len-- > 0 && *str)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': strcpy( buffer + i, "\\n" );  i += 2; break;
        case '\r': strcpy( buffer + i, "\\r" );  i += 2; break;
        case '\t': strcpy( buffer + i, "\\t" );  i += 2; break;
        case '"':  strcpy( buffer + i, "\\\"" ); i += 2; break;
        case '\\': strcpy( buffer + i, "\\\\" ); i += 2; break;
        default:
            if (c >= ' ' && c <= 126) buffer[i++] = c;
            else i += sprintf( buffer + i, "\\u%04x",c);
        }
    }
    buffer[i++] = '"';
    buffer[i] = 0;
    return buffer;
}

const void*	PRD(unsigned long prd, unsigned long len)
{
    return (prd + len > dump_total_len) ? NULL : (const char*)dump_base + prd;
}

unsigned long Offset(const void* ptr)
{
    if (ptr < dump_base) {printf("<<<<<ptr below\n");return 0;}
    if ((const char *)ptr >= (const char*)dump_base + dump_total_len) {printf("<<<<<ptr above\n");return 0;}
    return (const char *)ptr - (const char *)dump_base;
}

static const struct dumper
{
    enum FileSig        kind;
    enum FileSig        (*get_kind)(void);
    file_dumper         dumper; /* default dump tool */
    enum FileSig        (*alt_get_kind)( int fd );
    void                (*alt_dumper)( int fd );
}
dumpers[] =
{
    {SIG_DOS,           get_kind_exec,  dos_dump},
    {SIG_PE,            get_kind_exec,  pe_dump},
    {SIG_DBG,           get_kind_dbg,   dbg_dump},
    {SIG_PDB,           .alt_get_kind = get_kind_pdb,   .alt_dumper = pdb_dump},
    {SIG_NE,            get_kind_exec,  ne_dump},
    {SIG_LE,            get_kind_exec,  le_dump},
    {SIG_COFFLIB,       get_kind_lib,   lib_dump},
    {SIG_MDMP,          get_kind_mdmp,  mdmp_dump},
    {SIG_LNK,           get_kind_lnk,   lnk_dump},
    {SIG_EMF,           get_kind_emf,   emf_dump},
    {SIG_EMFSPOOL,      get_kind_emfspool, emfspool_dump},
    {SIG_MF,            get_kind_mf,    mf_dump},
    {SIG_FNT,           get_kind_fnt,   fnt_dump},
    {SIG_TLB,           get_kind_tlb,   tlb_dump},
    {SIG_NLS,           get_kind_nls,   nls_dump},
    {SIG_REG,           get_kind_reg,   reg_dump},
    {SIG_UNKNOWN,       NULL,           NULL} /* sentinel */
};

BOOL dump_analysis(const char *name, file_dumper fn, enum FileSig wanted_sig)
{
    BOOL                ret = TRUE;
    const struct dumper*dpr;
    int                 fd;
    struct stat         st;

    setbuf(stdout, NULL);

    if ((fd = open( name, O_RDONLY | O_BINARY )) == -1) fatal( "Cannot read file" );
    fstat( fd, &st );
    printf("Contents of %s: %llu bytes\n\n", name, (unsigned long long)st.st_size);

    for (dpr = dumpers; dpr->kind != SIG_UNKNOWN; dpr++)
    {
        if (!dpr->alt_get_kind) continue;
        /* alt interface isn't compatible with incoming file_dumper */
        if (wanted_sig == dpr->kind)
            assert( !fn );

        lseek( fd, 0, SEEK_SET );
        if (dpr->alt_get_kind( fd ) == dpr->kind &&
            (wanted_sig == SIG_UNKNOWN || wanted_sig == dpr->kind))
        {
            lseek( fd, 0, SEEK_SET );
            dpr->alt_dumper( fd );
            break;
        }
    }
    close(fd);

    if (dpr->kind == SIG_UNKNOWN)
    {
        if (!(dump_base = read_file( name, &dump_total_len ))) fatal( "Cannot read file" );

        for (dpr = dumpers; dpr->kind != SIG_UNKNOWN; dpr++)
        {
            if (!dpr->get_kind) continue;
            if (dpr->get_kind() == dpr->kind &&
                (wanted_sig == SIG_UNKNOWN || wanted_sig == dpr->kind))
            {
                if (fn) fn(); else dpr->dumper();
                break;
            }
        }
        if (dpr->kind == SIG_UNKNOWN)
        {
            printf("Can't get a suitable file signature, aborting\n");
            ret = FALSE;
        }

        free( dump_base );
    }
    if (ret) printf("Done dumping %s\n", name);

    return ret;
}

void	dump_file(const char* name)
{
    dump_analysis(name, NULL, SIG_UNKNOWN);
}
