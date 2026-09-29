/* FullUnlock v4.0 project.
   Cloaking Filter implementation.

   Path redirection: file names looked up by the filter are matched against a
   replacement list read from HKLM\Software\OEM\Cloaking (value name = original
   name, value data = replacement name), letting stock system modules be served
   from patched copies on disk.

   (C) ultrashot 2012
*/
#include "stdafx.h"
#include "CList.hpp"

#define ENABLELOG 1

#define E_INSUFFICIENT_BUFFER HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
#define E_DATATYPE_MISMATCH HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH)

typedef struct
{
    int oldFileNameLength;
    wchar_t oldFileName[500];
    wchar_t newFileName[500];
} REPLACEFILESTRUCT;

/**
 * Read a REG_SZ value using the WinCE FSD registry-hook calling convention
 * (the subkey path is smuggled through the reserved parameter).
 *
 * @param hKey          Open registry key.
 * @param pszSubKey     Subkey path, passed through the reserved slot.
 * @param pszValueName  Name of the value to read.
 * @param pszData       Buffer that receives the string.
 * @param cchData       Size of the buffer, in characters.
 *
 * @return S_OK on success, or an error HRESULT.
 */
HRESULT
RegistryGetString7(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszValueName,
                   LPTSTR pszData, UINT cchData)
{
    HRESULT hr = S_OK;
    LONG lResult = ERROR_SUCCESS;
    DWORD dwType = REG_NONE;
    DWORD cbData = 0;

    if (hKey == NULL)
        return E_INVALIDARG;
    if (pszData == NULL)
        return E_INVALIDARG;

    // Make sure that the data passed in makes sense

    cbData = (cchData * sizeof(TCHAR));

    // Do the query using the undocumented registry hack
    lResult = ::RegQueryValueEx(hKey, pszValueName, (LPDWORD)pszSubKey /*hack*/,
                                &dwType, (LPBYTE)(pszData), &cbData);
    if (dwType != REG_SZ)
        return E_DATATYPE_MISMATCH;

    if (ERROR_MORE_DATA != lResult)
    {
        if (lResult != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(lResult);
    }
    else
    {
        hr = E_INSUFFICIENT_BUFFER;
    }

    return hr;
}

List<REPLACEFILESTRUCT> ReplaceList;

bool isCloakListLoaded = false;

/**
 * Load the redirection list from HKLM\Software\OEM\Cloaking on first call.
 * Each registry value maps an original file name to its replacement name.
 */
void
LoadCloakList()
{
    if (isCloakListLoaded == false)
    {
        RETAILMSG(
            ENABLELOG,
            (L"[CLKFLT] List not loaded. Collecting registry entries...\r\n"));
        __try
        {
            HKEY hKey;

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\OEM\\Cloaking", 0,
                             KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                RETAILMSG(ENABLELOG,
                          (L"[CLKFLT] Key opened. Enumerating values...\r\n"));
                wchar_t keyName[500];
                DWORD keyNameSize = 50;
                DWORD dwIndex = 0;

                DWORD dwType = REG_NONE;
                while (RegEnumValue(hKey, dwIndex++, keyName, &keyNameSize,
                                    NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
                {
                    if (dwType != REG_NONE)
                    {
                        REPLACEFILESTRUCT *rfs = new REPLACEFILESTRUCT;
                        if (rfs)
                        {
                            memset(rfs, 0, sizeof(REPLACEFILESTRUCT));
                            wcscpy_s(rfs->oldFileName, 500, keyName);

                            RegistryGetString7(HKEY_LOCAL_MACHINE,
                                               L"Software\\OEM\\Cloaking",
                                               keyName, rfs->newFileName, 500);

                            RETAILMSG(
                                ENABLELOG,
                                (L"[CLKFLT] Got %ls (->%ls) and pushed to the list\r\n",
                                 rfs->oldFileName, rfs->newFileName));
                            rfs->oldFileNameLength = wcslen(rfs->oldFileName);
                            if (rfs->oldFileNameLength > 0)
                            {
                                ReplaceList.Add(rfs);
                            }
                        }
                    }
                    keyNameSize = 500;
                }
                RETAILMSG(ENABLELOG, (L"[CLKFLT] Done. Closing the key.\r\n"));
                RegCloseKey(hKey);
            }
            else
            {
                RETAILMSG(ENABLELOG, (L"[CLKFLT] RegOpenKeyEx error\r\n"));
            }
        }
        __except (1)
        {
        }
        isCloakListLoaded = true;
    }
}

/**
 * Return the replacement path for a file name if it is on the cloak list.
 *
 * @param lpFileName    File name to look up.
 *
 * @return Pointer to the replacement name, or lpFileName if it is not cloaked.
 */
wchar_t *
Cloaking_GetNewPath(wchar_t *lpFileName)
{
    if (lpFileName == NULL)
        return NULL;

    __try
    {
        int length = wcslen(lpFileName);
        UINT count = ReplaceList.Count();
        for (UINT x = 0; x < count; x++)
        {
            REPLACEFILESTRUCT *rfs = (REPLACEFILESTRUCT *)ReplaceList.Get(x);
            if (length == rfs->oldFileNameLength)
            {
                if (towlower(lpFileName[0]) == towlower(rfs->oldFileName[0]))
                {
                    if (wcsicmp(lpFileName, rfs->oldFileName) == 0)
                    {
                        RETAILMSG(ENABLELOG,
                                  (L"[CLKFLT] Redirected %ls to %ls\n",
                                   lpFileName, rfs->newFileName));
                        return rfs->newFileName;
                    }
                }
            }
        }
    }
    __except (1)
    {
        return lpFileName;
    }

    return lpFileName;
}
