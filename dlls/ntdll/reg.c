/*
 *	registry functions
 *
 * NOTES:
 * 	HKEY_LOCAL_MACHINE	\\REGISTRY\\MACHINE
 *	HKEY_USERS		\\REGISTRY\\USER
 *	HKEY_CURRENT_CONFIG	\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT
  *	HKEY_CLASSES		\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES
 */

#include "debugtools.h"
#include "winreg.h"
#include "winerror.h"
#include "file.h"
#include "server.h"
#include "ntddk.h"
#include "crtdll.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

/* copy a key name into the request buffer */
static inline NTSTATUS copy_nameU( LPWSTR Dest, PUNICODE_STRING Name, UINT Offset )
{
	if (Name->Buffer) 
	{
	  if ((Name->Length-Offset) > MAX_PATH) return STATUS_BUFFER_OVERFLOW;
	  lstrcpyW( Dest, Name->Buffer+Offset );
	}
	else Dest[0] = 0;
	return STATUS_SUCCESS;
}

/* translates predefined paths to HKEY_ constants */
static BOOLEAN _NtKeyToWinKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT UINT * Offset,	/* offset within ObjectName */
	OUT HKEY * KeyHandle)	/* translated handle */
{
	static const WCHAR KeyPath_HKLM[] = {
		'\\','R','E','G','I','S','T','R','Y',
		'\\','M','A','C','H','I','N','E',0};
	static const WCHAR KeyPath_HKU [] = {
		'\\','R','E','G','I','S','T','R','Y',
		'\\','U','S','E','R',0};
	static const WCHAR KeyPath_HCC [] = {
		'\\','R','E','G','I','S','T','R','Y',
		'\\','M','A','C','H','I','N','E',
		'\\','S','Y','S','T','E','M',
		'\\','C','U','R','R','E','N','T','C','O','N','T','R','O','L','S','E','T',
		'\\','H','A','R','D','W','A','R','E','P','R','O','F','I','L','E','S',
		'\\','C','U','R','R','E','N','T',0};
	static const WCHAR KeyPath_HCR [] = {
		'\\','R','E','G','I','S','T','R','Y',
		'\\','M','A','C','H','I','N','E',
		'\\','S','O','F','T','W','A','R','E',
		'\\','C','L','A','S','S','E','S',0};
	int len;
	PUNICODE_STRING ObjectName = ObjectAttributes->ObjectName;

	if(ObjectAttributes->RootDirectory)
	{
	  len = 0;
	  *KeyHandle = ObjectAttributes->RootDirectory;
	}
	else if((ObjectName->Length > (len=lstrlenW(KeyPath_HKLM)))
	&& (0==CRTDLL__wcsnicmp(ObjectName->Buffer,KeyPath_HKLM,len)))
	{  *KeyHandle = HKEY_LOCAL_MACHINE;
	}
	else if((ObjectName->Length > (len=lstrlenW(KeyPath_HKU)))
	&& (0==CRTDLL__wcsnicmp(ObjectName->Buffer,KeyPath_HKU,len)))
	{  *KeyHandle = HKEY_USERS;
	}
	else if((ObjectName->Length > (len=lstrlenW(KeyPath_HCR)))
	&& (0==CRTDLL__wcsnicmp(ObjectName->Buffer,KeyPath_HCR,len)))
	{  *KeyHandle = HKEY_CLASSES_ROOT;
	}
	else if((ObjectName->Length > (len=lstrlenW(KeyPath_HCC)))
	&& (0==CRTDLL__wcsnicmp(ObjectName->Buffer,KeyPath_HCC,len)))
	{  *KeyHandle = HKEY_CURRENT_CONFIG;
	}
	else
	{
	  *KeyHandle = 0;
	  *Offset = 0;
	  return FALSE;
	}

	if (len > 0 && ObjectName->Buffer[len] == (WCHAR)'\\') len++;
	*Offset = len;

	TRACE("off=%u hkey=0x%08x\n", *Offset, *KeyHandle);
	return TRUE;
}

/******************************************************************************
 * NtCreateKey [NTDLL]
 * ZwCreateKey
 */
