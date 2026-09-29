// CFastCache v2
// (C) ultrashot 2013
#include "stdafx.h"
#include "CFastCache.h"
#include "HashString.h"

/**
 * Construct an empty cache.
 */
CFastCache::CFastCache() {}

/**
 * Destroy the cache.
 */
CFastCache::~CFastCache() {}

/**
 * Read a file through the filter hook and add its contents to the cache.
 *
 * @param fileName      Name of the file to cache.
 * @param fileNameHash  Precomputed hash of the file name.
 * @param pState        Volume state providing the underlying filter hook.
 * @param hProc         Handle of the calling process.
 *
 * @return New cache entry, or NULL if the file is already cached, empty, or
 *         could not be read.
 */
FAST_CACHE_FILE_ENTRY *
CFastCache::AddToCache(wchar_t *fileName, DWORD fileNameHash,
                       VolumeState *pState, HANDLE hProc)
{
    RETAILMSG(DEBUGLOG, (L"[CLKFLT-CACHE] AddToCache(%ls, %08X)\r\n", fileName,
                         fileNameHash));
    if (GetCacheEntry(fileNameHash) != NULL)
    {
        RETAILMSG(
            DEBUGLOG,
            (L"[CLKFLT-CACHE] AddToCache failed: reason - %ls is already in cache\r\n",
             fileName));
        return 0;
    }

    HANDLE hFile = pState->FilterHook.pCreateFileW(
        pState->FilterHook.hVolume, hProc, fileName, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FAST_CACHE_FILE_ENTRY *fEntry = NULL;
        DWORD dwRet = 0;
        DWORD dwSizeHigh = 0;
        DWORD dwFileSize =
            pState->FilterHook.pGetFileSize((DWORD)hFile, &dwSizeHigh);
        if (dwFileSize > 0)
        {
            fEntry = new FAST_CACHE_FILE_ENTRY;
            if (fEntry != NULL)
            {
                wcscpy(fEntry->wsFileName, fileName);
                fEntry->dwFileNameHash = fileNameHash;
                fEntry->byteArrayLength = dwFileSize;
                fEntry->byteArray = new BYTE[dwFileSize];
                if (fEntry->byteArray != NULL)
                {
                    DWORD dwBytesRead = 0;
                    pState->FilterHook.pReadFile((DWORD)hFile,
                                                 fEntry->byteArray, dwFileSize,
                                                 &dwBytesRead, NULL);
                    if (dwBytesRead == dwFileSize)
                    {
                        ReserveEntry(fEntry);
                        RETAILMSG(1, (L"[CLKFLT-CACHE] %ls added to cache\r\n",
                                      fileName));
                        dwRet = dwBytesRead;
                    }
                }
            }

            if (dwRet == 0)
            {
                // cleanup
                if (fEntry->byteArray != NULL)
                    delete[] fEntry->byteArray;

                if (fEntry)
                    delete fEntry;

                fEntry = NULL;
            }
        }
        pState->FilterHook.pCloseFile((DWORD)hFile);

        return fEntry;
    }
    else
    {
        RETAILMSG(
            DEBUGLOG,
            (L"[CLKFLT-CACHE] AddToCache failed: reason - couldn't open %ls\r\n",
             fileName));
    }
    return NULL;
}

/**
 * Compact the cache table after a slot is erased, shifting the following
 * entries down by one position (across list blocks where necessary).
 *
 * @param listEntry     Index of the list block where the erasure happened.
 * @param erasedIndex   Slot index within that block that was erased.
 */
void
CFastCache::ShiftItemsDown(int listEntry, int erasedIndex)
{
    int cnt = _list.Count();
    for (int i = listEntry; i < cnt; ++i)
    {
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        int j = 0;
        if (i == listEntry)
            j = erasedIndex;
        for (j = j; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            DWORD hash = 0;
            FAST_CACHE_FILE_ENTRY *entry = NULL;
            if (j == (ITEMS_IN_LIST_ENTRY - 1))
            {
                if (i != (cnt - 1))
                {
                    FAST_CACHE_LIST_ENTRY *nextListEntry =
                        (FAST_CACHE_LIST_ENTRY *)_list.Get(i + 1);
                    hash = nextListEntry->hash[0];
                    entry = nextListEntry->entry[0];
                    nextListEntry->hash[0] = 0;
                    nextListEntry->entry[0] = NULL;
                }
            }
            else
            {
                hash = lEntry->hash[j + 1];
                entry = lEntry->entry[j + 1];
                lEntry->hash[j + 1] = 0;
                lEntry->entry[j + 1] = NULL;
            }
            lEntry->hash[j] = hash;
            lEntry->entry[j] = entry;
        }
    }
}

/**
 * Remove an entry from the cache and free its buffered data.
 *
 * @param entry    Cache entry to remove.
 */
void
CFastCache::RemoveFromCache(FAST_CACHE_FILE_ENTRY *entry)
{
    int cnt = _list.Count();
    for (int i = 0; i < cnt; ++i)
    {
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        for (int j = 0; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            DWORD tempHash = lEntry->hash[j];
            if (tempHash == entry->dwFileNameHash)
            {
                RETAILMSG(1, (L"[CLKFLT-CACHE] %ls removed from cache\r\n",
                              entry->wsFileName));
                ShiftItemsDown(i, j);
                delete[] entry->byteArray;
                delete entry;
                break;
            }
        }
    }
}
/**
 * Copy a range of a cached file's contents into a caller-supplied buffer.
 *
 * @param inEntry       Cache entry to read from.
 * @param buffer        Destination buffer.
 * @param filePosition  Offset within the cached file to start at.
 * @param bufferLength  Number of bytes requested.
 * @param outLength     Receives the number of bytes actually copied.
 *
 * @return TRUE on success, FALSE if the offset is out of range.
 */
