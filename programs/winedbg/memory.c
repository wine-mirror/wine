/*
 * Debugger memory handling
 *
 * Copyright 1993 Eric Youngdale
 * Copyright 1995 Alexandre Julliard
 * Copyright 2000-2004 Eric Pouech
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

void* be_cpu_linearize(HANDLE hThread, const ADDRESS* addr)
{
    assert(addr->Mode == AddrModeFlat);
    return (void*)addr->Offset;
}

unsigned be_cpu_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS* addr, 
                           unsigned seg, unsigned long offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    addr->Offset  = offset;
    return TRUE;
}

void* memory_to_linear_addr(const ADDRESS* addr)
{
    return be_cpu->linearize(dbg_curr_thread->handle, addr);
}

BOOL memory_get_current_pc(ADDRESS* addr)
{
    assert(be_cpu->get_addr);
    return be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context, 
                            be_cpu_addr_pc, addr);
}

BOOL memory_get_current_stack(ADDRESS* addr)
{
    assert(be_cpu->get_addr);
    return be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context, 
                            be_cpu_addr_stack, addr);
}

BOOL memory_get_current_frame(ADDRESS* addr)
{
    assert(be_cpu->get_addr);
    return be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context, 
                            be_cpu_addr_frame, addr);
}

void	memory_report_invalid_addr(const void* addr)
{
    ADDRESS     address;

    address.Mode    = AddrModeFlat;
    address.Segment = 0;
    address.Offset  = (unsigned long)addr;
    dbg_printf("*** Invalid address ");
    print_address(&address, FALSE);
    dbg_printf("\n");
}

/***********************************************************************
 *           memory_read_value
 *
 * Read a memory value.
 */
BOOL memory_read_value(const struct dbg_lvalue* lvalue, DWORD size, void* result)
{
    if (lvalue->cookie == DLV_TARGET)
    {
        if (!dbg_read_memory_verbose(memory_to_linear_addr(&lvalue->addr), result, size))
            return FALSE;
    }
    else
    {
        assert(lvalue->addr.Mode == AddrModeFlat);
        if (!lvalue->addr.Offset) return FALSE;
        memcpy(result, (void*)lvalue->addr.Offset, size);
    }
    return TRUE;
}

/***********************************************************************
 *           memory_write_value
 *
 * Store a value in memory.
 */
BOOL memory_write_value(const struct dbg_lvalue* lvalue, DWORD size, void* value)
{
    BOOL        ret = TRUE;
    DWORD       os;
    DWORD       linear = (DWORD)memory_to_linear_addr(&lvalue->addr);

    os = ~size;
    types_get_info(linear, lvalue->typeid, TI_GET_LENGTH, &os);
    assert(size == os);

    /* FIXME: only works on little endian systems */
    if (lvalue->cookie == DLV_TARGET)
    {
        ret = dbg_write_memory_verbose((void*)linear, value, size);
    }
    else 
    {
        assert(lvalue->addr.Mode == AddrModeFlat);
        memcpy((void*)lvalue->addr.Offset, value, size);
    }
    return ret;
}

/***********************************************************************
 *           memory_examine
 *
 * Implementation of the 'x' command.
 */
void memory_examine(const struct dbg_lvalue* lvalue, int count, char format)
{
    int			i;
    ADDRESS             x;
    char                buffer[256];

    x.Mode = AddrModeFlat;
    x.Offset = types_extract_as_integer(lvalue);
    if (format != 'i' && count > 1)
    {
        print_address(&x, FALSE);
        dbg_printf(": ");
    }

    switch (format)
    {
    case 'u':
        if (count == 1) count = 256;
        memory_get_string(dbg_curr_thread->handle, (void*)x.Offset, lvalue->cookie, 
                          TRUE, buffer, min(count, sizeof(buffer)));
        dbg_printf("%s\n", buffer);
        return;
    case 's':
        if (count == 1) count = 256;
        memory_get_string(dbg_curr_thread->handle, (void*)x.Offset, lvalue->cookie, 
                          FALSE, buffer, min(count, sizeof(buffer)));
        dbg_printf("%s\n", buffer);
        return;
    case 'i':
        while (count-- && memory_disasm_one_insn(&x));
        return;
    case 'g':
        while (count--)
        {
            GUID guid;
            if (!dbg_read_memory_verbose((void*)x.Offset, &guid, sizeof(guid))) break;
            dbg_printf("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                       guid.Data1, guid.Data2, guid.Data3,
                       guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                       guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            x.Offset += sizeof(guid);
            if (count)
            {
                print_address(&x, FALSE);
                dbg_printf(": ");
            }
        }
        return;

#define DO_DUMP2(_t,_l,_f,_vv) {                                        \
            _t _v;                                                      \
            for (i = 0; i < count; i++) {                               \
                if (!dbg_read_memory_verbose((void*)x.Offset, &_v,      \
                                             sizeof(_t))) break;        \
                dbg_printf(_f, (_vv));                                  \
                x.Offset += sizeof(_t);                                 \
                if ((i % (_l)) == (_l) - 1) {                           \
                    dbg_printf("\n");                                   \
                    print_address(&x, FALSE);                           \
                    dbg_printf(": ");                                   \
                }                                                       \
            }                                                           \
            dbg_printf("\n");                                           \
        }                                                               \
	return
#define DO_DUMP(_t,_l,_f) DO_DUMP2(_t,_l,_f,_v)

    case 'x': DO_DUMP(int, 4, " %8.8x");
    case 'd': DO_DUMP(unsigned int, 4, " %10d");
    case 'w': DO_DUMP(unsigned short, 8, " %04x");
    case 'c': DO_DUMP2(char, 32, " %c", (_v < 0x20) ? ' ' : _v);
    case 'b': DO_DUMP2(char, 16, " %02x", (_v) & 0xff);
    }
}

