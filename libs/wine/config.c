/*
 * Configuration parameters shared between Wine server and clients
 *
 * Copyright 2002 Alexandre Julliard
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

static const char * const server_config_dir = "/.wine";        /* config dir relative to $HOME */
static const char * const server_root_prefix = "/tmp/.wine-";  /* prefix for server root dir */
static const char * const server_dir_prefix = "/server-";      /* prefix for server dir */

static char *config_dir;
static char *server_dir;
static char *user_name;
static char *argv0_path;

#ifdef __GNUC__
static void fatal_error( const char *err, ... )  __attribute__((noreturn,format(printf,1,2)));
static void fatal_perror( const char *err, ... )  __attribute__((noreturn,format(printf,1,2)));
#endif

/* die on a fatal error */
static void fatal_error( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    va_end( args );
    exit(1);
}

/* die on a fatal error */
static void fatal_perror( const char *err, ... )
{
    va_list args;

    va_start( args, err );
    fprintf( stderr, "wine: " );
    vfprintf( stderr, err, args );
    perror( " " );
    va_end( args );
    exit(1);
}

/* malloc wrapper */
static void *xmalloc( size_t size )
{
    void *res;

    if (!size) size = 1;
    if (!(res = malloc( size ))) fatal_error( "virtual memory exhausted\n");
    return res;
}

/* strdup wrapper */
static char *xstrdup( const char *str )
{
    size_t len = strlen(str) + 1;
    char *res = xmalloc( len );
    memcpy( res, str, len );
    return res;
}

/* remove all trailing slashes from a path name */
inline static void remove_trailing_slashes( char *path )
{
    int len = strlen( path );
    while (len > 1 && path[len-1] == '/') path[--len] = 0;
}

/* initialize all the paths values */
static void init_paths(void)
{
    struct stat st;
    char *p;

    const char *home = getenv( "HOME" );
    const char *user = NULL;
    const char *prefix = getenv( "WINEPREFIX" );

#ifdef HAVE_GETPWUID
    char uid_str[32];
    struct passwd *pwd = getpwuid( getuid() );

    if (pwd)
    {
        user = pwd->pw_name;
        if (!home) home = pwd->pw_dir;
    }
    if (!user)
    {
        sprintf( uid_str, "%d", getuid() );
        user = uid_str;
    }
#else  /* HAVE_GETPWUID */
    if (!(user = getenv( "USER" )))
        fatal_error( "cannot determine your user name, set the USER environment variable\n" );
#endif  /* HAVE_GETPWUID */
    user_name = xstrdup( user );

    /* build config_dir */

    if (prefix)
    {
        if (!(config_dir = strdup( prefix ))) fatal_error( "virtual memory exhausted\n");
        remove_trailing_slashes( config_dir );
        if (config_dir[0] != '/')
            fatal_error( "invalid directory %s in WINEPREFIX: not an absolute path\n", prefix );
        if (stat( config_dir, &st ) == -1)
            fatal_perror( "cannot open %s as specified in WINEPREFIX", config_dir );
    }
    else
    {
        if (!home) fatal_error( "could not determine your home directory\n" );
        if (home[0] != '/') fatal_error( "your home directory %s is not an absolute path\n", home );
        config_dir = xmalloc( strlen(home) + strlen(server_config_dir) + 1 );
        strcpy( config_dir, home );
        remove_trailing_slashes( config_dir );
        strcat( config_dir, server_config_dir );
        if (stat( config_dir, &st ) == -1)
            fatal_perror( "cannot open %s", config_dir );
    }
    if (!S_ISDIR(st.st_mode)) fatal_error( "%s is not a directory\n", config_dir );

    /* build server_dir */

    server_dir = xmalloc( strlen(server_root_prefix) + strlen(user) + strlen( server_dir_prefix ) +
                          2*sizeof(st.st_dev) + 2*sizeof(st.st_ino) + 2 );
    strcpy( server_dir, server_root_prefix );
    p = server_dir + strlen(server_dir);
    strcpy( p, user );
    while (*p)
    {
        if (*p == '/') *p = '!';
        p++;
    }
    strcpy( p, server_dir_prefix );

    if (sizeof(st.st_dev) > sizeof(unsigned long) && st.st_dev > ~0UL)
        sprintf( server_dir + strlen(server_dir), "%lx%08lx-",
                 (unsigned long)(st.st_dev >> 32), (unsigned long)st.st_dev );
    else
        sprintf( server_dir + strlen(server_dir), "%lx-", (unsigned long)st.st_dev );

    if (sizeof(st.st_ino) > sizeof(unsigned long) && st.st_ino > ~0UL)
        sprintf( server_dir + strlen(server_dir), "%lx%08lx",
                 (unsigned long)(st.st_ino >> 32), (unsigned long)st.st_ino );
    else
        sprintf( server_dir + strlen(server_dir), "%lx", (unsigned long)st.st_ino );
}

/* initialize the argv0 path */
void init_argv0_path( const char *argv0 )
{
    size_t size, len;
    const char *p;
    char *cwd;

    if (!(p = strrchr( argv0, '/' )))
        return;  /* if argv0 doesn't contain a path, don't do anything */

    len = p - argv0 + 1;
    if (argv0[0] == '/')  /* absolute path */
    {
        argv0_path = xmalloc( len + 1 );
        memcpy( argv0_path, argv0, len );
        argv0_path[len] = 0;
        return;
    }

    /* relative path, make it absolute */
    for (size = 256 + len; ; size *= 2)
    {
        if (!(cwd = malloc( size ))) break;
        if (getcwd( cwd, size - len ))
        {
            argv0_path = cwd;
            cwd += strlen(cwd);
            *cwd++ = '/';
            memcpy( cwd, argv0, len );
            cwd[len] = 0;
            return;
        }
        free( cwd );
        if (errno != ERANGE) break;
    }
}

/* return the configuration directory ($WINEPREFIX or $HOME/.wine) */
const char *wine_get_config_dir(void)
{
    if (!config_dir) init_paths();
    return config_dir;
}

/* return the full name of the server directory (the one containing the socket) */
const char *wine_get_server_dir(void)
{
    if (!server_dir) init_paths();
    return server_dir;
}

/* return the current user name */
const char *wine_get_user_name(void)
{
    if (!user_name) init_paths();
    return user_name;
}

/* return the path of argv[0], including a trailing slash */
const char *wine_get_argv0_path(void)
{
    return argv0_path;
}
