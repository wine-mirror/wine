/*******************************************************************************
 *
 *	Font metric data for Helvetica Narrow Oblique
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
    {  32, 0x0020,  228, GN_space },
    {  33, 0x0021,  228, GN_exclam },
    {  34, 0x0022,  291, GN_quotedbl },
    {  35, 0x0023,  456, GN_numbersign },
    {  36, 0x0024,  456, GN_dollar },
    {  37, 0x0025,  729, GN_percent },
    {  38, 0x0026,  547, GN_ampersand },
    { 169, 0x0027,  157, GN_quotesingle },
    {  40, 0x0028,  273, GN_parenleft },
    {  41, 0x0029,  273, GN_parenright },
    {  42, 0x002a,  319, GN_asterisk },
    {  43, 0x002b,  479, GN_plus },
    {  44, 0x002c,  228, GN_comma },
    {  45, 0x002d,  273, GN_hyphen },
    {  46, 0x002e,  228, GN_period },
    {  47, 0x002f,  228, GN_slash },
    {  48, 0x0030,  456, GN_zero },
    {  49, 0x0031,  456, GN_one },
    {  50, 0x0032,  456, GN_two },
    {  51, 0x0033,  456, GN_three },
    {  52, 0x0034,  456, GN_four },
    {  53, 0x0035,  456, GN_five },
    {  54, 0x0036,  456, GN_six },
    {  55, 0x0037,  456, GN_seven },
    {  56, 0x0038,  456, GN_eight },
    {  57, 0x0039,  456, GN_nine },
    {  58, 0x003a,  228, GN_colon },
    {  59, 0x003b,  228, GN_semicolon },
    {  60, 0x003c,  479, GN_less },
    {  61, 0x003d,  479, GN_equal },
    {  62, 0x003e,  479, GN_greater },
    {  63, 0x003f,  456, GN_question },
    {  64, 0x0040,  832, GN_at },
    {  65, 0x0041,  547, GN_A },
    {  66, 0x0042,  547, GN_B },
    {  67, 0x0043,  592, GN_C },
    {  68, 0x0044,  592, GN_D },
    {  69, 0x0045,  547, GN_E },
    {  70, 0x0046,  501, GN_F },
    {  71, 0x0047,  638, GN_G },
    {  72, 0x0048,  592, GN_H },
    {  73, 0x0049,  228, GN_I },
    {  74, 0x004a,  410, GN_J },
    {  75, 0x004b,  547, GN_K },
    {  76, 0x004c,  456, GN_L },
    {  77, 0x004d,  683, GN_M },
    {  78, 0x004e,  592, GN_N },
    {  79, 0x004f,  638, GN_O },
    {  80, 0x0050,  547, GN_P },
    {  81, 0x0051,  638, GN_Q },
    {  82, 0x0052,  592, GN_R },
    {  83, 0x0053,  547, GN_S },
    {  84, 0x0054,  501, GN_T },
    {  85, 0x0055,  592, GN_U },
    {  86, 0x0056,  547, GN_V },
    {  87, 0x0057,  774, GN_W },
    {  88, 0x0058,  547, GN_X },
    {  89, 0x0059,  547, GN_Y },
    {  90, 0x005a,  501, GN_Z },
    {  91, 0x005b,  228, GN_bracketleft },
    {  92, 0x005c,  228, GN_backslash },
    {  93, 0x005d,  228, GN_bracketright },
    {  94, 0x005e,  385, GN_asciicircum },
    {  95, 0x005f,  456, GN_underscore },
    { 193, 0x0060,  273, GN_grave },
    {  97, 0x0061,  456, GN_a },
    {  98, 0x0062,  456, GN_b },
    {  99, 0x0063,  410, GN_c },
    { 100, 0x0064,  456, GN_d },
    { 101, 0x0065,  456, GN_e },
    { 102, 0x0066,  228, GN_f },
    { 103, 0x0067,  456, GN_g },
    { 104, 0x0068,  456, GN_h },
    { 105, 0x0069,  182, GN_i },
    { 106, 0x006a,  182, GN_j },
    { 107, 0x006b,  410, GN_k },
    { 108, 0x006c,  182, GN_l },
    { 109, 0x006d,  683, GN_m },
    { 110, 0x006e,  456, GN_n },
    { 111, 0x006f,  456, GN_o },
    { 112, 0x0070,  456, GN_p },
    { 113, 0x0071,  456, GN_q },
    { 114, 0x0072,  273, GN_r },
    { 115, 0x0073,  410, GN_s },
    { 116, 0x0074,  228, GN_t },
    { 117, 0x0075,  456, GN_u },
    { 118, 0x0076,  410, GN_v },
    { 119, 0x0077,  592, GN_w },
    { 120, 0x0078,  410, GN_x },
    { 121, 0x0079,  410, GN_y },
    { 122, 0x007a,  410, GN_z },
    { 123, 0x007b,  274, GN_braceleft },
    { 124, 0x007c,  213, GN_bar },
    { 125, 0x007d,  274, GN_braceright },
    { 126, 0x007e,  479, GN_asciitilde },
    { 161, 0x00a1,  273, GN_exclamdown },
    { 162, 0x00a2,  456, GN_cent },
    { 163, 0x00a3,  456, GN_sterling },
    { 168, 0x00a4,  456, GN_currency },
    { 165, 0x00a5,  456, GN_yen },
    {  -1, 0x00a6,  213, GN_brokenbar },
    { 167, 0x00a7,  456, GN_section },
    { 200, 0x00a8,  273, GN_dieresis },
    {  -1, 0x00a9,  604, GN_copyright },
    { 227, 0x00aa,  303, GN_ordfeminine },
    { 171, 0x00ab,  456, GN_guillemotleft },
    {  -1, 0x00ac,  479, GN_logicalnot },
    {  -1, 0x00ae,  604, GN_registered },
    {  -1, 0x00b0,  328, GN_degree },
    {  -1, 0x00b1,  479, GN_plusminus },
    {  -1, 0x00b2,  273, GN_twosuperior },
    {  -1, 0x00b3,  273, GN_threesuperior },
    { 194, 0x00b4,  273, GN_acute },
    { 182, 0x00b6,  440, GN_paragraph },
    { 180, 0x00b7,  228, GN_periodcentered },
    { 203, 0x00b8,  273, GN_cedilla },
    {  -1, 0x00b9,  273, GN_onesuperior },
    { 235, 0x00ba,  299, GN_ordmasculine },
    { 187, 0x00bb,  456, GN_guillemotright },
    {  -1, 0x00bc,  684, GN_onequarter },
    {  -1, 0x00bd,  684, GN_onehalf },
    {  -1, 0x00be,  684, GN_threequarters },
    { 191, 0x00bf,  501, GN_questiondown },
    {  -1, 0x00c0,  547, GN_Agrave },
    {  -1, 0x00c1,  547, GN_Aacute },
    {  -1, 0x00c2,  547, GN_Acircumflex },
    {  -1, 0x00c3,  547, GN_Atilde },
    {  -1, 0x00c4,  547, GN_Adieresis },
    {  -1, 0x00c5,  547, GN_Aring },
    { 225, 0x00c6,  820, GN_AE },
    {  -1, 0x00c7,  592, GN_Ccedilla },
    {  -1, 0x00c8,  547, GN_Egrave },
    {  -1, 0x00c9,  547, GN_Eacute },
    {  -1, 0x00ca,  547, GN_Ecircumflex },
    {  -1, 0x00cb,  547, GN_Edieresis },
    {  -1, 0x00cc,  228, GN_Igrave },
    {  -1, 0x00cd,  228, GN_Iacute },
    {  -1, 0x00ce,  228, GN_Icircumflex },
    {  -1, 0x00cf,  228, GN_Idieresis },
    {  -1, 0x00d0,  592, GN_Eth },
    {  -1, 0x00d1,  592, GN_Ntilde },
    {  -1, 0x00d2,  638, GN_Ograve },
    {  -1, 0x00d3,  638, GN_Oacute },
    {  -1, 0x00d4,  638, GN_Ocircumflex },
    {  -1, 0x00d5,  638, GN_Otilde },
    {  -1, 0x00d6,  638, GN_Odieresis },
    {  -1, 0x00d7,  479, GN_multiply },
    { 233, 0x00d8,  638, GN_Oslash },
    {  -1, 0x00d9,  592, GN_Ugrave },
    {  -1, 0x00da,  592, GN_Uacute },
    {  -1, 0x00db,  592, GN_Ucircumflex },
    {  -1, 0x00dc,  592, GN_Udieresis },
    {  -1, 0x00dd,  547, GN_Yacute },
    {  -1, 0x00de,  547, GN_Thorn },
    { 251, 0x00df,  501, GN_germandbls },
    {  -1, 0x00e0,  456, GN_agrave },
    {  -1, 0x00e1,  456, GN_aacute },
    {  -1, 0x00e2,  456, GN_acircumflex },
    {  -1, 0x00e3,  456, GN_atilde },
    {  -1, 0x00e4,  456, GN_adieresis },
    {  -1, 0x00e5,  456, GN_aring },
    { 241, 0x00e6,  729, GN_ae },
    {  -1, 0x00e7,  410, GN_ccedilla },
    {  -1, 0x00e8,  456, GN_egrave },
    {  -1, 0x00e9,  456, GN_eacute },
    {  -1, 0x00ea,  456, GN_ecircumflex },
    {  -1, 0x00eb,  456, GN_edieresis },
    {  -1, 0x00ec,  228, GN_igrave },
    {  -1, 0x00ed,  228, GN_iacute },
    {  -1, 0x00ee,  228, GN_icircumflex },
    {  -1, 0x00ef,  228, GN_idieresis },
    {  -1, 0x00f0,  456, GN_eth },
    {  -1, 0x00f1,  456, GN_ntilde },
    {  -1, 0x00f2,  456, GN_ograve },
    {  -1, 0x00f3,  456, GN_oacute },
    {  -1, 0x00f4,  456, GN_ocircumflex },
    {  -1, 0x00f5,  456, GN_otilde },
    {  -1, 0x00f6,  456, GN_odieresis },
    {  -1, 0x00f7,  479, GN_divide },
    { 249, 0x00f8,  501, GN_oslash },
    {  -1, 0x00f9,  456, GN_ugrave },
    {  -1, 0x00fa,  456, GN_uacute },
    {  -1, 0x00fb,  456, GN_ucircumflex },
    {  -1, 0x00fc,  456, GN_udieresis },
    {  -1, 0x00fd,  410, GN_yacute },
    {  -1, 0x00fe,  456, GN_thorn },
    {  -1, 0x00ff,  410, GN_ydieresis },
    { 245, 0x0131,  228, GN_dotlessi },
    { 232, 0x0141,  456, GN_Lslash },
    { 248, 0x0142,  182, GN_lslash },
    { 234, 0x0152,  820, GN_OE },
    { 250, 0x0153,  774, GN_oe },
    {  -1, 0x0160,  547, GN_Scaron },
    {  -1, 0x0161,  410, GN_scaron },
    {  -1, 0x0178,  547, GN_Ydieresis },
    {  -1, 0x017d,  501, GN_Zcaron },
    {  -1, 0x017e,  410, GN_zcaron },
    { 166, 0x0192,  456, GN_florin },
    { 195, 0x02c6,  273, GN_circumflex },
    { 207, 0x02c7,  273, GN_caron },
    { 197, 0x02c9,  273, GN_macron },
    { 198, 0x02d8,  273, GN_breve },
    { 199, 0x02d9,  273, GN_dotaccent },
    { 202, 0x02da,  273, GN_ring },
    { 206, 0x02db,  273, GN_ogonek },
    { 196, 0x02dc,  273, GN_tilde },
    { 205, 0x02dd,  273, GN_hungarumlaut },
    {  -1, 0x03bc,  456, GN_mu },
    { 177, 0x2013,  456, GN_endash },
    { 208, 0x2014,  820, GN_emdash },
    {  96, 0x2018,  182, GN_quoteleft },
    {  39, 0x2019,  182, GN_quoteright },
    { 184, 0x201a,  182, GN_quotesinglbase },
    { 170, 0x201c,  273, GN_quotedblleft },
    { 186, 0x201d,  273, GN_quotedblright },
    { 185, 0x201e,  273, GN_quotedblbase },
    { 178, 0x2020,  456, GN_dagger },
    { 179, 0x2021,  456, GN_daggerdbl },
    { 183, 0x2022,  287, GN_bullet },
    { 188, 0x2026,  820, GN_ellipsis },
    { 189, 0x2030,  820, GN_perthousand },
    { 172, 0x2039,  273, GN_guilsinglleft },
    { 173, 0x203a,  273, GN_guilsinglright },
    {  -1, 0x2122,  820, GN_trademark },
    {  -1, 0x2212,  479, GN_minus },
    { 164, 0x2215,  137, GN_fraction },
    { 174, 0xfb01,  410, GN_fi },
    { 175, 0xfb02,  410, GN_fl }
};


/*
 *  Font metrics
 */

