/*******************************************************************************
 *
 *	Font and glyph data for the Wine PostScript driver
 *
 *	Copyright 2001 Ian Pilcher
 *
 *
 *	This data is derived from the Adobe Glyph list at
 *
 *	    http://partners.adobe.com/asn/developer/type/glyphlist.txt
 *
 *	and the Adobe Font Metrics files at
 *
 *	    ftp://ftp.adobe.com/pub/adobe/type/win/all/afmfiles/base35/
 *
 *	which are Copyright 1985-1998 Adobe Systems Incorporated.
 *
 */

#include "psdrv.h"
#include "data/agl.h"


/*
 *  Built-in font metrics
 */

const AFM *const PSDRV_BuiltinAFMs[] =
{
    &PSDRV_AvantGarde_Demi,
    &PSDRV_AvantGarde_DemiOblique,
    &PSDRV_AvantGarde_Book,
    &PSDRV_AvantGarde_BookOblique,
    &PSDRV_Bookman_Demi,
    &PSDRV_Bookman_DemiItalic,
    &PSDRV_Bookman_Light,
    &PSDRV_Bookman_LightItalic,
    &PSDRV_Courier_Bold,
    &PSDRV_Courier_BoldOblique,
    &PSDRV_Courier,
    &PSDRV_Courier_Oblique,
    &PSDRV_Helvetica,
    &PSDRV_Helvetica_Bold,
    &PSDRV_Helvetica_BoldOblique,
    &PSDRV_Helvetica_Narrow,
    &PSDRV_Helvetica_Narrow_Bold,
    &PSDRV_Helvetica_Narrow_BoldOblique,
    &PSDRV_Helvetica_Narrow_Oblique,
    &PSDRV_Helvetica_Oblique,
    &PSDRV_NewCenturySchlbk_Bold,
    &PSDRV_NewCenturySchlbk_BoldItalic,
    &PSDRV_NewCenturySchlbk_Italic,
    &PSDRV_NewCenturySchlbk_Roman,
    &PSDRV_Palatino_Bold,
    &PSDRV_Palatino_BoldItalic,
    &PSDRV_Palatino_Italic,
    &PSDRV_Palatino_Roman,
    &PSDRV_Symbol,
    &PSDRV_Times_Bold,
    &PSDRV_Times_BoldItalic,
    &PSDRV_Times_Italic,
    &PSDRV_Times_Roman,
    &PSDRV_ZapfChancery_MediumItalic,
    &PSDRV_ZapfDingbats,
    NULL
};


/*
 *  Every glyph name in the AGL and the 35 core PostScript fonts
 */

const INT PSDRV_AGLGlyphNamesSize = 1258;

