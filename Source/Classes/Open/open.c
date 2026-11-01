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

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/intuition_protos.h>

#include "class_def.h"
#include "open_item.h"

#define DEFAULT_SIZE  40
#define S_NAME        "name"
#define S_DEFAULT     "default"

#ifndef ICONA_Dummy
#define ICONA_Dummy (TAG_USER + 0x9000)
#endif
#ifndef ICONA_ErrorCode
#define ICONA_ErrorCode (ICONA_Dummy + 1)
#endif
#ifndef ICONGETA_FailIfUnavailable
#define ICONGETA_FailIfUnavailable (ICONA_Dummy + 47)
#endif
#ifndef ICONDRAWA_Borderless
#define ICONDRAWA_Borderless (ICONA_Dummy + 83)
#endif
#ifndef ICONDRAWA_EraseBackground
#define ICONDRAWA_EraseBackground (ICONA_Dummy + 71)
#endif
#ifndef IDS_NORMAL
#define IDS_NORMAL 0
#endif
#ifndef IDS_SELECTED
#define IDS_SELECTED 1
#endif

#pragma libcall IconBase GetIconTagList b4 9802
#pragma libcall IconBase DrawIconStateA a2 B210A9807
#pragma libcall IconBase GetIconRectangleA a8 CBA9805
#pragma libcall IconBase LayoutIconA c0 A9803

struct Library *DOSBase;
struct Library *IconBase;
struct Library *WorkbenchBase;
struct Library *DataTypesBase;
struct OpenLibData *StaticData;

STATIC VOID layout_icon(struct DiskObject *icon)
{
    struct Screen *screen;

    if( icon == NULL || IconBase == NULL || IconBase->lib_Version < 44 ) {
        return;
    }
    if( screen = LockPubScreen(NULL) ) {
        LayoutIconA(icon, screen, NULL);
        UnlockPubScreen(NULL, screen);
    }
}

STATIC struct DiskObject *load_open_icon(VOID)
{
    struct DiskObject *icon;
    struct TagItem tags[3];
    LONG error;

    icon = NULL;
    error = 0;
    if( IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        icon = GetIconTagList("SYS:Utilities/MultiView", tags);
    }
    if( icon == NULL ) {
        icon = GetDiskObject("SYS:Utilities/MultiView");
    }
    if( icon == NULL && IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = FALSE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        icon = GetIconTagList("PROGDIR:DockOpen", tags);
    }
    if( icon == NULL ) {
        icon = GetDiskObjectNew("PROGDIR:DockOpen");
    }
    if( icon == NULL ) {
        icon = GetDefDiskObject(WBPROJECT);
    }
    layout_icon(icon);
    return icon;
}

ULONG __saveds open_lib_init(struct OpenLibData *old)
{
    if( old->dosBase = OpenLibrary("dos.library", 37) ) {
        DOSBase = old->dosBase;
        if( old->iconBase = OpenLibrary("icon.library", 37) ) {
            IconBase = old->iconBase;
            if( old->wbBase = OpenLibrary("workbench.library", 37) ) {
                WorkbenchBase = old->wbBase;
                old->dtBase = OpenLibrary("datatypes.library", 39);
                DataTypesBase = old->dtBase;
                StaticData = old;
                return 1;
            }
            CloseLibrary(old->iconBase);
        }
        CloseLibrary(old->dosBase);
    }
    old->dosBase = NULL;
    old->iconBase = NULL;
    old->wbBase = NULL;
    old->dtBase = NULL;
    return 0;
}

ULONG __saveds open_lib_expunge(struct OpenLibData *old)
{
    if( old->dtBase ) {
        CloseLibrary(old->dtBase);
        old->dtBase = NULL;
        DataTypesBase = NULL;
    }
    if( old->wbBase ) {
        CloseLibrary(old->wbBase);
        old->wbBase = NULL;
        WorkbenchBase = NULL;
    }
    if( old->iconBase ) {
        CloseLibrary(old->iconBase);
        old->iconBase = NULL;
        IconBase = NULL;
    }
    if( old->dosBase ) {
        CloseLibrary(old->dosBase);
        old->dosBase = NULL;
        DOSBase = NULL;
    }
    StaticData = NULL;
    return 1;
}

DB_METHOD_D(NEW)

    data->diskObj = load_open_icon();
    data->name = NULL;
    data->defaultTool = NULL;
    data->imageW = DEFAULT_SIZE;
    data->imageH = DEFAULT_SIZE;
    data->iconState = IDS_NORMAL;
    data->counter = 0;
    return 1;
}

DB_METHOD_D(DISPOSE)

    if( data->diskObj ) {
        FreeDiskObject(data->diskObj);
        data->diskObj = NULL;
    }
    FREE_STRING(data->name);
    FREE_STRING(data->defaultTool);
    return 1;
}

