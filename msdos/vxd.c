/*
 * VxD emulation
 *
 * Copyright 1995 Anand Kumria
 */

#include <fcntl.h>
#include <memory.h>
#include "windows.h"
#include "winbase.h"
#include "msdos.h"
#include "miscemu.h"
#include "selectors.h"
#include "module.h"
#include "task.h"
#include "process.h"
#include "file.h"
#include "debug.h"


#define VXD_BARF(context,name) \
    DUMP( "vxd %s: unknown/not implemented parameters:\n" \
                     "vxd %s: AX %04x, BX %04x, CX %04x, DX %04x, " \
                     "SI %04x, DI %04x, DS %04x, ES %04x\n", \
             (name), (name), AX_reg(context), BX_reg(context), \
             CX_reg(context), DX_reg(context), SI_reg(context), \
             DI_reg(context), (WORD)DS_reg(context), (WORD)ES_reg(context) )


static WORD VXD_WinVersion(void)
{
    WORD version = LOWORD(GetVersion16());
    return (version >> 8) | (version << 8);
}

/***********************************************************************
 *           VXD_VMM
 */
void VXD_VMM ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] VMM  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    case 0x026d: /* Get_Debug_Flag '/m' */
    case 0x026e: /* Get_Debug_Flag '/n' */
        AL_reg(context) = 0;
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VMM" );
    }
}

/***********************************************************************
 *           VXD_PageFile
 */
void WINAPI VXD_PageFile( CONTEXT *context )
{
    unsigned	service = AX_reg(context);

    /* taken from Ralf Brown's Interrupt List */

    TRACE(vxd,"[%04x] PageFile\n", (UINT16)service );

    switch(service)
    {
    case 0x00: /* get version, is this windows version? */
	TRACE(vxd,"returning version\n");
        AX_reg(context) = VXD_WinVersion();
	RESET_CFLAG(context);
	break;

    case 0x01: /* get swap file info */
	TRACE(vxd,"VxD PageFile: returning swap file info\n");
	AX_reg(context) = 0x00; /* paging disabled */
	ECX_reg(context) = 0;   /* maximum size of paging file */	
	/* FIXME: do I touch DS:SI or DS:DI? */
	RESET_CFLAG(context);
	break;

    case 0x02: /* delete permanent swap on exit */
	TRACE(vxd,"VxD PageFile: supposed to delete swap\n");
	RESET_CFLAG(context);
	break;

    case 0x03: /* current temporary swap file size */
	TRACE(vxd,"VxD PageFile: what is current temp. swap size\n");
	RESET_CFLAG(context);
	break;

    case 0x04: /* read or write?? INTERRUP.D */
    case 0x05: /* cancel?? INTERRUP.D */
    case 0x06: /* test I/O valid INTERRUP.D */
    default:
	VXD_BARF( context, "pagefile" );
	break;
    }
}

/***********************************************************************
 *           VXD_Reboot
 */
void VXD_Reboot ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] VMM  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "REBOOT" );
    }
}

/***********************************************************************
 *           VXD_VDD
 */
void VXD_VDD ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] VDD  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VDD" );
    }
}

/***********************************************************************
 *           VXD_VMD
 */
void VXD_VMD ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] VMD  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VMD" );
    }
}

/***********************************************************************
 *           VXD_Shell
 */
void WINAPI VXD_Shell( CONTEXT *context )
{
    unsigned	service = DX_reg(context);

    TRACE(vxd,"[%04x] Shell\n", (UINT16)service);

    switch (service) /* Ralf Brown says EDX, but I use DX instead */
    {
    case 0x0000:
	TRACE(vxd,"returning version\n");
        AX_reg(context) = VXD_WinVersion();
	EBX_reg(context) = 1; /* system VM Handle */
	break;

    case 0x0001:
    case 0x0002:
    case 0x0003:
    case 0x0004:
    case 0x0005:
	VXD_BARF( context, "shell" );
	break;

    case 0x0006: /* SHELL_Get_VM_State */
	TRACE(vxd,"VxD Shell: returning VM state\n");
	/* Actually we don't, not yet. We have to return a structure
         * and I am not to sure how to set it up and return it yet,
         * so for now let's do nothing. I can (hopefully) get this
         * by the next release
         */
	/* RESET_CFLAG(context); */
	break;

    case 0x0007:
    case 0x0008:
    case 0x0009:
    case 0x000A:
    case 0x000B:
    case 0x000C:
    case 0x000D:
    case 0x000E:
    case 0x000F:
    case 0x0010:
    case 0x0011:
    case 0x0012:
    case 0x0013:
    case 0x0014:
    case 0x0015:
    case 0x0016:
	VXD_BARF( context, "SHELL" );
	break;

    /* the new Win95 shell API */
    case 0x0100:     /* get version */
        AX_reg(context) = VXD_WinVersion();
	break;

    case 0x0104:   /* retrieve Hook_Properties list */
    case 0x0105:   /* call Hook_Properties callbacks */
	VXD_BARF( context, "SHELL" );
	break;

    case 0x0106:   /* install timeout callback */
	TRACE( vxd, "VxD Shell: ignoring shell callback (%ld sec.)\n",
                    EBX_reg( context ) );
	SET_CFLAG(context);
	break;

    case 0x0107:   /* get version of any VxD */
    default:
	VXD_BARF( context, "SHELL" );
	break;
    }
}


/***********************************************************************
 *           VXD_Comm
 */
