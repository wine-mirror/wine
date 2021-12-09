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

/* Rosetta on Apple Silicon allocates memory starting at 0x100000000 (the 4GB line)
 * before the preloader runs, which prevents any nonrelocatable EXEs with that
 * base address from running.
 *
 * This empty linker section forces Rosetta's allocations (currently ~132 MB)
 * to start at 0x114000000, and they should end below 0x120000000.
 */
#if defined(__x86_64__)
__asm__(".zerofill WINE_4GB_RESERVE,WINE_4GB_RESERVE,___wine_4gb_reserve,0x14000000");
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
    { (void *)0x7f000000, 0x03000000 },  /* top-down allocations + shared heap + virtual heap */
#else  /* __i386__ */
    { (void *)0x000000010000, 0x00100000 },  /* DOS area */
    { (void *)0x000000110000, 0x67ef0000 },  /* low memory area */
    { (void *)0x00007ff00000, 0x000f0000 },  /* shared user data */
    { (void *)0x000100000000, 0x14000000 },  /* WINE_4GB_RESERVE section */
    { (void *)0x7ffd00000000, 0x01ff0000 },  /* top-down allocations + virtual heap */
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

#ifdef __i386__

static const size_t page_size = 0x1000;
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
                   "\t.byte 0x6a,0x00\n"            /* pushl $0 */
                   "\t.byte 0x89,0xe5\n"            /* movl %esp,%ebp */
                   "\t.byte 0x83,0xe4,0xf0\n"       /* andl $-16,%esp */
                   "\t.byte 0x83,0xec,0x10\n"       /* subl $16,%esp */
                   "\t.byte 0x8b,0x5d,0x04\n"       /* movl 4(%ebp),%ebx */
                   "\t.byte 0x89,0x5c,0x24,0x00\n"  /* movl %ebx,0(%esp) */

                   "\tleal 4(%ebp),%eax\n"
                   "\tmovl %eax,0(%esp)\n"          /* stack */
                   "\tleal 8(%esp),%eax\n"
                   "\tmovl %eax,4(%esp)\n"          /* &is_unix_thread */
                   "\tmovl $0,(%eax)\n"
                   "\tcall _wld_start\n"

                   "\tmovl 4(%ebp),%edi\n"
                   "\tdecl %edi\n"                  /* argc */
                   "\tleal 12(%ebp),%esi\n"         /* argv */
                   "\tleal 4(%esi,%edi,4),%edx\n"   /* env */
                   "\tmovl %edx,%ecx\n"             /* apple data */
                   "1:\tmovl (%ecx),%ebx\n"
                   "\tadd $4,%ecx\n"
                   "\torl %ebx,%ebx\n"
                   "\tjnz 1b\n"

                   "\tcmpl $0,8(%esp)\n"
                   "\tjne 2f\n"

                   /* LC_MAIN */
                   "\tmovl %edi,0(%esp)\n"          /* argc */
                   "\tmovl %esi,4(%esp)\n"          /* argv */
                   "\tmovl %edx,8(%esp)\n"          /* env */
                   "\tmovl %ecx,12(%esp)\n"         /* apple data */
                   "\tcall *%eax\n"
                   "\tmovl %eax,(%esp)\n"
                   "\tcall _wld_exit\n"
                   "\thlt\n"

                   /* LC_UNIXTHREAD */
                   "2:\tmovl (%ecx),%ebx\n"
                   "\tadd $4,%ecx\n"
                   "\torl %ebx,%ebx\n"
                   "\tjnz 2b\n"

                   "\tsubl %ebp,%ecx\n"
                   "\tsubl $8,%ecx\n"
                   "\tleal 4(%ebp),%esp\n"
                   "\tsubl %ecx,%esp\n"

                   "\tmovl %edi,(%esp)\n"           /* argc */
                   "\tleal 4(%esp),%edi\n"
                   "\tshrl $2,%ecx\n"
                   "\tcld\n"
                   "\trep; movsd\n"                 /* argv, ... */

                   "\tmovl $0,%ebp\n"
                   "\tjmpl *%eax\n" )

#elif defined(__x86_64__)

