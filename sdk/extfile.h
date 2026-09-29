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
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

extfile.h

Abstract:  

Notes: 


--*/
#ifndef __EXTFILE_H__
#define __EXTFILE_H__

#define SHCNE_RENAMEITEM          0x00000001L
#define SHCNE_CREATE	          0x00000002L
#define SHCNE_DELETE	          0x00000004L
#define SHCNE_MKDIR	              0x00000008L
#define SHCNE_RMDIR               0x00000010L
#define SHCNE_MEDIAINSERTED       0x00000020L
#define SHCNE_MEDIAREMOVED        0x00000040L
#define SHCNE_DRIVEREMOVED        0x00000080L
#define SHCNE_DRIVEADD            0x00000100L
#define SHCNE_NETSHARE            0x00000200L
#define SHCNE_NETUNSHARE          0x00000400L
#define SHCNE_ATTRIBUTES          0x00000800L
#define SHCNE_UPDATEDIR           0x00001000L
#define SHCNE_UPDATEITEM          0x00002000L
#define SHCNE_SERVERDISCONNECT    0x00004000L
#define SHCNE_UPDATEIMAGE         0x00008000L
#define SHCNE_DRIVEADDGUI         0x00010000L
#define SHCNE_RENAMEFOLDER        0x00020000L
#define SHCNE_ASSOCCHANGED        0x08000000L

#define SHCNF_PATH        0x0001
#define SHCNF_FLUSHNOWAIT 0x2000
/*
typedef struct _FILECHANGEINFO {
	DWORD cbSize;              //  Size of this structure
	LONG wEventId;             //  File System Notification flags
	ULONG uFlags;              //  Flags
	DWORD dwItem1;            
	DWORD dwItem2;
	DWORD dwAttributes;        //  File attributes
	FILETIME ftModified;       //  Modified time
	ULONG nFileSize;           
} FILECHANGEINFO, * LPFILECHANGEINFO;
typedef const FILECHANGEINFO * LPCFILECHANGEINFO;
*/
typedef BOOL (* SHELLFILECHANGEFUNC_t)(LPCFILECHANGEINFO lpfci);


#define AFS_VERSION     0x00000004

#define AFS_ROOTNUM_STORECARD 0
#define AFS_ROOTNUM_NETWORK 1

// Number of API's in some system calls
#define NUM_FILE_APIS   16
#define NUM_FIND_APIS   3
#define NUM_AFS_APIS    22

// Common OID definitions
#define OID_FIRST_AFS           0
#define SYSTEM_MNTVOLUME		0xe

#define OIDFROMAFS(iAFS)        ((CEOID)((SYSTEM_MNTVOLUME<<28)|((iAFS)&0xfffffff)))
#define AFSFROMOID(oid)         ((DWORD)((oid)&0xfffffff))
#define INVALID_OID             ((CEOID)(INVALID_HANDLE_VALUE))

// NotifyMountedFS bit definitions
#define FSNOTIFY_POWER_ON       0x00000001
#define FSNOTIFY_POWER_OFF      0x00000002
#define FSNOTIFY_DEVICES_ON     0x00000004
#define FSNOTIFY_RESERVED       0xFFFFFFF8

#ifdef __cplusplus
extern "C" {
#endif


WINBASEAPI
BOOL
WINAPI
AFS_Unmount(
	HANDLE hFileSys
	);


WINBASEAPI
BOOL
WINAPI
AFS_CreateDirectoryW(
	HANDLE hFileSys,
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );


WINBASEAPI
BOOL
WINAPI
AFS_RemoveDirectoryW(
	HANDLE hFileSys,
    LPCWSTR lpPathName
    );


WINBASEAPI
DWORD
WINAPI
AFS_GetFileAttributesW(
	HANDLE hFileSys,
    LPCWSTR lpFileName
    );


WINBASEAPI
BOOL
WINAPI
AFS_SetFileAttributesW(
	HANDLE hFileSys,
	LPCWSTR lpszName,
	DWORD dwAttributes
	);


WINBASEAPI
HANDLE
WINAPI
AFS_CreateFileW(
	HANDLE hFileSys,
	HANDLE	hProc,
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );


WINBASEAPI
BOOL
WINAPI
AFS_DeleteFileW(
	HANDLE hFileSys,
    LPCWSTR lpFileName
    );


WINBASEAPI
BOOL
WINAPI
AFS_MoveFileW(
	HANDLE hFileSys,
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    );


WINBASEAPI
HANDLE
WINAPI
AFS_FindFirstFileW(
	HANDLE hFileSys,
	HANDLE hProc,
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    );

WINBASEAPI
BOOL
WINAPI
AFS_RegisterFileSystemFunction(
	HANDLE hFileSys,
	SHELLFILECHANGEFUNC_t pShellFunc
);

WINBASEAPI
BOOL
WINAPI
AFS_PrestoChangoFileName(
	HANDLE hFileSys,
	LPCWSTR lpszOldFile,
	LPCWSTR lpszNewFile
	);


WINBASEAPI
BOOL
WINAPI
AFS_CloseAllFileHandles(
	HANDLE hFileSys,
	HANDLE hProc
	);


WINBASEAPI
BOOL
WINAPI
AFS_GetDiskFreeSpace(
	HANDLE hFileSys,
	LPCWSTR lpPathName,
	LPDWORD lpSectorsPerCluster,
	LPDWORD lpBytesPerSector,
	LPDWORD lpFreeClusters,
	LPDWORD lpClusters
	);

WINBASEAPI
void
WINAPI
AFS_NotifyMountedFS(
	HANDLE hFileSys,
	DWORD flags
	);

WINBASEAPI
HANDLE
WINAPI
AFS_FindFirstChangeNotificationW(
    HANDLE hFileSys,
    HANDLE hProc,
    IN LPCWSTR lpPathName,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter
    );

WINBASEAPI
BOOL
AFS_FsIoControlW(
    HANDLE hFileSys,
    HANDLE hProc,
    DWORD dwIoctl,
    LPVOID lpInBuf,
    DWORD nInBufSize,
    LPVOID lpOutBuf,
    DWORD nOutBufSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    );

#ifdef WINCEMACRO
#include <mextfile.h>
#endif

#ifdef __cplusplus
}
#endif


#endif  // __EXTFILE_H__
