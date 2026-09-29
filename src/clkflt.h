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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//

#ifndef __CLKFLT_H__
#define __CLKFLT_H__

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#include <windows.h>
#include <tchar.h>
#include <types.h>
#include <excpt.h>
#include <memory.h>
#include <fsdmgr.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Volume abstraction.
//  Add additional members to this structure to manage additional properties
//  of a volume.
typedef struct
{
    HDSK hDsk;
    FILTERHOOK FilterHook;
} VolumeState;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Handle abstraction.
//  Add additional members to this structure to manage additional properties
//  of a file.
typedef struct
{
    DWORD h;
    HDSK hDsk;
    PFILTERHOOK pHook;
    DWORD dwFileNameHash;
    wchar_t wsFileName[256];
    LONG lFilePosition;
    DWORD dwFileSize;
    BOOL fWasRemovedFromCache;
    VolumeState *pVolumeState;
} HandleState;

#endif // __CLKFLT_H__
