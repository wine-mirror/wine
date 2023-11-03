/*
 * PatchAPI PA19 file handlers
 *
 * Copyright 2019 Conor McCarthy
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
 *
 * TODO
 * - Normalization of 32-bit PE executable files and reversal of special
 *   processing of these executables is not implemented.
 *   Without normalization, old files cannot be validated for patching. The
 *   function NormalizeFileForPatchSignature() in Windows could be used to work
 *   out exactly how normalization works.
 *   Most/all of the special processing seems to be relocation of targets for
 *   some jump/call instructions to match more of the old file and improve
 *   compression. Patching of 64-bit exes works because mspatchc.dll does not
 *   implement special processing of them. In 32-bit patches, the variable
 *   named here 'unknown_count' seems to indicate presence of data related to
 *   reversing the processing. The changes that must be reversed occur at some,
 *   but not all, of the positions listed in the PE .reloc table.
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"

#include "patchapi.h"

#include "pa19.h"
#include "lzxd_dec.h"

WINE_DEFAULT_DEBUG_CHANNEL(mspatcha);

#define PA19_FILE_MAGIC 0x39314150
#define PATCH_OPTION_EXTRA_FLAGS 0x80000000

static UINT32 compute_zero_crc32(UINT32 crc, INT_PTR len)
{
    static const BYTE zero_buffer[1024];

    while (len)
    {
        crc = RtlComputeCrc32(crc, zero_buffer, min(len, sizeof(zero_buffer)));
        len -= min(len, sizeof(zero_buffer));
    }
    return crc;
}

/***********************************************************************************
 *  PatchAPI PA19 file header
 *
 *  BYTE magic[4];
 *  UINT32 options;
 *  UINT32 options_2; (present if PATCH_OPTION_EXTRA_FLAGS set)
 *  UINT32 timestamp; (if PATCH_OPTION_NO_TIMESTAMP is SET in options)
 *  UVLI rebase;      (present if PATCH_OPTION_NO_REBASE is not set; used on 32-bit executables)
 *  UVLI unpatched_size;
 *  UINT32 crc32_patched;
 *  BYTE input_file_count;
 *
 *  For each source file:
 *      SVLI (patched_size - unpatched_size);
 *      UINT32 crc32_unpatched;
 *      BYTE ignore_range_count;
 *      For each ignore range:
 *          SVLI OffsetInOldFile;
 *          UVLI LengthInBytes;
 *      BYTE retain_range_count;
 *      For each retain range:
 *          SVLI (OffsetInOldFile - (prevOffsetInOldFile + prevLengthInBytes));
 *          SVLI (OffsetInNewFile - OffsetInOldFile);
 *          UVLI LengthInBytes;
 *      UVLI unknown_count; (a count of pairs of values related to the reversal of special
 *                           processing done to improve compression of 32-bit executables)
 *      UVLI interleave_count; (present only if PATCH_OPTION_INTERLEAVE_FILES is set in options)
 *          UVLI interleave_values[interleave_count * 3 - 1];
 *      UVLI lzxd_input_size;
 *
 *  For each source file:
 *      UINT16 lzxd_block[lzxd_input_size / 2]; (NOT always 16-bit aligned)
 *
 *  UINT32 crc_hack; (rounds out the entire file crc32 to 0)
*/


#define MAX_RANGES 255

struct input_file_info {
    size_t input_size;
    DWORD crc32;
    BYTE ignore_range_count;
    BYTE retain_range_count;
    PATCH_IGNORE_RANGE ignore_table[MAX_RANGES];
    PATCH_RETAIN_RANGE retain_table[MAX_RANGES];
    size_t unknown_count;
    size_t stream_size;
    const BYTE *stream_start;
    int next_i;
    int next_r;
};

struct patch_file_header {
    DWORD flags;
    DWORD timestamp;
    size_t patched_size;
    DWORD patched_crc32;
    unsigned input_file_count;
    struct input_file_info *file_table;
    const BYTE *src;
    const BYTE *end;
    DWORD err;
};


/* Currently supported options. Some such as PATCH_OPTION_FAIL_IF_BIGGER don't
 * affect decoding but can get recorded in the patch file anyway */
