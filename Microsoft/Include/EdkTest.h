/** @file
*
*  Copyright (c) Microsoft Corporation. All rights reserved.
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
#ifndef __EDKTEST__
#define __EDKTEST__

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif // C_ASSERT

// The formatted print function used across the test
#define PRINTF_ Print

// Stringizing Macros

#define TO_WSTR(X) L## #X
#define WIDEN(X)   L## X

// Function Pointer Types

typedef VOID* PVOID;
typedef VOID (*CALLBACK_FUNC) (VOID);
typedef BOOLEAN (*PREDICATE_FUNC) (VOID);

typedef CALLBACK_FUNC TEST_FUNC;
typedef PREDICATE_FUNC SETUP_FUNC;
typedef CALLBACK_FUNC CLEANUP_FUNC;

typedef enum {
  TestLogError = 0,
  TestLogComment,
} TEST_LOG_LEVEL;

// Test Internal Datatypes

// Support up-to 32 test function per module
#define TEST_METADATA_MAX 32

typedef struct {
  CONST CHAR16    *Name;
  CONST CHAR16    *Desc;
  CALLBACK_FUNC   Func;
} TEST_METADATA;

typedef struct {
  CONST CHAR16    *Desc;
  SETUP_FUNC      ModuleSetupFunc;
  CLEANUP_FUNC    ModuleCleanupFunc;
  SETUP_FUNC      TestSetupFunc;
  CLEANUP_FUNC    TestCleanupFunc;
  UINT32          TestMetadataCount;
  TEST_METADATA   TestMetadata[TEST_METADATA_MAX];
} MODULE_METADATA;

typedef struct {
  UINT32                    TestPassedCount;
  UINT32                    TestFailedCount;
  UINT32                    TestSkippedCount;
  TEST_LOG_LEVEL            LoggingLevel;
  BASE_LIBRARY_JUMP_BUFFER  ExceptionEnv[1];
} MODULE_CONTEXT;

extern MODULE_METADATA gModuleMetadata__;
extern MODULE_CONTEXT gModuleContext__;
// The ARM high-performance counter frequency.
extern UINT64 gHpcTicksPerSeconds__;


// Test Exception Emulation

#define TEST_TRY_(ENV__)     if (!SetJump (ENV__))

#define TEST_CATCH_          else

#define TEST_TRHOW_(ENV__)   LongJump (ENV__, 1)

// Generic Helpers

#define LOG_MSG(LVL, ...) \
  do { \
    if (LVL <= gModuleContext__.LoggingLevel) { \
      PRINTF_ (__VA_ARGS__); \
    } \
  } while (FALSE)

// Logging

#define LOG_COMMENT(...) \
  do { \
    LOG_MSG (TestLogComment, L## __VA_ARGS__); \
  } while (FALSE)

#define LOG_ERROR(...) \
  do { \
    LOG_MSG (TestLogError, L## __VA_ARGS__); \
    LOG_MSG (TestLogError, L" [File: %a, Function: %a, Line: %d]", __FILE__, __FUNCTION__, __LINE__); \
  } while (FALSE)

#define SET_LOG_LEVEL(LVL) \
  do { \
    gModuleContext__.LoggingLevel = LVL; \
  } while (FALSE)

/** Gets the current high-performance counter tick count.

  The returned tick count is considered as a time stamp and can used later in
  elapsed time calculations.

  @retval Current high-performance counter tick count.
**/
__inline__
static
UINT64
HpcTimerStart (
  VOID
  )
{
  return GetPerformanceCounter ();
}

/** Calculates the elapsed milliseconds since a specific timer time stamp.

  @param[in] TimerStartTimestamp The high-performance counter tick count to use
  as the starting point in elapsed time calculation.

  @retval The milliseconds elapsed.
**/
__inline__
static
UINT64
HpcTimerElapsedMilliseconds (
  IN UINT64   TimerStartTimestamp
  )
{
  return (((GetPerformanceCounter () - TimerStartTimestamp) * 1000ULL) /
          gHpcTicksPerSeconds__);
}

// Internal Test Management

