/*
 * File cpu_x86_64.c
 *
 * Copyright (C) 1999, 2005 Alexandre Julliard
 * Copyright (C) 2009       Eric Pouech.
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

#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/* x86-64 unwind information, for PE modules, as described on MSDN */

typedef enum _UNWIND_OP_CODES
{
    UWOP_PUSH_NONVOL = 0,
    UWOP_ALLOC_LARGE,
    UWOP_ALLOC_SMALL,
    UWOP_SET_FPREG,
    UWOP_SAVE_NONVOL,
    UWOP_SAVE_NONVOL_FAR,
    UWOP_SAVE_XMM128,
    UWOP_SAVE_XMM128_FAR,
    UWOP_PUSH_MACHFRAME
} UNWIND_CODE_OPS;

typedef union _UNWIND_CODE
{
    struct
    {
        BYTE CodeOffset;
        BYTE UnwindOp : 4;
        BYTE OpInfo   : 4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

typedef struct _UNWIND_INFO
{
    BYTE Version       : 3;
    BYTE Flags         : 5;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegister : 4;
    BYTE FrameOffset   : 4;
    UNWIND_CODE UnwindCode[1]; /* actually CountOfCodes (aligned) */
/*
 *  union
 *  {
 *      OPTIONAL ULONG ExceptionHandler;
 *      OPTIONAL ULONG FunctionEntry;
 *  };
 *  OPTIONAL ULONG ExceptionData[];
 */
} UNWIND_INFO, *PUNWIND_INFO;

#define GetUnwindCodeEntry(info, index) \
    ((info)->UnwindCode[index])

#define GetLanguageSpecificDataPtr(info) \
    ((PVOID)&GetUnwindCodeEntry((info),((info)->CountOfCodes + 1) & ~1))

#define GetExceptionHandler(base, info) \
    ((PEXCEPTION_HANDLER)((base) + *(PULONG)GetLanguageSpecificDataPtr(info)))

#define GetChainedFunctionEntry(base, info) \
    ((PRUNTIME_FUNCTION)((base) + *(PULONG)GetLanguageSpecificDataPtr(info)))

#define GetExceptionDataPtr(info) \
    ((PVOID)((PULONG)GetLanguageSpecificData(info) + 1)

static unsigned x86_64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                                enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode = AddrModeFlat;
    switch (ca)
    {
#ifdef __x86_64__
    case cpu_addr_pc:    addr->Segment = ctx->SegCs; addr->Offset = ctx->Rip; return TRUE;
    case cpu_addr_stack: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rsp; return TRUE;
    case cpu_addr_frame: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rbp; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

enum st_mode {stm_start, stm_64bit, stm_done};

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentSwitch   1
#define __NextSwitch      2

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

#ifdef __x86_64__
union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

static void dump_unwind_info(HANDLE hProcess, ULONG64 base, RUNTIME_FUNCTION *function)
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    union handler_data *handler_data;
    char buffer[sizeof(UNWIND_INFO) + 256 * sizeof(UNWIND_CODE)];
    UNWIND_INFO* info = (UNWIND_INFO*)buffer;
    unsigned int i, count;
    SIZE_T r;

    TRACE("**** func %x-%x\n", function->BeginAddress, function->EndAddress);
    for (;;)
    {
        if (function->UnwindData & 1)
        {
#if 0
            RUNTIME_FUNCTION *next = (RUNTIME_FUNCTION*)((char*)base + (function->UnwindData & ~1));
            TRACE("unwind info for function %p-%p chained to function %p-%p\n",
                  (char*)base + function->BeginAddress, (char*)base + function->EndAddress,
                  (char*)base + next->BeginAddress, (char*)base + next->EndAddress);
            function = next;
            continue;
#else
            FIXME("NOT SUPPORTED\n");
#endif
        }
        ReadProcessMemory(hProcess, (char*)base + function->UnwindData, info, sizeof(*info), &r);
        ReadProcessMemory(hProcess, (char*)base + function->UnwindData + FIELD_OFFSET(UNWIND_INFO, UnwindCode),
                          info->UnwindCode, 256 * sizeof(UNWIND_CODE), &r);
        TRACE("unwind info at %p flags %x prolog 0x%x bytes function %p-%p\n",
              info, info->Flags, info->SizeOfProlog,
              (char*)base + function->BeginAddress, (char*)base + function->EndAddress);

        if (info->FrameRegister)
            TRACE("    frame register %s offset 0x%x(%%rsp)\n",
                  reg_names[info->FrameRegister], info->FrameOffset * 16);

        for (i = 0; i < info->CountOfCodes; i++)
        {
            TRACE("    0x%x: ", info->UnwindCode[i].CodeOffset);
            switch (info->UnwindCode[i].UnwindOp)
            {
            case UWOP_PUSH_NONVOL:
                TRACE("pushq %%%s\n", reg_names[info->UnwindCode[i].OpInfo]);
                break;
            case UWOP_ALLOC_LARGE:
                if (info->UnwindCode[i].OpInfo)
                {
                    count = *(DWORD*)&info->UnwindCode[i+1];
                    i += 2;
                }
                else
                {
                    count = *(USHORT*)&info->UnwindCode[i+1] * 8;
                    i++;
                }
                TRACE("subq $0x%x,%%rsp\n", count);
                break;
            case UWOP_ALLOC_SMALL:
                count = (info->UnwindCode[i].OpInfo + 1) * 8;
                TRACE("subq $0x%x,%%rsp\n", count);
                break;
            case UWOP_SET_FPREG:
                TRACE("leaq 0x%x(%%rsp),%s\n",
                      info->FrameOffset * 16, reg_names[info->FrameRegister]);
                break;
            case UWOP_SAVE_NONVOL:
                count = *(USHORT*)&info->UnwindCode[i+1] * 8;
                TRACE("movq %%%s,0x%x(%%rsp)\n", reg_names[info->UnwindCode[i].OpInfo], count);
                i++;
                break;
            case UWOP_SAVE_NONVOL_FAR:
                count = *(DWORD*)&info->UnwindCode[i+1];
                TRACE("movq %%%s,0x%x(%%rsp)\n", reg_names[info->UnwindCode[i].OpInfo], count);
                i += 2;
                break;
            case UWOP_SAVE_XMM128:
                count = *(USHORT*)&info->UnwindCode[i+1] * 16;
                TRACE("movaps %%xmm%u,0x%x(%%rsp)\n", info->UnwindCode[i].OpInfo, count);
                i++;
                break;
            case UWOP_SAVE_XMM128_FAR:
                count = *(DWORD*)&info->UnwindCode[i+1];
                TRACE("movaps %%xmm%u,0x%x(%%rsp)\n", info->UnwindCode[i].OpInfo, count);
                i += 2;
                break;
            case UWOP_PUSH_MACHFRAME:
                TRACE("PUSH_MACHFRAME %u\n", info->UnwindCode[i].OpInfo);
                break;
            default:
                FIXME("unknown code %u\n", info->UnwindCode[i].UnwindOp);
                break;
            }
        }

        handler_data = (union handler_data*)&info->UnwindCode[(info->CountOfCodes + 1) & ~1];
        if (info->Flags & UNW_FLAG_CHAININFO)
        {
            TRACE("    chained to function %p-%p\n",
                  (char*)base + handler_data->chain.BeginAddress,
                  (char*)base + handler_data->chain.EndAddress);
            function = &handler_data->chain;
            continue;
        }
        if (info->Flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
            TRACE("    handler %p data at %p\n",
                  (char*)base + handler_data->handler, &handler_data->handler + 1);
        break;
    }
}

/* highly derivated from dlls/ntdlls/signal_x86_64.c */
static ULONG64 get_int_reg(CONTEXT *context, int reg)
{
    return *(&context->Rax + reg);
}

static void set_int_reg(CONTEXT *context, int reg, ULONG64 val)
{
    *(&context->Rax + reg) = val;
}

static void set_float_reg(CONTEXT *context, int reg, M128A val)
{
    *(&context->u.s.Xmm0 + reg) = val;
}

static int get_opcode_size(UNWIND_CODE op)
{
    switch (op.UnwindOp)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.OpInfo != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

static BOOL is_inside_epilog(struct cpu_stack_walk* csw, DWORD64 pc)
{
    BYTE        op0, op1, op2;

    if (!sw_read_mem(csw, pc, &op0, 1)) return FALSE;

    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((op0 & 0xf8) == 0x48)
    {
        if (!sw_read_mem(csw, pc + 1, &op1, 1)) return FALSE;
        switch (op1)
        {
        case 0x81: /* add $nnnn,%rsp */
            if (!sw_read_mem(csw, pc + 2, &op2, 1)) return FALSE;
            if (op0 == 0x48 && op2 == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (op0 == 0x48 && op2 == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (op0 & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((op2 >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((op2 & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((op2 >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((op2 >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */
    for (;;)
    {
        BYTE rex = 0;

        if (!sw_read_mem(csw, pc, &op0, 1)) return FALSE;
        if ((op0 & 0xf0) == 0x40)
        {
            rex = op0 & 0x0f;  /* rex prefix */
            if (!sw_read_mem(csw, ++pc, &op0, 1)) return FALSE;
        }

        switch (op0)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        /* FIXME: add various jump instructions */
        }
        return FALSE;
    }
}

static BOOL default_unwind(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    if (!sw_read_mem(csw, frame->AddrStack.Offset,
                     &frame->AddrReturn.Offset, sizeof(DWORD64)))
    {
        WARN("Cannot read new frame offset %s\n", wine_dbgstr_longlong(frame->AddrStack.Offset));
        return FALSE;
    }
    context->Rip = frame->AddrReturn.Offset;
    frame->AddrStack.Offset += sizeof(DWORD64);
    context->Rsp += sizeof(DWORD64);
    return TRUE;
}

static BOOL interpret_function_table_entry(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame,
                                           CONTEXT* context, RUNTIME_FUNCTION* function, DWORD64 base)
{
    char                buffer[sizeof(UNWIND_INFO) + 256 * sizeof(UNWIND_CODE)];
    UNWIND_INFO*        info = (UNWIND_INFO*)buffer;
    unsigned            i;
    DWORD64             newframe, prolog_offset, off, value;
    M128A               floatvalue;
    union handler_data  handler_data;

    /* FIXME: we have some assumptions here */
    assert(context);
    if (context->Rsp != frame->AddrStack.Offset) FIXME("unconsistent Stack Pointer\n");
    if (context->Rip != frame->AddrPC.Offset) FIXME("unconsistent Instruction Pointer\n");
    dump_unwind_info(csw->hProcess, sw_module_base(csw, frame->AddrPC.Offset), frame->FuncTableEntry);
    newframe = context->Rsp;
    for (;;)
    {
        if (!sw_read_mem(csw, base + function->UnwindData, info, sizeof(*info)) ||
            !sw_read_mem(csw, base + function->UnwindData + FIELD_OFFSET(UNWIND_INFO, UnwindCode),
                         info->UnwindCode, info->CountOfCodes * sizeof(UNWIND_CODE)))
        {
            WARN("Couldn't read unwind_code at %lx\n", base + function->UnwindData);
            return FALSE;
        }

        if (info->Version != 1)
        {
            WARN("unknown unwind info version %u at %lx\n", info->Version, base + function->UnwindData);
            return FALSE;
        }

        if (info->FrameRegister)
            newframe = get_int_reg(context, info->FrameRegister) - info->FrameOffset * 16;

        /* check if in prolog */
        if (frame->AddrPC.Offset >= base + function->BeginAddress &&
            frame->AddrPC.Offset < base + function->BeginAddress + info->SizeOfProlog)
        {
            prolog_offset = frame->AddrPC.Offset - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            if (is_inside_epilog(csw, frame->AddrPC.Offset))
            {
                FIXME("epilog management not fully done\n");
                /* interpret_epilog((const BYTE*)frame->AddrPC.Offset, context); */
                return TRUE;
            }
        }

        for (i = 0; i < info->CountOfCodes; i += get_opcode_size(info->UnwindCode[i]))
        {
            if (prolog_offset < info->UnwindCode[i].CodeOffset) continue; /* skip it */

            switch (info->UnwindCode[i].UnwindOp)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                if (!sw_read_mem(csw, context->Rsp, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].OpInfo, value);
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->UnwindCode[i].OpInfo) context->Rsp += *(DWORD*)&info->UnwindCode[i+1];
                else context->Rsp += *(USHORT*)&info->UnwindCode[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->UnwindCode[i].OpInfo + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = newframe;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = newframe + *(USHORT*)&info->UnwindCode[i+1] * 8;
                if (!sw_read_mem(csw, context->Rsp, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].OpInfo, value);
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = newframe + *(DWORD*)&info->UnwindCode[i+1];
                if (!sw_read_mem(csw, context->Rsp, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].OpInfo, value);
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = newframe + *(USHORT*)&info->UnwindCode[i+1] * 16;
                if (!sw_read_mem(csw, context->Rsp, &floatvalue, sizeof(M128A))) return FALSE;
                set_float_reg(context, info->UnwindCode[i].OpInfo, floatvalue);
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = newframe + *(DWORD*)&info->UnwindCode[i+1];
                if (!sw_read_mem(csw, context->Rsp, &floatvalue, sizeof(M128A))) return FALSE;
                set_float_reg(context, info->UnwindCode[i].OpInfo, floatvalue);
                break;
            case UWOP_PUSH_MACHFRAME:
                FIXME("PUSH_MACHFRAME %u\n", info->UnwindCode[i].OpInfo);
                break;
            default:
                FIXME("unknown code %u\n", info->UnwindCode[i].UnwindOp);
                break;
            }
        }
        if (!(info->Flags & UNW_FLAG_CHAININFO)) break;
        if (!sw_read_mem(csw, base + function->UnwindData + FIELD_OFFSET(UNWIND_INFO, UnwindCode) +
                                   ((info->CountOfCodes + 1) & ~1) * sizeof(UNWIND_CODE),
                         &handler_data, sizeof(handler_data))) return FALSE;
        function = &handler_data.chain;  /* restart with the chained info */
    }
    frame->AddrStack.Offset = context->Rsp;
    return default_unwind(csw, frame, context);
}

static BOOL x86_64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    DWORD64     base;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;
    assert(!csw->is32);

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit");

    if (curr_mode == stm_start)
    {
        if ((frame->AddrPC.Mode == AddrModeFlat) &&
            (frame->AddrFrame.Mode != AddrModeFlat))
        {
            WARN("Bad AddrPC.Mode / AddrFrame.Mode combination\n");
            goto done_err;
        }

        /* Init done */
        curr_mode = stm_64bit;
        curr_switch = 0;
        frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (frame->AddrReturn.Offset == 0) goto done_err;
        frame->AddrPC = frame->AddrReturn;
    }

    if (frame->AddrPC.Offset && (base = sw_module_base(csw, frame->AddrPC.Offset)))
        frame->FuncTableEntry = sw_table_access(csw, frame->AddrPC.Offset);
    else
        frame->FuncTableEntry = NULL;
    if (frame->FuncTableEntry)
    {
        if (!interpret_function_table_entry(csw, frame, context, frame->FuncTableEntry, base))
            goto done_err;
    }
    /* FIXME: should check "native" debug format for native modules */
    else if (!default_unwind(csw, frame, context)) goto done_err;

    memset(&frame->Params, 0, sizeof(frame->Params));

    frame->Far = TRUE;
    frame->Virtual = TRUE;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit",
          frame->FuncTableEntry);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}
#else
static BOOL x86_64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    return FALSE;
}
#endif

static void*    x86_64_find_runtime_function(struct module* module, DWORD64 addr)
{
#ifdef __x86_64__
    RUNTIME_FUNCTION*   rtf;
    ULONG               size;
    int                 min, max;

    rtf = (RUNTIME_FUNCTION*)pe_map_directory(module, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size);
    if (rtf) for (min = 0, max = size / sizeof(*rtf); min <= max; )
    {
        int pos = (min + max) / 2;
        if (addr < module->module.BaseOfImage + rtf[pos].BeginAddress) max = pos - 1;
        else if (addr >= module->module.BaseOfImage + rtf[pos].EndAddress) min = pos + 1;
        else
        {
            rtf += pos;
            while (rtf->UnwindData & 1)  /* follow chained entry */
            {
                FIXME("RunTime_Function outside IMAGE_DIRECTORY_ENTRY_EXCEPTION unimplemented yet!\n");
                /* we need to read into the other process */
                /* rtf = (RUNTIME_FUNCTION*)(module->module.BaseOfImage + (rtf->UnwindData & ~1)); */
            }
            return rtf;
        }
    }
#endif
    return NULL;
}

struct cpu cpu_x86_64 = {
    IMAGE_FILE_MACHINE_AMD64,
    8,
    x86_64_get_addr,
    x86_64_stack_walk,
    x86_64_find_runtime_function,
};
