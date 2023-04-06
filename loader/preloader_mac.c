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

#if defined(__x86_64__)
/* Reserve the low 8GB using a zero-fill section, this is the only way to
 * prevent system frameworks from using any of it (including allocations
 * before any preloader code runs)
 */
__asm__(".zerofill WINE_RESERVE,WINE_RESERVE,___wine_reserve,0x1fffff000");

static const struct wine_preload_info zerofill_sections[] =
{
    { (void *)0x000000001000, 0x1fffff000 }, /* WINE_RESERVE section */
    { 0, 0 }                                 /* end of list */
};
#else
static const struct wine_preload_info zerofill_sections[] =
{
    { 0, 0 }                                 /* end of list */
};
#endif

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
    /* On macOS, we allocate the low 64k area in two steps because PAGEZERO
     * might not always be available. */
#ifdef __i386__
    { (void *)0x00000000, 0x00001000 },  /* first page */
    { (void *)0x00001000, 0x0000f000 },  /* low 64k */
    { (void *)0x00010000, 0x00100000 },  /* DOS area */
    { (void *)0x00110000, 0x67ef0000 },  /* low memory area */
    { (void *)0x7f000000, 0x03000000 },  /* top-down allocations + shared user data + virtual heap */
#else  /* __i386__ */
    { (void *)0x000000001000, 0x1fffff000 }, /* WINE_RESERVE section */
    { (void *)0x7ff000000000, 0x01ff0000 },  /* top-down allocations + virtual heap */
#endif /* __i386__ */
    { 0, 0 },                            /* PE exe range set with WINEPRELOADRESERVE */
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

#ifdef __i386__

static const size_t page_mask = 0xfff;
#define target_mach_header      mach_header
#define target_segment_command  segment_command
#define TARGET_LC_SEGMENT       LC_SEGMENT
#define target_thread_state_t   i386_thread_state_t
#ifdef __DARWIN_UNIX03
#define target_thread_ip(x)     (x)->__eip
#else
#define target_thread_ip(x)     (x)->eip
#endif

#define SYSCALL_FUNC( name, nr ) \
    __ASM_GLOBAL_FUNC( name, \
                       "\tmovl $" #nr ",%eax\n" \
                       "\tint $0x80\n" \
                       "\tjnb 1f\n" \
                       "\tmovl $-1,%eax\n" \
                       "1:\tret\n" )

#define SYSCALL_NOERR( name, nr ) \
    __ASM_GLOBAL_FUNC( name, \
                       "\tmovl $" #nr ",%eax\n" \
                       "\tint $0x80\n" \
                       "\tret\n" )

