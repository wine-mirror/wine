#ifndef _WINE_INTERNET_H_
#define _WINE_INTERNET_H_

typedef enum
{
    WH_HINIT = INTERNET_HANDLE_TYPE_INTERNET,
    WH_HFTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_FTP,
    WH_HGOPHERSESSION = INTERNET_HANDLE_TYPE_CONNECT_GOPHER,
    WH_HHTTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_HTTP,
    WH_HFILE = INTERNET_HANDLE_TYPE_FTP_FILE,
    WH_HFINDNEXT = INTERNET_HANDLE_TYPE_FTP_FIND,
    WH_HHTTPREQ = INTERNET_HANDLE_TYPE_HTTP_REQUEST,
} WH_TYPE;

typedef struct _WININETHANDLEHEADER
{
    WH_TYPE htype;
    DWORD  dwFlags;
    DWORD  dwContext;
    DWORD  dwError;
    struct _WININETHANDLEHEADER *lpwhparent;
} WININETHANDLEHEADER, *LPWININETHANDLEHEADER;


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPSTR  lpszAgent;
    LPSTR  lpszProxy;
    LPSTR  lpszProxyBypass;
    DWORD   dwAccessType;
    INTERNET_STATUS_CALLBACK lpfnStatusCB;
} WININETAPPINFOA, *LPWININETAPPINFOA;


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPSTR  lpszServerName;
    LPSTR  lpszUserName;
    INTERNET_PORT nServerPort;
    struct sockaddr_in socketAddress;
    struct hostent *phostent;
} WININETHTTPSESSIONA, *LPWININETHTTPSESSIONA;

#define HDR_ISREQUEST		0x0001
#define HDR_COMMADELIMITED	0x0002
#define HDR_SEMIDELIMITED	0x0004

typedef struct
{
    LPSTR lpszField;
    LPSTR lpszValue;
    WORD wFlags;
    WORD wCount;
} HTTPHEADERA, *LPHTTPHEADERA;


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPSTR lpszPath;
    LPSTR lpszVerb;
    LPSTR lpszHostName;
    INT	nSocketFD;
    HTTPHEADERA StdHeaders[HTTP_QUERY_MAX+1];
    HTTPHEADERA *pCustHeaders;
    INT nCustHeaders;
} WININETHTTPREQA, *LPWININETHTTPREQA;


typedef struct
{
    WININETHANDLEHEADER hdr;
    int sndSocket;
    int lstnSocket;
    struct sockaddr_in socketAddress;
    struct sockaddr_in lstnSocketAddress;
    struct hostent *phostent;
    LPSTR  lpszPassword;
    LPSTR  lpszUserName;
} WININETFTPSESSIONA, *LPWININETFTPSESSIONA;


typedef struct
{
    WININETHANDLEHEADER hdr;
    int nDataSocket;
} WININETFILE, *LPWININETFILE;


typedef struct
{
    BOOL bIsDirectory;
    LPSTR lpszName;
    DWORD nSize;
    struct tm tmLastModified;
    unsigned short permissions;
} FILEPROPERTIESA, *LPFILEPROPERTIESA;


typedef struct
{
    WININETHANDLEHEADER hdr;
    int index;
    DWORD size;
    LPFILEPROPERTIESA lpafp;
} WININETFINDNEXTA, *LPWININETFINDNEXTA;

typedef enum
{
    FTPPUTFILEA,
    FTPSETCURRENTDIRECTORYA,
    FTPCREATEDIRECTORYA,
    FTPFINDFIRSTFILEA,
    FTPGETCURRENTDIRECTORYA,
    FTPOPENFILEA,
    FTPGETFILEA,
    FTPDELETEFILEA,
    FTPREMOVEDIRECTORYA,
    FTPRENAMEFILEA,
    INTERNETFINDNEXTA,
    HTTPSENDREQUESTA,
    HTTPOPENREQUESTA,
} ASYNC_FUNC;

