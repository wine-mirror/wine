/*
 * sqltypes.h
 */
#ifndef __SQLTYPES_H
#define __SQLTYPES_H

#ifndef ODBCVER
#define ODBCVER	0x0351
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WINDOWS
#define FAR
#define CALLBACK
#define SQL_API
#define	BOOL				int
typedef void*				HWND;
typedef char*				GUID;
#define GUID_DEFINED
typedef char				CHAR;
typedef void				VOID;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef unsigned char		BYTE;
typedef unsigned short 		WCHAR;
typedef WCHAR* 		        LPWSTR;
typedef const char*         LPCSTR;
typedef char*               LPSTR;
typedef DWORD*           	LPDWORD;
#endif

typedef unsigned char   SQLCHAR;

#if (ODBCVER >= 0x0300)
typedef unsigned char   SQLDATE;
typedef unsigned char   SQLDECIMAL;
typedef double          SQLDOUBLE;
typedef double          SQLFLOAT;
#endif

typedef long            SQLINTEGER;

#if (ODBCVER >= 0x0300)
typedef unsigned char   SQLNUMERIC;
#endif

typedef void *          SQLPOINTER;

#if (ODBCVER >= 0x0300)
typedef float           SQLREAL;
#endif

typedef signed short int   SQLSMALLINT;
typedef unsigned short  SQLUSMALLINT;

#if (ODBCVER >= 0x0300)
typedef unsigned char   SQLTIME;
typedef unsigned char   SQLTIMESTAMP;
typedef unsigned char   SQLVARCHAR;
#endif

typedef SQLSMALLINT     SQLRETURN;

/* typedef void * 			SQLHANDLE; */

typedef SQLINTEGER		SQLHANDLE;
#if (ODBCVER >= 0x0300)

typedef SQLHANDLE               SQLHENV;
typedef SQLHANDLE               SQLHDBC;
typedef SQLHANDLE               SQLHSTMT;
typedef SQLHANDLE               SQLHDESC;

#else
typedef SQLINTEGER              SQLHENV;
typedef SQLINTEGER              SQLHDBC;
typedef SQLINTEGER              SQLHSTMT;
#endif

typedef signed char             SCHAR;
typedef SCHAR                   SQLSCHAR;
typedef long int                SDWORD;
typedef signed short int        SWORD;
typedef unsigned long int       UDWORD;
typedef unsigned short int      UWORD;
typedef UDWORD                  SQLUINTEGER;
typedef signed long             SLONG;
typedef signed short            SSHORT;
typedef double                  SDOUBLE;
typedef double            		LDOUBLE;
typedef float                   SFLOAT;
typedef void*              		PTR;
typedef signed short            RETCODE;
typedef void*                   SQLHWND;

typedef SQLHANDLE          		HENV;
typedef SQLHANDLE          		HDBC;
typedef SQLHANDLE          		HSTMT;

#ifndef	__SQLDATE
#define	__SQLDATE
typedef struct tagDATE_STRUCT
{
        SQLSMALLINT    year;
        SQLUSMALLINT   month;
        SQLUSMALLINT   day;
} DATE_STRUCT;

#if (ODBCVER >= 0x0300)
typedef DATE_STRUCT	SQL_DATE_STRUCT;
#endif

typedef struct tagTIME_STRUCT
{
        SQLUSMALLINT   hour;
        SQLUSMALLINT   minute;
        SQLUSMALLINT   second;
} TIME_STRUCT;

#if (ODBCVER >= 0x0300)
typedef TIME_STRUCT	SQL_TIME_STRUCT;
#endif

typedef struct tagTIMESTAMP_STRUCT
{
        SQLSMALLINT    year;
        SQLUSMALLINT   month;
        SQLUSMALLINT   day;
        SQLUSMALLINT   hour;
        SQLUSMALLINT   minute;
        SQLUSMALLINT   second;
        SQLUINTEGER    fraction;
} TIMESTAMP_STRUCT;

#if (ODBCVER >= 0x0300)
typedef TIMESTAMP_STRUCT	SQL_TIMESTAMP_STRUCT;
#endif


#if (ODBCVER >= 0x0300)
typedef enum
{
	SQL_IS_YEAR						= 1,
	SQL_IS_MONTH					= 2,
	SQL_IS_DAY						= 3,
	SQL_IS_HOUR						= 4,
	SQL_IS_MINUTE					= 5,
	SQL_IS_SECOND					= 6,
	SQL_IS_YEAR_TO_MONTH			= 7,
	SQL_IS_DAY_TO_HOUR				= 8,
	SQL_IS_DAY_TO_MINUTE			= 9,
	SQL_IS_DAY_TO_SECOND			= 10,
	SQL_IS_HOUR_TO_MINUTE			= 11,
	SQL_IS_HOUR_TO_SECOND			= 12,
	SQL_IS_MINUTE_TO_SECOND			= 13
} SQLINTERVAL;

#endif

#if (ODBCVER >= 0x0300)
typedef struct tagSQL_YEAR_MONTH
{
		SQLUINTEGER		year;
		SQLUINTEGER		month;
} SQL_YEAR_MONTH_STRUCT;

typedef struct tagSQL_DAY_SECOND
{
		SQLUINTEGER		day;
		SQLUINTEGER		hour;
		SQLUINTEGER		minute;
		SQLUINTEGER		second;
		SQLUINTEGER		fraction;
} SQL_DAY_SECOND_STRUCT;

typedef struct tagSQL_INTERVAL_STRUCT
{
	SQLINTERVAL		interval_type;
	SQLSMALLINT		interval_sign;
	union {
		SQL_YEAR_MONTH_STRUCT		year_month;
		SQL_DAY_SECOND_STRUCT		day_second;
	} intval;

} SQL_INTERVAL_STRUCT;

#endif

#endif

#if (ODBCVER >= 0x0300)
#define ODBCINT64	long
#ifdef ODBCINT64
typedef ODBCINT64	SQLBIGINT;
typedef unsigned ODBCINT64	SQLUBIGINT;
#endif
#endif

#if (ODBCVER >= 0x0300)
#define SQL_MAX_NUMERIC_LEN		16
typedef struct tagSQL_NUMERIC_STRUCT
{
	SQLCHAR		precision;
	SQLSCHAR	scale;
	SQLCHAR		sign;	/* 1=pos 0=neg */
	SQLCHAR		val[SQL_MAX_NUMERIC_LEN];
} SQL_NUMERIC_STRUCT;
#endif

#if (ODBCVER >= 0x0350)
#ifdef GUID_DEFINED
typedef GUID	SQLGUID;
#else
typedef struct  tagSQLGUID
{
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
} SQLGUID;
#endif
#endif


typedef unsigned long int       BOOKMARK;

#define SQL_WCHAR             (-8)
#define SQL_C_WCHAR            SQL_WCHAR

#ifdef __cplusplus
}
#endif

#endif
