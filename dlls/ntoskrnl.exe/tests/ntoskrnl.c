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
#include "regstr.h"
#include "dbt.h"
#include "initguid.h"
#include "devguid.h"
#include "devpkey.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"
#include "ddk/hidpi.h"
#include "wine/test.h"
#include "wine/mssign.h"

#include "driver.h"

static const GUID GUID_NULL;

static HANDLE device;

static struct test_data *test_data;

static BOOL (WINAPI *pCancelIoEx)(HANDLE, OVERLAPPED *);
static BOOL (WINAPI *pSetFileCompletionNotificationModes)(HANDLE, UCHAR);

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
    ok(file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", debugstr_w(filename), GetLastError());

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
    ok(ret, "Failed to create container, error %#lx\n", GetLastError());

    ret = CryptGenKey(ctx->provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key);
    ok(ret, "Failed to create key, error %#lx\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#lx\n", GetLastError());
    ret = CryptGetUserKey(ctx->provider, AT_SIGNATURE, &key);
    ok(ret, "Failed to get user key, error %#lx\n", GetLastError());
    ret = CryptDestroyKey(key);
    ok(ret, "Failed to destroy key, error %#lx\n", GetLastError());

    size = sizeof(encoded_name);
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=winetest_cert", CERT_X500_NAME_STR, NULL, encoded_name, &size, NULL);
    ok(ret, "Failed to convert name, error %#lx\n", GetLastError());
    key_info.CertIssuer.cbData = size;
    key_info.CertIssuer.pbData = encoded_name;

    size = sizeof(public_key_info_buffer);
    ret = CryptExportPublicKeyInfo(ctx->provider, AT_SIGNATURE, X509_ASN_ENCODING, public_key_info, &size);
    ok(ret, "Failed to export public key, error %#lx\n", GetLastError());
    cert_info.SubjectPublicKeyInfo = *public_key_info;

    size = sizeof(hash_buffer);
    ret = CryptHashPublicKeyInfo(ctx->provider, CALG_MD5, 0, X509_ASN_ENCODING, public_key_info, hash_buffer, &size);
    ok(ret, "Failed to hash public key, error %#lx\n", GetLastError());

    key_info.KeyId.cbData = size;
    key_info.KeyId.pbData = hash_buffer;

    RtlGenRandom(serial, sizeof(serial));
    key_info.CertSerialNumber.cbData = sizeof(serial);
    key_info.CertSerialNumber.pbData = serial;

    size = sizeof(encoded_key_id);
    ret = CryptEncodeObject(X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID, &key_info, encoded_key_id, &size);
    ok(ret, "Failed to convert name, error %#lx\n", GetLastError());

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
    ok(ret, "Failed to create certificate, error %#lx\n", GetLastError());

    ctx->cert = CertCreateCertificateContext(X509_ASN_ENCODING, cert_buffer, size);
    ok(!!ctx->cert, "Failed to create context, error %#lx\n", GetLastError());

    size = sizeof(provider_nameA);
    ret = CryptGetProvParam(ctx->provider, PP_NAME, provider_nameA, &size, 0);
    ok(ret, "Failed to get prov param, error %#lx\n", GetLastError());
    MultiByteToWideChar(CP_ACP, 0, (char *)provider_nameA, -1, provider_nameW, ARRAY_SIZE(provider_nameW));

    provider_info.pwszContainerName = (WCHAR *)container_name;
    provider_info.pwszProvName = provider_nameW;
    provider_info.dwProvType = PROV_RSA_FULL;
    provider_info.dwKeySpec = AT_SIGNATURE;
    ret = CertSetCertificateContextProperty(ctx->cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &provider_info);
    ok(ret, "Failed to set provider info, error %#lx\n", GetLastError());

    ctx->root_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, "root");
    if (!ctx->root_store && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to open root store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %lu\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %lu\n", GetLastError());

        return FALSE;
    }
    ok(!!ctx->root_store, "Failed to open store, error %lu\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->root_store, ctx->cert, CERT_STORE_ADD_ALWAYS, &ctx->root_cert);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to add self-signed certificate to store.\n");

        ret = CertFreeCertificateContext(ctx->cert);
        ok(ret, "Failed to free certificate, error %lu\n", GetLastError());
        ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
        ok(ret, "Failed to close store, error %lu\n", GetLastError());
        ret = CryptReleaseContext(ctx->provider, 0);
        ok(ret, "failed to release context, error %lu\n", GetLastError());

        return FALSE;
    }
    ok(ret, "Failed to add certificate, error %lu\n", GetLastError());

    ctx->publisher_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_A, 0, 0,
            CERT_SYSTEM_STORE_LOCAL_MACHINE, "trustedpublisher");
    ok(!!ctx->publisher_store, "Failed to open store, error %lu\n", GetLastError());
    ret = CertAddCertificateContextToStore(ctx->publisher_store, ctx->cert,
            CERT_STORE_ADD_ALWAYS, &ctx->publisher_cert);
    ok(ret, "Failed to add certificate, error %lu\n", GetLastError());

    return TRUE;
}

