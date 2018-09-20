## @file
#   OP-TEE Client Libraries for the client side use in EDKII services implemented
#   in Trusted Applications running in the secure world OP-TEE OS.
#
#   Copyright (c) 2015, Microsoft Corporation. All rights reserved.
#
#   This program and the accompanying materials
#   are licensed and made available under the terms and conditions of the BSD License
#   which accompanies this distribution. The full text of the license may be found at
#   http://opensource.org/licenses/bsd-license.
#
#   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##

[Defines]
  PLATFORM_NAME                  = OpteeClientPkg
  PLATFORM_GUID                  = 0F54C9D6-0E6C-43D2-BF1C-9EF321E8FCFE
  PLATFORM_VERSION               = 0.01
  DSC_SPECIFICATION              = 0x00010006
  OUTPUT_DIRECTORY               = Build/OpteeClientPkg
  SUPPORTED_ARCHITECTURES        = ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

[PcdsFixedAtBuild]

[LibraryClasses]

[Components]
  Microsoft/OpteeClientPkg/Library/OpteeClientApiLib/OpteeClientApiLib.inf

