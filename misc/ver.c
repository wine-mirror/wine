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
#include "stackframe.h"	/* MAKE_SEGPTR */
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

#define LZREAD(what)	if (sizeof(*what)!=LZRead(lzfd,MAKE_SEGPTR(what),sizeof(*what))) return 0;

int
read_ne_header(HFILE lzfd,struct ne_header_s *nehd) {
	struct	mz_header_s	mzh;

	LZSeek(lzfd,0,SEEK_SET);
	if (sizeof(mzh)!=LZRead(lzfd,MAKE_SEGPTR(&mzh),sizeof(mzh)))
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

	nehdoffset=LZSeek(lzfd,nehd->resource_tab_offset,SEEK_CUR);
	LZREAD(&shiftcount);
	dprintf_resource(stderr,"shiftcount is %d\n",shiftcount);
	dprintf_resource(stderr,"reading resource typeinfo dir.\n");

	if (!HIWORD(typeid)) typeid = (SEGPTR)((WORD)typeid | 0x8000);
	if (!HIWORD(resid))  resid  = (SEGPTR)((WORD)resid | 0x8000);
	while (1) {
		int	skipflag;

		LZREAD(&ti);
		if (!ti.type_id)
			return 0;
		dprintf_resource(stderr,"    ti.typeid =%04x,count=%d\n",ti.type_id,ti.count);
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

				whereleft=LZSeek(
					lzfd,
					nehdoffset+nehd->resource_tab_offset+ti.type_id,
					SEEK_SET
				);
				LZREAD(&len);
				str=xmalloc(len);
				if (len!=LZRead(lzfd,MAKE_SEGPTR(str),len))
					return 0;
				dprintf_resource(stderr,"read %s to compare it with %s\n",
					str,(char*)PTR_SEG_TO_LIN(typeid)
				);
				if (lstrcmpi(str,(char*)PTR_SEG_TO_LIN(typeid)))
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
			dprintf_resource(stderr,"	ni.id=%4x,offset=%d,length=%d\n",
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

					whereleft=LZSeek(
						lzfd,
						nehdoffset+nehd->resource_tab_offset+ni.id,
						SEEK_SET
					);
					LZREAD(&len);
					str=xmalloc(len);
					if (len!=LZRead(lzfd,MAKE_SEGPTR(str),len))
						return 0;
					dprintf_resource(stderr,"read %s to compare it with %s\n",
						str,(char*)PTR_SEG_TO_LIN(typeid)
					);
					if (!lstrcmpi(str,(char*)PTR_SEG_TO_LIN(typeid)))
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
			if (len!=LZRead(lzfd,MAKE_SEGPTR(rdata),len)) {
				free(rdata);
				return 0;
			}
			dprintf_resource(stderr,"resource found.\n");
			*resdata= (BYTE*)rdata;
			*reslen	= len;
			return 1;
		}
	}
}

