/*
 * Command line Registry implementation
 *
 * Copyright 1999 Sylvain St-Germain
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
 * Note: Please consult the README file for more information.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winreg.h>
#include <winerror.h>
#include <winnt.h>
#include <string.h>

/******************************************************************************
 * Defines and consts
 */
#define IDENTICAL             0
#define COMMAND_COUNT         7

#define KEY_MAX_LEN             1024
#define STDIN_MAX_LEN         102400

/* Return values */
#define COMMAND_NOT_FOUND     -1
#define SUCCESS               0
#define NOT_ENOUGH_MEMORY     1
#define KEY_VALUE_ALREADY_SET 2
#define COMMAND_NOT_SUPPORTED 3

/* Generic global */
static BOOL bForce            = FALSE; /* Is set to TRUE when -force is
                                          passed on the command line */

/* Globals used by the api setValue, queryValue */
static LPSTR currentKeyName   = NULL;
static HKEY  currentKeyClass  = 0;
static HKEY  currentKeyHandle = 0;
static BOOL  bTheKeyIsOpen    = FALSE;

/* Delimiters used to parse the "value"="data" pair for setValue*/
#define SET_VALUE_MAX_ARGS    2
/* Delimiters used to parse the "value" to query queryValue*/
#define QUERY_VALUE_MAX_ARGS  1

static const char *setValueDelim[SET_VALUE_MAX_ARGS]   = {"=", ""};
static const char *queryValueDelim[QUERY_VALUE_MAX_ARGS]   = {""};


/*
 * Forward declaration
 */
typedef void (*commandAPI)(LPSTR lpsLine);

static void doSetValue(LPSTR lpsLine);
static void doDeleteValue(LPSTR lpsLine);
static void doCreateKey(LPSTR lpsLine);
static void doDeleteKey(LPSTR lpsLine);
static void doQueryValue(LPSTR lpsLine);
static void doRegisterDLL(LPSTR lpsLine);
static void doUnregisterDLL(LPSTR lpsLine);

/*
 * Currently supported api
 */
static const char* commandNames[COMMAND_COUNT] = {
  "setValue",
  "deleteValue",
  "createKey",
  "deleteKey",
  "queryValue",
  "registerDLL",
  "unregisterDLL"
};

/*
 * Pointers to processing entry points
 */
static const commandAPI commandAPIs[COMMAND_COUNT] = {
  doSetValue,
  doDeleteValue,
  doCreateKey,
  doDeleteKey,
  doQueryValue,
  doRegisterDLL,
  doUnregisterDLL
};

/*
 * Generic prototypes
 */
static DWORD   getDataType(LPSTR *lpValue, DWORD* parse_type);
static LPSTR   getRegKeyName(LPSTR lpLine);
static HKEY    getRegClass(LPSTR lpLine);
static LPSTR   getArg(LPSTR arg);
static INT     getCommand(LPSTR commandName);
static DWORD   convertHexToDWord(char *str, BYTE *buf);
static DWORD   convertHexCSVToHex(char *str, BYTE *buf, ULONG bufLen);
static LPSTR   convertHexToHexCSV( BYTE *buf, ULONG len);
static LPSTR   convertHexToDWORDStr( BYTE *buf, ULONG len);
static HRESULT openKey(LPSTR stdInput);
static void    closeKey();

/*
 * api setValue prototypes
 */
static void    processSetValue(LPSTR cmdline);
static HRESULT setValue(LPSTR *argv);

/*
 * api queryValue prototypes
 */
static void    processQueryValue(LPSTR cmdline);

/*
 * Help Text displayed when invalid parameters are provided
 */