__inline__
static
VOID
RegisterTestFunc_ (
  MODULE_METADATA  *ModuleMetadata,
  CONST CHAR16*    Name,
  TEST_FUNC        Func
  )
{
  UINT32 CurrentIdx = ModuleMetadata->TestMetadataCount;

  if (CurrentIdx >= TEST_METADATA_MAX) {
    LOG_MSG (
      TestLogError,
      L"Number of test functions can't exceed %d. "
      L"Skipping %s registration\n",
      TEST_METADATA_MAX,
      Name);
    return;
  }

  ModuleMetadata->TestMetadata[CurrentIdx].Name = Name;
  ModuleMetadata->TestMetadata[CurrentIdx].Func = Func;

  ModuleMetadata->TestMetadataCount += 1;
}

__inline__
static
VOID
RunTestFunc_ (
  CONST MODULE_METADATA   *ModuleMetadata,
  CONST TEST_METADATA     *TestMetadata,
  MODULE_CONTEXT          *ModuleContext
  )
{
  UINT64 TimerStart = 0;
  UINT64 ElapsedMs;

  if (ModuleMetadata->TestSetupFunc != NULL) {
    LOG_MSG (TestLogComment, L"[Test Setup]\n");
    if (!ModuleMetadata->TestSetupFunc ()) {
      ModuleContext->TestSkippedCount += 1;
      LOG_MSG (TestLogError,  L"Test Setup Failed. Skipping Test\n");
      return;
    }
  }

  TEST_TRY_ (ModuleContext->ExceptionEnv) {
    LOG_MSG (TestLogComment, L"\n--> Test Enter: %s\n", TestMetadata->Name);
    ModuleContext->LoggingLevel = TestLogComment;
    TimerStart = HpcTimerStart ();
    TestMetadata->Func ();
    ElapsedMs = HpcTimerElapsedMilliseconds (TimerStart);
    ModuleContext->LoggingLevel = TestLogComment;
    LOG_MSG (
      TestLogComment,
      L"<-- Test Exit: %s (%lldms)\n",
      TestMetadata->Name,
      ElapsedMs);

    ModuleContext->TestPassedCount += 1;
    LOG_MSG (TestLogComment, L"*** %s: [PASSED] ***\n", TestMetadata->Name);
  } TEST_CATCH_ {
    ElapsedMs = HpcTimerElapsedMilliseconds (TimerStart);
    ModuleContext->LoggingLevel = TestLogComment;
    LOG_MSG (
      TestLogComment,
      L"<-- Test Exit: %s (%lldms)\n",
      TestMetadata->Name,
      ElapsedMs);

    ModuleContext->TestFailedCount += 1;
    LOG_MSG (TestLogComment, L"***  %s: [FAILED] ***\n", TestMetadata->Name);
  }

  if (ModuleMetadata->TestCleanupFunc != NULL) {
    LOG_MSG (TestLogComment,L"[Test Cleanup]\n");
    ModuleMetadata->TestCleanupFunc ();
  }
}

__inline__
static
BOOLEAN
RunModule_ (
  INT32                   Argc,
  CONST CHAR8             **Argv,
  CONST MODULE_METADATA   *ModuleMetadata,
  MODULE_CONTEXT          *ModuleContext
  )
{
  UINT32 TestIdx;

  gHpcTicksPerSeconds__ = GetPerformanceCounterProperties (NULL, NULL);
  ASSERT (gHpcTicksPerSeconds__ != 0);

  if (ModuleMetadata->TestMetadataCount >= TEST_METADATA_MAX) {
    LOG_MSG (TestLogError, L"Number of registered test functions can't exceed %d\n", TEST_METADATA_MAX);
    return FALSE;
  }

  if (ModuleMetadata->ModuleSetupFunc != NULL) {
    LOG_MSG (TestLogComment, L"[Module Setup]\n");
    if (!ModuleMetadata->ModuleSetupFunc ()) {
      LOG_MSG (TestLogError, L"Module Setup Failed\n");
      return FALSE;
    }
  }

  for (TestIdx = 0; TestIdx < ModuleMetadata->TestMetadataCount; ++TestIdx) {
    RunTestFunc_ (ModuleMetadata, ModuleMetadata->TestMetadata + TestIdx, ModuleContext);
  }
  
  if (ModuleMetadata->ModuleCleanupFunc != NULL) {
    LOG_MSG (TestLogComment, L"[Module Cleanup]\n");
    ModuleMetadata->ModuleCleanupFunc ();
  }

  LOG_MSG (
    TestLogComment,
    L"\nSummary: Total = %d, Passed = %d, Failed = %d, Skipped = %d\n\n",
    ModuleMetadata->TestMetadataCount,
    ModuleContext->TestPassedCount,
    ModuleContext->TestFailedCount,
    ModuleContext->TestSkippedCount);

  return TRUE;
}

