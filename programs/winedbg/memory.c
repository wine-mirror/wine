/*
 * Debugger memory handling
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 * Copyright 2000-2005 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debugger.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

void* be_cpu_linearize(HANDLE hThread, const ADDRESS64* addr)
{
    assert(addr->Mode == AddrModeFlat);
    return (void*)(DWORD_PTR)addr->Offset;
}

BOOL be_cpu_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS64* addr,
                       unsigned seg, unsigned long offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    addr->Offset  = offset;
    return TRUE;
}

void* memory_to_linear_addr(const ADDRESS64* addr)
{
    return be_cpu->linearize(dbg_curr_thread->handle, addr);
}

BOOL memory_get_current_pc(ADDRESS64* addr)
{
    assert(be_cpu->get_addr);
    return be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context, 
                            be_cpu_addr_pc, addr);
}

BOOL memory_get_current_stack(ADDRESS64* addr)
{
    assert(be_cpu->get_addr);
    return be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context, 
                            be_cpu_addr_stack, addr);
}

static void	memory_report_invalid_addr(const void* addr)
{
    ADDRESS64   address;

    address.Mode    = AddrModeFlat;
    address.Segment = 0;
    address.Offset  = (ULONG_PTR)addr;
    dbg_printf("*** Invalid address ");
    print_address(&address, FALSE);
    dbg_printf(" ***\n");
}

/***********************************************************************
 *           memory_read_value
 *
 * Read a memory value.
 */
BOOL memory_read_value(const struct dbg_lvalue* lvalue, DWORD size, void* result)
{
    BOOL ret = FALSE;

    if (lvalue->cookie == DLV_TARGET)
    {
        void*   linear = memory_to_linear_addr(&lvalue->addr);
        if (!(ret = dbg_read_memory(linear, result, size)))
            memory_report_invalid_addr(linear);
    }
    else
    {
        if (lvalue->addr.Offset)
        {
            memcpy(result, (void*)(DWORD_PTR)lvalue->addr.Offset, size);
            ret = TRUE;
        }
    }
    return ret;
}

/***********************************************************************
 *           memory_write_value
 *
 * Store a value in memory.
 */
BOOL memory_write_value(const struct dbg_lvalue* lvalue, DWORD size, void* value)
{
    BOOL        ret = TRUE;
    DWORD64     os;

    if (!types_get_info(&lvalue->type, TI_GET_LENGTH, &os)) return FALSE;
    if (size != os)
    {
        dbg_printf("Size mismatch in memory_write_value, got %u from type while expecting %u\n",
                   (DWORD)os, size);
        return FALSE;
    }

    /* FIXME: only works on little endian systems */
    if (lvalue->cookie == DLV_TARGET)
    {
        void*       linear = memory_to_linear_addr(&lvalue->addr);
        if (!(ret = dbg_write_memory(linear, value, size)))
            memory_report_invalid_addr(linear);
    }
    else 
    {
        memcpy((void*)(DWORD_PTR)lvalue->addr.Offset, value, size);
    }
    return ret;
}

/***********************************************************************
 *           memory_examine
 *
 * Implementation of the 'x' command.
 */
