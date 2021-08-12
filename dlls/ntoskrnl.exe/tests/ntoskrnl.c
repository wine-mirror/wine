/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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

#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "winsvc.h"
#include "winioctl.h"
#include "winternl.h"
#include "winsock2.h"
#include "wincrypt.h"
#include "ntsecapi.h"
#include "mscat.h"
#include "mssip.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "newdev.h"
#include "dbt.h"
#include "initguid.h"
#include "devguid.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"
#include "ddk/hidpi.h"
#include "wine/test.h"
#include "wine/heap.h"
#include "wine/mssign.h"

#include "driver.h"

static const GUID GUID_NULL;

static HANDLE device;

static struct test_data *test_data;

static BOOL (WINAPI *pRtlDosPathNameToNtPathName_U)(const WCHAR *, UNICODE_STRING *, WCHAR **, CURDIR *);
static BOOL (WINAPI *pRtlFreeUnicodeString)(UNICODE_STRING *);
static BOOL (WINAPI *pCancelIoEx)(HANDLE, OVERLAPPED *);
static BOOL (WINAPI *pIsWow64Process)(HANDLE, BOOL *);
static BOOL (WINAPI *pSetFileCompletionNotificationModes)(HANDLE, UCHAR);
static HRESULT (WINAPI *pSignerSign)(SIGNER_SUBJECT_INFO *subject, SIGNER_CERT *cert,
        SIGNER_SIGNATURE_INFO *signature, SIGNER_PROVIDER_INFO *provider,
        const WCHAR *timestamp, CRYPT_ATTRIBUTES *attr, void *sip_data);

static void load_resource(const WCHAR *name, WCHAR *filename)
{
    static WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(path), path);
    GetTempFileNameW(path, name, 0, filename);

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "failed to create %s, error %u\n", debugstr_w(filename), GetLastError());

    res = FindResourceW(NULL, name, L"TESTDLL");
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

struct testsign_context
{
    HCRYPTPROV provider;
    const CERT_CONTEXT *cert, *root_cert, *publisher_cert;
    HCERTSTORE root_store, publisher_store;
};

static BOOL testsign_create_cert(struct testsign_context *ctx)
{
    BYTE encoded_name[100], encoded_key_id[200], public_key_info_buffer[1000];
    WCHAR container_name[26];
    BYTE hash_buffer[16], cert_buffer[1000], provider_nameA[100], serial[16];
    CERT_PUBLIC_KEY_INFO *public_key_info = (CERT_PUBLIC_KEY_INFO *)public_key_info_buffer;
    CRYPT_KEY_PROV_INFO provider_info = {0};
    CRYPT_ALGORITHM_IDENTIFIER algid = {0};
    CERT_AUTHORITY_KEY_ID_INFO key_info;
    CERT_INFO cert_info = {0};
    WCHAR provider_nameW[100];
    CERT_EXTENSION extension;
    HCRYPTKEY key;
    DWORD size;
    BOOL ret;

    memset(ctx, 0, sizeof(*ctx));

    srand(time(NULL));
    swprintf(container_name, ARRAY_SIZE(container_name), L"wine_testsign%u", rand());

    ret = CryptAcquireContextW(&ctx->provider, container_name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    ok(ret, "Failed to create container, error %#x\n", GetLastError());

    ret = CryptGenKey(ctx->provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key);
    ok(ret, "Failed to create key, error %#x\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#x\n", GetLastError());
    ret = CryptGetUserKey(ctx->provider, AT_SIGNATURE, &key);
    ok(ret, "Failed to get user key, error %#x\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#x\n", GetLastError());

    size = sizeof(encoded_name);
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=winetest_cert", CERT_X500_NAME_STR, NULL, encoded_name, &size, NULL);
    ok(ret, "Failed to convert name, error %#x\n", GetLastError());
    key_info.CertIssuer.cbData = size;
    key_info.CertIssuer.pbData = encoded_name;

    size = sizeof(public_key_info_buffer);
    ret = CryptExportPublicKeyInfo(ctx->provider, AT_SIGNATURE, X509_ASN_ENCODING, public_key_info, &size);
    ok(ret, "Failed to export public key, error %#x\n", GetLastError());
    cert_info.SubjectPublicKeyInfo = *public_key_info;

    size = sizeof(hash_buffer);
    ret = CryptHashPublicKeyInfo(ctx->provider, CALG_MD5, 0, X509_ASN_ENCODING, public_key_info, hash_buffer, &size);
    ok(ret, "Failed to hash public key, error %#x\n", GetLastError());

    key_info.KeyId.cbData = size;
    key_info.KeyId.pbData = hash_buffer;

    RtlGenRandom(serial, sizeof(serial));
    key_info.CertSerialNumber.cbData = sizeof(serial);
    key_info.CertSerialNumber.pbData = serial;

    size = sizeof(encoded_key_id);
    ret = CryptEncodeObject(X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID, &key_info, encoded_key_id, &size);
    ok(ret, "Failed to convert name, error %#x\n", GetLastError());

    extension.pszObjId = (char *)szOID_AUTHORITY_KEY_IDENTIFIER;
    extension.fCritical = TRUE;
    extension.Value.cbData = size;
    extension.Value.pbData = encoded_key_id;

    cert_info.dwVersion = CERT_V3;
    cert_info.SerialNumber = key_info.CertSerialNumber;
    cert_info.SignatureAlgorithm.pszObjId = (char *)szOID_RSA_SHA1RSA;
    cert_info.Issuer = key_info.CertIssuer;
    GetSystemTimeAsFileTime(&cert_info.NotBefore);
    GetSystemTimeAsFileTime(&cert_info.NotAfter);
    cert_info.NotAfter.dwHighDateTime += 1;
    cert_info.Subject = key_info.CertIssuer;
    cert_info.cExtension = 1;
    cert_info.rgExtension = &extension;
    algid.pszObjId = (char *)szOID_RSA_SHA1RSA;
    size = sizeof(cert_buffer);
    ret = CryptSignAndEncodeCertificate(ctx->provider, AT_SIGNATURE, X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED, &cert_info, &algid, NULL, cert_buffer, &size);
    ok(ret, "Failed to create certificate, error %#x\n", GetLastError());

    ctx->cert = CertCreateCertificateContext(X509_ASN_ENCODING, cert_buffer, size);
    ok(!!ctx->cert, "Failed to create context, error %#x\n", GetLastError());

    size = sizeof(provider_nameA);
    ret = CryptGetProvParam(ctx->provider, PP_NAME, provider_nameA, &size, 0);
    ok(ret, "Failed to get prov param, error %#x\n", GetLastError());
    MultiByteToWideChar(CP_ACP, 0, (char *)provider_nameA, -1, provider_nameW, ARRAY_SIZE(provider_nameW));

    provider_info.pwszContainerName = (WCHAR *)container_name;
    provider_info.pwszProvName = provider_nameW;
    provider_info.dwProvType = PROV_RSA_FULL;
    provider_info.dwKeySpec = AT_SIGNATURE;
    ret = CertSetCertificateContextProperty(ctx->cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &provider_info);
    ok(ret, "Failed to set provider info, error %#x\n", GetLastError());

    ctx->root_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, "root");
    if (!ctx->root_store && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to open root store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %u\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %u\n", GetLastError());

        return FALSE;
    }
    ok(!!ctx->root_store, "Failed to open store, error %u\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->root_store, ctx->cert, CERT_STORE_ADD_ALWAYS, &ctx->root_cert);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to add self-signed certificate to store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %u\n", GetLastError());
        ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
        ok(ret, "Failed to close store, error %u\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %u\n", GetLastError());

        return FALSE;
    }
    ok(ret, "Failed to add certificate, error %u\n", GetLastError());

    ctx->publisher_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0,
            CERT_SYSTEM_STORE_LOCAL_MACHINE, "trustedpublisher");
    ok(!!ctx->publisher_store, "Failed to open store, error %u\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->publisher_store, ctx->cert,
            CERT_STORE_ADD_ALWAYS, &ctx->publisher_cert);
    ok(ret, "Failed to add certificate, error %u\n", GetLastError());

    return TRUE;
}

static void testsign_cleanup(struct testsign_context *ctx)
{
    BOOL ret;

    ret = CertFreeCertificateContext(ctx->cert);
    ok(ret, "Failed to free certificate, error %u\n", GetLastError());

    ret = CertDeleteCertificateFromStore(ctx->root_cert);
    ok(ret, "Failed to remove certificate, error %u\n", GetLastError());
    ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %u\n", GetLastError());

    ret = CertDeleteCertificateFromStore(ctx->publisher_cert);
    ok(ret, "Failed to remove certificate, error %u\n", GetLastError());
    ret = CertCloseStore(ctx->publisher_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %u\n", GetLastError());

    ret = CryptReleaseContext(ctx->provider, 0);
    ok(ret, "failed to release context, error %u\n", GetLastError());
}

static void testsign_sign(struct testsign_context *ctx, const WCHAR *filename)
{
    SIGNER_ATTR_AUTHCODE authcode = {sizeof(authcode)};
    SIGNER_SIGNATURE_INFO signature = {sizeof(signature)};
    SIGNER_SUBJECT_INFO subject = {sizeof(subject)};
    SIGNER_CERT_STORE_INFO store = {sizeof(store)};
    SIGNER_CERT cert_info = {sizeof(cert_info)};
    SIGNER_FILE_INFO file = {sizeof(file)};
    DWORD index = 0;
    HRESULT hr;

    subject.dwSubjectChoice = 1;
    subject.pdwIndex = &index;
    subject.pSignerFileInfo = &file;
    file.pwszFileName = (WCHAR *)filename;
    cert_info.dwCertChoice = 2;
    cert_info.pCertStoreInfo = &store;
    store.pSigningCert = ctx->cert;
    store.dwCertPolicy = 0;
    signature.algidHash = CALG_SHA_256;
    signature.dwAttrChoice = SIGNER_AUTHCODE_ATTR;
    signature.pAttrAuthcode = &authcode;
    authcode.pwszName = L"";
    authcode.pwszInfo = L"";
    hr = pSignerSign(&subject, &cert_info, &signature, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == S_OK || broken(hr == NTE_BAD_ALGID) /* < 7 */, "Failed to sign, hr %#x\n", hr);
}

static void unload_driver(SC_HANDLE service)
{
    SERVICE_STATUS status;

    ControlService(service, SERVICE_CONTROL_STOP, &status);
    while (status.dwCurrentState == SERVICE_STOP_PENDING)
    {
        BOOL ret;
        Sleep(100);
        ret = QueryServiceStatus(service, &status);
        ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_STOPPED,
       "expected SERVICE_STOPPED, got %d\n", status.dwCurrentState);

    DeleteService(service);
    CloseServiceHandle(service);
}

static SC_HANDLE load_driver(struct testsign_context *ctx, WCHAR *filename,
        const WCHAR *resname, const WCHAR *driver_name)
{
    SC_HANDLE manager, service;

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!manager && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to open SC manager, not enough permissions\n");
        return FALSE;
    }
    ok(!!manager, "OpenSCManager failed\n");

    /* stop any old drivers running under this name */
    service = OpenServiceW(manager, driver_name, SERVICE_ALL_ACCESS);
    if (service) unload_driver(service);

    load_resource(resname, filename);
    testsign_sign(ctx, filename);
    trace("Trying to load driver %s\n", debugstr_w(filename));

    service = CreateServiceW(manager, driver_name, driver_name,
                             SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
                             SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                             filename, NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "CreateService failed: %u\n", GetLastError());

    CloseServiceHandle(manager);
    return service;
}

static BOOL start_driver(HANDLE service, BOOL vista_plus)
{
    SERVICE_STATUS status;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = StartServiceA(service, 0, NULL);
    if (!ret && (GetLastError() == ERROR_DRIVER_BLOCKED || GetLastError() == ERROR_INVALID_IMAGE_HASH
            || (vista_plus && GetLastError() == ERROR_FILE_NOT_FOUND)))
    {
        if (vista_plus && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            skip("Windows Vista or newer is required to run this service.\n");
        }
        else
        {
            /* If Secure Boot is enabled or the machine is 64-bit, it will reject an unsigned driver. */
            skip("Failed to start service; probably your machine doesn't accept unsigned drivers.\n");
        }
        DeleteService(service);
        CloseServiceHandle(service);
        return FALSE;
    }
    ok(ret, "StartService failed: %u\n", GetLastError());

    /* wait for the service to start up properly */
    ret = QueryServiceStatus(service, &status);
    ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    while (status.dwCurrentState == SERVICE_START_PENDING)
    {
        Sleep(100);
        ret = QueryServiceStatus(service, &status);
        ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_RUNNING,
       "expected SERVICE_RUNNING, got %d\n", status.dwCurrentState);
    ok(status.dwServiceType == SERVICE_KERNEL_DRIVER,
       "expected SERVICE_KERNEL_DRIVER, got %#x\n", status.dwServiceType);

    return TRUE;
}

