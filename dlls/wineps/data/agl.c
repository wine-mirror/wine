/*******************************************************************************
 *
 *	Adobe Glyph List data for the Wine PostScript driver
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


/*
 *  Every glyph name in the AGL and the 39 core PostScript fonts
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
 *  The AGL encoding vector, sorted by Unicode value
 */

static const UNICODEGLYPH encoding[1051] = 
{
    { 0x0020, PSDRV_AGLGlyphNames + 1169 },	/* space */
    { 0x0021, PSDRV_AGLGlyphNames +  907 },	/* exclam */
    { 0x0022, PSDRV_AGLGlyphNames + 1118 },	/* quotedbl */
    { 0x0023, PSDRV_AGLGlyphNames + 1043 },	/* numbersign */
    { 0x0024, PSDRV_AGLGlyphNames +  866 },	/* dollar */
    { 0x0025, PSDRV_AGLGlyphNames + 1094 },	/* percent */
    { 0x0026, PSDRV_AGLGlyphNames +  755 },	/* ampersand */
    { 0x0027, PSDRV_AGLGlyphNames + 1126 },	/* quotesingle */
    { 0x0028, PSDRV_AGLGlyphNames + 1081 },	/* parenleft */
    { 0x0029, PSDRV_AGLGlyphNames + 1087 },	/* parenright */
    { 0x002a, PSDRV_AGLGlyphNames +  782 },	/* asterisk */
    { 0x002b, PSDRV_AGLGlyphNames + 1105 },	/* plus */
    { 0x002c, PSDRV_AGLGlyphNames +  835 },	/* comma */
    { 0x002d, PSDRV_AGLGlyphNames +  961 },	/* hyphen */
    { 0x002e, PSDRV_AGLGlyphNames + 1095 },	/* period */
    { 0x002f, PSDRV_AGLGlyphNames + 1167 },	/* slash */
    { 0x0030, PSDRV_AGLGlyphNames + 1253 },	/* zero */
    { 0x0031, PSDRV_AGLGlyphNames + 1060 },	/* one */
    { 0x0032, PSDRV_AGLGlyphNames + 1206 },	/* two */
    { 0x0033, PSDRV_AGLGlyphNames + 1188 },	/* three */
    { 0x0034, PSDRV_AGLGlyphNames +  930 },	/* four */
    { 0x0035, PSDRV_AGLGlyphNames +  922 },	/* five */
    { 0x0036, PSDRV_AGLGlyphNames + 1163 },	/* six */
    { 0x0037, PSDRV_AGLGlyphNames + 1154 },	/* seven */
    { 0x0038, PSDRV_AGLGlyphNames +  886 },	/* eight */
    { 0x0039, PSDRV_AGLGlyphNames + 1032 },	/* nine */
    { 0x003a, PSDRV_AGLGlyphNames +  833 },	/* colon */
    { 0x003b, PSDRV_AGLGlyphNames + 1153 },	/* semicolon */
    { 0x003c, PSDRV_AGLGlyphNames + 1003 },	/* less */
    { 0x003d, PSDRV_AGLGlyphNames +  900 },	/* equal */
    { 0x003e, PSDRV_AGLGlyphNames +  948 },	/* greater */
    { 0x003f, PSDRV_AGLGlyphNames + 1114 },	/* question */
    { 0x0040, PSDRV_AGLGlyphNames +  785 },	/* at */
    { 0x0041, PSDRV_AGLGlyphNames +    0 },	/* A */
    { 0x0042, PSDRV_AGLGlyphNames +   25 },	/* B */
    { 0x0043, PSDRV_AGLGlyphNames +   29 },	/* C */
    { 0x0044, PSDRV_AGLGlyphNames +   42 },	/* D */
    { 0x0045, PSDRV_AGLGlyphNames +   52 },	/* E */
    { 0x0046, PSDRV_AGLGlyphNames +   75 },	/* F */
    { 0x0047, PSDRV_AGLGlyphNames +   77 },	/* G */
    { 0x0048, PSDRV_AGLGlyphNames +   87 },	/* H */
    { 0x0049, PSDRV_AGLGlyphNames +   97 },	/* I */
    { 0x004a, PSDRV_AGLGlyphNames +  118 },	/* J */
    { 0x004b, PSDRV_AGLGlyphNames +  121 },	/* K */
    { 0x004c, PSDRV_AGLGlyphNames +  125 },	/* L */
    { 0x004d, PSDRV_AGLGlyphNames +  135 },	/* M */
    { 0x004e, PSDRV_AGLGlyphNames +  140 },	/* N */
    { 0x004f, PSDRV_AGLGlyphNames +  148 },	/* O */
    { 0x0050, PSDRV_AGLGlyphNames +  174 },	/* P */
    { 0x0051, PSDRV_AGLGlyphNames +  179 },	/* Q */
    { 0x0052, PSDRV_AGLGlyphNames +  181 },	/* R */
    { 0x0053, PSDRV_AGLGlyphNames +  189 },	/* S */
    { 0x0054, PSDRV_AGLGlyphNames +  238 },	/* T */
    { 0x0055, PSDRV_AGLGlyphNames +  248 },	/* U */
    { 0x0056, PSDRV_AGLGlyphNames +  269 },	/* V */
    { 0x0057, PSDRV_AGLGlyphNames +  271 },	/* W */
    { 0x0058, PSDRV_AGLGlyphNames +  277 },	/* X */
    { 0x0059, PSDRV_AGLGlyphNames +  280 },	/* Y */
    { 0x005a, PSDRV_AGLGlyphNames +  288 },	/* Z */
    { 0x005b, PSDRV_AGLGlyphNames +  801 },	/* bracketleft */
    { 0x005c, PSDRV_AGLGlyphNames +  788 },	/* backslash */
    { 0x005d, PSDRV_AGLGlyphNames +  805 },	/* bracketright */
    { 0x005e, PSDRV_AGLGlyphNames +  780 },	/* asciicircum */
    { 0x005f, PSDRV_AGLGlyphNames + 1221 },	/* underscore */
    { 0x0060, PSDRV_AGLGlyphNames +  945 },	/* grave */
    { 0x0061, PSDRV_AGLGlyphNames +  295 },	/* a */
    { 0x0062, PSDRV_AGLGlyphNames +  787 },	/* b */
    { 0x0063, PSDRV_AGLGlyphNames +  813 },	/* c */
    { 0x0064, PSDRV_AGLGlyphNames +  848 },	/* d */
    { 0x0065, PSDRV_AGLGlyphNames +  878 },	/* e */
    { 0x0066, PSDRV_AGLGlyphNames +  913 },	/* f */
    { 0x0067, PSDRV_AGLGlyphNames +  936 },	/* g */
    { 0x0068, PSDRV_AGLGlyphNames +  954 },	/* h */
    { 0x0069, PSDRV_AGLGlyphNames +  964 },	/* i */
    { 0x006a, PSDRV_AGLGlyphNames +  989 },	/* j */
    { 0x006b, PSDRV_AGLGlyphNames +  991 },	/* k */
    { 0x006c, PSDRV_AGLGlyphNames +  995 },	/* l */
    { 0x006d, PSDRV_AGLGlyphNames + 1016 },	/* m */
    { 0x006e, PSDRV_AGLGlyphNames + 1027 },	/* n */
    { 0x006f, PSDRV_AGLGlyphNames + 1044 },	/* o */
    { 0x0070, PSDRV_AGLGlyphNames + 1079 },	/* p */
    { 0x0071, PSDRV_AGLGlyphNames + 1113 },	/* q */
    { 0x0072, PSDRV_AGLGlyphNames + 1127 },	/* r */
    { 0x0073, PSDRV_AGLGlyphNames + 1145 },	/* s */
    { 0x0074, PSDRV_AGLGlyphNames + 1178 },	/* t */
    { 0x0075, PSDRV_AGLGlyphNames + 1212 },	/* u */
    { 0x0076, PSDRV_AGLGlyphNames + 1234 },	/* v */
    { 0x0077, PSDRV_AGLGlyphNames + 1235 },	/* w */
    { 0x0078, PSDRV_AGLGlyphNames + 1241 },	/* x */
    { 0x0079, PSDRV_AGLGlyphNames + 1243 },	/* y */
    { 0x007a, PSDRV_AGLGlyphNames + 1249 },	/* z */
    { 0x007b, PSDRV_AGLGlyphNames +  793 },	/* braceleft */
    { 0x007c, PSDRV_AGLGlyphNames +  789 },	/* bar */
    { 0x007d, PSDRV_AGLGlyphNames +  797 },	/* braceright */
    { 0x007e, PSDRV_AGLGlyphNames +  781 },	/* asciitilde */
    { 0x00a0, PSDRV_AGLGlyphNames + 1169 },	/* space */
    { 0x00a1, PSDRV_AGLGlyphNames +  909 },	/* exclamdown */
    { 0x00a2, PSDRV_AGLGlyphNames +  822 },	/* cent */
    { 0x00a3, PSDRV_AGLGlyphNames + 1173 },	/* sterling */
    { 0x00a4, PSDRV_AGLGlyphNames +  843 },	/* currency */
    { 0x00a5, PSDRV_AGLGlyphNames + 1247 },	/* yen */
    { 0x00a6, PSDRV_AGLGlyphNames +  810 },	/* brokenbar */
    { 0x00a7, PSDRV_AGLGlyphNames + 1152 },	/* section */
    { 0x00a8, PSDRV_AGLGlyphNames +  859 },	/* dieresis */
    { 0x00a9, PSDRV_AGLGlyphNames +  840 },	/* copyright */
    { 0x00aa, PSDRV_AGLGlyphNames + 1071 },	/* ordfeminine */
    { 0x00ab, PSDRV_AGLGlyphNames +  950 },	/* guillemotleft */
    { 0x00ac, PSDRV_AGLGlyphNames + 1009 },	/* logicalnot */
    { 0x00ad, PSDRV_AGLGlyphNames +  961 },	/* hyphen */
    { 0x00ae, PSDRV_AGLGlyphNames + 1135 },	/* registered */
    { 0x00af, PSDRV_AGLGlyphNames + 1017 },	/* macron */
    { 0x00b0, PSDRV_AGLGlyphNames +  856 },	/* degree */
    { 0x00b1, PSDRV_AGLGlyphNames + 1106 },	/* plusminus */
    { 0x00b2, PSDRV_AGLGlyphNames + 1210 },	/* twosuperior */
    { 0x00b3, PSDRV_AGLGlyphNames + 1194 },	/* threesuperior */
    { 0x00b4, PSDRV_AGLGlyphNames +  500 },	/* acute */
    { 0x00b5, PSDRV_AGLGlyphNames + 1023 },	/* mu */
    { 0x00b6, PSDRV_AGLGlyphNames + 1080 },	/* paragraph */
    { 0x00b7, PSDRV_AGLGlyphNames + 1096 },	/* periodcentered */
    { 0x00b8, PSDRV_AGLGlyphNames +  821 },	/* cedilla */
    { 0x00b9, PSDRV_AGLGlyphNames + 1068 },	/* onesuperior */
    { 0x00ba, PSDRV_AGLGlyphNames + 1072 },	/* ordmasculine */
    { 0x00bb, PSDRV_AGLGlyphNames +  951 },	/* guillemotright */
    { 0x00bc, PSDRV_AGLGlyphNames + 1067 },	/* onequarter */
    { 0x00bd, PSDRV_AGLGlyphNames + 1064 },	/* onehalf */
    { 0x00be, PSDRV_AGLGlyphNames + 1192 },	/* threequarters */
    { 0x00bf, PSDRV_AGLGlyphNames + 1115 },	/* questiondown */
    { 0x00c0, PSDRV_AGLGlyphNames +   13 },	/* Agrave */
    { 0x00c1, PSDRV_AGLGlyphNames +    4 },	/* Aacute */
    { 0x00c2, PSDRV_AGLGlyphNames +    7 },	/* Acircumflex */
    { 0x00c3, PSDRV_AGLGlyphNames +   23 },	/* Atilde */
    { 0x00c4, PSDRV_AGLGlyphNames +   11 },	/* Adieresis */
    { 0x00c5, PSDRV_AGLGlyphNames +   19 },	/* Aring */
    { 0x00c6, PSDRV_AGLGlyphNames +    1 },	/* AE */
    { 0x00c7, PSDRV_AGLGlyphNames +   34 },	/* Ccedilla */
    { 0x00c8, PSDRV_AGLGlyphNames +   62 },	/* Egrave */
    { 0x00c9, PSDRV_AGLGlyphNames +   53 },	/* Eacute */
    { 0x00ca, PSDRV_AGLGlyphNames +   57 },	/* Ecircumflex */
    { 0x00cb, PSDRV_AGLGlyphNames +   59 },	/* Edieresis */
    { 0x00cc, PSDRV_AGLGlyphNames +  109 },	/* Igrave */
    { 0x00cd, PSDRV_AGLGlyphNames +   99 },	/* Iacute */
    { 0x00ce, PSDRV_AGLGlyphNames +  102 },	/* Icircumflex */
    { 0x00cf, PSDRV_AGLGlyphNames +  104 },	/* Idieresis */
    { 0x00d0, PSDRV_AGLGlyphNames +   72 },	/* Eth */
    { 0x00d1, PSDRV_AGLGlyphNames +  145 },	/* Ntilde */
    { 0x00d2, PSDRV_AGLGlyphNames +  159 },	/* Ograve */
    { 0x00d3, PSDRV_AGLGlyphNames +  151 },	/* Oacute */
    { 0x00d4, PSDRV_AGLGlyphNames +  154 },	/* Ocircumflex */
    { 0x00d5, PSDRV_AGLGlyphNames +  172 },	/* Otilde */
    { 0x00d6, PSDRV_AGLGlyphNames +  156 },	/* Odieresis */
    { 0x00d7, PSDRV_AGLGlyphNames + 1024 },	/* multiply */
    { 0x00d8, PSDRV_AGLGlyphNames +  168 },	/* Oslash */
    { 0x00d9, PSDRV_AGLGlyphNames +  256 },	/* Ugrave */
    { 0x00da, PSDRV_AGLGlyphNames +  249 },	/* Uacute */
    { 0x00db, PSDRV_AGLGlyphNames +  252 },	/* Ucircumflex */
    { 0x00dc, PSDRV_AGLGlyphNames +  254 },	/* Udieresis */
    { 0x00dd, PSDRV_AGLGlyphNames +  281 },	/* Yacute */
    { 0x00de, PSDRV_AGLGlyphNames +  244 },	/* Thorn */
    { 0x00df, PSDRV_AGLGlyphNames +  943 },	/* germandbls */
    { 0x00e0, PSDRV_AGLGlyphNames +  750 },	/* agrave */
    { 0x00e1, PSDRV_AGLGlyphNames +  497 },	/* aacute */
    { 0x00e2, PSDRV_AGLGlyphNames +  499 },	/* acircumflex */
    { 0x00e3, PSDRV_AGLGlyphNames +  786 },	/* atilde */
    { 0x00e4, PSDRV_AGLGlyphNames +  502 },	/* adieresis */
    { 0x00e5, PSDRV_AGLGlyphNames +  764 },	/* aring */
    { 0x00e6, PSDRV_AGLGlyphNames +  503 },	/* ae */
    { 0x00e7, PSDRV_AGLGlyphNames +  818 },	/* ccedilla */
    { 0x00e8, PSDRV_AGLGlyphNames +  885 },	/* egrave */
    { 0x00e9, PSDRV_AGLGlyphNames +  879 },	/* eacute */
    { 0x00ea, PSDRV_AGLGlyphNames +  882 },	/* ecircumflex */
    { 0x00eb, PSDRV_AGLGlyphNames +  883 },	/* edieresis */
    { 0x00ec, PSDRV_AGLGlyphNames +  969 },	/* igrave */
    { 0x00ed, PSDRV_AGLGlyphNames +  965 },	/* iacute */
    { 0x00ee, PSDRV_AGLGlyphNames +  967 },	/* icircumflex */
    { 0x00ef, PSDRV_AGLGlyphNames +  968 },	/* idieresis */
    { 0x00f0, PSDRV_AGLGlyphNames +  906 },	/* eth */
    { 0x00f1, PSDRV_AGLGlyphNames + 1041 },	/* ntilde */
    { 0x00f2, PSDRV_AGLGlyphNames + 1051 },	/* ograve */
    { 0x00f3, PSDRV_AGLGlyphNames + 1045 },	/* oacute */
    { 0x00f4, PSDRV_AGLGlyphNames + 1047 },	/* ocircumflex */
    { 0x00f5, PSDRV_AGLGlyphNames + 1077 },	/* otilde */
    { 0x00f6, PSDRV_AGLGlyphNames + 1048 },	/* odieresis */
    { 0x00f7, PSDRV_AGLGlyphNames +  863 },	/* divide */
    { 0x00f8, PSDRV_AGLGlyphNames + 1074 },	/* oslash */
    { 0x00f9, PSDRV_AGLGlyphNames + 1217 },	/* ugrave */
    { 0x00fa, PSDRV_AGLGlyphNames + 1213 },	/* uacute */
    { 0x00fb, PSDRV_AGLGlyphNames + 1215 },	/* ucircumflex */
    { 0x00fc, PSDRV_AGLGlyphNames + 1216 },	/* udieresis */
    { 0x00fd, PSDRV_AGLGlyphNames + 1244 },	/* yacute */
    { 0x00fe, PSDRV_AGLGlyphNames + 1187 },	/* thorn */
    { 0x00ff, PSDRV_AGLGlyphNames + 1246 },	/* ydieresis */
    { 0x0100, PSDRV_AGLGlyphNames +   17 },	/* Amacron */
    { 0x0101, PSDRV_AGLGlyphNames +  754 },	/* amacron */
    { 0x0102, PSDRV_AGLGlyphNames +    6 },	/* Abreve */
    { 0x0103, PSDRV_AGLGlyphNames +  498 },	/* abreve */
    { 0x0104, PSDRV_AGLGlyphNames +   18 },	/* Aogonek */
    { 0x0105, PSDRV_AGLGlyphNames +  761 },	/* aogonek */
    { 0x0106, PSDRV_AGLGlyphNames +   30 },	/* Cacute */
    { 0x0107, PSDRV_AGLGlyphNames +  814 },	/* cacute */
    { 0x0108, PSDRV_AGLGlyphNames +   36 },	/* Ccircumflex */
    { 0x0109, PSDRV_AGLGlyphNames +  819 },	/* ccircumflex */
    { 0x010a, PSDRV_AGLGlyphNames +   37 },	/* Cdotaccent */
    { 0x010b, PSDRV_AGLGlyphNames +  820 },	/* cdotaccent */
    { 0x010c, PSDRV_AGLGlyphNames +   33 },	/* Ccaron */
    { 0x010d, PSDRV_AGLGlyphNames +  817 },	/* ccaron */
    { 0x010e, PSDRV_AGLGlyphNames +   43 },	/* Dcaron */
    { 0x010f, PSDRV_AGLGlyphNames +  853 },	/* dcaron */
    { 0x0110, PSDRV_AGLGlyphNames +   44 },	/* Dcroat */
    { 0x0111, PSDRV_AGLGlyphNames +  854 },	/* dcroat */
    { 0x0112, PSDRV_AGLGlyphNames +   64 },	/* Emacron */
    { 0x0113, PSDRV_AGLGlyphNames +  892 },	/* emacron */
    { 0x0114, PSDRV_AGLGlyphNames +   55 },	/* Ebreve */
    { 0x0115, PSDRV_AGLGlyphNames +  880 },	/* ebreve */
    { 0x0116, PSDRV_AGLGlyphNames +   61 },	/* Edotaccent */
    { 0x0117, PSDRV_AGLGlyphNames +  884 },	/* edotaccent */
    { 0x0118, PSDRV_AGLGlyphNames +   66 },	/* Eogonek */
    { 0x0119, PSDRV_AGLGlyphNames +  897 },	/* eogonek */
    { 0x011a, PSDRV_AGLGlyphNames +   56 },	/* Ecaron */
    { 0x011b, PSDRV_AGLGlyphNames +  881 },	/* ecaron */
    { 0x011c, PSDRV_AGLGlyphNames +   81 },	/* Gcircumflex */
    { 0x011d, PSDRV_AGLGlyphNames +  940 },	/* gcircumflex */
    { 0x011e, PSDRV_AGLGlyphNames +   79 },	/* Gbreve */
    { 0x011f, PSDRV_AGLGlyphNames +  938 },	/* gbreve */
    { 0x0120, PSDRV_AGLGlyphNames +   83 },	/* Gdotaccent */
    { 0x0121, PSDRV_AGLGlyphNames +  942 },	/* gdotaccent */
    { 0x0122, PSDRV_AGLGlyphNames +   82 },	/* Gcommaaccent */
    { 0x0123, PSDRV_AGLGlyphNames +  941 },	/* gcommaaccent */
    { 0x0124, PSDRV_AGLGlyphNames +   93 },	/* Hcircumflex */
    { 0x0125, PSDRV_AGLGlyphNames +  956 },	/* hcircumflex */
    { 0x0126, PSDRV_AGLGlyphNames +   92 },	/* Hbar */
    { 0x0127, PSDRV_AGLGlyphNames +  955 },	/* hbar */
    { 0x0128, PSDRV_AGLGlyphNames +  117 },	/* Itilde */
    { 0x0129, PSDRV_AGLGlyphNames +  988 },	/* itilde */
    { 0x012a, PSDRV_AGLGlyphNames +  111 },	/* Imacron */
    { 0x012b, PSDRV_AGLGlyphNames +  971 },	/* imacron */
    { 0x012c, PSDRV_AGLGlyphNames +  101 },	/* Ibreve */
    { 0x012d, PSDRV_AGLGlyphNames +  966 },	/* ibreve */
    { 0x012e, PSDRV_AGLGlyphNames +  112 },	/* Iogonek */
    { 0x012f, PSDRV_AGLGlyphNames +  982 },	/* iogonek */
    { 0x0130, PSDRV_AGLGlyphNames +  107 },	/* Idotaccent */
    { 0x0131, PSDRV_AGLGlyphNames +  873 },	/* dotlessi */
    { 0x0132, PSDRV_AGLGlyphNames +   98 },	/* IJ */
    { 0x0133, PSDRV_AGLGlyphNames +  970 },	/* ij */
    { 0x0134, PSDRV_AGLGlyphNames +  119 },	/* Jcircumflex */
    { 0x0135, PSDRV_AGLGlyphNames +  990 },	/* jcircumflex */
    { 0x0136, PSDRV_AGLGlyphNames +  123 },	/* Kcommaaccent */
    { 0x0137, PSDRV_AGLGlyphNames +  993 },	/* kcommaaccent */
    { 0x0138, PSDRV_AGLGlyphNames +  994 },	/* kgreenlandic */
    { 0x0139, PSDRV_AGLGlyphNames +  127 },	/* Lacute */
    { 0x013a, PSDRV_AGLGlyphNames +  996 },	/* lacute */
    { 0x013b, PSDRV_AGLGlyphNames +  130 },	/* Lcommaaccent */
    { 0x013c, PSDRV_AGLGlyphNames + 1000 },	/* lcommaaccent */
    { 0x013d, PSDRV_AGLGlyphNames +  129 },	/* Lcaron */
    { 0x013e, PSDRV_AGLGlyphNames +  999 },	/* lcaron */
    { 0x013f, PSDRV_AGLGlyphNames +  131 },	/* Ldot */
    { 0x0140, PSDRV_AGLGlyphNames + 1001 },	/* ldot */
    { 0x0141, PSDRV_AGLGlyphNames +  132 },	/* Lslash */
    { 0x0142, PSDRV_AGLGlyphNames + 1013 },	/* lslash */
    { 0x0143, PSDRV_AGLGlyphNames +  141 },	/* Nacute */
    { 0x0144, PSDRV_AGLGlyphNames + 1028 },	/* nacute */
    { 0x0145, PSDRV_AGLGlyphNames +  143 },	/* Ncommaaccent */
    { 0x0146, PSDRV_AGLGlyphNames + 1031 },	/* ncommaaccent */
    { 0x0147, PSDRV_AGLGlyphNames +  142 },	/* Ncaron */
    { 0x0148, PSDRV_AGLGlyphNames + 1030 },	/* ncaron */
    { 0x0149, PSDRV_AGLGlyphNames + 1029 },	/* napostrophe */
    { 0x014a, PSDRV_AGLGlyphNames +   65 },	/* Eng */
    { 0x014b, PSDRV_AGLGlyphNames +  896 },	/* eng */
    { 0x014c, PSDRV_AGLGlyphNames +  163 },	/* Omacron */
    { 0x014d, PSDRV_AGLGlyphNames + 1054 },	/* omacron */
    { 0x014e, PSDRV_AGLGlyphNames +  153 },	/* Obreve */
    { 0x014f, PSDRV_AGLGlyphNames + 1046 },	/* obreve */
    { 0x0150, PSDRV_AGLGlyphNames +  162 },	/* Ohungarumlaut */
    { 0x0151, PSDRV_AGLGlyphNames + 1053 },	/* ohungarumlaut */
    { 0x0152, PSDRV_AGLGlyphNames +  149 },	/* OE */
    { 0x0153, PSDRV_AGLGlyphNames + 1049 },	/* oe */
    { 0x0154, PSDRV_AGLGlyphNames +  182 },	/* Racute */
    { 0x0155, PSDRV_AGLGlyphNames + 1128 },	/* racute */
    { 0x0156, PSDRV_AGLGlyphNames +  184 },	/* Rcommaaccent */
    { 0x0157, PSDRV_AGLGlyphNames + 1132 },	/* rcommaaccent */
    { 0x0158, PSDRV_AGLGlyphNames +  183 },	/* Rcaron */
    { 0x0159, PSDRV_AGLGlyphNames + 1131 },	/* rcaron */
    { 0x015a, PSDRV_AGLGlyphNames +  230 },	/* Sacute */
    { 0x015b, PSDRV_AGLGlyphNames + 1146 },	/* sacute */
    { 0x015c, PSDRV_AGLGlyphNames +  234 },	/* Scircumflex */
    { 0x015d, PSDRV_AGLGlyphNames + 1149 },	/* scircumflex */
    { 0x015e, PSDRV_AGLGlyphNames +  233 },	/* Scedilla */
    { 0x015f, PSDRV_AGLGlyphNames + 1148 },	/* scedilla */
    { 0x0160, PSDRV_AGLGlyphNames +  231 },	/* Scaron */
    { 0x0161, PSDRV_AGLGlyphNames + 1147 },	/* scaron */
    { 0x0162, PSDRV_AGLGlyphNames +  242 },	/* Tcommaaccent */
    { 0x0163, PSDRV_AGLGlyphNames + 1183 },	/* tcommaaccent */
    { 0x0164, PSDRV_AGLGlyphNames +  241 },	/* Tcaron */
    { 0x0165, PSDRV_AGLGlyphNames + 1182 },	/* tcaron */
    { 0x0166, PSDRV_AGLGlyphNames +  240 },	/* Tbar */
    { 0x0167, PSDRV_AGLGlyphNames + 1181 },	/* tbar */
    { 0x0168, PSDRV_AGLGlyphNames +  268 },	/* Utilde */
    { 0x0169, PSDRV_AGLGlyphNames + 1233 },	/* utilde */
    { 0x016a, PSDRV_AGLGlyphNames +  260 },	/* Umacron */
    { 0x016b, PSDRV_AGLGlyphNames + 1220 },	/* umacron */
    { 0x016c, PSDRV_AGLGlyphNames +  251 },	/* Ubreve */
    { 0x016d, PSDRV_AGLGlyphNames + 1214 },	/* ubreve */
    { 0x016e, PSDRV_AGLGlyphNames +  266 },	/* Uring */
    { 0x016f, PSDRV_AGLGlyphNames + 1232 },	/* uring */
    { 0x0170, PSDRV_AGLGlyphNames +  259 },	/* Uhungarumlaut */
    { 0x0171, PSDRV_AGLGlyphNames + 1219 },	/* uhungarumlaut */
    { 0x0172, PSDRV_AGLGlyphNames +  261 },	/* Uogonek */
    { 0x0173, PSDRV_AGLGlyphNames + 1225 },	/* uogonek */
    { 0x0174, PSDRV_AGLGlyphNames +  273 },	/* Wcircumflex */
    { 0x0175, PSDRV_AGLGlyphNames + 1237 },	/* wcircumflex */
    { 0x0176, PSDRV_AGLGlyphNames +  283 },	/* Ycircumflex */
    { 0x0177, PSDRV_AGLGlyphNames + 1245 },	/* ycircumflex */
    { 0x0178, PSDRV_AGLGlyphNames +  284 },	/* Ydieresis */
    { 0x0179, PSDRV_AGLGlyphNames +  289 },	/* Zacute */
    { 0x017a, PSDRV_AGLGlyphNames + 1250 },	/* zacute */
    { 0x017b, PSDRV_AGLGlyphNames +  292 },	/* Zdotaccent */
    { 0x017c, PSDRV_AGLGlyphNames + 1252 },	/* zdotaccent */
    { 0x017d, PSDRV_AGLGlyphNames +  290 },	/* Zcaron */
    { 0x017e, PSDRV_AGLGlyphNames + 1251 },	/* zcaron */
    { 0x017f, PSDRV_AGLGlyphNames + 1011 },	/* longs */
    { 0x0192, PSDRV_AGLGlyphNames +  928 },	/* florin */
    { 0x01a0, PSDRV_AGLGlyphNames +  161 },	/* Ohorn */
    { 0x01a1, PSDRV_AGLGlyphNames + 1052 },	/* ohorn */
    { 0x01af, PSDRV_AGLGlyphNames +  258 },	/* Uhorn */
    { 0x01b0, PSDRV_AGLGlyphNames + 1218 },	/* uhorn */
    { 0x01e6, PSDRV_AGLGlyphNames +   80 },	/* Gcaron */
    { 0x01e7, PSDRV_AGLGlyphNames +  939 },	/* gcaron */
    { 0x01fa, PSDRV_AGLGlyphNames +   20 },	/* Aringacute */
    { 0x01fb, PSDRV_AGLGlyphNames +  765 },	/* aringacute */
    { 0x01fc, PSDRV_AGLGlyphNames +    2 },	/* AEacute */
    { 0x01fd, PSDRV_AGLGlyphNames +  504 },	/* aeacute */
    { 0x01fe, PSDRV_AGLGlyphNames +  169 },	/* Oslashacute */
    { 0x01ff, PSDRV_AGLGlyphNames + 1075 },	/* oslashacute */
    { 0x0218, PSDRV_AGLGlyphNames +  235 },	/* Scommaaccent */
    { 0x0219, PSDRV_AGLGlyphNames + 1150 },	/* scommaaccent */
    { 0x021a, PSDRV_AGLGlyphNames +  242 },	/* Tcommaaccent */
    { 0x021b, PSDRV_AGLGlyphNames + 1183 },	/* tcommaaccent */
    { 0x02bc, PSDRV_AGLGlyphNames +  740 },	/* afii57929 */
    { 0x02bd, PSDRV_AGLGlyphNames +  749 },	/* afii64937 */
    { 0x02c6, PSDRV_AGLGlyphNames +  831 },	/* circumflex */
    { 0x02c7, PSDRV_AGLGlyphNames +  815 },	/* caron */
    { 0x02c9, PSDRV_AGLGlyphNames + 1017 },	/* macron */
    { 0x02d8, PSDRV_AGLGlyphNames +  809 },	/* breve */
    { 0x02d9, PSDRV_AGLGlyphNames +  871 },	/* dotaccent */
    { 0x02da, PSDRV_AGLGlyphNames + 1141 },	/* ring */
    { 0x02db, PSDRV_AGLGlyphNames + 1050 },	/* ogonek */
    { 0x02dc, PSDRV_AGLGlyphNames + 1195 },	/* tilde */
    { 0x02dd, PSDRV_AGLGlyphNames +  960 },	/* hungarumlaut */
    { 0x0300, PSDRV_AGLGlyphNames +  946 },	/* gravecomb */
    { 0x0301, PSDRV_AGLGlyphNames +  501 },	/* acutecomb */
    { 0x0303, PSDRV_AGLGlyphNames + 1196 },	/* tildecomb */
    { 0x0309, PSDRV_AGLGlyphNames +  958 },	/* hookabovecomb */
    { 0x0323, PSDRV_AGLGlyphNames +  872 },	/* dotbelowcomb */
    { 0x0384, PSDRV_AGLGlyphNames + 1197 },	/* tonos */
    { 0x0385, PSDRV_AGLGlyphNames +  862 },	/* dieresistonos */
    { 0x0386, PSDRV_AGLGlyphNames +   16 },	/* Alphatonos */
    { 0x0387, PSDRV_AGLGlyphNames +  760 },	/* anoteleia */
    { 0x0388, PSDRV_AGLGlyphNames +   68 },	/* Epsilontonos */
    { 0x0389, PSDRV_AGLGlyphNames +   71 },	/* Etatonos */
    { 0x038a, PSDRV_AGLGlyphNames +  115 },	/* Iotatonos */
    { 0x038c, PSDRV_AGLGlyphNames +  167 },	/* Omicrontonos */
    { 0x038e, PSDRV_AGLGlyphNames +  265 },	/* Upsilontonos */
    { 0x038f, PSDRV_AGLGlyphNames +  165 },	/* Omegatonos */
    { 0x0390, PSDRV_AGLGlyphNames +  985 },	/* iotadieresistonos */
    { 0x0391, PSDRV_AGLGlyphNames +   15 },	/* Alpha */
    { 0x0392, PSDRV_AGLGlyphNames +   26 },	/* Beta */
    { 0x0393, PSDRV_AGLGlyphNames +   78 },	/* Gamma */
    { 0x0394, PSDRV_AGLGlyphNames +   45 },	/* Delta */
    { 0x0395, PSDRV_AGLGlyphNames +   67 },	/* Epsilon */
    { 0x0396, PSDRV_AGLGlyphNames +  293 },	/* Zeta */
    { 0x0397, PSDRV_AGLGlyphNames +   70 },	/* Eta */
    { 0x0398, PSDRV_AGLGlyphNames +  243 },	/* Theta */
    { 0x0399, PSDRV_AGLGlyphNames +  113 },	/* Iota */
    { 0x039a, PSDRV_AGLGlyphNames +  122 },	/* Kappa */
    { 0x039b, PSDRV_AGLGlyphNames +  128 },	/* Lambda */
    { 0x039c, PSDRV_AGLGlyphNames +  139 },	/* Mu */
    { 0x039d, PSDRV_AGLGlyphNames +  147 },	/* Nu */
    { 0x039e, PSDRV_AGLGlyphNames +  278 },	/* Xi */
    { 0x039f, PSDRV_AGLGlyphNames +  166 },	/* Omicron */
    { 0x03a0, PSDRV_AGLGlyphNames +  176 },	/* Pi */
    { 0x03a1, PSDRV_AGLGlyphNames +  186 },	/* Rho */
    { 0x03a3, PSDRV_AGLGlyphNames +  236 },	/* Sigma */
    { 0x03a4, PSDRV_AGLGlyphNames +  239 },	/* Tau */
    { 0x03a5, PSDRV_AGLGlyphNames +  262 },	/* Upsilon */
    { 0x03a6, PSDRV_AGLGlyphNames +  175 },	/* Phi */
    { 0x03a7, PSDRV_AGLGlyphNames +   39 },	/* Chi */
    { 0x03a8, PSDRV_AGLGlyphNames +  177 },	/* Psi */
    { 0x03a9, PSDRV_AGLGlyphNames +  164 },	/* Omega */
    { 0x03aa, PSDRV_AGLGlyphNames +  114 },	/* Iotadieresis */
    { 0x03ab, PSDRV_AGLGlyphNames +  264 },	/* Upsilondieresis */
    { 0x03ac, PSDRV_AGLGlyphNames +  753 },	/* alphatonos */
    { 0x03ad, PSDRV_AGLGlyphNames +  899 },	/* epsilontonos */
    { 0x03ae, PSDRV_AGLGlyphNames +  905 },	/* etatonos */
    { 0x03af, PSDRV_AGLGlyphNames +  986 },	/* iotatonos */
    { 0x03b0, PSDRV_AGLGlyphNames + 1230 },	/* upsilondieresistonos */
    { 0x03b1, PSDRV_AGLGlyphNames +  752 },	/* alpha */
    { 0x03b2, PSDRV_AGLGlyphNames +  790 },	/* beta */
    { 0x03b3, PSDRV_AGLGlyphNames +  937 },	/* gamma */
    { 0x03b4, PSDRV_AGLGlyphNames +  857 },	/* delta */
    { 0x03b5, PSDRV_AGLGlyphNames +  898 },	/* epsilon */
    { 0x03b6, PSDRV_AGLGlyphNames + 1257 },	/* zeta */
    { 0x03b7, PSDRV_AGLGlyphNames +  904 },	/* eta */
    { 0x03b8, PSDRV_AGLGlyphNames + 1185 },	/* theta */
    { 0x03b9, PSDRV_AGLGlyphNames +  983 },	/* iota */
    { 0x03ba, PSDRV_AGLGlyphNames +  992 },	/* kappa */
    { 0x03bb, PSDRV_AGLGlyphNames +  997 },	/* lambda */
    { 0x03bc, PSDRV_AGLGlyphNames + 1023 },	/* mu */
    { 0x03bd, PSDRV_AGLGlyphNames + 1042 },	/* nu */
    { 0x03be, PSDRV_AGLGlyphNames + 1242 },	/* xi */
    { 0x03bf, PSDRV_AGLGlyphNames + 1058 },	/* omicron */
    { 0x03c0, PSDRV_AGLGlyphNames + 1104 },	/* pi */
    { 0x03c1, PSDRV_AGLGlyphNames + 1140 },	/* rho */
    { 0x03c2, PSDRV_AGLGlyphNames + 1161 },	/* sigma1 */
    { 0x03c3, PSDRV_AGLGlyphNames + 1160 },	/* sigma */
    { 0x03c4, PSDRV_AGLGlyphNames + 1180 },	/* tau */
    { 0x03c5, PSDRV_AGLGlyphNames + 1228 },	/* upsilon */
    { 0x03c6, PSDRV_AGLGlyphNames + 1102 },	/* phi */
    { 0x03c7, PSDRV_AGLGlyphNames +  827 },	/* chi */
    { 0x03c8, PSDRV_AGLGlyphNames + 1112 },	/* psi */
    { 0x03c9, PSDRV_AGLGlyphNames + 1055 },	/* omega */
    { 0x03ca, PSDRV_AGLGlyphNames +  984 },	/* iotadieresis */
    { 0x03cb, PSDRV_AGLGlyphNames + 1229 },	/* upsilondieresis */
    { 0x03cc, PSDRV_AGLGlyphNames + 1059 },	/* omicrontonos */
    { 0x03cd, PSDRV_AGLGlyphNames + 1231 },	/* upsilontonos */
    { 0x03ce, PSDRV_AGLGlyphNames + 1057 },	/* omegatonos */
    { 0x03d1, PSDRV_AGLGlyphNames + 1186 },	/* theta1 */
    { 0x03d2, PSDRV_AGLGlyphNames +  263 },	/* Upsilon1 */
    { 0x03d5, PSDRV_AGLGlyphNames + 1103 },	/* phi1 */
    { 0x03d6, PSDRV_AGLGlyphNames + 1056 },	/* omega1 */
    { 0x0401, PSDRV_AGLGlyphNames +  512 },	/* afii10023 */
    { 0x0402, PSDRV_AGLGlyphNames +  540 },	/* afii10051 */
    { 0x0403, PSDRV_AGLGlyphNames +  541 },	/* afii10052 */
    { 0x0404, PSDRV_AGLGlyphNames +  542 },	/* afii10053 */
    { 0x0405, PSDRV_AGLGlyphNames +  543 },	/* afii10054 */
    { 0x0406, PSDRV_AGLGlyphNames +  544 },	/* afii10055 */
    { 0x0407, PSDRV_AGLGlyphNames +  545 },	/* afii10056 */
    { 0x0408, PSDRV_AGLGlyphNames +  546 },	/* afii10057 */
    { 0x0409, PSDRV_AGLGlyphNames +  547 },	/* afii10058 */
    { 0x040a, PSDRV_AGLGlyphNames +  548 },	/* afii10059 */
    { 0x040b, PSDRV_AGLGlyphNames +  549 },	/* afii10060 */
    { 0x040c, PSDRV_AGLGlyphNames +  550 },	/* afii10061 */
    { 0x040e, PSDRV_AGLGlyphNames +  551 },	/* afii10062 */
    { 0x040f, PSDRV_AGLGlyphNames +  600 },	/* afii10145 */
    { 0x0410, PSDRV_AGLGlyphNames +  506 },	/* afii10017 */
    { 0x0411, PSDRV_AGLGlyphNames +  507 },	/* afii10018 */
    { 0x0412, PSDRV_AGLGlyphNames +  508 },	/* afii10019 */
    { 0x0413, PSDRV_AGLGlyphNames +  509 },	/* afii10020 */
    { 0x0414, PSDRV_AGLGlyphNames +  510 },	/* afii10021 */
    { 0x0415, PSDRV_AGLGlyphNames +  511 },	/* afii10022 */
    { 0x0416, PSDRV_AGLGlyphNames +  513 },	/* afii10024 */
    { 0x0417, PSDRV_AGLGlyphNames +  514 },	/* afii10025 */
    { 0x0418, PSDRV_AGLGlyphNames +  515 },	/* afii10026 */
    { 0x0419, PSDRV_AGLGlyphNames +  516 },	/* afii10027 */
    { 0x041a, PSDRV_AGLGlyphNames +  517 },	/* afii10028 */
    { 0x041b, PSDRV_AGLGlyphNames +  518 },	/* afii10029 */
    { 0x041c, PSDRV_AGLGlyphNames +  519 },	/* afii10030 */
    { 0x041d, PSDRV_AGLGlyphNames +  520 },	/* afii10031 */
    { 0x041e, PSDRV_AGLGlyphNames +  521 },	/* afii10032 */
    { 0x041f, PSDRV_AGLGlyphNames +  522 },	/* afii10033 */
    { 0x0420, PSDRV_AGLGlyphNames +  523 },	/* afii10034 */
    { 0x0421, PSDRV_AGLGlyphNames +  524 },	/* afii10035 */
    { 0x0422, PSDRV_AGLGlyphNames +  525 },	/* afii10036 */
    { 0x0423, PSDRV_AGLGlyphNames +  526 },	/* afii10037 */
    { 0x0424, PSDRV_AGLGlyphNames +  527 },	/* afii10038 */
    { 0x0425, PSDRV_AGLGlyphNames +  528 },	/* afii10039 */
    { 0x0426, PSDRV_AGLGlyphNames +  529 },	/* afii10040 */
    { 0x0427, PSDRV_AGLGlyphNames +  530 },	/* afii10041 */
    { 0x0428, PSDRV_AGLGlyphNames +  531 },	/* afii10042 */
    { 0x0429, PSDRV_AGLGlyphNames +  532 },	/* afii10043 */
    { 0x042a, PSDRV_AGLGlyphNames +  533 },	/* afii10044 */
    { 0x042b, PSDRV_AGLGlyphNames +  534 },	/* afii10045 */
    { 0x042c, PSDRV_AGLGlyphNames +  535 },	/* afii10046 */
    { 0x042d, PSDRV_AGLGlyphNames +  536 },	/* afii10047 */
    { 0x042e, PSDRV_AGLGlyphNames +  537 },	/* afii10048 */
    { 0x042f, PSDRV_AGLGlyphNames +  538 },	/* afii10049 */
    { 0x0430, PSDRV_AGLGlyphNames +  554 },	/* afii10065 */
    { 0x0431, PSDRV_AGLGlyphNames +  555 },	/* afii10066 */
    { 0x0432, PSDRV_AGLGlyphNames +  556 },	/* afii10067 */
    { 0x0433, PSDRV_AGLGlyphNames +  557 },	/* afii10068 */
    { 0x0434, PSDRV_AGLGlyphNames +  558 },	/* afii10069 */
    { 0x0435, PSDRV_AGLGlyphNames +  559 },	/* afii10070 */
    { 0x0436, PSDRV_AGLGlyphNames +  561 },	/* afii10072 */
    { 0x0437, PSDRV_AGLGlyphNames +  562 },	/* afii10073 */
    { 0x0438, PSDRV_AGLGlyphNames +  563 },	/* afii10074 */
    { 0x0439, PSDRV_AGLGlyphNames +  564 },	/* afii10075 */
    { 0x043a, PSDRV_AGLGlyphNames +  565 },	/* afii10076 */
    { 0x043b, PSDRV_AGLGlyphNames +  566 },	/* afii10077 */
    { 0x043c, PSDRV_AGLGlyphNames +  567 },	/* afii10078 */
    { 0x043d, PSDRV_AGLGlyphNames +  568 },	/* afii10079 */
    { 0x043e, PSDRV_AGLGlyphNames +  569 },	/* afii10080 */
    { 0x043f, PSDRV_AGLGlyphNames +  570 },	/* afii10081 */
    { 0x0440, PSDRV_AGLGlyphNames +  571 },	/* afii10082 */
    { 0x0441, PSDRV_AGLGlyphNames +  572 },	/* afii10083 */
    { 0x0442, PSDRV_AGLGlyphNames +  573 },	/* afii10084 */
    { 0x0443, PSDRV_AGLGlyphNames +  574 },	/* afii10085 */
    { 0x0444, PSDRV_AGLGlyphNames +  575 },	/* afii10086 */
    { 0x0445, PSDRV_AGLGlyphNames +  576 },	/* afii10087 */
    { 0x0446, PSDRV_AGLGlyphNames +  577 },	/* afii10088 */
    { 0x0447, PSDRV_AGLGlyphNames +  578 },	/* afii10089 */
    { 0x0448, PSDRV_AGLGlyphNames +  579 },	/* afii10090 */
    { 0x0449, PSDRV_AGLGlyphNames +  580 },	/* afii10091 */
    { 0x044a, PSDRV_AGLGlyphNames +  581 },	/* afii10092 */
    { 0x044b, PSDRV_AGLGlyphNames +  582 },	/* afii10093 */
    { 0x044c, PSDRV_AGLGlyphNames +  583 },	/* afii10094 */
    { 0x044d, PSDRV_AGLGlyphNames +  584 },	/* afii10095 */
    { 0x044e, PSDRV_AGLGlyphNames +  585 },	/* afii10096 */
    { 0x044f, PSDRV_AGLGlyphNames +  586 },	/* afii10097 */
    { 0x0451, PSDRV_AGLGlyphNames +  560 },	/* afii10071 */
    { 0x0452, PSDRV_AGLGlyphNames +  588 },	/* afii10099 */
    { 0x0453, PSDRV_AGLGlyphNames +  589 },	/* afii10100 */
    { 0x0454, PSDRV_AGLGlyphNames +  590 },	/* afii10101 */
    { 0x0455, PSDRV_AGLGlyphNames +  591 },	/* afii10102 */
    { 0x0456, PSDRV_AGLGlyphNames +  592 },	/* afii10103 */
    { 0x0457, PSDRV_AGLGlyphNames +  593 },	/* afii10104 */
    { 0x0458, PSDRV_AGLGlyphNames +  594 },	/* afii10105 */
    { 0x0459, PSDRV_AGLGlyphNames +  595 },	/* afii10106 */
    { 0x045a, PSDRV_AGLGlyphNames +  596 },	/* afii10107 */
    { 0x045b, PSDRV_AGLGlyphNames +  597 },	/* afii10108 */
    { 0x045c, PSDRV_AGLGlyphNames +  598 },	/* afii10109 */
    { 0x045e, PSDRV_AGLGlyphNames +  599 },	/* afii10110 */
    { 0x045f, PSDRV_AGLGlyphNames +  605 },	/* afii10193 */
    { 0x0462, PSDRV_AGLGlyphNames +  601 },	/* afii10146 */
    { 0x0463, PSDRV_AGLGlyphNames +  606 },	/* afii10194 */
    { 0x0472, PSDRV_AGLGlyphNames +  602 },	/* afii10147 */
    { 0x0473, PSDRV_AGLGlyphNames +  607 },	/* afii10195 */
    { 0x0474, PSDRV_AGLGlyphNames +  603 },	/* afii10148 */
    { 0x0475, PSDRV_AGLGlyphNames +  608 },	/* afii10196 */
    { 0x0490, PSDRV_AGLGlyphNames +  539 },	/* afii10050 */
    { 0x0491, PSDRV_AGLGlyphNames +  587 },	/* afii10098 */
    { 0x04d9, PSDRV_AGLGlyphNames +  611 },	/* afii10846 */
    { 0x05b0, PSDRV_AGLGlyphNames +  729 },	/* afii57799 */
    { 0x05b1, PSDRV_AGLGlyphNames +  731 },	/* afii57801 */
    { 0x05b2, PSDRV_AGLGlyphNames +  730 },	/* afii57800 */
    { 0x05b3, PSDRV_AGLGlyphNames +  732 },	/* afii57802 */
    { 0x05b4, PSDRV_AGLGlyphNames +  723 },	/* afii57793 */
    { 0x05b5, PSDRV_AGLGlyphNames +  724 },	/* afii57794 */
    { 0x05b6, PSDRV_AGLGlyphNames +  725 },	/* afii57795 */
    { 0x05b7, PSDRV_AGLGlyphNames +  728 },	/* afii57798 */
    { 0x05b8, PSDRV_AGLGlyphNames +  727 },	/* afii57797 */
    { 0x05b9, PSDRV_AGLGlyphNames +  735 },	/* afii57806 */
    { 0x05bb, PSDRV_AGLGlyphNames +  726 },	/* afii57796 */
    { 0x05bc, PSDRV_AGLGlyphNames +  736 },	/* afii57807 */
    { 0x05bd, PSDRV_AGLGlyphNames +  737 },	/* afii57839 */
    { 0x05be, PSDRV_AGLGlyphNames +  686 },	/* afii57645 */
    { 0x05bf, PSDRV_AGLGlyphNames +  738 },	/* afii57841 */
    { 0x05c0, PSDRV_AGLGlyphNames +  739 },	/* afii57842 */
    { 0x05c1, PSDRV_AGLGlyphNames +  734 },	/* afii57804 */
    { 0x05c2, PSDRV_AGLGlyphNames +  733 },	/* afii57803 */
    { 0x05c3, PSDRV_AGLGlyphNames +  687 },	/* afii57658 */
    { 0x05d0, PSDRV_AGLGlyphNames +  688 },	/* afii57664 */
    { 0x05d1, PSDRV_AGLGlyphNames +  689 },	/* afii57665 */
    { 0x05d2, PSDRV_AGLGlyphNames +  690 },	/* afii57666 */
    { 0x05d3, PSDRV_AGLGlyphNames +  691 },	/* afii57667 */
    { 0x05d4, PSDRV_AGLGlyphNames +  692 },	/* afii57668 */
    { 0x05d5, PSDRV_AGLGlyphNames +  693 },	/* afii57669 */
    { 0x05d6, PSDRV_AGLGlyphNames +  694 },	/* afii57670 */
    { 0x05d7, PSDRV_AGLGlyphNames +  695 },	/* afii57671 */
    { 0x05d8, PSDRV_AGLGlyphNames +  696 },	/* afii57672 */
    { 0x05d9, PSDRV_AGLGlyphNames +  697 },	/* afii57673 */
    { 0x05da, PSDRV_AGLGlyphNames +  698 },	/* afii57674 */
    { 0x05db, PSDRV_AGLGlyphNames +  699 },	/* afii57675 */
    { 0x05dc, PSDRV_AGLGlyphNames +  700 },	/* afii57676 */
    { 0x05dd, PSDRV_AGLGlyphNames +  701 },	/* afii57677 */
    { 0x05de, PSDRV_AGLGlyphNames +  702 },	/* afii57678 */
    { 0x05df, PSDRV_AGLGlyphNames +  703 },	/* afii57679 */
    { 0x05e0, PSDRV_AGLGlyphNames +  704 },	/* afii57680 */
    { 0x05e1, PSDRV_AGLGlyphNames +  705 },	/* afii57681 */
    { 0x05e2, PSDRV_AGLGlyphNames +  706 },	/* afii57682 */
    { 0x05e3, PSDRV_AGLGlyphNames +  707 },	/* afii57683 */
    { 0x05e4, PSDRV_AGLGlyphNames +  708 },	/* afii57684 */
    { 0x05e5, PSDRV_AGLGlyphNames +  709 },	/* afii57685 */
    { 0x05e6, PSDRV_AGLGlyphNames +  710 },	/* afii57686 */
    { 0x05e7, PSDRV_AGLGlyphNames +  711 },	/* afii57687 */
    { 0x05e8, PSDRV_AGLGlyphNames +  712 },	/* afii57688 */
    { 0x05e9, PSDRV_AGLGlyphNames +  713 },	/* afii57689 */
    { 0x05ea, PSDRV_AGLGlyphNames +  714 },	/* afii57690 */
    { 0x05f0, PSDRV_AGLGlyphNames +  719 },	/* afii57716 */
    { 0x05f1, PSDRV_AGLGlyphNames +  720 },	/* afii57717 */
    { 0x05f2, PSDRV_AGLGlyphNames +  721 },	/* afii57718 */
    { 0x060c, PSDRV_AGLGlyphNames +  616 },	/* afii57388 */
    { 0x061b, PSDRV_AGLGlyphNames +  627 },	/* afii57403 */
    { 0x061f, PSDRV_AGLGlyphNames +  628 },	/* afii57407 */
    { 0x0621, PSDRV_AGLGlyphNames +  629 },	/* afii57409 */
    { 0x0622, PSDRV_AGLGlyphNames +  630 },	/* afii57410 */
    { 0x0623, PSDRV_AGLGlyphNames +  631 },	/* afii57411 */
    { 0x0624, PSDRV_AGLGlyphNames +  632 },	/* afii57412 */
    { 0x0625, PSDRV_AGLGlyphNames +  633 },	/* afii57413 */
    { 0x0626, PSDRV_AGLGlyphNames +  634 },	/* afii57414 */
    { 0x0627, PSDRV_AGLGlyphNames +  635 },	/* afii57415 */
    { 0x0628, PSDRV_AGLGlyphNames +  636 },	/* afii57416 */
    { 0x0629, PSDRV_AGLGlyphNames +  637 },	/* afii57417 */
    { 0x062a, PSDRV_AGLGlyphNames +  638 },	/* afii57418 */
    { 0x062b, PSDRV_AGLGlyphNames +  639 },	/* afii57419 */
    { 0x062c, PSDRV_AGLGlyphNames +  640 },	/* afii57420 */
    { 0x062d, PSDRV_AGLGlyphNames +  641 },	/* afii57421 */
    { 0x062e, PSDRV_AGLGlyphNames +  642 },	/* afii57422 */
    { 0x062f, PSDRV_AGLGlyphNames +  643 },	/* afii57423 */
    { 0x0630, PSDRV_AGLGlyphNames +  644 },	/* afii57424 */
    { 0x0631, PSDRV_AGLGlyphNames +  645 },	/* afii57425 */
    { 0x0632, PSDRV_AGLGlyphNames +  646 },	/* afii57426 */
    { 0x0633, PSDRV_AGLGlyphNames +  647 },	/* afii57427 */
    { 0x0634, PSDRV_AGLGlyphNames +  648 },	/* afii57428 */
    { 0x0635, PSDRV_AGLGlyphNames +  649 },	/* afii57429 */
    { 0x0636, PSDRV_AGLGlyphNames +  650 },	/* afii57430 */
    { 0x0637, PSDRV_AGLGlyphNames +  651 },	/* afii57431 */
    { 0x0638, PSDRV_AGLGlyphNames +  652 },	/* afii57432 */
    { 0x0639, PSDRV_AGLGlyphNames +  653 },	/* afii57433 */
    { 0x063a, PSDRV_AGLGlyphNames +  654 },	/* afii57434 */
    { 0x0640, PSDRV_AGLGlyphNames +  655 },	/* afii57440 */
    { 0x0641, PSDRV_AGLGlyphNames +  656 },	/* afii57441 */
    { 0x0642, PSDRV_AGLGlyphNames +  657 },	/* afii57442 */
    { 0x0643, PSDRV_AGLGlyphNames +  658 },	/* afii57443 */
    { 0x0644, PSDRV_AGLGlyphNames +  659 },	/* afii57444 */
    { 0x0645, PSDRV_AGLGlyphNames +  660 },	/* afii57445 */
    { 0x0646, PSDRV_AGLGlyphNames +  661 },	/* afii57446 */
    { 0x0647, PSDRV_AGLGlyphNames +  673 },	/* afii57470 */
    { 0x0648, PSDRV_AGLGlyphNames +  662 },	/* afii57448 */
    { 0x0649, PSDRV_AGLGlyphNames +  663 },	/* afii57449 */
    { 0x064a, PSDRV_AGLGlyphNames +  664 },	/* afii57450 */
    { 0x064b, PSDRV_AGLGlyphNames +  665 },	/* afii57451 */
    { 0x064c, PSDRV_AGLGlyphNames +  666 },	/* afii57452 */
    { 0x064d, PSDRV_AGLGlyphNames +  667 },	/* afii57453 */
    { 0x064e, PSDRV_AGLGlyphNames +  668 },	/* afii57454 */
    { 0x064f, PSDRV_AGLGlyphNames +  669 },	/* afii57455 */
    { 0x0650, PSDRV_AGLGlyphNames +  670 },	/* afii57456 */
    { 0x0651, PSDRV_AGLGlyphNames +  671 },	/* afii57457 */
    { 0x0652, PSDRV_AGLGlyphNames +  672 },	/* afii57458 */
    { 0x0660, PSDRV_AGLGlyphNames +  617 },	/* afii57392 */
    { 0x0661, PSDRV_AGLGlyphNames +  618 },	/* afii57393 */
    { 0x0662, PSDRV_AGLGlyphNames +  619 },	/* afii57394 */
    { 0x0663, PSDRV_AGLGlyphNames +  620 },	/* afii57395 */
    { 0x0664, PSDRV_AGLGlyphNames +  621 },	/* afii57396 */
    { 0x0665, PSDRV_AGLGlyphNames +  622 },	/* afii57397 */
    { 0x0666, PSDRV_AGLGlyphNames +  623 },	/* afii57398 */
    { 0x0667, PSDRV_AGLGlyphNames +  624 },	/* afii57399 */
    { 0x0668, PSDRV_AGLGlyphNames +  625 },	/* afii57400 */
    { 0x0669, PSDRV_AGLGlyphNames +  626 },	/* afii57401 */
    { 0x066a, PSDRV_AGLGlyphNames +  615 },	/* afii57381 */
    { 0x066d, PSDRV_AGLGlyphNames +  748 },	/* afii63167 */
    { 0x0679, PSDRV_AGLGlyphNames +  679 },	/* afii57511 */
    { 0x067e, PSDRV_AGLGlyphNames +  675 },	/* afii57506 */
    { 0x0686, PSDRV_AGLGlyphNames +  676 },	/* afii57507 */
    { 0x0688, PSDRV_AGLGlyphNames +  680 },	/* afii57512 */
    { 0x0691, PSDRV_AGLGlyphNames +  681 },	/* afii57513 */
    { 0x0698, PSDRV_AGLGlyphNames +  677 },	/* afii57508 */
    { 0x06a4, PSDRV_AGLGlyphNames +  674 },	/* afii57505 */
    { 0x06af, PSDRV_AGLGlyphNames +  678 },	/* afii57509 */
    { 0x06ba, PSDRV_AGLGlyphNames +  682 },	/* afii57514 */
    { 0x06d2, PSDRV_AGLGlyphNames +  683 },	/* afii57519 */
    { 0x06d5, PSDRV_AGLGlyphNames +  684 },	/* afii57534 */
    { 0x1e80, PSDRV_AGLGlyphNames +  275 },	/* Wgrave */
    { 0x1e81, PSDRV_AGLGlyphNames + 1240 },	/* wgrave */
    { 0x1e82, PSDRV_AGLGlyphNames +  272 },	/* Wacute */
    { 0x1e83, PSDRV_AGLGlyphNames + 1236 },	/* wacute */
    { 0x1e84, PSDRV_AGLGlyphNames +  274 },	/* Wdieresis */
    { 0x1e85, PSDRV_AGLGlyphNames + 1238 },	/* wdieresis */
    { 0x1ef2, PSDRV_AGLGlyphNames +  286 },	/* Ygrave */
    { 0x1ef3, PSDRV_AGLGlyphNames + 1248 },	/* ygrave */
    { 0x200c, PSDRV_AGLGlyphNames +  747 },	/* afii61664 */
    { 0x200d, PSDRV_AGLGlyphNames +  614 },	/* afii301 */
    { 0x200e, PSDRV_AGLGlyphNames +  612 },	/* afii299 */
    { 0x200f, PSDRV_AGLGlyphNames +  613 },	/* afii300 */
    { 0x2012, PSDRV_AGLGlyphNames +  919 },	/* figuredash */
    { 0x2013, PSDRV_AGLGlyphNames +  895 },	/* endash */
    { 0x2014, PSDRV_AGLGlyphNames +  893 },	/* emdash */
    { 0x2015, PSDRV_AGLGlyphNames +  505 },	/* afii00208 */
    { 0x2017, PSDRV_AGLGlyphNames + 1222 },	/* underscoredbl */
    { 0x2018, PSDRV_AGLGlyphNames + 1122 },	/* quoteleft */
    { 0x2019, PSDRV_AGLGlyphNames + 1124 },	/* quoteright */
    { 0x201a, PSDRV_AGLGlyphNames + 1125 },	/* quotesinglbase */
    { 0x201b, PSDRV_AGLGlyphNames + 1123 },	/* quotereversed */
    { 0x201c, PSDRV_AGLGlyphNames + 1120 },	/* quotedblleft */
    { 0x201d, PSDRV_AGLGlyphNames + 1121 },	/* quotedblright */
    { 0x201e, PSDRV_AGLGlyphNames + 1119 },	/* quotedblbase */
    { 0x2020, PSDRV_AGLGlyphNames +  849 },	/* dagger */
    { 0x2021, PSDRV_AGLGlyphNames +  850 },	/* daggerdbl */
    { 0x2022, PSDRV_AGLGlyphNames +  812 },	/* bullet */
    { 0x2024, PSDRV_AGLGlyphNames + 1061 },	/* onedotenleader */
    { 0x2025, PSDRV_AGLGlyphNames + 1207 },	/* twodotenleader */
    { 0x2026, PSDRV_AGLGlyphNames +  891 },	/* ellipsis */
    { 0x202c, PSDRV_AGLGlyphNames +  744 },	/* afii61573 */
    { 0x202d, PSDRV_AGLGlyphNames +  745 },	/* afii61574 */
    { 0x202e, PSDRV_AGLGlyphNames +  746 },	/* afii61575 */
    { 0x2030, PSDRV_AGLGlyphNames + 1100 },	/* perthousand */
    { 0x2032, PSDRV_AGLGlyphNames + 1021 },	/* minute */
    { 0x2033, PSDRV_AGLGlyphNames + 1151 },	/* second */
    { 0x2039, PSDRV_AGLGlyphNames +  952 },	/* guilsinglleft */
    { 0x203a, PSDRV_AGLGlyphNames +  953 },	/* guilsinglright */
    { 0x203c, PSDRV_AGLGlyphNames +  908 },	/* exclamdbl */
    { 0x2044, PSDRV_AGLGlyphNames +  934 },	/* fraction */
    { 0x2070, PSDRV_AGLGlyphNames + 1256 },	/* zerosuperior */
    { 0x2074, PSDRV_AGLGlyphNames +  933 },	/* foursuperior */
    { 0x2075, PSDRV_AGLGlyphNames +  926 },	/* fivesuperior */
    { 0x2076, PSDRV_AGLGlyphNames + 1166 },	/* sixsuperior */
    { 0x2077, PSDRV_AGLGlyphNames + 1158 },	/* sevensuperior */
    { 0x2078, PSDRV_AGLGlyphNames +  889 },	/* eightsuperior */
    { 0x2079, PSDRV_AGLGlyphNames + 1035 },	/* ninesuperior */
    { 0x207d, PSDRV_AGLGlyphNames + 1085 },	/* parenleftsuperior */
    { 0x207e, PSDRV_AGLGlyphNames + 1091 },	/* parenrightsuperior */
    { 0x207f, PSDRV_AGLGlyphNames + 1040 },	/* nsuperior */
    { 0x2080, PSDRV_AGLGlyphNames + 1254 },	/* zeroinferior */
    { 0x2081, PSDRV_AGLGlyphNames + 1065 },	/* oneinferior */
    { 0x2082, PSDRV_AGLGlyphNames + 1208 },	/* twoinferior */
    { 0x2083, PSDRV_AGLGlyphNames + 1190 },	/* threeinferior */
    { 0x2084, PSDRV_AGLGlyphNames +  931 },	/* fourinferior */
    { 0x2085, PSDRV_AGLGlyphNames +  924 },	/* fiveinferior */
    { 0x2086, PSDRV_AGLGlyphNames + 1164 },	/* sixinferior */
    { 0x2087, PSDRV_AGLGlyphNames + 1156 },	/* seveninferior */
    { 0x2088, PSDRV_AGLGlyphNames +  887 },	/* eightinferior */
    { 0x2089, PSDRV_AGLGlyphNames + 1033 },	/* nineinferior */
    { 0x208d, PSDRV_AGLGlyphNames + 1084 },	/* parenleftinferior */
    { 0x208e, PSDRV_AGLGlyphNames + 1090 },	/* parenrightinferior */
    { 0x20a1, PSDRV_AGLGlyphNames +  834 },	/* colonmonetary */
    { 0x20a3, PSDRV_AGLGlyphNames +  935 },	/* franc */
    { 0x20a4, PSDRV_AGLGlyphNames + 1006 },	/* lira */
    { 0x20a7, PSDRV_AGLGlyphNames + 1101 },	/* peseta */
    { 0x20aa, PSDRV_AGLGlyphNames +  685 },	/* afii57636 */
    { 0x20ab, PSDRV_AGLGlyphNames +  870 },	/* dong */
    { 0x20ac, PSDRV_AGLGlyphNames +   74 },	/* Euro */
    { 0x2105, PSDRV_AGLGlyphNames +  741 },	/* afii61248 */
    { 0x2111, PSDRV_AGLGlyphNames +  108 },	/* Ifraktur */
    { 0x2113, PSDRV_AGLGlyphNames +  742 },	/* afii61289 */
    { 0x2116, PSDRV_AGLGlyphNames +  743 },	/* afii61352 */
    { 0x2118, PSDRV_AGLGlyphNames + 1239 },	/* weierstrass */
    { 0x211c, PSDRV_AGLGlyphNames +  185 },	/* Rfraktur */
    { 0x211e, PSDRV_AGLGlyphNames + 1107 },	/* prescription */
    { 0x2122, PSDRV_AGLGlyphNames + 1198 },	/* trademark */
    { 0x2126, PSDRV_AGLGlyphNames +  164 },	/* Omega */
    { 0x212e, PSDRV_AGLGlyphNames +  902 },	/* estimated */
    { 0x2135, PSDRV_AGLGlyphNames +  751 },	/* aleph */
    { 0x2153, PSDRV_AGLGlyphNames + 1069 },	/* onethird */
    { 0x2154, PSDRV_AGLGlyphNames + 1211 },	/* twothirds */
    { 0x215b, PSDRV_AGLGlyphNames + 1062 },	/* oneeighth */
    { 0x215c, PSDRV_AGLGlyphNames + 1189 },	/* threeeighths */
    { 0x215d, PSDRV_AGLGlyphNames +  923 },	/* fiveeighths */
    { 0x215e, PSDRV_AGLGlyphNames + 1155 },	/* seveneighths */
    { 0x2190, PSDRV_AGLGlyphNames +  774 },	/* arrowleft */
    { 0x2191, PSDRV_AGLGlyphNames +  776 },	/* arrowup */
    { 0x2192, PSDRV_AGLGlyphNames +  775 },	/* arrowright */
    { 0x2193, PSDRV_AGLGlyphNames +  772 },	/* arrowdown */
    { 0x2194, PSDRV_AGLGlyphNames +  766 },	/* arrowboth */
    { 0x2195, PSDRV_AGLGlyphNames +  777 },	/* arrowupdn */
    { 0x21a8, PSDRV_AGLGlyphNames +  778 },	/* arrowupdnbse */
    { 0x21b5, PSDRV_AGLGlyphNames +  816 },	/* carriagereturn */
    { 0x21d0, PSDRV_AGLGlyphNames +  769 },	/* arrowdblleft */
    { 0x21d1, PSDRV_AGLGlyphNames +  771 },	/* arrowdblup */
    { 0x21d2, PSDRV_AGLGlyphNames +  770 },	/* arrowdblright */
    { 0x21d3, PSDRV_AGLGlyphNames +  768 },	/* arrowdbldown */
    { 0x21d4, PSDRV_AGLGlyphNames +  767 },	/* arrowdblboth */
    { 0x2200, PSDRV_AGLGlyphNames + 1224 },	/* universal */
    { 0x2202, PSDRV_AGLGlyphNames + 1093 },	/* partialdiff */
    { 0x2203, PSDRV_AGLGlyphNames +  912 },	/* existential */
    { 0x2205, PSDRV_AGLGlyphNames +  894 },	/* emptyset */
    { 0x2206, PSDRV_AGLGlyphNames +   45 },	/* Delta */
    { 0x2207, PSDRV_AGLGlyphNames +  944 },	/* gradient */
    { 0x2208, PSDRV_AGLGlyphNames +  890 },	/* element */
    { 0x2209, PSDRV_AGLGlyphNames + 1037 },	/* notelement */
    { 0x220b, PSDRV_AGLGlyphNames + 1175 },	/* suchthat */
    { 0x220f, PSDRV_AGLGlyphNames + 1108 },	/* product */
    { 0x2211, PSDRV_AGLGlyphNames + 1176 },	/* summation */
    { 0x2212, PSDRV_AGLGlyphNames + 1020 },	/* minus */
    { 0x2215, PSDRV_AGLGlyphNames +  934 },	/* fraction */
    { 0x2217, PSDRV_AGLGlyphNames +  783 },	/* asteriskmath */
    { 0x2219, PSDRV_AGLGlyphNames + 1096 },	/* periodcentered */
    { 0x221a, PSDRV_AGLGlyphNames + 1129 },	/* radical */
    { 0x221d, PSDRV_AGLGlyphNames + 1111 },	/* proportional */
    { 0x221e, PSDRV_AGLGlyphNames +  973 },	/* infinity */
    { 0x221f, PSDRV_AGLGlyphNames + 1073 },	/* orthogonal */
    { 0x2220, PSDRV_AGLGlyphNames +  757 },	/* angle */
    { 0x2227, PSDRV_AGLGlyphNames + 1008 },	/* logicaland */
    { 0x2228, PSDRV_AGLGlyphNames + 1010 },	/* logicalor */
    { 0x2229, PSDRV_AGLGlyphNames +  978 },	/* intersection */
    { 0x222a, PSDRV_AGLGlyphNames + 1223 },	/* union */
    { 0x222b, PSDRV_AGLGlyphNames +  974 },	/* integral */
    { 0x2234, PSDRV_AGLGlyphNames + 1184 },	/* therefore */
    { 0x223c, PSDRV_AGLGlyphNames + 1162 },	/* similar */
    { 0x2245, PSDRV_AGLGlyphNames +  839 },	/* congruent */
    { 0x2248, PSDRV_AGLGlyphNames +  763 },	/* approxequal */
    { 0x2260, PSDRV_AGLGlyphNames + 1038 },	/* notequal */
    { 0x2261, PSDRV_AGLGlyphNames +  901 },	/* equivalence */
    { 0x2264, PSDRV_AGLGlyphNames + 1004 },	/* lessequal */
    { 0x2265, PSDRV_AGLGlyphNames +  949 },	/* greaterequal */
    { 0x2282, PSDRV_AGLGlyphNames + 1109 },	/* propersubset */
    { 0x2283, PSDRV_AGLGlyphNames + 1110 },	/* propersuperset */
    { 0x2284, PSDRV_AGLGlyphNames + 1039 },	/* notsubset */
    { 0x2286, PSDRV_AGLGlyphNames + 1133 },	/* reflexsubset */
    { 0x2287, PSDRV_AGLGlyphNames + 1134 },	/* reflexsuperset */
    { 0x2295, PSDRV_AGLGlyphNames +  830 },	/* circleplus */
    { 0x2297, PSDRV_AGLGlyphNames +  829 },	/* circlemultiply */
    { 0x22a5, PSDRV_AGLGlyphNames + 1099 },	/* perpendicular */
    { 0x22c5, PSDRV_AGLGlyphNames +  875 },	/* dotmath */
    { 0x2302, PSDRV_AGLGlyphNames +  959 },	/* house */
    { 0x2310, PSDRV_AGLGlyphNames + 1139 },	/* revlogicalnot */
    { 0x2320, PSDRV_AGLGlyphNames +  977 },	/* integraltp */
    { 0x2321, PSDRV_AGLGlyphNames +  975 },	/* integralbt */
    { 0x2329, PSDRV_AGLGlyphNames +  758 },	/* angleleft */
    { 0x232a, PSDRV_AGLGlyphNames +  759 },	/* angleright */
    { 0x2500, PSDRV_AGLGlyphNames +  199 },	/* SF100000 */
    { 0x2502, PSDRV_AGLGlyphNames +  200 },	/* SF110000 */
    { 0x250c, PSDRV_AGLGlyphNames +  190 },	/* SF010000 */
    { 0x2510, PSDRV_AGLGlyphNames +  192 },	/* SF030000 */
    { 0x2514, PSDRV_AGLGlyphNames +  191 },	/* SF020000 */
    { 0x2518, PSDRV_AGLGlyphNames +  193 },	/* SF040000 */
    { 0x251c, PSDRV_AGLGlyphNames +  197 },	/* SF080000 */
    { 0x2524, PSDRV_AGLGlyphNames +  198 },	/* SF090000 */
    { 0x252c, PSDRV_AGLGlyphNames +  195 },	/* SF060000 */
    { 0x2534, PSDRV_AGLGlyphNames +  196 },	/* SF070000 */
    { 0x253c, PSDRV_AGLGlyphNames +  194 },	/* SF050000 */
    { 0x2550, PSDRV_AGLGlyphNames +  218 },	/* SF430000 */
    { 0x2551, PSDRV_AGLGlyphNames +  206 },	/* SF240000 */
    { 0x2552, PSDRV_AGLGlyphNames +  226 },	/* SF510000 */
    { 0x2553, PSDRV_AGLGlyphNames +  227 },	/* SF520000 */
    { 0x2554, PSDRV_AGLGlyphNames +  214 },	/* SF390000 */
    { 0x2555, PSDRV_AGLGlyphNames +  204 },	/* SF220000 */
    { 0x2556, PSDRV_AGLGlyphNames +  203 },	/* SF210000 */
    { 0x2557, PSDRV_AGLGlyphNames +  207 },	/* SF250000 */
    { 0x2558, PSDRV_AGLGlyphNames +  225 },	/* SF500000 */
    { 0x2559, PSDRV_AGLGlyphNames +  224 },	/* SF490000 */
    { 0x255a, PSDRV_AGLGlyphNames +  213 },	/* SF380000 */
    { 0x255b, PSDRV_AGLGlyphNames +  210 },	/* SF280000 */
    { 0x255c, PSDRV_AGLGlyphNames +  209 },	/* SF270000 */
    { 0x255d, PSDRV_AGLGlyphNames +  208 },	/* SF260000 */
    { 0x255e, PSDRV_AGLGlyphNames +  211 },	/* SF360000 */
    { 0x255f, PSDRV_AGLGlyphNames +  212 },	/* SF370000 */
    { 0x2560, PSDRV_AGLGlyphNames +  217 },	/* SF420000 */
    { 0x2561, PSDRV_AGLGlyphNames +  201 },	/* SF190000 */
    { 0x2562, PSDRV_AGLGlyphNames +  202 },	/* SF200000 */
    { 0x2563, PSDRV_AGLGlyphNames +  205 },	/* SF230000 */
    { 0x2564, PSDRV_AGLGlyphNames +  222 },	/* SF470000 */
    { 0x2565, PSDRV_AGLGlyphNames +  223 },	/* SF480000 */
    { 0x2566, PSDRV_AGLGlyphNames +  216 },	/* SF410000 */
    { 0x2567, PSDRV_AGLGlyphNames +  220 },	/* SF450000 */
    { 0x2568, PSDRV_AGLGlyphNames +  221 },	/* SF460000 */
    { 0x2569, PSDRV_AGLGlyphNames +  215 },	/* SF400000 */
    { 0x256a, PSDRV_AGLGlyphNames +  229 },	/* SF540000 */
    { 0x256b, PSDRV_AGLGlyphNames +  228 },	/* SF530000 */
    { 0x256c, PSDRV_AGLGlyphNames +  219 },	/* SF440000 */
    { 0x2580, PSDRV_AGLGlyphNames + 1227 },	/* upblock */
    { 0x2584, PSDRV_AGLGlyphNames +  865 },	/* dnblock */
    { 0x2588, PSDRV_AGLGlyphNames +  791 },	/* block */
    { 0x258c, PSDRV_AGLGlyphNames + 1005 },	/* lfblock */
    { 0x2590, PSDRV_AGLGlyphNames + 1143 },	/* rtblock */
    { 0x2591, PSDRV_AGLGlyphNames + 1015 },	/* ltshade */
    { 0x2592, PSDRV_AGLGlyphNames + 1159 },	/* shade */
    { 0x2593, PSDRV_AGLGlyphNames +  864 },	/* dkshade */
    { 0x25a0, PSDRV_AGLGlyphNames +  920 },	/* filledbox */
    { 0x25a1, PSDRV_AGLGlyphNames +   91 },	/* H22073 */
    { 0x25aa, PSDRV_AGLGlyphNames +   89 },	/* H18543 */
    { 0x25ab, PSDRV_AGLGlyphNames +   90 },	/* H18551 */
    { 0x25ac, PSDRV_AGLGlyphNames +  921 },	/* filledrect */
    { 0x25b2, PSDRV_AGLGlyphNames + 1204 },	/* triagup */
    { 0x25ba, PSDRV_AGLGlyphNames + 1203 },	/* triagrt */
    { 0x25bc, PSDRV_AGLGlyphNames + 1201 },	/* triagdn */
    { 0x25c4, PSDRV_AGLGlyphNames + 1202 },	/* triaglf */
    { 0x25ca, PSDRV_AGLGlyphNames + 1012 },	/* lozenge */
    { 0x25cb, PSDRV_AGLGlyphNames +  828 },	/* circle */
    { 0x25cf, PSDRV_AGLGlyphNames +   88 },	/* H18533 */
    { 0x25d8, PSDRV_AGLGlyphNames +  979 },	/* invbullet */
    { 0x25d9, PSDRV_AGLGlyphNames +  980 },	/* invcircle */
    { 0x25e6, PSDRV_AGLGlyphNames + 1070 },	/* openbullet */
    { 0x263a, PSDRV_AGLGlyphNames + 1168 },	/* smileface */
    { 0x263b, PSDRV_AGLGlyphNames +  981 },	/* invsmileface */
    { 0x263c, PSDRV_AGLGlyphNames + 1177 },	/* sun */
    { 0x2640, PSDRV_AGLGlyphNames +  914 },	/* female */
    { 0x2642, PSDRV_AGLGlyphNames + 1018 },	/* male */
    { 0x2660, PSDRV_AGLGlyphNames + 1170 },	/* spade */
    { 0x2663, PSDRV_AGLGlyphNames +  832 },	/* club */
    { 0x2665, PSDRV_AGLGlyphNames +  957 },	/* heart */
    { 0x2666, PSDRV_AGLGlyphNames +  858 },	/* diamond */
    { 0x266a, PSDRV_AGLGlyphNames + 1025 },	/* musicalnote */
    { 0x266b, PSDRV_AGLGlyphNames + 1026 },	/* musicalnotedbl */
    { 0xf6be, PSDRV_AGLGlyphNames +  874 },	/* dotlessj */
    { 0xf6bf, PSDRV_AGLGlyphNames +  126 },	/* LL */
    { 0xf6c0, PSDRV_AGLGlyphNames + 1007 },	/* ll */
    { 0xf6c1, PSDRV_AGLGlyphNames +  233 },	/* Scedilla */
    { 0xf6c2, PSDRV_AGLGlyphNames + 1148 },	/* scedilla */
    { 0xf6c3, PSDRV_AGLGlyphNames +  836 },	/* commaaccent */
    { 0xf6c4, PSDRV_AGLGlyphNames +  552 },	/* afii10063 */
    { 0xf6c5, PSDRV_AGLGlyphNames +  553 },	/* afii10064 */
    { 0xf6c6, PSDRV_AGLGlyphNames +  604 },	/* afii10192 */
    { 0xf6c7, PSDRV_AGLGlyphNames +  609 },	/* afii10831 */
    { 0xf6c8, PSDRV_AGLGlyphNames +  610 },	/* afii10832 */
    { 0xf6c9, PSDRV_AGLGlyphNames +    9 },	/* Acute */
    { 0xf6ca, PSDRV_AGLGlyphNames +   31 },	/* Caron */
    { 0xf6cb, PSDRV_AGLGlyphNames +   46 },	/* Dieresis */
    { 0xf6cc, PSDRV_AGLGlyphNames +   47 },	/* DieresisAcute */
    { 0xf6cd, PSDRV_AGLGlyphNames +   48 },	/* DieresisGrave */
    { 0xf6ce, PSDRV_AGLGlyphNames +   84 },	/* Grave */
    { 0xf6cf, PSDRV_AGLGlyphNames +   95 },	/* Hungarumlaut */
    { 0xf6d0, PSDRV_AGLGlyphNames +  136 },	/* Macron */
    { 0xf6d1, PSDRV_AGLGlyphNames +  844 },	/* cyrBreve */
    { 0xf6d2, PSDRV_AGLGlyphNames +  845 },	/* cyrFlex */
    { 0xf6d3, PSDRV_AGLGlyphNames +  851 },	/* dblGrave */
    { 0xf6d4, PSDRV_AGLGlyphNames +  846 },	/* cyrbreve */
    { 0xf6d5, PSDRV_AGLGlyphNames +  847 },	/* cyrflex */
    { 0xf6d6, PSDRV_AGLGlyphNames +  852 },	/* dblgrave */
    { 0xf6d7, PSDRV_AGLGlyphNames +  860 },	/* dieresisacute */
    { 0xf6d8, PSDRV_AGLGlyphNames +  861 },	/* dieresisgrave */
    { 0xf6d9, PSDRV_AGLGlyphNames +  842 },	/* copyrightserif */
    { 0xf6da, PSDRV_AGLGlyphNames + 1137 },	/* registerserif */
    { 0xf6db, PSDRV_AGLGlyphNames + 1200 },	/* trademarkserif */
    { 0xf6dc, PSDRV_AGLGlyphNames + 1063 },	/* onefitted */
    { 0xf6dd, PSDRV_AGLGlyphNames + 1144 },	/* rupiah */
    { 0xf6de, PSDRV_AGLGlyphNames + 1193 },	/* threequartersemdash */
    { 0xf6df, PSDRV_AGLGlyphNames +  824 },	/* centinferior */
    { 0xf6e0, PSDRV_AGLGlyphNames +  826 },	/* centsuperior */
    { 0xf6e1, PSDRV_AGLGlyphNames +  837 },	/* commainferior */
    { 0xf6e2, PSDRV_AGLGlyphNames +  838 },	/* commasuperior */
    { 0xf6e3, PSDRV_AGLGlyphNames +  867 },	/* dollarinferior */
    { 0xf6e4, PSDRV_AGLGlyphNames +  869 },	/* dollarsuperior */
    { 0xf6e5, PSDRV_AGLGlyphNames +  962 },	/* hypheninferior */
    { 0xf6e6, PSDRV_AGLGlyphNames +  963 },	/* hyphensuperior */
    { 0xf6e7, PSDRV_AGLGlyphNames + 1097 },	/* periodinferior */
    { 0xf6e8, PSDRV_AGLGlyphNames + 1098 },	/* periodsuperior */
    { 0xf6e9, PSDRV_AGLGlyphNames +  784 },	/* asuperior */
    { 0xf6ea, PSDRV_AGLGlyphNames +  811 },	/* bsuperior */
    { 0xf6eb, PSDRV_AGLGlyphNames +  877 },	/* dsuperior */
    { 0xf6ec, PSDRV_AGLGlyphNames +  903 },	/* esuperior */
    { 0xf6ed, PSDRV_AGLGlyphNames +  987 },	/* isuperior */
    { 0xf6ee, PSDRV_AGLGlyphNames + 1014 },	/* lsuperior */
    { 0xf6ef, PSDRV_AGLGlyphNames + 1022 },	/* msuperior */
    { 0xf6f0, PSDRV_AGLGlyphNames + 1076 },	/* osuperior */
    { 0xf6f1, PSDRV_AGLGlyphNames + 1142 },	/* rsuperior */
    { 0xf6f2, PSDRV_AGLGlyphNames + 1172 },	/* ssuperior */
    { 0xf6f3, PSDRV_AGLGlyphNames + 1205 },	/* tsuperior */
    { 0xf6f4, PSDRV_AGLGlyphNames +   27 },	/* Brevesmall */
    { 0xf6f5, PSDRV_AGLGlyphNames +   32 },	/* Caronsmall */
    { 0xf6f6, PSDRV_AGLGlyphNames +   40 },	/* Circumflexsmall */
    { 0xf6f7, PSDRV_AGLGlyphNames +   50 },	/* Dotaccentsmall */
    { 0xf6f8, PSDRV_AGLGlyphNames +   96 },	/* Hungarumlautsmall */
    { 0xf6f9, PSDRV_AGLGlyphNames +  133 },	/* Lslashsmall */
    { 0xf6fa, PSDRV_AGLGlyphNames +  150 },	/* OEsmall */
    { 0xf6fb, PSDRV_AGLGlyphNames +  158 },	/* Ogoneksmall */
    { 0xf6fc, PSDRV_AGLGlyphNames +  187 },	/* Ringsmall */
    { 0xf6fd, PSDRV_AGLGlyphNames +  232 },	/* Scaronsmall */
    { 0xf6fe, PSDRV_AGLGlyphNames +  246 },	/* Tildesmall */
    { 0xf6ff, PSDRV_AGLGlyphNames +  291 },	/* Zcaronsmall */
    { 0xf721, PSDRV_AGLGlyphNames +  911 },	/* exclamsmall */
    { 0xf724, PSDRV_AGLGlyphNames +  868 },	/* dollaroldstyle */
    { 0xf726, PSDRV_AGLGlyphNames +  756 },	/* ampersandsmall */
    { 0xf730, PSDRV_AGLGlyphNames + 1255 },	/* zerooldstyle */
    { 0xf731, PSDRV_AGLGlyphNames + 1066 },	/* oneoldstyle */
    { 0xf732, PSDRV_AGLGlyphNames + 1209 },	/* twooldstyle */
    { 0xf733, PSDRV_AGLGlyphNames + 1191 },	/* threeoldstyle */
    { 0xf734, PSDRV_AGLGlyphNames +  932 },	/* fouroldstyle */
    { 0xf735, PSDRV_AGLGlyphNames +  925 },	/* fiveoldstyle */
    { 0xf736, PSDRV_AGLGlyphNames + 1165 },	/* sixoldstyle */
    { 0xf737, PSDRV_AGLGlyphNames + 1157 },	/* sevenoldstyle */
    { 0xf738, PSDRV_AGLGlyphNames +  888 },	/* eightoldstyle */
    { 0xf739, PSDRV_AGLGlyphNames + 1034 },	/* nineoldstyle */
    { 0xf73f, PSDRV_AGLGlyphNames + 1117 },	/* questionsmall */
    { 0xf760, PSDRV_AGLGlyphNames +   85 },	/* Gravesmall */
    { 0xf761, PSDRV_AGLGlyphNames +   22 },	/* Asmall */
    { 0xf762, PSDRV_AGLGlyphNames +   28 },	/* Bsmall */
    { 0xf763, PSDRV_AGLGlyphNames +   41 },	/* Csmall */
    { 0xf764, PSDRV_AGLGlyphNames +   51 },	/* Dsmall */
    { 0xf765, PSDRV_AGLGlyphNames +   69 },	/* Esmall */
    { 0xf766, PSDRV_AGLGlyphNames +   76 },	/* Fsmall */
    { 0xf767, PSDRV_AGLGlyphNames +   86 },	/* Gsmall */
    { 0xf768, PSDRV_AGLGlyphNames +   94 },	/* Hsmall */
    { 0xf769, PSDRV_AGLGlyphNames +  116 },	/* Ismall */
    { 0xf76a, PSDRV_AGLGlyphNames +  120 },	/* Jsmall */
    { 0xf76b, PSDRV_AGLGlyphNames +  124 },	/* Ksmall */
    { 0xf76c, PSDRV_AGLGlyphNames +  134 },	/* Lsmall */
    { 0xf76d, PSDRV_AGLGlyphNames +  138 },	/* Msmall */
    { 0xf76e, PSDRV_AGLGlyphNames +  144 },	/* Nsmall */
    { 0xf76f, PSDRV_AGLGlyphNames +  171 },	/* Osmall */
    { 0xf770, PSDRV_AGLGlyphNames +  178 },	/* Psmall */
    { 0xf771, PSDRV_AGLGlyphNames +  180 },	/* Qsmall */
    { 0xf772, PSDRV_AGLGlyphNames +  188 },	/* Rsmall */
    { 0xf773, PSDRV_AGLGlyphNames +  237 },	/* Ssmall */
    { 0xf774, PSDRV_AGLGlyphNames +  247 },	/* Tsmall */
    { 0xf775, PSDRV_AGLGlyphNames +  267 },	/* Usmall */
    { 0xf776, PSDRV_AGLGlyphNames +  270 },	/* Vsmall */
    { 0xf777, PSDRV_AGLGlyphNames +  276 },	/* Wsmall */
    { 0xf778, PSDRV_AGLGlyphNames +  279 },	/* Xsmall */
    { 0xf779, PSDRV_AGLGlyphNames +  287 },	/* Ysmall */
    { 0xf77a, PSDRV_AGLGlyphNames +  294 },	/* Zsmall */
    { 0xf7a1, PSDRV_AGLGlyphNames +  910 },	/* exclamdownsmall */
    { 0xf7a2, PSDRV_AGLGlyphNames +  825 },	/* centoldstyle */
    { 0xf7a8, PSDRV_AGLGlyphNames +   49 },	/* Dieresissmall */
    { 0xf7af, PSDRV_AGLGlyphNames +  137 },	/* Macronsmall */
    { 0xf7b4, PSDRV_AGLGlyphNames +   10 },	/* Acutesmall */
    { 0xf7b8, PSDRV_AGLGlyphNames +   38 },	/* Cedillasmall */
    { 0xf7bf, PSDRV_AGLGlyphNames + 1116 },	/* questiondownsmall */
    { 0xf7e0, PSDRV_AGLGlyphNames +   14 },	/* Agravesmall */
    { 0xf7e1, PSDRV_AGLGlyphNames +    5 },	/* Aacutesmall */
    { 0xf7e2, PSDRV_AGLGlyphNames +    8 },	/* Acircumflexsmall */
    { 0xf7e3, PSDRV_AGLGlyphNames +   24 },	/* Atildesmall */
    { 0xf7e4, PSDRV_AGLGlyphNames +   12 },	/* Adieresissmall */
    { 0xf7e5, PSDRV_AGLGlyphNames +   21 },	/* Aringsmall */
    { 0xf7e6, PSDRV_AGLGlyphNames +    3 },	/* AEsmall */
    { 0xf7e7, PSDRV_AGLGlyphNames +   35 },	/* Ccedillasmall */
    { 0xf7e8, PSDRV_AGLGlyphNames +   63 },	/* Egravesmall */
    { 0xf7e9, PSDRV_AGLGlyphNames +   54 },	/* Eacutesmall */
    { 0xf7ea, PSDRV_AGLGlyphNames +   58 },	/* Ecircumflexsmall */
    { 0xf7eb, PSDRV_AGLGlyphNames +   60 },	/* Edieresissmall */
    { 0xf7ec, PSDRV_AGLGlyphNames +  110 },	/* Igravesmall */
    { 0xf7ed, PSDRV_AGLGlyphNames +  100 },	/* Iacutesmall */
    { 0xf7ee, PSDRV_AGLGlyphNames +  103 },	/* Icircumflexsmall */
    { 0xf7ef, PSDRV_AGLGlyphNames +  105 },	/* Idieresissmall */
    { 0xf7f0, PSDRV_AGLGlyphNames +   73 },	/* Ethsmall */
    { 0xf7f1, PSDRV_AGLGlyphNames +  146 },	/* Ntildesmall */
    { 0xf7f2, PSDRV_AGLGlyphNames +  160 },	/* Ogravesmall */
    { 0xf7f3, PSDRV_AGLGlyphNames +  152 },	/* Oacutesmall */
    { 0xf7f4, PSDRV_AGLGlyphNames +  155 },	/* Ocircumflexsmall */
    { 0xf7f5, PSDRV_AGLGlyphNames +  173 },	/* Otildesmall */
    { 0xf7f6, PSDRV_AGLGlyphNames +  157 },	/* Odieresissmall */
    { 0xf7f8, PSDRV_AGLGlyphNames +  170 },	/* Oslashsmall */
    { 0xf7f9, PSDRV_AGLGlyphNames +  257 },	/* Ugravesmall */
    { 0xf7fa, PSDRV_AGLGlyphNames +  250 },	/* Uacutesmall */
    { 0xf7fb, PSDRV_AGLGlyphNames +  253 },	/* Ucircumflexsmall */
    { 0xf7fc, PSDRV_AGLGlyphNames +  255 },	/* Udieresissmall */
    { 0xf7fd, PSDRV_AGLGlyphNames +  282 },	/* Yacutesmall */
    { 0xf7fe, PSDRV_AGLGlyphNames +  245 },	/* Thornsmall */
    { 0xf7ff, PSDRV_AGLGlyphNames +  285 },	/* Ydieresissmall */
    { 0xf8e5, PSDRV_AGLGlyphNames + 1130 },	/* radicalex */
    { 0xf8e6, PSDRV_AGLGlyphNames +  779 },	/* arrowvertex */
    { 0xf8e7, PSDRV_AGLGlyphNames +  773 },	/* arrowhorizex */
    { 0xf8e8, PSDRV_AGLGlyphNames + 1136 },	/* registersans */
    { 0xf8e9, PSDRV_AGLGlyphNames +  841 },	/* copyrightsans */
    { 0xf8ea, PSDRV_AGLGlyphNames + 1199 },	/* trademarksans */
    { 0xf8eb, PSDRV_AGLGlyphNames + 1086 },	/* parenlefttp */
    { 0xf8ec, PSDRV_AGLGlyphNames + 1083 },	/* parenleftex */
    { 0xf8ed, PSDRV_AGLGlyphNames + 1082 },	/* parenleftbt */
    { 0xf8ee, PSDRV_AGLGlyphNames +  804 },	/* bracketlefttp */
    { 0xf8ef, PSDRV_AGLGlyphNames +  803 },	/* bracketleftex */
    { 0xf8f0, PSDRV_AGLGlyphNames +  802 },	/* bracketleftbt */
    { 0xf8f1, PSDRV_AGLGlyphNames +  796 },	/* bracelefttp */
    { 0xf8f2, PSDRV_AGLGlyphNames +  795 },	/* braceleftmid */
    { 0xf8f3, PSDRV_AGLGlyphNames +  794 },	/* braceleftbt */
    { 0xf8f4, PSDRV_AGLGlyphNames +  792 },	/* braceex */
    { 0xf8f5, PSDRV_AGLGlyphNames +  976 },	/* integralex */
    { 0xf8f6, PSDRV_AGLGlyphNames + 1092 },	/* parenrighttp */
    { 0xf8f7, PSDRV_AGLGlyphNames + 1089 },	/* parenrightex */
    { 0xf8f8, PSDRV_AGLGlyphNames + 1088 },	/* parenrightbt */
    { 0xf8f9, PSDRV_AGLGlyphNames +  808 },	/* bracketrighttp */
    { 0xf8fa, PSDRV_AGLGlyphNames +  807 },	/* bracketrightex */
    { 0xf8fb, PSDRV_AGLGlyphNames +  806 },	/* bracketrightbt */
    { 0xf8fc, PSDRV_AGLGlyphNames +  800 },	/* bracerighttp */
    { 0xf8fd, PSDRV_AGLGlyphNames +  799 },	/* bracerightmid */
    { 0xf8fe, PSDRV_AGLGlyphNames +  798 },	/* bracerightbt */
    { 0xfb00, PSDRV_AGLGlyphNames +  915 },	/* ff */
    { 0xfb01, PSDRV_AGLGlyphNames +  918 },	/* fi */
    { 0xfb02, PSDRV_AGLGlyphNames +  927 },	/* fl */
    { 0xfb03, PSDRV_AGLGlyphNames +  916 },	/* ffi */
    { 0xfb04, PSDRV_AGLGlyphNames +  917 },	/* ffl */
    { 0xfb1f, PSDRV_AGLGlyphNames +  718 },	/* afii57705 */
    { 0xfb2a, PSDRV_AGLGlyphNames +  715 },	/* afii57694 */
    { 0xfb2b, PSDRV_AGLGlyphNames +  716 },	/* afii57695 */
    { 0xfb35, PSDRV_AGLGlyphNames +  722 },	/* afii57723 */
    { 0xfb4b, PSDRV_AGLGlyphNames +  717 }	/* afii57700 */
};