void memory_examine(const struct dbg_lvalue *lvalue, int count, char format)
{
    int			i;
    char                buffer[256];
    ADDRESS64           addr;
    void               *linear;

    types_extract_as_address(lvalue, &addr);
    linear = memory_to_linear_addr(&addr);

    if (format != 'i' && count > 1)
    {
        print_address(&addr, FALSE);
        dbg_printf(": ");
    }

    switch (format)
    {
    case 'u':
        if (count == 1) count = 256;
        memory_get_string(dbg_curr_process, linear, 
                          TRUE, TRUE, buffer, min(count, sizeof(buffer)));
        dbg_printf("%s\n", buffer);
        return;
    case 's':
        if (count == 1) count = 256;
        memory_get_string(dbg_curr_process, linear,
                          TRUE, FALSE, buffer, min(count, sizeof(buffer)));
        dbg_printf("%s\n", buffer);
        return;
    case 'i':
        while (count-- && memory_disasm_one_insn(&addr));
        return;
    case 'g':
        while (count--)
        {
            GUID guid;
            if (!dbg_read_memory(linear, &guid, sizeof(guid)))
            {
                memory_report_invalid_addr(linear);
                break;
            }
            dbg_printf("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                       guid.Data1, guid.Data2, guid.Data3,
                       guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                       guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            linear = (char*)linear + sizeof(guid);
            addr.Offset += sizeof(guid);
            if (count)
            {
                print_address(&addr, FALSE);
                dbg_printf(": ");
            }
        }
        return;

#define DO_DUMP2(_t,_l,_f,_vv) {                                        \
            _t _v;                                                      \
            for (i = 0; i < count; i++) {                               \
                if (!dbg_read_memory(linear, &_v, sizeof(_t)))          \
                { memory_report_invalid_addr(linear); break; }          \
                dbg_printf(_f, (_vv));                                  \
                addr.Offset += sizeof(_t);                              \
                linear = (char*)linear + sizeof(_t);                    \
                if ((i % (_l)) == (_l) - 1 && i != count - 1)           \
                {                                                       \
                    dbg_printf("\n");                                   \
                    print_address(&addr, FALSE);                        \
                    dbg_printf(": ");                                   \
                }                                                       \
            }                                                           \
            dbg_printf("\n");                                           \
        }
#define DO_DUMP(_t,_l,_f) DO_DUMP2(_t,_l,_f,_v)

    case 'x': DO_DUMP(int, 4, " %8.8x"); break;
    case 'd': DO_DUMP(unsigned int, 4, " %4.4d"); break;
    case 'w': DO_DUMP(unsigned short, 8, " %04x"); break;
    case 'a':
        if (sizeof(DWORD_PTR) == 4)
        {
            DO_DUMP(DWORD_PTR, 4, " %8.8lx");
        }
        else
        {
            DO_DUMP(DWORD_PTR, 2, " %16.16lx");
        }
        break;
    case 'c': DO_DUMP2(char, 32, " %c", (_v < 0x20) ? ' ' : _v); break;
    case 'b': DO_DUMP2(char, 16, " %02x", (_v) & 0xff); break;
    }
}

BOOL memory_get_string(struct dbg_process* pcs, void* addr, BOOL in_debuggee,
                       BOOL unicode, char* buffer, int size)
{
    SIZE_T      sz;
    WCHAR*      buffW;

    buffer[0] = 0;
    if (!addr) return FALSE;
    if (in_debuggee)
    {
        BOOL ret;

        if (!unicode) ret = pcs->process_io->read(pcs->handle, addr, buffer, size, &sz);
        else
        {
            buffW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
            ret = pcs->process_io->read(pcs->handle, addr, buffW, size * sizeof(WCHAR), &sz);
            WideCharToMultiByte(CP_ACP, 0, buffW, sz / sizeof(WCHAR), buffer, size,
                                NULL, NULL);
            HeapFree(GetProcessHeap(), 0, buffW);
        }
        if (size) buffer[size-1] = 0;
        return ret;
    }
    else
    {
        lstrcpynA(buffer, addr, size);
    }
    return TRUE;
}

BOOL memory_get_string_indirect(struct dbg_process* pcs, void* addr, BOOL unicode, WCHAR* buffer, int size)
{
    void*       ad;
    SIZE_T	sz;

    buffer[0] = 0;
    if (addr &&
        pcs->process_io->read(pcs->handle, addr, &ad, sizeof(ad), &sz) && sz == sizeof(ad) && ad)
    {
        LPSTR buff;
        BOOL ret;

        if (unicode)
            ret = pcs->process_io->read(pcs->handle, ad, buffer, size * sizeof(WCHAR), &sz) && sz != 0;
        else
        {
            if ((buff = HeapAlloc(GetProcessHeap(), 0, size)))
            {
                ret = pcs->process_io->read(pcs->handle, ad, buff, size, &sz) && sz != 0;
                MultiByteToWideChar(CP_ACP, 0, buff, sz, buffer, size);
                HeapFree(GetProcessHeap(), 0, buff);
            }
            else ret = FALSE;
        }
        if (size) buffer[size-1] = 0;
        return ret;
    }
    return FALSE;
}

/*
 * Convert an address offset to hex string. If mode == 32, treat offset as
 * 32 bits (discard upper 32 bits), if mode == 64 use all 64 bits, if mode == 0
 * treat as either 32 or 64 bits, depending on whether we're running as
 * Wine32 or Wine64.
 */
