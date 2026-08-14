/*
 * Stack walking
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
 * Copyright 2004 Eric Pouech
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static DWORD64 WINAPI addr_to_linear(HANDLE hProcess, HANDLE hThread, ADDRESS64* addr)
{
    LDT_ENTRY	le;

    switch (addr->Mode)
    {
    case AddrMode1616:
        if (GetThreadSelectorEntry(hThread, addr->Segment, &le))
            return (le.HighWord.Bits.BaseHi << 24) +
                (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + LOWORD(addr->Offset);
        break;
    case AddrMode1632:
        if (GetThreadSelectorEntry(hThread, addr->Segment, &le))
            return (le.HighWord.Bits.BaseHi << 24) +
                (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + addr->Offset;
        break;
    case AddrModeReal:
        return (DWORD)(LOWORD(addr->Segment) << 4) + addr->Offset;
    case AddrModeFlat:
        return addr->Offset;
    default:
        FIXME("Unsupported (yet) mode (%x)\n", addr->Mode);
        return 0;
    }
    FIXME("Failed to linearize address %04x:%I64x (mode %x)\n",
          addr->Segment, addr->Offset, addr->Mode);
    return 0;
}

#ifndef _WIN64
static BOOL CALLBACK read_mem(HANDLE hProcess, DWORD addr, void* buffer,
                              DWORD size, LPDWORD nread)
{
    SIZE_T      r;
    if (!ReadProcessMemory(hProcess, (void*)(DWORD_PTR)addr, buffer, size, &r)) return FALSE;
    if (nread) *nread = r;
    return TRUE;
}
#endif

static BOOL CALLBACK read_mem64(HANDLE hProcess, DWORD64 addr, void* buffer,
                                DWORD size, LPDWORD nread)
{
    SIZE_T      r;
    if (!ReadProcessMemory(hProcess, (void*)(DWORD_PTR)addr, buffer, size, &r)) return FALSE;
    if (nread) *nread = r;
    return TRUE;
}

#ifndef _WIN64
static inline void addr_32to64(const ADDRESS* addr32, ADDRESS64* addr64)
{
    addr64->Offset = (ULONG64)addr32->Offset;
    addr64->Segment = addr32->Segment;
    addr64->Mode = addr32->Mode;
}
#endif

static inline void addr_64to32(const ADDRESS64* addr64, ADDRESS* addr32)
{
    addr32->Offset = (ULONG)addr64->Offset;
    addr32->Segment = addr64->Segment;
    addr32->Mode = addr64->Mode;
}

BOOL sw_read_mem(struct cpu_stack_walk* csw, DWORD64 addr, void* ptr, DWORD sz)
{
    DWORD bytes_read = 0;
    if (csw->is32)
        return csw->u.s32.f_read_mem(csw->hProcess, addr, ptr, sz, &bytes_read);
    else
        return csw->u.s64.f_read_mem(csw->hProcess, addr, ptr, sz, &bytes_read);
}

DWORD64 sw_xlat_addr(struct cpu_stack_walk* csw, ADDRESS64* addr)
{
    if (addr->Mode == AddrModeFlat) return addr->Offset;
    if (csw->is32)
    {
        ADDRESS         addr32;

        addr_64to32(addr, &addr32);
        return csw->u.s32.f_xlat_adr(csw->hProcess, csw->hThread, &addr32);
    }
    else if (csw->u.s64.f_xlat_adr)
        return csw->u.s64.f_xlat_adr(csw->hProcess, csw->hThread, addr);
    return addr_to_linear(csw->hProcess, csw->hThread, addr);
}

void* sw_table_access(struct cpu_stack_walk* csw, DWORD64 addr)
{
    if (csw->is32)
        return csw->u.s32.f_tabl_acs(csw->hProcess, addr);
    else
        return csw->u.s64.f_tabl_acs(csw->hProcess, addr);
}

DWORD64 sw_module_base(struct cpu_stack_walk* csw, DWORD64 addr)
{
    if (csw->is32)
        return csw->u.s32.f_modl_bas(csw->hProcess, addr);
    else
        return csw->u.s64.f_modl_bas(csw->hProcess, addr);
}

#ifndef _WIN64
/***********************************************************************
 *		StackWalk (DBGHELP.@)
 */
