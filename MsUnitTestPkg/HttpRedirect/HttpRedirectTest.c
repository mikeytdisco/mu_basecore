/** @file
HttpRedirectTestApp.c

This is a Unit Test for the Http Redirect operations.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <UnitTestTypes.h>

#include <Protocol/BootManagerPolicy.h>
#include <Protocol/Http.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/TlsConfig.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestLib.h>

#include "NetworkRequest.h"
#include "BasicOperations.h"

#define UNIT_TEST_APP_NAME        L"Http Redirect test cases"
#define UNIT_TEST_APP_VERSION     L"1.0"

/**

This test accesses a web site with an http URL, and processes
the redirect.

  Test1 - Access aka.ms


 */

#define TEST1_URL         "http://mikeytbds3.eastus.cloudapp.azure.com/"

#define TEST2_URL         "http://mikeytbds3.eastus.cloudapp.azure.com/RedirTest1"
#define TEST2_URLB        "http://mikeytbds3.eastus.cloudapp.azure.com/RedirTest3"
#define TEST2_CERT_FILE  L"mikeytbds3.cer"

#define TEST3_URL         "http://mikeytbds3.eastus.cloudapp.azure.com"

typedef struct {
    CHAR8                *Url;
    CHAR8                *UrlB;
    EFI_HTTP_METHOD       HttpMethod;
    CHAR16               *CertFileName;
    CHAR8                *TestUrl;
    CHAR8                *TestUrlB;
    CONST UINT8          *TestCert;
    EFI_STATUS            ExpectedStatus;
} BASIC_TEST_CONTEXT;

