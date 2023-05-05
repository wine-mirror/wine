/*******************************************************************************
 *
 *	Font metric data for New Century Schoolbook Bold Italic
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
    {  32, 0x0020,  287, GN_space },
    {  33, 0x0021,  333, GN_exclam },
    {  34, 0x0022,  400, GN_quotedbl },
    {  35, 0x0023,  574, GN_numbersign },
    {  36, 0x0024,  574, GN_dollar },
    {  37, 0x0025,  889, GN_percent },
    {  38, 0x0026,  889, GN_ampersand },
    { 169, 0x0027,  287, GN_quotesingle },
    {  40, 0x0028,  407, GN_parenleft },
    {  41, 0x0029,  407, GN_parenright },
    {  42, 0x002a,  500, GN_asterisk },
    {  43, 0x002b,  606, GN_plus },
    {  44, 0x002c,  287, GN_comma },
    {  45, 0x002d,  333, GN_hyphen },
    {  46, 0x002e,  287, GN_period },
    {  47, 0x002f,  278, GN_slash },
    {  48, 0x0030,  574, GN_zero },
    {  49, 0x0031,  574, GN_one },
    {  50, 0x0032,  574, GN_two },
    {  51, 0x0033,  574, GN_three },
    {  52, 0x0034,  574, GN_four },
    {  53, 0x0035,  574, GN_five },
    {  54, 0x0036,  574, GN_six },
    {  55, 0x0037,  574, GN_seven },
    {  56, 0x0038,  574, GN_eight },
    {  57, 0x0039,  574, GN_nine },
    {  58, 0x003a,  287, GN_colon },
    {  59, 0x003b,  287, GN_semicolon },
    {  60, 0x003c,  606, GN_less },
    {  61, 0x003d,  606, GN_equal },
    {  62, 0x003e,  606, GN_greater },
    {  63, 0x003f,  481, GN_question },
    {  64, 0x0040,  747, GN_at },
    {  65, 0x0041,  741, GN_A },
    {  66, 0x0042,  759, GN_B },
    {  67, 0x0043,  759, GN_C },
    {  68, 0x0044,  833, GN_D },
    {  69, 0x0045,  741, GN_E },
    {  70, 0x0046,  704, GN_F },
    {  71, 0x0047,  815, GN_G },
    {  72, 0x0048,  870, GN_H },
    {  73, 0x0049,  444, GN_I },
    {  74, 0x004a,  667, GN_J },
    {  75, 0x004b,  778, GN_K },
    {  76, 0x004c,  704, GN_L },
    {  77, 0x004d,  944, GN_M },
    {  78, 0x004e,  852, GN_N },
    {  79, 0x004f,  833, GN_O },
    {  80, 0x0050,  741, GN_P },
    {  81, 0x0051,  833, GN_Q },
    {  82, 0x0052,  796, GN_R },
    {  83, 0x0053,  685, GN_S },
    {  84, 0x0054,  722, GN_T },
    {  85, 0x0055,  833, GN_U },
    {  86, 0x0056,  741, GN_V },
    {  87, 0x0057,  944, GN_W },
    {  88, 0x0058,  741, GN_X },
    {  89, 0x0059,  704, GN_Y },
    {  90, 0x005a,  704, GN_Z },
    {  91, 0x005b,  407, GN_bracketleft },
    {  92, 0x005c,  606, GN_backslash },
    {  93, 0x005d,  407, GN_bracketright },
    {  94, 0x005e,  606, GN_asciicircum },
    {  95, 0x005f,  500, GN_underscore },
    { 193, 0x0060,  333, GN_grave },
    {  97, 0x0061,  667, GN_a },
    {  98, 0x0062,  611, GN_b },
    {  99, 0x0063,  537, GN_c },
    { 100, 0x0064,  667, GN_d },
    { 101, 0x0065,  519, GN_e },
    { 102, 0x0066,  389, GN_f },
    { 103, 0x0067,  611, GN_g },
    { 104, 0x0068,  685, GN_h },
    { 105, 0x0069,  389, GN_i },
    { 106, 0x006a,  370, GN_j },
    { 107, 0x006b,  648, GN_k },
    { 108, 0x006c,  389, GN_l },
    { 109, 0x006d,  944, GN_m },
    { 110, 0x006e,  685, GN_n },
    { 111, 0x006f,  574, GN_o },
    { 112, 0x0070,  648, GN_p },
    { 113, 0x0071,  630, GN_q },
    { 114, 0x0072,  519, GN_r },
    { 115, 0x0073,  481, GN_s },
    { 116, 0x0074,  407, GN_t },
    { 117, 0x0075,  685, GN_u },
    { 118, 0x0076,  556, GN_v },
    { 119, 0x0077,  833, GN_w },
    { 120, 0x0078,  574, GN_x },
    { 121, 0x0079,  519, GN_y },
    { 122, 0x007a,  519, GN_z },
    { 123, 0x007b,  407, GN_braceleft },
    { 124, 0x007c,  606, GN_bar },
    { 125, 0x007d,  407, GN_braceright },
    { 126, 0x007e,  606, GN_asciitilde },
    { 161, 0x00a1,  333, GN_exclamdown },
    { 162, 0x00a2,  574, GN_cent },
    { 163, 0x00a3,  574, GN_sterling },
    { 168, 0x00a4,  574, GN_currency },
    { 165, 0x00a5,  574, GN_yen },
    {  -1, 0x00a6,  606, GN_brokenbar },
    { 167, 0x00a7,  500, GN_section },
    { 200, 0x00a8,  333, GN_dieresis },
    {  -1, 0x00a9,  747, GN_copyright },
    { 227, 0x00aa,  412, GN_ordfeminine },
    { 171, 0x00ab,  481, GN_guillemotleft },
    {  -1, 0x00ac,  606, GN_logicalnot },
    {  -1, 0x00ae,  747, GN_registered },
    {  -1, 0x00b0,  400, GN_degree },
    {  -1, 0x00b1,  606, GN_plusminus },
    {  -1, 0x00b2,  344, GN_twosuperior },
    {  -1, 0x00b3,  344, GN_threesuperior },
    { 194, 0x00b4,  333, GN_acute },
    { 182, 0x00b6,  650, GN_paragraph },
    { 180, 0x00b7,  287, GN_periodcentered },
    { 203, 0x00b8,  333, GN_cedilla },
    {  -1, 0x00b9,  344, GN_onesuperior },
    { 235, 0x00ba,  356, GN_ordmasculine },
    { 187, 0x00bb,  481, GN_guillemotright },
    {  -1, 0x00bc,  861, GN_onequarter },
    {  -1, 0x00bd,  861, GN_onehalf },
    {  -1, 0x00be,  861, GN_threequarters },
    { 191, 0x00bf,  481, GN_questiondown },
    {  -1, 0x00c0,  741, GN_Agrave },
    {  -1, 0x00c1,  741, GN_Aacute },
    {  -1, 0x00c2,  741, GN_Acircumflex },
    {  -1, 0x00c3,  741, GN_Atilde },
    {  -1, 0x00c4,  741, GN_Adieresis },
    {  -1, 0x00c5,  741, GN_Aring },
    { 225, 0x00c6,  889, GN_AE },
    {  -1, 0x00c7,  759, GN_Ccedilla },
    {  -1, 0x00c8,  741, GN_Egrave },
    {  -1, 0x00c9,  741, GN_Eacute },
    {  -1, 0x00ca,  741, GN_Ecircumflex },
    {  -1, 0x00cb,  741, GN_Edieresis },
    {  -1, 0x00cc,  444, GN_Igrave },
    {  -1, 0x00cd,  444, GN_Iacute },
    {  -1, 0x00ce,  444, GN_Icircumflex },
    {  -1, 0x00cf,  444, GN_Idieresis },
    {  -1, 0x00d0,  833, GN_Eth },
    {  -1, 0x00d1,  852, GN_Ntilde },
    {  -1, 0x00d2,  833, GN_Ograve },
    {  -1, 0x00d3,  833, GN_Oacute },
    {  -1, 0x00d4,  833, GN_Ocircumflex },
    {  -1, 0x00d5,  833, GN_Otilde },
    {  -1, 0x00d6,  833, GN_Odieresis },
    {  -1, 0x00d7,  606, GN_multiply },
    { 233, 0x00d8,  833, GN_Oslash },
    {  -1, 0x00d9,  833, GN_Ugrave },
    {  -1, 0x00da,  833, GN_Uacute },
    {  -1, 0x00db,  833, GN_Ucircumflex },
    {  -1, 0x00dc,  833, GN_Udieresis },
    {  -1, 0x00dd,  704, GN_Yacute },
    {  -1, 0x00de,  741, GN_Thorn },
    { 251, 0x00df,  574, GN_germandbls },
    {  -1, 0x00e0,  667, GN_agrave },
    {  -1, 0x00e1,  667, GN_aacute },
    {  -1, 0x00e2,  667, GN_acircumflex },
    {  -1, 0x00e3,  667, GN_atilde },
    {  -1, 0x00e4,  667, GN_adieresis },
    {  -1, 0x00e5,  667, GN_aring },
    { 241, 0x00e6,  815, GN_ae },
    {  -1, 0x00e7,  537, GN_ccedilla },
    {  -1, 0x00e8,  519, GN_egrave },
    {  -1, 0x00e9,  519, GN_eacute },
    {  -1, 0x00ea,  519, GN_ecircumflex },
    {  -1, 0x00eb,  519, GN_edieresis },
    {  -1, 0x00ec,  389, GN_igrave },
    {  -1, 0x00ed,  389, GN_iacute },
    {  -1, 0x00ee,  389, GN_icircumflex },
    {  -1, 0x00ef,  389, GN_idieresis },
    {  -1, 0x00f0,  574, GN_eth },
    {  -1, 0x00f1,  685, GN_ntilde },
    {  -1, 0x00f2,  574, GN_ograve },
    {  -1, 0x00f3,  574, GN_oacute },
    {  -1, 0x00f4,  574, GN_ocircumflex },
    {  -1, 0x00f5,  574, GN_otilde },
    {  -1, 0x00f6,  574, GN_odieresis },
    {  -1, 0x00f7,  606, GN_divide },
    { 249, 0x00f8,  574, GN_oslash },
    {  -1, 0x00f9,  685, GN_ugrave },
    {  -1, 0x00fa,  685, GN_uacute },
    {  -1, 0x00fb,  685, GN_ucircumflex },
    {  -1, 0x00fc,  685, GN_udieresis },
    {  -1, 0x00fd,  519, GN_yacute },
    {  -1, 0x00fe,  648, GN_thorn },
    {  -1, 0x00ff,  519, GN_ydieresis },
    { 245, 0x0131,  389, GN_dotlessi },
    { 232, 0x0141,  704, GN_Lslash },
    { 248, 0x0142,  389, GN_lslash },
    { 234, 0x0152,  963, GN_OE },
    { 250, 0x0153,  852, GN_oe },
    {  -1, 0x0160,  685, GN_Scaron },
    {  -1, 0x0161,  481, GN_scaron },
    {  -1, 0x0178,  704, GN_Ydieresis },
    {  -1, 0x017d,  704, GN_Zcaron },
    {  -1, 0x017e,  519, GN_zcaron },
    { 166, 0x0192,  574, GN_florin },
    { 195, 0x02c6,  333, GN_circumflex },
    { 207, 0x02c7,  333, GN_caron },
    { 197, 0x02c9,  333, GN_macron },
    { 198, 0x02d8,  333, GN_breve },
    { 199, 0x02d9,  333, GN_dotaccent },
    { 202, 0x02da,  333, GN_ring },
    { 206, 0x02db,  333, GN_ogonek },
    { 196, 0x02dc,  333, GN_tilde },
    { 205, 0x02dd,  333, GN_hungarumlaut },
    {  -1, 0x03bc,  685, GN_mu },
    { 177, 0x2013,  500, GN_endash },
    { 208, 0x2014, 1000, GN_emdash },
    {  96, 0x2018,  259, GN_quoteleft },
    {  39, 0x2019,  259, GN_quoteright },
    { 184, 0x201a,  259, GN_quotesinglbase },
    { 170, 0x201c,  481, GN_quotedblleft },
    { 186, 0x201d,  481, GN_quotedblright },
    { 185, 0x201e,  481, GN_quotedblbase },
    { 178, 0x2020,  500, GN_dagger },
    { 179, 0x2021,  500, GN_daggerdbl },
    { 183, 0x2022,  606, GN_bullet },
    { 188, 0x2026, 1000, GN_ellipsis },
    { 189, 0x2030, 1167, GN_perthousand },
    { 172, 0x2039,  278, GN_guilsinglleft },
    { 173, 0x203a,  278, GN_guilsinglright },
    {  -1, 0x2122,  950, GN_trademark },
    {  -1, 0x2212,  606, GN_minus },
    { 164, 0x2215,  167, GN_fraction },
    { 174, 0xfb01,  685, GN_fi },
    { 175, 0xfb02,  685, GN_fl }
};


/*
 *  Font metrics
 */

