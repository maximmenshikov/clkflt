//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

/*++

Module Name:
    lckmgrhelp.h

Abstract:
    Lock Manager Help Functions for FSDs.

Revision History:

--*/

#ifndef __LOCKMGRHELP_H_
#define __LOCKMGRHELP_H_

#include <windows.h>
#include "lockmgrtypes.h"

//
// FSDMGR_OpenFileLockState - Used by FSD to initialize file lock state
//

#define FSDMGR_OpenFileLockState(pFileLockState) \
    do{ \
        (pFileLockState)->hevUnlock = NULL; \
        (pFileLockState)->fTerminal = FALSE; \
        (pFileLockState)->cQueue = 0; \
        (pFileLockState)->pvLockContainer = NULL; \
        (pFileLockState)->lpcs = (LPCRITICAL_SECTION)LocalAlloc(LPTR, sizeof(CRITICAL_SECTION)); \
        if (NULL != (pFileLockState)->lpcs) { \
            InitializeCriticalSection((pFileLockState)->lpcs); \
        }\
    }while(0)

//
// FSDMGR_CloseFileLockState - Used by FSD to deinitialize file lock state
//

#define FSDMGR_CloseFileLockState(pFileLockState, pfnEmptyLockContainer) \
    do {\
        DEBUGCHK(NULL != (pFileLockState)); \
        if (NULL != (pFileLockState)->pvLockContainer) { \
            if (NULL != (pfnEmptyLockContainer)) { \
                __try { \
                    (*(pfnEmptyLockContainer))(pFileLockState); \
                } __except(EXCEPTION_EXECUTE_HANDLER) { \
                    DEBUGCHK(0); \
                } \
            } \
        } \
        if (NULL != (pFileLockState)->hevUnlock) { \
            CloseHandle((pFileLockState)->hevUnlock); \
        } \
        if (NULL != (pFileLockState)->lpcs) { \
            DeleteCriticalSection((pFileLockState)->lpcs); \
            LocalFree((HLOCAL)(pFileLockState)->lpcs); \
        } \
    }while(0)

#endif // __LOCKMGRHELP_H_