BOOL
CFastCache::GetFileData(FAST_CACHE_FILE_ENTRY *inEntry, LPBYTE buffer,
                        DWORD filePosition, DWORD bufferLength,
                        DWORD *outLength)
{
    if (outLength)
        *outLength = 0;

    FAST_CACHE_FILE_ENTRY *entry = (FAST_CACHE_FILE_ENTRY *)inEntry;
    RETAILMSG(
        DEBUGLOG,
        (L"[CLKFLT-CACHE] GetFileData(%ls, hash = %X, pos = %d, length = %d)\r\n",
         entry->wsFileName, entry->dwFileNameHash, filePosition, bufferLength));

    // we found the file.
    if (filePosition < 0 || filePosition >= entry->byteArrayLength)
        return FALSE;

    DWORD length = bufferLength;
    if ((filePosition + bufferLength) > entry->byteArrayLength)
        length = entry->byteArrayLength - filePosition;

    memcpy(buffer, &entry->byteArray[filePosition], length);
    if (outLength)
        *outLength = length;
    RETAILMSG(DEBUGLOG,
              (L"[CLKFLT-CACHE] GetFileData(%ls) returned %d bytes\r\n",
               entry->wsFileName, length));
    return TRUE;
}

/**
 * Test whether a file name carries the magic "-addtocache" suffix.
 *
 * @param pwsFileName    File name to test.
 *
 * @return true if the suffix is present, false otherwise.
 */
bool
IsAddToCacheEntry(LPWSTR pwsFileName)
{
    bool found = false;
    int len = wcslen(pwsFileName);
    if (len > 11)
    {
        if (pwsFileName[len - 1 - 10] == L'-')
        {
            if (wcsicmp(&pwsFileName[len - 1 - 10], L"-addtocache") == 0)
            {
                found = true;
            }
        }
    }
    return found;
}

/**
 * Find a cache entry by its file-name hash.
 *
 * @param hash    File-name hash to look up.
 *
 * @return Matching cache entry, or NULL if the file is not cached.
 */
FAST_CACHE_FILE_ENTRY *
CFastCache::GetCacheEntry(DWORD hash)
{
    int cnt = _list.Count();
    for (int i = 0; i < cnt; ++i)
    {
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        for (int j = 0; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            DWORD tempHash = lEntry->hash[j];
            if (tempHash == 0)
                return NULL;
            if (tempHash == hash)
                return lEntry->entry[j];
        }
    }
    return NULL;
}

/**
 * Store an entry in the first free slot, allocating a new list block if the
 * existing blocks are full.
 *
 * @param inEntry    Entry to store.
 *
 * @return TRUE.
 */
BOOL
CFastCache::ReserveEntry(FAST_CACHE_FILE_ENTRY *inEntry)
{
    FAST_CACHE_FILE_ENTRY *entry = (FAST_CACHE_FILE_ENTRY *)inEntry;
    int cnt = _list.Count();
    for (int i = 0; i < cnt; ++i)
    {
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        for (int j = 0; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            DWORD tempHash = lEntry->hash[j];
            if (tempHash == 0)
            {
                lEntry->hash[j] = entry->dwFileNameHash;
                lEntry->entry[j] = entry;
                return TRUE;
            }
        }
    }
    // couldn't find any space in already allocated list entries.
    // let's create new list entry.
    FAST_CACHE_LIST_ENTRY *newEntry = new FAST_CACHE_LIST_ENTRY;
    memset(newEntry, 0, sizeof(FAST_CACHE_LIST_ENTRY));
    newEntry->hash[0] = entry->dwFileNameHash;
    newEntry->entry[0] = entry;
    _list.Add(newEntry);
    return TRUE;
}

/**
 * Dump the cache table (hashes and file names) to \cachelog.txt.
 */
void
CFastCache::DebugLog()
{
    FILE *f = _wfopen(L"\\cachelog.txt", L"at");
    int cnt = _list.Count();
    fwprintf(f, L"Entries: %d\r\n", cnt);
    for (int i = 0; i < cnt; ++i)
    {
        fwprintf(f, L"Entry #%d\r\n", i);
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        for (int j = 0; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            DWORD tempHash = lEntry->hash[j];
            fwprintf(f, L"  %X: %ls\r\n", tempHash,
                     lEntry->entry[j]->wsFileName);
            if (tempHash == 0)
                goto L_break;
        }
    }
L_break:
    fclose(f);
}

/**
 * Write every cached file's contents out under \Temp\FastCache.
 */
void
CFastCache::DebugDumpFiles()
{
    int cnt = _list.Count();
    for (int i = 0; i < cnt; ++i)
    {
        FAST_CACHE_LIST_ENTRY *lEntry = (FAST_CACHE_LIST_ENTRY *)_list.Get(i);
        for (int j = 0; j < ITEMS_IN_LIST_ENTRY; ++j)
        {
            if (lEntry->hash[j] == 0)
                return;
            FAST_CACHE_FILE_ENTRY *entry = lEntry->entry[j];
            wchar_t path[500];
            wcscpy(path, L"\\Temp\\FastCache\\");
            wcscat(path, wcsrchr(entry->wsFileName, L'\\') + 1);
            FILE *f = _wfopen(path, L"wb");
            fwrite(entry->byteArray, 1, entry->byteArrayLength, f);
            fflush(f);
            fclose(f);
        }
    }
}