const AFM PSDRV_NewCenturySchlbk_BoldItalic =
{
    "NewCenturySchlbk-BoldItalic",	    /* FontName */
    L"New Century Schoolbook Bold Italic",   /* FullName */
    L"New Century Schoolbook",		    /* FamilyName */
    L"AdobeStandardEncoding",		    /* EncodingScheme */
    FW_BOLD,				    /* Weight */
    -16,				    /* ItalicAngle */
    FALSE,				    /* IsFixedPitch */
    -100,				    /* UnderlinePosition */
    50,					    /* UnderlineThickness */
    { -205, -250, 1147, 991 },		    /* FontBBox */
    737,				    /* Ascender */
    -205,				    /* Descender */
    {
	1000,				    /* WinMetrics.usUnitsPerEm */
	986,				    /* WinMetrics.sAscender */
	-216,				    /* WinMetrics.sDescender */
	0,				    /* WinMetrics.sLineGap */
	515,				    /* WinMetrics.sAvgCharWidth */
	739,				    /* WinMetrics.sTypoAscender */
	-192,				    /* WinMetrics.sTypoDescender */
	138,				    /* WinMetrics.sTypoLineGap */
	985,				    /* WinMetrics.usWinAscent */
	216				    /* WinMetrics.usWinDescent */
    },
    228,				    /* NumofMetrics */
    metrics				    /* Metrics */
};
