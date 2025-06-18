/*
 * Inline assembly support
 *
 * Copyright 2019 Alexandre Julliard
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

#ifndef __WINE_WINE_ASM_H
#define __WINE_WINE_ASM_H

#if defined(__APPLE__) || (defined(__WINE_PE_BUILD) && defined(__i386__))
# define __ASM_NAME(name) "_" name
#else
# define __ASM_NAME(name) name
#endif

#if defined(__APPLE__)
# define __ASM_LOCAL_LABEL(label) "L" label
#else
# define __ASM_LOCAL_LABEL(label) ".L" label
#endif

#if defined(__GCC_HAVE_DWARF2_CFI_ASM) || ((defined(__APPLE__) || defined(__clang__)) && defined(__GNUC__) && !defined(__SEH__))
# define __ASM_CFI(str) str
#else
# define __ASM_CFI(str)
#endif

#if defined(__arm__) && defined(__ELF__) && defined(__GNUC__) && !defined(__SEH__) && !defined(__ARM_DWARF_EH__)
# define __ASM_EHABI(str) str
#else
# define __ASM_EHABI(str)
#endif

#if defined(__WINE_PE_BUILD) && !defined(__i386__)
# define __ASM_SEH(str) str
#else
# define __ASM_SEH(str)
#endif

#ifdef __WINE_PE_BUILD
# define __ASM_FUNC_TYPE(name) ".def " name "\n\t.scl 2\n\t.type 32\n\t.endef"
#elif defined(__APPLE__)
# define __ASM_FUNC_TYPE(name) ""
#elif defined(__arm__) && defined(__thumb__)
# define __ASM_FUNC_TYPE(name) ".type " name ",%function\n\t.thumb_func"
#elif defined(__arm__) || defined(__aarch64__)
# define __ASM_FUNC_TYPE(name) ".type " name ",%function"
#else
# define __ASM_FUNC_TYPE(name) ".type " name ",@function"
#endif

#ifdef __WINE_PE_BUILD
# define __ASM_GLOBL(name) ".globl " name "\n" name ":"
# define __ASM_FUNC_SIZE(name) ""
#elif defined(__APPLE__)
# define __ASM_GLOBL(name) ".globl " name "\n\t.private_extern " name "\n" name ":"
# define __ASM_FUNC_SIZE(name) ""
#else
# define __ASM_GLOBL(name) ".globl " name "\n\t.hidden " name "\n" name ":"
# define __ASM_FUNC_SIZE(name) ".size " name ",.-" name
#endif

#ifdef __arm64ec_x64__
# define __ASM_FUNC_SECTION(name) ".section .text,\"xr\",discard," name
# define __ASM_FUNC_ALIGN ".balign 16"
#else
# define __ASM_FUNC_SECTION(name) ".text"
# define __ASM_FUNC_ALIGN ".align 4"
#endif

#if !defined(__GNUC__) && !defined(__clang__)
# define __ASM_BLOCK_BEGIN(name) void __asm_dummy_##name(void) {
# define __ASM_BLOCK_END         }
#else
# define __ASM_BLOCK_BEGIN(name)
# define __ASM_BLOCK_END
#endif

#define __ASM_DEFINE_FUNC(name,code)  \
    __ASM_BLOCK_BEGIN(__LINE__) \
    asm( __ASM_FUNC_SECTION(name) "\n\t" \
         __ASM_FUNC_ALIGN "\n\t" \
         __ASM_FUNC_TYPE(name) "\n" \
         __ASM_GLOBL(name) "\n\t" \
         __ASM_SEH(".seh_proc " name "\n\t") \
         __ASM_CFI(".cfi_startproc\n\t") \
         __ASM_EHABI(".fnstart\n\t") \
         code "\n\t" \
         __ASM_CFI(".cfi_endproc\n\t") \
         __ASM_EHABI(".fnend\n\t") \
         __ASM_SEH(".seh_endproc\n\t") \
         __ASM_FUNC_SIZE(name)); \
    __ASM_BLOCK_END

#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(__ASM_NAME(#name),code)

#ifdef _WIN64
#define __ASM_GLOBAL_POINTER(name,value) \
    __ASM_BLOCK_BEGIN(__LINE__) \
    asm( ".data\n\t" \
         ".balign 8\n\t" \
         __ASM_GLOBL(name) "\n\t" \
         ".quad " value "\n\t" \
         ".text" );
    __ASM_BLOCK_END
#else
#define __ASM_GLOBAL_POINTER(name,value) \
    __ASM_BLOCK_BEGIN(__LINE__) \
    asm( ".data\n\t" \
         ".balign 4\n\t" \
         __ASM_GLOBL(name) "\n\t" \
         ".long " value "\n\t" \
         ".text" );
    __ASM_BLOCK_END
#endif

/* import variables */