#define MODULE(DESC__) \
  MODULE_METADATA gModuleMetadata__ = { \
    WIDEN (DESC__), \
    NULL, \
    NULL, \
    0 }; \
  MODULE_CONTEXT gModuleContext__ = { \
    0, \
    0, \
    0, \
    TestLogComment }; \
  UINT64 gHpcTicksPerSeconds__ = 0;


#define TEST_FUNC(NAME__) \
  RegisterTestFunc_ (&gModuleMetadata__, TO_WSTR(NAME__), NAME__)

#define MODULE_SETUP(NAME__) \
  do { \
    gModuleMetadata__.ModuleSetupFunc = NAME__; \
  } while (FALSE)

#define MODULE_CLEANUP(NAME__) \
  do { \
    gModuleMetadata__.ModuleCleanupFunc = NAME__; \
  } while (FALSE)

#define TEST_SETUP(NAME__) \
  do { \
    gModuleMetadata__.TestSetupFunc = NAME__; \
  } while (FALSE)
  
#define TEST_CLEANUP(NAME__) \
  do { \
    gModuleMetadata__.TestCleanupFunc = NAME__; \
  } while (FALSE)

#define RUN_MODULE(ARGC__, ARGV__) \
  RunModule_ (ARGC__, (CONST CHAR8 **)ARGV__, &gModuleMetadata__, &gModuleContext__)

// Verify Predicates

#define ARE_EQUAL_PRED_(LEFT__, RIGHT__)          ((LEFT__) == (RIGHT__))
#define SUCCEEDED_PRED_(STATUS__)                 (!EFI_ERROR(STATUS__))
#define IS_TRUE_PRED_(VALUE__)                    ((VALUE__) != FALSE)
#define IS_LESS_THAN_PRED_(LESSER__, GREATER__)   ((LESSER__) < (GREATER__))

// Verify Helpers

