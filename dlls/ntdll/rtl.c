/*
 * NT basis DLL
 *
 * This file contains the Rtl* API functions. These should be implementable.
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 1999      Alex Korobka
 * Copyright 2003      Thomas Mertes
 * Crc32 code Copyright 1986 Gary S. Brown (Public domain)
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "ddk/ntifs.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(debugstr);

/* CRC polynomial 0xedb88320 */
static const DWORD CRC_table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/******************************************************************************
 *	DbgPrint	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...)
{
    NTSTATUS ret;
    va_list args;

    va_start(args, fmt);
    ret = vDbgPrintEx(0, DPFLTR_ERROR_LEVEL, fmt, args);
    va_end(args);
    return ret;
}


/******************************************************************************
 *	DbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrintEx(ULONG iComponentId, ULONG Level, LPCSTR fmt, ...)
{
    NTSTATUS ret;
    va_list args;

    va_start(args, fmt);
    ret = vDbgPrintEx(iComponentId, Level, fmt, args);
    va_end(args);
    return ret;
}

/******************************************************************************
 *	vDbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintEx( ULONG id, ULONG level, LPCSTR fmt, va_list args )
{
    return vDbgPrintExWithPrefix( "", id, level, fmt, args );
}

/******************************************************************************
 *	vDbgPrintExWithPrefix  [NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintExWithPrefix( LPCSTR prefix, ULONG id, ULONG level, LPCSTR fmt, va_list args )
{
    ULONG level_mask = level <= 31 ? (1 << level) : level;
    SIZE_T len = strlen( prefix );
    char buf[1024], *end;

    strcpy( buf, prefix );
    len += _vsnprintf( buf + len, sizeof(buf) - len, fmt, args );
    end = buf + len - 1;

    WARN_(debugstr)(*end == '\n' ? "%08lx:%08lx: %s" : "%08lx:%08lx: %s\n", id, level_mask, buf);

    if (level_mask & (1 << DPFLTR_ERROR_LEVEL) && NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            EXCEPTION_RECORD record;
            record.ExceptionCode    = DBG_PRINTEXCEPTION_C;
            record.ExceptionFlags   = 0;
            record.ExceptionRecord  = NULL;
            record.ExceptionAddress = RtlRaiseException;
            record.NumberParameters = 2;
            record.ExceptionInformation[1] = (ULONG_PTR)buf;
            record.ExceptionInformation[0] = strlen( buf ) + 1;
            RtlRaiseException( &record );
        }
        __EXCEPT(debug_exception_handler)
        {
        }
        __ENDTRY
    }

    return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlInitializeGenericTable           [NTDLL.@]
 */
void WINAPI RtlInitializeGenericTable(RTL_GENERIC_TABLE *table, PRTL_GENERIC_COMPARE_ROUTINE compare,
                                      PRTL_GENERIC_ALLOCATE_ROUTINE allocate, PRTL_GENERIC_FREE_ROUTINE free,
                                      void *context)
{
    TRACE("(%p, %p, %p, %p, %p)\n", table, compare, allocate, free, context);

    table->TableRoot = NULL;
    table->InsertOrderList.Flink = &table->InsertOrderList;
    table->InsertOrderList.Blink = &table->InsertOrderList;
    table->OrderedPointer = &table->InsertOrderList;
    table->NumberGenericTableElements = 0;
    table->WhichOrderedElement = 0;
    table->CompareRoutine = compare;
    table->AllocateRoutine = allocate;
    table->FreeRoutine = free;
    table->TableContext = context;
}

/******************************************************************************
 *  RtlEnumerateGenericTableWithoutSplaying           [NTDLL.@]
 */
void * WINAPI RtlEnumerateGenericTableWithoutSplaying(RTL_GENERIC_TABLE *table, PVOID *previous)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(%p, %p) stub!\n", table, previous);
    return NULL;
}

/******************************************************************************
 *  RtlNumberGenericTableElements           [NTDLL.@]
 */
ULONG WINAPI RtlNumberGenericTableElements(RTL_GENERIC_TABLE *table)
{
    TRACE("(%p)\n", table);
    return table->NumberGenericTableElements;
}

/******************************************************************************
 *  RtlGetElementGenericTable           [NTDLL.@]
 */