static char helpText[] =
"NAME\n"
"          regapi - perform certain actions on the wine registry.\n"
"\n"
"SYNOPSIS\n"
"          regapi commandName [-force] < file\n"
"\n"
"DESCRIPTION\n"
"          regapi modifies settings in the wine registry.  It processes\n"
"          the given commandName for every line in the stdin data stream.\n"
"          Input data format may vary depending on the commandName\n"
"          (see INPUT FILE FORMAT).\n"
"\n"
"OPTIONS\n"
"          commandName\n"
"              Instruct regapi about what action to perform on the data stream.\n"
"              Currently, only setValue and queryValue are supported and\n"
"              implemented.\n"
"\n"
"          -force\n"
"              When provided the action will be performed anyway.  This may\n"
"              have a different meaning depending on the context.  For example,\n"
"              when providing -force to setValue, the value is set even if it\n"
"              was previously set to another value.\n"
"\n"
"          < file\n"
"              STDIN channel, provide a file name with line of the appropriate\n"
"              format.\n"
"\n"
"INPUT FILE FORMAT\n"
"\n"
"          setValue\n"
"              The input file is in format, obtained from regedit.exe export.\n"
"\n"
"              [KEY_CLASS\\Some\\Path\\For\\A\\Key]\n"
"              \"Value1\"=\"Data1\"\n"
"              \"Value2\"=\"Data2\"\n"
"              \"Valuen\"=\"Datan\"\n"
"              ...\n"
"\n"
"          queryValue\n"
"              The input file format required by the queryValue command is\n"
"              similar to the one required by setValue.  The only\n"
"              difference is that you only provide the value name.\n"
"\n"
"              [KEY_CLASS\\Some\\Path\\For\\A\\Key]\n"
"              \"Value1\"\n"
"              \"Value2\"\n"
"              \"Valuen\"\n"
"              ...\n"
"           registerDLL\n"
"               The input file format is a list of DLLs to register\n"
"\n"
"           unregisterDLL\n"
"               The input file format is a list of DLLs to unregister\n"
"                                February 1999.\n"
;


/******************************************************************************
 * This function returns the HKEY associated with the data type encoded in the
 * value.  It modifies the input parameter (key value) in order to skip this
 * "now useless" data type information.
 *
 * Note: Updated based on the algorithm used in 'server/registry.c'
 */
DWORD getDataType(LPSTR *lpValue, DWORD* parse_type)
{
    struct data_type { const char *tag; int len; int type; int parse_type; };

    static const struct data_type data_types[] =
    {                   /* actual type */  /* type to assume for parsing */
        { "\"",        1,   REG_SZ,              REG_SZ },
        { "str:\"",    5,   REG_SZ,              REG_SZ },
        { "str(2):\"", 8,   REG_EXPAND_SZ,       REG_SZ },
        { "str(7):\"", 8,   REG_MULTI_SZ,        REG_SZ },
        { "hex:",      4,   REG_BINARY,          REG_BINARY },
        { "dword:",    6,   REG_DWORD,           REG_DWORD },
        { "hex(",      4,   -1,                  REG_BINARY },
        { NULL,        0,    0,                  0 }
    };

    const struct data_type *ptr;
    int type;

    for (ptr = data_types; ptr->tag; ptr++)
    {
        if (memcmp( ptr->tag, *lpValue, ptr->len ))
            continue;

        /* Found! */
        *parse_type = ptr->parse_type;
        type=ptr->type;
        *lpValue+=ptr->len;
        if (type == -1) {
            char* end;
            /* "hex(xx):" is special */
            *lpValue += 4;
            type = (int)strtoul( *lpValue , &end, 16 );
            if (**lpValue=='\0' || *end!=')' || *(end+1)!=':') {
                type=REG_NONE;
            } else {
                *lpValue=end+2;
            }
        }
        return type;
    }
    return (**lpValue=='\0'?REG_SZ:REG_NONE);
}

/******************************************************************************
 * Extracts from a [HKEY\some\key\path] type of line the key name (what starts
 * after the first '\' and end before the ']'
 */
LPSTR getRegKeyName(LPSTR lpLine)
{
  LPSTR keyNameBeg = NULL;
  LPSTR keyNameEnd = NULL;
  char  lpLineCopy[KEY_MAX_LEN];

  if (lpLine == NULL)
    return NULL;

  strcpy(lpLineCopy, lpLine);

  keyNameBeg  = strstr(lpLineCopy, "\\");  /* The key name start by '\' */
  keyNameBeg++;                            /* but is not part of the key name */
  keyNameEnd  = strstr(lpLineCopy, "]");   /* The key name end by ']' */
  *keyNameEnd = '\0';                      /* Isolate the key name */

  currentKeyName = HeapAlloc(GetProcessHeap(), 0, strlen(keyNameBeg)+1);
  if (currentKeyName != NULL)
    strcpy(currentKeyName, keyNameBeg);

  return currentKeyName;
}