static void testsign_cleanup(struct testsign_context *ctx)
{
    BOOL ret;

    ret = CertFreeCertificateContext(ctx->cert);
    ok(ret, "Failed to free certificate, error %lu\n", GetLastError());

    ret = CertFreeCertificateContext(ctx->root_cert);
    ok(ret, "Failed to free certificate context, error %lu\n", GetLastError());
    ret = CertCloseStore(ctx->root_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %lu\n", GetLastError());

    ret = CertFreeCertificateContext(ctx->publisher_cert);
    ok(ret, "Failed to free certificate context, error %lu\n", GetLastError());
    ret = CertCloseStore(ctx->publisher_store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(ret, "Failed to close store, error %lu\n", GetLastError());

    ret = CryptReleaseContext(ctx->provider, 0);
    ok(ret, "failed to release context, error %lu\n", GetLastError());
}

static void testsign_sign(struct testsign_context *ctx, const WCHAR *filename)
{
    static HRESULT (WINAPI *pSignerSign)(SIGNER_SUBJECT_INFO *subject, SIGNER_CERT *cert,
            SIGNER_SIGNATURE_INFO *signature, SIGNER_PROVIDER_INFO *provider,
            const WCHAR *timestamp, CRYPT_ATTRIBUTES *attr, void *sip_data);

    SIGNER_ATTR_AUTHCODE authcode = {sizeof(authcode)};
    SIGNER_SIGNATURE_INFO signature = {sizeof(signature)};
    SIGNER_SUBJECT_INFO subject = {sizeof(subject)};
    SIGNER_CERT_STORE_INFO store = {sizeof(store)};
    SIGNER_CERT cert_info = {sizeof(cert_info)};
    SIGNER_FILE_INFO file = {sizeof(file)};
    DWORD index = 0;
    HRESULT hr;

    if (!pSignerSign)
        pSignerSign = (void *)GetProcAddress(LoadLibraryA("mssign32"), "SignerSign");

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
    todo_wine ok(hr == S_OK || broken(hr == NTE_BAD_ALGID) /* < 7 */, "Failed to sign, hr %#lx\n", hr);
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
        ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_STOPPED,
       "expected SERVICE_STOPPED, got %ld\n", status.dwCurrentState);

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
    ok(!!service, "CreateService failed: %lu\n", GetLastError());

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
    ok(ret, "StartService failed: %lu\n", GetLastError());

    /* wait for the service to start up properly */
    ret = QueryServiceStatus(service, &status);
    ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
    while (status.dwCurrentState == SERVICE_START_PENDING)
    {
        Sleep(100);
        ret = QueryServiceStatus(service, &status);
        ok(ret, "QueryServiceStatus failed: %lu\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_RUNNING,
       "expected SERVICE_RUNNING, got %ld\n", status.dwCurrentState);
    ok(status.dwServiceType == SERVICE_KERNEL_DRIVER,
       "expected SERVICE_KERNEL_DRIVER, got %#lx\n", status.dwServiceType);

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
        printf("%.*s", (int)size, buffer);
    } while (size == sizeof(buffer));

    SetFilePointer(okfile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(okfile);

    InterlockedAdd(&winetest_successes, InterlockedExchange(&test_data->successes, 0));
    winetest_add_failures(InterlockedExchange(&test_data->failures, 0));
    InterlockedAdd(&winetest_todo_successes, InterlockedExchange(&test_data->todo_successes, 0));
    winetest_add_failures(InterlockedExchange(&test_data->todo_failures, 0));
    InterlockedAdd(&winetest_skipped, InterlockedExchange(&test_data->skipped, 0));
}

static ULONG64 modified_value;

static void main_test(void)
{
    struct main_test_input test_input;
    DWORD size;
    BOOL res;

    test_input.process_id = GetCurrentProcessId();
    test_input.teststr_offset = (SIZE_T)((BYTE *)&teststr - (BYTE *)NtCurrentTeb()->Peb->ImageBaseAddress);
    test_input.modified_value = &modified_value;
    modified_value = 0;

    res = DeviceIoControl(device, IOCTL_WINETEST_MAIN_TEST, &test_input, sizeof(test_input), NULL, 0, &size, NULL);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    ok(!size, "got size %lu\n", size);
}

static void test_basic_ioctl(void)
{
    char inbuf[64], buf[32];
    DWORD written;
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    ok(written == sizeof(teststr), "got size %ld\n", written);
    ok(!strcmp(buf, teststr), "got '%s'\n", buf);

    memset(buf, 0, sizeof(buf));
    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, inbuf,
            sizeof(inbuf), buf, 10, &written, NULL);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    ok(written == 10, "got size %ld\n", written);
    ok(!strcmp(buf, "Wine is no"), "got '%s'\n", buf);
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
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    /* test cancelling all device requests */
    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %lu\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %lu\n", GetLastError());

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    ok(cancel_cnt == 0, "cancel_cnt = %lu\n", cancel_cnt);

    CancelIo(file);

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    ok(cancel_cnt == 2, "cancel_cnt = %lu\n", cancel_cnt);

    /* test cancelling selected overlapped event */
    if (pCancelIoEx)
    {
        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %lu\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %lu\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %lu\n", GetLastError());

        pCancelIoEx(file, &overlapped);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
        ok(cancel_cnt == 1, "cancel_cnt = %lu\n", cancel_cnt);

        pCancelIoEx(file, &overlapped2);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
        ok(cancel_cnt == 2, "cancel_cnt = %lu\n", cancel_cnt);
    }

    port = CreateIoCompletionPort(file, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %lu\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%lu)\n", res, GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(res, "GetQueuedCompletionStatus failed: %lu\n", GetLastError());
    ok(o == &overlapped, "o != overlapped\n");

    if (pSetFileCompletionNotificationModes)
    {
        res = pSetFileCompletionNotificationModes(file, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        ok(res, "SetFileCompletionNotificationModes failed: %lu\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %lu\n", GetLastError());
        res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
        ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%lu)\n", res, GetLastError());
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
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#lx\n", status.dwCurrentState);

    load = TRUE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_RUNNING, "got state %#lx\n", status.dwCurrentState);

    load = FALSE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %lu\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %lu\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#lx\n", status.dwCurrentState);
}

static void test_file_handles(void)
{
    DWORD count, ret_size;
    HANDLE file, dup, file2;
    BOOL ret;

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 2, "got %lu\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);

    file = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 3, "got %lu\n", count);

    file2 = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file2 != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 4, "got %lu\n", count);

    ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "failed to duplicate handle: %lu\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 4, "got %lu\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);

    ret = DeviceIoControl(file, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 3, "got %lu\n", count);

    ret = DeviceIoControl(file2, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 4, "got %lu\n", count);

    ret = DeviceIoControl(dup, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 3, "got %lu\n", count);

    CloseHandle(dup);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 1, "got %lu\n", count);

    CloseHandle(file2);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 2, "got %lu\n", count);

    CloseHandle(file);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %lu\n", GetLastError());
    ok(count == 3, "got %lu\n", count);
}

static unsigned int got_return_status_apc;

static void WINAPI return_status_apc(void *apc_user, IO_STATUS_BLOCK *io, ULONG reserved)
{
    ++got_return_status_apc;
    ok(apc_user == (void *)456, "got %p\n", apc_user);
    ok(!reserved, "got reserved %#lx\n", reserved);
}

