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

void dump_data( const unsigned char *ptr, unsigned int size, const char *prefix )
{
    unsigned int i, j;

    printf( "%s%08x: ", prefix, 0 );
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
            if (i < size-1) printf( "\n%s%08x: ", prefix, i + 1 );
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

static char* dump_want_n(unsigned sz)
{
    static char         buffer[4 * 1024];
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

const char* get_symbol_str(const char* symname)
{
    char*       tmp;
    const char* ret;

    if (!symname) return "(nil)";
    if (globals.do_demangle)
    {
        parsed_symbol   symbol;

        symbol_init(&symbol, symname);
        if (!symbol_demangle(&symbol))
            ret = symname;
        else if (symbol.flags & SYM_DATA)
        {
            ret = tmp = dump_want_n(strlen(symbol.arg_text[0]) + 1);
            if (tmp) strcpy(tmp, symbol.arg_text[0]);
        }
        else
        {
            unsigned int i, len, start = symbol.flags & SYM_THISCALL ? 1 : 0;

            len = strlen(symbol.return_text) + 3 /* ' __' */ +
                strlen(symbol_get_call_convention(&symbol)) + 1 /* ' ' */+
                strlen(symbol.function_name) + 1 /* ')' */;
            if (!symbol.argc || (symbol.argc == 1 && symbol.flags & SYM_THISCALL))
                len += 4 /* "void" */;
            else for (i = start; i < symbol.argc; i++)
                len += (i > start ? 2 /* ", " */ : 0 /* "" */) + strlen(symbol.arg_text[i]);
            if (symbol.varargs) len += 5 /* ", ..." */;
            len += 2; /* ")\0" */

            ret = tmp = dump_want_n(len);
            if (tmp)
            {
                sprintf(tmp, "%s __%s %s(",
                        symbol.return_text,
                        symbol_get_call_convention(&symbol),
                        symbol.function_name);
                if (!symbol.argc || (symbol.argc == 1 && symbol.flags & SYM_THISCALL))
                    strcat(tmp, "void");
                else for (i = start; i < symbol.argc; i++)
                {
                    if (i > start) strcat(tmp, ", ");
                    strcat(tmp, symbol.arg_text[i]);
                }
                if (symbol.varargs) strcat(tmp, ", ...");
                strcat(tmp, ")");
            }
        }
        symbol_clear(&symbol);
    }
    else ret = symname;
    return ret;
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
}
dumpers[] =
{
    {SIG_DOS,           get_kind_exec,  dos_dump},
    {SIG_PE,            get_kind_exec,  pe_dump},
    {SIG_DBG,           get_kind_dbg,   dbg_dump},
    {SIG_PDB,           get_kind_pdb,   pdb_dump},
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
    {SIG_UNKNOWN,       NULL,           NULL} /* sentinel */
};

BOOL dump_analysis(const char *name, file_dumper fn, enum FileSig wanted_sig)
{
    BOOL                ret = TRUE;
    const struct dumper *dpr;

    setbuf(stdout, NULL);

    if (!(dump_base = read_file( name, &dump_total_len ))) fatal( "Cannot read file" );

    printf("Contents of %s: %zu bytes\n\n", name, dump_total_len);

    for (dpr = dumpers; dpr->kind != SIG_UNKNOWN; dpr++)
    {
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

    if (ret) printf("Done dumping %s\n", name);
    free( dump_base );

    return ret;
}

void	dump_file(const char* name)
{
    dump_analysis(name, NULL, SIG_UNKNOWN);
}