DWORD
GetFileResourceSize(LPCSTR filename,SEGPTR restype,SEGPTR resid,LPDWORD off) {
	HFILE	lzfd;
	OFSTRUCT	ofs;
	BYTE	*resdata;
	int	reslen;
	struct	ne_header_s	nehd;

	fprintf(stderr,"GetFileResourceSize(%s,%lx,%lx,%p)\n",
		filename,(LONG)restype,(LONG)resid,off
	);
	lzfd=LZOpenFile(filename,&ofs,OF_READ);
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

DWORD
GetFileResource(LPCSTR filename,SEGPTR restype,SEGPTR resid,
		DWORD off,DWORD datalen,LPVOID data
) {
	HFILE	lzfd;
	OFSTRUCT	ofs;
	BYTE	*resdata;
	int	reslen;
	struct	ne_header_s	nehd;
	fprintf(stderr,"GetFileResource(%s,%lx,%lx,%ld,%ld,%p)\n",
		filename,(LONG)restype,(LONG)resid,off,datalen,data
	);

	lzfd=LZOpenFile(filename,&ofs,OF_READ);
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
	LZRead(lzfd,MAKE_SEGPTR(data),reslen);
	LZClose(lzfd);
	return reslen;
}

DWORD
GetFileVersionInfoSize(LPCSTR filename,LPDWORD handle) {
	DWORD	len,ret;
	BYTE	buf[72];
	VS_FIXEDFILEINFO *vffi;

	fprintf(stderr,"GetFileVersionInfoSize(%s,%p)\n",filename,handle);

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
	fprintf(stderr,"->strucver=%ld.%ld,filever=%ld.%ld,productver=%ld.%ld,flagmask=%lx,flags=%lx,OS=",
		(vffi->dwStrucVersion>>16),vffi->dwStrucVersion&0xFFFF,
		vffi->dwFileVersionMS,vffi->dwFileVersionLS,
		vffi->dwProductVersionMS,vffi->dwProductVersionLS,
		vffi->dwFileFlagsMask,vffi->dwFileFlags
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:fprintf(stderr,"DOS,");break;
	case VOS_OS216:fprintf(stderr,"OS/2-16,");break;
	case VOS_OS232:fprintf(stderr,"OS/2-32,");break;
	case VOS_NT:fprintf(stderr,"NT,");break;
	case VOS_UNKNOWN:
	default:
		fprintf(stderr,"UNKNOWN(%ld),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (vffi->dwFileOS & 0xFFFF) {
	case VOS__BASE:fprintf(stderr,"BASE");break;
	case VOS__WINDOWS16:fprintf(stderr,"WIN16");break;
	case VOS__WINDOWS32:fprintf(stderr,"WIN32");break;
	case VOS__PM16:fprintf(stderr,"PM16");break;
	case VOS__PM32:fprintf(stderr,"PM32");break;
	default:fprintf(stderr,"UNKNOWN(%ld)",vffi->dwFileOS&0xFFFF);break;
	}
	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		fprintf(stderr,"filetype=Unknown(%ld)",vffi->dwFileType);
		break;
	case VFT_APP:fprintf(stderr,"filetype=APP");break;
	case VFT_DLL:fprintf(stderr,"filetype=DLL");break;
	case VFT_DRV:
		fprintf(stderr,"filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			fprintf(stderr,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			fprintf(stderr,"PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			fprintf(stderr,"KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			fprintf(stderr,"LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			fprintf(stderr,"DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			fprintf(stderr,"MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			fprintf(stderr,"NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			fprintf(stderr,"SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			fprintf(stderr,"INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			fprintf(stderr,"SOUND");
			break;
		case VFT2_DRV_COMM:
			fprintf(stderr,"COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			fprintf(stderr,"INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
		fprintf(stderr,"filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			fprintf(stderr,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:fprintf(stderr,"RASTER");break;
		case VFT2_FONT_VECTOR:fprintf(stderr,"VECTOR");break;
		case VFT2_FONT_TRUETYPE:fprintf(stderr,"TRUETYPE");break;
		}
		break;
	case VFT_VXD:fprintf(stderr,"filetype=VXD");break;
	case VFT_STATIC_LIB:fprintf(stderr,"filetype=STATIC_LIB");break;
	}
	fprintf(stderr,"filedata=%lx.%lx\n",vffi->dwFileDateMS,vffi->dwFileDateLS);
	return len;
}

DWORD 
GetFileVersionInfo(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data) {
	fprintf(stderr,"GetFileVersionInfo(%s,%ld,%ld,%p)\n->",
		filename,handle,datasize,data
	);
	return GetFileResource(
		filename,VS_FILE_INFO,VS_VERSION_INFO,handle,datasize,data
	);
}

DWORD 
VerFindFile(
	UINT flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT *curdirlen,LPSTR destdir,UINT*destdirlen
) {
	fprintf(stderr,"VerFindFile(%x,%s,%s,%s,%p,%d,%p,%d)\n",
		flags,filename,windir,appdir,curdir,*curdirlen,destdir,*destdirlen
	);
	strcpy(curdir,"Z:\\ROOT\\.WINE\\");/*FIXME*/
	*curdirlen=strlen(curdir);
	strcpy(destdir,"Z:\\ROOT\\.WINE\\");/*FIXME*/
	*destdirlen=strlen(destdir);
	return 0;
}

DWORD
VerInstallFile(
	UINT flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPSTR tmpfile,UINT*tmpfilelen
) {
	fprintf(stderr,"VerInstallFile(%x,%s,%s,%s,%s,%p,%d)\n",
		flags,srcfilename,destfilename,srcdir,destdir,tmpfile,*tmpfilelen
	);
	return VIF_SRCOLD;
}

/* FIXME: This table should, of course, be language dependend */
static struct map_id2str {
	UINT	langid;
	char	*langname;
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


DWORD
VerLanguageName(UINT langid,LPSTR langname,UINT langnamelen) {
	int	i;

	fprintf(stderr,"VerLanguageName(%d,%p,%d)\n",langid,langname,langnamelen);
	for (i=0;languages[i].langid!=0;i++)
		if (langid==languages[i].langid)
			break;
	strncpy(langname,languages[i].langname,langnamelen);
	langname[langnamelen-1]='\0';
	return strlen(languages[i].langname);
}

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
		fprintf(stderr,"db=%p,db->nextoff=%d,db->datalen=%d,db->name=%s,db->data=%s\n",
			db,db->nextoff,db->datalen,db->name,(char*)((char*)db+4+((strlen(db->name)+4)&~3))
		);
		if (!db->nextoff)
			return NULL;

		fprintf(stderr,"comparing with %s\n",db->name);
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

/* take care, 'buffer' is NOT a SEGPTR, it just points to one */
DWORD
VerQueryValue(SEGPTR segblock,LPCSTR subblock,SEGPTR *buffer,UINT *buflen) {
	BYTE	*block=PTR_SEG_TO_LIN(segblock),*b;
	struct	db	*db;
	char	*s;

	fprintf(stderr,"VerQueryValue(%p,%s,%p,%d)\n",
		block,subblock,buffer,*buflen
	);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO")+strlen(subblock)+1);
	strcpy(s,"VS_VERSION_INFO");strcat(s,subblock);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&3);
	/* now look up what the resp. SEGPTR would be ... 
	 * we could use MAKE_SEGPTR , but we don't need to
	 */
	*buffer	= (b-block)+segblock;
	fprintf(stderr,"	-> %s=%s\n",subblock,b);
	return 1;
}

/*
   20 GETFILEVERSIONINFORAW
   21 VERFTHK_THUNKDATA16
   22 VERTHKSL_THUNKDATA16
*/
