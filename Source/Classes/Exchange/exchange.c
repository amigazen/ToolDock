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
#include <intuition/screens.h>
#include <intuition/imageclass.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

#include "class_def.h"
#include "applife.h"

#define DEFAULT_SIZE    40

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

STATIC STRPTR exchange_paths[] = {
    (STRPTR)"SYS:Utilities/Exchange",
    (STRPTR)"SYS:Prefs/Exchange",
    (STRPTR)"SYS:Tools/Commodities/Exchange",
    (STRPTR)"SYS:System/Commodities/Exchange",
    NULL
};

STATIC STRPTR find_exchange_path(VOID)
{
    UWORD i;
    BPTR lock;

    i = 0;
    while( exchange_paths[i] ) {
        lock = Lock(exchange_paths[i], ACCESS_READ);
        if( lock ) {
            UnLock(lock);
            return exchange_paths[i];
        }
        i++;
    }
    return NULL;
}

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

STATIC struct DiskObject *load_exchange_icon(STRPTR path)
{
    struct DiskObject *icon;
    struct TagItem tags[3];
    LONG error;
    UWORD i;

    icon = NULL;
    error = 0;

    if( IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = FALSE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        if( path ) {
            icon = GetIconTagList(path, tags);
        }
        if( icon == NULL ) {
            icon = GetIconTagList("PROGDIR:DockExchange", tags);
        }
        i = 0;
        while( icon == NULL && exchange_paths[i] ) {
            icon = GetIconTagList(exchange_paths[i], tags);
            i++;
        }
    }

    if( icon == NULL && path ) {
        icon = GetDiskObjectNew(path);
    }
    if( icon == NULL ) {
        icon = GetDiskObjectNew("PROGDIR:DockExchange");
    }
    i = 0;
    while( icon == NULL && exchange_paths[i] ) {
        icon = GetDiskObjectNew(exchange_paths[i]);
        i++;
    }
    if( icon == NULL ) {
        icon = GetDefDiskObject(WBTOOL);
    }

    layout_icon(icon);
    return icon;
}

STATIC VOID draw_fallback(struct RastPort *rp, struct Rect *bounds)
{
    struct IntuiText text;
    struct TextAttr ta;
    struct Screen *screen;
    struct DrawInfo *di;

    if( rp == NULL || bounds == NULL ) {
        return;
    }

    text.ITextFont = &ta;
    text.DrawMode = JAM1;
    text.NextText = NULL;
    text.IText = (STRPTR)"CX";
    text.FrontPen = 1;
    text.LeftEdge = bounds->x + 4;
    text.TopEdge = bounds->y + 4;
    ta.ta_Name = NULL;
    ta.ta_YSize = 8;
    ta.ta_Style = 0;
    ta.ta_Flags = 0;

    if( screen = LockPubScreen(NULL) ) {
        if( di = GetScreenDrawInfo(screen) ) {
            ta.ta_Name = di->dri_Font->tf_Message.mn_Node.ln_Name;
            ta.ta_YSize = di->dri_Font->tf_YSize;
            ta.ta_Style = di->dri_Font->tf_Style;
            ta.ta_Flags = di->dri_Font->tf_Flags;
            text.FrontPen = di->dri_Pens[TEXTPEN];
            text.LeftEdge = bounds->x + 2;
            text.TopEdge = bounds->y
                + (bounds->h - di->dri_Font->tf_YSize) / 2;
            FreeScreenDrawInfo(screen, di);
        }
        UnlockPubScreen(NULL, screen);
    }
    PrintIText(rp, &text, 0, 0);
}

ULONG __saveds exchange_lib_init(struct ExchangeLibData *eld)
{
    if( eld->dosBase = OpenLibrary("dos.library", 37) ) {
        DOSBase = eld->dosBase;
        if( eld->iconBase = OpenLibrary("icon.library", 37) ) {
            IconBase = eld->iconBase;
            applife_init();
            return 1;
        }
        CloseLibrary(eld->dosBase);
    }
    eld->dosBase = NULL;
    eld->iconBase = NULL;
    return 0;
}

ULONG __saveds exchange_lib_expunge(struct ExchangeLibData *eld)
{
    applife_expunge();
    if( eld->iconBase ) {
        CloseLibrary(eld->iconBase);
        eld->iconBase = NULL;
        IconBase = NULL;
    }
    if( eld->dosBase ) {
        CloseLibrary(eld->dosBase);
        eld->dosBase = NULL;
        DOSBase = NULL;
    }
    return 1;
}

DB_METHOD_D(NEW)

    data->launchPath = find_exchange_path();
    data->diskObj = load_exchange_icon(data->launchPath);
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
    data->launchPath = NULL;
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
        DrawIconStateA(msg->rp, data->diskObj, NULL, x, y,
            data->iconState, tags);
    } else if( data->diskObj && data->diskObj->do_Gadget.GadgetRender ) {
        x = env.gadgetBounds.x + (env.gadgetBounds.w - data->imageW) / 2;
        y = env.gadgetBounds.y + (env.gadgetBounds.h - data->imageH) / 2;
        DrawImage(msg->rp, data->diskObj->do_Gadget.GadgetRender, x, y);
    } else {
        draw_fallback(msg->rp, &env.gadgetBounds);
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

    data->iconState = IDS_SELECTED;
    data->counter = 2;
    TD_RequestDockGadgetDraw(o);

    data->launchPath = find_exchange_path();
    if( data->launchPath == NULL ) {
        TD_ShowError((STRPTR)MSG_ERR_NoExchange);
        return 1;
    }
    TD_RequestLaunch(o, data->launchPath, NULL, NULL, TRUE);
    return 1;
}

DB_METHOD_DM(POPUP, DockMessagePopup)

    if( ! applife_have_popup() ) {
        return 0;
    }
    if( applife_run_exchange_popup(msg->window, (STRPTR)MSG_Label,
        (STRPTR)MSG_POP_Show, (STRPTR)MSG_POP_Hide,
        (STRPTR)MSG_POP_Quit, (STRPTR)MSG_POP_None,
        (STRPTR)MSG_POP_Remove) == AL_POP_REMOVE ) {
        DoMethod(o, DM_REQ_REMOVE);
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

DB_METHOD_DM(GETLABEL, DockMessageGetLabel)

    msg->label = (STRPTR)MSG_Label;
    return 1;
}