NTSTATUS WINAPI NtCreateKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG Disposition)
{
	struct create_key_request *req = get_req_buffer();
	UINT ObjectNameOffset;
	HKEY RootDirectory;
	NTSTATUS ret;

	TRACE("(%p,0x%08lx,0x%08lx,%p(%s),0x%08lx,%p)\n",
	KeyHandle, DesiredAccess, TitleIndex, Class, debugstr_us(Class), CreateOptions, Disposition);
	dump_ObjectAttributes(ObjectAttributes);

	if (!KeyHandle)
	  return STATUS_INVALID_PARAMETER;

	_NtKeyToWinKey(ObjectAttributes, &ObjectNameOffset, &RootDirectory);

	req->parent  = RootDirectory;
	req->access  = DesiredAccess;
	req->options = CreateOptions;
	req->modif   = time(NULL);

	if (copy_nameU( req->name, ObjectAttributes->ObjectName, ObjectNameOffset ) != STATUS_SUCCESS) 
	  return STATUS_INVALID_PARAMETER;

	if (Class)
	{
	  int ClassLen = Class->Length+1;
	  if ( ClassLen*sizeof(WCHAR) > server_remaining(req->class)) return STATUS_BUFFER_OVERFLOW;
	  lstrcpynW( req->class, Class->Buffer, ClassLen);
	}
	else
	  req->class[0] = 0x0000;

	if (!(ret = server_call_noerr(REQ_CREATE_KEY)))
	{
	  *KeyHandle = req->hkey;
	  if (Disposition) *Disposition = req->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
	}
	return ret;
}

/******************************************************************************
 * NtOpenKey [NTDLL.129]
 * ZwOpenKey
 *   OUT	PHANDLE			KeyHandle (returns 0 when failure)
 *   IN		ACCESS_MASK		DesiredAccess
 *   IN		POBJECT_ATTRIBUTES 	ObjectAttributes 
 */
NTSTATUS WINAPI NtOpenKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes) 
{
	struct open_key_request *req = get_req_buffer();
	UINT ObjectNameOffset;
	HKEY RootDirectory;
	NTSTATUS ret;

	TRACE("(%p,0x%08lx)\n", KeyHandle, DesiredAccess);
	dump_ObjectAttributes(ObjectAttributes);

	if (!KeyHandle) return STATUS_INVALID_PARAMETER;
	*KeyHandle = 0;

	_NtKeyToWinKey(ObjectAttributes, &ObjectNameOffset, &RootDirectory);

	req->parent = RootDirectory;
	req->access = DesiredAccess;

	if (copy_nameU( req->name, ObjectAttributes->ObjectName, ObjectNameOffset ) != STATUS_SUCCESS) 
	  return STATUS_INVALID_PARAMETER;

	if (!(ret = server_call_noerr(REQ_OPEN_KEY)))
	{
	  *KeyHandle = req->hkey;
	}
	return ret;
}

/******************************************************************************
 * NtDeleteKey [NTDLL]
 * ZwDeleteKey
 */
NTSTATUS WINAPI NtDeleteKey(HANDLE KeyHandle)
{
	FIXME("(0x%08x) stub!\n",
	KeyHandle);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtDeleteValueKey [NTDLL]
 * ZwDeleteValueKey
 */
NTSTATUS WINAPI NtDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName)
{
	FIXME("(0x%08x,%p(%s)) stub!\n",
	KeyHandle, ValueName,debugstr_us(ValueName));
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtEnumerateKey [NTDLL]
 * ZwEnumerateKey
 *
 * NOTES
 *  the name copied into the buffer is NOT 0-terminated 
 */
NTSTATUS WINAPI NtEnumerateKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	struct enum_key_request *req = get_req_buffer();
	NTSTATUS ret;

	TRACE("(0x%08x,0x%08lx,0x%08x,%p,0x%08lx,%p)\n",
	KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);

	req->hkey = KeyHandle;
	req->index = Index;
	if ((ret = server_call_noerr(REQ_ENUM_KEY)) != STATUS_SUCCESS) return ret;

	switch (KeyInformationClass)
	{
	  case KeyBasicInformation:
	    {
	      PKEY_BASIC_INFORMATION kbi = KeyInformation;
	      UINT NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      *ResultLength = sizeof(KEY_BASIC_INFORMATION) - sizeof(WCHAR) + NameLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kbi->LastWriteTime, 0);
	      kbi->TitleIndex = 0;
	      kbi->NameLength = NameLength;
	      memcpy (kbi->Name, req->name, NameLength);
	    }
	    break;
	  case KeyFullInformation:
	    {
	      PKEY_FULL_INFORMATION kfi = KeyInformation;
	      kfi->ClassLength = lstrlenW(req->class) * sizeof(WCHAR);
	      kfi->ClassOffset = (kfi->ClassLength) ?
	        sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) : 0xffffffff;
	      *ResultLength = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) + kfi->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kfi->LastWriteTime, 0);
	      kfi->TitleIndex = 0;
