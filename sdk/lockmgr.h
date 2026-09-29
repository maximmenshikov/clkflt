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
    lckmgr.h

Abstract:
    Lock Manager API for FSDs/File Systems.

Revision History:

--*/

#ifndef __LOCKMGR_H_
#define __LOCKMGR_H_

#include <windows.h>
#include "lockmgrtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// FSDMGR_InstallFileLock - Lock Manager implementation of LockFileEx
//

BOOL
FSDMGR_InstallFileLock(
    PACQUIREFILELOCKSTATE pAcquireFileLockState,
    PRELEASEFILELOCKSTATE pReleaseFileLockState,
    DWORD dwHandle,
    DWORD dwFlags,
    DWORD dwReserved,
    DWORD nNumberOfBytesToLockLow,
    DWORD nNumberOfBytesToLockHigh,
    LPOVERLAPPED lpOverlapped
    );

//
// FSDMGR_RemoveFileLock - Lock Manager implementation of UnlockFileEx
//

BOOL
FSDMGR_RemoveFileLock(
    PACQUIREFILELOCKSTATE pAcquireFileLockState,
    PRELEASEFILELOCKSTATE pReleaseFileLockState,
    DWORD dwHandle,
    DWORD dwReserved,
    DWORD nNumberOfBytesToLockLow,
    DWORD nNumberOfBytesToLockHigh,
    LPOVERLAPPED lpOverlapped
    );

//
// FSDMGR_RemoveFileLockEx - Lock Manager routine to unlock locks owned by handle
//

BOOL
FSDMGR_RemoveFileLockEx(
    PACQUIREFILELOCKSTATE pAcquireFileLockState,
    PRELEASEFILELOCKSTATE pReleaseFileLockState,
    DWORD dwHandle
    );

//
// FSDMGR_TestFileLock - Lock Manager routine to authorize read/write request
//

BOOL
FSDMGR_TestFileLock(
    PACQUIREFILELOCKSTATE pAcquireFileLockState,
    PRELEASEFILELOCKSTATE pReleaseFileLockState,
    DWORD dwHandle,
    BOOL fRead,
    DWORD cbReadWrite
    );

//
// FSDMGR_TestFileLockEx - Lock Manager routine to authorize read/write request
//

BOOL
FSDMGR_TestFileLockEx(
    PACQUIREFILELOCKSTATE pAcquireFileLockState,
    PRELEASEFILELOCKSTATE pReleaseFileLockState,
    DWORD dwHandle,
    BOOL fRead,
    DWORD cbReadWrite,
    DWORD dwOffsetLow,
    DWORD dwOffsetHigh
    );

//
// FSDMGR_EmptyLockContainer - Lock Manager routine to unblock threads waiting to install lock
//

VOID
FSDMGR_EmptyLockContainer(
    PFILELOCKSTATE pFileLockState
    );

#ifdef __cplusplus
}
#endif

#endif // __LOCKMGR_H_

