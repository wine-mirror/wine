/*******************************************************************************
 *
 *	Font metric data for New Century Schoolbook Bold
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
    {  33, 0x0021,  296, GN_exclam },
    {  34, 0x0022,  333, GN_quotedbl },
    {  35, 0x0023,  574, GN_numbersign },
    {  36, 0x0024,  574, GN_dollar },
    {  37, 0x0025,  833, GN_percent },
    {  38, 0x0026,  852, GN_ampersand },
    { 169, 0x0027,  241, GN_quotesingle },
    {  40, 0x0028,  389, GN_parenleft },
    {  41, 0x0029,  389, GN_parenright },
    {  42, 0x002a,  500, GN_asterisk },
    {  43, 0x002b,  606, GN_plus },
    {  44, 0x002c,  278, GN_comma },
    {  45, 0x002d,  333, GN_hyphen },
    {  46, 0x002e,  278, GN_period },
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
    {  58, 0x003a,  278, GN_colon },
    {  59, 0x003b,  278, GN_semicolon },
    {  60, 0x003c,  606, GN_less },
    {  61, 0x003d,  606, GN_equal },
    {  62, 0x003e,  606, GN_greater },
    {  63, 0x003f,  500, GN_question },
    {  64, 0x0040,  747, GN_at },
    {  65, 0x0041,  759, GN_A },
    {  66, 0x0042,  778, GN_B },
    {  67, 0x0043,  778, GN_C },
    {  68, 0x0044,  833, GN_D },
    {  69, 0x0045,  759, GN_E },
    {  70, 0x0046,  722, GN_F },
    {  71, 0x0047,  833, GN_G },
    {  72, 0x0048,  870, GN_H },
    {  73, 0x0049,  444, GN_I },
    {  74, 0x004a,  648, GN_J },
    {  75, 0x004b,  815, GN_K },
    {  76, 0x004c,  722, GN_L },
    {  77, 0x004d,  981, GN_M },
    {  78, 0x004e,  833, GN_N },
    {  79, 0x004f,  833, GN_O },
    {  80, 0x0050,  759, GN_P },
    {  81, 0x0051,  833, GN_Q },
    {  82, 0x0052,  815, GN_R },
    {  83, 0x0053,  667, GN_S },
    {  84, 0x0054,  722, GN_T },
    {  85, 0x0055,  833, GN_U },
    {  86, 0x0056,  759, GN_V },
    {  87, 0x0057,  981, GN_W },
    {  88, 0x0058,  722, GN_X },
    {  89, 0x0059,  722, GN_Y },
    {  90, 0x005a,  667, GN_Z },
    {  91, 0x005b,  389, GN_bracketleft },
    {  92, 0x005c,  606, GN_backslash },
    {  93, 0x005d,  389, GN_bracketright },
    {  94, 0x005e,  606, GN_asciicircum },
    {  95, 0x005f,  500, GN_underscore },
    { 193, 0x0060,  333, GN_grave },
    {  97, 0x0061,  611, GN_a },
    {  98, 0x0062,  648, GN_b },
    {  99, 0x0063,  556, GN_c },
    { 100, 0x0064,  667, GN_d },
    { 101, 0x0065,  574, GN_e },
    { 102, 0x0066,  389, GN_f },
    { 103, 0x0067,  611, GN_g },
    { 104, 0x0068,  685, GN_h },
    { 105, 0x0069,  370, GN_i },
    { 106, 0x006a,  352, GN_j },
    { 107, 0x006b,  667, GN_k },
    { 108, 0x006c,  352, GN_l },
    { 109, 0x006d,  963, GN_m },
    { 110, 0x006e,  685, GN_n },
    { 111, 0x006f,  611, GN_o },
    { 112, 0x0070,  667, GN_p },
    { 113, 0x0071,  648, GN_q },
    { 114, 0x0072,  519, GN_r },
    { 115, 0x0073,  500, GN_s },
    { 116, 0x0074,  426, GN_t },
    { 117, 0x0075,  685, GN_u },
    { 118, 0x0076,  611, GN_v },
    { 119, 0x0077,  889, GN_w },
    { 120, 0x0078,  611, GN_x },
    { 121, 0x0079,  611, GN_y },
    { 122, 0x007a,  537, GN_z },
    { 123, 0x007b,  389, GN_braceleft },
    { 124, 0x007c,  606, GN_bar },
    { 125, 0x007d,  389, GN_braceright },
    { 126, 0x007e,  606, GN_asciitilde },
    { 161, 0x00a1,  296, GN_exclamdown },
    { 162, 0x00a2,  574, GN_cent },
    { 163, 0x00a3,  574, GN_sterling },
    { 168, 0x00a4,  574, GN_currency },
    { 165, 0x00a5,  574, GN_yen },
    {  -1, 0x00a6,  606, GN_brokenbar },
    { 167, 0x00a7,  500, GN_section },
    { 200, 0x00a8,  333, GN_dieresis },
    {  -1, 0x00a9,  747, GN_copyright },
    { 227, 0x00aa,  367, GN_ordfeminine },
    { 171, 0x00ab,  500, GN_guillemotleft },
    {  -1, 0x00ac,  606, GN_logicalnot },
    {  -1, 0x00ae,  747, GN_registered },
    {  -1, 0x00b0,  400, GN_degree },
    {  -1, 0x00b1,  606, GN_plusminus },
    {  -1, 0x00b2,  344, GN_twosuperior },
    {  -1, 0x00b3,  344, GN_threesuperior },
    { 194, 0x00b4,  333, GN_acute },
    { 182, 0x00b6,  747, GN_paragraph },
    { 180, 0x00b7,  278, GN_periodcentered },
    { 203, 0x00b8,  333, GN_cedilla },
    {  -1, 0x00b9,  344, GN_onesuperior },
    { 235, 0x00ba,  367, GN_ordmasculine },
    { 187, 0x00bb,  500, GN_guillemotright },
    {  -1, 0x00bc,  861, GN_onequarter },
    {  -1, 0x00bd,  861, GN_onehalf },
    {  -1, 0x00be,  861, GN_threequarters },
    { 191, 0x00bf,  500, GN_questiondown },
    {  -1, 0x00c0,  759, GN_Agrave },
    {  -1, 0x00c1,  759, GN_Aacute },
    {  -1, 0x00c2,  759, GN_Acircumflex },
    {  -1, 0x00c3,  759, GN_Atilde },
    {  -1, 0x00c4,  759, GN_Adieresis },
    {  -1, 0x00c5,  759, GN_Aring },
    { 225, 0x00c6,  981, GN_AE },
    {  -1, 0x00c7,  778, GN_Ccedilla },
    {  -1, 0x00c8,  759, GN_Egrave },
    {  -1, 0x00c9,  759, GN_Eacute },
    {  -1, 0x00ca,  759, GN_Ecircumflex },
    {  -1, 0x00cb,  759, GN_Edieresis },
    {  -1, 0x00cc,  444, GN_Igrave },
    {  -1, 0x00cd,  444, GN_Iacute },
    {  -1, 0x00ce,  444, GN_Icircumflex },
    {  -1, 0x00cf,  444, GN_Idieresis },
    {  -1, 0x00d0,  833, GN_Eth },
    {  -1, 0x00d1,  833, GN_Ntilde },
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
    {  -1, 0x00dd,  722, GN_Yacute },
    {  -1, 0x00de,  759, GN_Thorn },
    { 251, 0x00df,  611, GN_germandbls },
    {  -1, 0x00e0,  611, GN_agrave },
    {  -1, 0x00e1,  611, GN_aacute },
    {  -1, 0x00e2,  611, GN_acircumflex },
    {  -1, 0x00e3,  611, GN_atilde },
    {  -1, 0x00e4,  611, GN_adieresis },
    {  -1, 0x00e5,  611, GN_aring },
    { 241, 0x00e6,  870, GN_ae },
    {  -1, 0x00e7,  556, GN_ccedilla },
    {  -1, 0x00e8,  574, GN_egrave },
    {  -1, 0x00e9,  574, GN_eacute },
    {  -1, 0x00ea,  574, GN_ecircumflex },
    {  -1, 0x00eb,  574, GN_edieresis },
    {  -1, 0x00ec,  370, GN_igrave },
    {  -1, 0x00ed,  370, GN_iacute },
    {  -1, 0x00ee,  370, GN_icircumflex },
    {  -1, 0x00ef,  370, GN_idieresis },
    {  -1, 0x00f0,  611, GN_eth },
    {  -1, 0x00f1,  685, GN_ntilde },
    {  -1, 0x00f2,  611, GN_ograve },
    {  -1, 0x00f3,  611, GN_oacute },
    {  -1, 0x00f4,  611, GN_ocircumflex },
    {  -1, 0x00f5,  611, GN_otilde },
    {  -1, 0x00f6,  611, GN_odieresis },
    {  -1, 0x00f7,  606, GN_divide },
    { 249, 0x00f8,  611, GN_oslash },
    {  -1, 0x00f9,  685, GN_ugrave },
    {  -1, 0x00fa,  685, GN_uacute },
    {  -1, 0x00fb,  685, GN_ucircumflex },
    {  -1, 0x00fc,  685, GN_udieresis },
    {  -1, 0x00fd,  611, GN_yacute },
    {  -1, 0x00fe,  667, GN_thorn },
    {  -1, 0x00ff,  611, GN_ydieresis },
    { 245, 0x0131,  370, GN_dotlessi },
    { 232, 0x0141,  722, GN_Lslash },
    { 248, 0x0142,  352, GN_lslash },
    { 234, 0x0152, 1000, GN_OE },
    { 250, 0x0153,  907, GN_oe },
    {  -1, 0x0160,  667, GN_Scaron },
    {  -1, 0x0161,  500, GN_scaron },
    {  -1, 0x0178,  722, GN_Ydieresis },
    {  -1, 0x017d,  667, GN_Zcaron },
    {  -1, 0x017e,  537, GN_zcaron },
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
    {  96, 0x2018,  241, GN_quoteleft },
    {  39, 0x2019,  241, GN_quoteright },
    { 184, 0x201a,  241, GN_quotesinglbase },
    { 170, 0x201c,  481, GN_quotedblleft },
    { 186, 0x201d,  481, GN_quotedblright },
    { 185, 0x201e,  481, GN_quotedblbase },
    { 178, 0x2020,  500, GN_dagger },
    { 179, 0x2021,  500, GN_daggerdbl },
    { 183, 0x2022,  606, GN_bullet },
    { 188, 0x2026, 1000, GN_ellipsis },
    { 189, 0x2030, 1000, GN_perthousand },
    { 172, 0x2039,  333, GN_guilsinglleft },
    { 173, 0x203a,  333, GN_guilsinglright },
    {  -1, 0x2122, 1000, GN_trademark },
    {  -1, 0x2212,  606, GN_minus },
    { 164, 0x2215,  167, GN_fraction },
    { 174, 0xfb01,  685, GN_fi },
    { 175, 0xfb02,  685, GN_fl }
};


/*
 *  Font metrics
 */