/*	      kfi->SubKeys = req->subkeys;
	      kfi->MaxNameLength = req->max_subkey;
	      kfi->MaxClassLength = req->max_class;
	      kfi->Values = req->values;
	      kfi->MaxValueNameLen = req->max_value;
	      kfi->MaxValueDataLen = req->max_data;
*/
	      FIXME("incomplete\n");
	      if (kfi->ClassLength) memcpy (kfi->Class, req->class, kfi->ClassLength);
	    }
	    break;
	  case KeyNodeInformation:
	    {
	      PKEY_NODE_INFORMATION kni = KeyInformation;
	      kni->ClassLength = lstrlenW(req->class) * sizeof(WCHAR);
	      kni->NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      kni->ClassOffset = (kni->ClassLength) ?
	        sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength : 0xffffffff;

	      *ResultLength = sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength + kni->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kni->LastWriteTime, 0);
	      kni->TitleIndex = 0;
	      memcpy (kni->Name, req->name, kni->NameLength);
	      if (kni->ClassLength) memcpy (KeyInformation+kni->ClassOffset, req->class, kni->ClassLength);
	    }
	    break;
	  default:
	    FIXME("KeyInformationClass not implemented\n");
	    return STATUS_UNSUCCESSFUL;
	}
	TRACE("buf=%lu len=%lu\n", Length, *ResultLength);
	return ret;
}

/******************************************************************************
 * NtQueryKey [NTDLL]
 * ZwQueryKey
 */
NTSTATUS WINAPI NtQueryKey(
	HANDLE KeyHandle,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	struct query_key_info_request *req = get_req_buffer();
	NTSTATUS ret;
	
	TRACE("(0x%08x,0x%08x,%p,0x%08lx,%p) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
	
	req->hkey = KeyHandle;
	if ((ret = server_call_noerr(REQ_QUERY_KEY_INFO)) != STATUS_SUCCESS) return ret;
	
	switch (KeyInformationClass)
	{
	  case KeyBasicInformation:
	    {
	      PKEY_BASIC_INFORMATION kbi = KeyInformation;
	      UINT NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      *ResultLength = sizeof(KEY_BASIC_INFORMATION) - sizeof(WCHAR) + NameLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kbi->LastWriteTime, 0);
	      kbi->TitleIndex = 0;
	      kbi->NameLength = NameLength;
	      memcpy (kbi->Name, req->name, NameLength);
	    }
	    break;
	  case KeyFullInformation:
	    {
	      PKEY_FULL_INFORMATION kfi = KeyInformation;
	      kfi->ClassLength = lstrlenW(req->class) * sizeof(WCHAR);
	      kfi->ClassOffset = (kfi->ClassLength) ?
	        sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) : 0xffffffff;

	      *ResultLength = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) + kfi->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kfi->LastWriteTime, 0);
	      kfi->TitleIndex = 0;
	      kfi->SubKeys = req->subkeys;
	      kfi->MaxNameLen = req->max_subkey;
	      kfi->MaxClassLen = req->max_class;
	      kfi->Values = req->values;
	      kfi->MaxValueNameLen = req->max_value;
	      kfi->MaxValueDataLen = req->max_data;
	      if(kfi->ClassLength) memcpy (KeyInformation+kfi->ClassOffset, req->class, kfi->ClassLength);
	    }
	    break;
	  case KeyNodeInformation:
	    {
	      PKEY_NODE_INFORMATION kni = KeyInformation;
	      kni->ClassLength = lstrlenW(req->class) * sizeof(WCHAR);
	      kni->NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      kni->ClassOffset = (kni->ClassLength) ?
	        sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength : 0xffffffff;

	      *ResultLength = sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength + kni->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      DOSFS_UnixTimeToFileTime(req->modif, &kni->LastWriteTime, 0);
	      kni->TitleIndex = 0;
	      memcpy (kni->Name, req->name, kni->NameLength);
	      if(kni->ClassLength) memcpy (KeyInformation+kni->ClassOffset, req->class, kni->ClassLength);
	    }
	    break;
	  default:
	    FIXME("KeyInformationClass not implemented\n");
	    return STATUS_UNSUCCESSFUL;
	}
	return ret;
}

/******************************************************************************
 *  NtEnumerateValueKey	[NTDLL] 
 *  ZwEnumerateValueKey
 */
