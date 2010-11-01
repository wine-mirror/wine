/*
 * Copyright 2008 Stefan Dösinger
 * Copyright 2009 Matteo Bruni
 * Copyright 2010 Rico Schüller
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

#ifndef __WINE_D3DCOMPILER_PRIVATE_H
#define __WINE_D3DCOMPILER_PRIVATE_H

#include "wine/debug.h"
#include "wine/list.h"

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "d3dcompiler.h"

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

/* TRACE helper functions */
const char *debug_d3dcompiler_d3d_blob_part(D3D_BLOB_PART part) DECLSPEC_HIDDEN;

/* ID3DBlob */
struct d3dcompiler_blob
{
    const struct ID3D10BlobVtbl *vtbl;
    LONG refcount;

    SIZE_T size;
    void *data;
};

HRESULT d3dcompiler_blob_init(struct d3dcompiler_blob *blob, SIZE_T data_size) DECLSPEC_HIDDEN;

/* blob handling */
HRESULT d3dcompiler_get_blob_part(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob) DECLSPEC_HIDDEN;
HRESULT d3dcompiler_strip_shader(const void *data, SIZE_T data_size, UINT flags, ID3DBlob **blob) DECLSPEC_HIDDEN;

/* ID3D11ShaderReflection */
extern const struct ID3D11ShaderReflectionVtbl d3dcompiler_shader_reflection_vtbl DECLSPEC_HIDDEN;
struct d3dcompiler_shader_reflection
{
    const struct ID3D11ShaderReflectionVtbl *vtbl;
    LONG refcount;
};

/* Shader assembler definitions */
typedef enum _shader_type {
    ST_VERTEX,
    ST_PIXEL,
} shader_type;

typedef enum BWRITER_COMPARISON_TYPE {
    BWRITER_COMPARISON_NONE,
    BWRITER_COMPARISON_GT,
    BWRITER_COMPARISON_EQ,
    BWRITER_COMPARISON_GE,
    BWRITER_COMPARISON_LT,
    BWRITER_COMPARISON_NE,
    BWRITER_COMPARISON_LE
} BWRITER_COMPARISON_TYPE;

struct constant {
    DWORD                   regnum;
    union {
        float               f;
        INT                 i;
        BOOL                b;
        DWORD               d;
    }                       value[4];
};

struct shader_reg {
    DWORD                   type;
    DWORD                   regnum;
    struct shader_reg       *rel_reg;
    DWORD                   srcmod;
    union {
        DWORD               swizzle;
        DWORD               writemask;
    } u;
};

struct instruction {
    DWORD                   opcode;
    DWORD                   dstmod;
    DWORD                   shift;
    BWRITER_COMPARISON_TYPE comptype;
    BOOL                    has_dst;
    struct shader_reg       dst;
    struct shader_reg       *src;
    unsigned int            num_srcs; /* For freeing the rel_regs */
    BOOL                    has_predicate;
    struct shader_reg       predicate;
    BOOL                    coissue;
};

struct declaration {
    DWORD                   usage, usage_idx;
    DWORD                   regnum;
    DWORD                   mod;
    DWORD                   writemask;
    BOOL                    builtin;
};

struct samplerdecl {
    DWORD                   type;
    DWORD                   regnum;
    DWORD                   mod;
};

#define INSTRARRAY_INITIAL_SIZE 8
struct bwriter_shader {
    shader_type             type;

    /* Shader version selected */
    DWORD                   version;

    /* Local constants. Every constant that is not defined below is loaded from
     * the global constant set at shader runtime
     */
    struct constant         **constF;
    struct constant         **constI;
    struct constant         **constB;
    unsigned int            num_cf, num_ci, num_cb;

    /* Declared input and output varyings */
    struct declaration      *inputs, *outputs;
    unsigned int            num_inputs, num_outputs;
    struct samplerdecl      *samplers;
    unsigned int            num_samplers;

    /* Are special pixel shader 3.0 registers declared? */
    BOOL                    vPos, vFace;

