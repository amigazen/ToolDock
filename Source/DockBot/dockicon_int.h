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

#ifndef __DOCKICON_INT_H__
#define __DOCKICON_INT_H__

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <utility/hooks.h>
#include <intuition/classes.h>

#include "dockicon.h"

#define DI_TEXT_LEN 108

#define DISUP_OPEN          (1UL << 0)
#define DISUP_COPY          (1UL << 1)
#define DISUP_RENAME        (1UL << 2)
#define DISUP_INFO          (1UL << 3)
#define DISUP_SNAPSHOT      (1UL << 4)
#define DISUP_UNSNAPSHOT    (1UL << 5)
#define DISUP_LEAVEOUT      (1UL << 6)
#define DISUP_PUTAWAY       (1UL << 7)
#define DISUP_DELETE        (1UL << 8)
#define DISUP_FORMAT        (1UL << 9)
#define DISUP_EMPTYTRASH    (1UL << 10)
#define DISUP_EJECT         (1UL << 11)
#define DISUP_NOTIFYSEL     (1UL << 12)

struct DockIcon {
    struct MinNode n;
    ULONG di_ID;
    ULONG di_UserData;
    UBYTE di_Text[DI_TEXT_LEN];
    struct MsgPort *di_AppPort;
    BPTR di_Lock;
    struct DiskObject *di_DiskObj;
    struct DiskObject *di_DrawObj;
    struct Hook *di_RenderHook;
    ULONG di_Supports;
    Object *di_Gadget;
    struct DgNode *di_Node;
    struct DockWindow *di_Dock;
};

VOID dock_icons_init(struct DockWindow *dock);
VOID dock_icons_cleanup(struct DockWindow *dock);
VOID dock_icons_preserve(struct DockWindow *dock);
VOID dock_icons_restore(struct DockWindow *dock);
VOID handle_pub_port(struct DockWindow *dock);
VOID handle_di_reply(struct DockWindow *dock);
VOID dock_icon_send_event(struct DockIcon *icon, UWORD amClass,
    STRPTR *paths, UWORD pathCount);

Class *init_dock_appicon_class(VOID);
BOOL free_dock_appicon_class(Class *c);
VOID dock_appicon_bind(Object *o, struct DockIcon *icon);

#endif