static void do_return_status(ULONG ioctl, struct return_status_params *params)
{
    const char *expect_buffer;
    LARGE_INTEGER zero = {{0}};
    HANDLE file, port, event;
    NTSTATUS expect_status;
    ULONG_PTR key, value;
    IO_STATUS_BLOCK io;
    char buffer[7];
    DWORD size;
    BOOL ret;

    if (params->ret_status == STATUS_PENDING && !params->pending)
    {
        /* this causes kernel hangs under certain conditions */
        return;
    }

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    if (ioctl != IOCTL_WINETEST_RETURN_STATUS_BUFFERED)
        expect_buffer = "ghijkl";
    else if (NT_ERROR(params->iosb_status))
        expect_buffer = "abcdef";
    else
        expect_buffer = "ghidef";

    /* Test the non-overlapped case. */

    expect_status = (params->ret_status == STATUS_PENDING ? params->iosb_status : params->ret_status);

    strcpy(buffer, "abcdef");
    size = 0xdeadf00d;
    SetLastError(0xdeadf00d);
    ret = DeviceIoControl(device, ioctl, params, sizeof(*params), buffer, sizeof(buffer), &size, NULL);
    todo_wine_if (params->ret_status == STATUS_PENDING && params->iosb_status == STATUS_PENDING)
        ok(ret == NT_SUCCESS(expect_status), "got %d\n", ret);
    if (NT_SUCCESS(expect_status))
    {
        todo_wine_if (params->ret_status == STATUS_PENDING && params->iosb_status == STATUS_PENDING)
            ok(GetLastError() == 0xdeadf00d, "got error %lu\n", GetLastError());
    }
    else
    {
        ok(GetLastError() == RtlNtStatusToDosErrorNoTeb(expect_status), "got error %lu\n", GetLastError());
    }
    if (NT_ERROR(expect_status))
        ok(size == 0xdeadf00d, "got size %lu\n", size);
    else if (!NT_ERROR(params->iosb_status))
        ok(size == 3, "got size %lu\n", size);
    /* size is garbage if !NT_ERROR(expect_status) && NT_ERROR(iosb_status) */
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    strcpy(buffer, "abcdef");
    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    ret = NtDeviceIoControlFile(device, NULL, NULL, NULL, &io,
            ioctl, params, sizeof(*params), buffer, sizeof(buffer));
    ok(ret == expect_status, "got %#x\n", ret);
    if (NT_ERROR(params->iosb_status))
    {
        ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
        ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
    }
    else
    {
        ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
        ok(io.Information == 3, "got size %Iu\n", io.Information);
    }
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    /* Test the overlapped case. */

    file = CreateFileA("\\\\.\\WineTestDriver", FILE_ALL_ACCESS,
            0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device, error %lu\n", GetLastError());
    port = CreateIoCompletionPort(file, NULL, 123, 0);
    ok(port != NULL, "failed to create port, error %lu\n", GetLastError());

    ret = WaitForSingleObject(file, 0);
    ok(!ret, "got %d\n", ret);

    ResetEvent(event);
    strcpy(buffer, "abcdef");
    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    ret = NtDeviceIoControlFile(file, event, NULL, (void *)456, &io,
            ioctl, params, sizeof(*params), buffer, sizeof(buffer));
    ok(ret == params->ret_status
            || broken(NT_WARNING(params->ret_status) && ret == STATUS_PENDING), /* win10 */
            "got %#x\n", ret);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
        ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(event, 0);
        ok(ret == WAIT_TIMEOUT, "got %d\n", ret);
    }
    else
    {
        ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
        ok(io.Information == 3, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(event, 0);
        ok(!ret, "got %d\n", ret);
    }
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    ret = WaitForSingleObject(file, 0);
    ok(ret == WAIT_TIMEOUT, "got %d\n", ret);

    key = 0xdeadf00d;
    value = 0xdeadf00d;
    memset(&io, 0xcc, sizeof(io));
    ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(ret == STATUS_TIMEOUT, "got %#x\n", ret);
    }
    else
    {
        ok(!ret, "got %#x\n", ret);
        ok(key == 123, "got key %Iu\n", key);
        ok(value == 456, "got value %Iu\n", value);
        ok(io.Status == params->iosb_status, "got iosb status %#lx\n", io.Status);
        ok(io.Information == 3, "got information %Iu\n", io.Information);
    }

    /* As above, but set the event first, to show that the event is always
     * reset. */
    ResetEvent(event);
    strcpy(buffer, "abcdef");
    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    ret = NtDeviceIoControlFile(file, event, NULL, NULL, &io,
            ioctl, params, sizeof(*params), buffer, sizeof(buffer));
    ok(ret == params->ret_status
            || broken(NT_WARNING(params->ret_status) && ret == STATUS_PENDING), /* win10 */
            "got %#x\n", ret);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
        ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(event, 0);
        ok(ret == WAIT_TIMEOUT, "got %d\n", ret);
    }
    else
    {
        ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
        ok(io.Information == 3, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(event, 0);
        ok(!ret, "got %d\n", ret);
    }
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    /* As above, but use the file handle instead of an event. */
    ret = WaitForSingleObject(file, 0);
    ok(ret == WAIT_TIMEOUT, "got %d\n", ret);

    strcpy(buffer, "abcdef");
    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    ret = NtDeviceIoControlFile(file, NULL, NULL, NULL, &io,
            ioctl, params, sizeof(*params), buffer, sizeof(buffer));
    ok(ret == params->ret_status
            || broken(NT_WARNING(params->ret_status) && ret == STATUS_PENDING), /* win10 */
            "got %#x\n", ret);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
        ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(file, 0);
        ok(ret == WAIT_TIMEOUT, "got %d\n", ret);
    }
    else
    {
        ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
        ok(io.Information == 3, "got size %Iu\n", io.Information);
        ret = WaitForSingleObject(file, 0);
        ok(!ret, "got %d\n", ret);
    }
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    /* Test FILE_SKIP_COMPLETION_PORT_ON_SUCCESS. */

    if (pSetFileCompletionNotificationModes)
    {
        ret = pSetFileCompletionNotificationModes(file, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        ok(ret, "got error %lu\n", GetLastError());

        SetEvent(event);
        strcpy(buffer, "abcdef");
        io.Status = 0xdeadf00d;
        io.Information = 0xdeadf00d;
        ret = NtDeviceIoControlFile(file, event, NULL, (void *)456, &io,
                ioctl, params, sizeof(*params), buffer, sizeof(buffer));
        ok(ret == params->ret_status
                || broken(NT_WARNING(params->ret_status) && ret == STATUS_PENDING), /* win10 */
                "got %#x\n", ret);
        if (!params->pending && NT_ERROR(params->iosb_status))
        {
            ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
            ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
            ret = WaitForSingleObject(event, 0);
            ok(ret == WAIT_TIMEOUT, "got %d\n", ret);
        }
        else
        {
            ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
            ok(io.Information == 3, "got size %Iu\n", io.Information);
            ret = WaitForSingleObject(event, 0);
            ok(!ret, "got %d\n", ret);
        }
        ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

        key = 0xdeadf00d;
        value = 0xdeadf00d;
        memset(&io, 0xcc, sizeof(io));
        ret = NtRemoveIoCompletion(port, &key, &value, &io, &zero);
        if (!params->pending)
        {
            /* Completion is skipped on non-pending NT_ERROR regardless of file
             * options. Windows < 8 interprets
             * FILE_SKIP_COMPLETION_PORT_ON_SUCCESS to mean that !NT_ERROR
             * should also be skipped. Windows >= 8 restricts this to
             * NT_SUCCESS, which has the weird effect that non-pending
             * NT_WARNING does *not* skip completion. It's not clear whether
             * this is a bug or not—it looks like one, but on the other hand it
             * arguably follows the letter of the documentation more closely. */
            ok(ret == STATUS_TIMEOUT || (NT_WARNING(params->iosb_status) && !ret), "got %#x\n", ret);
        }
        else
        {
            ok(!ret, "got %#x\n", ret);
        }
        if (!ret)
        {
            ok(key == 123, "got key %Iu\n", key);
            ok(value == 456, "got value %Iu\n", value);
            ok(io.Status == params->iosb_status, "got iosb status %#lx\n", io.Status);
            ok(io.Information == 3, "got information %Iu\n", io.Information);
        }
    }

    ret = CloseHandle(file);
    ok(ret, "failed to close file, error %lu\n", GetLastError());
    ret = CloseHandle(port);
    ok(ret, "failed to close port, error %lu\n", GetLastError());

    /* Test with an APC. */

    got_return_status_apc = 0;

    file = CreateFileA("\\\\.\\WineTestDriver", FILE_ALL_ACCESS,
            0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device, error %lu\n", GetLastError());

    strcpy(buffer, "abcdef");
    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    ret = NtDeviceIoControlFile(file, NULL, return_status_apc, (void *)456, &io,
            ioctl, params, sizeof(*params), buffer, sizeof(buffer));
    ok(ret == params->ret_status, "got %#x\n", ret);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(io.Status == 0xdeadf00d, "got %#lx\n", io.Status);
        ok(io.Information == 0xdeadf00d, "got size %Iu\n", io.Information);
    }
    else
    {
        ok(io.Status == params->iosb_status, "got %#lx\n", io.Status);
        ok(io.Information == 3, "got size %Iu\n", io.Information);
    }
    ok(!strcmp(buffer, expect_buffer), "got buffer %s\n", buffer);

    ret = SleepEx(0, TRUE);
    if (!params->pending && NT_ERROR(params->iosb_status))
    {
        ok(!ret, "got %d\n", ret);
        ok(!got_return_status_apc, "got %u APC calls\n", got_return_status_apc);
    }
    else
    {
        ok(ret == WAIT_IO_COMPLETION, "got %d\n", ret);
        ok(got_return_status_apc == 1, "got %u APC calls\n", got_return_status_apc);
    }

    ret = CloseHandle(file);
    ok(ret, "failed to close file, error %lu\n", GetLastError());

    CloseHandle(event);
}