#define PATCH_OPTION_SUPPORTED_FLAGS ( \
      PATCH_OPTION_USE_LZX_A \
    | PATCH_OPTION_USE_LZX_B \
    | PATCH_OPTION_USE_LZX_LARGE \
    | PATCH_OPTION_NO_BINDFIX \
    | PATCH_OPTION_NO_LOCKFIX \
    | PATCH_OPTION_NO_REBASE \
    | PATCH_OPTION_FAIL_IF_SAME_FILE \
    | PATCH_OPTION_FAIL_IF_BIGGER \
    | PATCH_OPTION_NO_CHECKSUM \
    | PATCH_OPTION_NO_RESTIMEFIX \
    | PATCH_OPTION_NO_TIMESTAMP \
    | PATCH_OPTION_EXTRA_FLAGS)


/* read a byte-aligned little-endian UINT32 from input and set error if eof
 */
static inline UINT32 read_raw_uint32(struct patch_file_header *ph)
{
    const BYTE *src = ph->src;

    ph->src += 4;
    if (ph->src > ph->end)
    {
        ph->err = ERROR_PATCH_CORRUPT;
        return 0;
    }
    return src[0]
        | (src[1] << 8)
        | (src[2] << 16)
        | (src[3] << 24);
}

/* Read a variable-length integer from a sequence of bytes terminated by
 * a value with bit 7 set. Set error if invalid or eof */
static UINT64 read_uvli(struct patch_file_header *ph)
{
    const BYTE *vli = ph->src;
    UINT64 n;
    ptrdiff_t i;
    ptrdiff_t limit = min(ph->end - vli, 9);

    if (ph->src >= ph->end)
    {
        ph->err = ERROR_PATCH_CORRUPT;
        return 0;
    }

    n = vli[0] & 0x7F;
    for (i = 1; i < limit && vli[i - 1] < 0x80; ++i)
        n += (UINT64)(vli[i] & 0x7F) << (7 * i);

    if (vli[i - 1] < 0x80)
    {
        TRACE("exceeded maximum vli size\n");
        ph->err = ERROR_PATCH_CORRUPT;
        return 0;
    }

    ph->src += i;

    return n;
}

/* Signed variant of the above. First byte sign flag is 0x40.
 */
static INT64 read_svli(struct patch_file_header *ph)
{
    const BYTE *vli = ph->src;
    INT64 n;
    ptrdiff_t i;
    ptrdiff_t limit = min(ph->end - vli, 9);

    if (ph->src >= ph->end)
    {
        ph->err = ERROR_PATCH_CORRUPT;
        return 0;
    }

    n = vli[0] & 0x3F;
    for (i = 1; i < limit && vli[i - 1] < 0x80; ++i)
        n += (INT64)(vli[i] & 0x7F) << (7 * i - 1);

    if (vli[i - 1] < 0x80)
    {
        TRACE("exceeded maximum vli size\n");
        ph->err = ERROR_PATCH_CORRUPT;
        return 0;
    }

    if (vli[0] & 0x40)
        n = -n;

    ph->src += i;

    return n;
}

static int __cdecl compare_ignored_range(const void *a, const void *b)
{
    LONG delta = ((PATCH_IGNORE_RANGE*)a)->OffsetInOldFile - ((PATCH_IGNORE_RANGE*)b)->OffsetInOldFile;
    if (delta > 0)
        return 1;
    if (delta < 0)
        return -1;
    return 0;
}

static int __cdecl compare_retained_range_old(const void *a, const void *b)
{
    LONG delta = ((PATCH_RETAIN_RANGE*)a)->OffsetInOldFile - ((PATCH_RETAIN_RANGE*)b)->OffsetInOldFile;
    if (delta > 0)
        return 1;
    if (delta < 0)
        return -1;
    return 0;
}

static int __cdecl compare_retained_range_new(const void *a, const void *b)
{
    LONG delta = ((PATCH_RETAIN_RANGE*)a)->OffsetInNewFile - ((PATCH_RETAIN_RANGE*)b)->OffsetInNewFile;
    if (delta > 0)
        return 1;
    if (delta < 0)
        return -1;
    return 0;
}

