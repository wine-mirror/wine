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

AFM *const PSDRV_BuiltinAFMs[] =
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
    { -1, "A" },		    /* LATIN CAPITAL LETTER A */
    { -1, "AE" },		    /* LATIN CAPITAL LETTER AE */
    { -1, "AEacute" },		    /* LATIN CAPITAL LETTER AE WITH ACUTE */
    { -1, "AEsmall" },		    /* LATIN SMALL CAPITAL LETTER AE */
    { -1, "Aacute" },		    /* LATIN CAPITAL LETTER A WITH ACUTE */
    { -1, "Aacutesmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH ACUTE */
    { -1, "Abreve" },		    /* LATIN CAPITAL LETTER A WITH BREVE */
    { -1, "Acircumflex" },	    /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
    { -1, "Acircumflexsmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH CIRCUMFLEX */
    { -1, "Acute" },		    /* CAPITAL ACUTE ACCENT */
    { -1, "Acutesmall" },	    /* SMALL CAPITAL ACUTE ACCENT */
    { -1, "Adieresis" },	    /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    { -1, "Adieresissmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH DIAERESIS */
    { -1, "Agrave" },		    /* LATIN CAPITAL LETTER A WITH GRAVE */
    { -1, "Agravesmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH GRAVE */
    { -1, "Alpha" },		    /* GREEK CAPITAL LETTER ALPHA */
    { -1, "Alphatonos" },	    /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
    { -1, "Amacron" },		    /* LATIN CAPITAL LETTER A WITH MACRON */
    { -1, "Aogonek" },		    /* LATIN CAPITAL LETTER A WITH OGONEK */
    { -1, "Aring" },		    /* LATIN CAPITAL LETTER A WITH RING ABOVE */
    { -1, "Aringacute" },	    /* LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE */
    { -1, "Aringsmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH RING ABOVE */
    { -1, "Asmall" },		    /* LATIN SMALL CAPITAL LETTER A */
    { -1, "Atilde" },		    /* LATIN CAPITAL LETTER A WITH TILDE */
    { -1, "Atildesmall" },	    /* LATIN SMALL CAPITAL LETTER A WITH TILDE */
    { -1, "B" },		    /* LATIN CAPITAL LETTER B */
    { -1, "Beta" },		    /* GREEK CAPITAL LETTER BETA */
    { -1, "Brevesmall" },	    /* SMALL CAPITAL BREVE */
    { -1, "Bsmall" },		    /* LATIN SMALL CAPITAL LETTER B */
    { -1, "C" },		    /* LATIN CAPITAL LETTER C */
    { -1, "Cacute" },		    /* LATIN CAPITAL LETTER C WITH ACUTE */
    { -1, "Caron" },		    /* CAPITAL CARON */
    { -1, "Caronsmall" },	    /* SMALL CAPITAL CARON */
    { -1, "Ccaron" },		    /* LATIN CAPITAL LETTER C WITH CARON */
    { -1, "Ccedilla" },		    /* LATIN CAPITAL LETTER C WITH CEDILLA */
    { -1, "Ccedillasmall" },	    /* LATIN SMALL CAPITAL LETTER C WITH CEDILLA */
    { -1, "Ccircumflex" },	    /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    { -1, "Cdotaccent" },	    /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
    { -1, "Cedillasmall" },	    /* SMALL CAPITAL CEDILLA */
    { -1, "Chi" },		    /* GREEK CAPITAL LETTER CHI */
    { -1, "Circumflexsmall" },	    /* SMALL CAPITAL MODIFIER LETTER CIRCUMFLEX ACCENT */
    { -1, "Csmall" },		    /* LATIN SMALL CAPITAL LETTER C */
    { -1, "D" },		    /* LATIN CAPITAL LETTER D */
    { -1, "Dcaron" },		    /* LATIN CAPITAL LETTER D WITH CARON */
    { -1, "Dcroat" },		    /* LATIN CAPITAL LETTER D WITH STROKE */
    { -1, "Delta" },		    /* INCREMENT */
				    /* GREEK CAPITAL LETTER DELTA;Duplicate */
    { -1, "Dieresis" },		    /* CAPITAL DIAERESIS */
    { -1, "DieresisAcute" },	    /* CAPITAL DIAERESIS ACUTE ACCENT */
    { -1, "DieresisGrave" },	    /* CAPITAL DIAERESIS GRAVE ACCENT */
    { -1, "Dieresissmall" },	    /* SMALL CAPITAL DIAERESIS */
    { -1, "Dotaccentsmall" },	    /* SMALL CAPITAL DOT ABOVE */
    { -1, "Dsmall" },		    /* LATIN SMALL CAPITAL LETTER D */
    { -1, "E" },		    /* LATIN CAPITAL LETTER E */
    { -1, "Eacute" },		    /* LATIN CAPITAL LETTER E WITH ACUTE */
    { -1, "Eacutesmall" },	    /* LATIN SMALL CAPITAL LETTER E WITH ACUTE */
    { -1, "Ebreve" },		    /* LATIN CAPITAL LETTER E WITH BREVE */
    { -1, "Ecaron" },		    /* LATIN CAPITAL LETTER E WITH CARON */
    { -1, "Ecircumflex" },	    /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
    { -1, "Ecircumflexsmall" },	    /* LATIN SMALL CAPITAL LETTER E WITH CIRCUMFLEX */
    { -1, "Edieresis" },	    /* LATIN CAPITAL LETTER E WITH DIAERESIS */
    { -1, "Edieresissmall" },	    /* LATIN SMALL CAPITAL LETTER E WITH DIAERESIS */
    { -1, "Edotaccent" },	    /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
    { -1, "Egrave" },		    /* LATIN CAPITAL LETTER E WITH GRAVE */
    { -1, "Egravesmall" },	    /* LATIN SMALL CAPITAL LETTER E WITH GRAVE */
    { -1, "Emacron" },		    /* LATIN CAPITAL LETTER E WITH MACRON */
    { -1, "Eng" },		    /* LATIN CAPITAL LETTER ENG */
    { -1, "Eogonek" },		    /* LATIN CAPITAL LETTER E WITH OGONEK */
    { -1, "Epsilon" },		    /* GREEK CAPITAL LETTER EPSILON */
    { -1, "Epsilontonos" },	    /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
    { -1, "Esmall" },		    /* LATIN SMALL CAPITAL LETTER E */
    { -1, "Eta" },		    /* GREEK CAPITAL LETTER ETA */
    { -1, "Etatonos" },		    /* GREEK CAPITAL LETTER ETA WITH TONOS */
    { -1, "Eth" },		    /* LATIN CAPITAL LETTER ETH */
    { -1, "Ethsmall" },		    /* LATIN SMALL CAPITAL LETTER ETH */
    { -1, "Euro" },		    /* EURO SIGN */
    { -1, "F" },		    /* LATIN CAPITAL LETTER F */
    { -1, "Fsmall" },		    /* LATIN SMALL CAPITAL LETTER F */
    { -1, "G" },		    /* LATIN CAPITAL LETTER G */
    { -1, "Gamma" },		    /* GREEK CAPITAL LETTER GAMMA */
    { -1, "Gbreve" },		    /* LATIN CAPITAL LETTER G WITH BREVE */
    { -1, "Gcaron" },		    /* LATIN CAPITAL LETTER G WITH CARON */
    { -1, "Gcircumflex" },	    /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    { -1, "Gcommaaccent" },	    /* LATIN CAPITAL LETTER G WITH CEDILLA */
    { -1, "Gdotaccent" },	    /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
    { -1, "Grave" },		    /* CAPITAL GRAVE ACCENT */
    { -1, "Gravesmall" },	    /* SMALL CAPITAL GRAVE ACCENT */
    { -1, "Gsmall" },		    /* LATIN SMALL CAPITAL LETTER G */
    { -1, "H" },		    /* LATIN CAPITAL LETTER H */
    { -1, "H18533" },		    /* BLACK CIRCLE */
    { -1, "H18543" },		    /* BLACK SMALL SQUARE */
    { -1, "H18551" },		    /* WHITE SMALL SQUARE */
    { -1, "H22073" },		    /* WHITE SQUARE */
    { -1, "Hbar" },		    /* LATIN CAPITAL LETTER H WITH STROKE */
    { -1, "Hcircumflex" },	    /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    { -1, "Hsmall" },		    /* LATIN SMALL CAPITAL LETTER H */
    { -1, "Hungarumlaut" },	    /* CAPITAL DOUBLE ACUTE ACCENT */
    { -1, "Hungarumlautsmall" },    /* SMALL CAPITAL DOUBLE ACUTE ACCENT */
    { -1, "I" },		    /* LATIN CAPITAL LETTER I */
    { -1, "IJ" },		    /* LATIN CAPITAL LIGATURE IJ */
    { -1, "Iacute" },		    /* LATIN CAPITAL LETTER I WITH ACUTE */
    { -1, "Iacutesmall" },	    /* LATIN SMALL CAPITAL LETTER I WITH ACUTE */
    { -1, "Ibreve" },		    /* LATIN CAPITAL LETTER I WITH BREVE */
    { -1, "Icircumflex" },	    /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
    { -1, "Icircumflexsmall" },	    /* LATIN SMALL CAPITAL LETTER I WITH CIRCUMFLEX */
    { -1, "Idieresis" },	    /* LATIN CAPITAL LETTER I WITH DIAERESIS */
    { -1, "Idieresissmall" },	    /* LATIN SMALL CAPITAL LETTER I WITH DIAERESIS */
    { -1, "Idot" },		    /* FONT FAMILY;Courier */
    { -1, "Idotaccent" },	    /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    { -1, "Ifraktur" },		    /* BLACK-LETTER CAPITAL I */
    { -1, "Igrave" },		    /* LATIN CAPITAL LETTER I WITH GRAVE */
    { -1, "Igravesmall" },	    /* LATIN SMALL CAPITAL LETTER I WITH GRAVE */
    { -1, "Imacron" },		    /* LATIN CAPITAL LETTER I WITH MACRON */
    { -1, "Iogonek" },		    /* LATIN CAPITAL LETTER I WITH OGONEK */
    { -1, "Iota" },		    /* GREEK CAPITAL LETTER IOTA */
    { -1, "Iotadieresis" },	    /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    { -1, "Iotatonos" },	    /* GREEK CAPITAL LETTER IOTA WITH TONOS */
    { -1, "Ismall" },		    /* LATIN SMALL CAPITAL LETTER I */
    { -1, "Itilde" },		    /* LATIN CAPITAL LETTER I WITH TILDE */
    { -1, "J" },		    /* LATIN CAPITAL LETTER J */
    { -1, "Jcircumflex" },	    /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    { -1, "Jsmall" },		    /* LATIN SMALL CAPITAL LETTER J */
    { -1, "K" },		    /* LATIN CAPITAL LETTER K */
    { -1, "Kappa" },		    /* GREEK CAPITAL LETTER KAPPA */
    { -1, "Kcommaaccent" },	    /* LATIN CAPITAL LETTER K WITH CEDILLA */
    { -1, "Ksmall" },		    /* LATIN SMALL CAPITAL LETTER K */
    { -1, "L" },		    /* LATIN CAPITAL LETTER L */
    { -1, "LL" },		    /* LATIN CAPITAL LETTER LL */
    { -1, "Lacute" },		    /* LATIN CAPITAL LETTER L WITH ACUTE */
    { -1, "Lambda" },		    /* GREEK CAPITAL LETTER LAMDA */
    { -1, "Lcaron" },		    /* LATIN CAPITAL LETTER L WITH CARON */
    { -1, "Lcommaaccent" },	    /* LATIN CAPITAL LETTER L WITH CEDILLA */
    { -1, "Ldot" },		    /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
    { -1, "Lslash" },		    /* LATIN CAPITAL LETTER L WITH STROKE */
    { -1, "Lslashsmall" },	    /* LATIN SMALL CAPITAL LETTER L WITH STROKE */
    { -1, "Lsmall" },		    /* LATIN SMALL CAPITAL LETTER L */
    { -1, "M" },		    /* LATIN CAPITAL LETTER M */
    { -1, "Macron" },		    /* CAPITAL MACRON */
    { -1, "Macronsmall" },	    /* SMALL CAPITAL MACRON */
    { -1, "Msmall" },		    /* LATIN SMALL CAPITAL LETTER M */
    { -1, "Mu" },		    /* GREEK CAPITAL LETTER MU */
    { -1, "N" },		    /* LATIN CAPITAL LETTER N */
    { -1, "Nacute" },		    /* LATIN CAPITAL LETTER N WITH ACUTE */
    { -1, "Ncaron" },		    /* LATIN CAPITAL LETTER N WITH CARON */
    { -1, "Ncommaaccent" },	    /* LATIN CAPITAL LETTER N WITH CEDILLA */
    { -1, "Nsmall" },		    /* LATIN SMALL CAPITAL LETTER N */
    { -1, "Ntilde" },		    /* LATIN CAPITAL LETTER N WITH TILDE */
    { -1, "Ntildesmall" },	    /* LATIN SMALL CAPITAL LETTER N WITH TILDE */
    { -1, "Nu" },		    /* GREEK CAPITAL LETTER NU */
    { -1, "O" },		    /* LATIN CAPITAL LETTER O */
    { -1, "OE" },		    /* LATIN CAPITAL LIGATURE OE */
    { -1, "OEsmall" },		    /* LATIN SMALL CAPITAL LIGATURE OE */
    { -1, "Oacute" },		    /* LATIN CAPITAL LETTER O WITH ACUTE */
    { -1, "Oacutesmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH ACUTE */
    { -1, "Obreve" },		    /* LATIN CAPITAL LETTER O WITH BREVE */
    { -1, "Ocircumflex" },	    /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
    { -1, "Ocircumflexsmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH CIRCUMFLEX */
    { -1, "Odieresis" },	    /* LATIN CAPITAL LETTER O WITH DIAERESIS */
    { -1, "Odieresissmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH DIAERESIS */
    { -1, "Ogoneksmall" },	    /* SMALL CAPITAL OGONEK */
    { -1, "Ograve" },		    /* LATIN CAPITAL LETTER O WITH GRAVE */
    { -1, "Ogravesmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH GRAVE */
    { -1, "Ohorn" },		    /* LATIN CAPITAL LETTER O WITH HORN */
    { -1, "Ohungarumlaut" },	    /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    { -1, "Omacron" },		    /* LATIN CAPITAL LETTER O WITH MACRON */
    { -1, "Omega" },		    /* OHM SIGN */
				    /* GREEK CAPITAL LETTER OMEGA;Duplicate */
    { -1, "Omegatonos" },	    /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
    { -1, "Omicron" },		    /* GREEK CAPITAL LETTER OMICRON */
    { -1, "Omicrontonos" },	    /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
    { -1, "Oslash" },		    /* LATIN CAPITAL LETTER O WITH STROKE */
    { -1, "Oslashacute" },	    /* LATIN CAPITAL LETTER O WITH STROKE AND ACUTE */
    { -1, "Oslashsmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH STROKE */
    { -1, "Osmall" },		    /* LATIN SMALL CAPITAL LETTER O */
    { -1, "Otilde" },		    /* LATIN CAPITAL LETTER O WITH TILDE */
    { -1, "Otildesmall" },	    /* LATIN SMALL CAPITAL LETTER O WITH TILDE */
    { -1, "P" },		    /* LATIN CAPITAL LETTER P */
    { -1, "Phi" },		    /* GREEK CAPITAL LETTER PHI */
    { -1, "Pi" },		    /* GREEK CAPITAL LETTER PI */
    { -1, "Psi" },		    /* GREEK CAPITAL LETTER PSI */
    { -1, "Psmall" },		    /* LATIN SMALL CAPITAL LETTER P */
    { -1, "Q" },		    /* LATIN CAPITAL LETTER Q */
    { -1, "Qsmall" },		    /* LATIN SMALL CAPITAL LETTER Q */
    { -1, "R" },		    /* LATIN CAPITAL LETTER R */
    { -1, "Racute" },		    /* LATIN CAPITAL LETTER R WITH ACUTE */
    { -1, "Rcaron" },		    /* LATIN CAPITAL LETTER R WITH CARON */
    { -1, "Rcommaaccent" },	    /* LATIN CAPITAL LETTER R WITH CEDILLA */
    { -1, "Rfraktur" },		    /* BLACK-LETTER CAPITAL R */
    { -1, "Rho" },		    /* GREEK CAPITAL LETTER RHO */
    { -1, "Ringsmall" },	    /* SMALL CAPITAL RING ABOVE */
    { -1, "Rsmall" },		    /* LATIN SMALL CAPITAL LETTER R */
    { -1, "S" },		    /* LATIN CAPITAL LETTER S */
    { -1, "SF010000" },		    /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    { -1, "SF020000" },		    /* BOX DRAWINGS LIGHT UP AND RIGHT */
    { -1, "SF030000" },		    /* BOX DRAWINGS LIGHT DOWN AND LEFT */
    { -1, "SF040000" },		    /* BOX DRAWINGS LIGHT UP AND LEFT */
    { -1, "SF050000" },		    /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    { -1, "SF060000" },		    /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    { -1, "SF070000" },		    /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    { -1, "SF080000" },		    /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    { -1, "SF090000" },		    /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    { -1, "SF100000" },		    /* BOX DRAWINGS LIGHT HORIZONTAL */
    { -1, "SF110000" },		    /* BOX DRAWINGS LIGHT VERTICAL */
    { -1, "SF190000" },		    /* BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE */
    { -1, "SF200000" },		    /* BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE */
    { -1, "SF210000" },		    /* BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE */
    { -1, "SF220000" },		    /* BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE */
    { -1, "SF230000" },		    /* BOX DRAWINGS DOUBLE VERTICAL AND LEFT */
    { -1, "SF240000" },		    /* BOX DRAWINGS DOUBLE VERTICAL */
    { -1, "SF250000" },		    /* BOX DRAWINGS DOUBLE DOWN AND LEFT */
    { -1, "SF260000" },		    /* BOX DRAWINGS DOUBLE UP AND LEFT */
    { -1, "SF270000" },		    /* BOX DRAWINGS UP DOUBLE AND LEFT SINGLE */
    { -1, "SF280000" },		    /* BOX DRAWINGS UP SINGLE AND LEFT DOUBLE */
    { -1, "SF360000" },		    /* BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE */
    { -1, "SF370000" },		    /* BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE */
    { -1, "SF380000" },		    /* BOX DRAWINGS DOUBLE UP AND RIGHT */
    { -1, "SF390000" },		    /* BOX DRAWINGS DOUBLE DOWN AND RIGHT */
    { -1, "SF400000" },		    /* BOX DRAWINGS DOUBLE UP AND HORIZONTAL */
    { -1, "SF410000" },		    /* BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL */
    { -1, "SF420000" },		    /* BOX DRAWINGS DOUBLE VERTICAL AND RIGHT */
    { -1, "SF430000" },		    /* BOX DRAWINGS DOUBLE HORIZONTAL */
    { -1, "SF440000" },		    /* BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL */
    { -1, "SF450000" },		    /* BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE */
    { -1, "SF460000" },		    /* BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE */
    { -1, "SF470000" },		    /* BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE */
    { -1, "SF480000" },		    /* BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE */
    { -1, "SF490000" },		    /* BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE */
    { -1, "SF500000" },		    /* BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE */
    { -1, "SF510000" },		    /* BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE */
    { -1, "SF520000" },		    /* BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE */
    { -1, "SF530000" },		    /* BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE */
    { -1, "SF540000" },		    /* BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE */
    { -1, "Sacute" },		    /* LATIN CAPITAL LETTER S WITH ACUTE */
    { -1, "Scaron" },		    /* LATIN CAPITAL LETTER S WITH CARON */
    { -1, "Scaronsmall" },	    /* LATIN SMALL CAPITAL LETTER S WITH CARON */
    { -1, "Scedilla" },		    /* LATIN CAPITAL LETTER S WITH CEDILLA */
				    /* LATIN CAPITAL LETTER S WITH CEDILLA;Duplicate */
    { -1, "Scircumflex" },	    /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    { -1, "Scommaaccent" },	    /* LATIN CAPITAL LETTER S WITH COMMA BELOW */
    { -1, "Sigma" },		    /* GREEK CAPITAL LETTER SIGMA */
    { -1, "Ssmall" },		    /* LATIN SMALL CAPITAL LETTER S */
    { -1, "T" },		    /* LATIN CAPITAL LETTER T */
    { -1, "Tau" },		    /* GREEK CAPITAL LETTER TAU */
    { -1, "Tbar" },		    /* LATIN CAPITAL LETTER T WITH STROKE */
    { -1, "Tcaron" },		    /* LATIN CAPITAL LETTER T WITH CARON */
    { -1, "Tcommaaccent" },	    /* LATIN CAPITAL LETTER T WITH CEDILLA */
				    /* LATIN CAPITAL LETTER T WITH COMMA BELOW;Duplicate */
    { -1, "Theta" },		    /* GREEK CAPITAL LETTER THETA */
    { -1, "Thorn" },		    /* LATIN CAPITAL LETTER THORN */
    { -1, "Thornsmall" },	    /* LATIN SMALL CAPITAL LETTER THORN */
    { -1, "Tildesmall" },	    /* SMALL CAPITAL SMALL TILDE */
    { -1, "Tsmall" },		    /* LATIN SMALL CAPITAL LETTER T */
    { -1, "U" },		    /* LATIN CAPITAL LETTER U */
    { -1, "Uacute" },		    /* LATIN CAPITAL LETTER U WITH ACUTE */
    { -1, "Uacutesmall" },	    /* LATIN SMALL CAPITAL LETTER U WITH ACUTE */
    { -1, "Ubreve" },		    /* LATIN CAPITAL LETTER U WITH BREVE */
    { -1, "Ucircumflex" },	    /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
    { -1, "Ucircumflexsmall" },	    /* LATIN SMALL CAPITAL LETTER U WITH CIRCUMFLEX */
    { -1, "Udieresis" },	    /* LATIN CAPITAL LETTER U WITH DIAERESIS */
    { -1, "Udieresissmall" },	    /* LATIN SMALL CAPITAL LETTER U WITH DIAERESIS */
    { -1, "Ugrave" },		    /* LATIN CAPITAL LETTER U WITH GRAVE */
    { -1, "Ugravesmall" },	    /* LATIN SMALL CAPITAL LETTER U WITH GRAVE */
    { -1, "Uhorn" },		    /* LATIN CAPITAL LETTER U WITH HORN */
    { -1, "Uhungarumlaut" },	    /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    { -1, "Umacron" },		    /* LATIN CAPITAL LETTER U WITH MACRON */
    { -1, "Uogonek" },		    /* LATIN CAPITAL LETTER U WITH OGONEK */
    { -1, "Upsilon" },		    /* GREEK CAPITAL LETTER UPSILON */
    { -1, "Upsilon1" },		    /* GREEK UPSILON WITH HOOK SYMBOL */
    { -1, "Upsilondieresis" },	    /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    { -1, "Upsilontonos" },	    /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
    { -1, "Uring" },		    /* LATIN CAPITAL LETTER U WITH RING ABOVE */
    { -1, "Usmall" },		    /* LATIN SMALL CAPITAL LETTER U */
    { -1, "Utilde" },		    /* LATIN CAPITAL LETTER U WITH TILDE */
    { -1, "V" },		    /* LATIN CAPITAL LETTER V */
    { -1, "Vsmall" },		    /* LATIN SMALL CAPITAL LETTER V */
    { -1, "W" },		    /* LATIN CAPITAL LETTER W */
    { -1, "Wacute" },		    /* LATIN CAPITAL LETTER W WITH ACUTE */
    { -1, "Wcircumflex" },	    /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
    { -1, "Wdieresis" },	    /* LATIN CAPITAL LETTER W WITH DIAERESIS */
    { -1, "Wgrave" },		    /* LATIN CAPITAL LETTER W WITH GRAVE */
    { -1, "Wsmall" },		    /* LATIN SMALL CAPITAL LETTER W */
    { -1, "X" },		    /* LATIN CAPITAL LETTER X */
    { -1, "Xi" },		    /* GREEK CAPITAL LETTER XI */
    { -1, "Xsmall" },		    /* LATIN SMALL CAPITAL LETTER X */
    { -1, "Y" },		    /* LATIN CAPITAL LETTER Y */
    { -1, "Yacute" },		    /* LATIN CAPITAL LETTER Y WITH ACUTE */
    { -1, "Yacutesmall" },	    /* LATIN SMALL CAPITAL LETTER Y WITH ACUTE */
    { -1, "Ycircumflex" },	    /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
    { -1, "Ydieresis" },	    /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
    { -1, "Ydieresissmall" },	    /* LATIN SMALL CAPITAL LETTER Y WITH DIAERESIS */
    { -1, "Ygrave" },		    /* LATIN CAPITAL LETTER Y WITH GRAVE */
    { -1, "Ysmall" },		    /* LATIN SMALL CAPITAL LETTER Y */
    { -1, "Z" },		    /* LATIN CAPITAL LETTER Z */
    { -1, "Zacute" },		    /* LATIN CAPITAL LETTER Z WITH ACUTE */
    { -1, "Zcaron" },		    /* LATIN CAPITAL LETTER Z WITH CARON */
    { -1, "Zcaronsmall" },	    /* LATIN SMALL CAPITAL LETTER Z WITH CARON */
    { -1, "Zdotaccent" },	    /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    { -1, "Zeta" },		    /* GREEK CAPITAL LETTER ZETA */
    { -1, "Zsmall" },		    /* LATIN SMALL CAPITAL LETTER Z */
    { -1, "a" },		    /* LATIN SMALL LETTER A */
    { -1, "a1" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a10" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a100" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a101" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a102" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a103" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a104" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a105" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a106" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a107" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a108" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a109" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a11" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a110" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a111" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a112" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a117" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a118" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a119" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a12" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a120" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a121" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a122" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a123" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a124" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a125" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a126" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a127" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a128" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a129" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a13" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a130" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a131" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a132" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a133" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a134" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a135" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a136" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a137" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a138" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a139" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a14" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a140" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a141" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a142" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a143" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a144" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a145" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a146" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a147" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a148" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a149" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a15" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a150" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a151" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a152" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a153" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a154" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a155" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a156" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a157" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a158" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a159" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a16" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a160" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a161" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a162" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a163" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a164" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a165" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a166" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a167" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a168" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a169" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a17" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a170" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a171" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a172" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a173" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a174" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a175" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a176" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a177" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a178" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a179" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a18" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a180" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a181" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a182" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a183" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a184" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a185" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a186" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a187" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a188" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a189" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a19" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a190" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a191" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a192" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a193" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a194" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a195" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a196" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a197" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a198" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a199" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a2" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a20" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a200" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a201" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a202" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a203" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a204" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a205" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a206" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a21" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a22" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a23" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a24" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a25" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a26" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a27" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a28" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a29" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a3" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a30" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a31" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a32" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a33" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a34" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a35" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a36" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a37" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a38" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a39" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a4" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a40" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a41" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a42" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a43" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a44" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a45" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a46" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a47" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a48" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a49" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a5" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a50" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a51" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a52" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a53" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a54" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a55" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a56" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a57" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a58" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a59" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a6" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a60" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a61" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a62" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a63" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a64" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a65" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a66" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a67" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a68" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a69" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a7" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a70" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a71" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a72" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a73" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a74" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a75" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a76" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a77" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a78" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a79" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a8" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a81" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a82" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a83" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a84" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a85" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a86" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a87" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a88" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a89" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a9" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a90" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a91" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a92" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a93" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a94" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a95" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a96" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a97" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a98" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "a99" },		    /* FONT FAMILY;ITC Zapf Dingbats */
    { -1, "aacute" },		    /* LATIN SMALL LETTER A WITH ACUTE */
    { -1, "abreve" },		    /* LATIN SMALL LETTER A WITH BREVE */
    { -1, "acircumflex" },	    /* LATIN SMALL LETTER A WITH CIRCUMFLEX */
    { -1, "acute" },		    /* ACUTE ACCENT */
    { -1, "acutecomb" },	    /* COMBINING ACUTE ACCENT */
    { -1, "adieresis" },	    /* LATIN SMALL LETTER A WITH DIAERESIS */
    { -1, "ae" },		    /* LATIN SMALL LETTER AE */
    { -1, "aeacute" },		    /* LATIN SMALL LETTER AE WITH ACUTE */
    { -1, "afii00208" },	    /* HORIZONTAL BAR */
    { -1, "afii10017" },	    /* CYRILLIC CAPITAL LETTER A */
    { -1, "afii10018" },	    /* CYRILLIC CAPITAL LETTER BE */
    { -1, "afii10019" },	    /* CYRILLIC CAPITAL LETTER VE */
    { -1, "afii10020" },	    /* CYRILLIC CAPITAL LETTER GHE */
    { -1, "afii10021" },	    /* CYRILLIC CAPITAL LETTER DE */
    { -1, "afii10022" },	    /* CYRILLIC CAPITAL LETTER IE */
    { -1, "afii10023" },	    /* CYRILLIC CAPITAL LETTER IO */
    { -1, "afii10024" },	    /* CYRILLIC CAPITAL LETTER ZHE */
    { -1, "afii10025" },	    /* CYRILLIC CAPITAL LETTER ZE */
    { -1, "afii10026" },	    /* CYRILLIC CAPITAL LETTER I */
    { -1, "afii10027" },	    /* CYRILLIC CAPITAL LETTER SHORT I */
    { -1, "afii10028" },	    /* CYRILLIC CAPITAL LETTER KA */
    { -1, "afii10029" },	    /* CYRILLIC CAPITAL LETTER EL */
    { -1, "afii10030" },	    /* CYRILLIC CAPITAL LETTER EM */
    { -1, "afii10031" },	    /* CYRILLIC CAPITAL LETTER EN */
    { -1, "afii10032" },	    /* CYRILLIC CAPITAL LETTER O */
    { -1, "afii10033" },	    /* CYRILLIC CAPITAL LETTER PE */
    { -1, "afii10034" },	    /* CYRILLIC CAPITAL LETTER ER */
    { -1, "afii10035" },	    /* CYRILLIC CAPITAL LETTER ES */
    { -1, "afii10036" },	    /* CYRILLIC CAPITAL LETTER TE */
    { -1, "afii10037" },	    /* CYRILLIC CAPITAL LETTER U */
    { -1, "afii10038" },	    /* CYRILLIC CAPITAL LETTER EF */
    { -1, "afii10039" },	    /* CYRILLIC CAPITAL LETTER HA */
    { -1, "afii10040" },	    /* CYRILLIC CAPITAL LETTER TSE */
    { -1, "afii10041" },	    /* CYRILLIC CAPITAL LETTER CHE */
    { -1, "afii10042" },	    /* CYRILLIC CAPITAL LETTER SHA */
    { -1, "afii10043" },	    /* CYRILLIC CAPITAL LETTER SHCHA */
    { -1, "afii10044" },	    /* CYRILLIC CAPITAL LETTER HARD SIGN */
    { -1, "afii10045" },	    /* CYRILLIC CAPITAL LETTER YERU */
    { -1, "afii10046" },	    /* CYRILLIC CAPITAL LETTER SOFT SIGN */
    { -1, "afii10047" },	    /* CYRILLIC CAPITAL LETTER E */
    { -1, "afii10048" },	    /* CYRILLIC CAPITAL LETTER YU */
    { -1, "afii10049" },	    /* CYRILLIC CAPITAL LETTER YA */
    { -1, "afii10050" },	    /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
    { -1, "afii10051" },	    /* CYRILLIC CAPITAL LETTER DJE */
    { -1, "afii10052" },	    /* CYRILLIC CAPITAL LETTER GJE */
    { -1, "afii10053" },	    /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    { -1, "afii10054" },	    /* CYRILLIC CAPITAL LETTER DZE */
    { -1, "afii10055" },	    /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    { -1, "afii10056" },	    /* CYRILLIC CAPITAL LETTER YI */
    { -1, "afii10057" },	    /* CYRILLIC CAPITAL LETTER JE */
    { -1, "afii10058" },	    /* CYRILLIC CAPITAL LETTER LJE */
    { -1, "afii10059" },	    /* CYRILLIC CAPITAL LETTER NJE */
    { -1, "afii10060" },	    /* CYRILLIC CAPITAL LETTER TSHE */
    { -1, "afii10061" },	    /* CYRILLIC CAPITAL LETTER KJE */
    { -1, "afii10062" },	    /* CYRILLIC CAPITAL LETTER SHORT U */
    { -1, "afii10063" },	    /* CYRILLIC SMALL LETTER GHE VARIANT */
    { -1, "afii10064" },	    /* CYRILLIC SMALL LETTER BE VARIANT */
    { -1, "afii10065" },	    /* CYRILLIC SMALL LETTER A */
    { -1, "afii10066" },	    /* CYRILLIC SMALL LETTER BE */
    { -1, "afii10067" },	    /* CYRILLIC SMALL LETTER VE */
    { -1, "afii10068" },	    /* CYRILLIC SMALL LETTER GHE */
    { -1, "afii10069" },	    /* CYRILLIC SMALL LETTER DE */
    { -1, "afii10070" },	    /* CYRILLIC SMALL LETTER IE */
    { -1, "afii10071" },	    /* CYRILLIC SMALL LETTER IO */
    { -1, "afii10072" },	    /* CYRILLIC SMALL LETTER ZHE */
    { -1, "afii10073" },	    /* CYRILLIC SMALL LETTER ZE */
    { -1, "afii10074" },	    /* CYRILLIC SMALL LETTER I */
    { -1, "afii10075" },	    /* CYRILLIC SMALL LETTER SHORT I */
    { -1, "afii10076" },	    /* CYRILLIC SMALL LETTER KA */
    { -1, "afii10077" },	    /* CYRILLIC SMALL LETTER EL */
    { -1, "afii10078" },	    /* CYRILLIC SMALL LETTER EM */
    { -1, "afii10079" },	    /* CYRILLIC SMALL LETTER EN */
    { -1, "afii10080" },	    /* CYRILLIC SMALL LETTER O */
    { -1, "afii10081" },	    /* CYRILLIC SMALL LETTER PE */
    { -1, "afii10082" },	    /* CYRILLIC SMALL LETTER ER */
    { -1, "afii10083" },	    /* CYRILLIC SMALL LETTER ES */
    { -1, "afii10084" },	    /* CYRILLIC SMALL LETTER TE */
    { -1, "afii10085" },	    /* CYRILLIC SMALL LETTER U */
    { -1, "afii10086" },	    /* CYRILLIC SMALL LETTER EF */
    { -1, "afii10087" },	    /* CYRILLIC SMALL LETTER HA */
    { -1, "afii10088" },	    /* CYRILLIC SMALL LETTER TSE */
    { -1, "afii10089" },	    /* CYRILLIC SMALL LETTER CHE */
    { -1, "afii10090" },	    /* CYRILLIC SMALL LETTER SHA */
    { -1, "afii10091" },	    /* CYRILLIC SMALL LETTER SHCHA */
    { -1, "afii10092" },	    /* CYRILLIC SMALL LETTER HARD SIGN */
    { -1, "afii10093" },	    /* CYRILLIC SMALL LETTER YERU */
    { -1, "afii10094" },	    /* CYRILLIC SMALL LETTER SOFT SIGN */
    { -1, "afii10095" },	    /* CYRILLIC SMALL LETTER E */
    { -1, "afii10096" },	    /* CYRILLIC SMALL LETTER YU */
    { -1, "afii10097" },	    /* CYRILLIC SMALL LETTER YA */
    { -1, "afii10098" },	    /* CYRILLIC SMALL LETTER GHE WITH UPTURN */
    { -1, "afii10099" },	    /* CYRILLIC SMALL LETTER DJE */
    { -1, "afii10100" },	    /* CYRILLIC SMALL LETTER GJE */
    { -1, "afii10101" },	    /* CYRILLIC SMALL LETTER UKRAINIAN IE */
    { -1, "afii10102" },	    /* CYRILLIC SMALL LETTER DZE */
    { -1, "afii10103" },	    /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    { -1, "afii10104" },	    /* CYRILLIC SMALL LETTER YI */
    { -1, "afii10105" },	    /* CYRILLIC SMALL LETTER JE */
    { -1, "afii10106" },	    /* CYRILLIC SMALL LETTER LJE */
    { -1, "afii10107" },	    /* CYRILLIC SMALL LETTER NJE */
    { -1, "afii10108" },	    /* CYRILLIC SMALL LETTER TSHE */
    { -1, "afii10109" },	    /* CYRILLIC SMALL LETTER KJE */
    { -1, "afii10110" },	    /* CYRILLIC SMALL LETTER SHORT U */
    { -1, "afii10145" },	    /* CYRILLIC CAPITAL LETTER DZHE */
    { -1, "afii10146" },	    /* CYRILLIC CAPITAL LETTER YAT */
    { -1, "afii10147" },	    /* CYRILLIC CAPITAL LETTER FITA */
    { -1, "afii10148" },	    /* CYRILLIC CAPITAL LETTER IZHITSA */
    { -1, "afii10192" },	    /* CYRILLIC SMALL LETTER DE VARIANT */
    { -1, "afii10193" },	    /* CYRILLIC SMALL LETTER DZHE */
    { -1, "afii10194" },	    /* CYRILLIC SMALL LETTER YAT */
    { -1, "afii10195" },	    /* CYRILLIC SMALL LETTER FITA */
    { -1, "afii10196" },	    /* CYRILLIC SMALL LETTER IZHITSA */
    { -1, "afii10831" },	    /* CYRILLIC SMALL LETTER PE VARIANT */
    { -1, "afii10832" },	    /* CYRILLIC SMALL LETTER TE VARIANT */
    { -1, "afii10846" },	    /* CYRILLIC SMALL LETTER SCHWA */
    { -1, "afii299" },		    /* LEFT-TO-RIGHT MARK */
    { -1, "afii300" },		    /* RIGHT-TO-LEFT MARK */
    { -1, "afii301" },		    /* ZERO WIDTH JOINER */
    { -1, "afii57381" },	    /* ARABIC PERCENT SIGN */
    { -1, "afii57388" },	    /* ARABIC COMMA */
    { -1, "afii57392" },	    /* ARABIC-INDIC DIGIT ZERO */
    { -1, "afii57393" },	    /* ARABIC-INDIC DIGIT ONE */
    { -1, "afii57394" },	    /* ARABIC-INDIC DIGIT TWO */
    { -1, "afii57395" },	    /* ARABIC-INDIC DIGIT THREE */
    { -1, "afii57396" },	    /* ARABIC-INDIC DIGIT FOUR */
    { -1, "afii57397" },	    /* ARABIC-INDIC DIGIT FIVE */
    { -1, "afii57398" },	    /* ARABIC-INDIC DIGIT SIX */
    { -1, "afii57399" },	    /* ARABIC-INDIC DIGIT SEVEN */
    { -1, "afii57400" },	    /* ARABIC-INDIC DIGIT EIGHT */
    { -1, "afii57401" },	    /* ARABIC-INDIC DIGIT NINE */
    { -1, "afii57403" },	    /* ARABIC SEMICOLON */
    { -1, "afii57407" },	    /* ARABIC QUESTION MARK */
    { -1, "afii57409" },	    /* ARABIC LETTER HAMZA */
    { -1, "afii57410" },	    /* ARABIC LETTER ALEF WITH MADDA ABOVE */
    { -1, "afii57411" },	    /* ARABIC LETTER ALEF WITH HAMZA ABOVE */
    { -1, "afii57412" },	    /* ARABIC LETTER WAW WITH HAMZA ABOVE */
    { -1, "afii57413" },	    /* ARABIC LETTER ALEF WITH HAMZA BELOW */
    { -1, "afii57414" },	    /* ARABIC LETTER YEH WITH HAMZA ABOVE */
    { -1, "afii57415" },	    /* ARABIC LETTER ALEF */
    { -1, "afii57416" },	    /* ARABIC LETTER BEH */
    { -1, "afii57417" },	    /* ARABIC LETTER TEH MARBUTA */
    { -1, "afii57418" },	    /* ARABIC LETTER TEH */
    { -1, "afii57419" },	    /* ARABIC LETTER THEH */
    { -1, "afii57420" },	    /* ARABIC LETTER JEEM */
    { -1, "afii57421" },	    /* ARABIC LETTER HAH */
    { -1, "afii57422" },	    /* ARABIC LETTER KHAH */
    { -1, "afii57423" },	    /* ARABIC LETTER DAL */
    { -1, "afii57424" },	    /* ARABIC LETTER THAL */
    { -1, "afii57425" },	    /* ARABIC LETTER REH */
    { -1, "afii57426" },	    /* ARABIC LETTER ZAIN */
    { -1, "afii57427" },	    /* ARABIC LETTER SEEN */
    { -1, "afii57428" },	    /* ARABIC LETTER SHEEN */
    { -1, "afii57429" },	    /* ARABIC LETTER SAD */
    { -1, "afii57430" },	    /* ARABIC LETTER DAD */
    { -1, "afii57431" },	    /* ARABIC LETTER TAH */
    { -1, "afii57432" },	    /* ARABIC LETTER ZAH */
    { -1, "afii57433" },	    /* ARABIC LETTER AIN */
    { -1, "afii57434" },	    /* ARABIC LETTER GHAIN */
    { -1, "afii57440" },	    /* ARABIC TATWEEL */
    { -1, "afii57441" },	    /* ARABIC LETTER FEH */
    { -1, "afii57442" },	    /* ARABIC LETTER QAF */
    { -1, "afii57443" },	    /* ARABIC LETTER KAF */
    { -1, "afii57444" },	    /* ARABIC LETTER LAM */
    { -1, "afii57445" },	    /* ARABIC LETTER MEEM */
    { -1, "afii57446" },	    /* ARABIC LETTER NOON */
    { -1, "afii57448" },	    /* ARABIC LETTER WAW */
    { -1, "afii57449" },	    /* ARABIC LETTER ALEF MAKSURA */
    { -1, "afii57450" },	    /* ARABIC LETTER YEH */
    { -1, "afii57451" },	    /* ARABIC FATHATAN */
    { -1, "afii57452" },	    /* ARABIC DAMMATAN */
    { -1, "afii57453" },	    /* ARABIC KASRATAN */
    { -1, "afii57454" },	    /* ARABIC FATHA */
    { -1, "afii57455" },	    /* ARABIC DAMMA */
    { -1, "afii57456" },	    /* ARABIC KASRA */
    { -1, "afii57457" },	    /* ARABIC SHADDA */
    { -1, "afii57458" },	    /* ARABIC SUKUN */
    { -1, "afii57470" },	    /* ARABIC LETTER HEH */
    { -1, "afii57505" },	    /* ARABIC LETTER VEH */
    { -1, "afii57506" },	    /* ARABIC LETTER PEH */
    { -1, "afii57507" },	    /* ARABIC LETTER TCHEH */
    { -1, "afii57508" },	    /* ARABIC LETTER JEH */
    { -1, "afii57509" },	    /* ARABIC LETTER GAF */
    { -1, "afii57511" },	    /* ARABIC LETTER TTEH */
    { -1, "afii57512" },	    /* ARABIC LETTER DDAL */
    { -1, "afii57513" },	    /* ARABIC LETTER RREH */
    { -1, "afii57514" },	    /* ARABIC LETTER NOON GHUNNA */
    { -1, "afii57519" },	    /* ARABIC LETTER YEH BARREE */
    { -1, "afii57534" },	    /* ARABIC LETTER AE */
    { -1, "afii57636" },	    /* NEW SHEQEL SIGN */
    { -1, "afii57645" },	    /* HEBREW PUNCTUATION MAQAF */
    { -1, "afii57658" },	    /* HEBREW PUNCTUATION SOF PASUQ */
    { -1, "afii57664" },	    /* HEBREW LETTER ALEF */
    { -1, "afii57665" },	    /* HEBREW LETTER BET */
    { -1, "afii57666" },	    /* HEBREW LETTER GIMEL */
    { -1, "afii57667" },	    /* HEBREW LETTER DALET */
    { -1, "afii57668" },	    /* HEBREW LETTER HE */
    { -1, "afii57669" },	    /* HEBREW LETTER VAV */
    { -1, "afii57670" },	    /* HEBREW LETTER ZAYIN */
    { -1, "afii57671" },	    /* HEBREW LETTER HET */
    { -1, "afii57672" },	    /* HEBREW LETTER TET */
    { -1, "afii57673" },	    /* HEBREW LETTER YOD */
    { -1, "afii57674" },	    /* HEBREW LETTER FINAL KAF */
    { -1, "afii57675" },	    /* HEBREW LETTER KAF */
    { -1, "afii57676" },	    /* HEBREW LETTER LAMED */
    { -1, "afii57677" },	    /* HEBREW LETTER FINAL MEM */
    { -1, "afii57678" },	    /* HEBREW LETTER MEM */
    { -1, "afii57679" },	    /* HEBREW LETTER FINAL NUN */
    { -1, "afii57680" },	    /* HEBREW LETTER NUN */
    { -1, "afii57681" },	    /* HEBREW LETTER SAMEKH */
    { -1, "afii57682" },	    /* HEBREW LETTER AYIN */
    { -1, "afii57683" },	    /* HEBREW LETTER FINAL PE */
    { -1, "afii57684" },	    /* HEBREW LETTER PE */
    { -1, "afii57685" },	    /* HEBREW LETTER FINAL TSADI */
    { -1, "afii57686" },	    /* HEBREW LETTER TSADI */
    { -1, "afii57687" },	    /* HEBREW LETTER QOF */
    { -1, "afii57688" },	    /* HEBREW LETTER RESH */
    { -1, "afii57689" },	    /* HEBREW LETTER SHIN */
    { -1, "afii57690" },	    /* HEBREW LETTER TAV */
    { -1, "afii57694" },	    /* HEBREW LETTER SHIN WITH SHIN DOT */
    { -1, "afii57695" },	    /* HEBREW LETTER SHIN WITH SIN DOT */
    { -1, "afii57700" },	    /* HEBREW LETTER VAV WITH HOLAM */
    { -1, "afii57705" },	    /* HEBREW LIGATURE YIDDISH YOD YOD PATAH */
    { -1, "afii57716" },	    /* HEBREW LIGATURE YIDDISH DOUBLE VAV */
    { -1, "afii57717" },	    /* HEBREW LIGATURE YIDDISH VAV YOD */
    { -1, "afii57718" },	    /* HEBREW LIGATURE YIDDISH DOUBLE YOD */
    { -1, "afii57723" },	    /* HEBREW LETTER VAV WITH DAGESH */
    { -1, "afii57793" },	    /* HEBREW POINT HIRIQ */
    { -1, "afii57794" },	    /* HEBREW POINT TSERE */
    { -1, "afii57795" },	    /* HEBREW POINT SEGOL */
    { -1, "afii57796" },	    /* HEBREW POINT QUBUTS */
    { -1, "afii57797" },	    /* HEBREW POINT QAMATS */
    { -1, "afii57798" },	    /* HEBREW POINT PATAH */
    { -1, "afii57799" },	    /* HEBREW POINT SHEVA */
    { -1, "afii57800" },	    /* HEBREW POINT HATAF PATAH */
    { -1, "afii57801" },	    /* HEBREW POINT HATAF SEGOL */
    { -1, "afii57802" },	    /* HEBREW POINT HATAF QAMATS */
    { -1, "afii57803" },	    /* HEBREW POINT SIN DOT */
    { -1, "afii57804" },	    /* HEBREW POINT SHIN DOT */
    { -1, "afii57806" },	    /* HEBREW POINT HOLAM */
    { -1, "afii57807" },	    /* HEBREW POINT DAGESH OR MAPIQ */
    { -1, "afii57839" },	    /* HEBREW POINT METEG */
    { -1, "afii57841" },	    /* HEBREW POINT RAFE */
    { -1, "afii57842" },	    /* HEBREW PUNCTUATION PASEQ */
    { -1, "afii57929" },	    /* MODIFIER LETTER APOSTROPHE */
    { -1, "afii61248" },	    /* CARE OF */
    { -1, "afii61289" },	    /* SCRIPT SMALL L */
    { -1, "afii61352" },	    /* NUMERO SIGN */
    { -1, "afii61573" },	    /* POP DIRECTIONAL FORMATTING */
    { -1, "afii61574" },	    /* LEFT-TO-RIGHT OVERRIDE */
    { -1, "afii61575" },	    /* RIGHT-TO-LEFT OVERRIDE */
    { -1, "afii61664" },	    /* ZERO WIDTH NON-JOINER */
    { -1, "afii63167" },	    /* ARABIC FIVE POINTED STAR */
    { -1, "afii64937" },	    /* MODIFIER LETTER REVERSED COMMA */
    { -1, "agrave" },		    /* LATIN SMALL LETTER A WITH GRAVE */
    { -1, "aleph" },		    /* ALEF SYMBOL */
    { -1, "alpha" },		    /* GREEK SMALL LETTER ALPHA */
    { -1, "alphatonos" },	    /* GREEK SMALL LETTER ALPHA WITH TONOS */
    { -1, "amacron" },		    /* LATIN SMALL LETTER A WITH MACRON */
    { -1, "ampersand" },	    /* AMPERSAND */
    { -1, "ampersandsmall" },	    /* SMALL CAPITAL AMPERSAND */
    { -1, "angle" },		    /* ANGLE */
    { -1, "angleleft" },	    /* LEFT-POINTING ANGLE BRACKET */
    { -1, "angleright" },	    /* RIGHT-POINTING ANGLE BRACKET */
    { -1, "anoteleia" },	    /* GREEK ANO TELEIA */
    { -1, "aogonek" },		    /* LATIN SMALL LETTER A WITH OGONEK */
    { -1, "apple" },		    /* FONT FAMILY;Symbol */
    { -1, "approxequal" },	    /* ALMOST EQUAL TO */
    { -1, "aring" },		    /* LATIN SMALL LETTER A WITH RING ABOVE */
    { -1, "aringacute" },	    /* LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE */
    { -1, "arrowboth" },	    /* LEFT RIGHT ARROW */
    { -1, "arrowdblboth" },	    /* LEFT RIGHT DOUBLE ARROW */
    { -1, "arrowdbldown" },	    /* DOWNWARDS DOUBLE ARROW */
    { -1, "arrowdblleft" },	    /* LEFTWARDS DOUBLE ARROW */
    { -1, "arrowdblright" },	    /* RIGHTWARDS DOUBLE ARROW */
    { -1, "arrowdblup" },	    /* UPWARDS DOUBLE ARROW */
    { -1, "arrowdown" },	    /* DOWNWARDS ARROW */
    { -1, "arrowhorizex" },	    /* HORIZONTAL ARROW EXTENDER */
    { -1, "arrowleft" },	    /* LEFTWARDS ARROW */
    { -1, "arrowright" },	    /* RIGHTWARDS ARROW */
    { -1, "arrowup" },		    /* UPWARDS ARROW */
    { -1, "arrowupdn" },	    /* UP DOWN ARROW */
    { -1, "arrowupdnbse" },	    /* UP DOWN ARROW WITH BASE */
    { -1, "arrowvertex" },	    /* VERTICAL ARROW EXTENDER */
    { -1, "asciicircum" },	    /* CIRCUMFLEX ACCENT */
    { -1, "asciitilde" },	    /* TILDE */
    { -1, "asterisk" },		    /* ASTERISK */
    { -1, "asteriskmath" },	    /* ASTERISK OPERATOR */
    { -1, "asuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER A */
    { -1, "at" },		    /* COMMERCIAL AT */
    { -1, "atilde" },		    /* LATIN SMALL LETTER A WITH TILDE */
    { -1, "b" },		    /* LATIN SMALL LETTER B */
    { -1, "backslash" },	    /* REVERSE SOLIDUS */
    { -1, "bar" },		    /* VERTICAL LINE */
    { -1, "beta" },		    /* GREEK SMALL LETTER BETA */
    { -1, "block" },		    /* FULL BLOCK */
    { -1, "braceex" },		    /* CURLY BRACKET EXTENDER */
    { -1, "braceleft" },	    /* LEFT CURLY BRACKET */
    { -1, "braceleftbt" },	    /* LEFT CURLY BRACKET BOTTOM */
    { -1, "braceleftmid" },	    /* LEFT CURLY BRACKET MID */
    { -1, "bracelefttp" },	    /* LEFT CURLY BRACKET TOP */
    { -1, "braceright" },	    /* RIGHT CURLY BRACKET */
    { -1, "bracerightbt" },	    /* RIGHT CURLY BRACKET BOTTOM */
    { -1, "bracerightmid" },	    /* RIGHT CURLY BRACKET MID */
    { -1, "bracerighttp" },	    /* RIGHT CURLY BRACKET TOP */
    { -1, "bracketleft" },	    /* LEFT SQUARE BRACKET */
    { -1, "bracketleftbt" },	    /* LEFT SQUARE BRACKET BOTTOM */
    { -1, "bracketleftex" },	    /* LEFT SQUARE BRACKET EXTENDER */
    { -1, "bracketlefttp" },	    /* LEFT SQUARE BRACKET TOP */
    { -1, "bracketright" },	    /* RIGHT SQUARE BRACKET */
    { -1, "bracketrightbt" },	    /* RIGHT SQUARE BRACKET BOTTOM */
    { -1, "bracketrightex" },	    /* RIGHT SQUARE BRACKET EXTENDER */
    { -1, "bracketrighttp" },	    /* RIGHT SQUARE BRACKET TOP */
    { -1, "breve" },		    /* BREVE */
    { -1, "brokenbar" },	    /* BROKEN BAR */
    { -1, "bsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER B */
    { -1, "bullet" },		    /* BULLET */
    { -1, "c" },		    /* LATIN SMALL LETTER C */
    { -1, "cacute" },		    /* LATIN SMALL LETTER C WITH ACUTE */
    { -1, "caron" },		    /* CARON */
    { -1, "carriagereturn" },	    /* DOWNWARDS ARROW WITH CORNER LEFTWARDS */
    { -1, "ccaron" },		    /* LATIN SMALL LETTER C WITH CARON */
    { -1, "ccedilla" },		    /* LATIN SMALL LETTER C WITH CEDILLA */
    { -1, "ccircumflex" },	    /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
    { -1, "cdotaccent" },	    /* LATIN SMALL LETTER C WITH DOT ABOVE */
    { -1, "cedilla" },		    /* CEDILLA */
    { -1, "cent" },		    /* CENT SIGN */
    { -1, "center" },		    /* FONT FAMILY;Courier */
    { -1, "centinferior" },	    /* SUBSCRIPT CENT SIGN */
    { -1, "centoldstyle" },	    /* OLDSTYLE CENT SIGN */
    { -1, "centsuperior" },	    /* SUPERSCRIPT CENT SIGN */
    { -1, "chi" },		    /* GREEK SMALL LETTER CHI */
    { -1, "circle" },		    /* WHITE CIRCLE */
    { -1, "circlemultiply" },	    /* CIRCLED TIMES */
    { -1, "circleplus" },	    /* CIRCLED PLUS */
    { -1, "circumflex" },	    /* MODIFIER LETTER CIRCUMFLEX ACCENT */
    { -1, "club" },		    /* BLACK CLUB SUIT */
    { -1, "colon" },		    /* COLON */
    { -1, "colonmonetary" },	    /* COLON SIGN */
    { -1, "comma" },		    /* COMMA */
    { -1, "commaaccent" },	    /* COMMA BELOW */
    { -1, "commainferior" },	    /* SUBSCRIPT COMMA */
    { -1, "commasuperior" },	    /* SUPERSCRIPT COMMA */
    { -1, "congruent" },	    /* APPROXIMATELY EQUAL TO */
    { -1, "copyright" },	    /* COPYRIGHT SIGN */
    { -1, "copyrightsans" },	    /* COPYRIGHT SIGN SANS SERIF */
    { -1, "copyrightserif" },	    /* COPYRIGHT SIGN SERIF */
    { -1, "currency" },		    /* CURRENCY SIGN */
    { -1, "cyrBreve" },		    /* CAPITAL CYRILLIC BREVE */
    { -1, "cyrFlex" },		    /* CAPITAL CYRILLIC CIRCUMFLEX */
    { -1, "cyrbreve" },		    /* CYRILLIC BREVE */
    { -1, "cyrflex" },		    /* CYRILLIC CIRCUMFLEX */
    { -1, "d" },		    /* LATIN SMALL LETTER D */
    { -1, "dagger" },		    /* DAGGER */
    { -1, "daggerdbl" },	    /* DOUBLE DAGGER */
    { -1, "dblGrave" },		    /* CAPITAL DOUBLE GRAVE ACCENT */
    { -1, "dblgrave" },		    /* DOUBLE GRAVE ACCENT */
    { -1, "dcaron" },		    /* LATIN SMALL LETTER D WITH CARON */
    { -1, "dcroat" },		    /* LATIN SMALL LETTER D WITH STROKE */
    { -1, "dectab" },		    /* FONT FAMILY;Courier */
    { -1, "degree" },		    /* DEGREE SIGN */
    { -1, "delta" },		    /* GREEK SMALL LETTER DELTA */
    { -1, "diamond" },		    /* BLACK DIAMOND SUIT */
    { -1, "dieresis" },		    /* DIAERESIS */
    { -1, "dieresisacute" },	    /* DIAERESIS ACUTE ACCENT */
    { -1, "dieresisgrave" },	    /* DIAERESIS GRAVE ACCENT */
    { -1, "dieresistonos" },	    /* GREEK DIALYTIKA TONOS */
    { -1, "divide" },		    /* DIVISION SIGN */
    { -1, "dkshade" },		    /* DARK SHADE */
    { -1, "dnblock" },		    /* LOWER HALF BLOCK */
    { -1, "dollar" },		    /* DOLLAR SIGN */
    { -1, "dollarinferior" },	    /* SUBSCRIPT DOLLAR SIGN */
    { -1, "dollaroldstyle" },	    /* OLDSTYLE DOLLAR SIGN */
    { -1, "dollarsuperior" },	    /* SUPERSCRIPT DOLLAR SIGN */
    { -1, "dong" },		    /* DONG SIGN */
    { -1, "dotaccent" },	    /* DOT ABOVE */
    { -1, "dotbelowcomb" },	    /* COMBINING DOT BELOW */
    { -1, "dotlessi" },		    /* LATIN SMALL LETTER DOTLESS I */
    { -1, "dotlessj" },		    /* LATIN SMALL LETTER DOTLESS J */
    { -1, "dotmath" },		    /* DOT OPERATOR */
    { -1, "down" },		    /* FONT FAMILY;Courier */
    { -1, "dsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER D */
    { -1, "e" },		    /* LATIN SMALL LETTER E */
    { -1, "eacute" },		    /* LATIN SMALL LETTER E WITH ACUTE */
    { -1, "ebreve" },		    /* LATIN SMALL LETTER E WITH BREVE */
    { -1, "ecaron" },		    /* LATIN SMALL LETTER E WITH CARON */
    { -1, "ecircumflex" },	    /* LATIN SMALL LETTER E WITH CIRCUMFLEX */
    { -1, "edieresis" },	    /* LATIN SMALL LETTER E WITH DIAERESIS */
    { -1, "edotaccent" },	    /* LATIN SMALL LETTER E WITH DOT ABOVE */
    { -1, "egrave" },		    /* LATIN SMALL LETTER E WITH GRAVE */
    { -1, "eight" },		    /* DIGIT EIGHT */
    { -1, "eightinferior" },	    /* SUBSCRIPT EIGHT */
    { -1, "eightoldstyle" },	    /* OLDSTYLE DIGIT EIGHT */
    { -1, "eightsuperior" },	    /* SUPERSCRIPT EIGHT */
    { -1, "element" },		    /* ELEMENT OF */
    { -1, "ellipsis" },		    /* HORIZONTAL ELLIPSIS */
    { -1, "emacron" },		    /* LATIN SMALL LETTER E WITH MACRON */
    { -1, "emdash" },		    /* EM DASH */
    { -1, "emptyset" },		    /* EMPTY SET */
    { -1, "endash" },		    /* EN DASH */
    { -1, "eng" },		    /* LATIN SMALL LETTER ENG */
    { -1, "eogonek" },		    /* LATIN SMALL LETTER E WITH OGONEK */
    { -1, "epsilon" },		    /* GREEK SMALL LETTER EPSILON */
    { -1, "epsilontonos" },	    /* GREEK SMALL LETTER EPSILON WITH TONOS */
    { -1, "equal" },		    /* EQUALS SIGN */
    { -1, "equivalence" },	    /* IDENTICAL TO */
    { -1, "estimated" },	    /* ESTIMATED SYMBOL */
    { -1, "esuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER E */
    { -1, "eta" },		    /* GREEK SMALL LETTER ETA */
    { -1, "etatonos" },		    /* GREEK SMALL LETTER ETA WITH TONOS */
    { -1, "eth" },		    /* LATIN SMALL LETTER ETH */
    { -1, "exclam" },		    /* EXCLAMATION MARK */
    { -1, "exclamdbl" },	    /* DOUBLE EXCLAMATION MARK */
    { -1, "exclamdown" },	    /* INVERTED EXCLAMATION MARK */
    { -1, "exclamdownsmall" },	    /* SMALL CAPITAL INVERTED EXCLAMATION MARK */
    { -1, "exclamsmall" },	    /* SMALL CAPITAL EXCLAMATION MARK */
    { -1, "existential" },	    /* THERE EXISTS */
    { -1, "f" },		    /* LATIN SMALL LETTER F */
    { -1, "female" },		    /* FEMALE SIGN */
    { -1, "ff" },		    /* LATIN SMALL LIGATURE FF */
    { -1, "ffi" },		    /* LATIN SMALL LIGATURE FFI */
    { -1, "ffl" },		    /* LATIN SMALL LIGATURE FFL */
    { -1, "fi" },		    /* LATIN SMALL LIGATURE FI */
    { -1, "figuredash" },	    /* FIGURE DASH */
    { -1, "filledbox" },	    /* BLACK SQUARE */
    { -1, "filledrect" },	    /* BLACK RECTANGLE */
    { -1, "five" },		    /* DIGIT FIVE */
    { -1, "fiveeighths" },	    /* VULGAR FRACTION FIVE EIGHTHS */
    { -1, "fiveinferior" },	    /* SUBSCRIPT FIVE */
    { -1, "fiveoldstyle" },	    /* OLDSTYLE DIGIT FIVE */
    { -1, "fivesuperior" },	    /* SUPERSCRIPT FIVE */
    { -1, "fl" },		    /* LATIN SMALL LIGATURE FL */
    { -1, "florin" },		    /* LATIN SMALL LETTER F WITH HOOK */
    { -1, "format" },		    /* FONT FAMILY;Courier */
    { -1, "four" },		    /* DIGIT FOUR */
    { -1, "fourinferior" },	    /* SUBSCRIPT FOUR */
    { -1, "fouroldstyle" },	    /* OLDSTYLE DIGIT FOUR */
    { -1, "foursuperior" },	    /* SUPERSCRIPT FOUR */
    { -1, "fraction" },		    /* FRACTION SLASH */
				    /* DIVISION SLASH;Duplicate */
    { -1, "franc" },		    /* FRENCH FRANC SIGN */
    { -1, "g" },		    /* LATIN SMALL LETTER G */
    { -1, "gamma" },		    /* GREEK SMALL LETTER GAMMA */
    { -1, "gbreve" },		    /* LATIN SMALL LETTER G WITH BREVE */
    { -1, "gcaron" },		    /* LATIN SMALL LETTER G WITH CARON */
    { -1, "gcircumflex" },	    /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
    { -1, "gcommaaccent" },	    /* LATIN SMALL LETTER G WITH CEDILLA */
    { -1, "gdotaccent" },	    /* LATIN SMALL LETTER G WITH DOT ABOVE */
    { -1, "germandbls" },	    /* LATIN SMALL LETTER SHARP S */
    { -1, "gradient" },		    /* NABLA */
    { -1, "grave" },		    /* GRAVE ACCENT */
    { -1, "gravecomb" },	    /* COMBINING GRAVE ACCENT */
    { -1, "graybox" },		    /* FONT FAMILY;Courier */
    { -1, "greater" },		    /* GREATER-THAN SIGN */
    { -1, "greaterequal" },	    /* GREATER-THAN OR EQUAL TO */
    { -1, "guillemotleft" },	    /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
    { -1, "guillemotright" },	    /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
    { -1, "guilsinglleft" },	    /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
    { -1, "guilsinglright" },	    /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
    { -1, "h" },		    /* LATIN SMALL LETTER H */
    { -1, "hbar" },		    /* LATIN SMALL LETTER H WITH STROKE */
    { -1, "hcircumflex" },	    /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
    { -1, "heart" },		    /* BLACK HEART SUIT */
    { -1, "hookabovecomb" },	    /* COMBINING HOOK ABOVE */
    { -1, "house" },		    /* HOUSE */
    { -1, "hungarumlaut" },	    /* DOUBLE ACUTE ACCENT */
    { -1, "hyphen" },		    /* HYPHEN-MINUS */
				    /* SOFT HYPHEN;Duplicate */
    { -1, "hypheninferior" },	    /* SUBSCRIPT HYPHEN-MINUS */
    { -1, "hyphensuperior" },	    /* SUPERSCRIPT HYPHEN-MINUS */
    { -1, "i" },		    /* LATIN SMALL LETTER I */
    { -1, "iacute" },		    /* LATIN SMALL LETTER I WITH ACUTE */
    { -1, "ibreve" },		    /* LATIN SMALL LETTER I WITH BREVE */
    { -1, "icircumflex" },	    /* LATIN SMALL LETTER I WITH CIRCUMFLEX */
    { -1, "idieresis" },	    /* LATIN SMALL LETTER I WITH DIAERESIS */
    { -1, "igrave" },		    /* LATIN SMALL LETTER I WITH GRAVE */
    { -1, "ij" },		    /* LATIN SMALL LIGATURE IJ */
    { -1, "imacron" },		    /* LATIN SMALL LETTER I WITH MACRON */
    { -1, "indent" },		    /* FONT FAMILY;Courier */
    { -1, "infinity" },		    /* INFINITY */
    { -1, "integral" },		    /* INTEGRAL */
    { -1, "integralbt" },	    /* BOTTOM HALF INTEGRAL */
    { -1, "integralex" },	    /* INTEGRAL EXTENDER */
    { -1, "integraltp" },	    /* TOP HALF INTEGRAL */
    { -1, "intersection" },	    /* INTERSECTION */
    { -1, "invbullet" },	    /* INVERSE BULLET */
    { -1, "invcircle" },	    /* INVERSE WHITE CIRCLE */
    { -1, "invsmileface" },	    /* BLACK SMILING FACE */
    { -1, "iogonek" },		    /* LATIN SMALL LETTER I WITH OGONEK */
    { -1, "iota" },		    /* GREEK SMALL LETTER IOTA */
    { -1, "iotadieresis" },	    /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    { -1, "iotadieresistonos" },    /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    { -1, "iotatonos" },	    /* GREEK SMALL LETTER IOTA WITH TONOS */
    { -1, "isuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER I */
    { -1, "itilde" },		    /* LATIN SMALL LETTER I WITH TILDE */
    { -1, "j" },		    /* LATIN SMALL LETTER J */
    { -1, "jcircumflex" },	    /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
    { -1, "k" },		    /* LATIN SMALL LETTER K */
    { -1, "kappa" },		    /* GREEK SMALL LETTER KAPPA */
    { -1, "kcommaaccent" },	    /* LATIN SMALL LETTER K WITH CEDILLA */
    { -1, "kgreenlandic" },	    /* LATIN SMALL LETTER KRA */
    { -1, "l" },		    /* LATIN SMALL LETTER L */
    { -1, "lacute" },		    /* LATIN SMALL LETTER L WITH ACUTE */
    { -1, "lambda" },		    /* GREEK SMALL LETTER LAMDA */
    { -1, "largebullet" },	    /* FONT FAMILY;Courier */
    { -1, "lcaron" },		    /* LATIN SMALL LETTER L WITH CARON */
    { -1, "lcommaaccent" },	    /* LATIN SMALL LETTER L WITH CEDILLA */
    { -1, "ldot" },		    /* LATIN SMALL LETTER L WITH MIDDLE DOT */
    { -1, "left" },		    /* FONT FAMILY;Courier */
    { -1, "less" },		    /* LESS-THAN SIGN */
    { -1, "lessequal" },	    /* LESS-THAN OR EQUAL TO */
    { -1, "lfblock" },		    /* LEFT HALF BLOCK */
    { -1, "lira" },		    /* LIRA SIGN */
    { -1, "ll" },		    /* LATIN SMALL LETTER LL */
    { -1, "logicaland" },	    /* LOGICAL AND */
    { -1, "logicalnot" },	    /* NOT SIGN */
    { -1, "logicalor" },	    /* LOGICAL OR */
    { -1, "longs" },		    /* LATIN SMALL LETTER LONG S */
    { -1, "lozenge" },		    /* LOZENGE */
    { -1, "lslash" },		    /* LATIN SMALL LETTER L WITH STROKE */
    { -1, "lsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER L */
    { -1, "ltshade" },		    /* LIGHT SHADE */
    { -1, "m" },		    /* LATIN SMALL LETTER M */
    { -1, "macron" },		    /* MACRON */
				    /* MODIFIER LETTER MACRON;Duplicate */
    { -1, "male" },		    /* MALE SIGN */
    { -1, "merge" },		    /* FONT FAMILY;Courier */
    { -1, "minus" },		    /* MINUS SIGN */
    { -1, "minute" },		    /* PRIME */
    { -1, "msuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER M */
    { -1, "mu" },		    /* MICRO SIGN */
				    /* GREEK SMALL LETTER MU;Duplicate */
    { -1, "multiply" },		    /* MULTIPLICATION SIGN */
    { -1, "musicalnote" },	    /* EIGHTH NOTE */
    { -1, "musicalnotedbl" },	    /* BEAMED EIGHTH NOTES */
    { -1, "n" },		    /* LATIN SMALL LETTER N */
    { -1, "nacute" },		    /* LATIN SMALL LETTER N WITH ACUTE */
    { -1, "napostrophe" },	    /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    { -1, "ncaron" },		    /* LATIN SMALL LETTER N WITH CARON */
    { -1, "ncommaaccent" },	    /* LATIN SMALL LETTER N WITH CEDILLA */
    { -1, "nine" },		    /* DIGIT NINE */
    { -1, "nineinferior" },	    /* SUBSCRIPT NINE */
    { -1, "nineoldstyle" },	    /* OLDSTYLE DIGIT NINE */
    { -1, "ninesuperior" },	    /* SUPERSCRIPT NINE */
    { -1, "notegraphic" },	    /* FONT FAMILY;Courier */
    { -1, "notelement" },	    /* NOT AN ELEMENT OF */
    { -1, "notequal" },		    /* NOT EQUAL TO */
    { -1, "notsubset" },	    /* NOT A SUBSET OF */
    { -1, "nsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER N */
    { -1, "ntilde" },		    /* LATIN SMALL LETTER N WITH TILDE */
    { -1, "nu" },		    /* GREEK SMALL LETTER NU */
    { -1, "numbersign" },	    /* NUMBER SIGN */
    { -1, "o" },		    /* LATIN SMALL LETTER O */
    { -1, "oacute" },		    /* LATIN SMALL LETTER O WITH ACUTE */
    { -1, "obreve" },		    /* LATIN SMALL LETTER O WITH BREVE */
    { -1, "ocircumflex" },	    /* LATIN SMALL LETTER O WITH CIRCUMFLEX */
    { -1, "odieresis" },	    /* LATIN SMALL LETTER O WITH DIAERESIS */
    { -1, "oe" },		    /* LATIN SMALL LIGATURE OE */
    { -1, "ogonek" },		    /* OGONEK */
    { -1, "ograve" },		    /* LATIN SMALL LETTER O WITH GRAVE */
    { -1, "ohorn" },		    /* LATIN SMALL LETTER O WITH HORN */
    { -1, "ohungarumlaut" },	    /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    { -1, "omacron" },		    /* LATIN SMALL LETTER O WITH MACRON */
    { -1, "omega" },		    /* GREEK SMALL LETTER OMEGA */
    { -1, "omega1" },		    /* GREEK PI SYMBOL */
    { -1, "omegatonos" },	    /* GREEK SMALL LETTER OMEGA WITH TONOS */
    { -1, "omicron" },		    /* GREEK SMALL LETTER OMICRON */
    { -1, "omicrontonos" },	    /* GREEK SMALL LETTER OMICRON WITH TONOS */
    { -1, "one" },		    /* DIGIT ONE */
    { -1, "onedotenleader" },	    /* ONE DOT LEADER */
    { -1, "oneeighth" },	    /* VULGAR FRACTION ONE EIGHTH */
    { -1, "onefitted" },	    /* PROPORTIONAL DIGIT ONE */
    { -1, "onehalf" },		    /* VULGAR FRACTION ONE HALF */
    { -1, "oneinferior" },	    /* SUBSCRIPT ONE */
    { -1, "oneoldstyle" },	    /* OLDSTYLE DIGIT ONE */
    { -1, "onequarter" },	    /* VULGAR FRACTION ONE QUARTER */
    { -1, "onesuperior" },	    /* SUPERSCRIPT ONE */
    { -1, "onethird" },		    /* VULGAR FRACTION ONE THIRD */
    { -1, "openbullet" },	    /* WHITE BULLET */
    { -1, "ordfeminine" },	    /* FEMININE ORDINAL INDICATOR */
    { -1, "ordmasculine" },	    /* MASCULINE ORDINAL INDICATOR */
    { -1, "orthogonal" },	    /* RIGHT ANGLE */
    { -1, "oslash" },		    /* LATIN SMALL LETTER O WITH STROKE */
    { -1, "oslashacute" },	    /* LATIN SMALL LETTER O WITH STROKE AND ACUTE */
    { -1, "osuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER O */
    { -1, "otilde" },		    /* LATIN SMALL LETTER O WITH TILDE */
    { -1, "overscore" },	    /* FONT FAMILY;Courier */
    { -1, "p" },		    /* LATIN SMALL LETTER P */
    { -1, "paragraph" },	    /* PILCROW SIGN */
    { -1, "parenleft" },	    /* LEFT PARENTHESIS */
    { -1, "parenleftbt" },	    /* LEFT PAREN BOTTOM */
    { -1, "parenleftex" },	    /* LEFT PAREN EXTENDER */
    { -1, "parenleftinferior" },    /* SUBSCRIPT LEFT PARENTHESIS */
    { -1, "parenleftsuperior" },    /* SUPERSCRIPT LEFT PARENTHESIS */
    { -1, "parenlefttp" },	    /* LEFT PAREN TOP */
    { -1, "parenright" },	    /* RIGHT PARENTHESIS */
    { -1, "parenrightbt" },	    /* RIGHT PAREN BOTTOM */
    { -1, "parenrightex" },	    /* RIGHT PAREN EXTENDER */
    { -1, "parenrightinferior" },   /* SUBSCRIPT RIGHT PARENTHESIS */
    { -1, "parenrightsuperior" },   /* SUPERSCRIPT RIGHT PARENTHESIS */
    { -1, "parenrighttp" },	    /* RIGHT PAREN TOP */
    { -1, "partialdiff" },	    /* PARTIAL DIFFERENTIAL */
    { -1, "percent" },		    /* PERCENT SIGN */
    { -1, "period" },		    /* FULL STOP */
    { -1, "periodcentered" },	    /* MIDDLE DOT */
				    /* BULLET OPERATOR;Duplicate */
    { -1, "periodinferior" },	    /* SUBSCRIPT FULL STOP */
    { -1, "periodsuperior" },	    /* SUPERSCRIPT FULL STOP */
    { -1, "perpendicular" },	    /* UP TACK */
    { -1, "perthousand" },	    /* PER MILLE SIGN */
    { -1, "peseta" },		    /* PESETA SIGN */
    { -1, "phi" },		    /* GREEK SMALL LETTER PHI */
    { -1, "phi1" },		    /* GREEK PHI SYMBOL */
    { -1, "pi" },		    /* GREEK SMALL LETTER PI */
    { -1, "plus" },		    /* PLUS SIGN */
    { -1, "plusminus" },	    /* PLUS-MINUS SIGN */
    { -1, "prescription" },	    /* PRESCRIPTION TAKE */
    { -1, "product" },		    /* N-ARY PRODUCT */
    { -1, "propersubset" },	    /* SUBSET OF */
    { -1, "propersuperset" },	    /* SUPERSET OF */
    { -1, "proportional" },	    /* PROPORTIONAL TO */
    { -1, "psi" },		    /* GREEK SMALL LETTER PSI */
    { -1, "q" },		    /* LATIN SMALL LETTER Q */
    { -1, "question" },		    /* QUESTION MARK */
    { -1, "questiondown" },	    /* INVERTED QUESTION MARK */
    { -1, "questiondownsmall" },    /* SMALL CAPITAL INVERTED QUESTION MARK */
    { -1, "questionsmall" },	    /* SMALL CAPITAL QUESTION MARK */
    { -1, "quotedbl" },		    /* QUOTATION MARK */
    { -1, "quotedblbase" },	    /* DOUBLE LOW-9 QUOTATION MARK */
    { -1, "quotedblleft" },	    /* LEFT DOUBLE QUOTATION MARK */
    { -1, "quotedblright" },	    /* RIGHT DOUBLE QUOTATION MARK */
    { -1, "quoteleft" },	    /* LEFT SINGLE QUOTATION MARK */
    { -1, "quotereversed" },	    /* SINGLE HIGH-REVERSED-9 QUOTATION MARK */
    { -1, "quoteright" },	    /* RIGHT SINGLE QUOTATION MARK */
    { -1, "quotesinglbase" },	    /* SINGLE LOW-9 QUOTATION MARK */
    { -1, "quotesingle" },	    /* APOSTROPHE */
    { -1, "r" },		    /* LATIN SMALL LETTER R */
    { -1, "racute" },		    /* LATIN SMALL LETTER R WITH ACUTE */
    { -1, "radical" },		    /* SQUARE ROOT */
    { -1, "radicalex" },	    /* RADICAL EXTENDER */
    { -1, "rcaron" },		    /* LATIN SMALL LETTER R WITH CARON */
    { -1, "rcommaaccent" },	    /* LATIN SMALL LETTER R WITH CEDILLA */
    { -1, "reflexsubset" },	    /* SUBSET OF OR EQUAL TO */
    { -1, "reflexsuperset" },	    /* SUPERSET OF OR EQUAL TO */
    { -1, "registered" },	    /* REGISTERED SIGN */
    { -1, "registersans" },	    /* REGISTERED SIGN SANS SERIF */
    { -1, "registerserif" },	    /* REGISTERED SIGN SERIF */
    { -1, "return" },		    /* FONT FAMILY;Courier */
    { -1, "revlogicalnot" },	    /* REVERSED NOT SIGN */
    { -1, "rho" },		    /* GREEK SMALL LETTER RHO */
    { -1, "ring" },		    /* RING ABOVE */
    { -1, "rsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER R */
    { -1, "rtblock" },		    /* RIGHT HALF BLOCK */
    { -1, "rupiah" },		    /* RUPIAH SIGN */
    { -1, "s" },		    /* LATIN SMALL LETTER S */
    { -1, "sacute" },		    /* LATIN SMALL LETTER S WITH ACUTE */
    { -1, "scaron" },		    /* LATIN SMALL LETTER S WITH CARON */
    { -1, "scedilla" },		    /* LATIN SMALL LETTER S WITH CEDILLA */
				    /* LATIN SMALL LETTER S WITH CEDILLA;Duplicate */
    { -1, "scircumflex" },	    /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
    { -1, "scommaaccent" },	    /* LATIN SMALL LETTER S WITH COMMA BELOW */
    { -1, "second" },		    /* DOUBLE PRIME */
    { -1, "section" },		    /* SECTION SIGN */
    { -1, "semicolon" },	    /* SEMICOLON */
    { -1, "seven" },		    /* DIGIT SEVEN */
    { -1, "seveneighths" },	    /* VULGAR FRACTION SEVEN EIGHTHS */
    { -1, "seveninferior" },	    /* SUBSCRIPT SEVEN */
    { -1, "sevenoldstyle" },	    /* OLDSTYLE DIGIT SEVEN */
    { -1, "sevensuperior" },	    /* SUPERSCRIPT SEVEN */
    { -1, "shade" },		    /* MEDIUM SHADE */
    { -1, "sigma" },		    /* GREEK SMALL LETTER SIGMA */
    { -1, "sigma1" },		    /* GREEK SMALL LETTER FINAL SIGMA */
    { -1, "similar" },		    /* TILDE OPERATOR */
    { -1, "six" },		    /* DIGIT SIX */
    { -1, "sixinferior" },	    /* SUBSCRIPT SIX */
    { -1, "sixoldstyle" },	    /* OLDSTYLE DIGIT SIX */
    { -1, "sixsuperior" },	    /* SUPERSCRIPT SIX */
    { -1, "slash" },		    /* SOLIDUS */
    { -1, "smileface" },	    /* WHITE SMILING FACE */
    { -1, "space" },		    /* SPACE */
				    /* NO-BREAK SPACE;Duplicate */
    { -1, "spade" },		    /* BLACK SPADE SUIT */
    { -1, "square" },		    /* FONT FAMILY;Courier */
    { -1, "ssuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER S */
    { -1, "sterling" },		    /* POUND SIGN */
    { -1, "stop" },		    /* FONT FAMILY;Courier */
    { -1, "suchthat" },		    /* CONTAINS AS MEMBER */
    { -1, "summation" },	    /* N-ARY SUMMATION */
    { -1, "sun" },		    /* WHITE SUN WITH RAYS */
    { -1, "t" },		    /* LATIN SMALL LETTER T */
    { -1, "tab" },		    /* FONT FAMILY;Courier */
    { -1, "tau" },		    /* GREEK SMALL LETTER TAU */
    { -1, "tbar" },		    /* LATIN SMALL LETTER T WITH STROKE */
    { -1, "tcaron" },		    /* LATIN SMALL LETTER T WITH CARON */
    { -1, "tcommaaccent" },	    /* LATIN SMALL LETTER T WITH CEDILLA */
				    /* LATIN SMALL LETTER T WITH COMMA BELOW;Duplicate */
    { -1, "therefore" },	    /* THEREFORE */
    { -1, "theta" },		    /* GREEK SMALL LETTER THETA */
    { -1, "theta1" },		    /* GREEK THETA SYMBOL */
    { -1, "thorn" },		    /* LATIN SMALL LETTER THORN */
    { -1, "three" },		    /* DIGIT THREE */
    { -1, "threeeighths" },	    /* VULGAR FRACTION THREE EIGHTHS */
    { -1, "threeinferior" },	    /* SUBSCRIPT THREE */
    { -1, "threeoldstyle" },	    /* OLDSTYLE DIGIT THREE */
    { -1, "threequarters" },	    /* VULGAR FRACTION THREE QUARTERS */
    { -1, "threequartersemdash" },  /* THREE QUARTERS EM DASH */
    { -1, "threesuperior" },	    /* SUPERSCRIPT THREE */
    { -1, "tilde" },		    /* SMALL TILDE */
    { -1, "tildecomb" },	    /* COMBINING TILDE */
    { -1, "tonos" },		    /* GREEK TONOS */
    { -1, "trademark" },	    /* TRADE MARK SIGN */
    { -1, "trademarksans" },	    /* TRADE MARK SIGN SANS SERIF */
    { -1, "trademarkserif" },	    /* TRADE MARK SIGN SERIF */
    { -1, "triagdn" },		    /* BLACK DOWN-POINTING TRIANGLE */
    { -1, "triaglf" },		    /* BLACK LEFT-POINTING POINTER */
    { -1, "triagrt" },		    /* BLACK RIGHT-POINTING POINTER */
    { -1, "triagup" },		    /* BLACK UP-POINTING TRIANGLE */
    { -1, "tsuperior" },	    /* SUPERSCRIPT LATIN SMALL LETTER T */
    { -1, "two" },		    /* DIGIT TWO */
    { -1, "twodotenleader" },	    /* TWO DOT LEADER */
    { -1, "twoinferior" },	    /* SUBSCRIPT TWO */
    { -1, "twooldstyle" },	    /* OLDSTYLE DIGIT TWO */
    { -1, "twosuperior" },	    /* SUPERSCRIPT TWO */
    { -1, "twothirds" },	    /* VULGAR FRACTION TWO THIRDS */
    { -1, "u" },		    /* LATIN SMALL LETTER U */
    { -1, "uacute" },		    /* LATIN SMALL LETTER U WITH ACUTE */
    { -1, "ubreve" },		    /* LATIN SMALL LETTER U WITH BREVE */
    { -1, "ucircumflex" },	    /* LATIN SMALL LETTER U WITH CIRCUMFLEX */
    { -1, "udieresis" },	    /* LATIN SMALL LETTER U WITH DIAERESIS */
    { -1, "ugrave" },		    /* LATIN SMALL LETTER U WITH GRAVE */
    { -1, "uhorn" },		    /* LATIN SMALL LETTER U WITH HORN */
    { -1, "uhungarumlaut" },	    /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    { -1, "umacron" },		    /* LATIN SMALL LETTER U WITH MACRON */
    { -1, "underscore" },	    /* LOW LINE */
    { -1, "underscoredbl" },	    /* DOUBLE LOW LINE */
    { -1, "union" },		    /* UNION */
    { -1, "universal" },	    /* FOR ALL */
    { -1, "uogonek" },		    /* LATIN SMALL LETTER U WITH OGONEK */
    { -1, "up" },		    /* FONT FAMILY;Courier */
    { -1, "upblock" },		    /* UPPER HALF BLOCK */
    { -1, "upsilon" },		    /* GREEK SMALL LETTER UPSILON */
    { -1, "upsilondieresis" },	    /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    { -1, "upsilondieresistonos" }, /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    { -1, "upsilontonos" },	    /* GREEK SMALL LETTER UPSILON WITH TONOS */
    { -1, "uring" },		    /* LATIN SMALL LETTER U WITH RING ABOVE */
    { -1, "utilde" },		    /* LATIN SMALL LETTER U WITH TILDE */
    { -1, "v" },		    /* LATIN SMALL LETTER V */
    { -1, "w" },		    /* LATIN SMALL LETTER W */
    { -1, "wacute" },		    /* LATIN SMALL LETTER W WITH ACUTE */
    { -1, "wcircumflex" },	    /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
    { -1, "wdieresis" },	    /* LATIN SMALL LETTER W WITH DIAERESIS */
    { -1, "weierstrass" },	    /* SCRIPT CAPITAL P */
    { -1, "wgrave" },		    /* LATIN SMALL LETTER W WITH GRAVE */
    { -1, "x" },		    /* LATIN SMALL LETTER X */
    { -1, "xi" },		    /* GREEK SMALL LETTER XI */
    { -1, "y" },		    /* LATIN SMALL LETTER Y */
    { -1, "yacute" },		    /* LATIN SMALL LETTER Y WITH ACUTE */
    { -1, "ycircumflex" },	    /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
    { -1, "ydieresis" },	    /* LATIN SMALL LETTER Y WITH DIAERESIS */
    { -1, "yen" },		    /* YEN SIGN */
    { -1, "ygrave" },		    /* LATIN SMALL LETTER Y WITH GRAVE */
    { -1, "z" },		    /* LATIN SMALL LETTER Z */
    { -1, "zacute" },		    /* LATIN SMALL LETTER Z WITH ACUTE */
    { -1, "zcaron" },		    /* LATIN SMALL LETTER Z WITH CARON */
    { -1, "zdotaccent" },	    /* LATIN SMALL LETTER Z WITH DOT ABOVE */
    { -1, "zero" },		    /* DIGIT ZERO */
    { -1, "zeroinferior" },	    /* SUBSCRIPT ZERO */
    { -1, "zerooldstyle" },	    /* OLDSTYLE DIGIT ZERO */
    { -1, "zerosuperior" },	    /* SUPERSCRIPT ZERO */
    { -1, "zeta" }		    /* GREEK SMALL LETTER ZETA */
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
