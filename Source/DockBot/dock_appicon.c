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

#include "dockicon_host.h"
#include "dock_appicon.h"
#include "applife.h"

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <workbench/icon.h>
#include <libraries/pm.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <clib/alib_protos.h>
#include <clib/utility_protos.h>
#include <clib/graphics_protos.h>
#include <clib/icon_protos.h>
#include <clib/intuition_protos.h>
#include <proto/popupmenu.h>

#ifndef ICONDRAWA_Borderless
#define ICONDRAWA_Borderless (TAG_USER + 0x9000 + 83)
#endif
#ifndef ICONDRAWA_EraseBackground
#define ICONDRAWA_EraseBackground (TAG_USER + 0x9000 + 71)
#endif
#ifndef IDS_NORMAL
#define IDS_NORMAL 0
#endif
#ifndef IDS_SELECTED
#define IDS_SELECTED 1
#endif

#ifndef WBAPPICONA_RenderHook
#define WBAPPICONA_RenderHook (TAG_USER + 0xA000 + 13)
#endif

extern struct Library *IconBase;
extern struct Library *GfxBase;

#pragma libcall IconBase DrawIconStateA a2 B210A9807
#pragma libcall IconBase GetIconRectangleA a8 CBA9805
#pragma libcall IconBase LayoutIconA c0 A9803

struct DockAppIconData {
    struct DockIcon *icon;
    UWORD imageW;
    UWORD imageH;
    UWORD iconState;
    UWORD counter;
};

STATIC VOID layout_draw_icon(struct DiskObject *icon)
{
    struct Screen *screen;

    if( icon == NULL || IconBase == NULL || IconBase->lib_Version < 44 ) {
        return;
    }
    screen = LockPubScreen(NULL);
    if( screen ) {
        LayoutIconA(icon, screen, NULL);
        UnlockPubScreen(NULL, screen);
    }
}

STATIC struct DiskObject *draw_obj(struct DockAppIconData *d)
{
    if( d->icon == NULL ) {
        return NULL;
    }
    if( d->icon->di_DrawObj ) {
        return d->icon->di_DrawObj;
    }
    return d->icon->di_DiskObj;
}

VOID dock_appicon_bind(Object *o, struct DockIcon *icon)
{
    struct DockAppIconData *d;

    if( o == NULL ) {
        return;
    }
    d = INST_DATA(OCLASS(o), o);
    d->icon = icon;
    d->imageW = 0;
    d->imageH = 0;
    d->iconState = IDS_NORMAL;
    d->counter = 0;
    layout_draw_icon(draw_obj(d));
}

STATIC VOID measure_icon(struct DockAppIconData *d, UWORD *w, UWORD *h)
{
    struct DiskObject *obj;
    struct Rectangle r;
    struct TagItem tags[2];

    *w = DEFAULT_SIZE;
    *h = DEFAULT_SIZE;
    obj = draw_obj(d);
    if( obj == NULL ) {
        return;
    }
    if( IconBase && IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONDRAWA_Borderless;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag = TAG_DONE;
        if( GetIconRectangleA(NULL, obj, NULL, &r, tags) ) {
            d->imageW = (UWORD)(r.MaxX - r.MinX + 1);
            d->imageH = (UWORD)(r.MaxY - r.MinY + 1);
            *w = d->imageW;
            *h = d->imageH;
        }
    } else {
        d->imageW = obj->do_Gadget.Width;
        d->imageH = obj->do_Gadget.Height;
        *w = d->imageW;
        *h = d->imageH;
    }
}

STATIC VOID draw_icon(Class *c, Object *o, struct RastPort *rp)
{
    struct DockAppIconData *d;
    struct GadgetEnvironment env;
    struct DiskObject *obj;
    struct AppIconRenderMsg arm;
    struct TagItem tags[3];
    struct Hook *hook;
    ULONG state;
    BOOL drawDefault;
    WORD x;
    WORD y;

    d = INST_DATA(c, o);
    obj = draw_obj(d);
    TD_GetDockGadgetEnvironment(o, &env);
    if( env.gadgetBounds.w == 0 || env.gadgetBounds.h == 0 ) {
        return;
    }

    state = d->iconState;
    drawDefault = TRUE;
    x = (WORD)(env.gadgetBounds.x + (env.gadgetBounds.w - d->imageW) / 2);
    y = (WORD)(env.gadgetBounds.y + (env.gadgetBounds.h - d->imageH) / 2);

    /* Same tags the render hook should pass to DrawIconStateA. */
    tags[0].ti_Tag = ICONDRAWA_Borderless;
    tags[0].ti_Data = TRUE;
    tags[1].ti_Tag = ICONDRAWA_EraseBackground;
    tags[1].ti_Data = FALSE;
    tags[2].ti_Tag = TAG_DONE;

    hook = NULL;
    if( d->icon ) {
        hook = d->icon->di_RenderHook;
    }
    if( hook && hook->h_Entry ) {
        arm.arm_RastPort = rp;
        arm.arm_Icon = obj;
        arm.arm_Label = d->icon ? (STRPTR)d->icon->di_Text : NULL;
        arm.arm_Tags = tags;
        arm.arm_Left = x;
        arm.arm_Top = y;
        arm.arm_Width = (WORD)env.gadgetBounds.w;
        arm.arm_Height = (WORD)env.gadgetBounds.h;
        arm.arm_State = state;
        /* wb.doc: TRUE = draw the regular image; FALSE = hook drew it. */
        if( ! CallHookPkt(hook, NULL, &arm) ) {
            drawDefault = FALSE;
        }
    }

    if( drawDefault && obj && IconBase && IconBase->lib_Version >= 44 ) {
        DrawIconStateA(rp, obj, NULL, x, y, state, tags);
    } else if( drawDefault && obj && obj->do_Gadget.GadgetRender ) {
        DrawImage(rp, obj->do_Gadget.GadgetRender, x, y);
    }

    if( env.showBorders ) {
        if( d->iconState == IDS_NORMAL ) {
            TD_DrawOutsetFrame(rp, &env.gadgetBounds);
        } else {
            TD_DrawInsetFrame(rp, &env.gadgetBounds);
        }
    }
}