static const size_t page_size = 0x1000;
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
                   "\tpushq $0\n"
                   "\tmovq %rsp,%rbp\n"
                   "\tandq $-16,%rsp\n"
                   "\tsubq $16,%rsp\n"

                   "\tleaq 8(%rbp),%rdi\n"          /* stack */
                   "\tmovq %rsp,%rsi\n"             /* &is_unix_thread */
                   "\tmovq $0,(%rsi)\n"
                   "\tcall _wld_start\n"

                   "\tmovq 8(%rbp),%rdi\n"
                   "\tdec %rdi\n"                   /* argc */
                   "\tleaq 24(%rbp),%rsi\n"         /* argv */
                   "\tleaq 8(%rsi,%rdi,8),%rdx\n"   /* env */
                   "\tmovq %rdx,%rcx\n"             /* apple data */
                   "1:\tmovq (%rcx),%r8\n"
                   "\taddq $8,%rcx\n"
                   "\torq %r8,%r8\n"
                   "\tjnz 1b\n"

                   "\tcmpl $0,0(%rsp)\n"
                   "\tjne 2f\n"

                   /* LC_MAIN */
                   "\taddq $16,%rsp\n"
                   "\tcall *%rax\n"
                   "\tmovq %rax,%rdi\n"
                   "\tcall _wld_exit\n"
                   "\thlt\n"

                   /* LC_UNIXTHREAD */
                   "2:\tmovq (%rcx),%r8\n"
                   "\taddq $8,%rcx\n"
                   "\torq %r8,%r8\n"
                   "\tjnz 2b\n"

                   "\tsubq %rbp,%rcx\n"
                   "\tsubq $16,%rcx\n"
                   "\tleaq 8(%rbp),%rsp\n"
                   "\tsubq %rcx,%rsp\n"

                   "\tmovq %rdi,(%rsp)\n"           /* argc */
                   "\tleaq 8(%rsp),%rdi\n"
                   "\tshrq $3,%rcx\n"
                   "\tcld\n"
                   "\trep; movsq\n"                 /* argv, ... */

                   "\tmovq $0,%rbp\n"
                   "\tjmpq *%rax\n" )

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

int wld_mincore( void *addr, size_t length, unsigned char *vec );
SYSCALL_FUNC( wld_mincore, 78 /* SYS_mincore */ );

static intptr_t (*p_dyld_get_image_slide)( const struct target_mach_header* mh );

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(dlopen);
MAKE_FUNCPTR(dlsym);
MAKE_FUNCPTR(dladdr);
#undef MAKE_FUNCPTR

extern int _dyld_func_lookup( const char *dyld_func_name, void **address );

/* replacement for libc functions */

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
            static const char reserved_segname[] = "WINE_4GB_RESERVE";

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

static int is_region_empty( struct wine_preload_info *info )
{
    unsigned char vec[1024];
    size_t pos, size, block = 1024 * page_size;
    int i;

    for (pos = 0; pos < info->size; pos += size)
    {
        size = (pos + block <= info->size) ? block : (info->size - pos);
        if (wld_mincore( (char *)info->addr + pos, size, vec ) == -1)
        {
            if (size <= page_size) continue;
            block = page_size; size = 0;  /* retry with smaller block size */
        }
        else
        {
            for (i = 0; i < size / page_size; i++)
                if (vec[i] & 1) return 0;
        }
    }

    return 1;
}

static int map_region( struct wine_preload_info *info )
{
    int flags = MAP_PRIVATE | MAP_ANON;
    void *ret;

    if (!info->addr) flags |= MAP_FIXED;

    for (;;)
    {
        ret = wld_mmap( info->addr, info->size, PROT_NONE, flags, -1, 0 );
        if (ret == info->addr) return 1;
        if (ret != (void *)-1) wld_munmap( ret, info->size );
        if (flags & MAP_FIXED) break;

        /* Some versions of macOS ignore the address hint passed to mmap -
         * use mincore() to check if its empty and then use MAP_FIXED */
        if (!is_region_empty( info )) break;
        flags |= MAP_FIXED;
    }

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

void *wld_start( void *stack, int *is_unix_thread )
{
    struct wine_preload_info builtin_dlls = { (void *)0x7a000000, 0x02000000 };
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

    if (!map_region( &builtin_dlls ))
        builtin_dlls.size = 0;

    /* load the main binary */
    if (!(mod = pdlopen( argv[1], RTLD_NOW )))
        fatal_error( "%s: could not load binary\n", argv[1] );

    if (builtin_dlls.size)
        wld_munmap( builtin_dlls.addr, builtin_dlls.size );

    /* store pointer to the preload info into the appropriate main binary variable */
    wine_main_preload_info = pdlsym( mod, "wine_main_preload_info" );
    if (wine_main_preload_info) *wine_main_preload_info = preload_info;
    else wld_printf( "wine_main_preload_info not found\n" );

    if (!pdladdr( wine_main_preload_info, &info ) || !(mh = info.dli_fbase))
        fatal_error( "%s: could not find mach header\n", argv[1] );
    if (!(entry = get_entry_point( mh, p_dyld_get_image_slide(mh), is_unix_thread )))
        fatal_error( "%s: could not find entry point\n", argv[1] );

    return entry;
}

#endif /* __APPLE__ */
