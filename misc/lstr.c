static char Copyright[] = "Copyright  Yngvi Sigurjonsson (yngvi@hafro.is), 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>

#include "prototypes.h"
#include "regfunc.h"
#include "windows.h"


  /* Funny to divide them between user and kernel. */

/* KERNEL.89 */
LPSTR lstrcat(LPSTR target,LPCSTR source)
{
  fprintf(stderr,"lstrcat(%s,%s)\n",target,source);
  return strcat(target,source);
}

/* USER.430 */
int lstrcmp(LPCSTR str1,LPCSTR str2)
{
  return strcmp(str1,str2);
}

/* USER.471 */
int lstrcmpi(LPCSTR str1,LPCSTR str2)
{
  int i;
  i=0;
  while((toupper(str1[i])==toupper(str2[i]))&&(str1[i]!=0))
    i++;
  return toupper(str1[i])-toupper(str2[i]);
}

/* KERNEL.88 */
LPSTR lstrcpy(LPSTR target,LPCSTR source)
{
  return strcpy(target,source);
}

/* KERNEL.353 */
LPSTR lstrcpyn(LPSTR target,LPCSTR source,int n)
{
  return strncpy(target,source,n);
}

/* KERNEL.90 */
int lstrlen(LPCSTR str)
{
  return strlen(str);
}

/* AnsiUpper USER.431 */
char FAR* AnsiUpper(char FAR* strOrChar)
{
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */
  if((int)strOrChar<256)
    return (char FAR*) toupper((int)strOrChar);
  else {
    int i;
    for(i=0;(i<65536)&&strOrChar[i];i++)
      strOrChar[i]=toupper(strOrChar[i]);
    return strOrChar;	
  }
}

/* AnsiLower USER.432 */
char FAR* AnsiLower(char FAR* strOrChar)
{
  /* I am not sure if the locale stuff works with tolower, but then again 
     I am not sure if the Linux libc locale stuffs works at all */
  if((int)strOrChar<256)
    return (char FAR*)tolower((int)strOrChar);
  else {
    int i;
    for(i=0;(i<65536)&&strOrChar[i];i++)
      strOrChar[i]=tolower(strOrChar[i]);
    return strOrChar;	
  }
}

/* AnsiUpperBuff USER.437 */
UINT AnsiUpperBuff(LPSTR str,UINT len)
{
  int i;
  len=(len==0)?65536:len;

  for(i=0;i<len;i++)
    str[i]=toupper(str[i]);
  return i;	
}

/* AnsiLowerBuff USER.438 */
UINT AnsiLowerBuff(LPSTR str,UINT len)
{
  int i;
  len=(len==0)?65536:len;
  i=0;

  for(i=0;i<len;i++)
    str[i]=tolower(str[i]);
 
  return i;	
}

/* AnsiNext USER.472 */
LPSTR AnsiNext(LPSTR current)
{
  return (*current)?current+1:current;
}

/* AnsiPrev USER.473 */
char FAR*  AnsiPrev(/*const*/ char FAR* start,char FAR* current)
{
  return (current==start)?start:current-1;
}

/* IsCharAlpha USER 433 */
BOOL IsCharAlpha(char ch)
{
  return isalpha(ch);   /* This is probably not right for NLS */
}
/* IsCharAlphanumeric USER 434 */
BOOL IsCharAlphanumeric(char ch)
{
  return (ch<'0')?0:(ch<'9');
}

/* IsCharUpper USER 435 */
BOOL IsCharUpper(char ch)
{
  return isupper(ch);
}

/* IsCharUpper USER 436 */
BOOL IsCharLower(char ch)
{
  return islower(ch);
}

static char Oem2Ansi[256];
static char Ansi2Oem[256];

void InitOemAnsiTranslations()
{
  static int inited=0; /* should called called in some init function*/
  int transfile,i;
  if(inited) return;
  if(transfile=open("oem2ansi.trl",O_RDONLY)){
    read(transfile,Oem2Ansi,256);
    close(transfile);
  }
  else {  /* sets up passive translations if it does not find the file */
    for(i=0;i<256;i++)  /* Needs some fixing */
      Oem2Ansi[i]=i;  
  }
  if(transfile=open("ansi2oem.trl",O_RDONLY)){
    read(transfile,Ansi2Oem,256);
    close(transfile);
  }
  else {  /* sets up passive translations if it does not find the file */
    for(i=0;i<256;i++)  /* Needs some fixing */
      Ansi2Oem[i]=i;  
  }
  inited=1;
}

/* AnsiToOem Keyboard.5 */
int AnsiToOem(LPSTR lpAnsiStr, LPSTR lpOemStr)   /* why is this int ??? */
{
  InitOemAnsiTranslations(); /* should called called in some init function*/
  while(*lpAnsiStr){
    *lpOemStr++=Ansi2Oem[*lpAnsiStr++];
  }
  return -1;
}

/* OemToAnsi Keyboard.6 */
BOOL OemToAnsi(LPSTR lpOemStr, LPSTR lpAnsiStr)   /* why is this BOOL ???? */
{
  InitOemAnsiTranslations(); /* should called called in some init function*/
  while(*lpOemStr){
    *lpAnsiStr++=Oem2Ansi[*lpOemStr++];
  }
  return -1;
}

/* AnsiToOemBuff Keyboard.134 */
void AnsiToOemBuff(LPSTR lpAnsiStr, LPSTR lpOemStr, int nLength)
{
  int i;
  InitOemAnsiTranslations(); /* should called called in some init function*/
  for(i=0;i<nLength;i++)
    lpOemStr[i]=Ansi2Oem[lpAnsiStr[i]];  
}

/* OemToAnsi Keyboard.135 */
void OemToAnsiBuff(LPSTR lpOemStr, LPSTR lpAnsiStr, int nLength)
{
  int i;
  InitOemAnsiTranslations(); /* should called called in some init function*/
  for(i=0;i<nLength;i++)
    lpAnsiStr[i]=Oem2Ansi[lpOemStr[i]];
}
