/*
 *	OLE2NLS library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1998  David Lee Lambert
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "windows.h"
#include "heap.h"
#include "ole.h"
#include "options.h"
#include "winnls.h"
#include "winreg.h"
#include "winerror.h"
#include "debug.h"

/* Locale name to id map. used by EnumSystemLocales, GetLocalInfoA 
 * MUST contain all #defines from winnls.h
 * last entry has NULL name, 0 id.
 */ 
#define LOCALE_ENTRY(x)	{#x,LOCALE_##x}
static struct tagLOCALE_NAME2ID {
	char	*name;
	DWORD	id;
} locale_name2id[]= {
	LOCALE_ENTRY(ILANGUAGE),
	LOCALE_ENTRY(SLANGUAGE),
	LOCALE_ENTRY(SENGLANGUAGE),
	LOCALE_ENTRY(SABBREVLANGNAME),
	LOCALE_ENTRY(SNATIVELANGNAME),
	LOCALE_ENTRY(ICOUNTRY),
	LOCALE_ENTRY(SCOUNTRY),
	LOCALE_ENTRY(SENGCOUNTRY),
	LOCALE_ENTRY(SABBREVCTRYNAME),
	LOCALE_ENTRY(SNATIVECTRYNAME),
	LOCALE_ENTRY(IDEFAULTLANGUAGE),
	LOCALE_ENTRY(IDEFAULTCOUNTRY),
	LOCALE_ENTRY(IDEFAULTCODEPAGE),
	LOCALE_ENTRY(IDEFAULTANSICODEPAGE),
	LOCALE_ENTRY(SLIST),
	LOCALE_ENTRY(IMEASURE),
	LOCALE_ENTRY(SDECIMAL),
	LOCALE_ENTRY(STHOUSAND),
	LOCALE_ENTRY(SGROUPING),
	LOCALE_ENTRY(IDIGITS),
	LOCALE_ENTRY(ILZERO),
	LOCALE_ENTRY(INEGNUMBER),
	LOCALE_ENTRY(SNATIVEDIGITS),
	LOCALE_ENTRY(SCURRENCY),
	LOCALE_ENTRY(SINTLSYMBOL),
	LOCALE_ENTRY(SMONDECIMALSEP),
	LOCALE_ENTRY(SMONTHOUSANDSEP),
	LOCALE_ENTRY(SMONGROUPING),
	LOCALE_ENTRY(ICURRDIGITS),
	LOCALE_ENTRY(IINTLCURRDIGITS),
	LOCALE_ENTRY(ICURRENCY),
	LOCALE_ENTRY(INEGCURR),
	LOCALE_ENTRY(SDATE),
	LOCALE_ENTRY(STIME),
	LOCALE_ENTRY(SSHORTDATE),
	LOCALE_ENTRY(SLONGDATE),
	LOCALE_ENTRY(STIMEFORMAT),
	LOCALE_ENTRY(IDATE),
	LOCALE_ENTRY(ILDATE),
	LOCALE_ENTRY(ITIME),
	LOCALE_ENTRY(ITIMEMARKPOSN),
	LOCALE_ENTRY(ICENTURY),
	LOCALE_ENTRY(ITLZERO),
	LOCALE_ENTRY(IDAYLZERO),
	LOCALE_ENTRY(IMONLZERO),
	LOCALE_ENTRY(S1159),
	LOCALE_ENTRY(S2359),
	LOCALE_ENTRY(ICALENDARTYPE),
	LOCALE_ENTRY(IOPTIONALCALENDAR),
	LOCALE_ENTRY(IFIRSTDAYOFWEEK),
	LOCALE_ENTRY(IFIRSTWEEKOFYEAR),
	LOCALE_ENTRY(SDAYNAME1),
	LOCALE_ENTRY(SDAYNAME2),
	LOCALE_ENTRY(SDAYNAME3),
	LOCALE_ENTRY(SDAYNAME4),
	LOCALE_ENTRY(SDAYNAME5),
	LOCALE_ENTRY(SDAYNAME6),
	LOCALE_ENTRY(SDAYNAME7),
	LOCALE_ENTRY(SABBREVDAYNAME1),
	LOCALE_ENTRY(SABBREVDAYNAME2),
	LOCALE_ENTRY(SABBREVDAYNAME3),
	LOCALE_ENTRY(SABBREVDAYNAME4),
	LOCALE_ENTRY(SABBREVDAYNAME5),
	LOCALE_ENTRY(SABBREVDAYNAME6),
	LOCALE_ENTRY(SABBREVDAYNAME7),
	LOCALE_ENTRY(SMONTHNAME1),
	LOCALE_ENTRY(SMONTHNAME2),
	LOCALE_ENTRY(SMONTHNAME3),
	LOCALE_ENTRY(SMONTHNAME4),
	LOCALE_ENTRY(SMONTHNAME5),
	LOCALE_ENTRY(SMONTHNAME6),
	LOCALE_ENTRY(SMONTHNAME7),
	LOCALE_ENTRY(SMONTHNAME8),
	LOCALE_ENTRY(SMONTHNAME9),
	LOCALE_ENTRY(SMONTHNAME10),
	LOCALE_ENTRY(SMONTHNAME11),
	LOCALE_ENTRY(SMONTHNAME12),
	LOCALE_ENTRY(SMONTHNAME13),
	LOCALE_ENTRY(SABBREVMONTHNAME1),
	LOCALE_ENTRY(SABBREVMONTHNAME2),
	LOCALE_ENTRY(SABBREVMONTHNAME3),
	LOCALE_ENTRY(SABBREVMONTHNAME4),
	LOCALE_ENTRY(SABBREVMONTHNAME5),
	LOCALE_ENTRY(SABBREVMONTHNAME6),
	LOCALE_ENTRY(SABBREVMONTHNAME7),
	LOCALE_ENTRY(SABBREVMONTHNAME8),
	LOCALE_ENTRY(SABBREVMONTHNAME9),
	LOCALE_ENTRY(SABBREVMONTHNAME10),
	LOCALE_ENTRY(SABBREVMONTHNAME11),
	LOCALE_ENTRY(SABBREVMONTHNAME12),
	LOCALE_ENTRY(SABBREVMONTHNAME13),
	LOCALE_ENTRY(SPOSITIVESIGN),
	LOCALE_ENTRY(SNEGATIVESIGN),
	LOCALE_ENTRY(IPOSSIGNPOSN),
	LOCALE_ENTRY(INEGSIGNPOSN),
	LOCALE_ENTRY(IPOSSYMPRECEDES),
	LOCALE_ENTRY(IPOSSEPBYSPACE),
	LOCALE_ENTRY(INEGSYMPRECEDES),
	LOCALE_ENTRY(INEGSEPBYSPACE),
/*	LOCALE_ENTRY(FONTSIGNATURE),*/
	{NULL,0},
};

const struct map_lcid2str {
	LCID		langid;
	const char	*langname;
} languages[]={
	{0x0401,"Arabisch"},
	{0x0402,"Bulgarisch"},
	{0x0403,"Katalanisch"},
	{0x0404,"Traditionales Chinesisch"},
	{0x0405,"Tschecisch"},
	{0x0406,"D‰nisch"},
	{0x0407,"Deutsch"},
	{0x0408,"Griechisch"},
	{0x0409,"Amerikanisches Englisch"},
	{0x040A,"Kastilisches Spanisch"},
	{0x040B,"Finnisch"},
	{0x040C,"Franzvsisch"},
	{0x040D,"Hebrdisch"},
	{0x040E,"Ungarisch"},
	{0x040F,"Isldndisch"},
	{0x0410,"Italienisch"},
	{0x0411,"Japanisch"},
	{0x0412,"Koreanisch"},
	{0x0413,"Niederldndisch"},
	{0x0414,"Norwegisch-Bokmal"},
	{0x0415,"Polnisch"},
	{0x0416,"Brasilianisches Portugiesisch"},
	{0x0417,"Rdtoromanisch"},
	{0x0418,"Rumdnisch"},
	{0x0419,"Russisch"},
	{0x041A,"Kroatoserbisch (lateinisch)"},
	{0x041B,"Slowenisch"},
	{0x041C,"Albanisch"},
	{0x041D,"Schwedisch"},
	{0x041E,"Thai"},
	{0x041F,"T|rkisch"},
	{0x0420,"Urdu"},
	{0x0421,"Bahasa"},
	{0x0804,"Vereinfachtes Chinesisch"},
	{0x0807,"Schweizerdeutsch"},
	{0x0809,"Britisches Englisch"},
	{0x080A,"Mexikanisches Spanisch"},
	{0x080C,"Belgisches Franzvsisch"},
	{0x0810,"Schweizerisches Italienisch"},
	{0x0813,"Belgisches Niederldndisch"},
	{0x0814,"Norgwegisch-Nynorsk"},
	{0x0816,"Portugiesisch"},
	{0x081A,"Serbokratisch (kyrillisch)"},
	{0x0C1C,"Kanadisches Franzvsisch"},
	{0x100C,"Schweizerisches Franzvsisch"},
	{0x0000,"Unbekannt"},
};

/***********************************************************************
 *           GetUserDefaultLCID       (OLE2NLS.1)
 */
LCID WINAPI GetUserDefaultLCID()
{
/* Default sorting, neutral sublanguage */
    switch(Options.language)
    {
 /* case LANG_Bu: return 0x02; */      /* Bulgarian */
 /* case LANG_Ch: return 0x04; */      /* Chinese */
    case LANG_Cz: return 0x05;         /* Czech */
    case LANG_Da: return 0x06;         /* Danish */
    case LANG_De: return 0x07;         /* German */
 /* case LANG_Gr: return 0x08; */      /* Greek */
    case LANG_En: return 0x09;         /* English */
    case LANG_Es: return 0x0a;         /* Spanish */
    case LANG_Fi: return 0x0b;         /* Finnish */
    case LANG_Fr: return 0x0c;         /* French */
    case LANG_Hu: return 0x0e;         /* Hungarian */
 /* case LANG_Ic: return 0x0f; */      /* Icelandic */
    case LANG_It: return 0x10;         /* Italian */
 /* case LANG_Jp: return 0x11; */      /* Japanese */
    case LANG_Ko: return 0x12;         /* Korean */
 /* case LANG_Du: return 0x13; */      /* Dutch */
    case LANG_No: return 0x14;         /* Norwegian */
    case LANG_Pl: return 0x15;         /* Polish */
    case LANG_Po: return 0x16;         /* Portuguese */
 /* case LANG_Ro: return 0x18; */      /* Romanian */
 /* case LANG_Ru: return 0x19; */      /* Russian */
 /* case LANG_Cr: return 0x1a; */      /* Croatian */
 /* case LANG_Sl: return 0x1b; */      /* Slovak */
    case LANG_Sw: return 0x1d;         /* Swedish */
 /* case LANG_Tu: return 0x1f; */      /* Turkish */
 /* case LANG_Sv: return 0x24; */      /* Slovenian */
    case LANG_Eo: return 0x25;         /* Esperanto (not official) */
    case LANG_Ca: return 0x26;         /* Catalan */

    default:
	return 0x00;                   /* Neutral language */
    }
}

/***********************************************************************
 *         GetSystemDefaultLCID       (OLE2NLS.2)
 */
LCID WINAPI GetSystemDefaultLCID()
{
	return GetUserDefaultLCID();
}

/***********************************************************************
 *         GetUserDefaultLangID       (OLE2NLS.3)
 */
LANGID WINAPI GetUserDefaultLangID()
{
	return (WORD)GetUserDefaultLCID();
}

/***********************************************************************
 *         GetSystemDefaultLangID     (OLE2NLS.4)
 */
LANGID WINAPI GetSystemDefaultLangID()
{
	return GetUserDefaultLangID();
}

/***********************************************************************
 *         GetLocaleInfoA             (OLE2NLS.5)
 * Is the last parameter really WORD for Win16?
 */
INT16 WINAPI GetLocaleInfo16(LCID lcid,LCTYPE LCType,LPSTR buf,INT16 len)
{
	return GetLocaleInfo32A(lcid,LCType,buf,len);
}