char* memory_offset_to_string(char *str, DWORD64 offset, unsigned mode)
{
    if (mode != 32 && mode != 64)
    {
#ifdef _WIN64
        mode = 64;
#else
        mode = 32;
#endif
    }

    if (mode == 32)
        sprintf(str, "0x%08x", (unsigned int) offset);
    else
        sprintf(str, "0x%08x%08x", (unsigned int)(offset >> 32),
                (unsigned int)offset);

    return str;
}

static void dbg_print_longlong(LONGLONG sv, BOOL is_signed)
{
    char      tmp[24], *ptr = tmp + sizeof(tmp) - 1;
    ULONGLONG uv, div;
    *ptr = '\0';
    if (is_signed && sv < 0)    uv = -sv;
    else                        { uv = sv; is_signed = FALSE; }
    for (div = 10; uv; div *= 10, uv /= 10)
        *--ptr = '0' + (uv % 10);
    if (ptr == tmp + sizeof(tmp) - 1) *--ptr = '0';
    if (is_signed) *--ptr = '-';
    dbg_printf("%s", ptr);
}

static void dbg_print_hex(DWORD size, ULONGLONG sv)
{
    if (!sv)
        dbg_printf("0");
    else if (size > 4 && (sv >> 32))
        dbg_printf("0x%x%08x", (DWORD)(sv >> 32), (DWORD)sv);
    else
        dbg_printf("0x%x", (DWORD)sv);
}

static void print_typed_basic(const struct dbg_lvalue* lvalue)
{
    LONGLONG            val_int;
    void*               val_ptr;
    long double         val_real;
    DWORD64             size64;
    DWORD               tag, size, count, bt;
    struct dbg_type     type = lvalue->type;
    struct dbg_type     sub_type;
    struct dbg_lvalue   sub_lvalue;

    if (!types_get_real_type(&type, &tag)) return;

    switch (tag)
    {
    case SymTagBaseType:
        if (!types_get_info(&type, TI_GET_LENGTH, &size64) ||
            !types_get_info(&type, TI_GET_BASETYPE, &bt))
        {
            WINE_ERR("Couldn't get information\n");
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
            return;
        }
        size = (DWORD)size64;
        switch (bt)
        {
        case btInt:
        case btLong:
            if (!be_cpu->fetch_integer(lvalue, size, TRUE, &val_int)) return;
            if (size == 1) goto print_char;
            dbg_print_hex(size, val_int);
            break;
        case btUInt:
        case btULong:
            if (!be_cpu->fetch_integer(lvalue, size, FALSE, &val_int)) return;
            dbg_print_hex(size, val_int);
            break;
        case btFloat:
            if (!be_cpu->fetch_float(lvalue, size, &val_real)) return;
            dbg_printf("%Lf", val_real);
            break;
        case btChar:
        case btWChar:
            /* sometimes WCHAR is defined as btChar with size = 2, so discrimate
             * Ansi/Unicode based on size, not on basetype
             */
            if (!be_cpu->fetch_integer(lvalue, size, TRUE, &val_int)) return;
        print_char:
            if (size == 1 && isprint((char)val_int))
                dbg_printf("'%c'", (char)val_int);
            else if (size == 2 && isprintW((WCHAR)val_int))
            {
                WCHAR   wch = (WCHAR)val_int;
                dbg_printf("'");
                dbg_outputW(&wch, 1);
                dbg_printf("'");
            }
            else
                dbg_printf("%d", (int)val_int);
            break;
        case btBool:
            if (!be_cpu->fetch_integer(lvalue, size, TRUE, &val_int)) return;
            dbg_printf("%s", val_int ? "true" : "false");
            break;
        default:
            WINE_FIXME("Unsupported basetype %u\n", bt);
            break;
        }
        break;
    case SymTagPointerType:
        if (!types_array_index(lvalue, 0, &sub_lvalue))
        {
            dbg_printf("Internal symbol error: unable to access memory location %p",
                       memory_to_linear_addr(&lvalue->addr));
            break;
        }
        val_ptr = memory_to_linear_addr(&sub_lvalue.addr);
        if (types_get_real_type(&sub_lvalue.type, &tag) && tag == SymTagBaseType &&
            types_get_info(&sub_lvalue.type, TI_GET_BASETYPE, &bt) &&
            types_get_info(&sub_lvalue.type, TI_GET_LENGTH, &size64))
        {
            char    buffer[1024];

            if (!val_ptr) dbg_printf("0x0");
            else if (((bt == btChar || bt == btInt) && size64 == 1) || (bt == btUInt && size64 == 2))
            {
                if (memory_get_string(dbg_curr_process, val_ptr, sub_lvalue.cookie == DLV_TARGET,
                                      size64 == 2, buffer, sizeof(buffer)))
                    dbg_printf("\"%s\"", buffer);
                else
                    dbg_printf("*** invalid address %p ***", val_ptr);
                break;
            }
        }
        dbg_printf("%p", val_ptr);
        break;
    case SymTagArrayType:
    case SymTagUDT:
        if (!memory_read_value(lvalue, sizeof(val_ptr), &val_ptr)) return;
        dbg_printf("%p", val_ptr);
        break;
    case SymTagEnum:
        {
            BOOL        ok = FALSE;

            /* FIXME: it depends on underlying type for enums 
             * (not supported yet in dbghelp)
             * Assuming 4 as for an int
             */
            if (!be_cpu->fetch_integer(lvalue, 4, TRUE, &val_int)) return;

            if (types_get_info(&type, TI_GET_CHILDRENCOUNT, &count))
            {
                char                    buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
                TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
                WCHAR*                  ptr;
                char                    tmp[256];
                VARIANT                 variant;
                int                     i;

                fcp->Start = 0;
                while (count)
                {
                    fcp->Count = min(count, 256);
                    if (types_get_info(&type, TI_FINDCHILDREN, fcp))
                    {
                        sub_type.module = type.module;
                        for (i = 0; i < min(fcp->Count, count); i++)
                        {
                            sub_type.id = fcp->ChildId[i];
                            if (!types_get_info(&sub_type, TI_GET_VALUE, &variant)) 
                                continue;
                            switch (variant.n1.n2.vt)
                            {
                            case VT_I4: ok = (val_int == variant.n1.n2.n3.lVal); break;
                            default: WINE_FIXME("Unsupported variant type (%u)\n", variant.n1.n2.vt);
                            }
                            if (ok)
                            {
                                ptr = NULL;
                                types_get_info(&sub_type, TI_GET_SYMNAME, &ptr);
                                if (!ptr) continue;
                                WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                                HeapFree(GetProcessHeap(), 0, ptr);
                                dbg_printf("%s", tmp);
                                count = 0; /* so that we'll get away from outter loop */
                                break;
                            }
                        }
                    }
                    count -= min(count, 256);
                    fcp->Start += 256;
                }
            }
            if (!ok) dbg_print_longlong(val_int, TRUE);
        }
        break;
    default:
        WINE_FIXME("Unsupported tag %u\n", tag);
        break;
    }
}

