/*******************************************************************************
 *
 *	Font metric data for ITC Bookman Light
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
    {  32, 0x0020,  320, GN_space },
    {  33, 0x0021,  300, GN_exclam },
    {  34, 0x0022,  380, GN_quotedbl },
    {  35, 0x0023,  620, GN_numbersign },
    {  36, 0x0024,  620, GN_dollar },
    {  37, 0x0025,  900, GN_percent },
    {  38, 0x0026,  800, GN_ampersand },
    { 169, 0x0027,  220, GN_quotesingle },
    {  40, 0x0028,  300, GN_parenleft },
    {  41, 0x0029,  300, GN_parenright },
    {  42, 0x002a,  440, GN_asterisk },
    {  43, 0x002b,  600, GN_plus },
    {  44, 0x002c,  320, GN_comma },
    {  45, 0x002d,  400, GN_hyphen },
    {  46, 0x002e,  320, GN_period },
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
    {  58, 0x003a,  320, GN_colon },
    {  59, 0x003b,  320, GN_semicolon },
    {  60, 0x003c,  600, GN_less },
    {  61, 0x003d,  600, GN_equal },
    {  62, 0x003e,  600, GN_greater },
    {  63, 0x003f,  540, GN_question },
    {  64, 0x0040,  820, GN_at },
    {  65, 0x0041,  680, GN_A },
    {  66, 0x0042,  740, GN_B },
    {  67, 0x0043,  740, GN_C },
    {  68, 0x0044,  800, GN_D },
    {  69, 0x0045,  720, GN_E },
    {  70, 0x0046,  640, GN_F },
    {  71, 0x0047,  800, GN_G },
    {  72, 0x0048,  800, GN_H },
    {  73, 0x0049,  340, GN_I },
    {  74, 0x004a,  600, GN_J },
    {  75, 0x004b,  720, GN_K },
    {  76, 0x004c,  600, GN_L },
    {  77, 0x004d,  920, GN_M },
    {  78, 0x004e,  740, GN_N },
    {  79, 0x004f,  800, GN_O },
    {  80, 0x0050,  620, GN_P },
    {  81, 0x0051,  820, GN_Q },
    {  82, 0x0052,  720, GN_R },
    {  83, 0x0053,  660, GN_S },
    {  84, 0x0054,  620, GN_T },
    {  85, 0x0055,  780, GN_U },
    {  86, 0x0056,  700, GN_V },
    {  87, 0x0057,  960, GN_W },
    {  88, 0x0058,  720, GN_X },
    {  89, 0x0059,  640, GN_Y },
    {  90, 0x005a,  640, GN_Z },
    {  91, 0x005b,  300, GN_bracketleft },
    {  92, 0x005c,  600, GN_backslash },
    {  93, 0x005d,  300, GN_bracketright },
    {  94, 0x005e,  600, GN_asciicircum },
    {  95, 0x005f,  500, GN_underscore },
    { 193, 0x0060,  340, GN_grave },
    {  97, 0x0061,  580, GN_a },
    {  98, 0x0062,  620, GN_b },
    {  99, 0x0063,  520, GN_c },
    { 100, 0x0064,  620, GN_d },
    { 101, 0x0065,  520, GN_e },
    { 102, 0x0066,  320, GN_f },
    { 103, 0x0067,  540, GN_g },
    { 104, 0x0068,  660, GN_h },
    { 105, 0x0069,  300, GN_i },
    { 106, 0x006a,  300, GN_j },
    { 107, 0x006b,  620, GN_k },
    { 108, 0x006c,  300, GN_l },
    { 109, 0x006d,  940, GN_m },
    { 110, 0x006e,  660, GN_n },
    { 111, 0x006f,  560, GN_o },
    { 112, 0x0070,  620, GN_p },
    { 113, 0x0071,  580, GN_q },
    { 114, 0x0072,  440, GN_r },
    { 115, 0x0073,  520, GN_s },
    { 116, 0x0074,  380, GN_t },
    { 117, 0x0075,  680, GN_u },
    { 118, 0x0076,  520, GN_v },
    { 119, 0x0077,  780, GN_w },
    { 120, 0x0078,  560, GN_x },
    { 121, 0x0079,  540, GN_y },
    { 122, 0x007a,  480, GN_z },
    { 123, 0x007b,  280, GN_braceleft },
    { 124, 0x007c,  600, GN_bar },
    { 125, 0x007d,  280, GN_braceright },
    { 126, 0x007e,  600, GN_asciitilde },
    { 161, 0x00a1,  300, GN_exclamdown },
    { 162, 0x00a2,  620, GN_cent },
    { 163, 0x00a3,  620, GN_sterling },
    { 168, 0x00a4,  620, GN_currency },
    { 165, 0x00a5,  620, GN_yen },
    {  -1, 0x00a6,  600, GN_brokenbar },
    { 167, 0x00a7,  520, GN_section },
    { 200, 0x00a8,  420, GN_dieresis },
    {  -1, 0x00a9,  740, GN_copyright },
    { 227, 0x00aa,  420, GN_ordfeminine },
    { 171, 0x00ab,  360, GN_guillemotleft },
    {  -1, 0x00ac,  600, GN_logicalnot },
    {  -1, 0x00ae,  740, GN_registered },
    {  -1, 0x00b0,  400, GN_degree },
    {  -1, 0x00b1,  600, GN_plusminus },
    {  -1, 0x00b2,  372, GN_twosuperior },
    {  -1, 0x00b3,  372, GN_threesuperior },
    { 194, 0x00b4,  340, GN_acute },
    { 182, 0x00b6,  600, GN_paragraph },
    { 180, 0x00b7,  320, GN_periodcentered },
    { 203, 0x00b8,  320, GN_cedilla },
    {  -1, 0x00b9,  372, GN_onesuperior },
    { 235, 0x00ba,  420, GN_ordmasculine },
    { 187, 0x00bb,  360, GN_guillemotright },
    {  -1, 0x00bc,  930, GN_onequarter },
    {  -1, 0x00bd,  930, GN_onehalf },
    {  -1, 0x00be,  930, GN_threequarters },
    { 191, 0x00bf,  540, GN_questiondown },
    {  -1, 0x00c0,  680, GN_Agrave },
    {  -1, 0x00c1,  680, GN_Aacute },
    {  -1, 0x00c2,  680, GN_Acircumflex },
    {  -1, 0x00c3,  680, GN_Atilde },
    {  -1, 0x00c4,  680, GN_Adieresis },
    {  -1, 0x00c5,  680, GN_Aring },
    { 225, 0x00c6, 1260, GN_AE },
    {  -1, 0x00c7,  740, GN_Ccedilla },
    {  -1, 0x00c8,  720, GN_Egrave },
    {  -1, 0x00c9,  720, GN_Eacute },
    {  -1, 0x00ca,  720, GN_Ecircumflex },
    {  -1, 0x00cb,  720, GN_Edieresis },
    {  -1, 0x00cc,  340, GN_Igrave },
    {  -1, 0x00cd,  340, GN_Iacute },
    {  -1, 0x00ce,  340, GN_Icircumflex },
    {  -1, 0x00cf,  340, GN_Idieresis },
    {  -1, 0x00d0,  800, GN_Eth },
    {  -1, 0x00d1,  740, GN_Ntilde },
    {  -1, 0x00d2,  800, GN_Ograve },
    {  -1, 0x00d3,  800, GN_Oacute },
    {  -1, 0x00d4,  800, GN_Ocircumflex },
    {  -1, 0x00d5,  800, GN_Otilde },
    {  -1, 0x00d6,  800, GN_Odieresis },
    {  -1, 0x00d7,  600, GN_multiply },
    { 233, 0x00d8,  800, GN_Oslash },
    {  -1, 0x00d9,  780, GN_Ugrave },
    {  -1, 0x00da,  780, GN_Uacute },
    {  -1, 0x00db,  780, GN_Ucircumflex },
    {  -1, 0x00dc,  780, GN_Udieresis },
    {  -1, 0x00dd,  640, GN_Yacute },
    {  -1, 0x00de,  620, GN_Thorn },
    { 251, 0x00df,  660, GN_germandbls },
    {  -1, 0x00e0,  580, GN_agrave },
    {  -1, 0x00e1,  580, GN_aacute },
    {  -1, 0x00e2,  580, GN_acircumflex },
    {  -1, 0x00e3,  580, GN_atilde },
    {  -1, 0x00e4,  580, GN_adieresis },
    {  -1, 0x00e5,  580, GN_aring },
    { 241, 0x00e6,  860, GN_ae },
    {  -1, 0x00e7,  520, GN_ccedilla },
    {  -1, 0x00e8,  520, GN_egrave },
    {  -1, 0x00e9,  520, GN_eacute },
    {  -1, 0x00ea,  520, GN_ecircumflex },
    {  -1, 0x00eb,  520, GN_edieresis },
    {  -1, 0x00ec,  300, GN_igrave },
    {  -1, 0x00ed,  300, GN_iacute },
    {  -1, 0x00ee,  300, GN_icircumflex },
    {  -1, 0x00ef,  300, GN_idieresis },
    {  -1, 0x00f0,  560, GN_eth },
    {  -1, 0x00f1,  660, GN_ntilde },
    {  -1, 0x00f2,  560, GN_ograve },
    {  -1, 0x00f3,  560, GN_oacute },
    {  -1, 0x00f4,  560, GN_ocircumflex },
    {  -1, 0x00f5,  560, GN_otilde },
    {  -1, 0x00f6,  560, GN_odieresis },
    {  -1, 0x00f7,  600, GN_divide },
    { 249, 0x00f8,  560, GN_oslash },
    {  -1, 0x00f9,  680, GN_ugrave },
    {  -1, 0x00fa,  680, GN_uacute },
    {  -1, 0x00fb,  680, GN_ucircumflex },
    {  -1, 0x00fc,  680, GN_udieresis },
    {  -1, 0x00fd,  540, GN_yacute },
    {  -1, 0x00fe,  620, GN_thorn },
    {  -1, 0x00ff,  540, GN_ydieresis },
    { 245, 0x0131,  300, GN_dotlessi },
    { 232, 0x0141,  600, GN_Lslash },
    { 248, 0x0142,  320, GN_lslash },
    { 234, 0x0152, 1240, GN_OE },
    { 250, 0x0153,  900, GN_oe },
    {  -1, 0x0160,  660, GN_Scaron },
    {  -1, 0x0161,  520, GN_scaron },
    {  -1, 0x0178,  640, GN_Ydieresis },
    {  -1, 0x017d,  640, GN_Zcaron },
    {  -1, 0x017e,  480, GN_zcaron },
    { 166, 0x0192,  620, GN_florin },
    { 195, 0x02c6,  420, GN_circumflex },
    { 207, 0x02c7,  420, GN_caron },
    { 197, 0x02c9,  440, GN_macron },
    { 198, 0x02d8,  460, GN_breve },
    { 199, 0x02d9,  260, GN_dotaccent },
    { 202, 0x02da,  320, GN_ring },
    { 206, 0x02db,  320, GN_ogonek },
    { 196, 0x02dc,  440, GN_tilde },
    { 205, 0x02dd,  380, GN_hungarumlaut },
    {  -1, 0x03bc,  680, GN_mu },
    { 177, 0x2013,  500, GN_endash },
    { 208, 0x2014, 1000, GN_emdash },
    {  96, 0x2018,  220, GN_quoteleft },
    {  39, 0x2019,  220, GN_quoteright },
    { 184, 0x201a,  220, GN_quotesinglbase },
    { 170, 0x201c,  400, GN_quotedblleft },
    { 186, 0x201d,  400, GN_quotedblright },
    { 185, 0x201e,  400, GN_quotedblbase },
    { 178, 0x2020,  540, GN_dagger },
    { 179, 0x2021,  540, GN_daggerdbl },
    { 183, 0x2022,  460, GN_bullet },
    { 188, 0x2026, 1000, GN_ellipsis },
    { 189, 0x2030, 1280, GN_perthousand },
    { 172, 0x2039,  240, GN_guilsinglleft },
    { 173, 0x203a,  240, GN_guilsinglright },
    {  -1, 0x2122,  980, GN_trademark },
    {  -1, 0x2212,  600, GN_minus },
    { 164, 0x2215,  140, GN_fraction },
    { 174, 0xfb01,  620, GN_fi },
    { 175, 0xfb02,  620, GN_fl }
};


/*
 *  Font metrics
 */

