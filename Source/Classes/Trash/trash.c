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
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <utility/tagitem.h>

#include "trash_move.h"
#include "applife.h"

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>

#include "class_def.h"

#define TRASH_VOLUME    "TRASH:"
#define DEFAULT_SIZE    40
#define TICK_INTERVAL   20

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
struct TrashLibData *StaticData;

STATIC BOOL name_is_info(STRPTR name)
{
    UWORD n;
    BOOL isInfo;

    isInfo = FALSE;
    if( name == NULL ) {
        return FALSE;
    }
    n = 0;
    while( name[n] ) {
        n++;
    }
    if( n < 5 ) {
        return FALSE;
    }
    if( name[n - 5] == '.'
        && (name[n - 4] == 'i' || name[n - 4] == 'I')
        && (name[n - 3] == 'n' || name[n - 3] == 'N')
        && (name[n - 2] == 'f' || name[n - 2] == 'F')
        && (name[n - 1] == 'o' || name[n - 1] == 'O') ) {
        isInfo = TRUE;
    }
    return isInfo;
}

STATIC BOOL trash_device_present(VOID)
{
    BPTR lock;

    lock = Lock(TRASH_VOLUME, ACCESS_READ);
    if( lock ) {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
}

STATIC BOOL trash_has_contents(VOID)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL full;

    full = FALSE;
    lock = Lock(TRASH_VOLUME, ACCESS_READ);
    if( lock == NULL ) {
        return FALSE;
    }

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if( fib && Examine(lock, fib) ) {
        while( ExNext(lock, fib) ) {
            if( fib->fib_FileName[0] == '.' ) {
                continue;
            }
            if( name_is_info(fib->fib_FileName) ) {
                continue;
            }
            full = TRUE;
            break;
        }
    }
    if( fib ) {
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
    return full;
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

STATIC struct DiskObject *load_trash_icon(VOID)
{
    struct DiskObject *icon;
    struct TagItem tags[3];
    LONG error;

    icon = NULL;
    error = 0;

    /* TRASH:Disk.info is the overlay volume icon (trashcan imagery). */
    if( IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = FALSE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        icon = GetIconTagList(TRASH_VOLUME, tags);
        if( icon == NULL ) {
            icon = GetIconTagList("TRASH:Disk", tags);
        }
    }

    if( icon == NULL ) {
        icon = GetDiskObjectNew(TRASH_VOLUME);
    }
    if( icon == NULL ) {
        icon = GetDiskObjectNew("TRASH:Disk");
    }
    if( icon == NULL ) {
        icon = GetDefDiskObject(WBGARBAGE);
    }

    layout_icon(icon);
    return icon;
}

STATIC VOID free_trash_icon(struct TrashGadgetData *data)
{
    if( data->diskObj ) {
        FreeDiskObject(data->diskObj);
        data->diskObj = NULL;
    }
}

STATIC VOID refresh_trash_state(struct TrashGadgetData *data)
{
    BOOL present;

    present = trash_device_present();
    if( present && data->diskObj == NULL ) {
        data->diskObj = load_trash_icon();
    }
    data->devicePresent = present;
    data->hasContents = present ? trash_has_contents() : FALSE;
}

STATIC BOOL move_paths_to_trash(STRPTR *paths, UWORD pathCount)
{
    UWORD i;
    BOOL ok;

    ok = TRUE;
    if( pathCount == 0 || paths == NULL ) {
        return FALSE;
    }
    for( i = 0; i < pathCount; i++ ) {
        if( paths[i] && ! trash_move_path(paths[i]) ) {
            DEBUG(TD_Printf(__METHOD__ "move failed: %s\n", paths[i]));
            ok = FALSE;
        }
    }
    return ok;
}

ULONG __saveds trash_lib_init(struct TrashLibData *tld)
{
    if( tld->dosBase = OpenLibrary("dos.library", 37) ) {
        DOSBase = tld->dosBase;
        if( tld->iconBase = OpenLibrary("icon.library", 37) ) {
            IconBase = tld->iconBase;
            if( tld->wbBase = OpenLibrary("workbench.library", 37) ) {
                WorkbenchBase = tld->wbBase;
                StaticData = tld;
                applife_init();
                return 1;
            }
            CloseLibrary(tld->iconBase);
        }
        CloseLibrary(tld->dosBase);
    }
    tld->dosBase = NULL;
    tld->iconBase = NULL;
    tld->wbBase = NULL;
    return 0;
}

ULONG __saveds trash_lib_expunge(struct TrashLibData *tld)
{
    applife_expunge();
    if( tld->wbBase ) {
        CloseLibrary(tld->wbBase);
        tld->wbBase = NULL;
        WorkbenchBase = NULL;
    }
    if( tld->iconBase ) {
        CloseLibrary(tld->iconBase);
        tld->iconBase = NULL;
        IconBase = NULL;
    }
    if( tld->dosBase ) {
        CloseLibrary(tld->dosBase);
        tld->dosBase = NULL;
        DOSBase = NULL;
    }
    StaticData = NULL;
    return 1;
}

DB_METHOD_D(NEW)

    DEBUG(TD_Printf(__METHOD__ "\n"));

    data->diskObj = load_trash_icon();
    data->imageW = DEFAULT_SIZE;
    data->imageH = DEFAULT_SIZE;
    data->iconState = IDS_NORMAL;
    data->counter = 0;
    refresh_trash_state(data);
    return 1;
}

DB_METHOD_D(DISPOSE)

    DEBUG(TD_Printf(__METHOD__ "\n"));

    free_trash_icon(data);
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
            data->hasContents ? IDS_SELECTED : data->iconState, tags);
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

    BOOL wasFull;
    BOOL wasPresent;

    if( data->counter > 0 ) {
        data->counter--;
        if( data->counter == 0 ) {
            data->iconState = IDS_NORMAL;
            TD_RequestDockGadgetDraw(o);
        }
        return 1;
    }

    wasFull = data->hasContents;
    wasPresent = data->devicePresent;
    refresh_trash_state(data);
    if( wasFull != data->hasContents || wasPresent != data->devicePresent ) {
        TD_RequestDockGadgetDraw(o);
    }
    data->counter = TICK_INTERVAL;
    return 1;
}

