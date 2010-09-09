/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2010 CodeWeavers, Aric Stewart
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
 */

#define Script_Latin   1
#define Script_CR      2
#define Script_Numeric 3
#define Script_Control 4
#define Script_Punctuation 5
#define Script_Arabic  6
#define Script_Arabic_Numeric  7
#define Script_Hebrew  8
#define Script_Syriac  9
#define Script_Persian 10
#define Script_Thaana  11
#define Script_Greek   12
#define Script_Cyrillic   13
#define Script_Armenian 14
#define Script_Georgian 15
/* Unicode Chapter 10 */
#define Script_Sinhala 16
#define Script_Tibetan 17
#define Script_Tibetan_Numeric 18
#define Script_Phags_pa 19
/* Unicode Chapter 11 */
#define Script_Thai 20
#define Script_Thai_Numeric 21
#define Script_Lao 22
#define Script_Lao_Numeric 23

#define GLYPH_BLOCK_SHIFT 8
#define GLYPH_BLOCK_SIZE  (1UL << GLYPH_BLOCK_SHIFT)
#define GLYPH_BLOCK_MASK  (GLYPH_BLOCK_SIZE - 1)
#define GLYPH_MAX         65536

typedef struct {
    char    tag[4];
    LPCVOID  feature;
} LoadedFeature;

typedef struct {
    LOGFONTW lf;
    TEXTMETRICW tm;
    WORD *glyphs[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    ABC *widths[GLYPH_MAX / GLYPH_BLOCK_SIZE];
    LPVOID GSUB_Table;
    INT feature_count;
    LoadedFeature *features;
} ScriptCache;

#define odd(x) ((x) & 1)

BOOL BIDI_DetermineLevels( LPCWSTR lpString, INT uCount, const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c, WORD *lpOutLevels );
BOOL BIDI_GetStrengths(LPCWSTR lpString, INT uCount, const SCRIPT_CONTROL *c,
                      WORD* lpStrength);
INT BIDI_ReorderV2lLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse);
INT BIDI_ReorderL2vLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse);
void SHAPE_ContextualShaping(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, WORD *pwLogClust);
void SHAPE_ApplyDefaultOpentypeFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WORD* pwOutGlyphs, INT* pcGlyphs, INT cMaxGlyphs, INT cChars, WORD *pwLogClust);
HRESULT SHAPE_CheckFontForRequiredFeatures(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa);