#define PRINT_VERIFY_MSG_(LVL, FMT, ...) \
  do { \
    if (!IsEmptyString (L## FMT)) { \
      PRINT_MSG_HELPER_ (LVL, L": " L## FMT, __VA_ARGS__); \
    } \
  } while (FALSE)

#define VERIFY_BINARY_PREDICATE_HELPER_(T__, PRED__, PRED_NAME__, EXPECTED__, ACTUAL__, EXPECTED_PRED_RESULT__, ...) \
  do { \
    T__ ExpectedVal__ = (T__) EXPECTED__; \
    T__ ActualVal__ = (T__) ACTUAL__; \
    BOOLEAN ActualPredResult__ = PRED__ (ExpectedVal__, ActualVal__); \
    BOOLEAN LocalVerifyResult__ = (ActualPredResult__ == EXPECTED_PRED_RESULT__); \
    if (!LocalVerifyResult__) { \
      LOG_MSG (TestLogError, L"ERROR VERIFY: " WIDEN(PRED_NAME__) L"(" TO_WSTR(EXPECTED__) L", " TO_WSTR(ACTUAL__) L"): "); \
      LOG_MSG (TestLogError, __VA_ARGS__); \
      LOG_MSG (TestLogError, L". [File: %a, Function: %a, Line: %d]\n", __FILE__, __FUNCTION__, __LINE__); \
      TEST_TRHOW_ (gModuleContext__.ExceptionEnv); \
    } else { \
      LOG_MSG (TestLogComment, L"VERIFY: " WIDEN(PRED_NAME__) L"(" TO_WSTR(EXPECTED__) L", " TO_WSTR(ACTUAL__) L"): "); \
      LOG_MSG (TestLogComment, __VA_ARGS__); \
      LOG_MSG (TestLogComment, L"\n"); \
    } \
  } while (FALSE)

#define VERIFY_UNARY_PREDICATE_HELPER_(T__, PRED__, PRED_NAME__, ACTUAL__, EXPECTED_PRED_RESULT__, ...) \
  do { \
    T__ ActualVal__ = (T__) ACTUAL__; \
    BOOLEAN ActualPredResult__ = PRED__(ActualVal__); \
    BOOLEAN LocalVerifyResult__ = (ActualPredResult__ == EXPECTED_PRED_RESULT__); \
    if (!LocalVerifyResult__) { \
      LOG_MSG (TestLogError, L"ERROR: VERIFY: " WIDEN(PRED_NAME__) L"(" TO_WSTR(ACTUAL__) L"): "); \
      LOG_MSG (TestLogError, __VA_ARGS__); \
      LOG_MSG (TestLogError, L". [File: %a, Function: %a, Line: %d]\n", __FILE__, __FUNCTION__, __LINE__); \
      TEST_TRHOW_ (gModuleContext__.ExceptionEnv); \
    } else { \
      LOG_MSG (TestLogComment, L"VERIFY: " WIDEN(PRED_NAME__) L"(" TO_WSTR(ACTUAL__) L")"); \
      LOG_MSG (TestLogComment, __VA_ARGS__); \
      LOG_MSG (TestLogComment, L"\n"); \
    } \
  } while (FALSE)

// Verify Macros

#define VERIFY_ARE_EQUAL(T__, EXPECTED__, ACTUAL__, ...) \
  VERIFY_BINARY_PREDICATE_HELPER_ ( \
    T__, \
    ARE_EQUAL_PRED_, \
    "ARE_EQUAL", \
    EXPECTED__, \
    ACTUAL__, \
    TRUE, \
    L"" __VA_ARGS__, "")

#define VERIFY_ARE_NOT_EQUAL(T__, EXPECTED__, ACTUAL__, ...) \
  VERIFY_BINARY_PREDICATE_HELPER_ ( \
    T__, \
    ARE_EQUAL_PRED_, \
    "ARE_NOT_EQUAL", \
    EXPECTED__, \
    ACTUAL__ , \
    FALSE, \
    L"" __VA_ARGS__, "")

#define VERIFY_IS_LESS_THAN(T__, LESSER__, GREATER__, ...) \
  VERIFY_BINARY_PREDICATE_HELPER_ ( \
    T__, \
    IS_LESS_THAN_PRED_, \
    "IS_LESS_THAN", \
    LESSER__, \
    GREATER__ , \
    TRUE, \
    L"" __VA_ARGS__, "")

#define VERIFY_SUCCEEDED(STATUS__, ...) \
  VERIFY_UNARY_PREDICATE_HELPER_ ( \
    EFI_STATUS, \
    SUCCEEDED_PRED_, \
    "SUCCEEDED", \
    STATUS__, \
    TRUE, \
    L"" __VA_ARGS__, "")

#define VERIFY_FAILED(STATUS__, ...) \
  VERIFY_UNARY_PREDICATE_HELPER_ ( \
    EFI_STATUS, \
    SUCCEEDED_PRED_, \
    "FAILED", \
    STATUS__, \
    FALSE, \
    L"" __VA_ARGS__, "")

#define VERIFY_IS_TRUE(CONDITION__, ...) \
  VERIFY_UNARY_PREDICATE_HELPER_ ( \
    BOOLEAN, \
    IS_TRUE_PRED_, \
    "IS_TRUE", \
    CONDITION__, \
    TRUE, \
    L"" __VA_ARGS__, "")

#define VERIFY_IS_FALSE(CONDITION__, ...) \
  VERIFY_UNARY_PREDICATE_HELPER_ ( \
    BOOLEAN, \
    IS_TRUE_PRED_, \
    "IS_FALSE", \
    CONDITION__, \
    FALSE, \
    L"" __VA_ARGS__, "")

#define VERIFY_IS_NULL(VALUE__, ...) \
  VERIFY_BINARY_PREDICATE_HELPER_ ( \
    PVOID, \
    ARE_EQUAL_PRED_, \
    "IS_NULL", \
    VALUE__, \
    NULL, \
    TRUE, \
    L"" __VA_ARGS__, "")

#define VERIFY_IS_NOT_NULL(VALUE__, ...) \
  VERIFY_BINARY_PREDICATE_HELPER_ ( \
    PVOID, \
    ARE_EQUAL_PRED_, \
    "IS_NOT_NULL", \
    VALUE__, \
    NULL, \
    FALSE, \
    L"" __VA_ARGS__, "")

#endif // __EDKTEST__