void * WINAPI RtlGetElementGenericTable(RTL_GENERIC_TABLE *table, ULONG index)
{
    FIXME("(%p, %lu) stub!\n", table, index);
    return NULL;
}

/******************************************************************************
 *  RtlLookupElementGenericTable           [NTDLL.@]
 */
void * WINAPI RtlLookupElementGenericTable(RTL_GENERIC_TABLE *table, void *buffer)
{
    FIXME("(%p, %p) stub!\n", table, buffer);
    return NULL;
}

/******************************************************************************
 *  RtlAssert                           [NTDLL.@]
 *
 * Fail a debug assertion.
 *
 * RETURNS
 *  Nothing. This call does not return control to its caller.
 *
 * NOTES
 * Not implemented in non-debug versions.
 */
void WINAPI RtlAssert(void *assertion, void *filename, ULONG linenumber, char *message)
{
    FIXME("(%s, %s, %lu, %s): stub\n", debugstr_a((char*)assertion), debugstr_a((char*)filename),
        linenumber, debugstr_a(message));
}

/*********************************************************************
 *                  RtlComputeCrc32   [NTDLL.@]
 *
 * Calculate the CRC32 checksum of a block of bytes
 *
 * PARAMS
 *  dwInitial [I] Initial CRC value
 *  pData     [I] Data block
 *  iLen      [I] Length of the byte block
 *
 * RETURNS
 *  The cumulative CRC32 of dwInitial and iLen bytes of the pData block.
 */
DWORD WINAPI RtlComputeCrc32(DWORD dwInitial, const BYTE *pData, INT iLen)
{
  DWORD crc = ~dwInitial;

  TRACE("(%lu,%p,%d)\n", dwInitial, pData, iLen);

  while (iLen > 0)
  {
    crc = CRC_table[(crc ^ *pData) & 0xff] ^ (crc >> 8);
    pData++;
    iLen--;
  }
  return ~crc;
}


/*************************************************************************
 * RtlUlonglongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned long long value.
 *
 * PARAMS
 *  i [I] Value to swap bytes of
 *
 * RETURNS
 *  The value with its bytes swapped.
 */
#ifdef __i386__
__ASM_FASTCALL_FUNC(RtlUlonglongByteSwap, 8,
                    "movl 4(%esp),%edx\n\t"
                    "bswap %edx\n\t"
                    "movl 8(%esp),%eax\n\t"
                    "bswap %eax\n\t"
                    "ret $8")
#endif

/*************************************************************************
 * RtlUlongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned int value.
 *
 * NOTES
 *  ix86 version takes argument in %ecx. Other systems use the inline version.
 */
#ifdef __i386__
__ASM_FASTCALL_FUNC(RtlUlongByteSwap, 4,
                    "movl %ecx,%eax\n\t"
                    "bswap %eax\n\t"
                    "ret")
#endif

/*************************************************************************
 * RtlUshortByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned short value.
 *
 * NOTES
 *  i386 version takes argument in %cx. Other systems use the inline version.
 */
#ifdef __i386__
__ASM_FASTCALL_FUNC(RtlUshortByteSwap, 4,
                    "movb %ch,%al\n\t"
                    "movb %cl,%ah\n\t"
                    "ret")
#endif


/*************************************************************************
 * RtlUniform   [NTDLL.@]
 *
 * Generates a uniform random number
 *
 * PARAMS
 *  seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number uniformly distributed over [0..MAXLONG-1].
 *
 * NOTES
 *  Generates a uniform random number using a linear congruential generator.
 *
 * DIFFERENCES
 *  The native documentation states that the random number is
 *  uniformly distributed over [0..MAXLONG]. In reality the native
 *  function and our function return a random number uniformly
 *  distributed over [0..MAXLONG-1].
 */