const AFM PSDRV_Helvetica_Narrow_Oblique =
{
    "Helvetica-Narrow-Oblique",		    /* FontName */
    L"Helvetica Narrow Oblique",	    /* FullName */
    L"Helvetica Narrow",		    /* FamilyName */
    L"AdobeStandardEncoding",		    /* EncodingScheme */
    FW_NORMAL,				    /* Weight */
    -12,				    /* ItalicAngle */
    FALSE,				    /* IsFixedPitch */
    -100,				    /* UnderlinePosition */
    50,					    /* UnderlineThickness */
    { -139, -225, 915, 931 },		    /* FontBBox */
    718,				    /* Ascender */
    -207,				    /* Descender */
    {
	1000,				    /* WinMetrics.usUnitsPerEm */
	936,				    /* WinMetrics.sAscender */
	-212,				    /* WinMetrics.sDescender */
	0,				    /* WinMetrics.sLineGap */
	362,				    /* WinMetrics.sAvgCharWidth */
	728,				    /* WinMetrics.sTypoAscender */
	-208,				    /* WinMetrics.sTypoDescender */
	134,				    /* WinMetrics.sTypoLineGap */
	915,				    /* WinMetrics.usWinAscent */
	210				    /* WinMetrics.usWinDescent */
    },
    228,				    /* NumofMetrics */
    metrics				    /* Metrics */
};
