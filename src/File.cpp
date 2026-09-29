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

CFastCache *cache = NULL;

/**
 * Lazily allocate the global file-content cache on first use.
 */
inline void
EnsureCacheLoaded()
{
    if (cache == NULL)
        cache = new CFastCache();
}
int ticks = 0;

// Handle-based file I/O APIs.

/**
 * Filter hook for CloseFile: close the file, re-add it to the cache if it was
 * evicted for writing, and free the wrapper HandleState.
 *
 * @param pfh    File handle wrapper.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_CloseFile(PFILE pfh)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"CloseFile");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG,
              (L"%s (%08X) hFile=%08X\r\n", pszFname, pHandle->hDsk, pfh));

    __try
    {
        fSuccess = pHandle->pHook->pCloseFile(pHandle->h);

        if (pHandle->fWasRemovedFromCache)
        {
            // lets cache it again.
            cache->AddToCache(pHandle->wsFileName, pHandle->dwFileNameHash,
                              pHandle->pVolumeState, GetCurrentProcess());
        }
        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));

            delete pHandle;
            pHandle = NULL;
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call CloseFile\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for CreateFileW: apply cloak-list redirection to the name, open
 * the file on the underlying volume, wrap it in a HandleState, and evict any
 * cached copy when the file is opened for writing.
 *
 * @param pvol                  Volume state.
 * @param hProc                 Handle of the calling process.
 * @param lpFileName            File name (redirected via the cloak list).
 * @param dwAccess              Desired access mask.
 * @param dwShareMode           Share mode.
 * @param lpSecurityAttributes  Security attributes.
 * @param dwCreate              Creation disposition.
 * @param dwFlagsAndAttributes  Flags and attributes.
 * @param hTemplateFile         Template file handle.
 *
 * @return Wrapped file handle, or INVALID_HANDLE_VALUE on failure.
 */
HANDLE
FILTER_CreateFileW(PVOLUME pvol, HANDLE hProc, LPCWSTR lpFileName,
                   DWORD dwAccess, DWORD dwShareMode,
                   LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreate,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    lpFileName = Cloaking_GetNewPath((wchar_t *)lpFileName);

    EnsureCacheLoaded();
    //RegistrySetDWORD7((DWORD)HKEY_CURRENT_USER, L"Software", L"MyTest", 555);
    HANDLE hFile = INVALID_HANDLE_VALUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"CreateFileW");

    if (!pState)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hProc=%08X lpFileName=%s dwDesiredAccess=%08X dwShareMode=%08X dwCreationDisposition=%08X dwFlagsAndAttributes=%08X\r\n",
         pszFname, pState->hDsk, hProc, lpFileName, dwAccess, dwShareMode,
         dwCreate, dwFlagsAndAttributes));

    __try
    {
        hFile = pState->FilterHook.pCreateFileW(
            pState->FilterHook.hVolume, hProc, lpFileName, dwAccess,
            dwShareMode, lpSecurityAttributes, dwCreate, dwFlagsAndAttributes,
            hTemplateFile);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();
            HandleState *pHandle = new HandleState;
            if (pHandle)
            {
                pHandle->h = (DWORD)hFile;
                pHandle->hDsk = pState->hDsk;
                pHandle->pHook = &pState->FilterHook;
                if (wcslen(lpFileName) < 256)
                {
                    wcscpy(pHandle->wsFileName, lpFileName);
                }
                else
                {
                    wcsncpy(pHandle->wsFileName, lpFileName, 255);
                    pHandle->wsFileName[255] = L'\0';
                }
                pHandle->dwFileNameHash = HashString(pHandle->wsFileName);
                pHandle->fWasRemovedFromCache = FALSE;
                pHandle->lFilePosition = 0;
                pHandle->dwFileSize =
                    pState->FilterHook.pGetFileSize((DWORD)hFile, NULL);
                pHandle->pVolumeState = pState;

                if (ticks == 0)
                    ticks = GetTickCount();
                if ((GetTickCount() - ticks) >= 120000)
                    RETAILMSG(
                        DEBUGLOG,
                        (L"[CLKFLT-DBG] CreateFile(name = %ls, hash = %X, access = %X)\r\n",
                         pHandle->wsFileName, pHandle->dwFileNameHash,
                         dwAccess));
                RETAILMSG(
                    DEBUGLOG,
                    (L"%s (%08X) Succeeded with handle %08X returning %08X\r\n",
                     pszFname, pState->hDsk, hFile, pHandle));

                if (dwAccess & GENERIC_WRITE)
                {
                    // removing item from cache since it is about to be written
                    FAST_CACHE_FILE_ENTRY *entry =
                        cache->GetCacheEntry(pHandle->dwFileNameHash);
                    if (entry)
                    {
                        cache->RemoveFromCache(entry);
                        pHandle->fWasRemovedFromCache = TRUE;
                    }
                }
                hFile = (HANDLE)pHandle;
                SetLastError(err);
            }
            else
            {
                RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to allocate handle\r\n",
                                     pszFname, pState->hDsk));

                hFile = INVALID_HANDLE_VALUE;
            }
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call CreateFileW\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return hFile;
}

