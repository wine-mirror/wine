/*
 *	File dumping utility
 *
 * 	Copyright 2001,2005 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winedump.h"
#include "pe.h"

static void*			dump_base;
static unsigned long		dump_total_len;

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

const char*	get_time_str(unsigned long _t)
{
    time_t 	t = (time_t)_t;
    const char *str = ctime(&t);
    size_t len;
    static char	buf[128];

    if (!str) /* not valid time */
    {
        strcpy(buf, "not valid time");
        return buf;
    }

    len = strlen(str);
    /* FIXME: I don't get the same values from MS' pedump running under Wine...
     * I wonder if Wine isn't broken wrt to GMT settings...
     */
    if (len && str[len-1] == '\n') len--;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy( buf, str, len );
    buf[len] = 0;
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

const void*	PRD(unsigned long prd, unsigned long len)
{
    return (prd + len > dump_total_len) ? NULL : (const char*)dump_base + prd;
}

unsigned long Offset(const void* ptr)
{
    if (ptr < dump_base) {printf("<<<<<ptr below\n");return 0;}
    if ((const char *)ptr >= (const char*)dump_base + dump_total_len) {printf("<<<<<ptr above\n");return 0;}
    return (char*)ptr - (char*)dump_base;
}

static	void	do_dump( enum FileSig sig, const void* pmt )
{
    if (sig == SIG_NE)
    {
        ne_dump( dump_base, dump_total_len );
        return;
    }

    if (sig == SIG_LE)
    {
        le_dump( dump_base, dump_total_len );
        return;
    }

    pe_dump(pmt);
}

static enum FileSig check_headers(void)
{
    const WORD*		pw;
    const DWORD*		pdw;
    const IMAGE_DOS_HEADER*	dh;
    enum FileSig	sig;

    pw = PRD(0, sizeof(WORD));
    if (!pw) {printf("Can't get main signature, aborting\n"); return 0;}

    switch (*pw)
    {
    case IMAGE_DOS_SIGNATURE:
	sig = SIG_DOS;
	dh = PRD(0, sizeof(IMAGE_DOS_HEADER));
	if (dh)
	{
	    /* the signature is the first DWORD */
	    pdw = PRD(dh->e_lfanew, sizeof(DWORD));
	    if (pdw)
	    {
		if (*pdw == IMAGE_NT_SIGNATURE)
		{
		    sig = SIG_PE;
		}
                else if (*(const WORD *)pdw == IMAGE_OS2_SIGNATURE)
                {
                    sig = SIG_NE;
                }
		else if (*(const WORD *)pdw == IMAGE_VXD_SIGNATURE)
		{
                    sig = SIG_LE;
		}
		else
		{
		    printf("No PE Signature found\n");
		}
	    }
	    else
	    {
		printf("Can't get the extented signature, aborting\n");
	    }
	}
	break;
    case 0x4944: /* "DI" */
	sig = SIG_DBG;
	break;
    case 0x444D: /* "MD" */
        pdw = PRD(0, sizeof(DWORD));
        if (pdw && *pdw == 0x504D444D) /* "MDMP" */
            sig = SIG_MDMP;
        else
            sig = SIG_UNKNOWN;
        break;
    default:
	printf("No known main signature (%.2s/%x), aborting\n", (char*)pw, *pw);
	sig = SIG_UNKNOWN;
    }

    return sig;
}

int dump_analysis(const char *name, file_dumper fn, enum FileSig wanted_sig)
{
    int			fd;
    enum FileSig	effective_sig;
    int			ret = 1;
    struct stat		s;

    setbuf(stdout, NULL);

    fd = open(name, O_RDONLY | O_BINARY);
    if (fd == -1) fatal("Can't open file");

    if (fstat(fd, &s) < 0) fatal("Can't get size");
    dump_total_len = s.st_size;

#ifdef HAVE_MMAP
    if ((dump_base = mmap(NULL, dump_total_len, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1)
#endif
    {
        if (!(dump_base = malloc( dump_total_len ))) fatal( "Out of memory" );
        if ((unsigned long)read( fd, dump_base, dump_total_len ) != dump_total_len) fatal( "Cannot read file" );
    }

    effective_sig = check_headers();

    if (effective_sig == SIG_UNKNOWN)
    {
	printf("Can't get a recognized file signature, aborting\n");
	ret = 0;
    }
    else if (wanted_sig == SIG_UNKNOWN || wanted_sig == effective_sig)
    {
	switch (effective_sig)
	{
	case SIG_UNKNOWN: /* shouldn't happen... */
	    ret = 0; break;
	case SIG_PE:
	case SIG_NE:
	case SIG_LE:
	    printf("Contents of \"%s\": %ld bytes\n\n", name, dump_total_len);
	    (*fn)(effective_sig, dump_base);
	    break;
	case SIG_DBG:
	    dump_separate_dbg();
	    break;
	case SIG_DOS:
	    ret = 0; break;
        case SIG_MDMP:
            mdmp_dump();
            break;
	}
    }
    else
    {
	printf("Can't get a suitable file signature, aborting\n");
	ret = 0;
    }

    if (ret) printf("Done dumping %s\n", name);
#ifdef HAVE_MMAP
    if (munmap(dump_base, dump_total_len) == -1)
#endif
    {
        free( dump_base );
    }
    close(fd);

    return ret;
}

void	dump_file(const char* name)
{
    dump_analysis(name, do_dump, SIG_UNKNOWN);
}