NTSTATUS WINAPI NtEnumerateValueKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	struct enum_key_value_request *req = get_req_buffer();
	UINT NameLength;
	NTSTATUS ret;
	
	TRACE("(0x%08x,0x%08lx,0x%08x,%p,0x%08lx,%p)\n",
	KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);

	req->hkey = KeyHandle;
	req->index = Index;
	if ((ret = server_call_noerr(REQ_ENUM_KEY_VALUE)) != STATUS_SUCCESS) return ret;

 	switch (KeyInformationClass)
	{
	  case KeyBasicInformation:
	    {
	      PKEY_VALUE_BASIC_INFORMATION kbi = KeyInformation;
	      
	      NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) - sizeof(WCHAR) + NameLength;
	      if (*ResultLength > Length) return STATUS_BUFFER_TOO_SMALL;

	      kbi->TitleIndex = 0;
	      kbi->Type = req->type;
	      kbi->NameLength = NameLength;
	      memcpy(kbi->Name, req->name, kbi->NameLength);
	    }
	    break;
	  case KeyValueFullInformation:
	    {
	      PKEY_VALUE_FULL_INFORMATION kbi = KeyInformation;
	      UINT DataOffset;

	      NameLength = lstrlenW(req->name) * sizeof(WCHAR);
	      DataOffset = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR) + NameLength;
	      *ResultLength = DataOffset + req->len;

	      if (*ResultLength > Length) return STATUS_BUFFER_TOO_SMALL;

	      kbi->TitleIndex = 0;
	      kbi->Type = req->type;
	      kbi->DataOffset = DataOffset;
	      kbi->DataLength = req->len;
	      kbi->NameLength = NameLength;
	      memcpy(kbi->Name, req->name, kbi->NameLength);
	      memcpy(((LPBYTE)kbi) + DataOffset, req->data, req->len);
	    }
	    break;
	  case KeyValuePartialInformation:
	    {
	      PKEY_VALUE_PARTIAL_INFORMATION kbi = KeyInformation;
	      
	      *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) - sizeof(WCHAR) + req->len;

	      if (*ResultLength > Length) return STATUS_BUFFER_TOO_SMALL;

	      kbi->TitleIndex = 0;
	      kbi->Type = req->type;
	      kbi->DataLength = req->len;
	      memcpy(kbi->Data, req->data, req->len);
	    }
	    break;
	  default:
	    FIXME("not implemented\n");
	}
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtFlushKey	[NTDLL] 
 *  ZwFlushKey
 */
NTSTATUS WINAPI NtFlushKey(HANDLE KeyHandle)
{
	FIXME("(0x%08x) stub!\n",
	KeyHandle);
	return 1;
}

/******************************************************************************
 *  NtLoadKey	[NTDLL] 
 *  ZwLoadKey
 */
NTSTATUS WINAPI NtLoadKey(
	PHANDLE KeyHandle,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	FIXME("(%p),stub!\n", KeyHandle);
	dump_ObjectAttributes(ObjectAttributes);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtNotifyChangeKey	[NTDLL] 
 *  ZwNotifyChangeKey
 */
NTSTATUS WINAPI NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous,
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree)
{
	FIXME("(0x%08x,0x%08x,%p,%p,%p,0x%08lx, 0x%08x,%p,0x%08lx,0x%08x) stub!\n",
	KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter,
	Asynchroneous, ChangeBuffer, Length, WatchSubtree);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQueryMultipleValueKey [NTDLL]
 * ZwQueryMultipleValueKey
 */

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HANDLE KeyHandle,
	PVALENTW ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength)
{
	FIXME("(0x%08x,%p,0x%08lx,%p,0x%08lx,%p) stub!\n",
	KeyHandle, ListOfValuesToQuery, NumberOfItems, MultipleValueInformation,
	Length,ReturnLength);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQueryValueKey [NTDLL]
 * ZwQueryValueKey
 *
 * NOTES
 *  the name in the KeyValueInformation is never set
 */
NTSTATUS WINAPI NtQueryValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	struct get_key_value_request *req = get_req_buffer();
	NTSTATUS ret;

	TRACE("(0x%08x,%s,0x%08x,%p,0x%08lx,%p)\n",
	KeyHandle, debugstr_us(ValueName), KeyValueInformationClass, KeyValueInformation, Length, ResultLength);

	req->hkey = KeyHandle;
	if (copy_nameU(req->name, ValueName, 0) != STATUS_SUCCESS) return STATUS_BUFFER_OVERFLOW;
	if ((ret = server_call_noerr(REQ_GET_KEY_VALUE)) != STATUS_SUCCESS) return ret;

	switch(KeyValueInformationClass)
	{
	  case KeyValueBasicInformation:
	    {
	      PKEY_VALUE_BASIC_INFORMATION kbi = (PKEY_VALUE_BASIC_INFORMATION) KeyValueInformation;
	      kbi->Type = req->type;

	      *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION)-sizeof(WCHAR);
	      if (Length <= *ResultLength) return STATUS_BUFFER_OVERFLOW;
	      kbi->NameLength = 0;
	    }  
	    break;
	  case KeyValueFullInformation:
	    {
	      PKEY_VALUE_FULL_INFORMATION  kfi = (PKEY_VALUE_FULL_INFORMATION) KeyValueInformation;
	      ULONG DataOffset;
	      kfi->Type = req->type;

	      DataOffset = sizeof(KEY_VALUE_FULL_INFORMATION)-sizeof(WCHAR);
	      *ResultLength = DataOffset + req->len;
	      if (Length <= *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      kfi->NameLength = 0;
	      kfi->DataOffset = DataOffset;
	      kfi->DataLength = req->len;
	      memcpy(KeyValueInformation+DataOffset, req->data, req->len);
	    }  
	    break;
	  case KeyValuePartialInformation:
	    {
	      PKEY_VALUE_PARTIAL_INFORMATION kpi = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueInformation;
	      kpi->Type = req->type;

	      *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION)-sizeof(UCHAR)+req->len;
	      if (Length <= *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      kpi->DataLength = req->len;
	      memcpy(kpi->Data, req->data, req->len);
	    }  
	    break;
	  default:
	    FIXME("KeyValueInformationClass not implemented\n");
	    return STATUS_UNSUCCESSFUL;
	  
	}
	return ret;
}

