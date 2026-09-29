#pragma once

#include "debug.h"
#include "CList.hpp"
#include "fsdmgr.h"
#include "clkflt.h"

#define ITEMS_IN_LIST_ENTRY 50

typedef struct
{
    wchar_t wsFileName[256];
    DWORD dwFileNameHash;
    DWORD byteArrayLength;
    BYTE *byteArray;
} FAST_CACHE_FILE_ENTRY;

typedef struct
{
    DWORD hash[ITEMS_IN_LIST_ENTRY];
    FAST_CACHE_FILE_ENTRY *entry[ITEMS_IN_LIST_ENTRY];
} FAST_CACHE_LIST_ENTRY;

class CFastCache
{
  public:
    CFastCache();
    ~CFastCache();
    FAST_CACHE_FILE_ENTRY *AddToCache(wchar_t *fileName, DWORD fileNameHash,
                                      VolumeState *pState, HANDLE hProc);
    void RemoveFromCache(FAST_CACHE_FILE_ENTRY *entry);
    BOOL GetFileData(FAST_CACHE_FILE_ENTRY *entry, LPBYTE buffer,
                     DWORD filePosition, DWORD bufferLength, DWORD *outLength);
    FAST_CACHE_FILE_ENTRY *GetCacheEntry(DWORD hash);
    void DebugLog();
    void DebugDumpFiles();

  private:
    BOOL ReserveEntry(FAST_CACHE_FILE_ENTRY *entry);
    List<FAST_CACHE_LIST_ENTRY> _list;
    void ShiftItemsDown(int listEntry, int erasedIndex);
};

bool IsAddToCacheEntry(LPWSTR pwsFileName);
