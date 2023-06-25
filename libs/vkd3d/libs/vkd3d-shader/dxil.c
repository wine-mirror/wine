/*
 * Copyright 2023 Conor McCarthy for CodeWeavers
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

#include "vkd3d_shader_private.h"

#define VKD3D_SM6_VERSION_MAJOR(version) (((version) >> 4) & 0xf)
#define VKD3D_SM6_VERSION_MINOR(version) (((version) >> 0) & 0xf)

#define BITCODE_MAGIC VKD3D_MAKE_TAG('B', 'C', 0xc0, 0xde)
#define DXIL_OP_MAX_OPERANDS 17

enum bitcode_block_id
{
    BLOCKINFO_BLOCK           =  0,
    MODULE_BLOCK              =  8,
    PARAMATTR_BLOCK           =  9,
    PARAMATTR_GROUP_BLOCK     = 10,
    CONSTANTS_BLOCK           = 11,
    FUNCTION_BLOCK            = 12,
    VALUE_SYMTAB_BLOCK        = 14,
    METADATA_BLOCK            = 15,
    METADATA_ATTACHMENT_BLOCK = 16,
    TYPE_BLOCK                = 17,
    USELIST_BLOCK             = 18,
};

enum bitcode_blockinfo_code
{
    SETBID = 1,
    BLOCKNAME = 2,
    SETRECORDNAME = 3,
};

enum bitcode_block_abbreviation
{
    END_BLOCK = 0,
    ENTER_SUBBLOCK = 1,
    DEFINE_ABBREV = 2,
    UNABBREV_RECORD = 3,
};

enum bitcode_abbrev_type
{
    ABBREV_FIXED = 1,
    ABBREV_VBR   = 2,
    ABBREV_ARRAY = 3,
    ABBREV_CHAR  = 4,
    ABBREV_BLOB  = 5,
};

enum bitcode_address_space
{
    ADDRESS_SPACE_DEFAULT,
    ADDRESS_SPACE_DEVICEMEM,
    ADDRESS_SPACE_CBUFFER,
    ADDRESS_SPACE_GROUPSHARED,
};

enum bitcode_module_code
{
    MODULE_CODE_VERSION     =  1,
    MODULE_CODE_GLOBALVAR   =  7,
    MODULE_CODE_FUNCTION    =  8,
};

enum bitcode_constant_code
{
    CST_CODE_SETTYPE         =  1,
    CST_CODE_NULL            =  2,
    CST_CODE_UNDEF           =  3,
    CST_CODE_INTEGER         =  4,
    CST_CODE_FLOAT           =  6,
    CST_CODE_STRING          =  8,
    CST_CODE_CE_GEP          = 12,
    CST_CODE_CE_INBOUNDS_GEP = 20,
    CST_CODE_DATA            = 22,
};

enum bitcode_function_code
{
    FUNC_CODE_DECLAREBLOCKS    =  1,
    FUNC_CODE_INST_BINOP       =  2,
    FUNC_CODE_INST_CAST        =  3,
    FUNC_CODE_INST_RET         = 10,
    FUNC_CODE_INST_BR          = 11,
    FUNC_CODE_INST_SWITCH      = 12,
    FUNC_CODE_INST_PHI         = 16,
    FUNC_CODE_INST_ALLOCA      = 19,
    FUNC_CODE_INST_LOAD        = 20,
    FUNC_CODE_INST_EXTRACTVAL  = 26,
    FUNC_CODE_INST_CMP2        = 28,
    FUNC_CODE_INST_VSELECT     = 29,
    FUNC_CODE_INST_CALL        = 34,
    FUNC_CODE_INST_ATOMICRMW   = 38,
    FUNC_CODE_INST_LOADATOMIC  = 41,
    FUNC_CODE_INST_GEP         = 43,
    FUNC_CODE_INST_STORE       = 44,
    FUNC_CODE_INST_STOREATOMIC = 45,
    FUNC_CODE_INST_CMPXCHG     = 46,
};

enum bitcode_type_code
{
    TYPE_CODE_NUMENTRY     =  1,
    TYPE_CODE_VOID         =  2,
    TYPE_CODE_FLOAT        =  3,
    TYPE_CODE_DOUBLE       =  4,
    TYPE_CODE_LABEL        =  5,
    TYPE_CODE_INTEGER      =  7,
    TYPE_CODE_POINTER      =  8,
    TYPE_CODE_HALF         = 10,
    TYPE_CODE_ARRAY        = 11,
    TYPE_CODE_VECTOR       = 12,
    TYPE_CODE_METADATA     = 16,
    TYPE_CODE_STRUCT_ANON  = 18,
    TYPE_CODE_STRUCT_NAME  = 19,
    TYPE_CODE_STRUCT_NAMED = 20,
    TYPE_CODE_FUNCTION     = 21,
};

enum bitcode_value_symtab_code
{
    VST_CODE_ENTRY   = 1,
    VST_CODE_BBENTRY = 2,
};

enum dx_intrinsic_opcode
{
    DX_STORE_OUTPUT                 =   5,
};

struct sm6_pointer_info
{
    const struct sm6_type *type;
    enum bitcode_address_space addr_space;
};

struct sm6_struct_info
{
    const char *name;
    unsigned int elem_count;
    const struct sm6_type *elem_types[];
};

struct sm6_function_info
{
    const struct sm6_type *ret_type;
    unsigned int param_count;
    const struct sm6_type *param_types[];
};

struct sm6_array_info
{
    unsigned int count;
    const struct sm6_type *elem_type;
};

enum sm6_type_class
{
    TYPE_CLASS_VOID,
    TYPE_CLASS_INTEGER,
    TYPE_CLASS_FLOAT,
    TYPE_CLASS_POINTER,
    TYPE_CLASS_STRUCT,
    TYPE_CLASS_FUNCTION,
    TYPE_CLASS_VECTOR,
    TYPE_CLASS_ARRAY,
    TYPE_CLASS_LABEL,
    TYPE_CLASS_METADATA,
};

struct sm6_type
{
    enum sm6_type_class class;
    union
    {
        unsigned int width;
        struct sm6_pointer_info pointer;
        struct sm6_struct_info *struc;
        struct sm6_function_info *function;
        struct sm6_array_info array;
    } u;
};

enum sm6_value_type
{
    VALUE_TYPE_FUNCTION,
    VALUE_TYPE_REG,
};

struct sm6_function_data
{
    const char *name;
    bool is_prototype;
    unsigned int attribs_id;
};

struct sm6_value
{
    const struct sm6_type *type;
    enum sm6_value_type value_type;
    bool is_undefined;
    union
    {
        struct sm6_function_data function;
        struct vkd3d_shader_register reg;
    } u;
};

struct dxil_record
{
    unsigned int code;
    unsigned int operand_count;
    uint64_t operands[];
};

struct sm6_symbol
{
    unsigned int id;
    const char *name;
};

struct sm6_block
{
    struct vkd3d_shader_instruction *instructions;
    size_t instruction_capacity;
    size_t instruction_count;
};

struct sm6_function
{
    const struct sm6_value *declaration;

    struct sm6_block *blocks[1];
    size_t block_count;

    size_t value_count;
};

struct dxil_block
{
    const struct dxil_block *parent;
    enum bitcode_block_id id;
    unsigned int abbrev_len;
    unsigned int start;
    unsigned int length;
    unsigned int level;

    /* The abbrev, block and record structs are not relocatable. */
    struct dxil_abbrev **abbrevs;
    size_t abbrev_capacity;
    size_t abbrev_count;
    unsigned int blockinfo_bid;
    bool has_bid;

    struct dxil_block **child_blocks;
    size_t child_block_capacity;
    size_t child_block_count;

    struct dxil_record **records;
    size_t record_capacity;
    size_t record_count;
};

struct sm6_parser
{
    const uint32_t *ptr, *start, *end;
    unsigned int bitpos;

    struct dxil_block root_block;
    struct dxil_block *current_block;

    struct dxil_global_abbrev **abbrevs;
    size_t abbrev_capacity;
    size_t abbrev_count;

    struct sm6_type *types;
    size_t type_count;

    struct sm6_symbol *global_symbols;
    size_t global_symbol_count;

    struct vkd3d_shader_dst_param *output_params;

    struct sm6_function *functions;
    size_t function_count;

    struct sm6_value *values;
    size_t value_count;
    size_t value_capacity;
    size_t cur_max_value;

    struct vkd3d_shader_parser p;
};

struct dxil_abbrev_operand
{
    uint64_t context;
    bool (*read_operand)(struct sm6_parser *sm6, uint64_t context, uint64_t *operand);
};

struct dxil_abbrev
{
    unsigned int count;
    bool is_array;
    struct dxil_abbrev_operand operands[];
};

struct dxil_global_abbrev
{
    unsigned int block_id;
    struct dxil_abbrev abbrev;
};

static const uint64_t CALL_CONV_FLAG_EXPLICIT_TYPE = 1ull << 15;

static size_t size_add_with_overflow_check(size_t a, size_t b)
{
    size_t i = a + b;
    return (i < a) ? SIZE_MAX : i;
}

static struct sm6_parser *sm6_parser(struct vkd3d_shader_parser *parser)
{
    return CONTAINING_RECORD(parser, struct sm6_parser, p);
}

static bool sm6_parser_is_end(struct sm6_parser *sm6)
{
    return sm6->ptr == sm6->end;
}

static uint32_t sm6_parser_read_uint32(struct sm6_parser *sm6)
{
    if (sm6_parser_is_end(sm6))
    {
        sm6->p.failed = true;
        return 0;
    }
    return *sm6->ptr++;
}

static uint32_t sm6_parser_read_bits(struct sm6_parser *sm6, unsigned int length)
{
    unsigned int l, prev_len = 0;
    uint32_t bits;

    if (!length)
        return 0;

    assert(length < 32);

    if (sm6_parser_is_end(sm6))
    {
        sm6->p.failed = true;
        return 0;
    }

    assert(sm6->bitpos < 32);
    bits = *sm6->ptr >> sm6->bitpos;
    l = 32 - sm6->bitpos;
    if (l <= length)
    {
        ++sm6->ptr;
        if (sm6_parser_is_end(sm6) && l < length)
        {
            sm6->p.failed = true;
            return bits;
        }
        sm6->bitpos = 0;
        bits |= *sm6->ptr << l;
        prev_len = l;
    }
    sm6->bitpos += length - prev_len;

    return bits & ((1 << length) - 1);
}

static uint64_t sm6_parser_read_vbr(struct sm6_parser *sm6, unsigned int length)
{
    unsigned int bits, flag, mask, shift = 0;
    uint64_t result = 0;

    if (!length)
        return 0;

    if (sm6_parser_is_end(sm6))
    {
        sm6->p.failed = true;
        return 0;
    }

    flag = 1 << (length - 1);
    mask = flag - 1;
    do
    {
        bits = sm6_parser_read_bits(sm6, length);
        result |= (uint64_t)(bits & mask) << shift;
        shift += length - 1;
    } while ((bits & flag) && !sm6->p.failed && shift < 64);

    sm6->p.failed |= !!(bits & flag);

    return result;
}

static void sm6_parser_align_32(struct sm6_parser *sm6)
{
    if (!sm6->bitpos)
        return;

    if (sm6_parser_is_end(sm6))
    {
        sm6->p.failed = true;
        return;
    }

    ++sm6->ptr;
    sm6->bitpos = 0;
}

