#include "win.h"
#include "debug.h"
#include "windows.h"
#include "winreg.h"
#include "ntdll.h"
#include "ntddk.h"

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
	FIXME(ntdll,"(%p,0x%08lx,%p (%s),0x%08lx, %p(%s),0x%08lx,%p),stub!\n",
	KeyHandle, DesiredAccess, ObjectAttributes,debugstr_w(ObjectAttributes->ObjectName->Buffer),
	TitleIndex, Class, debugstr_w(Class->Buffer), CreateOptions, Disposition);
	return 0;
}

/******************************************************************************
 * NtDeleteKey [NTDLL]
 * ZwDeleteKey
 */
NTSTATUS NtDeleteKey(HANDLE32 KeyHandle)
{
	FIXME(ntdll,"(0x%08x) stub!\n",
	KeyHandle);
	return 1;
}

/******************************************************************************
 * NtDeleteValueKey [NTDLL]
 * ZwDeleteValueKey
 */
NTSTATUS WINAPI NtDeleteValueKey(
	IN HANDLE32 KeyHandle,
	IN PUNICODE_STRING ValueName)
{
	FIXME(ntdll,"(0x%08x,%p(%s)) stub!\n",
	KeyHandle, ValueName,debugstr_w(ValueName->Buffer));
	return 1;

}
/******************************************************************************
 * NtEnumerateKey [NTDLL]
 * ZwEnumerateKey
 */
NTSTATUS WINAPI NtEnumerateKey(
	HANDLE32 KeyHandle,
	ULONG Index,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,0x%08lx,0x%08x,%p,0x%08lx,%p) stub\n",
	KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
	return 1;
}

/******************************************************************************
 *  NtEnumerateValueKey	[NTDLL] 
 *  ZwEnumerateValueKey
 */
NTSTATUS WINAPI NtEnumerateValueKey(
	HANDLE32 KeyHandle,
	ULONG Index,
	KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,0x%08lx,0x%08x,%p,0x%08lx,%p) stub!\n",
	KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
	return 1;
}

/******************************************************************************
 *  NtFlushKey	[NTDLL] 
 *  ZwFlushKey
 */
NTSTATUS NtFlushKey(HANDLE32 KeyHandle)
{
	FIXME(ntdll,"(0x%08x) stub!\n",
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
	FIXME(ntdll,"(%p,%p (%s)),stub!\n",
	KeyHandle, ObjectAttributes,debugstr_w(ObjectAttributes->ObjectName->Buffer));
	return 0;

}

/******************************************************************************
 *  NtNotifyChangeKey	[NTDLL] 
 *  ZwNotifyChangeKey
 */
NTSTATUS WINAPI NtNotifyChangeKey(
	IN HANDLE32 KeyHandle,
	IN HANDLE32 Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous,
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,%p,%p,0x%08lx, 0x%08x,%p,0x%08lx,0x%08x) stub!\n",
	KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter,
	Asynchroneous, ChangeBuffer, Length, WatchSubtree);
	return 0;
}


/******************************************************************************
 * NtOpenKey [NTDLL.129]
 * ZwOpenKey
 *   OUT	PHANDLE			KeyHandle,
 *   IN		ACCESS_MASK		DesiredAccess,
 *   IN		POBJECT_ATTRIBUTES 	ObjectAttributes 
 */
NTSTATUS WINAPI NtOpenKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes) 
{
	FIXME(ntdll,"(%p,0x%08lx,%p (%s)),stub!\n",
	KeyHandle, DesiredAccess, ObjectAttributes,debugstr_w(ObjectAttributes->ObjectName->Buffer));
	return 0;
}

/******************************************************************************
 * NtQueryKey [NTDLL]
 * ZwQueryKey
 */
NTSTATUS WINAPI NtQueryKey(
	HANDLE32 KeyHandle,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx,%p) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
	return 0;
}

