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
/* Two seems to be the maximum but leave some extra room. */
#define VKD3D_SM6_MAX_METADATA_TABLES 4

#define BITCODE_MAGIC VKD3D_MAKE_TAG('B', 'C', 0xc0, 0xde)
#define DXIL_OP_MAX_OPERANDS 17
static const uint64_t MAX_ALIGNMENT_EXPONENT = 29;
static const uint64_t GLOBALVAR_FLAG_IS_CONSTANT = 1;
static const uint64_t GLOBALVAR_FLAG_EXPLICIT_TYPE = 2;
static const unsigned int GLOBALVAR_ADDRESS_SPACE_SHIFT = 2;
static const unsigned int SHADER_DESCRIPTOR_TYPE_COUNT = 4;

static const unsigned int dx_max_thread_group_size[3] = {1024, 1024, 64};

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

enum bitcode_metadata_code
{
    METADATA_STRING             =  1,
    METADATA_VALUE              =  2,
    METADATA_NODE               =  3,
    METADATA_NAME               =  4,
    METADATA_DISTINCT_NODE      =  5,
    METADATA_KIND               =  6,
    METADATA_LOCATION           =  7,
    METADATA_NAMED_NODE         = 10,
    METADATA_ATTACHMENT         = 11,
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

enum bitcode_linkage
{
    LINKAGE_EXTERNAL  = 0,
    LINKAGE_APPENDING = 2,
    LINKAGE_INTERNAL  = 3,
};

enum dxil_component_type
{
    COMPONENT_TYPE_INVALID     =  0,
    COMPONENT_TYPE_I1          =  1,
    COMPONENT_TYPE_I16         =  2,
    COMPONENT_TYPE_U16         =  3,
    COMPONENT_TYPE_I32         =  4,
    COMPONENT_TYPE_U32         =  5,
    COMPONENT_TYPE_I64         =  6,
    COMPONENT_TYPE_U64         =  7,
    COMPONENT_TYPE_F16         =  8,
    COMPONENT_TYPE_F32         =  9,
    COMPONENT_TYPE_F64         = 10,
    COMPONENT_TYPE_SNORMF16    = 11,
    COMPONENT_TYPE_UNORMF16    = 12,
    COMPONENT_TYPE_SNORMF32    = 13,
    COMPONENT_TYPE_UNORMF32    = 14,
    COMPONENT_TYPE_SNORMF64    = 15,
    COMPONENT_TYPE_UNORMF64    = 16,
    COMPONENT_TYPE_PACKEDS8X32 = 17,
    COMPONENT_TYPE_PACKEDU8X32 = 18,
};

enum dxil_semantic_kind
{
    SEMANTIC_KIND_ARBITRARY            =  0,
    SEMANTIC_KIND_VERTEXID             =  1,
    SEMANTIC_KIND_INSTANCEID           =  2,
    SEMANTIC_KIND_POSITION             =  3,
    SEMANTIC_KIND_RTARRAYINDEX         =  4,
    SEMANTIC_KIND_VIEWPORTARRAYINDEX   =  5,
    SEMANTIC_KIND_CLIPDISTANCE         =  6,
    SEMANTIC_KIND_CULLDISTANCE         =  7,
    SEMANTIC_KIND_OUTPUTCONTROLPOINTID =  8,
    SEMANTIC_KIND_DOMAINLOCATION       =  9,
    SEMANTIC_KIND_PRIMITIVEID          = 10,
    SEMANTIC_KIND_GSINSTANCEID         = 11,
    SEMANTIC_KIND_SAMPLEINDEX          = 12,
    SEMANTIC_KIND_ISFRONTFACE          = 13,
    SEMANTIC_KIND_COVERAGE             = 14,
    SEMANTIC_KIND_INNERCOVERAGE        = 15,
    SEMANTIC_KIND_TARGET               = 16,
    SEMANTIC_KIND_DEPTH                = 17,
    SEMANTIC_KIND_DEPTHLESSEQUAL       = 18,
    SEMANTIC_KIND_DEPTHGREATEREQUAL    = 19,
    SEMANTIC_KIND_STENCILREF           = 20,
    SEMANTIC_KIND_DISPATCHTHREADID     = 21,
    SEMANTIC_KIND_GROUPID              = 22,
    SEMANTIC_KIND_GROUPINDEX           = 23,
    SEMANTIC_KIND_GROUPTHREADID        = 24,
    SEMANTIC_KIND_TESSFACTOR           = 25,
    SEMANTIC_KIND_INSIDETESSFACTOR     = 26,
    SEMANTIC_KIND_VIEWID               = 27,
    SEMANTIC_KIND_BARYCENTRICS         = 28,
    SEMANTIC_KIND_SHADINGRATE          = 29,
    SEMANTIC_KIND_CULLPRIMITIVE        = 30,
    SEMANTIC_KIND_COUNT                = 31,
    SEMANTIC_KIND_INVALID              = SEMANTIC_KIND_COUNT,
};

enum dxil_element_additional_tag
{
    ADDITIONAL_TAG_STREAM_INDEX  = 0,
    ADDITIONAL_TAG_GLOBAL_SYMBOL = 1, /* not used */
    ADDITIONAL_TAG_RELADDR_MASK  = 2,
    ADDITIONAL_TAG_USED_MASK     = 3,
};

enum dxil_shader_properties_tag
{
    SHADER_PROPERTIES_FLAGS              =  0,
    SHADER_PROPERTIES_GEOMETRY           =  1,
    SHADER_PROPERTIES_DOMAIN             =  2,
    SHADER_PROPERTIES_HULL               =  3,
    SHADER_PROPERTIES_COMPUTE            =  4,
    SHADER_PROPERTIES_AUTO_BINDING_SPACE =  5,
    SHADER_PROPERTIES_RAY_PAYLOAD_SIZE   =  6,
    SHADER_PROPERTIES_RAY_ATTRIB_SIZE    =  7,
    SHADER_PROPERTIES_SHADER_KIND        =  8,
    SHADER_PROPERTIES_MESH               =  9,
    SHADER_PROPERTIES_AMPLIFICATION      = 10,
    SHADER_PROPERTIES_WAVE_SIZE          = 11,
    SHADER_PROPERTIES_ENTRY_ROOT_SIG     = 12,
};

enum dxil_binop_code
{
    BINOP_ADD  =  0,
    BINOP_SUB  =  1,
    BINOP_MUL  =  2,
    BINOP_UDIV =  3,
    BINOP_SDIV =  4,
    BINOP_UREM =  5,
    BINOP_SREM =  6,
    BINOP_SHL  =  7,
    BINOP_LSHR =  8,
    BINOP_ASHR =  9,
    BINOP_AND  = 10,
    BINOP_OR   = 11,
    BINOP_XOR  = 12
};

enum dxil_fast_fp_flags
{
    FP_ALLOW_UNSAFE_ALGEBRA =  0x1,
    FP_NO_NAN               =  0x2,
    FP_NO_INF               =  0x4,
    FP_NO_SIGNED_ZEROS      =  0x8,
    FP_ALLOW_RECIPROCAL     = 0x10,
};

enum dxil_overflowing_binop_flags
{
    /* Operation is known to never overflow. */
    OB_NO_UNSIGNED_WRAP = 0x1,
    OB_NO_SIGNED_WRAP   = 0x2,
};

enum dxil_possibly_exact_binop_flags
{
    /* "A udiv or sdiv instruction, which can be marked as "exact", indicating that no bits are destroyed." */
    PEB_EXACT = 0x1,
};

enum dx_intrinsic_opcode
{
    DX_LOAD_INPUT                   =   4,
    DX_STORE_OUTPUT                 =   5,
    DX_CREATE_HANDLE                =  57,
    DX_CBUFFER_LOAD_LEGACY          =  59,
};

enum dxil_cast_code
{
    CAST_TRUNC    =  0,
    CAST_ZEXT     =  1,
    CAST_SEXT     =  2,
    CAST_FPTOUI   =  3,
    CAST_FPTOSI   =  4,
    CAST_UITOFP   =  5,
    CAST_SITOFP   =  6,
    CAST_FPTRUNC  =  7,
    CAST_FPEXT    =  8,
    CAST_PTRTOINT =  9,
    CAST_INTTOPTR = 10,
    CAST_BITCAST  = 11,
    CAST_ADDRSPACECAST = 12,
};

enum dxil_predicate
{
    FCMP_FALSE =  0,
    FCMP_OEQ   =  1,
    FCMP_OGT   =  2,
    FCMP_OGE   =  3,
    FCMP_OLT   =  4,
    FCMP_OLE   =  5,
    FCMP_ONE   =  6,
    FCMP_ORD   =  7,
    FCMP_UNO   =  8,
    FCMP_UEQ   =  9,
    FCMP_UGT   = 10,
    FCMP_UGE   = 11,
    FCMP_ULT   = 12,
    FCMP_ULE   = 13,
    FCMP_UNE   = 14,
    FCMP_TRUE  = 15,
    ICMP_EQ    = 32,
    ICMP_NE    = 33,
    ICMP_UGT   = 34,
    ICMP_UGE   = 35,
    ICMP_ULT   = 36,
    ICMP_ULE   = 37,
    ICMP_SGT   = 38,
    ICMP_SGE   = 39,
    ICMP_SLT   = 40,
    ICMP_SLE   = 41,
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
    VALUE_TYPE_ICB,
    VALUE_TYPE_HANDLE,
};

struct sm6_function_data
{
    const char *name;
    bool is_prototype;
    unsigned int attribs_id;
};

struct sm6_handle_data
{
    const struct sm6_descriptor_info *d;
    struct vkd3d_shader_register reg;
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
        const struct vkd3d_shader_immediate_constant_buffer *icb;
        struct sm6_handle_data handle;
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

enum sm6_metadata_type
{
    VKD3D_METADATA_KIND,
    VKD3D_METADATA_NODE,
    VKD3D_METADATA_STRING,
    VKD3D_METADATA_VALUE,
};

struct sm6_metadata_node
{
    bool is_distinct;
    unsigned int operand_count;
    struct sm6_metadata_value *operands[];
};

struct sm6_metadata_kind
{
    uint64_t id;
    char *name;
};

struct sm6_metadata_value
{
    enum sm6_metadata_type type;
    const struct sm6_type *value_type;
    union
    {
        char *string_value;
        const struct sm6_value *value;
        struct sm6_metadata_node *node;
        struct sm6_metadata_kind kind;
    } u;
};

struct sm6_metadata_table
{
    struct sm6_metadata_value *values;
    unsigned int count;
};

struct sm6_named_metadata
{
    char *name;
    struct sm6_metadata_value value;
};

struct sm6_descriptor_info
{
    enum vkd3d_shader_descriptor_type type;
    unsigned int id;
    struct vkd3d_shader_register_range range;
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
    struct sm6_type *bool_type;
    struct sm6_type *metadata_type;
    struct sm6_type *handle_type;

    struct sm6_symbol *global_symbols;
    size_t global_symbol_count;

    const char *entry_point;

    struct vkd3d_shader_dst_param *output_params;
    struct vkd3d_shader_dst_param *input_params;

    struct sm6_function *functions;
    size_t function_count;

    struct sm6_metadata_table metadata_tables[VKD3D_SM6_MAX_METADATA_TABLES];
    struct sm6_named_metadata *named_metadata;
    unsigned int named_metadata_count;

    struct sm6_descriptor_info *descriptors;
    size_t descriptor_capacity;
    size_t descriptor_count;

    unsigned int indexable_temp_count;