const AFM PSDRV_Bookman_Light =
{
    "Bookman-Light",			    /* FontName */
    L"ITC Bookman Light",		    /* FullName */
    L"ITC Bookman",			    /* FamilyName */
    L"AdobeStandardEncoding",		    /* EncodingScheme */
    FW_NORMAL,				    /* Weight */
    0,					    /* ItalicAngle */
    FALSE,				    /* IsFixedPitch */
    -100,				    /* UnderlinePosition */
    50,					    /* UnderlineThickness */
    { -188, -251, 1266, 908 },		    /* FontBBox */
    717,				    /* Ascender */
    -228,				    /* Descender */
    {
	1000,				    /* WinMetrics.usUnitsPerEm */
	942,				    /* WinMetrics.sAscender */
	-232,				    /* WinMetrics.sDescender */
	0,				    /* WinMetrics.sLineGap */
	492,				    /* WinMetrics.sAvgCharWidth */
	716,				    /* WinMetrics.sTypoAscender */
	-225,				    /* WinMetrics.sTypoDescender */
	128,				    /* WinMetrics.sTypoLineGap */
	894,				    /* WinMetrics.usWinAscent */
	231				    /* WinMetrics.usWinDescent */
    },
    228,				    /* NumofMetrics */
    metrics				    /* Metrics */
};