void WINAPI VXD_Comm( CONTEXT *context )
{
    unsigned	service = AX_reg(context);

    TRACE(vxd,"[%04x] Comm\n", (UINT16)service);

    switch (service)
    {
    case 0x0000: /* get version */
	TRACE(vxd,"returning version\n");
        AX_reg(context) = VXD_WinVersion();
	RESET_CFLAG(context);
	break;

    case 0x0001: /* set port global */
    case 0x0002: /* get focus */
    case 0x0003: /* virtualise port */
    default:
        VXD_BARF( context, "comm" );
    }
}

/***********************************************************************
 *           VXD_Timer
 */
void VXD_Timer( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] Virtual Timer\n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
	AX_reg(context) = VXD_WinVersion();
	RESET_CFLAG(context);
	break;

    case 0x0100: /* clock tick time, in 840nsecs */
	EAX_reg(context) = GetTickCount();

	EDX_reg(context) = EAX_reg(context) >> 22;
	EAX_reg(context) <<= 10; /* not very precise */
	break;

    case 0x0101: /* current Windows time, msecs */
    case 0x0102: /* current VM time, msecs */
	EAX_reg(context) = GetTickCount();
	break;

    default:
	VXD_BARF( context, "VTD" );
    }
}

/***********************************************************************
 *           VXD_TimerAPI
 */
static DWORD System_Time = 0;
static WORD  System_Time_Selector = 0;
static void  System_Time_Tick( WORD timer ) { System_Time += 55; }
void VXD_TimerAPI ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] TimerAPI  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    case 0x0009: /* get system time selector */
        if ( !System_Time_Selector )
        {
            System_Time_Selector = SELECTOR_AllocBlock( &System_Time, sizeof(DWORD), 
                                                        SEGMENT_DATA, FALSE, TRUE );
            CreateSystemTimer( 55, System_Time_Tick );
        }

        AX_reg(context) = System_Time_Selector;
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "VTDAPI" );
    }
}

/***********************************************************************
 *           VXD_ConfigMG
 */
void VXD_ConfigMG ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] ConfigMG  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "CONFIGMG" );
    }
}

/***********************************************************************
 *           VXD_Enable
 */
void VXD_Enable ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] Enable  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "ENABLE" );
    }
}

/***********************************************************************
 *           VXD_APM
 */
void VXD_APM ( CONTEXT *context )
{
    unsigned service = AX_reg(context);

    TRACE(vxd,"[%04x] APM  \n", (UINT16)service);

    switch(service)
    {
    case 0x0000: /* version */
        AX_reg(context) = VXD_WinVersion();
        RESET_CFLAG(context);
        break;

    default:
        VXD_BARF( context, "APM" );
    }
}

/***********************************************************************
 *           VXD_Win32s
 *
 * This is an implementation of the services of the Win32s VxD.
 * Since official documentation of these does not seem to be available,
 * certain arguments of some of the services remain unclear.  
 *
 * FIXME: The following services are currently unimplemented: 
 *        Exception handling      (0x01, 0x1C)
 *        Debugger support        (0x0C, 0x14, 0x17)
 *        Low-level memory access (0x02, 0x03, 0x0A, 0x0B)
 *        Memory Statistics       (0x1B)
 *        
 *
 * We have a specific problem running Win32s on Linux (and probably also
 * the other x86 unixes), since Win32s tries to allocate its main 'flat
 * code/data segment' selectors with a base of 0xffff0000 (and limit 4GB).
 * The rationale for this seems to be that they want one the one hand to 
 * be able to leave the Win 3.1 memory (starting with the main DOS memory) 
 * at linear address 0, but want at other hand to have offset 0 of the
 * flat data/code segment point to an unmapped page (to catch NULL pointer
 * accesses). Hence they allocate the flat segments with a base of 0xffff0000
 * so that the Win 3.1 memory area at linear address zero shows up in the
 * flat segments at offset 0x10000 (since linear addresses wrap around at
 * 4GB). To compensate for that discrepancy between flat segment offsets
 * and plain linear addresses, all flat pointers passed between the 32-bit
 * and the 16-bit parts of Win32s are shifted by 0x10000 in the appropriate
 * direction by the glue code (mainly) in W32SKRNL and WIN32S16.
 *
 * The problem for us is now that Linux does not allow a LDT selector with
 * base 0xffff0000 to be created, since it would 'see' a part of the kernel
 * address space. To address this problem we introduce *another* offset:
 * We add 0x10000 to every linear address we get as an argument from Win32s.
 * This means especially that the flat code/data selectors get actually
 * allocated with base 0x0, so that flat offsets and (real) linear addresses
 * do again agree!  In fact, every call e.g. of a Win32s VxD service now
 * has all pointer arguments (which are offsets in the flat data segement)
 * first reduced by 0x10000 by the W32SKRNL glue code, and then again
 * increased by 0x10000 by *our* code.
 *
 * Note that to keep everything consistent, this offset has to be applied by
 * every Wine function that operates on 'linear addresses' passed to it by
 * Win32s. Fortunately, since Win32s does not directly call any Wine 32-bit
 * API routines, this affects only two locations: this VxD and the DPMI
 * handler. (NOTE: Should any Win32s application pass a linear address to
 * any routine apart from those, e.g. some other VxD handler, that code
 * would have to take the offset into account as well!)
 *
 * The application of the offset is triggered by marking the current process
 * as a Win32s process by setting the PDB32_WIN32S_PROC flag in the process
 * database. This is done the first time any application calls the GetVersion()
 * service of the Win32s VxD. (Note that the flag is never removed.)
 * 
 */

