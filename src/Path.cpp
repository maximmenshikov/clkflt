/* FullUnlock v4.0 project.
   Cloaking Filter implementation.
   
   (C) ultrashot 2012
*/
#include "stdafx.h"
#include "clkflt.h"
#include "Cloaking.h"
#include "HashString.h"
#include "CFastCache.h"
#include "debug.h"
extern CFastCache *cache;

// Path-based file I/O APIs.

/**
 * Filter hook for CreateDirectoryW: create a directory on the underlying
 * volume.
 *
 * @param pvol                  Volume state.
 * @param pwsPathName           Directory path to create.
 * @param lpSecurityAttributes  Security attributes (unused on WinCE).
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_CreateDirectoryW(PVOLUME pvol, PCWSTR pwsPathName,
                        LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL fSuccess = FALSE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"CreateDirectoryW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpPathName=%s\r\n", pszFname, pState->hDsk,
                         pwsPathName));

    __try
    {
        fSuccess = pState->FilterHook.pCreateDirectoryW(
            pState->FilterHook.hVolume, pwsPathName, lpSecurityAttributes);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call CreateDirectoryW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for DeleteAndRenameFileW: delete pwsNewFile and rename
 * pwsOldFile to it in one operation.
 *
 * @param pvol         Volume state.
 * @param pwsOldFile   Source file that is renamed.
 * @param pwsNewFile   Destination file, deleted first if present.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_DeleteAndRenameFileW(PVOLUME pvol, PCWSTR pwsOldFile, PCWSTR pwsNewFile)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"DeleteAndRenameFileW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpszDestFile=%s lpszSourceFile=%s\r\n",
                         pszFname, pState->hDsk, pwsOldFile, pwsNewFile));

    __try
    {
        fSuccess = pState->FilterHook.pDeleteAndRenameFileW(
            pState->FilterHook.hVolume, pwsOldFile, pwsNewFile);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS,
                 (L"%s (%08X) Failed to call DeleteAndRenameFileW\r\n",
                  pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for DeleteFileW: delete a file on the underlying volume.
 *
 * @param pvol         Volume state.
 * @param pwsFileName  File to delete.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_DeleteFileW(PVOLUME pvol, PCWSTR pwsFileName)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"DeleteFileW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpFileName=%s\r\n", pszFname, pState->hDsk,
                         pwsFileName));

    __try
    {
        fSuccess = pState->FilterHook.pDeleteFileW(pState->FilterHook.hVolume,
                                                   pwsFileName);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call DeleteFileW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for GetDiskFreeSpaceW: report free-space geometry of the volume.
 *
 * @param pvol                Volume state.
 * @param pwsPathName         Path on the volume.
 * @param pSectorsPerCluster  Receives sectors per cluster.
 * @param pBytesPerSector     Receives bytes per sector.
 * @param pFreeClusters       Receives the free cluster count.
 * @param pClusters           Receives the total cluster count.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_GetDiskFreeSpaceW(PVOLUME pvol, PCWSTR pwsPathName,
                         PDWORD pSectorsPerCluster, PDWORD pBytesPerSector,
                         PDWORD pFreeClusters, PDWORD pClusters)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"GetDiskFreeSpaceW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) pwsPathName=%s\r\n", pszFname,
                         pState->hDsk, pwsPathName));

    __try
    {
        fSuccess = pState->FilterHook.pGetDiskFreeSpaceW(
            pState->FilterHook.hVolume, pwsPathName, pSectorsPerCluster,
            pBytesPerSector, pFreeClusters, pClusters);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded \r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call GetDiskFreeSpaceW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

bool IsAddToCacheEntry(LPWSTR pwsFileName);
/**
 * Filter hook for GetFileAttributesW: return a file's attributes, applying
 * cloak-list redirection to the queried name.
 *
 * @param pvol         Volume state.
 * @param pwsFileName  File to query.
 *
 * @return Attribute flags, or 0xFFFFFFFF on failure.
 */
