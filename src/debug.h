#pragma once

// Master switch for the RETAILMSG diagnostics emitted throughout the filter.
#define DEBUGLOG 1

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Debug zones.
#ifdef DEBUG
// Debug zone IDs.
#define ZONEID_APIS 0
// Debug zone masks.
#define ZONEMASK_APIS (1 << ZONEID_APIS)
// DEBUGMSG ZONE_Xxx argument.
#define ZONE_APIS DEBUGZONE(ZONEID_APIS)
#else
#define ZONE_APIS 0
#endif // DEBUG

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Insert SETFNAME(L"<function name>") at the start of a function.  If set,
// the function name can be referenced as 'pszFname' throughout the body of the
// function.  This removes the need to explicitly write the function name in
// the prefix of a debug message.
#ifdef DEBUGLOG
#define STR_MODULE L"CLKFLT!"
#define SETFNAME(name) LPCTSTR pszFname = STR_MODULE name L":"
#else
#define SETFNAME(name)
#endif // DEBUG