INT32 WINAPI GetLocaleInfo32A(LCID lcid,LCTYPE LCType,LPSTR buf,INT32 len)
{
	char	*retString;
	int	found,i;

	TRACE(ole,"(%8lX,%8lX,%p,%4X)\n",
			lcid,LCType,buf,len);

	LCType &= ~(LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP);

	/* As an option, we could obtain the value from win.ini.
	   This would not match the Wine compile-time option.
	   Also, not all identifiers are available from win.ini */
	retString=0;
	/* If we are through all of this, retLen should not be zero anymore.
	   If it is, the value is not supported */
	i=0;
	while (locale_name2id[i].name!=NULL) {
		if (LCType == locale_name2id[i].id) {
			retString = locale_name2id[i].name;
			break;
		}
		i++;
	}
	if (!retString) {
		WARN(ole,"Unkown LC type %lX\n",LCType);
		return 0;
	}

#define LOCVAL(type,value) case type:retString=value;break;

/* Now, the language specific definitions. They don't have to be
   complete */
    found=1;
    switch(Options.language)
    {
    case LANG_De:
    	switch (LCType) {
#include "nls/deu.nls"
	default: found=0;break;
	}
    break;  /* LANG(De) */

    case LANG_Da:
    	switch (LCType) {
#include "nls/dnk.nls"
	default: found=0;break;
	}
    break; /* LANG(Da) */

    case LANG_En:
    	switch (LCType) {
#include "nls/enu.nls"
	default: found=0;break;
	}
    break;  /* LANG(En) */

    case LANG_Eo:
    	switch (LCType) {
#include "nls/esperanto.nls"
	default: found=0;break;
	}
    break;  /* LANG(Eo) */

    case LANG_Fi:
    	switch (LCType) {
#include "nls/fin.nls"
	default: found=0;break;
	}
    break;  /* LANG(Fi) */

    case LANG_It:
    	switch (LCType) {
#include "nls/ita.nls"
	default: found=0;break;
	}
    break;  /* LANG(It) */

    case 0x0809:
    	switch (LCType) {
#include "nls/eng.nls"
	default: found=0;break;
	}
    break; /* LANG(0x0809) (U.K. English) */

    case LANG_Ko: /* string using codepage 949 */
        switch (LCType) {
#include "nls/kor.nls"
        default: found=0;break;
        }
    break;  /* LANG(Ko) */

    case LANG_Hu:
    	switch (LCType) {
#include "nls/hun.nls"
	default: found=0;break;
	}
    break;  /* LANG(En) */

    case LANG_Pl:
    	switch (LCType) {
#include "nls/plk.nls"
	default: found=0;break;
	}
    break;  /* LANG(Pl) */

    case LANG_Po:
    	switch (LCType) {
#include "nls/ptb.nls"
	default: found=0;break;
	}
    break; /* LANG(Po) */

    case LANG_Sw:
    	switch (LCType) {
#include "nls/sve.nls"
	default: found=0;break;
	}
    break; /* LANG(Sw) */

/*Insert other languages here*/

    default:
    	found=0;
	break;
    }  /* switch */

	if(!found) {
		ERR(ole,"'%s' not supported for your language.\n",
			retString);
		retString = "<WINE-NLS-unknown>";
		/*return 0;*/
	}
	if (buf)
		lstrcpyn32A(buf,retString,len);
	return strlen(retString)+1;
}

/***********************************************************************
 *         GetLocaleInfo32W             (KERNEL32.230)
 * Is the last parameter really WORD for Win16?
 */
INT32 WINAPI GetLocaleInfo32W(LCID lcid,LCTYPE LCType,LPWSTR wbuf,INT32 len)
{
	LPSTR abuf = (LPSTR)HeapAlloc(GetProcessHeap(),0,len);

	INT32 n = GetLocaleInfo32A(lcid, LCType, abuf, len);
	if (wbuf)
		lstrcpynAtoW(wbuf,abuf,len);
	HeapFree(GetProcessHeap(),0,abuf);
	return n;
}

/***********************************************************************
 *           SetLocaleInfoA       [KERNEL32.499]
 */
BOOL16 WINAPI SetLocaleInfoA(DWORD lcid, DWORD lctype, LPCSTR data)
{
    FIXME(ole,"(%ld,%ld,%s): stub\n",lcid,lctype,data);
    return TRUE;
}

/***********************************************************************
 *           IsValidLocale       [KERNEL32.361]
 */
BOOL32 WINAPI IsValidLocale(LCID lcid,DWORD flags)
{
	/* we support ANY language. Well, at least say that...*/
	return TRUE;
}

/***********************************************************************
 *              EnumSystemLocales32W                [KERNEL32.93]
 */
BOOL32 WINAPI EnumSystemLocales32W( LOCALE_ENUMPROC32W lpfnLocaleEnum,
                                    DWORD flags )
{
	int	i;
	BOOL32	ret;
	WCHAR	buffer[200];
	HKEY	xhkey;

	TRACE(win32,"(%p,%08lx)\n",lpfnLocaleEnum,flags );
	/* see if we can reuse the Win95 registry entries.... */
	if (ERROR_SUCCESS==RegOpenKey32A(HKEY_LOCAL_MACHINE,"\\System\\CurrentControlSet\\control\\Nls\\Locale\\",&xhkey)) {
		i=0;
		while (1) {
			if (ERROR_SUCCESS!=RegEnumKey32W(xhkey,i,buffer,sizeof(buffer)))
				break;
            		if (!lpfnLocaleEnum(buffer))
				break;
			i++;
		}
		RegCloseKey(xhkey);
		return TRUE;
	}

	i=0;
	while (languages[i].langname!=NULL)
        {
            LPWSTR cp;
	    char   xbuffer[10];
  	
	    sprintf(xbuffer,"%08lx",(DWORD)languages[i].langid);

	    cp = HEAP_strdupAtoW( GetProcessHeap(), 0, xbuffer );
            ret = lpfnLocaleEnum(cp);
            HeapFree( GetProcessHeap(), 0, cp );
            if (!ret) break;
            i++;
	}
	return TRUE;
}

/***********************************************************************
 *              EnumSystemLocales32A                [KERNEL32.92]
 */
BOOL32 WINAPI EnumSystemLocales32A(LOCALE_ENUMPROC32A lpfnLocaleEnum,
                                   DWORD flags)
{
	int	i;
	CHAR	buffer[200];
	HKEY	xhkey;

	TRACE(win32,"(%p,%08lx)\n",
		lpfnLocaleEnum,flags
	);
	if (ERROR_SUCCESS==RegOpenKey32A(HKEY_LOCAL_MACHINE,"\\System\\CurrentControlSet\\control\\Nls\\Locale\\",&xhkey)) {
		i=0;
		while (1) {
			if (ERROR_SUCCESS!=RegEnumKey32A(xhkey,i,buffer,sizeof(buffer)))
				break;
            		if (!lpfnLocaleEnum(buffer))
				break;
			i++;
		}
		RegCloseKey(xhkey);
		return TRUE;
	}
	i=0;
	while (languages[i].langname!=NULL) {
		sprintf(buffer,"%08lx",(DWORD)languages[i].langid);
		if (!lpfnLocaleEnum(buffer))
			break;
		i++;
	}
	return TRUE;
}

