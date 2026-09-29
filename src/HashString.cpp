#include "stdafx.h"

DWORD
HashString(wchar_t *str, unsigned int key, bool notowlower)
{
    size_t length = 0;
    if (StringCchLength(str, STRSAFE_MAX_CCH, &length) != S_OK)
        return 0;
    if (!length)
        return 0;
    unsigned int hash = key;
    for (unsigned int x = 0; x < length; x++)
    {
        hash = hash + (hash << 5);
        hash = hash + (notowlower ? str[x] : towlower(str[x]));
    }
    if (hash & 0x80000000)
        hash ^= 0x80000000;
    return hash;
}
