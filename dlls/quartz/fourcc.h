/*
 * Common FOURCC
 *
 * Copyright 2003 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define FromHex(n)  (((n) >= 'A') ? ((n) + 10 - 'A') : ((n) - '0'))
#define StreamFromFOURCC(fcc) ((WORD) ((FromHex(LOBYTE(LOWORD(fcc))) << 4) + (FromHex(HIBYTE(LOWORD(fcc))))))
#define TWOCCFromFOURCC(fcc)    HIWORD(fcc)

#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

/* FIXME: endianess? */
#define aviFCC(ch0, ch1, ch2, ch3) ((DWORD)(BYTE)(ch3) << 24 | (DWORD)(BYTE)(ch2) << 16 | (DWORD)(BYTE)(ch1) << 8 | ((DWORD)(BYTE)(ch0)))

/* four character codes used in AVI files */
#define ckidAVI               aviFCC('A','V','I',' ')
#define ckidRIFF              aviFCC('R','I','F','F')
#define ckidLIST              aviFCC('L','I','S','T')
#define ckidJUNK              aviFCC('J','U','N','K')
#define ckidHEADERLIST        aviFCC('h','d','r','l')
#define ckidAVIMOVIE          aviFCC('m','o','v','i')
#define ckidSTREAMNAME        aviFCC('s','t','r','n')
#define ckidSTREAMHANDLERDATA aviFCC('s','t','r','d')
#ifndef ckidMAINAVIHEADER
# define ckidMAINAVIHEADER    aviFCC('a','v','i','h')
# define ckidODML             aviFCC('o','d','m','l')
# define ckidAVIEXTHEADER     aviFCC('d','m','l','h')
# define ckidSTREAMLIST       aviFCC('s','t','r','l')
# define ckidSTREAMHEADER     aviFCC('s','t','r','h')
# define ckidSTREAMFORMAT     aviFCC('s','t','r','f')
# define ckidAVIOLDINDEX      aviFCC('i','d','x','1')
# define ckidAVISUPERINDEX    aviFCC('i','n','d','x')
#endif
#ifndef streamtypeVIDEO
#define streamtypeVIDEO       aviFCC('v','i','d','s')
#define streamtypeAUDIO       aviFCC('a','u','d','s')
#define streamtypeMIDI        aviFCC('m','i','d','s')
#define streamtypeTEXT        aviFCC('t','x','t','s')
#endif
#define cktypeDIBbits         aviTWOCC('d','b')
#define cktypeDIBcompressed   aviTWOCC('d','c')
#define cktypePALchange       aviTWOCC('p','c')
#define cktypeWAVEbytes       aviTWOCC('w','b')
