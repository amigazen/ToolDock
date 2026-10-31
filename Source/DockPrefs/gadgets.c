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

#include "prefs.h"

#include <clib/alib_protos.h>

VOID dock_gadget_get_info(Object *obj, STRPTR *name, STRPTR *version,
    STRPTR *description, STRPTR *copyright)
{
    struct DockMessageGetInfo msg;

    msg.MethodID = DM_GETINFO;
    msg.name = NULL;
    msg.version = NULL;
    msg.description = NULL;
    msg.copyright = NULL;

    DoMethodA(obj, (Msg)&msg);

    *name = msg.name;
    *version = msg.version;
    *description = msg.description;
    *copyright = msg.copyright;
}

VOID dock_gadget_get_label(Object *obj, STRPTR *label)
{
    struct DockMessageGetLabel msg;

    msg.MethodID = DM_GETLABEL;
    msg.label = NULL;

    DoMethodA(obj, (Msg)&msg);

    *label = msg.label;
}

BOOL dock_gadget_can_edit(Object *obj)
{
    struct DockMessageCanEdit msg;

    msg.MethodID = DM_CANEDIT;
    msg.canEdit = FALSE;

    DoMethodA(obj, (Msg)&msg);

    return msg.canEdit;
}

VOID dock_gadget_init_button(Object *obj, STRPTR name, STRPTR path)
{
    struct DockMessageInitButton msg;

    msg.MethodID = DM_INITBUTTON;
    msg.name = name;
    msg.path = path;

    DoMethodA(obj, (Msg)&msg);
}

VOID dock_gadget_read_settings(Object *obj, struct DockSettings *settings)
{
    struct DockMessageConfig msg;

    msg.MethodID = DM_READCONFIG;
    msg.settings = settings;

    DoMethodA(obj, (Msg)&msg);
}

VOID dock_gadget_write_settings(Object *obj, struct DockSettings *settings)
{
    struct DockMessageConfig msg;

    msg.MethodID = DM_WRITECONFIG;
    msg.settings = settings;

    DoMethodA(obj, (Msg)&msg);
}