void VXD_Win32s( CONTEXT *context )
{
    switch (AX_reg(context))
    {
    case 0x0000: /* Get Version */
        /*
         * Input:   None
         *
         * Output:  EAX: LoWord: Win32s Version (1.30)
         *               HiWord: VxD Version (200)
         *
         *          EBX: Build (172)
         *
         *          ECX: ???   (1)
         *
         *          EDX: Debugging Flags
         *
         *          EDI: Error Flag 
         *               0 if OK,
         *               1 if VMCPD VxD not found
         */

        TRACE(vxd, "GetVersion()\n");
        
	EAX_reg(context) = VXD_WinVersion() | (200 << 16);
        EBX_reg(context) = 0;
        ECX_reg(context) = 0;
        EDX_reg(context) = 0;
        EDI_reg(context) = 0;

        /* 
         * If this is the first time we are called for this process,
         * hack the memory image of WIN32S16 so that it doesn't try
         * to access the GDT directly ...
         *
         * The first code segment of WIN32S16 (version 1.30) contains 
         * an unexported function somewhere between the exported functions
         * SetFS and StackLinearToSegmented that tries to find a selector
         * in the LDT that maps to the memory image of the LDT itself.
         * If it succeeds, it stores this selector into a global variable
         * which will be used to speed up execution by using this selector
         * to modify the LDT directly instead of using the DPMI calls.
         *
         * To perform this search of the LDT, this function uses the
         * sgdt and sldt instructions to find the linear address of
         * the (GDT and then) LDT. While those instructions themselves
         * execute without problem, the linear address that sgdt returns
         * points (at least under Linux) to the kernel address space, so
         * that any subsequent access leads to a segfault.
         *
         * Fortunately, WIN32S16 still contains as a fallback option the
         * mechanism of using DPMI calls to modify LDT selectors instead
         * of direct writes to the LDT. Thus we can circumvent the problem
         * by simply replacing the first byte of the offending function
         * with an 'retf' instruction. This means that the global variable
         * supposed to contain the LDT alias selector will remain zero,
         * and hence WIN32S16 will fall back to using DPMI calls.
         *
         * The heuristic we employ to _find_ that function is as follows:
         * We search between the addresses of the exported symbols SetFS
         * and StackLinearToSegmented for the byte sequence '0F 01 04'
         * (this is the opcode of 'sgdt [si]'). We then search backwards
         * from this address for the last occurrance of 'CB' (retf) that marks
         * the end of the preceeding function. The following byte (which
         * should now be the first byte of the function we are looking for)
         * will be replaced by 'CB' (retf).
         *
         * This heuristic works for the retail as well as the debug version
         * of Win32s version 1.30. For versions earlier than that this
         * hack should not be necessary at all, since the whole mechanism
         * ('PERF130') was introduced only in 1.30 to improve the overall
         * performance of Win32s.
         */

        if (!(PROCESS_Current()->flags & PDB32_WIN32S_PROC))
        {
            HMODULE16 hModule = GetModuleHandle16("win32s16");
            SEGPTR func1 = (SEGPTR)WIN32_GetProcAddress16(hModule, "SetFS");
            SEGPTR func2 = (SEGPTR)WIN32_GetProcAddress16(hModule, 
                                                  "StackLinearToSegmented");

            if (   hModule && func1 && func2 
                && SELECTOROF(func1) == SELECTOROF(func2))
            {
                BYTE *start = PTR_SEG_TO_LIN(func1);
                BYTE *end   = PTR_SEG_TO_LIN(func2);
                BYTE *p, *retv = NULL;
                int found = 0;

                for (p = start; p < end; p++)
                    if (*p == 0xCB) found = 0, retv = p;
                    else if (*p == 0x0F) found = 1;
                    else if (*p == 0x01 && found == 1) found = 2;
                    else if (*p == 0x04 && found == 2) { found = 3; break; }
                    else found = 0;

                if (found == 3 && retv)
                {
                    TRACE(vxd, "PERF130 hack: "
                               "Replacing byte %02X at offset %04X:%04X\n",
                               *(retv+1), SELECTOROF(func1), 
                                          OFFSETOF(func1) + retv+1-start);

                    *(retv+1) = (BYTE)0xCB;
                }
            }
        }

        /* 
         * Mark process as Win32s, so that subsequent DPMI calls
         * will perform the W32S_APP2WINE/W32S_WINE2APP address shift.
         */

        PROCESS_Current()->flags |= PDB32_WIN32S_PROC;
	break;


    case 0x0001: /* Install Exception Handling */
        /*
         * Input:   EBX: Flat address of W32SKRNL Exception Data
         *
         *          ECX: LoWord: Flat Code Selector
         *               HiWord: Flat Data Selector
         *
         *          EDX: Flat address of W32SKRNL Exception Handler 
         *               (this is equal to W32S_BackTo32 + 0x40)
         *
         *          ESI: SEGPTR KERNEL.HASGPHANDLER
         *
         *          EDI: SEGPTR phCurrentTask (KERNEL.THHOOK + 0x10)
         *
         * Output:  EAX: 0 if OK
         */

        TRACE(vxd, "[0001] EBX=%lx ECX=%lx EDX=%lx ESI=%lx EDI=%lx\n", 
                   EBX_reg(context), ECX_reg(context), EDX_reg(context),
                   ESI_reg(context), EDI_reg(context));

        /* FIXME */

        EAX_reg(context) = 0;
        break;


    case 0x0002: /* Set Page Access Flags */
        /*
         * Input:   EBX: New access flags
         *               Bit 2: User Page if set, Supervisor Page if clear
         *               Bit 1: Read-Write if set, Read-Only if clear
         *
         *          ECX: Size of memory area to change
         *
         *          EDX: Flat start address of memory area
         *
         * Output:  EAX: Size of area changed
         */

        TRACE(vxd, "[0002] EBX=%lx ECX=%lx EDX=%lx\n", 
                   EBX_reg(context), ECX_reg(context), EDX_reg(context));

        /* FIXME */

        EAX_reg(context) = ECX_reg(context);
        break;


    case 0x0003: /* Get Page Access Flags */
        /*
         * Input:   EDX: Flat address of page to query
         *
         * Output:  EAX: Page access flags
         *               Bit 2: User Page if set, Supervisor Page if clear
         *               Bit 1: Read-Write if set, Read-Only if clear
         */

        TRACE(vxd, "[0003] EDX=%lx\n", EDX_reg(context));

        /* FIXME */

        EAX_reg(context) = 6;
        break;


    case 0x0004: /* Map Module */
        /*
         * Input:   ECX: IMTE (offset in Module Table) of new module
         *
         *          EDX: Flat address of Win32s Module Table
         *
         * Output:  EAX: 0 if OK
         */

    if (!EDX_reg(context) || CX_reg(context) == 0xFFFF)
    {
        TRACE(vxd, "MapModule: Initialization call\n");
        EAX_reg(context) = 0;
    }
    else
    {
        /*
         * Structure of a Win32s Module Table Entry:
         */
        struct Win32sModule
        {
            DWORD  flags;
            DWORD  flatBaseAddr;
            LPCSTR moduleName;
            LPCSTR pathName;
            LPCSTR unknown;
            LPBYTE baseAddr;
            DWORD  hModule;
            DWORD  relocDelta;
        };

        /* 
         * Note: This function should set up a demand-paged memory image 
         *       of the given module. Since mmap does not allow file offsets
         *       not aligned at 1024 bytes, we simply load the image fully
         *       into memory.
         */

        struct Win32sModule *moduleTable = 
                            (struct Win32sModule *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        struct Win32sModule *module = moduleTable + ECX_reg(context);

        IMAGE_NT_HEADERS *nt_header = PE_HEADER(module->baseAddr);
        IMAGE_SECTION_HEADER *pe_seg = PE_SECTIONS(module->baseAddr);

        HFILE32 image = _lopen32(module->pathName, OF_READ);
        BOOL32 error = (image == INVALID_HANDLE_VALUE32);
        UINT32 i;

        TRACE(vxd, "MapModule: Loading %s\n", module->pathName);

        for (i = 0; 
             !error && i < nt_header->FileHeader.NumberOfSections; 
             i++, pe_seg++)
            if(!(pe_seg->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA))
            {
                DWORD  off  = pe_seg->PointerToRawData;
                DWORD  len  = pe_seg->SizeOfRawData;
                LPBYTE addr = module->baseAddr + pe_seg->VirtualAddress;

                TRACE(vxd, "MapModule: "
                           "Section %d at %08lx from %08lx len %08lx\n", 
                           i, (DWORD)addr, off, len);

                if (   _llseek32(image, off, SEEK_SET) != off
                    || _lread32(image, addr, len) != len)
                    error = TRUE;
            }
        
        _lclose32(image);

        if (error)
            ERR(vxd, "MapModule: Unable to load %s\n", module->pathName);

        else if (module->relocDelta != 0)
        {
            IMAGE_DATA_DIRECTORY *dir = nt_header->OptionalHeader.DataDirectory
                                      + IMAGE_DIRECTORY_ENTRY_BASERELOC;
            IMAGE_BASE_RELOCATION *r = (IMAGE_BASE_RELOCATION *)
                (dir->Size? module->baseAddr + dir->VirtualAddress : 0);

            TRACE(vxd, "MapModule: Reloc delta %08lx\n", module->relocDelta);

            while (r && r->VirtualAddress)
            {
                LPBYTE page  = module->baseAddr + r->VirtualAddress;
                int    count = (r->SizeOfBlock - 8) / 2;

                TRACE(vxd, "MapModule: %d relocations for page %08lx\n", 
                           count, (DWORD)page);

                for(i = 0; i < count; i++)
                {
                    int offset = r->TypeOffset[i] & 0xFFF;
                    int type   = r->TypeOffset[i] >> 12;
                    switch(type)
                    {
                    case IMAGE_REL_BASED_ABSOLUTE: 
                        break;
                    case IMAGE_REL_BASED_HIGH:
                        *(WORD *)(page+offset) += HIWORD(module->relocDelta);
                        break;
                    case IMAGE_REL_BASED_LOW:
                        *(WORD *)(page+offset) += LOWORD(module->relocDelta);
                        break;
                    case IMAGE_REL_BASED_HIGHLOW:
                        *(DWORD*)(page+offset) += module->relocDelta;
                        break;
                    default:
                        WARN(vxd, "MapModule: Unsupported fixup type\n");
                        break;
                    }
                }

                r = (IMAGE_BASE_RELOCATION *)((LPBYTE)r + r->SizeOfBlock);
            }
        }

        EAX_reg(context) = 0;
        RESET_CFLAG(context);
    }
    break;


    case 0x0005: /* UnMap Module */
        /*
         * Input:   EDX: Flat address of module image 
         *
         * Output:  EAX: 1 if OK
         */
        
        TRACE(vxd, "UnMapModule: %lx\n", (DWORD)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET));

        /* As we didn't map anything, there's nothing to unmap ... */

        EAX_reg(context) = 1;
        break;


    case 0x0006: /* VirtualAlloc */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv     [out] Flat base address of allocated region
         *   LPVOID base     [in]  Flat address of region to reserve/commit
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of allocation
         *   DWORD  prot     [in]  Type of access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD  size   = stack[2];
        DWORD  type   = stack[3];
        DWORD  prot   = stack[4];
        DWORD  result;

        TRACE(vxd, "VirtualAlloc(%lx, %lx, %lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, size, type, prot);

        if (type & 0x80000000)
        {
            WARN(vxd, "VirtualAlloc: strange type %lx\n", type);
            type &= 0x7fffffff;
        }

        if (!base && (type & MEM_COMMIT) && prot == PAGE_READONLY)
        {
            WARN(vxd, "VirtualAlloc: NLS hack, allowing write access!\n");
            prot = PAGE_READWRITE;
        }

        result = (DWORD)VirtualAlloc(base, size, type, prot);

        if (W32S_WINE2APP(result, W32S_OFFSET))
            *retv            = W32S_WINE2APP(result, W32S_OFFSET),
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = 0,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0007: /* VirtualFree */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of operation
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD  size   = stack[2];
        DWORD  type   = stack[3];
        DWORD  result;

        TRACE(vxd, "VirtualFree(%lx, %lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, size, type);

        result = VirtualFree(base, size, type);

        if (result)
            *retv            = TRUE,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0008: /* VirtualProtect */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  new_prot [in]  Desired access protection
         *   DWORD *old_prot [out] Previous access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv     = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base     = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD  size     = stack[2];
        DWORD  new_prot = stack[3];
        DWORD *old_prot = (DWORD *)W32S_APP2WINE(stack[4], W32S_OFFSET);
        DWORD  result;

        TRACE(vxd, "VirtualProtect(%lx, %lx, %lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, size, new_prot, (DWORD)old_prot);

        result = VirtualProtect(base, size, new_prot, old_prot);

        if (result)
            *retv            = TRUE,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0009: /* VirtualQuery */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv                     [out] Nr. bytes returned
         *   LPVOID base                     [in]  Flat address of region
         *   LPMEMORY_BASIC_INFORMATION info [out] Info buffer
         *   DWORD  len                      [in]  Size of buffer
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        LPMEMORY_BASIC_INFORMATION info = 
                        (LPMEMORY_BASIC_INFORMATION)W32S_APP2WINE(stack[2], W32S_OFFSET);
        DWORD  len    = stack[3];
        DWORD  result;

        TRACE(vxd, "VirtualQuery(%lx, %lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, (DWORD)info, len);

        result = VirtualQuery(base, info, len);

        *retv            = result;
        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    case 0x000A: /* SetVirtMemProcess */
        /*
         * Input:   ECX: Process Handle
         *
         *          EDX: Flat address of region
         *
         * Output:  EAX: NtStatus
         */

        TRACE(vxd, "[000a] ECX=%lx EDX=%lx\n",
                   ECX_reg(context), EDX_reg(context));

        /* FIXME */

        EAX_reg(context) = STATUS_SUCCESS;
        break;


    case 0x000B: /* ??? some kind of cleanup */
        /*
         * Input:   ECX: Process Handle
         *
         * Output:  EAX: NtStatus
         */

        TRACE(vxd, "[000b] ECX=%lx\n", ECX_reg(context));

        /* FIXME */

        EAX_reg(context) = STATUS_SUCCESS;
        break;


    case 0x000C: /* Set Debug Flags */
        /*
         * Input:   EDX: Debug Flags
         *
         * Output:  EDX: Previous Debug Flags
         */

        FIXME(vxd, "[000c] EDX=%lx\n", EDX_reg(context));

        /* FIXME */

        EDX_reg(context) = 0;
        break;


    case 0x000D: /* NtCreateSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   HANDLE32 *retv      [out] Handle of Section created
         *   DWORD  flags1       [in]  (?? unknown ??)
         *   DWORD  atom         [in]  Name of Section to create
         *   LARGE_INTEGER *size [in]  Size of Section
         *   DWORD  protect      [in]  Access protection
         *   DWORD  flags2       [in]  (?? unknown ??)
         *   HFILE32 hFile       [in]  Handle of file to map
         *   DWORD  psp          [in]  (Win32s: PSP that hFile belongs to)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)   W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        HANDLE32 *retv  = (HANDLE32 *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        DWORD  flags1   = stack[1];
        DWORD  atom     = stack[2];
        LARGE_INTEGER *size = (LARGE_INTEGER *)W32S_APP2WINE(stack[3], W32S_OFFSET);
        DWORD  protect  = stack[4];
        DWORD  flags2   = stack[5];
        HFILE32 hFile   = FILE_GetHandle32(stack[6]);
        DWORD  psp      = stack[7];

        HANDLE32 result = INVALID_HANDLE_VALUE32;
        char name[128];

        TRACE(vxd, "NtCreateSection(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)retv, flags1, atom, (DWORD)size, protect, flags2,
                   (DWORD)hFile, psp);

        if (!atom || GlobalGetAtomName32A(atom, name, sizeof(name)))
        {
            TRACE(vxd, "NtCreateSection: name=%s\n", atom? name : NULL);

            result = CreateFileMapping32A(hFile, NULL, protect, 
                                          size? size->HighPart : 0, 
                                          size? size->LowPart  : 0, 
                                          atom? name : NULL);
        }

        if (result == INVALID_HANDLE_VALUE32)
            WARN(vxd, "NtCreateSection: failed!\n");
        else
            TRACE(vxd, "NtCreateSection: returned %lx\n", (DWORD)result);

        if (result != INVALID_HANDLE_VALUE32)
            *retv            = result,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = result,
            EAX_reg(context) = STATUS_NO_MEMORY;   /* FIXME */
    }
    break;


    case 0x000E: /* NtOpenSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   HANDLE32 *retv  [out] Handle of Section opened
         *   DWORD  protect  [in]  Access protection
         *   DWORD  atom     [in]  Name of Section to create
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        HANDLE32 *retv  = (HANDLE32 *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        DWORD  protect  = stack[1];
        DWORD  atom     = stack[2];

        HANDLE32 result = INVALID_HANDLE_VALUE32;
        char name[128];

        TRACE(vxd, "NtOpenSection(%lx, %lx, %lx)\n", 
                   (DWORD)retv, protect, atom);

        if (atom && GlobalGetAtomName32A(atom, name, sizeof(name)))
        {
            TRACE(vxd, "NtOpenSection: name=%s\n", name);

            result = OpenFileMapping32A(protect, FALSE, name);
        }

        if (result == INVALID_HANDLE_VALUE32)
            WARN(vxd, "NtOpenSection: failed!\n");
        else
            TRACE(vxd, "NtOpenSection: returned %lx\n", (DWORD)result);

        if (result != INVALID_HANDLE_VALUE32)
            *retv            = result,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = result,
            EAX_reg(context) = STATUS_NO_MEMORY;   /* FIXME */
    }
    break;


    case 0x000F: /* NtCloseSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   HANDLE32 handle  [in]  Handle of Section to close
         *   DWORD *id        [out] Unique ID  (?? unclear ??)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        HANDLE32 handle = stack[0];
        DWORD *id       = (DWORD *)W32S_APP2WINE(stack[1], W32S_OFFSET);

        TRACE(vxd, "NtCloseSection(%lx, %lx)\n", (DWORD)handle, (DWORD)id);

        CloseHandle(handle);
        if (id) *id = 0; /* FIXME */

        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    case 0x0010: /* NtDupSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   HANDLE32 handle  [in]  Handle of Section to duplicate
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack    = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        HANDLE32 handle = stack[0];
        HANDLE32 new_handle;

        TRACE(vxd, "NtDupSection(%lx)\n", (DWORD)handle);
 
        DuplicateHandle( GetCurrentProcess(), handle,
                         GetCurrentProcess(), &new_handle,
                         0, FALSE, DUPLICATE_SAME_ACCESS );
        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    case 0x0011: /* NtMapViewOfSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   HANDLE32 SectionHandle       [in]     Section to be mapped
         *   DWORD    ProcessHandle       [in]     Process to be mapped into
         *   DWORD *  BaseAddress         [in/out] Address to be mapped at
         *   DWORD    ZeroBits            [in]     (?? unclear ??)
         *   DWORD    CommitSize          [in]     (?? unclear ??)
         *   LARGE_INTEGER *SectionOffset [in]     Offset within section
         *   DWORD *  ViewSize            [in]     Size of view
         *   DWORD    InheritDisposition  [in]     (?? unclear ??)
         *   DWORD    AllocationType      [in]     (?? unclear ??)
         *   DWORD    Protect             [in]     Access protection
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *  stack          = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        HANDLE32 SectionHandle  = stack[0];
        DWORD    ProcessHandle  = stack[1]; /* ignored */
        DWORD *  BaseAddress    = (DWORD *)W32S_APP2WINE(stack[2], W32S_OFFSET);
        DWORD    ZeroBits       = stack[3];
        DWORD    CommitSize     = stack[4];
        LARGE_INTEGER *SectionOffset = (LARGE_INTEGER *)W32S_APP2WINE(stack[5], W32S_OFFSET);
        DWORD *  ViewSize       = (DWORD *)W32S_APP2WINE(stack[6], W32S_OFFSET);
        DWORD    InheritDisposition = stack[7];
        DWORD    AllocationType = stack[8];
        DWORD    Protect        = stack[9];

        LPBYTE address = (LPBYTE)(BaseAddress?
			W32S_APP2WINE(*BaseAddress, W32S_OFFSET) : 0);
        DWORD  access = 0, result;

        switch (Protect & ~(PAGE_GUARD|PAGE_NOCACHE))
        {
            case PAGE_READONLY:           access = FILE_MAP_READ;  break;
            case PAGE_READWRITE:          access = FILE_MAP_WRITE; break;
            case PAGE_WRITECOPY:          access = FILE_MAP_COPY;  break;

            case PAGE_EXECUTE_READ:       access = FILE_MAP_READ;  break;
            case PAGE_EXECUTE_READWRITE:  access = FILE_MAP_WRITE; break;
            case PAGE_EXECUTE_WRITECOPY:  access = FILE_MAP_COPY;  break;
        }

        TRACE(vxd, "NtMapViewOfSection"
                   "(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n",
                   (DWORD)SectionHandle, ProcessHandle, (DWORD)BaseAddress, 
                   ZeroBits, CommitSize, (DWORD)SectionOffset, (DWORD)ViewSize,
                   InheritDisposition, AllocationType, Protect);
        TRACE(vxd, "NtMapViewOfSection: "
                   "base=%lx, offset=%lx, size=%lx, access=%lx\n", 
                   (DWORD)address, SectionOffset? SectionOffset->LowPart : 0, 
                   ViewSize? *ViewSize : 0, access);

        result = (DWORD)MapViewOfFileEx(SectionHandle, access, 
                            SectionOffset? SectionOffset->HighPart : 0, 
                            SectionOffset? SectionOffset->LowPart  : 0,
                            ViewSize? *ViewSize : 0, address);

        TRACE(vxd, "NtMapViewOfSection: result=%lx\n", result);

        if (W32S_WINE2APP(result, W32S_OFFSET))
        {
            if (BaseAddress) *BaseAddress = W32S_WINE2APP(result, W32S_OFFSET);
            EAX_reg(context) = STATUS_SUCCESS;
        }
        else
            EAX_reg(context) = STATUS_NO_MEMORY; /* FIXME */
    }
    break;


    case 0x0012: /* NtUnmapViewOfSection */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   DWORD  ProcessHandle  [in]  Process (defining address space)
         *   LPBYTE BaseAddress    [in]  Base address of view to be unmapped
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack          = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD  ProcessHandle  = stack[0]; /* ignored */
        LPBYTE BaseAddress    = (LPBYTE)W32S_APP2WINE(stack[1], W32S_OFFSET);

        TRACE(vxd, "NtUnmapViewOfSection(%lx, %lx)\n", 
                   ProcessHandle, (DWORD)BaseAddress);

        UnmapViewOfFile(BaseAddress);

        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    case 0x0013: /* NtFlushVirtualMemory */
        /* 
         * Input:   EDX: Flat address of arguments on stack
         * 
         *   DWORD   ProcessHandle  [in]  Process (defining address space)
         *   LPBYTE *BaseAddress    [in?] Base address of range to be flushed
         *   DWORD  *ViewSize       [in?] Number of bytes to be flushed
         *   DWORD  *unknown        [???] (?? unknown ??)
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack          = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD  ProcessHandle  = stack[0]; /* ignored */
        DWORD *BaseAddress    = (DWORD *)W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD *ViewSize       = (DWORD *)W32S_APP2WINE(stack[2], W32S_OFFSET);
        DWORD *unknown        = (DWORD *)W32S_APP2WINE(stack[3], W32S_OFFSET);
        
        LPBYTE address = (LPBYTE)(BaseAddress? W32S_APP2WINE(*BaseAddress, W32S_OFFSET) : 0);
        DWORD  size    = ViewSize? *ViewSize : 0;

        TRACE(vxd, "NtFlushVirtualMemory(%lx, %lx, %lx, %lx)\n", 
                   ProcessHandle, (DWORD)BaseAddress, (DWORD)ViewSize, 
                   (DWORD)unknown);
        TRACE(vxd, "NtFlushVirtualMemory: base=%lx, size=%lx\n", 
                   (DWORD)address, size);

        FlushViewOfFile(address, size);

        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    case 0x0014: /* Get/Set Debug Registers */
        /*
         * Input:   ECX: 0 if Get, 1 if Set
         *
         *          EDX: Get: Flat address of buffer to receive values of
         *                    debug registers DR0 .. DR7
         *               Set: Flat address of buffer containing values of
         *                    debug registers DR0 .. DR7 to be set
         * Output:  None
         */

        FIXME(vxd, "[0014] ECX=%lx EDX=%lx\n", 
                   ECX_reg(context), EDX_reg(context));

        /* FIXME */
        break;


    case 0x0015: /* Set Coprocessor Emulation Flag */
        /*
         * Input:   EDX: 0 to deactivate, 1 to activate coprocessor emulation
         *
         * Output:  None
         */

        TRACE(vxd, "[0015] EDX=%lx\n", EDX_reg(context));

        /* We don't care, as we always have a coprocessor anyway */
        break;


    case 0x0016: /* Init Win32S VxD PSP */
        /*
         * If called to query required PSP size:
         *
         *     Input:  EBX: 0
         *     Output: EDX: Required size of Win32s VxD PSP
         *
         * If called to initialize allocated PSP:
         *
         *     Input:  EBX: LoWord: Selector of Win32s VxD PSP
         *                  HiWord: Paragraph of Win32s VxD PSP (DOSMEM)
         *     Output: None
         */
 
        if (EBX_reg(context) == 0)
            EDX_reg(context) = 0x80;
        else
        {
            PDB *psp = PTR_SEG_OFF_TO_LIN(BX_reg(context), 0);
            psp->nbFiles = 32;
            psp->fileHandlesPtr = MAKELONG(HIWORD(EBX_reg(context)), 0x5c);
            memset((LPBYTE)psp + 0x5c, '\xFF', 32);
        }
        break;


    case 0x0017: /* Set Break Point */
        /*
         * Input:   EBX: Offset of Break Point
         *          CX:  Selector of Break Point
         *
         * Output:  None
         */

        FIXME(vxd, "[0017] EBX=%lx CX=%x\n", 
                   EBX_reg(context), CX_reg(context));

        /* FIXME */
        break;


    case 0x0018: /* VirtualLock */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of range to lock
         *   DWORD  size     [in]  Size of range
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD  size   = stack[2];
        DWORD  result;

        TRACE(vxd, "VirtualLock(%lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, size);

        result = VirtualLock(base, size);

        if (result)
            *retv            = TRUE,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x0019: /* VirtualUnlock */
        /* 
         * Input:   ECX: Current Process
         *
         *          EDX: Flat address of arguments on stack
         * 
         *   DWORD *retv     [out] TRUE if success, FALSE if failure
         *   LPVOID base     [in]  Flat address of range to unlock
         *   DWORD  size     [in]  Size of range
         *
         * Output:  EAX: NtStatus
         */
    {
        DWORD *stack  = (DWORD *)W32S_APP2WINE(EDX_reg(context), W32S_OFFSET);
        DWORD *retv   = (DWORD *)W32S_APP2WINE(stack[0], W32S_OFFSET);
        LPVOID base   = (LPVOID) W32S_APP2WINE(stack[1], W32S_OFFSET);
        DWORD  size   = stack[2];
        DWORD  result;

        TRACE(vxd, "VirtualUnlock(%lx, %lx, %lx)\n", 
                   (DWORD)retv, (DWORD)base, size);

        result = VirtualUnlock(base, size);

        if (result)
            *retv            = TRUE,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            *retv            = FALSE,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x001A: /* KGetSystemInfo */
        /*
         * Input:   None
         *
         * Output:  ECX:  Start of sparse memory arena
         *          EDX:  End of sparse memory arena
         */

        TRACE(vxd, "KGetSystemInfo()\n");
        
        /*
         * Note: Win32s reserves 0GB - 2GB for Win 3.1 and uses 2GB - 4GB as 
         *       sparse memory arena. We do it the other way around, since 
         *       we have to reserve 3GB - 4GB for Linux, and thus use
         *       0GB - 3GB as sparse memory arena.
         *
         *       FIXME: What about other OSes ?
         */

        ECX_reg(context) = W32S_WINE2APP(0x00000000, W32S_OFFSET);
        EDX_reg(context) = W32S_WINE2APP(0xbfffffff, W32S_OFFSET);
        break;


    case 0x001B: /* KGlobalMemStat */
        /*
         * Input:   ESI: Flat address of buffer to receive memory info
         *
         * Output:  None
         */
    {
        struct Win32sMemoryInfo
        {
            DWORD DIPhys_Count;       /* Total physical pages */
            DWORD DIFree_Count;       /* Free physical pages */
            DWORD DILin_Total_Count;  /* Total virtual pages (private arena) */
            DWORD DILin_Total_Free;   /* Free virtual pages (private arena) */

            DWORD SparseTotal;        /* Total size of sparse arena (bytes ?) */
            DWORD SparseFree;         /* Free size of sparse arena (bytes ?) */
        };

        struct Win32sMemoryInfo *info = 
                       (struct Win32sMemoryInfo *)W32S_APP2WINE(ESI_reg(context), W32S_OFFSET);

        FIXME(vxd, "KGlobalMemStat(%lx)\n", (DWORD)info);

        /* FIXME */
    }
    break;


    case 0x001C: /* Enable/Disable Exceptions */
        /*
         * Input:   ECX: 0 to disable, 1 to enable exception handling
         *
         * Output:  None
         */

        TRACE(vxd, "[001c] ECX=%lx\n", ECX_reg(context));

        /* FIXME */
        break;


    case 0x001D: /* VirtualAlloc called from 16-bit code */
        /* 
         * Input:   EDX: Segmented address of arguments on stack
         * 
         *   LPVOID base     [in]  Flat address of region to reserve/commit
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of allocation
         *   DWORD  prot     [in]  Type of access protection
         *
         * Output:  EAX: NtStatus
         *          EDX: Flat base address of allocated region
         */
    {
        DWORD *stack  = PTR_SEG_OFF_TO_LIN(LOWORD(EDX_reg(context)), 
                                           HIWORD(EDX_reg(context)));
        LPVOID base   = (LPVOID)W32S_APP2WINE(stack[0], W32S_OFFSET);
        DWORD  size   = stack[1];
        DWORD  type   = stack[2];
        DWORD  prot   = stack[3];
        DWORD  result;

        TRACE(vxd, "VirtualAlloc16(%lx, %lx, %lx, %lx)\n", 
                   (DWORD)base, size, type, prot);

        if (type & 0x80000000)
        {
            WARN(vxd, "VirtualAlloc16: strange type %lx\n", type);
            type &= 0x7fffffff;
        }

        result = (DWORD)VirtualAlloc(base, size, type, prot);

        if (W32S_WINE2APP(result, W32S_OFFSET))
            EDX_reg(context) = W32S_WINE2APP(result, W32S_OFFSET),
            EAX_reg(context) = STATUS_SUCCESS;
        else
            EDX_reg(context) = 0,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
	TRACE(vxd, "VirtualAlloc16: returning base %lx\n", EDX_reg(context));
    }
    break;


    case 0x001E: /* VirtualFree called from 16-bit code */
        /* 
         * Input:   EDX: Segmented address of arguments on stack
         * 
         *   LPVOID base     [in]  Flat address of region
         *   DWORD  size     [in]  Size of region
         *   DWORD  type     [in]  Type of operation
         *
         * Output:  EAX: NtStatus
         *          EDX: TRUE if success, FALSE if failure
         */
    {
        DWORD *stack  = PTR_SEG_OFF_TO_LIN(LOWORD(EDX_reg(context)), 
                                           HIWORD(EDX_reg(context)));
        LPVOID base   = (LPVOID)W32S_APP2WINE(stack[0], W32S_OFFSET);
        DWORD  size   = stack[1];
        DWORD  type   = stack[2];
        DWORD  result;

        TRACE(vxd, "VirtualFree16(%lx, %lx, %lx)\n", 
                   (DWORD)base, size, type);

        result = VirtualFree(base, size, type);

        if (result)
            EDX_reg(context) = TRUE,
            EAX_reg(context) = STATUS_SUCCESS;
        else
            EDX_reg(context) = FALSE,
            EAX_reg(context) = STATUS_NO_MEMORY;  /* FIXME */
    }
    break;


    case 0x001F: /* FWorkingSetSize */
        /*
         * Input:   EDX: 0 if Get, 1 if Set
         *
         *          ECX: Get: Buffer to receive Working Set Size
         *               Set: Buffer containing Working Set Size
         *
         * Output:  NtStatus
         */
    {
        DWORD *ptr = (DWORD *)W32S_APP2WINE(ECX_reg(context), W32S_OFFSET);
        BOOL32 set = EDX_reg(context);
        
        TRACE(vxd, "FWorkingSetSize(%lx, %lx)\n", (DWORD)ptr, (DWORD)set);

        if (set)
            /* We do it differently ... */;
        else
            *ptr = 0x100;

        EAX_reg(context) = STATUS_SUCCESS;
    }
    break;


    default:
	VXD_BARF( context, "W32S" );
    }

}