STATIC BOOL open_icon_popup(struct DockIcon *icon, struct Window *win)
{
    struct PopupMenu *menu;
    ULONG result;
    ULONG supports;
    STRPTR title;

    if( icon == NULL || win == NULL || ! applife_have_popup() ) {
        return TRUE;
    }

    supports = icon->di_Supports;
    title = icon->di_Text[0] ? (STRPTR)icon->di_Text : (STRPTR)DB_APPICON_CLASS;

    menu = PMMenu(title),
        PMItem((STRPTR)"Open"),
            PM_UserData, (ULONG)AMCLASSICON_Open + 1,
            PMEnd,
        PMItem((STRPTR)"Information"),
            PM_UserData, (ULONG)AMCLASSICON_Information + 1,
            PM_Disabled, (supports & DISUP_INFORMATION) ? FALSE : TRUE,
            PMEnd,
        PMItem((STRPTR)"Empty Trash"),
            PM_UserData, (ULONG)AMCLASSICON_EmptyTrash + 1,
            PM_Disabled, (supports & DISUP_EMPTYTRASH) ? FALSE : TRUE,
            PMEnd,
        PMEnd;

    if( menu ) {
        result = PM_OpenPopupMenu(win, PM_Menu, menu, TAG_DONE);
        PM_FreePopupMenu(menu);
        if( result ) {
            dock_icon_send_event(icon, (UWORD)(result - 1), NULL, 0);
        }
    }
    return TRUE;
}

ULONG __saveds dock_appicon_dispatch(Class *c, Object *o, Msg msg)
{
    struct DockAppIconData *d;
    struct DockMessageDraw *dm;
    struct DockMessageGetSize *gsm;
    struct DockMessageGetLabel *glm;
    struct DockMessagePopup *pm;
    struct DockMessageDrop *drop;

    d = INST_DATA(c, o);

    switch( msg->MethodID )
    {
        case DM_CLICK:
            if( d->icon ) {
                d->iconState = IDS_SELECTED;
                d->counter = 2;
                TD_RequestDockGadgetDraw(o);
                dock_icon_send_event(d->icon, AMCLASSICON_Open, NULL, 0);
            }
            break;

        case DM_DROP:
            drop = (struct DockMessageDrop *)msg;
            if( d->icon ) {
                d->iconState = IDS_SELECTED;
                d->counter = 2;
                TD_RequestDockGadgetDraw(o);
                dock_icon_send_event(d->icon, AMCLASSICON_Open,
                    drop->paths, drop->pathCount);
            }
            break;

        case DM_POPUP:
            pm = (struct DockMessagePopup *)msg;
            return (ULONG)open_icon_popup(d->icon, pm->window);

        case DM_TICK:
            if( d->counter > 0 ) {
                d->counter--;
                if( d->counter == 0 ) {
                    d->iconState = IDS_NORMAL;
                    TD_RequestDockGadgetDraw(o);
                }
            }
            break;

        case DM_DRAW:
            dm = (struct DockMessageDraw *)msg;
            draw_icon(c, o, dm->rp);
            break;

        case DM_GETSIZE:
            gsm = (struct DockMessageGetSize *)msg;
            measure_icon(d, &gsm->w, &gsm->h);
            break;

        case DM_REMAP:
            layout_draw_icon(draw_obj(d));
            break;

        case DM_GETLABEL:
            glm = (struct DockMessageGetLabel *)msg;
            if( d->icon && d->icon->di_Text[0] ) {
                glm->label = (STRPTR)d->icon->di_Text;
            } else {
                glm->label = (STRPTR)DB_APPICON_CLASS;
            }
            break;

        case DM_BUILTIN:
            return (ULONG)TRUE;

        default:
            return DoSuperMethodA(c, o, msg);
    }

    return 1;
}

Class *init_dock_appicon_class(VOID)
{
    ULONG HookEntry();
    Class *c;

    c = MakeClass(DB_APPICON_CLASS, DB_ROOT_CLASS, NULL,
        sizeof(struct DockAppIconData), 0);
    if( c ) {
        c->cl_Dispatcher.h_Entry = HookEntry;
        c->cl_Dispatcher.h_SubEntry = dock_appicon_dispatch;
        AddClass(c);
    }
    return c;
}

BOOL free_dock_appicon_class(Class *c)
{
    return FreeClass(c);
}