__ASM_GLOBAL_FUNC( start,
                   __ASM_CFI("\t.cfi_undefined %eip\n")
                   /* The first 16 bytes are used as a function signature on i386 */
                   "\t.byte 0x6a,0x00\n"            /* pushl $0: push a zero for debugger end of frames marker */
                   "\t.byte 0x89,0xe5\n"            /* movl %esp,%ebp: pointer to base of kernel frame */
                   "\t.byte 0x83,0xe4,0xf0\n"       /* andl $-16,%esp: force SSE alignment */
                   "\t.byte 0x83,0xec,0x10\n"       /* subl $16,%esp: room for new argc, argv, & envp, SSE aligned */
                   "\t.byte 0x8b,0x5d,0x04\n"       /* movl 4(%ebp),%ebx: pickup argc in %ebx */
                   "\t.byte 0x89,0x5c,0x24,0x00\n"  /* movl %ebx,0(%esp): argc to reserved stack word */

                   /* call wld_start(stack, &is_unix_thread) */
                   "\tleal 4(%ebp),%eax\n"
                   "\tmovl %eax,0(%esp)\n"          /* stack */
                   "\tleal 8(%esp),%eax\n"
                   "\tmovl %eax,4(%esp)\n"          /* &is_unix_thread */
                   "\tmovl $0,(%eax)\n"
                   "\tcall _wld_start\n"

                   /* jmp based on is_unix_thread */
                   "\tcmpl $0,8(%esp)\n"
                   "\tjne 2f\n"

                   "\tmovl 4(%ebp),%edi\n"          /* %edi = argc */
                   "\tleal 8(%ebp),%esi\n"          /* %esi = argv */
                   "\tleal 4(%esi,%edi,4),%edx\n"   /* %edx = env */
                   "\tmovl %edx,%ecx\n"
                   "1:\tmovl (%ecx),%ebx\n"
                   "\tadd $4,%ecx\n"
                   "\torl %ebx,%ebx\n"              /* look for the NULL ending the env[] array */
                   "\tjnz 1b\n"                     /* %ecx = apple data */

                   /* LC_MAIN */
                   "\tmovl %edi,0(%esp)\n"          /* argc */
                   "\tmovl %esi,4(%esp)\n"          /* argv */
                   "\tmovl %edx,8(%esp)\n"          /* env */
                   "\tmovl %ecx,12(%esp)\n"         /* apple data */
                   "\tcall *%eax\n"                 /* call main(argc,argv,env,apple) */
                   "\tmovl %eax,(%esp)\n"           /* pass result from main() to exit() */
                   "\tcall _wld_exit\n"             /* need to use call to keep stack aligned */
                   "\thlt\n"

                   /* LC_UNIXTHREAD */
                   "\t2:movl %ebp,%esp\n"           /* restore the unaligned stack pointer */
                   "\taddl $4,%esp\n"               /* remove the debugger end frame marker */
                   "\tmovl $0,%ebp\n"               /* restore ebp back to zero */
                   "\tjmpl *%eax\n" )               /* jump to the entry point */

#elif defined(__x86_64__)

static const size_t page_mask = 0xfff;
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

#else
#error preloader not implemented for this CPU
#endif

void wld_exit( int code ) __attribute__((noreturn));
SYSCALL_NOERR( wld_exit, 1 /* SYS_exit */ );

ssize_t wld_write( int fd, const void *buffer, size_t len );
SYSCALL_FUNC( wld_write, 4 /* SYS_write */ );

void *wld_mmap( void *start, size_t len, int prot, int flags, int fd, off_t offset );
SYSCALL_FUNC( wld_mmap, 197 /* SYS_mmap */ );

void *wld_munmap( void *start, size_t len );
SYSCALL_FUNC( wld_munmap, 73 /* SYS_munmap */ );

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