//*----------------------------------------------------------------------------------*
//* Test Contexts                                                                    *
//*----------------------------------------------------------------------------------*
//
// WARNING:  The HttpDxe driver doesn't parse the URL safely.  Be sure your URL's end
//           with a trailing /.
//
static BASIC_TEST_CONTEXT mTest1  = { TEST1_URL, NULL,       HttpMethodGet, NULL,             NULL, NULL, NULL, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest2  = { TEST2_URL, TEST2_URLB, HttpMethodGet, TEST2_CERT_FILE,  NULL, NULL, NULL, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest3  = { TEST2_URL, TEST2_URLB, HttpMethodGet, TEST2_CERT_FILE,  NULL, NULL, NULL, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest4  = { TEST3_URL, NULL,       HttpMethodGet, NULL,             NULL, NULL, NULL, EFI_SUCCESS };

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================
/**
 * ReadFileIntoMemory
 *
 *
 * @param FileName  - Pointer to file name
 * @param Buffer    - Pointer to buffer allocated by this routine
 * @param BufferSize- Pointer to file size
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ReadFileIntoMemory (IN  CONST CHAR16  *FileName,
                    OUT CONST UINT8  **Buffer,
                    OUT       UINT64  *BufferSize) {

    SHELL_FILE_HANDLE               FileHandle;
    UINT64                          ReadSize;
    EFI_STATUS                      Status;
    UINT8                          *LocalBuffer;

    if ((NULL == FileName) || (NULL == Buffer) || (NULL == BufferSize)) {
        DEBUG((DEBUG_ERROR, "Internal error in SetDfciVariable\n"));
        return EFI_INVALID_PARAMETER;
    }

    Status = ShellOpenFileByName(FileName,
                                 &FileHandle,
                                 EFI_FILE_MODE_READ,
                                 0);
    if (EFI_ERROR(Status)) {
        DEBUG(( DEBUG_ERROR, "Failed to open %s file. Status = %r\n", FileName, Status));
        return Status;
    }

    Status = ShellGetFileSize (FileHandle,
                               BufferSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to get filesize of %s. Status = %r\n", FileName, Status));
        return Status;
    }

    LocalBuffer = AllocatePool (*BufferSize);

    if (NULL == LocalBuffer) {
        DEBUG((DEBUG_ERROR, "Unable to allocate buffer for %s\n", FileName));
        Status = EFI_OUT_OF_RESOURCES;
        ReadSize = 0;
    } else {
        ReadSize = *BufferSize;
        Status = ShellReadFile (FileHandle, &ReadSize, LocalBuffer);
        ShellCloseFile (&FileHandle);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error reading file %s. Code = %r\n", FileName, Status));
        } else if (ReadSize != *BufferSize) {
            DEBUG((DEBUG_ERROR, "File Read not complete reading file %s. Req=%d,Act=%dr\n", FileName, *BufferSize, ReadSize));
            Status = EFI_BUFFER_TOO_SMALL;
        }
    }

    if (EFI_ERROR(Status)) {
        if (NULL != LocalBuffer) {
            FreePool (LocalBuffer);
        }
    }
    *Buffer = LocalBuffer;

    return Status;
}

/*
    CleanUpTestContext

    Cleans up after a test case.  Free's any allocated buffers if a test
    takes the error exit.

*/
static
UNIT_TEST_STATUS
EFIAPI
CleanUpTestContext (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    if (NULL != Btc->TestUrl) {
        FreePool ((VOID *) Btc->TestUrl);
    }

    if (NULL != Btc->TestUrlB) {
        FreePool ((VOID *) Btc->TestUrlB);
    }

    if (NULL != Btc->TestCert) {
        FreePool ((VOID *) Btc->TestCert);
    }

    Btc->TestUrl = NULL;
    Btc->TestUrlB = NULL;
    Btc->TestCert = NULL;

    return UNIT_TEST_PASSED;
}


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

/*
    Try Http request

    Verify that the characters are allowed.
*/
static
UNIT_TEST_STATUS
EFIAPI
TryHttp (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    CHAR16             *Message = NULL;
    NETWORK_REQUEST     NetworkRequest;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    ZeroMem (&NetworkRequest, sizeof(NetworkRequest));

    Btc->TestUrl = AllocateCopyPool (AsciiStrSize (Btc->Url), Btc->Url);
    NetworkRequest.HttpRequest.Url = Btc->TestUrl;
    NetworkRequest.HttpRequest.UrlSize = AsciiStrSize (Btc->TestUrl);

    if (Btc->UrlB != NULL) {
        Btc->TestUrlB = AllocateCopyPool (AsciiStrSize (Btc->UrlB), Btc->UrlB);
        NetworkRequest.HttpRequest.BootstrapUrl = Btc->TestUrlB;
        NetworkRequest.HttpRequest.BootstrapUrlSize = AsciiStrSize (Btc->TestUrlB);
    }
    NetworkRequest.HttpRequest.HttpMethod = Btc->HttpMethod;

    if (Btc->CertFileName != NULL) {
        Status = ReadFileIntoMemory (Btc->CertFileName,
                                    &NetworkRequest.HttpsCert,
                                    &NetworkRequest.HttpsCertSize);

        UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

        Btc->TestCert = NetworkRequest.HttpsCert;
    }


    // Try every NIC in the system until one fills the first part of the request.
    Status = ProcessNetworkRequest (&NetworkRequest, &Message);

    DEBUG((DEBUG_ERROR, "%s\n", Message));

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);

    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
EFIAPI
TryHttpWA (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    CHAR16             *Message = NULL;
    NETWORK_REQUEST     NetworkRequest;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    ZeroMem (&NetworkRequest, sizeof(NetworkRequest));

    Btc->TestUrl = AllocateCopyPool (AsciiStrSize (Btc->Url), Btc->Url);
    NetworkRequest.HttpRequest.Url = Btc->TestUrl;
    NetworkRequest.HttpRequest.UrlSize = AsciiStrSize (Btc->TestUrl);

    if (Btc->UrlB != NULL) {
        Btc->TestUrlB = AllocateCopyPool (AsciiStrSize (Btc->UrlB), Btc->UrlB);
        NetworkRequest.HttpRequest.BootstrapUrl = Btc->TestUrlB;
        NetworkRequest.HttpRequest.BootstrapUrlSize = AsciiStrSize (Btc->TestUrlB);
    }
    NetworkRequest.HttpRequest.HttpMethod = Btc->HttpMethod;

    if (Btc->CertFileName != NULL) {
        Status = ReadFileIntoMemory (Btc->CertFileName,
                                    &NetworkRequest.HttpsCert,
                                    &NetworkRequest.HttpsCertSize);

        UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

        Btc->TestCert = NetworkRequest.HttpsCert;
    }


    // Try every NIC in the system until one fills the first part of the request.
    Status = ProcessNetworkRequestWORKAROUND (&NetworkRequest, &Message);

    DEBUG((DEBUG_ERROR, "%s\n", Message));

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);

    return UNIT_TEST_PASSED;
}
///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  HttpRedirectTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
HttpRedirectTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    UNIT_TEST_FRAMEWORK       *Fw = NULL;
    UNIT_TEST_SUITE           *HttpRedirectTests;
    CHAR16                     ShortName[100];
    EFI_STATUS                 Status;

    ShortName[0] = L'\0';
    UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName);
    DEBUG(( DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
        goto EXIT;
    }

    //
    // Populate the HttpRedirect Test Suite.
    //
    Status = CreateUnitTestSuite ( &HttpRedirectTests, Fw, L"Validate Http Redirect operations", L"HttpRedirect.Test", NULL, NULL);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Http Redirect Tests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    //-----------Suite---------------DescriptionURL------- -----Class------------Test Function-------Pre---Clean-Context
    AddTestCase ( HttpRedirectTests, L"Simple Get",           L"Access",       TryHttp,            NULL, NULL, &mTest1);
    AddTestCase ( HttpRedirectTests, L"Redirect To Https",    L"Access",       TryHttp,            NULL, NULL, &mTest2);
    AddTestCase ( HttpRedirectTests, L"Redirect To Https WA", L"Access",       TryHttpWA,          NULL, NULL, &mTest3);

    //The following test causes an exception:
    AddTestCase ( HttpRedirectTests, L"No ending /",          L"Access",       TryHttp,            NULL, NULL, &mTest4);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);

EXIT:
    if (Fw) {
        FreeUnitTestFramework (Fw);
    }

    return Status;
}