/***********************************************************************
 *           print_basic
 *
 * Implementation of the 'print' command.
 */
void print_basic(const struct dbg_lvalue* lvalue, char format)
{
    if (lvalue->type.id == dbg_itype_none)
    {
        dbg_printf("Unable to evaluate expression\n");
        return;
    }

    if (format != 0)
    {
        unsigned size;
        LONGLONG res = types_extract_as_longlong(lvalue, &size, NULL);
        WCHAR wch;

        switch (format)
        {
        case 'x':
            dbg_print_hex(size, (ULONGLONG)res);
            return;

        case 'd':
            dbg_print_longlong(res, TRUE);
            return;

        case 'c':
            dbg_printf("%d = '%c'", (char)(res & 0xff), (char)(res & 0xff));
            return;

        case 'u':
            wch = (WCHAR)(res & 0xFFFF);
            dbg_printf("%d = '", wch);
            dbg_outputW(&wch, 1);
            dbg_printf("'");
            return;

        case 'i':
        case 's':
        case 'w':
        case 'b':
            dbg_printf("Format specifier '%c' is meaningless in 'print' command\n", format);
        }
    }
    if (lvalue->type.id == dbg_itype_segptr)
    {
        dbg_print_longlong(types_extract_as_longlong(lvalue, NULL, NULL), TRUE);
    }
    else print_typed_basic(lvalue);
}

