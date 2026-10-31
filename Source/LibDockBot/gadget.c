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

#include <clib/alib_protos.h>
#include <clib/intuition_protos.h>

#include "dockbot.h"
#include "dockbot_protos.h"


VOID __asm __saveds TD_GetDockGadgetEnvironment(
	register __a0 Object *obj, 
	register __a1 struct GadgetEnvironment *env)
{
    
    struct DockMessageGetEnvironment msg = {
        DM_GETENV
    };

    msg.env = env;

    DoMethodA(obj, (Msg)&msg);
}


VOID __asm __saveds TD_RequestDockQuit(
	register __a0 Object *obj)
{
    DoMethod(obj, DM_REQ_QUIT);
}


VOID __asm __saveds TD_RequestDockGadgetDraw(
	register __a0 Object *obj) 
{
    DoMethod(obj, DM_REQ_DRAW);
}

VOID __asm __saveds TD_RequestLaunch(
    register __a0 Object *obj,
    register __a1 STRPTR path,
    register __a2 STRPTR args,
    register __a3 STRPTR console,
    register __d0 BOOL wb) 
{
    struct DockMessageLaunch msg = {
        DM_REQ_LAUNCH
    };

    msg.path = path;
    msg.args = args;
    msg.console = console;
    msg.wb = wb;

    DoMethodA(obj, (Msg)&msg);    
}

VOID __asm __saveds TD_RegisterPort(
    register __a0 Object *obj,
    register __a1 struct MsgPort *port)
{
    struct DockMessagePort msg = {
        DM_REG_PORT
    };

    msg.port = port;

    DoMethodA(obj, (Msg)&msg);
}

VOID __asm __saveds TD_UnregisterPort(
    register __a0 Object *obj,
    register __a1 struct MsgPort *port)
{
    struct DockMessagePort msg = {
        DM_UNREG_PORT
    };

    msg.port = port;

    DoMethodA(obj, (Msg)&msg);
}
