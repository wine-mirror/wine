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
#include "wine/unicode.h"
#include "server.h"
#include "ntddk.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

static const WCHAR root_name[] = { '\\','R','e','g','i','s','t','r','y','\\',0 };
static const UNICODE_STRING root_path =
{
    sizeof(root_name)-sizeof(WCHAR),  /* Length */
    sizeof(root_name),                /* MaximumLength */
    (LPWSTR)root_name                 /* Buffer */
};

/* copy a key name into the request buffer */
static NTSTATUS copy_key_name( LPWSTR dest, const UNICODE_STRING *name )
{
    int len = name->Length, pos = 0;

    if (len >= MAX_PATH) return STATUS_BUFFER_OVERFLOW;
    if (RtlPrefixUnicodeString( &root_path, name, TRUE ))
    {
        pos += root_path.Length / sizeof(WCHAR);
        len -= root_path.Length;
    }
    if (len) memcpy( dest, name->Buffer + pos, len );
    dest[len / sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;
}


/* copy a key name into the request buffer */
static inline NTSTATUS copy_nameU( LPWSTR Dest, const UNICODE_STRING *name, UINT max )
{
    if (name->Length >= max) return STATUS_BUFFER_OVERFLOW;
    if (name->Length) memcpy( Dest, name->Buffer, name->Length );
    Dest[name->Length / sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;
}


/******************************************************************************
 * NtCreateKey [NTDLL]
 * ZwCreateKey
 */
NTSTATUS WINAPI NtCreateKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos )
{
    struct create_key_request *req = get_req_buffer();
    NTSTATUS ret;

    TRACE( "(0x%x,%s,%s,%lx,%lx,%p)\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
           debugstr_us(class), options, access, retkey );

    if (!retkey) return STATUS_INVALID_PARAMETER;

    req->parent  = attr->RootDirectory;
    req->access  = access;
    req->options = options;
    req->modif   = 0;

    if ((ret = copy_key_name( req->name, attr->ObjectName ))) return ret;
    req->class[0] = 0;
    if (class)
    {
        if ((ret = copy_nameU( req->class, class, server_remaining(req->class) ))) return ret;
    }

    if (!(ret = server_call_noerr( REQ_CREATE_KEY )))
    {
        *retkey = req->hkey;
        if (dispos) *dispos = req->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
    }
    return ret;
}


/******************************************************************************
 * NtOpenKey [NTDLL.129]
 * ZwOpenKey
 *   OUT	PHANDLE			retkey (returns 0 when failure)
 *   IN		ACCESS_MASK		access
 *   IN		POBJECT_ATTRIBUTES 	attr 
 */
NTSTATUS WINAPI NtOpenKey( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    struct open_key_request *req = get_req_buffer();
    NTSTATUS ret;

    TRACE( "(0x%x,%s,%lx,%p)\n", attr->RootDirectory,
           debugstr_us(attr->ObjectName), access, retkey );

    if (!retkey) return STATUS_INVALID_PARAMETER;
    *retkey = 0;

    req->parent = attr->RootDirectory;
    req->access = access;

    if ((ret = copy_key_name( req->name, attr->ObjectName ))) return ret;
    if (!(ret = server_call_noerr( REQ_OPEN_KEY ))) *retkey = req->hkey;
    return ret;
}


/******************************************************************************
 * NtDeleteKey [NTDLL]
 * ZwDeleteKey
 */
NTSTATUS WINAPI NtDeleteKey( HANDLE hkey )
{
    struct delete_key_request *req = get_req_buffer();

    TRACE( "(%x)\n", hkey );
    req->hkey = hkey;
    req->name[0] = 0;
    return server_call_noerr( REQ_DELETE_KEY );
}


/******************************************************************************
 * NtDeleteValueKey [NTDLL]
 * ZwDeleteValueKey
 */
NTSTATUS WINAPI NtDeleteValueKey( HANDLE hkey, const UNICODE_STRING *name )
{
    NTSTATUS ret;
    struct delete_key_value_request *req = get_req_buffer();

    TRACE( "(0x%x,%s)\n", hkey, debugstr_us(name) );

    req->hkey = hkey;
    if (!(ret = copy_nameU( req->name, name, MAX_PATH )))
        ret = server_call_noerr( REQ_DELETE_KEY_VALUE );
    return ret;
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
	      UINT NameLength = strlenW(req->name) * sizeof(WCHAR);
	      *ResultLength = sizeof(KEY_BASIC_INFORMATION) - sizeof(WCHAR) + NameLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kbi->LastWriteTime);
	      kbi->TitleIndex = 0;
	      kbi->NameLength = NameLength;
	      memcpy (kbi->Name, req->name, NameLength);
	    }
	    break;
	  case KeyFullInformation:
	    {
	      PKEY_FULL_INFORMATION kfi = KeyInformation;
	      kfi->ClassLength = strlenW(req->class) * sizeof(WCHAR);
	      kfi->ClassOffset = (kfi->ClassLength) ?
	        sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) : 0xffffffff;
	      *ResultLength = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) + kfi->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kfi->LastWriteTime);
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
	      kni->ClassLength = strlenW(req->class) * sizeof(WCHAR);
	      kni->NameLength = strlenW(req->name) * sizeof(WCHAR);
	      kni->ClassOffset = (kni->ClassLength) ?
	        sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength : 0xffffffff;

	      *ResultLength = sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength + kni->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kni->LastWriteTime);
	      kni->TitleIndex = 0;
	      memcpy (kni->Name, req->name, kni->NameLength);
	      if (kni->ClassLength) memcpy ((char *) KeyInformation + kni->ClassOffset, req->class, kni->ClassLength);
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
	      UINT NameLength = strlenW(req->name) * sizeof(WCHAR);
	      *ResultLength = sizeof(KEY_BASIC_INFORMATION) - sizeof(WCHAR) + NameLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kbi->LastWriteTime);
	      kbi->TitleIndex = 0;
	      kbi->NameLength = NameLength;
	      memcpy (kbi->Name, req->name, NameLength);
	    }
	    break;
	  case KeyFullInformation:
	    {
	      PKEY_FULL_INFORMATION kfi = KeyInformation;
	      kfi->ClassLength = strlenW(req->class) * sizeof(WCHAR);
	      kfi->ClassOffset = (kfi->ClassLength) ?
	        sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) : 0xffffffff;

	      *ResultLength = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR) + kfi->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kfi->LastWriteTime);
	      kfi->TitleIndex = 0;
	      kfi->SubKeys = req->subkeys;
	      kfi->MaxNameLen = req->max_subkey;
	      kfi->MaxClassLen = req->max_class;
	      kfi->Values = req->values;
	      kfi->MaxValueNameLen = req->max_value;
	      kfi->MaxValueDataLen = req->max_data;
	      if(kfi->ClassLength) memcpy ((char *) KeyInformation + kfi->ClassOffset, req->class, kfi->ClassLength);
	    }
	    break;
	  case KeyNodeInformation:
	    {
	      PKEY_NODE_INFORMATION kni = KeyInformation;
	      kni->ClassLength = strlenW(req->class) * sizeof(WCHAR);
	      kni->NameLength = strlenW(req->name) * sizeof(WCHAR);
	      kni->ClassOffset = (kni->ClassLength) ?
	        sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength : 0xffffffff;

	      *ResultLength = sizeof(KEY_NODE_INFORMATION) - sizeof(WCHAR) + kni->NameLength + kni->ClassLength;
	      if (Length < *ResultLength) return STATUS_BUFFER_OVERFLOW;

	      RtlSecondsSince1970ToTime(req->modif, &kni->LastWriteTime);
	      kni->TitleIndex = 0;
	      memcpy (kni->Name, req->name, kni->NameLength);
	      if(kni->ClassLength) memcpy ((char *) KeyInformation + kni->ClassOffset, req->class, kni->ClassLength);
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
	      
	      NameLength = strlenW(req->name) * sizeof(WCHAR);
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

	      NameLength = strlenW(req->name) * sizeof(WCHAR);
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
	if (copy_nameU(req->name, ValueName, MAX_PATH) != STATUS_SUCCESS) return STATUS_BUFFER_OVERFLOW;
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
	      memcpy((char *) KeyValueInformation + DataOffset, req->data, req->len);
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