static int read_header(struct patch_file_header *ph, const BYTE *buf, size_t size)
{
    unsigned fileno;

    ph->src = buf;
    ph->end = buf + size;

    ph->file_table = NULL;
    ph->err = ERROR_SUCCESS;

    if (read_raw_uint32(ph) != PA19_FILE_MAGIC)
    {
        TRACE("no PA19 signature\n");
        ph->err = ERROR_PATCH_CORRUPT;
        return -1;
    }

    ph->flags = read_raw_uint32(ph);
    if ((ph->flags & PATCH_OPTION_SUPPORTED_FLAGS) != ph->flags)
    {
        FIXME("unsupported option flag(s): 0x%08lx\n", ph->flags & ~PATCH_OPTION_SUPPORTED_FLAGS);
        ph->err = ERROR_PATCH_PACKAGE_UNSUPPORTED;
        return -1;
    }

    /* additional 32-bit flag field */
    if (ph->flags & PATCH_OPTION_EXTRA_FLAGS)
    {
        TRACE("skipping extra flag field\n");
        (void)read_raw_uint32(ph);
    }

    /* the meaning of PATCH_OPTION_NO_TIMESTAMP is inverted for decoding */
    if(ph->flags & PATCH_OPTION_NO_TIMESTAMP)
        ph->timestamp = read_raw_uint32(ph);

    /* not sure what this value is for, but its absence seems to mean only that timestamps
     * in the decompressed 32-bit exe are not modified */
    if (!(ph->flags & PATCH_OPTION_NO_REBASE))
    {
        TRACE("skipping rebase field\n");
        (void)read_uvli(ph);
    }

    ph->patched_size = (size_t)read_uvli(ph);
    TRACE("patched file size will be %u\n", (unsigned)ph->patched_size);
    ph->patched_crc32 = read_raw_uint32(ph);

    ph->input_file_count = *ph->src;
    ++ph->src;
    TRACE("patch supports %u old file(s)\n", ph->input_file_count);
    /* if no old file used, input_file_count is still 1 */
    if (ph->input_file_count == 0)
    {
        ph->err = ERROR_PATCH_CORRUPT;
        return -1;
    }

    if (ph->err != ERROR_SUCCESS)
        return -1;

    ph->file_table = calloc(ph->input_file_count, sizeof(struct input_file_info));
    if (ph->file_table == NULL)
    {
        ph->err = ERROR_OUTOFMEMORY;
        return -1;
    }

    for (fileno = 0; fileno < ph->input_file_count; ++fileno) {
        struct input_file_info *fi = ph->file_table + fileno;
        ptrdiff_t delta;
        unsigned i;

        delta = (ptrdiff_t)read_svli(ph);
        fi->input_size = ph->patched_size + delta;

        fi->crc32 = read_raw_uint32(ph);

        fi->ignore_range_count = *ph->src;
        ++ph->src;
        TRACE("found %u range(s) to ignore\n", fi->ignore_range_count);

        for (i = 0; i < fi->ignore_range_count; ++i) {
            PATCH_IGNORE_RANGE *ir = fi->ignore_table + i;

            ir->OffsetInOldFile = (LONG)read_svli(ph);
            ir->LengthInBytes = (ULONG)read_uvli(ph);

            if (i != 0)
            {
                ir->OffsetInOldFile += fi->ignore_table[i - 1].OffsetInOldFile
                    + fi->ignore_table[i - 1].LengthInBytes;
            }
            if (ir->OffsetInOldFile > fi->input_size
                || ir->OffsetInOldFile + ir->LengthInBytes > fi->input_size
                || ir->LengthInBytes > fi->input_size)
            {
                ph->err = ERROR_PATCH_CORRUPT;
                return -1;
            }
        }

        fi->retain_range_count = *ph->src;
        ++ph->src;
        TRACE("found %u range(s) to retain\n", fi->retain_range_count);

        for (i = 0; i < fi->retain_range_count; ++i) {
            PATCH_RETAIN_RANGE *rr = fi->retain_table + i;

            rr->OffsetInOldFile = (LONG)read_svli(ph);
            if (i != 0)
                rr->OffsetInOldFile +=
                    fi->retain_table[i - 1].OffsetInOldFile + fi->retain_table[i - 1].LengthInBytes;

            rr->OffsetInNewFile = rr->OffsetInOldFile + (LONG)read_svli(ph);
            rr->LengthInBytes = (ULONG)read_uvli(ph);

            if (rr->OffsetInOldFile > fi->input_size
                || rr->OffsetInOldFile + rr->LengthInBytes > fi->input_size
                || rr->OffsetInNewFile > ph->patched_size
                || rr->OffsetInNewFile + rr->LengthInBytes > ph->patched_size
                || rr->LengthInBytes > ph->patched_size)
            {
                ph->err = ERROR_PATCH_CORRUPT;
                return -1;
            }

            /* ranges in new file must be equal and in the same order for all source files */
            if (fileno != 0)
            {
                PATCH_RETAIN_RANGE *rr_0 = ph->file_table[0].retain_table + i;
                if (rr->OffsetInNewFile != rr_0->OffsetInNewFile
                    || rr->LengthInBytes != rr_0->LengthInBytes)
                {
                    ph->err = ERROR_PATCH_CORRUPT;
                    return -1;
                }
            }
        }

        fi->unknown_count = (size_t)read_uvli(ph);
        if (fi->unknown_count)
        {
            FIXME("special processing of 32-bit executables not implemented.\n");
            ph->err = ERROR_PATCH_PACKAGE_UNSUPPORTED;
            return -1;
        }
        fi->stream_size = (size_t)read_uvli(ph);
    }

    for (fileno = 0; fileno < ph->input_file_count; ++fileno)
    {
        struct input_file_info *fi = ph->file_table + fileno;

        qsort(fi->ignore_table, fi->ignore_range_count, sizeof(fi->ignore_table[0]), compare_ignored_range);
        qsort(fi->retain_table, fi->retain_range_count, sizeof(fi->retain_table[0]), compare_retained_range_old);

        fi->stream_start = ph->src;
        ph->src += fi->stream_size;
    }

    /* skip the crc adjustment field */
    ph->src = min(ph->src + 4, ph->end);

    {
        UINT32 crc = RtlComputeCrc32(0, buf, ph->src - buf) ^ 0xFFFFFFFF;
        if (crc != 0)
        {
            TRACE("patch file crc32 failed\n");
            if (ph->src < ph->end)
                FIXME("probable header parsing error\n");
            ph->err = ERROR_PATCH_CORRUPT;
        }
    }

    return (ph->err == ERROR_SUCCESS) ? 0 : -1;
}