static const unsigned char CT_CType2_LUT[] = {
  C2_NOTAPPLICABLE, /*   -   0 */
  C2_NOTAPPLICABLE, /*   -   1 */
  C2_NOTAPPLICABLE, /*   -   2 */
  C2_NOTAPPLICABLE, /*   -   3 */
  C2_NOTAPPLICABLE, /*   -   4 */
  C2_NOTAPPLICABLE, /*   -   5 */
  C2_NOTAPPLICABLE, /*   -   6 */
  C2_NOTAPPLICABLE, /*   -   7 */
  C2_NOTAPPLICABLE, /*   -   8 */
  C2_SEGMENTSEPARATOR, /*   -   9 */
  C2_NOTAPPLICABLE, /*   -  10 */
  C2_NOTAPPLICABLE, /*   -  11 */
  C2_NOTAPPLICABLE, /*   -  12 */
  C2_NOTAPPLICABLE, /*   -  13 */
  C2_NOTAPPLICABLE, /*   -  14 */
  C2_NOTAPPLICABLE, /*   -  15 */
  C2_NOTAPPLICABLE, /*   -  16 */
  C2_NOTAPPLICABLE, /*   -  17 */
  C2_NOTAPPLICABLE, /*   -  18 */
  C2_NOTAPPLICABLE, /*   -  19 */
  C2_NOTAPPLICABLE, /*   -  20 */
  C2_NOTAPPLICABLE, /*   -  21 */
  C2_NOTAPPLICABLE, /*   -  22 */
  C2_NOTAPPLICABLE, /*   -  23 */
  C2_NOTAPPLICABLE, /*   -  24 */
  C2_NOTAPPLICABLE, /*   -  25 */
  C2_NOTAPPLICABLE, /*   -  26 */
  C2_NOTAPPLICABLE, /*   -  27 */
  C2_NOTAPPLICABLE, /*   -  28 */
  C2_NOTAPPLICABLE, /*   -  29 */
  C2_NOTAPPLICABLE, /*   -  30 */
  C2_NOTAPPLICABLE, /*   -  31 */
  C2_WHITESPACE, /*   -  32 */
  C2_OTHERNEUTRAL, /* ! -  33 */
  C2_OTHERNEUTRAL, /* " -  34 */ /* " */
  C2_EUROPETERMINATOR, /* # -  35 */
  C2_EUROPETERMINATOR, /* $ -  36 */
  C2_EUROPETERMINATOR, /* % -  37 */
  C2_LEFTTORIGHT, /* & -  38 */
  C2_OTHERNEUTRAL, /* ' -  39 */
  C2_OTHERNEUTRAL, /* ( -  40 */
  C2_OTHERNEUTRAL, /* ) -  41 */
  C2_OTHERNEUTRAL, /* * -  42 */
  C2_EUROPETERMINATOR, /* + -  43 */
  C2_COMMONSEPARATOR, /* , -  44 */
  C2_EUROPETERMINATOR, /* - -  45 */
  C2_EUROPESEPARATOR, /* . -  46 */
  C2_EUROPESEPARATOR, /* / -  47 */
  C2_EUROPENUMBER, /* 0 -  48 */
  C2_EUROPENUMBER, /* 1 -  49 */
  C2_EUROPENUMBER, /* 2 -  50 */
  C2_EUROPENUMBER, /* 3 -  51 */
  C2_EUROPENUMBER, /* 4 -  52 */
  C2_EUROPENUMBER, /* 5 -  53 */
  C2_EUROPENUMBER, /* 6 -  54 */
  C2_EUROPENUMBER, /* 7 -  55 */
  C2_EUROPENUMBER, /* 8 -  56 */
  C2_EUROPENUMBER, /* 9 -  57 */
  C2_COMMONSEPARATOR, /* : -  58 */
  C2_OTHERNEUTRAL, /* ; -  59 */
  C2_OTHERNEUTRAL, /* < -  60 */
  C2_OTHERNEUTRAL, /* = -  61 */
  C2_OTHERNEUTRAL, /* > -  62 */
  C2_OTHERNEUTRAL, /* ? -  63 */
  C2_LEFTTORIGHT, /* @ -  64 */
  C2_LEFTTORIGHT, /* A -  65 */
  C2_LEFTTORIGHT, /* B -  66 */
  C2_LEFTTORIGHT, /* C -  67 */
  C2_LEFTTORIGHT, /* D -  68 */
  C2_LEFTTORIGHT, /* E -  69 */
  C2_LEFTTORIGHT, /* F -  70 */
  C2_LEFTTORIGHT, /* G -  71 */
  C2_LEFTTORIGHT, /* H -  72 */
  C2_LEFTTORIGHT, /* I -  73 */
  C2_LEFTTORIGHT, /* J -  74 */
  C2_LEFTTORIGHT, /* K -  75 */
  C2_LEFTTORIGHT, /* L -  76 */
  C2_LEFTTORIGHT, /* M -  77 */
  C2_LEFTTORIGHT, /* N -  78 */
  C2_LEFTTORIGHT, /* O -  79 */
  C2_LEFTTORIGHT, /* P -  80 */
  C2_LEFTTORIGHT, /* Q -  81 */
  C2_LEFTTORIGHT, /* R -  82 */
  C2_LEFTTORIGHT, /* S -  83 */
  C2_LEFTTORIGHT, /* T -  84 */
  C2_LEFTTORIGHT, /* U -  85 */
  C2_LEFTTORIGHT, /* V -  86 */
  C2_LEFTTORIGHT, /* W -  87 */
  C2_LEFTTORIGHT, /* X -  88 */
  C2_LEFTTORIGHT, /* Y -  89 */
  C2_LEFTTORIGHT, /* Z -  90 */
  C2_OTHERNEUTRAL, /* [ -  91 */
  C2_OTHERNEUTRAL, /* \ -  92 */
  C2_OTHERNEUTRAL, /* ] -  93 */
  C2_OTHERNEUTRAL, /* ^ -  94 */
  C2_OTHERNEUTRAL, /* _ -  95 */
  C2_OTHERNEUTRAL, /* ` -  96 */
  C2_LEFTTORIGHT, /* a -  97 */
  C2_LEFTTORIGHT, /* b -  98 */
  C2_LEFTTORIGHT, /* c -  99 */
  C2_LEFTTORIGHT, /* d - 100 */
  C2_LEFTTORIGHT, /* e - 101 */
  C2_LEFTTORIGHT, /* f - 102 */
  C2_LEFTTORIGHT, /* g - 103 */
  C2_LEFTTORIGHT, /* h - 104 */
  C2_LEFTTORIGHT, /* i - 105 */
  C2_LEFTTORIGHT, /* j - 106 */
  C2_LEFTTORIGHT, /* k - 107 */
  C2_LEFTTORIGHT, /* l - 108 */
  C2_LEFTTORIGHT, /* m - 109 */
  C2_LEFTTORIGHT, /* n - 110 */
  C2_LEFTTORIGHT, /* o - 111 */
  C2_LEFTTORIGHT, /* p - 112 */
  C2_LEFTTORIGHT, /* q - 113 */
  C2_LEFTTORIGHT, /* r - 114 */
  C2_LEFTTORIGHT, /* s - 115 */
  C2_LEFTTORIGHT, /* t - 116 */
  C2_LEFTTORIGHT, /* u - 117 */
  C2_LEFTTORIGHT, /* v - 118 */
  C2_LEFTTORIGHT, /* w - 119 */
  C2_LEFTTORIGHT, /* x - 120 */
  C2_LEFTTORIGHT, /* y - 121 */
  C2_LEFTTORIGHT, /* z - 122 */
  C2_OTHERNEUTRAL, /* { - 123 */
  C2_OTHERNEUTRAL, /* | - 124 */
  C2_OTHERNEUTRAL, /* } - 125 */
  C2_OTHERNEUTRAL, /* ~ - 126 */
  C2_NOTAPPLICABLE, /*  - 127 */
  C2_NOTAPPLICABLE, /* Ä - 128 */
  C2_NOTAPPLICABLE, /* Å - 129 */
  C2_OTHERNEUTRAL, /* Ç - 130 */
  C2_LEFTTORIGHT, /* É - 131 */
  C2_OTHERNEUTRAL, /* Ñ - 132 */
  C2_OTHERNEUTRAL, /* Ö - 133 */
  C2_OTHERNEUTRAL, /* Ü - 134 */
  C2_OTHERNEUTRAL, /* á - 135 */
  C2_LEFTTORIGHT, /* à - 136 */
  C2_EUROPETERMINATOR, /* â - 137 */
  C2_LEFTTORIGHT, /* ä - 138 */
  C2_OTHERNEUTRAL, /* ã - 139 */
  C2_LEFTTORIGHT, /* å - 140 */
  C2_NOTAPPLICABLE, /* ç - 141 */
  C2_NOTAPPLICABLE, /* é - 142 */
  C2_NOTAPPLICABLE, /* è - 143 */
  C2_NOTAPPLICABLE, /* ê - 144 */
  C2_OTHERNEUTRAL, /* ë - 145 */
  C2_OTHERNEUTRAL, /* í - 146 */
  C2_OTHERNEUTRAL, /* ì - 147 */
  C2_OTHERNEUTRAL, /* î - 148 */
  C2_OTHERNEUTRAL, /* ï - 149 */
  C2_OTHERNEUTRAL, /* ñ - 150 */
  C2_OTHERNEUTRAL, /* ó - 151 */
  C2_LEFTTORIGHT, /* ò - 152 */
  C2_OTHERNEUTRAL, /* ô - 153 */
  C2_LEFTTORIGHT, /* ö - 154 */
  C2_OTHERNEUTRAL, /* õ - 155 */
  C2_LEFTTORIGHT, /* ú - 156 */
  C2_NOTAPPLICABLE, /* ù - 157 */
  C2_NOTAPPLICABLE, /* û - 158 */
  C2_LEFTTORIGHT, /* ü - 159 */
  C2_WHITESPACE, /* † - 160 */
  C2_OTHERNEUTRAL, /* ° - 161 */
  C2_EUROPETERMINATOR, /* ¢ - 162 */
  C2_EUROPETERMINATOR, /* £ - 163 */
  C2_EUROPETERMINATOR, /* § - 164 */
  C2_EUROPETERMINATOR, /* • - 165 */
  C2_OTHERNEUTRAL, /* ¶ - 166 */
  C2_OTHERNEUTRAL, /* ß - 167 */
  C2_OTHERNEUTRAL, /* ® - 168 */
  C2_OTHERNEUTRAL, /* © - 169 */
  C2_OTHERNEUTRAL, /* ™ - 170 */
  C2_OTHERNEUTRAL, /* ´ - 171 */
  C2_OTHERNEUTRAL, /* ¨ - 172 */
  C2_OTHERNEUTRAL, /* ≠ - 173 */
  C2_OTHERNEUTRAL, /* Æ - 174 */
  C2_OTHERNEUTRAL, /* Ø - 175 */
  C2_EUROPETERMINATOR, /* ∞ - 176 */
  C2_EUROPETERMINATOR, /* ± - 177 */
  C2_EUROPENUMBER, /* ≤ - 178 */
  C2_EUROPENUMBER, /* ≥ - 179 */
  C2_OTHERNEUTRAL, /* ¥ - 180 */
  C2_OTHERNEUTRAL, /* µ - 181 */
  C2_OTHERNEUTRAL, /* ∂ - 182 */
  C2_OTHERNEUTRAL, /* ∑ - 183 */
  C2_OTHERNEUTRAL, /* ∏ - 184 */
  C2_EUROPENUMBER, /* π - 185 */
  C2_OTHERNEUTRAL, /* ∫ - 186 */
  C2_OTHERNEUTRAL, /* ª - 187 */
  C2_OTHERNEUTRAL, /* º - 188 */
  C2_OTHERNEUTRAL, /* Ω - 189 */
  C2_OTHERNEUTRAL, /* æ - 190 */
  C2_OTHERNEUTRAL, /* ø - 191 */
  C2_LEFTTORIGHT, /* ¿ - 192 */
  C2_LEFTTORIGHT, /* ¡ - 193 */
  C2_LEFTTORIGHT, /* ¬ - 194 */
  C2_LEFTTORIGHT, /* √ - 195 */
  C2_LEFTTORIGHT, /* ƒ - 196 */
  C2_LEFTTORIGHT, /* ≈ - 197 */
  C2_LEFTTORIGHT, /* ∆ - 198 */
  C2_LEFTTORIGHT, /* « - 199 */
  C2_LEFTTORIGHT, /* » - 200 */
  C2_LEFTTORIGHT, /* … - 201 */
  C2_LEFTTORIGHT, /*   - 202 */
  C2_LEFTTORIGHT, /* À - 203 */
  C2_LEFTTORIGHT, /* Ã - 204 */
  C2_LEFTTORIGHT, /* Õ - 205 */
  C2_LEFTTORIGHT, /* Œ - 206 */
  C2_LEFTTORIGHT, /* œ - 207 */
  C2_LEFTTORIGHT, /* – - 208 */
  C2_LEFTTORIGHT, /* — - 209 */
  C2_LEFTTORIGHT, /* “ - 210 */
  C2_LEFTTORIGHT, /* ” - 211 */
  C2_LEFTTORIGHT, /* ‘ - 212 */
  C2_LEFTTORIGHT, /* ’ - 213 */
  C2_LEFTTORIGHT, /* ÷ - 214 */
  C2_OTHERNEUTRAL, /* ◊ - 215 */
  C2_LEFTTORIGHT, /* ÿ - 216 */
  C2_LEFTTORIGHT, /* Ÿ - 217 */
  C2_LEFTTORIGHT, /* ⁄ - 218 */
  C2_LEFTTORIGHT, /* € - 219 */
  C2_LEFTTORIGHT, /* ‹ - 220 */
  C2_LEFTTORIGHT, /* › - 221 */
  C2_LEFTTORIGHT, /* ﬁ - 222 */
  C2_LEFTTORIGHT, /* ﬂ - 223 */
  C2_LEFTTORIGHT, /* ‡ - 224 */
  C2_LEFTTORIGHT, /* · - 225 */
  C2_LEFTTORIGHT, /* ‚ - 226 */
  C2_LEFTTORIGHT, /* „ - 227 */
  C2_LEFTTORIGHT, /* ‰ - 228 */
  C2_LEFTTORIGHT, /* Â - 229 */
  C2_LEFTTORIGHT, /* Ê - 230 */
  C2_LEFTTORIGHT, /* Á - 231 */
  C2_LEFTTORIGHT, /* Ë - 232 */
  C2_LEFTTORIGHT, /* È - 233 */
  C2_LEFTTORIGHT, /* Í - 234 */
  C2_LEFTTORIGHT, /* Î - 235 */
  C2_LEFTTORIGHT, /* Ï - 236 */
  C2_LEFTTORIGHT, /* Ì - 237 */
  C2_LEFTTORIGHT, /* Ó - 238 */
  C2_LEFTTORIGHT, /* Ô - 239 */
  C2_LEFTTORIGHT, /*  - 240 */
  C2_LEFTTORIGHT, /* Ò - 241 */
  C2_LEFTTORIGHT, /* Ú - 242 */
  C2_LEFTTORIGHT, /* Û - 243 */
  C2_LEFTTORIGHT, /* Ù - 244 */
  C2_LEFTTORIGHT, /* ı - 245 */
  C2_LEFTTORIGHT, /* ˆ - 246 */
  C2_OTHERNEUTRAL, /* ˜ - 247 */
  C2_LEFTTORIGHT, /* ¯ - 248 */
  C2_LEFTTORIGHT, /* ˘ - 249 */
  C2_LEFTTORIGHT, /* ˙ - 250 */
  C2_LEFTTORIGHT, /* ˚ - 251 */
  C2_LEFTTORIGHT, /* ¸ - 252 */
  C2_LEFTTORIGHT, /* ˝ - 253 */
  C2_LEFTTORIGHT, /* ˛ - 254 */
  C2_LEFTTORIGHT /* ˇ - 255 */
};

