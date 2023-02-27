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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Zydis/Zydis.h>

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

void* be_cpu_linearize(HANDLE hThread, const ADDRESS64* addr)
{
    assert(addr->Mode == AddrModeFlat);
    return (void*)(DWORD_PTR)addr->Offset;
}

BOOL be_cpu_build_addr(HANDLE hThread, const dbg_ctx_t *ctx, ADDRESS64* addr,
                       unsigned seg, DWORD64 offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    addr->Offset  = offset;
    return TRUE;
}

void* memory_to_linear_addr(const ADDRESS64* addr)
{
    return dbg_curr_process->be_cpu->linearize(dbg_curr_thread->handle, addr);
}

BOOL memory_get_current_pc(ADDRESS64* addr)
{
    assert(dbg_curr_process->be_cpu->get_addr);
    return dbg_curr_process->be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context,
                            be_cpu_addr_pc, addr);
}

BOOL memory_get_current_stack(ADDRESS64* addr)
{
    assert(dbg_curr_process->be_cpu->get_addr);
    return dbg_curr_process->be_cpu->get_addr(dbg_curr_thread->handle, &dbg_context,
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

    if (lvalue->in_debuggee)
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
        dbg_printf("Size mismatch in memory_write_value, got %I64u from type while expecting %lu\n",
                   os, size);
        return FALSE;
    }

    /* FIXME: only works on little endian systems */
    if (lvalue->in_debuggee)
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

/* transfer a block of memory
 * the two lvalue:s are expected to be of same size
 */
BOOL memory_transfer_value(const struct dbg_lvalue* to, const struct dbg_lvalue* from)
{
    DWORD64 size_to, size_from;
    BYTE tmp[256];
    BYTE* ptr = tmp;
    BOOL ret;

    if (to->bitlen || from->bitlen) return FALSE;
    if (!types_get_info(&to->type, TI_GET_LENGTH, &size_to) ||
        !types_get_info(&from->type, TI_GET_LENGTH, &size_from) ||
        size_from != size_to) return FALSE;
    /* optimize debugger to debugger transfer */
    if (!to->in_debuggee && !from->in_debuggee)
    {
        memcpy(memory_to_linear_addr(&to->addr), memory_to_linear_addr(&from->addr), size_from);
        return TRUE;
    }
    if (size_to > sizeof(tmp))
    {
        ptr = malloc(size_from);
        if (!ptr) return FALSE;
    }
    ret = memory_read_value(from, size_from, ptr) &&
        memory_write_value(to, size_from, ptr);
    if (size_to > sizeof(tmp)) free(ptr);
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
            dbg_printf("{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
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
        if (ADDRSIZE == 4)
        {
            DO_DUMP(DWORD, 4, " %8.8lx");
        }
        else
        {
            DO_DUMP(DWORD64, 2, " %16.16I64x");
        }
        break;
    case 'c': DO_DUMP2(char, 32, " %c", (_v < 0x20) ? ' ' : _v); break;
    case 'b': DO_DUMP2(char, 16, " %02x", (_v) & 0xff); break;
    }
}

BOOL memory_fetch_integer(const struct dbg_lvalue* lvalue, unsigned size,
                          BOOL is_signed, dbg_lgint_t* ret)
{
    /* size must fit in ret and be a power of two */
    if (size > sizeof(*ret) || (size & (size - 1))) return FALSE;

    if (lvalue->bitlen)
    {
        struct dbg_lvalue alt_lvalue = *lvalue;
        dbg_lguint_t mask;
        DWORD bt;
        /* FIXME: this test isn't sufficient, depending on start of bitfield
         * (ie a 64 bit field can spread across 9 bytes)
         */
        if (lvalue->bitlen > 8 * sizeof(dbg_lgint_t)) return FALSE;
        alt_lvalue.addr.Offset += lvalue->bitstart >> 3;
        /*
         * Bitfield operation.  We have to extract the field.
         */
        if (!memory_read_value(&alt_lvalue, sizeof(*ret), ret)) return FALSE;
        mask = ~(dbg_lguint_t)0 << lvalue->bitlen;
        *ret >>= lvalue->bitstart & 7;
        *ret &= ~mask;

        /*
         * OK, now we have the correct part of the number.
         * Check to see whether the basic type is signed or not, and if so,
         * we need to sign extend the number.
         */
        if (types_get_info(&lvalue->type, TI_GET_BASETYPE, &bt) &&
            (bt == btInt || bt == btLong) && (*ret & (1 << (lvalue->bitlen - 1))))
        {
            *ret |= mask;
        }
    }
    else
    {
        /* we are on little endian CPU */
        memset(ret, 0, sizeof(*ret)); /* clear unread bytes */
        if (!memory_read_value(lvalue, size, ret)) return FALSE;

        /* propagate sign information */
        if (is_signed && size < 8 && (*ret >> (size * 8 - 1)) != 0)
        {
            dbg_lguint_t neg = -1;
            *ret |= neg << (size * 8);
        }
    }
    return TRUE;
}

BOOL memory_store_integer(const struct dbg_lvalue* lvalue, dbg_lgint_t val)
{
    DWORD64 size;
    if (!types_get_info(&lvalue->type, TI_GET_LENGTH, &size)) return FALSE;
    if (lvalue->bitlen)
    {
        struct dbg_lvalue alt_lvalue = *lvalue;
        dbg_lguint_t mask, dst;

        /* FIXME: this test isn't sufficient, depending on start of bitfield
         * (ie a 64 bit field can spread across 9 bytes)
         */
        if (lvalue->bitlen > 8 * sizeof(dbg_lgint_t)) return FALSE;
        /* mask is 1 where bitfield is present, 0 outside */
        mask = (~(dbg_lguint_t)0 >> (sizeof(val) * 8 - lvalue->bitlen)) << (lvalue->bitstart & 7);
        alt_lvalue.addr.Offset += lvalue->bitstart >> 3;
        val <<= lvalue->bitstart & 7;
        if (!memory_read_value(&alt_lvalue, (unsigned)size, &dst)) return FALSE;
        val = (dst & ~mask) | (val & mask);
        return memory_write_value(&alt_lvalue, (unsigned)size, &val);
    }
    /* this is simple if we're on a little endian CPU */
    return memory_write_value(lvalue, (unsigned)size, &val);
}

BOOL memory_fetch_float(const struct dbg_lvalue* lvalue, double *ret)
{
    DWORD64 size;
    if (!types_get_info(&lvalue->type, TI_GET_LENGTH, &size)) return FALSE;
    /* FIXME: this assumes that debuggee and debugger use the same
     * representation for reals
     */
    if (size > sizeof(*ret)) return FALSE;
    if (!memory_read_value(lvalue, size, ret)) return FALSE;

    if (size == sizeof(float)) *ret = *(float*)ret;
    else if (size != sizeof(double)) return FALSE;

    return TRUE;
}

BOOL memory_store_float(const struct dbg_lvalue* lvalue, double *ret)
{
    DWORD64 size;
    if (!types_get_info(&lvalue->type, TI_GET_LENGTH, &size)) return FALSE;
    /* FIXME: this assumes that debuggee and debugger use the same
     * representation for reals
     */
    if (size > sizeof(*ret)) return FALSE;
    if (size == sizeof(float))
    {
        float f = *ret;
        return memory_write_value(lvalue, size, &f);
    }
    if (size != sizeof(double)) return FALSE;
    return memory_write_value(lvalue, size, ret);
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
            buffW = malloc(size * sizeof(WCHAR));
            ret = pcs->process_io->read(pcs->handle, addr, buffW, size * sizeof(WCHAR), &sz);
            WideCharToMultiByte(CP_ACP, 0, buffW, sz / sizeof(WCHAR), buffer, size,
                                NULL, NULL);
            free(buffW);
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
    void*       ad = 0;
    SIZE_T	sz;

    buffer[0] = 0;
    if (addr &&
        pcs->process_io->read(pcs->handle, addr, &ad, pcs->be_cpu->pointer_size, &sz) && sz == pcs->be_cpu->pointer_size && ad)
    {
        LPSTR buff;
        BOOL ret;

        if (unicode)
            ret = pcs->process_io->read(pcs->handle, ad, buffer, size * sizeof(WCHAR), &sz) && sz != 0;
        else
        {
            if ((buff = malloc(size)))
            {
                ret = pcs->process_io->read(pcs->handle, ad, buff, size, &sz) && sz != 0;
                MultiByteToWideChar(CP_ACP, 0, buff, sz, buffer, size);
                free(buff);
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
        sprintf(str, "%#016I64x", offset);

    return str;
}

static void dbg_print_sdecimal(dbg_lgint_t sv)
{
    dbg_printf("%I64d", sv);
}

static void dbg_print_hex(DWORD size, dbg_lgint_t sv)
{
    if (!sv)
        dbg_printf("0");
    else
        /* clear unneeded high bits, esp. sign extension */
        dbg_printf("%#I64x", sv & (~(dbg_lguint_t)0 >> (8 * (sizeof(dbg_lgint_t) - size))));
}

static void print_typed_basic(const struct dbg_lvalue* lvalue)
{
    dbg_lgint_t         val_int;
    void*               val_ptr;
    double              val_real;
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
            if (!memory_fetch_integer(lvalue, size, TRUE, &val_int)) return;
            dbg_print_hex(size, val_int);
            break;
        case btUInt:
        case btULong:
            if (!memory_fetch_integer(lvalue, size, FALSE, &val_int)) return;
            dbg_print_hex(size, val_int);
            break;
        case btFloat:
            if (!memory_fetch_float(lvalue, &val_real)) return;
            dbg_printf("%f", val_real);
            break;
        case btChar:
            if (!memory_fetch_integer(lvalue, size, TRUE, &val_int)) return;
            if (size == 1 && isprint((char)val_int))
                dbg_printf("'%c'", (char)val_int);
            else
                dbg_print_hex(size, val_int);
            break;
        case btWChar:
            if (!memory_fetch_integer(lvalue, size, TRUE, &val_int)) return;
            if (size == 2 && iswprint((WCHAR)val_int))
                dbg_printf("L'%lc'", (WCHAR)val_int);
            else
                dbg_print_hex(size, val_int);
            break;
        case btBool:
            if (!memory_fetch_integer(lvalue, size, TRUE, &val_int)) return;
            dbg_printf("%s", val_int ? "true" : "false");
            break;
        default:
            WINE_FIXME("Unsupported basetype %lu\n", bt);
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
            else if ((bt == btChar && size64 == 1) || (bt == btWChar && size64 == 2))
            {
                if (memory_get_string(dbg_curr_process, val_ptr, sub_lvalue.in_debuggee,
                                      bt == btWChar, buffer, sizeof(buffer)))
                    dbg_printf("%s\"%s\"", bt == btWChar ? "L" : "", buffer);
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

            if (!types_get_info(&type, TI_GET_LENGTH, &size64) ||
                !memory_fetch_integer(lvalue, size64, TRUE, &val_int)) return;

            if (types_get_info(&type, TI_GET_CHILDRENCOUNT, &count))
            {
                char                    buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
                TI_FINDCHILDREN_PARAMS* fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
                WCHAR*                  ptr;
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
                            switch (V_VT(&variant))
                            {
                            case VT_I1: ok = (val_int == V_I1(&variant)); break;
                            case VT_I2: ok = (val_int == V_I2(&variant)); break;
                            case VT_I4: ok = (val_int == V_I4(&variant)); break;
                            case VT_I8: ok = (val_int == V_I8(&variant)); break;
                            default: WINE_FIXME("Unsupported variant type (%u)\n", V_VT(&variant));
                            }
                            if (ok && types_get_info(&sub_type, TI_GET_SYMNAME, &ptr) && ptr)
                            {
                                dbg_printf("%ls", ptr);
                                HeapFree(GetProcessHeap(), 0, ptr);
                                count = 0; /* so that we'll get away from outer loop */
                                break;
                            }
                        }
                    }
                    count -= min(count, 256);
                    fcp->Start += 256;
                }
            }
            if (!ok) dbg_print_sdecimal(val_int);
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
        dbg_lgint_t res = types_extract_as_lgint(lvalue, &size, NULL);

        switch (format)
        {
        case 'x':
            dbg_print_hex(size, res);
            return;

        case 'd':
            dbg_print_sdecimal(res);
            return;

        case 'c':
            dbg_printf("%d = '%c'", (char)(res & 0xff), (char)(res & 0xff));
            return;

        case 'u':
            dbg_printf("%d = '%lc'", (WCHAR)(res & 0xFFFF), (WCHAR)(res & 0xFFFF));
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
        dbg_print_sdecimal(types_extract_as_lgint(lvalue, NULL, NULL));
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
        dbg_printf("Unknown mode %x", addr->Mode);
        break;
    }
}

static void print_address_symbol(const ADDRESS64* addr, BOOL with_line, const char *sep)
{
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        si = (SYMBOL_INFO*)buffer;
    DWORD_PTR           lin = (DWORD_PTR)memory_to_linear_addr(addr);
    DWORD64             disp64;
    DWORD               disp;
    IMAGEHLP_MODULE     im;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen   = 256;
    im.SizeOfStruct  = 0;
    if (SymFromAddr(dbg_curr_process->handle, lin, &disp64, si) && disp64 < si->Size)
    {
        dbg_printf("%s %s", sep, si->Name);
        if (disp64) dbg_printf("+0x%I64x", disp64);
    }
    else
    {
        im.SizeOfStruct = sizeof(im);
        if (!SymGetModuleInfo(dbg_curr_process->handle, lin, &im)) return;
        dbg_printf("%s %s", sep, im.ModuleName);
        if (lin > im.BaseOfImage)
            dbg_printf("+0x%Ix", lin - (DWORD_PTR)im.BaseOfImage);
    }
    if (with_line)
    {
        IMAGEHLP_LINE64             il;

        il.SizeOfStruct = sizeof(il);
        if (SymGetLineFromAddr64(dbg_curr_process->handle, lin, &disp, &il))
            dbg_printf(" [%s:%lu]", il.FileName, il.LineNumber);
        if (im.SizeOfStruct == 0) /* don't display again module if address is in module+disp form */
        {
            im.SizeOfStruct = sizeof(im);
            if (SymGetModuleInfo(dbg_curr_process->handle, lin, &im))
                dbg_printf(" in %s", im.ModuleName);
        }
    }
}

/***********************************************************************
 *           print_address
 *
 * Print an 16- or 32-bit address, with the nearest symbol if any.
 */
void print_address(const ADDRESS64* addr, BOOLEAN with_line)
{
    print_bare_address(addr);
    print_address_symbol(addr, with_line, "");
}

void memory_disasm_one_x86_insn(ADDRESS64 *addr, int display)
{
    ZydisDisassembledInstruction instr = { .runtime_address = addr->Offset };
    ZydisDecoder decoder;
    ZydisDecoderContext ctx;
    unsigned char buffer[16];
    SIZE_T len;
    int i;

    if (!dbg_curr_process->process_io->read( dbg_curr_process->handle, memory_to_linear_addr(addr),
                                             buffer, sizeof(buffer), &len )) return;

    switch (addr->Mode)
    {
    case AddrModeReal:
    case AddrMode1616:
        ZydisDecoderInit( &decoder, ZYDIS_MACHINE_MODE_LEGACY_16, ZYDIS_STACK_WIDTH_16 );
        break;
    default:
        if (ADDRSIZE == 4)
            ZydisDecoderInit( &decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32 );
        else
            ZydisDecoderInit( &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64 );
        break;
    }
    ZydisDecoderDecodeInstruction( &decoder, &ctx, buffer, len, &instr.info );
    ZydisDecoderDecodeOperands( &decoder, &ctx, &instr.info,
                                instr.operands, instr.info.operand_count );

    if (display)
    {
        ZydisFormatter formatter;
        ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_ATT );
        ZydisFormatterSetProperty( &formatter, ZYDIS_FORMATTER_PROP_HEX_UPPERCASE, 0 );
        ZydisFormatterFormatInstruction( &formatter, &instr.info, instr.operands,
                                         instr.info.operand_count_visible, instr.text,
                                         sizeof(instr.text), instr.runtime_address, NULL );
        dbg_printf( "%s", instr.text );
        for (i = 0; i < instr.info.operand_count_visible; i++)
        {
            ADDRESS64 a = { .Mode = AddrModeFlat };
            ZyanU64 addr;

            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress( &instr.info, &instr.operands[i],
                                                        instr.runtime_address, &addr )))
                continue;

            if (instr.info.meta.branch_type == ZYDIS_BRANCH_TYPE_NEAR &&
                instr.operands[i].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                instr.operands[i].mem.disp.has_displacement &&
                instr.operands[i].mem.index == ZYDIS_REGISTER_NONE &&
                (instr.operands[i].mem.base == ZYDIS_REGISTER_NONE ||
                 instr.operands[i].mem.base == ZYDIS_REGISTER_EIP ||
                 instr.operands[i].mem.base == ZYDIS_REGISTER_RIP))
            {
                unsigned char dest[8];
                if (dbg_read_memory( (void *)(ULONG_PTR)addr, dest, ADDRSIZE ))
                {
                    dbg_printf( " -> " );
                    a.Offset = ADDRSIZE == 4 ? *(DWORD *)dest : *(DWORD64 *)dest;
                    print_address( &a, TRUE );
                    break;
                }
            }
            a.Offset = addr;
            print_address_symbol( &a, TRUE, instr.info.operand_count_visible > 1 ? " ;" : "" );
            break;
        }
    }
    addr->Offset += instr.info.length;
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
    dbg_curr_process->be_cpu->disasm_one_insn(addr, TRUE);
    dbg_printf("\n");
    return TRUE;
}

void memory_disassemble(const struct dbg_lvalue* xstart, 
                        const struct dbg_lvalue* xend, int instruction_count)
{
    static ADDRESS64 last = {0,0,0};
    dbg_lgint_t stop = 0;
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

BOOL memory_get_register(DWORD regno, struct dbg_lvalue* lvalue, char* buffer, int len)
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

    for (div = dbg_curr_process->be_cpu->context_vars; div->name; div++)
    {
        if (div->val == regno)
        {
            if (!stack_get_register_frame(div, lvalue))
            {
                if (buffer) snprintf(buffer, len, "<register %s not accessible in this frame>", div->name);
                return FALSE;
            }

            if (buffer) lstrcpynA(buffer, div->name, len);
            return TRUE;
        }
    }
    if (buffer) snprintf(buffer, len, "<unknown register %lu>", regno);
    return FALSE;
}