#define TICKS_PER_SEC 10000000
#define SEC_TO_UNIX_EPOCH ((369 * 365 + 89) * (ULONGLONG)86400)

static void posix_time_to_file_time(ULONG timestamp, FILETIME *ft)
{
    UINT64 ticks = ((UINT64)timestamp + SEC_TO_UNIX_EPOCH) * TICKS_PER_SEC;
    ft->dwLowDateTime = (DWORD)ticks;
    ft->dwHighDateTime = (DWORD)(ticks >> 32);
}

/* Get the next range to ignore in the old file.
 * fi->next_i must be initialized before use */
static ULONG next_ignored_range(const struct input_file_info *fi, size_t index, ULONG old_file_size, ULONG *end)
{
    ULONG start = old_file_size;
    *end = old_file_size;
    /* if patching is unnecessary, the ignored ranges are skipped during crc calc */
    if (fi->next_i < fi->ignore_range_count && fi->stream_size != 0)
    {
        start = fi->ignore_table[fi->next_i].OffsetInOldFile;
        *end = max(start + fi->ignore_table[fi->next_i].LengthInBytes, index);
        start = max(start, index);
    }
    return start;
}

/* Get the next range to retain from the old file.
 * fi->next_r must be initialized before use */
static ULONG next_retained_range_old(const struct input_file_info *fi, size_t index, ULONG old_file_size, ULONG *end)
{
    ULONG start = old_file_size;
    *end = old_file_size;
    if (fi->next_r < fi->retain_range_count)
    {
        start = fi->retain_table[fi->next_r].OffsetInOldFile;
        *end = max(start + fi->retain_table[fi->next_r].LengthInBytes, index);
        start = max(start, index);
    }
    return start;
}

