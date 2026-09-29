/* FullUnlock v4.0 project.
   Cloaking Filter implementation.
   
   (C) ultrashot 2012
*/
#ifndef CLOAKING_H
#define CLOAKING_H

// Load the redirection list from the registry once (lazy, first-call).
void LoadCloakList();

// Return the replacement path for lpFileName, or lpFileName itself if the
// name is not on the cloak list. The returned pointer is either the input or
// storage owned by the internal list; the caller must not free it.
wchar_t *Cloaking_GetNewPath(wchar_t *lpFileName);

#endif