BOOL memory_get_string(HANDLE hp, void* addr, unsigned cookie, BOOL unicode, 
                       char* buffer, int size)
{
    DWORD       sz;
    WCHAR*      buffW;

    buffer[0] = 0;
    if (!addr) return FALSE;
    switch (cookie)
    {
    case DLV_TARGET:
        if (!unicode) return ReadProcessMemory(hp, addr, buffer, size, &sz);

        buffW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        ReadProcessMemory(hp, addr, buffW, size * sizeof(WCHAR), &sz);
        WideCharToMultiByte(CP_ACP, 0, buffW, sz / sizeof(WCHAR), buffer, size, 
                            NULL, NULL);
        HeapFree(GetProcessHeap(), 0, buffW);
        return TRUE;
    case DLV_HOST:
        strncpy(buffer, addr, size);
        buffer[size - 1] = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL memory_get_string_indirect(HANDLE hp, void* addr, BOOL unicode, char* buffer, int size)
{
    void*       ad;
    DWORD	sz;

    buffer[0] = 0;
    if (addr && 
        ReadProcessMemory(hp, addr, &ad, sizeof(ad), &sz) && sz == sizeof(ad) && ad)
    {
        return memory_get_string(hp, ad, DLV_TARGET, unicode, buffer, size);
    }
    return FALSE;
}

static void print_typed_basic(const struct dbg_lvalue* lvalue)
{
    long long int       val_int;
    void*               val_ptr;
    long double         val_real;
    DWORD               tag, size, count, bt, rtype;
    DWORD               linear = (DWORD)memory_to_linear_addr(&lvalue->addr);

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);

    if (lvalue->typeid == dbg_itype_none ||
        !types_get_info(linear, lvalue->typeid, TI_GET_SYMTAG, &tag))
        return;

    switch (tag)
    {
    case SymTagBaseType:
        if (!types_get_info(linear, lvalue->typeid, TI_GET_LENGTH, &size) ||
            !types_get_info(linear, lvalue->typeid, TI_GET_BASETYPE, &bt))
        {
            WINE_ERR("Couldn't get information\n");
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        }

        switch (bt)
        {
        case btInt:
            if (!be_cpu->fetch_integer(lvalue, size, TRUE, &val_int)) return;
            dbg_printf("%lld", val_int);
            break;
        case btUInt:
            if (!be_cpu->fetch_integer(lvalue, size, FALSE, &val_int)) return;
            dbg_printf("%llu", val_int);
            break;
        case btFloat:
            if (!be_cpu->fetch_float(lvalue, size, &val_real)) return;
            dbg_printf("%Lf", val_real);
            break;
        case btChar:
            if (!be_cpu->fetch_integer(lvalue, size, TRUE, &val_int)) return;
            /* FIXME: should do the same for a Unicode character (size == 2) */
            if (size == 1 && (val_int < 0x20 || val_int > 0x80))
                dbg_printf("%d", (int)val_int);
            else
                dbg_printf("'%c'", (char)val_int);
            break;
        default:
            WINE_FIXME("Unsupported basetype %lu\n", bt);
            break;
        }
        break;
    case SymTagPointerType:
        if (!memory_read_value(lvalue, sizeof(void*), &val_ptr)) return;

        if (!types_get_info(linear, lvalue->typeid, TI_GET_TYPE, &rtype) ||
            rtype == dbg_itype_none)
        {
            dbg_printf("Internal symbol error: unable to access memory location %p", val_ptr);
            break;
        }

        if (types_get_info(linear, rtype, TI_GET_SYMTAG, &tag) && tag == SymTagBaseType &&
            types_get_info(linear, rtype, TI_GET_BASETYPE, &bt) && bt == btChar &&
            types_get_info(linear, rtype, TI_GET_LENGTH, &size))
        {
            char    buffer[1024];

            memory_get_string(dbg_curr_thread->handle, (void*)val_ptr, lvalue->cookie,
                              size == 2, buffer, sizeof(buffer));
            dbg_printf("\"%s\"", buffer);
        }
        else dbg_printf("%p", val_ptr);
        break;
    case SymTagArrayType:
    case SymTagUDT:
        assert(lvalue->cookie == DLV_TARGET);
        if (!memory_read_value(lvalue, sizeof(val_ptr), &val_ptr)) return;
        dbg_printf("%p", val_ptr);
        break;
    case SymTagEnum:
        {
            BOOL        ok = FALSE;

            assert(lvalue->cookie == DLV_TARGET);
            /* FIXME: it depends on underlying type for enums 
             * (not supported yet in dbghelp)
             * Assuming 4 as for an int
             */
            if (!be_cpu->fetch_integer(lvalue, 4, TRUE, &val_int)) return;

            if (types_get_info(linear, lvalue->typeid, TI_GET_CHILDRENCOUNT, &count))
            {
                char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
                TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
                WCHAR*                      ptr;
                char                        tmp[256];
                VARIANT                     variant;
                int                         i;

                fcp->Start = 0;
                while (count)
                {
                    fcp->Count = min(count, 256);
                    if (types_get_info(linear, lvalue->typeid, TI_FINDCHILDREN, fcp))
                    {
                        for (i = 0; i < min(fcp->Count, count); i++)
                        {
                            if (!types_get_info(linear, fcp->ChildId[i], TI_GET_VALUE, &variant)) 
                                continue;
                            switch (variant.n1.n2.vt)
                            {
                            case VT_I4: ok = (val_int == variant.n1.n2.n3.lVal); break;
                            default: WINE_FIXME("Unsupported variant type (%u)\n", variant.n1.n2.vt);
                            }
                            if (ok)
                            {
                                ptr = NULL;
                                types_get_info(linear, fcp->ChildId[i], TI_GET_SYMNAME, &ptr);
                                if (!ptr) continue;
                                WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                                HeapFree(GetProcessHeap(), 0, ptr);
                                dbg_printf("%s", tmp);
                                count = 0; /* so that we'll get away from outter loop */
                                break;
                            }
                        }
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
            if (!ok) dbg_printf("%lld", val_int);
        }
        break;
    default:
        WINE_FIXME("Unsupported tag %lu\n", tag);
        break;
    }
}

/***********************************************************************
 *           print_basic
 *
 * Implementation of the 'print' command.
 */
void print_basic(const struct dbg_lvalue* lvalue, int count, char format)
{
    long int    res;

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);
    if (lvalue->typeid == dbg_itype_none)
    {
        dbg_printf("Unable to evaluate expression\n");
        return;
    }

    res = types_extract_as_integer(lvalue);

    /* FIXME: this implies i386 byte ordering */
    switch (format)
    {
    case 'x':
        if (lvalue->addr.Mode != AddrModeFlat)
            dbg_printf("0x%04lx", res);
        else
            dbg_printf("0x%08lx", res);
        break;

    case 'd':
        dbg_printf("%ld\n", res);
        break;

    case 'c':
        dbg_printf("%d = '%c'", (char)(res & 0xff), (char)(res & 0xff));
        break;

    case 'u':
        {
            WCHAR wch = (WCHAR)(res & 0xFFFF);
            dbg_printf("%d = '", wch);
            dbg_outputW(&wch, 1);
            dbg_printf("'");
        }
        break;

    case 'i':
    case 's':
    case 'w':
    case 'b':
        dbg_printf("Format specifier '%c' is meaningless in 'print' command\n", format);
    case 0:
        print_typed_basic(lvalue);
        break;
    }
}

void print_bare_address(const ADDRESS* addr)
{
    switch (addr->Mode)
    {
    case AddrModeFlat: 
        dbg_printf("0x%08lx", addr->Offset); 
        break;
    case AddrModeReal:
    case AddrMode1616:
        dbg_printf("0x%04x:0x%04lx", addr->Segment, addr->Offset);
        break;
    case AddrMode1632:
        dbg_printf("0x%04x:0x%08lx", addr->Segment, addr->Offset);
        break;
    }
}

/***********************************************************************
 *           print_address
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
void print_address(const ADDRESS* addr, BOOLEAN with_line)
{
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        si = (SYMBOL_INFO*)buffer;
    void*               lin = memory_to_linear_addr(addr);

    print_bare_address(addr);

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    if (!SymFromAddr(dbg_curr_process->handle, (unsigned long)lin, NULL, si)) return;
    dbg_printf(" %s", si->Name);
    if (with_line)
    {
        IMAGEHLP_LINE               il;
        IMAGEHLP_MODULE             im;

        il.SizeOfStruct = sizeof(il);
        if (SymGetLineFromAddr(dbg_curr_process->handle, (unsigned long)lin, NULL, &il))
            dbg_printf(" [%s:%lu]", il.FileName, il.LineNumber);
        im.SizeOfStruct = sizeof(im);
        if (SymGetModuleInfo(dbg_curr_process->handle, (unsigned long)lin, &im))
            dbg_printf(" in %s", im.ModuleName);
    }
}

struct foo
{
    char*       tmp;
    DWORD       frame;
};

static BOOL WINAPI sym_enum_cb(SYMBOL_INFO* sym_info, ULONG size, void* user)
{
    struct foo* foo = (struct foo*)user;
    DWORD       addr;
    unsigned    val;
    long        offset;

    if ((sym_info->Flags & (SYMFLAG_PARAMETER|SYMFLAG_FRAMEREL)) == (SYMFLAG_PARAMETER|SYMFLAG_FRAMEREL))
    {
        if (foo->tmp[0]) strcat(foo->tmp, ", ");
        addr = foo->frame;
        types_get_info(sym_info->ModBase, sym_info->TypeIndex, TI_GET_OFFSET, &offset);
        addr += offset;
        dbg_read_memory_verbose((char*)addr, &val, sizeof(val));
        sprintf(foo->tmp + strlen(foo->tmp), "%s=0x%x", sym_info->Name, val);
    }
    return TRUE;
}

void print_addr_and_args(const ADDRESS* pc, const ADDRESS* frame)
{
    char                        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                si = (SYMBOL_INFO*)buffer;
    IMAGEHLP_STACK_FRAME        isf;
    IMAGEHLP_LINE               il;
    IMAGEHLP_MODULE             im;
    struct foo                  foo;
    char                        tmp[1024];
    DWORD                       disp;

    if (pc->Mode != AddrModeFlat) 
        dbg_printf("0x%04x:0x%04lx", pc->Segment, pc->Offset);
    else
        dbg_printf("0x%08lx", pc->Offset);

    isf.InstructionOffset = (unsigned long)memory_to_linear_addr(pc);
    isf.FrameOffset       = (unsigned long)memory_to_linear_addr(frame);

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    if (!SymFromAddr(dbg_curr_process->handle, isf.InstructionOffset, &disp, si))
        return;

    dbg_printf(" %s", si->Name);
    if (disp) dbg_printf("+0x%lx", disp);

    SymSetContext(dbg_curr_process->handle, &isf, NULL);
    foo.tmp = tmp;
    foo.frame = isf.FrameOffset;
    tmp[0] = '\0';
    SymEnumSymbols(dbg_curr_process->handle, 0, NULL, sym_enum_cb, &foo);
    if (tmp[0]) dbg_printf("(%s)", tmp);

    il.SizeOfStruct = sizeof(il);
    if (SymGetLineFromAddr(dbg_curr_process->handle, isf.InstructionOffset,
                           NULL, &il))
        dbg_printf(" [%s:%lu]", il.FileName, il.LineNumber);
    im.SizeOfStruct = sizeof(im);
    if (SymGetModuleInfo(dbg_curr_process->handle, isf.InstructionOffset, &im))
        dbg_printf(" in %s", im.ModuleName);
}

BOOL memory_disasm_one_insn(ADDRESS* addr)
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
                        const struct dbg_lvalue* xend, int offset)
{
    static ADDRESS last = {0,0,0};

    if (!xstart && !xend) 
    {
        if (!last.Segment && !last.Offset) memory_get_current_pc(&last);
    }
    else
    {
        if (xstart)
        {
            last.Mode = AddrModeFlat;
            last.Offset = types_extract_as_integer(xstart);
        }
        if (xend) offset = types_extract_as_integer(xend) - last.Offset + 1;
    }
    while (offset-- > 0 && memory_disasm_one_insn(&last));
}