/******************************************************************************
 * Extracts from a [HKEY/some/key/path] type of line the key class (what
 * starts after the '[' and ends before the first '\'
 */
static HKEY getRegClass(LPSTR lpClass)
{
  LPSTR classNameEnd;
  LPSTR classNameBeg;

  char  lpClassCopy[KEY_MAX_LEN];

  if (lpClass == NULL)
    return (HKEY)ERROR_INVALID_PARAMETER;

  strcpy(lpClassCopy, lpClass);

  classNameEnd  = strstr(lpClassCopy, "\\");  /* The class name end by '\' */
  *classNameEnd = '\0';                       /* Isolate the class name */
  classNameBeg  = &lpClassCopy[1];            /* Skip the '[' */

  if      (strcmp( classNameBeg, "HKEY_LOCAL_MACHINE") == IDENTICAL )
    return  HKEY_LOCAL_MACHINE;
  else if (strcmp( classNameBeg, "HKEY_USERS") == IDENTICAL )
    return  HKEY_USERS;
  else if (strcmp( classNameBeg, "HKEY_CLASSES_ROOT") == IDENTICAL )
    return  HKEY_CLASSES_ROOT;
  else if (strcmp( classNameBeg, "HKEY_CURRENT_CONFIG") == IDENTICAL )
    return  HKEY_CURRENT_CONFIG;
  else if (strcmp( classNameBeg, "HKEY_CURRENT_USER") == IDENTICAL )
    return  HKEY_CURRENT_USER;
  else
    return (HKEY)ERROR_INVALID_PARAMETER;
}

/******************************************************************************
 * This is a replacement for strsep which is not portable (missing on Solaris).
 */
static char* getToken(char** str, const char* delims)
{
    char* token;

    if (*str==NULL) {
        /* No more tokens */
        return NULL;
    }

    token=*str;
    while (**str!='\0') {
        if (strchr(delims,**str)!=NULL) {
            **str='\0';
            (*str)++;
            return token;
        }
        (*str)++;
    }
    /* There is no other token */
    *str=NULL;
    return token;
}

/******************************************************************************
 * Returns an allocated buffer with a cleaned copy (removed the surrounding
 * dbl quotes) of the passed value.
 */
static LPSTR getArg( LPSTR arg)
{
  LPSTR tmp = NULL;
  ULONG len;

  if (arg == NULL)
    return NULL;

  /*
   * Get rid of surrounding quotes
   */
  len = strlen(arg);

  if( arg[len-1] == '\"' ) arg[len-1] = '\0';
  if( arg[0]     == '\"' ) arg++;

  tmp = HeapAlloc(GetProcessHeap(), 0, strlen(arg)+1);
  strcpy(tmp, arg);

  return tmp;
}

/******************************************************************************
 * Returns the index in the commands array of the command to process.
 */
