/*******************************************************************************
 *
 *	Font metric data for ITC Bookman Light Italic
 *
 *	Copyright 2001 Ian Pilcher
 *
 *
 *	See dlls/wineps/data/COPYRIGHTS for font copyright information.
 *
 */

#include "psdrv.h"
#include "data/agl.h"


/*
 *  Glyph metrics
 */

static const AFMMETRICS metrics[228] =
{
    {  32, 0x0020,  300, GN_space },
    {  33, 0x0021,  320, GN_exclam },
    {  34, 0x0022,  360, GN_quotedbl },
    {  35, 0x0023,  620, GN_numbersign },
    {  36, 0x0024,  620, GN_dollar },
    {  37, 0x0025,  800, GN_percent },
    {  38, 0x0026,  820, GN_ampersand },
    { 169, 0x0027,  200, GN_quotesingle },
    {  40, 0x0028,  280, GN_parenleft },
    {  41, 0x0029,  280, GN_parenright },
    {  42, 0x002a,  440, GN_asterisk },
    {  43, 0x002b,  600, GN_plus },
    {  44, 0x002c,  300, GN_comma },
    {  45, 0x002d,  320, GN_hyphen },
    {  46, 0x002e,  300, GN_period },
    {  47, 0x002f,  600, GN_slash },
    {  48, 0x0030,  620, GN_zero },
    {  49, 0x0031,  620, GN_one },
    {  50, 0x0032,  620, GN_two },
    {  51, 0x0033,  620, GN_three },
    {  52, 0x0034,  620, GN_four },
    {  53, 0x0035,  620, GN_five },
    {  54, 0x0036,  620, GN_six },
    {  55, 0x0037,  620, GN_seven },
    {  56, 0x0038,  620, GN_eight },
    {  57, 0x0039,  620, GN_nine },
    {  58, 0x003a,  300, GN_colon },
    {  59, 0x003b,  300, GN_semicolon },
    {  60, 0x003c,  600, GN_less },
    {  61, 0x003d,  600, GN_equal },
    {  62, 0x003e,  600, GN_greater },
    {  63, 0x003f,  540, GN_question },
    {  64, 0x0040,  780, GN_at },
    {  65, 0x0041,  700, GN_A },
    {  66, 0x0042,  720, GN_B },
    {  67, 0x0043,  720, GN_C },
    {  68, 0x0044,  740, GN_D },
    {  69, 0x0045,  680, GN_E },
    {  70, 0x0046,  620, GN_F },
    {  71, 0x0047,  760, GN_G },
    {  72, 0x0048,  800, GN_H },
    {  73, 0x0049,  320, GN_I },
    {  74, 0x004a,  560, GN_J },
    {  75, 0x004b,  720, GN_K },
    {  76, 0x004c,  580, GN_L },
    {  77, 0x004d,  860, GN_M },
    {  78, 0x004e,  720, GN_N },
    {  79, 0x004f,  760, GN_O },
    {  80, 0x0050,  600, GN_P },
    {  81, 0x0051,  780, GN_Q },
    {  82, 0x0052,  700, GN_R },
    {  83, 0x0053,  640, GN_S },
    {  84, 0x0054,  600, GN_T },
    {  85, 0x0055,  720, GN_U },
    {  86, 0x0056,  680, GN_V },
    {  87, 0x0057,  960, GN_W },
    {  88, 0x0058,  700, GN_X },
    {  89, 0x0059,  660, GN_Y },
    {  90, 0x005a,  580, GN_Z },
    {  91, 0x005b,  260, GN_bracketleft },
    {  92, 0x005c,  600, GN_backslash },
    {  93, 0x005d,  260, GN_bracketright },
    {  94, 0x005e,  600, GN_asciicircum },
    {  95, 0x005f,  500, GN_underscore },
    { 193, 0x0060,  340, GN_grave },
    {  97, 0x0061,  620, GN_a },
    {  98, 0x0062,  600, GN_b },
    {  99, 0x0063,  480, GN_c },
    { 100, 0x0064,  640, GN_d },
    { 101, 0x0065,  540, GN_e },
    { 102, 0x0066,  340, GN_f },
    { 103, 0x0067,  560, GN_g },
    { 104, 0x0068,  620, GN_h },
    { 105, 0x0069,  280, GN_i },
    { 106, 0x006a,  280, GN_j },
    { 107, 0x006b,  600, GN_k },
    { 108, 0x006c,  280, GN_l },
    { 109, 0x006d,  880, GN_m },
    { 110, 0x006e,  620, GN_n },
    { 111, 0x006f,  540, GN_o },
    { 112, 0x0070,  600, GN_p },
    { 113, 0x0071,  560, GN_q },
    { 114, 0x0072,  400, GN_r },
    { 115, 0x0073,  540, GN_s },
    { 116, 0x0074,  340, GN_t },
    { 117, 0x0075,  620, GN_u },
    { 118, 0x0076,  540, GN_v },
    { 119, 0x0077,  880, GN_w },
    { 120, 0x0078,  540, GN_x },
    { 121, 0x0079,  600, GN_y },
    { 122, 0x007a,  520, GN_z },
    { 123, 0x007b,  360, GN_braceleft },
    { 124, 0x007c,  600, GN_bar },
    { 125, 0x007d,  380, GN_braceright },
    { 126, 0x007e,  600, GN_asciitilde },
    { 161, 0x00a1,  320, GN_exclamdown },
    { 162, 0x00a2,  620, GN_cent },
    { 163, 0x00a3,  620, GN_sterling },
    { 168, 0x00a4,  620, GN_currency },
    { 165, 0x00a5,  620, GN_yen },
    {  -1, 0x00a6,  600, GN_brokenbar },
    { 167, 0x00a7,  620, GN_section },
    { 200, 0x00a8,  420, GN_dieresis },
    {  -1, 0x00a9,  740, GN_copyright },
    { 227, 0x00aa,  440, GN_ordfeminine },
    { 171, 0x00ab,  300, GN_guillemotleft },
    {  -1, 0x00ac,  600, GN_logicalnot },
    {  -1, 0x00ae,  740, GN_registered },
    {  -1, 0x00b0,  400, GN_degree },
    {  -1, 0x00b1,  600, GN_plusminus },
    {  -1, 0x00b2,  372, GN_twosuperior },
    {  -1, 0x00b3,  372, GN_threesuperior },
    { 194, 0x00b4,  320, GN_acute },
    { 182, 0x00b6,  620, GN_paragraph },
    { 180, 0x00b7,  300, GN_periodcentered },
    { 203, 0x00b8,  320, GN_cedilla },
    {  -1, 0x00b9,  372, GN_onesuperior },
    { 235, 0x00ba,  400, GN_ordmasculine },
    { 187, 0x00bb,  300, GN_guillemotright },
    {  -1, 0x00bc,  930, GN_onequarter },
    {  -1, 0x00bd,  930, GN_onehalf },
    {  -1, 0x00be,  930, GN_threequarters },
    { 191, 0x00bf,  540, GN_questiondown },
    {  -1, 0x00c0,  700, GN_Agrave },
    {  -1, 0x00c1,  700, GN_Aacute },
    {  -1, 0x00c2,  700, GN_Acircumflex },
    {  -1, 0x00c3,  700, GN_Atilde },
    {  -1, 0x00c4,  700, GN_Adieresis },
    {  -1, 0x00c5,  700, GN_Aring },
    { 225, 0x00c6, 1220, GN_AE },
    {  -1, 0x00c7,  720, GN_Ccedilla },
    {  -1, 0x00c8,  680, GN_Egrave },
    {  -1, 0x00c9,  680, GN_Eacute },
    {  -1, 0x00ca,  680, GN_Ecircumflex },
    {  -1, 0x00cb,  680, GN_Edieresis },
    {  -1, 0x00cc,  320, GN_Igrave },
    {  -1, 0x00cd,  320, GN_Iacute },
    {  -1, 0x00ce,  320, GN_Icircumflex },
    {  -1, 0x00cf,  320, GN_Idieresis },
    {  -1, 0x00d0,  740, GN_Eth },
    {  -1, 0x00d1,  720, GN_Ntilde },
    {  -1, 0x00d2,  760, GN_Ograve },
    {  -1, 0x00d3,  760, GN_Oacute },
    {  -1, 0x00d4,  760, GN_Ocircumflex },
    {  -1, 0x00d5,  760, GN_Otilde },
    {  -1, 0x00d6,  760, GN_Odieresis },
    {  -1, 0x00d7,  600, GN_multiply },
    { 233, 0x00d8,  760, GN_Oslash },
    {  -1, 0x00d9,  720, GN_Ugrave },
    {  -1, 0x00da,  720, GN_Uacute },
    {  -1, 0x00db,  720, GN_Ucircumflex },
    {  -1, 0x00dc,  720, GN_Udieresis },
    {  -1, 0x00dd,  660, GN_Yacute },
    {  -1, 0x00de,  600, GN_Thorn },
    { 251, 0x00df,  620, GN_germandbls },
    {  -1, 0x00e0,  620, GN_agrave },
    {  -1, 0x00e1,  620, GN_aacute },
    {  -1, 0x00e2,  620, GN_acircumflex },
    {  -1, 0x00e3,  620, GN_atilde },
    {  -1, 0x00e4,  620, GN_adieresis },
    {  -1, 0x00e5,  620, GN_aring },
    { 241, 0x00e6,  880, GN_ae },
    {  -1, 0x00e7,  480, GN_ccedilla },
    {  -1, 0x00e8,  540, GN_egrave },
    {  -1, 0x00e9,  540, GN_eacute },
    {  -1, 0x00ea,  540, GN_ecircumflex },
    {  -1, 0x00eb,  540, GN_edieresis },
    {  -1, 0x00ec,  280, GN_igrave },
    {  -1, 0x00ed,  280, GN_iacute },
    {  -1, 0x00ee,  280, GN_icircumflex },
    {  -1, 0x00ef,  280, GN_idieresis },
    {  -1, 0x00f0,  540, GN_eth },
    {  -1, 0x00f1,  620, GN_ntilde },
    {  -1, 0x00f2,  540, GN_ograve },
    {  -1, 0x00f3,  540, GN_oacute },
    {  -1, 0x00f4,  540, GN_ocircumflex },
    {  -1, 0x00f5,  540, GN_otilde },
    {  -1, 0x00f6,  540, GN_odieresis },
    {  -1, 0x00f7,  600, GN_divide },
    { 249, 0x00f8,  540, GN_oslash },
    {  -1, 0x00f9,  620, GN_ugrave },
    {  -1, 0x00fa,  620, GN_uacute },
    {  -1, 0x00fb,  620, GN_ucircumflex },
    {  -1, 0x00fc,  620, GN_udieresis },
    {  -1, 0x00fd,  600, GN_yacute },
    {  -1, 0x00fe,  600, GN_thorn },
    {  -1, 0x00ff,  600, GN_ydieresis },
    { 245, 0x0131,  280, GN_dotlessi },
    { 232, 0x0141,  580, GN_Lslash },
    { 248, 0x0142,  340, GN_lslash },
    { 234, 0x0152, 1180, GN_OE },
    { 250, 0x0153,  900, GN_oe },
    {  -1, 0x0160,  640, GN_Scaron },
    {  -1, 0x0161,  540, GN_scaron },
    {  -1, 0x0178,  660, GN_Ydieresis },
    {  -1, 0x017d,  580, GN_Zcaron },
    {  -1, 0x017e,  520, GN_zcaron },
    { 166, 0x0192,  620, GN_florin },
    { 195, 0x02c6,  440, GN_circumflex },
    { 207, 0x02c7,  440, GN_caron },
    { 197, 0x02c9,  440, GN_macron },
    { 198, 0x02d8,  440, GN_breve },
    { 199, 0x02d9,  260, GN_dotaccent },
    { 202, 0x02da,  300, GN_ring },
    { 206, 0x02db,  260, GN_ogonek },
    { 196, 0x02dc,  440, GN_tilde },
    { 205, 0x02dd,  340, GN_hungarumlaut },
    {  -1, 0x03bc,  620, GN_mu },
    { 177, 0x2013,  500, GN_endash },
    { 208, 0x2014, 1000, GN_emdash },
    {  96, 0x2018,  280, GN_quoteleft },
    {  39, 0x2019,  280, GN_quoteright },
    { 184, 0x201a,  320, GN_quotesinglbase },
    { 170, 0x201c,  440, GN_quotedblleft },
    { 186, 0x201d,  440, GN_quotedblright },
    { 185, 0x201e,  480, GN_quotedblbase },
    { 178, 0x2020,  620, GN_dagger },
    { 179, 0x2021,  620, GN_daggerdbl },
    { 183, 0x2022,  460, GN_bullet },
    { 188, 0x2026, 1000, GN_ellipsis },
    { 189, 0x2030, 1180, GN_perthousand },
    { 172, 0x2039,  180, GN_guilsinglleft },
    { 173, 0x203a,  180, GN_guilsinglright },
    {  -1, 0x2122,  980, GN_trademark },
    {  -1, 0x2212,  600, GN_minus },
    { 164, 0x2215,   20, GN_fraction },
    { 174, 0xfb01,  640, GN_fi },
    { 175, 0xfb02,  660, GN_fl }
};