static int wld_strncmp( const char *str1, const char *str2, size_t len )
{
    if (len <= 0) return 0;
    while ((--len > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
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

static int preloader_overlaps_range( const void *start, const void *end )
{
    intptr_t slide = p_dyld_get_image_slide(&_mh_execute_header);
    struct load_command *cmd = (struct load_command*)(&_mh_execute_header + 1);
    int i;

    for (i = 0; i < _mh_execute_header.ncmds; ++i)
    {
        if (cmd->cmd == TARGET_LC_SEGMENT)
        {
            struct target_segment_command *seg = (struct target_segment_command*)cmd;
            const void *seg_start = (const void*)(seg->vmaddr + slide);
            const void *seg_end = (const char*)seg_start + seg->vmsize;
            static const char reserved_segname[] = "WINE_RESERVE";

            if (!wld_strncmp( seg->segname, reserved_segname, sizeof(reserved_segname)-1 ))
                continue;

            if (end > seg_start && start <= seg_end)
            {
                char segname[sizeof(seg->segname) + 1];
                memcpy(segname, seg->segname, sizeof(seg->segname));
                segname[sizeof(segname) - 1] = 0;
                wld_printf( "WINEPRELOADRESERVE range %p-%p overlaps preloader %s segment %p-%p\n",
                             start, end, segname, seg_start, seg_end );
                return 1;
            }
        }
        cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
    }

    return 0;
}

/*
 *  preload_reserve
 *
 * Reserve a range specified in string format
 */
static void preload_reserve( const char *str )
{
    const char *p;
    unsigned long result = 0;
    void *start = NULL, *end = NULL;
    int i, first = 1;

    for (p = str; *p; p++)
    {
        if (*p >= '0' && *p <= '9') result = result * 16 + *p - '0';
        else if (*p >= 'a' && *p <= 'f') result = result * 16 + *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') result = result * 16 + *p - 'A' + 10;
        else if (*p == '-')
        {
            if (!first) goto error;
            start = (void *)(result & ~page_mask);
            result = 0;
            first = 0;
        }
        else goto error;
    }
    if (!first) end = (void *)((result + page_mask) & ~page_mask);
    else if (result) goto error;  /* single value '0' is allowed */

    /* sanity checks */
    if (end <= start || preloader_overlaps_range(start, end))
        start = end = NULL;

    /* check for overlap with low memory areas */
    for (i = 0; preload_info[i].size; i++)
    {
        if ((char *)preload_info[i].addr > (char *)0x00110000) break;
        if ((char *)end <= (char *)preload_info[i].addr + preload_info[i].size)
        {
            start = end = NULL;
            break;
        }
        if ((char *)start < (char *)preload_info[i].addr + preload_info[i].size)
            start = (char *)preload_info[i].addr + preload_info[i].size;
    }

    while (preload_info[i].size) i++;
    preload_info[i].addr = start;
    preload_info[i].size = (char *)end - (char *)start;
    return;

error:
    fatal_error( "invalid WINEPRELOADRESERVE value '%s'\n", str );
}

/* remove a range from the preload list */
static void remove_preload_range( int i )
{
    while (preload_info[i].size)
    {
        preload_info[i].addr = preload_info[i+1].addr;
        preload_info[i].size = preload_info[i+1].size;
        i++;
    }
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

static int is_zerofill( struct wine_preload_info *info )
{
    int i;

    for (i = 0; zerofill_sections[i].size; i++)
    {
        if ((zerofill_sections[i].addr == info->addr) &&
            (zerofill_sections[i].size == info->size))
            return 1;
    }
    return 0;
}

static int map_region( struct wine_preload_info *info )
{
    int flags = MAP_PRIVATE | MAP_ANON;
    void *ret;

    if (!info->addr || is_zerofill( info )) flags |= MAP_FIXED;

    ret = wld_mmap( info->addr, info->size, PROT_NONE, flags, -1, 0 );
    if (ret == info->addr) return 1;
    if (ret != (void *)-1) wld_munmap( ret, info->size );

    /* don't warn for zero page */
    if (info->addr >= (void *)0x1000)
        wld_printf( "preloader: Warning: failed to reserve range %p-%p\n",
                    info->addr, (char *)info->addr + info->size );
    return 0;
}

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
#ifdef __i386__
    struct wine_preload_info builtin_dlls = { (void *)0x7a000000, 0x02000000 };
#endif
    struct wine_preload_info **wine_main_preload_info;
    char **argv, **p, *reserve = NULL;
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
    while (*p)
    {
        static const char res[] = "WINEPRELOADRESERVE=";
        if (!wld_strncmp( *p, res, sizeof(res)-1 )) reserve = *p + sizeof(res) - 1;
        p++;
    }

    LOAD_POSIX_DYLD_FUNC( dlopen );
    LOAD_POSIX_DYLD_FUNC( dlsym );
    LOAD_POSIX_DYLD_FUNC( dladdr );
    LOAD_MACHO_DYLD_FUNC( _dyld_get_image_slide );

    /* reserve memory that Wine needs */
    if (reserve) preload_reserve( reserve );
    for (i = 0; preload_info[i].size; i++)
    {
        if (!map_region( &preload_info[i] ))
        {
            remove_preload_range( i );
            i--;
        }
    }

#ifdef __i386__
    if (!map_region( &builtin_dlls ))
        builtin_dlls.size = 0;
#endif

    /* load the main binary */
    if (!(mod = pdlopen( argv[1], RTLD_NOW )))
        fatal_error( "%s: could not load binary\n", argv[1] );

#ifdef __i386__
    if (builtin_dlls.size)
        wld_munmap( builtin_dlls.addr, builtin_dlls.size );
#endif

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