static HANDLE okfile;

static void cat_okfile(void)
{
    char buffer[512];
    DWORD size;

    SetFilePointer(okfile, 0, NULL, FILE_BEGIN);

    do
    {
        ReadFile(okfile, buffer, sizeof(buffer), &size, NULL);
        printf("%.*s", size, buffer);
    } while (size == sizeof(buffer));

    SetFilePointer(okfile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(okfile);

    winetest_add_failures(InterlockedExchange(&test_data->failures, 0));
    winetest_add_failures(InterlockedExchange(&test_data->todo_failures, 0));
}

static ULONG64 modified_value;

static void main_test(void)
{
    struct main_test_input *test_input;
    DWORD size;
    BOOL res;

    test_input = heap_alloc( sizeof(*test_input) );
    test_input->process_id = GetCurrentProcessId();
    test_input->teststr_offset = (SIZE_T)((BYTE *)&teststr - (BYTE *)NtCurrentTeb()->Peb->ImageBaseAddress);
    test_input->modified_value = &modified_value;
    modified_value = 0;

    res = DeviceIoControl(device, IOCTL_WINETEST_MAIN_TEST, test_input, sizeof(*test_input), NULL, 0, &size, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(!size, "got size %u\n", size);

    heap_free(test_input);
}

static void test_basic_ioctl(void)
{
    char inbuf[64], buf[32];
    DWORD written;
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == sizeof(teststr), "got size %d\n", written);
    ok(!strcmp(buf, teststr), "got '%s'\n", buf);

    memset(buf, 0, sizeof(buf));
    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, inbuf,
            sizeof(inbuf), buf, 10, &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == 10, "got size %d\n", written);
    ok(!strcmp(buf, "Wine is no"), "got '%s'\n", buf);
}

static void test_mismatched_status_ioctl(void)
{
    DWORD written;
    char buf[32];
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_MISMATCHED_STATUS, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    todo_wine ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    todo_wine ok(!strcmp(buf, teststr), "got '%s'\n", buf);
}

static void test_overlapped(void)
{
    OVERLAPPED overlapped, overlapped2, *o;
    DWORD cancel_cnt, size;
    HANDLE file, port;
    ULONG_PTR key;
    BOOL res;

    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    overlapped2.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    file = CreateFileA("\\\\.\\WineTestDriver", FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                       0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    /* test cancelling all device requests */
    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(cancel_cnt == 0, "cancel_cnt = %u\n", cancel_cnt);

    CancelIo(file);

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(cancel_cnt == 2, "cancel_cnt = %u\n", cancel_cnt);

    /* test cancelling selected overlapped event */
    if (pCancelIoEx)
    {
        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

        pCancelIoEx(file, &overlapped);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        ok(cancel_cnt == 1, "cancel_cnt = %u\n", cancel_cnt);

        pCancelIoEx(file, &overlapped2);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        ok(cancel_cnt == 2, "cancel_cnt = %u\n", cancel_cnt);
    }

    port = CreateIoCompletionPort(file, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %u\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%u)\n", res, GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(res, "GetQueuedCompletionStatus failed: %u\n", GetLastError());
    ok(o == &overlapped, "o != overlapped\n");

    if (pSetFileCompletionNotificationModes)
    {
        res = pSetFileCompletionNotificationModes(file, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        ok(res, "SetFileCompletionNotificationModes failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
        ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%u)\n", res, GetLastError());
    }

    CloseHandle(port);
    CloseHandle(overlapped.hEvent);
    CloseHandle(overlapped2.hEvent);
    CloseHandle(file);
}

static void test_load_driver(SC_HANDLE service)
{
    SERVICE_STATUS status;
    BOOL load, res;
    DWORD sz;

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#x\n", status.dwCurrentState);

    load = TRUE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_RUNNING, "got state %#x\n", status.dwCurrentState);

    load = FALSE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#x\n", status.dwCurrentState);
}

static void test_file_handles(void)
{
    DWORD count, ret_size;
    HANDLE file, dup, file2;
    BOOL ret;

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 2, "got %u\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    file = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    file2 = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file2 != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "failed to duplicate handle: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    ret = DeviceIoControl(file, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    ret = DeviceIoControl(file2, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DeviceIoControl(dup, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    CloseHandle(dup);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    CloseHandle(file2);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 2, "got %u\n", count);

    CloseHandle(file);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);
}

static void test_return_status(void)
{
    NTSTATUS status;
    char buffer[7];
    DWORD ret_size;
    BOOL ret;

    strcpy(buffer, "abcdef");
    status = STATUS_SUCCESS;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(ret, "ioctl failed\n");
    ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = STATUS_TIMEOUT;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x0eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x4eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x8eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(!ret, "ioctl succeeded\n");
    ok(GetLastError() == ERROR_MR_MID_NOT_FOUND, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0xceadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(!ret, "ioctl succeeded\n");
    ok(GetLastError() == ERROR_MR_MID_NOT_FOUND, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "abcdef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);
}

static BOOL compare_unicode_string(const WCHAR *buffer, ULONG len, const WCHAR *expect)
{
    return len == wcslen(expect) * sizeof(WCHAR) && !memcmp(buffer, expect, len);
}

static void test_object_info(void)
{
    char buffer[200];
    OBJECT_NAME_INFORMATION *name_info = (OBJECT_NAME_INFORMATION *)buffer;
    OBJECT_TYPE_INFORMATION *type_info = (OBJECT_TYPE_INFORMATION *)buffer;
    FILE_FS_VOLUME_INFORMATION *volume_info = (FILE_FS_VOLUME_INFORMATION *)buffer;
    FILE_NAME_INFORMATION *file_info = (FILE_NAME_INFORMATION *)buffer;
    HANDLE file;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    ULONG size;

    status = NtQueryObject(device, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryObject(device, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(device, &io, buffer, sizeof(buffer), FileNameInformation);
    todo_wine ok(status == STATUS_INVALID_DEVICE_REQUEST, "got %#x\n", status);

    status = NtQueryVolumeInformationFile(device, &io, buffer, sizeof(buffer), FileFsVolumeInformation);
    todo_wine ok(status == STATUS_INVALID_DEVICE_REQUEST, "got %#x\n", status);

    file = CreateFileA("\\\\.\\WineTestDriver\\subfile", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    todo_wine ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());
    if (file == INVALID_HANDLE_VALUE) return;

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), &size);
    ok(!status, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfile"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, size - 2, &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfil"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(*name_info), &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);

    status = NtQueryObject(file, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(file_info->FileName, file_info->FileNameLength, L"\\subfile"),
            "wrong name %s\n", debugstr_wn(file_info->FileName, file_info->FileNameLength / sizeof(WCHAR)));

    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsVolumeInformation);
    ok(!status, "got %#x\n", status);
    ok(volume_info->VolumeSerialNumber == 0xdeadbeef,
            "wrong serial number 0x%08x\n", volume_info->VolumeSerialNumber);
    ok(compare_unicode_string(volume_info->VolumeLabel, volume_info->VolumeLabelLength, L"WineTestDriver"),
            "wrong name %s\n", debugstr_wn(volume_info->VolumeLabel, volume_info->VolumeLabelLength / sizeof(WCHAR)));

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\notimpl", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_NOT_IMPLEMENTED, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badparam", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_INVALID_PARAMETER, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\genfail", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_UNSUCCESSFUL, "got %#x\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_UNSUCCESSFUL, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badtype", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#x\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#x\n", status);

    CloseHandle(file);
}

static void test_driver3(struct testsign_context *ctx)
{
    WCHAR filename[MAX_PATH];
    SC_HANDLE service;
    BOOL ret;

    service = load_driver(ctx, filename, L"driver3.dll", L"WineTestDriver3");
    ok(service != NULL, "driver3 failed to load\n");

    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "driver3 should fail to start\n");
    ok(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ||
       GetLastError() == ERROR_INVALID_FUNCTION ||
       GetLastError() == ERROR_PROC_NOT_FOUND /* XP */ ||
       GetLastError() == ERROR_FILE_NOT_FOUND /* Win7 */, "got %u\n", GetLastError());

    DeleteService(service);
    CloseServiceHandle(service);
    DeleteFileW(filename);
}

static DWORD WINAPI wsk_test_thread(void *parameter)
{
    static const char test_send_string[] = "Client test string 1.";
    static const WORD version = MAKEWORD(2, 2);
    SOCKET s_listen, s_accept, s_connect;
    struct sockaddr_in addr;
    char buffer[256];
    int ret, err;
    WSADATA data;
    int opt_val;

    ret = WSAStartup(version, &data);
    ok(!ret, "WSAStartup() failed, ret %u.\n", ret);

    s_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s_connect != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    s_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s_listen != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    opt_val = 1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt_val, sizeof(opt_val));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CLIENT_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(0x7f000001);
    ret = bind(s_listen, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "Got unexpected ret %d, WSAGetLastError() %u.\n", ret, WSAGetLastError());

    ret = listen(s_listen, SOMAXCONN);
    ok(!ret, "Got unexpected ret %d, WSAGetLastError() %u.\n", ret, WSAGetLastError());

    addr.sin_port = htons(SERVER_LISTEN_PORT);

    ret = connect(s_connect, (struct sockaddr *)&addr, sizeof(addr));
    while (ret && ((err = WSAGetLastError()) == WSAECONNREFUSED || err == WSAECONNABORTED))
    {
        SwitchToThread();
        ret = connect(s_connect, (struct sockaddr *)&addr, sizeof(addr));
    }
    ok(!ret, "Error connecting, WSAGetLastError() %u.\n", WSAGetLastError());

    ret = send(s_connect, test_send_string, sizeof(test_send_string), 0);
    ok(ret == sizeof(test_send_string), "Got unexpected ret %d.\n", ret);

    ret = recv(s_connect, buffer, sizeof(buffer), 0);
    ok(ret == sizeof(buffer), "Got unexpected ret %d.\n", ret);
    ok(!strcmp(buffer, "Server test string 1."), "Received unexpected data.\n");

    s_accept = accept(s_listen, NULL, NULL);
    ok(s_accept != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    closesocket(s_accept);
    closesocket(s_connect);
    closesocket(s_listen);
    return TRUE;
}

static void test_driver_netio(struct testsign_context *ctx)
{
    WCHAR filename[MAX_PATH];
    SC_HANDLE service;
    HANDLE hthread;
    BOOL ret;

    if (!(service = load_driver(ctx, filename, L"driver_netio.dll", L"winetest_netio")))
        return;

    if (!start_driver(service, TRUE))
    {
        DeleteFileW(filename);
        return;
    }

    device = CreateFileA("\\\\.\\winetest_netio", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    hthread = CreateThread(NULL, 0, wsk_test_thread, NULL, 0, NULL);
    main_test();
    WaitForSingleObject(hthread, INFINITE);

    CloseHandle(device);

    unload_driver(service);
    ret = DeleteFileW(filename);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());

    cat_okfile();
}

#ifdef __i386__
#define EXT "x86"
#elif defined(__x86_64__)
#define EXT "amd64"
#elif defined(__arm__)
#define EXT "arm"
#elif defined(__aarch64__)
#define EXT "arm64"
#else
#define EXT
#endif

static const char inf_text[] =
    "[Version]\n"
    "Signature=$Chicago$\n"
    "ClassGuid={4d36e97d-e325-11ce-bfc1-08002be10318}\n"
    "CatalogFile=winetest.cat\n"
    "DriverVer=09/21/2006,6.0.5736.1\n"

    "[Manufacturer]\n"
    "Wine=mfg_section,NT" EXT "\n"

    "[mfg_section.NT" EXT "]\n"
    "Wine test root driver=device_section,test_hardware_id\n"

    "[device_section.NT" EXT "]\n"
    "CopyFiles=file_section\n"

    "[device_section.NT" EXT ".Services]\n"
    "AddService=winetest,0x2,svc_section\n"

    "[file_section]\n"
    "winetest.sys\n"

    "[SourceDisksFiles]\n"
    "winetest.sys=1\n"

    "[SourceDisksNames]\n"
    "1=,winetest.sys\n"

    "[DestinationDirs]\n"
    "DefaultDestDir=12\n"

    "[svc_section]\n"
    "ServiceBinary=%12%\\winetest.sys\n"
    "ServiceType=1\n"
    "StartType=3\n"
    "ErrorControl=1\n"
    "LoadOrderGroup=Extended Base\n"
    "DisplayName=\"winetest bus driver\"\n"
    "; they don't sleep anymore, on the beach\n";

static void add_file_to_catalog(HANDLE catalog, const WCHAR *file)
{
    SIP_SUBJECTINFO subject_info = {sizeof(SIP_SUBJECTINFO)};
    SIP_INDIRECT_DATA *indirect_data;
    const WCHAR *filepart = file;
    CRYPTCATMEMBER *member;
    WCHAR hash_buffer[100];
    GUID subject_guid;
    unsigned int i;
    DWORD size;
    BOOL ret;

    ret = CryptSIPRetrieveSubjectGuidForCatalogFile(file, NULL, &subject_guid);
    todo_wine ok(ret, "Failed to get subject guid, error %u\n", GetLastError());

    size = 0;
    subject_info.pgSubjectType = &subject_guid;
    subject_info.pwsFileName = file;
    subject_info.DigestAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;
    subject_info.dwFlags = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG | SPC_EXC_PE_PAGE_HASHES_FLAG | 0x10000;
    ret = CryptSIPCreateIndirectData(&subject_info, &size, NULL);
    todo_wine ok(ret, "Failed to get indirect data size, error %u\n", GetLastError());

    indirect_data = malloc(size);
    ret = CryptSIPCreateIndirectData(&subject_info, &size, indirect_data);
    todo_wine ok(ret, "Failed to get indirect data, error %u\n", GetLastError());
    if (ret)
    {
        memset(hash_buffer, 0, sizeof(hash_buffer));
        for (i = 0; i < indirect_data->Digest.cbData; ++i)
            swprintf(&hash_buffer[i * 2], 2, L"%02X", indirect_data->Digest.pbData[i]);

        member = CryptCATPutMemberInfo(catalog, (WCHAR *)file,
                hash_buffer, &subject_guid, 0, size, (BYTE *)indirect_data);
        ok(!!member, "Failed to write member, error %u\n", GetLastError());

        if (wcsrchr(file, '\\'))
            filepart = wcsrchr(file, '\\') + 1;

        ret = !!CryptCATPutAttrInfo(catalog, member, (WCHAR *)L"File",
                CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                (wcslen(filepart) + 1) * 2, (BYTE *)filepart);
        ok(ret, "Failed to write attr, error %u\n", GetLastError());

        ret = !!CryptCATPutAttrInfo(catalog, member, (WCHAR *)L"OSAttr",
                CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                sizeof(L"2:6.0"), (BYTE *)L"2:6.0");
        ok(ret, "Failed to write attr, error %u\n", GetLastError());
    }
}

static const GUID bus_class     = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc1}};
static const GUID child_class   = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc2}};