#ifdef __WINE_PE_BUILD
# ifdef __arm64ec__
#  define __ASM_DEFINE_IMPORT(name) \
     __ASM_GLOBAL_POINTER("__imp_" name, "\"#" name "\"") \
     __ASM_GLOBAL_POINTER("__imp_aux_" name, name)
# else
#  define __ASM_DEFINE_IMPORT(name) __ASM_GLOBAL_POINTER("__imp_" name, name)
# endif
# define __ASM_GLOBAL_IMPORT(name) __ASM_DEFINE_IMPORT(__ASM_NAME(#name))
#else
# define __ASM_GLOBAL_IMPORT(name) /* nothing */
#endif

/* stdcall support */

#ifdef __i386__
# ifdef __WINE_PE_BUILD
#  define __ASM_STDCALL(name,args)  "_" name "@" #args
#  define __ASM_FASTCALL(name,args) "@" name "@" #args
#  define __ASM_STDCALL_IMPORT(name,args) __ASM_DEFINE_IMPORT(__ASM_STDCALL(#name,args))
# else
#  define __ASM_STDCALL(name,args)  __ASM_NAME(name)
#  define __ASM_FASTCALL(name,args) __ASM_NAME("__fastcall_" name)
#  define __ASM_STDCALL_IMPORT(name,args) /* nothing */
# endif
# define __ASM_STDCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(__ASM_STDCALL(#name,args),code)
# define __ASM_FASTCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(__ASM_FASTCALL(#name,args),code)
#endif

/* fastcall support */

#if defined(__i386__) && !defined(__WINE_PE_BUILD)

