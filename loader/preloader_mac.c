/*
 * Preloader for macOS
 *
 * Copyright (C) 1995,96,97,98,99,2000,2001,2002 Free Software Foundation, Inc.
 * Copyright (C) 2004 Mike McCormack for CodeWeavers
 * Copyright (C) 2004 Alexandre Julliard
 * Copyright (C) 2017 Michael Müller
 * Copyright (C) 2017 Sebastian Lackner
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

#ifdef __APPLE__

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif
#include <unistd.h>
#include <dlfcn.h>
#ifdef HAVE_MACH_O_LOADER_H
#include <mach/thread_status.h>
#include <mach-o/loader.h>
#include <mach-o/ldsyms.h>
#endif

#include "wine/asm.h"
#include "main.h"

/* Reserve the low 8GB using a zero-fill section, this is the only way to
 * prevent system frameworks from using any of it (including allocations
 * before any preloader code runs)
 */
__asm__(".zerofill WINE_RESERVE,WINE_RESERVE,___wine_reserve,0x1fffff000");

#ifndef LC_MAIN
#define LC_MAIN 0x80000028
struct entry_point_command
{
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff;
    uint64_t stacksize;
};
#endif

static struct wine_preload_info preload_info[] =
{
    { (void *)0x00001000, 0x1fffff000 }, /* WINE_RESERVE section */
    { 0, 0 }                             /* end of list */
};

/*
 * These functions are only called when file is compiled with -fstack-protector.
 * They are normally provided by libc's startup files, but since we
 * build the preloader with "-nostartfiles -nodefaultlibs", we have to
 * provide our own versions, otherwise the linker fails.
 */
void *__stack_chk_guard = 0;
void __stack_chk_fail_local(void) { return; }
void __stack_chk_fail(void) { return; }

/* Binaries targeting 10.6 and 10.7 contain the __program_vars section, and
 * dyld4 (starting in Monterey) does not like it to be missing:
 * - running vmmap on a Wine process prints this warning:
 *   "Process exists but has not fully started -- dyld has initialized but libSystem has not"
 * - because libSystem is not initialized, dlerror() always returns NULL (causing GStreamer
 *   to crash on init).
 * - starting with macOS Sonoma, Wine crashes on launch if libSystem is not initialized.
 *
 * Adding __program_vars fixes those issues, and also allows more of the vars to
 * be set correctly by the preloader for the loaded binary.
 *
 * See also:
 * <https://github.com/apple-oss-distributions/Csu/blob/Csu-88/crt.c#L42>
 * <https://github.com/apple-oss-distributions/dyld/blob/dyld-1042.1/common/MachOAnalyzer.cpp#L2185>
 */
int           NXArgc = 0;
const char**  NXArgv = NULL;
const char**  environ = NULL;
const char*   __progname = NULL;

extern void* __dso_handle;
struct ProgramVars
{
    void*           mh;
    int*            NXArgcPtr;
    const char***   NXArgvPtr;
    const char***   environPtr;
    const char**    __prognamePtr;
};
__attribute__((used))  static struct ProgramVars pvars
__attribute__ ((section ("__DATA,__program_vars")))  = { &__dso_handle, &NXArgc, &NXArgv, &environ, &__progname };


/*
 * When 'start' is called, stack frame looks like:
 *
 *	       :
 *	| STRING AREA |
 *	+-------------+
 *	|      0      |
 *	+-------------+
 *	|  exec_path  | extra "apple" parameters start after NULL terminating env array
 *	+-------------+
 *	|      0      |
 *	+-------------+
 *	|    env[n]   |
 *	+-------------+
 *	       :
 *	       :
 *	+-------------+
 *	|    env[0]   |
 *	+-------------+
 *	|      0      |
 *	+-------------+
 *	| arg[argc-1] |
 *	+-------------+
 *	       :
 *	       :
 *	+-------------+
 *	|    arg[0]   |
 *	+-------------+
 *	|     argc    | argc is always 4 bytes long, even in 64-bit architectures
 *	+-------------+ <- sp
 *
 *	Where arg[i] and env[i] point into the STRING AREA
 *
 *  See also:
 *  macOS C runtime 'start':
 *  <https://github.com/apple-oss-distributions/Csu/blob/Csu-88/start.s>
 *
 *  macOS dyld '__dyld_start' (pre-dyld4):
 *  <https://github.com/apple-oss-distributions/dyld/blob/dyld-852.2/src/dyldStartup.s>
 */