static void test_return_status(void)
{
    struct return_status_params params;
    unsigned int i, j, k;

    static const ULONG method_tests[] =
    {
        IOCTL_WINETEST_RETURN_STATUS_BUFFERED,
        IOCTL_WINETEST_RETURN_STATUS_DIRECT,
        IOCTL_WINETEST_RETURN_STATUS_NEITHER,
    };

    static const NTSTATUS status_tests[] =
    {
        STATUS_SUCCESS,
        STATUS_PENDING,
        STATUS_TIMEOUT,
        0x0eadbeef,
        0x4eadbeef,
        STATUS_BUFFER_OVERFLOW,
        0x8eadbeef,
        STATUS_NOT_IMPLEMENTED,
        0xceadbeef,
    };

    for (i = 0; i < ARRAY_SIZE(status_tests); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(status_tests); ++j)
        {
            for (params.pending = 0; params.pending <= 1; ++params.pending)
            {
                for (k = 0; k < ARRAY_SIZE(method_tests); ++k)
                {
                    params.ret_status = status_tests[i];
                    params.iosb_status = status_tests[j];

                    winetest_push_context("return 0x%08lx, iosb 0x%08lx, pending %d, method %lu",
                            params.ret_status, params.iosb_status, params.pending, method_tests[k] & 3);

                    do_return_status(method_tests[k], &params);

                    winetest_pop_context();
                }
            }
        }
    }
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
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryObject(device, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(device, &io, buffer, sizeof(buffer), FileNameInformation);
    todo_wine ok(status == STATUS_INVALID_DEVICE_REQUEST, "got %#lx\n", status);

    status = NtQueryVolumeInformationFile(device, &io, buffer, sizeof(buffer), FileFsVolumeInformation);
    todo_wine ok(status == STATUS_INVALID_DEVICE_REQUEST, "got %#lx\n", status);

    file = CreateFileA("\\\\.\\WineTestDriver\\subfile", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    todo_wine ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());
    if (file == INVALID_HANDLE_VALUE) return;

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), &size);
    ok(!status, "got %#lx\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %lu\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfile"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, size - 2, &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %lu\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfil"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(*name_info), &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %lu\n", size);

    status = NtQueryObject(file, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(file_info->FileName, file_info->FileNameLength, L"\\subfile"),
            "wrong name %s\n", debugstr_wn(file_info->FileName, file_info->FileNameLength / sizeof(WCHAR)));

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsVolumeInformation);
    ok(!status, "got %#lx\n", status);
    ok(!io.Status, "got status %#lx\n", io.Status);
    size = offsetof(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + volume_info->VolumeLabelLength;
    ok(io.Information == size, "expected information %lu, got %Iu\n", size, io.Information);
    ok(volume_info->VolumeSerialNumber == 0xdeadbeef,
            "wrong serial number 0x%08lx\n", volume_info->VolumeSerialNumber);
    ok(compare_unicode_string(volume_info->VolumeLabel, volume_info->VolumeLabelLength, L"WineTestDriver"),
            "wrong name %s\n", debugstr_wn(volume_info->VolumeLabel, volume_info->VolumeLabelLength / sizeof(WCHAR)));

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsAttributeInformation);
    ok(status == STATUS_NOT_IMPLEMENTED, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %Iu\n", io.Information);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\notimpl", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_NOT_IMPLEMENTED, "got %#lx\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badparam", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#lx\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\genfail", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_UNSUCCESSFUL, "got %#lx\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_UNSUCCESSFUL, "got %#lx\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badtype", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#lx\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#lx\n", status);

    CloseHandle(file);
}