static const WORD CT_CType3_LUT[] = { 
  0x0000, /*   -   0 */
  0x0000, /*   -   1 */
  0x0000, /*   -   2 */
  0x0000, /*   -   3 */
  0x0000, /*   -   4 */
  0x0000, /*   -   5 */
  0x0000, /*   -   6 */
  0x0000, /*   -   7 */
  0x0000, /*   -   8 */
  0x0008, /*   -   9 */
  0x0008, /*   -  10 */
  0x0008, /*   -  11 */
  0x0008, /*   -  12 */
  0x0008, /*   -  13 */
  0x0000, /*   -  14 */
  0x0000, /*   -  15 */
  0x0000, /*   -  16 */
  0x0000, /*   -  17 */
  0x0000, /*   -  18 */
  0x0000, /*   -  19 */
  0x0000, /*   -  20 */
  0x0000, /*   -  21 */
  0x0000, /*   -  22 */
  0x0000, /*   -  23 */
  0x0000, /*   -  24 */
  0x0000, /*   -  25 */
  0x0000, /*   -  26 */
  0x0000, /*   -  27 */
  0x0000, /*   -  28 */
  0x0000, /*   -  29 */
  0x0000, /*   -  30 */
  0x0000, /*   -  31 */
  0x0048, /*   -  32 */
  0x0048, /* ! -  33 */
  0x0448, /* " -  34 */ /* " */
  0x0048, /* # -  35 */
  0x0448, /* $ -  36 */
  0x0048, /* % -  37 */
  0x0048, /* & -  38 */
  0x0440, /* ' -  39 */
  0x0048, /* ( -  40 */
  0x0048, /* ) -  41 */
  0x0048, /* * -  42 */
  0x0048, /* + -  43 */
  0x0048, /* , -  44 */
  0x0440, /* - -  45 */
  0x0048, /* . -  46 */
  0x0448, /* / -  47 */
  0x0040, /* 0 -  48 */
  0x0040, /* 1 -  49 */
  0x0040, /* 2 -  50 */
  0x0040, /* 3 -  51 */
  0x0040, /* 4 -  52 */
  0x0040, /* 5 -  53 */
  0x0040, /* 6 -  54 */
  0x0040, /* 7 -  55 */
  0x0040, /* 8 -  56 */
  0x0040, /* 9 -  57 */
  0x0048, /* : -  58 */
  0x0048, /* ; -  59 */
  0x0048, /* < -  60 */
  0x0448, /* = -  61 */
  0x0048, /* > -  62 */
  0x0048, /* ? -  63 */
  0x0448, /* @ -  64 */
  0x8040, /* A -  65 */
  0x8040, /* B -  66 */
  0x8040, /* C -  67 */
  0x8040, /* D -  68 */
  0x8040, /* E -  69 */
  0x8040, /* F -  70 */
  0x8040, /* G -  71 */
  0x8040, /* H -  72 */
  0x8040, /* I -  73 */
  0x8040, /* J -  74 */
  0x8040, /* K -  75 */
  0x8040, /* L -  76 */
  0x8040, /* M -  77 */
  0x8040, /* N -  78 */
  0x8040, /* O -  79 */
  0x8040, /* P -  80 */
  0x8040, /* Q -  81 */
  0x8040, /* R -  82 */
  0x8040, /* S -  83 */
  0x8040, /* T -  84 */
  0x8040, /* U -  85 */
  0x8040, /* V -  86 */
  0x8040, /* W -  87 */
  0x8040, /* X -  88 */
  0x8040, /* Y -  89 */
  0x8040, /* Z -  90 */
  0x0048, /* [ -  91 */
  0x0448, /* \ -  92 */
  0x0048, /* ] -  93 */
  0x0448, /* ^ -  94 */
  0x0448, /* _ -  95 */
  0x0448, /* ` -  96 */
  0x8040, /* a -  97 */
  0x8040, /* b -  98 */
  0x8040, /* c -  99 */
  0x8040, /* d - 100 */
  0x8040, /* e - 101 */
  0x8040, /* f - 102 */
  0x8040, /* g - 103 */
  0x8040, /* h - 104 */
  0x8040, /* i - 105 */
  0x8040, /* j - 106 */
  0x8040, /* k - 107 */
  0x8040, /* l - 108 */
  0x8040, /* m - 109 */
  0x8040, /* n - 110 */
  0x8040, /* o - 111 */
  0x8040, /* p - 112 */
  0x8040, /* q - 113 */
  0x8040, /* r - 114 */
  0x8040, /* s - 115 */
  0x8040, /* t - 116 */
  0x8040, /* u - 117 */
  0x8040, /* v - 118 */
  0x8040, /* w - 119 */
  0x8040, /* x - 120 */
  0x8040, /* y - 121 */
  0x8040, /* z - 122 */
  0x0048, /* { - 123 */
  0x0048, /* | - 124 */
  0x0048, /* } - 125 */
  0x0448, /* ~ - 126 */
  0x0000, /*  - 127 */
  0x0000, /* Ä - 128 */
  0x0000, /* Å - 129 */
  0x0008, /* Ç - 130 */
  0x8000, /* É - 131 */
  0x0008, /* Ñ - 132 */
  0x0008, /* Ö - 133 */
  0x0008, /* Ü - 134 */
  0x0008, /* á - 135 */
  0x0001, /* à - 136 */
  0x0008, /* â - 137 */
  0x8003, /* ä - 138 */
  0x0008, /* ã - 139 */
  0x8000, /* å - 140 */
  0x0000, /* ç - 141 */
  0x0000, /* é - 142 */
  0x0000, /* è - 143 */
  0x0000, /* ê - 144 */
  0x0088, /* ë - 145 */
  0x0088, /* í - 146 */
  0x0088, /* ì - 147 */
  0x0088, /* î - 148 */
  0x0008, /* ï - 149 */
  0x0400, /* ñ - 150 */
  0x0400, /* ó - 151 */
  0x0408, /* ò - 152 */
  0x0000, /* ô - 153 */
  0x8003, /* ö - 154 */
  0x0008, /* õ - 155 */
  0x8000, /* ú - 156 */
  0x0000, /* ù - 157 */
  0x0000, /* û - 158 */
  0x8003, /* ü - 159 */
  0x0008, /* † - 160 */
  0x0008, /* ° - 161 */
  0x0048, /* ¢ - 162 */
  0x0048, /* £ - 163 */
  0x0008, /* § - 164 */
  0x0048, /* • - 165 */
  0x0048, /* ¶ - 166 */
  0x0008, /* ß - 167 */
  0x0408, /* ® - 168 */
  0x0008, /* © - 169 */
  0x0400, /* ™ - 170 */
  0x0008, /* ´ - 171 */
  0x0048, /* ¨ - 172 */
  0x0408, /* ≠ - 173 */
  0x0008, /* Æ - 174 */
  0x0448, /* Ø - 175 */
  0x0008, /* ∞ - 176 */
  0x0008, /* ± - 177 */
  0x0000, /* ≤ - 178 */
  0x0000, /* ≥ - 179 */
  0x0408, /* ¥ - 180 */
  0x0008, /* µ - 181 */
  0x0008, /* ∂ - 182 */
  0x0008, /* ∑ - 183 */
  0x0408, /* ∏ - 184 */
  0x0000, /* π - 185 */
  0x0400, /* ∫ - 186 */
  0x0008, /* ª - 187 */
  0x0000, /* º - 188 */
  0x0000, /* Ω - 189 */
  0x0000, /* æ - 190 */
  0x0008, /* ø - 191 */
  0x8003, /* ¿ - 192 */
  0x8003, /* ¡ - 193 */
  0x8003, /* ¬ - 194 */
  0x8003, /* √ - 195 */
  0x8003, /* ƒ - 196 */
  0x8003, /* ≈ - 197 */
  0x8000, /* ∆ - 198 */
  0x8003, /* « - 199 */
  0x8003, /* » - 200 */
  0x8003, /* … - 201 */
  0x8003, /*   - 202 */
  0x8003, /* À - 203 */
  0x8003, /* Ã - 204 */
  0x8003, /* Õ - 205 */
  0x8003, /* Œ - 206 */
  0x8003, /* œ - 207 */
  0x8000, /* – - 208 */
  0x8003, /* — - 209 */
  0x8003, /* “ - 210 */
  0x8003, /* ” - 211 */
  0x8003, /* ‘ - 212 */
  0x8003, /* ’ - 213 */
  0x8003, /* ÷ - 214 */
  0x0008, /* ◊ - 215 */
  0x8003, /* ÿ - 216 */
  0x8003, /* Ÿ - 217 */
  0x8003, /* ⁄ - 218 */
  0x8003, /* € - 219 */
  0x8003, /* ‹ - 220 */
  0x8003, /* › - 221 */
  0x8000, /* ﬁ - 222 */
  0x8000, /* ﬂ - 223 */
  0x8003, /* ‡ - 224 */
  0x8003, /* · - 225 */
  0x8003, /* ‚ - 226 */
  0x8003, /* „ - 227 */
  0x8003, /* ‰ - 228 */
  0x8003, /* Â - 229 */
  0x8000, /* Ê - 230 */
  0x8003, /* Á - 231 */
  0x8003, /* Ë - 232 */
  0x8003, /* È - 233 */
  0x8003, /* Í - 234 */
  0x8003, /* Î - 235 */
  0x8003, /* Ï - 236 */
  0x8003, /* Ì - 237 */
  0x8003, /* Ó - 238 */
  0x8003, /* Ô - 239 */
  0x8000, /*  - 240 */
  0x8003, /* Ò - 241 */
  0x8003, /* Ú - 242 */
  0x8003, /* Û - 243 */
  0x8003, /* Ù - 244 */
  0x8003, /* ı - 245 */
  0x8003, /* ˆ - 246 */
  0x0008, /* ˜ - 247 */
  0x8003, /* ¯ - 248 */
  0x8003, /* ˘ - 249 */
  0x8003, /* ˙ - 250 */
  0x8003, /* ˚ - 251 */
  0x8003, /* ¸ - 252 */
  0x8003, /* ˝ - 253 */
  0x8000, /* ˛ - 254 */
  0x8003  /* ˇ - 255 */
};

/***********************************************************************
 *              GetStringTypeA                [OLE2NLS.7]
 */
BOOL16 WINAPI GetStringType16(LCID locale,DWORD dwInfoType,LPCSTR src,
                              INT16 cchSrc,LPWORD chartype)
{
	return GetStringTypeEx32A(locale,dwInfoType,src,cchSrc,chartype);
}
/***********************************************************************
 *              GetStringTypeA                [KERNEL32.277]
 */
BOOL32 WINAPI GetStringType32A(LCID locale,DWORD dwInfoType,LPCSTR src,
                               INT32 cchSrc,LPWORD chartype)
{
	return GetStringTypeEx32A(locale,dwInfoType,src,cchSrc,chartype);
}

/***********************************************************************
 *              GetStringTypeExA                [KERNEL32.276]
 *
 * FIXME: Ignores the locale.
 */
BOOL32 WINAPI GetStringTypeEx32A(LCID locale,DWORD dwInfoType,LPCSTR src,
                                 INT32 cchSrc,LPWORD chartype)
{
	int	i;
	
	if ((src==NULL) || (chartype==NULL) || (src==(LPSTR)chartype))
	{
	  SetLastError(ERROR_INVALID_PARAMETER);
	  return FALSE;
	}

	if (cchSrc==-1)
	  cchSrc=lstrlen32A(src)+1;
	  
	switch (dwInfoType) {
	case CT_CTYPE1:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i] = 0;
	    if (isdigit(src[i])) chartype[i]|=C1_DIGIT;
	    if (isalpha(src[i])) chartype[i]|=C1_ALPHA;
	    if (islower(src[i])) chartype[i]|=C1_LOWER;
	    if (isupper(src[i])) chartype[i]|=C1_UPPER;
	    if (isspace(src[i])) chartype[i]|=C1_SPACE;
	    if (ispunct(src[i])) chartype[i]|=C1_PUNCT;
	    if (iscntrl(src[i])) chartype[i]|=C1_CNTRL;
/* FIXME: isblank() is a GNU extension */
/*		if (isblank(src[i])) chartype[i]|=C1_BLANK; */
	    if ((src[i] == ' ') || (src[i] == '\t')) chartype[i]|=C1_BLANK;
	    /* C1_XDIGIT */
	}
	return TRUE;

	case CT_CTYPE2:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i]=(WORD)CT_CType2_LUT[i];
	  }
	  return TRUE;

	case CT_CTYPE3:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i]=CT_CType3_LUT[i];
	  }
	  return TRUE;

	default:
	  ERR(ole,"Unknown dwInfoType:%ld\n",dwInfoType);
	  return FALSE;
	}
}

/*********************************************************************
 *  GetStringTypeW [KERNEL32.279]
 *
 * NOTES
 * Yes, this is missing LCID locale. MS fault.
 */
BOOL32 WINAPI GetStringType32W(DWORD dwInfoType,LPCWSTR src,INT32 cchSrc,
                               LPWORD chartype)
{
	return GetStringTypeEx32W(0/*defaultlocale*/,dwInfoType,src,cchSrc,chartype);
}

/*********************************************************************
 *   GetStringTypeW   [KERNEL32.278]
 * FIXME: unicode chars are assumed chars
 */
BOOL32 WINAPI GetStringTypeEx32W(LCID locale,DWORD dwInfoType,LPCWSTR src,
                                 INT32 cchSrc,LPWORD chartype)
{
	int	i;


	if (cchSrc==-1)
	  cchSrc=lstrlen32W(src)+1;
	
	switch (dwInfoType) {
	case CT_CTYPE2:
		FIXME(ole,"CT_CTYPE2 not supported.\n");
		return FALSE;
	case CT_CTYPE3:
		FIXME(ole,"CT_CTYPE3 not supported.\n");
		return FALSE;
	default:break;
	}
	for (i=0;i<cchSrc;i++) {
		chartype[i] = 0;
		if (isdigit(src[i])) chartype[i]|=C1_DIGIT;
		if (isalpha(src[i])) chartype[i]|=C1_ALPHA;
		if (islower(src[i])) chartype[i]|=C1_LOWER;
		if (isupper(src[i])) chartype[i]|=C1_UPPER;
		if (isspace(src[i])) chartype[i]|=C1_SPACE;
		if (ispunct(src[i])) chartype[i]|=C1_PUNCT;
		if (iscntrl(src[i])) chartype[i]|=C1_CNTRL;
/* FIXME: isblank() is a GNU extension */
/*		if (isblank(src[i])) chartype[i]|=C1_BLANK; */
                if ((src[i] == ' ') || (src[i] == '\t')) chartype[i]|=C1_BLANK;
		/* C1_XDIGIT */
	}
	return TRUE;
}

/*****************************************************************
 * VerLanguageName16   [VER.10] 
 */
DWORD WINAPI VerLanguageName16(UINT16 langid,LPSTR langname,UINT16 langnamelen)
{
	int	i;
	DWORD	result;
	char	buffer[80];

	TRACE(ver,"(%d,%p,%d)\n",langid,langname,langnamelen);
	/* First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
	 * from the registry. 
	 */
	sprintf(buffer,
		"\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
		langid);
	result = RegQueryValue16(HKEY_LOCAL_MACHINE, buffer, langname,
				(LPDWORD)&langnamelen);
	if (result == ERROR_SUCCESS) {
		langname[langnamelen-1]='\0';
		return langnamelen;
	}
	/* if that fails, use the internal table */
	for (i=0;languages[i].langid!=0;i++)
		if (langid==languages[i].langid)
			break;
	strncpy(langname,languages[i].langname,langnamelen);
	langname[langnamelen-1]='\0';
	return strlen(languages[i].langname);
}

/*****************************************************************
 * VerLanguageName32A				[VERSION.9] 
 */
DWORD WINAPI VerLanguageName32A(UINT32 langid,LPSTR langname,
                                UINT32 langnamelen)
{
	return VerLanguageName16(langid,langname,langnamelen);
}

/*****************************************************************
 * VerLanguageName32W				[VERSION.10] 
 */