UNICODEVECTOR PSDRV_AdobeGlyphList = { 1051, encoding };


/*
 *  Built-in font metrics
 */
extern AFM PSDRV_AvantGarde_Book;
extern AFM PSDRV_AvantGarde_BookOblique;
extern AFM PSDRV_AvantGarde_Demi;
extern AFM PSDRV_AvantGarde_DemiOblique;
extern AFM PSDRV_Bookman_Demi;
extern AFM PSDRV_Bookman_DemiItalic;
extern AFM PSDRV_Bookman_Light;
extern AFM PSDRV_Bookman_LightItalic;
extern AFM PSDRV_Courier;
extern AFM PSDRV_Courier_Bold;
extern AFM PSDRV_Courier_BoldOblique;
extern AFM PSDRV_Courier_Oblique;
extern AFM PSDRV_Helvetica;
extern AFM PSDRV_Helvetica_Bold;
extern AFM PSDRV_Helvetica_BoldOblique;
extern AFM PSDRV_Helvetica_Condensed;
extern AFM PSDRV_Helvetica_Condensed_Bold;
extern AFM PSDRV_Helvetica_Condensed_BoldObl;
extern AFM PSDRV_Helvetica_Condensed_Oblique;
extern AFM PSDRV_Helvetica_Narrow;
extern AFM PSDRV_Helvetica_Narrow_Bold;
extern AFM PSDRV_Helvetica_Narrow_BoldOblique;
extern AFM PSDRV_Helvetica_Narrow_Oblique;
extern AFM PSDRV_Helvetica_Oblique;
extern AFM PSDRV_NewCenturySchlbk_Bold;
extern AFM PSDRV_NewCenturySchlbk_BoldItalic;
extern AFM PSDRV_NewCenturySchlbk_Italic;
extern AFM PSDRV_NewCenturySchlbk_Roman;
extern AFM PSDRV_Palatino_Bold;
extern AFM PSDRV_Palatino_BoldItalic;
extern AFM PSDRV_Palatino_Italic;
extern AFM PSDRV_Palatino_Roman;
extern AFM PSDRV_Symbol;
extern AFM PSDRV_Times_Bold;
extern AFM PSDRV_Times_BoldItalic;
extern AFM PSDRV_Times_Italic;
extern AFM PSDRV_Times_Roman;
extern AFM PSDRV_ZapfChancery_MediumItalic;
extern AFM PSDRV_ZapfDingbats;

AFM *const PSDRV_BuiltinAFMs[40] =
{
    &PSDRV_AvantGarde_Book,
    &PSDRV_AvantGarde_BookOblique,
    &PSDRV_AvantGarde_Demi,
    &PSDRV_AvantGarde_DemiOblique,
    &PSDRV_Bookman_Demi,
    &PSDRV_Bookman_DemiItalic,
    &PSDRV_Bookman_Light,
    &PSDRV_Bookman_LightItalic,
    &PSDRV_Courier,
    &PSDRV_Courier_Bold,
    &PSDRV_Courier_BoldOblique,
    &PSDRV_Courier_Oblique,
    &PSDRV_Helvetica,
    &PSDRV_Helvetica_Bold,
    &PSDRV_Helvetica_BoldOblique,
    &PSDRV_Helvetica_Condensed,
    &PSDRV_Helvetica_Condensed_Bold,
    &PSDRV_Helvetica_Condensed_BoldObl,
    &PSDRV_Helvetica_Condensed_Oblique,
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
