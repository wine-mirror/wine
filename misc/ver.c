/* 
 * Implementation of VER.DLL
 * 
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "ver.h"
#include "lzexpand.h"
#include "module.h"
#include "neexe.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"
#include "string32.h"

#define LZREAD(what) \
  if (sizeof(*what)!=LZRead32(lzfd,what,sizeof(*what))) return 0;
#define LZTELL(lzfd) LZSeek(lzfd, 0, SEEK_CUR);

#define strdupW2A(x)	STRING32_DupUniToAnsi(x)
#define strdupA2W(x)	STRING32_DupAnsiToUni(x)

int
read_ne_header(HFILE lzfd,struct ne_header_s *nehd) {
	struct	mz_header_s	mzh;

	LZSeek(lzfd,0,SEEK_SET);
	if (sizeof(mzh)!=LZRead32(lzfd,&mzh,sizeof(mzh)))
		return 0;
	if (mzh.mz_magic!=MZ_SIGNATURE)
		return 0;
	LZSeek(lzfd,mzh.ne_offset,SEEK_SET);
	LZREAD(nehd);
	if (nehd->ne_magic == NE_SIGNATURE) {
		LZSeek(lzfd,mzh.ne_offset,SEEK_SET);
		return 1;
	}
	/* must handle PE files too. Later. */
	return 0;
}


int
find_ne_resource(
	HFILE lzfd,struct ne_header_s *nehd,SEGPTR typeid,SEGPTR resid,
	BYTE **resdata,int *reslen,DWORD *off
) {
	NE_TYPEINFO	ti;
	NE_NAMEINFO	ni;
	int		i;
	WORD		shiftcount;
	DWORD		nehdoffset;

	nehdoffset = LZTELL(lzfd);
	LZSeek(lzfd,nehd->resource_tab_offset,SEEK_CUR);
	LZREAD(&shiftcount);
	dprintf_ver(stddeb,"shiftcount is %d\n",shiftcount);
	dprintf_ver(stddeb,"reading resource typeinfo dir.\n");

	if (!HIWORD(typeid)) typeid = (SEGPTR)(LOWORD(typeid) | 0x8000);
	if (!HIWORD(resid))  resid  = (SEGPTR)(LOWORD(resid) | 0x8000);
	while (1) {
		int	skipflag;

		LZREAD(&ti);
		if (!ti.type_id)
			return 0;
		dprintf_ver(stddeb,"    ti.typeid =%04x,count=%d\n",ti.type_id,ti.count);
		skipflag=0;
		if (!HIWORD(typeid)) {
			if ((ti.type_id&0x8000)&&(typeid!=ti.type_id))
				skipflag=1;
		} else {
			if (ti.type_id & 0x8000) {
				skipflag=1; 
			} else {
				BYTE	len;
				char	*str;
				DWORD	whereleft;

				whereleft = LZTELL(lzfd);
				LZSeek(
					lzfd,
					nehdoffset+nehd->resource_tab_offset+ti.type_id,
					SEEK_SET
				);
				LZREAD(&len);
				str=xmalloc(len);
				if (len!=LZRead32(lzfd,str,len))
					return 0;
				dprintf_ver(stddeb,"read %s to compare it with %s\n",
					str,(char*)PTR_SEG_TO_LIN(typeid)
				);
				if (lstrcmpi32A(str,(char*)PTR_SEG_TO_LIN(typeid)))
					skipflag=1;
				free(str);
				LZSeek(lzfd,whereleft,SEEK_SET);
			}
		}
		if (skipflag) {
			LZSeek(lzfd,ti.count*sizeof(ni),SEEK_CUR);
			continue;
		}
		for (i=0;i<ti.count;i++) {
			WORD	*rdata;
			int	len;

			LZREAD(&ni);
			dprintf_ver(stddeb,"	ni.id=%4x,offset=%d,length=%d\n",
				ni.id,ni.offset,ni.length
			);
			skipflag=1;
			if (!HIWORD(resid)) {
				if (ni.id == resid)
					skipflag=0;
			} else {
				if (!(ni.id & 0x8000)) {
					BYTE	len;
					char	*str;
					DWORD	whereleft;

					whereleft = LZTELL(lzfd);
					  LZSeek(
						lzfd,
						nehdoffset+nehd->resource_tab_offset+ni.id,
						SEEK_SET
					);
					LZREAD(&len);
					str=xmalloc(len);
					if (len!=LZRead32(lzfd,str,len))
						return 0;
					dprintf_ver(stddeb,"read %s to compare it with %s\n",
						str,(char*)PTR_SEG_TO_LIN(typeid)
					);
					if (!lstrcmpi32A(str,(char*)PTR_SEG_TO_LIN(typeid)))
						skipflag=0;
					free(str);
					LZSeek(lzfd,whereleft,SEEK_SET);
				}
			}
			if (skipflag)
				continue;
			LZSeek(lzfd,((int)ni.offset<<shiftcount),SEEK_SET);
			*off	= (int)ni.offset<<shiftcount;
			len	= ni.length<<shiftcount;
			rdata=(WORD*)xmalloc(len);
			if (len!=LZRead32(lzfd,rdata,len)) {
				free(rdata);
				return 0;
			}
			dprintf_ver(stddeb,"resource found.\n");
			*resdata= (BYTE*)rdata;
			*reslen	= len;
			return 1;
		}
	}
}