DWORD WINAPI VerLanguageName32W(UINT32 langid,LPWSTR langname,
                                UINT32 langnamelen)
{
	int	i;
	LPWSTR	keyname;
	DWORD	result;
	char	buffer[80];

	/* First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
	 * from the registry. 
	 */
	sprintf(buffer,
		"\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
		langid);
	keyname = HEAP_strdupAtoW( GetProcessHeap(), 0, buffer );
	result = RegQueryValue32W(HKEY_LOCAL_MACHINE, keyname, langname,
				(LPDWORD)&langnamelen);
	HeapFree( GetProcessHeap(), 0, keyname );
	if (result != ERROR_SUCCESS) {
		/* if that fails, use the internal table */
		for (i=0;languages[i].langid!=0;i++)
			if (langid==languages[i].langid)
				break;
        	lstrcpyAtoW( langname, languages[i].langname );
		langnamelen = strlen(languages[i].langname);
	 	/* same as strlenW(langname); */
	}
	return langnamelen;
}
 
static int is_punctuation(unsigned char c) 
{
  /* punctuation characters are :
     1-8, 14-31, 39, 45, 127-129, 141-144, 150-151, 157-158, 173 

     "punctuation character" in this context is a character which is 
     considered "less important" during word sort comparison.
     See LCMapString for the precise definition of "less important". */
  if (c>=141)
  {
    if (c<=151)
      return (c<=144) || (c>=150);
    if (c<=158)
      return (c>=157);
    return (c==173);
  }
  if (c>=127)
    return (c<=129);
  if (c>=14)
    return (c<=31) || (c==39) || (c==45);
  return (c<=8);
}

static int identity(int c)
{
  return c;
}

static const unsigned char LCM_Unicode_LUT[] = {
  6      ,   3, /*   -   1 */  
  6      ,   4, /*   -   2 */  
  6      ,   5, /*   -   3 */  
  6      ,   6, /*   -   4 */  
  6      ,   7, /*   -   5 */  
  6      ,   8, /*   -   6 */  
  6      ,   9, /*   -   7 */  
  6      ,  10, /*   -   8 */  
  7      ,   5, /*   -   9 */  
  7      ,   6, /*   -  10 */  
  7      ,   7, /*   -  11 */  
  7      ,   8, /*   -  12 */  
  7      ,   9, /*   -  13 */  
  6      ,  11, /*   -  14 */  
  6      ,  12, /*   -  15 */  
  6      ,  13, /*   -  16 */  
  6      ,  14, /*   -  17 */  
  6      ,  15, /*   -  18 */  
  6      ,  16, /*   -  19 */  
  6      ,  17, /*   -  20 */  
  6      ,  18, /*   -  21 */  
  6      ,  19, /*   -  22 */  
  6      ,  20, /*   -  23 */  
  6      ,  21, /*   -  24 */  
  6      ,  22, /*   -  25 */  
  6      ,  23, /*   -  26 */  
  6      ,  24, /*   -  27 */  
  6      ,  25, /*   -  28 */  
  6      ,  26, /*   -  29 */  
  6      ,  27, /*   -  30 */  
  6      ,  28, /*   -  31 */  
  7      ,   2, /*   -  32 */
  7      ,  28, /* ! -  33 */
  7      ,  29, /* " -  34 */ /* " */
  7      ,  31, /* # -  35 */
  7      ,  33, /* $ -  36 */
  7      ,  35, /* % -  37 */
  7      ,  37, /* & -  38 */
  6      , 128, /* ' -  39 */
  7      ,  39, /* ( -  40 */
  7      ,  42, /* ) -  41 */
  7      ,  45, /* * -  42 */
  8      ,   3, /* + -  43 */
  7      ,  47, /* , -  44 */
  6      , 130, /* - -  45 */
  7      ,  51, /* . -  46 */
  7      ,  53, /* / -  47 */
 12      ,   3, /* 0 -  48 */
 12      ,  33, /* 1 -  49 */
 12      ,  51, /* 2 -  50 */
 12      ,  70, /* 3 -  51 */
 12      ,  88, /* 4 -  52 */
 12      , 106, /* 5 -  53 */
 12      , 125, /* 6 -  54 */
 12      , 144, /* 7 -  55 */
 12      , 162, /* 8 -  56 */
 12      , 180, /* 9 -  57 */
  7      ,  55, /* : -  58 */
  7      ,  58, /* ; -  59 */
  8      ,  14, /* < -  60 */
  8      ,  18, /* = -  61 */
  8      ,  20, /* > -  62 */
  7      ,  60, /* ? -  63 */
  7      ,  62, /* @ -  64 */
 14      ,   2, /* A -  65 */
 14      ,   9, /* B -  66 */
 14      ,  10, /* C -  67 */
 14      ,  26, /* D -  68 */
 14      ,  33, /* E -  69 */
 14      ,  35, /* F -  70 */
 14      ,  37, /* G -  71 */
 14      ,  44, /* H -  72 */
 14      ,  50, /* I -  73 */
 14      ,  53, /* J -  74 */
 14      ,  54, /* K -  75 */
 14      ,  72, /* L -  76 */
 14      ,  81, /* M -  77 */
 14      , 112, /* N -  78 */
 14      , 124, /* O -  79 */
 14      , 126, /* P -  80 */
 14      , 137, /* Q -  81 */
 14      , 138, /* R -  82 */
 14      , 145, /* S -  83 */
 14      , 153, /* T -  84 */
 14      , 159, /* U -  85 */
 14      , 162, /* V -  86 */
 14      , 164, /* W -  87 */
 14      , 166, /* X -  88 */
 14      , 167, /* Y -  89 */
 14      , 169, /* Z -  90 */
  7      ,  63, /* [ -  91 */
  7      ,  65, /* \ -  92 */
  7      ,  66, /* ] -  93 */
  7      ,  67, /* ^ -  94 */
  7      ,  68, /* _ -  95 */
  7      ,  72, /* ` -  96 */
 14      ,   2, /* a -  97 */
 14      ,   9, /* b -  98 */
 14      ,  10, /* c -  99 */
 14      ,  26, /* d - 100 */
 14      ,  33, /* e - 101 */
 14      ,  35, /* f - 102 */
 14      ,  37, /* g - 103 */
 14      ,  44, /* h - 104 */
 14      ,  50, /* i - 105 */
 14      ,  53, /* j - 106 */
 14      ,  54, /* k - 107 */
 14      ,  72, /* l - 108 */
 14      ,  81, /* m - 109 */
 14      , 112, /* n - 110 */
 14      , 124, /* o - 111 */
 14      , 126, /* p - 112 */
 14      , 137, /* q - 113 */
 14      , 138, /* r - 114 */
 14      , 145, /* s - 115 */
 14      , 153, /* t - 116 */
 14      , 159, /* u - 117 */
 14      , 162, /* v - 118 */
 14      , 164, /* w - 119 */
 14      , 166, /* x - 120 */
 14      , 167, /* y - 121 */
 14      , 169, /* z - 122 */
  7      ,  74, /* { - 123 */
  7      ,  76, /* | - 124 */
  7      ,  78, /* } - 125 */
  7      ,  80, /* ~ - 126 */
  6      ,  29, /*  - 127 */
  6      ,  30, /* Ä - 128 */
  6      ,  31, /* Å - 129 */
  7      , 123, /* Ç - 130 */
 14      ,  35, /* É - 131 */
  7      , 127, /* Ñ - 132 */
 10      ,  21, /* Ö - 133 */
 10      ,  15, /* Ü - 134 */
 10      ,  16, /* á - 135 */
  7      ,  67, /* à - 136 */
 10      ,  22, /* â - 137 */
 14      , 145, /* ä - 138 */
  7      , 136, /* ã - 139 */
 14 + 16 , 124, /* å - 140 */
  6      ,  43, /* ç - 141 */
  6      ,  44, /* é - 142 */
  6      ,  45, /* è - 143 */
  6      ,  46, /* ê - 144 */
  7      , 121, /* ë - 145 */
  7      , 122, /* í - 146 */
  7      , 125, /* ì - 147 */
  7      , 126, /* î - 148 */
 10      ,  17, /* ï - 149 */
  6      , 137, /* ñ - 150 */
  6      , 139, /* ó - 151 */
  7      ,  93, /* ò - 152 */
 14      , 156, /* ô - 153 */
 14      , 145, /* ö - 154 */
  7      , 137, /* õ - 155 */
 14 + 16 , 124, /* ú - 156 */
  6      ,  59, /* ù - 157 */
  6      ,  60, /* û - 158 */
 14      , 167, /* ü - 159 */
  7      ,   4, /* † - 160 */
  7      ,  81, /* ° - 161 */
 10      ,   2, /* ¢ - 162 */
 10      ,   3, /* £ - 163 */
 10      ,   4, /* § - 164 */
 10      ,   5, /* • - 165 */
  7      ,  82, /* ¶ - 166 */
 10      ,   6, /* ß - 167 */
  7      ,  83, /* ® - 168 */
 10      ,   7, /* © - 169 */
 14      ,   2, /* ™ - 170 */
  8      ,  24, /* ´ - 171 */
 10      ,   8, /* ¨ - 172 */
  6      , 131, /* ≠ - 173 */
 10      ,   9, /* Æ - 174 */
  7      ,  84, /* Ø - 175 */
 10      ,  10, /* ∞ - 176 */
  8      ,  23, /* ± - 177 */
 12      ,  51, /* ≤ - 178 */
 12      ,  70, /* ≥ - 179 */
  7      ,  85, /* ¥ - 180 */
 10      ,  11, /* µ - 181 */
 10      ,  12, /* ∂ - 182 */
 10      ,  13, /* ∑ - 183 */
  7      ,  86, /* ∏ - 184 */
 12      ,  33, /* π - 185 */
 14      , 124, /* ∫ - 186 */
  8      ,  26, /* ª - 187 */
 12      ,  21, /* º - 188 */
 12      ,  25, /* Ω - 189 */
 12      ,  29, /* æ - 190 */
  7      ,  87, /* ø - 191 */
 14      ,   2, /* ¿ - 192 */
 14      ,   2, /* ¡ - 193 */
 14      ,   2, /* ¬ - 194 */
 14      ,   2, /* √ - 195 */
 14      ,   2, /* ƒ - 196 */
 14      ,   2, /* ≈ - 197 */
 14 + 16 ,   2, /* ∆ - 198 */
 14      ,  10, /* « - 199 */
 14      ,  33, /* » - 200 */
 14      ,  33, /* … - 201 */
 14      ,  33, /*   - 202 */
 14      ,  33, /* À - 203 */
 14      ,  50, /* Ã - 204 */
 14      ,  50, /* Õ - 205 */
 14      ,  50, /* Œ - 206 */
 14      ,  50, /* œ - 207 */
 14      ,  26, /* – - 208 */
 14      , 112, /* — - 209 */
 14      , 124, /* “ - 210 */
 14      , 124, /* ” - 211 */
 14      , 124, /* ‘ - 212 */
 14      , 124, /* ’ - 213 */
 14      , 124, /* ÷ - 214 */
  8      ,  28, /* ◊ - 215 */
 14      , 124, /* ÿ - 216 */
 14      , 159, /* Ÿ - 217 */
 14      , 159, /* ⁄ - 218 */
 14      , 159, /* € - 219 */
 14      , 159, /* ‹ - 220 */
 14      , 167, /* › - 221 */
 14 + 32 , 153, /* ﬁ - 222 */
 14 + 48 , 145, /* ﬂ - 223 */
 14      ,   2, /* ‡ - 224 */
 14      ,   2, /* · - 225 */
 14      ,   2, /* ‚ - 226 */
 14      ,   2, /* „ - 227 */
 14      ,   2, /* ‰ - 228 */
 14      ,   2, /* Â - 229 */
 14 + 16 ,   2, /* Ê - 230 */
 14      ,  10, /* Á - 231 */
 14      ,  33, /* Ë - 232 */
 14      ,  33, /* È - 233 */
 14      ,  33, /* Í - 234 */
 14      ,  33, /* Î - 235 */
 14      ,  50, /* Ï - 236 */
 14      ,  50, /* Ì - 237 */
 14      ,  50, /* Ó - 238 */
 14      ,  50, /* Ô - 239 */
 14      ,  26, /*  - 240 */
 14      , 112, /* Ò - 241 */
 14      , 124, /* Ú - 242 */
 14      , 124, /* Û - 243 */
 14      , 124, /* Ù - 244 */
 14      , 124, /* ı - 245 */
 14      , 124, /* ˆ - 246 */
  8      ,  29, /* ˜ - 247 */
 14      , 124, /* ¯ - 248 */
 14      , 159, /* ˘ - 249 */
 14      , 159, /* ˙ - 250 */
 14      , 159, /* ˚ - 251 */
 14      , 159, /* ¸ - 252 */
 14      , 167, /* ˝ - 253 */
 14 + 32 , 153, /* ˛ - 254 */
 14      , 167  /* ˇ - 255 */ };

static const unsigned char LCM_Unicode_LUT_2[] = { 33, 44, 145 };

#define LCM_Diacritic_Start 131