static INT getCommand(LPSTR commandName)
{
  INT count;
  for (count=0; count < COMMAND_COUNT; count++)
    if ( strcmp(commandName, commandNames[count]) == IDENTICAL)
      return count;

  return COMMAND_NOT_FOUND;
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
static DWORD convertHexToDWord(char *str, BYTE *buf)
{
  DWORD dw;
  char xbuf[9];

  memcpy(xbuf,str,8);
  xbuf[8]='\0';
  sscanf(xbuf,"%08lx",&dw);
  memcpy(buf,&dw,sizeof(DWORD));
  return sizeof(DWORD);
}

/******************************************************************************
 * Converts a hex buffer into a hex comma separated values
 */
static char* convertHexToHexCSV(BYTE *buf, ULONG bufLen)
{
  char* str;
  char* ptrStr;
  BYTE* ptrBuf;

  ULONG current = 0;

  str    = HeapAlloc(GetProcessHeap(), 0, (bufLen+1)*2);
  memset(str, 0, (bufLen+1)*2);
  ptrStr = str;  /* Pointer to result  */
  ptrBuf = buf;  /* Pointer to current */

  while (current < bufLen)
  {
    BYTE bCur = ptrBuf[current++];
    char res[3];

    sprintf(res, "%02x", (unsigned int)*&bCur);
    strcat(str, res);
    strcat(str, ",");
  }

  /* Get rid of the last comma */
  str[strlen(str)-1] = '\0';
  return str;
}

/******************************************************************************
 * Converts a hex buffer into a DWORD string
 */
static char* convertHexToDWORDStr(BYTE *buf, ULONG bufLen)
{
  char* str;
  DWORD dw;

  if ( bufLen != sizeof(DWORD) ) return NULL;

  str = HeapAlloc(GetProcessHeap(), 0, (bufLen*2)+1);

  memcpy(&dw,buf,sizeof(DWORD));
  sprintf(str, "%08lx", dw);

  /* Get rid of the last comma */
  return str;
}
/******************************************************************************
 * Converts a hex comma separated values list into a hex list.
 */
static DWORD convertHexCSVToHex(char *str, BYTE *buf, ULONG bufLen)
{
  char *s = str;  /* Pointer to current */
  char *b = buf;  /* Pointer to result  */

  ULONG strLen    = strlen(str);
  ULONG strPos    = 0;
  DWORD byteCount = 0;

  memset(buf, 0, bufLen);

  /*
   * warn the user if we are here with a string longer than 2 bytes that does
   * not contains ",".  It is more likely because the data is invalid.
   */
  if ( ( strlen(str) > 2) && ( strstr(str, ",") == NULL) )
    printf("regapi: WARNING converting CSV hex stream with no comma, "
           "input data seems invalid.\n");

  while (strPos < strLen)
  {
    char xbuf[3];
    char wc;

    memcpy(xbuf,s,2); xbuf[3]='\0';
    sscanf(xbuf,"%02x",(UINT*)&wc);
    *b++ =(unsigned char)wc;

    s+=3;
    strPos+=3;
    byteCount++;
  }

  return byteCount;
}


/******************************************************************************
 * Sets the value in argv[0] to the data in argv[1] for the currently
 * opened key.
 */
static HRESULT setValue(LPSTR *argv)
{
  HRESULT hRes;
  DWORD   dwSize          = KEY_MAX_LEN;
  DWORD   dwType          = 0;
  DWORD   dwDataType,dwParseType;

  LPSTR   lpsCurrentValue;

  LPSTR   keyValue = getArg(argv[0]);
  LPSTR   keyData  = argv[1];

  /* Make some checks */
  if ( (keyValue == NULL) || (keyData == NULL) )
    return ERROR_INVALID_PARAMETER;

  lpsCurrentValue=HeapAlloc(GetProcessHeap(), 0,KEY_MAX_LEN);
  /*
   * Default registry values are encoded in the input stream as '@' but as
   * blank in the wine registry.
   */
  if( (keyValue[0] == '@') && (strlen(keyValue) == 1) )
    keyValue[0] = '\0';

  /* Get the data type stored into the value field */
  dwDataType = getDataType(&keyData,&dwParseType);

  memset(lpsCurrentValue, 0, KEY_MAX_LEN);
  hRes = RegQueryValueExA(
          currentKeyHandle,
          keyValue,
          NULL,
          &dwType,
          (LPBYTE)lpsCurrentValue,
          &dwSize);

  while(hRes==ERROR_MORE_DATA){
      dwSize+=KEY_MAX_LEN;
      lpsCurrentValue=HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lpsCurrentValue,dwSize);
      hRes = RegQueryValueExA(currentKeyHandle,keyValue,NULL,&dwType,(LPBYTE)lpsCurrentValue,&dwSize);
  }

  if( ( strlen(lpsCurrentValue) == 0 ) ||  /* The value is not existing */
      ( bForce ))         /* -force option */
  {
    LPBYTE lpbData;
    BYTE   convert[KEY_MAX_LEN];
    DWORD  dwLen;

    if ( dwParseType == REG_SZ)        /* no conversion for string */
    {
      dwLen   = strlen(keyData);
      if (dwLen>0 && keyData[dwLen-1]=='"')
      {
        dwLen--;
        keyData[dwLen]='\0';
      }
      lpbData = keyData;
    }
    else if (dwParseType == REG_DWORD)  /* Convert the dword types */
    {
      dwLen   = convertHexToDWord(keyData, convert);
      lpbData = convert;
    }
    else                               /* Convert the hexadecimal types */
    {
      dwLen   = convertHexCSVToHex(keyData, convert, KEY_MAX_LEN);
      lpbData = convert;
    }

    hRes = RegSetValueEx(
            currentKeyHandle,
            keyValue,
            0,                  /* Reserved */
            dwDataType,
            lpbData,
            dwLen);
  }
  else
  {
    /* return the current value data into argv[1] */
    if (argv[1] != NULL)
    {
      HeapFree(GetProcessHeap(), 0, argv[1]);
      argv[1] = HeapAlloc(GetProcessHeap(), 0, dwSize+1);

      if ( argv[1] != NULL ) {
        strncpy(argv[1], lpsCurrentValue, dwSize);
        argv[1][dwSize]='\0';
      }
    }

    hRes=KEY_VALUE_ALREADY_SET;
  }
  if (keyValue != NULL)
      HeapFree(GetProcessHeap(), 0, keyValue);
  return hRes;
}