typedef struct WORKREQ
{
    ASYNC_FUNC asyncall;
    DWORD param1;
#define HFTPSESSION       param1

    DWORD param2;
#define LPSZLOCALFILE     param2
#define LPSZREMOTEFILE    param2
#define LPSZFILENAME      param2
#define LPSZSRCFILE       param2
#define LPSZDIRECTORY     param2
#define LPSZSEARCHFILE    param2
#define LPSZHEADER        param2
#define LPSZVERB          param2

    DWORD param3;
#define LPSZNEWREMOTEFILE param3
#define LPSZNEWFILE       param3
#define LPFINDFILEDATA    param3
#define LPDWDIRECTORY     param3
#define FDWACCESS         param3
#define LPSZDESTFILE      param3
#define DWHEADERLENGTH    param3
#define LPSZOBJECTNAME    param3

    DWORD param4;
#define DWFLAGS           param4
#define LPOPTIONAL        param4

    DWORD param5;
#define DWCONTEXT         param5
#define DWOPTIONALLENGTH  param5

    DWORD param6;
#define FFAILIFEXISTS     param6
#define LPSZVERSION       param6

    DWORD param7;
#define DWLOCALFLAGSATTRIBUTE param7
#define LPSZREFERRER          param7

    DWORD param8;
#define LPSZACCEPTTYPES       param8

    struct WORKREQ *next;
    struct WORKREQ *prev;

} WORKREQUEST, *LPWORKREQUEST;


time_t ConvertTimeString(LPCSTR asctime);

HINTERNET FTP_Connect(HINTERNET hInterent, LPCSTR lpszServerName, 
	INTERNET_PORT nServerPort, LPCSTR lpszUserName,
	LPCSTR lpszPassword, DWORD dwFlags, DWORD dwContext);

HINTERNET HTTP_Connect(HINTERNET hInterent, LPCSTR lpszServerName, 
	INTERNET_PORT nServerPort, LPCSTR lpszUserName,
	LPCSTR lpszPassword, DWORD dwFlags, DWORD dwContext);

BOOL GetAddress(LPCSTR lpszServerName, INTERNET_PORT nServerPort,
	struct hostent **phe, struct sockaddr_in *psa);

int INTERNET_WriteDataToStream(int nDataSocket, LPCVOID Buffer, DWORD BytesToWrite);
int INTERNET_ReadDataFromStream(int nDataSocket, LPVOID Buffer, DWORD BytesToRead);
void INTERNET_SetLastError(DWORD dwError);
DWORD INTERNET_GetLastError();
BOOL INTERNET_AsyncCall(LPWORKREQUEST lpWorkRequest);
LPSTR INTERNET_GetResponseBuffer();
LPSTR INTERNET_GetNextLine(INT nSocket, LPSTR lpszBuffer, LPDWORD dwBuffer);

BOOL FTP_CloseSessionHandle(LPWININETFTPSESSIONA lpwfs);
BOOL FTP_CloseFindNextHandle(LPWININETFINDNEXTA lpwfn);
BOOLAPI FTP_FtpPutFileA(HINTERNET hConnect, LPCSTR lpszLocalFile,
    LPCSTR lpszNewRemoteFile, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpSetCurrentDirectoryA(HINTERNET hConnect, LPCSTR lpszDirectory);
BOOLAPI FTP_FtpCreateDirectoryA(HINTERNET hConnect, LPCSTR lpszDirectory);
INTERNETAPI HINTERNET WINAPI FTP_FtpFindFirstFileA(HINTERNET hConnect,
    LPCSTR lpszSearchFile, LPWIN32_FIND_DATAA lpFindFileData, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpGetCurrentDirectoryA(HINTERNET hFtpSession, LPSTR lpszCurrentDirectory, 
	LPDWORD lpdwCurrentDirectory);
BOOL FTP_ConvertFileProp(LPFILEPROPERTIESA lpafp, LPWIN32_FIND_DATAA lpFindFileData);
BOOL FTP_FtpRenameFileA(HINTERNET hFtpSession, LPCSTR lpszSrc, LPCSTR lpszDest);
BOOL FTP_FtpRemoveDirectoryA(HINTERNET hFtpSession, LPCSTR lpszDirectory);
BOOL FTP_FtpDeleteFileA(HINTERNET hFtpSession, LPCSTR lpszFileName);
HINTERNET FTP_FtpOpenFileA(HINTERNET hFtpSession, LPCSTR lpszFileName, 
	DWORD fdwAccess, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpGetFileA(HINTERNET hInternet, LPCSTR lpszRemoteFile, LPCSTR lpszNewFile,
	BOOL fFailIfExists, DWORD dwLocalFlagsAttribute, DWORD dwInternetFlags,
	DWORD dwContext);

BOOLAPI HTTP_HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength);
INTERNETAPI HINTERNET WINAPI HTTP_HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes, 
	DWORD dwFlags, DWORD dwContext);
void HTTP_CloseHTTPSessionHandle(LPWININETHTTPSESSIONA lpwhs);
void HTTP_CloseHTTPRequestHandle(LPWININETHTTPREQA lpwhr);


#define MAX_REPLY_LEN	 	0x5B4


#endif /* _WINE_INTERNET_H_ */