static const unsigned char LCM_Diacritic_LUT[] = { 
123,  /* É - 131 */
  2,  /* Ñ - 132 */
  2,  /* Ö - 133 */
  2,  /* Ü - 134 */
  2,  /* á - 135 */
  3,  /* à - 136 */
  2,  /* â - 137 */
 20,  /* ä - 138 */
  2,  /* ã - 139 */
  2,  /* å - 140 */
  2,  /* ç - 141 */
  2,  /* é - 142 */
  2,  /* è - 143 */
  2,  /* ê - 144 */
  2,  /* ë - 145 */
  2,  /* í - 146 */
  2,  /* ì - 147 */
  2,  /* î - 148 */
  2,  /* ï - 149 */
  2,  /* ñ - 150 */
  2,  /* ó - 151 */
  2,  /* ò - 152 */
  2,  /* ô - 153 */
 20,  /* ö - 154 */
  2,  /* õ - 155 */
  2,  /* ú - 156 */
  2,  /* ù - 157 */
  2,  /* û - 158 */
 19,  /* ü - 159 */
  2,  /* † - 160 */
  2,  /* ° - 161 */
  2,  /* ¢ - 162 */
  2,  /* £ - 163 */
  2,  /* § - 164 */
  2,  /* • - 165 */
  2,  /* ¶ - 166 */
  2,  /* ß - 167 */
  2,  /* ® - 168 */
  2,  /* © - 169 */
  3,  /* ™ - 170 */
  2,  /* ´ - 171 */
  2,  /* ¨ - 172 */
  2,  /* ≠ - 173 */
  2,  /* Æ - 174 */
  2,  /* Ø - 175 */
  2,  /* ∞ - 176 */
  2,  /* ± - 177 */
  2,  /* ≤ - 178 */
  2,  /* ≥ - 179 */
  2,  /* ¥ - 180 */
  2,  /* µ - 181 */
  2,  /* ∂ - 182 */
  2,  /* ∑ - 183 */
  2,  /* ∏ - 184 */
  2,  /* π - 185 */
  3,  /* ∫ - 186 */
  2,  /* ª - 187 */
  2,  /* º - 188 */
  2,  /* Ω - 189 */
  2,  /* æ - 190 */
  2,  /* ø - 191 */
 15,  /* ¿ - 192 */
 14,  /* ¡ - 193 */
 18,  /* ¬ - 194 */
 25,  /* √ - 195 */
 19,  /* ƒ - 196 */
 26,  /* ≈ - 197 */
  2,  /* ∆ - 198 */
 28,  /* « - 199 */
 15,  /* » - 200 */
 14,  /* … - 201 */
 18,  /*   - 202 */
 19,  /* À - 203 */
 15,  /* Ã - 204 */
 14,  /* Õ - 205 */
 18,  /* Œ - 206 */
 19,  /* œ - 207 */
104,  /* – - 208 */
 25,  /* — - 209 */
 15,  /* “ - 210 */
 14,  /* ” - 211 */
 18,  /* ‘ - 212 */
 25,  /* ’ - 213 */
 19,  /* ÷ - 214 */
  2,  /* ◊ - 215 */
 33,  /* ÿ - 216 */
 15,  /* Ÿ - 217 */
 14,  /* ⁄ - 218 */
 18,  /* € - 219 */
 19,  /* ‹ - 220 */
 14,  /* › - 221 */
  2,  /* ﬁ - 222 */
  2,  /* ﬂ - 223 */
 15,  /* ‡ - 224 */
 14,  /* · - 225 */
 18,  /* ‚ - 226 */
 25,  /* „ - 227 */
 19,  /* ‰ - 228 */
 26,  /* Â - 229 */
  2,  /* Ê - 230 */
 28,  /* Á - 231 */
 15,  /* Ë - 232 */
 14,  /* È - 233 */
 18,  /* Í - 234 */
 19,  /* Î - 235 */
 15,  /* Ï - 236 */
 14,  /* Ì - 237 */
 18,  /* Ó - 238 */
 19,  /* Ô - 239 */
104,  /*  - 240 */
 25,  /* Ò - 241 */
 15,  /* Ú - 242 */
 14,  /* Û - 243 */
 18,  /* Ù - 244 */
 25,  /* ı - 245 */
 19,  /* ˆ - 246 */
  2,  /* ˜ - 247 */
 33,  /* ¯ - 248 */
 15,  /* ˘ - 249 */
 14,  /* ˙ - 250 */
 18,  /* ˚ - 251 */
 19,  /* ¸ - 252 */
 14,  /* ˝ - 253 */
  2,  /* ˛ - 254 */
 19,  /* ˇ - 255 */
} ;

/*************************************************************************
 * Convert a string, or generate a sort key from it.
 *
 * If (mapflags & LCMAP_SORTKEY), the function will generate
 * a sort key for the source string.  Else, it will convert it
 * accordingly to the flags LCMAP_UPPERCASE, LCMAP_LOWERCASE,...
 *
 * RETURNS
 *    Error : 0.
 *    Success : length of the result string.
 *
 * REMARKS
 *    If called with scrlen = -1, the function will compute the length
 *      of the 0-terminated string strsrc by itself.      
 * 
 *    If called with dstlen = 0, returns the buffer length that 
 *      would be required.
 */
INT32 WINAPI LCMapString32A(
	LCID lcid /* locale identifier created with MAKELCID; 
		     LOCALE_SYSTEM_DEFAULT and LOCALE_USER_DEFAULT are predefined
		     values. */,
	DWORD mapflags /* flags */,
	LPCSTR srcstr  /* source buffer */,
	INT32 srclen   /* source length */,
	LPSTR dststr   /* destination buffer */,
	INT32 dstlen   /* destination buffer length */) 
{
  int i;

  TRACE(string,"(0x%04lx,0x%08lx,%s,%d,%p,%d)\n",
	lcid,mapflags,srcstr,srclen,dststr,dstlen);

  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen==-1) 
    srclen = lstrlen32A(srcstr) + 1 ;    /* (include final '\0') */

  if (mapflags & ~ ( LCMAP_UPPERCASE | LCMAP_LOWERCASE | LCMAP_SORTKEY |
		     NORM_IGNORECASE | NORM_IGNORENONSPACE | SORT_STRINGSORT) )
  {
    FIXME(string,"(0x%04lx,0x%08lx,%p,%d,%p,%d): "
	  "unimplemented flags: 0x%08lx\n",
	  lcid,mapflags,srcstr,srclen,dststr,dstlen,mapflags);
  }
  
  if ( !(mapflags & LCMAP_SORTKEY) )
  {
    int (*f)(int)=identity; 

    if (dstlen==0)
      return srclen;  /* dstlen=0 means "do nothing but return required length" */
    if (dstlen<srclen)
      srclen=dstlen;  /* No, this case is not an error under Windows 95.
		         And no '\0' gets written. */
    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    for (i=0; i < srclen; i++)
      dststr[i] = (CHAR) f(srcstr[i]);
    return srclen;
  }

  /* else ... (mapflags & LCMAP_SORTKEY)  */
  {
    int unicode_len=0;
    int case_len=0;
    int diacritic_len=0;
    int delayed_punctuation_len=0;
    char *case_component;
    char *diacritic_component;
    char *delayed_punctuation_component;
    int room,count;
    int flag_stringsort = mapflags & SORT_STRINGSORT;

    /* compute how much room we will need */
    for (i=0;i<srclen;i++)
    {
      int ofs;
      unsigned char source_char = srcstr[i];
      if (source_char!='\0') 
      {
	if (flag_stringsort || !is_punctuation(source_char))
	{
	  unicode_len++;
	  if ( LCM_Unicode_LUT[-2+2*source_char] & ~15 )
	    unicode_len++;             /* double letter */
	}
	else
	{
	  delayed_punctuation_len++;
	}	  
      }
	  
      if (isupper(source_char))
	case_len=unicode_len; 

      ofs = source_char - LCM_Diacritic_Start;
      if ((ofs>=0) && (LCM_Diacritic_LUT[ofs]!=2))
	diacritic_len=unicode_len;
    }

    if (mapflags & NORM_IGNORECASE)
      case_len=0;                   
    if (mapflags & NORM_IGNORENONSPACE)
      diacritic_len=0;

    room =  2 * unicode_len              /* "unicode" component */
      +     diacritic_len                /* "diacritic" component */
      +     case_len                     /* "case" component */
      +     4 * delayed_punctuation_len  /* punctuation in word sort mode */
      +     4                            /* four '\1' separators */
      +     1  ;                         /* terminal '\0' */
    if (dstlen==0)
      return room;      
    else if (dstlen<room)
    {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }

    /*FIXME the Pointercheck should not be nessesary */
    if (IsBadWritePtr32 (dststr,room))
    { ERR (string,"bad destination buffer (dststr) : %p,%d\n",dststr,dstlen);
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }

    /* locate each component, write separators */
    diacritic_component = dststr + 2*unicode_len ;
    *diacritic_component++ = '\1'; 
    case_component = diacritic_component + diacritic_len ;
    *case_component++ = '\1'; 
    delayed_punctuation_component = case_component + case_len ;
    *delayed_punctuation_component++ = '\1';
    *delayed_punctuation_component++ = '\1';

    /* read source string char by char, write 
       corresponding weight in each component. */
    for (i=0,count=0;i<srclen;i++)
    {
      unsigned char source_char=srcstr[i];
      if (source_char!='\0') 
      {
	int type,longcode;
	type = LCM_Unicode_LUT[-2+2*source_char];
	longcode = type >> 4;
	type &= 15;
	if (!flag_stringsort && is_punctuation(source_char)) 
	{
	  UINT16 encrypted_location = (1<<15) + 7 + 4*count;
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location>>8);
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location&255);
                     /* big-endian is used here because it lets string comparison be
			compatible with numerical comparison */

	  *delayed_punctuation_component++ = type;
	  *delayed_punctuation_component++ = LCM_Unicode_LUT[-1+2*source_char];  
                     /* assumption : a punctuation character is never a 
			double or accented letter */
	}
	else
	{
	  dststr[2*count] = type;
	  dststr[2*count+1] = LCM_Unicode_LUT[-1+2*source_char];  
	  if (longcode)
	  {
	    if (count<case_len)
	      case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	    if (count<diacritic_len)
	      diacritic_component[count] = 2; /* assumption: a double letter
						 is never accented */
	    count++;
	    
	    dststr[2*count] = type;
	    dststr[2*count+1] = *(LCM_Unicode_LUT_2 - 1 + longcode); 
	    /* 16 in the first column of LCM_Unicode_LUT  -->  longcode = 1 
	       32 in the first column of LCM_Unicode_LUT  -->  longcode = 2 
	       48 in the first column of LCM_Unicode_LUT  -->  longcode = 3 */
	  }

	  if (count<case_len)
	    case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	  if (count<diacritic_len)
	  {
	    int ofs = source_char - LCM_Diacritic_Start;
	    diacritic_component[count] = (ofs>=0 ? LCM_Diacritic_LUT[ofs] : 2);
	  }
	  count++;
	}
      }
    }
    dststr[room-1] = '\0';
    return room;
  }
}
		     
INT32 WINAPI LCMapString32W(
	LCID lcid,DWORD mapflags,LPCWSTR srcstr,INT32 srclen,LPWSTR dststr,
	INT32 dstlen)
{
  int i;
 
  TRACE(string,"(0x%04lx,0x%08lx,%p,%d,%p,%d)\n",
	lcid,mapflags,srcstr,srclen,dststr,dstlen);

  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen==-1) 
    srclen = lstrlen32W(srcstr)+1;
  if (mapflags & LCMAP_SORTKEY) 
  {
    FIXME(string,"(0x%04lx,0x%08lx,%p,%d,%p,%d): "
	  "unimplemented flags: 0x%08lx\n",
	  lcid,mapflags,srcstr,srclen,dststr,dstlen,mapflags);
    return 0;
  }
  else
  {
    int (*f)(int)=identity; 

    if (dstlen==0)
      return srclen;  
    if (dstlen<srclen)
      return 0;       
    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    for (i=0; i < srclen; i++)
      dststr[i] = (WCHAR) f(srcstr[i]);
    return srclen;
  }
}

/***********************************************************************
 *           CompareString16       (OLE2NLS.8)
 */
UINT16 WINAPI CompareString16(DWORD lcid,DWORD fdwStyle,
                              LPCSTR s1,DWORD l1,LPCSTR s2,DWORD l2)
{
	return (UINT16)CompareString32A(lcid,fdwStyle,s1,l1,s2,l2);
}

/***********************************************************************
 *           CompareString32A   (KERNEL32.29)
 * This implementation ignores the locale
 * FIXME
 * Moreover it is quite inefficient. FIXME too!
 */