DWORD
FILTER_GetFileAttributesW(PVOLUME pvol, PCWSTR pwsFileName)
{
    DWORD dwRet = 0;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"GetFileAttributesW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpFileName=%s\r\n", pszFname, pState->hDsk,
                         pwsFileName));

    if (wcsicmp((LPWSTR)pwsFileName, L"\\getcachelognow") == 0)
    {
        cache->DebugDumpFiles();
    }

    // addtocache
    if (IsAddToCacheEntry((LPWSTR)pwsFileName))
    {
        RETAILMSG(DEBUGLOG, (L"[CLKFLT-CACHE] Found -addtocache entry: %ls\r\n",
                             pwsFileName));
        wchar_t fname[256];
        if (wcslen(pwsFileName) < 256)
        {
            wcscpy(fname, pwsFileName);
        }
        else
        {
            wcsncpy(fname, pwsFileName, 255);
            fname[255] = L'\0';
        }
        wchar_t *t = wcsstr(fname, L"-addtocache");
        if (t)
            *t = L'\0';
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-CACHE] But we treat it as %ls\r\n", fname));
        DWORD dwFileNameHash = HashString(fname);
        cache->AddToCache(fname, dwFileNameHash, pState, GetCurrentProcess());
    }
    pwsFileName = Cloaking_GetNewPath((LPWSTR)pwsFileName);
    __try
    {
        dwRet = pState->FilterHook.pGetFileAttributesW(
            pState->FilterHook.hVolume, pwsFileName);

        if (0xFFFFFFFF != dwRet)
        {
            DEBUGMSG(ZONE_APIS, (L"%s (%08X) Succeeded returning %ld\r\n",
                                 pszFname, pState->hDsk, dwRet));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call GetFileAttributesW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return dwRet;
}

/**
 * Filter hook for MoveFileW: move or rename a file on the underlying volume.
 *
 * @param pvol            Volume state.
 * @param pwsOldFileName  Source path.
 * @param pwsNewFileName  Destination path.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_MoveFileW(PVOLUME pvol, PCWSTR pwsOldFileName, PCWSTR pwsNewFileName)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"MoveFileW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS,
             (L"%s (%08X) lpExistingFileName=%s lpNewFileName=%s\r\n", pszFname,
              pState->hDsk, pwsOldFileName, pwsNewFileName));

    __try
    {
        fSuccess = pState->FilterHook.pMoveFileW(
            pState->FilterHook.hVolume, pwsOldFileName, pwsNewFileName);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call MoveFileW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for volume change notifications forwarded by FSDMGR.
 *
 * @param pvol     Volume state.
 * @param dwFlags  Notification flags.
 */
VOID
FILTER_Notify(PVOLUME pvol, DWORD dwFlags)
{
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"Notify");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS,
             (L"%s (%08X) dwFlags=%08X\r\n", pszFname, pState->hDsk, dwFlags));

    __try
    {
        pState->FilterHook.pNotify(pState->FilterHook.hVolume, dwFlags);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call Notify\r\n", pszFname,
                             pState->hDsk));
    }

exit:;

    return;
}

/**
 * Filter hook for RegisterFileSystemFunction: register the shell file-change
 * callback with the underlying volume.
 *
 * @param pvol  Volume state.
 * @param pfn   Shell file-change callback.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_RegisterFileSystemFunction(PVOLUME pvol, SHELLFILECHANGEFUNC_t pfn)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"RegisterFileSystemFunction");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X)\r\n", pszFname, pState->hDsk));

    __try
    {
        fSuccess = pState->FilterHook.pRegisterFileSystemFunction(
            pState->FilterHook.hVolume, pfn);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS,
                 (L"%s (%08X) Failed to call RegisterFileSystemFunction\r\n",
                  pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for RemoveDirectoryW: remove a directory on the underlying
 * volume.
 *
 * @param pvol         Volume state.
 * @param pwsPathName  Directory to remove.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_RemoveDirectoryW(PVOLUME pvol, PCWSTR pwsPathName)
{
    BOOL fSuccess = FALSE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"RemoveDirectoryW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpPathName=%s\r\n", pszFname, pState->hDsk,
                         pwsPathName));

    __try
    {
        fSuccess = pState->FilterHook.pRemoveDirectoryW(
            pState->FilterHook.hVolume, pwsPathName);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call RemoveDirectoryW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for SetFileAttributesW: set the attributes of a file.
 *
 * @param pvol          Volume state.
 * @param pwsFileName   Target file.
 * @param dwAttributes  New attribute flags.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_SetFileAttributesW(PVOLUME pvol, PCWSTR pwsFileName, DWORD dwAttributes)
{
    BOOL fSuccess = TRUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"SetFileAttributesW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) lpFileName=%s dwFileAttributes=%08X\r\n",
                         pszFname, pState->hDsk, pwsFileName, dwAttributes));

    __try
    {
        fSuccess = pState->FilterHook.pSetFileAttributesW(
            pState->FilterHook.hVolume, pwsFileName, dwAttributes);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call SetFileAttributesW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}