static unsigned int got_bus_arrival, got_bus_removal, got_child_arrival, got_child_removal;

static LRESULT WINAPI device_notify_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message != WM_DEVICECHANGE)
        return DefWindowProcA(window, message, wparam, lparam);

    switch (wparam)
    {
        case DBT_DEVNODES_CHANGED:
            if (winetest_debug > 1) trace("device nodes changed\n");

            ok(InSendMessageEx(NULL) == ISMEX_NOTIFY, "got message flags %#x\n", InSendMessageEx(NULL));
            ok(!lparam, "got lparam %#Ix\n", lparam);
            break;

        case DBT_DEVICEARRIVAL:
        {
            const DEV_BROADCAST_DEVICEINTERFACE_A *iface = (const DEV_BROADCAST_DEVICEINTERFACE_A *)lparam;
            DWORD expect_size = offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[strlen(iface->dbcc_name)]);

            if (winetest_debug > 1) trace("device arrival %s\n", iface->dbcc_name);

            ok(InSendMessageEx(NULL) == ISMEX_SEND, "got message flags %#x\n", InSendMessageEx(NULL));

            ok(iface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE,
                    "got unexpected notification type %#x\n", iface->dbcc_devicetype);
            ok(iface->dbcc_size >= expect_size, "expected size at least %u, got %u\n", expect_size, iface->dbcc_size);
            ok(!iface->dbcc_reserved, "got reserved %#x\n", iface->dbcc_reserved);
            if (IsEqualGUID(&iface->dbcc_classguid, &bus_class))
            {
                ++got_bus_arrival;
                todo_wine ok(!strcmp(iface->dbcc_name, "\\\\?\\ROOT#WINETEST#0#{deadbeef-29ef-4538-a5fd-b69573a362c1}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            else if (IsEqualGUID(&iface->dbcc_classguid, &child_class))
            {
                ++got_child_arrival;
                todo_wine ok(!strcmp(iface->dbcc_name, "\\\\?\\wine#test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            break;
        }

        case DBT_DEVICEREMOVECOMPLETE:
        {
            const DEV_BROADCAST_DEVICEINTERFACE_A *iface = (const DEV_BROADCAST_DEVICEINTERFACE_A *)lparam;
            DWORD expect_size = offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[strlen(iface->dbcc_name)]);

            if (winetest_debug > 1) trace("device removal %s\n", iface->dbcc_name);

            ok(InSendMessageEx(NULL) == ISMEX_SEND, "got message flags %#x\n", InSendMessageEx(NULL));

            ok(iface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE,
                    "got unexpected notification type %#x\n", iface->dbcc_devicetype);
            ok(iface->dbcc_size >= expect_size, "expected size at least %u, got %u\n", expect_size, iface->dbcc_size);
            ok(!iface->dbcc_reserved, "got reserved %#x\n", iface->dbcc_reserved);
            if (IsEqualGUID(&iface->dbcc_classguid, &bus_class))
            {
                ++got_bus_removal;
                todo_wine ok(!strcmp(iface->dbcc_name, "\\\\?\\ROOT#WINETEST#0#{deadbeef-29ef-4538-a5fd-b69573a362c1}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            else if (IsEqualGUID(&iface->dbcc_classguid, &child_class))
            {
                ++got_child_removal;
                todo_wine ok(!strcmp(iface->dbcc_name, "\\\\?\\wine#test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            break;
        }
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

static void pump_messages(void)
{
    MSG msg;

    if (!MsgWaitForMultipleObjects(0, NULL, FALSE, 200, QS_ALLINPUT))
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

static void test_pnp_devices(void)
{
    static const char expect_hardware_id[] = "winetest_hardware\0winetest_hardware_1\0";
    static const char expect_compat_id[] = "winetest_compat\0winetest_compat_1\0";

    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *iface_detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DEV_BROADCAST_DEVICEINTERFACE_A filter =
    {
        .dbcc_size = sizeof(filter),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
    };
    static const WNDCLASSA class =
    {
        .lpszClassName = "ntoskrnl_test_wc",
        .lpfnWndProc = device_notify_proc,
    };
    HDEVNOTIFY notify_handle;
    DWORD size, type, dword;
    HANDLE bus, child, tmp;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    OVERLAPPED ovl = {0};
    IO_STATUS_BLOCK io;
    HDEVINFO set;
    HWND window;
    BOOL ret;
    int id;

    ret = RegisterClassA(&class);
    ok(ret, "failed to register class\n");
    window = CreateWindowA("ntoskrnl_test_wc", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    ok(!!window, "failed to create window\n");
    notify_handle = RegisterDeviceNotificationA(window, &filter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    ok(!!notify_handle, "failed to register window, error %u\n", GetLastError());

    set = SetupDiGetClassDevsA(&control_class, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());

    ret = SetupDiEnumDeviceInfo(set, 0, &device);
    ok(ret, "failed to get device, error %#x\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &GUID_DEVCLASS_SYSTEM), "wrong class %s\n", debugstr_guid(&device.ClassGuid));

    ret = SetupDiGetDeviceInstanceIdA(set, &device, buffer, sizeof(buffer), NULL);
    ok(ret, "failed to get device ID, error %#x\n", GetLastError());
    ok(!strcasecmp(buffer, "root\\winetest\\0"), "got ID %s\n", debugstr_a(buffer));

    ret = SetupDiEnumDeviceInterfaces(set, NULL, &control_class, 0, &iface);
    ok(ret, "failed to get interface, error %#x\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &control_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == SPINT_ACTIVE, "got flags %#x\n", iface.Flags);

    iface_detail->cbSize = sizeof(*iface_detail);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, iface_detail, sizeof(buffer), NULL, NULL);
    ok(ret, "failed to get interface path, error %#x\n", GetLastError());
    ok(!strcasecmp(iface_detail->DevicePath, "\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}"),
            "wrong path %s\n", debugstr_a(iface_detail->DevicePath));

    SetupDiDestroyDeviceInfoList(set);

    bus = CreateFileA(iface_detail->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(bus != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_MAIN, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    /* Test IoRegisterDeviceInterface() and IoSetDeviceInterfaceState(). */

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#x\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_REGISTER_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#x\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(!iface.Flags, "got flags %#x\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#x\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_ENABLE_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    pump_messages();
    ok(got_bus_arrival == 1, "got %u bus arrival messages\n", got_bus_arrival);
    ok(!got_bus_removal, "got %u bus removal messages\n", got_bus_removal);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#x\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == SPINT_ACTIVE, "got flags %#x\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_DISABLE_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    pump_messages();
    ok(got_bus_arrival == 1, "got %u bus arrival messages\n", got_bus_arrival);
    ok(got_bus_removal == 1, "got %u bus removal messages\n", got_bus_removal);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#x\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(!iface.Flags, "got flags %#x\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#x\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    /* Test exposing a child device. */

    id = 1;
    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_ADD_CHILD, &id, sizeof(id), NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    pump_messages();
    ok(got_child_arrival == 1, "got %u child arrival messages\n", got_child_arrival);
    ok(!got_child_removal, "got %u child removal messages\n", got_child_removal);

    set = SetupDiGetClassDevsA(&child_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());

    ret = SetupDiEnumDeviceInfo(set, 0, &device);
    ok(ret, "failed to get device, error %#x\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &GUID_NULL), "wrong class %s\n", debugstr_guid(&device.ClassGuid));

    ret = SetupDiGetDeviceInstanceIdA(set, &device, buffer, sizeof(buffer), NULL);
    ok(ret, "failed to get device ID, error %#x\n", GetLastError());
    ok(!strcasecmp(buffer, "wine\\test\\1"), "got ID %s\n", debugstr_a(buffer));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CAPABILITIES,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    todo_wine ok(ret, "got error %#x\n", GetLastError());
    if (ret)
    {
        ok(dword == (CM_DEVCAP_EJECTSUPPORTED | CM_DEVCAP_UNIQUEID
                | CM_DEVCAP_RAWDEVICEOK | CM_DEVCAP_SURPRISEREMOVALOK), "got flags %#x\n", dword);
        ok(type == REG_DWORD, "got type %u\n", type);
    }

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASSGUID,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    todo_wine ok(!ret, "expected failure\n");
    if (ret)
        ok(GetLastError() == ERROR_INVALID_DATA, "got error %#x\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVTYPE,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "got error %#x\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "got error %#x\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            &type, (BYTE *)buffer, sizeof(buffer), &size);
    ok(ret, "got error %#x\n", GetLastError());
    ok(type == REG_MULTI_SZ, "got type %u\n", type);
    ok(size == sizeof(expect_hardware_id), "got size %u\n", size);
    ok(!memcmp(buffer, expect_hardware_id, size), "got hardware IDs %s\n", debugstr_an(buffer, size));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_COMPATIBLEIDS,
            &type, (BYTE *)buffer, sizeof(buffer), &size);
    ok(ret, "got error %#x\n", GetLastError());
    ok(type == REG_MULTI_SZ, "got type %u\n", type);
    ok(size == sizeof(expect_compat_id), "got size %u\n", size);
    ok(!memcmp(buffer, expect_compat_id, size), "got compatible IDs %s\n", debugstr_an(buffer, size));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    todo_wine ok(ret, "got error %#x\n", GetLastError());
    if (ret)
    {
        ok(type == REG_SZ, "got type %u\n", type);
        ok(!strcmp(buffer, "\\Device\\winetest_pnp_1"), "got PDO name %s\n", debugstr_a(buffer));
    }

    SetupDiDestroyDeviceInfoList(set);

    RtlInitUnicodeString(&string, L"\\Device\\winetest_pnp_1");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtOpenFile(&child, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    ok(!ret, "failed to open child: %#x\n", ret);

    id = 0xdeadbeef;
    ret = DeviceIoControl(child, IOCTL_WINETEST_CHILD_GET_ID, NULL, 0, &id, sizeof(id), &size, NULL);
    ok(ret, "got error %u\n", GetLastError());
    ok(id == 1, "got id %d\n", id);
    ok(size == sizeof(id), "got size %u\n", size);

    CloseHandle(child);

    ret = NtOpenFile(&child, SYNCHRONIZE, &attr, &io, 0, 0);
    ok(!ret, "failed to open child: %#x\n", ret);

    ret = DeviceIoControl(child, IOCTL_WINETEST_MARK_PENDING, NULL, 0, NULL, 0, &size, &ovl);
    ok(!ret, "DeviceIoControl succeded\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %u\n", GetLastError());
    ok(size == 0, "got size %u\n", size);

    id = 1;
    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_REMOVE_CHILD, &id, sizeof(id), NULL, 0, &size, NULL);
    ok(ret, "got error %u\n", GetLastError());

    pump_messages();
    ok(got_child_arrival == 1, "got %u child arrival messages\n", got_child_arrival);
    ok(got_child_removal == 1, "got %u child removal messages\n", got_child_removal);

    ret = DeviceIoControl(child, IOCTL_WINETEST_CHECK_REMOVED, NULL, 0, NULL, 0, &size, NULL);
    todo_wine ok(ret, "got error %u\n", GetLastError());

    ret = NtOpenFile(&tmp, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    todo_wine ok(ret == STATUS_NO_SUCH_DEVICE, "got %#x\n", ret);

    ret = GetOverlappedResult(child, &ovl, &size, TRUE);
    ok(!ret, "unexpected success.\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %u\n", GetLastError());
    ok(size == 0, "got size %u\n", size);

    CloseHandle(child);

    pump_messages();
    ok(got_child_arrival == 1, "got %u child arrival messages\n", got_child_arrival);
    ok(got_child_removal == 1, "got %u child removal messages\n", got_child_removal);

    ret = NtOpenFile(&tmp, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    ok(ret == STATUS_OBJECT_NAME_NOT_FOUND, "got %#x\n", ret);

    CloseHandle(bus);

    UnregisterDeviceNotification(notify_handle);
    DestroyWindow(window);
    UnregisterClassA("ntoskrnl_test_wc", GetModuleHandleA(NULL));
}

static void test_pnp_driver(struct testsign_context *ctx)
{
    static const char hardware_id[] = "test_hardware_id\0";
    char path[MAX_PATH], dest[MAX_PATH], *filepart;
    SP_DEVINFO_DATA device = {sizeof(device)};
    char cwd[MAX_PATH], tempdir[MAX_PATH];
    WCHAR driver_filename[MAX_PATH];
    SC_HANDLE manager, service;
    BOOL ret, need_reboot;
    HANDLE catalog, file;
    unsigned int i;
    HDEVINFO set;
    FILE *f;

    GetCurrentDirectoryA(ARRAY_SIZE(cwd), cwd);
    GetTempPathA(ARRAY_SIZE(tempdir), tempdir);
    SetCurrentDirectoryA(tempdir);

    load_resource(L"driver_pnp.dll", driver_filename);
    ret = MoveFileExW(driver_filename, L"winetest.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    ok(ret, "failed to move file, error %u\n", GetLastError());

    f = fopen("winetest.inf", "w");
    ok(!!f, "failed to open winetest.inf: %s\n", strerror(errno));
    fputs(inf_text, f);
    fclose(f);

    /* Create the catalog file. */

    catalog = CryptCATOpen((WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#x\n", GetLastError());

    ret = !!CryptCATPutCatAttrInfo(catalog, (WCHAR *)L"HWID1",
            CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
            sizeof(L"test_hardware_id"), (BYTE *)L"test_hardware_id");
    todo_wine ok(ret, "failed to add attribute, error %#x\n", GetLastError());

    ret = !!CryptCATPutCatAttrInfo(catalog, (WCHAR *)L"OS",
            CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
            sizeof(L"VistaX64"), (BYTE *)L"VistaX64");
    todo_wine ok(ret, "failed to add attribute, error %#x\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest.sys");
    add_file_to_catalog(catalog, L"winetest.inf");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %u\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %u\n", GetLastError());

    testsign_sign(ctx, L"winetest.cat");

    /* Install the driver. */

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "failed to create device list, error %#x\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "root\\winetest\\0", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "failed to create device, error %#x\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA( set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id) );
    ok(ret, "failed to create set hardware ID, error %#x\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "failed to register device, error %#x\n", GetLastError());

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = UpdateDriverForPlugAndPlayDevicesA(NULL, hardware_id, path, INSTALLFLAG_FORCE, &need_reboot);
    ok(ret, "failed to install device, error %#x\n", GetLastError());
    ok(!need_reboot, "expected no reboot necessary\n");

    /* Tests. */

    test_pnp_devices();

    /* Clean up. */

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "failed to remove device, error %#x\n", GetLastError());

    file = CreateFileA("\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file == INVALID_HANDLE_VALUE, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %u\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "failed to destroy set, error %#x\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "wine", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
        ok(ret, "failed to remove device, error %#x\n", GetLastError());
    }

    /* Windows stops the service but does not delete it. */
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(!!manager, "failed to open service manager, error %u\n", GetLastError());
    service = OpenServiceA(manager, "winetest", SERVICE_STOP | DELETE);
    ok(!!service, "failed to open service, error %u\n", GetLastError());
    unload_driver(service);
    CloseServiceHandle(manager);

    cat_okfile();

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = SetupCopyOEMInfA(path, NULL, 0, 0, dest, sizeof(dest), NULL, &filepart);
    ok(ret, "Failed to copy INF, error %#x\n", GetLastError());
    ret = SetupUninstallOEMInfA(filepart, 0, NULL);
    ok(ret, "Failed to uninstall INF, error %u\n", GetLastError());

    ret = DeleteFileA("winetest.cat");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    ret = DeleteFileA("winetest.inf");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    ret = DeleteFileA("winetest.sys");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    /* Windows 10 apparently deletes the image in SetupUninstallOEMInf(). */
    ret = DeleteFileA("C:/windows/system32/drivers/winetest.sys");
    ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %u\n", GetLastError());

    SetCurrentDirectoryA(cwd);
}

#define check_member_(file, line, val, exp, fmt, member)               \
        ok_(file, line)((val).member == (exp).member,                  \
                        "got " #member " " fmt ", expected " fmt "\n", \
                        (val).member, (exp).member)
#define check_member(val, exp, fmt, member) check_member_(__FILE__, __LINE__, val, exp, fmt, member)

#define check_hidp_caps(a, b) check_hidp_caps_(__LINE__, a, b)
static inline void check_hidp_caps_(int line, HIDP_CAPS *caps, const HIDP_CAPS *exp)
{
    check_member_(__FILE__, line, *caps, *exp, "%04x", Usage);
    check_member_(__FILE__, line, *caps, *exp, "%04x", UsagePage);
    check_member_(__FILE__, line, *caps, *exp, "%d", InputReportByteLength);
    check_member_(__FILE__, line, *caps, *exp, "%d", OutputReportByteLength);
    check_member_(__FILE__, line, *caps, *exp, "%d", FeatureReportByteLength);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberLinkCollectionNodes);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberInputButtonCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberInputValueCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberInputDataIndices);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberOutputButtonCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberOutputValueCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberOutputDataIndices);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberFeatureButtonCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberFeatureValueCaps);
    check_member_(__FILE__, line, *caps, *exp, "%d", NumberFeatureDataIndices);
}

#define check_hidp_link_collection_node(a, b) check_hidp_link_collection_node_(__LINE__, a, b)
static inline void check_hidp_link_collection_node_(int line, HIDP_LINK_COLLECTION_NODE *node,
                                                    const HIDP_LINK_COLLECTION_NODE *exp)
{
    check_member_(__FILE__, line, *node, *exp, "%04x", LinkUsage);
    check_member_(__FILE__, line, *node, *exp, "%04x", LinkUsagePage);
    check_member_(__FILE__, line, *node, *exp, "%d", Parent);
    check_member_(__FILE__, line, *node, *exp, "%d", NumberOfChildren);
    check_member_(__FILE__, line, *node, *exp, "%d", NextSibling);
    check_member_(__FILE__, line, *node, *exp, "%d", FirstChild);
    check_member_(__FILE__, line, *node, *exp, "%d", CollectionType);
    check_member_(__FILE__, line, *node, *exp, "%d", IsAlias);
}

#define check_hidp_button_caps(a, b) check_hidp_button_caps_(__LINE__, a, b)
static inline void check_hidp_button_caps_(int line, HIDP_BUTTON_CAPS *caps, const HIDP_BUTTON_CAPS *exp)
{
    check_member_(__FILE__, line, *caps, *exp, "%04x", UsagePage);
    check_member_(__FILE__, line, *caps, *exp, "%d", ReportID);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsAlias);
    check_member_(__FILE__, line, *caps, *exp, "%d", BitField);
    check_member_(__FILE__, line, *caps, *exp, "%d", LinkCollection);
    check_member_(__FILE__, line, *caps, *exp, "%04x", LinkUsage);
    check_member_(__FILE__, line, *caps, *exp, "%04x", LinkUsagePage);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsStringRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsDesignatorRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsAbsolute);

    if (!caps->IsRange && !exp->IsRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%04x", NotRange.Usage);
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.DataIndex);
    }
    else if (caps->IsRange && exp->IsRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%04x", Range.UsageMin);
        check_member_(__FILE__, line, *caps, *exp, "%04x", Range.UsageMax);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DataIndexMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DataIndexMax);
    }

    if (!caps->IsRange && !exp->IsRange)
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.StringIndex);
    else if (caps->IsStringRange && exp->IsStringRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.StringMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.StringMax);
    }

    if (!caps->IsDesignatorRange && !exp->IsDesignatorRange)
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.DesignatorIndex);
    else if (caps->IsDesignatorRange && exp->IsDesignatorRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DesignatorMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DesignatorMax);
    }
}

#define check_hidp_value_caps(a, b) check_hidp_value_caps_(__LINE__, a, b)
static inline void check_hidp_value_caps_(int line, HIDP_VALUE_CAPS *caps, const HIDP_VALUE_CAPS *exp)
{
    check_member_(__FILE__, line, *caps, *exp, "%04x", UsagePage);
    check_member_(__FILE__, line, *caps, *exp, "%d", ReportID);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsAlias);
    check_member_(__FILE__, line, *caps, *exp, "%d", BitField);
    check_member_(__FILE__, line, *caps, *exp, "%d", LinkCollection);
    check_member_(__FILE__, line, *caps, *exp, "%d", LinkUsage);
    check_member_(__FILE__, line, *caps, *exp, "%d", LinkUsagePage);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsStringRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsDesignatorRange);
    check_member_(__FILE__, line, *caps, *exp, "%d", IsAbsolute);

    check_member_(__FILE__, line, *caps, *exp, "%d", HasNull);
    check_member_(__FILE__, line, *caps, *exp, "%d", BitSize);
    check_member_(__FILE__, line, *caps, *exp, "%d", ReportCount);
    check_member_(__FILE__, line, *caps, *exp, "%d", UnitsExp);
    check_member_(__FILE__, line, *caps, *exp, "%d", Units);
    check_member_(__FILE__, line, *caps, *exp, "%d", LogicalMin);
    check_member_(__FILE__, line, *caps, *exp, "%d", LogicalMax);
    check_member_(__FILE__, line, *caps, *exp, "%d", PhysicalMin);
    check_member_(__FILE__, line, *caps, *exp, "%d", PhysicalMax);

    if (!caps->IsRange && !exp->IsRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%04x", NotRange.Usage);
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.DataIndex);
    }
    else if (caps->IsRange && exp->IsRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%04x", Range.UsageMin);
        check_member_(__FILE__, line, *caps, *exp, "%04x", Range.UsageMax);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DataIndexMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DataIndexMax);
    }

    if (!caps->IsRange && !exp->IsRange)
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.StringIndex);
    else if (caps->IsStringRange && exp->IsStringRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.StringMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.StringMax);
    }

    if (!caps->IsDesignatorRange && !exp->IsDesignatorRange)
        check_member_(__FILE__, line, *caps, *exp, "%d", NotRange.DesignatorIndex);
    else if (caps->IsDesignatorRange && exp->IsDesignatorRange)
    {
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DesignatorMin);
        check_member_(__FILE__, line, *caps, *exp, "%d", Range.DesignatorMax);
    }
}

static BOOL sync_ioctl(HANDLE file, DWORD code, void *in_buf, DWORD in_len, void *out_buf, DWORD *ret_len)
{
    OVERLAPPED ovl = {0};
    DWORD out_len = ret_len ? *ret_len : 0;
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ret = DeviceIoControl(file, code, in_buf, in_len, out_buf, out_len, &out_len, &ovl);
    if (!ret && GetLastError() == ERROR_IO_PENDING) ret = GetOverlappedResult(file, &ovl, &out_len, TRUE);
    CloseHandle(ovl.hEvent);

    if (ret_len) *ret_len = out_len;
    return ret;
}

static void test_hidp(HANDLE file, HANDLE async_file, int report_id, BOOL polled)
{
    const HIDP_CAPS expect_hidp_caps[] =
    {
        /* without report id */
        {
            .Usage = HID_USAGE_GENERIC_JOYSTICK,
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .InputReportByteLength = 24,
            .OutputReportByteLength = 3,
            .FeatureReportByteLength = 18,
            .NumberLinkCollectionNodes = 10,
            .NumberInputButtonCaps = 13,
            .NumberInputValueCaps = 7,
            .NumberInputDataIndices = 43,
            .NumberFeatureButtonCaps = 1,
            .NumberFeatureValueCaps = 5,
            .NumberFeatureDataIndices = 7,
        },
        /* with report id */
        {
            .Usage = HID_USAGE_GENERIC_JOYSTICK,
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .InputReportByteLength = 23,
            .OutputReportByteLength = 2,
            .FeatureReportByteLength = 17,
            .NumberLinkCollectionNodes = 10,
            .NumberInputButtonCaps = 13,
            .NumberInputValueCaps = 7,
            .NumberInputDataIndices = 43,
            .NumberFeatureButtonCaps = 1,
            .NumberFeatureValueCaps = 5,
            .NumberFeatureDataIndices = 7,
        },
    };
    const HIDP_BUTTON_CAPS expect_button_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsRange = TRUE,
            .IsAbsolute = TRUE,
            .Range.UsageMin = 1,
            .Range.UsageMax = 8,
            .Range.DataIndexMin = 2,
            .Range.DataIndexMax = 9,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 3,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = TRUE,
            .IsAbsolute = TRUE,
            .Range.UsageMin = 0x18,
            .Range.UsageMax = 0x1f,
            .Range.DataIndexMin = 10,
            .Range.DataIndexMax = 17,
        },
        {
            .UsagePage = HID_USAGE_PAGE_KEYBOARD,
            .ReportID = report_id,
            .BitField = 0x1fc,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = TRUE,
            .IsAbsolute = FALSE,
            .Range.UsageMin = 0x8,
            .Range.UsageMax = 0xf,
            .Range.DataIndexMin = 18,
            .Range.DataIndexMax = 25,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = FALSE,
            .IsAbsolute = TRUE,
            .NotRange.Usage = 0x20,
            .NotRange.Reserved1 = 0x20,
            .NotRange.DataIndex = 26,
            .NotRange.Reserved4 = 26,
        },
    };
    const HIDP_VALUE_CAPS expect_value_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 8,
            .ReportCount = 1,
            .LogicalMin = -128,
            .LogicalMax = 127,
            .NotRange.Usage = HID_USAGE_GENERIC_Y,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_Y,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 8,
            .ReportCount = 1,
            .LogicalMin = -128,
            .LogicalMax = 127,
            .NotRange.Usage = HID_USAGE_GENERIC_X,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_X,
            .NotRange.DataIndex = 1,
            .NotRange.Reserved4 = 1,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .ReportCount = 1,
            .LogicalMax = 1,
            .IsRange = TRUE,
            .Range.UsageMin = 0x21,
            .Range.UsageMax = 0x22,
            .Range.DataIndexMin = 27,
            .Range.DataIndexMax = 28,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 4,
            .ReportCount = 2,
            .LogicalMin = 1,
            .LogicalMax = 8,
            .NotRange.Usage = HID_USAGE_GENERIC_HATSWITCH,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_HATSWITCH,
            .NotRange.DataIndex = 29,
            .NotRange.Reserved4 = 29,
        },
    };
    static const HIDP_LINK_COLLECTION_NODE expect_collections[] =
    {
        {
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .CollectionType = 1,
            .NumberOfChildren = 7,
            .FirstChild = 9,
        },
        {
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .CollectionType = 2,
        },
    };
    static const HIDP_DATA expect_data[] =
    {
        { .DataIndex = 0, },
        { .DataIndex = 1, },
        { .DataIndex = 5, .RawValue = 1, },
        { .DataIndex = 7, .RawValue = 1, },
        { .DataIndex = 19, .RawValue = 1, },
        { .DataIndex = 21, .RawValue = 1, },
        { .DataIndex = 30, },
        { .DataIndex = 31, },
        { .DataIndex = 32, .RawValue = 0xfeedcafe, },
        { .DataIndex = 37, .RawValue = 1, },
        { .DataIndex = 39, .RawValue = 1, },
    };

    OVERLAPPED overlapped = {0}, overlapped2 = {0};
    HIDP_LINK_COLLECTION_NODE collections[16];
    PHIDP_PREPARSED_DATA preparsed_data;
    USAGE_AND_PAGE usage_and_pages[16];
    HIDP_BUTTON_CAPS button_caps[16];
    HIDP_VALUE_CAPS value_caps[16];
    char buffer[200], report[200];
    DWORD collection_count;
    DWORD waveform_list;
    HIDP_DATA data[32];
    USAGE usages[16];
    NTSTATUS status;
    HIDP_CAPS caps;
    unsigned int i;
    USHORT count;
    ULONG value;
    BOOL ret;

    ret = HidD_GetPreparsedData(file, &preparsed_data);
    ok(ret, "HidD_GetPreparsedData failed with error %u\n", GetLastError());

    memset(buffer, 0, sizeof(buffer));
    status = HidP_GetCaps((PHIDP_PREPARSED_DATA)buffer, &caps);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetCaps returned %#x\n", status);
    status = HidP_GetCaps(preparsed_data, &caps);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetCaps returned %#x\n", status);
    check_hidp_caps(&caps, &expect_hidp_caps[report_id]);

    collection_count = 0;
    status = HidP_GetLinkCollectionNodes(collections, &collection_count, preparsed_data);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetLinkCollectionNodes returned %#x\n", status);
    ok(collection_count == caps.NumberLinkCollectionNodes, "got %d collection nodes, expected %d\n",
       collection_count, caps.NumberLinkCollectionNodes);
    collection_count = ARRAY_SIZE(collections);
    status = HidP_GetLinkCollectionNodes(collections, &collection_count, (PHIDP_PREPARSED_DATA)buffer);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetLinkCollectionNodes returned %#x\n", status);
    status = HidP_GetLinkCollectionNodes(collections, &collection_count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetLinkCollectionNodes returned %#x\n", status);
    ok(collection_count == caps.NumberLinkCollectionNodes, "got %d collection nodes, expected %d\n",
       collection_count, caps.NumberLinkCollectionNodes);

    for (i = 0; i < ARRAY_SIZE(expect_collections); ++i)
    {
        winetest_push_context("collections[%d]", i);
        check_hidp_link_collection_node(&collections[i], &expect_collections[i]);
        winetest_pop_context();
    }

    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps(HidP_Output, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetButtonCaps returned %#x\n", status);
    status = HidP_GetButtonCaps(HidP_Feature + 1, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetButtonCaps returned %#x\n", status);
    count = 0;
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetButtonCaps returned %#x\n", status);
    ok(count == caps.NumberInputButtonCaps, "HidP_GetButtonCaps returned count %d, expected %d\n",
       count, caps.NumberInputButtonCaps);
    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetButtonCaps returned %#x\n", status);
    memset(button_caps, 0, sizeof(button_caps));
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetButtonCaps returned %#x\n", status);
    ok(count == caps.NumberInputButtonCaps, "HidP_GetButtonCaps returned count %d, expected %d\n",
       count, caps.NumberInputButtonCaps);

    for (i = 0; i < ARRAY_SIZE(expect_button_caps); ++i)
    {
        winetest_push_context("button_caps[%d]", i);
        check_hidp_button_caps(&button_caps[i], &expect_button_caps[i]);
        winetest_pop_context();
    }

    count = ARRAY_SIZE(button_caps) - 1;
    status = HidP_GetSpecificButtonCaps(HidP_Output, 0, 0, 0, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    status = HidP_GetSpecificButtonCaps(HidP_Feature + 1, 0, 0, 0, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    count = 0;
    status = HidP_GetSpecificButtonCaps(HidP_Input, 0, 0, 0, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
       count, caps.NumberInputButtonCaps);
    count = ARRAY_SIZE(button_caps) - 1;
    status = HidP_GetSpecificButtonCaps(HidP_Input, 0, 0, 0, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificButtonCaps returned %#x\n", status);

    status = HidP_GetSpecificButtonCaps(HidP_Input, 0, 0, 0, button_caps + 1, &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
       count, caps.NumberInputButtonCaps);
    check_hidp_button_caps(&button_caps[1], &button_caps[0]);

    status = HidP_GetSpecificButtonCaps(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, 5, button_caps + 1,
                                        &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == 1, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 1);
    check_hidp_button_caps(&button_caps[1], &button_caps[0]);

    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps(HidP_Input, 0xfffe, 0, 0, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0);
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps(HidP_Input, 0, 0xfffe, 0, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0);
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps(HidP_Input, 0, 0, 0xfffe, button_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0);

    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps(HidP_Output, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetValueCaps returned %#x\n", status);
    status = HidP_GetValueCaps(HidP_Feature + 1, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetValueCaps returned %#x\n", status);
    count = 0;
    status = HidP_GetValueCaps(HidP_Input, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetValueCaps returned %#x\n", status);
    ok(count == caps.NumberInputValueCaps, "HidP_GetValueCaps returned count %d, expected %d\n",
       count, caps.NumberInputValueCaps);
    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps(HidP_Input, value_caps, &count, (PHIDP_PREPARSED_DATA)buffer);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetValueCaps returned %#x\n", status);
    status = HidP_GetValueCaps(HidP_Input, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetValueCaps returned %#x\n", status);
    ok(count == caps.NumberInputValueCaps, "HidP_GetValueCaps returned count %d, expected %d\n",
       count, caps.NumberInputValueCaps);

    for (i = 0; i < ARRAY_SIZE(expect_value_caps); ++i)
    {
        winetest_push_context("value_caps[%d]", i);
        check_hidp_value_caps(&value_caps[i], &expect_value_caps[i]);
        winetest_pop_context();
    }

    count = ARRAY_SIZE(value_caps) - 4;
    status = HidP_GetSpecificValueCaps(HidP_Output, 0, 0, 0, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status);
    status = HidP_GetSpecificValueCaps(HidP_Feature + 1, 0, 0, 0, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificValueCaps returned %#x\n", status);
    count = 0;
    status = HidP_GetSpecificValueCaps(HidP_Input, 0, 0, 0, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
       count, caps.NumberInputValueCaps);
    count = ARRAY_SIZE(value_caps) - 4;
    status = HidP_GetSpecificValueCaps(HidP_Input, 0, 0, 0, value_caps + 4, &count, (PHIDP_PREPARSED_DATA)buffer);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificValueCaps returned %#x\n", status);

    status = HidP_GetSpecificValueCaps(HidP_Input, 0, 0, 0, value_caps + 4, &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
       count, caps.NumberInputValueCaps);
    check_hidp_value_caps(&value_caps[4], &value_caps[0]);
    check_hidp_value_caps(&value_caps[5], &value_caps[1]);
    check_hidp_value_caps(&value_caps[6], &value_caps[2]);
    check_hidp_value_caps(&value_caps[7], &value_caps[3]);

    count = 1;
    status = HidP_GetSpecificValueCaps(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                       value_caps + 4, &count, preparsed_data);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == 1, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 1);
    check_hidp_value_caps(&value_caps[4], &value_caps[3]);

    count = 0xdead;
    status = HidP_GetSpecificValueCaps(HidP_Input, 0xfffe, 0, 0, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0);
    count = 0xdead;
    status = HidP_GetSpecificValueCaps(HidP_Input, 0, 0xfffe, 0, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0);
    count = 0xdead;
    status = HidP_GetSpecificValueCaps(HidP_Input, 0, 0, 0xfffe, value_caps, &count, preparsed_data);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status);
    ok(count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0);

    status = HidP_InitializeReportForID(HidP_Input, 0, (PHIDP_PREPARSED_DATA)buffer, report, sizeof(report));
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_InitializeReportForID returned %#x\n", status);
    status = HidP_InitializeReportForID(HidP_Feature + 1, 0, preparsed_data, report, sizeof(report));
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_InitializeReportForID returned %#x\n", status);
    status = HidP_InitializeReportForID(HidP_Input, 0, preparsed_data, report, sizeof(report));
    ok(status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#x\n", status);
    status = HidP_InitializeReportForID(HidP_Input, 0, preparsed_data, report, caps.InputReportByteLength + 1);
    ok(status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#x\n", status);
    status = HidP_InitializeReportForID(HidP_Input, 1 - report_id, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status);

    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status);

    memset(buffer, 0xcd, sizeof(buffer));
    memset(buffer, 0, caps.InputReportByteLength);
    buffer[0] = report_id;
    ok(!memcmp(buffer, report, sizeof(buffer)), "unexpected report data\n");

    status = HidP_SetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                     sizeof(buffer), preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_SetUsageValueArray returned %#x\n", status);
    memset(buffer, 0xcd, sizeof(buffer));
    status = HidP_SetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, buffer,
                                     0, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#x\n", status);
    status = HidP_SetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, buffer,
                                     8, preparsed_data, report, caps.InputReportByteLength);
    todo_wine
    ok(status == HIDP_STATUS_NOT_IMPLEMENTED, "HidP_SetUsageValueArray returned %#x\n", status);

    status = HidP_GetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                     sizeof(buffer), preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_GetUsageValueArray returned %#x\n", status);
    memset(buffer, 0xcd, sizeof(buffer));
    status = HidP_GetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, buffer,
                                     0, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#x\n", status);
    status = HidP_GetUsageValueArray(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, buffer,
                                     8, preparsed_data, report, caps.InputReportByteLength);
    todo_wine
    ok(status == HIDP_STATUS_NOT_IMPLEMENTED, "HidP_GetUsageValueArray returned %#x\n", status);

    value = -128;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                &value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status);
    ok(value == 0x80, "got value %x, expected %#x\n", value, 0x80);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == -128, "got value %x, expected %#x\n", value, -128);

    value = 127;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 127, "got value %x, expected %#x\n", value, 127);

    value = 0;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0, "got value %x, expected %#x\n", value, 0);

    value = 0x7fffffff;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_VALUE_OUT_OF_RANGE, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0, "got value %x, expected %#x\n", value, 0);
    value = 0xdeadbeef;
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                &value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status);
    ok(value == 0x7fffffff, "got value %x, expected %#x\n", value, 0x7fffffff);

    value = 0x3fffffff;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0x7fffffff, "got value %x, expected %#x\n", value, 0x7fffffff);

    value = 0;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0x80000000, "got value %x, expected %#x\n", value, 0x80000000);

    value = 0;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0, "got value %x, expected %#x\n", value, 0);

    value = 0xfeedcafe;
    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                      (LONG *)&value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BAD_LOG_PHY_VALUES, "HidP_GetScaledUsageValue returned %#x\n", status);
    ok(value == 0, "got value %x, expected %#x\n", value, 0);

    value = HidP_MaxUsageListLength(HidP_Feature + 1, 0, preparsed_data);
    ok(value == 0, "HidP_MaxUsageListLength(HidP_Feature + 1, 0) returned %d, expected %d\n", value, 0);
    value = HidP_MaxUsageListLength(HidP_Input, 0, preparsed_data);
    ok(value == 42, "HidP_MaxUsageListLength(HidP_Input, 0) returned %d, expected %d\n", value, 42);
    value = HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON, preparsed_data);
    ok(value == 32, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON) returned %d, expected %d\n", value, 32);
    value = HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_LED, preparsed_data);
    ok(value == 8, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_LED) returned %d, expected %d\n", value, 8);
    value = HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_BUTTON, preparsed_data);
    ok(value == 8, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_BUTTON) returned %d, expected %d\n", value, 8);
    value = HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_LED, preparsed_data);
    ok(value == 0, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_LED) returned %d, expected %d\n", value, 0);

    usages[0] = 0xff;
    value = 1;
    status = HidP_SetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value,
                            preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#x\n", status);
    usages[1] = 2;
    usages[2] = 0xff;
    value = 3;
    status = HidP_SetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value,
                            preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#x\n", status);
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value,
                            preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status);
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages(HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                            report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status);

    value = ARRAY_SIZE(usages);
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                            report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status);
    ok(value == 0, "got usage count %d, expected %d\n", value, 2);

    usages[0] = 0x9;
    usages[1] = 0xb;
    usages[2] = 0xa;
    value = 3;
    ok(report[6] == 0, "got report[6] %x expected 0\n", report[6]);
    ok(report[7] == 0, "got report[7] %x expected 0\n", report[7]);
    memcpy(buffer, report, caps.InputReportByteLength);
    status = HidP_SetUsages(HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                            report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsages returned %#x\n", status);
    buffer[6] = 2;
    buffer[7] = 4;
    ok(!memcmp(buffer, report, caps.InputReportByteLength), "unexpected report data\n");

    status = HidP_SetUsageValue(HidP_Input, HID_USAGE_PAGE_LED, 0, 6, 1,
                                preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status);

    value = 0xdeadbeef;
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_LED, 0, 6, &value,
                                preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status);
    ok(value == 0xdeadbeef, "got value %x, expected %#x\n", value, 0xdeadbeef);

    value = 1;
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value,
                            preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsages returned %#x\n", status);
    ok(value == 2, "got usage count %d, expected %d\n", value, 2);
    value = ARRAY_SIZE(usages);
    memset(usages, 0xcd, sizeof(usages));
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value,
                            preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status);
    ok(value == 2, "got usage count %d, expected %d\n", value, 2);
    ok(usages[0] == 4, "got usages[0] %x, expected %x\n", usages[0], 4);
    ok(usages[1] == 6, "got usages[1] %x, expected %x\n", usages[1], 6);

    value = ARRAY_SIZE(usages);
    memset(usages, 0xcd, sizeof(usages));
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                            report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status);
    ok(value == 2, "got usage count %d, expected %d\n", value, 2);
    ok(usages[0] == 6, "got usages[0] %x, expected %x\n", usages[0], 6);
    ok(usages[1] == 4, "got usages[1] %x, expected %x\n", usages[1], 4);

    value = ARRAY_SIZE(usage_and_pages);
    memset(usage_and_pages, 0xcd, sizeof(usage_and_pages));
    status = HidP_GetUsagesEx(HidP_Input, 0, usage_and_pages, &value, preparsed_data, report,
                              caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsagesEx returned %#x\n", status);
    ok(value == 6, "got usage count %d, expected %d\n", value, 4);
    ok(usage_and_pages[0].UsagePage == HID_USAGE_PAGE_BUTTON, "got usage_and_pages[0] UsagePage %x, expected %x\n",
       usage_and_pages[0].UsagePage, HID_USAGE_PAGE_BUTTON);
    ok(usage_and_pages[1].UsagePage == HID_USAGE_PAGE_BUTTON, "got usage_and_pages[1] UsagePage %x, expected %x\n",
       usage_and_pages[1].UsagePage, HID_USAGE_PAGE_BUTTON);
    ok(usage_and_pages[2].UsagePage == HID_USAGE_PAGE_KEYBOARD, "got usage_and_pages[2] UsagePage %x, expected %x\n",
       usage_and_pages[2].UsagePage, HID_USAGE_PAGE_KEYBOARD);
    ok(usage_and_pages[3].UsagePage == HID_USAGE_PAGE_KEYBOARD, "got usage_and_pages[3] UsagePage %x, expected %x\n",
       usage_and_pages[3].UsagePage, HID_USAGE_PAGE_KEYBOARD);
    ok(usage_and_pages[4].UsagePage == HID_USAGE_PAGE_LED, "got usage_and_pages[4] UsagePage %x, expected %x\n",
       usage_and_pages[4].UsagePage, HID_USAGE_PAGE_LED);
    ok(usage_and_pages[5].UsagePage == HID_USAGE_PAGE_LED, "got usage_and_pages[5] UsagePage %x, expected %x\n",
       usage_and_pages[5].UsagePage, HID_USAGE_PAGE_LED);
    ok(usage_and_pages[0].Usage == 4, "got usage_and_pages[0] Usage %x, expected %x\n",
       usage_and_pages[0].Usage, 4);
    ok(usage_and_pages[1].Usage == 6, "got usage_and_pages[1] Usage %x, expected %x\n",
       usage_and_pages[1].Usage, 6);
    ok(usage_and_pages[2].Usage == 9, "got usage_and_pages[2] Usage %x, expected %x\n",
       usage_and_pages[2].Usage, 9);
    ok(usage_and_pages[3].Usage == 11, "got usage_and_pages[3] Usage %x, expected %x\n",
       usage_and_pages[3].Usage, 11);
    ok(usage_and_pages[4].Usage == 6, "got usage_and_pages[4] Usage %x, expected %x\n",
       usage_and_pages[4].Usage, 6);
    ok(usage_and_pages[5].Usage == 4, "got usage_and_pages[5] Usage %x, expected %x\n",
       usage_and_pages[5].Usage, 4);

    value = HidP_MaxDataListLength(HidP_Feature + 1, preparsed_data);
    ok(value == 0, "HidP_MaxDataListLength(HidP_Feature + 1) returned %d, expected %d\n", value, 0);
    value = HidP_MaxDataListLength(HidP_Input, preparsed_data);
    ok(value == 50, "HidP_MaxDataListLength(HidP_Input) returned %d, expected %d\n", value, 50);
    value = HidP_MaxDataListLength(HidP_Output, preparsed_data);
    ok(value == 0, "HidP_MaxDataListLength(HidP_Output) returned %d, expected %d\n", value, 0);
    value = HidP_MaxDataListLength(HidP_Feature, preparsed_data);
    ok(value == 13, "HidP_MaxDataListLength(HidP_Feature) returned %d, expected %d\n", value, 13);

    value = 1;
    status = HidP_GetData(HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetData returned %#x\n", status);
    ok(value == 11, "got data count %d, expected %d\n", value, 11);
    memset(data, 0, sizeof(data));
    status = HidP_GetData(HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetData returned %#x\n", status);
    for (i = 0; i < ARRAY_SIZE(expect_data); ++i)
    {
        winetest_push_context("data[%d]", i);
        check_member(data[i], expect_data[i], "%d", DataIndex);
        check_member(data[i], expect_data[i], "%d", RawValue);
        winetest_pop_context();
    }

    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Feature, 3, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status);

    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Feature, report_id, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status);

    memset(buffer, 0xcd, sizeof(buffer));
    memset(buffer, 0, caps.FeatureReportByteLength);
    buffer[0] = report_id;
    ok(!memcmp(buffer, report, sizeof(buffer)), "unexpected report data\n");

    for (i = 0; i < caps.NumberLinkCollectionNodes; ++i)
    {
        if (collections[i].LinkUsagePage != HID_USAGE_PAGE_HAPTICS) continue;
        if (collections[i].LinkUsage == HID_USAGE_HAPTICS_WAVEFORM_LIST) break;
    }
    ok(i < caps.NumberLinkCollectionNodes,
       "HID_USAGE_HAPTICS_WAVEFORM_LIST collection not found\n");
    waveform_list = i;

    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, (PHIDP_PREPARSED_DATA)buffer,
                                report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_SetUsageValue returned %#x\n", status);
    status = HidP_SetUsageValue(HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_SetUsageValue returned %#x\n", status);
    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                caps.FeatureReportByteLength + 1);
    ok(status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_SetUsageValue returned %#x\n", status);
    report[0] = 1 - report_id;
    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                caps.FeatureReportByteLength);
    ok(status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
       "HidP_SetUsageValue returned %#x\n", status);
    report[0] = 2;
    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_SetUsageValue returned %#x\n", status);
    report[0] = report_id;
    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, HID_USAGE_HAPTICS_WAVEFORM_RUMBLE,
                                preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status);

    status = HidP_SetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status);

    memset(buffer, 0xcd, sizeof(buffer));
    memset(buffer, 0, caps.FeatureReportByteLength);
    buffer[0] = report_id;
    value = HID_USAGE_HAPTICS_WAVEFORM_RUMBLE;
    memcpy(buffer + 1, &value, 2);
    ok(!memcmp(buffer, report, sizeof(buffer)), "unexpected report data\n");

    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                (PHIDP_PREPARSED_DATA)buffer, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetUsageValue returned %#x\n", status);
    status = HidP_GetUsageValue(HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                &value, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetUsageValue returned %#x\n", status);
    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                preparsed_data, report, caps.FeatureReportByteLength + 1);
    ok(status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_GetUsageValue returned %#x\n", status);
    report[0] = 1 - report_id;
    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
       "HidP_GetUsageValue returned %#x\n", status);
    report[0] = 2;
    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_GetUsageValue returned %#x\n", status);
    report[0] = report_id;
    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, &value,
                                preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#x\n", status);

    value = 0xdeadbeef;
    status = HidP_GetUsageValue(HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status);
    ok(value == HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, "got value %x, expected %#x\n", value,
       HID_USAGE_HAPTICS_WAVEFORM_RUMBLE);

    memset(buffer, 0xff, sizeof(buffer));
    status = HidP_SetUsageValueArray(HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer,
                                     0, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#x\n", status);
    status = HidP_SetUsageValueArray(HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer,
                                     64, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValueArray returned %#x\n", status);
    ok(!memcmp(report + 9, buffer, 8), "unexpected report data\n");

    memset(buffer, 0, sizeof(buffer));
    status = HidP_GetUsageValueArray(HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer,
                                     0, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#x\n", status);
    status = HidP_GetUsageValueArray(HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer,
                                     64, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValueArray returned %#x\n", status);
    memset(buffer + 16, 0xff, 8);
    ok(!memcmp(buffer, buffer + 16, 16), "unexpected report value\n");


    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status);

    SetLastError(0xdeadbeef);
    ret = HidD_GetInputReport(file, report, 0);
    ok(!ret, "HidD_GetInputReport succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetInputReport returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = HidD_GetInputReport(file, report, caps.InputReportByteLength - 1);
    ok(!ret, "HidD_GetInputReport succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
       "HidD_GetInputReport returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memset(buffer, 0x5a, sizeof(buffer));
    ret = HidD_GetInputReport(file, buffer, caps.InputReportByteLength);
    if (report_id || broken(!ret) /* w7u */)
    {
        ok(!ret, "HidD_GetInputReport succeeded, last error %u\n", GetLastError());
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
           "HidD_GetInputReport returned error %u\n", GetLastError());
    }
    else
    {
        ok(ret, "HidD_GetInputReport failed, last error %u\n", GetLastError());
        ok(buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0]);
    }

    SetLastError(0xdeadbeef);
    ret = HidD_GetInputReport(file, report, caps.InputReportByteLength);
    ok(ret, "HidD_GetInputReport failed, last error %u\n", GetLastError());
    ok(report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id);

    SetLastError(0xdeadbeef);
    value = caps.InputReportByteLength * 2;
    ret = sync_ioctl(file, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, report, &value);
    ok(ret, "IOCTL_HID_GET_INPUT_REPORT failed, last error %u\n", GetLastError());
    ok(value == 3, "got length %u, expected 3\n", value);
    ok(report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id);


    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Feature, report_id, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status);

    SetLastError(0xdeadbeef);
    ret = HidD_GetFeature(file, report, 0);
    ok(!ret, "HidD_GetFeature succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetFeature returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = HidD_GetFeature(file, report, caps.FeatureReportByteLength - 1);
    ok(!ret, "HidD_GetFeature succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
       "HidD_GetFeature returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memset(buffer, 0x5a, sizeof(buffer));
    ret = HidD_GetFeature(file, buffer, caps.FeatureReportByteLength);
    if (report_id || broken(!ret))
    {
        ok(!ret, "HidD_GetFeature succeeded, last error %u\n", GetLastError());
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
           "HidD_GetFeature returned error %u\n", GetLastError());
    }
    else
    {
        ok(ret, "HidD_GetFeature failed, last error %u\n", GetLastError());
        ok(buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0]);
    }

    SetLastError(0xdeadbeef);
    ret = HidD_GetFeature(file, report, caps.FeatureReportByteLength);
    ok(ret, "HidD_GetFeature failed, last error %u\n", GetLastError());
    ok(report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id);

    value = caps.FeatureReportByteLength * 2;
    SetLastError(0xdeadbeef);
    ret = sync_ioctl(file, IOCTL_HID_GET_FEATURE, NULL, 0, report, &value);
    ok(ret, "IOCTL_HID_GET_FEATURE failed, last error %u\n", GetLastError());
    ok(value == 3, "got length %u, expected 3\n", value);
    ok(report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id);


    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Feature, report_id, preparsed_data, report, caps.FeatureReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status);

    SetLastError(0xdeadbeef);
    ret = HidD_SetFeature(file, report, 0);
    ok(!ret, "HidD_SetFeature succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetFeature returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = HidD_SetFeature(file, report, caps.FeatureReportByteLength - 1);
    ok(!ret, "HidD_SetFeature succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
       "HidD_SetFeature returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memset(buffer, 0x5a, sizeof(buffer));
    ret = HidD_SetFeature(file, buffer, caps.FeatureReportByteLength);
    if (report_id || broken(!ret))
    {
        ok(!ret, "HidD_SetFeature succeeded, last error %u\n", GetLastError());
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
           "HidD_SetFeature returned error %u\n", GetLastError());
    }
    else
    {
        ok(ret, "HidD_SetFeature failed, last error %u\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = HidD_SetFeature(file, report, caps.FeatureReportByteLength);
    ok(ret, "HidD_SetFeature failed, last error %u\n", GetLastError());

    value = caps.FeatureReportByteLength * 2;
    SetLastError(0xdeadbeef);
    ret = sync_ioctl(file, IOCTL_HID_SET_FEATURE, NULL, 0, report, &value);
    ok(!ret, "IOCTL_HID_SET_FEATURE succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "IOCTL_HID_SET_FEATURE returned error %u\n", GetLastError());
    value = 0;
    SetLastError(0xdeadbeef);
    ret = sync_ioctl(file, IOCTL_HID_SET_FEATURE, report, caps.FeatureReportByteLength * 2, NULL, &value);
    ok(ret, "IOCTL_HID_SET_FEATURE failed, last error %u\n", GetLastError());
    ok(value == 3, "got length %u, expected 3\n", value);


    memset(report, 0xcd, sizeof(report));
    status = HidP_InitializeReportForID(HidP_Output, report_id, preparsed_data, report, caps.OutputReportByteLength);
    ok(status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status);
    memset(report, 0, caps.OutputReportByteLength);
    report[0] = report_id;

    SetLastError(0xdeadbeef);
    ret = HidD_SetOutputReport(file, report, 0);
    ok(!ret, "HidD_SetOutputReport succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetOutputReport returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = HidD_SetOutputReport(file, report, caps.OutputReportByteLength - 1);
    ok(!ret, "HidD_SetOutputReport succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
       "HidD_SetOutputReport returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memset(buffer, 0x5a, sizeof(buffer));
    ret = HidD_SetOutputReport(file, buffer, caps.OutputReportByteLength);
    if (report_id || broken(!ret))
    {
        ok(!ret, "HidD_SetOutputReport succeeded, last error %u\n", GetLastError());
        ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_CRC),
           "HidD_SetOutputReport returned error %u\n", GetLastError());
    }
    else
    {
        ok(ret, "HidD_SetOutputReport failed, last error %u\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = HidD_SetOutputReport(file, report, caps.OutputReportByteLength);
    ok(ret, "HidD_SetOutputReport failed, last error %u\n", GetLastError());

    value = caps.OutputReportByteLength * 2;
    SetLastError(0xdeadbeef);
    ret = sync_ioctl(file, IOCTL_HID_SET_OUTPUT_REPORT, NULL, 0, report, &value);
    ok(!ret, "IOCTL_HID_SET_OUTPUT_REPORT succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "IOCTL_HID_SET_OUTPUT_REPORT returned error %u\n", GetLastError());
    value = 0;
    SetLastError(0xdeadbeef);
    ret = sync_ioctl(file, IOCTL_HID_SET_OUTPUT_REPORT, report, caps.OutputReportByteLength * 2, NULL, &value);
    ok(ret, "IOCTL_HID_SET_OUTPUT_REPORT failed, last error %u\n", GetLastError());
    ok(value == 3, "got length %u, expected 3\n", value);


    SetLastError(0xdeadbeef);
    ret = WriteFile(file, report, 0, &value, NULL);
    ok(!ret, "WriteFile succeeded\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "WriteFile returned error %u\n", GetLastError());
    ok(value == 0, "WriteFile returned %x\n", value);
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, report, caps.OutputReportByteLength - 1, &value, NULL);
    ok(!ret, "WriteFile succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_USER_BUFFER,
       "WriteFile returned error %u\n", GetLastError());
    ok(value == 0, "WriteFile returned %x\n", value);

    memset(report, 0xcd, sizeof(report));
    report[0] = 0xa5;
    SetLastError(0xdeadbeef);
    ret = WriteFile(file, report, caps.OutputReportByteLength * 2, &value, NULL);
    if (report_id || broken(!ret) /* w7u */)
    {
        ok(!ret, "WriteFile succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "WriteFile returned error %u\n", GetLastError());
        ok(value == 0, "WriteFile wrote %u\n", value);
        SetLastError(0xdeadbeef);
        report[0] = report_id;
        ret = WriteFile(file, report, caps.OutputReportByteLength, &value, NULL);
    }

    if (report_id)
    {
        ok(ret, "WriteFile failed, last error %u\n", GetLastError());
        ok(value == 2, "WriteFile wrote %u\n", value);
    }
    else
    {
        ok(ret, "WriteFile failed, last error %u\n", GetLastError());
        ok(value == 3, "WriteFile wrote %u\n", value);
    }


    memset(report, 0xcd, sizeof(report));
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, report, 0, &value, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %u\n", GetLastError());
    ok(value == 0, "ReadFile returned %x\n", value);
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, report, caps.InputReportByteLength - 1, &value, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %u\n", GetLastError());
    ok(value == 0, "ReadFile returned %x\n", value);

    if (polled)
    {
        memset(report, 0xcd, sizeof(report));
        SetLastError(0xdeadbeef);
        ret = ReadFile(file, report, caps.InputReportByteLength, &value, NULL);
        ok(ret, "ReadFile failed, last error %u\n", GetLastError());
        ok(value == (report_id ? 3 : 4), "ReadFile returned %x\n", value);
        ok(report[0] == report_id, "unexpected report data\n");

        overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
        overlapped2.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

        /* drain available input reports */
        SetLastError(0xdeadbeef);
        while (ReadFile(async_file, report, caps.InputReportByteLength, NULL, &overlapped))
            ResetEvent(overlapped.hEvent);
        ok(GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError());
        ret = GetOverlappedResult(async_file, &overlapped, &value, TRUE);
        ok(ret, "GetOverlappedResult failed, last error %u\n", GetLastError());
        ok(value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value);
        ResetEvent(overlapped.hEvent);

        memcpy(buffer, report, caps.InputReportByteLength);
        memcpy(buffer + caps.InputReportByteLength, report, caps.InputReportByteLength);

        SetLastError(0xdeadbeef);
        ret = ReadFile(async_file, report, caps.InputReportByteLength, NULL, &overlapped);
        ok(!ret, "ReadFile succeded\n");
        ok(GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = ReadFile(async_file, buffer, caps.InputReportByteLength, NULL, &overlapped2);
        ok(!ret, "ReadFile succeded\n");
        ok(GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError());

        /* wait for second report to be ready */
        ret = GetOverlappedResult(async_file, &overlapped2, &value, TRUE);
        ok(ret, "GetOverlappedResult failed, last error %u\n", GetLastError());
        ok(value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value);
        /* first report should be ready and the same */
        ret = GetOverlappedResult(async_file, &overlapped, &value, FALSE);
        ok(ret, "GetOverlappedResult failed, last error %u\n", GetLastError());
        ok(value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value);
        ok(memcmp(report, buffer + caps.InputReportByteLength, caps.InputReportByteLength),
           "expected different report\n");
        ok(!memcmp(report, buffer, caps.InputReportByteLength), "expected identical reports\n");

        CloseHandle(overlapped.hEvent);
        CloseHandle(overlapped2.hEvent);
    }


    HidD_FreePreparsedData(preparsed_data);
}

static void test_hid_device(DWORD report_id, DWORD polled)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *iface_detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    ULONG count, poll_freq, out_len;
    HANDLE file, async_file;
    BOOL ret, found = FALSE;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;

    winetest_push_context("id %d%s", report_id, polled ? " poll" : "");

    set = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError());

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiEnumDeviceInterfaces(set, &device, &GUID_DEVINTERFACE_HID, 0, &iface);
        ok(ret, "failed to get interface, error %#x\n", GetLastError());
        ok(IsEqualGUID(&iface.InterfaceClassGuid, &GUID_DEVINTERFACE_HID),
                "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
        ok(iface.Flags == SPINT_ACTIVE, "got flags %#x\n", iface.Flags);

        iface_detail->cbSize = sizeof(*iface_detail);
        ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, iface_detail, sizeof(buffer), NULL, NULL);
        ok(ret, "failed to get interface path, error %#x\n", GetLastError());

        if (strstr(iface_detail->DevicePath, "\\\\?\\hid#winetest#1"))
        {
            found = TRUE;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(set);

    todo_wine ok(found, "didn't find device\n");

    file = CreateFileA(iface_detail->DevicePath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = HidD_GetNumInputBuffers(file, &count);
    ok(ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError());
    ok(count == 32, "HidD_GetNumInputBuffers returned %u\n", count);

    SetLastError(0xdeadbeef);
    ret = HidD_SetNumInputBuffers(file, 1);
    ok(!ret, "HidD_SetNumInputBuffers succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %u\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = HidD_SetNumInputBuffers(file, 513);
    ok(!ret, "HidD_SetNumInputBuffers succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = HidD_SetNumInputBuffers(file, 16);
    ok(ret, "HidD_SetNumInputBuffers failed last error %u\n", GetLastError());

    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = HidD_GetNumInputBuffers(file, &count);
    ok(ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError());
    ok(count == 16, "HidD_GetNumInputBuffers returned %u\n", count);

    async_file = CreateFileA(iface_detail->DevicePath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
    ok(async_file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = HidD_GetNumInputBuffers(async_file, &count);
    ok(ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError());
    ok(count == 32, "HidD_GetNumInputBuffers returned %u\n", count);

    SetLastError(0xdeadbeef);
    ret = HidD_SetNumInputBuffers(async_file, 2);
    ok(ret, "HidD_SetNumInputBuffers failed last error %u\n", GetLastError());

    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = HidD_GetNumInputBuffers(async_file, &count);
    ok(ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError());
    ok(count == 2, "HidD_GetNumInputBuffers returned %u\n", count);
    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = HidD_GetNumInputBuffers(file, &count);
    ok(ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError());
    ok(count == 16, "HidD_GetNumInputBuffers returned %u\n", count);

    if (polled)
    {
        out_len = sizeof(ULONG);
        SetLastError(0xdeadbeef);
        ret = sync_ioctl(file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len);
        ok(ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError());
        ok(out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len);
        todo_wine ok(poll_freq == 5, "got poll_freq %u, expected 5\n", poll_freq);

        out_len = 0;
        poll_freq = 500;
        SetLastError(0xdeadbeef);
        ret = sync_ioctl(file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len);
        ok(ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError());
        ok(out_len == 0, "got out_len %u, expected 0\n", out_len);

        out_len = sizeof(ULONG);
        SetLastError(0xdeadbeef);
        ret = sync_ioctl(file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len);
        ok(ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError());
        ok(out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len);
        ok(poll_freq == 500, "got poll_freq %u, expected 100\n", poll_freq);

        out_len = sizeof(ULONG);
        SetLastError(0xdeadbeef);
        ret = sync_ioctl(async_file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len);
        ok(ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError());
        ok(out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len);
        ok(poll_freq == 500, "got poll_freq %u, expected 100\n", poll_freq);
    }

    test_hidp(file, async_file, report_id, polled);

    CloseHandle(async_file);
    CloseHandle(file);

    RtlInitUnicodeString(&string, L"\\??\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenFile(&file, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    todo_wine ok(status == STATUS_UNSUCCESSFUL, "got %#x\n", status);

    winetest_pop_context();
}

static void test_hid_driver(struct testsign_context *ctx, DWORD report_id, DWORD polled)
{
    static const char hardware_id[] = "test_hardware_id\0";
    char path[MAX_PATH], dest[MAX_PATH], *filepart;
    SP_DEVINFO_DATA device = {sizeof(device)};
    char cwd[MAX_PATH], tempdir[MAX_PATH];
    WCHAR driver_filename[MAX_PATH];
    SC_HANDLE manager, service;
    BOOL ret, need_reboot;
    HANDLE catalog, file;
    LSTATUS status;
    HDEVINFO set;
    HKEY hkey;
    FILE *f;

    GetCurrentDirectoryA(ARRAY_SIZE(cwd), cwd);
    GetTempPathA(ARRAY_SIZE(tempdir), tempdir);
    SetCurrentDirectoryA(tempdir);

    status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\winetest", 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    ok(!status, "RegCreateKeyExW returned %#x\n", status);

    status = RegSetValueExW(hkey, L"ReportID", 0, REG_DWORD, (void *)&report_id, sizeof(report_id));
    ok(!status, "RegSetValueExW returned %#x\n", status);

    status = RegSetValueExW(hkey, L"PolledMode", 0, REG_DWORD, (void *)&polled, sizeof(polled));
    ok(!status, "RegSetValueExW returned %#x\n", status);

    load_resource(L"driver_hid.dll", driver_filename);
    ret = MoveFileExW(driver_filename, L"winetest.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    ok(ret, "failed to move file, error %u\n", GetLastError());

    f = fopen("winetest.inf", "w");
    ok(!!f, "failed to open winetest.inf: %s\n", strerror(errno));
    fputs(inf_text, f);
    fclose(f);

    /* Create the catalog file. */

    catalog = CryptCATOpen((WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#x\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest.sys");
    add_file_to_catalog(catalog, L"winetest.inf");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %u\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %u\n", GetLastError());

    testsign_sign(ctx, L"winetest.cat");

    /* Install the driver. */

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "failed to create device list, error %#x\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "root\\winetest\\0", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "failed to create device, error %#x\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA( set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id) );
    ok(ret, "failed to create set hardware ID, error %#x\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "failed to register device, error %#x\n", GetLastError());

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = UpdateDriverForPlugAndPlayDevicesA(NULL, hardware_id, path, INSTALLFLAG_FORCE, &need_reboot);
    ok(ret, "failed to install device, error %#x\n", GetLastError());
    ok(!need_reboot, "expected no reboot necessary\n");

    /* Tests. */

    test_hid_device(report_id, polled);

    /* Clean up. */

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "failed to remove device, error %#x\n", GetLastError());

    file = CreateFileA("\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file == INVALID_HANDLE_VALUE, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %u\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "failed to destroy set, error %#x\n", GetLastError());

    /* Windows stops the service but does not delete it. */
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(!!manager, "failed to open service manager, error %u\n", GetLastError());
    service = OpenServiceA(manager, "winetest", SERVICE_STOP | DELETE);
    ok(!!service, "failed to open service, error %u\n", GetLastError());
    unload_driver(service);
    CloseServiceHandle(manager);

    cat_okfile();

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = SetupCopyOEMInfA(path, NULL, 0, 0, dest, sizeof(dest), NULL, &filepart);
    ok(ret, "Failed to copy INF, error %#x\n", GetLastError());
    ret = SetupUninstallOEMInfA(filepart, 0, NULL);
    ok(ret, "Failed to uninstall INF, error %u\n", GetLastError());

    ret = DeleteFileA("winetest.cat");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    ret = DeleteFileA("winetest.inf");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    ret = DeleteFileA("winetest.sys");
    ok(ret, "Failed to delete file, error %u\n", GetLastError());
    /* Windows 10 apparently deletes the image in SetupUninstallOEMInf(). */
    ret = DeleteFileA("C:/windows/system32/drivers/winetest.sys");
    ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %u\n", GetLastError());

    SetCurrentDirectoryA(cwd);
}

START_TEST(ntoskrnl)
{
    WCHAR filename[MAX_PATH], filename2[MAX_PATH];
    struct testsign_context ctx;
    SC_HANDLE service, service2;
    BOOL ret, is_wow64;
    HANDLE mapping;
    DWORD written;

    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(GetModuleHandleA("ntdll"), "RtlDosPathNameToNtPathName_U");
    pRtlFreeUnicodeString = (void *)GetProcAddress(GetModuleHandleA("ntdll"), "RtlFreeUnicodeString");
    pCancelIoEx = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CancelIoEx");
    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    pSetFileCompletionNotificationModes = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                                 "SetFileCompletionNotificationModes");
    pSignerSign = (void *)GetProcAddress(LoadLibraryA("mssign32"), "SignerSign");

    if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
    {
        skip("Running in WoW64.\n");
        return;
    }

    if (!testsign_create_cert(&ctx))
        return;

    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, sizeof(*test_data), "Global\\winetest_ntoskrnl_section");
    ok(!!mapping, "got error %u\n", GetLastError());
    test_data = MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024);
    test_data->running_under_wine = !strcmp(winetest_platform, "wine");
    test_data->winetest_report_success = winetest_report_success;
    test_data->winetest_debug = winetest_debug;

    okfile = CreateFileA("C:\\windows\\winetest_ntoskrnl_okfile", GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    ok(okfile != INVALID_HANDLE_VALUE, "failed to create file, error %u\n", GetLastError());

    subtest("driver");
    if (!(service = load_driver(&ctx, filename, L"driver.dll", L"WineTestDriver")))
        goto out;

    if (!start_driver(service, FALSE))
    {
        DeleteFileW(filename);
        goto out;
    }
    service2 = load_driver(&ctx, filename2, L"driver2.dll", L"WineTestDriver2");

    device = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    test_basic_ioctl();
    test_mismatched_status_ioctl();

    main_test();
    todo_wine ok(modified_value == 0xdeadbeeffeedcafe, "Got unexpected value %#I64x.\n", modified_value);

    test_overlapped();
    test_load_driver(service2);
    test_file_handles();
    test_return_status();
    test_object_info();

    /* We need a separate ioctl to call IoDetachDevice(); calling it in the
     * driver unload routine causes a live-lock. */
    ret = DeviceIoControl(device, IOCTL_WINETEST_DETACH, NULL, 0, NULL, 0, &written, NULL);
    ok(ret, "DeviceIoControl failed: %u\n", GetLastError());

    CloseHandle(device);

    unload_driver(service2);
    unload_driver(service);
    ret = DeleteFileW(filename);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());
    ret = DeleteFileW(filename2);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());

    cat_okfile();

    test_driver3(&ctx);
    subtest("driver_netio");
    test_driver_netio(&ctx);

    subtest("driver_pnp");
    test_pnp_driver(&ctx);

    subtest("driver_hid");
    test_hid_driver(&ctx, 0, FALSE);
    test_hid_driver(&ctx, 1, FALSE);
    test_hid_driver(&ctx, 0, TRUE);
    test_hid_driver(&ctx, 1, TRUE);

out:
    testsign_cleanup(&ctx);
    UnmapViewOfFile(test_data);
    CloseHandle(mapping);
    CloseHandle(okfile);
    DeleteFileA("C:\\windows\\winetest_ntoskrnl_okfile");
}
