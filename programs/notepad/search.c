/*
 *  Notepad (search.c)
 *  Copyright (C) 1999 by Marcel Baur
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  This file features Heuristic Boyer-Moore Text Search
 *
 *  Always:   - Buf is the Buffer containing the whole text
 *  =======   - SP is the Search Pattern, which has to be found in Buf.
 *
 */

#include <windows.h>

 #define CHARSETSIZE 255

 int delta[CHARSETSIZE];

 /* rightmostpos: return rightmost position of ch in szSP (or -1) */
 int rightmostpos(char ch, LPSTR szSP, int nSPLen) {
    int i = nSPLen;
    while ((i>0) & (szSP[i]!=ch)) i--;
    return(i);
 }

 /* setup_delta: setup delta1 cache */
 void setup_delta(LPSTR szSP, int nSPLen) {
    int i;

    for (i=0; i<CHARSETSIZE; i++) {
       delta[i] = nSPLen;
    }

    for (i=0; i<nSPLen; i++) {
       delta[(int)szSP[i]] = (nSPLen - rightmostpos(szSP[i], szSP, nSPLen));
    }
 }

 int bm_search(LPSTR szBuf, int nBufLen, LPSTR szSP, int nSPLen) {
    int i = nSPLen;
    int j = nSPLen;

    do {
       if ((szBuf[i] = szSP[j])) {
         i--; j--;
       } else {
         if ((nSPLen-j+1) > delta[(int)szBuf[i]]) {
           i+= (nSPLen-j+1);
         } else {
           i+= delta[(int)szBuf[i]];
         }
       }
    } while (j>0 && i<=nBufLen);
    return(i+1);
 }

