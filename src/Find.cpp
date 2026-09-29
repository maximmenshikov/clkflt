/* FullUnlock v4.0 project.
   Cloaking Filter implementation.
   
   (C) ultrashot 2012
*/
#include "stdafx.h"
#include "clkflt.h"
#include "debug.h"
// Search-based file I/O APIs.

/**
 * Filter hook for FindClose: close the underlying search handle and free the
 * wrapper HandleState.
 *
 * @param psh    Search handle wrapper returned by FILTER_FindFirstFileW.
 *
 * @return TRUE if the search was closed, FALSE otherwise.
 */
BOOL
FILTER_FindClose(PSEARCH psh)
{
    BOOL fSuccess = TRUE;
    HandleState *pHandle = (HandleState *)psh;

    SETFNAME(L"FindClose");

    DEBUGMSG(ZONE_APIS,
             (L"%s (%08X) hFindFile=%08X\r\n", pszFname, pHandle->hDsk, psh));

    __try
    {
        fSuccess = pHandle->pHook->pFindClose(pHandle->h);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));

            delete pHandle;
            pHandle = NULL;
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call FindClose\r\n",
                             pszFname, pHandle->hDsk));
    }

    return fSuccess;
}

/**
 * Filter hook for FindFirstFileW: start a search on the underlying volume and
 * wrap the returned search handle in a HandleState.
 *
 * @param pvol          Volume state.
 * @param hProc         Handle of the calling process.
 * @param pwsFileSpec   Search pattern.
 * @param pfd           Receives the first matching entry.
 *
 * @return Wrapped search handle, or INVALID_HANDLE_VALUE on failure.
 */
HANDLE
FILTER_FindFirstFileW(PVOLUME pvol, HANDLE hProc, PCWSTR pwsFileSpec,
                      PWIN32_FIND_DATAW pfd)
{
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    VolumeState *pState = (VolumeState *)pvol;

    SETFNAME(L"FindFirstFileW");

    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS,
             (L"%s (%08X) hProc=%08X lpFileName=%s lpFindFileData=%08X\r\n",
              pszFname, pState->hDsk, hProc, pwsFileSpec, pfd));

    __try
    {
        hSearch = pState->FilterHook.pFindFirstFileW(pState->FilterHook.hVolume,
                                                     hProc, pwsFileSpec, pfd);

        if (hSearch != INVALID_HANDLE_VALUE)
        {
            HandleState *pHandle = new HandleState;
            if (pHandle)
            {
                pHandle->h = (DWORD)hSearch;
                pHandle->hDsk = pState->hDsk;
                pHandle->pHook = &pState->FilterHook;
                hSearch = (HANDLE)pHandle;
            }
            else
            {
                DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to allocate handle\r\n",
                                     pszFname, pState->hDsk));

                pState->FilterHook.pFindClose((DWORD)hSearch);
                hSearch = INVALID_HANDLE_VALUE;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call FindFirstFileW\r\n",
                             pszFname, pState->hDsk));

        hSearch = INVALID_HANDLE_VALUE;
    }

exit:;

    return hSearch;
}

/**
 * Filter hook for FindNextFileW: fetch the next entry of an ongoing search.
 *
 * @param psh    Search handle wrapper.
 * @param pfd    Receives the next matching entry.
 *
 * @return TRUE if an entry was returned, FALSE at end of search or on error.
 */
BOOL
FILTER_FindNextFileW(PSEARCH psh, PWIN32_FIND_DATAW pfd)
{
    BOOL fSuccess = FALSE;
    HandleState *pHandle = (HandleState *)psh;

    SETFNAME(L"FindNextFileW");

    if (!pHandle)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PSEARCH null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS, (L"%s (%08X) hFindFile=%08X lpFindFileData=%08X\r\n",
                         pszFname, pHandle->hDsk, psh, pfd));

    __try
    {
        fSuccess = pHandle->pHook->pFindNextFileW(pHandle->h, pfd);

        if (fSuccess)
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Succeeded\r\n", pszFname, pHandle->hDsk));
        }
        else
        {
            DEBUGMSG(ZONE_APIS,
                     (L"%s (%08X) Failed\r\n", pszFname, pHandle->hDsk));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_APIS, (L"%s (%08X) Failed to call FindNextFileW\r\n",
                             pszFname, pHandle->hDsk));
    }

exit:;

    return fSuccess;
}
