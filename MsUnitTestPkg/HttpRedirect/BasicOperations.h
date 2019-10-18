/** @file
BasicOperations.h

Header file for NetworkRequest block

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BASIC_OPERATIONS_H__
#define __BASIC_OPERATIONS_H__


#define MAX_DELAY_BEFORE_RETRY 24
#define MIN_DELAY_BEFORE_RETRY  1

/**
 * Process Http Recovery
 *
 * @param NetworkRequest     - Private data
 * @param Message            - Where to store the status message
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
ProcessNetworkRequest (
    IN  NETWORK_REQUEST         *NetworkRequest,
    OUT CHAR16                 **Message
  );

/**
 * Process Http Recovery
 *
 * @param NetworkRequest     - Private data
 * @param Message            - Where to store the status message
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
ProcessNetworkRequestWORKAROUND (
    IN  NETWORK_REQUEST         *NetworkRequest,
    OUT CHAR16                 **Message
  );

#endif // __BASIC_OPERATIONS_H__