void print_bare_address(const ADDRESS64* addr)
{
    char hexbuf[MAX_OFFSET_TO_STR_LEN];

    switch (addr->Mode)
    {
    case AddrModeFlat: 
        dbg_printf("%s", memory_offset_to_string(hexbuf, addr->Offset, 0)); 
        break;
    case AddrModeReal:
    case AddrMode1616:
        dbg_printf("0x%04x:0x%04x", addr->Segment, (unsigned) addr->Offset);
        break;
    case AddrMode1632:
        dbg_printf("0x%04x:%s", addr->Segment,
                   memory_offset_to_string(hexbuf, addr->Offset, 32));
        break;
    default:
        dbg_printf("Unknown mode %x\n", addr->Mode);
        break;
    }
}

/***********************************************************************
 *           print_address
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
void print_address(const ADDRESS64* addr, BOOLEAN with_line)
{
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        si = (SYMBOL_INFO*)buffer;
    void*               lin = memory_to_linear_addr(addr);
    DWORD64             disp64;
    DWORD               disp;

    print_bare_address(addr);

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    if (!SymFromAddr(dbg_curr_process->handle, (DWORD_PTR)lin, &disp64, si)) return;
    dbg_printf(" %s", si->Name);
    if (disp64) dbg_printf("+0x%lx", (DWORD_PTR)disp64);
    if (with_line)
    {
        IMAGEHLP_LINE64             il;
        IMAGEHLP_MODULE             im;

        il.SizeOfStruct = sizeof(il);
        if (SymGetLineFromAddr64(dbg_curr_process->handle, (DWORD_PTR)lin, &disp, &il))
            dbg_printf(" [%s:%u]", il.FileName, il.LineNumber);
        im.SizeOfStruct = sizeof(im);
        if (SymGetModuleInfo(dbg_curr_process->handle, (DWORD_PTR)lin, &im))
            dbg_printf(" in %s", im.ModuleName);
    }
}

BOOL memory_disasm_one_insn(ADDRESS64* addr)
{
    char        ch;

    print_address(addr, TRUE);
    dbg_printf(": ");
    if (!dbg_read_memory(memory_to_linear_addr(addr), &ch, sizeof(ch)))
    {
        dbg_printf("-- no code accessible --\n");
        return FALSE;
    }
    be_cpu->disasm_one_insn(addr, TRUE);
    dbg_printf("\n");
    return TRUE;
}

void memory_disassemble(const struct dbg_lvalue* xstart, 
                        const struct dbg_lvalue* xend, int instruction_count)
{
    static ADDRESS64 last = {0,0,0};
    int stop = 0;
    int i;

    if (!xstart && !xend) 
    {
        if (!last.Segment && !last.Offset) memory_get_current_pc(&last);
    }
    else
    {
        if (xstart)
            types_extract_as_address(xstart, &last);
        if (xend) 
            stop = types_extract_as_integer(xend);
    }
    for (i = 0; (instruction_count == 0 || i < instruction_count)  &&
                (stop == 0 || last.Offset <= stop); i++)
        memory_disasm_one_insn(&last);
}

BOOL memory_get_register(DWORD regno, DWORD_PTR** value, char* buffer, int len)
{
    const struct dbg_internal_var*  div;

    /* negative register values are wine's dbghelp hacks
     * see dlls/dbghelp/dbghelp_private.h for the details
     */
    switch (regno)
    {
    case -1:
        if (buffer) snprintf(buffer, len, "<internal error>");
        return FALSE;
    case -2:
        if (buffer) snprintf(buffer, len, "<couldn't compute location>");
        return FALSE;
    case -3:
        if (buffer) snprintf(buffer, len, "<is not available>");
        return FALSE;
    case -4:
        if (buffer) snprintf(buffer, len, "<couldn't read memory>");
        return FALSE;
    case -5:
        if (buffer) snprintf(buffer, len, "<has been optimized away by compiler>");
        return FALSE;
    }

    for (div = be_cpu->context_vars; div->name; div++)
    {
        if (div->val == regno)
        {
            if (!stack_get_register_frame(div, value))
            {
                if (buffer) snprintf(buffer, len, "<register %s not accessible in this frame>", div->name);
                return FALSE;
            }

            if (buffer) lstrcpynA(buffer, div->name, len);
            return TRUE;
        }
    }
    if (buffer) snprintf(buffer, len, "<unknown register %u>", regno);
    return FALSE;
}
