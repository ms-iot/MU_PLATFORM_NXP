/** @file
  The OP-TEE Share Memory component is used to manage memory allocations
  from the memory block used for parameters & buffers shared between the
  client in normal world and the OpTEE OS in secure world.
 
  Copyright (c) Microsoft Corporation. All rights reserved.
 
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License which accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.
 
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __OPTEE_CLIENT_API_LIB_H__
#define __OPTEE_CLIENT_API_LIB_H__

EFI_STATUS
OpteeClientApiInitialize (
  IN EFI_HANDLE   ImageHandle
  );

BOOLEAN
OpteeClientApiFinalize (
  VOID
  );

#endif // __OPTEE_CLIENT_API_LIB_H__