static void test_blocking_irp(void)
{
    char buffer[40];
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE file;

    file = CreateFileA("\\\\.\\WineTestDriver\\", FILE_ALL_ACCESS, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    memset(&io, 0xcc, sizeof(io));
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsSizeInformation);
    ok(!status, "got %#lx\n", status);
    ok(!io.Status, "got iosb status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsFullSizeInformation);
    ok(status == STATUS_DEVICE_NOT_READY, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got iosb status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %#Ix\n", io.Information);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\", FILE_ALL_ACCESS, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    memset(&io, 0xcc, sizeof(io));
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsSizeInformation);
    ok(!status, "got %#lx\n", status);
    ok(!io.Status, "got iosb status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

    memset(&io, 0xcc, sizeof(io));
    status = NtQueryVolumeInformationFile(file, &io, buffer, sizeof(buffer), FileFsFullSizeInformation);
    ok(status == STATUS_DEVICE_NOT_READY, "got %#lx\n", status);
    ok(io.Status == STATUS_DEVICE_NOT_READY, "got iosb status %#lx\n", io.Status);
    ok(!io.Information, "got information %#Ix\n", io.Information);

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
       GetLastError() == ERROR_FILE_NOT_FOUND /* Win7 */, "got %lu\n", GetLastError());

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
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    hthread = CreateThread(NULL, 0, wsk_test_thread, NULL, 0, NULL);
    main_test();
    WaitForSingleObject(hthread, INFINITE);

    CloseHandle(device);

    unload_driver(service);
    ret = DeleteFileW(filename);
    ok(ret, "DeleteFile failed: %lu\n", GetLastError());

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
    CRYPTCATMEMBER *member;
    WCHAR hash_buffer[100];
    GUID subject_guid;
    unsigned int i;
    DWORD size;
    BOOL ret;

    ret = CryptSIPRetrieveSubjectGuidForCatalogFile(file, NULL, &subject_guid);
    todo_wine ok(ret, "Failed to get subject guid, error %lu\n", GetLastError());

    size = 0;
    subject_info.pgSubjectType = &subject_guid;
    subject_info.pwsFileName = file;
    subject_info.DigestAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;
    ret = CryptSIPCreateIndirectData(&subject_info, &size, NULL);
    todo_wine ok(ret, "Failed to get indirect data size, error %lu\n", GetLastError());

    indirect_data = malloc(size);
    ret = CryptSIPCreateIndirectData(&subject_info, &size, indirect_data);
    todo_wine ok(ret, "Failed to get indirect data, error %lu\n", GetLastError());
    if (ret)
    {
        memset(hash_buffer, 0, sizeof(hash_buffer));
        for (i = 0; i < indirect_data->Digest.cbData; ++i)
            swprintf(&hash_buffer[i * 2], 2, L"%02X", indirect_data->Digest.pbData[i]);

        member = CryptCATPutMemberInfo(catalog, (WCHAR *)file,
                hash_buffer, &subject_guid, 0, size, (BYTE *)indirect_data);
        ok(!!member, "Failed to write member, error %lu\n", GetLastError());
    }

    free(indirect_data);
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

            ok(InSendMessageEx(NULL) == ISMEX_NOTIFY, "got message flags %#lx\n", InSendMessageEx(NULL));
            ok(!lparam, "got lparam %#Ix\n", lparam);
            break;

        case DBT_DEVICEARRIVAL:
        {
            const DEV_BROADCAST_DEVICEINTERFACE_A *iface = (const DEV_BROADCAST_DEVICEINTERFACE_A *)lparam;
            DWORD expect_size = offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[strlen(iface->dbcc_name)]);

            if (winetest_debug > 1) trace("device arrival %s\n", iface->dbcc_name);

            ok(InSendMessageEx(NULL) == ISMEX_SEND, "got message flags %#lx\n", InSendMessageEx(NULL));

            ok(iface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE,
                    "got unexpected notification type %#lx\n", iface->dbcc_devicetype);
            ok(iface->dbcc_size >= expect_size, "expected size at least %lu, got %lu\n", expect_size, iface->dbcc_size);
            ok(!iface->dbcc_reserved, "got reserved %#lx\n", iface->dbcc_reserved);
            if (IsEqualGUID(&iface->dbcc_classguid, &bus_class))
            {
                ++got_bus_arrival;
                ok(!strcmp(iface->dbcc_name, "\\\\?\\ROOT#WINETEST#0#{deadbeef-29ef-4538-a5fd-b69573a362c1}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            else if (IsEqualGUID(&iface->dbcc_classguid, &child_class))
            {
                ++got_child_arrival;
                ok(!strcmp(iface->dbcc_name, "\\\\?\\Wine#Test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            break;
        }

        case DBT_DEVICEREMOVECOMPLETE:
        {
            const DEV_BROADCAST_DEVICEINTERFACE_A *iface = (const DEV_BROADCAST_DEVICEINTERFACE_A *)lparam;
            DWORD expect_size = offsetof(DEV_BROADCAST_DEVICEINTERFACE_A, dbcc_name[strlen(iface->dbcc_name)]);

            if (winetest_debug > 1) trace("device removal %s\n", iface->dbcc_name);

            ok(InSendMessageEx(NULL) == ISMEX_SEND, "got message flags %#lx\n", InSendMessageEx(NULL));

            ok(iface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE,
                    "got unexpected notification type %#lx\n", iface->dbcc_devicetype);
            ok(iface->dbcc_size >= expect_size, "expected size at least %lu, got %lu\n", expect_size, iface->dbcc_size);
            ok(!iface->dbcc_reserved, "got reserved %#lx\n", iface->dbcc_reserved);
            if (IsEqualGUID(&iface->dbcc_classguid, &bus_class))
            {
                ++got_bus_removal;
                ok(!strcmp(iface->dbcc_name, "\\\\?\\ROOT#WINETEST#0#{deadbeef-29ef-4538-a5fd-b69573a362c1}"),
                        "got name %s\n", debugstr_a(iface->dbcc_name));
            }
            else if (IsEqualGUID(&iface->dbcc_classguid, &child_class))
            {
                ++got_child_removal;
                ok(!strcmp(iface->dbcc_name, "\\\\?\\Wine#Test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}"),
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
    static const GUID expect_container_id_guid = {0x12345678, 0x1234, 0x1234, {0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x91, 0x23}};
    static const char expect_hardware_id[] = "winetest_hardware\0winetest_hardware_1\0";
    static const WCHAR expect_hardware_id_w[] = L"winetest_hardware\0winetest_hardware_1\0";
    static const char expect_compat_id[] = "winetest_compat\0winetest_compat_1\0";
    static const WCHAR expect_compat_id_w[] = L"winetest_compat\0winetest_compat_1\0";
    static const WCHAR expect_container_id_w[] = L"{12345678-1234-1234-1234-123456789123}";
    static const char foobar[] = "foobar";
    static const char foo[] = "foo";
    static const char bar[] = "bar";

    char buffer[200];
    WCHAR buffer_w[200];
    GUID buffer_guid = {0};
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *iface_detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DEVPROP_BOOLEAN enabled = DEVPROP_FALSE;
    DEVPROPTYPE prop_type = DEVPROP_TYPE_EMPTY;
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
    DWORD size = 0, type, dword;
    HANDLE bus, child, tmp;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    OVERLAPPED ovl = {0};
    IO_STATUS_BLOCK io;
    HDEVINFO set;
    HWND window;
    LSTATUS status;
    HKEY key;
    BOOL ret;
    int id;

    ret = RegisterClassA(&class);
    ok(ret, "failed to register class\n");
    window = CreateWindowA("ntoskrnl_test_wc", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    ok(!!window, "failed to create window\n");
    notify_handle = RegisterDeviceNotificationA(window, &filter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    ok(!!notify_handle, "failed to register window, error %lu\n", GetLastError());

    set = SetupDiGetClassDevsA(&control_class, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());

    ret = SetupDiEnumDeviceInfo(set, 0, &device);
    ok(ret, "failed to get device, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &GUID_DEVCLASS_SYSTEM), "wrong class %s\n", debugstr_guid(&device.ClassGuid));

    ret = SetupDiGetDeviceInstanceIdA(set, &device, buffer, sizeof(buffer), NULL);
    ok(ret, "failed to get device ID, error %#lx\n", GetLastError());
    ok(!strcmp(buffer, "ROOT\\WINETEST\\0"), "got ID %s\n", debugstr_a(buffer));

    ret = SetupDiEnumDeviceInterfaces(set, NULL, &control_class, 0, &iface);
    ok(ret, "failed to get interface, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &control_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == SPINT_ACTIVE, "got flags %#lx\n", iface.Flags);

    iface_detail->cbSize = sizeof(*iface_detail);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, iface_detail, sizeof(buffer), NULL, NULL);
    ok(ret, "failed to get interface path, error %#lx\n", GetLastError());
    ok(!strcmp(iface_detail->DevicePath, "\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}"),
            "wrong path %s\n", debugstr_a(iface_detail->DevicePath));

    ret = SetupDiGetDeviceInterfacePropertyW(set, &iface, &DEVPKEY_DeviceInterface_Enabled, &prop_type,
                                             (BYTE *)&enabled, sizeof(enabled), &size, 0);
    ok(ret, "failed to get device interface property, got error %lu\n", GetLastError());
    ok(prop_type == DEVPROP_TYPE_BOOLEAN, "got prop_type %#lx\n", prop_type);
    ok(size == sizeof(enabled), "got size %lu\n", size);
    ok(enabled == DEVPROP_TRUE, "got enabled %d\n", enabled);

    /* Create a device parameter for testing IoOpenDeviceRegistryKey */
    key = SetupDiCreateDevRegKeyA(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "failed to create a hardware parameters key, got error %#lx\n", GetLastError());
    status = RegSetValueExA(key, foobar, 0, REG_SZ, (const BYTE *)foo, sizeof(foo));
    ok(status == ERROR_SUCCESS, "failed to save a device parameter, got error %lu\n", status);
    RegCloseKey(key);

    SetupDiDestroyDeviceInfoList(set);

    bus = CreateFileA(iface_detail->DevicePath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(bus != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_MAIN, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    /* Test IoRegisterDeviceInterface() and IoSetDeviceInterfaceState(). */

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#lx\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_REGISTER_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(!iface.Flags, "got flags %#lx\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#lx\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_ENABLE_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    pump_messages();
    ok(got_bus_arrival == 1, "got %u bus arrival messages\n", got_bus_arrival);
    ok(!got_bus_removal, "got %u bus removal messages\n", got_bus_removal);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == SPINT_ACTIVE, "got flags %#lx\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_DISABLE_IFACE, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    pump_messages();
    ok(got_bus_arrival == 1, "got %u bus arrival messages\n", got_bus_arrival);
    ok(got_bus_removal == 1, "got %u bus removal messages\n", got_bus_removal);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(ret, "failed to get interface, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &bus_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(!iface.Flags, "got flags %#lx\n", iface.Flags);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&bus_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, NULL, &bus_class, 0, &iface);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "got error %#lx\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    /* Test exposing a child device. */

    id = 1;
    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_ADD_CHILD, &id, sizeof(id), NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    pump_messages();
    ok(got_child_arrival == 1, "got %u child arrival messages\n", got_child_arrival);
    ok(!got_child_removal, "got %u child removal messages\n", got_child_removal);

    set = SetupDiGetClassDevsA(&child_class, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());

    ret = SetupDiEnumDeviceInfo(set, 0, &device);
    ok(ret, "failed to get device, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &GUID_NULL), "wrong class %s\n", debugstr_guid(&device.ClassGuid));

    ret = SetupDiGetDeviceInstanceIdA(set, &device, buffer, sizeof(buffer), NULL);
    ok(ret, "failed to get device ID, error %#lx\n", GetLastError());
    ok(!strcmp(buffer, "WINE\\TEST\\1"), "got ID %s\n", debugstr_a(buffer));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CAPABILITIES,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    todo_wine ok(ret, "got error %#lx\n", GetLastError());
    if (ret)
    {
        ok(dword == (CM_DEVCAP_EJECTSUPPORTED | CM_DEVCAP_UNIQUEID
                | CM_DEVCAP_RAWDEVICEOK | CM_DEVCAP_SURPRISEREMOVALOK), "got flags %#lx\n", dword);
        ok(type == REG_DWORD, "got type %lu\n", type);
    }

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASSGUID,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    todo_wine ok(!ret, "expected failure\n");
    if (ret)
        ok(GetLastError() == ERROR_INVALID_DATA, "got error %#lx\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CONFIGFLAGS,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    ok(ret, "got error %#lx\n", GetLastError());
    /* windows 7 sets CONFIGFLAG_FINISH_INSTALL; it's not clear what this means */
    ok(!(dword & ~CONFIGFLAG_FINISH_INSTALL), "got flags %#lx\n", dword);
    ok(type == REG_DWORD, "got type %lu\n", type);

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVTYPE,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "got error %#lx\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "got error %#lx\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            &type, (BYTE *)buffer, sizeof(buffer), &size);
    ok(ret, "got error %#lx\n", GetLastError());
    ok(type == REG_MULTI_SZ, "got type %lu\n", type);
    ok(size == sizeof(expect_hardware_id), "got size %lu\n", size);
    ok(!memcmp(buffer, expect_hardware_id, size), "got hardware IDs %s\n", debugstr_an(buffer, size));

    /* Using the WCHAR variant because Windows returns a WCHAR for this property even when using SetupDiGetDeviceRegistryPropertyA */
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_BASE_CONTAINERID,
            &type, (BYTE *)buffer_w, sizeof(buffer_w), &size);
    ok(ret, "got error %#lx\n", GetLastError());
    ok(type == REG_SZ, "got type %lu\n", type);
    ok(size == sizeof(expect_container_id_w), "got size %lu\n", size);
    ok(!memcmp(buffer_w, expect_container_id_w, size), "got container ID %s\n", debugstr_w(buffer_w));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_COMPATIBLEIDS,
            &type, (BYTE *)buffer, sizeof(buffer), &size);
    ok(ret, "got error %#lx\n", GetLastError());
    ok(type == REG_MULTI_SZ, "got type %lu\n", type);
    ok(size == sizeof(expect_compat_id), "got size %lu\n", size);
    ok(!memcmp(buffer, expect_compat_id, size), "got compatible IDs %s\n", debugstr_an(buffer, size));

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
            &type, (BYTE *)buffer, sizeof(buffer), NULL);
    todo_wine ok(ret, "got error %#lx\n", GetLastError());
    if (ret)
    {
        ok(type == REG_SZ, "got type %lu\n", type);
        ok(!strcmp(buffer, "\\Device\\winetest_pnp_1"), "got PDO name %s\n", debugstr_a(buffer));
    }

    prop_type = DEVPROP_TYPE_EMPTY;
    size = 0;
    memset(buffer_w, 0, sizeof(buffer_w));
    ret = SetupDiGetDevicePropertyW(set, &device, &DEVPKEY_Device_HardwareIds, &prop_type, (BYTE *)buffer_w,
                                    sizeof(buffer_w), &size, 0);
    todo_wine ok(ret, "failed to get device property, error %#lx\n", GetLastError());
    todo_wine ok(prop_type == DEVPROP_TYPE_STRING_LIST, "got type %#lx\n", prop_type);
    todo_wine ok(size == sizeof(expect_hardware_id_w), "got size %lu\n", size);
    todo_wine ok(!memcmp(buffer_w, expect_hardware_id_w, size), "got hardware IDs %s\n", debugstr_wn(buffer_w, size));

    prop_type = DEVPROP_TYPE_EMPTY;
    size = 0;
    memset(buffer_w, 0, sizeof(buffer_w));
    ret = SetupDiGetDevicePropertyW(set, &device, &DEVPKEY_Device_CompatibleIds, &prop_type, (BYTE *)buffer_w,
                                    sizeof(buffer_w), &size, 0);
    todo_wine ok(ret, "failed to get device property, error %#lx\n", GetLastError());
    todo_wine ok(prop_type == DEVPROP_TYPE_STRING_LIST, "got type %#lx\n", prop_type);
    todo_wine ok(size == sizeof(expect_compat_id_w), "got size %lu\n", size);
    todo_wine ok(!memcmp(buffer_w, expect_compat_id_w, size), "got compatible IDs %s\n", debugstr_wn(buffer_w, size));

    prop_type = DEVPROP_TYPE_EMPTY;
    size = 0;
    ret = SetupDiGetDevicePropertyW(set, &device, &DEVPKEY_Device_ContainerId, &prop_type, (BYTE *)&buffer_guid,
                                    sizeof(buffer_guid), &size, 0);
    todo_wine ok(ret, "failed to get device property, error %#lx\n", GetLastError());
    todo_wine ok(prop_type == DEVPROP_TYPE_GUID, "got type %#lx\n", prop_type);
    todo_wine ok(size == sizeof(expect_container_id_guid), "got size %lu\n", size);
    todo_wine ok(IsEqualGUID(&buffer_guid, &expect_container_id_guid), "got container ID %s != %s\n",
                 debugstr_guid(&buffer_guid), debugstr_guid(&expect_container_id_guid));

    ret = SetupDiEnumDeviceInterfaces(set, NULL, &child_class, 0, &iface);
    ok(ret, "failed to get interface, error %#lx\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &child_class),
            "wrong class %s\n", debugstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == SPINT_ACTIVE, "got flags %#lx\n", iface.Flags);

    iface_detail->cbSize = sizeof(*iface_detail);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, iface_detail, sizeof(buffer), NULL, NULL);
    ok(ret, "failed to get interface path, error %#lx\n", GetLastError());
    ok(!strcmp(iface_detail->DevicePath, "\\\\?\\wine#test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}"),
            "wrong path %s\n", debugstr_a(iface_detail->DevicePath));

    prop_type = DEVPROP_TYPE_EMPTY;
    size = 0;
    enabled = DEVPROP_FALSE;
    ret = SetupDiGetDeviceInterfacePropertyW(set, &iface, &DEVPKEY_DeviceInterface_Enabled, &prop_type,
                                             (BYTE *)&enabled, sizeof(enabled), &size, 0);
    ok(ret, "failed to get device interface property, got error %lu\n", GetLastError());
    ok(prop_type == DEVPROP_TYPE_BOOLEAN, "got prop_type %#lx\n", prop_type);
    ok(size == sizeof(enabled), "got size %lu\n", size);
    ok(enabled == DEVPROP_TRUE, "got enabled %d\n", enabled);

    /* Create a device parameter for testing IoOpenDeviceRegistryKey */
    key = SetupDiCreateDevRegKeyA(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "failed to create a hardware parameters key, got error %#lx\n", GetLastError());
    status = RegSetValueExA(key, foobar, 0, REG_SZ, (const BYTE *)bar, sizeof(bar));
    ok(status == ERROR_SUCCESS, "failed to save a device parameter, got error %lu\n", status);
    RegCloseKey(key);

    SetupDiDestroyDeviceInfoList(set);

    RtlInitUnicodeString(&string, L"\\Device\\winetest_pnp_1");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL);
    ret = NtOpenFile(&child, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    ok(!ret, "failed to open child: %#x\n", ret);

    id = 0xdeadbeef;
    ret = DeviceIoControl(child, IOCTL_WINETEST_CHILD_GET_ID, NULL, 0, &id, sizeof(id), &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());
    ok(id == 1, "got id %d\n", id);
    ok(size == sizeof(id), "got size %lu\n", size);

    ret = DeviceIoControl(child, IOCTL_WINETEST_CHILD_MAIN, NULL, 0, NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    CloseHandle(child);

    ret = NtOpenFile(&child, SYNCHRONIZE, &attr, &io, 0, 0);
    ok(!ret, "failed to open child: %#x\n", ret);

    ret = DeviceIoControl(child, IOCTL_WINETEST_CHILD_MARK_PENDING, NULL, 0, NULL, 0, &size, &ovl);
    ok(!ret, "DeviceIoControl succeeded\n");
    ok(GetLastError() == ERROR_IO_PENDING, "got error %lu\n", GetLastError());
    ok(size == 0, "got size %lu\n", size);

    id = 1;
    ret = DeviceIoControl(bus, IOCTL_WINETEST_BUS_REMOVE_CHILD, &id, sizeof(id), NULL, 0, &size, NULL);
    ok(ret, "got error %lu\n", GetLastError());

    pump_messages();
    ok(got_child_arrival == 1, "got %u child arrival messages\n", got_child_arrival);
    ok(got_child_removal == 1, "got %u child removal messages\n", got_child_removal);

    ret = DeviceIoControl(child, IOCTL_WINETEST_CHILD_CHECK_REMOVED, NULL, 0, NULL, 0, &size, NULL);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = NtOpenFile(&tmp, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT);
    todo_wine ok(ret == STATUS_NO_SUCH_DEVICE, "got %#x\n", ret);

    ret = GetOverlappedResult(child, &ovl, &size, TRUE);
    ok(!ret, "unexpected success.\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError());
    ok(size == 0, "got size %lu\n", size);

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
    DWORD dword, type;
    unsigned int i;
    HDEVINFO set;
    FILE *f;

    GetCurrentDirectoryA(ARRAY_SIZE(cwd), cwd);
    GetTempPathA(ARRAY_SIZE(tempdir), tempdir);
    SetCurrentDirectoryA(tempdir);

    load_resource(L"driver_pnp.dll", driver_filename);
    ret = MoveFileExW(driver_filename, L"winetest.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
    ok(ret, "failed to move file, error %lu\n", GetLastError());

    f = fopen("winetest.inf", "w");
    ok(!!f, "failed to open winetest.inf: %s\n", strerror(errno));
    fputs(inf_text, f);
    fclose(f);

    /* Create the catalog file. */

    catalog = CryptCATOpen((WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0);
    ok(catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %#lx\n", GetLastError());

    ret = !!CryptCATPutCatAttrInfo(catalog, (WCHAR *)L"HWID1",
            CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
            sizeof(L"test_hardware_id"), (BYTE *)L"test_hardware_id");
    todo_wine ok(ret, "failed to add attribute, error %#lx\n", GetLastError());

    ret = !!CryptCATPutCatAttrInfo(catalog, (WCHAR *)L"OS",
            CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
            sizeof(L"VistaX64"), (BYTE *)L"VistaX64");
    todo_wine ok(ret, "failed to add attribute, error %#lx\n", GetLastError());

    add_file_to_catalog(catalog, L"winetest.sys");
    add_file_to_catalog(catalog, L"winetest.inf");

    ret = CryptCATPersistStore(catalog);
    todo_wine ok(ret, "Failed to write catalog, error %lu\n", GetLastError());

    ret = CryptCATClose(catalog);
    ok(ret, "Failed to close catalog, error %lu\n", GetLastError());

    testsign_sign(ctx, L"winetest.cat");

    /* Install the driver. */

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "failed to create device list, error %#lx\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "root\\winetest\\0", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "failed to create device, error %#lx\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA( set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id) );
    ok(ret, "failed to create set hardware ID, error %#lx\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "failed to register device, error %#lx\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CONFIGFLAGS,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "got error %#lx\n", GetLastError());

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = UpdateDriverForPlugAndPlayDevicesA(NULL, hardware_id, path, INSTALLFLAG_FORCE, &need_reboot);
    ok(ret, "failed to install device, error %#lx\n", GetLastError());
    ok(!need_reboot, "expected no reboot necessary\n");

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CONFIGFLAGS,
            &type, (BYTE *)&dword, sizeof(dword), NULL);
    ok(ret, "got error %#lx\n", GetLastError());
    ok(!dword, "got flags %#lx\n", dword);
    ok(type == REG_DWORD, "got type %lu\n", type);

    /* Tests. */

    test_pnp_devices();

    /* Clean up. */

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "failed to remove device, error %#lx\n", GetLastError());

    file = CreateFileA("\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file == INVALID_HANDLE_VALUE, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "failed to destroy set, error %#lx\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "wine", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "failed to get device list, error %#lx\n", GetLastError());

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
        ok(ret, "failed to remove device, error %#lx\n", GetLastError());
    }

    SetupDiDestroyDeviceInfoList(set);

    /* Windows stops the service but does not delete it. */
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(!!manager, "failed to open service manager, error %lu\n", GetLastError());
    service = OpenServiceA(manager, "winetest", SERVICE_STOP | DELETE);
    ok(!!service, "failed to open service, error %lu\n", GetLastError());
    unload_driver(service);
    CloseServiceHandle(manager);

    cat_okfile();

    GetFullPathNameA("winetest.inf", sizeof(path), path, NULL);
    ret = SetupCopyOEMInfA(path, NULL, 0, 0, dest, sizeof(dest), NULL, &filepart);
    ok(ret, "Failed to copy INF, error %#lx\n", GetLastError());
    ret = SetupUninstallOEMInfA(filepart, SUOI_FORCEDELETE, NULL);
    ok(ret, "Failed to uninstall INF, error %lu\n", GetLastError());

    ret = DeleteFileA("winetest.cat");
    ok(ret, "Failed to delete file, error %lu\n", GetLastError());
    ret = DeleteFileA("winetest.inf");
    ok(ret, "Failed to delete file, error %lu\n", GetLastError());
    ret = DeleteFileA("winetest.sys");
    ok(ret, "Failed to delete file, error %lu\n", GetLastError());
    /* Windows 10 apparently deletes the image in SetupUninstallOEMInf(). */
    ret = DeleteFileA("C:/windows/system32/drivers/winetest.sys");
    ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %lu\n", GetLastError());

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

    pCancelIoEx = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CancelIoEx");
    pSetFileCompletionNotificationModes = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                                 "SetFileCompletionNotificationModes");

    if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
    {
        skip("Running in WoW64.\n");
        return;
    }

    if (!testsign_create_cert(&ctx))
        return;

    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, sizeof(*test_data), "Global\\winetest_ntoskrnl_section");
    ok(!!mapping, "got error %lu\n", GetLastError());
    test_data = MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024);
    test_data->running_under_wine = !strcmp(winetest_platform, "wine");
    test_data->winetest_report_success = winetest_report_success;
    test_data->winetest_debug = winetest_debug;

    okfile = CreateFileA("C:\\windows\\winetest_ntoskrnl_okfile", GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    ok(okfile != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError());

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
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %lu\n", GetLastError());

    test_basic_ioctl();

    main_test();
    todo_wine ok(modified_value == 0xdeadbeeffeedcafe, "Got unexpected value %#I64x.\n", modified_value);

    test_overlapped();
    test_load_driver(service2);
    test_file_handles();
    test_return_status();
    test_object_info();
    test_blocking_irp();

    /* We need a separate ioctl to call IoDetachDevice(); calling it in the
     * driver unload routine causes a live-lock. */
    ret = DeviceIoControl(device, IOCTL_WINETEST_DETACH, NULL, 0, NULL, 0, &written, NULL);
    ok(ret, "DeviceIoControl failed: %lu\n", GetLastError());

    CloseHandle(device);

    unload_driver(service2);
    unload_driver(service);
    ret = DeleteFileW(filename);
    ok(ret, "DeleteFile failed: %lu\n", GetLastError());
    ret = DeleteFileW(filename2);
    ok(ret, "DeleteFile failed: %lu\n", GetLastError());

    cat_okfile();

    test_driver3(&ctx);
    subtest("driver_netio");
    test_driver_netio(&ctx);

    subtest("driver_pnp");
    test_pnp_driver(&ctx);

out:
    testsign_cleanup(&ctx);
    UnmapViewOfFile(test_data);
    CloseHandle(mapping);
    CloseHandle(okfile);
    DeleteFileA("C:\\windows\\winetest_ntoskrnl_okfile");
}
