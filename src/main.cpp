/* FullUnlock v4.0 project.
   Cloaking Filter implementation.
   
   (C) ultrashot 2012
*/
#include "stdafx.h"
#include "clkflt.h"
#include "Cloaking.h"
#include "debug.h"
// File system filter interface APIs.

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("FSDSPY"),
    {TEXT("Apis"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""),
     TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""),
     TEXT(""), TEXT("")},
    0xffffffff // Enable all debug zones.
};
#endif // DEBUG

// Global critical section.
static CRITICAL_SECTION csMain;

/**
 * DLL entry point. Initialises the global critical section on process attach
 * and releases it on detach.
 *
 * @param hInstance     Module instance handle.
 * @param dwReason      Reason the entry point is invoked (DLL_PROCESS_*).
 * @param lpReserved    Reserved loader context.
 *
 * @return TRUE.
 */
BOOL WINAPI
DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DEBUGREGISTER((HINSTANCE)hInstance);
            InitializeCriticalSection(&csMain);
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            DeleteCriticalSection(&csMain);
            break;
        }
    }

    return TRUE;
}

/**
 * FSDMGR filter entry point: attach the filter to a mounted volume. Loads the
 * cloak list and allocates a VolumeState capturing the disk handle and the
 * underlying filter hook table.
 *
 * @param hDsk    Disk handle of the volume being hooked.
 * @param pHook   Filter hook table supplied by FSDMGR.
 *
 * @return Newly allocated volume state, or NULL on failure.
 */
PVOLUME
FILTER_HookVolume(HDSK hDsk, FILTERHOOK *pHook)
{
    VolumeState *pState = NULL;

    LoadCloakList();

    SETFNAME(L"HookVolume");

    if (!hDsk)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: HDSK null\r\n", pszFname));
        goto exit;
    }

    if (!pHook)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: FILTERHOOK null\r\n", pszFname));
        goto exit;
    }

    DEBUGMSG(ZONE_APIS,
             (L"%s hDsk=%08X pHook=%08X\r\n", pszFname, hDsk, pHook));

    pState = new VolumeState;
    if (!pState)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Failed to allocate volume\r\n", pszFname));
        goto exit;
    }

    pState->hDsk = hDsk;
    memcpy(&pState->FilterHook, pHook, sizeof(FILTERHOOK));

exit:;

    return (PVOLUME)pState;
}

/**
 * FSDMGR filter entry point: detach the filter from a volume and free its
 * VolumeState.
 *
 * @param pvol    Volume state previously returned by FILTER_HookVolume.
 *
 * @return TRUE on success, FALSE if pvol is NULL.
 */
BOOL
FILTER_UnhookVolume(PVOLUME pvol)
{
    BOOL fRet = FALSE;

    SETFNAME(L"UnhookVolume");

    if (!pvol)
    {
        DEBUGMSG(ZONE_APIS, (L"%s Error: PVOLUME null\r\n", pszFname));
        goto exit;
    }

    delete (VolumeState *)pvol;
    pvol = NULL;

    fRet = TRUE;

exit:;

    return fRet;
}