/* Get the next range to retain in the new file.
 * fi->next_r must be initialized before use */
static ULONG next_retained_range_new(const struct input_file_info *fi, size_t index, ULONG new_file_size, ULONG *end)
{
    ULONG start = new_file_size;
    *end = new_file_size;
    if (fi->next_r < fi->retain_range_count)
    {
        start = fi->retain_table[fi->next_r].OffsetInNewFile;
        *end = max(start + fi->retain_table[fi->next_r].LengthInBytes, index);
        start = max(start, index);
    }
    return start;
}

/* Find the next range in the old file which must be assumed zero-filled during crc32 calc
 */
static ULONG next_zeroed_range(struct input_file_info *fi, size_t index, ULONG old_file_size, ULONG *end)
{
    ULONG start = old_file_size;
    ULONG end_i;
    ULONG start_i;
    ULONG end_r;
    ULONG start_r;

    *end = old_file_size;

    start_i = next_ignored_range(fi, index, old_file_size, &end_i);
    start_r = next_retained_range_old(fi, index, old_file_size, &end_r);

    if (start_i < start_r)
    {
        start = start_i;
        *end = end_i;
        ++fi->next_i;
    }
    else
    {
        start = start_r;
        *end = end_r;
        ++fi->next_r;
    }
    return start;
}

/* Use the crc32 of the input file to match the file with an entry in the patch file table
 */
struct input_file_info *find_matching_old_file(const struct patch_file_header *ph, const BYTE *old_file_view, ULONG old_file_size)
{
    unsigned i;

    for (i = 0; i < ph->input_file_count; ++i)
    {
        DWORD crc32 = 0;
        ULONG index;

        if (ph->file_table[i].input_size != old_file_size)
            continue;

        ph->file_table[i].next_i = 0;
        for (index = 0; index < old_file_size; )
        {
            ULONG end;
            ULONG start = next_zeroed_range(ph->file_table + i, index, old_file_size, &end);
            crc32 = RtlComputeCrc32(crc32, old_file_view + index, start - index);
            crc32 = compute_zero_crc32(crc32, end - start);
            index = end;
        }
        if (ph->file_table[i].crc32 == crc32)
            return ph->file_table + i;
    }
    return NULL;
}

/* Zero-fill ignored ranges in the old file data for decoder matching
 */
static void zero_fill_ignored_ranges(BYTE *old_file_buf, const struct input_file_info *fi)
{
    size_t i;
    for (i = 0; i < fi->ignore_range_count; ++i)
    {
        memset(old_file_buf + fi->ignore_table[i].OffsetInOldFile,
            0,
            fi->ignore_table[i].LengthInBytes);
    }
}

/* Zero-fill retained ranges in the old file data for decoder matching
 */
static void zero_fill_retained_ranges(BYTE *old_file_buf, BYTE *new_file_buf, const struct input_file_info *fi)
{
    size_t i;
    for (i = 0; i < fi->retain_range_count; ++i)
    {
        memset(old_file_buf + fi->retain_table[i].OffsetInOldFile,
            0,
            fi->retain_table[i].LengthInBytes);
    }
}

/* Copy the retained ranges to the new file buffer
 */
static void apply_retained_ranges(const BYTE *old_file_buf, BYTE *new_file_buf, const struct input_file_info *fi)
{
    size_t i;

    if (old_file_buf == NULL)
        return;

    for (i = 0; i < fi->retain_range_count; ++i)
    {
        memcpy(new_file_buf + fi->retain_table[i].OffsetInNewFile,
            old_file_buf + fi->retain_table[i].OffsetInOldFile,
            fi->retain_table[i].LengthInBytes);
    }
}

/* Compute the crc32 for the new file, assuming zero for the retained ranges
 */