/******************************************************************************
 * Open the key
 */
static HRESULT openKey( LPSTR stdInput)
{
  DWORD   dwDisp;
  HRESULT hRes;

  /* Sanity checks */
  if (stdInput == NULL)
    return ERROR_INVALID_PARAMETER;

  /* Get the registry class */
  currentKeyClass = getRegClass(stdInput); /* Sets global variable */
  if (currentKeyClass == (HKEY)ERROR_INVALID_PARAMETER)
    return (HRESULT)ERROR_INVALID_PARAMETER;

  /* Get the key name */
  currentKeyName = getRegKeyName(stdInput); /* Sets global variable */
  if (currentKeyName == NULL)
    return ERROR_INVALID_PARAMETER;

  hRes = RegCreateKeyEx(
          currentKeyClass,          /* Class     */
          currentKeyName,           /* Sub Key   */
          0,                        /* MUST BE 0 */
          NULL,                     /* object type */
          REG_OPTION_NON_VOLATILE,  /* option, REG_OPTION_NON_VOLATILE ... */
          KEY_ALL_ACCESS,           /* access mask, KEY_ALL_ACCESS */
          NULL,                     /* security attribute */
          &currentKeyHandle,        /* result */
          &dwDisp);                 /* disposition, REG_CREATED_NEW_KEY or
                                                    REG_OPENED_EXISTING_KEY */

  if (hRes == ERROR_SUCCESS)
    bTheKeyIsOpen = TRUE;

  return hRes;

}
/******************************************************************************
 * This function is a wrapper for the setValue function.  It prepares the
 * land and clean the area once completed.
 */
static void processSetValue(LPSTR cmdline)
{
  LPSTR argv[SET_VALUE_MAX_ARGS];  /* args storage    */

  LPSTR token         = NULL;      /* current token analyzed */
  ULONG argCounter    = 0;         /* counter of args */
  INT   counter;
  HRESULT hRes = 0;

  /*
   * Init storage and parse the line
   */
  for (counter=0; counter<SET_VALUE_MAX_ARGS; counter++)
    argv[counter]=NULL;

  while( (token = getToken(&cmdline, setValueDelim[argCounter])) != NULL )
  {
    argv[argCounter++] = token;

    if (argCounter == SET_VALUE_MAX_ARGS)
      break;  /* Stop processing args no matter what */
  }

  hRes = setValue(argv);
  if ( hRes == ERROR_SUCCESS )
    printf(
      "regapi: Value \"%s\" has been set to \"%s\" in key [%s]\n",
      argv[0],
      argv[1],
      currentKeyName);

  else if ( hRes == KEY_VALUE_ALREADY_SET )
    printf(
      "regapi: Value \"%s\" already set to \"%s\" in key [%s]\n",
      argv[0],
      argv[1],
      currentKeyName);

  else
    printf("regapi: ERROR Key %s not created. Value: %s, Data: %s\n",
      currentKeyName,
      argv[0],
      argv[1]);
}

/******************************************************************************
 * This function is a wrapper for the queryValue function.  It prepares the
 * land and clean the area once completed.
 */
