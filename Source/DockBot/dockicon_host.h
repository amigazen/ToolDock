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

#ifndef __DOCKICON_HOST_H__
#define __DOCKICON_HOST_H__

#include "dock.h"
#include "dockicon.h"

#define DI_TEXT_LEN 108

#define DISUP_OPEN          (1UL << 0)
#define DISUP_COPY          (1UL << 1)
#define DISUP_RENAME        (1UL << 2)
#define DISUP_INFORMATION   (1UL << 3)
#define DISUP_SNAPSHOT      (1UL << 4)
#define DISUP_UNSNAPSHOT    (1UL << 5)
#define DISUP_LEAVEOUT      (1UL << 6)
#define DISUP_PUTAWAY       (1UL << 7)
#define DISUP_DELETE        (1UL << 8)
#define DISUP_FORMATDISK    (1UL << 9)
#define DISUP_EMPTYTRASH    (1UL << 10)
#define DISUP_EJECTDISK     (1UL << 11)

struct DockIcon {
    struct MinNode n;
    ULONG di_ID;
    ULONG di_UserData;
    UBYTE di_Text[DI_TEXT_LEN];
    struct MsgPort *di_AppPort;
    BPTR di_Lock;
    struct DiskObject *di_DiskObj;
    struct DiskObject *di_DrawObj;
    ULONG di_Supports;
    struct Hook *di_RenderHook;
    BOOL di_NotifySelect;
    Object *di_Gadget;
    struct DgNode *di_Node;
    struct DockWindow *di_Dock;
};

BOOL init_dock_icons(struct DockWindow *dock);
VOID free_dock_icons(struct DockWindow *dock);
VOID dock_icons_preserve(struct DockWindow *dock);
VOID dock_icons_restore(struct DockWindow *dock);
VOID handle_pub_port(struct DockWindow *dock);
VOID handle_dockicon_replies(struct DockWindow *dock);
VOID dock_icon_send_event(struct DockIcon *icon, UWORD amClass,
    STRPTR *paths, UWORD pathCount);
VOID dock_icon_bind(Object *o, struct DockIcon *icon);

#endif