/**
 * Filter hook for DeviceIoControl on an open file handle.
 *
 * @param pfh              File handle wrapper.
 * @param dwIoControlCode  Control code.
 * @param lpInBuf          Input buffer.
 * @param nInBufSize       Input buffer size, in bytes.
 * @param lpOutBuf         Output buffer.
 * @param nOutBufSize      Output buffer size, in bytes.
 * @param lpBytesReturned  Receives the number of bytes returned.
 * @param lpOverlapped     Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_DeviceIoControl(PFILE pfh, DWORD dwIoControlCode, LPVOID lpInBuf,
                       DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize,
                       LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"DeviceIoControl");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG, (L"%s (%08X) hFile=%08X dwIoControlCode=%08X"
                         L" lpInBuffer=%08X nInBufferSize=%d"
                         L" lpOutBuffer=%08X nOutBufferSize=%d"
                         L" lpBytesReturned=%08X\r\n",
                         pszFname, pHandle->hDsk, pfh, dwIoControlCode, lpInBuf,
                         nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned));

    if (dwIoControlCode == 0x940BC &&
        cache->GetCacheEntry(pHandle->dwFileNameHash))
    {
        RETAILMSG(DEBUGLOG, (L"[CLKFLT-CACHE] SUPRESSING CACHEFILT for %ls\r\n",
                             pHandle->wsFileName));
        return FALSE;
    }
    __try
    {
        fSuccess = pHandle->pHook->pDeviceIoControl(
            pHandle->h, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf,
            nOutBufSize, lpBytesReturned, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call DeviceIoControl\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for FlushFileBuffers: flush an open file to the volume.
 *
 * @param pfh    File handle wrapper.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_FlushFileBuffers(PFILE pfh)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"FlushFileBuffers");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG,
              (L"%s (%08X) hFile=%08X\r\n", pszFname, pHandle->hDsk, pfh));

    __try
    {
        fSuccess = pHandle->pHook->pFlushFileBuffers(pHandle->h);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call FlushFileBuffers\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for FsIoControl: volume-level device control.
 *
 * @param pvol             Volume state.
 * @param dwIoControlCode  Control code.
 * @param lpInBuf          Input buffer.
 * @param nInBufSize       Input buffer size, in bytes.
 * @param lpOutBuf         Output buffer.
 * @param nOutBufSize      Output buffer size, in bytes.
 * @param lpBytesReturned  Receives the number of bytes returned.
 * @param lpOverlapped     Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_FsIoControl(PVOLUME pvol, DWORD dwIoControlCode, LPVOID lpInBuf,
                   DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize,
                   LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"GetVolumeInfo");

    if (!pState)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) dwIoControlCode=%08X lpInBuf=%08X nInBufSize=%ld lpOutBuf=%08X nOutBufSize=%ld lpBytesReturned=%08X\r\n",
         pszFname, pState->hDsk, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf,
         nOutBufSize, lpBytesReturned));

    __try
    {
        fSuccess = pState->FilterHook.pFsIoControl(
            pState->FilterHook.hVolume, dwIoControlCode, lpInBuf, nInBufSize,
            lpOutBuf, nOutBufSize, lpBytesReturned, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call FsIoControl\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for GetFileInformationByHandle.
 *
 * @param pfh         File handle wrapper.
 * @param lpFileInfo  Receives the file information.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_GetFileInformationByHandle(PFILE pfh,
                                  LPBY_HANDLE_FILE_INFORMATION lpFileInfo)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"GetFileInformationByHandle");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG, (L"%s (%08X) hFile=%08X lpFileInfo=%08X\r\n", pszFname,
                         pHandle->hDsk, pfh, lpFileInfo));

    __try
    {
        fSuccess =
            pHandle->pHook->pGetFileInformationByHandle(pHandle->h, lpFileInfo);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded returning dwAttributes=%ld\r\n",
                       pszFname, pHandle->hDsk, lpFileInfo->dwFileAttributes));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG,
                  (L"%s (%08X) Failed to call GetFileInformationByHandle\r\n",
                   pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for GetFileSize.
 *
 * @param pfh            File handle wrapper.
 * @param lpFileSizeHigh Receives the high 32 bits of the size, or NULL.
 *
 * @return Low 32 bits of the file size, or INVALID_FILE_SIZE on failure.
 */