static DWORD compute_target_crc32(struct input_file_info *fi, const BYTE *new_file_buf, ULONG new_file_size)
{
    DWORD crc32 = 0;
    ULONG index;

    qsort(fi->retain_table, fi->retain_range_count, sizeof(fi->retain_table[0]), compare_retained_range_new);
    fi->next_r = 0;

    for (index = 0; index < new_file_size; )
    {
        ULONG end;
        ULONG start = next_retained_range_new(fi, index, new_file_size, &end);
        ++fi->next_r;
        crc32 = RtlComputeCrc32(crc32, new_file_buf + index, start - index);
        crc32 = compute_zero_crc32(crc32, end - start);
        index = end;
    }
    return crc32;
}

DWORD apply_patch_to_file_by_buffers(const BYTE *patch_file_view, const ULONG patch_file_size,
    const BYTE *old_file_view, ULONG old_file_size,
    BYTE **pnew_file_buf, const ULONG new_file_buf_size, ULONG *new_file_size,
    FILETIME *new_file_time,
    const ULONG apply_option_flags,
    PATCH_PROGRESS_CALLBACK *progress_fn, void *progress_ctx,
    const BOOL test_header_only)
{
    DWORD err = ERROR_SUCCESS;
    struct input_file_info *file_info;
    struct patch_file_header ph;
    size_t buf_size;
    BYTE *new_file_buf = NULL;
    BYTE *decode_buf = NULL;

    if (pnew_file_buf == NULL)
    {
        if (!test_header_only && !(apply_option_flags & APPLY_OPTION_TEST_ONLY))
            return ERROR_INVALID_PARAMETER;
    }
    else
    {
        new_file_buf = *pnew_file_buf;
    }

    if (old_file_view == NULL)
        old_file_size = 0;

    if (read_header(&ph, patch_file_view, patch_file_size))
    {
        err = ph.err;
        goto free_patch_header;
    }

    if (new_file_size != NULL)
        *new_file_size = (ULONG)ph.patched_size;

    if (new_file_buf != NULL && new_file_buf_size < ph.patched_size)
    {
        err = ERROR_INSUFFICIENT_BUFFER;
        goto free_patch_header;
    }

    file_info = find_matching_old_file(&ph, old_file_view, old_file_size);
    if (file_info == NULL)
    {
        err = ERROR_PATCH_WRONG_FILE;
        goto free_patch_header;
    }
    if (file_info->input_size != old_file_size)
    {
        err = ERROR_PATCH_CORRUPT;
        goto free_patch_header;
    }
    if (file_info->stream_size == 0 && (apply_option_flags & APPLY_OPTION_FAIL_IF_EXACT))
    {
        err = ERROR_PATCH_NOT_NECESSARY;
        goto free_patch_header;
    }
    if (file_info->stream_size != 0
        && file_info->input_size > ((ph.flags & PATCH_OPTION_USE_LZX_LARGE) ? MAX_LARGE_WINDOW : MAX_NORMAL_WINDOW))
    {
        /* interleaved by default but not the same as PATCH_OPTION_INTERLEAVE_FILES */
        FIXME("interleaved LZXD decompression is not supported.\n");
        err = ERROR_PATCH_PACKAGE_UNSUPPORTED;
        goto free_patch_header;
    }

    if (test_header_only)
        goto free_patch_header;

    /* missing lzxd stream means it's a header test extract */
    if (file_info->stream_start + file_info->stream_size > ph.end)
    {
        err = ERROR_PATCH_NOT_AVAILABLE;
        goto free_patch_header;
    }

    buf_size = old_file_size + ph.patched_size;
    decode_buf = new_file_buf;
    if (new_file_buf == NULL || new_file_buf_size < buf_size)
    {
        /* decode_buf must have room for both files, so allocate a new buffer if
         * necessary. This will be returned to the caller if new_file_buf == NULL */
        decode_buf = VirtualAlloc(NULL, buf_size, MEM_COMMIT, PAGE_READWRITE);
        if (decode_buf == NULL)
        {
            err = GetLastError();
            goto free_patch_header;
        }
    }

    if (old_file_view != NULL)
        memcpy(decode_buf, old_file_view, file_info->input_size);

    zero_fill_ignored_ranges(decode_buf, file_info);
    zero_fill_retained_ranges(decode_buf, decode_buf + file_info->input_size, file_info);

    if (file_info->stream_size != 0)
    {
        err = decode_lzxd_stream(file_info->stream_start, file_info->stream_size,
            decode_buf, ph.patched_size, file_info->input_size,
            ph.flags & PATCH_OPTION_USE_LZX_LARGE,
            progress_fn, progress_ctx);
    }
    else if (file_info->input_size == ph.patched_size)
    {
        /* files are identical so copy old to new. copying is avoidable but rare */
        memcpy(decode_buf + file_info->input_size, decode_buf, ph.patched_size);
    }
    else
    {
        err = ERROR_PATCH_CORRUPT;
        goto free_decode_buf;
    }

    if(err != ERROR_SUCCESS)
    {
        if (err == ERROR_PATCH_DECODE_FAILURE)
            FIXME("decode failure: data corruption or bug.\n");
        goto free_decode_buf;
    }

    apply_retained_ranges(old_file_view, decode_buf + file_info->input_size, file_info);

    if (ph.patched_crc32 != compute_target_crc32(file_info, decode_buf + file_info->input_size, ph.patched_size))
    {
        err = ERROR_PATCH_CORRUPT;
        goto free_decode_buf;
    }

    /* retained ranges must be ignored for this test */
    if ((apply_option_flags & APPLY_OPTION_FAIL_IF_EXACT)
        && file_info->input_size == ph.patched_size
        && memcmp(decode_buf, decode_buf + file_info->input_size, ph.patched_size) == 0)
    {
        err = ERROR_PATCH_NOT_NECESSARY;
        goto free_decode_buf;
    }

    if (!(apply_option_flags & APPLY_OPTION_TEST_ONLY))
    {
        if (new_file_buf == NULL)
        {
            /* caller will VirtualFree the buffer */
            new_file_buf = decode_buf;
            *pnew_file_buf = new_file_buf;
        }
        memmove(new_file_buf, decode_buf + old_file_size, ph.patched_size);
    }

    if (new_file_time != NULL)
    {
        new_file_time->dwLowDateTime = 0;
        new_file_time->dwHighDateTime = 0;

        /* the meaning of PATCH_OPTION_NO_TIMESTAMP is inverted for decoding */
        if (ph.flags & PATCH_OPTION_NO_TIMESTAMP)
            posix_time_to_file_time(ph.timestamp, new_file_time);
    }

free_decode_buf:
    if(decode_buf != NULL && decode_buf != new_file_buf)
        VirtualFree(decode_buf, 0, MEM_RELEASE);

free_patch_header:
    free(ph.file_table);

    return err;
}

