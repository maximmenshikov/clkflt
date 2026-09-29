#pragma once

DWORD HashString(wchar_t *str, unsigned int key = 0x1505,
                 bool notowlower = false);
