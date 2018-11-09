/** @file
*
*  Copyright (c) 2018 Microsoft Corporation. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/
#ifndef __AUTHVARTESTFFS_H__
#define __AUTHVARTESTFFS_H__

// Guid used for accessing the SecureBoot signature database (db) binary blob in the FV.
extern EFI_GUID gEfiSecureBootDbImageGuid;

// Guid used for accessing the SecureBoot Key Enrollment Key database (KEK) binary blob in the FV.
extern EFI_GUID gEfiSecureBootKekImageGuid;

#endif // __AUTHVARTESTFFS_H__