UINT32 WINAPI CompareString32A(DWORD lcid, DWORD fdwStyle, 
                               LPCSTR s1, DWORD l1, LPCSTR s2,DWORD l2)
{
  int mapstring_flags;
  int len1,len2;
  int result;
  LPSTR sk1,sk2;
  TRACE(ole,"%s and %s\n",
	debugstr_a (s1), debugstr_a (s2));

  if ( (s1==NULL) || (s2==NULL) )
  {    
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if(fdwStyle & NORM_IGNORESYMBOLS)
    FIXME(ole, "IGNORESYMBOLS not supported\n");
  	
  mapstring_flags = LCMAP_SORTKEY | fdwStyle ;
  len1 = LCMapString32A(lcid,mapstring_flags,s1,l1,NULL,0);
  len2 = LCMapString32A(lcid,mapstring_flags,s2,l2,NULL,0);

  if ((len1==0)||(len2==0))
    return 0;     /* something wrong happened */

  sk1 = (LPSTR)HeapAlloc(GetProcessHeap(),0,len1);
  sk2 = (LPSTR)HeapAlloc(GetProcessHeap(),0,len2);
  if ( (!LCMapString32A(lcid,mapstring_flags,s1,l1,sk1,len1))
	 || (!LCMapString32A(lcid,mapstring_flags,s2,l2,sk2,len2)) )
  { ERR(ole,"Bug in LCmapString32A.\n");
    result = 0;
  }
  else
  { result = strcmp(sk1,sk2)+2;
  }
  HeapFree(GetProcessHeap(),0,sk1);
  HeapFree(GetProcessHeap(),0,sk2);
  return result;
}

/***********************************************************************
 *           CompareString32W       (KERNEL32.30)
 * This implementation ignores the locale
 * FIXME :  Does only string sort.  Should
 * be reimplemented the same way as CompareString32A.
 */
UINT32 WINAPI CompareString32W(DWORD lcid, DWORD fdwStyle, 
                               LPCWSTR s1, DWORD l1, LPCWSTR s2,DWORD l2)
{
	int len,ret;
	if(fdwStyle & NORM_IGNORENONSPACE)
		FIXME(ole,"IGNORENONSPACE not supprted\n");
	if(fdwStyle & NORM_IGNORESYMBOLS)
		FIXME(ole,"IGNORESYMBOLS not supported\n");

	/* Is strcmp defaulting to string sort or to word sort?? */
	/* FIXME: Handle NORM_STRINGSORT */
	l1 = (l1==-1)?lstrlen32W(s1):l1;
	l2 = (l2==-1)?lstrlen32W(s2):l2;
	len = l1<l2 ? l1:l2;
	ret = (fdwStyle & NORM_IGNORECASE) ?
		lstrncmpi32W(s1,s2,len)	: lstrncmp32W(s1,s2,len);
	/* not equal, return 1 or 3 */
	if(ret!=0) return ret+2;
	/* same len, return 2 */
	if(l1==l2) return 2;
	/* the longer one is lexically greater */
	return (l1<l2)? 1 : 3;
}

/*****************************************************************
 *
 *  OLE_GetFormatA() [internal]


 This function implements stuff for GetDateFormat() and 
 GetTimeFormat().

  d    single-digit (no leading zero) day (of month)
  dd   two-digit day (of month)
  ddd  short day-of-week name
  dddd long day-of-week name
  M    single-digit month
  MM   two-digit month
  MMM  short month name
  MMMM full month name
  y    two-digit year, no leading 0
  yy   two-digit year
  yyyy four-digit year
  gg   era string
  h    hours with no leading zero (12-hour)
  hh   hours with full two digits
  H    hours with no leading zero (24-hour)
  HH   hours with full two digits
  m    minutes with no leading zero
  mm   minutes with full two digits
  s    seconds with no leading zero
  ss   seconds with full two digits
  t    time marker (A or P)
  tt   time marker (AM, PM)
  ''   used to quote literal characters
  ''   (within a quoted string) indicates a literal '

 These functions REQUIRE valid locale, date,  and format. 

 */

INT32 WINAPI OLE_GetFormatA(LCID locale,
			    DWORD flags,
			    LPSYSTEMTIME xtime,
			    LPCSTR format, 
			    LPSTR date, INT32 datelen)
{
   INT32 inpos, outpos;
   int count, type, inquote, Overflow;
   char buf[40];
   int buflen;

   const char * _dgfmt[] = { "%d", "%02d" };
   const char ** dgfmt = _dgfmt - 1; 

   /* report, for debugging */
   TRACE(ole, "func(%8lx,%8lx, time(d=%d,h=%d,m=%d,s=%d), fmt:\'%s\' (at %p), %p (%9s), len=%d)\n", locale, flags,
	 xtime->wDay, xtime->wHour, xtime->wMinute, xtime->wSecond,
	 format, format, date, date, datelen);
  
   /* initalize state variables and output buffer */
   inpos = outpos = 0;
   count = 0; inquote = 0; Overflow = 0;
   type = '\0';
   date[0] = buf[0] = '\0';
      
   for (inpos = 0;; inpos++) {
      /* TRACE(ole, "STATE inpos=%2d outpos=%2d count=%d inquote=%d type=%c buf,date = %c,%c\n", inpos, outpos, count, inquote, type, buf[inpos], date[outpos]); */
      if (inquote) {
	 if (format[inpos] == '\'') {
	    if (format[inpos+1] == '\'') {
	       inpos += 1;
	       date[outpos++] = '\'';
	    } else {
	       inquote = 0;
	       continue; /* we did nothing to the output */
	    }
	 } else if (format[inpos] == '\0') {
	    date[outpos++] = '\0';
	    if (outpos > datelen) Overflow = 1;
	    break;
	 } else {
	    date[outpos++] = format[inpos];
	    if (outpos > datelen) {
	       Overflow = 1;
	       date[outpos-1] = '\0'; /* this is the last place where
					 it's safe to write */
	       break;
	    }
	 }
      } else if (  (count && (format[inpos] != type))
		   || count == 4
		   || (count == 2 && strchr("ghHmst", type)) )
       {
	    if         (type == 'd') {
	       if        (count == 4) {
		  GetLocaleInfo32A(locale,
				   LOCALE_SDAYNAME1
				   + xtime->wDayOfWeek - 1,
				   buf, sizeof(buf));
	       } else if (count == 3) {
			   GetLocaleInfo32A(locale, 
					    LOCALE_SABBREVDAYNAME1 
					    + xtime->wDayOfWeek - 1,
					    buf, sizeof(buf));
		      } else {
		  sprintf(buf, dgfmt[count], xtime->wDay);
	       }
	    } else if (type == 'M') {
	       if (count == 3) {
		  GetLocaleInfo32A(locale, 
				   LOCALE_SABBREVMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
	       } else if (count == 4) {
		  GetLocaleInfo32A(locale,
				   LOCALE_SMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
		 } else {
		  sprintf(buf, dgfmt[count], xtime->wMonth);
	       }
	    } else if (type == 'y') {
	       if (count == 4) {
		      sprintf(buf, "%d", xtime->wYear);
	       } else if (count == 3) {
		  strcpy(buf, "yyy");
		  WARN(ole,
			      "unknown format,\
c=%c, n=%d\n",  type, count);
		 } else {
		  sprintf(buf, dgfmt[count], xtime->wYear % 100);
	       }
	    } else if (type == 'g') {
	       if        (count == 2) {
		  FIXME(ole, "LOCALE_ICALENDARTYPE unimp.\n");
		  strcpy(buf, "AD");
	    } else {
		  strcpy(buf, "g");
		  WARN(ole,
			       "unknown format, \
c=%c, n=%d\n", type, count);
	       }
	    } else if (type == 'h') {
	       /* gives us hours 1:00 -- 12:00 */
	       sprintf(buf, dgfmt[count], (xtime->wHour-1)%12 +1);
	    } else if (type == 'H') {
	       /* 24-hour time */
	       sprintf(buf, dgfmt[count], xtime->wHour);
	    } else if (type == 'm') {
	       sprintf(buf, dgfmt[count], xtime->wMinute);
	    } else if (type == 's') {
	       sprintf(buf, dgfmt[count], xtime->wSecond);
	    } else if (type == 't') {
	       if        (count == 1) {
		  sprintf(buf, "%c", (xtime->wHour < 12) ? 'A' : 'P');
	       } else if (count == 2) {
		  /* sprintf(buf, "%s", (xtime->wHour < 12) ? "AM" : "PM"); */
		  GetLocaleInfo32A(locale,
				   (xtime->wHour<12) 
				   ? LOCALE_S1159 : LOCALE_S2359,
				   buf, sizeof(buf));
	       }
	    };

	    /* we need to check the next char in the format string 
	       again, no matter what happened */
	    inpos--;
	    
	    /* add the contents of buf to the output */
	    buflen = strlen(buf);
	    if (outpos + buflen < datelen) {
	       date[outpos] = '\0'; /* for strcat to hook onto */
		 strcat(date, buf);
	       outpos += buflen;
	    } else {
	       date[outpos] = '\0';
	       strncat(date, buf, datelen - outpos);
		 date[datelen - 1] = '\0';
		 SetLastError(ERROR_INSUFFICIENT_BUFFER);
	       WARN(ole, "insufficient buffer\n");
		 return 0;
	    }

	    /* reset the variables we used to keep track of this item */
	    count = 0;
	    type = '\0';
	 } else if (format[inpos] == '\0') {
	    /* we can't check for this at the loop-head, because
	       that breaks the printing of the last format-item */
	    date[outpos] = '\0';
	    break;
         } else if (count) {
	    /* continuing a code for an item */
	    count +=1;
	    continue;
	 } else if (strchr("hHmstyMdg", format[inpos])) {
	    type = format[inpos];
	    count = 1;
	    continue;
	 } else if (format[inpos] == '\'') {
	    inquote = 1;
	    continue;
       } else {
	    date[outpos++] = format[inpos];
	 }
      /* now deal with a possible buffer overflow */
      if (outpos >= datelen) {
       date[datelen - 1] = '\0';
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return 0;
      }
   }
   
   if (Overflow) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
   };

   /* finish it off with a string terminator */
   outpos++;
   /* sanity check */
   if (outpos > datelen-1) outpos = datelen-1;
   date[outpos] = '\0';
   
   TRACE(ole, "OLE_GetFormatA returns string '%s', len %d\n",
	       date, outpos);
   return outpos;
}

/**************************************************************
 * OLE_GetFormatW  [internal]  
 */
INT32 WINAPI OLE_GetFormatW(LCID locale, DWORD flags,
			    LPSYSTEMTIME xtime,
			    LPCWSTR format,
			    LPWSTR output, INT32 outlen)
{
   INT32   inpos, outpos;
   int     count, type=0, inquote;
   int     Overflow; /* loop check */
   int usedate, usetime;
   WCHAR   buf[40];
   int     buflen=0;
   char    abuf[40];
   WCHAR   arg0[] = {0}, arg1[] = {'%','d',0};
   WCHAR   arg2[] = {'%','0','2','d',0};
   WCHAR  *argarr[] = {arg0, arg1, arg2};
   int     datevars=0, timevars=0;

   /* make a debug report */
   lstrcpynWtoA(abuf, format, sizeof(format));
   TRACE(ole, "args: %8lx, %8lx, time(d=%d,h=%d,m=%d,s=%d), fmt:\'%s\' (at %p), %p with max len %d\n",
	 locale, flags, 
	 xtime->wDay, xtime->wHour, xtime->wMinute, xtime->wSecond,
	 abuf, format, output, outlen);
   

   /* initialize state variables */
   inpos = outpos = 0;
   count = 0;
   inquote = Overflow = 0;
   /* this is really just a sanity check */
   output[0] = buf[0] = 0;
   abuf[0] = '\0';
   /* for compatibility with official Windows behavior */
   usedate = flags & DATE_DATEVARSONLY;
   usetime = flags & TIME_TIMEVARSONLY;
   
   /* this loop is the core of the function */
   for (inpos = 0; /* we have several break points */ ; inpos++) {
      if (inquote) {
	 if (format[inpos] == (WCHAR) '\'') {
	    if (format[inpos+1] == '\'') {
	       inpos++;
	       output[outpos++] = '\'';
	    } else {
	       inquote = 0;
	       continue;
	    }
	 } else if (format[inpos] == 0) {
	    output[outpos++] = 0;
	    if (outpos > outlen) Overflow = 1;
	    break;  /*  normal exit (within a quote) */
	 } else {
	    output[outpos++] = format[inpos]; /* copy input */
	    if (outpos > outlen) {
	       Overflow = 1;
	       output[outpos-1] = 0; 
	       break;
	    }
	 }
      } else if (  (count && (format[inpos] != type))
		   || ( (count==4 && type =='y') ||
			(count==4 && type =='M') ||
			(count==4 && type =='d') ||
			(count==2 && type =='g') ||
			(count==2 && type =='h') ||
			(count==2 && type =='H') ||
			(count==2 && type =='m') ||
			(count==2 && type =='s') ||
			(count==2 && type =='t') )  ) {
	 if        (type == 'd') {
	    if        (count == 3) {
	       GetLocaleInfo32W(locale,
			     LOCALE_SDAYNAME1 + xtime->wDayOfWeek -1,
			     buf, sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfo32W(locale,
				LOCALE_SABBREVDAYNAME1 +
				xtime->wDayOfWeek -1,
				buf, sizeof(buf)/sizeof(WCHAR) );
	    } else {
	       wsnprintf32W(buf, 5, argarr[count], xtime->wDay );
	    };
	 } else if (type == 'M') {
	    if        (count == 4) {
	       GetLocaleInfo32W(locale,  LOCALE_SMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfo32W(locale,  LOCALE_SABBREVMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else {
	       wsnprintf32W(buf, 5, argarr[count], xtime->wMonth);
	    }
	 } else if (type == 'y') {
	    if        (count == 4) {
	       wsnprintf32W(buf, 6, argarr[1] /* "%d" */,
			 xtime->wYear);
	    } else if (count == 3) {
	       lstrcpynAtoW(buf, "yyy", 5);
	    } else {
	       wsnprintf32W(buf, 6, argarr[count],
			    xtime->wYear % 100);
	    }
	 } else if (type == 'g') {
	    if        (count == 2) {
	       FIXME(ole, "LOCALE_ICALENDARTYPE unimplemented\n");
	       lstrcpynAtoW(buf, "AD", 5);
	    } else {
	       /* Win API sez we copy it verbatim */
	       lstrcpynAtoW(buf, "g", 5);
	    }
	 } else if (type == 'h') {
	    /* hours 1:00-12:00 --- is this right? */
	    wsnprintf32W(buf, 5, argarr[count], 
			 (xtime->wHour-1)%12 +1);
	 } else if (type == 'H') {
	    wsnprintf32W(buf, 5, argarr[count], 
			 xtime->wHour);
	 } else if (type == 'm') {
	    wsnprintf32W(buf, 5, argarr[count],
			 xtime->wMinute);
	 } else if (type == 's') {
	    wsnprintf32W(buf, 5, argarr[count],
			 xtime->wSecond);
	 } else if (type == 't') {
	    GetLocaleInfo32W(locale, (xtime->wHour < 12) ?
			     LOCALE_S1159 : LOCALE_S2359,
			     buf, sizeof(buf) );
	    if        (count == 1) {
	       buf[1] = 0;
	    }
}

	 /* no matter what happened,  we need to check this next 
	    character the next time we loop through */
	 inpos--;

	 /* cat buf onto the output */
	 outlen = lstrlen32W(buf);
	 if (outpos + buflen < outlen) {
	    output[outpos] = 0;  /* a "hook" for strcat */
	    lstrcat32W(output, buf);
	    outpos += buflen;
	 } else {
	    output[outpos] = 0;
	    lstrcatn32W(output, buf, outlen - outpos);
	    output[outlen - 1] = 0;
	    Overflow = 1;
	    break; /* Abnormal exit */
	 }

	 /* reset the variables we used this time */
	 count = 0;
	 type = '\0';
      } else if (format[inpos] == 0) {
	 /* we can't check for this at the beginning,  because that 
	 would keep us from printing a format spec that ended the 
	 string */
	 output[outpos] = 0;
	 break;  /*  NORMAL EXIT  */
      } else if (count) {
	 /* how we keep track of the middle of a format spec */
	 count++;
	 continue;
      } else if ( (datevars && (format[inpos]=='d' ||
				format[inpos]=='M' ||
				format[inpos]=='y' ||
				format[inpos]=='g')  ) ||
		  (timevars && (format[inpos]=='H' ||
				format[inpos]=='h' ||
				format[inpos]=='m' ||
				format[inpos]=='s' ||
				format[inpos]=='t') )    ) {
	 type = format[inpos];
	 count = 1;
	 continue;
      } else if (format[inpos] == '\'') {
	 inquote = 1;
	 continue;
      } else {
	 /* unquoted literals */
	 output[outpos++] = format[inpos];
      }
   }

   if (Overflow) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      WARN(ole, " buffer overflow\n");
   };

   /* final string terminator and sanity check */
   outpos++;
   if (outpos > outlen-1) outpos = outlen-1;
   output[outpos] = '0';

   lstrcpynWtoA(abuf, output, sizeof(abuf) );
   TRACE(ole, " returning string \'%s\'\n", abuf);
	
   return (!Overflow) ? outlen : 0;
   
}


/*****************************************************************
 *  GetDateFormat32A() [KERNEL32.310] Makes an ASCII string of the date
 *
 * This function uses format to format the date,  or,  if format
 * is NULL, uses the default for the locale.  format is a string
 * of literal fields and characters as follows:
 *
 * - d    single-digit (no leading zero) day (of month)
 * - dd   two-digit day (of month)
 * - ddd  short day-of-week name
 * - dddd long day-of-week name
 * - M    single-digit month
 * - MM   two-digit month
 * - MMM  short month name
 * - MMMM full month name
 * - y    two-digit year, no leading 0
 * - yy   two-digit year
 * - yyyy four-digit year
 * - gg   era string
 *
 */

INT32 WINAPI GetDateFormat32A(LCID locale,DWORD flags,
			      LPSYSTEMTIME xtime,
			      LPCSTR format, LPSTR date,INT32 datelen) 
{
   
  char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale;

  INT32 ret;

  TRACE(ole,"(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",
	      locale,flags,xtime,format,date,datelen);
  
  if (!locale) {
     locale = LOCALE_SYSTEM_DEFAULT;
     };
  
  if (locale == LOCALE_SYSTEM_DEFAULT) {
     thislocale = GetSystemDefaultLCID();
  } else if (locale == LOCALE_USER_DEFAULT) {
     thislocale = GetUserDefaultLCID();
  } else {
     thislocale = locale;
   };

  if (xtime == NULL) {
     GetSystemTime(&t);
     thistime = &t;
  } else {
     thistime = xtime;
  };

  if (format == NULL) {
     GetLocaleInfo32A(thislocale, ((flags&DATE_LONGDATE) 
				   ? LOCALE_SLONGDATE
				   : LOCALE_SSHORTDATE),
		      format_buf, sizeof(format_buf));
     thisformat = format_buf;
  } else {
     thisformat = format;
  };

  
  ret = OLE_GetFormatA(thislocale, flags, thistime, thisformat, 
		       date, datelen);
  

   TRACE(ole, 
	       "GetDateFormat32A() returning %d, with data=%s\n",
	       ret, date);
  return ret;
}

/* ****************************************************************
 * GetDateFormat32W() [KERNEL32.311] Makes a Unicode string of the date
 *
 * Acts the same as GetDateFormat32A(),  except that it's Unicode.
 * Accepts & returns sizes as counts of Unicode characters.
 *
 */
INT32 WINAPI GetDateFormat32W(LCID locale,DWORD flags,
			      LPSYSTEMTIME xtime,
			      LPCWSTR format,
			      LPWSTR date, INT32 datelen)
{
   short datearr[] = {'1','9','9','4','-','1','-','1',0};

   FIXME(ole, "STUB (should call OLE_GetFormatW)\n");   
   lstrcpyn32W(date, datearr, datelen);
   return (  datelen < 9) ? datelen : 9;
   
   
}


/**************************************************************************
 *              GetNumberFormat32A	(KERNEL32.355)
 * NOTE: type of lpFormat should be CONST NUMBERFORMAT 
 */
INT32 WINAPI GetNumberFormat32A(LCID locale, DWORD dwflags,
			       LPCSTR lpvalue,  char *lpFormat,
			       LPSTR lpNumberStr, int cchNumber)
{
 int n;

 FIXME(file,"%s: stub, no reformating done\n",lpvalue);

 n = strlen(lpvalue);
 if (cchNumber) { 
   strncpy(lpNumberStr,lpvalue,cchNumber);
   if (cchNumber <= n) {
     lpNumberStr[cchNumber-1] = 0;
     n = cchNumber-1;
   }
 }
 return n;
}
 


/*****************************************************************
 *
 * GetTimeFormat32A() [KERNEL32.422] Makes an ASCII string of the time
 *
 * Formats date according to format,  or locale default if format is
 * NULL. The format consists of literal characters and fields as follows:
 *
 * h  hours with no leading zero (12-hour)
 * hh hours with full two digits
 * H  hours with no leading zero (24-hour)
 * HH hours with full two digits
 * m  minutes with no leading zero
 * mm minutes with full two digits
 * s  seconds with no leading zero
 * ss seconds with full two digits
 * t  time marker (A or P)
 * tt time marker (AM, PM)
 *
 */

INT32 WINAPI 
GetTimeFormat32A(LCID locale,        /* in  */
		 DWORD flags,        /* in  */
		 LPSYSTEMTIME xtime, /* in  */ 
		 LPCSTR format,      /* in  */
		 LPSTR timestr,      /* out */
		 INT32 timelen       /* in  */) 
{ char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale=0;
  DWORD thisflags=LOCALE_STIMEFORMAT; /* standart timeformat */;
  
  TRACE(ole,"GetTimeFormat(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",locale,flags,xtime,format,timestr,timelen);
  if (!locale) 
  { locale = LOCALE_SYSTEM_DEFAULT;
  }
  
  if (locale == LOCALE_SYSTEM_DEFAULT) 
  { thislocale = GetSystemDefaultLCID();
  } 
  else if (locale == LOCALE_USER_DEFAULT) 
  { thislocale = GetUserDefaultLCID();
	}
  else
  { thislocale = locale;
  }
  if (format == NULL) 
  { if (flags&LOCALE_NOUSEROVERRIDE)  /*use system default*/
    { thislocale = GetSystemDefaultLCID();
    }
    switch (flags)
    { case TIME_NOSECONDS:
        FIXME(ole,"TIME_NOSECONDS not yet implemented");
        break;
      case TIME_NOMINUTESORSECONDS:
        FIXME(ole,"TIME_NOMINUTESORSECONDS not yet implemented");
       break;
    }   
    GetLocaleInfo32A(thislocale, thisflags, format_buf,sizeof(format_buf));
    thisformat = format_buf;
  }
  else 
  { thisformat = format;
  }
  
  if (!locale) 
  {  locale = GetSystemDefaultLCID();
  }
  if (xtime == NULL) /* NULL means use the current local time*/
  { GetSystemTime(&t);
    thistime = &t;
  } 
  else
  { thistime = xtime;
  }
  return OLE_GetFormatA(thislocale, thisflags, thistime, thisformat,timestr, timelen);
}


/* ****************************************************************
 *  GetTimeFormat32W()  [KERNEL32.423] Makes a Unicode string of the time
 * 
 *
 */

INT32 WINAPI 
GetTimeFormat32W(LCID locale,DWORD flags,
			      LPSYSTEMTIME xtime,
			      LPCWSTR format, 
			      LPWSTR timestr,INT32 timelen) 
{
   char debugbuf[40];
   WCHAR buf[20];
   LPCWSTR realformat=0;
   SYSTEMTIME t;
   LPSYSTEMTIME realtime;
   WCHAR * fmt_buf = NULL;
   int fmt_buf_size = 0; /* units of WCHARs */
   INT32 retval;
   
   lstrcpynWtoA(debugbuf, format, (sizeof(buf))/2);
   TRACE(ole, "GetTimeFormatW len %d flags 0x%lX format >%s<\n",
		timelen, flags, debugbuf);

   /* Enforce the Windows behavior */
   flags |= ~LOCALE_TIMEDATEBOTH;
   flags |= TIME_TIMEVARSONLY;

   /* take care of the format or locale not being given */
   if (format) {
      realformat = format;
   } else if (locale) {
      /* allocate memory */
      retval = ERROR_INSUFFICIENT_BUFFER;
      fmt_buf = malloc((fmt_buf_size+=7) * sizeof(WCHAR));
      if (!fmt_buf)
	goto out_nomem;

      while(!GetLocaleInfo32W(locale, LOCALE_STIMEFORMAT,
			      fmt_buf, fmt_buf_size)) {
	retval = ERROR_OUTOFMEMORY;
	fmt_buf = realloc(fmt_buf, 
			   (fmt_buf_size += 10) * sizeof(WCHAR));
	if (!fmt_buf)
		goto out_nomem;
      }
      realformat = fmt_buf;
   } else {
      FIXME(ole,  "caller gave no locale,  no format;  what should we do?\n");
      SetLastError(ERROR_BAD_FORMAT);
   }
   if (!locale)
      locale = GetSystemDefaultLCID();

   realtime = xtime;
   if (!realtime) {
      realtime = &t;
      GetSystemTime(realtime);
   }

   retval = OLE_GetFormatW(locale, flags, realtime, realformat, timestr,  timelen);
   if (fmt_buf)
	free(fmt_buf);
   return retval;

out_nomem:
	SetLastError(retval);
	WARN(ole, "could not allocate %d chars of memory\n", fmt_buf_size); 
	return 0;
}
