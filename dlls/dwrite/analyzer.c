/*
 *    Text analyzer
 *
 * Copyright 2011 Aric Stewart for CodeWeavers
 * Copyright 2012, 2014 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include "dwrite_private.h"
#include "scripts.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwrite);

extern const unsigned short wine_linebreak_table[];
extern const unsigned short wine_scripts_table[];

struct dwritescript_properties {
    DWRITE_SCRIPT_PROPERTIES props;
    BOOL is_complex;
    const struct scriptshaping_ops *ops;
};

/* NOTE: keep this array synced with script ids from scripts.h */
static const struct dwritescript_properties dwritescripts_properties[Script_LastId+1] = {
    { /* Zzzz */ { 0x7a7a7a5a, 999, 15, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Zyyy */ { 0x7979795a, 998,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Arab */ { 0x62617241, 160,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, TRUE },
    { /* Armn */ { 0x6e6d7241, 230,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Avst */ { 0x74737641, 134,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Bali */ { 0x696c6142, 360, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 } },
    { /* Bamu */ { 0x756d6142, 435,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Bass */ { 0x73736142, 259,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Batk */ { 0x6b746142, 365,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Beng */ { 0x676e6542, 325, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, TRUE },
    { /* Bopo */ { 0x6f706f42, 285,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Brah */ { 0x68617242, 300,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Brai */ { 0x69617242, 570,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Bugi */ { 0x69677542, 367,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Buhd */ { 0x64687542, 372,  8, 0x0020, 0, 0, 1, 0, 0, 0, 0 } },
    { /* Cans */ { 0x736e6143, 440,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Cari */ { 0x69726143, 201,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 } },
    { /* Aghb */ { 0x62686741, 239,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Cakm */ { 0x6d6b6143, 349,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Cham */ { 0x6d616843, 358,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Cher */ { 0x72656843, 445,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Copt */ { 0x74706f43, 204,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Xsux */ { 0x78757358,  20,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Cprt */ { 0x74727043, 403,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 } },
    { /* Cyrl */ { 0x6c727943, 220,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Dsrt */ { 0x74727344, 250,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Deva */ { 0x61766544, 315, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, TRUE },
    { /* Dupl */ { 0x6c707544, 755,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Egyp */ { 0x70796745,  50,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Elba */ { 0x61626c45, 226,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Ethi */ { 0x69687445, 430,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Geor */ { 0x726f6547, 240,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Glag */ { 0x67616c47, 225,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Goth */ { 0x68746f47, 206,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Gran */ { 0x6e617247, 343,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Grek */ { 0x6b657247, 200,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Gujr */ { 0x726a7547, 320, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Guru */ { 0x75727547, 310, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 }, TRUE },
    { /* Hani */ { 0x696e6148, 500,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Hang */ { 0x676e6148, 286,  8, 0x0020, 1, 1, 1, 1, 0, 0, 0 }, TRUE },
    { /* Hano */ { 0x6f6e6148, 371,  8, 0x0020, 0, 0, 1, 0, 0, 0, 0 } },
    { /* Hebr */ { 0x72626548, 125,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Hira */ { 0x61726948, 410,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Armi */ { 0x696d7241, 124,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Phli */ { 0x696c6850, 131,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Prti */ { 0x69747250, 130,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Java */ { 0x6176614a, 361, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 } },
    { /* Kthi */ { 0x6968744b, 317, 15, 0x0020, 1, 1, 0, 0, 0, 1, 0 } },
    { /* Knda */ { 0x61646e4b, 345, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Kana */ { 0x616e614b, 411,  8, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Kali */ { 0x696c614b, 357,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Khar */ { 0x7261684b, 305, 15, 0x0020, 1, 0, 1, 0, 0, 0, 0 } },
    { /* Khmr */ { 0x726d684b, 355,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, TRUE },
    { /* Khoj */ { 0x6a6f684b, 322,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Sind */ { 0x646e6953, 318,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Laoo */ { 0x6f6f614c, 356,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, TRUE },
    { /* Latn */ { 0x6e74614c, 215,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, FALSE, &latn_shaping_ops },
    { /* Lepc */ { 0x6370654c, 335,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Limb */ { 0x626d694c, 336,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Lina */ { 0x616e694c, 400,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Linb */ { 0x626e694c, 401,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Lisu */ { 0x7573694c, 399,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Lyci */ { 0x6963794c, 202,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Lydi */ { 0x6964794c, 116,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Mahj */ { 0x6a68614d, 314,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Mlym */ { 0x6d796c4d, 347, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Mand */ { 0x646e614d, 140,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 } },
    { /* Mani */ { 0x696e614d, 139,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Mtei */ { 0x6965744d, 337,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Mend */ { 0x646e654d, 438,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Merc */ { 0x6372654d, 101,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Mero */ { 0x6f72654d, 100,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Plrd */ { 0x64726c50, 282,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Modi */ { 0x69646f4d, 324,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Mong */ { 0x676e6f4d, 145,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, TRUE },
    { /* Mroo */ { 0x6f6f724d, 199,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Mymr */ { 0x726d794d, 350, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Nbat */ { 0x7461624e, 159,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Talu */ { 0x756c6154, 354,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Nkoo */ { 0x6f6f6b4e, 165,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, TRUE },
    { /* Ogam */ { 0x6d61674f, 212,  1, 0x1680, 0, 1, 0, 0, 0, 1, 0 }, TRUE },
    { /* Olck */ { 0x6b636c4f, 261,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Ital */ { 0x6c617449, 210,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Narb */ { 0x6272614e, 106,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Perm */ { 0x6d726550, 227,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Xpeo */ { 0x6f657058,  30,  1, 0x0020, 0, 1, 1, 1, 0, 0, 0 }, TRUE },
    { /* Sarb */ { 0x62726153, 105,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Orkh */ { 0x686b724f, 175,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Orya */ { 0x6179724f, 327, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Osma */ { 0x616d734f, 260,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Hmng */ { 0x676e6d48, 450,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Palm */ { 0x6d6c6150, 126,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Pauc */ { 0x63756150, 263,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Phag */ { 0x67616850, 331,  8, 0x0020, 0, 1, 0, 0, 0, 1, 1 }, TRUE },
    { /* Phnx */ { 0x786e6850, 115,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 } },
    { /* Phlp */ { 0x706c6850, 132,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Rjng */ { 0x676e6a52, 363,  8, 0x0020, 1, 0, 1, 0, 0, 0, 0 } },
    { /* Runr */ { 0x726e7552, 211,  1, 0x0020, 0, 0, 1, 0, 0, 0, 0 }, TRUE },
    { /* Samr */ { 0x726d6153, 123,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Saur */ { 0x72756153, 344,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Shrd */ { 0x64726853, 319,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Shaw */ { 0x77616853, 281,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Sidd */ { 0x64646953, 302,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Sinh */ { 0x686e6953, 348,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Sora */ { 0x61726f53, 398,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Sund */ { 0x646e7553, 362,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 } },
    { /* Sylo */ { 0x6f6c7953, 316,  8, 0x0020, 1, 1, 0, 0, 0, 1, 0 } },
    { /* Syrc */ { 0x63727953, 135,  8, 0x0640, 0, 1, 0, 0, 0, 1, 1 }, TRUE },
    { /* Tglg */ { 0x676c6754, 370,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Tagb */ { 0x62676154, 373,  8, 0x0020, 0, 1, 1, 0, 0, 0, 0 } },
    { /* Tale */ { 0x656c6154, 353,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Lana */ { 0x616e614c, 351,  8, 0x0020, 1, 0, 1, 0, 0, 0, 0 } },
    { /* Tavt */ { 0x74766154, 359,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 } },
    { /* Takr */ { 0x726b6154, 321,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Taml */ { 0x6c6d6154, 346, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Telu */ { 0x756c6554, 340, 15, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Thaa */ { 0x61616854, 170,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Thai */ { 0x69616854, 352,  8, 0x0020, 1, 0, 1, 0, 1, 0, 0 }, TRUE },
    { /* Tibt */ { 0x74626954, 330,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Tfng */ { 0x676e6654, 120,  8, 0x0020, 1, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Tirh */ { 0x68726954, 326,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Ugar */ { 0x72616755,  40,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 } },
    { /* Vaii */ { 0x69696156, 470,  1, 0x0020, 0, 1, 1, 0, 0, 0, 0 }, TRUE },
    { /* Wara */ { 0x61726157, 262,  1, 0x0020, 0, 0, 0, 0, 0, 0, 0 } },
    { /* Yiii */ { 0x69696959, 460,  1, 0x0020, 0, 0, 1, 1, 0, 0, 0 }, TRUE }
};

struct dwrite_numbersubstitution {
    IDWriteNumberSubstitution IDWriteNumberSubstitution_iface;
    LONG ref;

    DWRITE_NUMBER_SUBSTITUTION_METHOD method;
    WCHAR *locale;
    BOOL ignore_user_override;
};

static inline struct dwrite_numbersubstitution *impl_from_IDWriteNumberSubstitution(IDWriteNumberSubstitution *iface)
{
    return CONTAINING_RECORD(iface, struct dwrite_numbersubstitution, IDWriteNumberSubstitution_iface);
}

static inline UINT32 decode_surrogate_pair(const WCHAR *str, UINT32 index, UINT32 end)
{
    if (index < end-1 && IS_SURROGATE_PAIR(str[index], str[index+1])) {
        UINT32 ch = 0x10000 + ((str[index] - 0xd800) << 10) + (str[index+1] - 0xdc00);
        TRACE("surrogate pair (%x %x) => %x\n", str[index], str[index+1], ch);
        return ch;
    }
    return 0;
}

static inline UINT16 get_char_script(WCHAR c)
{
    UINT16 script = get_table_entry(wine_scripts_table, c);
    if (script == Script_Unknown) {
        WORD type;
        if (GetStringTypeW(CT_CTYPE1, &c, 1, &type) && (type & C1_CNTRL))
            script = Script_Common;
    }
    return script;
}

static HRESULT analyze_script(const WCHAR *text, UINT32 len, IDWriteTextAnalysisSink *sink)
{
    DWRITE_SCRIPT_ANALYSIS sa;
    UINT32 pos, i, length;

    if (!len) return S_OK;

    sa.script = get_char_script(*text);

    pos = 0;
    length = 1;

    for (i = 1; i < len; i++)
    {
        UINT16 script = get_char_script(text[i]);

        /* Unknown type is ignored when preceded or followed by another script */
        if (sa.script == Script_Unknown) sa.script = script;
        if (script == Script_Unknown && sa.script != Script_Common) script = sa.script;
        /* this is a length of a sequence to be reported next */
        if (sa.script == script) length++;

        if (sa.script != script)
        {
            HRESULT hr;

            sa.shapes = sa.script != Script_Common ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
            hr = IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
            if (FAILED(hr)) return hr;
            pos = i;
            length = 1;
            sa.script = script;
        }
    }

    /* 1 length case or normal completion call */
    sa.shapes = sa.script != Script_Common ? DWRITE_SCRIPT_SHAPES_DEFAULT : DWRITE_SCRIPT_SHAPES_NO_VISUAL;
    return IDWriteTextAnalysisSink_SetScriptAnalysis(sink, pos, length, &sa);
}

struct linebreaking_state {
    DWRITE_LINE_BREAKPOINT *breakpoints;
    UINT32 count;
};

enum BreakConditionLocation {
    BreakConditionBefore,
    BreakConditionAfter
};

enum linebreaking_classes {
    b_BK = 1,
    b_CR,
    b_LF,
    b_CM,
    b_SG,
    b_GL,
    b_CB,
    b_SP,
    b_ZW,
    b_NL,
    b_WJ,
    b_JL,
    b_JV,
    b_JT,
    b_H2,
    b_H3,
    b_XX,
    b_OP,
    b_CL,
    b_CP,
    b_QU,
    b_NS,
    b_EX,
    b_SY,
    b_IS,
    b_PR,
    b_PO,
    b_NU,
    b_AL,
    b_ID,
    b_IN,
    b_HY,
    b_BB,
    b_BA,
    b_SA,
    b_AI,
    b_B2,
    b_HL,
    b_CJ,
    b_RI
};

/* "Can break" is a weak condition, stronger "may not break" and "must break" override it. Initially all conditions are
    set to "can break" and could only be changed once. */
static inline void set_break_condition(UINT32 pos, enum BreakConditionLocation location, DWRITE_BREAK_CONDITION condition,
    struct linebreaking_state *state)
{
    if (location == BreakConditionBefore) {
        if (state->breakpoints[pos].breakConditionBefore != DWRITE_BREAK_CONDITION_CAN_BREAK)
            return;
        state->breakpoints[pos].breakConditionBefore = condition;
        if (pos > 0)
            state->breakpoints[pos-1].breakConditionAfter = condition;
    }
    else {
        if (state->breakpoints[pos].breakConditionAfter != DWRITE_BREAK_CONDITION_CAN_BREAK)
            return;
        state->breakpoints[pos].breakConditionAfter = condition;
        if (pos + 1 < state->count)
            state->breakpoints[pos+1].breakConditionBefore = condition;
    }
}

static HRESULT analyze_linebreaks(const WCHAR *text, UINT32 count, DWRITE_LINE_BREAKPOINT *breakpoints)
{
    struct linebreaking_state state;
    short *break_class;
    int i, j;

    break_class = heap_alloc(count*sizeof(short));
    if (!break_class)
        return E_OUTOFMEMORY;

    state.breakpoints = breakpoints;
    state.count = count;

    /* LB31 - allow breaks everywhere. It will be overridden if needed as
       other rules dictate. */
    for (i = 0; i < count; i++)
    {
        break_class[i] = get_table_entry(wine_linebreak_table, text[i]);

        breakpoints[i].breakConditionBefore = DWRITE_BREAK_CONDITION_CAN_BREAK;
        breakpoints[i].breakConditionAfter  = DWRITE_BREAK_CONDITION_CAN_BREAK;
        breakpoints[i].isWhitespace = break_class[i] == b_BK || break_class[i] == b_ZW || break_class[i] == b_SP || isspaceW(text[i]);
        breakpoints[i].isSoftHyphen = FALSE;
        breakpoints[i].padding = 0;

        /* LB1 - resolve some classes. TODO: use external algorithms for these classes. */
        switch (break_class[i])
        {
            case b_AI:
            case b_SA:
            case b_SG:
            case b_XX:
                break_class[i] = b_AL;
                break;
            case b_CJ:
                break_class[i] = b_NS;
                break;
        }
    }

    /* LB2 - never break at the start */
    set_break_condition(0, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
    /* LB3 - always break at the end. This one is ignored. */

    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB4 - LB6 */
            case b_CR:
                /* LB5 - don't break CR x LF */
                if (i < count-1 && break_class[i+1] == b_LF)
                {
                    set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    break;
                }
            case b_LF:
            case b_NL:
            case b_BK:
                /* LB4 - LB5 - always break after hard breaks */
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MUST_BREAK, &state);
                /* LB6 - do not break before hard breaks */
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB7 - do not break before spaces */
            case b_SP:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_ZW:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB8 - break before character after zero-width space, skip spaces in-between */
                while (i < count-1 && break_class[i+1] == b_SP)
                    i++;
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
        }
    }

    /* LB9 - LB10 */
    for (i = 0; i < count; i++)
    {
        if (break_class[i] == b_CM)
        {
            if (i > 0)
            {
                switch (break_class[i-1])
                {
                    case b_SP:
                    case b_BK:
                    case b_CR:
                    case b_LF:
                    case b_NL:
                    case b_ZW:
                        break_class[i] = b_AL;
                        break;
                    default:
                        break_class[i] = break_class[i-1];
                }
            }
            else break_class[i] = b_AL;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch (break_class[i])
        {
            /* LB11 - don't break before and after word joiner */
            case b_WJ:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB12 - don't break after glue */
            case b_GL:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB12a */
                if (i > 0)
                {
                    if (break_class[i-1] != b_SP && break_class[i-1] != b_BA && break_class[i-1] != b_HY)
                        set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
                break;
            /* LB13 */
            case b_CL:
            case b_CP:
            case b_EX:
            case b_IS:
            case b_SY:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB14 */
            case b_OP:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                while (i < count-1 && break_class[i+1] == b_SP) {
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    i++;
                }
                break;
            /* LB15 */
            case b_QU:
                j = i+1;
                while (j < count-1 && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_OP)
                    for (; j > i; j--)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB16 */
            case b_NS:
                j = i-1;
                while(j > 0 && break_class[j] == b_SP)
                    j--;
                if (break_class[j] == b_CL || break_class[j] == b_CP)
                    for (j++; j <= i; j++)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB17 */
            case b_B2:
                j = i+1;
                while (j < count && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_B2)
                    for (; j > i; j--)
                        set_break_condition(j, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB18 - break is allowed after space */
            case b_SP:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB19 - don't break before or after quotation mark */
            case b_QU:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB20 */
            case b_CB:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_CAN_BREAK, &state);
                break;
            /* LB21 */
            case b_BA:
            case b_HY:
            case b_NS:
                set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            case b_BB:
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                break;
            /* LB21a */
            case b_HL:
                if (i < count-2)
                    switch (break_class[i+1])
                    {
                    case b_HY:
                    case b_BA:
                        set_break_condition(i+1, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                break;
            /* LB22 */
            case b_IN:
                if (i > 0)
                {
                    switch (break_class[i-1])
                    {
                        case b_AL:
                        case b_HL:
                        case b_ID:
                        case b_IN:
                        case b_NU:
                            set_break_condition(i, BreakConditionBefore, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                    }
                }
                break;
        }

        if (i < count-1)
        {
            /* LB23 */
            if ((break_class[i] == b_ID && break_class[i+1] == b_PO) ||
                (break_class[i] == b_AL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_AL) ||
                (break_class[i] == b_NU && break_class[i+1] == b_HL))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            /* LB24 */
            if ((break_class[i] == b_PR && break_class[i+1] == b_ID) ||
                (break_class[i] == b_PR && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PR && break_class[i+1] == b_HL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_HL))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB25 */
            if ((break_class[i] == b_CL && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PR) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PO) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PR) ||
                (break_class[i] == b_PO && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PO && break_class[i+1] == b_NU) ||
                (break_class[i] == b_PR && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PR && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HY && break_class[i+1] == b_NU) ||
                (break_class[i] == b_IS && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_NU) ||
                (break_class[i] == b_SY && break_class[i+1] == b_NU))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB26 */
            if (break_class[i] == b_JL)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }
            if ((break_class[i] == b_JV || break_class[i] == b_H2) &&
                (break_class[i+1] == b_JV || break_class[i+1] == b_JT))
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if ((break_class[i] == b_JT || break_class[i] == b_H3) &&
                 break_class[i+1] == b_JT)
                    set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB27 */
            switch (break_class[i])
            {
                case b_JL:
                case b_JV:
                case b_JT:
                case b_H2:
                case b_H3:
                    if (break_class[i+1] == b_IN || break_class[i+1] == b_PO)
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            }
            if (break_class[i] == b_PO)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_JT:
                    case b_H2:
                    case b_H3:
                        set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
                }
            }

            /* LB28 */
            if ((break_class[i] == b_AL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_AL && break_class[i+1] == b_HL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB29 */
            if ((break_class[i] == b_IS && break_class[i+1] == b_AL) ||
                (break_class[i] == b_IS && break_class[i+1] == b_HL))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30 */
            if ((break_class[i] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU) &&
                 break_class[i+1] == b_OP)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
            if (break_class[i] == b_CP &&
               (break_class[i+1] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU))
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);

            /* LB30a */
            if (break_class[i] == b_RI && break_class[i+1] == b_RI)
                set_break_condition(i, BreakConditionAfter, DWRITE_BREAK_CONDITION_MAY_NOT_BREAK, &state);
        }
    }

    heap_free(break_class);
    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer_QueryInterface(IDWriteTextAnalyzer2 *iface, REFIID riid, void **obj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteTextAnalyzer2) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer1) ||
        IsEqualIID(riid, &IID_IDWriteTextAnalyzer) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;

}

static ULONG WINAPI dwritetextanalyzer_AddRef(IDWriteTextAnalyzer2 *iface)
{
    return 2;
}

static ULONG WINAPI dwritetextanalyzer_Release(IDWriteTextAnalyzer2 *iface)
{
    return 1;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeScript(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    const WCHAR *text;
    HRESULT hr;
    UINT32 len;

    TRACE("(%p %u %u %p)\n", source, position, length, sink);

    hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &text, &len);
    if (FAILED(hr)) return hr;

    return analyze_script(text, len, sink);
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeBidi(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    UINT8 *levels = NULL, *explicit = NULL;
    UINT8 baselevel, level, explicit_level;
    WCHAR *buff = NULL;
    const WCHAR *text;
    UINT32 len, pos, i;
    HRESULT hr;

    TRACE("(%p %u %u %p)\n", source, position, length, sink);

    if (length == 0)
        return S_OK;

    /* get some, check for length */
    text = NULL;
    len = 0;
    hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &text, &len);
    if (FAILED(hr)) return hr;

    if (len < length) {
        UINT32 read;

        buff = heap_alloc(length*sizeof(WCHAR));
        if (!buff)
            return E_OUTOFMEMORY;
        memcpy(buff, text, len*sizeof(WCHAR));
        read = len;

        while (read < length && text) {
            text = NULL;
            len = 0;
            hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, read, &text, &len);
            if (FAILED(hr))
                goto done;
            memcpy(&buff[read], text, min(len, length-read)*sizeof(WCHAR));
            read += len;
        }

        text = buff;
    }

    levels = heap_alloc(length*sizeof(*levels));
    explicit = heap_alloc(length*sizeof(*explicit));

    if (!levels || !explicit) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    baselevel = IDWriteTextAnalysisSource_GetParagraphReadingDirection(source);
    hr = bidi_computelevels(text, length, baselevel, explicit, levels);
    if (FAILED(hr))
        goto done;

    level = levels[0];
    explicit_level = explicit[0];
    pos = 0;
    for (i = 1; i < length; i++) {
        if (levels[i] != level || explicit[i] != explicit_level) {
            hr = IDWriteTextAnalysisSink_SetBidiLevel(sink, pos, i - pos, explicit_level, level);
            if (FAILED(hr))
                break;
            level = levels[i];
            explicit_level = explicit[i];
            pos = i;
        }

        if (i == length - 1)
            hr = IDWriteTextAnalysisSink_SetBidiLevel(sink, pos, length - pos, explicit_level, level);
    }

done:
    heap_free(explicit);
    heap_free(levels);
    heap_free(buff);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeNumberSubstitution(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    FIXME("(%p %u %u %p): stub\n", source, position, length, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer_AnalyzeLineBreakpoints(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource* source, UINT32 position, UINT32 length, IDWriteTextAnalysisSink* sink)
{
    DWRITE_LINE_BREAKPOINT *breakpoints = NULL;
    WCHAR *buff = NULL;
    const WCHAR *text;
    HRESULT hr;
    UINT32 len;

    TRACE("(%p %u %u %p)\n", source, position, length, sink);

    if (length == 0)
        return S_OK;

    /* get some, check for length */
    text = NULL;
    len = 0;
    hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, position, &text, &len);
    if (FAILED(hr)) return hr;

    if (len < length) {
        UINT32 read;

        buff = heap_alloc(length*sizeof(WCHAR));
        if (!buff)
            return E_OUTOFMEMORY;
        memcpy(buff, text, len*sizeof(WCHAR));
        read = len;

        while (read < length && text) {
            text = NULL;
            len = 0;
            hr = IDWriteTextAnalysisSource_GetTextAtPosition(source, read, &text, &len);
            if (FAILED(hr))
                goto done;
            memcpy(&buff[read], text, min(len, length-read)*sizeof(WCHAR));
            read += len;
        }

        text = buff;
    }

    breakpoints = heap_alloc(length*sizeof(*breakpoints));
    if (!breakpoints) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = analyze_linebreaks(text, length, breakpoints);
    if (FAILED(hr))
        goto done;

    hr = IDWriteTextAnalysisSink_SetLineBreakpoints(sink, position, length, breakpoints);

done:
    heap_free(breakpoints);
    heap_free(buff);

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphs(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT32 length, IDWriteFontFace* fontface, BOOL is_sideways,
    BOOL is_rtl, DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale,
    IDWriteNumberSubstitution* substitution, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_len, UINT32 feature_ranges, UINT32 max_glyph_count,
    UINT16* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* text_props, UINT16* glyph_indices,
    DWRITE_SHAPING_GLYPH_PROPERTIES* glyph_props, UINT32* actual_glyph_count)
{
    const struct dwritescript_properties *scriptprops;
    struct scriptshaping_context context;
    struct scriptshaping_cache *cache = NULL;
    WCHAR *string;
    BOOL update_cluster;
    UINT32 i, g;
    HRESULT hr = S_OK;
    UINT16 script;

    TRACE("(%s:%u %p %d %d %p %s %p %p %p %u %u %p %p %p %p %p)\n", debugstr_wn(text, length),
        length, fontface, is_sideways, is_rtl, analysis, debugstr_w(locale), substitution, features, feature_range_len,
        feature_ranges, max_glyph_count, clustermap, text_props, glyph_indices, glyph_props, actual_glyph_count);

    script = analysis->script > Script_LastId ? Script_Unknown : analysis->script;

    if (max_glyph_count < length)
        return E_NOT_SUFFICIENT_BUFFER;

    if (substitution)
        FIXME("number substitution is not supported.\n");

    for (i = 0; i < length; i++) {
        /* FIXME: set to better values */
        glyph_props[i].justification = text[i] == ' ' ? SCRIPT_JUSTIFY_BLANK : SCRIPT_JUSTIFY_CHARACTER;
        glyph_props[i].isClusterStart = 1;
        glyph_props[i].isDiacritic = 0;
        glyph_props[i].isZeroWidthSpace = 0;
        glyph_props[i].reserved = 0;

        /* FIXME: have the shaping engine set this */
        text_props[i].isShapedAlone = 0;
        text_props[i].reserved = 0;

        clustermap[i] = i;
    }

    for (; i < max_glyph_count; i++) {
        glyph_props[i].justification = SCRIPT_JUSTIFY_NONE;
        glyph_props[i].isClusterStart = 0;
        glyph_props[i].isDiacritic = 0;
        glyph_props[i].isZeroWidthSpace = 0;
        glyph_props[i].reserved = 0;
    }

    string = heap_alloc(sizeof(WCHAR)*length);
    if (!string)
        return E_OUTOFMEMORY;

    for (i = 0, g = 0, update_cluster = FALSE; i < length; i++) {
        UINT32 codepoint;

        if (!update_cluster) {
            codepoint = decode_surrogate_pair(text, i, length);
            if (!codepoint) {
                codepoint = is_rtl ? bidi_get_mirrored_char(text[i]) : text[i];
                string[i] = codepoint;
            }
            else {
                string[i] = text[i];
                string[i+1] = text[i+1];
                update_cluster = TRUE;
            }

            hr = IDWriteFontFace_GetGlyphIndices(fontface, &codepoint, 1, &glyph_indices[g]);
            if (FAILED(hr))
                goto done;

            g++;
        }
        else {
            INT32 k;

            update_cluster = FALSE;
            /* mark surrogate halves with same cluster */
            clustermap[i] = clustermap[i-1];
            /* update following clusters */
            for (k = i + 1; k >= 0 && k < length; k++)
                clustermap[k]--;
        }
    }
    *actual_glyph_count = g;

    hr = create_scriptshaping_cache(fontface, &cache);
    if (FAILED(hr))
        goto done;

    context.cache = cache;
    context.text = text;
    context.length = length;
    context.is_rtl = is_rtl;
    context.max_glyph_count = max_glyph_count;
    context.language_tag = DWRITE_MAKE_OPENTYPE_TAG('d','f','l','t');
    if (locale) {
        WCHAR tag[5];
        if (GetLocaleInfoEx(locale, LOCALE_SOPENTYPELANGUAGETAG, tag, sizeof(tag)/sizeof(WCHAR)))
            context.language_tag = DWRITE_MAKE_OPENTYPE_TAG(tag[0],tag[1],tag[2],tag[3]);
    }

    scriptprops = &dwritescripts_properties[script];
    if (scriptprops->ops && scriptprops->ops->contextual_shaping) {
        hr = scriptprops->ops->contextual_shaping(&context, clustermap, glyph_indices, actual_glyph_count);
        if (FAILED(hr))
            goto done;
    }

    /* FIXME: apply default features */

    if (scriptprops->ops && scriptprops->ops->set_text_glyphs_props)
        hr = scriptprops->ops->set_text_glyphs_props(&context, clustermap, glyph_indices, *actual_glyph_count, text_props, glyph_props);
    else
        hr = default_shaping_ops.set_text_glyphs_props(&context, clustermap, glyph_indices, *actual_glyph_count, text_props, glyph_props);

done:
    release_scriptshaping_cache(cache);
    heap_free(string);

    return hr;
}

static inline FLOAT get_scaled_advance_width(INT32 advance, FLOAT emSize, const DWRITE_FONT_METRICS *metrics)
{
    return (FLOAT)advance * emSize / (FLOAT)metrics->designUnitsPerEm;
}

static HRESULT WINAPI dwritetextanalyzer_GetGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* props,
    UINT32 text_len, UINT16 const* glyphs, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace *fontface, FLOAT emSize, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_len, UINT32 feature_ranges, FLOAT *advances, DWRITE_GLYPH_OFFSET *offsets)
{
    DWRITE_FONT_METRICS metrics;
    IDWriteFontFace1 *fontface1;
    HRESULT hr;
    UINT32 i;

    TRACE("(%s %p %p %u %p %p %u %p %.2f %d %d %p %s %p %p %u %p %p)\n", debugstr_w(text),
        clustermap, props, text_len, glyphs, glyph_props, glyph_count, fontface, emSize, is_sideways,
        is_rtl, analysis, debugstr_w(locale), features, feature_range_len, feature_ranges, advances, offsets);

    if (glyph_count == 0)
        return S_OK;

    hr = IDWriteFontFace_QueryInterface(fontface, &IID_IDWriteFontFace1, (void**)&fontface1);
    if (FAILED(hr)) {
        WARN("failed to get IDWriteFontFace1.\n");
        return hr;
    }

    IDWriteFontFace_GetMetrics(fontface, &metrics);
    for (i = 0; i < glyph_count; i++) {
        INT32 a;

        hr = IDWriteFontFace1_GetDesignGlyphAdvances(fontface1, 1, &glyphs[i], &a, is_sideways);
        if (FAILED(hr))
            a = 0;

        advances[i] = get_scaled_advance_width(a, emSize, &metrics);
        offsets[i].advanceOffset = 0.0;
        offsets[i].ascenderOffset = 0.0;
    }

    /* FIXME: actually apply features */
    return S_OK;
}

static HRESULT WINAPI dwritetextanalyzer_GetGdiCompatibleGlyphPlacements(IDWriteTextAnalyzer2 *iface,
    WCHAR const* text, UINT16 const* clustermap, DWRITE_SHAPING_TEXT_PROPERTIES* props,
    UINT32 text_len, UINT16 const* glyph_indices, DWRITE_SHAPING_GLYPH_PROPERTIES const* glyph_props,
    UINT32 glyph_count, IDWriteFontFace * font_face, FLOAT fontEmSize, FLOAT pixels_per_dip,
    DWRITE_MATRIX const* transform, BOOL use_gdi_natural, BOOL is_sideways, BOOL is_rtl,
    DWRITE_SCRIPT_ANALYSIS const* analysis, WCHAR const* locale, DWRITE_TYPOGRAPHIC_FEATURES const** features,
    UINT32 const* feature_range_lengths, UINT32 feature_ranges, FLOAT* glyph_advances, DWRITE_GLYPH_OFFSET* glyph_offsets)
{
    FIXME("(%s %p %p %u %p %p %u %p %f %f %p %d %d %d %p %s %p %p %u %p %p): stub\n", debugstr_w(text),
        clustermap, props, text_len, glyph_indices, glyph_props, glyph_count, font_face, fontEmSize, pixels_per_dip,
        transform, use_gdi_natural, is_sideways, is_rtl, analysis, debugstr_w(locale), features, feature_range_lengths,
        feature_ranges, glyph_advances, glyph_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_ApplyCharacterSpacing(IDWriteTextAnalyzer2 *iface,
    FLOAT leading_spacing, FLOAT trailing_spacing, FLOAT min_advance_width, UINT32 len,
    UINT32 glyph_count, UINT16 const *clustermap, FLOAT const *advances, DWRITE_GLYPH_OFFSET const *offsets,
    DWRITE_SHAPING_GLYPH_PROPERTIES const *props, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    FIXME("(%.2f %.2f %.2f %u %u %p %p %p %p %p %p): stub\n", leading_spacing, trailing_spacing, min_advance_width,
        len, glyph_count, clustermap, advances, offsets, props, modified_advances, modified_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetBaseline(IDWriteTextAnalyzer2 *iface, IDWriteFontFace *face,
    DWRITE_BASELINE baseline, BOOL vertical, BOOL is_simulation_allowed, DWRITE_SCRIPT_ANALYSIS sa,
    const WCHAR *localeName, INT32 *baseline_coord, BOOL *exists)
{
    FIXME("(%p %d %d %u %s %p %p): stub\n", face, vertical, is_simulation_allowed, sa.script, debugstr_w(localeName),
        baseline_coord, exists);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation(IDWriteTextAnalyzer2 *iface,
    IDWriteTextAnalysisSource1* source, UINT32 text_pos, UINT32 len, IDWriteTextAnalysisSink1 *sink)
{
    FIXME("(%p %u %u %p): stub\n", source, text_pos, len, sink);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, DWRITE_MATRIX *transform)
{
    FIXME("(%d %d %p): stub\n", angle, is_sideways, transform);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetScriptProperties(IDWriteTextAnalyzer2 *iface, DWRITE_SCRIPT_ANALYSIS sa,
    DWRITE_SCRIPT_PROPERTIES *props)
{
    TRACE("(%u %p)\n", sa.script, props);

    if (sa.script > Script_LastId)
        return E_INVALIDARG;

    *props = dwritescripts_properties[sa.script].props;
    return S_OK;
}

static inline BOOL is_char_from_simple_script(WCHAR c)
{
    if (IS_HIGH_SURROGATE(c) || IS_LOW_SURROGATE(c))
        return FALSE;
    else {
        UINT16 script = get_char_script(c);
        return !dwritescripts_properties[script].is_complex;
    }
}

static HRESULT WINAPI dwritetextanalyzer1_GetTextComplexity(IDWriteTextAnalyzer2 *iface, const WCHAR *text,
    UINT32 len, IDWriteFontFace *face, BOOL *is_simple, UINT32 *len_read, UINT16 *indices)
{
    HRESULT hr = S_OK;
    int i;

    TRACE("(%s:%u %p %p %p %p)\n", debugstr_wn(text, len), len, face, is_simple, len_read, indices);

    *is_simple = FALSE;
    *len_read = 0;

    if (!face)
        return E_INVALIDARG;

    if (len == 0) {
        *is_simple = TRUE;
        return S_OK;
    }

    *is_simple = text[0] && is_char_from_simple_script(text[0]);
    for (i = 1; i < len && text[i]; i++) {
        if (is_char_from_simple_script(text[i])) {
            if (!*is_simple)
                break;
        }
        else
            *is_simple = FALSE;
    }

    *len_read = i;

    /* fetch indices */
    if (*is_simple && indices) {
        UINT32 *codepoints = heap_alloc(*len_read*sizeof(UINT32));
        if (!codepoints)
            return E_OUTOFMEMORY;

        for (i = 0; i < *len_read; i++)
            codepoints[i] = text[i];

        hr = IDWriteFontFace_GetGlyphIndices(face, codepoints, *len_read, indices);
        heap_free(codepoints);
    }

    return hr;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustificationOpportunities(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length, UINT32 glyph_count,
    const WCHAR *text, const UINT16 *clustermap, const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, DWRITE_JUSTIFICATION_OPPORTUNITY *jo)
{
    FIXME("(%p %.2f %u %u %u %s %p %p %p): stub\n", face, font_em_size, sa.script, length, glyph_count,
        debugstr_w(text), clustermap, prop, jo);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_JustifyGlyphAdvances(IDWriteTextAnalyzer2 *iface,
    FLOAT width, UINT32 glyph_count, const DWRITE_JUSTIFICATION_OPPORTUNITY *jo, const FLOAT *advances,
    const DWRITE_GLYPH_OFFSET *offsets, FLOAT *justifiedadvances, DWRITE_GLYPH_OFFSET *justifiedoffsets)
{
    FIXME("(%.2f %u %p %p %p %p %p): stub\n", width, glyph_count, jo, advances, offsets, justifiedadvances,
        justifiedoffsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer1_GetJustifiedGlyphs(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, FLOAT font_em_size, DWRITE_SCRIPT_ANALYSIS sa, UINT32 length,
    UINT32 glyph_count, UINT32 max_glyphcount, const UINT16 *clustermap, const UINT16 *indices,
    const FLOAT *advances, const FLOAT *justifiedadvances, const DWRITE_GLYPH_OFFSET *justifiedoffsets,
    const DWRITE_SHAPING_GLYPH_PROPERTIES *prop, UINT32 *actual_count, UINT16 *modified_clustermap,
    UINT16 *modified_indices, FLOAT *modified_advances, DWRITE_GLYPH_OFFSET *modified_offsets)
{
    FIXME("(%p %.2f %u %u %u %u %p %p %p %p %p %p %p %p %p %p %p): stub\n", face, font_em_size, sa.script,
        length, glyph_count, max_glyphcount, clustermap, indices, advances, justifiedadvances, justifiedoffsets,
        prop, actual_count, modified_clustermap, modified_indices, modified_advances, modified_offsets);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_GetGlyphOrientationTransform(IDWriteTextAnalyzer2 *iface,
    DWRITE_GLYPH_ORIENTATION_ANGLE angle, BOOL is_sideways, FLOAT originX, FLOAT originY, DWRITE_MATRIX *transform)
{
    FIXME("(%d %d %.2f %.2f %p): stub\n", angle, is_sideways, originX, originY, transform);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_GetTypographicFeatures(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *localeName,
    UINT32 max_tagcount, UINT32 *actual_tagcount, DWRITE_FONT_FEATURE_TAG *tags)
{
    FIXME("(%p %u %s %u %p %p): stub\n", face, sa.script, debugstr_w(localeName), max_tagcount, actual_tagcount,
        tags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dwritetextanalyzer2_CheckTypographicFeature(IDWriteTextAnalyzer2 *iface,
    IDWriteFontFace *face, DWRITE_SCRIPT_ANALYSIS sa, const WCHAR *localeName,
    DWRITE_FONT_FEATURE_TAG feature, UINT32 glyph_count, const UINT16 *indices, UINT8 *feature_applies)
{
    FIXME("(%p %u %s %x %u %p %p): stub\n", face, sa.script, debugstr_w(localeName), feature, glyph_count, indices,
        feature_applies);
    return E_NOTIMPL;
}

static const struct IDWriteTextAnalyzer2Vtbl textanalyzervtbl = {
    dwritetextanalyzer_QueryInterface,
    dwritetextanalyzer_AddRef,
    dwritetextanalyzer_Release,
    dwritetextanalyzer_AnalyzeScript,
    dwritetextanalyzer_AnalyzeBidi,
    dwritetextanalyzer_AnalyzeNumberSubstitution,
    dwritetextanalyzer_AnalyzeLineBreakpoints,
    dwritetextanalyzer_GetGlyphs,
    dwritetextanalyzer_GetGlyphPlacements,
    dwritetextanalyzer_GetGdiCompatibleGlyphPlacements,
    dwritetextanalyzer1_ApplyCharacterSpacing,
    dwritetextanalyzer1_GetBaseline,
    dwritetextanalyzer1_AnalyzeVerticalGlyphOrientation,
    dwritetextanalyzer1_GetGlyphOrientationTransform,
    dwritetextanalyzer1_GetScriptProperties,
    dwritetextanalyzer1_GetTextComplexity,
    dwritetextanalyzer1_GetJustificationOpportunities,
    dwritetextanalyzer1_JustifyGlyphAdvances,
    dwritetextanalyzer1_GetJustifiedGlyphs,
    dwritetextanalyzer2_GetGlyphOrientationTransform,
    dwritetextanalyzer2_GetTypographicFeatures,
    dwritetextanalyzer2_CheckTypographicFeature
};

static IDWriteTextAnalyzer2 textanalyzer = { &textanalyzervtbl };

HRESULT get_textanalyzer(IDWriteTextAnalyzer **ret)
{
    *ret = (IDWriteTextAnalyzer*)&textanalyzer;
    return S_OK;
}

static HRESULT WINAPI dwritenumbersubstitution_QueryInterface(IDWriteNumberSubstitution *iface, REFIID riid, void **obj)
{
    struct dwrite_numbersubstitution *This = impl_from_IDWriteNumberSubstitution(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDWriteNumberSubstitution) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDWriteNumberSubstitution_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI dwritenumbersubstitution_AddRef(IDWriteNumberSubstitution *iface)
{
    struct dwrite_numbersubstitution *This = impl_from_IDWriteNumberSubstitution(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dwritenumbersubstitution_Release(IDWriteNumberSubstitution *iface)
{
    struct dwrite_numbersubstitution *This = impl_from_IDWriteNumberSubstitution(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref) {
        heap_free(This->locale);
        heap_free(This);
    }

    return ref;
}

static const struct IDWriteNumberSubstitutionVtbl numbersubstitutionvtbl = {
    dwritenumbersubstitution_QueryInterface,
    dwritenumbersubstitution_AddRef,
    dwritenumbersubstitution_Release
};

HRESULT create_numbersubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD method, const WCHAR *locale,
    BOOL ignore_user_override, IDWriteNumberSubstitution **ret)
{
    struct dwrite_numbersubstitution *substitution;

    *ret = NULL;

    if ((UINT32)method > DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL)
        return E_INVALIDARG;

    if (method != DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE && !IsValidLocaleName(locale))
        return E_INVALIDARG;

    substitution = heap_alloc(sizeof(*substitution));
    if (!substitution)
        return E_OUTOFMEMORY;

    substitution->IDWriteNumberSubstitution_iface.lpVtbl = &numbersubstitutionvtbl;
    substitution->ref = 1;
    substitution->ignore_user_override = ignore_user_override;
    substitution->method = method;
    substitution->locale = heap_strdupW(locale);
    if (locale && !substitution->locale) {
        heap_free(substitution);
        return E_OUTOFMEMORY;
    }

    *ret = &substitution->IDWriteNumberSubstitution_iface;
    return S_OK;
}