static void processQueryValue(LPSTR cmdline)
{
  LPSTR   argv[QUERY_VALUE_MAX_ARGS];/* args storage    */
  LPSTR   token      = NULL;         /* current token analyzed */
  ULONG   argCounter = 0;            /* counter of args */
  INT     counter;
  HRESULT hRes       = 0;
  LPSTR   keyValue   = NULL;
  LPSTR   lpsRes     = NULL;

  /*
   * Init storage and parse the line
   */
  for (counter=0; counter<QUERY_VALUE_MAX_ARGS; counter++)
    argv[counter]=NULL;

  while( (token = getToken(&cmdline, queryValueDelim[argCounter])) != NULL )
  {
    argv[argCounter++] = getArg(token);

    if (argCounter == QUERY_VALUE_MAX_ARGS)
      break;  /* Stop processing args no matter what */
  }

  /* The value we look for is the first token on the line */
  if ( argv[0] == NULL )
    return; /* SHOULD NOT HAPPEN */
  else
    keyValue = argv[0];

  if( (keyValue[0] == '@') && (strlen(keyValue) == 1) )
  {
    LONG  lLen  = KEY_MAX_LEN;
    CHAR*  lpsData=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,KEY_MAX_LEN);
    /*
     * We need to query the key default value
     */
    hRes = RegQueryValue(
             currentKeyHandle,
             currentKeyName,
             (LPBYTE)lpsData,
             &lLen);

    if (hRes==ERROR_MORE_DATA) {
        lpsData=HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lpsData,lLen);
        hRes = RegQueryValue(currentKeyHandle,currentKeyName,(LPBYTE)lpsData,&lLen);
    }

    if (hRes == ERROR_SUCCESS)
    {
      lpsRes = HeapAlloc( GetProcessHeap(), 0, lLen);
      strncpy(lpsRes, lpsData, lLen);
      lpsRes[lLen-1]='\0';
    }
    HeapFree(GetProcessHeap(), 0, lpsData);
  }
  else
  {
    DWORD  dwLen  = KEY_MAX_LEN;
    BYTE*  lpbData=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,KEY_MAX_LEN);
    DWORD  dwType;
    /*
     * We need to query a specific value for the key
     */
    hRes = RegQueryValueEx(
             currentKeyHandle,
             keyValue,
             0,
             &dwType,
             (LPBYTE)lpbData,
             &dwLen);

    if (hRes==ERROR_MORE_DATA) {
        lpbData=HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lpbData,dwLen);
        hRes = RegQueryValueEx(currentKeyHandle,keyValue,NULL,&dwType,(LPBYTE)lpbData,&dwLen);
    }

    if (hRes == ERROR_SUCCESS)
    {
      /*
       * Convert the returned data to a displayable format
       */
      switch ( dwType )
      {
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
          lpsRes = HeapAlloc( GetProcessHeap(), 0, dwLen);
          strncpy(lpsRes, lpbData, dwLen);
          lpsRes[dwLen-1]='\0';
          break;
        }
        case REG_DWORD:
        {
          lpsRes = convertHexToDWORDStr(lpbData, dwLen);
          break;
        }
        default:
        {
          lpsRes = convertHexToHexCSV(lpbData, dwLen);
          break;
        }
      }
    }

    HeapFree(GetProcessHeap(), 0, lpbData);
  }


  if ( hRes == ERROR_SUCCESS )
    printf(
      "regapi: Value \"%s\" = \"%s\" in key [%s]\n",
      keyValue,
      lpsRes,
      currentKeyName);

  else
    printf("regapi: ERROR Value \"%s\" not found. for key \"%s\"\n",
      keyValue,
      currentKeyName);

  /*
   * Do some cleanup
   */
  for (counter=0; counter<argCounter; counter++)
    if (argv[counter] != NULL)
      HeapFree(GetProcessHeap(), 0, argv[counter]);

  if (lpsRes != NULL)
    HeapFree(GetProcessHeap(), 0, lpsRes);

}

/******************************************************************************
 * Close the currently opened key.
 */
static void closeKey()
{
  RegCloseKey(currentKeyHandle);

  HeapFree(GetProcessHeap(), 0, currentKeyName); /* Allocated by getKeyName */

  bTheKeyIsOpen    = FALSE;

  currentKeyName   = NULL;
  currentKeyClass  = 0;
  currentKeyHandle = 0;
}