/******************************************************************************
 * NtQueryMultipleValueKey [NTDLL]
 * ZwQueryMultipleValueKey
 */

NTSTATUS WINAPI NtQueryMultipleValueKey(
	HANDLE32 KeyHandle,
	PVALENTW ListOfValuesToQuery,
	ULONG NumberOfItems,
	PVOID MultipleValueInformation,
	ULONG Length,
	PULONG  ReturnLength)
{
	FIXME(ntdll,"(0x%08x,%p,0x%08lx,%p,0x%08lx,%p) stub!\n",
	KeyHandle, ListOfValuesToQuery, NumberOfItems, MultipleValueInformation,
	Length,ReturnLength);
	return 0;
}

/******************************************************************************
 * NtQueryValueKey [NTDLL]
 * ZwQueryValueKey
 */
NTSTATUS WINAPI NtQueryValueKey(
	HANDLE32 KeyHandle,
	PUNICODE_STRING ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID KeyValueInformation,
	ULONG Length,
	PULONG ResultLength)
{
	FIXME(ntdll,"(0x%08x,%p,0x%08x,%p,0x%08lx,%p) stub\n",
	KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
	return 0;
}

/******************************************************************************
 * NtReplaceKey [NTDLL]
 * ZwReplaceKey
 */
NTSTATUS WINAPI NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE32 Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	FIXME(ntdll,"(%p(%s),0x%08x,%p (%s)),stub!\n",
	ObjectAttributes,debugstr_w(ObjectAttributes->ObjectName->Buffer),Key,
	ReplacedObjectAttributes,debugstr_w(ReplacedObjectAttributes->ObjectName->Buffer));
	return 0;

}
/******************************************************************************
 * NtRestoreKey [NTDLL]
 * ZwRestoreKey
 */
NTSTATUS WINAPI NtRestoreKey(
	HANDLE32 KeyHandle,
	HANDLE32 FileHandle,
	ULONG RestoreFlags)
{
	FIXME(ntdll,"(0x%08x,0x%08x,0x%08lx) stub\n",
	KeyHandle, FileHandle, RestoreFlags);
	return 0;

}
/******************************************************************************
 * NtSaveKey [NTDLL]
 * ZwSaveKey
 */
NTSTATUS WINAPI NtSaveKey(
	IN HANDLE32 KeyHandle,
	IN HANDLE32 FileHandle)
{
	FIXME(ntdll,"(0x%08x,0x%08x) stub\n",
	KeyHandle, FileHandle);
	return 0;
}
/******************************************************************************
 * NtSetInformationKey [NTDLL]
 * ZwSetInformationKey
 */
NTSTATUS WINAPI NtSetInformationKey(
	IN HANDLE32 KeyHandle,
	IN const int KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength)
{
	FIXME(ntdll,"(0x%08x,0x%08x,%p,0x%08lx) stub\n",
	KeyHandle, KeyInformationClass, KeyInformation, KeyInformationLength);
	return 0;
}
/******************************************************************************
 * NtSetValueKey [NTDLL]
 * ZwSetValueKey
 */
NTSTATUS WINAPI NtSetValueKey(
	HANDLE32 KeyHandle,
	PUNICODE_STRING ValueName,
	ULONG TitleIndex,
	ULONG Type,
	PVOID Data,
	ULONG DataSize)
{
	FIXME(ntdll,"(0x%08x,%p(%s), 0x%08lx, 0x%08lx, %p, 0x%08lx) stub!\n",
	KeyHandle, ValueName,debugstr_w(ValueName->Buffer), TitleIndex, Type, Data, DataSize);
	return 1;

}

/******************************************************************************
 * NtUnloadKey [NTDLL]
 * ZwUnloadKey
 */
NTSTATUS WINAPI NtUnloadKey(
	IN HANDLE32 KeyHandle)
{
	FIXME(ntdll,"(0x%08x) stub\n",
	KeyHandle);
	return 0;
}
