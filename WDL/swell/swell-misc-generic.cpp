/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux/OSX)
   Copyright (C) 2006 and later, Cockos, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/
  
#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-internal.h"

bool IsRightClickEmulateEnabled()
{
  return false;
}

void SWELL_EnableRightClickEmulate(BOOL enable)
{
}
HANDLE SWELL_CreateProcess(const char *exe, int nparams, const char **params)
{
  if (fork() == 0)
  {
    char **pp = (char **)calloc(nparams+2,sizeof(char*));
    pp[0] = strdup(exe);
    for (int x=0;x<nparams;x++) pp[x+1] = strdup(params[x]?params[x]:"");
    execv(exe,pp);
    exit(0);
  }

  return 0; // todo
}


#endif