GLYPHNAME PSDRV_AGLGlyphNames[1258] =
{
    {    0, "A" },			/* LATIN CAPITAL LETTER A */
    {    1, "AE" },			/* LATIN CAPITAL LETTER AE */
    {    2, "AEacute" },		/* LATIN CAPITAL LETTER AE WITH ACUTE */
    {    3, "AEsmall" },		/* LATIN SMALL CAPITAL LETTER AE */
    {    4, "Aacute" },			/* LATIN CAPITAL LETTER A WITH ACUTE */
    {    5, "Aacutesmall" },		/* LATIN SMALL CAPITAL LETTER A WITH ACUTE */
    {    6, "Abreve" },			/* LATIN CAPITAL LETTER A WITH BREVE */
    {    7, "Acircumflex" },		/* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
    {    8, "Acircumflexsmall" },	/* LATIN SMALL CAPITAL LETTER A WITH CIRCUMFLEX */
    {    9, "Acute" },			/* CAPITAL ACUTE ACCENT */
    {   10, "Acutesmall" },		/* SMALL CAPITAL ACUTE ACCENT */
    {   11, "Adieresis" },		/* LATIN CAPITAL LETTER A WITH DIAERESIS */
    {   12, "Adieresissmall" },		/* LATIN SMALL CAPITAL LETTER A WITH DIAERESIS */
    {   13, "Agrave" },			/* LATIN CAPITAL LETTER A WITH GRAVE */
    {   14, "Agravesmall" },		/* LATIN SMALL CAPITAL LETTER A WITH GRAVE */
    {   15, "Alpha" },			/* GREEK CAPITAL LETTER ALPHA */
    {   16, "Alphatonos" },		/* GREEK CAPITAL LETTER ALPHA WITH TONOS */
    {   17, "Amacron" },		/* LATIN CAPITAL LETTER A WITH MACRON */
    {   18, "Aogonek" },		/* LATIN CAPITAL LETTER A WITH OGONEK */
    {   19, "Aring" },			/* LATIN CAPITAL LETTER A WITH RING ABOVE */
    {   20, "Aringacute" },		/* LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE */
    {   21, "Aringsmall" },		/* LATIN SMALL CAPITAL LETTER A WITH RING ABOVE */
    {   22, "Asmall" },			/* LATIN SMALL CAPITAL LETTER A */
    {   23, "Atilde" },			/* LATIN CAPITAL LETTER A WITH TILDE */
    {   24, "Atildesmall" },		/* LATIN SMALL CAPITAL LETTER A WITH TILDE */
    {   25, "B" },			/* LATIN CAPITAL LETTER B */
    {   26, "Beta" },			/* GREEK CAPITAL LETTER BETA */
    {   27, "Brevesmall" },		/* SMALL CAPITAL BREVE */
    {   28, "Bsmall" },			/* LATIN SMALL CAPITAL LETTER B */
    {   29, "C" },			/* LATIN CAPITAL LETTER C */
    {   30, "Cacute" },			/* LATIN CAPITAL LETTER C WITH ACUTE */
    {   31, "Caron" },			/* CAPITAL CARON */
    {   32, "Caronsmall" },		/* SMALL CAPITAL CARON */
    {   33, "Ccaron" },			/* LATIN CAPITAL LETTER C WITH CARON */
    {   34, "Ccedilla" },		/* LATIN CAPITAL LETTER C WITH CEDILLA */
    {   35, "Ccedillasmall" },		/* LATIN SMALL CAPITAL LETTER C WITH CEDILLA */
    {   36, "Ccircumflex" },		/* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    {   37, "Cdotaccent" },		/* LATIN CAPITAL LETTER C WITH DOT ABOVE */
    {   38, "Cedillasmall" },		/* SMALL CAPITAL CEDILLA */
    {   39, "Chi" },			/* GREEK CAPITAL LETTER CHI */
    {   40, "Circumflexsmall" },	/* SMALL CAPITAL MODIFIER LETTER CIRCUMFLEX ACCENT */
    {   41, "Csmall" },			/* LATIN SMALL CAPITAL LETTER C */
    {   42, "D" },			/* LATIN CAPITAL LETTER D */
    {   43, "Dcaron" },			/* LATIN CAPITAL LETTER D WITH CARON */
    {   44, "Dcroat" },			/* LATIN CAPITAL LETTER D WITH STROKE */
    {   45, "Delta" },			/* INCREMENT */
					/* GREEK CAPITAL LETTER DELTA;Duplicate */
    {   46, "Dieresis" },		/* CAPITAL DIAERESIS */
    {   47, "DieresisAcute" },		/* CAPITAL DIAERESIS ACUTE ACCENT */
    {   48, "DieresisGrave" },		/* CAPITAL DIAERESIS GRAVE ACCENT */
    {   49, "Dieresissmall" },		/* SMALL CAPITAL DIAERESIS */
    {   50, "Dotaccentsmall" },		/* SMALL CAPITAL DOT ABOVE */
    {   51, "Dsmall" },			/* LATIN SMALL CAPITAL LETTER D */
    {   52, "E" },			/* LATIN CAPITAL LETTER E */
    {   53, "Eacute" },			/* LATIN CAPITAL LETTER E WITH ACUTE */
    {   54, "Eacutesmall" },		/* LATIN SMALL CAPITAL LETTER E WITH ACUTE */
    {   55, "Ebreve" },			/* LATIN CAPITAL LETTER E WITH BREVE */
    {   56, "Ecaron" },			/* LATIN CAPITAL LETTER E WITH CARON */
    {   57, "Ecircumflex" },		/* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
    {   58, "Ecircumflexsmall" },	/* LATIN SMALL CAPITAL LETTER E WITH CIRCUMFLEX */
    {   59, "Edieresis" },		/* LATIN CAPITAL LETTER E WITH DIAERESIS */
    {   60, "Edieresissmall" },		/* LATIN SMALL CAPITAL LETTER E WITH DIAERESIS */
    {   61, "Edotaccent" },		/* LATIN CAPITAL LETTER E WITH DOT ABOVE */
    {   62, "Egrave" },			/* LATIN CAPITAL LETTER E WITH GRAVE */
    {   63, "Egravesmall" },		/* LATIN SMALL CAPITAL LETTER E WITH GRAVE */
    {   64, "Emacron" },		/* LATIN CAPITAL LETTER E WITH MACRON */
    {   65, "Eng" },			/* LATIN CAPITAL LETTER ENG */
    {   66, "Eogonek" },		/* LATIN CAPITAL LETTER E WITH OGONEK */
    {   67, "Epsilon" },		/* GREEK CAPITAL LETTER EPSILON */
    {   68, "Epsilontonos" },		/* GREEK CAPITAL LETTER EPSILON WITH TONOS */
    {   69, "Esmall" },			/* LATIN SMALL CAPITAL LETTER E */
    {   70, "Eta" },			/* GREEK CAPITAL LETTER ETA */
    {   71, "Etatonos" },		/* GREEK CAPITAL LETTER ETA WITH TONOS */
    {   72, "Eth" },			/* LATIN CAPITAL LETTER ETH */
    {   73, "Ethsmall" },		/* LATIN SMALL CAPITAL LETTER ETH */
    {   74, "Euro" },			/* EURO SIGN */
    {   75, "F" },			/* LATIN CAPITAL LETTER F */
    {   76, "Fsmall" },			/* LATIN SMALL CAPITAL LETTER F */
    {   77, "G" },			/* LATIN CAPITAL LETTER G */
    {   78, "Gamma" },			/* GREEK CAPITAL LETTER GAMMA */
    {   79, "Gbreve" },			/* LATIN CAPITAL LETTER G WITH BREVE */
    {   80, "Gcaron" },			/* LATIN CAPITAL LETTER G WITH CARON */
    {   81, "Gcircumflex" },		/* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    {   82, "Gcommaaccent" },		/* LATIN CAPITAL LETTER G WITH CEDILLA */
    {   83, "Gdotaccent" },		/* LATIN CAPITAL LETTER G WITH DOT ABOVE */
    {   84, "Grave" },			/* CAPITAL GRAVE ACCENT */
    {   85, "Gravesmall" },		/* SMALL CAPITAL GRAVE ACCENT */
    {   86, "Gsmall" },			/* LATIN SMALL CAPITAL LETTER G */
    {   87, "H" },			/* LATIN CAPITAL LETTER H */
    {   88, "H18533" },			/* BLACK CIRCLE */
    {   89, "H18543" },			/* BLACK SMALL SQUARE */
    {   90, "H18551" },			/* WHITE SMALL SQUARE */
    {   91, "H22073" },			/* WHITE SQUARE */
    {   92, "Hbar" },			/* LATIN CAPITAL LETTER H WITH STROKE */
    {   93, "Hcircumflex" },		/* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    {   94, "Hsmall" },			/* LATIN SMALL CAPITAL LETTER H */
    {   95, "Hungarumlaut" },		/* CAPITAL DOUBLE ACUTE ACCENT */
    {   96, "Hungarumlautsmall" },	/* SMALL CAPITAL DOUBLE ACUTE ACCENT */
    {   97, "I" },			/* LATIN CAPITAL LETTER I */
    {   98, "IJ" },			/* LATIN CAPITAL LIGATURE IJ */
    {   99, "Iacute" },			/* LATIN CAPITAL LETTER I WITH ACUTE */
    {  100, "Iacutesmall" },		/* LATIN SMALL CAPITAL LETTER I WITH ACUTE */
    {  101, "Ibreve" },			/* LATIN CAPITAL LETTER I WITH BREVE */
    {  102, "Icircumflex" },		/* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
    {  103, "Icircumflexsmall" },	/* LATIN SMALL CAPITAL LETTER I WITH CIRCUMFLEX */
    {  104, "Idieresis" },		/* LATIN CAPITAL LETTER I WITH DIAERESIS */
    {  105, "Idieresissmall" },		/* LATIN SMALL CAPITAL LETTER I WITH DIAERESIS */
    {  106, "Idot" },			/* FONT FAMILY;Courier */
    {  107, "Idotaccent" },		/* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    {  108, "Ifraktur" },		/* BLACK-LETTER CAPITAL I */
    {  109, "Igrave" },			/* LATIN CAPITAL LETTER I WITH GRAVE */
    {  110, "Igravesmall" },		/* LATIN SMALL CAPITAL LETTER I WITH GRAVE */
    {  111, "Imacron" },		/* LATIN CAPITAL LETTER I WITH MACRON */
    {  112, "Iogonek" },		/* LATIN CAPITAL LETTER I WITH OGONEK */
    {  113, "Iota" },			/* GREEK CAPITAL LETTER IOTA */
    {  114, "Iotadieresis" },		/* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    {  115, "Iotatonos" },		/* GREEK CAPITAL LETTER IOTA WITH TONOS */
    {  116, "Ismall" },			/* LATIN SMALL CAPITAL LETTER I */
    {  117, "Itilde" },			/* LATIN CAPITAL LETTER I WITH TILDE */
    {  118, "J" },			/* LATIN CAPITAL LETTER J */
    {  119, "Jcircumflex" },		/* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    {  120, "Jsmall" },			/* LATIN SMALL CAPITAL LETTER J */
    {  121, "K" },			/* LATIN CAPITAL LETTER K */
    {  122, "Kappa" },			/* GREEK CAPITAL LETTER KAPPA */
    {  123, "Kcommaaccent" },		/* LATIN CAPITAL LETTER K WITH CEDILLA */
    {  124, "Ksmall" },			/* LATIN SMALL CAPITAL LETTER K */
    {  125, "L" },			/* LATIN CAPITAL LETTER L */
    {  126, "LL" },			/* LATIN CAPITAL LETTER LL */
    {  127, "Lacute" },			/* LATIN CAPITAL LETTER L WITH ACUTE */
    {  128, "Lambda" },			/* GREEK CAPITAL LETTER LAMDA */
    {  129, "Lcaron" },			/* LATIN CAPITAL LETTER L WITH CARON */
    {  130, "Lcommaaccent" },		/* LATIN CAPITAL LETTER L WITH CEDILLA */
    {  131, "Ldot" },			/* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
    {  132, "Lslash" },			/* LATIN CAPITAL LETTER L WITH STROKE */
    {  133, "Lslashsmall" },		/* LATIN SMALL CAPITAL LETTER L WITH STROKE */
    {  134, "Lsmall" },			/* LATIN SMALL CAPITAL LETTER L */
    {  135, "M" },			/* LATIN CAPITAL LETTER M */
    {  136, "Macron" },			/* CAPITAL MACRON */
    {  137, "Macronsmall" },		/* SMALL CAPITAL MACRON */
    {  138, "Msmall" },			/* LATIN SMALL CAPITAL LETTER M */
    {  139, "Mu" },			/* GREEK CAPITAL LETTER MU */
    {  140, "N" },			/* LATIN CAPITAL LETTER N */
    {  141, "Nacute" },			/* LATIN CAPITAL LETTER N WITH ACUTE */
    {  142, "Ncaron" },			/* LATIN CAPITAL LETTER N WITH CARON */
    {  143, "Ncommaaccent" },		/* LATIN CAPITAL LETTER N WITH CEDILLA */
    {  144, "Nsmall" },			/* LATIN SMALL CAPITAL LETTER N */
    {  145, "Ntilde" },			/* LATIN CAPITAL LETTER N WITH TILDE */
    {  146, "Ntildesmall" },		/* LATIN SMALL CAPITAL LETTER N WITH TILDE */
    {  147, "Nu" },			/* GREEK CAPITAL LETTER NU */
    {  148, "O" },			/* LATIN CAPITAL LETTER O */
    {  149, "OE" },			/* LATIN CAPITAL LIGATURE OE */
    {  150, "OEsmall" },		/* LATIN SMALL CAPITAL LIGATURE OE */
    {  151, "Oacute" },			/* LATIN CAPITAL LETTER O WITH ACUTE */
    {  152, "Oacutesmall" },		/* LATIN SMALL CAPITAL LETTER O WITH ACUTE */
    {  153, "Obreve" },			/* LATIN CAPITAL LETTER O WITH BREVE */
    {  154, "Ocircumflex" },		/* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
    {  155, "Ocircumflexsmall" },	/* LATIN SMALL CAPITAL LETTER O WITH CIRCUMFLEX */
    {  156, "Odieresis" },		/* LATIN CAPITAL LETTER O WITH DIAERESIS */
    {  157, "Odieresissmall" },		/* LATIN SMALL CAPITAL LETTER O WITH DIAERESIS */
    {  158, "Ogoneksmall" },		/* SMALL CAPITAL OGONEK */
    {  159, "Ograve" },			/* LATIN CAPITAL LETTER O WITH GRAVE */
    {  160, "Ogravesmall" },		/* LATIN SMALL CAPITAL LETTER O WITH GRAVE */
    {  161, "Ohorn" },			/* LATIN CAPITAL LETTER O WITH HORN */
    {  162, "Ohungarumlaut" },		/* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    {  163, "Omacron" },		/* LATIN CAPITAL LETTER O WITH MACRON */
    {  164, "Omega" },			/* OHM SIGN */
					/* GREEK CAPITAL LETTER OMEGA;Duplicate */
    {  165, "Omegatonos" },		/* GREEK CAPITAL LETTER OMEGA WITH TONOS */
    {  166, "Omicron" },		/* GREEK CAPITAL LETTER OMICRON */
    {  167, "Omicrontonos" },		/* GREEK CAPITAL LETTER OMICRON WITH TONOS */
    {  168, "Oslash" },			/* LATIN CAPITAL LETTER O WITH STROKE */
    {  169, "Oslashacute" },		/* LATIN CAPITAL LETTER O WITH STROKE AND ACUTE */
    {  170, "Oslashsmall" },		/* LATIN SMALL CAPITAL LETTER O WITH STROKE */
    {  171, "Osmall" },			/* LATIN SMALL CAPITAL LETTER O */
    {  172, "Otilde" },			/* LATIN CAPITAL LETTER O WITH TILDE */
    {  173, "Otildesmall" },		/* LATIN SMALL CAPITAL LETTER O WITH TILDE */
    {  174, "P" },			/* LATIN CAPITAL LETTER P */
    {  175, "Phi" },			/* GREEK CAPITAL LETTER PHI */
    {  176, "Pi" },			/* GREEK CAPITAL LETTER PI */
    {  177, "Psi" },			/* GREEK CAPITAL LETTER PSI */
    {  178, "Psmall" },			/* LATIN SMALL CAPITAL LETTER P */
    {  179, "Q" },			/* LATIN CAPITAL LETTER Q */
    {  180, "Qsmall" },			/* LATIN SMALL CAPITAL LETTER Q */
    {  181, "R" },			/* LATIN CAPITAL LETTER R */
    {  182, "Racute" },			/* LATIN CAPITAL LETTER R WITH ACUTE */
    {  183, "Rcaron" },			/* LATIN CAPITAL LETTER R WITH CARON */
    {  184, "Rcommaaccent" },		/* LATIN CAPITAL LETTER R WITH CEDILLA */
    {  185, "Rfraktur" },		/* BLACK-LETTER CAPITAL R */
    {  186, "Rho" },			/* GREEK CAPITAL LETTER RHO */
    {  187, "Ringsmall" },		/* SMALL CAPITAL RING ABOVE */
    {  188, "Rsmall" },			/* LATIN SMALL CAPITAL LETTER R */
    {  189, "S" },			/* LATIN CAPITAL LETTER S */
    {  190, "SF010000" },		/* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    {  191, "SF020000" },		/* BOX DRAWINGS LIGHT UP AND RIGHT */
    {  192, "SF030000" },		/* BOX DRAWINGS LIGHT DOWN AND LEFT */
    {  193, "SF040000" },		/* BOX DRAWINGS LIGHT UP AND LEFT */
    {  194, "SF050000" },		/* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    {  195, "SF060000" },		/* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    {  196, "SF070000" },		/* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    {  197, "SF080000" },		/* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    {  198, "SF090000" },		/* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    {  199, "SF100000" },		/* BOX DRAWINGS LIGHT HORIZONTAL */
    {  200, "SF110000" },		/* BOX DRAWINGS LIGHT VERTICAL */
    {  201, "SF190000" },		/* BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE */
    {  202, "SF200000" },		/* BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE */
    {  203, "SF210000" },		/* BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE */
    {  204, "SF220000" },		/* BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE */
    {  205, "SF230000" },		/* BOX DRAWINGS DOUBLE VERTICAL AND LEFT */
    {  206, "SF240000" },		/* BOX DRAWINGS DOUBLE VERTICAL */
    {  207, "SF250000" },		/* BOX DRAWINGS DOUBLE DOWN AND LEFT */
    {  208, "SF260000" },		/* BOX DRAWINGS DOUBLE UP AND LEFT */
    {  209, "SF270000" },		/* BOX DRAWINGS UP DOUBLE AND LEFT SINGLE */
    {  210, "SF280000" },		/* BOX DRAWINGS UP SINGLE AND LEFT DOUBLE */
    {  211, "SF360000" },		/* BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE */
    {  212, "SF370000" },		/* BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE */
    {  213, "SF380000" },		/* BOX DRAWINGS DOUBLE UP AND RIGHT */
    {  214, "SF390000" },		/* BOX DRAWINGS DOUBLE DOWN AND RIGHT */
    {  215, "SF400000" },		/* BOX DRAWINGS DOUBLE UP AND HORIZONTAL */
    {  216, "SF410000" },		/* BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL */
    {  217, "SF420000" },		/* BOX DRAWINGS DOUBLE VERTICAL AND RIGHT */
    {  218, "SF430000" },		/* BOX DRAWINGS DOUBLE HORIZONTAL */
    {  219, "SF440000" },		/* BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL */
    {  220, "SF450000" },		/* BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE */
    {  221, "SF460000" },		/* BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE */
    {  222, "SF470000" },		/* BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE */
    {  223, "SF480000" },		/* BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE */
    {  224, "SF490000" },		/* BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE */
    {  225, "SF500000" },		/* BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE */
    {  226, "SF510000" },		/* BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE */
    {  227, "SF520000" },		/* BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE */
    {  228, "SF530000" },		/* BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE */
    {  229, "SF540000" },		/* BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE */
    {  230, "Sacute" },			/* LATIN CAPITAL LETTER S WITH ACUTE */
    {  231, "Scaron" },			/* LATIN CAPITAL LETTER S WITH CARON */
    {  232, "Scaronsmall" },		/* LATIN SMALL CAPITAL LETTER S WITH CARON */
    {  233, "Scedilla" },		/* LATIN CAPITAL LETTER S WITH CEDILLA */
					/* LATIN CAPITAL LETTER S WITH CEDILLA;Duplicate */
    {  234, "Scircumflex" },		/* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    {  235, "Scommaaccent" },		/* LATIN CAPITAL LETTER S WITH COMMA BELOW */
    {  236, "Sigma" },			/* GREEK CAPITAL LETTER SIGMA */
    {  237, "Ssmall" },			/* LATIN SMALL CAPITAL LETTER S */
    {  238, "T" },			/* LATIN CAPITAL LETTER T */
    {  239, "Tau" },			/* GREEK CAPITAL LETTER TAU */
    {  240, "Tbar" },			/* LATIN CAPITAL LETTER T WITH STROKE */
    {  241, "Tcaron" },			/* LATIN CAPITAL LETTER T WITH CARON */
    {  242, "Tcommaaccent" },		/* LATIN CAPITAL LETTER T WITH CEDILLA */
					/* LATIN CAPITAL LETTER T WITH COMMA BELOW;Duplicate */
    {  243, "Theta" },			/* GREEK CAPITAL LETTER THETA */
    {  244, "Thorn" },			/* LATIN CAPITAL LETTER THORN */
    {  245, "Thornsmall" },		/* LATIN SMALL CAPITAL LETTER THORN */
    {  246, "Tildesmall" },		/* SMALL CAPITAL SMALL TILDE */
    {  247, "Tsmall" },			/* LATIN SMALL CAPITAL LETTER T */
    {  248, "U" },			/* LATIN CAPITAL LETTER U */
    {  249, "Uacute" },			/* LATIN CAPITAL LETTER U WITH ACUTE */
    {  250, "Uacutesmall" },		/* LATIN SMALL CAPITAL LETTER U WITH ACUTE */
    {  251, "Ubreve" },			/* LATIN CAPITAL LETTER U WITH BREVE */
    {  252, "Ucircumflex" },		/* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
    {  253, "Ucircumflexsmall" },	/* LATIN SMALL CAPITAL LETTER U WITH CIRCUMFLEX */
    {  254, "Udieresis" },		/* LATIN CAPITAL LETTER U WITH DIAERESIS */
    {  255, "Udieresissmall" },		/* LATIN SMALL CAPITAL LETTER U WITH DIAERESIS */
    {  256, "Ugrave" },			/* LATIN CAPITAL LETTER U WITH GRAVE */
    {  257, "Ugravesmall" },		/* LATIN SMALL CAPITAL LETTER U WITH GRAVE */
    {  258, "Uhorn" },			/* LATIN CAPITAL LETTER U WITH HORN */
    {  259, "Uhungarumlaut" },		/* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    {  260, "Umacron" },		/* LATIN CAPITAL LETTER U WITH MACRON */
    {  261, "Uogonek" },		/* LATIN CAPITAL LETTER U WITH OGONEK */
    {  262, "Upsilon" },		/* GREEK CAPITAL LETTER UPSILON */
    {  263, "Upsilon1" },		/* GREEK UPSILON WITH HOOK SYMBOL */
    {  264, "Upsilondieresis" },	/* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    {  265, "Upsilontonos" },		/* GREEK CAPITAL LETTER UPSILON WITH TONOS */
    {  266, "Uring" },			/* LATIN CAPITAL LETTER U WITH RING ABOVE */
    {  267, "Usmall" },			/* LATIN SMALL CAPITAL LETTER U */
    {  268, "Utilde" },			/* LATIN CAPITAL LETTER U WITH TILDE */
    {  269, "V" },			/* LATIN CAPITAL LETTER V */
    {  270, "Vsmall" },			/* LATIN SMALL CAPITAL LETTER V */
    {  271, "W" },			/* LATIN CAPITAL LETTER W */
    {  272, "Wacute" },			/* LATIN CAPITAL LETTER W WITH ACUTE */
    {  273, "Wcircumflex" },		/* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
    {  274, "Wdieresis" },		/* LATIN CAPITAL LETTER W WITH DIAERESIS */
    {  275, "Wgrave" },			/* LATIN CAPITAL LETTER W WITH GRAVE */
    {  276, "Wsmall" },			/* LATIN SMALL CAPITAL LETTER W */
    {  277, "X" },			/* LATIN CAPITAL LETTER X */
    {  278, "Xi" },			/* GREEK CAPITAL LETTER XI */
    {  279, "Xsmall" },			/* LATIN SMALL CAPITAL LETTER X */
    {  280, "Y" },			/* LATIN CAPITAL LETTER Y */
    {  281, "Yacute" },			/* LATIN CAPITAL LETTER Y WITH ACUTE */
    {  282, "Yacutesmall" },		/* LATIN SMALL CAPITAL LETTER Y WITH ACUTE */
    {  283, "Ycircumflex" },		/* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
    {  284, "Ydieresis" },		/* LATIN CAPITAL LETTER Y WITH DIAERESIS */
    {  285, "Ydieresissmall" },		/* LATIN SMALL CAPITAL LETTER Y WITH DIAERESIS */
    {  286, "Ygrave" },			/* LATIN CAPITAL LETTER Y WITH GRAVE */
    {  287, "Ysmall" },			/* LATIN SMALL CAPITAL LETTER Y */
    {  288, "Z" },			/* LATIN CAPITAL LETTER Z */
    {  289, "Zacute" },			/* LATIN CAPITAL LETTER Z WITH ACUTE */
    {  290, "Zcaron" },			/* LATIN CAPITAL LETTER Z WITH CARON */
    {  291, "Zcaronsmall" },		/* LATIN SMALL CAPITAL LETTER Z WITH CARON */
    {  292, "Zdotaccent" },		/* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    {  293, "Zeta" },			/* GREEK CAPITAL LETTER ZETA */
    {  294, "Zsmall" },			/* LATIN SMALL CAPITAL LETTER Z */
    {  295, "a" },			/* LATIN SMALL LETTER A */
    {  296, "a1" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  297, "a10" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  298, "a100" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  299, "a101" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  300, "a102" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  301, "a103" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  302, "a104" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  303, "a105" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  304, "a106" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  305, "a107" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  306, "a108" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  307, "a109" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  308, "a11" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  309, "a110" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  310, "a111" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  311, "a112" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  312, "a117" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  313, "a118" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  314, "a119" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  315, "a12" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  316, "a120" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  317, "a121" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  318, "a122" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  319, "a123" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  320, "a124" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  321, "a125" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  322, "a126" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  323, "a127" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  324, "a128" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  325, "a129" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  326, "a13" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  327, "a130" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  328, "a131" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  329, "a132" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  330, "a133" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  331, "a134" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  332, "a135" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  333, "a136" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  334, "a137" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  335, "a138" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  336, "a139" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  337, "a14" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  338, "a140" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  339, "a141" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  340, "a142" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  341, "a143" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  342, "a144" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  343, "a145" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  344, "a146" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  345, "a147" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  346, "a148" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  347, "a149" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  348, "a15" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  349, "a150" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  350, "a151" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  351, "a152" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  352, "a153" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  353, "a154" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  354, "a155" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  355, "a156" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  356, "a157" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  357, "a158" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  358, "a159" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  359, "a16" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  360, "a160" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  361, "a161" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  362, "a162" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  363, "a163" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  364, "a164" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  365, "a165" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  366, "a166" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  367, "a167" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  368, "a168" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  369, "a169" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  370, "a17" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  371, "a170" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  372, "a171" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  373, "a172" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  374, "a173" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  375, "a174" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  376, "a175" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  377, "a176" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  378, "a177" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  379, "a178" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  380, "a179" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  381, "a18" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  382, "a180" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  383, "a181" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  384, "a182" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  385, "a183" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  386, "a184" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  387, "a185" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  388, "a186" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  389, "a187" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  390, "a188" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  391, "a189" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  392, "a19" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  393, "a190" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  394, "a191" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  395, "a192" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  396, "a193" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  397, "a194" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  398, "a195" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  399, "a196" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  400, "a197" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  401, "a198" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  402, "a199" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  403, "a2" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  404, "a20" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  405, "a200" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  406, "a201" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  407, "a202" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  408, "a203" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  409, "a204" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  410, "a205" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  411, "a206" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  412, "a21" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  413, "a22" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  414, "a23" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  415, "a24" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  416, "a25" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  417, "a26" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  418, "a27" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  419, "a28" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  420, "a29" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  421, "a3" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  422, "a30" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  423, "a31" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  424, "a32" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  425, "a33" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  426, "a34" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  427, "a35" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  428, "a36" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  429, "a37" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  430, "a38" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  431, "a39" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  432, "a4" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  433, "a40" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  434, "a41" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  435, "a42" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  436, "a43" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  437, "a44" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  438, "a45" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  439, "a46" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  440, "a47" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  441, "a48" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  442, "a49" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  443, "a5" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  444, "a50" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  445, "a51" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  446, "a52" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  447, "a53" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  448, "a54" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  449, "a55" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  450, "a56" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  451, "a57" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  452, "a58" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  453, "a59" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  454, "a6" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  455, "a60" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  456, "a61" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  457, "a62" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  458, "a63" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  459, "a64" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  460, "a65" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  461, "a66" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  462, "a67" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  463, "a68" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  464, "a69" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  465, "a7" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  466, "a70" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  467, "a71" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  468, "a72" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  469, "a73" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  470, "a74" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  471, "a75" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  472, "a76" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  473, "a77" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  474, "a78" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  475, "a79" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  476, "a8" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  477, "a81" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  478, "a82" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  479, "a83" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  480, "a84" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  481, "a85" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  482, "a86" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  483, "a87" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  484, "a88" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  485, "a89" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  486, "a9" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  487, "a90" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  488, "a91" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  489, "a92" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  490, "a93" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  491, "a94" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  492, "a95" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  493, "a96" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  494, "a97" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  495, "a98" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  496, "a99" },			/* FONT FAMILY;ITC Zapf Dingbats */
    {  497, "aacute" },			/* LATIN SMALL LETTER A WITH ACUTE */
    {  498, "abreve" },			/* LATIN SMALL LETTER A WITH BREVE */
    {  499, "acircumflex" },		/* LATIN SMALL LETTER A WITH CIRCUMFLEX */
    {  500, "acute" },			/* ACUTE ACCENT */
    {  501, "acutecomb" },		/* COMBINING ACUTE ACCENT */
    {  502, "adieresis" },		/* LATIN SMALL LETTER A WITH DIAERESIS */
    {  503, "ae" },			/* LATIN SMALL LETTER AE */
    {  504, "aeacute" },		/* LATIN SMALL LETTER AE WITH ACUTE */
    {  505, "afii00208" },		/* HORIZONTAL BAR */
    {  506, "afii10017" },		/* CYRILLIC CAPITAL LETTER A */
    {  507, "afii10018" },		/* CYRILLIC CAPITAL LETTER BE */
    {  508, "afii10019" },		/* CYRILLIC CAPITAL LETTER VE */
    {  509, "afii10020" },		/* CYRILLIC CAPITAL LETTER GHE */
    {  510, "afii10021" },		/* CYRILLIC CAPITAL LETTER DE */
    {  511, "afii10022" },		/* CYRILLIC CAPITAL LETTER IE */
    {  512, "afii10023" },		/* CYRILLIC CAPITAL LETTER IO */
    {  513, "afii10024" },		/* CYRILLIC CAPITAL LETTER ZHE */
    {  514, "afii10025" },		/* CYRILLIC CAPITAL LETTER ZE */
    {  515, "afii10026" },		/* CYRILLIC CAPITAL LETTER I */
    {  516, "afii10027" },		/* CYRILLIC CAPITAL LETTER SHORT I */
    {  517, "afii10028" },		/* CYRILLIC CAPITAL LETTER KA */
    {  518, "afii10029" },		/* CYRILLIC CAPITAL LETTER EL */
    {  519, "afii10030" },		/* CYRILLIC CAPITAL LETTER EM */
    {  520, "afii10031" },		/* CYRILLIC CAPITAL LETTER EN */
    {  521, "afii10032" },		/* CYRILLIC CAPITAL LETTER O */
    {  522, "afii10033" },		/* CYRILLIC CAPITAL LETTER PE */
    {  523, "afii10034" },		/* CYRILLIC CAPITAL LETTER ER */
    {  524, "afii10035" },		/* CYRILLIC CAPITAL LETTER ES */
    {  525, "afii10036" },		/* CYRILLIC CAPITAL LETTER TE */
    {  526, "afii10037" },		/* CYRILLIC CAPITAL LETTER U */
    {  527, "afii10038" },		/* CYRILLIC CAPITAL LETTER EF */
    {  528, "afii10039" },		/* CYRILLIC CAPITAL LETTER HA */
    {  529, "afii10040" },		/* CYRILLIC CAPITAL LETTER TSE */
    {  530, "afii10041" },		/* CYRILLIC CAPITAL LETTER CHE */
    {  531, "afii10042" },		/* CYRILLIC CAPITAL LETTER SHA */
    {  532, "afii10043" },		/* CYRILLIC CAPITAL LETTER SHCHA */
    {  533, "afii10044" },		/* CYRILLIC CAPITAL LETTER HARD SIGN */
    {  534, "afii10045" },		/* CYRILLIC CAPITAL LETTER YERU */
    {  535, "afii10046" },		/* CYRILLIC CAPITAL LETTER SOFT SIGN */
    {  536, "afii10047" },		/* CYRILLIC CAPITAL LETTER E */
    {  537, "afii10048" },		/* CYRILLIC CAPITAL LETTER YU */
    {  538, "afii10049" },		/* CYRILLIC CAPITAL LETTER YA */
    {  539, "afii10050" },		/* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
    {  540, "afii10051" },		/* CYRILLIC CAPITAL LETTER DJE */
    {  541, "afii10052" },		/* CYRILLIC CAPITAL LETTER GJE */
    {  542, "afii10053" },		/* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    {  543, "afii10054" },		/* CYRILLIC CAPITAL LETTER DZE */
    {  544, "afii10055" },		/* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    {  545, "afii10056" },		/* CYRILLIC CAPITAL LETTER YI */
    {  546, "afii10057" },		/* CYRILLIC CAPITAL LETTER JE */
    {  547, "afii10058" },		/* CYRILLIC CAPITAL LETTER LJE */
    {  548, "afii10059" },		/* CYRILLIC CAPITAL LETTER NJE */
    {  549, "afii10060" },		/* CYRILLIC CAPITAL LETTER TSHE */
    {  550, "afii10061" },		/* CYRILLIC CAPITAL LETTER KJE */
    {  551, "afii10062" },		/* CYRILLIC CAPITAL LETTER SHORT U */
    {  552, "afii10063" },		/* CYRILLIC SMALL LETTER GHE VARIANT */
    {  553, "afii10064" },		/* CYRILLIC SMALL LETTER BE VARIANT */
    {  554, "afii10065" },		/* CYRILLIC SMALL LETTER A */
    {  555, "afii10066" },		/* CYRILLIC SMALL LETTER BE */
    {  556, "afii10067" },		/* CYRILLIC SMALL LETTER VE */
    {  557, "afii10068" },		/* CYRILLIC SMALL LETTER GHE */
    {  558, "afii10069" },		/* CYRILLIC SMALL LETTER DE */
    {  559, "afii10070" },		/* CYRILLIC SMALL LETTER IE */
    {  560, "afii10071" },		/* CYRILLIC SMALL LETTER IO */
    {  561, "afii10072" },		/* CYRILLIC SMALL LETTER ZHE */
    {  562, "afii10073" },		/* CYRILLIC SMALL LETTER ZE */
    {  563, "afii10074" },		/* CYRILLIC SMALL LETTER I */
    {  564, "afii10075" },		/* CYRILLIC SMALL LETTER SHORT I */
    {  565, "afii10076" },		/* CYRILLIC SMALL LETTER KA */
    {  566, "afii10077" },		/* CYRILLIC SMALL LETTER EL */
    {  567, "afii10078" },		/* CYRILLIC SMALL LETTER EM */
    {  568, "afii10079" },		/* CYRILLIC SMALL LETTER EN */
    {  569, "afii10080" },		/* CYRILLIC SMALL LETTER O */
    {  570, "afii10081" },		/* CYRILLIC SMALL LETTER PE */
    {  571, "afii10082" },		/* CYRILLIC SMALL LETTER ER */
    {  572, "afii10083" },		/* CYRILLIC SMALL LETTER ES */
    {  573, "afii10084" },		/* CYRILLIC SMALL LETTER TE */
    {  574, "afii10085" },		/* CYRILLIC SMALL LETTER U */
    {  575, "afii10086" },		/* CYRILLIC SMALL LETTER EF */
    {  576, "afii10087" },		/* CYRILLIC SMALL LETTER HA */
    {  577, "afii10088" },		/* CYRILLIC SMALL LETTER TSE */
    {  578, "afii10089" },		/* CYRILLIC SMALL LETTER CHE */
    {  579, "afii10090" },		/* CYRILLIC SMALL LETTER SHA */
    {  580, "afii10091" },		/* CYRILLIC SMALL LETTER SHCHA */
    {  581, "afii10092" },		/* CYRILLIC SMALL LETTER HARD SIGN */
    {  582, "afii10093" },		/* CYRILLIC SMALL LETTER YERU */
    {  583, "afii10094" },		/* CYRILLIC SMALL LETTER SOFT SIGN */
    {  584, "afii10095" },		/* CYRILLIC SMALL LETTER E */
    {  585, "afii10096" },		/* CYRILLIC SMALL LETTER YU */
    {  586, "afii10097" },		/* CYRILLIC SMALL LETTER YA */
    {  587, "afii10098" },		/* CYRILLIC SMALL LETTER GHE WITH UPTURN */
    {  588, "afii10099" },		/* CYRILLIC SMALL LETTER DJE */
    {  589, "afii10100" },		/* CYRILLIC SMALL LETTER GJE */
    {  590, "afii10101" },		/* CYRILLIC SMALL LETTER UKRAINIAN IE */
    {  591, "afii10102" },		/* CYRILLIC SMALL LETTER DZE */
    {  592, "afii10103" },		/* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    {  593, "afii10104" },		/* CYRILLIC SMALL LETTER YI */
    {  594, "afii10105" },		/* CYRILLIC SMALL LETTER JE */
    {  595, "afii10106" },		/* CYRILLIC SMALL LETTER LJE */
    {  596, "afii10107" },		/* CYRILLIC SMALL LETTER NJE */
    {  597, "afii10108" },		/* CYRILLIC SMALL LETTER TSHE */
    {  598, "afii10109" },		/* CYRILLIC SMALL LETTER KJE */
    {  599, "afii10110" },		/* CYRILLIC SMALL LETTER SHORT U */
    {  600, "afii10145" },		/* CYRILLIC CAPITAL LETTER DZHE */
    {  601, "afii10146" },		/* CYRILLIC CAPITAL LETTER YAT */
    {  602, "afii10147" },		/* CYRILLIC CAPITAL LETTER FITA */
    {  603, "afii10148" },		/* CYRILLIC CAPITAL LETTER IZHITSA */
    {  604, "afii10192" },		/* CYRILLIC SMALL LETTER DE VARIANT */
    {  605, "afii10193" },		/* CYRILLIC SMALL LETTER DZHE */
    {  606, "afii10194" },		/* CYRILLIC SMALL LETTER YAT */
    {  607, "afii10195" },		/* CYRILLIC SMALL LETTER FITA */
    {  608, "afii10196" },		/* CYRILLIC SMALL LETTER IZHITSA */
    {  609, "afii10831" },		/* CYRILLIC SMALL LETTER PE VARIANT */
    {  610, "afii10832" },		/* CYRILLIC SMALL LETTER TE VARIANT */
    {  611, "afii10846" },		/* CYRILLIC SMALL LETTER SCHWA */
    {  612, "afii299" },		/* LEFT-TO-RIGHT MARK */
    {  613, "afii300" },		/* RIGHT-TO-LEFT MARK */
    {  614, "afii301" },		/* ZERO WIDTH JOINER */
    {  615, "afii57381" },		/* ARABIC PERCENT SIGN */
    {  616, "afii57388" },		/* ARABIC COMMA */
    {  617, "afii57392" },		/* ARABIC-INDIC DIGIT ZERO */
    {  618, "afii57393" },		/* ARABIC-INDIC DIGIT ONE */
    {  619, "afii57394" },		/* ARABIC-INDIC DIGIT TWO */
    {  620, "afii57395" },		/* ARABIC-INDIC DIGIT THREE */
    {  621, "afii57396" },		/* ARABIC-INDIC DIGIT FOUR */
    {  622, "afii57397" },		/* ARABIC-INDIC DIGIT FIVE */
    {  623, "afii57398" },		/* ARABIC-INDIC DIGIT SIX */
    {  624, "afii57399" },		/* ARABIC-INDIC DIGIT SEVEN */
    {  625, "afii57400" },		/* ARABIC-INDIC DIGIT EIGHT */
    {  626, "afii57401" },		/* ARABIC-INDIC DIGIT NINE */
    {  627, "afii57403" },		/* ARABIC SEMICOLON */
    {  628, "afii57407" },		/* ARABIC QUESTION MARK */
    {  629, "afii57409" },		/* ARABIC LETTER HAMZA */
    {  630, "afii57410" },		/* ARABIC LETTER ALEF WITH MADDA ABOVE */
    {  631, "afii57411" },		/* ARABIC LETTER ALEF WITH HAMZA ABOVE */
    {  632, "afii57412" },		/* ARABIC LETTER WAW WITH HAMZA ABOVE */
    {  633, "afii57413" },		/* ARABIC LETTER ALEF WITH HAMZA BELOW */
    {  634, "afii57414" },		/* ARABIC LETTER YEH WITH HAMZA ABOVE */
    {  635, "afii57415" },		/* ARABIC LETTER ALEF */
    {  636, "afii57416" },		/* ARABIC LETTER BEH */
    {  637, "afii57417" },		/* ARABIC LETTER TEH MARBUTA */
    {  638, "afii57418" },		/* ARABIC LETTER TEH */
    {  639, "afii57419" },		/* ARABIC LETTER THEH */
    {  640, "afii57420" },		/* ARABIC LETTER JEEM */
    {  641, "afii57421" },		/* ARABIC LETTER HAH */
    {  642, "afii57422" },		/* ARABIC LETTER KHAH */
    {  643, "afii57423" },		/* ARABIC LETTER DAL */
    {  644, "afii57424" },		/* ARABIC LETTER THAL */
    {  645, "afii57425" },		/* ARABIC LETTER REH */
    {  646, "afii57426" },		/* ARABIC LETTER ZAIN */
    {  647, "afii57427" },		/* ARABIC LETTER SEEN */
    {  648, "afii57428" },		/* ARABIC LETTER SHEEN */
    {  649, "afii57429" },		/* ARABIC LETTER SAD */
    {  650, "afii57430" },		/* ARABIC LETTER DAD */
    {  651, "afii57431" },		/* ARABIC LETTER TAH */
    {  652, "afii57432" },		/* ARABIC LETTER ZAH */
    {  653, "afii57433" },		/* ARABIC LETTER AIN */
    {  654, "afii57434" },		/* ARABIC LETTER GHAIN */
    {  655, "afii57440" },		/* ARABIC TATWEEL */
    {  656, "afii57441" },		/* ARABIC LETTER FEH */
    {  657, "afii57442" },		/* ARABIC LETTER QAF */
    {  658, "afii57443" },		/* ARABIC LETTER KAF */
    {  659, "afii57444" },		/* ARABIC LETTER LAM */
    {  660, "afii57445" },		/* ARABIC LETTER MEEM */
    {  661, "afii57446" },		/* ARABIC LETTER NOON */
    {  662, "afii57448" },		/* ARABIC LETTER WAW */
    {  663, "afii57449" },		/* ARABIC LETTER ALEF MAKSURA */
    {  664, "afii57450" },		/* ARABIC LETTER YEH */
    {  665, "afii57451" },		/* ARABIC FATHATAN */
    {  666, "afii57452" },		/* ARABIC DAMMATAN */
    {  667, "afii57453" },		/* ARABIC KASRATAN */
    {  668, "afii57454" },		/* ARABIC FATHA */
    {  669, "afii57455" },		/* ARABIC DAMMA */
    {  670, "afii57456" },		/* ARABIC KASRA */
    {  671, "afii57457" },		/* ARABIC SHADDA */
    {  672, "afii57458" },		/* ARABIC SUKUN */
    {  673, "afii57470" },		/* ARABIC LETTER HEH */
    {  674, "afii57505" },		/* ARABIC LETTER VEH */
    {  675, "afii57506" },		/* ARABIC LETTER PEH */
    {  676, "afii57507" },		/* ARABIC LETTER TCHEH */
    {  677, "afii57508" },		/* ARABIC LETTER JEH */
    {  678, "afii57509" },		/* ARABIC LETTER GAF */
    {  679, "afii57511" },		/* ARABIC LETTER TTEH */
    {  680, "afii57512" },		/* ARABIC LETTER DDAL */
    {  681, "afii57513" },		/* ARABIC LETTER RREH */
    {  682, "afii57514" },		/* ARABIC LETTER NOON GHUNNA */
    {  683, "afii57519" },		/* ARABIC LETTER YEH BARREE */
    {  684, "afii57534" },		/* ARABIC LETTER AE */
    {  685, "afii57636" },		/* NEW SHEQEL SIGN */
    {  686, "afii57645" },		/* HEBREW PUNCTUATION MAQAF */
    {  687, "afii57658" },		/* HEBREW PUNCTUATION SOF PASUQ */
    {  688, "afii57664" },		/* HEBREW LETTER ALEF */
    {  689, "afii57665" },		/* HEBREW LETTER BET */
    {  690, "afii57666" },		/* HEBREW LETTER GIMEL */
    {  691, "afii57667" },		/* HEBREW LETTER DALET */
    {  692, "afii57668" },		/* HEBREW LETTER HE */
    {  693, "afii57669" },		/* HEBREW LETTER VAV */
    {  694, "afii57670" },		/* HEBREW LETTER ZAYIN */
    {  695, "afii57671" },		/* HEBREW LETTER HET */
    {  696, "afii57672" },		/* HEBREW LETTER TET */
    {  697, "afii57673" },		/* HEBREW LETTER YOD */
    {  698, "afii57674" },		/* HEBREW LETTER FINAL KAF */
    {  699, "afii57675" },		/* HEBREW LETTER KAF */
    {  700, "afii57676" },		/* HEBREW LETTER LAMED */
    {  701, "afii57677" },		/* HEBREW LETTER FINAL MEM */
    {  702, "afii57678" },		/* HEBREW LETTER MEM */
    {  703, "afii57679" },		/* HEBREW LETTER FINAL NUN */
    {  704, "afii57680" },		/* HEBREW LETTER NUN */
    {  705, "afii57681" },		/* HEBREW LETTER SAMEKH */
    {  706, "afii57682" },		/* HEBREW LETTER AYIN */
    {  707, "afii57683" },		/* HEBREW LETTER FINAL PE */
    {  708, "afii57684" },		/* HEBREW LETTER PE */
    {  709, "afii57685" },		/* HEBREW LETTER FINAL TSADI */
    {  710, "afii57686" },		/* HEBREW LETTER TSADI */
    {  711, "afii57687" },		/* HEBREW LETTER QOF */
    {  712, "afii57688" },		/* HEBREW LETTER RESH */
    {  713, "afii57689" },		/* HEBREW LETTER SHIN */
    {  714, "afii57690" },		/* HEBREW LETTER TAV */
    {  715, "afii57694" },		/* HEBREW LETTER SHIN WITH SHIN DOT */
    {  716, "afii57695" },		/* HEBREW LETTER SHIN WITH SIN DOT */
    {  717, "afii57700" },		/* HEBREW LETTER VAV WITH HOLAM */
    {  718, "afii57705" },		/* HEBREW LIGATURE YIDDISH YOD YOD PATAH */
    {  719, "afii57716" },		/* HEBREW LIGATURE YIDDISH DOUBLE VAV */
    {  720, "afii57717" },		/* HEBREW LIGATURE YIDDISH VAV YOD */
    {  721, "afii57718" },		/* HEBREW LIGATURE YIDDISH DOUBLE YOD */
    {  722, "afii57723" },		/* HEBREW LETTER VAV WITH DAGESH */
    {  723, "afii57793" },		/* HEBREW POINT HIRIQ */
    {  724, "afii57794" },		/* HEBREW POINT TSERE */
    {  725, "afii57795" },		/* HEBREW POINT SEGOL */
    {  726, "afii57796" },		/* HEBREW POINT QUBUTS */
    {  727, "afii57797" },		/* HEBREW POINT QAMATS */
    {  728, "afii57798" },		/* HEBREW POINT PATAH */
    {  729, "afii57799" },		/* HEBREW POINT SHEVA */
    {  730, "afii57800" },		/* HEBREW POINT HATAF PATAH */
    {  731, "afii57801" },		/* HEBREW POINT HATAF SEGOL */
    {  732, "afii57802" },		/* HEBREW POINT HATAF QAMATS */
    {  733, "afii57803" },		/* HEBREW POINT SIN DOT */
    {  734, "afii57804" },		/* HEBREW POINT SHIN DOT */
    {  735, "afii57806" },		/* HEBREW POINT HOLAM */
    {  736, "afii57807" },		/* HEBREW POINT DAGESH OR MAPIQ */
    {  737, "afii57839" },		/* HEBREW POINT METEG */
    {  738, "afii57841" },		/* HEBREW POINT RAFE */
    {  739, "afii57842" },		/* HEBREW PUNCTUATION PASEQ */
    {  740, "afii57929" },		/* MODIFIER LETTER APOSTROPHE */
    {  741, "afii61248" },		/* CARE OF */
    {  742, "afii61289" },		/* SCRIPT SMALL L */
    {  743, "afii61352" },		/* NUMERO SIGN */
    {  744, "afii61573" },		/* POP DIRECTIONAL FORMATTING */
    {  745, "afii61574" },		/* LEFT-TO-RIGHT OVERRIDE */
    {  746, "afii61575" },		/* RIGHT-TO-LEFT OVERRIDE */
    {  747, "afii61664" },		/* ZERO WIDTH NON-JOINER */
    {  748, "afii63167" },		/* ARABIC FIVE POINTED STAR */
    {  749, "afii64937" },		/* MODIFIER LETTER REVERSED COMMA */
    {  750, "agrave" },			/* LATIN SMALL LETTER A WITH GRAVE */
    {  751, "aleph" },			/* ALEF SYMBOL */
    {  752, "alpha" },			/* GREEK SMALL LETTER ALPHA */
    {  753, "alphatonos" },		/* GREEK SMALL LETTER ALPHA WITH TONOS */
    {  754, "amacron" },		/* LATIN SMALL LETTER A WITH MACRON */
    {  755, "ampersand" },		/* AMPERSAND */
    {  756, "ampersandsmall" },		/* SMALL CAPITAL AMPERSAND */
    {  757, "angle" },			/* ANGLE */
    {  758, "angleleft" },		/* LEFT-POINTING ANGLE BRACKET */
    {  759, "angleright" },		/* RIGHT-POINTING ANGLE BRACKET */
    {  760, "anoteleia" },		/* GREEK ANO TELEIA */
    {  761, "aogonek" },		/* LATIN SMALL LETTER A WITH OGONEK */
    {  762, "apple" },			/* FONT FAMILY;Symbol */
    {  763, "approxequal" },		/* ALMOST EQUAL TO */
    {  764, "aring" },			/* LATIN SMALL LETTER A WITH RING ABOVE */
    {  765, "aringacute" },		/* LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE */
    {  766, "arrowboth" },		/* LEFT RIGHT ARROW */
    {  767, "arrowdblboth" },		/* LEFT RIGHT DOUBLE ARROW */
    {  768, "arrowdbldown" },		/* DOWNWARDS DOUBLE ARROW */
    {  769, "arrowdblleft" },		/* LEFTWARDS DOUBLE ARROW */
    {  770, "arrowdblright" },		/* RIGHTWARDS DOUBLE ARROW */
    {  771, "arrowdblup" },		/* UPWARDS DOUBLE ARROW */
    {  772, "arrowdown" },		/* DOWNWARDS ARROW */
    {  773, "arrowhorizex" },		/* HORIZONTAL ARROW EXTENDER */
    {  774, "arrowleft" },		/* LEFTWARDS ARROW */
    {  775, "arrowright" },		/* RIGHTWARDS ARROW */
    {  776, "arrowup" },		/* UPWARDS ARROW */
    {  777, "arrowupdn" },		/* UP DOWN ARROW */
    {  778, "arrowupdnbse" },		/* UP DOWN ARROW WITH BASE */
    {  779, "arrowvertex" },		/* VERTICAL ARROW EXTENDER */
    {  780, "asciicircum" },		/* CIRCUMFLEX ACCENT */
    {  781, "asciitilde" },		/* TILDE */
    {  782, "asterisk" },		/* ASTERISK */
    {  783, "asteriskmath" },		/* ASTERISK OPERATOR */
    {  784, "asuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER A */
    {  785, "at" },			/* COMMERCIAL AT */
    {  786, "atilde" },			/* LATIN SMALL LETTER A WITH TILDE */
    {  787, "b" },			/* LATIN SMALL LETTER B */
    {  788, "backslash" },		/* REVERSE SOLIDUS */
    {  789, "bar" },			/* VERTICAL LINE */
    {  790, "beta" },			/* GREEK SMALL LETTER BETA */
    {  791, "block" },			/* FULL BLOCK */
    {  792, "braceex" },		/* CURLY BRACKET EXTENDER */
    {  793, "braceleft" },		/* LEFT CURLY BRACKET */
    {  794, "braceleftbt" },		/* LEFT CURLY BRACKET BOTTOM */
    {  795, "braceleftmid" },		/* LEFT CURLY BRACKET MID */
    {  796, "bracelefttp" },		/* LEFT CURLY BRACKET TOP */
    {  797, "braceright" },		/* RIGHT CURLY BRACKET */
    {  798, "bracerightbt" },		/* RIGHT CURLY BRACKET BOTTOM */
    {  799, "bracerightmid" },		/* RIGHT CURLY BRACKET MID */
    {  800, "bracerighttp" },		/* RIGHT CURLY BRACKET TOP */
    {  801, "bracketleft" },		/* LEFT SQUARE BRACKET */
    {  802, "bracketleftbt" },		/* LEFT SQUARE BRACKET BOTTOM */
    {  803, "bracketleftex" },		/* LEFT SQUARE BRACKET EXTENDER */
    {  804, "bracketlefttp" },		/* LEFT SQUARE BRACKET TOP */
    {  805, "bracketright" },		/* RIGHT SQUARE BRACKET */
    {  806, "bracketrightbt" },		/* RIGHT SQUARE BRACKET BOTTOM */
    {  807, "bracketrightex" },		/* RIGHT SQUARE BRACKET EXTENDER */
    {  808, "bracketrighttp" },		/* RIGHT SQUARE BRACKET TOP */
    {  809, "breve" },			/* BREVE */
    {  810, "brokenbar" },		/* BROKEN BAR */
    {  811, "bsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER B */
    {  812, "bullet" },			/* BULLET */
    {  813, "c" },			/* LATIN SMALL LETTER C */
    {  814, "cacute" },			/* LATIN SMALL LETTER C WITH ACUTE */
    {  815, "caron" },			/* CARON */
    {  816, "carriagereturn" },		/* DOWNWARDS ARROW WITH CORNER LEFTWARDS */
    {  817, "ccaron" },			/* LATIN SMALL LETTER C WITH CARON */
    {  818, "ccedilla" },		/* LATIN SMALL LETTER C WITH CEDILLA */
    {  819, "ccircumflex" },		/* LATIN SMALL LETTER C WITH CIRCUMFLEX */
    {  820, "cdotaccent" },		/* LATIN SMALL LETTER C WITH DOT ABOVE */
    {  821, "cedilla" },		/* CEDILLA */
    {  822, "cent" },			/* CENT SIGN */
    {  823, "center" },			/* FONT FAMILY;Courier */
    {  824, "centinferior" },		/* SUBSCRIPT CENT SIGN */
    {  825, "centoldstyle" },		/* OLDSTYLE CENT SIGN */
    {  826, "centsuperior" },		/* SUPERSCRIPT CENT SIGN */
    {  827, "chi" },			/* GREEK SMALL LETTER CHI */
    {  828, "circle" },			/* WHITE CIRCLE */
    {  829, "circlemultiply" },		/* CIRCLED TIMES */
    {  830, "circleplus" },		/* CIRCLED PLUS */
    {  831, "circumflex" },		/* MODIFIER LETTER CIRCUMFLEX ACCENT */
    {  832, "club" },			/* BLACK CLUB SUIT */
    {  833, "colon" },			/* COLON */
    {  834, "colonmonetary" },		/* COLON SIGN */
    {  835, "comma" },			/* COMMA */
    {  836, "commaaccent" },		/* COMMA BELOW */
    {  837, "commainferior" },		/* SUBSCRIPT COMMA */
    {  838, "commasuperior" },		/* SUPERSCRIPT COMMA */
    {  839, "congruent" },		/* APPROXIMATELY EQUAL TO */
    {  840, "copyright" },		/* COPYRIGHT SIGN */
    {  841, "copyrightsans" },		/* COPYRIGHT SIGN SANS SERIF */
    {  842, "copyrightserif" },		/* COPYRIGHT SIGN SERIF */
    {  843, "currency" },		/* CURRENCY SIGN */
    {  844, "cyrBreve" },		/* CAPITAL CYRILLIC BREVE */
    {  845, "cyrFlex" },		/* CAPITAL CYRILLIC CIRCUMFLEX */
    {  846, "cyrbreve" },		/* CYRILLIC BREVE */
    {  847, "cyrflex" },		/* CYRILLIC CIRCUMFLEX */
    {  848, "d" },			/* LATIN SMALL LETTER D */
    {  849, "dagger" },			/* DAGGER */
    {  850, "daggerdbl" },		/* DOUBLE DAGGER */
    {  851, "dblGrave" },		/* CAPITAL DOUBLE GRAVE ACCENT */
    {  852, "dblgrave" },		/* DOUBLE GRAVE ACCENT */
    {  853, "dcaron" },			/* LATIN SMALL LETTER D WITH CARON */
    {  854, "dcroat" },			/* LATIN SMALL LETTER D WITH STROKE */
    {  855, "dectab" },			/* FONT FAMILY;Courier */
    {  856, "degree" },			/* DEGREE SIGN */
    {  857, "delta" },			/* GREEK SMALL LETTER DELTA */
    {  858, "diamond" },		/* BLACK DIAMOND SUIT */
    {  859, "dieresis" },		/* DIAERESIS */
    {  860, "dieresisacute" },		/* DIAERESIS ACUTE ACCENT */
    {  861, "dieresisgrave" },		/* DIAERESIS GRAVE ACCENT */
    {  862, "dieresistonos" },		/* GREEK DIALYTIKA TONOS */
    {  863, "divide" },			/* DIVISION SIGN */
    {  864, "dkshade" },		/* DARK SHADE */
    {  865, "dnblock" },		/* LOWER HALF BLOCK */
    {  866, "dollar" },			/* DOLLAR SIGN */
    {  867, "dollarinferior" },		/* SUBSCRIPT DOLLAR SIGN */
    {  868, "dollaroldstyle" },		/* OLDSTYLE DOLLAR SIGN */
    {  869, "dollarsuperior" },		/* SUPERSCRIPT DOLLAR SIGN */
    {  870, "dong" },			/* DONG SIGN */
    {  871, "dotaccent" },		/* DOT ABOVE */
    {  872, "dotbelowcomb" },		/* COMBINING DOT BELOW */
    {  873, "dotlessi" },		/* LATIN SMALL LETTER DOTLESS I */
    {  874, "dotlessj" },		/* LATIN SMALL LETTER DOTLESS J */
    {  875, "dotmath" },		/* DOT OPERATOR */
    {  876, "down" },			/* FONT FAMILY;Courier */
    {  877, "dsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER D */
    {  878, "e" },			/* LATIN SMALL LETTER E */
    {  879, "eacute" },			/* LATIN SMALL LETTER E WITH ACUTE */
    {  880, "ebreve" },			/* LATIN SMALL LETTER E WITH BREVE */
    {  881, "ecaron" },			/* LATIN SMALL LETTER E WITH CARON */
    {  882, "ecircumflex" },		/* LATIN SMALL LETTER E WITH CIRCUMFLEX */
    {  883, "edieresis" },		/* LATIN SMALL LETTER E WITH DIAERESIS */
    {  884, "edotaccent" },		/* LATIN SMALL LETTER E WITH DOT ABOVE */
    {  885, "egrave" },			/* LATIN SMALL LETTER E WITH GRAVE */
    {  886, "eight" },			/* DIGIT EIGHT */
    {  887, "eightinferior" },		/* SUBSCRIPT EIGHT */
    {  888, "eightoldstyle" },		/* OLDSTYLE DIGIT EIGHT */
    {  889, "eightsuperior" },		/* SUPERSCRIPT EIGHT */
    {  890, "element" },		/* ELEMENT OF */
    {  891, "ellipsis" },		/* HORIZONTAL ELLIPSIS */
    {  892, "emacron" },		/* LATIN SMALL LETTER E WITH MACRON */
    {  893, "emdash" },			/* EM DASH */
    {  894, "emptyset" },		/* EMPTY SET */
    {  895, "endash" },			/* EN DASH */
    {  896, "eng" },			/* LATIN SMALL LETTER ENG */
    {  897, "eogonek" },		/* LATIN SMALL LETTER E WITH OGONEK */
    {  898, "epsilon" },		/* GREEK SMALL LETTER EPSILON */
    {  899, "epsilontonos" },		/* GREEK SMALL LETTER EPSILON WITH TONOS */
    {  900, "equal" },			/* EQUALS SIGN */
    {  901, "equivalence" },		/* IDENTICAL TO */
    {  902, "estimated" },		/* ESTIMATED SYMBOL */
    {  903, "esuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER E */
    {  904, "eta" },			/* GREEK SMALL LETTER ETA */
    {  905, "etatonos" },		/* GREEK SMALL LETTER ETA WITH TONOS */
    {  906, "eth" },			/* LATIN SMALL LETTER ETH */
    {  907, "exclam" },			/* EXCLAMATION MARK */
    {  908, "exclamdbl" },		/* DOUBLE EXCLAMATION MARK */
    {  909, "exclamdown" },		/* INVERTED EXCLAMATION MARK */
    {  910, "exclamdownsmall" },	/* SMALL CAPITAL INVERTED EXCLAMATION MARK */
    {  911, "exclamsmall" },		/* SMALL CAPITAL EXCLAMATION MARK */
    {  912, "existential" },		/* THERE EXISTS */
    {  913, "f" },			/* LATIN SMALL LETTER F */
    {  914, "female" },			/* FEMALE SIGN */
    {  915, "ff" },			/* LATIN SMALL LIGATURE FF */
    {  916, "ffi" },			/* LATIN SMALL LIGATURE FFI */
    {  917, "ffl" },			/* LATIN SMALL LIGATURE FFL */
    {  918, "fi" },			/* LATIN SMALL LIGATURE FI */
    {  919, "figuredash" },		/* FIGURE DASH */
    {  920, "filledbox" },		/* BLACK SQUARE */
    {  921, "filledrect" },		/* BLACK RECTANGLE */
    {  922, "five" },			/* DIGIT FIVE */
    {  923, "fiveeighths" },		/* VULGAR FRACTION FIVE EIGHTHS */
    {  924, "fiveinferior" },		/* SUBSCRIPT FIVE */
    {  925, "fiveoldstyle" },		/* OLDSTYLE DIGIT FIVE */
    {  926, "fivesuperior" },		/* SUPERSCRIPT FIVE */
    {  927, "fl" },			/* LATIN SMALL LIGATURE FL */
    {  928, "florin" },			/* LATIN SMALL LETTER F WITH HOOK */
    {  929, "format" },			/* FONT FAMILY;Courier */
    {  930, "four" },			/* DIGIT FOUR */
    {  931, "fourinferior" },		/* SUBSCRIPT FOUR */
    {  932, "fouroldstyle" },		/* OLDSTYLE DIGIT FOUR */
    {  933, "foursuperior" },		/* SUPERSCRIPT FOUR */
    {  934, "fraction" },		/* FRACTION SLASH */
					/* DIVISION SLASH;Duplicate */
    {  935, "franc" },			/* FRENCH FRANC SIGN */
    {  936, "g" },			/* LATIN SMALL LETTER G */
    {  937, "gamma" },			/* GREEK SMALL LETTER GAMMA */
    {  938, "gbreve" },			/* LATIN SMALL LETTER G WITH BREVE */
    {  939, "gcaron" },			/* LATIN SMALL LETTER G WITH CARON */
    {  940, "gcircumflex" },		/* LATIN SMALL LETTER G WITH CIRCUMFLEX */
    {  941, "gcommaaccent" },		/* LATIN SMALL LETTER G WITH CEDILLA */
    {  942, "gdotaccent" },		/* LATIN SMALL LETTER G WITH DOT ABOVE */
    {  943, "germandbls" },		/* LATIN SMALL LETTER SHARP S */
    {  944, "gradient" },		/* NABLA */
    {  945, "grave" },			/* GRAVE ACCENT */
    {  946, "gravecomb" },		/* COMBINING GRAVE ACCENT */
    {  947, "graybox" },		/* FONT FAMILY;Courier */
    {  948, "greater" },		/* GREATER-THAN SIGN */
    {  949, "greaterequal" },		/* GREATER-THAN OR EQUAL TO */
    {  950, "guillemotleft" },		/* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
    {  951, "guillemotright" },		/* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
    {  952, "guilsinglleft" },		/* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
    {  953, "guilsinglright" },		/* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
    {  954, "h" },			/* LATIN SMALL LETTER H */
    {  955, "hbar" },			/* LATIN SMALL LETTER H WITH STROKE */
    {  956, "hcircumflex" },		/* LATIN SMALL LETTER H WITH CIRCUMFLEX */
    {  957, "heart" },			/* BLACK HEART SUIT */
    {  958, "hookabovecomb" },		/* COMBINING HOOK ABOVE */
    {  959, "house" },			/* HOUSE */
    {  960, "hungarumlaut" },		/* DOUBLE ACUTE ACCENT */
    {  961, "hyphen" },			/* HYPHEN-MINUS */
					/* SOFT HYPHEN;Duplicate */
    {  962, "hypheninferior" },		/* SUBSCRIPT HYPHEN-MINUS */
    {  963, "hyphensuperior" },		/* SUPERSCRIPT HYPHEN-MINUS */
    {  964, "i" },			/* LATIN SMALL LETTER I */
    {  965, "iacute" },			/* LATIN SMALL LETTER I WITH ACUTE */
    {  966, "ibreve" },			/* LATIN SMALL LETTER I WITH BREVE */
    {  967, "icircumflex" },		/* LATIN SMALL LETTER I WITH CIRCUMFLEX */
    {  968, "idieresis" },		/* LATIN SMALL LETTER I WITH DIAERESIS */
    {  969, "igrave" },			/* LATIN SMALL LETTER I WITH GRAVE */
    {  970, "ij" },			/* LATIN SMALL LIGATURE IJ */
    {  971, "imacron" },		/* LATIN SMALL LETTER I WITH MACRON */
    {  972, "indent" },			/* FONT FAMILY;Courier */
    {  973, "infinity" },		/* INFINITY */
    {  974, "integral" },		/* INTEGRAL */
    {  975, "integralbt" },		/* BOTTOM HALF INTEGRAL */
    {  976, "integralex" },		/* INTEGRAL EXTENDER */
    {  977, "integraltp" },		/* TOP HALF INTEGRAL */
    {  978, "intersection" },		/* INTERSECTION */
    {  979, "invbullet" },		/* INVERSE BULLET */
    {  980, "invcircle" },		/* INVERSE WHITE CIRCLE */
    {  981, "invsmileface" },		/* BLACK SMILING FACE */
    {  982, "iogonek" },		/* LATIN SMALL LETTER I WITH OGONEK */
    {  983, "iota" },			/* GREEK SMALL LETTER IOTA */
    {  984, "iotadieresis" },		/* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    {  985, "iotadieresistonos" },	/* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    {  986, "iotatonos" },		/* GREEK SMALL LETTER IOTA WITH TONOS */
    {  987, "isuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER I */
    {  988, "itilde" },			/* LATIN SMALL LETTER I WITH TILDE */
    {  989, "j" },			/* LATIN SMALL LETTER J */
    {  990, "jcircumflex" },		/* LATIN SMALL LETTER J WITH CIRCUMFLEX */
    {  991, "k" },			/* LATIN SMALL LETTER K */
    {  992, "kappa" },			/* GREEK SMALL LETTER KAPPA */
    {  993, "kcommaaccent" },		/* LATIN SMALL LETTER K WITH CEDILLA */
    {  994, "kgreenlandic" },		/* LATIN SMALL LETTER KRA */
    {  995, "l" },			/* LATIN SMALL LETTER L */
    {  996, "lacute" },			/* LATIN SMALL LETTER L WITH ACUTE */
    {  997, "lambda" },			/* GREEK SMALL LETTER LAMDA */
    {  998, "largebullet" },		/* FONT FAMILY;Courier */
    {  999, "lcaron" },			/* LATIN SMALL LETTER L WITH CARON */
    { 1000, "lcommaaccent" },		/* LATIN SMALL LETTER L WITH CEDILLA */
    { 1001, "ldot" },			/* LATIN SMALL LETTER L WITH MIDDLE DOT */
    { 1002, "left" },			/* FONT FAMILY;Courier */
    { 1003, "less" },			/* LESS-THAN SIGN */
    { 1004, "lessequal" },		/* LESS-THAN OR EQUAL TO */
    { 1005, "lfblock" },		/* LEFT HALF BLOCK */
    { 1006, "lira" },			/* LIRA SIGN */
    { 1007, "ll" },			/* LATIN SMALL LETTER LL */
    { 1008, "logicaland" },		/* LOGICAL AND */
    { 1009, "logicalnot" },		/* NOT SIGN */
    { 1010, "logicalor" },		/* LOGICAL OR */
    { 1011, "longs" },			/* LATIN SMALL LETTER LONG S */
    { 1012, "lozenge" },		/* LOZENGE */
    { 1013, "lslash" },			/* LATIN SMALL LETTER L WITH STROKE */
    { 1014, "lsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER L */
    { 1015, "ltshade" },		/* LIGHT SHADE */
    { 1016, "m" },			/* LATIN SMALL LETTER M */
    { 1017, "macron" },			/* MACRON */
					/* MODIFIER LETTER MACRON;Duplicate */
    { 1018, "male" },			/* MALE SIGN */
    { 1019, "merge" },			/* FONT FAMILY;Courier */
    { 1020, "minus" },			/* MINUS SIGN */
    { 1021, "minute" },			/* PRIME */
    { 1022, "msuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER M */
    { 1023, "mu" },			/* MICRO SIGN */
					/* GREEK SMALL LETTER MU;Duplicate */
    { 1024, "multiply" },		/* MULTIPLICATION SIGN */
    { 1025, "musicalnote" },		/* EIGHTH NOTE */
    { 1026, "musicalnotedbl" },		/* BEAMED EIGHTH NOTES */
    { 1027, "n" },			/* LATIN SMALL LETTER N */
    { 1028, "nacute" },			/* LATIN SMALL LETTER N WITH ACUTE */
    { 1029, "napostrophe" },		/* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    { 1030, "ncaron" },			/* LATIN SMALL LETTER N WITH CARON */
    { 1031, "ncommaaccent" },		/* LATIN SMALL LETTER N WITH CEDILLA */
    { 1032, "nine" },			/* DIGIT NINE */
    { 1033, "nineinferior" },		/* SUBSCRIPT NINE */
    { 1034, "nineoldstyle" },		/* OLDSTYLE DIGIT NINE */
    { 1035, "ninesuperior" },		/* SUPERSCRIPT NINE */
    { 1036, "notegraphic" },		/* FONT FAMILY;Courier */
    { 1037, "notelement" },		/* NOT AN ELEMENT OF */
    { 1038, "notequal" },		/* NOT EQUAL TO */
    { 1039, "notsubset" },		/* NOT A SUBSET OF */
    { 1040, "nsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER N */
    { 1041, "ntilde" },			/* LATIN SMALL LETTER N WITH TILDE */
    { 1042, "nu" },			/* GREEK SMALL LETTER NU */
    { 1043, "numbersign" },		/* NUMBER SIGN */
    { 1044, "o" },			/* LATIN SMALL LETTER O */
    { 1045, "oacute" },			/* LATIN SMALL LETTER O WITH ACUTE */
    { 1046, "obreve" },			/* LATIN SMALL LETTER O WITH BREVE */
    { 1047, "ocircumflex" },		/* LATIN SMALL LETTER O WITH CIRCUMFLEX */
    { 1048, "odieresis" },		/* LATIN SMALL LETTER O WITH DIAERESIS */
    { 1049, "oe" },			/* LATIN SMALL LIGATURE OE */
    { 1050, "ogonek" },			/* OGONEK */
    { 1051, "ograve" },			/* LATIN SMALL LETTER O WITH GRAVE */
    { 1052, "ohorn" },			/* LATIN SMALL LETTER O WITH HORN */
    { 1053, "ohungarumlaut" },		/* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    { 1054, "omacron" },		/* LATIN SMALL LETTER O WITH MACRON */
    { 1055, "omega" },			/* GREEK SMALL LETTER OMEGA */
    { 1056, "omega1" },			/* GREEK PI SYMBOL */
    { 1057, "omegatonos" },		/* GREEK SMALL LETTER OMEGA WITH TONOS */
    { 1058, "omicron" },		/* GREEK SMALL LETTER OMICRON */
    { 1059, "omicrontonos" },		/* GREEK SMALL LETTER OMICRON WITH TONOS */
    { 1060, "one" },			/* DIGIT ONE */
    { 1061, "onedotenleader" },		/* ONE DOT LEADER */
    { 1062, "oneeighth" },		/* VULGAR FRACTION ONE EIGHTH */
    { 1063, "onefitted" },		/* PROPORTIONAL DIGIT ONE */
    { 1064, "onehalf" },		/* VULGAR FRACTION ONE HALF */
    { 1065, "oneinferior" },		/* SUBSCRIPT ONE */
    { 1066, "oneoldstyle" },		/* OLDSTYLE DIGIT ONE */
    { 1067, "onequarter" },		/* VULGAR FRACTION ONE QUARTER */
    { 1068, "onesuperior" },		/* SUPERSCRIPT ONE */
    { 1069, "onethird" },		/* VULGAR FRACTION ONE THIRD */
    { 1070, "openbullet" },		/* WHITE BULLET */
    { 1071, "ordfeminine" },		/* FEMININE ORDINAL INDICATOR */
    { 1072, "ordmasculine" },		/* MASCULINE ORDINAL INDICATOR */
    { 1073, "orthogonal" },		/* RIGHT ANGLE */
    { 1074, "oslash" },			/* LATIN SMALL LETTER O WITH STROKE */
    { 1075, "oslashacute" },		/* LATIN SMALL LETTER O WITH STROKE AND ACUTE */
    { 1076, "osuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER O */
    { 1077, "otilde" },			/* LATIN SMALL LETTER O WITH TILDE */
    { 1078, "overscore" },		/* FONT FAMILY;Courier */
    { 1079, "p" },			/* LATIN SMALL LETTER P */
    { 1080, "paragraph" },		/* PILCROW SIGN */
    { 1081, "parenleft" },		/* LEFT PARENTHESIS */
    { 1082, "parenleftbt" },		/* LEFT PAREN BOTTOM */
    { 1083, "parenleftex" },		/* LEFT PAREN EXTENDER */
    { 1084, "parenleftinferior" },	/* SUBSCRIPT LEFT PARENTHESIS */
    { 1085, "parenleftsuperior" },	/* SUPERSCRIPT LEFT PARENTHESIS */
    { 1086, "parenlefttp" },		/* LEFT PAREN TOP */
    { 1087, "parenright" },		/* RIGHT PARENTHESIS */
    { 1088, "parenrightbt" },		/* RIGHT PAREN BOTTOM */
    { 1089, "parenrightex" },		/* RIGHT PAREN EXTENDER */
    { 1090, "parenrightinferior" },	/* SUBSCRIPT RIGHT PARENTHESIS */
    { 1091, "parenrightsuperior" },	/* SUPERSCRIPT RIGHT PARENTHESIS */
    { 1092, "parenrighttp" },		/* RIGHT PAREN TOP */
    { 1093, "partialdiff" },		/* PARTIAL DIFFERENTIAL */
    { 1094, "percent" },		/* PERCENT SIGN */
    { 1095, "period" },			/* FULL STOP */
    { 1096, "periodcentered" },		/* MIDDLE DOT */
					/* BULLET OPERATOR;Duplicate */
    { 1097, "periodinferior" },		/* SUBSCRIPT FULL STOP */
    { 1098, "periodsuperior" },		/* SUPERSCRIPT FULL STOP */
    { 1099, "perpendicular" },		/* UP TACK */
    { 1100, "perthousand" },		/* PER MILLE SIGN */
    { 1101, "peseta" },			/* PESETA SIGN */
    { 1102, "phi" },			/* GREEK SMALL LETTER PHI */
    { 1103, "phi1" },			/* GREEK PHI SYMBOL */
    { 1104, "pi" },			/* GREEK SMALL LETTER PI */
    { 1105, "plus" },			/* PLUS SIGN */
    { 1106, "plusminus" },		/* PLUS-MINUS SIGN */
    { 1107, "prescription" },		/* PRESCRIPTION TAKE */
    { 1108, "product" },		/* N-ARY PRODUCT */
    { 1109, "propersubset" },		/* SUBSET OF */
    { 1110, "propersuperset" },		/* SUPERSET OF */
    { 1111, "proportional" },		/* PROPORTIONAL TO */
    { 1112, "psi" },			/* GREEK SMALL LETTER PSI */
    { 1113, "q" },			/* LATIN SMALL LETTER Q */
    { 1114, "question" },		/* QUESTION MARK */
    { 1115, "questiondown" },		/* INVERTED QUESTION MARK */
    { 1116, "questiondownsmall" },	/* SMALL CAPITAL INVERTED QUESTION MARK */
    { 1117, "questionsmall" },		/* SMALL CAPITAL QUESTION MARK */
    { 1118, "quotedbl" },		/* QUOTATION MARK */
    { 1119, "quotedblbase" },		/* DOUBLE LOW-9 QUOTATION MARK */
    { 1120, "quotedblleft" },		/* LEFT DOUBLE QUOTATION MARK */
    { 1121, "quotedblright" },		/* RIGHT DOUBLE QUOTATION MARK */
    { 1122, "quoteleft" },		/* LEFT SINGLE QUOTATION MARK */
    { 1123, "quotereversed" },		/* SINGLE HIGH-REVERSED-9 QUOTATION MARK */
    { 1124, "quoteright" },		/* RIGHT SINGLE QUOTATION MARK */
    { 1125, "quotesinglbase" },		/* SINGLE LOW-9 QUOTATION MARK */
    { 1126, "quotesingle" },		/* APOSTROPHE */
    { 1127, "r" },			/* LATIN SMALL LETTER R */
    { 1128, "racute" },			/* LATIN SMALL LETTER R WITH ACUTE */
    { 1129, "radical" },		/* SQUARE ROOT */
    { 1130, "radicalex" },		/* RADICAL EXTENDER */
    { 1131, "rcaron" },			/* LATIN SMALL LETTER R WITH CARON */
    { 1132, "rcommaaccent" },		/* LATIN SMALL LETTER R WITH CEDILLA */
    { 1133, "reflexsubset" },		/* SUBSET OF OR EQUAL TO */
    { 1134, "reflexsuperset" },		/* SUPERSET OF OR EQUAL TO */
    { 1135, "registered" },		/* REGISTERED SIGN */
    { 1136, "registersans" },		/* REGISTERED SIGN SANS SERIF */
    { 1137, "registerserif" },		/* REGISTERED SIGN SERIF */
    { 1138, "return" },			/* FONT FAMILY;Courier */
    { 1139, "revlogicalnot" },		/* REVERSED NOT SIGN */
    { 1140, "rho" },			/* GREEK SMALL LETTER RHO */
    { 1141, "ring" },			/* RING ABOVE */
    { 1142, "rsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER R */
    { 1143, "rtblock" },		/* RIGHT HALF BLOCK */
    { 1144, "rupiah" },			/* RUPIAH SIGN */
    { 1145, "s" },			/* LATIN SMALL LETTER S */
    { 1146, "sacute" },			/* LATIN SMALL LETTER S WITH ACUTE */
    { 1147, "scaron" },			/* LATIN SMALL LETTER S WITH CARON */
    { 1148, "scedilla" },		/* LATIN SMALL LETTER S WITH CEDILLA */
					/* LATIN SMALL LETTER S WITH CEDILLA;Duplicate */
    { 1149, "scircumflex" },		/* LATIN SMALL LETTER S WITH CIRCUMFLEX */
    { 1150, "scommaaccent" },		/* LATIN SMALL LETTER S WITH COMMA BELOW */
    { 1151, "second" },			/* DOUBLE PRIME */
    { 1152, "section" },		/* SECTION SIGN */
    { 1153, "semicolon" },		/* SEMICOLON */
    { 1154, "seven" },			/* DIGIT SEVEN */
    { 1155, "seveneighths" },		/* VULGAR FRACTION SEVEN EIGHTHS */
    { 1156, "seveninferior" },		/* SUBSCRIPT SEVEN */
    { 1157, "sevenoldstyle" },		/* OLDSTYLE DIGIT SEVEN */
    { 1158, "sevensuperior" },		/* SUPERSCRIPT SEVEN */
    { 1159, "shade" },			/* MEDIUM SHADE */
    { 1160, "sigma" },			/* GREEK SMALL LETTER SIGMA */
    { 1161, "sigma1" },			/* GREEK SMALL LETTER FINAL SIGMA */
    { 1162, "similar" },		/* TILDE OPERATOR */
    { 1163, "six" },			/* DIGIT SIX */
    { 1164, "sixinferior" },		/* SUBSCRIPT SIX */
    { 1165, "sixoldstyle" },		/* OLDSTYLE DIGIT SIX */
    { 1166, "sixsuperior" },		/* SUPERSCRIPT SIX */
    { 1167, "slash" },			/* SOLIDUS */
    { 1168, "smileface" },		/* WHITE SMILING FACE */
    { 1169, "space" },			/* SPACE */
					/* NO-BREAK SPACE;Duplicate */
    { 1170, "spade" },			/* BLACK SPADE SUIT */
    { 1171, "square" },			/* FONT FAMILY;Courier */
    { 1172, "ssuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER S */
    { 1173, "sterling" },		/* POUND SIGN */
    { 1174, "stop" },			/* FONT FAMILY;Courier */
    { 1175, "suchthat" },		/* CONTAINS AS MEMBER */
    { 1176, "summation" },		/* N-ARY SUMMATION */
    { 1177, "sun" },			/* WHITE SUN WITH RAYS */
    { 1178, "t" },			/* LATIN SMALL LETTER T */
    { 1179, "tab" },			/* FONT FAMILY;Courier */
    { 1180, "tau" },			/* GREEK SMALL LETTER TAU */
    { 1181, "tbar" },			/* LATIN SMALL LETTER T WITH STROKE */
    { 1182, "tcaron" },			/* LATIN SMALL LETTER T WITH CARON */
    { 1183, "tcommaaccent" },		/* LATIN SMALL LETTER T WITH CEDILLA */
					/* LATIN SMALL LETTER T WITH COMMA BELOW;Duplicate */
    { 1184, "therefore" },		/* THEREFORE */
    { 1185, "theta" },			/* GREEK SMALL LETTER THETA */
    { 1186, "theta1" },			/* GREEK THETA SYMBOL */
    { 1187, "thorn" },			/* LATIN SMALL LETTER THORN */
    { 1188, "three" },			/* DIGIT THREE */
    { 1189, "threeeighths" },		/* VULGAR FRACTION THREE EIGHTHS */
    { 1190, "threeinferior" },		/* SUBSCRIPT THREE */
    { 1191, "threeoldstyle" },		/* OLDSTYLE DIGIT THREE */
    { 1192, "threequarters" },		/* VULGAR FRACTION THREE QUARTERS */
    { 1193, "threequartersemdash" },	/* THREE QUARTERS EM DASH */
    { 1194, "threesuperior" },		/* SUPERSCRIPT THREE */
    { 1195, "tilde" },			/* SMALL TILDE */
    { 1196, "tildecomb" },		/* COMBINING TILDE */
    { 1197, "tonos" },			/* GREEK TONOS */
    { 1198, "trademark" },		/* TRADE MARK SIGN */
    { 1199, "trademarksans" },		/* TRADE MARK SIGN SANS SERIF */
    { 1200, "trademarkserif" },		/* TRADE MARK SIGN SERIF */
    { 1201, "triagdn" },		/* BLACK DOWN-POINTING TRIANGLE */
    { 1202, "triaglf" },		/* BLACK LEFT-POINTING POINTER */
    { 1203, "triagrt" },		/* BLACK RIGHT-POINTING POINTER */
    { 1204, "triagup" },		/* BLACK UP-POINTING TRIANGLE */
    { 1205, "tsuperior" },		/* SUPERSCRIPT LATIN SMALL LETTER T */
    { 1206, "two" },			/* DIGIT TWO */
    { 1207, "twodotenleader" },		/* TWO DOT LEADER */
    { 1208, "twoinferior" },		/* SUBSCRIPT TWO */
    { 1209, "twooldstyle" },		/* OLDSTYLE DIGIT TWO */
    { 1210, "twosuperior" },		/* SUPERSCRIPT TWO */
    { 1211, "twothirds" },		/* VULGAR FRACTION TWO THIRDS */
    { 1212, "u" },			/* LATIN SMALL LETTER U */
    { 1213, "uacute" },			/* LATIN SMALL LETTER U WITH ACUTE */
    { 1214, "ubreve" },			/* LATIN SMALL LETTER U WITH BREVE */
    { 1215, "ucircumflex" },		/* LATIN SMALL LETTER U WITH CIRCUMFLEX */
    { 1216, "udieresis" },		/* LATIN SMALL LETTER U WITH DIAERESIS */
    { 1217, "ugrave" },			/* LATIN SMALL LETTER U WITH GRAVE */
    { 1218, "uhorn" },			/* LATIN SMALL LETTER U WITH HORN */
    { 1219, "uhungarumlaut" },		/* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    { 1220, "umacron" },		/* LATIN SMALL LETTER U WITH MACRON */
    { 1221, "underscore" },		/* LOW LINE */
    { 1222, "underscoredbl" },		/* DOUBLE LOW LINE */
    { 1223, "union" },			/* UNION */
    { 1224, "universal" },		/* FOR ALL */
    { 1225, "uogonek" },		/* LATIN SMALL LETTER U WITH OGONEK */
    { 1226, "up" },			/* FONT FAMILY;Courier */
    { 1227, "upblock" },		/* UPPER HALF BLOCK */
    { 1228, "upsilon" },		/* GREEK SMALL LETTER UPSILON */
    { 1229, "upsilondieresis" },	/* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    { 1230, "upsilondieresistonos" },	/* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    { 1231, "upsilontonos" },		/* GREEK SMALL LETTER UPSILON WITH TONOS */
    { 1232, "uring" },			/* LATIN SMALL LETTER U WITH RING ABOVE */
    { 1233, "utilde" },			/* LATIN SMALL LETTER U WITH TILDE */
    { 1234, "v" },			/* LATIN SMALL LETTER V */
    { 1235, "w" },			/* LATIN SMALL LETTER W */
    { 1236, "wacute" },			/* LATIN SMALL LETTER W WITH ACUTE */
    { 1237, "wcircumflex" },		/* LATIN SMALL LETTER W WITH CIRCUMFLEX */
    { 1238, "wdieresis" },		/* LATIN SMALL LETTER W WITH DIAERESIS */
    { 1239, "weierstrass" },		/* SCRIPT CAPITAL P */
    { 1240, "wgrave" },			/* LATIN SMALL LETTER W WITH GRAVE */
    { 1241, "x" },			/* LATIN SMALL LETTER X */
    { 1242, "xi" },			/* GREEK SMALL LETTER XI */
    { 1243, "y" },			/* LATIN SMALL LETTER Y */
    { 1244, "yacute" },			/* LATIN SMALL LETTER Y WITH ACUTE */
    { 1245, "ycircumflex" },		/* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
    { 1246, "ydieresis" },		/* LATIN SMALL LETTER Y WITH DIAERESIS */
    { 1247, "yen" },			/* YEN SIGN */
    { 1248, "ygrave" },			/* LATIN SMALL LETTER Y WITH GRAVE */
    { 1249, "z" },			/* LATIN SMALL LETTER Z */
    { 1250, "zacute" },			/* LATIN SMALL LETTER Z WITH ACUTE */
    { 1251, "zcaron" },			/* LATIN SMALL LETTER Z WITH CARON */
    { 1252, "zdotaccent" },		/* LATIN SMALL LETTER Z WITH DOT ABOVE */
    { 1253, "zero" },			/* DIGIT ZERO */
    { 1254, "zeroinferior" },		/* SUBSCRIPT ZERO */
    { 1255, "zerooldstyle" },		/* OLDSTYLE DIGIT ZERO */
    { 1256, "zerosuperior" },		/* SUPERSCRIPT ZERO */
    { 1257, "zeta" }			/* GREEK SMALL LETTER ZETA */
};


/*
 *  The AGL encoding vector, sorted by glyph name - duplicates omitted
 */

const INT PSDRV_AGLbyNameSize = 1039;

const UNICODEGLYPH PSDRV_AGLbyName[1039] =
{
    { 0x0041, GN_A },			    { 0x00c6, GN_AE },
    { 0x01fc, GN_AEacute },		    { 0xf7e6, GN_AEsmall },
    { 0x00c1, GN_Aacute },		    { 0xf7e1, GN_Aacutesmall },
    { 0x0102, GN_Abreve },		    { 0x00c2, GN_Acircumflex },
    { 0xf7e2, GN_Acircumflexsmall },	    { 0xf6c9, GN_Acute },
    { 0xf7b4, GN_Acutesmall },		    { 0x00c4, GN_Adieresis },
    { 0xf7e4, GN_Adieresissmall },	    { 0x00c0, GN_Agrave },
    { 0xf7e0, GN_Agravesmall },		    { 0x0391, GN_Alpha },
    { 0x0386, GN_Alphatonos },		    { 0x0100, GN_Amacron },
    { 0x0104, GN_Aogonek },		    { 0x00c5, GN_Aring },
    { 0x01fa, GN_Aringacute },		    { 0xf7e5, GN_Aringsmall },
    { 0xf761, GN_Asmall },		    { 0x00c3, GN_Atilde },
    { 0xf7e3, GN_Atildesmall },		    { 0x0042, GN_B },
    { 0x0392, GN_Beta },		    { 0xf6f4, GN_Brevesmall },
    { 0xf762, GN_Bsmall },		    { 0x0043, GN_C },
    { 0x0106, GN_Cacute },		    { 0xf6ca, GN_Caron },
    { 0xf6f5, GN_Caronsmall },		    { 0x010c, GN_Ccaron },
    { 0x00c7, GN_Ccedilla },		    { 0xf7e7, GN_Ccedillasmall },
    { 0x0108, GN_Ccircumflex },		    { 0x010a, GN_Cdotaccent },
    { 0xf7b8, GN_Cedillasmall },	    { 0x03a7, GN_Chi },
    { 0xf6f6, GN_Circumflexsmall },	    { 0xf763, GN_Csmall },
    { 0x0044, GN_D },			    { 0x010e, GN_Dcaron },
    { 0x0110, GN_Dcroat },		    { 0x2206, GN_Delta },
    { 0xf6cb, GN_Dieresis },		    { 0xf6cc, GN_DieresisAcute },
    { 0xf6cd, GN_DieresisGrave },	    { 0xf7a8, GN_Dieresissmall },
    { 0xf6f7, GN_Dotaccentsmall },	    { 0xf764, GN_Dsmall },
    { 0x0045, GN_E },			    { 0x00c9, GN_Eacute },
    { 0xf7e9, GN_Eacutesmall },		    { 0x0114, GN_Ebreve },
    { 0x011a, GN_Ecaron },		    { 0x00ca, GN_Ecircumflex },
    { 0xf7ea, GN_Ecircumflexsmall },	    { 0x00cb, GN_Edieresis },
    { 0xf7eb, GN_Edieresissmall },	    { 0x0116, GN_Edotaccent },
    { 0x00c8, GN_Egrave },		    { 0xf7e8, GN_Egravesmall },
    { 0x0112, GN_Emacron },		    { 0x014a, GN_Eng },
    { 0x0118, GN_Eogonek },		    { 0x0395, GN_Epsilon },
    { 0x0388, GN_Epsilontonos },	    { 0xf765, GN_Esmall },
    { 0x0397, GN_Eta },			    { 0x0389, GN_Etatonos },
    { 0x00d0, GN_Eth },			    { 0xf7f0, GN_Ethsmall },
    { 0x20ac, GN_Euro },		    { 0x0046, GN_F },
    { 0xf766, GN_Fsmall },		    { 0x0047, GN_G },
    { 0x0393, GN_Gamma },		    { 0x011e, GN_Gbreve },
    { 0x01e6, GN_Gcaron },		    { 0x011c, GN_Gcircumflex },
    { 0x0122, GN_Gcommaaccent },	    { 0x0120, GN_Gdotaccent },
    { 0xf6ce, GN_Grave },		    { 0xf760, GN_Gravesmall },
    { 0xf767, GN_Gsmall },		    { 0x0048, GN_H },
    { 0x25cf, GN_H18533 },		    { 0x25aa, GN_H18543 },
    { 0x25ab, GN_H18551 },		    { 0x25a1, GN_H22073 },
    { 0x0126, GN_Hbar },		    { 0x0124, GN_Hcircumflex },
    { 0xf768, GN_Hsmall },		    { 0xf6cf, GN_Hungarumlaut },
    { 0xf6f8, GN_Hungarumlautsmall },	    { 0x0049, GN_I },
    { 0x0132, GN_IJ },			    { 0x00cd, GN_Iacute },
    { 0xf7ed, GN_Iacutesmall },		    { 0x012c, GN_Ibreve },
    { 0x00ce, GN_Icircumflex },		    { 0xf7ee, GN_Icircumflexsmall },
    { 0x00cf, GN_Idieresis },		    { 0xf7ef, GN_Idieresissmall },
    { 0x0130, GN_Idotaccent },		    { 0x2111, GN_Ifraktur },
    { 0x00cc, GN_Igrave },		    { 0xf7ec, GN_Igravesmall },
    { 0x012a, GN_Imacron },		    { 0x012e, GN_Iogonek },
    { 0x0399, GN_Iota },		    { 0x03aa, GN_Iotadieresis },
    { 0x038a, GN_Iotatonos },		    { 0xf769, GN_Ismall },
    { 0x0128, GN_Itilde },		    { 0x004a, GN_J },
    { 0x0134, GN_Jcircumflex },		    { 0xf76a, GN_Jsmall },
    { 0x004b, GN_K },			    { 0x039a, GN_Kappa },
    { 0x0136, GN_Kcommaaccent },	    { 0xf76b, GN_Ksmall },
    { 0x004c, GN_L },			    { 0xf6bf, GN_LL },
    { 0x0139, GN_Lacute },		    { 0x039b, GN_Lambda },
    { 0x013d, GN_Lcaron },		    { 0x013b, GN_Lcommaaccent },
    { 0x013f, GN_Ldot },		    { 0x0141, GN_Lslash },
    { 0xf6f9, GN_Lslashsmall },		    { 0xf76c, GN_Lsmall },
    { 0x004d, GN_M },			    { 0xf6d0, GN_Macron },
    { 0xf7af, GN_Macronsmall },		    { 0xf76d, GN_Msmall },
    { 0x039c, GN_Mu },			    { 0x004e, GN_N },
    { 0x0143, GN_Nacute },		    { 0x0147, GN_Ncaron },
    { 0x0145, GN_Ncommaaccent },	    { 0xf76e, GN_Nsmall },
    { 0x00d1, GN_Ntilde },		    { 0xf7f1, GN_Ntildesmall },
    { 0x039d, GN_Nu },			    { 0x004f, GN_O },
    { 0x0152, GN_OE },			    { 0xf6fa, GN_OEsmall },
    { 0x00d3, GN_Oacute },		    { 0xf7f3, GN_Oacutesmall },
    { 0x014e, GN_Obreve },		    { 0x00d4, GN_Ocircumflex },
    { 0xf7f4, GN_Ocircumflexsmall },	    { 0x00d6, GN_Odieresis },
    { 0xf7f6, GN_Odieresissmall },	    { 0xf6fb, GN_Ogoneksmall },
    { 0x00d2, GN_Ograve },		    { 0xf7f2, GN_Ogravesmall },
    { 0x01a0, GN_Ohorn },		    { 0x0150, GN_Ohungarumlaut },
    { 0x014c, GN_Omacron },		    { 0x2126, GN_Omega },
    { 0x038f, GN_Omegatonos },		    { 0x039f, GN_Omicron },
    { 0x038c, GN_Omicrontonos },	    { 0x00d8, GN_Oslash },
    { 0x01fe, GN_Oslashacute },		    { 0xf7f8, GN_Oslashsmall },
    { 0xf76f, GN_Osmall },		    { 0x00d5, GN_Otilde },
    { 0xf7f5, GN_Otildesmall },		    { 0x0050, GN_P },
    { 0x03a6, GN_Phi },			    { 0x03a0, GN_Pi },
    { 0x03a8, GN_Psi },			    { 0xf770, GN_Psmall },
    { 0x0051, GN_Q },			    { 0xf771, GN_Qsmall },
    { 0x0052, GN_R },			    { 0x0154, GN_Racute },
    { 0x0158, GN_Rcaron },		    { 0x0156, GN_Rcommaaccent },
    { 0x211c, GN_Rfraktur },		    { 0x03a1, GN_Rho },
    { 0xf6fc, GN_Ringsmall },		    { 0xf772, GN_Rsmall },
    { 0x0053, GN_S },			    { 0x250c, GN_SF010000 },
    { 0x2514, GN_SF020000 },		    { 0x2510, GN_SF030000 },
    { 0x2518, GN_SF040000 },		    { 0x253c, GN_SF050000 },
    { 0x252c, GN_SF060000 },		    { 0x2534, GN_SF070000 },
    { 0x251c, GN_SF080000 },		    { 0x2524, GN_SF090000 },
    { 0x2500, GN_SF100000 },		    { 0x2502, GN_SF110000 },
    { 0x2561, GN_SF190000 },		    { 0x2562, GN_SF200000 },
    { 0x2556, GN_SF210000 },		    { 0x2555, GN_SF220000 },
    { 0x2563, GN_SF230000 },		    { 0x2551, GN_SF240000 },
    { 0x2557, GN_SF250000 },		    { 0x255d, GN_SF260000 },
    { 0x255c, GN_SF270000 },		    { 0x255b, GN_SF280000 },
    { 0x255e, GN_SF360000 },		    { 0x255f, GN_SF370000 },
    { 0x255a, GN_SF380000 },		    { 0x2554, GN_SF390000 },
    { 0x2569, GN_SF400000 },		    { 0x2566, GN_SF410000 },
    { 0x2560, GN_SF420000 },		    { 0x2550, GN_SF430000 },
    { 0x256c, GN_SF440000 },		    { 0x2567, GN_SF450000 },
    { 0x2568, GN_SF460000 },		    { 0x2564, GN_SF470000 },
    { 0x2565, GN_SF480000 },		    { 0x2559, GN_SF490000 },
    { 0x2558, GN_SF500000 },		    { 0x2552, GN_SF510000 },
    { 0x2553, GN_SF520000 },		    { 0x256b, GN_SF530000 },
    { 0x256a, GN_SF540000 },		    { 0x015a, GN_Sacute },
    { 0x0160, GN_Scaron },		    { 0xf6fd, GN_Scaronsmall },
    { 0x015e, GN_Scedilla },		    { 0x015c, GN_Scircumflex },
    { 0x0218, GN_Scommaaccent },	    { 0x03a3, GN_Sigma },
    { 0xf773, GN_Ssmall },		    { 0x0054, GN_T },
    { 0x03a4, GN_Tau },			    { 0x0166, GN_Tbar },
    { 0x0164, GN_Tcaron },		    { 0x0162, GN_Tcommaaccent },
    { 0x0398, GN_Theta },		    { 0x00de, GN_Thorn },
    { 0xf7fe, GN_Thornsmall },		    { 0xf6fe, GN_Tildesmall },
    { 0xf774, GN_Tsmall },		    { 0x0055, GN_U },
    { 0x00da, GN_Uacute },		    { 0xf7fa, GN_Uacutesmall },
    { 0x016c, GN_Ubreve },		    { 0x00db, GN_Ucircumflex },
    { 0xf7fb, GN_Ucircumflexsmall },	    { 0x00dc, GN_Udieresis },
    { 0xf7fc, GN_Udieresissmall },	    { 0x00d9, GN_Ugrave },
    { 0xf7f9, GN_Ugravesmall },		    { 0x01af, GN_Uhorn },
    { 0x0170, GN_Uhungarumlaut },	    { 0x016a, GN_Umacron },
    { 0x0172, GN_Uogonek },		    { 0x03a5, GN_Upsilon },
    { 0x03d2, GN_Upsilon1 },		    { 0x03ab, GN_Upsilondieresis },
    { 0x038e, GN_Upsilontonos },	    { 0x016e, GN_Uring },
    { 0xf775, GN_Usmall },		    { 0x0168, GN_Utilde },
    { 0x0056, GN_V },			    { 0xf776, GN_Vsmall },
    { 0x0057, GN_W },			    { 0x1e82, GN_Wacute },
    { 0x0174, GN_Wcircumflex },		    { 0x1e84, GN_Wdieresis },
    { 0x1e80, GN_Wgrave },		    { 0xf777, GN_Wsmall },
    { 0x0058, GN_X },			    { 0x039e, GN_Xi },
    { 0xf778, GN_Xsmall },		    { 0x0059, GN_Y },
    { 0x00dd, GN_Yacute },		    { 0xf7fd, GN_Yacutesmall },
    { 0x0176, GN_Ycircumflex },		    { 0x0178, GN_Ydieresis },
    { 0xf7ff, GN_Ydieresissmall },	    { 0x1ef2, GN_Ygrave },
    { 0xf779, GN_Ysmall },		    { 0x005a, GN_Z },
    { 0x0179, GN_Zacute },		    { 0x017d, GN_Zcaron },
    { 0xf6ff, GN_Zcaronsmall },		    { 0x017b, GN_Zdotaccent },
    { 0x0396, GN_Zeta },		    { 0xf77a, GN_Zsmall },
    { 0x0061, GN_a },			    { 0x00e1, GN_aacute },
    { 0x0103, GN_abreve },		    { 0x00e2, GN_acircumflex },
    { 0x00b4, GN_acute },		    { 0x0301, GN_acutecomb },
    { 0x00e4, GN_adieresis },		    { 0x00e6, GN_ae },
    { 0x01fd, GN_aeacute },		    { 0x2015, GN_afii00208 },
    { 0x0410, GN_afii10017 },		    { 0x0411, GN_afii10018 },
    { 0x0412, GN_afii10019 },		    { 0x0413, GN_afii10020 },
    { 0x0414, GN_afii10021 },		    { 0x0415, GN_afii10022 },
    { 0x0401, GN_afii10023 },		    { 0x0416, GN_afii10024 },
    { 0x0417, GN_afii10025 },		    { 0x0418, GN_afii10026 },
    { 0x0419, GN_afii10027 },		    { 0x041a, GN_afii10028 },
    { 0x041b, GN_afii10029 },		    { 0x041c, GN_afii10030 },
    { 0x041d, GN_afii10031 },		    { 0x041e, GN_afii10032 },
    { 0x041f, GN_afii10033 },		    { 0x0420, GN_afii10034 },
    { 0x0421, GN_afii10035 },		    { 0x0422, GN_afii10036 },
    { 0x0423, GN_afii10037 },		    { 0x0424, GN_afii10038 },
    { 0x0425, GN_afii10039 },		    { 0x0426, GN_afii10040 },
    { 0x0427, GN_afii10041 },		    { 0x0428, GN_afii10042 },
    { 0x0429, GN_afii10043 },		    { 0x042a, GN_afii10044 },
    { 0x042b, GN_afii10045 },		    { 0x042c, GN_afii10046 },
    { 0x042d, GN_afii10047 },		    { 0x042e, GN_afii10048 },
    { 0x042f, GN_afii10049 },		    { 0x0490, GN_afii10050 },
    { 0x0402, GN_afii10051 },		    { 0x0403, GN_afii10052 },
    { 0x0404, GN_afii10053 },		    { 0x0405, GN_afii10054 },
    { 0x0406, GN_afii10055 },		    { 0x0407, GN_afii10056 },
    { 0x0408, GN_afii10057 },		    { 0x0409, GN_afii10058 },
    { 0x040a, GN_afii10059 },		    { 0x040b, GN_afii10060 },
    { 0x040c, GN_afii10061 },		    { 0x040e, GN_afii10062 },
    { 0xf6c4, GN_afii10063 },		    { 0xf6c5, GN_afii10064 },
    { 0x0430, GN_afii10065 },		    { 0x0431, GN_afii10066 },
    { 0x0432, GN_afii10067 },		    { 0x0433, GN_afii10068 },
    { 0x0434, GN_afii10069 },		    { 0x0435, GN_afii10070 },
    { 0x0451, GN_afii10071 },		    { 0x0436, GN_afii10072 },
    { 0x0437, GN_afii10073 },		    { 0x0438, GN_afii10074 },
    { 0x0439, GN_afii10075 },		    { 0x043a, GN_afii10076 },
    { 0x043b, GN_afii10077 },		    { 0x043c, GN_afii10078 },
    { 0x043d, GN_afii10079 },		    { 0x043e, GN_afii10080 },
    { 0x043f, GN_afii10081 },		    { 0x0440, GN_afii10082 },
    { 0x0441, GN_afii10083 },		    { 0x0442, GN_afii10084 },
    { 0x0443, GN_afii10085 },		    { 0x0444, GN_afii10086 },
    { 0x0445, GN_afii10087 },		    { 0x0446, GN_afii10088 },
    { 0x0447, GN_afii10089 },		    { 0x0448, GN_afii10090 },
    { 0x0449, GN_afii10091 },		    { 0x044a, GN_afii10092 },
    { 0x044b, GN_afii10093 },		    { 0x044c, GN_afii10094 },
    { 0x044d, GN_afii10095 },		    { 0x044e, GN_afii10096 },
    { 0x044f, GN_afii10097 },		    { 0x0491, GN_afii10098 },
    { 0x0452, GN_afii10099 },		    { 0x0453, GN_afii10100 },
    { 0x0454, GN_afii10101 },		    { 0x0455, GN_afii10102 },
    { 0x0456, GN_afii10103 },		    { 0x0457, GN_afii10104 },
    { 0x0458, GN_afii10105 },		    { 0x0459, GN_afii10106 },
    { 0x045a, GN_afii10107 },		    { 0x045b, GN_afii10108 },
    { 0x045c, GN_afii10109 },		    { 0x045e, GN_afii10110 },
    { 0x040f, GN_afii10145 },		    { 0x0462, GN_afii10146 },
    { 0x0472, GN_afii10147 },		    { 0x0474, GN_afii10148 },
    { 0xf6c6, GN_afii10192 },		    { 0x045f, GN_afii10193 },
    { 0x0463, GN_afii10194 },		    { 0x0473, GN_afii10195 },
    { 0x0475, GN_afii10196 },		    { 0xf6c7, GN_afii10831 },
    { 0xf6c8, GN_afii10832 },		    { 0x04d9, GN_afii10846 },
    { 0x200e, GN_afii299 },		    { 0x200f, GN_afii300 },
    { 0x200d, GN_afii301 },		    { 0x066a, GN_afii57381 },
    { 0x060c, GN_afii57388 },		    { 0x0660, GN_afii57392 },
    { 0x0661, GN_afii57393 },		    { 0x0662, GN_afii57394 },
    { 0x0663, GN_afii57395 },		    { 0x0664, GN_afii57396 },
    { 0x0665, GN_afii57397 },		    { 0x0666, GN_afii57398 },
    { 0x0667, GN_afii57399 },		    { 0x0668, GN_afii57400 },
    { 0x0669, GN_afii57401 },		    { 0x061b, GN_afii57403 },
    { 0x061f, GN_afii57407 },		    { 0x0621, GN_afii57409 },
    { 0x0622, GN_afii57410 },		    { 0x0623, GN_afii57411 },
    { 0x0624, GN_afii57412 },		    { 0x0625, GN_afii57413 },
    { 0x0626, GN_afii57414 },		    { 0x0627, GN_afii57415 },
    { 0x0628, GN_afii57416 },		    { 0x0629, GN_afii57417 },
    { 0x062a, GN_afii57418 },		    { 0x062b, GN_afii57419 },
    { 0x062c, GN_afii57420 },		    { 0x062d, GN_afii57421 },
    { 0x062e, GN_afii57422 },		    { 0x062f, GN_afii57423 },
    { 0x0630, GN_afii57424 },		    { 0x0631, GN_afii57425 },
    { 0x0632, GN_afii57426 },		    { 0x0633, GN_afii57427 },
    { 0x0634, GN_afii57428 },		    { 0x0635, GN_afii57429 },
    { 0x0636, GN_afii57430 },		    { 0x0637, GN_afii57431 },
    { 0x0638, GN_afii57432 },		    { 0x0639, GN_afii57433 },
    { 0x063a, GN_afii57434 },		    { 0x0640, GN_afii57440 },
    { 0x0641, GN_afii57441 },		    { 0x0642, GN_afii57442 },
    { 0x0643, GN_afii57443 },		    { 0x0644, GN_afii57444 },
    { 0x0645, GN_afii57445 },		    { 0x0646, GN_afii57446 },
    { 0x0648, GN_afii57448 },		    { 0x0649, GN_afii57449 },
    { 0x064a, GN_afii57450 },		    { 0x064b, GN_afii57451 },
    { 0x064c, GN_afii57452 },		    { 0x064d, GN_afii57453 },
    { 0x064e, GN_afii57454 },		    { 0x064f, GN_afii57455 },
    { 0x0650, GN_afii57456 },		    { 0x0651, GN_afii57457 },
    { 0x0652, GN_afii57458 },		    { 0x0647, GN_afii57470 },
    { 0x06a4, GN_afii57505 },		    { 0x067e, GN_afii57506 },
    { 0x0686, GN_afii57507 },		    { 0x0698, GN_afii57508 },
    { 0x06af, GN_afii57509 },		    { 0x0679, GN_afii57511 },
    { 0x0688, GN_afii57512 },		    { 0x0691, GN_afii57513 },
    { 0x06ba, GN_afii57514 },		    { 0x06d2, GN_afii57519 },
    { 0x06d5, GN_afii57534 },		    { 0x20aa, GN_afii57636 },
    { 0x05be, GN_afii57645 },		    { 0x05c3, GN_afii57658 },
    { 0x05d0, GN_afii57664 },		    { 0x05d1, GN_afii57665 },
    { 0x05d2, GN_afii57666 },		    { 0x05d3, GN_afii57667 },
    { 0x05d4, GN_afii57668 },		    { 0x05d5, GN_afii57669 },
    { 0x05d6, GN_afii57670 },		    { 0x05d7, GN_afii57671 },
    { 0x05d8, GN_afii57672 },		    { 0x05d9, GN_afii57673 },
    { 0x05da, GN_afii57674 },		    { 0x05db, GN_afii57675 },
    { 0x05dc, GN_afii57676 },		    { 0x05dd, GN_afii57677 },
    { 0x05de, GN_afii57678 },		    { 0x05df, GN_afii57679 },
    { 0x05e0, GN_afii57680 },		    { 0x05e1, GN_afii57681 },
    { 0x05e2, GN_afii57682 },		    { 0x05e3, GN_afii57683 },
    { 0x05e4, GN_afii57684 },		    { 0x05e5, GN_afii57685 },
    { 0x05e6, GN_afii57686 },		    { 0x05e7, GN_afii57687 },
    { 0x05e8, GN_afii57688 },		    { 0x05e9, GN_afii57689 },
    { 0x05ea, GN_afii57690 },		    { 0xfb2a, GN_afii57694 },
    { 0xfb2b, GN_afii57695 },		    { 0xfb4b, GN_afii57700 },
    { 0xfb1f, GN_afii57705 },		    { 0x05f0, GN_afii57716 },
    { 0x05f1, GN_afii57717 },		    { 0x05f2, GN_afii57718 },
    { 0xfb35, GN_afii57723 },		    { 0x05b4, GN_afii57793 },
    { 0x05b5, GN_afii57794 },		    { 0x05b6, GN_afii57795 },
    { 0x05bb, GN_afii57796 },		    { 0x05b8, GN_afii57797 },
    { 0x05b7, GN_afii57798 },		    { 0x05b0, GN_afii57799 },
    { 0x05b2, GN_afii57800 },		    { 0x05b1, GN_afii57801 },
    { 0x05b3, GN_afii57802 },		    { 0x05c2, GN_afii57803 },
    { 0x05c1, GN_afii57804 },		    { 0x05b9, GN_afii57806 },
    { 0x05bc, GN_afii57807 },		    { 0x05bd, GN_afii57839 },
    { 0x05bf, GN_afii57841 },		    { 0x05c0, GN_afii57842 },
    { 0x02bc, GN_afii57929 },		    { 0x2105, GN_afii61248 },
    { 0x2113, GN_afii61289 },		    { 0x2116, GN_afii61352 },
    { 0x202c, GN_afii61573 },		    { 0x202d, GN_afii61574 },
    { 0x202e, GN_afii61575 },		    { 0x200c, GN_afii61664 },
    { 0x066d, GN_afii63167 },		    { 0x02bd, GN_afii64937 },
    { 0x00e0, GN_agrave },		    { 0x2135, GN_aleph },
    { 0x03b1, GN_alpha },		    { 0x03ac, GN_alphatonos },
    { 0x0101, GN_amacron },		    { 0x0026, GN_ampersand },
    { 0xf726, GN_ampersandsmall },	    { 0x2220, GN_angle },
    { 0x2329, GN_angleleft },		    { 0x232a, GN_angleright },
    { 0x0387, GN_anoteleia },		    { 0x0105, GN_aogonek },
    { 0x2248, GN_approxequal },		    { 0x00e5, GN_aring },
    { 0x01fb, GN_aringacute },		    { 0x2194, GN_arrowboth },
    { 0x21d4, GN_arrowdblboth },	    { 0x21d3, GN_arrowdbldown },
    { 0x21d0, GN_arrowdblleft },	    { 0x21d2, GN_arrowdblright },
    { 0x21d1, GN_arrowdblup },		    { 0x2193, GN_arrowdown },
    { 0xf8e7, GN_arrowhorizex },	    { 0x2190, GN_arrowleft },
    { 0x2192, GN_arrowright },		    { 0x2191, GN_arrowup },
    { 0x2195, GN_arrowupdn },		    { 0x21a8, GN_arrowupdnbse },
    { 0xf8e6, GN_arrowvertex },		    { 0x005e, GN_asciicircum },
    { 0x007e, GN_asciitilde },		    { 0x002a, GN_asterisk },
    { 0x2217, GN_asteriskmath },	    { 0xf6e9, GN_asuperior },
    { 0x0040, GN_at },			    { 0x00e3, GN_atilde },
    { 0x0062, GN_b },			    { 0x005c, GN_backslash },
    { 0x007c, GN_bar },			    { 0x03b2, GN_beta },
    { 0x2588, GN_block },		    { 0xf8f4, GN_braceex },
    { 0x007b, GN_braceleft },		    { 0xf8f3, GN_braceleftbt },
    { 0xf8f2, GN_braceleftmid },	    { 0xf8f1, GN_bracelefttp },
    { 0x007d, GN_braceright },		    { 0xf8fe, GN_bracerightbt },
    { 0xf8fd, GN_bracerightmid },	    { 0xf8fc, GN_bracerighttp },
    { 0x005b, GN_bracketleft },		    { 0xf8f0, GN_bracketleftbt },
    { 0xf8ef, GN_bracketleftex },	    { 0xf8ee, GN_bracketlefttp },
    { 0x005d, GN_bracketright },	    { 0xf8fb, GN_bracketrightbt },
    { 0xf8fa, GN_bracketrightex },	    { 0xf8f9, GN_bracketrighttp },
    { 0x02d8, GN_breve },		    { 0x00a6, GN_brokenbar },
    { 0xf6ea, GN_bsuperior },		    { 0x2022, GN_bullet },
    { 0x0063, GN_c },			    { 0x0107, GN_cacute },
    { 0x02c7, GN_caron },		    { 0x21b5, GN_carriagereturn },
    { 0x010d, GN_ccaron },		    { 0x00e7, GN_ccedilla },
    { 0x0109, GN_ccircumflex },		    { 0x010b, GN_cdotaccent },
    { 0x00b8, GN_cedilla },		    { 0x00a2, GN_cent },
    { 0xf6df, GN_centinferior },	    { 0xf7a2, GN_centoldstyle },
    { 0xf6e0, GN_centsuperior },	    { 0x03c7, GN_chi },
    { 0x25cb, GN_circle },		    { 0x2297, GN_circlemultiply },
    { 0x2295, GN_circleplus },		    { 0x02c6, GN_circumflex },
    { 0x2663, GN_club },		    { 0x003a, GN_colon },
    { 0x20a1, GN_colonmonetary },	    { 0x002c, GN_comma },
    { 0xf6c3, GN_commaaccent },		    { 0xf6e1, GN_commainferior },
    { 0xf6e2, GN_commasuperior },	    { 0x2245, GN_congruent },
    { 0x00a9, GN_copyright },		    { 0xf8e9, GN_copyrightsans },
    { 0xf6d9, GN_copyrightserif },	    { 0x00a4, GN_currency },
    { 0xf6d1, GN_cyrBreve },		    { 0xf6d2, GN_cyrFlex },
    { 0xf6d4, GN_cyrbreve },		    { 0xf6d5, GN_cyrflex },
    { 0x0064, GN_d },			    { 0x2020, GN_dagger },
    { 0x2021, GN_daggerdbl },		    { 0xf6d3, GN_dblGrave },
    { 0xf6d6, GN_dblgrave },		    { 0x010f, GN_dcaron },
    { 0x0111, GN_dcroat },		    { 0x00b0, GN_degree },
    { 0x03b4, GN_delta },		    { 0x2666, GN_diamond },
    { 0x00a8, GN_dieresis },		    { 0xf6d7, GN_dieresisacute },
    { 0xf6d8, GN_dieresisgrave },	    { 0x0385, GN_dieresistonos },
    { 0x00f7, GN_divide },		    { 0x2593, GN_dkshade },
    { 0x2584, GN_dnblock },		    { 0x0024, GN_dollar },
    { 0xf6e3, GN_dollarinferior },	    { 0xf724, GN_dollaroldstyle },
    { 0xf6e4, GN_dollarsuperior },	    { 0x20ab, GN_dong },
    { 0x02d9, GN_dotaccent },		    { 0x0323, GN_dotbelowcomb },
    { 0x0131, GN_dotlessi },		    { 0xf6be, GN_dotlessj },
    { 0x22c5, GN_dotmath },		    { 0xf6eb, GN_dsuperior },
    { 0x0065, GN_e },			    { 0x00e9, GN_eacute },
    { 0x0115, GN_ebreve },		    { 0x011b, GN_ecaron },
    { 0x00ea, GN_ecircumflex },		    { 0x00eb, GN_edieresis },
    { 0x0117, GN_edotaccent },		    { 0x00e8, GN_egrave },
    { 0x0038, GN_eight },		    { 0x2088, GN_eightinferior },
    { 0xf738, GN_eightoldstyle },	    { 0x2078, GN_eightsuperior },
    { 0x2208, GN_element },		    { 0x2026, GN_ellipsis },
    { 0x0113, GN_emacron },		    { 0x2014, GN_emdash },
    { 0x2205, GN_emptyset },		    { 0x2013, GN_endash },
    { 0x014b, GN_eng },			    { 0x0119, GN_eogonek },
    { 0x03b5, GN_epsilon },		    { 0x03ad, GN_epsilontonos },
    { 0x003d, GN_equal },		    { 0x2261, GN_equivalence },
    { 0x212e, GN_estimated },		    { 0xf6ec, GN_esuperior },
    { 0x03b7, GN_eta },			    { 0x03ae, GN_etatonos },
    { 0x00f0, GN_eth },			    { 0x0021, GN_exclam },
    { 0x203c, GN_exclamdbl },		    { 0x00a1, GN_exclamdown },
    { 0xf7a1, GN_exclamdownsmall },	    { 0xf721, GN_exclamsmall },
    { 0x2203, GN_existential },		    { 0x0066, GN_f },
    { 0x2640, GN_female },		    { 0xfb00, GN_ff },
    { 0xfb03, GN_ffi },			    { 0xfb04, GN_ffl },
    { 0xfb01, GN_fi },			    { 0x2012, GN_figuredash },
    { 0x25a0, GN_filledbox },		    { 0x25ac, GN_filledrect },
    { 0x0035, GN_five },		    { 0x215d, GN_fiveeighths },
    { 0x2085, GN_fiveinferior },	    { 0xf735, GN_fiveoldstyle },
    { 0x2075, GN_fivesuperior },	    { 0xfb02, GN_fl },
    { 0x0192, GN_florin },		    { 0x0034, GN_four },
    { 0x2084, GN_fourinferior },	    { 0xf734, GN_fouroldstyle },
    { 0x2074, GN_foursuperior },	    { 0x2044, GN_fraction },
    { 0x20a3, GN_franc },		    { 0x0067, GN_g },
    { 0x03b3, GN_gamma },		    { 0x011f, GN_gbreve },
    { 0x01e7, GN_gcaron },		    { 0x011d, GN_gcircumflex },
    { 0x0123, GN_gcommaaccent },	    { 0x0121, GN_gdotaccent },
    { 0x00df, GN_germandbls },		    { 0x2207, GN_gradient },
    { 0x0060, GN_grave },		    { 0x0300, GN_gravecomb },
    { 0x003e, GN_greater },		    { 0x2265, GN_greaterequal },
    { 0x00ab, GN_guillemotleft },	    { 0x00bb, GN_guillemotright },
    { 0x2039, GN_guilsinglleft },	    { 0x203a, GN_guilsinglright },
    { 0x0068, GN_h },			    { 0x0127, GN_hbar },
    { 0x0125, GN_hcircumflex },		    { 0x2665, GN_heart },
    { 0x0309, GN_hookabovecomb },	    { 0x2302, GN_house },
    { 0x02dd, GN_hungarumlaut },	    { 0x002d, GN_hyphen },
    { 0xf6e5, GN_hypheninferior },	    { 0xf6e6, GN_hyphensuperior },
    { 0x0069, GN_i },			    { 0x00ed, GN_iacute },
    { 0x012d, GN_ibreve },		    { 0x00ee, GN_icircumflex },
    { 0x00ef, GN_idieresis },		    { 0x00ec, GN_igrave },
    { 0x0133, GN_ij },			    { 0x012b, GN_imacron },
    { 0x221e, GN_infinity },		    { 0x222b, GN_integral },
    { 0x2321, GN_integralbt },		    { 0xf8f5, GN_integralex },
    { 0x2320, GN_integraltp },		    { 0x2229, GN_intersection },
    { 0x25d8, GN_invbullet },		    { 0x25d9, GN_invcircle },
    { 0x263b, GN_invsmileface },	    { 0x012f, GN_iogonek },
    { 0x03b9, GN_iota },		    { 0x03ca, GN_iotadieresis },
    { 0x0390, GN_iotadieresistonos },	    { 0x03af, GN_iotatonos },
    { 0xf6ed, GN_isuperior },		    { 0x0129, GN_itilde },
    { 0x006a, GN_j },			    { 0x0135, GN_jcircumflex },
    { 0x006b, GN_k },			    { 0x03ba, GN_kappa },
    { 0x0137, GN_kcommaaccent },	    { 0x0138, GN_kgreenlandic },
    { 0x006c, GN_l },			    { 0x013a, GN_lacute },
    { 0x03bb, GN_lambda },		    { 0x013e, GN_lcaron },
    { 0x013c, GN_lcommaaccent },	    { 0x0140, GN_ldot },
    { 0x003c, GN_less },		    { 0x2264, GN_lessequal },
    { 0x258c, GN_lfblock },		    { 0x20a4, GN_lira },
    { 0xf6c0, GN_ll },			    { 0x2227, GN_logicaland },
    { 0x00ac, GN_logicalnot },		    { 0x2228, GN_logicalor },
    { 0x017f, GN_longs },		    { 0x25ca, GN_lozenge },
    { 0x0142, GN_lslash },		    { 0xf6ee, GN_lsuperior },
    { 0x2591, GN_ltshade },		    { 0x006d, GN_m },
    { 0x00af, GN_macron },		    { 0x2642, GN_male },
    { 0x2212, GN_minus },		    { 0x2032, GN_minute },
    { 0xf6ef, GN_msuperior },		    { 0x00b5, GN_mu },
    { 0x00d7, GN_multiply },		    { 0x266a, GN_musicalnote },
    { 0x266b, GN_musicalnotedbl },	    { 0x006e, GN_n },
    { 0x0144, GN_nacute },		    { 0x0149, GN_napostrophe },
    { 0x0148, GN_ncaron },		    { 0x0146, GN_ncommaaccent },
    { 0x0039, GN_nine },		    { 0x2089, GN_nineinferior },
    { 0xf739, GN_nineoldstyle },	    { 0x2079, GN_ninesuperior },
    { 0x2209, GN_notelement },		    { 0x2260, GN_notequal },
    { 0x2284, GN_notsubset },		    { 0x207f, GN_nsuperior },
    { 0x00f1, GN_ntilde },		    { 0x03bd, GN_nu },
    { 0x0023, GN_numbersign },		    { 0x006f, GN_o },
    { 0x00f3, GN_oacute },		    { 0x014f, GN_obreve },
    { 0x00f4, GN_ocircumflex },		    { 0x00f6, GN_odieresis },
    { 0x0153, GN_oe },			    { 0x02db, GN_ogonek },
    { 0x00f2, GN_ograve },		    { 0x01a1, GN_ohorn },
    { 0x0151, GN_ohungarumlaut },	    { 0x014d, GN_omacron },
    { 0x03c9, GN_omega },		    { 0x03d6, GN_omega1 },
    { 0x03ce, GN_omegatonos },		    { 0x03bf, GN_omicron },
    { 0x03cc, GN_omicrontonos },	    { 0x0031, GN_one },
    { 0x2024, GN_onedotenleader },	    { 0x215b, GN_oneeighth },
    { 0xf6dc, GN_onefitted },		    { 0x00bd, GN_onehalf },
    { 0x2081, GN_oneinferior },		    { 0xf731, GN_oneoldstyle },
    { 0x00bc, GN_onequarter },		    { 0x00b9, GN_onesuperior },
    { 0x2153, GN_onethird },		    { 0x25e6, GN_openbullet },
    { 0x00aa, GN_ordfeminine },		    { 0x00ba, GN_ordmasculine },
    { 0x221f, GN_orthogonal },		    { 0x00f8, GN_oslash },
    { 0x01ff, GN_oslashacute },		    { 0xf6f0, GN_osuperior },
    { 0x00f5, GN_otilde },		    { 0x0070, GN_p },
    { 0x00b6, GN_paragraph },		    { 0x0028, GN_parenleft },
    { 0xf8ed, GN_parenleftbt },		    { 0xf8ec, GN_parenleftex },
    { 0x208d, GN_parenleftinferior },	    { 0x207d, GN_parenleftsuperior },
    { 0xf8eb, GN_parenlefttp },		    { 0x0029, GN_parenright },
    { 0xf8f8, GN_parenrightbt },	    { 0xf8f7, GN_parenrightex },
    { 0x208e, GN_parenrightinferior },	    { 0x207e, GN_parenrightsuperior },
    { 0xf8f6, GN_parenrighttp },	    { 0x2202, GN_partialdiff },
    { 0x0025, GN_percent },		    { 0x002e, GN_period },
    { 0x00b7, GN_periodcentered },	    { 0xf6e7, GN_periodinferior },
    { 0xf6e8, GN_periodsuperior },	    { 0x22a5, GN_perpendicular },
    { 0x2030, GN_perthousand },		    { 0x20a7, GN_peseta },
    { 0x03c6, GN_phi },			    { 0x03d5, GN_phi1 },
    { 0x03c0, GN_pi },			    { 0x002b, GN_plus },
    { 0x00b1, GN_plusminus },		    { 0x211e, GN_prescription },
    { 0x220f, GN_product },		    { 0x2282, GN_propersubset },
    { 0x2283, GN_propersuperset },	    { 0x221d, GN_proportional },
    { 0x03c8, GN_psi },			    { 0x0071, GN_q },
    { 0x003f, GN_question },		    { 0x00bf, GN_questiondown },
    { 0xf7bf, GN_questiondownsmall },	    { 0xf73f, GN_questionsmall },
    { 0x0022, GN_quotedbl },		    { 0x201e, GN_quotedblbase },
    { 0x201c, GN_quotedblleft },	    { 0x201d, GN_quotedblright },
    { 0x2018, GN_quoteleft },		    { 0x201b, GN_quotereversed },
    { 0x2019, GN_quoteright },		    { 0x201a, GN_quotesinglbase },
    { 0x0027, GN_quotesingle },		    { 0x0072, GN_r },
    { 0x0155, GN_racute },		    { 0x221a, GN_radical },
    { 0xf8e5, GN_radicalex },		    { 0x0159, GN_rcaron },
    { 0x0157, GN_rcommaaccent },	    { 0x2286, GN_reflexsubset },
    { 0x2287, GN_reflexsuperset },	    { 0x00ae, GN_registered },
    { 0xf8e8, GN_registersans },	    { 0xf6da, GN_registerserif },
    { 0x2310, GN_revlogicalnot },	    { 0x03c1, GN_rho },
    { 0x02da, GN_ring },		    { 0xf6f1, GN_rsuperior },
    { 0x2590, GN_rtblock },		    { 0xf6dd, GN_rupiah },
    { 0x0073, GN_s },			    { 0x015b, GN_sacute },
    { 0x0161, GN_scaron },		    { 0x015f, GN_scedilla },
    { 0x015d, GN_scircumflex },		    { 0x0219, GN_scommaaccent },
    { 0x2033, GN_second },		    { 0x00a7, GN_section },
    { 0x003b, GN_semicolon },		    { 0x0037, GN_seven },
    { 0x215e, GN_seveneighths },	    { 0x2087, GN_seveninferior },
    { 0xf737, GN_sevenoldstyle },	    { 0x2077, GN_sevensuperior },
    { 0x2592, GN_shade },		    { 0x03c3, GN_sigma },
    { 0x03c2, GN_sigma1 },		    { 0x223c, GN_similar },
    { 0x0036, GN_six },			    { 0x2086, GN_sixinferior },
    { 0xf736, GN_sixoldstyle },		    { 0x2076, GN_sixsuperior },
    { 0x002f, GN_slash },		    { 0x263a, GN_smileface },
    { 0x0020, GN_space },		    { 0x2660, GN_spade },
    { 0xf6f2, GN_ssuperior },		    { 0x00a3, GN_sterling },
    { 0x220b, GN_suchthat },		    { 0x2211, GN_summation },
    { 0x263c, GN_sun },			    { 0x0074, GN_t },
    { 0x03c4, GN_tau },			    { 0x0167, GN_tbar },
    { 0x0165, GN_tcaron },		    { 0x0163, GN_tcommaaccent },
    { 0x2234, GN_therefore },		    { 0x03b8, GN_theta },
    { 0x03d1, GN_theta1 },		    { 0x00fe, GN_thorn },
    { 0x0033, GN_three },		    { 0x215c, GN_threeeighths },
    { 0x2083, GN_threeinferior },	    { 0xf733, GN_threeoldstyle },
    { 0x00be, GN_threequarters },	    { 0xf6de, GN_threequartersemdash },
    { 0x00b3, GN_threesuperior },	    { 0x02dc, GN_tilde },
    { 0x0303, GN_tildecomb },		    { 0x0384, GN_tonos },
    { 0x2122, GN_trademark },		    { 0xf8ea, GN_trademarksans },
    { 0xf6db, GN_trademarkserif },	    { 0x25bc, GN_triagdn },
    { 0x25c4, GN_triaglf },		    { 0x25ba, GN_triagrt },
    { 0x25b2, GN_triagup },		    { 0xf6f3, GN_tsuperior },
    { 0x0032, GN_two },			    { 0x2025, GN_twodotenleader },
    { 0x2082, GN_twoinferior },		    { 0xf732, GN_twooldstyle },
    { 0x00b2, GN_twosuperior },		    { 0x2154, GN_twothirds },
    { 0x0075, GN_u },			    { 0x00fa, GN_uacute },
    { 0x016d, GN_ubreve },		    { 0x00fb, GN_ucircumflex },
    { 0x00fc, GN_udieresis },		    { 0x00f9, GN_ugrave },
    { 0x01b0, GN_uhorn },		    { 0x0171, GN_uhungarumlaut },
    { 0x016b, GN_umacron },		    { 0x005f, GN_underscore },
    { 0x2017, GN_underscoredbl },	    { 0x222a, GN_union },
    { 0x2200, GN_universal },		    { 0x0173, GN_uogonek },
    { 0x2580, GN_upblock },		    { 0x03c5, GN_upsilon },
    { 0x03cb, GN_upsilondieresis },	    { 0x03b0, GN_upsilondieresistonos },
    { 0x03cd, GN_upsilontonos },	    { 0x016f, GN_uring },
    { 0x0169, GN_utilde },		    { 0x0076, GN_v },
    { 0x0077, GN_w },			    { 0x1e83, GN_wacute },
    { 0x0175, GN_wcircumflex },		    { 0x1e85, GN_wdieresis },
    { 0x2118, GN_weierstrass },		    { 0x1e81, GN_wgrave },
    { 0x0078, GN_x },			    { 0x03be, GN_xi },
    { 0x0079, GN_y },			    { 0x00fd, GN_yacute },
    { 0x0177, GN_ycircumflex },		    { 0x00ff, GN_ydieresis },
    { 0x00a5, GN_yen },			    { 0x1ef3, GN_ygrave },
    { 0x007a, GN_z },			    { 0x017a, GN_zacute },
    { 0x017e, GN_zcaron },		    { 0x017c, GN_zdotaccent },
    { 0x0030, GN_zero },		    { 0x2080, GN_zeroinferior },
    { 0xf730, GN_zerooldstyle },	    { 0x2070, GN_zerosuperior },
    { 0x03b6, GN_zeta }
};


/*
 *  The AGL encoding vector, sorted by Unicode value - duplicates included
 */

const INT PSDRV_AGLbyUVSize = 1051;

const UNICODEGLYPH PSDRV_AGLbyUV[1051] =
{
    { 0x0020, GN_space },		    { 0x0021, GN_exclam },
    { 0x0022, GN_quotedbl },		    { 0x0023, GN_numbersign },
    { 0x0024, GN_dollar },		    { 0x0025, GN_percent },
    { 0x0026, GN_ampersand },		    { 0x0027, GN_quotesingle },
    { 0x0028, GN_parenleft },		    { 0x0029, GN_parenright },
    { 0x002a, GN_asterisk },		    { 0x002b, GN_plus },
    { 0x002c, GN_comma },		    { 0x002d, GN_hyphen },
    { 0x002e, GN_period },		    { 0x002f, GN_slash },
    { 0x0030, GN_zero },		    { 0x0031, GN_one },
    { 0x0032, GN_two },			    { 0x0033, GN_three },
    { 0x0034, GN_four },		    { 0x0035, GN_five },
    { 0x0036, GN_six },			    { 0x0037, GN_seven },
    { 0x0038, GN_eight },		    { 0x0039, GN_nine },
    { 0x003a, GN_colon },		    { 0x003b, GN_semicolon },
    { 0x003c, GN_less },		    { 0x003d, GN_equal },
    { 0x003e, GN_greater },		    { 0x003f, GN_question },
    { 0x0040, GN_at },			    { 0x0041, GN_A },
    { 0x0042, GN_B },			    { 0x0043, GN_C },
    { 0x0044, GN_D },			    { 0x0045, GN_E },
    { 0x0046, GN_F },			    { 0x0047, GN_G },
    { 0x0048, GN_H },			    { 0x0049, GN_I },
    { 0x004a, GN_J },			    { 0x004b, GN_K },
    { 0x004c, GN_L },			    { 0x004d, GN_M },
    { 0x004e, GN_N },			    { 0x004f, GN_O },
    { 0x0050, GN_P },			    { 0x0051, GN_Q },
    { 0x0052, GN_R },			    { 0x0053, GN_S },
    { 0x0054, GN_T },			    { 0x0055, GN_U },
    { 0x0056, GN_V },			    { 0x0057, GN_W },
    { 0x0058, GN_X },			    { 0x0059, GN_Y },
    { 0x005a, GN_Z },			    { 0x005b, GN_bracketleft },
    { 0x005c, GN_backslash },		    { 0x005d, GN_bracketright },
    { 0x005e, GN_asciicircum },		    { 0x005f, GN_underscore },
    { 0x0060, GN_grave },		    { 0x0061, GN_a },
    { 0x0062, GN_b },			    { 0x0063, GN_c },
    { 0x0064, GN_d },			    { 0x0065, GN_e },
    { 0x0066, GN_f },			    { 0x0067, GN_g },
    { 0x0068, GN_h },			    { 0x0069, GN_i },
    { 0x006a, GN_j },			    { 0x006b, GN_k },
    { 0x006c, GN_l },			    { 0x006d, GN_m },
    { 0x006e, GN_n },			    { 0x006f, GN_o },
    { 0x0070, GN_p },			    { 0x0071, GN_q },
    { 0x0072, GN_r },			    { 0x0073, GN_s },
    { 0x0074, GN_t },			    { 0x0075, GN_u },
    { 0x0076, GN_v },			    { 0x0077, GN_w },
    { 0x0078, GN_x },			    { 0x0079, GN_y },
    { 0x007a, GN_z },			    { 0x007b, GN_braceleft },
    { 0x007c, GN_bar },			    { 0x007d, GN_braceright },
    { 0x007e, GN_asciitilde },		    { 0x00a0, GN_space },
    { 0x00a1, GN_exclamdown },		    { 0x00a2, GN_cent },
    { 0x00a3, GN_sterling },		    { 0x00a4, GN_currency },
    { 0x00a5, GN_yen },			    { 0x00a6, GN_brokenbar },
    { 0x00a7, GN_section },		    { 0x00a8, GN_dieresis },
    { 0x00a9, GN_copyright },		    { 0x00aa, GN_ordfeminine },
    { 0x00ab, GN_guillemotleft },	    { 0x00ac, GN_logicalnot },
    { 0x00ad, GN_hyphen },		    { 0x00ae, GN_registered },
    { 0x00af, GN_macron },		    { 0x00b0, GN_degree },
    { 0x00b1, GN_plusminus },		    { 0x00b2, GN_twosuperior },
    { 0x00b3, GN_threesuperior },	    { 0x00b4, GN_acute },
    { 0x00b5, GN_mu },			    { 0x00b6, GN_paragraph },
    { 0x00b7, GN_periodcentered },	    { 0x00b8, GN_cedilla },
    { 0x00b9, GN_onesuperior },		    { 0x00ba, GN_ordmasculine },
    { 0x00bb, GN_guillemotright },	    { 0x00bc, GN_onequarter },
    { 0x00bd, GN_onehalf },		    { 0x00be, GN_threequarters },
    { 0x00bf, GN_questiondown },	    { 0x00c0, GN_Agrave },
    { 0x00c1, GN_Aacute },		    { 0x00c2, GN_Acircumflex },
    { 0x00c3, GN_Atilde },		    { 0x00c4, GN_Adieresis },
    { 0x00c5, GN_Aring },		    { 0x00c6, GN_AE },
    { 0x00c7, GN_Ccedilla },		    { 0x00c8, GN_Egrave },
    { 0x00c9, GN_Eacute },		    { 0x00ca, GN_Ecircumflex },
    { 0x00cb, GN_Edieresis },		    { 0x00cc, GN_Igrave },
    { 0x00cd, GN_Iacute },		    { 0x00ce, GN_Icircumflex },
    { 0x00cf, GN_Idieresis },		    { 0x00d0, GN_Eth },
    { 0x00d1, GN_Ntilde },		    { 0x00d2, GN_Ograve },
    { 0x00d3, GN_Oacute },		    { 0x00d4, GN_Ocircumflex },
    { 0x00d5, GN_Otilde },		    { 0x00d6, GN_Odieresis },
    { 0x00d7, GN_multiply },		    { 0x00d8, GN_Oslash },
    { 0x00d9, GN_Ugrave },		    { 0x00da, GN_Uacute },
    { 0x00db, GN_Ucircumflex },		    { 0x00dc, GN_Udieresis },
    { 0x00dd, GN_Yacute },		    { 0x00de, GN_Thorn },
    { 0x00df, GN_germandbls },		    { 0x00e0, GN_agrave },
    { 0x00e1, GN_aacute },		    { 0x00e2, GN_acircumflex },
    { 0x00e3, GN_atilde },		    { 0x00e4, GN_adieresis },
    { 0x00e5, GN_aring },		    { 0x00e6, GN_ae },
    { 0x00e7, GN_ccedilla },		    { 0x00e8, GN_egrave },
    { 0x00e9, GN_eacute },		    { 0x00ea, GN_ecircumflex },
    { 0x00eb, GN_edieresis },		    { 0x00ec, GN_igrave },
    { 0x00ed, GN_iacute },		    { 0x00ee, GN_icircumflex },
    { 0x00ef, GN_idieresis },		    { 0x00f0, GN_eth },
    { 0x00f1, GN_ntilde },		    { 0x00f2, GN_ograve },
    { 0x00f3, GN_oacute },		    { 0x00f4, GN_ocircumflex },
    { 0x00f5, GN_otilde },		    { 0x00f6, GN_odieresis },
    { 0x00f7, GN_divide },		    { 0x00f8, GN_oslash },
    { 0x00f9, GN_ugrave },		    { 0x00fa, GN_uacute },
    { 0x00fb, GN_ucircumflex },		    { 0x00fc, GN_udieresis },
    { 0x00fd, GN_yacute },		    { 0x00fe, GN_thorn },
    { 0x00ff, GN_ydieresis },		    { 0x0100, GN_Amacron },
    { 0x0101, GN_amacron },		    { 0x0102, GN_Abreve },
    { 0x0103, GN_abreve },		    { 0x0104, GN_Aogonek },
    { 0x0105, GN_aogonek },		    { 0x0106, GN_Cacute },
    { 0x0107, GN_cacute },		    { 0x0108, GN_Ccircumflex },
    { 0x0109, GN_ccircumflex },		    { 0x010a, GN_Cdotaccent },
    { 0x010b, GN_cdotaccent },		    { 0x010c, GN_Ccaron },
    { 0x010d, GN_ccaron },		    { 0x010e, GN_Dcaron },
    { 0x010f, GN_dcaron },		    { 0x0110, GN_Dcroat },
    { 0x0111, GN_dcroat },		    { 0x0112, GN_Emacron },
    { 0x0113, GN_emacron },		    { 0x0114, GN_Ebreve },
    { 0x0115, GN_ebreve },		    { 0x0116, GN_Edotaccent },
    { 0x0117, GN_edotaccent },		    { 0x0118, GN_Eogonek },
    { 0x0119, GN_eogonek },		    { 0x011a, GN_Ecaron },
    { 0x011b, GN_ecaron },		    { 0x011c, GN_Gcircumflex },
    { 0x011d, GN_gcircumflex },		    { 0x011e, GN_Gbreve },
    { 0x011f, GN_gbreve },		    { 0x0120, GN_Gdotaccent },
    { 0x0121, GN_gdotaccent },		    { 0x0122, GN_Gcommaaccent },
    { 0x0123, GN_gcommaaccent },	    { 0x0124, GN_Hcircumflex },
    { 0x0125, GN_hcircumflex },		    { 0x0126, GN_Hbar },
    { 0x0127, GN_hbar },		    { 0x0128, GN_Itilde },
    { 0x0129, GN_itilde },		    { 0x012a, GN_Imacron },
    { 0x012b, GN_imacron },		    { 0x012c, GN_Ibreve },
    { 0x012d, GN_ibreve },		    { 0x012e, GN_Iogonek },
    { 0x012f, GN_iogonek },		    { 0x0130, GN_Idotaccent },
    { 0x0131, GN_dotlessi },		    { 0x0132, GN_IJ },
    { 0x0133, GN_ij },			    { 0x0134, GN_Jcircumflex },
    { 0x0135, GN_jcircumflex },		    { 0x0136, GN_Kcommaaccent },
    { 0x0137, GN_kcommaaccent },	    { 0x0138, GN_kgreenlandic },
    { 0x0139, GN_Lacute },		    { 0x013a, GN_lacute },
    { 0x013b, GN_Lcommaaccent },	    { 0x013c, GN_lcommaaccent },
    { 0x013d, GN_Lcaron },		    { 0x013e, GN_lcaron },
    { 0x013f, GN_Ldot },		    { 0x0140, GN_ldot },
    { 0x0141, GN_Lslash },		    { 0x0142, GN_lslash },
    { 0x0143, GN_Nacute },		    { 0x0144, GN_nacute },
    { 0x0145, GN_Ncommaaccent },	    { 0x0146, GN_ncommaaccent },
    { 0x0147, GN_Ncaron },		    { 0x0148, GN_ncaron },
    { 0x0149, GN_napostrophe },		    { 0x014a, GN_Eng },
    { 0x014b, GN_eng },			    { 0x014c, GN_Omacron },
    { 0x014d, GN_omacron },		    { 0x014e, GN_Obreve },
    { 0x014f, GN_obreve },		    { 0x0150, GN_Ohungarumlaut },
    { 0x0151, GN_ohungarumlaut },	    { 0x0152, GN_OE },
    { 0x0153, GN_oe },			    { 0x0154, GN_Racute },
    { 0x0155, GN_racute },		    { 0x0156, GN_Rcommaaccent },
    { 0x0157, GN_rcommaaccent },	    { 0x0158, GN_Rcaron },
    { 0x0159, GN_rcaron },		    { 0x015a, GN_Sacute },
    { 0x015b, GN_sacute },		    { 0x015c, GN_Scircumflex },
    { 0x015d, GN_scircumflex },		    { 0x015e, GN_Scedilla },
    { 0x015f, GN_scedilla },		    { 0x0160, GN_Scaron },
    { 0x0161, GN_scaron },		    { 0x0162, GN_Tcommaaccent },
    { 0x0163, GN_tcommaaccent },	    { 0x0164, GN_Tcaron },
    { 0x0165, GN_tcaron },		    { 0x0166, GN_Tbar },
    { 0x0167, GN_tbar },		    { 0x0168, GN_Utilde },
    { 0x0169, GN_utilde },		    { 0x016a, GN_Umacron },
    { 0x016b, GN_umacron },		    { 0x016c, GN_Ubreve },
    { 0x016d, GN_ubreve },		    { 0x016e, GN_Uring },
    { 0x016f, GN_uring },		    { 0x0170, GN_Uhungarumlaut },
    { 0x0171, GN_uhungarumlaut },	    { 0x0172, GN_Uogonek },
    { 0x0173, GN_uogonek },		    { 0x0174, GN_Wcircumflex },
    { 0x0175, GN_wcircumflex },		    { 0x0176, GN_Ycircumflex },
    { 0x0177, GN_ycircumflex },		    { 0x0178, GN_Ydieresis },
    { 0x0179, GN_Zacute },		    { 0x017a, GN_zacute },
    { 0x017b, GN_Zdotaccent },		    { 0x017c, GN_zdotaccent },
    { 0x017d, GN_Zcaron },		    { 0x017e, GN_zcaron },
    { 0x017f, GN_longs },		    { 0x0192, GN_florin },
    { 0x01a0, GN_Ohorn },		    { 0x01a1, GN_ohorn },
    { 0x01af, GN_Uhorn },		    { 0x01b0, GN_uhorn },
    { 0x01e6, GN_Gcaron },		    { 0x01e7, GN_gcaron },
    { 0x01fa, GN_Aringacute },		    { 0x01fb, GN_aringacute },
    { 0x01fc, GN_AEacute },		    { 0x01fd, GN_aeacute },
    { 0x01fe, GN_Oslashacute },		    { 0x01ff, GN_oslashacute },
    { 0x0218, GN_Scommaaccent },	    { 0x0219, GN_scommaaccent },
    { 0x021a, GN_Tcommaaccent },	    { 0x021b, GN_tcommaaccent },
    { 0x02bc, GN_afii57929 },		    { 0x02bd, GN_afii64937 },
    { 0x02c6, GN_circumflex },		    { 0x02c7, GN_caron },
    { 0x02c9, GN_macron },		    { 0x02d8, GN_breve },
    { 0x02d9, GN_dotaccent },		    { 0x02da, GN_ring },
    { 0x02db, GN_ogonek },		    { 0x02dc, GN_tilde },
    { 0x02dd, GN_hungarumlaut },	    { 0x0300, GN_gravecomb },
    { 0x0301, GN_acutecomb },		    { 0x0303, GN_tildecomb },
    { 0x0309, GN_hookabovecomb },	    { 0x0323, GN_dotbelowcomb },
    { 0x0384, GN_tonos },		    { 0x0385, GN_dieresistonos },
    { 0x0386, GN_Alphatonos },		    { 0x0387, GN_anoteleia },
    { 0x0388, GN_Epsilontonos },	    { 0x0389, GN_Etatonos },
    { 0x038a, GN_Iotatonos },		    { 0x038c, GN_Omicrontonos },
    { 0x038e, GN_Upsilontonos },	    { 0x038f, GN_Omegatonos },
    { 0x0390, GN_iotadieresistonos },	    { 0x0391, GN_Alpha },
    { 0x0392, GN_Beta },		    { 0x0393, GN_Gamma },
    { 0x0394, GN_Delta },		    { 0x0395, GN_Epsilon },
    { 0x0396, GN_Zeta },		    { 0x0397, GN_Eta },
    { 0x0398, GN_Theta },		    { 0x0399, GN_Iota },
    { 0x039a, GN_Kappa },		    { 0x039b, GN_Lambda },
    { 0x039c, GN_Mu },			    { 0x039d, GN_Nu },
    { 0x039e, GN_Xi },			    { 0x039f, GN_Omicron },
    { 0x03a0, GN_Pi },			    { 0x03a1, GN_Rho },
    { 0x03a3, GN_Sigma },		    { 0x03a4, GN_Tau },
    { 0x03a5, GN_Upsilon },		    { 0x03a6, GN_Phi },
    { 0x03a7, GN_Chi },			    { 0x03a8, GN_Psi },
    { 0x03a9, GN_Omega },		    { 0x03aa, GN_Iotadieresis },
    { 0x03ab, GN_Upsilondieresis },	    { 0x03ac, GN_alphatonos },
    { 0x03ad, GN_epsilontonos },	    { 0x03ae, GN_etatonos },
    { 0x03af, GN_iotatonos },		    { 0x03b0, GN_upsilondieresistonos },
    { 0x03b1, GN_alpha },		    { 0x03b2, GN_beta },
    { 0x03b3, GN_gamma },		    { 0x03b4, GN_delta },
    { 0x03b5, GN_epsilon },		    { 0x03b6, GN_zeta },
    { 0x03b7, GN_eta },			    { 0x03b8, GN_theta },
    { 0x03b9, GN_iota },		    { 0x03ba, GN_kappa },
    { 0x03bb, GN_lambda },		    { 0x03bc, GN_mu },
    { 0x03bd, GN_nu },			    { 0x03be, GN_xi },
    { 0x03bf, GN_omicron },		    { 0x03c0, GN_pi },
    { 0x03c1, GN_rho },			    { 0x03c2, GN_sigma1 },
    { 0x03c3, GN_sigma },		    { 0x03c4, GN_tau },
    { 0x03c5, GN_upsilon },		    { 0x03c6, GN_phi },
    { 0x03c7, GN_chi },			    { 0x03c8, GN_psi },
    { 0x03c9, GN_omega },		    { 0x03ca, GN_iotadieresis },
    { 0x03cb, GN_upsilondieresis },	    { 0x03cc, GN_omicrontonos },
    { 0x03cd, GN_upsilontonos },	    { 0x03ce, GN_omegatonos },
    { 0x03d1, GN_theta1 },		    { 0x03d2, GN_Upsilon1 },
    { 0x03d5, GN_phi1 },		    { 0x03d6, GN_omega1 },
    { 0x0401, GN_afii10023 },		    { 0x0402, GN_afii10051 },
    { 0x0403, GN_afii10052 },		    { 0x0404, GN_afii10053 },
    { 0x0405, GN_afii10054 },		    { 0x0406, GN_afii10055 },
    { 0x0407, GN_afii10056 },		    { 0x0408, GN_afii10057 },
    { 0x0409, GN_afii10058 },		    { 0x040a, GN_afii10059 },
    { 0x040b, GN_afii10060 },		    { 0x040c, GN_afii10061 },
    { 0x040e, GN_afii10062 },		    { 0x040f, GN_afii10145 },
    { 0x0410, GN_afii10017 },		    { 0x0411, GN_afii10018 },
    { 0x0412, GN_afii10019 },		    { 0x0413, GN_afii10020 },
    { 0x0414, GN_afii10021 },		    { 0x0415, GN_afii10022 },
    { 0x0416, GN_afii10024 },		    { 0x0417, GN_afii10025 },
    { 0x0418, GN_afii10026 },		    { 0x0419, GN_afii10027 },
    { 0x041a, GN_afii10028 },		    { 0x041b, GN_afii10029 },
    { 0x041c, GN_afii10030 },		    { 0x041d, GN_afii10031 },
    { 0x041e, GN_afii10032 },		    { 0x041f, GN_afii10033 },
    { 0x0420, GN_afii10034 },		    { 0x0421, GN_afii10035 },
    { 0x0422, GN_afii10036 },		    { 0x0423, GN_afii10037 },
    { 0x0424, GN_afii10038 },		    { 0x0425, GN_afii10039 },
    { 0x0426, GN_afii10040 },		    { 0x0427, GN_afii10041 },
    { 0x0428, GN_afii10042 },		    { 0x0429, GN_afii10043 },
    { 0x042a, GN_afii10044 },		    { 0x042b, GN_afii10045 },
    { 0x042c, GN_afii10046 },		    { 0x042d, GN_afii10047 },
    { 0x042e, GN_afii10048 },		    { 0x042f, GN_afii10049 },
    { 0x0430, GN_afii10065 },		    { 0x0431, GN_afii10066 },
    { 0x0432, GN_afii10067 },		    { 0x0433, GN_afii10068 },
    { 0x0434, GN_afii10069 },		    { 0x0435, GN_afii10070 },
    { 0x0436, GN_afii10072 },		    { 0x0437, GN_afii10073 },
    { 0x0438, GN_afii10074 },		    { 0x0439, GN_afii10075 },
    { 0x043a, GN_afii10076 },		    { 0x043b, GN_afii10077 },
    { 0x043c, GN_afii10078 },		    { 0x043d, GN_afii10079 },
    { 0x043e, GN_afii10080 },		    { 0x043f, GN_afii10081 },
    { 0x0440, GN_afii10082 },		    { 0x0441, GN_afii10083 },
    { 0x0442, GN_afii10084 },		    { 0x0443, GN_afii10085 },
    { 0x0444, GN_afii10086 },		    { 0x0445, GN_afii10087 },
    { 0x0446, GN_afii10088 },		    { 0x0447, GN_afii10089 },
    { 0x0448, GN_afii10090 },		    { 0x0449, GN_afii10091 },
    { 0x044a, GN_afii10092 },		    { 0x044b, GN_afii10093 },
    { 0x044c, GN_afii10094 },		    { 0x044d, GN_afii10095 },
    { 0x044e, GN_afii10096 },		    { 0x044f, GN_afii10097 },
    { 0x0451, GN_afii10071 },		    { 0x0452, GN_afii10099 },
    { 0x0453, GN_afii10100 },		    { 0x0454, GN_afii10101 },
    { 0x0455, GN_afii10102 },		    { 0x0456, GN_afii10103 },
    { 0x0457, GN_afii10104 },		    { 0x0458, GN_afii10105 },
    { 0x0459, GN_afii10106 },		    { 0x045a, GN_afii10107 },
    { 0x045b, GN_afii10108 },		    { 0x045c, GN_afii10109 },
    { 0x045e, GN_afii10110 },		    { 0x045f, GN_afii10193 },
    { 0x0462, GN_afii10146 },		    { 0x0463, GN_afii10194 },
    { 0x0472, GN_afii10147 },		    { 0x0473, GN_afii10195 },
    { 0x0474, GN_afii10148 },		    { 0x0475, GN_afii10196 },
    { 0x0490, GN_afii10050 },		    { 0x0491, GN_afii10098 },
    { 0x04d9, GN_afii10846 },		    { 0x05b0, GN_afii57799 },
    { 0x05b1, GN_afii57801 },		    { 0x05b2, GN_afii57800 },
    { 0x05b3, GN_afii57802 },		    { 0x05b4, GN_afii57793 },
    { 0x05b5, GN_afii57794 },		    { 0x05b6, GN_afii57795 },
    { 0x05b7, GN_afii57798 },		    { 0x05b8, GN_afii57797 },
    { 0x05b9, GN_afii57806 },		    { 0x05bb, GN_afii57796 },
    { 0x05bc, GN_afii57807 },		    { 0x05bd, GN_afii57839 },
    { 0x05be, GN_afii57645 },		    { 0x05bf, GN_afii57841 },
    { 0x05c0, GN_afii57842 },		    { 0x05c1, GN_afii57804 },
    { 0x05c2, GN_afii57803 },		    { 0x05c3, GN_afii57658 },
    { 0x05d0, GN_afii57664 },		    { 0x05d1, GN_afii57665 },
    { 0x05d2, GN_afii57666 },		    { 0x05d3, GN_afii57667 },
    { 0x05d4, GN_afii57668 },		    { 0x05d5, GN_afii57669 },
    { 0x05d6, GN_afii57670 },		    { 0x05d7, GN_afii57671 },
    { 0x05d8, GN_afii57672 },		    { 0x05d9, GN_afii57673 },
    { 0x05da, GN_afii57674 },		    { 0x05db, GN_afii57675 },
    { 0x05dc, GN_afii57676 },		    { 0x05dd, GN_afii57677 },
    { 0x05de, GN_afii57678 },		    { 0x05df, GN_afii57679 },
    { 0x05e0, GN_afii57680 },		    { 0x05e1, GN_afii57681 },
    { 0x05e2, GN_afii57682 },		    { 0x05e3, GN_afii57683 },
    { 0x05e4, GN_afii57684 },		    { 0x05e5, GN_afii57685 },
    { 0x05e6, GN_afii57686 },		    { 0x05e7, GN_afii57687 },
    { 0x05e8, GN_afii57688 },		    { 0x05e9, GN_afii57689 },
    { 0x05ea, GN_afii57690 },		    { 0x05f0, GN_afii57716 },
    { 0x05f1, GN_afii57717 },		    { 0x05f2, GN_afii57718 },
    { 0x060c, GN_afii57388 },		    { 0x061b, GN_afii57403 },
    { 0x061f, GN_afii57407 },		    { 0x0621, GN_afii57409 },
    { 0x0622, GN_afii57410 },		    { 0x0623, GN_afii57411 },
    { 0x0624, GN_afii57412 },		    { 0x0625, GN_afii57413 },
    { 0x0626, GN_afii57414 },		    { 0x0627, GN_afii57415 },
    { 0x0628, GN_afii57416 },		    { 0x0629, GN_afii57417 },
    { 0x062a, GN_afii57418 },		    { 0x062b, GN_afii57419 },
    { 0x062c, GN_afii57420 },		    { 0x062d, GN_afii57421 },
    { 0x062e, GN_afii57422 },		    { 0x062f, GN_afii57423 },
    { 0x0630, GN_afii57424 },		    { 0x0631, GN_afii57425 },
    { 0x0632, GN_afii57426 },		    { 0x0633, GN_afii57427 },
    { 0x0634, GN_afii57428 },		    { 0x0635, GN_afii57429 },
    { 0x0636, GN_afii57430 },		    { 0x0637, GN_afii57431 },
    { 0x0638, GN_afii57432 },		    { 0x0639, GN_afii57433 },
    { 0x063a, GN_afii57434 },		    { 0x0640, GN_afii57440 },
    { 0x0641, GN_afii57441 },		    { 0x0642, GN_afii57442 },
    { 0x0643, GN_afii57443 },		    { 0x0644, GN_afii57444 },
    { 0x0645, GN_afii57445 },		    { 0x0646, GN_afii57446 },
    { 0x0647, GN_afii57470 },		    { 0x0648, GN_afii57448 },
    { 0x0649, GN_afii57449 },		    { 0x064a, GN_afii57450 },
    { 0x064b, GN_afii57451 },		    { 0x064c, GN_afii57452 },
    { 0x064d, GN_afii57453 },		    { 0x064e, GN_afii57454 },
    { 0x064f, GN_afii57455 },		    { 0x0650, GN_afii57456 },
    { 0x0651, GN_afii57457 },		    { 0x0652, GN_afii57458 },
    { 0x0660, GN_afii57392 },		    { 0x0661, GN_afii57393 },
    { 0x0662, GN_afii57394 },		    { 0x0663, GN_afii57395 },
    { 0x0664, GN_afii57396 },		    { 0x0665, GN_afii57397 },
    { 0x0666, GN_afii57398 },		    { 0x0667, GN_afii57399 },
    { 0x0668, GN_afii57400 },		    { 0x0669, GN_afii57401 },
    { 0x066a, GN_afii57381 },		    { 0x066d, GN_afii63167 },
    { 0x0679, GN_afii57511 },		    { 0x067e, GN_afii57506 },
    { 0x0686, GN_afii57507 },		    { 0x0688, GN_afii57512 },
    { 0x0691, GN_afii57513 },		    { 0x0698, GN_afii57508 },
    { 0x06a4, GN_afii57505 },		    { 0x06af, GN_afii57509 },
    { 0x06ba, GN_afii57514 },		    { 0x06d2, GN_afii57519 },
    { 0x06d5, GN_afii57534 },		    { 0x1e80, GN_Wgrave },
    { 0x1e81, GN_wgrave },		    { 0x1e82, GN_Wacute },
    { 0x1e83, GN_wacute },		    { 0x1e84, GN_Wdieresis },
    { 0x1e85, GN_wdieresis },		    { 0x1ef2, GN_Ygrave },
    { 0x1ef3, GN_ygrave },		    { 0x200c, GN_afii61664 },
    { 0x200d, GN_afii301 },		    { 0x200e, GN_afii299 },
    { 0x200f, GN_afii300 },		    { 0x2012, GN_figuredash },
    { 0x2013, GN_endash },		    { 0x2014, GN_emdash },
    { 0x2015, GN_afii00208 },		    { 0x2017, GN_underscoredbl },
    { 0x2018, GN_quoteleft },		    { 0x2019, GN_quoteright },
    { 0x201a, GN_quotesinglbase },	    { 0x201b, GN_quotereversed },
    { 0x201c, GN_quotedblleft },	    { 0x201d, GN_quotedblright },
    { 0x201e, GN_quotedblbase },	    { 0x2020, GN_dagger },
    { 0x2021, GN_daggerdbl },		    { 0x2022, GN_bullet },
    { 0x2024, GN_onedotenleader },	    { 0x2025, GN_twodotenleader },
    { 0x2026, GN_ellipsis },		    { 0x202c, GN_afii61573 },
    { 0x202d, GN_afii61574 },		    { 0x202e, GN_afii61575 },
    { 0x2030, GN_perthousand },		    { 0x2032, GN_minute },
    { 0x2033, GN_second },		    { 0x2039, GN_guilsinglleft },
    { 0x203a, GN_guilsinglright },	    { 0x203c, GN_exclamdbl },
    { 0x2044, GN_fraction },		    { 0x2070, GN_zerosuperior },
    { 0x2074, GN_foursuperior },	    { 0x2075, GN_fivesuperior },
    { 0x2076, GN_sixsuperior },		    { 0x2077, GN_sevensuperior },
    { 0x2078, GN_eightsuperior },	    { 0x2079, GN_ninesuperior },
    { 0x207d, GN_parenleftsuperior },	    { 0x207e, GN_parenrightsuperior },
    { 0x207f, GN_nsuperior },		    { 0x2080, GN_zeroinferior },
    { 0x2081, GN_oneinferior },		    { 0x2082, GN_twoinferior },
    { 0x2083, GN_threeinferior },	    { 0x2084, GN_fourinferior },
    { 0x2085, GN_fiveinferior },	    { 0x2086, GN_sixinferior },
    { 0x2087, GN_seveninferior },	    { 0x2088, GN_eightinferior },
    { 0x2089, GN_nineinferior },	    { 0x208d, GN_parenleftinferior },
    { 0x208e, GN_parenrightinferior },	    { 0x20a1, GN_colonmonetary },
    { 0x20a3, GN_franc },		    { 0x20a4, GN_lira },
    { 0x20a7, GN_peseta },		    { 0x20aa, GN_afii57636 },
    { 0x20ab, GN_dong },		    { 0x20ac, GN_Euro },
    { 0x2105, GN_afii61248 },		    { 0x2111, GN_Ifraktur },
    { 0x2113, GN_afii61289 },		    { 0x2116, GN_afii61352 },
    { 0x2118, GN_weierstrass },		    { 0x211c, GN_Rfraktur },
    { 0x211e, GN_prescription },	    { 0x2122, GN_trademark },
    { 0x2126, GN_Omega },		    { 0x212e, GN_estimated },
    { 0x2135, GN_aleph },		    { 0x2153, GN_onethird },
    { 0x2154, GN_twothirds },		    { 0x215b, GN_oneeighth },
    { 0x215c, GN_threeeighths },	    { 0x215d, GN_fiveeighths },
    { 0x215e, GN_seveneighths },	    { 0x2190, GN_arrowleft },
    { 0x2191, GN_arrowup },		    { 0x2192, GN_arrowright },
    { 0x2193, GN_arrowdown },		    { 0x2194, GN_arrowboth },
    { 0x2195, GN_arrowupdn },		    { 0x21a8, GN_arrowupdnbse },
    { 0x21b5, GN_carriagereturn },	    { 0x21d0, GN_arrowdblleft },
    { 0x21d1, GN_arrowdblup },		    { 0x21d2, GN_arrowdblright },
    { 0x21d3, GN_arrowdbldown },	    { 0x21d4, GN_arrowdblboth },
    { 0x2200, GN_universal },		    { 0x2202, GN_partialdiff },
    { 0x2203, GN_existential },		    { 0x2205, GN_emptyset },
    { 0x2206, GN_Delta },		    { 0x2207, GN_gradient },
    { 0x2208, GN_element },		    { 0x2209, GN_notelement },
    { 0x220b, GN_suchthat },		    { 0x220f, GN_product },
    { 0x2211, GN_summation },		    { 0x2212, GN_minus },
    { 0x2215, GN_fraction },		    { 0x2217, GN_asteriskmath },
    { 0x2219, GN_periodcentered },	    { 0x221a, GN_radical },
    { 0x221d, GN_proportional },	    { 0x221e, GN_infinity },
    { 0x221f, GN_orthogonal },		    { 0x2220, GN_angle },
    { 0x2227, GN_logicaland },		    { 0x2228, GN_logicalor },
    { 0x2229, GN_intersection },	    { 0x222a, GN_union },
    { 0x222b, GN_integral },		    { 0x2234, GN_therefore },
    { 0x223c, GN_similar },		    { 0x2245, GN_congruent },
    { 0x2248, GN_approxequal },		    { 0x2260, GN_notequal },
    { 0x2261, GN_equivalence },		    { 0x2264, GN_lessequal },
    { 0x2265, GN_greaterequal },	    { 0x2282, GN_propersubset },
    { 0x2283, GN_propersuperset },	    { 0x2284, GN_notsubset },
    { 0x2286, GN_reflexsubset },	    { 0x2287, GN_reflexsuperset },
    { 0x2295, GN_circleplus },		    { 0x2297, GN_circlemultiply },
    { 0x22a5, GN_perpendicular },	    { 0x22c5, GN_dotmath },
    { 0x2302, GN_house },		    { 0x2310, GN_revlogicalnot },
    { 0x2320, GN_integraltp },		    { 0x2321, GN_integralbt },
    { 0x2329, GN_angleleft },		    { 0x232a, GN_angleright },
    { 0x2500, GN_SF100000 },		    { 0x2502, GN_SF110000 },
    { 0x250c, GN_SF010000 },		    { 0x2510, GN_SF030000 },
    { 0x2514, GN_SF020000 },		    { 0x2518, GN_SF040000 },
    { 0x251c, GN_SF080000 },		    { 0x2524, GN_SF090000 },
    { 0x252c, GN_SF060000 },		    { 0x2534, GN_SF070000 },
    { 0x253c, GN_SF050000 },		    { 0x2550, GN_SF430000 },
    { 0x2551, GN_SF240000 },		    { 0x2552, GN_SF510000 },
    { 0x2553, GN_SF520000 },		    { 0x2554, GN_SF390000 },
    { 0x2555, GN_SF220000 },		    { 0x2556, GN_SF210000 },
    { 0x2557, GN_SF250000 },		    { 0x2558, GN_SF500000 },
    { 0x2559, GN_SF490000 },		    { 0x255a, GN_SF380000 },
    { 0x255b, GN_SF280000 },		    { 0x255c, GN_SF270000 },
    { 0x255d, GN_SF260000 },		    { 0x255e, GN_SF360000 },
    { 0x255f, GN_SF370000 },		    { 0x2560, GN_SF420000 },
    { 0x2561, GN_SF190000 },		    { 0x2562, GN_SF200000 },
    { 0x2563, GN_SF230000 },		    { 0x2564, GN_SF470000 },
    { 0x2565, GN_SF480000 },		    { 0x2566, GN_SF410000 },
    { 0x2567, GN_SF450000 },		    { 0x2568, GN_SF460000 },
    { 0x2569, GN_SF400000 },		    { 0x256a, GN_SF540000 },
    { 0x256b, GN_SF530000 },		    { 0x256c, GN_SF440000 },
    { 0x2580, GN_upblock },		    { 0x2584, GN_dnblock },
    { 0x2588, GN_block },		    { 0x258c, GN_lfblock },
    { 0x2590, GN_rtblock },		    { 0x2591, GN_ltshade },
    { 0x2592, GN_shade },		    { 0x2593, GN_dkshade },
    { 0x25a0, GN_filledbox },		    { 0x25a1, GN_H22073 },
    { 0x25aa, GN_H18543 },		    { 0x25ab, GN_H18551 },
    { 0x25ac, GN_filledrect },		    { 0x25b2, GN_triagup },
    { 0x25ba, GN_triagrt },		    { 0x25bc, GN_triagdn },
    { 0x25c4, GN_triaglf },		    { 0x25ca, GN_lozenge },
    { 0x25cb, GN_circle },		    { 0x25cf, GN_H18533 },
    { 0x25d8, GN_invbullet },		    { 0x25d9, GN_invcircle },
    { 0x25e6, GN_openbullet },		    { 0x263a, GN_smileface },
    { 0x263b, GN_invsmileface },	    { 0x263c, GN_sun },
    { 0x2640, GN_female },		    { 0x2642, GN_male },
    { 0x2660, GN_spade },		    { 0x2663, GN_club },
    { 0x2665, GN_heart },		    { 0x2666, GN_diamond },
    { 0x266a, GN_musicalnote },		    { 0x266b, GN_musicalnotedbl },
    { 0xf6be, GN_dotlessj },		    { 0xf6bf, GN_LL },
    { 0xf6c0, GN_ll },			    { 0xf6c1, GN_Scedilla },
    { 0xf6c2, GN_scedilla },		    { 0xf6c3, GN_commaaccent },
    { 0xf6c4, GN_afii10063 },		    { 0xf6c5, GN_afii10064 },
    { 0xf6c6, GN_afii10192 },		    { 0xf6c7, GN_afii10831 },
    { 0xf6c8, GN_afii10832 },		    { 0xf6c9, GN_Acute },
    { 0xf6ca, GN_Caron },		    { 0xf6cb, GN_Dieresis },
    { 0xf6cc, GN_DieresisAcute },	    { 0xf6cd, GN_DieresisGrave },
    { 0xf6ce, GN_Grave },		    { 0xf6cf, GN_Hungarumlaut },
    { 0xf6d0, GN_Macron },		    { 0xf6d1, GN_cyrBreve },
    { 0xf6d2, GN_cyrFlex },		    { 0xf6d3, GN_dblGrave },
    { 0xf6d4, GN_cyrbreve },		    { 0xf6d5, GN_cyrflex },
    { 0xf6d6, GN_dblgrave },		    { 0xf6d7, GN_dieresisacute },
    { 0xf6d8, GN_dieresisgrave },	    { 0xf6d9, GN_copyrightserif },
    { 0xf6da, GN_registerserif },	    { 0xf6db, GN_trademarkserif },
    { 0xf6dc, GN_onefitted },		    { 0xf6dd, GN_rupiah },
    { 0xf6de, GN_threequartersemdash },	    { 0xf6df, GN_centinferior },
    { 0xf6e0, GN_centsuperior },	    { 0xf6e1, GN_commainferior },
    { 0xf6e2, GN_commasuperior },	    { 0xf6e3, GN_dollarinferior },
    { 0xf6e4, GN_dollarsuperior },	    { 0xf6e5, GN_hypheninferior },
    { 0xf6e6, GN_hyphensuperior },	    { 0xf6e7, GN_periodinferior },
    { 0xf6e8, GN_periodsuperior },	    { 0xf6e9, GN_asuperior },
    { 0xf6ea, GN_bsuperior },		    { 0xf6eb, GN_dsuperior },
    { 0xf6ec, GN_esuperior },		    { 0xf6ed, GN_isuperior },
    { 0xf6ee, GN_lsuperior },		    { 0xf6ef, GN_msuperior },
    { 0xf6f0, GN_osuperior },		    { 0xf6f1, GN_rsuperior },
    { 0xf6f2, GN_ssuperior },		    { 0xf6f3, GN_tsuperior },
    { 0xf6f4, GN_Brevesmall },		    { 0xf6f5, GN_Caronsmall },
    { 0xf6f6, GN_Circumflexsmall },	    { 0xf6f7, GN_Dotaccentsmall },
    { 0xf6f8, GN_Hungarumlautsmall },	    { 0xf6f9, GN_Lslashsmall },
    { 0xf6fa, GN_OEsmall },		    { 0xf6fb, GN_Ogoneksmall },
    { 0xf6fc, GN_Ringsmall },		    { 0xf6fd, GN_Scaronsmall },
    { 0xf6fe, GN_Tildesmall },		    { 0xf6ff, GN_Zcaronsmall },
    { 0xf721, GN_exclamsmall },		    { 0xf724, GN_dollaroldstyle },
    { 0xf726, GN_ampersandsmall },	    { 0xf730, GN_zerooldstyle },
    { 0xf731, GN_oneoldstyle },		    { 0xf732, GN_twooldstyle },
    { 0xf733, GN_threeoldstyle },	    { 0xf734, GN_fouroldstyle },
    { 0xf735, GN_fiveoldstyle },	    { 0xf736, GN_sixoldstyle },
    { 0xf737, GN_sevenoldstyle },	    { 0xf738, GN_eightoldstyle },
    { 0xf739, GN_nineoldstyle },	    { 0xf73f, GN_questionsmall },
    { 0xf760, GN_Gravesmall },		    { 0xf761, GN_Asmall },
    { 0xf762, GN_Bsmall },		    { 0xf763, GN_Csmall },
    { 0xf764, GN_Dsmall },		    { 0xf765, GN_Esmall },
    { 0xf766, GN_Fsmall },		    { 0xf767, GN_Gsmall },
    { 0xf768, GN_Hsmall },		    { 0xf769, GN_Ismall },
    { 0xf76a, GN_Jsmall },		    { 0xf76b, GN_Ksmall },
    { 0xf76c, GN_Lsmall },		    { 0xf76d, GN_Msmall },
    { 0xf76e, GN_Nsmall },		    { 0xf76f, GN_Osmall },
    { 0xf770, GN_Psmall },		    { 0xf771, GN_Qsmall },
    { 0xf772, GN_Rsmall },		    { 0xf773, GN_Ssmall },
    { 0xf774, GN_Tsmall },		    { 0xf775, GN_Usmall },
    { 0xf776, GN_Vsmall },		    { 0xf777, GN_Wsmall },
    { 0xf778, GN_Xsmall },		    { 0xf779, GN_Ysmall },
    { 0xf77a, GN_Zsmall },		    { 0xf7a1, GN_exclamdownsmall },
    { 0xf7a2, GN_centoldstyle },	    { 0xf7a8, GN_Dieresissmall },
    { 0xf7af, GN_Macronsmall },		    { 0xf7b4, GN_Acutesmall },
    { 0xf7b8, GN_Cedillasmall },	    { 0xf7bf, GN_questiondownsmall },
    { 0xf7e0, GN_Agravesmall },		    { 0xf7e1, GN_Aacutesmall },
    { 0xf7e2, GN_Acircumflexsmall },	    { 0xf7e3, GN_Atildesmall },
    { 0xf7e4, GN_Adieresissmall },	    { 0xf7e5, GN_Aringsmall },
    { 0xf7e6, GN_AEsmall },		    { 0xf7e7, GN_Ccedillasmall },
    { 0xf7e8, GN_Egravesmall },		    { 0xf7e9, GN_Eacutesmall },
    { 0xf7ea, GN_Ecircumflexsmall },	    { 0xf7eb, GN_Edieresissmall },
    { 0xf7ec, GN_Igravesmall },		    { 0xf7ed, GN_Iacutesmall },
    { 0xf7ee, GN_Icircumflexsmall },	    { 0xf7ef, GN_Idieresissmall },
    { 0xf7f0, GN_Ethsmall },		    { 0xf7f1, GN_Ntildesmall },
    { 0xf7f2, GN_Ogravesmall },		    { 0xf7f3, GN_Oacutesmall },
    { 0xf7f4, GN_Ocircumflexsmall },	    { 0xf7f5, GN_Otildesmall },
    { 0xf7f6, GN_Odieresissmall },	    { 0xf7f8, GN_Oslashsmall },
    { 0xf7f9, GN_Ugravesmall },		    { 0xf7fa, GN_Uacutesmall },
    { 0xf7fb, GN_Ucircumflexsmall },	    { 0xf7fc, GN_Udieresissmall },
    { 0xf7fd, GN_Yacutesmall },		    { 0xf7fe, GN_Thornsmall },
    { 0xf7ff, GN_Ydieresissmall },	    { 0xf8e5, GN_radicalex },
    { 0xf8e6, GN_arrowvertex },		    { 0xf8e7, GN_arrowhorizex },
    { 0xf8e8, GN_registersans },	    { 0xf8e9, GN_copyrightsans },
    { 0xf8ea, GN_trademarksans },	    { 0xf8eb, GN_parenlefttp },
    { 0xf8ec, GN_parenleftex },		    { 0xf8ed, GN_parenleftbt },
    { 0xf8ee, GN_bracketlefttp },	    { 0xf8ef, GN_bracketleftex },
    { 0xf8f0, GN_bracketleftbt },	    { 0xf8f1, GN_bracelefttp },
    { 0xf8f2, GN_braceleftmid },	    { 0xf8f3, GN_braceleftbt },
    { 0xf8f4, GN_braceex },		    { 0xf8f5, GN_integralex },
    { 0xf8f6, GN_parenrighttp },	    { 0xf8f7, GN_parenrightex },
    { 0xf8f8, GN_parenrightbt },	    { 0xf8f9, GN_bracketrighttp },
    { 0xf8fa, GN_bracketrightex },	    { 0xf8fb, GN_bracketrightbt },
    { 0xf8fc, GN_bracerighttp },	    { 0xf8fd, GN_bracerightmid },
    { 0xf8fe, GN_bracerightbt },	    { 0xfb00, GN_ff },
    { 0xfb01, GN_fi },			    { 0xfb02, GN_fl },
    { 0xfb03, GN_ffi },			    { 0xfb04, GN_ffl },
    { 0xfb1f, GN_afii57705 },		    { 0xfb2a, GN_afii57694 },
    { 0xfb2b, GN_afii57695 },		    { 0xfb35, GN_afii57723 },
    { 0xfb4b, GN_afii57700 }
};