/* GetFileResourceSize				[VER.2] */
DWORD
GetFileResourceSize(LPCSTR filename,SEGPTR restype,SEGPTR resid,LPDWORD off) {
	HFILE	lzfd;
	OFSTRUCT	ofs;
	BYTE	*resdata;
	int	reslen;
	struct	ne_header_s	nehd;

	dprintf_ver(stddeb,"GetFileResourceSize(%s,%lx,%lx,%p)\n",
		filename,(LONG)restype,(LONG)resid,off
	);
	lzfd=LZOpenFile16(filename,&ofs,OF_READ);
	if (lzfd==0)
		return 0;
	if (!read_ne_header(lzfd,&nehd)) {
		LZClose(lzfd);
		return 0;
	}
	if (!find_ne_resource(lzfd,&nehd,restype,resid,&resdata,&reslen,off)) {
		LZClose(lzfd);
		return 0;
	}
	free(resdata);
	LZClose(lzfd);
	return reslen;
}

/* GetFileResource				[VER.3] */
DWORD
GetFileResource(LPCSTR filename,SEGPTR restype,SEGPTR resid,
		DWORD off,DWORD datalen,LPVOID data
) {
	HFILE	lzfd;
	OFSTRUCT	ofs;
	BYTE	*resdata;
	int	reslen=datalen;
	struct	ne_header_s	nehd;
	dprintf_ver(stddeb,"GetFileResource(%s,%lx,%lx,%ld,%ld,%p)\n",
		filename,(LONG)restype,(LONG)resid,off,datalen,data
	);

	lzfd=LZOpenFile16(filename,&ofs,OF_READ);
	if (lzfd==0)
		return 0;
	if (!off) {
		if (!read_ne_header(lzfd,&nehd)) {
			LZClose(lzfd);
			return 0;
		}
		if (!find_ne_resource(lzfd,&nehd,restype,resid,&resdata,&reslen,&off)) {
			LZClose(lzfd);
			return 0;
		}
		free(resdata);
	}
	LZSeek(lzfd,off,SEEK_SET);
	if (reslen>datalen)
		reslen=datalen;
	LZRead32(lzfd,data,reslen);
	LZClose(lzfd);
	return reslen;
}