BOOL WINAPI StackWalk(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                      LPSTACKFRAME frame32, PVOID ctx,
                      PREAD_PROCESS_MEMORY_ROUTINE f_read_mem,
                      PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                      PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                      PTRANSLATE_ADDRESS_ROUTINE f_xlat_adr)
{
    struct cpu_stack_walk       csw;
    STACKFRAME64                frame64;
    BOOL                        ret;
    struct cpu*                 cpu;

    TRACE("(%ld, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame32, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (!(cpu = cpu_find(MachineType)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    addr_32to64(&frame32->AddrPC,     &frame64.AddrPC);
    addr_32to64(&frame32->AddrReturn, &frame64.AddrReturn);
    addr_32to64(&frame32->AddrFrame,  &frame64.AddrFrame);
    addr_32to64(&frame32->AddrStack,  &frame64.AddrStack);
    addr_32to64(&frame32->AddrBStore, &frame64.AddrBStore);
    frame64.FuncTableEntry = frame32->FuncTableEntry; /* FIXME */
    frame64.Far = frame32->Far;
    frame64.Virtual = frame32->Virtual;
    frame64.Reserved[0] = frame32->Reserved[0];
    frame64.Reserved[1] = frame32->Reserved[1];
    frame64.Reserved[2] = frame32->Reserved[2];
    /* we don't handle KdHelp */

    csw.hProcess = hProcess;
    csw.hThread = hThread;
    csw.is32 = TRUE;
    csw.cpu = cpu;
    /* sigh... MS isn't even consistent in the func prototypes */
    csw.u.s32.f_read_mem = (f_read_mem) ? f_read_mem : read_mem;
    csw.u.s32.f_xlat_adr = f_xlat_adr;
    csw.u.s32.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess;
    csw.u.s32.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase;

    if ((ret = cpu->stack_walk(&csw, &frame64, ctx)))
    {
        addr_64to32(&frame64.AddrPC,     &frame32->AddrPC);
        addr_64to32(&frame64.AddrReturn, &frame32->AddrReturn);
        addr_64to32(&frame64.AddrFrame,  &frame32->AddrFrame);
        addr_64to32(&frame64.AddrStack,  &frame32->AddrStack);
        addr_64to32(&frame64.AddrBStore, &frame32->AddrBStore);
        frame32->FuncTableEntry = frame64.FuncTableEntry; /* FIXME */
        frame32->Params[0] = frame64.Params[0];
        frame32->Params[1] = frame64.Params[1];
        frame32->Params[2] = frame64.Params[2];
        frame32->Params[3] = frame64.Params[3];
        frame32->Far = frame64.Far;
        frame32->Virtual = frame64.Virtual;
        frame32->Reserved[0] = frame64.Reserved[0];
        frame32->Reserved[1] = frame64.Reserved[1];
        frame32->Reserved[2] = frame64.Reserved[2];
    }

    return ret;
}
#endif


/***********************************************************************
 *		StackWalk64 (DBGHELP.@)
 */
BOOL WINAPI StackWalk64(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                        LPSTACKFRAME64 frame, PVOID ctx,
                        PREAD_PROCESS_MEMORY_ROUTINE64 f_read_mem,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                        PTRANSLATE_ADDRESS_ROUTINE64 f_xlat_adr)
{
    struct cpu_stack_walk       csw;
    struct cpu*                 cpu;

    TRACE("(%ld, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (!(cpu = cpu_find(MachineType)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    csw.hProcess = hProcess;
    csw.hThread = hThread;
    csw.is32 = FALSE;
    csw.cpu = cpu;
    /* sigh... MS isn't even consistent in the func prototypes */
    csw.u.s64.f_read_mem = (f_read_mem) ? f_read_mem : read_mem64;
    csw.u.s64.f_xlat_adr = (f_xlat_adr) ? f_xlat_adr : addr_to_linear;
    csw.u.s64.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess64;
    csw.u.s64.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase64;

    if (!cpu->stack_walk(&csw, frame, ctx)) return FALSE;

    /* we don't handle KdHelp */

    return TRUE;
}

/* all the fields of STACKFRAME64 are present in STACKFRAME_EX at same offset
 * So casting down a STACKFRAME_EX into a STACKFRAME64 is valid!
 */
C_ASSERT(sizeof(STACKFRAME64) == FIELD_OFFSET(STACKFRAME_EX, StackFrameSize));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, AddrPC) == FIELD_OFFSET(STACKFRAME_EX, AddrPC));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, AddrReturn) == FIELD_OFFSET(STACKFRAME_EX, AddrReturn));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, AddrFrame) == FIELD_OFFSET(STACKFRAME_EX, AddrFrame));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, AddrStack) == FIELD_OFFSET(STACKFRAME_EX, AddrStack));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, AddrBStore) == FIELD_OFFSET(STACKFRAME_EX, AddrBStore));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, FuncTableEntry) == FIELD_OFFSET(STACKFRAME_EX, FuncTableEntry));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, Params) == FIELD_OFFSET(STACKFRAME_EX, Params));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, Far) == FIELD_OFFSET(STACKFRAME_EX, Far));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, Virtual) == FIELD_OFFSET(STACKFRAME_EX, Virtual));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, Reserved) == FIELD_OFFSET(STACKFRAME_EX, Reserved));
C_ASSERT(FIELD_OFFSET(STACKFRAME64, KdHelp) == FIELD_OFFSET(STACKFRAME_EX, KdHelp));

