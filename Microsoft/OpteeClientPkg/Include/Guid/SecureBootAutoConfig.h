#ifndef __SECUREBOOTAUTOCONFIG_H__
#define __SECUREBOOTAUTOCONFIG_H__

// Guid used for accessing the SecureBoot signature database (db) binary blob in the FV.
extern EFI_GUID gEfiSecureBootDbImageGuid;

// Guid used for accessing the SecureBoot revoked signature database (dbx) binary blob in the FV.
extern EFI_GUID gEfiSecureBootDbxImageGuid;

// Guid used for accessing the SecureBoot Platform Key (PK) binary blob in the FV.
extern EFI_GUID gEfiSecureBootPkImageGuid;

// Guid used for accessing the SecureBoot Key Enrollment Key database (KEK) binary blob in the FV.
extern EFI_GUID gEfiSecureBootKekImageGuid;

#endif // __SECUREBOOTAUTOCONFIG_H__