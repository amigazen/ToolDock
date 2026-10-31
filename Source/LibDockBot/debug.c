/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
************************************/

#include "lib.h"

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>


extern struct DockBotLibrary *ToolDockBaseFull;

#ifdef DEBUG_BUILD

BOOL __DebugEnabled = TRUE;

#endif

VOID __asm __saveds TD_RegisterDebugStream(
	register __a0 BPTR fh)
{
    ToolDockBaseFull->l_DebugOutput = fh;

    if( fh ) {
        VFPrintf(fh, "Library Logging Enabled\n", NULL);
    }
}


VOID DebugLogInternal(STRPTR fmt, LONG *argv)
{
    if( ! ToolDockBaseFull->l_DebugOutput ) {
        return;
    }

    VFPrintf(ToolDockBaseFull->l_DebugOutput, fmt, argv);
}

VOID __asm __saveds TD_DebugLog(
	register __a0 STRPTR fmt,
	register __a1 LONG *argv)
{
    DebugLogInternal(fmt, argv);
}

VOID DebugLog(STRPTR fmt, ...)
{
    DebugLogInternal(fmt, (LONG *)(&fmt + 1));
}