const AFM PSDRV_NewCenturySchlbk_Bold =
{
    "NewCenturySchlbk-Bold",		    /* FontName */
    L"New Century Schoolbook Bold",	    /* FullName */
    L"New Century Schoolbook",		    /* FamilyName */
    L"AdobeStandardEncoding",		    /* EncodingScheme */
    FW_BOLD,				    /* Weight */
    0,					    /* ItalicAngle */
    FALSE,				    /* IsFixedPitch */
    -100,				    /* UnderlinePosition */
    50,					    /* UnderlineThickness */
    { -165, -250, 1000, 988 },		    /* FontBBox */
    737,				    /* Ascender */
    -205,				    /* Descender */
    {
	1000,				    /* WinMetrics.usUnitsPerEm */
	986,				    /* WinMetrics.sAscender */
	-216,				    /* WinMetrics.sDescender */
	0,				    /* WinMetrics.sLineGap */
	524,				    /* WinMetrics.sAvgCharWidth */
	741,				    /* WinMetrics.sTypoAscender */
	-195,				    /* WinMetrics.sTypoDescender */
	134,				    /* WinMetrics.sTypoLineGap */
	966,				    /* WinMetrics.usWinAscent */
	215				    /* WinMetrics.usWinDescent */
    },
    228,				    /* NumofMetrics */
    metrics				    /* Metrics */
};