    /* Array of shader instructions - The shader code itself */
    struct instruction      **instr;
    unsigned int            num_instrs, instr_alloc_size;
};

static inline LPVOID asm_alloc(SIZE_T size) {
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline LPVOID asm_realloc(LPVOID ptr, SIZE_T size) {
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static inline BOOL asm_free(LPVOID ptr) {
    return HeapFree(GetProcessHeap(), 0, ptr);
}

struct asm_parser;

/* This structure is only used in asmshader.y, but since the .l file accesses the semantic types
 * too it has to know it as well
 */
struct rel_reg {
    BOOL            has_rel_reg;
    DWORD           type;
    DWORD           additional_offset;
    DWORD           rel_regnum;
    DWORD           swizzle;
};

#define MAX_SRC_REGS 4

struct src_regs {
    struct shader_reg reg[MAX_SRC_REGS];
    unsigned int      count;
};

struct asmparser_backend {
    void (*constF)(struct asm_parser *This, DWORD reg, float x, float y, float z, float w);
    void (*constI)(struct asm_parser *This, DWORD reg, INT x, INT y, INT z, INT w);
    void (*constB)(struct asm_parser *This, DWORD reg, BOOL x);

    void (*dstreg)(struct asm_parser *This, struct instruction *instr,
                   const struct shader_reg *dst);
    void (*srcreg)(struct asm_parser *This, struct instruction *instr, int num,
                   const struct shader_reg *src);

    void (*predicate)(struct asm_parser *This,
                      const struct shader_reg *predicate);
    void (*coissue)(struct asm_parser *This);

    void (*dcl_output)(struct asm_parser *This, DWORD usage, DWORD num,
                       const struct shader_reg *reg);
    void (*dcl_input)(struct asm_parser *This, DWORD usage, DWORD num,
                      DWORD mod, const struct shader_reg *reg);
    void (*dcl_sampler)(struct asm_parser *This, DWORD samptype, DWORD mod,
                        DWORD regnum, unsigned int line_no);

    void (*end)(struct asm_parser *This);

    void (*instr)(struct asm_parser *This, DWORD opcode, DWORD mod, DWORD shift,
                  BWRITER_COMPARISON_TYPE comp, const struct shader_reg *dst,
                  const struct src_regs *srcs, int expectednsrcs);
};

struct instruction *alloc_instr(unsigned int srcs) DECLSPEC_HIDDEN;
BOOL add_instruction(struct bwriter_shader *shader, struct instruction *instr) DECLSPEC_HIDDEN;
BOOL add_constF(struct bwriter_shader *shader, DWORD reg, float x, float y, float z, float w) DECLSPEC_HIDDEN;
BOOL add_constI(struct bwriter_shader *shader, DWORD reg, INT x, INT y, INT z, INT w) DECLSPEC_HIDDEN;
BOOL add_constB(struct bwriter_shader *shader, DWORD reg, BOOL x) DECLSPEC_HIDDEN;
BOOL record_declaration(struct bwriter_shader *shader, DWORD usage, DWORD usage_idx,
        DWORD mod, BOOL output, DWORD regnum, DWORD writemask, BOOL builtin) DECLSPEC_HIDDEN;
BOOL record_sampler(struct bwriter_shader *shader, DWORD samptype, DWORD mod, DWORD regnum) DECLSPEC_HIDDEN;

#define MESSAGEBUFFER_INITIAL_SIZE 256

struct asm_parser {
    /* The function table of the parser implementation */
    const struct asmparser_backend *funcs;

    /* Private data follows */
    struct bwriter_shader *shader;
    unsigned int m3x3pad_count;

    enum parse_status {
        PARSE_SUCCESS = 0,
        PARSE_WARN = 1,
        PARSE_ERR = 2
    } status;
    char *messages;
    unsigned int messagesize;
    unsigned int messagecapacity;
    unsigned int line_no;
};

extern struct asm_parser asm_ctx DECLSPEC_HIDDEN;

void create_vs10_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs11_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs20_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs2x_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_vs30_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps10_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps11_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps12_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps13_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps14_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps20_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps2x_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;
void create_ps30_parser(struct asm_parser *ret) DECLSPEC_HIDDEN;

struct bwriter_shader *parse_asm_shader(char **messages) DECLSPEC_HIDDEN;

#ifdef __GNUC__
#define PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define PRINTF_ATTR(fmt,args)
#endif

void asmparser_message(struct asm_parser *ctx, const char *fmt, ...) PRINTF_ATTR(2,3) DECLSPEC_HIDDEN;
void set_parse_status(struct asm_parser *ctx, enum parse_status status) DECLSPEC_HIDDEN;

/* A reasonable value as initial size */
#define BYTECODEBUFFER_INITIAL_SIZE 32
struct bytecode_buffer {
    DWORD *data;
    DWORD size;
    DWORD alloc_size;
    /* For tracking rare out of memory situations without passing
     * return values around everywhere
     */
    HRESULT state;
};

struct bc_writer; /* Predeclaration for use in vtable parameters */

typedef void (*instr_writer)(struct bc_writer *This,
                             const struct instruction *instr,
                             struct bytecode_buffer *buffer);

struct bytecode_backend {
    void (*header)(struct bc_writer *This, const struct bwriter_shader *shader,
                   struct bytecode_buffer *buffer);
    void (*end)(struct bc_writer *This, const struct bwriter_shader *shader,
                struct bytecode_buffer *buffer);
    void (*srcreg)(struct bc_writer *This, const struct shader_reg *reg,
                   struct bytecode_buffer *buffer);
    void (*dstreg)(struct bc_writer *This, const struct shader_reg *reg,
                   struct bytecode_buffer *buffer, DWORD shift, DWORD mod);
    void (*opcode)(struct bc_writer *This, const struct instruction *instr,
                   DWORD token, struct bytecode_buffer *buffer);

    const struct instr_handler_table {
        DWORD opcode;
        instr_writer func;
    } *instructions;
};

/* Bytecode writing stuff */
struct bc_writer {
    const struct bytecode_backend *funcs;

    /* Avoid result checking */
    HRESULT                       state;

    DWORD                         version;

    /* Vertex shader varying mapping */
    DWORD                         oPos_regnum;
    DWORD                         oD_regnum[2];
    DWORD                         oT_regnum[8];
    DWORD                         oFog_regnum;
    DWORD                         oFog_mask;
    DWORD                         oPts_regnum;
    DWORD                         oPts_mask;

    /* Pixel shader specific members */
    DWORD                         t_regnum[8];
    DWORD                         v_regnum[2];
};

/* Debug utility routines */
const char *debug_print_srcmod(DWORD mod) DECLSPEC_HIDDEN;
const char *debug_print_dstmod(DWORD mod) DECLSPEC_HIDDEN;
const char *debug_print_shift(DWORD shift) DECLSPEC_HIDDEN;
const char *debug_print_dstreg(const struct shader_reg *reg) DECLSPEC_HIDDEN;
const char *debug_print_srcreg(const struct shader_reg *reg) DECLSPEC_HIDDEN;
const char *debug_print_comp(DWORD comp) DECLSPEC_HIDDEN;
const char *debug_print_opcode(DWORD opcode) DECLSPEC_HIDDEN;

/* Used to signal an incorrect swizzle/writemask */
#define SWIZZLE_ERR ~0U

/*
  Enumerations and defines used in the bytecode writer
  intermediate representation
*/
typedef enum _BWRITERSHADER_INSTRUCTION_OPCODE_TYPE {
    BWRITERSIO_NOP,
    BWRITERSIO_MOV,
    BWRITERSIO_ADD,
    BWRITERSIO_SUB,
    BWRITERSIO_MAD,
    BWRITERSIO_MUL,
    BWRITERSIO_RCP,
    BWRITERSIO_RSQ,
    BWRITERSIO_DP3,
    BWRITERSIO_DP4,
    BWRITERSIO_MIN,
    BWRITERSIO_MAX,
    BWRITERSIO_SLT,
    BWRITERSIO_SGE,
    BWRITERSIO_EXP,
    BWRITERSIO_LOG,
    BWRITERSIO_LIT,
    BWRITERSIO_DST,
    BWRITERSIO_LRP,
    BWRITERSIO_FRC,
    BWRITERSIO_M4x4,
    BWRITERSIO_M4x3,
    BWRITERSIO_M3x4,
    BWRITERSIO_M3x3,
    BWRITERSIO_M3x2,
    BWRITERSIO_CALL,
    BWRITERSIO_CALLNZ,
    BWRITERSIO_LOOP,
    BWRITERSIO_RET,
    BWRITERSIO_ENDLOOP,
    BWRITERSIO_LABEL,
    BWRITERSIO_DCL,
    BWRITERSIO_POW,
    BWRITERSIO_CRS,
    BWRITERSIO_SGN,
    BWRITERSIO_ABS,
    BWRITERSIO_NRM,
    BWRITERSIO_SINCOS,
    BWRITERSIO_REP,
    BWRITERSIO_ENDREP,
    BWRITERSIO_IF,
    BWRITERSIO_IFC,
    BWRITERSIO_ELSE,
    BWRITERSIO_ENDIF,
    BWRITERSIO_BREAK,
    BWRITERSIO_BREAKC,
    BWRITERSIO_MOVA,
    BWRITERSIO_DEFB,
    BWRITERSIO_DEFI,

    BWRITERSIO_TEXCOORD,
    BWRITERSIO_TEXKILL,
    BWRITERSIO_TEX,
    BWRITERSIO_TEXBEM,
    BWRITERSIO_TEXBEML,
    BWRITERSIO_TEXREG2AR,
    BWRITERSIO_TEXREG2GB,
    BWRITERSIO_TEXM3x2PAD,
    BWRITERSIO_TEXM3x2TEX,
    BWRITERSIO_TEXM3x3PAD,
    BWRITERSIO_TEXM3x3TEX,
    BWRITERSIO_TEXM3x3SPEC,
    BWRITERSIO_TEXM3x3VSPEC,
    BWRITERSIO_EXPP,
    BWRITERSIO_LOGP,
    BWRITERSIO_CND,
    BWRITERSIO_DEF,
    BWRITERSIO_TEXREG2RGB,
    BWRITERSIO_TEXDP3TEX,
    BWRITERSIO_TEXM3x2DEPTH,
    BWRITERSIO_TEXDP3,
    BWRITERSIO_TEXM3x3,
    BWRITERSIO_TEXDEPTH,
    BWRITERSIO_CMP,
    BWRITERSIO_BEM,
    BWRITERSIO_DP2ADD,
    BWRITERSIO_DSX,
    BWRITERSIO_DSY,
    BWRITERSIO_TEXLDD,
    BWRITERSIO_SETP,
    BWRITERSIO_TEXLDL,
    BWRITERSIO_BREAKP,
    BWRITERSIO_TEXLDP,
    BWRITERSIO_TEXLDB,

    BWRITERSIO_PHASE,
    BWRITERSIO_COMMENT,
    BWRITERSIO_END,
} BWRITERSHADER_INSTRUCTION_OPCODE_TYPE;

typedef enum _BWRITERSHADER_PARAM_REGISTER_TYPE {
    BWRITERSPR_TEMP,
    BWRITERSPR_INPUT,
    BWRITERSPR_CONST,
    BWRITERSPR_ADDR,
    BWRITERSPR_TEXTURE,
    BWRITERSPR_RASTOUT,
    BWRITERSPR_ATTROUT,
    BWRITERSPR_TEXCRDOUT,
    BWRITERSPR_OUTPUT,
    BWRITERSPR_CONSTINT,
    BWRITERSPR_COLOROUT,
    BWRITERSPR_DEPTHOUT,
    BWRITERSPR_SAMPLER,
    BWRITERSPR_CONSTBOOL,
    BWRITERSPR_LOOP,
    BWRITERSPR_MISCTYPE,
    BWRITERSPR_LABEL,
    BWRITERSPR_PREDICATE
} BWRITERSHADER_PARAM_REGISTER_TYPE;

typedef enum _BWRITERVS_RASTOUT_OFFSETS
{
    BWRITERSRO_POSITION,
    BWRITERSRO_FOG,
    BWRITERSRO_POINT_SIZE
} BWRITERVS_RASTOUT_OFFSETS;

#define BWRITERSP_WRITEMASK_0   0x1 /* .x r */
#define BWRITERSP_WRITEMASK_1   0x2 /* .y g */
#define BWRITERSP_WRITEMASK_2   0x4 /* .z b */
#define BWRITERSP_WRITEMASK_3   0x8 /* .w a */
#define BWRITERSP_WRITEMASK_ALL 0xf /* all */

typedef enum _BWRITERSHADER_PARAM_DSTMOD_TYPE {
    BWRITERSPDM_NONE = 0,
    BWRITERSPDM_SATURATE = 1,
    BWRITERSPDM_PARTIALPRECISION = 2,
    BWRITERSPDM_MSAMPCENTROID = 4,
} BWRITERSHADER_PARAM_DSTMOD_TYPE;

typedef enum _BWRITERSAMPLER_TEXTURE_TYPE {
    BWRITERSTT_UNKNOWN = 0,
    BWRITERSTT_1D = 1,
    BWRITERSTT_2D = 2,
    BWRITERSTT_CUBE = 3,
    BWRITERSTT_VOLUME = 4,
} BWRITERSAMPLER_TEXTURE_TYPE;

#define BWRITERSI_TEXLD_PROJECT 1
#define BWRITERSI_TEXLD_BIAS    2

typedef enum _BWRITERSHADER_PARAM_SRCMOD_TYPE {
    BWRITERSPSM_NONE = 0,
    BWRITERSPSM_NEG,
    BWRITERSPSM_BIAS,
    BWRITERSPSM_BIASNEG,
    BWRITERSPSM_SIGN,
    BWRITERSPSM_SIGNNEG,
    BWRITERSPSM_COMP,
    BWRITERSPSM_X2,
    BWRITERSPSM_X2NEG,
    BWRITERSPSM_DZ,
    BWRITERSPSM_DW,
    BWRITERSPSM_ABS,
    BWRITERSPSM_ABSNEG,
    BWRITERSPSM_NOT,
} BWRITERSHADER_PARAM_SRCMOD_TYPE;

#define BWRITER_SM1_VS  0xfffe
#define BWRITER_SM1_PS  0xffff

#define BWRITERPS_VERSION(major, minor) ((BWRITER_SM1_PS << 16) | ((major) << 8) | (minor))
#define BWRITERVS_VERSION(major, minor) ((BWRITER_SM1_VS << 16) | ((major) << 8) | (minor))

#define BWRITERVS_SWIZZLE_SHIFT      16
#define BWRITERVS_SWIZZLE_MASK       (0xFF << BWRITERVS_SWIZZLE_SHIFT)

#define BWRITERVS_X_X       (0 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_Y       (1 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_Z       (2 << BWRITERVS_SWIZZLE_SHIFT)
#define BWRITERVS_X_W       (3 << BWRITERVS_SWIZZLE_SHIFT)

#define BWRITERVS_Y_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 2))
#define BWRITERVS_Y_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 2))

#define BWRITERVS_Z_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 4))
#define BWRITERVS_Z_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 4))

#define BWRITERVS_W_X       (0 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_Y       (1 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_Z       (2 << (BWRITERVS_SWIZZLE_SHIFT + 6))
#define BWRITERVS_W_W       (3 << (BWRITERVS_SWIZZLE_SHIFT + 6))

#define BWRITERVS_NOSWIZZLE (BWRITERVS_X_X | BWRITERVS_Y_Y | BWRITERVS_Z_Z | BWRITERVS_W_W)

#define BWRITERVS_SWIZZLE_X (BWRITERVS_X_X | BWRITERVS_Y_X | BWRITERVS_Z_X | BWRITERVS_W_X)
#define BWRITERVS_SWIZZLE_Y (BWRITERVS_X_Y | BWRITERVS_Y_Y | BWRITERVS_Z_Y | BWRITERVS_W_Y)
#define BWRITERVS_SWIZZLE_Z (BWRITERVS_X_Z | BWRITERVS_Y_Z | BWRITERVS_Z_Z | BWRITERVS_W_Z)
#define BWRITERVS_SWIZZLE_W (BWRITERVS_X_W | BWRITERVS_Y_W | BWRITERVS_Z_W | BWRITERVS_W_W)

typedef enum _BWRITERDECLUSAGE {
    BWRITERDECLUSAGE_POSITION,
    BWRITERDECLUSAGE_BLENDWEIGHT,
    BWRITERDECLUSAGE_BLENDINDICES,
    BWRITERDECLUSAGE_NORMAL,
    BWRITERDECLUSAGE_PSIZE,
    BWRITERDECLUSAGE_TEXCOORD,
    BWRITERDECLUSAGE_TANGENT,
    BWRITERDECLUSAGE_BINORMAL,
    BWRITERDECLUSAGE_TESSFACTOR,
    BWRITERDECLUSAGE_POSITIONT,
    BWRITERDECLUSAGE_COLOR,
    BWRITERDECLUSAGE_FOG,
    BWRITERDECLUSAGE_DEPTH,
    BWRITERDECLUSAGE_SAMPLE
} BWRITERDECLUSAGE;

/* ps 1.x texture registers mappings */
#define T0_REG          2
#define T1_REG          3
#define T2_REG          4
#define T3_REG          5

struct bwriter_shader *SlAssembleShader(const char *text, char **messages) DECLSPEC_HIDDEN;
DWORD SlWriteBytecode(const struct bwriter_shader *shader, int dxversion, DWORD **result) DECLSPEC_HIDDEN;
void SlDeleteShader(struct bwriter_shader *shader) DECLSPEC_HIDDEN;

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_Aon9 MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_PCSG MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_RDEF MAKE_TAG('R', 'D', 'E', 'F')
#define TAG_SDBG MAKE_TAG('S', 'D', 'B', 'G')
#define TAG_STAT MAKE_TAG('S', 'T', 'A', 'T')
#define TAG_XNAP MAKE_TAG('X', 'N', 'A', 'P')
#define TAG_XNAS MAKE_TAG('X', 'N', 'A', 'S')

struct dxbc_section
{
    DWORD tag;
    const char *data;
    DWORD data_size;
};

struct dxbc
{
    UINT size;
    UINT count;
    struct dxbc_section *sections;
};

HRESULT dxbc_write_blob(struct dxbc *dxbc, ID3DBlob **blob) DECLSPEC_HIDDEN;
void dxbc_destroy(struct dxbc *dxbc) DECLSPEC_HIDDEN;
HRESULT dxbc_parse(const char *data, SIZE_T data_size, struct dxbc *dxbc) DECLSPEC_HIDDEN;
HRESULT dxbc_add_section(struct dxbc *dxbc, DWORD tag, const char *data, DWORD data_size) DECLSPEC_HIDDEN;
HRESULT dxbc_init(struct dxbc *dxbc, DWORD count) DECLSPEC_HIDDEN;

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

static inline void write_dword(char **ptr, DWORD d)
{
    memcpy(*ptr, &d, sizeof(d));
    *ptr += sizeof(d);
}

void skip_dword_unknown(const char **ptr, unsigned int count) DECLSPEC_HIDDEN;
void write_dword_unknown(char **ptr, DWORD d) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DCOMPILER_PRIVATE_H */