    struct sm6_value *values;
    size_t value_count;
    size_t value_capacity;
    size_t cur_max_value;
    unsigned int ssa_next_id;

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

static char *dxil_record_to_string(const struct dxil_record *record, unsigned int offset, struct sm6_parser *sm6)
{
    unsigned int i;
    char *str;

    assert(offset <= record->operand_count);
    if (!(str = vkd3d_calloc(record->operand_count - offset + 1, 1)))
    {
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating a string of length %u.", record->operand_count - offset);
        return NULL;
    }

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
                        sm6->bool_type = type;
                        break;
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
                sm6->metadata_type = type;
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
                if (!dxil_record_validate_operand_min_count(record, 1, sm6))
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

                if (!ascii_strcasecmp(struct_name, "dx.types.Handle"))
                    sm6->handle_type = type;

                type->u.struc->name = struct_name;
                struct_name = NULL;
                break;

            case TYPE_CODE_STRUCT_NAME:
                if (!(struct_name = dxil_record_to_string(record, 0, sm6)))
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

static bool sm6_type_is_bool_i16_i32_i64(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER && (type->u.width == 1 || type->u.width >= 16);
}

static bool sm6_type_is_bool(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER && type->u.width == 1;
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

static bool sm6_type_is_scalar(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER || type->class == TYPE_CLASS_FLOAT || type->class == TYPE_CLASS_POINTER;
}

static inline bool sm6_type_is_numeric(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_INTEGER || type->class == TYPE_CLASS_FLOAT;
}

static inline bool sm6_type_is_pointer(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_POINTER;
}

static bool sm6_type_is_aggregate(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_STRUCT || type->class == TYPE_CLASS_VECTOR || type->class == TYPE_CLASS_ARRAY;
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

static bool sm6_type_is_array(const struct sm6_type *type)
{
    return type->class == TYPE_CLASS_ARRAY;
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

/* Call for aggregate types only. */
static const struct sm6_type *sm6_type_get_element_type_at_index(const struct sm6_type *type, uint64_t elem_idx)
{
    switch (type->class)
    {
        case TYPE_CLASS_ARRAY:
        case TYPE_CLASS_VECTOR:
            if (elem_idx >= type->u.array.count)
                return NULL;
            return type->u.array.elem_type;

        case TYPE_CLASS_STRUCT:
            if (elem_idx >= type->u.struc->elem_count)
                return NULL;
            return type->u.struc->elem_types[elem_idx];

        default:
            vkd3d_unreachable();
    }
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

static unsigned int sm6_type_max_vector_size(const struct sm6_type *type)
{
    return min((VKD3D_VEC4_SIZE * sizeof(uint32_t) * CHAR_BIT) / type->u.width, VKD3D_VEC4_SIZE);
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
        if (!(symbol->name = dxil_record_to_string(record, 1, sm6)))
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
    if (!register_is_constant(reg) || (!data_type_is_integer(reg->data_type) && !data_type_is_bool(reg->data_type)))
        return UINT_MAX;

    if (reg->dimension == VSIR_DIMENSION_VEC4)
        WARN("Returning vec4.x.\n");

    if (reg->type == VKD3DSPR_IMMCONST64)
    {
        if (reg->u.immconst_uint64[0] > UINT_MAX)
            FIXME("Truncating 64-bit value.\n");
        return reg->u.immconst_uint64[0];
    }

    return reg->u.immconst_uint[0];
}

static uint64_t register_get_uint64_value(const struct vkd3d_shader_register *reg)
{
    if (!register_is_constant(reg) || !data_type_is_integer(reg->data_type))
        return UINT64_MAX;

    if (reg->dimension == VSIR_DIMENSION_VEC4)
        WARN("Returning vec4.x.\n");

    return (reg->type == VKD3DSPR_IMMCONST64) ? reg->u.immconst_uint64[0] : reg->u.immconst_uint[0];
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

static bool sm6_value_is_handle(const struct sm6_value *value)
{
    return value->value_type == VALUE_TYPE_HANDLE;
}

static inline bool sm6_value_is_constant(const struct sm6_value *value)
{
    return sm6_value_is_register(value) && register_is_constant(&value->u.reg);
}

static inline bool sm6_value_is_undef(const struct sm6_value *value)
{
    return sm6_value_is_register(value) && value->u.reg.type == VKD3DSPR_UNDEF;
}

static bool sm6_value_is_icb(const struct sm6_value *value)
{
    return value->value_type == VALUE_TYPE_ICB;
}

static inline unsigned int sm6_value_get_constant_uint(const struct sm6_value *value)
{
    if (!sm6_value_is_constant(value))
        return UINT_MAX;
    return register_get_uint_value(&value->u.reg);
}

static unsigned int sm6_parser_alloc_ssa_id(struct sm6_parser *sm6)
{
    return sm6->ssa_next_id++;
}

static struct vkd3d_shader_src_param *instruction_src_params_alloc(struct vkd3d_shader_instruction *ins,
        unsigned int count, struct sm6_parser *sm6)
{
    struct vkd3d_shader_src_param *params = shader_parser_get_src_params(&sm6->p, count);
    if (!params)
    {
        ERR("Failed to allocate src params.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating instruction src parameters.");
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
                "Out of memory allocating instruction dst parameters.");
        return NULL;
    }
    ins->dst = params;
    ins->dst_count = count;
    return params;
}

static void register_init_with_id(struct vkd3d_shader_register *reg,
        enum vkd3d_shader_register_type reg_type, enum vkd3d_data_type data_type, unsigned int id)
{
    vsir_register_init(reg, reg_type, data_type, 1);
    reg->idx[0].offset = id;
}

static enum vkd3d_data_type vkd3d_data_type_from_sm6_type(const struct sm6_type *type)
{
    if (type->class == TYPE_CLASS_INTEGER)
    {
        switch (type->u.width)
        {
            case 1:
                return VKD3D_DATA_BOOL;
            case 8:
                return VKD3D_DATA_UINT8;
            case 32:
                return VKD3D_DATA_UINT;
            case 64:
                return VKD3D_DATA_UINT64;
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

static void register_init_ssa_vector(struct vkd3d_shader_register *reg, const struct sm6_type *type,
        unsigned int component_count, struct sm6_parser *sm6)
{
    enum vkd3d_data_type data_type;
    unsigned int id;

    id = sm6_parser_alloc_ssa_id(sm6);
    data_type = vkd3d_data_type_from_sm6_type(sm6_type_get_scalar_type(type, 0));
    register_init_with_id(reg, VKD3DSPR_SSA, data_type, id);
    reg->dimension = component_count > 1 ? VSIR_DIMENSION_VEC4 : VSIR_DIMENSION_SCALAR;
}

static void register_init_ssa_scalar(struct vkd3d_shader_register *reg, const struct sm6_type *type,
        struct sm6_parser *sm6)
{
    register_init_ssa_vector(reg, type, 1, sm6);
}

static void dst_param_init(struct vkd3d_shader_dst_param *param)
{
    param->write_mask = VKD3DSP_WRITEMASK_0;
    param->modifiers = 0;
    param->shift = 0;
}

static inline void dst_param_init_scalar(struct vkd3d_shader_dst_param *param, unsigned int component_idx)
{
    param->write_mask = 1u << component_idx;
    param->modifiers = 0;
    param->shift = 0;
}

static void dst_param_init_vector(struct vkd3d_shader_dst_param *param, unsigned int component_count)
{
    param->write_mask = (1u << component_count) - 1;
    param->modifiers = 0;
    param->shift = 0;
}

static void dst_param_init_ssa_scalar(struct vkd3d_shader_dst_param *param, const struct sm6_type *type,
        struct sm6_parser *sm6)
{
    dst_param_init(param);
    register_init_ssa_scalar(&param->reg, type, sm6);
}

static inline void src_param_init(struct vkd3d_shader_src_param *param)
{
    param->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    param->modifiers = VKD3DSPSM_NONE;
}

static void src_param_init_scalar(struct vkd3d_shader_src_param *param, unsigned int component_idx)
{
    param->swizzle = vkd3d_shader_create_swizzle(component_idx, component_idx, component_idx, component_idx);
    param->modifiers = VKD3DSPSM_NONE;
}

static void src_param_init_from_value(struct vkd3d_shader_src_param *param, const struct sm6_value *src)
{
    src_param_init(param);
    param->reg = src->u.reg;
}

static void src_param_init_vector_from_reg(struct vkd3d_shader_src_param *param,
        const struct vkd3d_shader_register *reg)
{
    param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    param->modifiers = VKD3DSPSM_NONE;
    param->reg = *reg;
}

static void register_index_address_init(struct vkd3d_shader_register_index *idx, const struct sm6_value *address,
        struct sm6_parser *sm6)
{
    if (sm6_value_is_constant(address))
    {
        idx->offset = sm6_value_get_constant_uint(address);
    }
    else if (sm6_value_is_undef(address))
    {
        idx->offset = 0;
    }
    else
    {
        struct vkd3d_shader_src_param *rel_addr = shader_parser_get_src_params(&sm6->p, 1);
        if (rel_addr)
            src_param_init_from_value(rel_addr, address);
        idx->offset = 0;
        idx->rel_addr = rel_addr;
    }
}

static void instruction_dst_param_init_ssa_scalar(struct vkd3d_shader_instruction *ins, struct sm6_parser *sm6)
{
    struct vkd3d_shader_dst_param *param = instruction_dst_params_alloc(ins, 1, sm6);
    struct sm6_value *dst = sm6_parser_get_current_value(sm6);

    dst_param_init_ssa_scalar(param, dst->type, sm6);
    param->write_mask = VKD3DSP_WRITEMASK_0;
    dst->u.reg = param->reg;
}

static void instruction_dst_param_init_ssa_vector(struct vkd3d_shader_instruction *ins,
        unsigned int component_count, struct sm6_parser *sm6)
{
    struct vkd3d_shader_dst_param *param = instruction_dst_params_alloc(ins, 1, sm6);
    struct sm6_value *dst = sm6_parser_get_current_value(sm6);

    dst_param_init_vector(param, component_count);
    register_init_ssa_vector(&param->reg, sm6_type_get_scalar_type(dst->type, 0), component_count, sm6);
    dst->u.reg = param->reg;
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

    /* This may underflow to produce a forward reference, but it must not exceed the final value count. */
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

static bool sm6_value_validate_is_register(const struct sm6_value *value, struct sm6_parser *sm6)
{
    if (!sm6_value_is_register(value))
    {
        WARN("Operand of type %u is not a register.\n", value->value_type);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A register operand passed to a DXIL instruction is not a register.");
        return false;
    }
    return true;
}

static bool sm6_value_validate_is_handle(const struct sm6_value *value, struct sm6_parser *sm6)
{
    if (!sm6_value_is_handle(value))
    {
        WARN("Handle parameter of type %u is not a handle.\n", value->value_type);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCE_HANDLE,
                "A handle parameter passed to a DX intrinsic function is not a handle.");
        return false;
    }
    return true;
}

static bool sm6_value_validate_is_pointer(const struct sm6_value *value, struct sm6_parser *sm6)
{
    if (!sm6_type_is_pointer(value->type))
    {
        WARN("Operand result type class %u is not a pointer.\n", value->type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A pointer operand passed to a DXIL instruction is not a pointer.");
        return false;
    }
    return true;
}

static bool sm6_value_validate_is_numeric(const struct sm6_value *value, struct sm6_parser *sm6)
{
    if (!sm6_type_is_numeric(value->type))
    {
        WARN("Operand result type class %u is not numeric.\n", value->type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A numeric operand passed to a DXIL instruction is not numeric.");
        return false;
    }
    return true;
}

static bool sm6_value_validate_is_bool(const struct sm6_value *value, struct sm6_parser *sm6)
{
    const struct sm6_type *type = value->type;
    if (!sm6_type_is_bool(type))
    {
        WARN("Operand of type class %u is not bool.\n", type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A bool operand of type class %u passed to a DXIL instruction is not a bool.", type->class);
        return false;
    }
    return true;
}

static const struct sm6_value *sm6_parser_get_value_safe(struct sm6_parser *sm6, unsigned int idx)
{
    if (idx < sm6->value_count)
        return &sm6->values[idx];

    WARN("Invalid value index %u.\n", idx);
    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
            "Invalid value index %u.", idx);
    return NULL;
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

static enum vkd3d_result value_allocate_constant_array(struct sm6_value *dst, const struct sm6_type *type,
        const uint64_t *operands, struct sm6_parser *sm6)
{
    struct vkd3d_shader_immediate_constant_buffer *icb;
    const struct sm6_type *elem_type;
    unsigned int i, size, count;

    elem_type = type->u.array.elem_type;
    /* Multidimensional arrays are emitted in flattened form. */
    if (elem_type->class != TYPE_CLASS_INTEGER && elem_type->class != TYPE_CLASS_FLOAT)
    {
        FIXME("Unhandled element type %u for data array.\n", elem_type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "The element data type for an immediate constant buffer is not scalar integer or floating point.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    /* Arrays of bool are not used in DXIL. dxc will emit an array of int32 instead if necessary. */
    if (!(size = elem_type->u.width / CHAR_BIT))
    {
        WARN("Invalid data type width %u.\n", elem_type->u.width);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "An immediate constant buffer is declared with boolean elements.");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    size = max(size, sizeof(icb->data[0]));
    count = type->u.array.count * size / sizeof(icb->data[0]);

    if (!(icb = vkd3d_malloc(offsetof(struct vkd3d_shader_immediate_constant_buffer, data[count]))))
    {
        ERR("Failed to allocate buffer, count %u.\n", count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating an immediate constant buffer of count %u.", count);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    if (!shader_instruction_array_add_icb(&sm6->p.instructions, icb))
    {
        ERR("Failed to store icb object.\n");
        vkd3d_free(icb);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory storing an immediate constant buffer object.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    dst->value_type = VALUE_TYPE_ICB;
    dst->u.icb = icb;

    icb->data_type = vkd3d_data_type_from_sm6_type(elem_type);
    icb->element_count = type->u.array.count;
    icb->component_count = 1;

    count = type->u.array.count;
    if (size > sizeof(icb->data[0]))
    {
        uint64_t *data = (uint64_t *)icb->data;
        for (i = 0; i < count; ++i)
            data[i] = operands[i];
    }
    else
    {
        for (i = 0; i < count; ++i)
            icb->data[i] = operands[i];
    }

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_constants_init(struct sm6_parser *sm6, const struct dxil_block *block)
{
    enum vkd3d_shader_register_type reg_type = VKD3DSPR_INVALID;
    const struct sm6_type *type, *elem_type;
    enum vkd3d_data_type reg_data_type;
    const struct dxil_record *record;
    enum vkd3d_result ret;
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
        vsir_register_init(&dst->u.reg, reg_type, reg_data_type, 0);

        switch (record->code)
        {
            case CST_CODE_NULL:
                if (sm6_type_is_array(type))
                {
                    FIXME("Constant null arrays are not supported.\n");
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                            "Constant null arrays are not supported.");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                /* For non-aggregates, register constant data is already zero-filled. */
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
                if (!sm6_type_is_array(type))
                {
                    WARN("Invalid type %u for data constant idx %zu.\n", type->class, value_idx);
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                            "The type of a constant array is not an array type.");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                if (!dxil_record_validate_operand_count(record, type->u.array.count, type->u.array.count, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                if ((ret = value_allocate_constant_array(dst, type, record->operands, sm6)) < 0)
                    return ret;

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

static bool bitcode_parse_alignment(uint64_t encoded_alignment, unsigned int *alignment)
{
    if (encoded_alignment > MAX_ALIGNMENT_EXPONENT + 1)
    {
        *alignment = 0;
        return false;
    }
    *alignment = (1u << encoded_alignment) >> 1;
    return true;
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
    vsir_instruction_init(ins, &sm6->p.location, handler_idx);
    ++sm6->p.instructions.count;
    return ins;
}

static void sm6_parser_declare_indexable_temp(struct sm6_parser *sm6, const struct sm6_type *elem_type,
        unsigned int count, unsigned int alignment, unsigned int init, struct sm6_value *dst)
{
    enum vkd3d_data_type data_type = vkd3d_data_type_from_sm6_type(elem_type);
    struct vkd3d_shader_instruction *ins;

    ins = sm6_parser_add_instruction(sm6, VKD3DSIH_DCL_INDEXABLE_TEMP);
    ins->declaration.indexable_temp.register_idx = sm6->indexable_temp_count++;
    ins->declaration.indexable_temp.register_size = count;
    ins->declaration.indexable_temp.alignment = alignment;
    ins->declaration.indexable_temp.data_type = data_type;
    ins->declaration.indexable_temp.component_count = 1;
    /* The initialiser value index will be resolved later so forward references can be handled. */
    ins->declaration.indexable_temp.initialiser = (void *)(uintptr_t)init;

    register_init_with_id(&dst->u.reg, VKD3DSPR_IDXTEMP, data_type, ins->declaration.indexable_temp.register_idx);
}

static bool sm6_parser_declare_global(struct sm6_parser *sm6, const struct dxil_record *record)
{
    const struct sm6_type *type, *scalar_type;
    unsigned int alignment, count;
    uint64_t address_space, init;
    struct sm6_value *dst;
    bool is_constant;

    if (!dxil_record_validate_operand_min_count(record, 6, sm6))
        return false;

    if (!(type = sm6_parser_get_type(sm6, record->operands[0])))
        return false;
    if (sm6_type_is_array(type))
    {
        if (!sm6_type_is_scalar(type->u.array.elem_type))
        {
            FIXME("Unsupported nested type class %u.\n", type->u.array.elem_type->class);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Global array variables with nested type class %u are not supported.",
                    type->u.array.elem_type->class);
            return false;
        }
        count = type->u.array.count;
        scalar_type = type->u.array.elem_type;
    }
    else if (sm6_type_is_scalar(type))
    {
        count = 1;
        scalar_type = type;
    }
    else
    {
        FIXME("Unsupported type class %u.\n", type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Global variables of type class %u are not supported.", type->class);
        return false;
    }

    is_constant = record->operands[1] & GLOBALVAR_FLAG_IS_CONSTANT;

    if (record->operands[1] & GLOBALVAR_FLAG_EXPLICIT_TYPE)
    {
        address_space = record->operands[1] >> GLOBALVAR_ADDRESS_SPACE_SHIFT;

        if (!(type = sm6_type_get_pointer_to_type(type, address_space, sm6)))
        {
            WARN("Failed to get pointer type for type class %u, address space %"PRIu64".\n",
                    type->class, address_space);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_MODULE,
                    "Module does not define a pointer type for a global variable.");
            return false;
        }
    }
    else
    {
        if (!sm6_type_is_pointer(type))
        {
            WARN("Type is not a pointer.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "The type of a global variable is not a pointer.");
            return false;
        }
        address_space = type->u.pointer.addr_space;
    }

    if ((init = record->operands[2]))
    {
        if (init - 1 >= sm6->value_capacity)
        {
            WARN("Invalid value index %"PRIu64" for initialiser.", init - 1);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Global variable initialiser value index %"PRIu64" is invalid.", init - 1);
            return false;
        }
    }

    /* LINKAGE_EXTERNAL is common but not relevant here. */
    if (record->operands[3] != LINKAGE_EXTERNAL && record->operands[3] != LINKAGE_INTERNAL)
        WARN("Ignoring linkage %"PRIu64".\n", record->operands[3]);

    if (!bitcode_parse_alignment(record->operands[4], &alignment))
        WARN("Invalid alignment %"PRIu64".\n", record->operands[4]);

    if (record->operands[5])
        WARN("Ignoring section code %"PRIu64".\n", record->operands[5]);

    if (!sm6_parser_get_global_symbol_name(sm6, sm6->value_count))
        WARN("Missing symbol name for global variable at index %zu.\n", sm6->value_count);
    /* TODO: store global symbol names in struct vkd3d_shader_desc? */

    if (record->operand_count > 6 && record->operands[6])
        WARN("Ignoring visibility %"PRIu64".\n", record->operands[6]);
    if (record->operand_count > 7 && record->operands[7])
        WARN("Ignoring thread local mode %"PRIu64".\n", record->operands[7]);
    /* record->operands[8] contains unnamed_addr, a flag indicating the address
     * is not important, only the content is. This info is not relevant. */
    if (record->operand_count > 9 && record->operands[9])
        WARN("Ignoring external_init %"PRIu64".\n", record->operands[9]);
    if (record->operand_count > 10 && record->operands[10])
        WARN("Ignoring dll storage class %"PRIu64".\n", record->operands[10]);
    if (record->operand_count > 11 && record->operands[11])
        WARN("Ignoring comdat %"PRIu64".\n", record->operands[11]);

    dst = sm6_parser_get_current_value(sm6);
    dst->type = type;
    dst->value_type = VALUE_TYPE_REG;

    if (is_constant && !init)
    {
        WARN("Constant array has no initialiser.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A constant global variable has no initialiser.");
        return false;
    }

    if (address_space == ADDRESS_SPACE_DEFAULT)
    {
        sm6_parser_declare_indexable_temp(sm6, scalar_type, count, alignment, init, dst);
    }
    else if (address_space == ADDRESS_SPACE_GROUPSHARED)
    {
        FIXME("Unsupported TGSM.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "TGSM global variables are not supported.");
        return false;
    }
    else
    {
        FIXME("Unhandled address space %"PRIu64".\n", address_space);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Global variables with address space %"PRIu64" are not supported.", address_space);
        return false;
    }

    ++sm6->value_count;
    return true;
}

static const struct vkd3d_shader_immediate_constant_buffer *resolve_forward_initialiser(
        size_t index, struct sm6_parser *sm6)
{
    const struct sm6_value *value;

    assert(index);
    --index;
    if (!(value = sm6_parser_get_value_safe(sm6, index)) || !sm6_value_is_icb(value))
    {
        WARN("Invalid initialiser index %zu.\n", index);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Global variable initialiser value index %zu is invalid.", index);
        return NULL;
    }
    else
    {
        return value->u.icb;
    }
}

static enum vkd3d_result sm6_parser_globals_init(struct sm6_parser *sm6)
{
    const struct dxil_block *block = &sm6->root_block;
    struct vkd3d_shader_instruction *ins;
    const struct dxil_record *record;
    enum vkd3d_result ret;
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
                if (!sm6_parser_declare_global(sm6, record))
                    return VKD3D_ERROR_INVALID_SHADER;
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

    for (i = 0; i < block->child_block_count; ++i)
    {
        if (block->child_blocks[i]->id == CONSTANTS_BLOCK
                && (ret = sm6_parser_constants_init(sm6, block->child_blocks[i])) < 0)
            return ret;
    }

    /* Resolve initialiser forward references. */
    for (i = 0; i < sm6->p.instructions.count; ++i)
    {
        ins = &sm6->p.instructions.elements[i];
        if (ins->handler_idx == VKD3DSIH_DCL_INDEXABLE_TEMP && ins->declaration.indexable_temp.initialiser)
        {
            ins->declaration.indexable_temp.initialiser = resolve_forward_initialiser(
                    (uintptr_t)ins->declaration.indexable_temp.initialiser, sm6);
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
    vsir_register_init(&param->reg, reg_type, vkd3d_data_type_from_component_type(component_type), 0);
    param->reg.dimension = VSIR_DIMENSION_VEC4;
}

static void sm6_parser_init_signature(struct sm6_parser *sm6, const struct shader_signature *s,
        enum vkd3d_shader_register_type reg_type, struct vkd3d_shader_dst_param *params)
{
    struct vkd3d_shader_dst_param *param;
    const struct signature_element *e;
    unsigned int i, count;

    for (i = 0; i < s->element_count; ++i)
    {
        e = &s->elements[i];

        param = &params[i];
        dst_param_io_init(param, e, reg_type);
        count = 0;
        if (e->register_count > 1)
            param->reg.idx[count++].offset = 0;
        param->reg.idx[count++].offset = i;
        param->reg.idx_count = count;
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

        ins->flags = e->interpolation_mode;
        *param = params[i];

        if (e->register_count > 1)
        {
            param->reg.idx[0].rel_addr = NULL;
            param->reg.idx[0].offset = e->register_count;
        }
    }
}

static void sm6_parser_init_output_signature(struct sm6_parser *sm6, const struct shader_signature *output_signature)
{
    sm6_parser_init_signature(sm6, output_signature, VKD3DSPR_OUTPUT, sm6->output_params);
}

static void sm6_parser_init_input_signature(struct sm6_parser *sm6, const struct shader_signature *input_signature)
{
    sm6_parser_init_signature(sm6, input_signature, VKD3DSPR_INPUT, sm6->input_params);
}

static void sm6_parser_emit_output_signature(struct sm6_parser *sm6, const struct shader_signature *output_signature)
{
    sm6_parser_emit_signature(sm6, output_signature, VKD3DSIH_DCL_OUTPUT, VKD3DSIH_DCL_OUTPUT_SIV, sm6->output_params);
}

static void sm6_parser_emit_input_signature(struct sm6_parser *sm6, const struct shader_signature *input_signature)
{
    sm6_parser_emit_signature(sm6, input_signature,
            (sm6->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL) ? VKD3DSIH_DCL_INPUT_PS : VKD3DSIH_DCL_INPUT,
            (sm6->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL) ? VKD3DSIH_DCL_INPUT_PS_SIV : VKD3DSIH_DCL_INPUT_SIV,
            sm6->input_params);
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

static enum vkd3d_shader_opcode map_binary_op(uint64_t code, const struct sm6_type *type_a,
        const struct sm6_type *type_b, struct sm6_parser *sm6)
{
    bool is_int = sm6_type_is_bool_i16_i32_i64(type_a);
    bool is_bool = sm6_type_is_bool(type_a);
    enum vkd3d_shader_opcode op;
    bool is_valid;

    if (!is_int && !sm6_type_is_floating_point(type_a))
    {
        WARN("Argument type %u is not bool, int16/32/64 or floating point.\n", type_a->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "An argument to a binary operation is not bool, int16/32/64 or floating point.");
        return VKD3DSIH_INVALID;
    }
    if (type_a != type_b)
    {
        WARN("Type mismatch, type %u width %u vs type %u width %u.\n", type_a->class,
                type_a->u.width, type_b->class, type_b->u.width);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                "Type mismatch in binary operation arguments.");
    }

    switch (code)
    {
        case BINOP_ADD:
        case BINOP_SUB:
            /* NEG is applied later for subtraction. */
            op = is_int ? VKD3DSIH_IADD : VKD3DSIH_ADD;
            is_valid = !is_bool;
            break;
        case BINOP_AND:
            op = VKD3DSIH_AND;
            is_valid = is_int;
            break;
        case BINOP_ASHR:
            op = VKD3DSIH_ISHR;
            is_valid = is_int && !is_bool;
            break;
        case BINOP_LSHR:
            op = VKD3DSIH_USHR;
            is_valid = is_int && !is_bool;
            break;
        case BINOP_MUL:
            op = is_int ? VKD3DSIH_UMUL : VKD3DSIH_MUL;
            is_valid = !is_bool;
            break;
        case BINOP_OR:
            op = VKD3DSIH_OR;
            is_valid = is_int;
            break;
        case BINOP_SDIV:
            op = is_int ? VKD3DSIH_IDIV : VKD3DSIH_DIV;
            is_valid = !is_bool;
            break;
        case BINOP_SREM:
            op = is_int ? VKD3DSIH_IDIV : VKD3DSIH_FREM;
            is_valid = !is_bool;
            break;
        case BINOP_SHL:
            op = VKD3DSIH_ISHL;
            is_valid = is_int && !is_bool;
            break;
        case BINOP_UDIV:
        case BINOP_UREM:
            op = VKD3DSIH_UDIV;
            is_valid = is_int && !is_bool;
            break;
        case BINOP_XOR:
            op = VKD3DSIH_XOR;
            is_valid = is_int;
            break;
        default:
            FIXME("Unhandled binary op %#"PRIx64".\n", code);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Binary operation %#"PRIx64" is unhandled.", code);
            return VKD3DSIH_INVALID;
    }

    if (!is_valid)
    {
        WARN("Invalid operation %u for type %u, width %u.\n", op, type_a->class, type_a->u.width);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_INVALID_OPERATION,
                "Binary operation %u is invalid on type class %u, width %u.", op, type_a->class, type_a->u.width);
    }

    return op;
}

static void sm6_parser_emit_binop(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    struct vkd3d_shader_src_param *src_params;
    enum vkd3d_shader_opcode handler_idx;
    const struct sm6_value *a, *b;
    uint64_t code, flags;
    bool silence_warning;
    unsigned int i = 0;

    a = sm6_parser_get_value_by_ref(sm6, record, NULL, &i);
    b = sm6_parser_get_value_by_ref(sm6, record, a->type, &i);
    if (!a || !b)
        return;

    if (!dxil_record_validate_operand_count(record, i + 1, i + 2, sm6))
        return;

    code = record->operands[i++];
    if ((handler_idx = map_binary_op(code, a->type, b->type, sm6)) == VKD3DSIH_INVALID)
        return;

    vsir_instruction_init(ins, &sm6->p.location, handler_idx);

    flags = (record->operand_count > i) ? record->operands[i] : 0;
    silence_warning = false;

    switch (handler_idx)
    {
        case VKD3DSIH_ADD:
        case VKD3DSIH_MUL:
        case VKD3DSIH_DIV:
        case VKD3DSIH_FREM:
            if (!(flags & FP_ALLOW_UNSAFE_ALGEBRA))
                ins->flags |= VKD3DSI_PRECISE_X;
            flags &= ~FP_ALLOW_UNSAFE_ALGEBRA;
            /* SPIR-V FPFastMathMode is only available in the Kernel executon model. */
            silence_warning = !(flags & ~(FP_NO_NAN | FP_NO_INF | FP_NO_SIGNED_ZEROS | FP_ALLOW_RECIPROCAL));
            break;
        case VKD3DSIH_IADD:
        case VKD3DSIH_UMUL:
        case VKD3DSIH_ISHL:
            silence_warning = !(flags & ~(OB_NO_UNSIGNED_WRAP | OB_NO_SIGNED_WRAP));
            break;
        case VKD3DSIH_ISHR:
        case VKD3DSIH_USHR:
        case VKD3DSIH_IDIV:
        case VKD3DSIH_UDIV:
            silence_warning = !(flags & ~PEB_EXACT);
            break;
        default:
            break;
    }
    /* The above flags are very common and cause warning spam. */
    if (flags && silence_warning)
    {
        TRACE("Ignoring flags %#"PRIx64".\n", flags);
    }
    else if (flags)
    {
        WARN("Ignoring flags %#"PRIx64".\n", flags);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Ignoring flags %#"PRIx64" for a binary operation.", flags);
    }

    src_params = instruction_src_params_alloc(ins, 2, sm6);
    src_param_init_from_value(&src_params[0], a);
    src_param_init_from_value(&src_params[1], b);
    if (code == BINOP_SUB)
        src_params[1].modifiers = VKD3DSPSM_NEG;

    dst->type = a->type;

    if (handler_idx == VKD3DSIH_UMUL || handler_idx == VKD3DSIH_UDIV || handler_idx == VKD3DSIH_IDIV)
    {
        struct vkd3d_shader_dst_param *dst_params = instruction_dst_params_alloc(ins, 2, sm6);
        unsigned int index = code != BINOP_UDIV && code != BINOP_SDIV;

        dst_param_init(&dst_params[0]);
        dst_param_init(&dst_params[1]);
        register_init_ssa_scalar(&dst_params[index].reg, a->type, sm6);
        vsir_register_init(&dst_params[index ^ 1].reg, VKD3DSPR_NULL, VKD3D_DATA_UNUSED, 0);
        dst->u.reg = dst_params[index].reg;
    }
    else
    {
        instruction_dst_param_init_ssa_scalar(ins, sm6);
    }
}

static void sm6_parser_emit_dx_cbuffer_load(struct sm6_parser *sm6, struct sm6_block *code_block,
        enum dx_intrinsic_opcode op, const struct sm6_value **operands, struct vkd3d_shader_instruction *ins)
{
    struct sm6_value *dst = sm6_parser_get_current_value(sm6);
    struct vkd3d_shader_src_param *src_param;
    const struct sm6_value *buffer;
    const struct sm6_type *type;

    buffer = operands[0];
    if (!sm6_value_validate_is_handle(buffer, sm6))
        return;

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOV);

    src_param = instruction_src_params_alloc(ins, 1, sm6);
    src_param_init_vector_from_reg(src_param, &buffer->u.handle.reg);
    register_index_address_init(&src_param->reg.idx[2], operands[1], sm6);
    assert(src_param->reg.idx_count == 3);

    type = sm6_type_get_scalar_type(dst->type, 0);
    assert(type);
    src_param->reg.data_type = vkd3d_data_type_from_sm6_type(type);

    instruction_dst_param_init_ssa_vector(ins, sm6_type_max_vector_size(type), sm6);
}

static const struct sm6_descriptor_info *sm6_parser_get_descriptor(struct sm6_parser *sm6,
        enum vkd3d_shader_descriptor_type type, unsigned int id, const struct sm6_value *address)
{
    const struct sm6_descriptor_info *d;
    unsigned int register_index;
    size_t i;

    for (i = 0; i < sm6->descriptor_count; ++i)
    {
        d = &sm6->descriptors[i];

        if (d->type != type || d->id != id)
            continue;

        if (!sm6_value_is_constant(address))
            return d;

        register_index = sm6_value_get_constant_uint(address);
        if (register_index >= d->range.first && register_index <= d->range.last)
            return d;
    }

    return NULL;
}

static void sm6_parser_emit_dx_create_handle(struct sm6_parser *sm6, struct sm6_block *code_block,
        enum dx_intrinsic_opcode op, const struct sm6_value **operands, struct vkd3d_shader_instruction *ins)
{
    enum vkd3d_shader_descriptor_type type;
    const struct sm6_descriptor_info *d;
    struct vkd3d_shader_register *reg;
    struct sm6_value *dst;
    unsigned int id;

    type = sm6_value_get_constant_uint(operands[0]);
    id = sm6_value_get_constant_uint(operands[1]);
    if (!(d = sm6_parser_get_descriptor(sm6, type, id, operands[2])))
    {
        WARN("Failed to find resource type %#x, id %#x.\n", type, id);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Descriptor for resource type %#x, id %#x was not found.", type, id);
        return;
    }

    dst = sm6_parser_get_current_value(sm6);
    dst->value_type = VALUE_TYPE_HANDLE;
    dst->u.handle.d = d;

    reg = &dst->u.handle.reg;
    /* Set idx_count to 3 for use with load instructions.
     * TODO: set register type from resource type when other types are supported. */
    vsir_register_init(reg, VKD3DSPR_CONSTBUFFER, VKD3D_DATA_FLOAT, 3);
    reg->idx[0].offset = id;
    register_index_address_init(&reg->idx[1], operands[2], sm6);
    reg->non_uniform = !!sm6_value_get_constant_uint(operands[3]);

    /* NOP is used to flag no instruction emitted. */
    ins->handler_idx = VKD3DSIH_NOP;
}

static void sm6_parser_emit_dx_load_input(struct sm6_parser *sm6, struct sm6_block *code_block,
        enum dx_intrinsic_opcode op, const struct sm6_value **operands, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_src_param *src_param;
    const struct shader_signature *signature;
    unsigned int row_index, column_index;
    const struct signature_element *e;

    row_index = sm6_value_get_constant_uint(operands[0]);
    column_index = sm6_value_get_constant_uint(operands[2]);

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOV);

    signature = &sm6->p.shader_desc.input_signature;
    if (row_index >= signature->element_count)
    {
        WARN("Invalid row index %u.\n", row_index);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Invalid input row index %u.", row_index);
        return;
    }
    e = &signature->elements[row_index];

    src_param = instruction_src_params_alloc(ins, 1, sm6);
    src_param->reg = sm6->input_params[row_index].reg;
    src_param_init_scalar(src_param, column_index);
    if (e->register_count > 1)
        register_index_address_init(&src_param->reg.idx[0], operands[1], sm6);

    instruction_dst_param_init_ssa_scalar(ins, sm6);
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

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOV);

    if (!(dst_param = instruction_dst_params_alloc(ins, 1, sm6)))
        return;
    dst_param_init_scalar(dst_param, column_index);
    dst_param->reg = sm6->output_params[row_index].reg;
    if (e->register_count > 1)
        register_index_address_init(&dst_param->reg.idx[0], operands[1], sm6);

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
    b -> constant int1
    c -> constant int8/16/32
    i -> int32
    H -> handle
    v -> void
    o -> overloaded
 */
static const struct sm6_dx_opcode_info sm6_dx_op_table[] =
{
    [DX_CBUFFER_LOAD_LEGACY           ] = {'o', "Hi",   sm6_parser_emit_dx_cbuffer_load},
    [DX_CREATE_HANDLE                 ] = {'H', "ccib", sm6_parser_emit_dx_create_handle},
    [DX_LOAD_INPUT                    ] = {'o', "ii8i", sm6_parser_emit_dx_load_input},
    [DX_STORE_OUTPUT                  ] = {'v', "ii8o", sm6_parser_emit_dx_store_output},
};

static bool sm6_parser_validate_operand_type(struct sm6_parser *sm6, const struct sm6_value *value, char info_type,
        bool is_return)
{
    const struct sm6_type *type = value->type;

    if (info_type != 'H' && !sm6_value_is_register(value))
        return false;

    switch (info_type)
    {
        case 0:
            FIXME("Invalid operand count.\n");
            return false;
        case '8':
            return sm6_type_is_i8(type);
        case 'b':
            return sm6_value_is_constant(value) && sm6_type_is_bool(type);
        case 'c':
            return sm6_value_is_constant(value) && sm6_type_is_integer(type) && type->u.width >= 8
                    && type->u.width <= 32;
        case 'i':
            return sm6_type_is_i32(type);
        case 'H':
            return (is_return || sm6_value_is_handle(value)) && type == sm6->handle_type;
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

    if (!sm6_parser_validate_operand_type(sm6, dst, info->ret_type, true))
    {
        WARN("Failed to validate return type for dx intrinsic id %u, '%s'.\n", op, name);
        /* Return type validation failure is not so critical. We only need to set
         * a data type for the SSA result. */
    }

    for (i = 0; i < operand_count; ++i)
    {
        const struct sm6_value *value = operands[i];
        if (!sm6_parser_validate_operand_type(sm6, value, info->operand_info[i], false))
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
    vsir_register_init(&dst->u.reg, VKD3DSPR_UNDEF, vkd3d_data_type_from_sm6_type(type), 0);
    /* dst->is_undefined is not set here because it flags only explicitly undefined values. */
}

static void sm6_parser_decode_dx_op(struct sm6_parser *sm6, struct sm6_block *code_block, enum dx_intrinsic_opcode op,
        const char *name, const struct sm6_value **operands, unsigned int operand_count,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    if (op >= ARRAY_SIZE(sm6_dx_op_table) || !sm6_dx_op_table[op].operand_info)
    {
        FIXME("Unhandled dx intrinsic function id %u, '%s'.\n", op, name);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_UNHANDLED_INTRINSIC,
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

static enum vkd3d_shader_opcode sm6_map_cast_op(uint64_t code, const struct sm6_type *from,
        const struct sm6_type *to, struct sm6_parser *sm6)
{
    enum vkd3d_shader_opcode op = VKD3DSIH_INVALID;
    bool from_int, to_int, from_fp, to_fp;
    bool is_valid = false;

    from_int = sm6_type_is_integer(from);
    to_int = sm6_type_is_integer(to);
    from_fp = sm6_type_is_floating_point(from);
    to_fp = sm6_type_is_floating_point(to);

    /* NOTE: DXIL currently doesn't use vectors here. */
    if ((!from_int && !from_fp) || (!to_int && !to_fp))
    {
        FIXME("Unhandled cast of type class %u to type class %u.\n", from->class, to->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Cast of type class %u to type class %u is not implemented.", from->class, to->class);
        return VKD3DSIH_INVALID;
    }
    if (to->u.width == 8 || from->u.width == 8)
    {
        FIXME("Unhandled 8-bit value.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Cast to/from an 8-bit type is not implemented.");
        return VKD3DSIH_INVALID;
    }

    /* DXC emits minimum precision types as 16-bit. These must be emitted
     * as 32-bit in VSIR, so all width extensions to 32 bits are no-ops. */
    switch (code)
    {
        case CAST_TRUNC:
            /* nop or min precision. TODO: native 16-bit */
            if (to->u.width == from->u.width || (to->u.width == 16 && from->u.width == 32))
                op = VKD3DSIH_NOP;
            else
                op = VKD3DSIH_UTOU;
            is_valid = from_int && to_int && to->u.width <= from->u.width;
            break;
        case CAST_ZEXT:
        case CAST_SEXT:
            /* nop or min precision. TODO: native 16-bit */
            if (to->u.width == from->u.width || (to->u.width == 32 && from->u.width == 16))
            {
                op = VKD3DSIH_NOP;
                is_valid = from_int && to_int;
            }
            else if (to->u.width > from->u.width)
            {
                op = (code == CAST_ZEXT) ? VKD3DSIH_UTOU : VKD3DSIH_ITOI;
                assert(from->u.width == 1 || to->u.width == 64);
                is_valid = from_int && to_int;
            }
            break;
        case CAST_FPTOUI:
            op = VKD3DSIH_FTOU;
            is_valid = from_fp && to_int && to->u.width > 1;
            break;
        case CAST_FPTOSI:
            op = VKD3DSIH_FTOI;
            is_valid = from_fp && to_int && to->u.width > 1;
            break;
        case CAST_UITOFP:
            op = VKD3DSIH_UTOF;
            is_valid = from_int && to_fp;
            break;
        case CAST_SITOFP:
            op = VKD3DSIH_ITOF;
            is_valid = from_int && to_fp;
            break;
        case CAST_FPTRUNC:
            /* TODO: native 16-bit */
            op = (from->u.width == 64) ? VKD3DSIH_DTOF : VKD3DSIH_NOP;
            is_valid = from_fp && to_fp;
            break;
        case CAST_FPEXT:
            /* TODO: native 16-bit */
            op = (to->u.width == 64) ? VKD3DSIH_FTOD : VKD3DSIH_NOP;
            is_valid = from_fp && to_fp;
            break;
        case CAST_BITCAST:
            op = VKD3DSIH_MOV;
            is_valid = to->u.width == from->u.width;
            break;
        default:
            FIXME("Unhandled cast op %"PRIu64".\n", code);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                    "Cast operation %"PRIu64" is unhandled.\n", code);
            return VKD3DSIH_INVALID;
    }

    if (!is_valid)
    {
        FIXME("Invalid types %u and/or %u for op %"PRIu64".\n", from->class, to->class, code);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Cast operation %"PRIu64" from type class %u, width %u to type class %u, width %u is invalid.\n",
                code, from->class, from->u.width, to->class, to->u.width);
        return VKD3DSIH_INVALID;
    }

    return op;
}

static void sm6_parser_emit_cast(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    struct vkd3d_shader_src_param *src_param;
    enum vkd3d_shader_opcode handler_idx;
    const struct sm6_value *value;
    const struct sm6_type *type;
    unsigned int i = 0;

    if (!(value = sm6_parser_get_value_by_ref(sm6, record, NULL, &i)))
        return;

    if (!dxil_record_validate_operand_count(record, i + 2, i + 2, sm6))
        return;

    if (!(type = sm6_parser_get_type(sm6, record->operands[i++])))
        return;

    dst->type = type;

    if (sm6_type_is_pointer(type))
    {
        *dst = *value;
        dst->type = type;
        ins->handler_idx = VKD3DSIH_NOP;
        return;
    }

    if ((handler_idx = sm6_map_cast_op(record->operands[i], value->type, type, sm6)) == VKD3DSIH_INVALID)
        return;

    vsir_instruction_init(ins, &sm6->p.location, handler_idx);

    if (handler_idx == VKD3DSIH_NOP)
    {
        dst->u.reg = value->u.reg;
        return;
    }

    src_param = instruction_src_params_alloc(ins, 1, sm6);
    src_param_init_from_value(src_param, value);

    instruction_dst_param_init_ssa_scalar(ins, sm6);

    /* bitcast */
    if (handler_idx == VKD3DSIH_MOV)
        src_param->reg.data_type = dst->u.reg.data_type;
}

struct sm6_cmp_info
{
    enum vkd3d_shader_opcode handler_idx;
    bool src_swap;
};

static const struct sm6_cmp_info *sm6_map_cmp2_op(uint64_t code)
{
    static const struct sm6_cmp_info cmp_op_table[] =
    {
        [FCMP_FALSE] = {VKD3DSIH_INVALID},
        [FCMP_OEQ]   = {VKD3DSIH_EQO},
        [FCMP_OGT]   = {VKD3DSIH_LTO, true},
        [FCMP_OGE]   = {VKD3DSIH_GEO},
        [FCMP_OLT]   = {VKD3DSIH_LTO},
        [FCMP_OLE]   = {VKD3DSIH_GEO, true},
        [FCMP_ONE]   = {VKD3DSIH_NEO},
        [FCMP_ORD]   = {VKD3DSIH_INVALID},
        [FCMP_UNO]   = {VKD3DSIH_INVALID},
        [FCMP_UEQ]   = {VKD3DSIH_EQU},
        [FCMP_UGT]   = {VKD3DSIH_LTU, true},
        [FCMP_UGE]   = {VKD3DSIH_GEU},
        [FCMP_ULT]   = {VKD3DSIH_LTU},
        [FCMP_ULE]   = {VKD3DSIH_GEU, true},
        [FCMP_UNE]   = {VKD3DSIH_NEU},
        [FCMP_TRUE]  = {VKD3DSIH_INVALID},

        [ICMP_EQ]    = {VKD3DSIH_IEQ},
        [ICMP_NE]    = {VKD3DSIH_INE},
        [ICMP_UGT]   = {VKD3DSIH_ULT, true},
        [ICMP_UGE]   = {VKD3DSIH_UGE},
        [ICMP_ULT]   = {VKD3DSIH_ULT},
        [ICMP_ULE]   = {VKD3DSIH_UGE, true},
        [ICMP_SGT]   = {VKD3DSIH_ILT, true},
        [ICMP_SGE]   = {VKD3DSIH_IGE},
        [ICMP_SLT]   = {VKD3DSIH_ILT},
        [ICMP_SLE]   = {VKD3DSIH_IGE, true},
    };

    return (code < ARRAY_SIZE(cmp_op_table)) ? &cmp_op_table[code] : NULL;
}

static void sm6_parser_emit_cmp2(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    struct vkd3d_shader_src_param *src_params;
    const struct sm6_type *type_a, *type_b;
    bool is_int, is_fp, silence_warning;
    const struct sm6_cmp_info *cmp;
    const struct sm6_value *a, *b;
    uint64_t code, flags;
    unsigned int i = 0;

    if (!(dst->type = sm6->bool_type))
    {
        WARN("Bool type not found.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_MODULE,
                "Module does not define a boolean type for comparison results.");
        return;
    }

    a = sm6_parser_get_value_by_ref(sm6, record, NULL, &i);
    b = sm6_parser_get_value_by_ref(sm6, record, a->type, &i);
    if (!a || !b)
        return;

    if (!dxil_record_validate_operand_count(record, i + 1, i + 2, sm6))
        return;

    type_a = a->type;
    type_b = b->type;
    is_int = sm6_type_is_bool_i16_i32_i64(type_a);
    is_fp = sm6_type_is_floating_point(type_a);

    code = record->operands[i++];

    if ((!is_int && !is_fp) || is_int != (code >= ICMP_EQ))
    {
        FIXME("Invalid operation %"PRIu64" on type class %u.\n", code, type_a->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Comparison operation %"PRIu64" on type class %u is invalid.", code, type_a->class);
        return;
    }

    if (type_a != type_b)
    {
        WARN("Type mismatch, type %u width %u vs type %u width %u.\n", type_a->class,
                type_a->u.width, type_b->class, type_b->u.width);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                "Type mismatch in comparison operation arguments.");
    }

    if (!(cmp = sm6_map_cmp2_op(code)) || !cmp->handler_idx || cmp->handler_idx == VKD3DSIH_INVALID)
    {
        FIXME("Unhandled operation %"PRIu64".\n", code);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Comparison operation %"PRIu64" is unhandled.", code);
        return;
    }

    vsir_instruction_init(ins, &sm6->p.location, cmp->handler_idx);

    flags = (record->operand_count > i) ? record->operands[i] : 0;
    silence_warning = false;

    if (is_fp)
    {
        if (!(flags & FP_ALLOW_UNSAFE_ALGEBRA))
            ins->flags |= VKD3DSI_PRECISE_X;
        flags &= ~FP_ALLOW_UNSAFE_ALGEBRA;
        /* SPIR-V FPFastMathMode is only available in the Kernel execution model. */
        silence_warning = !(flags & ~(FP_NO_NAN | FP_NO_INF | FP_NO_SIGNED_ZEROS | FP_ALLOW_RECIPROCAL));
    }
    if (flags && silence_warning)
    {
        TRACE("Ignoring fast FP modifier %#"PRIx64".\n", flags);
    }
    else if (flags)
    {
        WARN("Ignoring flags %#"PRIx64".\n", flags);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Ignoring flags %#"PRIx64" for a comparison operation.", flags);
    }

    src_params = instruction_src_params_alloc(ins, 2, sm6);
    src_param_init_from_value(&src_params[0 ^ cmp->src_swap], a);
    src_param_init_from_value(&src_params[1 ^ cmp->src_swap], b);

    instruction_dst_param_init_ssa_scalar(ins, sm6);
}

static void sm6_parser_emit_extractval(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    struct vkd3d_shader_src_param *src_param;
    const struct sm6_type *type;
    const struct sm6_value *src;
    unsigned int i = 0;
    uint64_t elem_idx;

    if (!(src = sm6_parser_get_value_by_ref(sm6, record, NULL, &i)))
        return;

    if (!dxil_record_validate_operand_min_count(record, i + 1, sm6))
        return;

    if (record->operand_count > i + 1)
    {
        FIXME("Unhandled multiple indices.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Multiple extractval indices are not supported.");
        return;
    }

    type = src->type;
    if (!sm6_type_is_aggregate(type))
    {
        WARN("Invalid extraction from non-aggregate.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Source type of an extractval instruction is not an aggregate.");
        return;
    }

    elem_idx = record->operands[i];
    if (!(type = sm6_type_get_element_type_at_index(type, elem_idx)))
    {
        WARN("Invalid element index %"PRIu64".\n", elem_idx);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Element index %"PRIu64" for an extractval instruction is out of bounds.", elem_idx);
        return;
    }
    if (!sm6_type_is_scalar(type))
    {
        FIXME("Nested extraction is not supported.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Extraction from nested aggregates is not supported.");
        return;
    }
    dst->type = type;

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOV);

    src_param = instruction_src_params_alloc(ins, 1, sm6);
    src_param_init_from_value(src_param, src);
    src_param->swizzle = vkd3d_shader_create_swizzle(elem_idx, elem_idx, elem_idx, elem_idx);

    instruction_dst_param_init_ssa_scalar(ins, sm6);
}

static void sm6_parser_emit_gep(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    const struct sm6_type *type, *pointee_type;
    unsigned int elem_idx, operand_idx = 2;
    enum bitcode_address_space addr_space;
    const struct sm6_value *elem_value;
    struct vkd3d_shader_register *reg;
    const struct sm6_value *src;
    bool is_in_bounds;

    if (!dxil_record_validate_operand_min_count(record, 5, sm6)
            || !(type = sm6_parser_get_type(sm6, record->operands[1]))
            || !(src = sm6_parser_get_value_by_ref(sm6, record, NULL, &operand_idx))
            || !sm6_value_validate_is_register(src, sm6)
            || !sm6_value_validate_is_pointer(src, sm6)
            || !dxil_record_validate_operand_min_count(record, operand_idx + 2, sm6))
    {
        return;
    }

    if (src->u.reg.idx_count > 1)
    {
        WARN("Unsupported stacked GEP.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "A GEP instruction on the result of a previous GEP is unsupported.");
        return;
    }

    is_in_bounds = record->operands[0];

    if ((pointee_type = src->type->u.pointer.type) != type)
    {
        WARN("Type mismatch, type %u width %u vs type %u width %u.\n", type->class,
                type->u.width, pointee_type->class, pointee_type->u.width);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                "Type mismatch in GEP operation arguments.");
    }
    addr_space = src->type->u.pointer.addr_space;

    if (!(elem_value = sm6_parser_get_value_by_ref(sm6, record, NULL, &operand_idx)))
        return;

    /* The first index is always zero, to form a simple pointer dereference. */
    if (sm6_value_get_constant_uint(elem_value))
    {
        WARN("Expected constant zero.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "The pointer dereference index for a GEP instruction is not constant zero.");
        return;
    }

    if (!sm6_type_is_array(pointee_type))
    {
        WARN("Invalid GEP on type class %u.\n", pointee_type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Source type for index 1 of a GEP instruction is not an array.");
        return;
    }

    if (!(elem_value = sm6_parser_get_value_by_ref(sm6, record, NULL, &operand_idx)))
        return;

    /* If indexing is dynamic, just get the type at offset zero. */
    elem_idx = sm6_value_is_constant(elem_value) ? sm6_value_get_constant_uint(elem_value) : 0;
    type = sm6_type_get_element_type_at_index(pointee_type, elem_idx);
    if (!type)
    {
        WARN("Invalid element index %u.\n", elem_idx);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Element index %u for a GEP instruction is out of bounds.", elem_idx);
        return;
    }

    if (operand_idx < record->operand_count)
    {
        FIXME("Multiple element indices are not implemented.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND,
                "Multi-dimensional addressing in GEP instructions is not supported.");
        return;
    }

    if (!(dst->type = sm6_type_get_pointer_to_type(type, addr_space, sm6)))
    {
        WARN("Failed to get pointer type for type %u.\n", type->class);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_MODULE,
                "Module does not define a pointer type for a GEP instruction.");
        return;
    }

    reg = &dst->u.reg;
    *reg = src->u.reg;
    reg->idx[1].offset = 0;
    register_index_address_init(&reg->idx[1], elem_value, sm6);
    reg->idx[1].is_in_bounds = is_in_bounds;
    reg->idx_count = 2;

    ins->handler_idx = VKD3DSIH_NOP;
}

static void sm6_parser_emit_load(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    const struct sm6_type *elem_type = NULL, *pointee_type;
    struct vkd3d_shader_src_param *src_param;
    unsigned int alignment, i = 0;
    const struct sm6_value *ptr;
    uint64_t alignment_code;

    if (!(ptr = sm6_parser_get_value_by_ref(sm6, record, NULL, &i)))
        return;
    if (!sm6_value_validate_is_register(ptr, sm6)
            || !sm6_value_validate_is_pointer(ptr, sm6)
            || !dxil_record_validate_operand_count(record, i + 2, i + 3, sm6))
        return;

    if (record->operand_count > i + 2 && !(elem_type = sm6_parser_get_type(sm6, record->operands[i++])))
        return;

    if (!elem_type)
    {
        elem_type = ptr->type->u.pointer.type;
    }
    else if (elem_type != (pointee_type = ptr->type->u.pointer.type))
    {
        WARN("Type mismatch.\n");
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                "Type mismatch in pointer load arguments.");
    }

    dst->type = elem_type;

    if (!sm6_value_validate_is_numeric(dst, sm6))
        return;

    alignment_code = record->operands[i++];
    if (!bitcode_parse_alignment(alignment_code, &alignment))
        WARN("Invalid alignment %"PRIu64".\n", alignment_code);

    if (record->operands[i])
        WARN("Ignoring volatile modifier.\n");

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOV);

    src_param = instruction_src_params_alloc(ins, 1, sm6);
    src_param_init_from_value(&src_param[0], ptr);
    src_param->reg.alignment = alignment;

    instruction_dst_param_init_ssa_scalar(ins, sm6);
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

static void sm6_parser_emit_vselect(struct sm6_parser *sm6, const struct dxil_record *record,
        struct vkd3d_shader_instruction *ins, struct sm6_value *dst)
{
    struct vkd3d_shader_src_param *src_params;
    const struct sm6_value *src[3];
    unsigned int i = 0;

    if (!(src[1] = sm6_parser_get_value_by_ref(sm6, record, NULL, &i))
            || !(src[2] = sm6_parser_get_value_by_ref(sm6, record, src[1]->type, &i))
            || !(src[0] = sm6_parser_get_value_by_ref(sm6, record, NULL, &i)))
    {
        return;
    }
    dxil_record_validate_operand_max_count(record, i, sm6);

    for (i = 0; i < 3; ++i)
    {
        if (!sm6_value_validate_is_register(src[i], sm6))
            return;
    }

    dst->type = src[1]->type;

    if (!sm6_value_validate_is_bool(src[0], sm6))
        return;

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_MOVC);

    src_params = instruction_src_params_alloc(ins, 3, sm6);
    for (i = 0; i < 3; ++i)
        src_param_init_from_value(&src_params[i], src[i]);

    instruction_dst_param_init_ssa_scalar(ins, sm6);
}

static bool sm6_metadata_value_is_node(const struct sm6_metadata_value *m)
{
    return m && m->type == VKD3D_METADATA_NODE;
}

static bool sm6_metadata_value_is_value(const struct sm6_metadata_value *m)
{
    return m && m->type == VKD3D_METADATA_VALUE;
}

static bool sm6_metadata_value_is_string(const struct sm6_metadata_value *m)
{
    return m && m->type == VKD3D_METADATA_STRING;
}

static bool sm6_metadata_get_uint_value(const struct sm6_parser *sm6,
        const struct sm6_metadata_value *m, unsigned int *u)
{
    const struct sm6_value *value;

    if (!m || m->type != VKD3D_METADATA_VALUE)
        return false;

    value = m->u.value;
    if (!sm6_value_is_constant(value))
        return false;
    if (!sm6_type_is_integer(value->type))
        return false;

    *u = register_get_uint_value(&value->u.reg);

    return true;
}

static bool sm6_metadata_get_uint64_value(const struct sm6_parser *sm6,
        const struct sm6_metadata_value *m, uint64_t *u)
{
    const struct sm6_value *value;

    if (!m || m->type != VKD3D_METADATA_VALUE)
        return false;

    value = m->u.value;
    if (!sm6_value_is_constant(value))
        return false;
    if (!sm6_type_is_integer(value->type))
        return false;

    *u = register_get_uint64_value(&value->u.reg);

    return true;
}

static enum vkd3d_result sm6_parser_function_init(struct sm6_parser *sm6, const struct dxil_block *block,
        struct sm6_function *function)
{
    struct vkd3d_shader_instruction *ins;
    size_t i, block_idx, block_count;
    const struct dxil_record *record;
    bool ret_found, is_terminator;
    struct sm6_block *code_block;
    struct sm6_value *dst;

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

    if (!(block_count = block->records[0]->operands[0]))
    {
        WARN("Function contains no blocks.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (block_count > 1)
    {
        FIXME("Branched shaders are not supported yet.\n");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!(function->blocks[0] = sm6_block_create()))
    {
        ERR("Failed to allocate code block.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    function->block_count = block_count;
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
            case FUNC_CODE_INST_BINOP:
                sm6_parser_emit_binop(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_CALL:
                sm6_parser_emit_call(sm6, record, code_block, ins, dst);
                break;
            case FUNC_CODE_INST_CAST:
                sm6_parser_emit_cast(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_CMP2:
                sm6_parser_emit_cmp2(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_EXTRACTVAL:
                sm6_parser_emit_extractval(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_GEP:
                sm6_parser_emit_gep(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_LOAD:
                sm6_parser_emit_load(sm6, record, ins, dst);
                break;
            case FUNC_CODE_INST_RET:
                sm6_parser_emit_ret(sm6, record, code_block, ins);
                is_terminator = true;
                ret_found = true;
                break;
            case FUNC_CODE_INST_VSELECT:
                sm6_parser_emit_vselect(sm6, record, ins, dst);
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
            /* Level 1 (global) constants are already done in sm6_parser_globals_init(). */
            if (level < 2)
                break;
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

static bool sm6_parser_allocate_named_metadata(struct sm6_parser *sm6)
{
    struct dxil_block *block;
    unsigned int i, j, count;

    for (i = 0, count = 0; i < sm6->root_block.child_block_count; ++i)
    {
        block = sm6->root_block.child_blocks[i];
        if (block->id != METADATA_BLOCK)
            continue;
        for (j = 0; j < block->record_count; ++j)
            count += block->records[j]->code == METADATA_NAMED_NODE;
    }

    if (!count)
        return true;

    return !!(sm6->named_metadata = vkd3d_calloc(count, sizeof(*sm6->named_metadata)));
}

static enum vkd3d_result metadata_value_create_node(struct sm6_metadata_value *m, struct sm6_metadata_table *table,
        unsigned int dst_idx, unsigned int end_count, const struct dxil_record *record, struct sm6_parser *sm6)
{
    struct sm6_metadata_node *node;
    unsigned int i, offset;

    m->type = VKD3D_METADATA_NODE;
    if (!(m->value_type = sm6->metadata_type))
    {
        WARN("Metadata type not found.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                "The type for metadata values was not found.");
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (!(node = vkd3d_malloc(offsetof(struct sm6_metadata_node, operands[record->operand_count]))))
    {
        ERR("Failed to allocate metadata node with %u operands.\n", record->operand_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating a metadata node with %u operands.", record->operand_count);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    m->u.node = node;

    node->is_distinct = record->code == METADATA_DISTINCT_NODE;

    offset = record->code != METADATA_NAMED_NODE;

    for (i = 0; i < record->operand_count; ++i)
    {
        uint64_t ref;

        ref = record->operands[i] - offset;
        if (record->operands[i] >= offset && ref >= end_count)
        {
            WARN("Invalid metadata index %"PRIu64".\n", ref);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                    "Metadata index %"PRIu64" is invalid.", ref);
            vkd3d_free(node);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if (!node->is_distinct && ref == dst_idx)
        {
            WARN("Metadata self-reference at index %u.\n", dst_idx);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                    "Metadata index %u is self-referencing.", dst_idx);
            vkd3d_free(node);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        node->operands[i] = (record->operands[i] >= offset) ? &table->values[ref] : NULL;
        if (record->code == METADATA_NAMED_NODE && !sm6_metadata_value_is_node(node->operands[i]))
        {
            WARN("Named node operand is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                    "The operand of a metadata named node is not a node.");
            vkd3d_free(node);
            return VKD3D_ERROR_INVALID_SHADER;
        }
    }

    node->operand_count = record->operand_count;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_metadata_init(struct sm6_parser *sm6, const struct dxil_block *block,
        struct sm6_metadata_table *table)
{
    unsigned int i, count, table_idx, value_idx;
    struct sm6_metadata_value *values, *m;
    const struct dxil_record *record;
    const struct sm6_value *value;
    enum vkd3d_result ret;
    char *name;

    for (i = 0, count = 0; i < block->record_count; ++i)
        count += block->records[i]->code != METADATA_NAME;

    if (!(values = vkd3d_calloc(count, sizeof(*values))))
    {
        ERR("Failed to allocate metadata tables.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating metadata tables.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    table->values = values;

    for (i = 0, name = NULL; i < block->record_count; ++i)
    {
        record = block->records[i];

        table_idx = table->count;
        m = &values[table_idx];

        switch (record->code)
        {
            case METADATA_NAMED_NODE:
                if (!name)
                {
                    WARN("Named node has no name.\n");
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                            "A metadata named node has no name.");
                    return VKD3D_ERROR_INVALID_SHADER;
                }

                /* When DXC emits metadata value array reference indices it assumes named nodes
                 * are not included in the array. Store named nodes separately. */
                m = &sm6->named_metadata[sm6->named_metadata_count].value;
                sm6->named_metadata[sm6->named_metadata_count].name = name;
                name = NULL;

                if ((ret = metadata_value_create_node(m, table, UINT_MAX, count, record, sm6)) < 0)
                    return ret;
                ++sm6->named_metadata_count;
                /* Skip incrementing the table count. */
                continue;

            case METADATA_DISTINCT_NODE:
            case METADATA_NODE:
                if ((ret = metadata_value_create_node(m, table, table_idx, count, record, sm6)) < 0)
                    return ret;
                break;

            case METADATA_KIND:
                if (!dxil_record_validate_operand_min_count(record, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                m->type = VKD3D_METADATA_KIND;
                m->u.kind.id = record->operands[0];
                if (!(m->u.kind.name = dxil_record_to_string(record, 1, sm6)))
                {
                    ERR("Failed to allocate name of a kind.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }
                break;

            case METADATA_NAME:
                /* Check the next record to avoid freeing 'name' in all exit paths. */
                if (i + 1 == block->record_count || block->records[i + 1]->code != METADATA_NAMED_NODE)
                {
                    WARN("Name is not followed by a named node.\n");
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                            "A metadata node name is not followed by a named node.");
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                /* LLVM allows an empty string here. */
                if (!(name = dxil_record_to_string(record, 0, sm6)))
                {
                    ERR("Failed to allocate name.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }
                continue;

            case METADATA_STRING:
                /* LLVM allows an empty string here. */
                m->type = VKD3D_METADATA_STRING;
                if (!(m->u.string_value = dxil_record_to_string(record, 0, sm6)))
                {
                    ERR("Failed to allocate string.\n");
                    return VKD3D_ERROR_OUT_OF_MEMORY;
                }
                break;

            case METADATA_VALUE:
                if (!dxil_record_validate_operand_count(record, 2, 2, sm6))
                    return VKD3D_ERROR_INVALID_SHADER;

                m->type = VKD3D_METADATA_VALUE;
                if (!(m->value_type = sm6_parser_get_type(sm6, record->operands[0])))
                    return VKD3D_ERROR_INVALID_SHADER;

                if (record->operands[1] > UINT_MAX)
                    WARN("Truncating value index %"PRIu64".\n", record->operands[1]);
                value_idx = record->operands[1];
                if (!(value = sm6_parser_get_value_safe(sm6, value_idx)))
                    return VKD3D_ERROR_INVALID_SHADER;

                if (!sm6_value_is_constant(value) && !sm6_value_is_undef(value) && !sm6_value_is_icb(value)
                        && !sm6_value_is_function_dcl(value))
                {
                    WARN("Value at index %u is not a constant or a function declaration.\n", value_idx);
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                            "Metadata value at index %u is not a constant or a function declaration.", value_idx);
                    return VKD3D_ERROR_INVALID_SHADER;
                }
                m->u.value = value;

                if (value->type != m->value_type)
                {
                    WARN("Type mismatch.\n");
                    vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                            "The type of a metadata value does not match its referenced value at index %u.", value_idx);
                }

                break;

            default:
                FIXME("Unhandled metadata type %u.\n", record->code);
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                        "Metadata type %u is unhandled.", record->code);
                return VKD3D_ERROR_INVALID_SHADER;
        }
        ++table->count;
    }

    return VKD3D_OK;
}

static enum vkd3d_shader_component_type vkd3d_component_type_from_dxil_component_type(enum dxil_component_type type)
{
    switch (type)
    {
        case COMPONENT_TYPE_I1:
            return VKD3D_SHADER_COMPONENT_BOOL;
        case COMPONENT_TYPE_I16:
        case COMPONENT_TYPE_I32:
            return VKD3D_SHADER_COMPONENT_INT;
        case COMPONENT_TYPE_U16:
        case COMPONENT_TYPE_U32:
            return VKD3D_SHADER_COMPONENT_UINT;
        case COMPONENT_TYPE_F16:
        case COMPONENT_TYPE_F32:
        case COMPONENT_TYPE_SNORMF32:
        case COMPONENT_TYPE_UNORMF32:
            return VKD3D_SHADER_COMPONENT_FLOAT;
        case COMPONENT_TYPE_F64:
        case COMPONENT_TYPE_SNORMF64:
        case COMPONENT_TYPE_UNORMF64:
            return VKD3D_SHADER_COMPONENT_DOUBLE;
        default:
            FIXME("Unhandled component type %u.\n", type);
            return VKD3D_SHADER_COMPONENT_UINT;
    }
}

static enum vkd3d_shader_minimum_precision minimum_precision_from_dxil_component_type(enum dxil_component_type type)
{
    switch (type)
    {
        case COMPONENT_TYPE_F16:
            return VKD3D_SHADER_MINIMUM_PRECISION_FLOAT_16;
        case COMPONENT_TYPE_I16:
            return VKD3D_SHADER_MINIMUM_PRECISION_INT_16;
        case COMPONENT_TYPE_U16:
            return VKD3D_SHADER_MINIMUM_PRECISION_UINT_16;
        default:
            return VKD3D_SHADER_MINIMUM_PRECISION_NONE;
    }
}

static const enum vkd3d_shader_sysval_semantic sysval_semantic_table[] =
{
    [SEMANTIC_KIND_ARBITRARY]            = VKD3D_SHADER_SV_NONE,
    [SEMANTIC_KIND_POSITION]             = VKD3D_SHADER_SV_POSITION,
    [SEMANTIC_KIND_TARGET]               = VKD3D_SHADER_SV_NONE,
};

static enum vkd3d_shader_sysval_semantic sysval_semantic_from_dxil_semantic_kind(enum dxil_semantic_kind kind)
{
    if (kind < ARRAY_SIZE(sysval_semantic_table))
    {
        return sysval_semantic_table[kind];
    }
    else
    {
        return VKD3D_SHADER_SV_NONE;
    }
}

static const struct sm6_metadata_value *sm6_parser_find_named_metadata(struct sm6_parser *sm6, const char *name)
{
    const struct sm6_metadata_node *node;
    unsigned int i;

    for (i = 0; i < sm6->named_metadata_count; ++i)
    {
        if (strcmp(sm6->named_metadata[i].name, name))
            continue;

        node = sm6->named_metadata[i].value.u.node;
        if (!node->operand_count)
            return NULL;
        if (node->operand_count > 1)
        {
            FIXME("Ignoring %u extra operands for %s.\n", node->operand_count - 1, name);
            vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                    "Ignoring %u extra operands for metadata node %s.", node->operand_count - 1, name);
        }
        return node->operands[0];
    }

    return NULL;
}

static bool sm6_parser_resources_load_register_range(struct sm6_parser *sm6,
        const struct sm6_metadata_node *node, struct vkd3d_shader_register_range *range)
{
    unsigned int size;

    if (!sm6_metadata_value_is_value(node->operands[1]))
    {
        WARN("Resource data type is not a value.\n");
        return false;
    }
    if (!sm6_type_is_pointer(node->operands[1]->value_type))
    {
        WARN("Resource type is not a pointer.\n");
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_TYPE_MISMATCH,
                "Resource metadata value type is not a pointer.");
    }

    if (!sm6_metadata_get_uint_value(sm6, node->operands[3], &range->space))
    {
        WARN("Failed to load register space.\n");
        return false;
    }
    if (!sm6_metadata_get_uint_value(sm6, node->operands[4], &range->first))
    {
        WARN("Failed to load register first.\n");
        return false;
    }
    if (!sm6_metadata_get_uint_value(sm6, node->operands[5], &size))
    {
        WARN("Failed to load register range size.\n");
        return false;
    }
    if (!size || (size != UINT_MAX && !vkd3d_bound_range(range->first, size, UINT_MAX)))
    {
        WARN("Invalid register range, first %u, size %u.\n", range->first, size);
        return false;
    }
    range->last = (size == UINT_MAX) ? UINT_MAX : range->first + size - 1;

    return true;
}

static enum vkd3d_result sm6_parser_resources_load_cbv(struct sm6_parser *sm6,
        const struct sm6_metadata_node *node, struct sm6_descriptor_info *d, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_register *reg;
    unsigned int buffer_size;

    if (node->operand_count < 7)
    {
        WARN("Invalid operand count %u.\n", node->operand_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
                "Invalid operand count %u for a CBV descriptor.", node->operand_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (node->operand_count > 7 && node->operands[7])
    {
        WARN("Ignoring %u extra operands.\n", node->operand_count - 7);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Ignoring %u extra operands for a CBV descriptor.", node->operand_count - 7);
    }

    if (!sm6_metadata_get_uint_value(sm6, node->operands[6], &buffer_size))
    {
        WARN("Failed to load buffer size.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                "Constant buffer size metadata value is not an integer.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    vsir_instruction_init(ins, &sm6->p.location, VKD3DSIH_DCL_CONSTANT_BUFFER);
    ins->resource_type = VKD3D_SHADER_RESOURCE_BUFFER;
    ins->declaration.cb.size = buffer_size;
    ins->declaration.cb.src.swizzle = VKD3D_SHADER_NO_SWIZZLE;
    ins->declaration.cb.src.modifiers = VKD3DSPSM_NONE;

    reg = &ins->declaration.cb.src.reg;
    vsir_register_init(reg, VKD3DSPR_CONSTBUFFER, VKD3D_DATA_FLOAT, 3);
    reg->idx[0].offset = d->id;
    reg->idx[1].offset = d->range.first;
    reg->idx[2].offset = d->range.last;

    ins->declaration.cb.range = d->range;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_descriptor_type_init(struct sm6_parser *sm6,
        enum vkd3d_shader_descriptor_type type, const struct sm6_metadata_node *descriptor_node)
{
    struct vkd3d_shader_instruction *ins;
    const struct sm6_metadata_node *node;
    const struct sm6_metadata_value *m;
    struct sm6_descriptor_info *d;
    enum vkd3d_result ret;
    unsigned int i;

    for (i = 0; i < descriptor_node->operand_count; ++i)
    {
        m = descriptor_node->operands[i];
        if (!sm6_metadata_value_is_node(m))
        {
            WARN("Resource descriptor is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                    "Resource descriptor is not a metadata node.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        node = m->u.node;
        if (node->operand_count < 6)
        {
            WARN("Invalid operand count %u.\n", node->operand_count);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
                    "Invalid operand count %u for a descriptor.", node->operand_count);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if (!vkd3d_array_reserve((void **)&sm6->descriptors, &sm6->descriptor_capacity,
                sm6->descriptor_count + 1, sizeof(*sm6->descriptors)))
        {
            ERR("Failed to allocate descriptor array.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                    "Out of memory allocating the descriptor array.");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        d = &sm6->descriptors[sm6->descriptor_count];
        d->type = type;

        if (!sm6_metadata_get_uint_value(sm6, node->operands[0], &d->id))
        {
            WARN("Failed to load resource id.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                    "Resource id metadata value is not an integer.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if (!sm6_parser_resources_load_register_range(sm6, node, &d->range))
        {
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                    "Resource register range is invalid.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if (!(ins = sm6_parser_require_space(sm6, 1)))
        {
            ERR("Failed to allocate instruction.\n");
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }

        switch (type)
        {
            case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
                if ((ret = sm6_parser_resources_load_cbv(sm6, node, d, ins)) < 0)
                    return ret;
                break;
            default:
                FIXME("Unsupported descriptor type %u.\n", type);
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                        "Resource descriptor type %u is unsupported.", type);
                return VKD3D_ERROR_INVALID_SHADER;
        }

        ++sm6->descriptor_count;
        ++sm6->p.instructions.count;
    }

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_resources_init(struct sm6_parser *sm6)
{
    const struct sm6_metadata_value *m = sm6_parser_find_named_metadata(sm6, "dx.resources");
    enum vkd3d_shader_descriptor_type type;
    const struct sm6_metadata_node *node;
    enum vkd3d_result ret;

    if (!m)
        return VKD3D_OK;

    node = m->u.node;
    if (node->operand_count != SHADER_DESCRIPTOR_TYPE_COUNT)
    {
        WARN("Unexpected descriptor type count %u.\n", node->operand_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                "Descriptor type count %u is invalid.", node->operand_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    for (type = 0; type < SHADER_DESCRIPTOR_TYPE_COUNT; ++type)
    {
        if (!(m = node->operands[type]))
            continue;

        if (!sm6_metadata_value_is_node(m))
        {
            WARN("Resource list is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_RESOURCES,
                    "Resource list is not a metadata node.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if ((ret = sm6_parser_descriptor_type_init(sm6, type, m->u.node)) < 0)
            return ret;
    }

    return VKD3D_OK;
}

static void signature_element_read_additional_element_values(struct signature_element *e,
        const struct sm6_metadata_node *node, struct sm6_parser *sm6)
{
    unsigned int i, operand_count, value, tag;

    if (node->operand_count < 11 || !node->operands[10])
        return;

    if (!sm6_metadata_value_is_node(node->operands[10]))
    {
        WARN("Additional values list is not a node.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                "Signature element additional values list is not a metadata node.");
        return;
    }

    node = node->operands[10]->u.node;
    if (node->operand_count & 1)
    {
        WARN("Operand count is not even.\n");
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                "Operand count for signature element additional tag/value pairs is not even.");
    }
    operand_count = node->operand_count & ~1u;

    for (i = 0; i < operand_count; i += 2)
    {
        if (!sm6_metadata_get_uint_value(sm6, node->operands[i], &tag)
                || !sm6_metadata_get_uint_value(sm6, node->operands[i + 1], &value))
        {
            WARN("Failed to extract tag/value pair.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Signature element tag/value pair at index %u is not an integer pair.", i);
            continue;
        }

        switch (tag)
        {
            case ADDITIONAL_TAG_STREAM_INDEX:
                e->stream_index = value;
                break;
            case ADDITIONAL_TAG_RELADDR_MASK:
                /* A mask of components accessed via relative addressing. Seems to replace TPF 'dcl_index_range'. */
                if (value > VKD3DSP_WRITEMASK_ALL)
                {
                    WARN("Invalid relative addressed mask %#x.\n", value);
                    vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_INVALID_MASK,
                            "Mask %#x of relative-addressed components is invalid.", value);
                }
                break;
            case ADDITIONAL_TAG_USED_MASK:
                if (value > VKD3DSP_WRITEMASK_ALL)
                {
                    WARN("Invalid used mask %#x.\n", value);
                    vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_INVALID_MASK,
                            "Mask %#x of used components is invalid.", value);
                    value &= VKD3DSP_WRITEMASK_ALL;
                }
                e->used_mask = value;
                break;
            default:
                FIXME("Unhandled tag %u, value %u.\n", tag, value);
                vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                        "Tag %#x for signature element additional value %#x is unhandled.", tag, value);
                break;
        }
    }
}

static enum vkd3d_result sm6_parser_read_signature(struct sm6_parser *sm6, const struct sm6_metadata_value *m,
        struct shader_signature *s)
{
    unsigned int i, j, column_count, operand_count, index;
    const struct sm6_metadata_node *node, *element_node;
    struct signature_element *elements, *e;
    unsigned int values[10];

    if (!m)
        return VKD3D_OK;

    if (!sm6_metadata_value_is_node(m))
    {
        WARN("Signature element list is not a node.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                "Signature element list is not a metadata node.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    node = m->u.node;
    operand_count = node->operand_count;

    if (!(elements = vkd3d_calloc(operand_count, sizeof(*elements))))
    {
        ERR("Failed to allocate %u signature elements.\n", operand_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating %u signature elements.", operand_count);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < operand_count; ++i)
    {
        m = node->operands[i];

        if (!sm6_metadata_value_is_node(m))
        {
            WARN("Signature element is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Signature element is not a metadata node.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        element_node = m->u.node;
        if (element_node->operand_count < 10)
        {
            WARN("Invalid operand count %u.\n", element_node->operand_count);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Invalid signature element operand count %u.", element_node->operand_count);
            return VKD3D_ERROR_INVALID_SHADER;
        }
        if (element_node->operand_count > 11)
        {
            WARN("Ignoring %u extra operands.\n", element_node->operand_count - 11);
            vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                    "Ignoring %u extra operands for a signature element.", element_node->operand_count - 11);
        }

        for (j = 0; j < 10; ++j)
        {
            /* 1 is the semantic name, 4 is semantic index metadata. */
            if (j == 1 || j == 4)
                continue;
            if (!sm6_metadata_get_uint_value(sm6, element_node->operands[j], &values[j]))
            {
                WARN("Failed to load uint value at index %u.\n", j);
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                        "Signature element value at index %u is not an integer.", j);
                return VKD3D_ERROR_INVALID_SHADER;
            }
        }

        e = &elements[i];

        if (values[0] != i)
        {
            FIXME("Unsupported element id %u not equal to its index %u.\n", values[0], i);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "A non-sequential and non-zero-based element id is not supported.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if (!sm6_metadata_value_is_string(element_node->operands[1]))
        {
            WARN("Element name is not a string.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Signature element name is not a metadata string.");
            return VKD3D_ERROR_INVALID_SHADER;
        }
        e->semantic_name = element_node->operands[1]->u.string_value;

        e->component_type = vkd3d_component_type_from_dxil_component_type(values[2]);
        e->min_precision = minimum_precision_from_dxil_component_type(values[2]);

        j = values[3];
        e->sysval_semantic = sysval_semantic_from_dxil_semantic_kind(j);
        if (j != SEMANTIC_KIND_ARBITRARY && j != SEMANTIC_KIND_TARGET && e->sysval_semantic == VKD3D_SHADER_SV_NONE)
        {
            WARN("Unhandled semantic kind %u.\n", j);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "DXIL semantic kind %u is unhandled.", j);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if ((e->interpolation_mode = values[5]) >= VKD3DSIM_COUNT)
        {
            WARN("Unhandled interpolation mode %u.\n", e->interpolation_mode);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Interpolation mode %u is unhandled.", e->interpolation_mode);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        e->register_count = values[6];
        column_count = values[7];
        e->register_index = values[8];
        e->target_location = e->register_index;
        if (e->register_index > MAX_REG_OUTPUT || e->register_count > MAX_REG_OUTPUT - e->register_index)
        {
            WARN("Invalid row start %u with row count %u.\n", e->register_index, e->register_count);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "A signature element starting row of %u with count %u is invalid.",
                    e->register_index, e->register_count);
            return VKD3D_ERROR_INVALID_SHADER;
        }
        index = values[9];
        if (index >= VKD3D_VEC4_SIZE || column_count > VKD3D_VEC4_SIZE - index)
        {
            WARN("Invalid column start %u with count %u.\n", index, column_count);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "A signature element starting column %u with count %u is invalid.", index, column_count);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        e->mask = vkd3d_write_mask_from_component_count(column_count);
        e->used_mask = e->mask;
        e->mask <<= index;

        signature_element_read_additional_element_values(e, element_node, sm6);
        e->used_mask <<= index;

        m = element_node->operands[4];
        if (!sm6_metadata_value_is_node(m))
        {
            WARN("Semantic index list is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                    "Signature element semantic index list is not a metadata node.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        element_node = m->u.node;
        for (j = 0; j < element_node->operand_count; ++j)
        {
            if (!sm6_metadata_get_uint_value(sm6, element_node->operands[j], &index))
            {
                WARN("Failed to get semantic index for row %u.\n", j);
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                        "Signature element semantic index for row %u is not an integer.", j);
            }
            else if (!j)
            {
                e->semantic_index = index;
            }
            else if (index != e->semantic_index + j)
            {
                WARN("Semantic index %u for row %u is not of an incrementing sequence.\n", index, j);
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                        "Signature element semantic index %u for row %u is not of an incrementing sequence.", index, j);
            }
        }
    }

    vkd3d_free(s->elements);
    s->elements = elements;
    s->element_count = operand_count;

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_signatures_init(struct sm6_parser *sm6, const struct sm6_metadata_value *m)
{
    enum vkd3d_result ret;

    if (!sm6_metadata_value_is_node(m))
    {
        WARN("Signature table is not a node.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_SIGNATURE,
                "Signature table is not a metadata node.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (m->u.node->operand_count && (ret = sm6_parser_read_signature(sm6, m->u.node->operands[0],
            &sm6->p.shader_desc.input_signature)) < 0)
    {
        return ret;
    }
    if (m->u.node->operand_count > 1 && (ret = sm6_parser_read_signature(sm6, m->u.node->operands[1],
            &sm6->p.shader_desc.output_signature)) < 0)
    {
        return ret;
    }
    /* TODO: patch constant signature in operand 2. */

    sm6_parser_init_input_signature(sm6, &sm6->p.shader_desc.input_signature);
    sm6_parser_init_output_signature(sm6, &sm6->p.shader_desc.output_signature);

    return VKD3D_OK;
}

static void sm6_parser_emit_global_flags(struct sm6_parser *sm6, const struct sm6_metadata_value *m)
{
    enum vkd3d_shader_global_flags global_flags, mask, rotated_flags;
    struct vkd3d_shader_instruction *ins;

    if (!sm6_metadata_get_uint64_value(sm6, m, (uint64_t*)&global_flags))
    {
        WARN("Failed to load global flags.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                "Global flags metadata value is not an integer.");
        return;
    }
    /* Rotate SKIP_OPTIMIZATION from bit 0 to bit 4 to match vkd3d_shader_global_flags. */
    mask = (VKD3DSGF_SKIP_OPTIMIZATION << 1) - 1;
    rotated_flags = global_flags & mask;
    rotated_flags = (rotated_flags >> 1) | ((rotated_flags & 1) << 4);
    global_flags = (global_flags & ~mask) | rotated_flags;

    ins = sm6_parser_add_instruction(sm6, VKD3DSIH_DCL_GLOBAL_FLAGS);
    ins->declaration.global_flags = global_flags;
}

static enum vkd3d_result sm6_parser_emit_thread_group(struct sm6_parser *sm6, const struct sm6_metadata_value *m)
{
    const struct sm6_metadata_node *node;
    struct vkd3d_shader_instruction *ins;
    unsigned int group_sizes[3];
    unsigned int i;

    if (sm6->p.shader_version.type != VKD3D_SHADER_TYPE_COMPUTE)
    {
        WARN("Shader of type %#x has thread group dimensions.\n", sm6->p.shader_version.type);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                "Shader has thread group dimensions but is not a compute shader.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!m || !sm6_metadata_value_is_node(m))
    {
        WARN("Thread group dimension value is not a node.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                "Thread group dimension metadata value is not a node.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    node = m->u.node;
    if (node->operand_count != 3)
    {
        WARN("Invalid operand count %u.\n", node->operand_count);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_OPERAND_COUNT,
                "Thread group dimension operand count %u is invalid.", node->operand_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    for (i = 0; i < 3; ++i)
    {
        if (!sm6_metadata_get_uint_value(sm6, node->operands[i], &group_sizes[i]))
        {
            WARN("Thread group dimension is not an integer value.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                    "Thread group dimension metadata value is not an integer.");
            return VKD3D_ERROR_INVALID_SHADER;
        }
        if (!group_sizes[i] || group_sizes[i] > dx_max_thread_group_size[i])
        {
            char dim = "XYZ"[i];
            WARN("Invalid thread group %c dimension %u.\n", dim, group_sizes[i]);
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                    "Thread group %c dimension %u is invalid.", dim, group_sizes[i]);
            return VKD3D_ERROR_INVALID_SHADER;
        }
    }

    ins = sm6_parser_add_instruction(sm6, VKD3DSIH_DCL_THREAD_GROUP);
    ins->declaration.thread_group_size.x = group_sizes[0];
    ins->declaration.thread_group_size.y = group_sizes[1];
    ins->declaration.thread_group_size.z = group_sizes[2];

    return VKD3D_OK;
}

static enum vkd3d_result sm6_parser_entry_point_init(struct sm6_parser *sm6)
{
    const struct sm6_metadata_value *m = sm6_parser_find_named_metadata(sm6, "dx.entryPoints");
    const struct sm6_metadata_node *node, *entry_node = m ? m->u.node : NULL;
    unsigned int i, operand_count, tag;
    const struct sm6_value *value;
    enum vkd3d_result ret;

    if (!entry_node || entry_node->operand_count < 2 || !(m = entry_node->operands[0]))
    {
        WARN("No entry point definition found.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_ENTRY_POINT,
                "No entry point definition found in the metadata.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (m->type != VKD3D_METADATA_VALUE)
    {
        WARN("Entry point definition is not a value.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_ENTRY_POINT,
                "Entry point definition is not a metadata value.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    value = m->u.value;
    if (!sm6_value_is_function_dcl(value))
    {
        WARN("Entry point value is not a function definition.\n");
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_ENTRY_POINT,
                "Entry point metadata value does not contain a function definition.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    sm6->entry_point = value->u.function.name;
    if (!sm6_metadata_value_is_string(entry_node->operands[1])
            || ascii_strcasecmp(sm6->entry_point, entry_node->operands[1]->u.string_value))
    {
        WARN("Entry point function name %s mismatch.\n", sm6->entry_point);
        vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_ENTRY_POINT_MISMATCH,
                "Entry point function name %s does not match the name in metadata.", sm6->entry_point);
    }

    if (entry_node->operand_count >= 3 && (m = entry_node->operands[2])
            && (ret = sm6_parser_signatures_init(sm6, m)) < 0)
    {
        return ret;
    }

    if (entry_node->operand_count >= 5 && (m = entry_node->operands[4]))
    {
        if (!sm6_metadata_value_is_node(m))
        {
            WARN("Shader properties list is not a node.\n");
            vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                    "Shader properties tag/value list is not a metadata node.");
            return VKD3D_ERROR_INVALID_SHADER;
        }

        node = m->u.node;
        if (node->operand_count & 1)
        {
            WARN("Operand count is not even.\n");
            vkd3d_shader_parser_warning(&sm6->p, VKD3D_SHADER_WARNING_DXIL_IGNORING_OPERANDS,
                    "Operand count for shader properties tag/value pairs is not even.");
        }
        operand_count = node->operand_count & ~1u;

        for (i = 0; i < operand_count; i += 2)
        {
            if (!sm6_metadata_get_uint_value(sm6, node->operands[i], &tag))
            {
                WARN("Tag is not an integer value.\n");
                vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                        "Shader properties tag at index %u is not an integer.", i);
                return VKD3D_ERROR_INVALID_SHADER;
            }

            switch (tag)
            {
                case SHADER_PROPERTIES_FLAGS:
                    sm6_parser_emit_global_flags(sm6, node->operands[i + 1]);
                    break;
               case SHADER_PROPERTIES_COMPUTE:
                    if ((ret = sm6_parser_emit_thread_group(sm6, node->operands[i + 1])) < 0)
                        return ret;
                    break;
                default:
                    FIXME("Unhandled tag %#x.\n", tag);
                    vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_PROPERTIES,
                            "Shader properties tag %#x is unhandled.", tag);
                    break;
            }
        }
    }

    return VKD3D_OK;
}

static void sm6_metadata_value_destroy(struct sm6_metadata_value *m)
{
    switch (m->type)
    {
        case VKD3D_METADATA_NODE:
            vkd3d_free(m->u.node);
            break;
        case VKD3D_METADATA_KIND:
            vkd3d_free(m->u.kind.name);
            break;
        case VKD3D_METADATA_STRING:
            vkd3d_free(m->u.string_value);
            break;
        default:
            break;
    }
}

static void sm6_parser_metadata_cleanup(struct sm6_parser *sm6)
{
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(sm6->metadata_tables); ++i)
    {
        for (j = 0; j < sm6->metadata_tables[i].count; ++j)
            sm6_metadata_value_destroy(&sm6->metadata_tables[i].values[j]);
        vkd3d_free(sm6->metadata_tables[i].values);
    }
    for (i = 0; i < sm6->named_metadata_count; ++i)
    {
        sm6_metadata_value_destroy(&sm6->named_metadata[i].value);
        vkd3d_free(sm6->named_metadata[i].name);
    }
    vkd3d_free(sm6->named_metadata);
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
    sm6_parser_metadata_cleanup(sm6);
    vkd3d_free(sm6->descriptors);
    vkd3d_free(sm6->values);
    free_shader_desc(&parser->shader_desc);
    vkd3d_free(sm6);
}

static const struct vkd3d_shader_parser_ops sm6_parser_ops =
{
    .parser_destroy = sm6_parser_destroy,
};

static struct sm6_function *sm6_parser_get_function(const struct sm6_parser *sm6, const char *name)
{
    size_t i;
    for (i = 0; i < sm6->function_count; ++i)
        if (!ascii_strcasecmp(sm6->functions[i].declaration->u.function.name, name))
            return &sm6->functions[i];
    return NULL;
}

static enum vkd3d_result sm6_parser_init(struct sm6_parser *sm6, const uint32_t *byte_code, size_t byte_code_size,
        const char *source_name, struct vkd3d_shader_message_context *message_context)
{
    const struct shader_signature *output_signature = &sm6->p.shader_desc.output_signature;
    const struct shader_signature *input_signature = &sm6->p.shader_desc.input_signature;
    const struct vkd3d_shader_location location = {.source_name = source_name};
    uint32_t version_token, dxil_version, token_count, magic;
    unsigned int chunk_offset, chunk_size;
    size_t count, length, function_count;
    enum bitcode_block_abbreviation abbr;
    struct vkd3d_shader_version version;
    struct dxil_block *block;
    struct sm6_function *fn;
    enum vkd3d_result ret;
    unsigned int i, j;

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

    if (!(sm6->output_params = shader_parser_get_dst_params(&sm6->p, output_signature->element_count))
            || !(sm6->input_params = shader_parser_get_dst_params(&sm6->p, input_signature->element_count)))
    {
        ERR("Failed to allocate input/output parameters.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory allocating input/output parameters.");
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
    sm6->ssa_next_id = 1;

    if ((ret = sm6_parser_globals_init(sm6)) < 0)
    {
        WARN("Failed to load global declarations.\n");
        return ret;
    }

    if (!sm6_parser_allocate_named_metadata(sm6))
    {
        ERR("Failed to allocate named metadata array.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0, j = 0; i < sm6->root_block.child_block_count; ++i)
    {
        block = sm6->root_block.child_blocks[i];
        if (block->id != METADATA_BLOCK)
            continue;

        if (j == ARRAY_SIZE(sm6->metadata_tables))
        {
            FIXME("Too many metadata tables.\n");
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_INVALID_METADATA,
                    "A metadata table count greater than %zu is unsupported.", ARRAY_SIZE(sm6->metadata_tables));
            return VKD3D_ERROR_INVALID_SHADER;
        }

        if ((ret = sm6_parser_metadata_init(sm6, block, &sm6->metadata_tables[j++])) < 0)
            return ret;
    }

    if ((ret = sm6_parser_entry_point_init(sm6)) < 0)
        return ret;

    if ((ret = sm6_parser_resources_init(sm6)) < 0)
        return ret;

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

    if (!sm6_parser_require_space(sm6, output_signature->element_count + input_signature->element_count))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory emitting shader signature declarations.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    sm6_parser_emit_output_signature(sm6, output_signature);
    sm6_parser_emit_input_signature(sm6, input_signature);

    sm6->p.shader_desc.ssa_count = sm6->ssa_next_id;

    if (!(fn = sm6_parser_get_function(sm6, sm6->entry_point)))
    {
        WARN("Failed to find entry point %s.\n", sm6->entry_point);
        vkd3d_shader_parser_error(&sm6->p, VKD3D_SHADER_ERROR_DXIL_INVALID_ENTRY_POINT,
                "The definition of the entry point function '%s' was not found.", sm6->entry_point);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    assert(sm6->function_count == 1);
    if (!sm6_block_emit_instructions(fn->blocks[0], sm6))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXIL_OUT_OF_MEMORY,
                "Out of memory emitting shader instructions.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
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

    if (!sm6->p.failed && ret >= 0)
        vsir_validate(&sm6->p);

    if (sm6->p.failed && ret >= 0)
        ret = VKD3D_ERROR_INVALID_SHADER;

    if (ret < 0)
    {
        WARN("Failed to initialise shader parser.\n");
        sm6_parser_destroy(&sm6->p);
        return ret;
    }

    *parser = &sm6->p;

    return ret;
}