/***********************************************************************
 *		StackWalkEx (DBGHELP.@)
 */
BOOL WINAPI StackWalkEx(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                        LPSTACKFRAME_EX frame, PVOID ctx,
                        PREAD_PROCESS_MEMORY_ROUTINE64 f_read_mem,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                        PTRANSLATE_ADDRESS_ROUTINE64 f_xlat_adr,
                        DWORD flags)
{
    struct cpu_stack_walk       csw;
    struct cpu*                 cpu;
    DWORD64                     addr;

    TRACE("(%ld, %p, %p, %p, %p, %p, %p, %p, %p, 0x%lx)\n",
          MachineType, hProcess, hThread, frame, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr, flags);

    if (!(cpu = cpu_find(MachineType)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (frame->StackFrameSize != sizeof(*frame))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (flags != 0)
    {
        FIXME("Unsupported yet flags 0x%lx\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    csw.hProcess = hProcess;
    csw.hThread = hThread;
    csw.is32 = FALSE;
    csw.cpu = cpu;
    /* sigh... MS isn't even consistent in the func prototypes */
    csw.u.s64.f_read_mem = (f_read_mem) ? f_read_mem : read_mem64;
    csw.u.s64.f_xlat_adr = (f_xlat_adr) ? f_xlat_adr : addr_to_linear;
    csw.u.s64.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess64;
    csw.u.s64.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase64;

    addr = sw_xlat_addr(&csw, &frame->AddrPC);

    if (IFC_MODE(frame->InlineFrameContext) == IFC_MODE_INLINE)
    {
        DWORD depth = SymAddrIncludeInlineTrace(hProcess, addr);
        if (IFC_DEPTH(frame->InlineFrameContext) + 1 < depth) /* move to next inlined function? */
        {
            TRACE("found inline ctx: depth=%lu current=%lu++\n",
                  depth, frame->InlineFrameContext);
            frame->InlineFrameContext++; /* just increase index, FIXME detect overflow */
        }
        else
        {
            frame->InlineFrameContext = IFC_MODE_REGULAR; /* move to next top level function */
        }
    }
    else
    {
        if (!cpu->stack_walk(&csw, (STACKFRAME64*)frame, ctx)) return FALSE;
        if (frame->InlineFrameContext != INLINE_FRAME_CONTEXT_IGNORE)
        {
            addr = sw_xlat_addr(&csw, &frame->AddrPC);
            frame->InlineFrameContext = SymAddrIncludeInlineTrace(hProcess, addr) == 0 ? IFC_MODE_REGULAR : IFC_MODE_INLINE;
            TRACE("setting IFC mode to %lx\n", frame->InlineFrameContext);
        }
    }

    /* we don't handle KdHelp */

    return TRUE;
}

#ifndef _WIN64
/******************************************************************
 *		SymRegisterFunctionEntryCallback (DBGHELP.@)
 *
 *
 */
BOOL WINAPI SymRegisterFunctionEntryCallback(HANDLE hProc,
                                             PSYMBOL_FUNCENTRY_CALLBACK cb, PVOID user)
{
    FIXME("(%p %p %p): stub!\n", hProc, cb, user);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

/******************************************************************
 *		SymRegisterFunctionEntryCallback64 (DBGHELP.@)
 *
 *
 */
BOOL WINAPI SymRegisterFunctionEntryCallback64(HANDLE hProc,
                                               PSYMBOL_FUNCENTRY_CALLBACK64 cb,
                                               ULONG64 user)
{
    FIXME("(%p %p %I64x): stub!\n", hProc, cb, user);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