# define __ASM_USE_FASTCALL_WRAPPER
# define DEFINE_FASTCALL1_WRAPPER(func) \
    __ASM_FASTCALL_FUNC( func, 4, \
                        "popl %eax\n\t"  \
                        "pushl %ecx\n\t" \
                        "pushl %eax\n\t" \
                        "jmp " __ASM_STDCALL(#func,4) )
# define DEFINE_FASTCALL_WRAPPER(func,args) \
    __ASM_FASTCALL_FUNC( func, args, \
                        "popl %eax\n\t"  \
                        "pushl %edx\n\t" \
                        "pushl %ecx\n\t" \
                        "pushl %eax\n\t" \
                        "jmp " __ASM_STDCALL(#func,args) )

#else  /* __i386__ */

# define DEFINE_FASTCALL1_WRAPPER(func) /* nothing */
# define DEFINE_FASTCALL_WRAPPER(func,args) /* nothing */

#endif  /* __i386__ */

/* thiscall support */

#if defined(__i386__) && !defined(__MINGW32__) && (!defined(_MSC_VER) || !defined(__clang__))

# define __ASM_USE_THISCALL_WRAPPER
# ifdef _MSC_VER
#  define DEFINE_THISCALL_WRAPPER(func,args) \
    __declspec(naked) void __thiscall_##func(void) \
    { __asm { \
        pop eax \
        push ecx \
        push eax \
        jmp func \
    } }
# else  /* _MSC_VER */
#  define DEFINE_THISCALL_WRAPPER(func,args) \
    extern void __thiscall_ ## func(void);  \
    __ASM_STDCALL_FUNC( __thiscall_ ## func, args, \
                       "popl %eax\n\t"  \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                        "jmp " __ASM_STDCALL(#func,args) )
# endif  /* _MSC_VER */

# define THISCALL(func) (void *)__thiscall_ ## func
# define THISCALL_NAME(func) __ASM_NAME("__thiscall_" #func)

#else  /* __i386__ */

# define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */
# define THISCALL(func) func
# define THISCALL_NAME(func) __ASM_NAME(#func)

#endif  /* __i386__ */

/* syscall support */

#ifdef __i386__
# ifdef __PIC__
#  define __ASM_SYSCALL_FUNC(id,name,args) \
    __ASM_STDCALL_FUNC( name, args, \
                        "call 1f\n" \
                        "1:\tpopl %eax\n\t" \
                        "movl " __ASM_NAME("__wine_syscall_dispatcher") "-1b(%eax),%edx\n\t" \
                        "movl $(" #id "),%eax\n\t" \
                        "call *%edx\n\t" \
                        "ret $" #args )
#  define DEFINE_SYSCALL_HELPER32()
# else
#  define __ASM_SYSCALL_FUNC(id,name,args) \
    __ASM_STDCALL_FUNC( name, args, \
                        "movl $(" #id "),%eax\n\t" \
                        "movl $" __ASM_NAME("__wine_syscall") ",%edx\n\t" \
                        "call *%edx\n\t" \
                        "ret $" #args )
#  define DEFINE_SYSCALL_HELPER32() \
    __ASM_GLOBAL_FUNC( __wine_syscall, "jmp *(" __ASM_NAME("__wine_syscall_dispatcher") ")" )
# endif
#elif defined __aarch64__
# define __ASM_SYSCALL_FUNC(id,name) \
    __ASM_GLOBAL_FUNC( name, \
                       ".seh_endprologue\n\t" \
                       "mov x8, #(" #id ")\n\t" \
                       "mov x9, x30\n\t" \
                       "ldr x16, 1f\n\t" \
                       "ldr x16, [x16]\n\t" \
                       "blr x16\n\t" \
                       "ret\n" \
                       "1:\t.quad " __ASM_NAME("__wine_syscall_dispatcher") )
#elif defined __arm64ec__
# define __ASM_SYSCALL_FUNC(id,name) \
    asm( ".seh_proc \"#" #name "$hp_target\"\n\t" \
         ".seh_endprologue\n\t" \
         "mov x8, #%0\n\t" \
         "mov x9, x30\n\t" \
         "adrp x16, __wine_syscall_dispatcher\n\t" \
         "ldr x16, [x16, :lo12:__wine_syscall_dispatcher]\n\t" \
         "blr x16\n\t" \
         "ret\n\t" \
         ".seh_endproc" :: "i" (id) )
#elif defined __arm64ec_x64__
# define __ASM_SYSCALL_FUNC(id,name) \
    __ASM_DEFINE_FUNC( "\"EXP+#" #name "\"", \
                       ".seh_endprologue\n\t" \
                       ".byte 0x4c,0x8b,0xd1\n\t" /* 00: movq %rcx,%r10 */ \
                       ".byte 0xb8\n\t"           /* 03: movl $i,%eax */ \
                       ".long (" #id ")\n\t"                            \
                       ".byte 0xf6,0x04,0x25,0x08,0x03,0xfe,0x7f,0x01\n\t" /* 08: testb $1,0x7ffe0308 */ \
                       ".byte 0x75,0x03\n\t"      /* 10: jne 15 */ \
                       ".byte 0x0f,0x05\n\t"      /* 12: syscall */ \
                       ".byte 0xc3\n\t"           /* 14: ret */ \
                       ".byte 0xcd,0x2e\n\t"      /* 15: int $0x2e */ \
                       ".byte 0xc3" )             /* 17: ret */
#elif defined __x86_64__
/* Chromium depends on syscall thunks having the same form as on
 * Windows. For 64-bit systems the only viable form we can emulate is
 * having an int $0x2e fallback. Since actually using an interrupt is
 * expensive, and since for some reason Chromium doesn't actually
 * validate that instruction, we can just put a jmp there instead. */
# define __ASM_SYSCALL_FUNC(id,name) \
    __ASM_GLOBAL_FUNC( name, \
                       __ASM_SEH(".seh_endprologue\n\t") \
                       ".byte 0x4c,0x8b,0xd1\n\t" /* 00: movq %rcx,%r10 */ \
                       ".byte 0xb8\n\t"           /* 03: movl $i,%eax */ \
                       ".long (" #id ")\n\t" \
                       ".byte 0xf6,0x04,0x25,0x08,0x03,0xfe,0x7f,0x01\n\t" /* 08: testb $1,0x7ffe0308 */ \
                       ".byte 0x75,0x03\n\t"      /* 10: jne 15 */ \
                       ".byte 0x0f,0x05\n\t"      /* 12: syscall */ \
                       ".byte 0xc3\n\t"           /* 14: ret */ \
                       ".byte 0xeb,0x01\n\t"      /* 15: jmp 18 */ \
                       ".byte 0xc3\n\t"           /* 17: ret */ \
                       ".byte 0xff,0x14,0x25\n\t" /* 18: callq *(0x7ffe1000) */ \
                       ".long 0x7ffe1000\n\t" \
                       ".byte 0xc3" )             /* 1f: ret */
#elif defined __arm__
# define __ASM_SYSCALL_FUNC(id,name,args) \
    __ASM_GLOBAL_FUNC( name, \
                       "push {r0-r3}\n\t" \
                       ".seh_save_regs {r0-r3}\n\t" \
                       ".seh_endprologue\n\t" \
                       "movw ip, #(" #id ")\n\t" \
                       "mov r3, lr\n\t" \
                       "bl __wine_syscall\n\t" \
                       "add sp, #16\n\t" \
                       "bx lr" )
# define DEFINE_SYSCALL_HELPER32() \
    __ASM_GLOBAL_FUNC( __wine_syscall, \
                       "movw r0, :lower16:__wine_syscall_dispatcher\n\t" \
                       "movt r0, :upper16:__wine_syscall_dispatcher\n\t" \
                       "ldr r0, [r0]\n\t" \
                       "bx r0" )
#endif

#endif  /* __WINE_WINE_ASM_H */
