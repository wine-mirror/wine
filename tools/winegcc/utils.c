/*
 * Useful functions for winegcc
 *
 * Copyright 2000 Francois Gouget
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2003 Richard Cohen
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"

int verbose = 0;

void error(const char* s, ...)
{
    va_list ap;
    
    va_start(ap, s);
    fprintf(stderr, "winegcc: ");
    vfprintf(stderr, s, ap);
    va_end(ap);
    exit(2);
}

void create_file(const char* name, int mode, const char* fmt, ...)
{
    va_list ap;
    FILE *file;

    if (verbose) printf("Creating file %s\n", name);
    va_start(ap, fmt);
    if ( !(file = fopen(name, "w")) )
	error("Unable to open %s for writing\n", name);
    vfprintf(file, fmt, ap);
    va_end(ap);
    fclose(file);
    chmod(name, mode);
}

file_type get_file_type(const char* filename)
{
    /* see tools/winebuild/res32.c: check_header for details */
    static const char res_sig[] = { 0,0,0,0, 32,0,0,0, 0xff,0xff, 0,0, 0xff,0xff, 0,0, 0,0,0,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0 };
    static const char elf_sig[4] = "\177ELF";
    static const char ar_sig[8] = "!<arch>\n";
    char buf[sizeof(res_sig)];
    int fd, cnt;

    fd = open( filename, O_RDONLY );
    if (fd == -1) return file_na;
    cnt = read(fd, buf, sizeof(buf));
    close( fd );
    if (cnt == -1) return file_na;

    if (cnt == sizeof(res_sig) && !memcmp(buf, res_sig, sizeof(res_sig))) return file_res;
    if (strendswith(filename, ".o")) return file_obj;
    if (strendswith(filename, ".obj")) return file_obj;
    if (strendswith(filename, ".a")) return file_arh;
    if (strendswith(filename, ".res")) return file_res;
    if (strendswith(filename, ".so")) return file_so;
    if (strendswith(filename, ".dylib")) return file_so;
    if (strendswith(filename, ".def")) return file_def;
    if (strendswith(filename, ".spec")) return file_spec;
    if (strendswith(filename, ".rc")) return file_rc;
    if (cnt >= sizeof(elf_sig) && !memcmp(buf, elf_sig, sizeof(elf_sig))) return file_so;  /* ELF lib */
    if (cnt >= sizeof(ar_sig) && !memcmp(buf, ar_sig, sizeof(ar_sig))) return file_arh;
    if (cnt >= sizeof(unsigned int) &&
        (*(unsigned int *)buf == 0xfeedface || *(unsigned int *)buf == 0xcefaedfe ||
         *(unsigned int *)buf == 0xfeedfacf || *(unsigned int *)buf == 0xcffaedfe))
        return file_so; /* Mach-O lib */

    return file_other;
}

static char* try_lib_path(const char *dir, const char *arch_dir, const char *pre,
			  const char* library, const char* ext,
			  file_type expected_type)
{
    char *fullname;
    file_type type;

    /* first try a subdir named from the library we are looking for */
    fullname = strmake("%s/%s%s/%s%s%s", dir, library, arch_dir, pre, library, ext);
    if (verbose > 1) fprintf(stderr, "Try %s...", fullname);
    type = get_file_type(fullname);
    if (verbose > 1) fprintf(stderr, type == expected_type ? "FOUND!\n" : "no\n");
    if (type == expected_type) return fullname;
    free( fullname );

    fullname = strmake("%s/%s%s%s", dir, pre, library, ext);
    if (verbose > 1) fprintf(stderr, "Try %s...", fullname);
    type = get_file_type(fullname);
    if (verbose > 1) fprintf(stderr, type == expected_type ? "FOUND!\n" : "no\n");
    if (type == expected_type) return fullname;
    free( fullname );
    return 0; 
}

static file_type guess_lib_type(struct target target, const char* dir,
                                const char* library, const char *prefix, const char *suffix, char** file)
{
    const char *arch_dir = "";

    if (target.platform != PLATFORM_WINDOWS &&
        target.platform != PLATFORM_MINGW &&
        target.platform != PLATFORM_CYGWIN)
    {
        /* Unix shared object */
        if ((*file = try_lib_path(dir, "", prefix, library, ".so", file_so)))
            return file_so;

        /* Mach-O (Darwin/Mac OS X) Dynamic Library behaves mostly like .so */
        if ((*file = try_lib_path(dir, "", prefix, library, ".dylib", file_so)))
            return file_so;

        /* Windows DLL */
        if ((*file = try_lib_path(dir, "", prefix, library, ".def", file_def)))
            return file_dll;
    }
    else
    {
        arch_dir = get_arch_dir( target );
        if (!strcmp( suffix, ".a" ))  /* try Mingw-style .dll.a import lib */
        {
            if ((*file = try_lib_path(dir, arch_dir, prefix, library, ".dll.a", file_arh)))
                return file_arh;
        }
    }

    /* static archives */
    if ((*file = try_lib_path(dir, arch_dir, prefix, library, suffix, file_arh)))
	return file_arh;

    return file_na;
}

file_type get_lib_type(struct target target, struct strarray path, const char *library,
                       const char *prefix, const char *suffix, char** file)
{
    unsigned int i;

    if (!suffix) suffix = ".a";
    for (i = 0; i < path.count; i++)
    {
        file_type type = guess_lib_type(target, path.str[i], library, prefix, suffix, file);
	if (type != file_na) return type;
    }
    return file_na;
}

const char *find_binary( struct strarray prefix, const char *name )
{
    char *file_name, *args;
    struct strarray dirs = empty_strarray;
    static struct strarray path;
    unsigned int i;

    if (strchr( name, '/' )) return name;

    file_name = xstrdup( name );
    if ((args = strchr( file_name, ' ' ))) *args++ = 0;

    if (!path.count) strarray_addall( &path, strarray_frompath( getenv( "PATH" )));

    strarray_addall( &dirs, prefix );
    strarray_addall( &dirs, path );
    for (i = 0; i < dirs.count; i++)
    {
        struct stat st;
        char *prog = strmake( "%s/%s%s", dirs.str[i], file_name, EXEEXT );
        if (stat( prog, &st ) == 0 && S_ISREG( st.st_mode ) && (st.st_mode & 0111))
            return args ? strmake( "%s %s", prog, args ) : prog;
        free( prog );
    }
    return NULL;
}

int spawn(struct strarray prefix, struct strarray args, int ignore_errors)
{
    int status;

    args.str[0] = find_binary( prefix, args.str[0] );
    if (verbose) strarray_trace( args );

    if ((status = strarray_spawn( args )) && !ignore_errors)
    {
	if (status > 0) error("%s failed\n", args.str[0]);
	else perror("winegcc");
	exit(3);
    }

    return status;
}
