/* Native RISC-V64 Winelib TEB lookup.
 *
 * The Linux RISC-V psABI reserves tp (x4) for ELF TLS.  Wine's Unix-side
 * ntdll.so already owns a pthread key containing struct thread_data and
 * exports NtCurrentTeb(), which retrieves that thread's TEB.
 *
 * Windows-side Winelib .so modules are not directly linked to ntdll.so;
 * use ELF dynamic symbol lookup to call the existing Unix implementation.
 * No Windows PE binary compatibility is intended by this implementation.
 */


#if defined(__riscv) && __riscv_xlen == 64 && !defined(__WINE_PE_BUILD)

#include <dlfcn.h>
#include <stddef.h>

struct _TEB;

typedef struct _TEB *(*teb_getter_t)(void);

struct _TEB *__wine_riscv64_get_teb(void)
{
    static teb_getter_t getter;
    teb_getter_t func = __atomic_load_n( &getter, __ATOMIC_ACQUIRE );

    if (!func)
    {
        void *symbol = dlsym( RTLD_DEFAULT, "NtCurrentTeb" );

        if (!symbol)
        {
            /* ntdll.so may have been loaded with RTLD_LOCAL.  Its SONAME
             * can still be found without loading a second instance. */
#ifdef RTLD_NOLOAD
            void *handle = dlopen( "ntdll.so", RTLD_NOW | RTLD_NOLOAD );
            if (handle)
            {
                symbol = dlsym( handle, "NtCurrentTeb" );
                dlclose( handle );
            }
#endif
        }

        if (!symbol) return NULL; /* ntdll not initialized yet */

        func = (teb_getter_t)symbol;
        __atomic_store_n( &getter, func, __ATOMIC_RELEASE );
    }

    return func();
}

#endif /* RISC-V64 native Winelib */