/******************************************************************************
 * This funtion is the main entry point to the setValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
static void doSetValue(LPSTR stdInput)
{
  /*
   * We encoutered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL)
  {
    if (bTheKeyIsOpen != FALSE)
      closeKey();

    return;
  }

  if      ( stdInput[0] == '[')      /* We are reading a new key */
  {
    if ( bTheKeyIsOpen != FALSE )
      closeKey();                    /* Close the previous key before */

    if ( openKey(stdInput) != ERROR_SUCCESS )
      printf ("regapi: doSetValue failed to open key %s\n", stdInput);
  }
  else if( ( bTheKeyIsOpen ) &&
           (( stdInput[0] == '@') || /* reading a default @=data pair */
            ( stdInput[0] == '\"'))) /* reading a new value=data pair */
  {
    processSetValue(stdInput);
  }
  else                               /* since we are assuming that the */
  {                                  /* file format is valid we must   */
    if ( bTheKeyIsOpen )             /* be reading a blank line which  */
      closeKey();                    /* indicate end of this key processing */
  }
}

/******************************************************************************
 * This funtion is the main entry point to the queryValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
static void doQueryValue(LPSTR stdInput) {
  /*
   * We encoutered the end of the file, make sure we
   * close the opened key and exit
   */
  if (stdInput == NULL)
  {
    if (bTheKeyIsOpen != FALSE)
      closeKey();

    return;
  }

  if      ( stdInput[0] == '[')      /* We are reading a new key */
  {
    if ( bTheKeyIsOpen != FALSE )
      closeKey();                    /* Close the previous key before */

    if ( openKey(stdInput) != ERROR_SUCCESS )
      printf ("regapi: doSetValue failed to open key %s\n", stdInput);
  }
  else if( ( bTheKeyIsOpen ) &&
           (( stdInput[0] == '@') || /* reading a default @=data pair */
            ( stdInput[0] == '\"'))) /* reading a new value=data pair */
  {
    processQueryValue(stdInput);
  }
  else                               /* since we are assuming that the */
  {                                  /* file format is valid we must   */
    if ( bTheKeyIsOpen )             /* be reading a blank line which  */
      closeKey();                    /* indicate end of this key processing */
  }
}

/******************************************************************************
 * This funtion is the main entry point to the deletetValue type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
static void doDeleteValue(LPSTR line) {
  printf ("regapi: deleteValue not yet implemented\n");
}
/******************************************************************************
 * This funtion is the main entry point to the deleteKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
static void doDeleteKey(LPSTR line)   {
  printf ("regapi: deleteKey not yet implemented\n");
}
/******************************************************************************
 * This funtion is the main entry point to the createKey type of action.  It
 * receives the currently read line and dispatch the work depending on the
 * context.
 */
static void doCreateKey(LPSTR line)   {
  printf ("regapi: createKey not yet implemented\n");
}

/******************************************************************************
 * This funtion is the main entry point to the registerDLL action.  It
 * receives the currently read line, then loads and registers the requested DLLs
 */
static void doRegisterDLL(LPSTR stdInput) {
  HMODULE theLib = 0;
  UINT retVal = 0;

  /* Check for valid input */
  if (stdInput == NULL)
    return;

  /* Load and register the library, then free it */
  theLib = LoadLibrary(stdInput);
  if (theLib)
  {
    FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllRegisterServer");
    if (lpfnDLLRegProc)
      retVal = (*lpfnDLLRegProc)();
    else
      printf("regapi: Couldn't find DllRegisterServer proc in '%s'.\n", stdInput);

    if (retVal != S_OK)
      printf("regapi: DLLRegisterServer error 0x%x in '%s'.\n", retVal, stdInput);

    FreeLibrary(theLib);
  }
  else
  {
    printf("regapi: Could not load DLL '%s'.\n", stdInput);
  }
}

/******************************************************************************
 * This funtion is the main entry point to the unregisterDLL action.  It
 * receives the currently read line, then loads and unregisters the requested DLLs
 */
static void doUnregisterDLL(LPSTR stdInput) {
  HMODULE theLib = 0;
  UINT retVal = 0;

  /* Check for valid input */
  if (stdInput == NULL)
    return;

  /* Load and unregister the library, then free it */
  theLib = LoadLibrary(stdInput);
  if (theLib)
  {
    FARPROC lpfnDLLRegProc = GetProcAddress(theLib, "DllUnregisterServer");
    if (lpfnDLLRegProc)
      retVal = (*lpfnDLLRegProc)();
    else
      printf("regapi: Couldn't find DllUnregisterServer proc in '%s'.\n", stdInput);

    if (retVal != S_OK)
      printf("regapi: DLLUnregisterServer error 0x%x in '%s'.\n", retVal, stdInput);

    FreeLibrary(theLib);
  }
  else
  {
    printf("regapi: Could not load DLL '%s'.\n", stdInput);
  }
}

