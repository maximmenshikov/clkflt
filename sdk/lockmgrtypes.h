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
    lckmgrtypes.h

Abstract:
    Lock Manager Types.

Revision History:

--*/

#include <windows.h>

#ifndef __LCKMGRTYPES_H_
#define __LCKMGRTYPES_H_

//
// FILELOCKSTATE - Sub-file lock management data
//

struct _FILELOCKSTATE;
typedef struct _FILELOCKSTATE {
    DWORD dwPosLow;
    DWORD dwPosHigh;
    DWORD dwAccess;
    LPCRITICAL_SECTION lpcs; // protect fTerminal, hevUnlock, cQueue, pvLockContainer
    BOOL fTerminal;
    HANDLE hevUnlock;
    DWORD cQueue;
    PVOID pvLockContainer;
} FILELOCKSTATE, * PFILELOCKSTATE;

//
// PEMPTYLOCKCONTAINER - Used by FSDMGR_OpenFileLockState, FSDMGR_CloseFileLockState
//

typedef
VOID
(*PEMPTYLOCKCONTAINER)(
    PFILELOCKSTATE pFileLockState
    );

//
// PACQUIREFILELOCKSTATE - Implemented by FSD, used by Lock Manager to get the file/lock
// state associated with the specified file; this function is required to enter the file
// lock state's critical section
//

typedef
BOOL
(*PACQUIREFILELOCKSTATE)(
    DWORD dwFile,
    PFILELOCKSTATE *ppFileLockState
    );

//
// PRELEASEFILELOCKSTATE - Implemented by FSD, used by Lock Manager to return the file/lock
// state associated with the specified file; this function is required to exit the file
// lock state's critical section
//

typedef
BOOL
(*PRELEASEFILELOCKSTATE)(
    DWORD dwFile,
    PFILELOCKSTATE *ppFileLockState
    );

#endif // __LCKMGRTYPES_H_