/******************************************************************************
 * NtReplaceKey [NTDLL]
 * ZwReplaceKey
 */
NTSTATUS WINAPI NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	FIXME("(0x%08x),stub!\n", Key);
	dump_ObjectAttributes(ObjectAttributes);
	dump_ObjectAttributes(ReplacedObjectAttributes);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtRestoreKey [NTDLL]
 * ZwRestoreKey
 */
NTSTATUS WINAPI NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags)
{
	FIXME("(0x%08x,0x%08x,0x%08lx) stub\n",
	KeyHandle, FileHandle, RestoreFlags);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSaveKey [NTDLL]
 * ZwSaveKey
 */
NTSTATUS WINAPI NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle)
{
	FIXME("(0x%08x,0x%08x) stub\n",
	KeyHandle, FileHandle);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSetInformationKey [NTDLL]
 * ZwSetInformationKey
 */
NTSTATUS WINAPI NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength)
{
	FIXME("(0x%08x,0x%08x,%p,0x%08lx) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, KeyInformationLength);
	return STATUS_SUCCESS;
}
/******************************************************************************
 * NtSetValueKey [NTDLL]
 * ZwSetValueKey
 */
NTSTATUS WINAPI NtSetValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	ULONG TitleIndex,
	ULONG Type,
	PVOID Data,
	ULONG DataSize)
{
	FIXME("(0x%08x,%p(%s), 0x%08lx, 0x%08lx, %p, 0x%08lx) stub!\n",
	KeyHandle, ValueName,debugstr_us(ValueName), TitleIndex, Type, Data, DataSize);
	return STATUS_SUCCESS;

}

/******************************************************************************
 * NtUnloadKey [NTDLL]
 * ZwUnloadKey
 */
NTSTATUS WINAPI NtUnloadKey(
	IN HANDLE KeyHandle)
{
	FIXME("(0x%08x) stub\n",
	KeyHandle);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlFormatCurrentUserKeyPath		[NTDLL.371] 
 */
NTSTATUS WINAPI RtlFormatCurrentUserKeyPath(
	IN OUT PUNICODE_STRING KeyPath)
{
/*	LPSTR Path = "\\REGISTRY\\USER\\S-1-5-21-0000000000-000000000-0000000000-500";*/
	LPSTR Path = "\\REGISTRY\\USER\\.DEFAULT";
	ANSI_STRING AnsiPath;

	FIXME("(%p) stub\n",KeyPath);
	RtlInitAnsiString(&AnsiPath, Path);
	return RtlAnsiStringToUnicodeString(KeyPath, &AnsiPath, TRUE);
}

/******************************************************************************
 *  RtlOpenCurrentUser		[NTDLL] 
 *
 * if we return just HKEY_CURRENT_USER the advapi try's to find a remote
 * registry (odd handle) and fails
 *
 */
DWORD WINAPI RtlOpenCurrentUser(
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE KeyHandle)	/* handle of HKEY_CURRENT_USER */
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING ObjectName;
	NTSTATUS ret;

	TRACE("(0x%08lx, %p) stub\n",DesiredAccess, KeyHandle);

	RtlFormatCurrentUserKeyPath(&ObjectName);
	InitializeObjectAttributes(&ObjectAttributes,&ObjectName,OBJ_CASE_INSENSITIVE,0, NULL);
	ret = NtOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
	RtlFreeUnicodeString(&ObjectName);
	return ret;
}