DB_METHOD_D(CLICK)

    DEBUG(TD_Printf(__METHOD__ "\n"));

    refresh_trash_state(data);
    if( ! data->devicePresent ) {
        TD_ShowError((STRPTR)MSG_ERR_NoTrash);
        return 1;
    }

    data->iconState = IDS_SELECTED;
    data->counter = 2;
    TD_RequestDockGadgetDraw(o);
    TD_RequestLaunch(o, TRASH_VOLUME, NULL, NULL, TRUE);
    return 1;
}

DB_METHOD_DM(POPUP, DockMessagePopup)

    ULONG choice;
    BOOL canEmpty;

    if( ! applife_have_popup() ) {
        return 0;
    }

    refresh_trash_state(data);
    canEmpty = (data->devicePresent && data->hasContents) ? TRUE : FALSE;
    choice = applife_open_action_popup(msg->window, (STRPTR)MSG_Label,
        (STRPTR)MSG_POP_Empty, canEmpty ? FALSE : TRUE, 1,
        (STRPTR)MSG_POP_Remove);
    if( choice == AL_POP_REMOVE ) {
        DoMethod(o, DM_REQ_REMOVE);
        return 1;
    }
    if( choice ) {
        if( ! data->devicePresent ) {
            TD_ShowError((STRPTR)MSG_ERR_NoTrash);
        } else if( ! trash_empty() ) {
            TD_ShowError((STRPTR)MSG_ERR_EmptyFailed);
        }
        refresh_trash_state(data);
        TD_RequestDockGadgetDraw(o);
    }
    return 1;
}

DB_METHOD_DM(DROP, DockMessageDrop)

    DEBUG(TD_Printf(__METHOD__ " count = %ld\n", (LONG)msg->pathCount));

    refresh_trash_state(data);
    if( ! data->devicePresent ) {
        TD_ShowError((STRPTR)MSG_ERR_NoTrash);
        return 1;
    }

    data->iconState = IDS_SELECTED;
    data->counter = 2;
    TD_RequestDockGadgetDraw(o);

    if( ! move_paths_to_trash(msg->paths, msg->pathCount) ) {
        TD_ShowError((STRPTR)MSG_ERR_MoveFailed);
    }

    refresh_trash_state(data);
    TD_RequestDockGadgetDraw(o);
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