/* GetFileVersionInfoSize			[VER.6] */
DWORD
GetFileVersionInfoSize16(LPCSTR filename,LPDWORD handle) {
	DWORD	len,ret;
	BYTE	buf[72];
	VS_FIXEDFILEINFO *vffi;

	dprintf_ver(stddeb,"GetFileVersionInfoSize16(%s,%p)\n",filename,handle);
	len=GetFileResourceSize(filename,VS_FILE_INFO,VS_VERSION_INFO,handle);
	if (!len)
		return 0;
	ret=GetFileResource(
		filename,VS_FILE_INFO,VS_VERSION_INFO,*handle,sizeof(buf),buf
	);
	if (!ret)
		return 0;

	vffi=(VS_FIXEDFILEINFO*)(buf+0x14);
	if (vffi->dwSignature != VS_FFI_SIGNATURE)
		return 0;
	if (*(WORD*)buf < len)
		len = *(WORD*)buf;
	dprintf_ver(stddeb,"->strucver=%ld.%ld,filever=%ld.%ld,productver=%ld.%ld,flagmask=%lx,flags=%lx,OS=",
		(vffi->dwStrucVersion>>16),vffi->dwStrucVersion&0xFFFF,
		vffi->dwFileVersionMS,vffi->dwFileVersionLS,
		vffi->dwProductVersionMS,vffi->dwProductVersionLS,
		vffi->dwFileFlagsMask,vffi->dwFileFlags
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:dprintf_ver(stddeb,"DOS,");break;
	case VOS_OS216:dprintf_ver(stddeb,"OS/2-16,");break;
	case VOS_OS232:dprintf_ver(stddeb,"OS/2-32,");break;
	case VOS_NT:dprintf_ver(stddeb,"NT,");break;
	case VOS_UNKNOWN:
	default:
		dprintf_ver(stddeb,"UNKNOWN(%ld),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (vffi->dwFileOS & 0xFFFF) {
	case VOS__BASE:dprintf_ver(stddeb,"BASE");break;
	case VOS__WINDOWS16:dprintf_ver(stddeb,"WIN16");break;
	case VOS__WINDOWS32:dprintf_ver(stddeb,"WIN32");break;
	case VOS__PM16:dprintf_ver(stddeb,"PM16");break;
	case VOS__PM32:dprintf_ver(stddeb,"PM32");break;
	default:dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileOS&0xFFFF);break;
	}
	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		dprintf_ver(stddeb,"filetype=Unknown(%ld)",vffi->dwFileType);
		break;
	case VFT_APP:dprintf_ver(stddeb,"filetype=APP");break;
	case VFT_DLL:dprintf_ver(stddeb,"filetype=DLL");break;
	case VFT_DRV:
		dprintf_ver(stddeb,"filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			dprintf_ver(stddeb,"PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			dprintf_ver(stddeb,"KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			dprintf_ver(stddeb,"LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			dprintf_ver(stddeb,"DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			dprintf_ver(stddeb,"MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			dprintf_ver(stddeb,"NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			dprintf_ver(stddeb,"SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			dprintf_ver(stddeb,"INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			dprintf_ver(stddeb,"SOUND");
			break;
		case VFT2_DRV_COMM:
			dprintf_ver(stddeb,"COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			dprintf_ver(stddeb,"INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
		dprintf_ver(stddeb,"filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:dprintf_ver(stddeb,"RASTER");break;
		case VFT2_FONT_VECTOR:dprintf_ver(stddeb,"VECTOR");break;
		case VFT2_FONT_TRUETYPE:dprintf_ver(stddeb,"TRUETYPE");break;
		}
		break;
	case VFT_VXD:dprintf_ver(stddeb,"filetype=VXD");break;
	case VFT_STATIC_LIB:dprintf_ver(stddeb,"filetype=STATIC_LIB");break;
	}
	dprintf_ver(stddeb,"filedata=%lx.%lx\n",vffi->dwFileDateMS,vffi->dwFileDateLS);
	return len;
}

/* GetFileVersionInfoSize32A			[VERSION.1] */
DWORD
GetFileVersionInfoSize32A(LPCSTR filename,LPDWORD handle) {
	dprintf_ver(stddeb,"GetFileVersionInfoSize32A(%s,%p)\n",filename,handle);
	return GetFileVersionInfoSize16(filename,handle);
}

/* GetFileVersionInfoSize32W			[VERSION.2] */
DWORD
GetFileVersionInfoSize32W(LPCWSTR filename,LPDWORD handle) {
	LPSTR	xfn;
	DWORD	ret;

	xfn	= strdupW2A(filename);
	ret=GetFileVersionInfoSize16(xfn,handle);
	free(xfn);
	return	ret;
}

/* GetFileVersionInfo				[VER.7] */
DWORD 
GetFileVersionInfo16(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data) {
	dprintf_ver(stddeb,"GetFileVersionInfo16(%s,%ld,%ld,%p)\n->",
		filename,handle,datasize,data
	);
	return GetFileResource(
		filename,VS_FILE_INFO,VS_VERSION_INFO,handle,datasize,data
	);
}

/* GetFileVersionInfoA				[VERSION.0] */
DWORD 
GetFileVersionInfo32A(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data) {
	return GetFileVersionInfo16(filename,handle,datasize,data);
}

/* GetFileVersionInfoW				[VERSION.3] */
DWORD 
GetFileVersionInfo32W(LPCWSTR filename,DWORD handle,DWORD datasize,LPVOID data){
	DWORD	ret;
	LPSTR	fn;

	fn	= strdupW2A(filename);
	ret	= GetFileVersionInfo16(fn,handle,datasize,data);
	free(fn);
	return	ret;
}

/* VerFindFile				[VER.8] */
DWORD
VerFindFile16(
	UINT16 flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT16 *curdirlen,LPSTR destdir,UINT16 *destdirlen
) {
	dprintf_ver(stddeb,"VerFindFile(%x,%s,%s,%s,%p,%d,%p,%d)\n",
		flags,filename,windir,appdir,curdir,*curdirlen,destdir,*destdirlen
	);
	strcpy(curdir,"Z:\\ROOT\\.WINE\\");/*FIXME*/
	*curdirlen=strlen(curdir);
	strcpy(destdir,"Z:\\ROOT\\.WINE\\");/*FIXME*/
	*destdirlen=strlen(destdir);
	return 0;
}

/* VerFindFileA						[VERSION.5] */
DWORD
VerFindFile32A(
	UINT32 flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT32 *pcurdirlen,LPSTR destdir,UINT32 *pdestdirlen )
{
    UINT16 curdirlen, destdirlen;
    DWORD ret = VerFindFile16(flags,filename,windir,appdir,
                              curdir,&curdirlen,destdir,&destdirlen);
    *pcurdirlen = curdirlen;
    *pdestdirlen = destdirlen;
    return ret;
}

/* VerFindFileW						[VERSION.6] */
DWORD
VerFindFile32W(
	UINT32 flags,LPCWSTR filename,LPCWSTR windir,LPCWSTR appdir,
	LPWSTR curdir,UINT32 *pcurdirlen,LPWSTR destdir,UINT32 *pdestdirlen )
{
    UINT16 curdirlen, destdirlen;
    LPSTR wfn,wwd,wad,wdd,wcd;
    DWORD ret;

    wfn = strdupW2A(filename);
    wwd = strdupW2A(windir);
    wad = strdupW2A(appdir);
    wcd = (LPSTR)malloc(*pcurdirlen);
    wdd = (LPSTR)malloc(*pdestdirlen);
    ret=VerFindFile16(flags,wfn,wwd,wad,wcd,&curdirlen,wdd,&destdirlen);
    STRING32_AnsiToUni(curdir,wcd);
    STRING32_AnsiToUni(destdir,wdd);
    *pcurdirlen = strlen(wcd);
    *pdestdirlen = strlen(wdd);
    return ret;
}

/* VerInstallFile					[VER.9] */
DWORD
VerInstallFile16(
	UINT16 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPSTR tmpfile,UINT16 *tmpfilelen
) {
	dprintf_ver(stddeb,"VerInstallFile(%x,%s,%s,%s,%s,%p,%d)\n",
		flags,srcfilename,destfilename,srcdir,destdir,tmpfile,*tmpfilelen
	);

	/* FIXME: Implementation still missing .... */

	return VIF_SRCOLD;
}

/* VerFindFileA					[VERSION.5] */
DWORD
VerInstallFile32A(
	UINT32 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPSTR tmpfile,UINT32 *tmpfilelen )
{
    UINT16 filelen;
    DWORD ret= VerInstallFile16(flags,srcfilename,destfilename,srcdir,
                                destdir,tmpfile,&filelen);
    *tmpfilelen = filelen;
    return ret;
}

/* VerFindFileW					[VERSION.6] */
DWORD
VerInstallFile32W(
	UINT32 flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPWSTR tmpfile,UINT32 *tmpfilelen
) {
	LPSTR	wsrcf,wsrcd,wdestf,wdestd,wtmpf;
	DWORD	ret;

	wsrcf	= strdupW2A(srcfilename);
	wsrcd	= strdupW2A(srcdir);
	wdestf	= strdupW2A(destfilename);
	wdestd	= strdupW2A(destdir);
	wtmpf	= strdupW2A(tmpfile);
	ret=VerInstallFile32A(flags,wsrcf,wdestf,wsrcd,wdestd,wtmpf,tmpfilelen);
	free(wsrcf);
	free(wsrcd);
	free(wdestf);
	free(wdestd);
	free(wtmpf);
	return ret;
}

/* FIXME: This table should, of course, be language dependend */
static const struct map_id2str {
	UINT	langid;
	const char *langname;
} languages[]={
	{0x0401,"Arabisch"},
	{0x0402,"Bulgarisch"},
	{0x0403,"Katalanisch"},
	{0x0404,"Traditionales Chinesisch"},
	{0x0405,"Tschecisch"},
	{0x0406,"Dänisch"},
	{0x0407,"Deutsch"},
	{0x0408,"Griechisch"},
	{0x0409,"Amerikanisches Englisch"},
	{0x040A,"Kastilisches Spanisch"},
	{0x040B,"Finnisch"},
	{0x040C,"Französisch"},
	{0x040D,"Hebräisch"},
	{0x040E,"Ungarisch"},
	{0x040F,"Isländisch"},
	{0x0410,"Italienisch"},
	{0x0411,"Japanisch"},
	{0x0412,"Koreanisch"},
	{0x0413,"Niederländisch"},
	{0x0414,"Norwegisch-Bokmal"},
	{0x0415,"Polnisch"},
	{0x0416,"Brasilianisches Portugiesisch"},
	{0x0417,"Rätoromanisch"},
	{0x0418,"Rumänisch"},
	{0x0419,"Russisch"},
	{0x041A,"Kroatoserbisch (lateinisch)"},
	{0x041B,"Slowenisch"},
	{0x041C,"Albanisch"},
	{0x041D,"Schwedisch"},
	{0x041E,"Thai"},
	{0x041F,"Türkisch"},
	{0x0420,"Urdu"},
	{0x0421,"Bahasa"},
	{0x0804,"Vereinfachtes Chinesisch"},
	{0x0807,"Schweizerdeutsch"},
	{0x0809,"Britisches Englisch"},
	{0x080A,"Mexikanisches Spanisch"},
	{0x080C,"Belgisches Französisch"},
	{0x0810,"Schweizerisches Italienisch"},
	{0x0813,"Belgisches Niederländisch"},
	{0x0814,"Norgwegisch-Nynorsk"},
	{0x0816,"Portugiesisch"},
	{0x081A,"Serbokratisch (kyrillisch)"},
	{0x0C1C,"Kanadisches Französisch"},
	{0x100C,"Schweizerisches Französisch"},
	{0x0000,"Unbekannt"},
};

/* VerLanguageName				[VER.10] */
DWORD
VerLanguageName16(UINT16 langid,LPSTR langname,UINT16 langnamelen) {
	int	i;
	char	*buf;

	dprintf_ver(stddeb,"VerLanguageName(%d,%p,%d)\n",langid,langname,langnamelen);
	/* First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
	 * from the registry. 
	 */
	buf=(char*)malloc(strlen("\\System\\CurrentControlSet\\control\\Nls\\Locale\\")+9);
	sprintf(buf,"\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",langid);
	if (ERROR_SUCCESS==RegQueryValue16(HKEY_LOCAL_MACHINE,buf,langname,(LPDWORD)&langnamelen)) {
		langname[langnamelen-1]='\0';
		return langnamelen;
	}
	/* if that fails, use the interal table */
	for (i=0;languages[i].langid!=0;i++)
		if (langid==languages[i].langid)
			break;
	strncpy(langname,languages[i].langname,langnamelen);
	langname[langnamelen-1]='\0';
	return strlen(languages[i].langname);
}

/* VerLanguageNameA				[VERSION.9] */
DWORD
VerLanguageName32A(UINT32 langid,LPSTR langname,UINT32 langnamelen) {
	return VerLanguageName16(langid,langname,langnamelen);
}

/* VerLanguageNameW				[VERSION.10] */
DWORD
VerLanguageName32W(UINT32 langid,LPWSTR langname,UINT32 langnamelen) {
	int	i;
	char	*buf;
	LPWSTR	keyname,result;

	/* First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
	 * from the registry. 
	 */
	buf=(char*)malloc(strlen("\\System\\CurrentControlSet\\control\\Nls\\Locale\\")+9);
	sprintf(buf,"\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",langid);
	keyname=strdupA2W(buf);free(buf);
	if (ERROR_SUCCESS==RegQueryValue32W(HKEY_LOCAL_MACHINE,keyname,langname,(LPDWORD)&langnamelen)) {
		free(keyname);
		return langnamelen;
	}
	free(keyname);
	/* if that fails, use the interal table */
	for (i=0;languages[i].langid!=0;i++)
		if (langid==languages[i].langid)
			break;
	result=strdupA2W(languages[i].langname);
	i=lstrlen32W(result)*sizeof(WCHAR);
	if (i>langnamelen)
		i=langnamelen;
	memcpy(langname,result,i);
	langname[langnamelen-1]='\0';
	free(result);
	return strlen(languages[i].langname); /* same as strlenW(result); */
}

/* FIXME: UNICODE? */
struct db {
	WORD	nextoff;
	WORD	datalen;
/* in memory structure... */
	char	name[1]; 	/* padded to dword alignment */
/* .... 
	char	data[datalen];     padded to dword alignemnt
	BYTE	subdirdata[];      until nextoff
 */
};

static BYTE*
_find_data(BYTE *block,LPCSTR str) {
	char	*nextslash;
	int	substrlen;
	struct	db	*db;

	while (*str && *str=='\\')
		str++;
	if (NULL!=(nextslash=strchr(str,'\\')))
		substrlen=nextslash-str;
	else
		substrlen=strlen(str);
	if (nextslash!=NULL) {
		while (*nextslash && *nextslash=='\\')
			nextslash++;
		if (!*nextslash)
			nextslash=NULL;
	}


	while (1) {
		db=(struct db*)block;
		dprintf_ver(stddeb,"db=%p,db->nextoff=%d,db->datalen=%d,db->name=%s,db->data=%s\n",
			db,db->nextoff,db->datalen,db->name,(char*)((char*)db+4+((strlen(db->name)+4)&~3))
		);
		if (!db->nextoff)
			return NULL;

		dprintf_ver(stddeb,"comparing with %s\n",db->name);
		if (!strncmp(db->name,str,substrlen)) {
			if (nextslash)
				return _find_data(
					block+4+((strlen(db->name)+4)&~3)+((db->datalen+3)&~3)
					,nextslash
				);
			else
				return block;
		}
		block=block+((db->nextoff+3)&~3);
	}
}

/* VerQueryValue 			[VER.11] */
/* take care, 'buffer' is NOT a SEGPTR, it just points to one */
DWORD
VerQueryValue16(SEGPTR segblock,LPCSTR subblock,SEGPTR *buffer,UINT16 *buflen)
{
	BYTE	*block=PTR_SEG_TO_LIN(segblock),*b;
	struct	db	*db;
	char	*s;

	dprintf_ver(stddeb,"VerQueryValue16(%p,%s,%p,%d)\n",
		block,subblock,buffer,*buflen
	);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(subblock)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,subblock);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	/* now look up what the resp. SEGPTR would be ... */
	*buffer	= (b-block)+segblock;
	dprintf_ver(stddeb,"	-> %s=%s\n",subblock,b);
	return 1;
}

DWORD
VerQueryValue32A(LPVOID vblock,LPCSTR subblock,LPVOID *vbuffer,UINT32 *buflen)
{
	BYTE	*b,*block=(LPBYTE)vblock,**buffer=(LPBYTE*)vbuffer;
	struct	db	*db;
	char	*s;

	dprintf_ver(stddeb,"VerQueryValue32A(%p,%s,%p,%d)\n",
		block,subblock,buffer,*buflen
	);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(subblock)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,subblock);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	*buffer	= b;
	dprintf_ver(stddeb,"	-> %s=%s\n",subblock,b);
	return 1;
}

DWORD
VerQueryValue32W(LPVOID vblock,LPCWSTR subblock,LPVOID *vbuffer,UINT32 *buflen)
{
	/* FIXME: hmm, we not only need to convert subblock, but also 
	 *        the content...or?
	 * And what about UNICODE version info?
	 * And the NAMES of the values?
	 */
	BYTE		*b,**buffer=(LPBYTE*)vbuffer,*block=(LPBYTE)vblock;
	struct	db	*db;
	char		*s,*sb;

	sb=strdupW2A(subblock);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(sb)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,sb);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		free(sb);
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	*buffer	= b;
	dprintf_ver(stddeb,"	-> %s=%s\n",sb,b);
	free(sb);
	return 1;
}
/* 20 GETFILEVERSIONINFORAW */