/*
 *  Font metrics
 */

const AFM PSDRV_Bookman_LightItalic =
{
    "Bookman-LightItalic",		    /* FontName */
    L"ITC Bookman Light Italic",	    /* FullName */
    L"ITC Bookman",			    /* FamilyName */
    L"AdobeStandardEncoding",		    /* EncodingScheme */
    FW_NORMAL,				    /* Weight */
    -10,				    /* ItalicAngle */
    FALSE,				    /* IsFixedPitch */
    -100,				    /* UnderlinePosition */
    50,					    /* UnderlineThickness */
    { -228, -250, 1269, 883 },		    /* FontBBox */
    717,				    /* Ascender */
    -212,				    /* Descender */
    {
	1000,				    /* WinMetrics.usUnitsPerEm */
	942,				    /* WinMetrics.sAscender */
	-232,				    /* WinMetrics.sDescender */
	0,				    /* WinMetrics.sLineGap */
	482,				    /* WinMetrics.sAvgCharWidth */
	716,				    /* WinMetrics.sTypoAscender */
	-230,				    /* WinMetrics.sTypoDescender */
	123,				    /* WinMetrics.sTypoLineGap */
	875,				    /* WinMetrics.usWinAscent */
	232				    /* WinMetrics.usWinDescent */
    },
    228,				    /* NumofMetrics */
    metrics				    /* Metrics */
};