DB_METHOD_DM(DRAW, DockMessageDraw)

    struct GadgetEnvironment env;
    struct TagItem tags[3];
    LONG x;
    LONG y;

    TD_GetDockGadgetEnvironment(o, &env);
    if( env.gadgetBounds.w == 0 || env.gadgetBounds.h == 0 ) {
        return 1;
    }

    if( data->diskObj && IconBase && IconBase->lib_Version >= 44 ) {
        x = env.gadgetBounds.x + (env.gadgetBounds.w - data->imageW) / 2;
        y = env.gadgetBounds.y + (env.gadgetBounds.h - data->imageH) / 2;
        tags[0].ti_Tag = ICONDRAWA_Borderless;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag = ICONDRAWA_EraseBackground;
        tags[1].ti_Data = FALSE;
        tags[2].ti_Tag = TAG_DONE;
        DrawIconStateA(msg->rp, data->diskObj, NULL, x, y, data->iconState, tags);
    } else if( data->diskObj && data->diskObj->do_Gadget.GadgetRender ) {
        x = env.gadgetBounds.x + (env.gadgetBounds.w - data->imageW) / 2;
        y = env.gadgetBounds.y + (env.gadgetBounds.h - data->imageH) / 2;
        DrawImage(msg->rp, data->diskObj->do_Gadget.GadgetRender, x, y);
    }

    if( env.showBorders ) {
        if( data->iconState == IDS_NORMAL ) {
            TD_DrawOutsetFrame(msg->rp, &env.gadgetBounds);
        } else {
            TD_DrawInsetFrame(msg->rp, &env.gadgetBounds);
        }
    }
    return 1;
}

DB_METHOD_D(REMAP)

    layout_icon(data->diskObj);
    return 1;
}

DB_METHOD_D(TICK)

    if( data->counter > 0 ) {
        data->counter--;
        if( data->counter == 0 ) {
            data->iconState = IDS_NORMAL;
            TD_RequestDockGadgetDraw(o);
        }
    }
    return 1;
}

DB_METHOD_D(CLICK)

    STRPTR path;

    data->iconState = IDS_SELECTED;
    data->counter = 2;
    TD_RequestDockGadgetDraw(o);

    path = TD_SelectFile((STRPTR)MSG_FR_Choose, (STRPTR)MSG_FR_Ok,
        (STRPTR)MSG_FR_Cancel, NULL);
    if( path ) {
        if( ! open_dropped_path(path, data->defaultTool) ) {
            TD_ShowError((STRPTR)MSG_ERR_OpenFailed);
        }
        FREE_STRING(path);
    }
    return 1;
}

DB_METHOD_DM(DROP, DockMessageDrop)

    UWORD i;
    BOOL ok;

    DEBUG(TD_Printf(__METHOD__ " count = %ld\n", (LONG)msg->pathCount));

    data->iconState = IDS_SELECTED;
    data->counter = 2;
    TD_RequestDockGadgetDraw(o);

    ok = TRUE;
    for( i = 0; i < msg->pathCount; i++ ) {
        if( msg->paths[i] && ! open_dropped_path(msg->paths[i], data->defaultTool) ) {
            ok = FALSE;
        }
    }
    if( ! ok ) {
        TD_ShowError((STRPTR)MSG_ERR_OpenFailed);
    }
    return 1;
}

DB_METHOD_D(LAUNCHED)

    data->iconState = IDS_NORMAL;
    TD_RequestDockGadgetDraw(o);
    return 1;
}

DB_METHOD_DM(GETSIZE, DockMessageGetSize)

    struct Rectangle r;
    struct TagItem tags[2];

    msg->w = DEFAULT_SIZE;
    msg->h = DEFAULT_SIZE;
    if( data->diskObj && IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONDRAWA_Borderless;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag = TAG_DONE;
        if( GetIconRectangleA(NULL, data->diskObj, NULL, &r, tags) ) {
            msg->w = data->imageW = r.MaxX - r.MinX + 1;
            msg->h = data->imageH = r.MaxY - r.MinY + 1;
        }
    } else if( data->diskObj ) {
        msg->w = data->imageW = data->diskObj->do_Gadget.Width;
        msg->h = data->imageH = data->diskObj->do_Gadget.Height;
    }
    return 1;
}

DB_METHOD_DM(READCONFIG, DockMessageConfig)

    struct DockSettingValue v;

    while( TD_ReadSetting(msg->settings, &v) ) {
        if( IS_KEY(S_NAME, v) ) {
            GET_STRING(v, data->name)
        } else if( IS_KEY(S_DEFAULT, v) ) {
            GET_STRING(v, data->defaultTool)
        }
    }
    return 1;
}

DB_METHOD_DM(WRITECONFIG, DockMessageConfig)

    if( data->name ) {
        TD_WriteSetting(msg->settings, S_NAME, data->name);
    }
    if( data->defaultTool ) {
        TD_WriteSetting(msg->settings, S_DEFAULT, data->defaultTool);
    }
    return 1;
}

DB_METHOD_DM(GETLABEL, DockMessageGetLabel)

    if( data->name && data->name[0] ) {
        msg->label = data->name;
    } else {
        msg->label = (STRPTR)MSG_Label;
    }
    return 1;
}