#define target_mach_header      mach_header_64
#define target_segment_command  segment_command_64
#define TARGET_LC_SEGMENT       LC_SEGMENT_64
#define target_thread_state_t   x86_thread_state64_t
#ifdef __DARWIN_UNIX03
#define target_thread_ip(x)     (x)->__rip
#else
#define target_thread_ip(x)     (x)->rip
#endif

#define SYSCALL_FUNC( name, nr ) \
    __ASM_GLOBAL_FUNC( name, \
                       "\tmovq %rcx, %r10\n" \
                       "\tmovq $(" #nr "|0x2000000),%rax\n" \
                       "\tsyscall\n" \
                       "\tjnb 1f\n" \
                       "\tmovq $-1,%rax\n" \
                       "1:\tret\n" )

#define SYSCALL_NOERR( name, nr ) \
    __ASM_GLOBAL_FUNC( name, \
                       "\tmovq %rcx, %r10\n" \
                       "\tmovq $(" #nr "|0x2000000),%rax\n" \
                       "\tsyscall\n" \
                       "\tret\n" )

__ASM_GLOBAL_FUNC( start,
                   __ASM_CFI("\t.cfi_undefined %rip\n")
                   "\tpushq $0\n"                   /* push a zero for debugger end of frames marker */
                   "\tmovq %rsp,%rbp\n"             /* pointer to base of kernel frame */
                   "\tandq $-16,%rsp\n"             /* force SSE alignment */
                   "\tsubq $16,%rsp\n"              /* room for local variables */

                   /* call wld_start(stack, &is_unix_thread) */
                   "\tleaq 8(%rbp),%rdi\n"          /* stack */
                   "\tmovq %rsp,%rsi\n"             /* &is_unix_thread */
                   "\tmovq $0,(%rsi)\n"
                   "\tcall _wld_start\n"

                   /* jmp based on is_unix_thread */
                   "\tcmpl $0,0(%rsp)\n"
                   "\tjne 2f\n"

                   /* LC_MAIN */
                   "\tmovq 8(%rbp),%rdi\n"          /* %rdi = argc */
                   "\tleaq 16(%rbp),%rsi\n"         /* %rsi = argv */
                   "\tleaq 8(%rsi,%rdi,8),%rdx\n"   /* %rdx = env */
                   "\tmovq %rdx,%rcx\n"
                   "1:\tmovq (%rcx),%r8\n"
                   "\taddq $8,%rcx\n"
                   "\torq %r8,%r8\n"                /* look for the NULL ending the env[] array */
                   "\tjnz 1b\n"                     /* %rcx = apple data */

                   "\taddq $16,%rsp\n"              /* remove local variables */
                   "\tcall *%rax\n"                 /* call main(argc,argv,env,apple) */
                   "\tmovq %rax,%rdi\n"             /* pass result from main() to exit() */
                   "\tcall _wld_exit\n"             /* need to use call to keep stack aligned */
                   "\thlt\n"

                   /* LC_UNIXTHREAD */
                   "\t2:movq %rbp,%rsp\n"           /* restore the unaligned stack pointer */
                   "\taddq $8,%rsp\n"               /* remove the debugger end frame marker */
                   "\tmovq $0,%rbp\n"               /* restore ebp back to zero */
                   "\tjmpq *%rax\n" )               /* jump to the entry point */

void wld_exit( int code ) __attribute__((noreturn));
SYSCALL_NOERR( wld_exit, 1 /* SYS_exit */ );

ssize_t wld_write( int fd, const void *buffer, size_t len );
SYSCALL_FUNC( wld_write, 4 /* SYS_write */ );

void *wld_mmap( void *start, size_t len, int prot, int flags, int fd, off_t offset );
SYSCALL_FUNC( wld_mmap, 197 /* SYS_mmap */ );