/******************************************************************************
 * MAIN - WinMain simply validates the first parameter (command to perform)
 *        It then reads the STDIN line by line forwarding their processing
 *        to the appropriate method.
 */
int PASCAL WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
  LPSTR  token          = NULL;  /* current token analyzed */
  LPSTR  stdInput       = NULL;  /* line read from stdin */
  INT    cmdIndex       = -1;    /* index of the command in array */
  LPSTR nextLine        = NULL;
  ULONG  currentSize    = STDIN_MAX_LEN;

  stdInput = HeapAlloc(GetProcessHeap(), 0, STDIN_MAX_LEN);
  nextLine = HeapAlloc(GetProcessHeap(), 0, STDIN_MAX_LEN);

  if (stdInput == NULL || nextLine== NULL)
    return NOT_ENOUGH_MEMORY;

  /*
   * get the command, should be the first arg (modify cmdLine)
   */
  token = getToken(&cmdline, " ");
  if (token != NULL)
  {
    cmdIndex = getCommand(token);
    if (cmdIndex == COMMAND_NOT_FOUND)
    {
      printf("regapi: Command \"%s\" is not supported.\n", token);
      printf(helpText);
      return COMMAND_NOT_SUPPORTED;
    }
  }
  else
  {
    printf(
      "regapi: The first item on the command line must be the command name.\n");
    printf(helpText);
    return COMMAND_NOT_SUPPORTED;
  }

  /*
   * check to see wether we force the action
   * (meaning differs depending on the command performed)
   */
  if ( cmdline != NULL ) /* will be NULL if '-force' is not provided */
    if ( strstr(cmdline, "-force") != NULL )
      bForce = TRUE;

  printf("Processing stdin...\n");

  while ( TRUE )
  {
    /*
     * read a line
     */
      ULONG curSize=STDIN_MAX_LEN;
      char* s=NULL;

      while((NULL!=(stdInput=fgets(stdInput,curSize,stdin))) && (NULL==(s=strchr(stdInput,'\n'))) ){
          fseek(stdin,-curSize,SEEK_CUR+1);
          stdInput=HeapReAlloc(GetProcessHeap(), 0,stdInput,curSize+=STDIN_MAX_LEN);
      }
    /*
     * Make some handy generic stuff here...
     */
    if ( stdInput != NULL )
    {
      stdInput[strlen(stdInput) -1] = '\0'; /* get rid of new line */

      if( stdInput[0] == '#' )              /* this is a comment, skip */
        continue;

      while( stdInput[strlen(stdInput) -1] == '\\' ){ /* a '\' char in the end of the current line means  */
                                                      /* that this line is not complete and we have to get */
          stdInput[strlen(stdInput) -1]= '\0';        /* the rest in the next lines */

          nextLine = fgets(nextLine, STDIN_MAX_LEN, stdin);

          nextLine[strlen(nextLine)-1] = '\0';

          if ( (strlen(stdInput)+strlen(nextLine)) > currentSize){

              stdInput=HeapReAlloc(GetProcessHeap(),0,stdInput,strlen(stdInput)+STDIN_MAX_LEN);

              currentSize+=STDIN_MAX_LEN;
          }

          strcat(stdInput,nextLine+2);
      }

    }

    /*
     * We process every line, even the NULL (last) line, to indicate the
     * end of the processing to the specific process.
     */
    commandAPIs[cmdIndex](stdInput);

    if (stdInput == NULL)  /* EOF encountered */
      break;
  }

#if 0
  /*
   * Save the registry only if it was modified
   */
 if ( commandSaveRegistry[cmdIndex] != FALSE )
       SHELL_SaveRegistry();
#endif
  HeapFree(GetProcessHeap(), 0, nextLine);

  HeapFree(GetProcessHeap(), 0, stdInput);

  return SUCCESS;
}