BOOL apply_patch_to_file_by_handles(HANDLE patch_file_hndl, HANDLE old_file_hndl, HANDLE new_file_hndl,
    const ULONG apply_option_flags,
    PATCH_PROGRESS_CALLBACK *progress_fn, void *progress_ctx,
    const BOOL test_header_only)
{
    LARGE_INTEGER patch_size;
    LARGE_INTEGER old_size;
    HANDLE patch_map;
    HANDLE old_map = NULL;
    BYTE *patch_buf;
    const BYTE *old_buf = NULL;
    BYTE *new_buf = NULL;
    ULONG new_size;
    FILETIME new_time;
    BOOL res = FALSE;
    DWORD err = ERROR_SUCCESS;

    /* truncate the output file if required, or set the handle to invalid */
    if (test_header_only || (apply_option_flags & APPLY_OPTION_TEST_ONLY))
    {
        new_file_hndl = INVALID_HANDLE_VALUE;
    }
    else if (SetFilePointer(new_file_hndl, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER
        || !SetEndOfFile(new_file_hndl))
    {
        err = GetLastError();
        return FALSE;
    }

    if (patch_file_hndl == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    old_size.QuadPart = 0;
    if (!GetFileSizeEx(patch_file_hndl, &patch_size)
        || (old_file_hndl != INVALID_HANDLE_VALUE && !GetFileSizeEx(old_file_hndl, &old_size)))
    {
        /* Last error set by API */
        return FALSE;
    }

    patch_map = CreateFileMappingW(patch_file_hndl, NULL, PAGE_READONLY, 0, 0, NULL);
    if (patch_map == NULL)
    {
        /* Last error set by API */
        return FALSE;
    }

    if (old_file_hndl != INVALID_HANDLE_VALUE)
    {
        old_map = CreateFileMappingW(old_file_hndl, NULL, PAGE_READONLY, 0, 0, NULL);
        if (old_map == NULL)
        {
            err = GetLastError();
            goto close_patch_map;
        }
    }

    patch_buf = MapViewOfFile(patch_map, FILE_MAP_READ, 0, 0, (SIZE_T)patch_size.QuadPart);
    if (patch_buf == NULL)
    {
        err = GetLastError();
        goto close_old_map;
    }

    if (old_size.QuadPart)
    {
        old_buf = MapViewOfFile(old_map, FILE_MAP_READ, 0, 0, (SIZE_T)old_size.QuadPart);
        if (old_buf == NULL)
        {
            err = GetLastError();
            goto unmap_patch_buf;
        }
    }

    err = apply_patch_to_file_by_buffers(patch_buf, (ULONG)patch_size.QuadPart,
        old_buf, (ULONG)old_size.QuadPart,
        &new_buf, 0, &new_size,
        &new_time,
        apply_option_flags, progress_fn, progress_ctx,
        test_header_only);

    if(err)
        goto free_new_buf;

    res = TRUE;

    if(new_file_hndl != INVALID_HANDLE_VALUE)
    {
        DWORD Written = 0;
        res = WriteFile(new_file_hndl, new_buf, new_size, &Written, NULL);

        if (!res)
            err = GetLastError();
        else if (new_time.dwLowDateTime || new_time.dwHighDateTime)
            SetFileTime(new_file_hndl, &new_time, NULL, &new_time);
    }

free_new_buf:
    if (new_buf != NULL)
        VirtualFree(new_buf, 0, MEM_RELEASE);

    if (old_buf != NULL)
        UnmapViewOfFile(old_buf);

unmap_patch_buf:
    UnmapViewOfFile(patch_buf);

close_old_map:
    if (old_map != NULL)
        CloseHandle(old_map);

close_patch_map:
    CloseHandle(patch_map);

    SetLastError(err);

    return res;
}

BOOL apply_patch_to_file(LPCWSTR patch_file_name, LPCWSTR old_file_name, LPCWSTR new_file_name,
    const ULONG apply_option_flags,
    PATCH_PROGRESS_CALLBACK *progress_fn, void *progress_ctx,
    const BOOL test_header_only)
{
    HANDLE patch_hndl;
    HANDLE old_hndl = INVALID_HANDLE_VALUE;
    HANDLE new_hndl = INVALID_HANDLE_VALUE;
    BOOL res = FALSE;
    DWORD err = ERROR_SUCCESS;

    patch_hndl = CreateFileW(patch_file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (patch_hndl == INVALID_HANDLE_VALUE)
    {
        /* last error set by CreateFileW */
        return FALSE;
    }

    if (old_file_name != NULL)
    {
        old_hndl = CreateFileW(old_file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (old_hndl == INVALID_HANDLE_VALUE)
        {
            err = GetLastError();
            goto close_patch_file;
        }
    }

    if (!test_header_only && !(apply_option_flags & APPLY_OPTION_TEST_ONLY))
    {
        new_hndl = CreateFileW(new_file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (new_hndl == INVALID_HANDLE_VALUE)
        {
            err = GetLastError();
            goto close_old_file;
        }
    }

    res = apply_patch_to_file_by_handles(patch_hndl, old_hndl, new_hndl, apply_option_flags, progress_fn, progress_ctx, test_header_only);
    if(!res)
        err = GetLastError();

    if (new_hndl != INVALID_HANDLE_VALUE)
    {
        CloseHandle(new_hndl);
        if (!res)
            DeleteFileW(new_file_name);
    }

close_old_file:
    if (old_hndl != INVALID_HANDLE_VALUE)
        CloseHandle(old_hndl);

close_patch_file:
    CloseHandle(patch_hndl);

    /* set last error even on success as per windows */
    SetLastError(err);

    return res;
}
