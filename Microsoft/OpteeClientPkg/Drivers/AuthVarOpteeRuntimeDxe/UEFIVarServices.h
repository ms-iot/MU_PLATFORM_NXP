/*++
Copyright (c) 2013 Microsoft Corporation. All Rights Reserved.
Module Name:
UEFIVarServices.h
Abstract:
UEFI Variable Services definitions for accessing operations.
Author:
Arnold Pereira(Arnoldp) Apr-09-2013
--*/
#pragma once

//
// Function codes for calling UEFI Variable service
//

typedef enum _VARIABLE_SERVICE_OPS
{
    VSGetOp = 0,
    VSGetNextVarOp,
    VSSetOp,
    VSQueryInfoOp,
    VSSignalExitBootServicesOp,
} VARIABLE_SERVICE_OPS;

//
// In the WTR environment this was #pragma pack(1) however this does not
// work in the GP environment that requires alignment.
//

//
// Parameter Struct for Set Operations. No data returned in result
//

typedef struct _VARIABLE_SET_PARAM
{
    UINT32 Size;
    UINT16 VariableNameSize;
    GUID VendorGuid;
    UINT32 Attributes;
    UINT32 DataSize;
    UINT32 OffsetVariableName;
    UINT32 OffsetData;
    _Field_size_bytes_(DataSize + VariableNameSize) BYTE Payload[1];
} VARIABLE_SET_PARAM, *PVARIABLE_SET_PARAM;

//
//  Struct for Get  and GetNextVariable operation needs only name and Guid
//

typedef struct _VARIABLE_GET_PARAM
{
    UINT32 Size;
    UINT16 VariableNameSize;
    GUID VendorGuid;
    _Field_size_bytes_(VariableNameSize) WCHAR VariableName[1];
} VARIABLE_GET_PARAM, VARIABLE_GET_NEXT_PARAM, VARIABLE_GET_NEXT_RESULT, *PVARIABLE_GET_PARAM;
typedef struct _VARIABLE_GET_PARAM *PVARIABLE_GET_NEXT_PARAM, *PVARIABLE_GET_NEXT_RESULT;

//
// Result struct for Get operation
//

typedef struct _VARIABLE_GET_RESULT
{
    UINT32 Size;
    UINT32 Attributes;
    UINT32 DataSize;
    _Field_size_bytes_(DataSize) BYTE Data[1];
} VARIABLE_GET_RESULT, *PVARIABLE_GET_RESULT;

//
// Parameter struct for Query
//

typedef struct _VARIABLE_QUERY_PARAM
{
    UINT32 Size;
    UINT32 Attributes;
} VARIABLE_QUERY_PARAM, *PVARIABLE_QUERY_PARAM;

typedef struct _VARIABLE_QUERY_RESULT
{
    UINT32 Size;
    UINT64 MaximumVariableStorageSize;
    UINT64 RemainingVariableStorageSize;
    UINT64 MaximumVariableSize;
} VARIABLE_QUERY_RESULT, *PVARIABLE_QUERY_RESULT;

//
// Parameter struct for querying fragmentation statistics.
//
// NonVolatileSpaceUsed - Space marked as used in non volatile storage
// IdealNonVolatileSpaceUsed - Space that should have ideally been used considering name, attributes, guids, timestamps, keys,
//                              data, datasize, etc (gives idea of total frag)
// IdealWithOverheadSpace - Above + contribution due to variable pointers and blob pointers (gives an idea of external frag)
//

typedef struct _VS_GET_FRAG_STATS
{
    UINT32 NonVolatileSpaceUsed;
    UINT32 IdealNonVolatileSpaceUsed;
    UINT32 IdealWithOverheadSpace;
} VS_GET_FRAG_STATS, *PVS_GET_FRAG_STATS;


//
// ISSUE-2013/06/14-ARNOLDP: Refactor Ntefi.h
//

#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE 0x00000040

#define EFI_KNOWN_ATTRIBUTES     (EFI_VARIABLE_NON_VOLATILE | \
                                  EFI_VARIABLE_BOOTSERVICE_ACCESS |\
                                  EFI_VARIABLE_RUNTIME_ACCESS |\
                                  EFI_VARIABLE_HARDWARE_ERROR_RECORD |\
                                  EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS |\
                                  EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |\
                                  EFI_VARIABLE_APPEND_WRITE)


#define VAR_LOG_STR_PARAM_MAX_LENGTH 31

#pragma pack(push, 1)
typedef struct _VAR_LOG_ENTRY {
    UINT32 StartTimeMs;
    UINT32 DurationMs;
    UINT32 Status;
    INT32 IntParam;
    UINT8 Operation;
    WCHAR StrParam[VAR_LOG_STR_PARAM_MAX_LENGTH];
    UINT32 CacheRebuildDurationMs;
    UINT8 CacheRebuildCount;
    UINT8 IsNV;
    UINT8 IsBS;
    UINT8 IsTimeAuthWrite;
    UINT8 IsAuthWrite;
    UINT8 IsAppendWrite;
    UINT8 IsHwError;
} VAR_LOG_ENTRY;
#pragma pack(pop)