ULONG WINAPI RtlUniform( ULONG *seed )
{
    /* See the tests for details. */
    return (*seed = ((ULONGLONG)*seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff);
}


/*************************************************************************
 * RtlRandom   [NTDLL.@]
 *
 * Generates a random number
 *
 * PARAMS
 *  seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number distributed over [0..MAXLONG-1].
 */
ULONG WINAPI RtlRandom (PULONG seed)
{
    static ULONG saved_value[128] =
    { /*   0 */ 0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626, 0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa,
      /*   8 */ 0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8, 0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09,
      /*  16 */ 0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5, 0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311,
      /*  24 */ 0x404331a9, 0x74d97781, 0x64495118, 0x323e04be, 0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82,
      /*  32 */ 0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4, 0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd,
      /*  40 */ 0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016, 0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52,
      /*  48 */ 0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c, 0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb,
      /*  56 */ 0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8, 0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e,
      /*  64 */ 0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb, 0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0,
      /*  72 */ 0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743, 0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd,
      /*  80 */ 0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78, 0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35,
      /*  88 */ 0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a, 0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379,
      /*  96 */ 0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d, 0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd,
      /* 104 */ 0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515, 0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b,
      /* 112 */ 0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975, 0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b,
      /* 120 */ 0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb, 0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d };
    ULONG rand;
    int pos;
    ULONG result;

    rand = (*seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    *seed = (rand * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    pos = *seed & 0x7f;
    result = saved_value[pos];
    saved_value[pos] = rand;
    return(result);
}


/*************************************************************************
 * RtlRandomEx   [NTDLL.@]
 */
ULONG WINAPI RtlRandomEx( ULONG *seed )
{
    WARN( "semi-stub: should use a different algorithm\n" );
    return RtlRandom( seed );
}

/***********************************************************************
 * get_pointer_obfuscator (internal)
 */
static DWORD_PTR get_pointer_obfuscator( void )
{
    static DWORD_PTR pointer_obfuscator;

    if (!pointer_obfuscator)
    {
        ULONG seed = NtGetTickCount();
        ULONG_PTR rand;

        /* generate a random value for the obfuscator */
        rand = RtlUniform( &seed );

        /* handle 64bit pointers */
        rand ^= (ULONG_PTR)RtlUniform( &seed ) << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        /* set the high bits so dereferencing obfuscated pointers will (usually) crash */
        rand |= (ULONG_PTR)0xc0000000 << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        InterlockedCompareExchangePointer( (void**) &pointer_obfuscator, (void*) rand, NULL );
    }

    return pointer_obfuscator;
}

/*************************************************************************
 * RtlEncodePointer   [NTDLL.@]
 */
PVOID WINAPI RtlEncodePointer( PVOID ptr )
{
    DWORD_PTR ptrval = (DWORD_PTR) ptr;
    return (PVOID)(ptrval ^ get_pointer_obfuscator());
}

PVOID WINAPI RtlDecodePointer( PVOID ptr )
{
    DWORD_PTR ptrval = (DWORD_PTR) ptr;
    return (PVOID)(ptrval ^ get_pointer_obfuscator());
}

/******************************************************************************
 *  RtlGetCompressionWorkSpaceSize		[NTDLL.@]
 */
NTSTATUS WINAPI RtlGetCompressionWorkSpaceSize(USHORT format, PULONG compress_workspace,
                                               PULONG decompress_workspace)
{
    FIXME("0x%04x, %p, %p: semi-stub\n", format, compress_workspace, decompress_workspace);

    switch (format & COMPRESSION_FORMAT_MASK)
    {
        case COMPRESSION_FORMAT_LZNT1:
            if (compress_workspace)
            {
                /* FIXME: The current implementation of RtlCompressBuffer does not use a
                 * workspace buffer, but Windows applications might expect a nonzero value. */
                *compress_workspace = 16;
            }
            if (decompress_workspace)
                *decompress_workspace = 0x1000;
            return STATUS_SUCCESS;

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}

/* compress data using LZNT1, currently only a stub */
static NTSTATUS lznt1_compress(UCHAR *src, ULONG src_size, UCHAR *dst, ULONG dst_size,
                               ULONG chunk_size, ULONG *final_size, UCHAR *workspace)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG block_size;

    while (src_cur < src_end)
    {
        /* determine size of current chunk */
        block_size = min(0x1000, src_end - src_cur);
        if (dst_cur + sizeof(WORD) + block_size > dst_end)
            return STATUS_BUFFER_TOO_SMALL;

        /* write (uncompressed) chunk header */
        *(WORD *)dst_cur = 0x3000 | (block_size - 1);
        dst_cur += sizeof(WORD);

        /* write chunk content */
        memcpy(dst_cur, src_cur, block_size);
        dst_cur += block_size;
        src_cur += block_size;
    }

    if (final_size)
        *final_size = dst_cur - dst;

    return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlCompressBuffer		[NTDLL.@]
 */
NTSTATUS WINAPI RtlCompressBuffer(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                                  PUCHAR compressed, ULONG compressed_size, ULONG chunk_size,
                                  PULONG final_size, PVOID workspace)
{
    FIXME("0x%04x, %p, %lu, %p, %lu, %lu, %p, %p: semi-stub\n", format, uncompressed,
          uncompressed_size, compressed, compressed_size, chunk_size, final_size, workspace);

    switch (format & COMPRESSION_FORMAT_MASK)
    {
        case COMPRESSION_FORMAT_LZNT1:
            return lznt1_compress(uncompressed, uncompressed_size, compressed,
                                  compressed_size, chunk_size, final_size, workspace);

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}

/* decompress a single LZNT1 chunk */
static UCHAR *lznt1_decompress_chunk(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG displacement_bits, length_bits;
    ULONG code_displacement, code_length;
    WORD flags, code;

    while (src_cur < src_end && dst_cur < dst_end)
    {
        flags = 0x8000 | *src_cur++;
        while ((flags & 0xff00) && src_cur < src_end)
        {
            if (flags & 1)
            {
                /* backwards reference */
                if (src_cur + sizeof(WORD) > src_end)
                    return NULL;

                code = *(WORD *)src_cur;
                src_cur += sizeof(WORD);

                /* find length / displacement bits */
                for (displacement_bits = 12; displacement_bits > 4; displacement_bits--)
                    if ((1 << (displacement_bits - 1)) < dst_cur - dst) break;

                length_bits       = 16 - displacement_bits;
                code_length       = (code & ((1 << length_bits) - 1)) + 3;
                code_displacement = (code >> length_bits) + 1;

                if (dst_cur < dst + code_displacement)
                    return NULL;

                /* copy bytes of chunk - we can't use memcpy() since source and dest can
                 * be overlapping, and the same bytes can be repeated over and over again */
                while (code_length--)
                {
                    if (dst_cur >= dst_end) return dst_cur;
                    *dst_cur = *(dst_cur - code_displacement);
                    dst_cur++;
                }
            }
            else
            {
                /* uncompressed data */
                if (dst_cur >= dst_end) return dst_cur;
                *dst_cur++ = *src_cur++;
            }
            flags >>= 1;
        }
    }

    return dst_cur;
}

/* decompress data encoded with LZNT1 */
static NTSTATUS lznt1_decompress(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size,
                                 ULONG offset, ULONG *final_size, UCHAR *workspace)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG chunk_size, block_size;
    WORD chunk_header;
    UCHAR *ptr;

    if (src_cur + sizeof(WORD) > src_end)
        return STATUS_BAD_COMPRESSION_BUFFER;

    /* skip over chunks with a distance >= 0x1000 to the destination offset */
    while (offset >= 0x1000 && src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        src_cur += chunk_size;
        offset  -= 0x1000;
    }

    /* handle partially included chunk */
    if (offset && src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            if (!workspace) return STATUS_ACCESS_VIOLATION;
            ptr = lznt1_decompress_chunk(workspace, 0x1000, src_cur, chunk_size);
            if (!ptr) return STATUS_BAD_COMPRESSION_BUFFER;
            if (ptr - workspace > offset)
            {
                block_size = min((ptr - workspace) - offset, dst_end - dst_cur);
                memcpy(dst_cur, workspace + offset, block_size);
                dst_cur += block_size;
            }
        }
        else
        {
            /* uncompressed chunk */
            if (chunk_size > offset)
            {
                block_size = min(chunk_size - offset, dst_end - dst_cur);
                memcpy(dst_cur, src_cur + offset, block_size);
                dst_cur += block_size;
            }
        }

        src_cur += chunk_size;
    }

    /* handle remaining chunks */
    while (src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        /* fill space with padding when the previous chunk was decompressed
         * to less than 4096 bytes. no padding is needed for the last chunk
         * or when the next chunk is truncated */
        block_size = ((dst_cur - dst) + offset) & 0xfff;
        if (block_size)
        {
            block_size = 0x1000 - block_size;
            if (dst_cur + block_size >= dst_end)
                goto out;
            memset(dst_cur, 0, block_size);
            dst_cur += block_size;
        }

        if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            dst_cur = lznt1_decompress_chunk(dst_cur, dst_end - dst_cur, src_cur, chunk_size);
            if (!dst_cur) return STATUS_BAD_COMPRESSION_BUFFER;
        }
        else
        {
            /* uncompressed chunk */
            block_size = min(chunk_size, dst_end - dst_cur);
            memcpy(dst_cur, src_cur, block_size);
            dst_cur += block_size;
        }

        src_cur += chunk_size;
    }

out:
    if (final_size)
        *final_size = dst_cur - dst;

    return STATUS_SUCCESS;

}

/******************************************************************************
 *  RtlDecompressFragment	[NTDLL.@]
 */
NTSTATUS WINAPI RtlDecompressFragment(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                               PUCHAR compressed, ULONG compressed_size, ULONG offset,
                               PULONG final_size, PVOID workspace)
{
    TRACE("0x%04x, %p, %lu, %p, %lu, %lu, %p, %p\n", format, uncompressed,
          uncompressed_size, compressed, compressed_size, offset, final_size, workspace);

    switch (format & COMPRESSION_FORMAT_MASK)
    {
        case COMPRESSION_FORMAT_LZNT1:
            return lznt1_decompress(uncompressed, uncompressed_size, compressed,
                                    compressed_size, offset, final_size, workspace);

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}


/******************************************************************************
 *  RtlDecompressBuffer		[NTDLL.@]
 */
NTSTATUS WINAPI RtlDecompressBuffer(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                                    PUCHAR compressed, ULONG compressed_size, PULONG final_size)
{
    TRACE("0x%04x, %p, %lu, %p, %lu, %p\n", format, uncompressed,
        uncompressed_size, compressed, compressed_size, final_size);

    return RtlDecompressFragment(format, uncompressed, uncompressed_size,
                                 compressed, compressed_size, 0, final_size, NULL);
}

/******************************************************************************
 * RtlGetCurrentTransaction [NTDLL.@]
 */
HANDLE WINAPI RtlGetCurrentTransaction(void)
{
    FIXME("() :stub\n");
    return NULL;
}

/******************************************************************************
 * RtlSetCurrentTransaction [NTDLL.@]
 */
BOOL WINAPI RtlSetCurrentTransaction(HANDLE new_transaction)
{
    FIXME("(%p) :stub\n", new_transaction);
    return FALSE;
}

/**********************************************************************
 *           RtlGetCurrentProcessorNumberEx [NTDLL.@]
 */
void WINAPI RtlGetCurrentProcessorNumberEx(PROCESSOR_NUMBER *processor)
{
    FIXME("(%p) :semi-stub\n", processor);
    processor->Group = 0;
    processor->Number = NtGetCurrentProcessorNumber();
    processor->Reserved = 0;
}

static RTL_BALANCED_NODE *rtl_node_parent( RTL_BALANCED_NODE *node )
{
    return (RTL_BALANCED_NODE *)(node->ParentValue & ~(ULONG_PTR)RTL_BALANCED_NODE_RESERVED_PARENT_MASK);
}

static void rtl_set_node_parent( RTL_BALANCED_NODE *node, RTL_BALANCED_NODE *parent )
{
    node->ParentValue = (ULONG_PTR)parent | (node->ParentValue & RTL_BALANCED_NODE_RESERVED_PARENT_MASK);
}

static void rtl_rotate( RTL_RB_TREE *tree, RTL_BALANCED_NODE *n, int right )
{
    RTL_BALANCED_NODE *child = n->Children[!right];
    RTL_BALANCED_NODE *parent = rtl_node_parent( n );

    if (!parent)                tree->root = child;
    else if (parent->Left == n) parent->Left = child;
    else                        parent->Right = child;

    n->Children[!right] = child->Children[right];
    if (n->Children[!right]) rtl_set_node_parent( n->Children[!right], n );
    child->Children[right] = n;
    rtl_set_node_parent( child, parent );
    rtl_set_node_parent( n, child );
}

static void rtl_flip_color( RTL_BALANCED_NODE *node )
{
    node->Red = !node->Red;
    node->Left->Red = !node->Left->Red;
    node->Right->Red = !node->Right->Red;
}

/*********************************************************************
 *           RtlRbInsertNodeEx [NTDLL.@]
 */
void WINAPI RtlRbInsertNodeEx( RTL_RB_TREE *tree, RTL_BALANCED_NODE *parent, BOOLEAN right, RTL_BALANCED_NODE *node )
{
    RTL_BALANCED_NODE *grandparent;

    TRACE( "tree %p, parent %p, right %d, node %p.\n", tree, parent, right, node );

    node->ParentValue = (ULONG_PTR)parent;
    node->Left = NULL;
    node->Right = NULL;

    if (!parent)
    {
        tree->root = tree->min = node;
        return;
    }
    if (right > 1)
    {
        ERR( "right %d.\n", right );
        return;
    }
    if (parent->Children[right])
    {
        ERR( "parent %p, right %d, child %p.\n", parent, right, parent->Children[right] );
        return;
    }

    node->Red = 1;
    parent->Children[right] = node;
    if (tree->min == parent && parent->Left == node) tree->min = node;
    grandparent = rtl_node_parent( parent );
    while (parent->Red)
    {
        right = (parent == grandparent->Right);
        if (grandparent->Children[!right] && grandparent->Children[!right]->Red)
        {
            node = grandparent;
            rtl_flip_color( node );
            if (!(parent = rtl_node_parent( node ))) break;
            grandparent = rtl_node_parent( parent );
            continue;
        }
        if (node == parent->Children[!right])
        {
            node = parent;
            rtl_rotate( tree, node, right );
            parent = rtl_node_parent( node );
            grandparent = rtl_node_parent( parent );
        }
        parent->Red = 0;
        grandparent->Red = 1;
        rtl_rotate( tree, grandparent, !right );
    }
    tree->root->Red = 0;
}

/*********************************************************************
 *           RtlRbRemoveNode [NTDLL.@]
 */
void WINAPI RtlRbRemoveNode( RTL_RB_TREE *tree, RTL_BALANCED_NODE *node )
{
    RTL_BALANCED_NODE *iter = NULL, *child, *parent, *w;
    BOOL need_fixup;
    int right;

    TRACE( "tree %p, node %p.\n", tree, node );

    if (node->Right && (node->Left || tree->min == node))
    {
        for (iter = node->Right; iter->Left; iter = iter->Left)
            ;
        if (tree->min == node) tree->min = iter;
    }
    else if (tree->min == node) tree->min = rtl_node_parent( node );
    if (!iter || !node->Left) iter = node;

    child = iter->Left ? iter->Left : iter->Right;

    if (!(parent = rtl_node_parent( iter ))) tree->root = child;
    else if (iter == parent->Left)           parent->Left = child;
    else                                     parent->Right = child;

    if (child) rtl_set_node_parent( child, parent );

    need_fixup = !iter->Red;

    if (node != iter)
    {
        *iter = *node;
        if (!(w = rtl_node_parent( iter ))) tree->root = iter;
        else if (node == w->Left)           w->Left = iter;
        else                                w->Right = iter;

        if (iter->Right) rtl_set_node_parent( iter->Right, iter );
        if (iter->Left)  rtl_set_node_parent( iter->Left, iter );
        if (parent == node) parent = iter;
    }

    if (!need_fixup)
    {
        if (tree->root) tree->root->Red = 0;
        return;
    }

    while (parent && !(child && child->Red))
    {
        right = (child == parent->Right);
        w = parent->Children[!right];
        if (w->Red)
        {
            w->Red = 0;
            parent->Red = 1;
            rtl_rotate( tree, parent, right );
            w = parent->Children[!right];
        }
        if ((w->Left && w->Left->Red) || (w->Right && w->Right->Red))
        {
            if (!(w->Children[!right] && w->Children[!right]->Red))
            {
                w->Children[right]->Red = 0;
                w->Red = 1;
                rtl_rotate( tree, w, !right );
                w = parent->Children[!right];
            }
            w->Red = parent->Red;
            parent->Red = 0;
            if (w->Children[!right]) w->Children[!right]->Red = 0;
            rtl_rotate( tree, parent, right );
            child = NULL;
            break;
        }
        w->Red = 1;
        child = parent;
        parent = rtl_node_parent( child );
    }
    if (child) child->Red = 0;
    if (tree->root) tree->root->Red = 0;
}

/***********************************************************************
 *           RtlInitializeGenericTableAvl  (NTDLL.@)
 */
void WINAPI RtlInitializeGenericTableAvl(PRTL_AVL_TABLE table, PRTL_AVL_COMPARE_ROUTINE compare,
                                         PRTL_AVL_ALLOCATE_ROUTINE allocate, PRTL_AVL_FREE_ROUTINE free, void *context)
{
    FIXME("%p %p %p %p %p: stub\n", table, compare, allocate, free, context);
}

/******************************************************************************
 *           RtlEnumerateGenericTableWithoutSplayingAvl  (NTDLL.@)
 */
void * WINAPI RtlEnumerateGenericTableWithoutSplayingAvl(RTL_AVL_TABLE *table, PVOID *previous)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(%p, %p) stub!\n", table, previous);
    return NULL;
}

/******************************************************************************
 *  RtlNumberGenericTableElementsAvl  (NTDLL.@)
 */
ULONG WINAPI RtlNumberGenericTableElementsAvl(RTL_AVL_TABLE *table)
{
    FIXME("(%p) stub!\n", table);
    return 0;
}

/***********************************************************************
 *           RtlInsertElementGenericTableAvl  (NTDLL.@)
 */
void WINAPI RtlInsertElementGenericTableAvl(PRTL_AVL_TABLE table, void *buffer, ULONG size, BOOL *element)
{
    FIXME("%p %p %lu %p: stub\n", table, buffer, size, element);
}

/******************************************************************************
 *           RtlLookupElementGenericTableAvl  (NTDLL.@)
 */
void * WINAPI RtlLookupElementGenericTableAvl(PRTL_AVL_TABLE table, void *buffer)
{
    FIXME("(%p, %p) stub!\n", table, buffer);
    return NULL;
}

/*********************************************************************
 *           RtlQueryPackageIdentity [NTDLL.@]
 */
NTSTATUS WINAPI RtlQueryPackageIdentity(HANDLE token, WCHAR *fullname, SIZE_T *fullname_size,
                                        WCHAR *appid, SIZE_T *appid_size, BOOLEAN *packaged)
{
    FIXME("(%p, %p, %p, %p, %p, %p): stub\n", token, fullname, fullname_size, appid, appid_size, packaged);
    return STATUS_NOT_FOUND;
}

/*********************************************************************
 *           RtlQueryProcessPlaceholderCompatibilityMode [NTDLL.@]
 */
char WINAPI RtlQueryProcessPlaceholderCompatibilityMode(void)
{
    FIXME("stub\n");
    return PHCM_APPLICATION_DEFAULT;
}

/*********************************************************************
 *           RtlGetDeviceFamilyInfoEnum [NTDLL.@]
 */
void WINAPI RtlGetDeviceFamilyInfoEnum(ULONGLONG *version, DWORD *family, DWORD *form)
{
    FIXME("%p %p %p: stub\n", version, family, form);

    if (version) *version = 0;
    if (family) *family = DEVICEFAMILYINFOENUM_DESKTOP;
    if (form) *form = DEVICEFAMILYDEVICEFORM_UNKNOWN;
}

/*********************************************************************
 *           RtlConvertDeviceFamilyInfoToString [NTDLL.@]
 */
DWORD WINAPI RtlConvertDeviceFamilyInfoToString(DWORD *device_family_size, DWORD *device_form_size,
                                                WCHAR *device_family, WCHAR *device_form)
{
    static const WCHAR *windows_desktop = L"Windows.Desktop";
    static const WCHAR *unknown_form = L"Unknown";
    DWORD family_length, form_length;

    TRACE("%p %p %p %p\n", device_family_size, device_form_size, device_family, device_form);

    family_length = (wcslen(windows_desktop) + 1) * sizeof(WCHAR);
    form_length = (wcslen(unknown_form) + 1) * sizeof(WCHAR);

    if (*device_family_size < family_length || *device_form_size < form_length)
    {
        *device_family_size = family_length;
        *device_form_size = form_length;
        return STATUS_BUFFER_TOO_SMALL;
    }

    memcpy(device_family, windows_desktop, family_length);
    memcpy(device_form, unknown_form, form_length);
    return STATUS_SUCCESS;
}