static intptr_t (*p_dyld_get_image_slide)( const struct target_mach_header* mh );

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(dlopen);
MAKE_FUNCPTR(dlsym);
MAKE_FUNCPTR(dladdr);
#undef MAKE_FUNCPTR

extern int _dyld_func_lookup( const char *dyld_func_name, void **address );

/* replacement for libc functions */

void * memmove( void *dst, const void *src, size_t len )
{
    char *d = dst;
    const char *s = src;
    if (d < s)
        while (len--)
            *d++ = *s++;
    else
    {
        const char *lasts = s + (len-1);
        char *lastd = d + (len-1);
        while (len--)
            *lastd-- = *lasts--;
    }
    return dst;
}

/*
 * wld_printf - just the basics
 *
 *  %x prints a hex number
 *  %s prints a string
 *  %p prints a pointer
 */
static int wld_vsprintf(char *buffer, const char *fmt, va_list args )
{
    static const char hex_chars[16] = "0123456789abcdef";
    const char *p = fmt;
    char *str = buffer;
    int i;

    while( *p )
    {
        if( *p == '%' )
        {
            p++;
            if( *p == 'x' )
            {
                unsigned int x = va_arg( args, unsigned int );
                for (i = 2*sizeof(x) - 1; i >= 0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
            }
            else if (p[0] == 'l' && p[1] == 'x')
            {
                unsigned long x = va_arg( args, unsigned long );
                for (i = 2*sizeof(x) - 1; i >= 0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
                p++;
            }
            else if( *p == 'p' )
            {
                unsigned long x = (unsigned long)va_arg( args, void * );
                for (i = 2*sizeof(x) - 1; i >= 0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
            }
            else if( *p == 's' )
            {
                char *s = va_arg( args, char * );
                while(*s)
                    *str++ = *s++;
            }
            else if( *p == 0 )
                break;
            p++;
        }
        *str++ = *p++;
    }
    *str = 0;
    return str - buffer;
}

static __attribute__((format(printf,1,2))) void wld_printf(const char *fmt, ... )
{
    va_list args;
    char buffer[256];
    int len;

    va_start( args, fmt );
    len = wld_vsprintf(buffer, fmt, args );
    va_end( args );
    wld_write(2, buffer, len);
}

static __attribute__((noreturn,format(printf,1,2))) void fatal_error(const char *fmt, ... )
{
    va_list args;
    char buffer[256];
    int len;

    va_start( args, fmt );
    len = wld_vsprintf(buffer, fmt, args );
    va_end( args );
    wld_write(2, buffer, len);
    wld_exit(1);
}

static void *get_entry_point( struct target_mach_header *mh, intptr_t slide, int *unix_thread )
{
    struct entry_point_command *entry;
    target_thread_state_t *state;
    struct load_command *cmd;
    int i;

    /* try LC_MAIN first */
    cmd = (struct load_command *)(mh + 1);
    for (i = 0; i < mh->ncmds; i++)
    {
        if (cmd->cmd == LC_MAIN)
        {
            *unix_thread = FALSE;
            entry = (struct entry_point_command *)cmd;
            return (char *)mh + entry->entryoff;
        }
        cmd = (struct load_command *)((char *)cmd + cmd->cmdsize);
    }

    /* then try LC_UNIXTHREAD */
    cmd = (struct load_command *)(mh + 1);
    for (i = 0; i < mh->ncmds; i++)
    {
        if (cmd->cmd == LC_UNIXTHREAD)
        {
            *unix_thread = TRUE;
            state = (target_thread_state_t *)((char *)cmd + 16);
            return (void *)(target_thread_ip(state) + slide);
        }
        cmd = (struct load_command *)((char *)cmd + cmd->cmdsize);
    }

    return NULL;
};

static inline void get_dyld_func( const char *name, void **func )
{
    _dyld_func_lookup( name, func );
    if (!*func) fatal_error( "Failed to get function pointer for %s\n", name );
}

#define LOAD_POSIX_DYLD_FUNC(f) get_dyld_func( "__dyld_" #f, (void **)&p##f )
#define LOAD_MACHO_DYLD_FUNC(f) get_dyld_func( "_" #f, (void **)&p##f )

static void fixup_stack( void *stack )
{
    int *pargc;
    char **argv, **env_new;
    static char dummyvar[] = "WINEPRELOADERDUMMYVAR=1";

    pargc = stack;
    argv = (char **)pargc + 1;

    /* decrement argc, and "remove" argv[0] */
    *pargc = *pargc - 1;
    memmove( &argv[0], &argv[1], (*pargc + 1) * sizeof(char *) );

    env_new = &argv[*pargc-1] + 2;
    /* In the launched binary on some OSes, _NSGetEnviron() returns
     * the original 'environ' pointer, so env_new[0] would be ignored.
     * Put a dummy variable in env_new[0], so nothing is lost in this case.
     */
    env_new[0] = dummyvar;
}

static void set_program_vars( void *stack, void *mod )
{
    int *pargc;
    const char **argv, **env;
    int *wine_NXArgc = pdlsym( mod, "NXArgc" );
    const char ***wine_NXArgv = pdlsym( mod, "NXArgv" );
    const char ***wine_environ = pdlsym( mod, "environ" );

    pargc = stack;
    argv = (const char **)pargc + 1;
    env = &argv[*pargc-1] + 2;

    /* set vars in the loaded binary */
    if (wine_NXArgc)
        *wine_NXArgc = *pargc;
    else
        wld_printf( "preloader: Warning: failed to set NXArgc\n" );

    if (wine_NXArgv)
        *wine_NXArgv = argv;
    else
        wld_printf( "preloader: Warning: failed to set NXArgv\n" );

    if (wine_environ)
        *wine_environ = env;
    else
        wld_printf( "preloader: Warning: failed to set environ\n" );

    /* set vars in the __program_vars section */
    NXArgc = *pargc;
    NXArgv = argv;
    environ = env;
}

void *wld_start( void *stack, int *is_unix_thread )
{
    struct wine_preload_info **wine_main_preload_info;
    char **argv, **p;
    struct target_mach_header *mh;
    void *mod, *entry;
    int *pargc, i;
    Dl_info info;

    pargc = stack;
    argv = (char **)pargc + 1;
    if (*pargc < 2) fatal_error( "Usage: %s wine_binary [args]\n", argv[0] );

    /* skip over the parameters */
    p = argv + *pargc + 1;

    /* skip over the environment */
    while (*p) p++;

    LOAD_POSIX_DYLD_FUNC( dlopen );
    LOAD_POSIX_DYLD_FUNC( dlsym );
    LOAD_POSIX_DYLD_FUNC( dladdr );
    LOAD_MACHO_DYLD_FUNC( _dyld_get_image_slide );

    /* reserve memory that Wine needs */
    for (i = 0; preload_info[i].size; i++)
    {
        wld_mmap( preload_info[i].addr, preload_info[i].size, PROT_NONE,
                  MAP_PRIVATE | MAP_ANON | MAP_NORESERVE | MAP_FIXED, -1, 0 );
    }

    /* load the main binary */
    if (!(mod = pdlopen( argv[1], RTLD_NOW )))
        fatal_error( "%s: could not load binary\n", argv[1] );

    /* store pointer to the preload info into the appropriate main binary variable */
    wine_main_preload_info = pdlsym( mod, "wine_main_preload_info" );
    if (wine_main_preload_info) *wine_main_preload_info = preload_info;
    else wld_printf( "wine_main_preload_info not found\n" );

    if (!pdladdr( wine_main_preload_info, &info ) || !(mh = info.dli_fbase))
        fatal_error( "%s: could not find mach header\n", argv[1] );
    if (!(entry = get_entry_point( mh, p_dyld_get_image_slide(mh), is_unix_thread )))
        fatal_error( "%s: could not find entry point\n", argv[1] );

    /* decrement argc and "remove" argv[0] */
    fixup_stack(stack);

    /* Set NXArgc, NXArgv, and environ in the new binary.
     * On different configurations these were either NULL/0 or still had their
     * values from this preloader's launch.
     *
     * In particular, environ was not being updated, resulting in environ[0] being lost.
     * And for LC_UNIXTHREAD binaries on Monterey and later, environ was just NULL.
     */
    set_program_vars( stack, mod );

    return entry;
}

#endif /* __APPLE__ */
