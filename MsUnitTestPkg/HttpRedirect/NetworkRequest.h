/** @file
NetworkRequest.h

Header file for NetworkRequest block

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __NETWORK_REQUEST_H__
#define __NETWORK_REQUEST_H__

typedef struct _NETWORK_REQUEST NETWORK_REQUEST;

struct _NETWORK_REQUEST {
    //
    // Shared input from DfciMenu Initialization
    //
    CONST UINT8                        *HttpsCert;
    UINTN                               HttpsCertSize;

    struct {
        //
        // Input parameters for network operations
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_REQUEST)
        CHAR8                          *Url;
        UINTN                           UrlSize;
        EFI_HTTP_METHOD                 HttpMethod;
        CHAR8                          *BootstrapUrl;
        UINTN                           BootstrapUrlSize;
        CHAR8                          *Body;
        UINTN                           BodySize;
    } HttpRequest;

    struct {
        //
        // Output Parameters  Caller to free Body and Headers
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_RESPONSE)
        CHAR8                          *Body;
        UINTN                           BodySize;
        EFI_HTTP_HEADER                *Headers;
        UINTN                           HeaderCount;
    } HttpResponse;

    struct {
        //
        // Http Request Status
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_STATUS)
        CHAR8                          *HttpReturnCode;
        UINTN                           HttpReturnCodeSize;
        CHAR8                          *HttpMessage;
        UINTN                           HttpMessageSize;
        EFI_HTTP_STATUS_CODE            HttpStatus;
    } HttpStatus;

    struct {
        //
        // Http service section
        //
        EFI_HTTP_PROTOCOL              *HttpProtocol;
        EFI_HANDLE                      HttpChildHandle;
        EFI_HTTP_CONFIG_DATA            ConfigData;
    } Http;

    struct {
        //
        // NIC Section
        //
        // This section is managed by the function TryEachNICThenProcessRequest
        EFI_HANDLE                      NicHandle;
        BOOLEAN                         DhcpRequested;
        EFI_SERVICE_BINDING_PROTOCOL   *HttpSbProtocol;

        //
        // IPv4 Specific section
        //
        EFI_HTTPv4_ACCESS_POINT         IPv4Node;

        //
        // Fields valid during DHCP delay
        //
        EFI_EVENT                       WaitEvent;
    } HttpNic;

};


/**
 *  Function to process the main logic of the Dfci network provider
 *
 * @param[in]  Network Request
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Error processing packet
 */
typedef
EFI_STATUS
(EFIAPI *MAIN_LOGIC) (
    IN  NETWORK_REQUEST   *NetworkRequest,
    OUT BOOLEAN           *DoneProcessing
);

#endif