static bool dxil_block_handle_blockinfo_record(struct dxil_block *block, struct dxil_record *record)
{
    /* BLOCKINFO blocks must only occur immediately below the module root block. */
    if (block->level > 1)
    {
        WARN("Invalid blockinfo block level %u.\n", block->level);
        return false;
    }

    switch (record->code)
    {
        case SETBID:
            if (!record->operand_count)
            {
                WARN("Missing id operand.\n");
                return false;
            }
            if (record->operands[0] > UINT_MAX)
                WARN("Truncating block id %"PRIu64".\n", record->operands[0]);
            block->blockinfo_bid = record->operands[0];
            block->has_bid = true;
            break;
        case BLOCKNAME:
        case SETRECORDNAME:
            break;
        default:
            FIXME("Unhandled BLOCKINFO record type %u.\n", record->code);
            break;
    }

    return true;
}

static enum vkd3d_result dxil_block_add_record(struct dxil_block *block, struct dxil_record *record)
{
    unsigned int reserve;

    switch (block->id)
    {
        /* Rough initial reserve sizes for small shaders. */
        case CONSTANTS_BLOCK: reserve = 32; break;
        case FUNCTION_BLOCK: reserve = 128; break;
        case METADATA_BLOCK: reserve = 32; break;
        case TYPE_BLOCK: reserve = 32; break;
        default: reserve = 8; break;
    }
    reserve = max(reserve, block->record_count + 1);
    if (!vkd3d_array_reserve((void **)&block->records, &block->record_capacity, reserve, sizeof(*block->records)))
    {
        ERR("Failed to allocate %u records.\n", reserve);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if (block->id == BLOCKINFO_BLOCK && !dxil_block_handle_blockinfo_record(block, record))
        return VKD3D_ERROR_INVALID_SHADER;

    block->records[block->record_count++] = record;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_read_unabbrev_record(struct sm6_parser *sm6)
{
    struct dxil_block *block = sm6->current_block;
    enum vkd3d_result ret = VKD3D_OK;
    unsigned int code, count, i;
    struct dxil_record *record;

    code = sm6_parser_read_vbr(sm6, 6);

    count = sm6_parser_read_vbr(sm6, 6);
    if (!(record = vkd3d_malloc(sizeof(*record) + count * sizeof(record->operands[0]))))
    {
        ERR("Failed to allocate record with %u operands.\n", count);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    record->code = code;
    record->operand_count = count;

    for (i = 0; i < count; ++i)
        record->operands[i] = sm6_parser_read_vbr(sm6, 6);
    if (sm6->p.failed)
        ret = VKD3D_ERROR_INVALID_SHADER;

    if (ret < 0 || (ret = dxil_block_add_record(block, record)) < 0)
        vkd3d_free(record);

    return ret;
}

static bool sm6_parser_read_literal_operand(struct sm6_parser *sm6, uint64_t context, uint64_t *op)
{
    *op = context;
    return !sm6->p.failed;
}

static bool sm6_parser_read_fixed_operand(struct sm6_parser *sm6, uint64_t context, uint64_t *op)
{
    *op = sm6_parser_read_bits(sm6, context);
    return !sm6->p.failed;
}

static bool sm6_parser_read_vbr_operand(struct sm6_parser *sm6, uint64_t context, uint64_t *op)
{
    *op = sm6_parser_read_vbr(sm6, context);
    return !sm6->p.failed;
}

static bool sm6_parser_read_char6_operand(struct sm6_parser *sm6, uint64_t context, uint64_t *op)
{
    *op = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._"[sm6_parser_read_bits(sm6, 6)];
    return !sm6->p.failed;
}

static bool sm6_parser_read_blob_operand(struct sm6_parser *sm6, uint64_t context, uint64_t *op)
{
    int count = sm6_parser_read_vbr(sm6, 6);
    sm6_parser_align_32(sm6);
    for (; count > 0; count -= 4)
        sm6_parser_read_uint32(sm6);
    FIXME("Unhandled blob operand.\n");
    return false;
}

static enum vkd3d_result dxil_abbrev_init(struct dxil_abbrev *abbrev, unsigned int count, struct sm6_parser *sm6)
{
    enum bitcode_abbrev_type prev_type, type;
    unsigned int i;

    abbrev->is_array = false;

    for (i = 0, prev_type = 0; i < count && !sm6->p.failed; ++i)
    {
        if (sm6_parser_read_bits(sm6, 1))
        {
            if (prev_type == ABBREV_ARRAY)
            {
                WARN("Unexpected literal abbreviation after array.\n");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            abbrev->operands[i].context = sm6_parser_read_vbr(sm6, 8);
            abbrev->operands[i].read_operand = sm6_parser_read_literal_operand;
            continue;
        }

        switch (type = sm6_parser_read_bits(sm6, 3))
        {
            case ABBREV_FIXED:
            case ABBREV_VBR:
                abbrev->operands[i].context = sm6_parser_read_vbr(sm6, 5);
                abbrev->operands[i].read_operand = (type == ABBREV_FIXED) ? sm6_parser_read_fixed_operand
                        : sm6_parser_read_vbr_operand;
                break;

            case ABBREV_ARRAY:
                if (prev_type == ABBREV_ARRAY || i != count - 2)
                {
                    WARN("Unexpected array abbreviation.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                abbrev->is_array = true;
                --i;
                --count;
                break;

            case ABBREV_CHAR:
                abbrev->operands[i].read_operand = sm6_parser_read_char6_operand;
                break;

            case ABBREV_BLOB:
                if (prev_type == ABBREV_ARRAY || i != count - 1)
                {
                    WARN("Unexpected blob abbreviation.\n");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                abbrev->operands[i].read_operand = sm6_parser_read_blob_operand;
                break;
        }

        prev_type = type;
    }

    abbrev->count = count;

    return sm6->p.failed ? VKD3D_ERROR_INVALID_SHADER : VKD3D_OK;
}

static enum vkd3d_result sm6_parser_add_global_abbrev(struct sm6_parser *sm6)
{
    struct dxil_block *block = sm6->current_block;
    unsigned int count = sm6_parser_read_vbr(sm6, 5);
    struct dxil_global_abbrev *global_abbrev;
    enum vkd3d_result ret;

    assert(block->id == BLOCKINFO_BLOCK);

    if (!vkd3d_array_reserve((void **)&sm6->abbrevs, &sm6->abbrev_capacity, sm6->abbrev_count + 1, sizeof(*sm6->abbrevs))
            || !(global_abbrev = vkd3d_malloc(sizeof(*global_abbrev) + count * sizeof(global_abbrev->abbrev.operands[0]))))
    {
        ERR("Failed to allocate global abbreviation.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if ((ret = dxil_abbrev_init(&global_abbrev->abbrev, count, sm6)) < 0)
    {
        vkd3d_free(global_abbrev);
        return ret;
    }

    if (!block->has_bid)
    {
        WARN("Missing blockinfo block id.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (block->blockinfo_bid == MODULE_BLOCK)
    {
        FIXME("Unhandled global abbreviation for module block.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    global_abbrev->block_id = block->blockinfo_bid;

    sm6->abbrevs[sm6->abbrev_count++] = global_abbrev;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_add_block_abbrev(struct sm6_parser *sm6)
{
    struct dxil_block *block = sm6->current_block;
    struct dxil_abbrev *abbrev;
    enum vkd3d_result ret;
    unsigned int count;

    if (block->id == BLOCKINFO_BLOCK)
        return sm6_parser_add_global_abbrev(sm6);

    count = sm6_parser_read_vbr(sm6, 5);
    if (!vkd3d_array_reserve((void **)&block->abbrevs, &block->abbrev_capacity, block->abbrev_count + 1, sizeof(*block->abbrevs))
            || !(abbrev = vkd3d_malloc(sizeof(*abbrev) + count * sizeof(abbrev->operands[0]))))
    {
        ERR("Failed to allocate block abbreviation.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if ((ret = dxil_abbrev_init(abbrev, count, sm6)) < 0)
    {
        vkd3d_free(abbrev);
        return ret;
    }

    block->abbrevs[block->abbrev_count++] = abbrev;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_read_abbrev_record(struct sm6_parser *sm6, unsigned int abbrev_id)
{
    enum vkd3d_result ret = VKD3D_ERROR_INVALID_SHADER;
    struct dxil_block *block = sm6->current_block;
    struct dxil_record *temp, *record;
    unsigned int i, count, array_len;
    struct dxil_abbrev *abbrev;
    uint64_t code;

    if (abbrev_id >= block->abbrev_count)
    {
        WARN("Invalid abbreviation id %u.\n", abbrev_id);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    abbrev = block->abbrevs[abbrev_id];
    if (!(count = abbrev->count))
        return VKD3D_OK;
    if (count == 1 && abbrev->is_array)
        return VKD3D_ERROR_INVALID_SHADER;

    /* First operand is the record code. The array is included in the count, but will be done separately. */
    count -= abbrev->is_array + 1;
    if (!(record = vkd3d_malloc(sizeof(*record) + count * sizeof(record->operands[0]))))
    {
        ERR("Failed to allocate record with %u operands.\n", count);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if (!abbrev->operands[0].read_operand(sm6, abbrev->operands[0].context, &code))
        goto fail;
    if (code > UINT_MAX)
        FIXME("Truncating 64-bit record code %#"PRIx64".\n", code);
    record->code = code;

    for (i = 0; i < count; ++i)
        if (!abbrev->operands[i + 1].read_operand(sm6, abbrev->operands[i + 1].context, &record->operands[i]))
            goto fail;
    record->operand_count = count;

    /* An array can occur only as the last operand. */
    if (abbrev->is_array)
    {
        array_len = sm6_parser_read_vbr(sm6, 6);
        if (!(temp = vkd3d_realloc(record, sizeof(*record) + (count + array_len) * sizeof(record->operands[0]))))
        {
            ERR("Failed to allocate record with %u operands.\n", count + array_len);
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        record = temp;

        for (i = 0; i < array_len; ++i)
        {
            if (!abbrev->operands[count + 1].read_operand(sm6, abbrev->operands[count + 1].context,
                    &record->operands[count + i]))
            {
                goto fail;
            }
        }
        record->operand_count += array_len;
    }

    if ((ret = dxil_block_add_record(block, record)) < 0)
        goto fail;

    return VKD3D_OK;

fail:
    vkd3d_free(record);
    return ret;
}

static enum vkd3d_result dxil_block_init(struct dxil_block *block, const struct dxil_block *parent,
        struct sm6_parser *sm6);

static enum vkd3d_result dxil_block_read(struct dxil_block *parent, struct sm6_parser *sm6)
{
    unsigned int reserve = (parent->id == MODULE_BLOCK) ? 12 : 2;
    struct dxil_block *block;
    enum vkd3d_result ret;

    sm6->current_block = parent;

    do
    {
        unsigned int abbrev_id = sm6_parser_read_bits(sm6, parent->abbrev_len);

        switch (abbrev_id)
        {
            case END_BLOCK:
                sm6_parser_align_32(sm6);
                return VKD3D_OK;

            case ENTER_SUBBLOCK:
                if (parent->id != MODULE_BLOCK && parent->id != FUNCTION_BLOCK)
                {
                    WARN("Invalid subblock parent id %u.\n", parent->id);
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                if (!vkd3d_array_reserve((void **)&parent->child_blocks, &parent->child_block_capacity,
                        max(reserve, parent->child_block_count + 1), sizeof(*parent->child_blocks))
                        || !(block = vkd3d_calloc(1, sizeof(*block))))
                {
                    ERR("Failed to allocate block.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }

                if ((ret = dxil_block_init(block, parent, sm6)) < 0)
                {
                    vkd3d_free(block);
                    return ret;
                }

                parent->child_blocks[parent->child_block_count++] = block;
                sm6->current_block = parent;
                break;

            case DEFINE_ABBREV:
                if ((ret = sm6_parser_add_block_abbrev(sm6)) < 0)
                    return ret;
                break;

            case UNABBREV_RECORD:
                if ((ret = sm6_parser_read_unabbrev_record(sm6)) < 0)
                {
                    WARN("Failed to read unabbreviated record.\n");
                    return ret;
                }
                break;

            default:
                if ((ret = sm6_parser_read_abbrev_record(sm6, abbrev_id - 4)) < 0)
                {
                    WARN("Failed to read abbreviated record.\n");
                    return ret;
                }
                break;
        }
    } while (!sm6->p.failed);

    return VKD3D_ERROR_INVALID_SHADER;
}

static size_t sm6_parser_compute_global_abbrev_count_for_block_id(struct sm6_parser *sm6,
        unsigned int block_id)
{
    size_t i, count;

    for (i = 0, count = 0; i < sm6->abbrev_count; ++i)
        count += sm6->abbrevs[i]->block_id == block_id;

    return count;
}

static void dxil_block_destroy(struct dxil_block *block)
{
    size_t i;

    for (i = 0; i < block->record_count; ++i)
        vkd3d_free(block->records[i]);
    vkd3d_free(block->records);

    for (i = 0; i < block->child_block_count; ++i)
    {
        dxil_block_destroy(block->child_blocks[i]);
        vkd3d_free(block->child_blocks[i]);
    }
    vkd3d_free(block->child_blocks);

    block->records = NULL;
    block->record_count = 0;
    block->child_blocks = NULL;
    block->child_block_count = 0;
}

static enum vkd3d_result dxil_block_init(struct dxil_block *block, const struct dxil_block *parent,
        struct sm6_parser *sm6)
{
    size_t i, abbrev_count = 0;
    enum vkd3d_result ret;

    block->parent = parent;
    block->level = parent ? parent->level + 1 : 0;
    block->id = sm6_parser_read_vbr(sm6, 8);
    block->abbrev_len = sm6_parser_read_vbr(sm6, 4);
    sm6_parser_align_32(sm6);
    block->length = sm6_parser_read_uint32(sm6);
    block->start = sm6->ptr - sm6->start;

    if (sm6->p.failed)
        return VKD3D_ERROR_INVALID_SHADER;

    if ((block->abbrev_count = sm6_parser_compute_global_abbrev_count_for_block_id(sm6, block->id)))
    {
        if (!vkd3d_array_reserve((void **)&block->abbrevs, &block->abbrev_capacity,
                block->abbrev_count, sizeof(*block->abbrevs)))
        {
            ERR("Failed to allocate block abbreviations.\n");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }

        for (i = 0; i < sm6->abbrev_count; ++i)
            if (sm6->abbrevs[i]->block_id == block->id)
                block->abbrevs[abbrev_count++] = &sm6->abbrevs[i]->abbrev;

        assert(abbrev_count == block->abbrev_count);
    }

    if ((ret = dxil_block_read(block, sm6)) < 0)
        dxil_block_destroy(block);

    for (i = abbrev_count; i < block->abbrev_count; ++i)
        vkd3d_free(block->abbrevs[i]);
    vkd3d_free(block->abbrevs);
    block->abbrevs = NULL;
    block->abbrev_count = 0;

    return ret;
}

static size_t dxil_block_compute_function_count(const struct dxil_block *root)
{
    size_t i, count;

    for (i = 0, count = 0; i < root->child_block_count; ++i)
        count += root->child_blocks[i]->id == FUNCTION_BLOCK;

    return count;
}

static size_t dxil_block_compute_module_decl_count(const struct dxil_block *block)
{
    size_t i, count;

    for (i = 0, count = 0; i < block->record_count; ++i)
        count += block->records[i]->code == MODULE_CODE_FUNCTION;
    return count;
}

static size_t dxil_block_compute_constants_count(const struct dxil_block *block)
{
    size_t i, count;

    for (i = 0, count = 0; i < block->record_count; ++i)
        count += block->records[i]->code != CST_CODE_SETTYPE;
    return count;
}

static void dxil_global_abbrevs_cleanup(struct dxil_global_abbrev **abbrevs, size_t count)
{
    size_t i;

    for (i = 0; i < count; ++i)
        vkd3d_free(abbrevs[i]);
    vkd3d_free(abbrevs);
}

static const struct dxil_block *sm6_parser_get_level_one_block(const struct sm6_parser *sm6,
        enum bitcode_block_id id, bool *is_unique)
{
    const struct dxil_block *block, *found = NULL;
    size_t i;

    for (i = 0, *is_unique = true; i < sm6->root_block.child_block_count; ++i)
    {
        block = sm6->root_block.child_blocks[i];
        if (block->id != id)
            continue;

        if (!found)
            found = block;
        else
            *is_unique = false;
    }

    return found;
}

static char *dxil_record_to_string(const struct dxil_record *record, unsigned int offset)
{
    unsigned int i;
    char *str;

    assert(offset <= record->operand_count);
    if (!(str = vkd3d_calloc(record->operand_count - offset + 1, 1)))
        return NULL;

    for (i = offset; i < record->operand_count; ++i)
        str[i - offset] = record->operands[i];

    return str;
}

static bool dxil_record_validate_operand_min_count(const struct dxil_record *record, unsigned int min_count,
        struct sm6_parser *sm6)
{
    if (record->operand_count >= min_count)
        return true;

    WARN("Invalid operand count %u for code %u.\n", record->operand_count, record->code);
    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
            "Invalid operand count %u for record code %u.", record->operand_count, record->code);
    return false;
}

static void dxil_record_validate_operand_max_count(const struct dxil_record *record, unsigned int max_count,
        struct sm6_parser *sm6)
{
    if (record->operand_count <= max_count)
        return;

    WARN("Ignoring %u extra operands for code %u.\n", record->operand_count - max_count, record->code);
    vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
            "Ignoring %u extra operands for record code %u.", record->operand_count - max_count, record->code);
}

static bool dxil_record_validate_operand_count(const struct dxil_record *record, unsigned int min_count,
        unsigned int max_count, struct sm6_parser *sm6)
{
    dxil_record_validate_operand_max_count(record, max_count, sm6);
    return dxil_record_validate_operand_min_count(record, min_count, sm6);
}

static enum vkd3d_result sm6_parser_type_table_init(struct sm6_parser *sm6)
{
    const struct dxil_record *record;
    size_t i, type_count, type_index;
    const struct dxil_block *block;
    char *struct_name = NULL;
    unsigned int j, count;
    struct sm6_type *type;
    uint64_t type_id;
    bool is_unique;

    sm6->p.location.line = 0;
    sm6->p.location.column = 0;

    if (!(block = sm6_parser_get_level_one_block(sm6, TYPE_BLOCK, &is_unique)))
    {
        WARN("No type definitions found.\n");
        return VKD3D_OK;
    }
    if (!is_unique)
        WARN("Ignoring invalid extra type table(s).\n");

    sm6->p.location.line = block->id;

    type_count = 0;
    for (i = 0; i < block->record_count; ++i)
        type_count += block->records[i]->code != TYPE_CODE_NUMENTRY && block->records[i]->code != TYPE_CODE_STRUCT_NAME;

    /* The type array must not be relocated. */
    if (!(sm6->types = vkd3d_calloc(type_count, sizeof(*sm6->types))))
    {
        ERR("Failed to allocate type array.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < block->record_count; ++i)
    {
        sm6->p.location.column = i;
        record = block->records[i];

        type = &sm6->types[sm6->type_count];
        type_index = sm6->type_count;

        switch (record->code)
        {
            case TYPE_CODE_ARRAY:
            case TYPE_CODE_VECTOR:
                if (!dxil_record_validate_operand_count(record, 2, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                type->class = record->code == TYPE_CODE_ARRAY ? TYPE_CLASS_ARRAY : TYPE_CLASS_VECTOR;

                if (!(type->u.array.count = record->operands[0]))
                {
                    TRACE("Setting unbounded for type %zu.\n", type_index);
                    type->u.array.count = UINT_MAX;
                }

                if ((type_id = record->operands[1]) >= type_count)
                {
                    WARN("Invalid contained type id %"PRIu64" for type %zu.\n", type_id, type_index);
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                type->u.array.elem_type = &sm6->types[type_id];
                break;

            case TYPE_CODE_DOUBLE:
                dxil_record_validate_operand_max_count(record, 0, sm6);
                type->class = TYPE_CLASS_FLOAT;
                type->u.width = 64;
                break;

            case TYPE_CODE_FLOAT:
                dxil_record_validate_operand_max_count(record, 0, sm6);
                type->class = TYPE_CLASS_FLOAT;
                type->u.width = 32;
                break;

            case TYPE_CODE_FUNCTION:
                if (!dxil_record_validate_operand_min_count(record, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;
                if (record->operands[0])
                    FIXME("Unhandled vararg function type %zu.\n", type_index);

                type->class = TYPE_CLASS_FUNCTION;

                if ((type_id = record->operands[1]) >= type_count)
                {
                    WARN("Invalid return type id %"PRIu64" for type %zu.\n", type_id, type_index);
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                count = record->operand_count - 2;
                if (vkd3d_object_range_overflow(sizeof(type->u.function), count, sizeof(type->u.function->param_types[0]))
                        || !(type->u.function = vkd3d_malloc(offsetof(struct sm6_function_info, param_types[count]))))
                {
                    ERR("Failed to allocate function parameter types.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }

                type->u.function->ret_type = &sm6->types[type_id];
                type->u.function->param_count = count;
                for (j = 0; j < count; ++j)
                {
                    if ((type_id = record->operands[j + 2]) >= type_count)
                    {
                        WARN("Invalid parameter type id %"PRIu64" for type %zu.\n", type_id, type_index);
                        vkd3d_free(type->u.function);
                        return VKD3D_ERROR_INVALID_SHADER;
                    }
                    type->u.function->param_types[j] = &sm6->types[type_id];
                }
                break;

            case TYPE_CODE_HALF:
                dxil_record_validate_operand_max_count(record, 0, sm6);
                type->class = TYPE_CLASS_FLOAT;
                type->u.width = 16;
                break;

            case TYPE_CODE_INTEGER:
            {
                uint64_t width;

                if (!dxil_record_validate_operand_count(record, 1, 1, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                type->class = TYPE_CLASS_INTEGER;

                switch ((width = record->operands[0]))
                {
                    case 1:
                    case 8:
                    case 16:
                    case 32:
                    case 64:
                        break;
                    default:
                        WARN("Invalid integer width %"PRIu64" for type %zu.\n", width, type_index);
                        return VKD3D_ERROR_INVALID_SHADER;
                }
                type->u.width = width;
                break;
            }

            case TYPE_CODE_LABEL:
                type->class = TYPE_CLASS_LABEL;
                break;

            case TYPE_CODE_METADATA:
                type->class = TYPE_CLASS_METADATA;
                break;

            case TYPE_CODE_NUMENTRY:
                continue;

            case TYPE_CODE_POINTER:
                if (!dxil_record_validate_operand_count(record, 1, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                type->class = TYPE_CLASS_POINTER;

                if ((type_id = record->operands[0]) >= type_count)
                {
                    WARN("Invalid pointee type id %"PRIu64" for type %zu.\n", type_id, type_index);
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                type->u.pointer.type = &sm6->types[type_id];
                type->u.pointer.addr_space = (record->operand_count > 1) ? record->operands[1] : ADDRESS_SPACE_DEFAULT;
                break;

            case TYPE_CODE_STRUCT_ANON:
            case TYPE_CODE_STRUCT_NAMED:
                if (!dxil_record_validate_operand_min_count(record, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;
                if (record->code == TYPE_CODE_STRUCT_NAMED && !struct_name)
                {
                    WARN("Missing struct name before struct type %zu.\n", type_index);
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                type->class = TYPE_CLASS_STRUCT;

                count = record->operand_count - 1;
                if (vkd3d_object_range_overflow(sizeof(type->u.struc), count, sizeof(type->u.struc->elem_types[0]))
                        || !(type->u.struc = vkd3d_malloc(offsetof(struct sm6_struct_info, elem_types[count]))))
                {
                    ERR("Failed to allocate struct element types.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }

                if (record->operands[0])
                    FIXME("Ignoring struct packed attribute.\n");

                type->u.struc->elem_count = count;
                for (j = 0; j < count; ++j)
                {
                    if ((type_id = record->operands[j + 1]) >= type_count)
                    {
                        WARN("Invalid contained type id %"PRIu64" for type %zu.\n", type_id, type_index);
                        vkd3d_free(type->u.struc);
                        return VKD3D_ERROR_INVALID_SHADER;
                    }
                    type->u.struc->elem_types[j] = &sm6->types[type_id];
                }

                if (record->code == TYPE_CODE_STRUCT_ANON)
                {
                    type->u.struc->name = NULL;
                    break;
                }

                type->u.struc->name = struct_name;
                struct_name = NULL;
                break;

            case TYPE_CODE_STRUCT_NAME:
                if (!(struct_name = dxil_record_to_string(record, 0)))
                {
                    ERR("Failed to allocate struct name.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }
                if (!struct_name[0])
                    WARN("Struct name is empty for type %zu.\n", type_index);
                continue;

            case TYPE_CODE_VOID:
                dxil_record_validate_operand_max_count(record, 0, sm6);
                type->class = TYPE_CLASS_VOID;
                break;

            default:
                FIXME("Unhandled type %u at index %zu.\n", record->code, type_index);
                return VKD3D_ERROR_INVALID_SHADER;
        }
        ++sm6->type_count;
    }

    assert(sm6->type_count == type_count);

    if (struct_name)
    {
        WARN("Unused struct name %s.\n", struct_name);
        vkd3d_free(struct_name);
    }

    return VKD3D_OK;
}

static inline bool sm6_type_is_void(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_VOID;
}

static inline bool sm6_type_is_integer(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER;
}

static inline bool sm6_type_is_i8(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER && type->u.width == 8;
}

static inline bool sm6_type_is_i32(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER && type->u.width == 32;
}

static inline bool sm6_type_is_floating_point(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_FLOAT;
}

static inline bool sm6_type_is_numeric(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER || type->class == TYPE_CLASS_FLOAT;
}

static inline bool sm6_type_is_pointer(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_POINTER;
}

static bool sm6_type_is_numeric_aggregate(const struct sm6_type *type)
{
    unsigned int i;

    switch (type->class)
    {
        case TYPE_CLASS_ARRAY:
        case TYPE_CLASS_VECTOR:
            return sm6_type_is_numeric(type->u.array.elem_type);

        case TYPE_CLASS_STRUCT:
            /* Do not handle nested structs. Support can be added if they show up. */
            for (i = 0; i < type->u.struc->elem_count; ++i)
                if (!sm6_type_is_numeric(type->u.struc->elem_types[i]))
                    return false;
            return true;

        default:
            return false;
    }
}

static inline bool sm6_type_is_struct(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_STRUCT;
}

static inline bool sm6_type_is_function(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_FUNCTION;
}

static inline bool sm6_type_is_function_pointer(const struct sm6_type *type)
{
    return sm6_type_is_pointer(type) && sm6_type_is_function(type->u.pointer.type);
}

static inline bool sm6_type_is_handle(const struct sm6_type *type)
{
    return sm6_type_is_struct(type) && !strcmp(type->u.struc->name, "dx.types.Handle");
}

static inline const struct sm6_type *sm6_type_get_element_type(const struct sm6_type *type)
{
    return (type->class == TYPE_CLASS_ARRAY || type->class == TYPE_CLASS_VECTOR) ? type->u.array.elem_type : type;
}

static const struct sm6_type *sm6_type_get_pointer_to_type(const struct sm6_type *type,
        enum bitcode_address_space addr_space, struct sm6_parser *sm6)
{
    size_t i, start = type - sm6->types;
    const struct sm6_type *pointer_type;

    /* DXC seems usually to place the pointer type immediately after its pointee. */
    for (i = (start + 1) % sm6->type_count; i != start; i = (i + 1) % sm6->type_count)
    {
        pointer_type = &sm6->types[i];
        if (sm6_type_is_pointer(pointer_type) && pointer_type->u.pointer.type == type
                && pointer_type->u.pointer.addr_space == addr_space)
            return pointer_type;
    }

    return NULL;
}

/* Never returns null for elem_idx 0. */
static const struct sm6_type *sm6_type_get_scalar_type(const struct sm6_type *type, unsigned int elem_idx)
{
    switch (type->class)
    {
        case TYPE_CLASS_ARRAY:
        case TYPE_CLASS_VECTOR:
            if (elem_idx >= type->u.array.count)
                return NULL;
            return sm6_type_get_scalar_type(type->u.array.elem_type, 0);

        case TYPE_CLASS_POINTER:
            return sm6_type_get_scalar_type(type->u.pointer.type, 0);

        case TYPE_CLASS_STRUCT:
            if (elem_idx >= type->u.struc->elem_count)
                return NULL;
            return sm6_type_get_scalar_type(type->u.struc->elem_types[elem_idx], 0);

        default:
            return type;
    }
}

static const struct sm6_type *sm6_parser_get_type(struct sm6_parser *sm6, uint64_t type_id)
{
    if (type_id >= sm6->type_count)
    {
        WARN("Invalid type index %"PRIu64" at %zu.\n", type_id, sm6->value_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_TYPE_ID,
                "DXIL type id %"PRIu64" is invalid.", type_id);
        return NULL;
    }
    return &sm6->types[type_id];
}

static int global_symbol_compare(const void *a, const void *b)
{
    return vkd3d_u32_compare(((const struct sm6_symbol *)a)->id, ((const struct sm6_symbol *)b)->id);
}

static enum vkd3d_result sm6_parser_symtab_init(struct sm6_parser *sm6)
{
    const struct dxil_record *record;
    const struct dxil_block *block;
    struct sm6_symbol *symbol;
    size_t i, count;
    bool is_unique;

    sm6->p.location.line = 0;
    sm6->p.location.column = 0;

    if (!(block = sm6_parser_get_level_one_block(sm6, VALUE_SYMTAB_BLOCK, &is_unique)))
    {
        /* There should always be at least one symbol: the name of the entry point function. */
        WARN("No value symtab block found.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (!is_unique)
        FIXME("Ignoring extra value symtab block(s).\n");

    sm6->p.location.line = block->id;

    for (i = 0, count = 0; i < block->record_count; ++i)
        count += block->records[i]->code == VST_CODE_ENTRY;

    if (!(sm6->global_symbols = vkd3d_calloc(count, sizeof(*sm6->global_symbols))))
    {
        ERR("Failed to allocate global symbols.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < block->record_count; ++i)
    {
        sm6->p.location.column = i;
        record = block->records[i];

        if (record->code != VST_CODE_ENTRY)
        {
            FIXME("Unhandled symtab code %u.\n", record->code);
            continue;
        }
        if (!dxil_record_validate_operand_min_count(record, 1, sm6))
            continue;

        symbol = &sm6->global_symbols[sm6->global_symbol_count];
        symbol->id = record->operands[0];
        if (!(symbol->name = dxil_record_to_string(record, 1)))
        {
            ERR("Failed to allocate symbol name.\n");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        ++sm6->global_symbol_count;
    }

    sm6->p.location.column = block->record_count;

    qsort(sm6->global_symbols, sm6->global_symbol_count, sizeof(*sm6->global_symbols), global_symbol_compare);
    for (i = 1; i < sm6->global_symbol_count; ++i)
    {
        if (sm6->global_symbols[i].id == sm6->global_symbols[i - 1].id)
        {
            WARN("Invalid duplicate symbol id %u.\n", sm6->global_symbols[i].id);
            return VKD3D_ERROR_INVALID_SHADER;
        }
    }

    return VKD3D_OK;
}

static const char *sm6_parser_get_global_symbol_name(const struct sm6_parser *sm6, size_t id)
{
    size_t i, start;

    /* id == array index is normally true */
    i = start = id % sm6->global_symbol_count;
    do
    {
        if (sm6->global_symbols[i].id == id)
            return sm6->global_symbols[i].name;
        i = (i + 1) % sm6->global_symbol_count;
    } while (i != start);

    return NULL;
}

static unsigned int register_get_uint_value(const struct vkd3d_shader_register *reg)
{
    if (!register_is_constant(reg) || !data_type_is_integer(reg->data_type))
        return UINT_MAX;

    if (reg->immconst_type == VKD3D_IMMCONST_VEC4)
        WARN("Returning vec4.x.\n");

    if (reg->type == VKD3DSPR_IMMCONST64)
    {
        if (reg->u.immconst_uint64[0] > UINT_MAX)
            FIXME("Truncating 64-bit value.\n");
        return reg->u.immconst_uint64[0];
    }

    return reg->u.immconst_uint[0];
}

static inline bool sm6_value_is_function_dcl(const struct sm6_value *value)
{
    return value->value_type == VALUE_TYPE_FUNCTION;
}

static inline bool sm6_value_is_dx_intrinsic_dcl(const struct sm6_value *fn)
{
    assert(sm6_value_is_function_dcl(fn));
    return fn->u.function.is_prototype && !strncmp(fn->u.function.name, "dx.op.", 6);
}

static inline struct sm6_value *sm6_parser_get_current_value(const struct sm6_parser *sm6)
{
    assert(sm6->value_count < sm6->value_capacity);
    return &sm6->values[sm6->value_count];
}

static inline bool sm6_value_is_register(const struct sm6_value *value)
{
    return value->value_type == VALUE_TYPE_REG;
}

static inline bool sm6_value_is_constant(const struct sm6_value *value)
{
    return sm6_value_is_register(value) && register_is_constant(&value->u.reg);
}

static inline bool sm6_value_is_undef(const struct sm6_value *value)
{
    return sm6_value_is_register(value) && value->u.reg.type == VKD3DSPR_UNDEF;
}

static inline unsigned int sm6_value_get_constant_uint(const struct sm6_value *value)
{
    if (!sm6_value_is_constant(value))
        return UINT_MAX;
    return register_get_uint_value(&value->u.reg);
}

static struct vkd3d_shader_src_param *instruction_src_params_alloc(struct vkd3d_shader_instruction *ins,
        unsigned int count, struct sm6_parser *sm6)
{
    struct vkd3d_shader_src_param *params = shader_parser_get_src_params(&sm6->p, count);
    if (!params)
    {
        ERR("Failed to allocate src params.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating instruction src paramaters.");
        return NULL;
    }
    ins->src = params;
    ins->src_count = count;
    return params;
}

static struct vkd3d_shader_dst_param *instruction_dst_params_alloc(struct vkd3d_shader_instruction *ins,
        unsigned int count, struct sm6_parser *sm6)
{
    struct vkd3d_shader_dst_param *params = shader_parser_get_dst_params(&sm6->p, count);
    if (!params)
    {
        ERR("Failed to allocate dst params.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating instruction dst paramaters.");
        return NULL;
    }
    ins->dst = params;
    ins->dst_count = count;
    return params;
}

static enum vkd3d_data_type vkd3d_data_type_from_sm6_type(const struct sm6_type *type)
{
    if (type->class == TYPE_CLASS_INTEGER)
    {
        switch (type->u.width)
        {
            case 8:
                return VKD3D_DATA_UINT8;
            case 32:
                return VKD3D_DATA_UINT;
            default:
                FIXME("Unhandled width %u.\n", type->u.width);
                return VKD3D_DATA_UINT;
        }
    }
    else if (type->class == TYPE_CLASS_FLOAT)
    {
        switch (type->u.width)
        {
            case 32:
                return VKD3D_DATA_FLOAT;
            case 64:
                return VKD3D_DATA_DOUBLE;
            default:
                FIXME("Unhandled width %u.\n", type->u.width);
                return VKD3D_DATA_FLOAT;
        }
    }

    FIXME("Unhandled type %u.\n", type->class);
    return VKD3D_DATA_UINT;
}

static inline void dst_param_init_scalar(struct vkd3d_shader_dst_param *param, unsigned int component_idx)
{
    param->write_mask = 1u << component_idx;
    param->modifiers = 0;
    param->shift = 0;
}

static inline void src_param_init(struct vkd3d_shader_src_param *param)
{
    param->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    param->modifiers = VKD3DSPSM_NONE;
}

static void src_param_init_from_value(struct vkd3d_shader_src_param *param, const struct sm6_value *src)
{
    src_param_init(param);
    param->reg = src->u.reg;
}

static void register_address_init(struct vkd3d_shader_register *reg, const struct sm6_value *address,
        unsigned int idx, struct sm6_parser *sm6)
{
    assert(idx < ARRAY_SIZE(reg->idx));
    if (sm6_value_is_constant(address))
    {
        reg->idx[idx].offset = sm6_value_get_constant_uint(address);
    }
    else if (sm6_value_is_undef(address))
    {
        reg->idx[idx].offset = 0;
    }
    else
    {
        struct vkd3d_shader_src_param *rel_addr = shader_parser_get_src_params(&sm6->p, 1);
        if (rel_addr)
            src_param_init_from_value(rel_addr, address);
        reg->idx[idx].offset = 0;
        reg->idx[idx].rel_addr = rel_addr;
    }
}

/* Recurse through the block tree while maintaining a current value count. The current
 * count is the sum of the global count plus all declarations within the current function.
 * Store into value_capacity the highest count seen. */
static size_t sm6_parser_compute_max_value_count(struct sm6_parser *sm6,
        const struct dxil_block *block, size_t value_count)
{
    size_t i, old_value_count = value_count;

    if (block->id == MODULE_BLOCK)
        value_count = size_add_with_overflow_check(value_count, dxil_block_compute_module_decl_count(block));

    for (i = 0; i < block->child_block_count; ++i)
        value_count = sm6_parser_compute_max_value_count(sm6, block->child_blocks[i], value_count);

    switch (block->id)
    {
        case CONSTANTS_BLOCK:
            /* Function local constants are contained in a child block of the function block. */
            value_count = size_add_with_overflow_check(value_count, dxil_block_compute_constants_count(block));
            break;
        case FUNCTION_BLOCK:
            /* A function must start with a block count, which emits no value. This formula is likely to
             * overestimate the value count somewhat, but this should be no problem. */
            value_count = size_add_with_overflow_check(value_count, max(block->record_count, 1u) - 1);
            sm6->value_capacity = max(sm6->value_capacity, value_count);
            sm6->functions[sm6->function_count].value_count = value_count;
            /* The value count returns to its previous value after handling a function. */
            if (value_count < SIZE_MAX)
                value_count = old_value_count;
            break;
        default:
            break;
    }

    return value_count;
}

static size_t sm6_parser_get_value_index(struct sm6_parser *sm6, uint64_t idx)
{
    size_t i;

    /* The value relative index is 32 bits. */
    if (idx > UINT32_MAX)
        WARN("Ignoring upper 32 bits of relative index.\n");
    i = (uint32_t)sm6->value_count - (uint32_t)idx;

    /* This may underflow to produce a forward reference, but it must not exceeed the final value count. */
    if (i >= sm6->cur_max_value)
    {
        WARN("Invalid value index %"PRIx64" at %zu.\n", idx, sm6->value_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Invalid value relative index %u.", (unsigned int)idx);
        return SIZE_MAX;
    }
    if (i == sm6->value_count)
    {
        WARN("Invalid value self-reference at %zu.\n", sm6->value_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND, "Invalid value self-reference.");
        return SIZE_MAX;
    }

    return i;
}

static size_t sm6_parser_get_value_idx_by_ref(struct sm6_parser *sm6, const struct dxil_record *record,
        const struct sm6_type *fwd_type, unsigned int *rec_idx)
{
    unsigned int idx;
    uint64_t val_ref;
    size_t operand;

    idx = *rec_idx;
    if (!dxil_record_validate_operand_min_count(record, idx + 1, sm6))
        return SIZE_MAX;
    val_ref = record->operands[idx++];

    operand = sm6_parser_get_value_index(sm6, val_ref);
    if (operand == SIZE_MAX)
        return SIZE_MAX;

    if (operand >= sm6->value_count)
    {
        if (!fwd_type)
        {
            /* Forward references are followed by a type id unless an earlier operand set the type,
             * or it is contained in a function declaration. */
            if (!dxil_record_validate_operand_min_count(record, idx + 1, sm6))
                return SIZE_MAX;
            if (!(fwd_type = sm6_parser_get_type(sm6, record->operands[idx++])))
                return SIZE_MAX;
        }
        FIXME("Forward value references are not supported yet.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Unsupported value forward reference.");
        return SIZE_MAX;
    }
    *rec_idx = idx;

    return operand;
}

static const struct sm6_value *sm6_parser_get_value_by_ref(struct sm6_parser *sm6,
        const struct dxil_record *record, const struct sm6_type *type, unsigned int *rec_idx)
{
    size_t operand = sm6_parser_get_value_idx_by_ref(sm6, record, type, rec_idx);
    return operand == SIZE_MAX ? NULL : &sm6->values[operand];
}

static bool sm6_parser_declare_function(struct sm6_parser *sm6, const struct dxil_record *record)
{
    const unsigned int max_count = 15;
    const struct sm6_type *ret_type;
    struct sm6_value *fn;
    unsigned int i, j;

    if (!dxil_record_validate_operand_count(record, 8, max_count, sm6))
        return false;

    fn = sm6_parser_get_current_value(sm6);
    fn->value_type = VALUE_TYPE_FUNCTION;
    if (!(fn->u.function.name = sm6_parser_get_global_symbol_name(sm6, sm6->value_count)))
    {
        WARN("Missing symbol name for function %zu.\n", sm6->value_count);
        fn->u.function.name = "";
    }

    if (!(fn->type = sm6_parser_get_type(sm6, record->operands[0])))
        return false;
    if (!sm6_type_is_function(fn->type))
    {
        WARN("Type is not a function.\n");
        return false;
    }
    ret_type = fn->type->u.function->ret_type;

    if (!(fn->type = sm6_type_get_pointer_to_type(fn->type, ADDRESS_SPACE_DEFAULT, sm6)))
    {
        WARN("Failed to get pointer type for type %u.\n", fn->type->class);
        return false;
    }

    if (record->operands[1])
        WARN("Ignoring calling convention %#"PRIx64".\n", record->operands[1]);

    fn->u.function.is_prototype = !!record->operands[2];

    if (record->operands[3])
        WARN("Ignoring linkage %#"PRIx64".\n", record->operands[3]);

    if (record->operands[4] > UINT_MAX)
        WARN("Invalid attributes id %#"PRIx64".\n", record->operands[4]);
    /* 1-based index. */
    if ((fn->u.function.attribs_id = record->operands[4]))
        TRACE("Ignoring function attributes.\n");

    /* These always seem to be zero. */
    for (i = 5, j = 0; i < min(record->operand_count, max_count); ++i)
        j += !!record->operands[i];
    if (j)
        WARN("Ignoring %u operands.\n", j);

    if (sm6_value_is_dx_intrinsic_dcl(fn) && !sm6_type_is_void(ret_type) && !sm6_type_is_numeric(ret_type)
            && !sm6_type_is_numeric_aggregate(ret_type) && !sm6_type_is_handle(ret_type))
    {
        WARN("Unexpected return type for dx intrinsic function '%s'.\n", fn->u.function.name);
    }

    ++sm6->value_count;

    return true;
}

static inline uint64_t decode_rotated_signed_value(uint64_t value)
{
    if (value != 1)
    {
        bool neg = value & 1;
        value >>= 1;
        return neg ? -value : value;
    }
    return value << 63;
}

static inline float bitcast_uint64_to_float(uint64_t value)
{
    union
    {
        uint32_t uint32_value;
        float float_value;
    } u;

    u.uint32_value = value;
    return u.float_value;
}

static inline double bitcast_uint64_to_double(uint64_t value)
{
    union
    {
        uint64_t uint64_value;
        double double_value;
    } u;

    u.uint64_value = value;
    return u.double_value;
}

static enum vkd3d_result sm6_parser_constants_init(struct sm6_parser *sm6, const struct dxil_block *block)
{
    enum vkd3d_shader_register_type reg_type = VKD3DSPR_INVALID;
    const struct sm6_type *type, *elem_type;
    enum vkd3d_data_type reg_data_type;
    const struct dxil_record *record;
    struct sm6_value *dst;
    size_t i, value_idx;
    uint64_t value;

    for (i = 0, type = NULL; i < block->record_count; ++i)
    {
        sm6->p.location.column = i;
        record = block->records[i];
        value_idx = sm6->value_count;

        if (record->code == CST_CODE_SETTYPE)
        {
            if (!dxil_record_validate_operand_count(record, 1, 1, sm6))
                return VKD3D_ERROR_INVALID_SHADER;

            if (!(type = sm6_parser_get_type(sm6, record->operands[0])))
                return VKD3D_ERROR_INVALID_SHADER;

            elem_type = sm6_type_get_element_type(type);
            if (sm6_type_is_numeric(elem_type))
            {
                reg_data_type = vkd3d_data_type_from_sm6_type(elem_type);
                reg_type = elem_type->u.width > 32 ? VKD3DSPR_IMMCONST64 : VKD3DSPR_IMMCONST;
            }
            else
            {
                reg_data_type = VKD3D_DATA_UNUSED;
                reg_type = VKD3DSPR_INVALID;
            }

            if (i == block->record_count - 1)
                WARN("Unused SETTYPE record.\n");

            continue;
        }

        if (!type)
        {
            WARN("Constant record %zu has no type.\n", value_idx);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        dst = sm6_parser_get_current_value(sm6);
        dst->type = type;
        dst->value_type = VALUE_TYPE_REG;
        dst->u.reg.type = reg_type;
        dst->u.reg.immconst_type = VKD3D_IMMCONST_SCALAR;
        dst->u.reg.data_type = reg_data_type;

        switch (record->code)
        {
            case CST_CODE_NULL:
                /* Register constant data is already zero-filled. */
                break;

            case CST_CODE_INTEGER:
                if (!dxil_record_validate_operand_count(record, 1, 1, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                if (!sm6_type_is_integer(type))
                {
                    WARN("Invalid integer of non-integer type %u at constant idx %zu.\n", type->class, value_idx);
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                value = decode_rotated_signed_value(record->operands[0]);
                if (type->u.width <= 32)
                    dst->u.reg.u.immconst_uint[0] = value & ((1ull << type->u.width) - 1);
                else
                    dst->u.reg.u.immconst_uint64[0] = value;

                break;

            case CST_CODE_FLOAT:
                if (!dxil_record_validate_operand_count(record, 1, 1, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                if (!sm6_type_is_floating_point(type))
                {
                    WARN("Invalid float of non-fp type %u at constant idx %zu.\n", type->class, value_idx);
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                if (type->u.width == 16)
                    FIXME("Half float type is not supported yet.\n");
                else if (type->u.width == 32)
                    dst->u.reg.u.immconst_float[0] = bitcast_uint64_to_float(record->operands[0]);
                else if (type->u.width == 64)
                    dst->u.reg.u.immconst_double[0] = bitcast_uint64_to_double(record->operands[0]);
                else
                    vkd3d_unreachable();

                break;

            case CST_CODE_DATA:
                WARN("Unhandled constant array.\n");
                break;

            case CST_CODE_UNDEF:
                dxil_record_validate_operand_max_count(record, 0, sm6);
                dst->u.reg.type = VKD3DSPR_UNDEF;
                /* Mark as explicitly undefined, not the result of a missing constant code or instruction. */
                dst->is_undefined = true;
                break;

            default:
                FIXME("Unhandled constant code %u.\n", record->code);
                dst->u.reg.type = VKD3DSPR_UNDEF;
                break;
        }

        ++sm6->value_count;
    }

    return VKD3D_OK;
}

static struct vkd3d_shader_instruction *sm6_parser_require_space(struct sm6_parser *sm6, size_t extra)
{
    if (!shader_instruction_array_reserve(&sm6->p.instructions, sm6->p.instructions.count + extra))
    {
        ERR("Failed to allocate instruction.\n");
        return NULL;
    }
    return &sm6->p.instructions.elements[sm6->p.instructions.count];
}

/* Space should be reserved before calling this. It is intended to require no checking of the returned pointer. */
static struct vkd3d_shader_instruction *sm6_parser_add_instruction(struct sm6_parser *sm6,
        enum vkd3d_shader_opcode handler_idx)
{
    struct vkd3d_shader_instruction *ins = sm6_parser_require_space(sm6, 1);
    assert(ins);
    shader_instruction_init(ins, handler_idx);
    ++sm6->p.instructions.count;
    return ins;
}

static enum vkd3d_result sm6_parser_globals_init(struct sm6_parser *sm6)
{
    const struct dxil_block *block = &sm6->root_block;
    const struct dxil_record *record;
    uint64_t version;
    size_t i;

    sm6->p.location.line = block->id;
    sm6->p.location.column = 0;

    for (i = 0; i < block->record_count; ++i)
    {
        sm6->p.location.column = i;
        record = block->records[i];
        switch (record->code)
        {
            case MODULE_CODE_FUNCTION:
                if (!sm6_parser_declare_function(sm6, record))
                {
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_FUNCTION_DCL,
                            "A DXIL function declaration is invalid.");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                break;

            case MODULE_CODE_GLOBALVAR:
                FIXME("Global variables are not implemented yet.\n");
                break;

            case MODULE_CODE_VERSION:
                if (!dxil_record_validate_operand_count(record, 1, 1, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;
                if ((version = record->operands[0]) != 1)
                {
                    FIXME("Unsupported format version %#"PRIx64".\n", version);
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_UNSUPPORTED_BITCODE_FORMAT,
                            "Bitcode format version %#"PRIx64" is unsupported.", version);
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                break;

            default:
                break;
        }
    }

    return VKD3D_OK;
}

static void dst_param_io_init(struct vkd3d_shader_dst_param *param,
        const struct signature_element *e, enum vkd3d_shader_register_type reg_type)
{
    enum vkd3d_shader_component_type component_type;

    param->write_mask = e->mask;
    param->modifiers = 0;
    param->shift = 0;
    /* DXIL types do not have signedness. Load signed elements as unsigned. */
    component_type = e->component_type == VKD3D_SHADER_COMPONENT_INT ? VKD3D_SHADER_COMPONENT_UINT : e->component_type;
    shader_register_init(&param->reg, reg_type, vkd3d_data_type_from_component_type(component_type), 0);
}

static void sm6_parser_init_signature(struct sm6_parser *sm6, const struct shader_signature *s,
        enum vkd3d_shader_register_type reg_type, struct vkd3d_shader_dst_param *params)
{
    struct vkd3d_shader_dst_param *param;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < s->element_count; ++i)
    {
        e = &s->elements[i];

        param = &params[i];
        dst_param_io_init(param, e, reg_type);
        param->reg.idx[0].offset = i;
        param->reg.idx_count = 1;
    }
}

static void sm6_parser_emit_signature(struct sm6_parser *sm6, const struct shader_signature *s,
        enum vkd3d_shader_opcode handler_idx, enum vkd3d_shader_opcode siv_handler_idx,
        struct vkd3d_shader_dst_param *params)
{
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_dst_param *param;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < s->element_count; ++i)
    {
        e = &s->elements[i];

        /* Do not check e->used_mask because in some cases it is zero for used elements.
         * TODO: scan ahead for used I/O elements. */

        if (e->sysval_semantic != VKD3D_SHADER_SV_NONE && e->sysval_semantic != VKD3D_SHADER_SV_TARGET)
        {
            ins = sm6_parser_add_instruction(sm6, siv_handler_idx);
            param = &ins->declaration.register_semantic.reg;
            ins->declaration.register_semantic.sysval_semantic = vkd3d_siv_from_sysval(e->sysval_semantic);
        }
        else
        {
            ins = sm6_parser_add_instruction(sm6, handler_idx);
            param = &ins->declaration.dst;
        }

        *param = params[i];
    }
}

static void sm6_parser_init_output_signature(struct sm6_parser *sm6, const struct shader_signature *output_signature)
{
    sm6_parser_init_signature(sm6, output_signature,
            (sm6->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL) ? VKD3DSPR_COLOROUT : VKD3DSPR_OUTPUT,
            sm6->output_params);
}

static void sm6_parser_emit_output_signature(struct sm6_parser *sm6, const struct shader_signature *output_signature)
{
    sm6_parser_emit_signature(sm6, output_signature, VKD3DSIH_DCL_OUTPUT, VKD3DSIH_DCL_OUTPUT_SIV, sm6->output_params);
}

static const struct sm6_value *sm6_parser_next_function_definition(struct sm6_parser *sm6)
{
    size_t i, count = sm6->function_count;

    for (i = 0; i < sm6->value_count; ++i)
    {
        if (sm6_type_is_function_pointer(sm6->values[i].type) && !sm6->values[i].u.function.is_prototype && !count--)
            break;
    }
    if (i == sm6->value_count)
        return NULL;

    ++sm6->function_count;
    return &sm6->values[i];
}

static struct sm6_block *sm6_block_create()
{
    struct sm6_block *block = vkd3d_calloc(1, sizeof(*block));
    return block;
}

static void sm6_parser_emit_dx_store_output(struct sm6_parser *sm6, struct sm6_block *code_block,
        enum dx_intrinsic_opcode op, const struct sm6_value **operands, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_dst_param *dst_param;
    const struct shader_signature *signature;
    unsigned int row_index, column_index;
    const struct signature_element *e;
    const struct sm6_value *value;

    row_index = sm6_value_get_constant_uint(operands[0]);
    column_index = sm6_value_get_constant_uint(operands[2]);

    signature = &sm6->p.shader_desc.output_signature;
    if (row_index >= signature->element_count)
    {
        WARN("Invalid row index %u.\n", row_index);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Invalid output row index %u.", row_index);
        return;
    }
    e = &signature->elements[row_index];

    if (column_index >= VKD3D_VEC4_SIZE)
    {
        WARN("Invalid column index %u.\n", column_index);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Invalid output column index %u.", column_index);
        return;
    }

    value = operands[3];
    if (!sm6_value_is_register(value))
    {
        WARN("Source value is not a register.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Expected store operation source to be a register.");
        return;
    }

    shader_instruction_init(ins, VKD3DSIH_MOV);

    if (!(dst_param = instruction_dst_params_alloc(ins, 1, sm6)))
        return;
    dst_param_init_scalar(dst_param, column_index);
    dst_param->reg = sm6->output_params[row_index].reg;
    if (e->register_count > 1)
        register_address_init(&dst_param->reg, operands[1], 0, sm6);

    if ((src_param = instruction_src_params_alloc(ins, 1, sm6)))
        src_param_init_from_value(src_param, value);
}

struct sm6_dx_opcode_info
{
    const char ret_type;
    const char *operand_info;
    void (*handler)(struct sm6_parser *, struct sm6_block *, enum dx_intrinsic_opcode,
            const struct sm6_value **, struct vkd3d_shader_instruction *);
};

/*
    8 -> int8
    i -> int32
    v -> void
    o -> overloaded
 */
static const struct sm6_dx_opcode_info sm6_dx_op_table[] =
{
    [DX_STORE_OUTPUT                  ] = {'v', "ii8o", sm6_parser_emit_dx_store_output},
};

static bool sm6_parser_validate_operand_type(struct sm6_parser *sm6, const struct sm6_type *type, char info_type)
{
    switch (info_type)
    {
        case 0:
            FIXME("Invalid operand count.\n");
            return false;
        case '8':
            return sm6_type_is_i8(type);
        case 'i':
            return sm6_type_is_i32(type);
        case 'v':
            return !type;
        case 'o':
            /* TODO: some type checking may be possible */
            return true;
        default:
            FIXME("Unhandled operand code '%c'.\n", info_type);
            return false;
    }
}

static bool sm6_parser_validate_dx_op(struct sm6_parser *sm6, enum dx_intrinsic_opcode op, const char *name,
        const struct sm6_value **operands, unsigned int operand_count, struct sm6_value *dst)
{
    const struct sm6_dx_opcode_info *info;
    unsigned int i;

    info = &sm6_dx_op_table[op];

    if (!sm6_parser_validate_operand_type(sm6, dst->type, info->ret_type))
    {
        WARN("Failed to validate return type for dx intrinsic id %u, '%s'.\n", op, name);
        /* Return type validation failure is not so critical. We only need to set
         * a data type for the SSA result. */
    }

    for (i = 0; i < operand_count; ++i)
    {
        const struct sm6_value *value = operands[i];
        if (!sm6_value_is_register(value) || !sm6_parser_validate_operand_type(sm6, value->type, info->operand_info[i]))
        {
            WARN("Failed to validate operand %u for dx intrinsic id %u, '%s'.\n", i + 1, op, name);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Operand %u for call to dx intrinsic function '%s' is invalid.", i + 1, name);
            return false;
        }
    }
    if (info->operand_info[operand_count])
    {
        WARN("Missing operands for dx intrinsic id %u, '%s'.\n", op, name);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
                "Call to dx intrinsic function '%s' has missing operands.", name);
        return false;
    }

    return true;
}

static void sm6_parser_emit_unhandled(struct sm6_parser *sm6, struct vkd3d_shader_instruction *ins,
        struct sm6_value *dst)
{
    const struct sm6_type *type;

    ins->handler_idx = VKD3DSIH_NOP;

    if (!dst->type)
        return;

    type = sm6_type_get_scalar_type(dst->type, 0);
    shader_register_init(&dst->u.reg, VKD3DSPR_UNDEF, vkd3d_data_type_from_sm6_type(type), 0);
    /* dst->is_undefined is not set here because it flags only explicitly undefined values. */
}

static void sm6_parser_decode_dx_op(struct sm6_parser *sm6, struct sm6_block *code_block, enum dx_intrinsic_opcode op,
        const char *name, const struct sm6_value **operands, unsigned int operand_count,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    if (op >= ARRAY_SIZE(sm6_dx_op_table) || !sm6_dx_op_table[op].operand_info)
    {
        FIXME("Unhandled dx intrinsic function id %u, '%s'.\n", op, name);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_UNHANDLED_INTRINSIC,
                "Call to intrinsic function %s is unhandled.", name);
        sm6_parser_emit_unhandled(sm6, ins, dst);
        return;
    }

    if (sm6_parser_validate_dx_op(sm6, op, name, operands, operand_count, dst))
        sm6_dx_op_table[op].handler(sm6, code_block, op, operands, ins);
    else
        sm6_parser_emit_unhandled(sm6, ins, dst);
}

static void sm6_parser_emit_call(struct sm6_parser *sm6, const struct dxil_record *record,
        struct sm6_block *code_block, struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    const struct sm6_value *operands[DXIL_OP_MAX_OPERANDS];
    const struct sm6_value *fn_value, *op_value;
    unsigned int i = 1, j, operand_count;
    const struct sm6_type *type = NULL;
    uint64_t call_conv;

    if (!dxil_record_validate_operand_min_count(record, 2, sm6))
        return;

    /* TODO: load the 1-based attributes index from record->operands[0] and validate against attribute count. */

    if ((call_conv = record->operands[i++]) & CALL_CONV_FLAG_EXPLICIT_TYPE)
        type = sm6_parser_get_type(sm6, record->operands[i++]);
    if (call_conv &= ~CALL_CONV_FLAG_EXPLICIT_TYPE)
        WARN("Ignoring calling convention %#"PRIx64".\n", call_conv);

    if (!(fn_value = sm6_parser_get_value_by_ref(sm6, record, NULL, &i)))
        return;
    if (!sm6_value_is_function_dcl(fn_value))
    {
        WARN("Function target value is not a function declaration.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Function call target value is not a function declaration.");
        return;
    }

    if (type && type != fn_value->type->u.pointer.type)
        WARN("Explicit call type does not match function type.\n");
    type = fn_value->type->u.pointer.type;

    if (!sm6_type_is_void(type->u.function->ret_type))
        dst->type = type->u.function->ret_type;

    operand_count = type->u.function->param_count;
    if (operand_count > ARRAY_SIZE(operands))
    {
        WARN("Ignoring %zu operands.\n", operand_count - ARRAY_SIZE(operands));
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Ignoring %zu operands for function call.", operand_count - ARRAY_SIZE(operands));
        operand_count = ARRAY_SIZE(operands);
    }

    for (j = 0; j < operand_count; ++j)
    {
        if (!(operands[j] = sm6_parser_get_value_by_ref(sm6, record, type->u.function->param_types[j], &i)))
            return;
    }
    if ((j = record->operand_count - i))
    {
        WARN("Ignoring %u operands beyond the function parameter list.\n", j);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Ignoring %u function call operands beyond the parameter list.", j);
    }

    if (!fn_value->u.function.is_prototype)
    {
        FIXME("Unhandled call to local function.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Call to a local function is unsupported.");
        return;
    }
    if (!sm6_value_is_dx_intrinsic_dcl(fn_value))
        WARN("External function is not a dx intrinsic.\n");

    if (!operand_count)
    {
        WARN("Missing dx intrinsic function id.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
                "The id for a dx intrinsic function is missing.");
        return;
    }

    op_value = operands[0];
    if (!sm6_value_is_constant(op_value) || !sm6_type_is_integer(op_value->type))
    {
        WARN("dx intrinsic function id is not a constant int.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Expected a constant integer dx intrinsic function id.");
        return;
    }
    sm6_parser_decode_dx_op(sm6, code_block, register_get_uint_value(&op_value->u.reg),
            fn_value->u.function.name, &operands[1], operand_count - 1, ins, dst);
}

static void sm6_parser_emit_ret(struct sm6_parser *sm6, const struct dxil_record *record,
        struct sm6_block *code_block, struct vkd3d_shader_instruction *ins)
{
    if (!dxil_record_validate_operand_count(record, 0, 1, sm6))
        return;

    if (record->operand_count)
        FIXME("Non-void return is not implemented.\n");

    ins->handler_idx = VKD3DSIH_NOP;
}

static enum vkd3d_result sm6_parser_function_init(struct sm6_parser *sm6, const struct dxil_block *block,
        struct sm6_function *function)
{
    struct vkd3d_shader_instruction *ins;
    const struct dxil_record *record;
    bool ret_found, is_terminator;
    struct sm6_block *code_block;
    struct sm6_value *dst;
    size_t i, block_idx;

    if (sm6->function_count)
    {
        FIXME("Multiple functions are not supported yet.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (!(function->declaration = sm6_parser_next_function_definition(sm6)))
    {
        WARN("Failed to find definition to match function body.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (block->record_count < 2)
    {
        /* It should contain at least a block count and a RET instruction. */
        WARN("Invalid function block record count %zu.\n", block->record_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (block->records[0]->code != FUNC_CODE_DECLAREBLOCKS || !block->records[0]->operand_count
            || block->records[0]->operands[0] > UINT_MAX)
    {
        WARN("Block count declaration not found or invalid.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!(function->block_count = block->records[0]->operands[0]))
    {
        WARN("Function contains no blocks.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (function->block_count > 1)
    {
        FIXME("Branched shaders are not supported yet.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!(function->blocks[0] = sm6_block_create()))
    {
        ERR("Failed to allocate code block.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    code_block = function->blocks[0];

    sm6->cur_max_value = function->value_count;

    for (i = 1, block_idx = 0, ret_found = false; i < block->record_count; ++i)
    {
        sm6->p.location.column = i;

        if (!code_block)
        {
            WARN("Invalid block count %zu.\n", function->block_count);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Invalid block count %zu.", function->block_count);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        /* block->record_count - 1 is the instruction count, but some instructions
         * can emit >1 IR instruction, so extra may be used. */
        if (!vkd3d_array_reserve((void **)&code_block->instructions, &code_block->instruction_capacity,
                max(code_block->instruction_count + 1, block->record_count), sizeof(*code_block->instructions)))
        {
            ERR("Failed to allocate instructions.\n");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }

        ins = &code_block->instructions[code_block->instruction_count];
        ins->handler_idx = VKD3DSIH_INVALID;

        dst = sm6_parser_get_current_value(sm6);
        dst->type = NULL;
        dst->value_type = VALUE_TYPE_REG;
        is_terminator = false;

        record = block->records[i];
        switch (record->code)
        {
            case FUNC_CODE_INST_CALL:
                sm6_parser_emit_call(sm6, record, code_block, ins, dst);
                break;
            case FUNC_CODE_INST_RET:
                sm6_parser_emit_ret(sm6, record, code_block, ins);
                is_terminator = true;
                ret_found = true;
                break;
            default:
                FIXME("Unhandled dxil instruction %u.\n", record->code);
                return VKD3D_ERROR_INVALID_SHADER;
        }

        if (sm6->p.failed)
            return VKD3D_ERROR;
        assert(ins->handler_idx != VKD3DSIH_INVALID);

        if (is_terminator)
        {
            ++block_idx;
            code_block = (block_idx < function->block_count) ? function->blocks[block_idx] : NULL;
        }
        if (code_block)
            code_block->instruction_count += ins->handler_idx != VKD3DSIH_NOP;
        else
            assert(ins->handler_idx == VKD3DSIH_NOP);

        sm6->value_count += !!dst->type;
    }

    if (!ret_found)
    {
        WARN("Function contains no RET instruction.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    return VKD3D_OK;
}

static bool sm6_block_emit_instructions(struct sm6_block *block, struct sm6_parser *sm6)
{
    struct vkd3d_shader_instruction *ins = sm6_parser_require_space(sm6, block->instruction_count + 1);

    if (!ins)
        return false;

    memcpy(ins, block->instructions, block->instruction_count * sizeof(*block->instructions));
    sm6->p.instructions.count += block->instruction_count;

    sm6_parser_add_instruction(sm6, VKD3DSIH_RET);

    return true;
}

static enum vkd3d_result sm6_parser_module_init(struct sm6_parser *sm6, const struct dxil_block *block,
        unsigned int level)
{
    size_t i, old_value_count = sm6->value_count;
    struct sm6_function *function;
    enum vkd3d_result ret;

    for (i = 0; i < block->child_block_count; ++i)
    {
        if ((ret = sm6_parser_module_init(sm6, block->child_blocks[i], level + 1)) < 0)
            return ret;
    }

    sm6->p.location.line = block->id;
    sm6->p.location.column = 0;

    switch (block->id)
    {
        case CONSTANTS_BLOCK:
            function = &sm6->functions[sm6->function_count];
            sm6->cur_max_value = function->value_count;
            return sm6_parser_constants_init(sm6, block);

        case FUNCTION_BLOCK:
            function = &sm6->functions[sm6->function_count];
            if ((ret = sm6_parser_function_init(sm6, block, function)) < 0)
                return ret;
            /* The value index returns to its previous value after handling a function. It's usually nonzero
             * at the start because of global constants/variables/function declarations. Function constants
             * occur in a child block, so value_count is already saved before they are emitted. */
            memset(&sm6->values[old_value_count], 0, (sm6->value_count - old_value_count) * sizeof(*sm6->values));
            sm6->value_count = old_value_count;
            break;

        case BLOCKINFO_BLOCK:
        case MODULE_BLOCK:
        case PARAMATTR_BLOCK:
        case PARAMATTR_GROUP_BLOCK:
        case VALUE_SYMTAB_BLOCK:
        case METADATA_BLOCK:
        case METADATA_ATTACHMENT_BLOCK:
        case TYPE_BLOCK:
            break;

        default:
            FIXME("Unhandled block id %u.\n", block->id);
            break;
    }

    return VKD3D_OK;
}

static void sm6_type_table_cleanup(struct sm6_type *types, size_t count)
{
    size_t i;

    if (!types)
        return;

    for (i = 0; i < count; ++i)
    {
        switch (types[i].class)
        {
            case TYPE_CLASS_STRUCT:
                vkd3d_free((void *)types[i].u.struc->name);
                vkd3d_free(types[i].u.struc);
                break;
            case TYPE_CLASS_FUNCTION:
                vkd3d_free(types[i].u.function);
                break;
            default:
                break;
        }
    }

    vkd3d_free(types);
}

static void sm6_symtab_cleanup(struct sm6_symbol *symbols, size_t count)
{
    size_t i;

    for (i = 0; i < count; ++i)
        vkd3d_free((void *)symbols[i].name);
    vkd3d_free(symbols);
}

static void sm6_block_destroy(struct sm6_block *block)
{
    vkd3d_free(block->instructions);
    vkd3d_free(block);
}

static void sm6_functions_cleanup(struct sm6_function *functions, size_t count)
{
    size_t i, j;

    for (i = 0; i < count; ++i)
    {
        for (j = 0; j < functions[i].block_count; ++j)
            sm6_block_destroy(functions[i].blocks[j]);
    }
    vkd3d_free(functions);
}

static void sm6_parser_destroy(struct vkd3d_shader_parser *parser)
{
    struct sm6_parser *sm6 = sm6_parser(parser);

    dxil_block_destroy(&sm6->root_block);
    dxil_global_abbrevs_cleanup(sm6->abbrevs, sm6->abbrev_count);
    shader_instruction_array_destroy(&parser->instructions);
    sm6_type_table_cleanup(sm6->types, sm6->type_count);
    sm6_symtab_cleanup(sm6->global_symbols, sm6->global_symbol_count);
    sm6_functions_cleanup(sm6->functions, sm6->function_count);
    vkd3d_free(sm6->values);
    free_shader_desc(&parser->shader_desc);
    vkd3d_free(sm6);
}

static const struct vkd3d_shader_parser_ops sm6_parser_ops =
{
    .parser_destroy = sm6_parser_destroy,
};

static enum vkd3d_result sm6_parser_init(struct sm6_parser *sm6, const uint32_t *byte_code, size_t byte_code_size,
        const char *source_name, struct vkd3d_shader_message_context *message_context)
{
    const struct shader_signature *output_signature = &sm6->p.shader_desc.output_signature;
    const struct vkd3d_shader_location location = {.source_name = source_name};
    uint32_t version_token, dxil_version, token_count, magic;
    unsigned int chunk_offset, chunk_size;
    size_t count, length, function_count;
    enum bitcode_block_abbreviation abbr;
    struct vkd3d_shader_version version;
    struct dxil_block *block;
    enum vkd3d_result ret;
    unsigned int i;

    count = byte_code_size / sizeof(*byte_code);
    if (count < 6)
    {
        WARN("Invalid data size %zu.\n", byte_code_size);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_SIZE,
                "DXIL chunk size %zu is smaller than the DXIL header size.", byte_code_size);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    version_token = byte_code[0];
    TRACE("Compiler version: 0x%08x.\n", version_token);
    token_count = byte_code[1];
    TRACE("Token count: %u.\n", token_count);

    if (token_count < 6 || count < token_count)
    {
        WARN("Invalid token count %u (word count %zu).\n", token_count, count);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_CHUNK_SIZE,
                "DXIL chunk token count %#x is invalid (word count %zu).", token_count, count);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (byte_code[2] != TAG_DXIL)
        WARN("Unknown magic number 0x%08x.\n", byte_code[2]);

    dxil_version = byte_code[3];
    if (dxil_version > 0x102)
        WARN("Unknown DXIL version: 0x%08x.\n", dxil_version);
    else
        TRACE("DXIL version: 0x%08x.\n", dxil_version);

    chunk_offset = byte_code[4];
    if (chunk_offset < 16 || chunk_offset >= byte_code_size)
    {
        WARN("Invalid bitcode chunk offset %#x (data size %zu).\n", chunk_offset, byte_code_size);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_CHUNK_OFFSET,
                "DXIL bitcode chunk has invalid offset %#x (data size %#zx).", chunk_offset, byte_code_size);
        return VKD3D_ERROR_INVALID_SHADER;
    }
    chunk_size = byte_code[5];
    if (chunk_size > byte_code_size - chunk_offset)
    {
        WARN("Invalid bitcode chunk size %#x (data size %zu, chunk offset %#x).\n",
                chunk_size, byte_code_size, chunk_offset);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_CHUNK_SIZE,
                "DXIL bitcode chunk has invalid size %#x (data size %#zx, chunk offset %#x).",
                chunk_size, byte_code_size, chunk_offset);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    sm6->start = (const uint32_t *)((const char*)&byte_code[2] + chunk_offset);
    if ((magic = sm6->start[0]) != BITCODE_MAGIC)
    {
        WARN("Unknown magic number 0x%08x.\n", magic);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_UNKNOWN_MAGIC_NUMBER,
                "DXIL bitcode chunk magic number 0x%08x is not the expected 0x%08x.", magic, BITCODE_MAGIC);
    }

    sm6->end = &sm6->start[(chunk_size + sizeof(*sm6->start) - 1) / sizeof(*sm6->start)];

    if ((version.type = version_token >> 16) >= VKD3D_SHADER_TYPE_COUNT)
    {
        FIXME("Unknown shader type %#x.\n", version.type);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_UNKNOWN_SHADER_TYPE,
                "Unknown shader type %#x.", version.type);
    }

    version.major = VKD3D_SM6_VERSION_MAJOR(version_token);
    version.minor = VKD3D_SM6_VERSION_MINOR(version_token);

    if ((abbr = sm6->start[1] & 3) != ENTER_SUBBLOCK)
    {
        WARN("Initial block abbreviation %u is not ENTER_SUBBLOCK.\n", abbr);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_BITCODE,
                "DXIL bitcode chunk has invalid initial block abbreviation %u.", abbr);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    /* Estimate instruction count to avoid reallocation in most shaders. */
    count = max(token_count, 400) - 400;
    vkd3d_shader_parser_init(&sm6->p, message_context, source_name, &version, &sm6_parser_ops,
            (count + (count >> 2)) / 2u + 10);
    sm6->ptr = &sm6->start[1];
    sm6->bitpos = 2;

    block = &sm6->root_block;
    if ((ret = dxil_block_init(block, NULL, sm6)) < 0)
    {
        if (ret == VKD3D_ERROR_OUT_OF_MEMORY)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory parsing DXIL bitcode chunk.");
        else if (ret == VKD3D_ERROR_INVALID_SHADER)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_BITCODE,
                    "DXIL bitcode chunk has invalid bitcode.");
        else
            vkd3d_unreachable();
        return ret;
    }

    dxil_global_abbrevs_cleanup(sm6->abbrevs, sm6->abbrev_count);
    sm6->abbrevs = NULL;
    sm6->abbrev_count = 0;

    length = sm6->ptr - sm6->start - block->start;
    if (length != block->length)
    {
        WARN("Invalid block length %zu; expected %u.\n", length, block->length);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_INVALID_BLOCK_LENGTH,
                "Root block ends with length %zu but indicated length is %u.", length, block->length);
    }
    if (sm6->ptr != sm6->end)
    {
        size_t expected_length = sm6->end - sm6->start;
        length = sm6->ptr - sm6->start;
        WARN("Invalid module length %zu; expected %zu.\n", length, expected_length);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_INVALID_MODULE_LENGTH,
                "Module ends with length %zu but indicated length is %zu.", length, expected_length);
    }

    if ((ret = sm6_parser_type_table_init(sm6)) < 0)
    {
        if (ret == VKD3D_ERROR_OUT_OF_MEMORY)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory parsing DXIL type table.");
        else if (ret == VKD3D_ERROR_INVALID_SHADER)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_TYPE_TABLE,
                    "DXIL type table is invalid.");
        else
            vkd3d_unreachable();
        return ret;
    }

    if ((ret = sm6_parser_symtab_init(sm6)) < 0)
    {
        if (ret == VKD3D_ERROR_OUT_OF_MEMORY)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory parsing DXIL value symbol table.");
        else if (ret == VKD3D_ERROR_INVALID_SHADER)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_VALUE_SYMTAB,
                    "DXIL value symbol table is invalid.");
        else
            vkd3d_unreachable();
        return ret;
    }

    if (!(sm6->output_params = shader_parser_get_dst_params(&sm6->p, output_signature->element_count)))
    {
        ERR("Failed to allocate output parameters.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating output parameters.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    function_count = dxil_block_compute_function_count(&sm6->root_block);
    if (!(sm6->functions = vkd3d_calloc(function_count, sizeof(*sm6->functions))))
    {
        ERR("Failed to allocate function array.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating DXIL function array.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if (sm6_parser_compute_max_value_count(sm6, &sm6->root_block, 0) == SIZE_MAX)
    {
        WARN("Value array count overflowed.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_MODULE,
                "Overflow occurred in the DXIL module value count.");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (!(sm6->values = vkd3d_calloc(sm6->value_capacity, sizeof(*sm6->values))))
    {
        ERR("Failed to allocate value array.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating DXIL value array.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if ((ret = sm6_parser_globals_init(sm6)) < 0)
    {
        WARN("Failed to load global declarations.\n");
        return ret;
    }

    sm6_parser_init_output_signature(sm6, output_signature);

    if ((ret = sm6_parser_module_init(sm6, &sm6->root_block, 0)) < 0)
    {
        if (ret == VKD3D_ERROR_OUT_OF_MEMORY)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory parsing DXIL module.");
        else if (ret == VKD3D_ERROR_INVALID_SHADER)
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_MODULE,
                    "DXIL module is invalid.");
        return ret;
    }

    if (!sm6_parser_require_space(sm6, output_signature->element_count))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory emitting shader signature declarations.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    sm6_parser_emit_output_signature(sm6, output_signature);

    for (i = 0; i < sm6->function_count; ++i)
    {
        if (!sm6_block_emit_instructions(sm6->functions[i].blocks[0], sm6))
        {
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory emitting shader instructions.");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
    }

    dxil_block_destroy(&sm6->root_block);

    return VKD3D_OK;
}

int vkd3d_shader_sm6_parser_create(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_parser **parser)
{
    struct vkd3d_shader_desc *shader_desc;
    uint32_t *byte_code = NULL;
    struct sm6_parser *sm6;
    int ret;

    ERR("Creating a DXIL parser. This is unsupported; you get to keep all the pieces if it breaks.\n");

    if (!(sm6 = vkd3d_calloc(1, sizeof(*sm6))))
    {
        ERR("Failed to allocate parser.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    shader_desc = &sm6->p.shader_desc;
    shader_desc->is_dxil = true;
    if ((ret = shader_extract_from_dxbc(&compile_info->source, message_context, compile_info->source_name,
            shader_desc)) < 0)
    {
        WARN("Failed to extract shader, vkd3d result %d.\n", ret);
        vkd3d_free(sm6);
        return ret;
    }

    sm6->p.shader_desc = *shader_desc;
    shader_desc = &sm6->p.shader_desc;

    if (((uintptr_t)shader_desc->byte_code & (VKD3D_DXBC_CHUNK_ALIGNMENT - 1)))
    {
        /* LLVM bitcode should be 32-bit aligned, but before dxc v1.7.2207 this was not always the case in the DXBC
         * container due to missing padding after signature names. Get an aligned copy to prevent unaligned access. */
        if (!(byte_code = vkd3d_malloc(align(shader_desc->byte_code_size, VKD3D_DXBC_CHUNK_ALIGNMENT))))
            ERR("Failed to allocate aligned chunk. Unaligned access will occur.\n");
        else
            memcpy(byte_code, shader_desc->byte_code, shader_desc->byte_code_size);
    }

    ret = sm6_parser_init(sm6, byte_code ? byte_code : shader_desc->byte_code, shader_desc->byte_code_size,
            compile_info->source_name, message_context);
    vkd3d_free(byte_code);

    if (ret < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        sm6_parser_destroy(&sm6->p);
        return ret;
    }

    *parser = &sm6->p;

    return ret;
}
