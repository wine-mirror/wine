/*
 * Useful functions for winegcc/winewrap
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"

#if !defined(min)
# define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

int verbose = 0;

void error(const char* s, ...)
{
    va_list ap;
    
    va_start(ap, s);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

void* xmalloc(size_t size)
{
    void* p;

    if ((p = malloc (size)) == NULL)
	error("Can not malloc %d bytes.", size);

    return p;
}

void *xrealloc(void* p, size_t size)
{
    void* p2;
    if ((p2 = realloc (p, size)) == NULL)
	error("Can not realloc %d bytes.", size);

    return p2;
}

int strendswith(const char* str, const char* end)
{
    int l = strlen(str);
    int m = strlen(end);
   
    return l >= m && strcmp(str + l - m, end) == 0; 
}

char* strmake(const char* fmt, ...)
{
    int n;
    size_t size = 100;
    char* p;
    va_list ap;

    p = xmalloc (size);
    while (1)
    {
        va_start(ap, fmt);
	n = vsnprintf (p, size, fmt, ap);
	va_end(ap);
        if (n > -1 && (size_t)n < size) return p;
	size = min( size*2, (size_t)n+1 );
	p = xrealloc (p, size);
    }
}

strarray* strarray_alloc(void)
{
    strarray* arr = xmalloc(sizeof(*arr));
    arr->maximum = arr->size = 0;
    arr->base = NULL;
    return arr;
}

void strarray_free(strarray* arr)
{
    free(arr->base);
    free(arr);
}

void strarray_add(strarray* arr, const char* str)
{
    if (arr->size == arr->maximum)
    {
	arr->maximum += 10;
	arr->base = xrealloc(arr->base, sizeof(*(arr->base)) * arr->maximum);
    }
    arr->base[arr->size++] = str;
}

strarray* strarray_dup(const strarray* arr)
{
    strarray* dup = strarray_alloc();
    int i;

    for (i = 0; i < arr->size; i++)
	strarray_add(dup, arr->base[i]);

    return dup;
}

char* get_basename(const char* file)
{
    const char* name;
    char *base_name, *p;

    if ((name = strrchr(file, '/'))) name++;
    else name = file;

    base_name = strdup(name);
    if ((p = strrchr(base_name, '.'))) *p = 0;

    return base_name;
}

void create_file(const char* name, const char* fmt, ...)
{
    va_list ap;
    FILE *file;

    if (verbose) printf("Creating file %s\n", name);
    va_start(ap, fmt);
    if ( !(file = fopen(name, "w")) )
	error ("Can not create %s.", name);
    vfprintf(file, fmt, ap);
    va_end(ap);
    fclose(file);
}

file_type get_file_type(const char* dir, const char* filename)
{
    /* see tools/winebuild/res32.c: check_header for details */
    static const char res_sig[] = { 0,0,0,0, 32,0,0,0, 0xff,0xff, 0,0, 0xff,0xff, 0,0, 0,0,0,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0 };
    char buf[sizeof(res_sig)];
    char *fullname;
    int fd, cnt;

    fullname = strmake("%s/%s", dir, filename);
    fd = open( fullname, O_RDONLY );
    cnt = read(fd, buf, sizeof(buf));
    if (cnt == -1) error("Can't read file: %s/%s", dir, filename);
    free( fullname );
    close( fd );

    if (fd == -1) return file_na;

    if (cnt == sizeof(res_sig) && !memcmp(buf, res_sig, sizeof(res_sig))) return file_res;
    if (strendswith(filename, ".o")) return file_obj;
    if (strendswith(filename, ".a")) return file_arh;
    if (strendswith(filename, ".res")) return file_res;
    if (strendswith(filename, ".so")) return file_so;
    if (strendswith(filename, ".def")) return file_dll;
    if (strendswith(filename, ".rc")) return file_rc;

    return file_other;
}

static file_type try_lib_path(const char* dir, const char* pre, 
			      const char* library, const char* ext)
{
    char *fullname;
    file_type type;

    fullname = strmake("%s%s%s", pre, library, ext);
    if (verbose > 1) fprintf(stderr, "Try %s/%s...", dir, fullname);
    type = get_file_type(dir, fullname);
    free( fullname );
    if (verbose > 1) fprintf(stderr, type == file_na ? "no\n" : "FOUND!\n");
    return type;
}

static file_type guess_lib_type(const char* dir, const char* library)
{
    /* Unix shared object */
    if (try_lib_path(dir, "lib", library, ".so") == file_so)
	return file_so;

    /* Windows DLL */
    if (try_lib_path(dir, "lib", library, ".def") == file_dll)
	return file_dll;
    if (try_lib_path(dir, "", library, ".def") == file_dll)
	return file_dll;

    /* Unix static archives */
    if (try_lib_path(dir, "lib", library, ".a") == file_arh)
	return file_arh;

    return file_na;
}

file_type get_lib_type(strarray* path, const char* library)
{
    int i;

    for (i = 0; i < path->size; i++)
    {
        file_type type = guess_lib_type(path->base[i], library);
	if (type != file_na) return type;
    }
    return file_na;
}

void spawn(const strarray* args)
{
    int i, status;
    strarray* arr = strarray_dup(args);
    const char **argv = arr->base;

    strarray_add(arr, NULL);
    if (verbose)
    {
	for(i = 0; argv[i]; i++) printf("%s ", argv[i]);
	printf("\n");
    }

    if ((status = spawnvp( _P_WAIT, argv[0], argv)))
    {
	if (status > 0) error("%s failed.", argv[0]);
	else perror("Error:");
	exit(3);
    }

    strarray_free(arr);
}