DWORD
FILTER_GetFileSize(PFILE pfh, LPDWORD lpFileSizeHigh)
{
    DWORD dwRet = 0;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"GetFileSize");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG, (L"%s (%08X) hFile=%08X lpFileSizeHigh=%08X\r\n",
                         pszFname, pHandle->hDsk, pfh, lpFileSizeHigh));

    __try
    {
        dwRet = pHandle->pHook->pGetFileSize(pHandle->h, lpFileSizeHigh);

        RETAILMSG(DEBUGLOG, (L"%s (%08X) Returning %ld\r\n", pszFname,
                             pHandle->hDsk, dwRet));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call GetFileSize\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return dwRet;
}

/**
 * Filter hook for GetFileTime.
 *
 * @param pfh           File handle wrapper.
 * @param lpCreation    Receives the creation time, or NULL.
 * @param lpLastAccess  Receives the last-access time, or NULL.
 * @param lpLastWrite   Receives the last-write time, or NULL.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_GetFileTime(PFILE pfh, LPFILETIME lpCreation, LPFILETIME lpLastAccess,
                   LPFILETIME lpLastWrite)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"GetFileTime");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lpCreationTime=%08X lpLastAccessTime=%08X lpLastWriteTime=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, lpCreation, lpLastAccess, lpLastWrite));

    __try
    {
        fSuccess = pHandle->pHook->pGetFileTime(pHandle->h, lpCreation,
                                                lpLastAccess, lpLastWrite);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call GetFileTime\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for GetVolumeInfo.
 *
 * @param pvol   Volume state.
 * @param pInfo  Receives the volume information.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_GetVolumeInfo(PVOLUME pvol, FSD_VOLUME_INFO *pInfo)
{
    BOOL fSuccess = FALSE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"GetVolumeInfo");

    if (!pState)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG,
              (L"%s (%08X) pInfo=%08X\r\n", pszFname, pState->hDsk, pInfo));

    __try
    {
        fSuccess = pState->FilterHook.pGetVolumeInfo(pState->FilterHook.hVolume,
                                                     pInfo);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pState->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pState->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call GetVolumeInfo\r\n",
                             pszFname, pState->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for LockFileEx: lock a byte range of an open file.
 *
 * @param pfh                        File handle wrapper.
 * @param dwFlags                    Lock flags.
 * @param dwReserved                 Reserved, must be zero.
 * @param nNumberOfBytesToLockLow    Low 32 bits of the range length.
 * @param nNumberOfBytesToLockHigh   High 32 bits of the range length.
 * @param lpOverlapped               Overlapped context giving the offset.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_LockFileEx(PFILE pfh, DWORD dwFlags, DWORD dwReserved,
                  DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh,
                  LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"LockFileEx");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"[CLKFLT-DBG] %s (%08X) hFile=%08X dwFlags=%08X dwReserved=%08X nNumberOfBytesToLockLow=%ld nNumberOfBytesToLockHigh=%ld\r\n",
         pszFname, pHandle->hDsk, pfh, dwFlags, dwReserved,
         nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh));

    __try
    {
        fSuccess = pHandle->pHook->pLockFileEx(
            pHandle->h, dwFlags, dwReserved, nNumberOfBytesToLockLow,
            nNumberOfBytesToLockHigh, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG, (L"[CLKFLT-DBG] %s (%08X) Succeeded\r\n",
                                 pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG, (L"[CLKFLT-DBG] %s (%08X) Failed\r\n", pszFname,
                                 pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s (%08X) Failed to call LockFileEx\r\n",
                   pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for ReadFile.
 *
 * @param pfh             File handle wrapper.
 * @param buffer          Destination buffer.
 * @param nBytesToRead    Number of bytes to read.
 * @param lpNumBytesRead  Receives the number of bytes read.
 * @param lpOverlapped    Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_ReadFile(PFILE pfh, LPVOID buffer, DWORD nBytesToRead,
                LPDWORD lpNumBytesRead, LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"ReadFile");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    if (ticks == 0)
        ticks = GetTickCount();
    if ((GetTickCount() - ticks) >= 120000)
        RETAILMSG(
            DEBUGLOG,
            (L"[CLKFLT-DBG] %s(%ls, buffer=%08X, position = %d, bytesToRead=%d)\r\n",
             pszFname, pHandle->wsFileName, buffer, pHandle->lFilePosition,
             nBytesToRead));
    /*
    if (FAST_CACHE_FILE_ENTRY *entry = cache->GetCacheEntry(pHandle->dwFileNameHash))
	{
		DWORD bRead = 0;
		cache->GetFileData(entry, (LPBYTE)buffer, pHandle->lFilePosition, nBytesToRead, &bRead);
		if (lpNumBytesRead)
			*lpNumBytesRead = bRead;
		pHandle->lFilePosition += bRead;	
		SetLastError(NO_ERROR);
		return TRUE;
	}
    */
    RETAILMSG(
        DEBUGLOG,
        (L"[CLKFLT-DBG] %s(%08X) hFile=%08X lpBuffer=%08X nNumberOfBytesToRead=%ld lpNumberOfBytesRead=%08X \r\n",
         pszFname, pHandle->hDsk, pfh, buffer, nBytesToRead, lpNumBytesRead));

    __try
    {
        fSuccess = pHandle->pHook->pReadFile(pHandle->h, buffer, nBytesToRead,
                                             lpNumBytesRead, lpOverlapped);

        if (fSuccess)
        {
            pHandle->lFilePosition += *lpNumBytesRead;
            RETAILMSG(
                DEBUGLOG,
                (L"[CLKFLT-DBG] %s (%08X) Succeeded returning %ld bytes\r\n",
                 pszFname, pHandle->hDsk,
                 lpNumBytesRead ? *lpNumBytesRead : 0));
        }
        else
        {
            RETAILMSG(DEBUGLOG, (L"[CLKFLT-DBG] %s (%08X) Failed\r\n", pszFname,
                                 pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s (%08X) Failed to call ReadFile\r\n",
                   pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for ReadFileScatter: scattered read into page-sized segments.
 *
 * @param pfh                   File handle wrapper.
 * @param aSegmentArray         Array of page-aligned destination segments.
 * @param nNumberOfBytesToRead  Total number of bytes to read.
 * @param lpReserved            Reserved.
 * @param lpOverlapped          Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_ReadFileScatter(PFILE pfh, FILE_SEGMENT_ELEMENT aSegmentArray[],
                       DWORD nNumberOfBytesToRead, LPDWORD lpReserved,
                       LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"ReadFileScatter");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    if (ticks == 0)
        ticks = GetTickCount();
    if ((GetTickCount() - ticks) >= 120000)
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s (%ls, bytesToRead=%d)\r\n", pszFname,
                   pHandle->wsFileName, nNumberOfBytesToRead));

    RETAILMSG(
        DEBUGLOG,
        (L"[CLKFLT-DBG] %s (%08X) hFile=%08X aSegmentArray=%08X nNumberOfBytesToRead=%ld lpReserved=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, aSegmentArray, nNumberOfBytesToRead,
         lpReserved));

    __try
    {
        fSuccess = pHandle->pHook->pReadFileScatter(pHandle->h, aSegmentArray,
                                                    nNumberOfBytesToRead,
                                                    lpReserved, lpOverlapped);

        pHandle->lFilePosition += nNumberOfBytesToRead;

        if (fSuccess)
        {
            RETAILMSG(
                DEBUGLOG,
                (L"[CLKFLT-DBG] %s (%08X) Succeeded returning %ld bytes\r\n",
                 pszFname, pHandle->hDsk, nNumberOfBytesToRead));
        }
        else
        {
            RETAILMSG(DEBUGLOG, (L"[CLKFLT-DBG] %s (%08X) Failed\r\n", pszFname,
                                 pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG,
                  (L"[CLKFLT-DBG] %s (%08X) Failed to call ReadFileScatter\r\n",
                   pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for ReadFileWithSeek: serve the read from the file-content cache
 * when the file is cached, otherwise forward to the underlying volume.
 *
 * @param pfh             File handle wrapper.
 * @param buffer          Destination buffer.
 * @param nBytesToRead    Number of bytes to read.
 * @param lpNumBytesRead  Receives the number of bytes read.
 * @param lpOverlapped    Overlapped context.
 * @param dwLowOffset     Low 32 bits of the file offset.
 * @param dwHighOffset    High 32 bits of the file offset.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_ReadFileWithSeek(PFILE pfh, LPVOID buffer, DWORD nBytesToRead,
                        LPDWORD lpNumBytesRead, LPOVERLAPPED lpOverlapped,
                        DWORD dwLowOffset, DWORD dwHighOffset)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;
    /*
	if ((0 == dwLowOffset) && (1 == nBytesToRead)) {
		// Kernel is reading its "test byte" which will be written back
		// to confirm it has write access to the map file. Since the file 
		// could actually be empty at this point, there may be no real data
		// to read from it. Give the kernel the return value it expects and
		// skipreading the actual file data.
		*lpNumBytesRead = 1;
		*((BYTE*)buffer) = 0xCE;
		return TRUE;
	}

	if (!buffer && !nBytesToRead && !lpNumBytesRead && !dwLowOffset && !dwHighOffset) {
		// Kernel is confirming that paging is supported. Always report
		// that it is. If the underlying file system does not actually
		// support paging, UncachedReadWriteWithSeek will emulate it.
		return TRUE;
    }
	*/
    EnsureCacheLoaded();

    SETFNAME(L"ReadFileWithSeek");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    if (ticks == 0)
        ticks = GetTickCount();
    if ((GetTickCount() - ticks) >= 120000)
        RETAILMSG(DEBUGLOG, (L"[CLKFLT-DBG] %s(%ls,"
                             L" buffer=%08X,"
                             L" bytesToRead=%d,"
                             L" dwLowOffset=%d,"
                             L" dwHighOffset=%d)\r\n",
                             pszFname, pHandle->wsFileName, buffer,
                             nBytesToRead, dwLowOffset, dwHighOffset));

    __try
    {
        if (FAST_CACHE_FILE_ENTRY *entry =
                cache->GetCacheEntry(pHandle->dwFileNameHash))
        {
            DWORD bRead = 0;
            cache->GetFileData(entry, (LPBYTE)buffer, dwLowOffset, nBytesToRead,
                               &bRead);
            if (lpNumBytesRead)
                *lpNumBytesRead = bRead;
            pHandle->lFilePosition = dwLowOffset + bRead;
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded returning %ld bytes\r\n", pszFname,
                       pHandle->hDsk, lpNumBytesRead ? *lpNumBytesRead : 0));
            SetLastError(NO_ERROR);
            return TRUE;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG,
                  (L"%s (%08X) Failed to call ReadFileWithSeek-CACHE\r\n",
                   pszFname, pHandle->hDsk));
    }
    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lpBuffer=%08X nNumberOfBytesToRead=%ld lpNumberOfBytesRead=%08X dwLowOffset=%ld dwHighOffset=%ld\r\n",
         pszFname, pHandle->hDsk, pfh, buffer, nBytesToRead, lpNumBytesRead,
         dwLowOffset, dwHighOffset));

    __try
    {
        fSuccess = pHandle->pHook->pReadFileWithSeek(
            pHandle->h, buffer, nBytesToRead, lpNumBytesRead, lpOverlapped,
            dwLowOffset, dwHighOffset);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded returning %ld bytes\r\n", pszFname,
                       pHandle->hDsk, lpNumBytesRead ? *lpNumBytesRead : 0));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call ReadFileWithSeek\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for SetEndOfFile: truncate or extend the file to the current
 * file pointer.
 *
 * @param pfh    File handle wrapper.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_SetEndOfFile(PFILE pfh)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"SetEndOfFile");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(DEBUGLOG,
              (L"%s (%08X) hFile=%08X\r\n", pszFname, pHandle->hDsk, pfh));

    __try
    {
        fSuccess = pHandle->pHook->pSetEndOfFile(pHandle->h);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call SetEndOfFile\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for SetFilePointer: move the file pointer and keep the cached
 * position in the HandleState in sync.
 *
 * @param pfh                  File handle wrapper.
 * @param lDistanceToMove      Low 32 bits of the signed move distance.
 * @param lpDistanceToMoveHigh High 32 bits of the move distance, or NULL.
 * @param dwMoveMethod         Origin of the move (FILE_BEGIN/CURRENT/END).
 *
 * @return Low 32 bits of the new position, or INVALID_SET_FILE_POINTER on
 *         failure.
 */
DWORD
FILTER_SetFilePointer(PFILE pfh, LONG lDistanceToMove,
                      PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    DWORD dwRet = 0;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"SetFilePointer");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }
    __try
    {
        if (FAST_CACHE_FILE_ENTRY *entry =
                cache->GetCacheEntry(pHandle->dwFileNameHash))
        {
            dwRet = INVALID_SET_FILE_POINTER;

            RETAILMSG(
                DEBUGLOG,
                (L"[CLKFLT-CACHE] %s(%ls, distance = %d, lpDistanceHigh = %X, dwMethod = %d)\r\n",
                 pszFname, pHandle->wsFileName, lDistanceToMove,
                 lpDistanceToMoveHigh, dwMoveMethod));
            __int64 lDistance = 0;
            if (lpDistanceToMoveHigh)
            {
                __try
                {
                    SetLastError(NO_ERROR);
                    lDistance = (__int64)*lpDistanceToMoveHigh << 32 |
                                (DWORD)lDistanceToMove;
                    *lpDistanceToMoveHigh = 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    SetLastError(ERROR_INVALID_HANDLE);
                    return dwRet;
                }
            }
            else
            {
                lDistance = lDistanceToMove;
            }

            RETAILMSG(
                DEBUGLOG,
                (L"[CLKFLT-CACHE] %s(method = %d, distance = %d, curPos = %d, fileSize = %d)\r\n",
                 pszFname, dwMoveMethod, lDistance, pHandle->lFilePosition,
                 pHandle->dwFileSize));
            switch (dwMoveMethod)
            {
                case FILE_BEGIN:
                    break;
                case FILE_CURRENT:
                {
                    lDistance += pHandle->lFilePosition;
                    break;
                }
                case FILE_END:
                {
                    lDistance += pHandle->dwFileSize;
                    break;
                }
                default:
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    dwRet = INVALID_SET_FILE_POINTER;
                    goto end;
                }
            }
            if (lDistance >> 32)
            {
                SetLastError(ERROR_NEGATIVE_SEEK);
                dwRet = INVALID_SET_FILE_POINTER;
            }
            else
            {
                pHandle->lFilePosition = lDistance;
                dwRet = (DWORD)lDistance;
            }
            RETAILMSG(
                DEBUGLOG,
                (L"[CLKFLT-CACHE] %s(dwRet = %d, lDistance = %d, phandle->lFilePosition = %d)\r\n",
                 pszFname, dwRet, lDistance, pHandle->lFilePosition));

        end:
            return dwRet;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(
            DEBUGLOG,
            (L"[CLKFLT-CACHE] %s (%08X) Failed to call ReadFileWithSeek-CACHE\r\n",
             pszFname, pHandle->hDsk));
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lDistanceToMove=%ld lpDistanceToMoveHigh=%08X dwMoveMethod=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, lDistanceToMove, lpDistanceToMoveHigh,
         dwMoveMethod));

    __try
    {
        dwRet = pHandle->pHook->pSetFilePointer(
            pHandle->h, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);

        if (dwRet != -1)
        {
            RETAILMSG(DEBUGLOG, (L"%s (%08X) Succeeded returning %ld\r\n",
                                 pszFname, pHandle->hDsk, dwRet));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call SetFilePointer\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return dwRet;
}

/**
 * Filter hook for SetFileTime.
 *
 * @param pfh           File handle wrapper.
 * @param lpCreation    New creation time, or NULL to leave unchanged.
 * @param lpLastAccess  New last-access time, or NULL to leave unchanged.
 * @param lpLastWrite   New last-write time, or NULL to leave unchanged.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_SetFileTime(PFILE pfh, const FILETIME *lpCreation,
                   const FILETIME *lpLastAccess, const FILETIME *lpLastWrite)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"SetFileTime");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lpCreationTime=%08X lpLastAccessTime=%08X lpLastWriteTime=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, lpCreation, lpLastAccess, lpLastWrite));

    __try
    {
        fSuccess = pHandle->pHook->pSetFileTime(pHandle->h, lpCreation,
                                                lpLastAccess, lpLastWrite);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call SetFileTime\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for WriteFile.
 *
 * @param pfh                File handle wrapper.
 * @param buffer             Source buffer.
 * @param nBytesToWrite      Number of bytes to write.
 * @param lpNumBytesWritten  Receives the number of bytes written.
 * @param lpOverlapped       Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_WriteFile(PFILE pfh, LPCVOID buffer, DWORD nBytesToWrite,
                 LPDWORD lpNumBytesWritten, LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"WriteFile");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lpBuffer=%08X nNumberOfBytesToWrite=%ld lpNumberOfBytesWritten=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, buffer, nBytesToWrite,
         lpNumBytesWritten));

    __try
    {
        fSuccess = pHandle->pHook->pWriteFile(pHandle->h, buffer, nBytesToWrite,
                                              lpNumBytesWritten, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG, (L"%s (%08X) Succeeded wrote %ld bytes\r\n",
                                 pszFname, pHandle->hDsk,
                                 lpNumBytesWritten ? *lpNumBytesWritten : 0));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call WriteFile\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for WriteFileWithSeek.
 *
 * @param pfh                File handle wrapper.
 * @param buffer             Source buffer.
 * @param nBytesToWrite      Number of bytes to write.
 * @param lpNumBytesWritten  Receives the number of bytes written.
 * @param lpOverlapped       Overlapped context.
 * @param dwLowOffset        Low 32 bits of the file offset.
 * @param dwHighOffset       High 32 bits of the file offset.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_WriteFileWithSeek(PFILE pfh, LPCVOID buffer, DWORD nBytesToWrite,
                         LPDWORD lpNumBytesWritten, LPOVERLAPPED lpOverlapped,
                         DWORD dwLowOffset, DWORD dwHighOffset)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"WriteFileWithSeek");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pHandle->wsFileName));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X lpBuffer=%08X nNumberOfBytesToWrite=%ld lpNumberOfBytesWritten=%08X dwLowOffset=%ld dwHighOffset=%ld\r\n",
         pszFname, pHandle->hDsk, pfh, buffer, nBytesToWrite, lpNumBytesWritten,
         dwLowOffset, dwHighOffset));

    __try
    {
        fSuccess = pHandle->pHook->pWriteFileWithSeek(
            pHandle->h, buffer, nBytesToWrite, lpNumBytesWritten, lpOverlapped,
            dwLowOffset, dwHighOffset);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG, (L"%s (%08X) Succeeded wrote %ld bytes\r\n",
                                 pszFname, pHandle->hDsk,
                                 lpNumBytesWritten ? *lpNumBytesWritten : 0));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call WriteFileWithSeek\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for WriteFileGather: gathered write from page-sized segments.
 *
 * @param pfh                    File handle wrapper.
 * @param aSegmentArray          Array of page-aligned source segments.
 * @param nNumberOfBytesToWrite  Total number of bytes to write.
 * @param lpReserved             Reserved.
 * @param lpOverlapped           Overlapped context.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_WriteFileGather(PFILE pfh, FILE_SEGMENT_ELEMENT aSegmentArray[],
                       DWORD nNumberOfBytesToWrite, LPDWORD lpReserved,
                       LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = false;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"WriteFileGather");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X aSegmentArray=%08X nNumberOfBytesToWrite=%ld lpReserved=%08X\r\n",
         pszFname, pHandle->hDsk, pfh, aSegmentArray, nNumberOfBytesToWrite,
         lpReserved));

    __try
    {
        fSuccess = pHandle->pHook->pWriteFileGather(pHandle->h, aSegmentArray,
                                                    nNumberOfBytesToWrite,
                                                    lpReserved, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded returning %ld bytes\r\n", pszFname,
                       pHandle->hDsk, nNumberOfBytesToWrite));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call WriteFileGather\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}

/**
 * Filter hook for UnlockFileEx: release a byte range previously locked.
 *
 * @param pfh                        File handle wrapper.
 * @param dwReserved                 Reserved, must be zero.
 * @param nNumberOfBytesToUnlockLow  Low 32 bits of the range length.
 * @param nNumberOfBytesToUnlockHigh High 32 bits of the range length.
 * @param lpOverlapped               Overlapped context giving the offset.
 *
 * @return TRUE on success, FALSE on failure.
 */
BOOL
FILTER_UnlockFileEx(PFILE pfh, DWORD dwReserved,
                    DWORD nNumberOfBytesToUnlockLow,
                    DWORD nNumberOfBytesToUnlockHigh, LPOVERLAPPED lpOverlapped)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)pfh;

    SETFNAME(L"UnlockFileEx");

    if (!pHandle)
    {
        RETAILMSG(DEBUGLOG, (L"%s Error: PFILE null\r\n", pszFname));
        goto exit;
    }

    RETAILMSG(
        DEBUGLOG,
        (L"%s (%08X) hFile=%08X dwReserved=%08X nNumberOfBytesToUnlockLow=%ld nNumberOfBytesToUnlockHigh=%ld\r\n",
         pszFname, pHandle->hDsk, pfh, dwReserved, nNumberOfBytesToUnlockLow,
         nNumberOfBytesToUnlockHigh));

    __try
    {
        fSuccess = pHandle->pHook->pUnlockFileEx(
            pHandle->h, dwReserved, nNumberOfBytesToUnlockLow,
            nNumberOfBytesToUnlockHigh, lpOverlapped);

        if (fSuccess)
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            RETAILMSG(DEBUGLOG,
                      (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RETAILMSG(DEBUGLOG, (L"%s (%08X) Failed to call UnlockFileEx\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}
