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
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

#include "dockbot.h"

#include "dockbot_protos.h"
#include "dockbot_pragmas.h"

#include "class_def.h"
#include "pragmas.h"
#include "net_socket.h"

struct Library *SocketBase = NULL;

extern struct GfxBase *GfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *DockGadgetBase;

#define COUNTER_LIB_OK 10
#define COUNTER_NOT_OK 20

#define NET_TOOL "C:SampleNetSpeed"

/**********************************************************
 **
 ** PRIVATE: OpenNetworkLib
 **
 **********************************************************/
BOOL OpenNetworkLib(VOID)
{
    struct NetLibData *nld;

    nld = (struct NetLibData *)GetLibData();

    if( nld->socketLib ) {
        DEBUG(TD_Printf(__METHOD__ "Socket Lib Available\n"));
        return TRUE;
    }

    DEBUG(TD_Printf(__METHOD__ "Opening Socket Lib..."));

    nld->socketLib = OpenLibrary("bsdsocket.library", 4);
    if( nld->socketLib )
    {
        DEBUG(TD_Printf(__METHOD__ "Connected\n"));

        SocketBase = nld->socketLib;
        return TRUE;
    }

    DEBUG(TD_Printf(__METHOD__ "Failed\n"));

    return FALSE;
}

/**********************************************************
 **
 ** PRIVATE: CloseNetworkLib
 **
 **********************************************************/
VOID CloseNetworkLib(VOID)
{
    struct NetLibData *nld;

    nld = (struct NetLibData *)GetLibData();

    if( nld->socketLib ) {
        CloseLibrary(nld->socketLib);
        nld->socketLib = NULL;
        SocketBase = NULL;
    }
}

/**********************************************************
 **
 ** PRIVATE: PrintSize
 **
 **********************************************************/

UBYTE * PrintSize(ULONG s, UBYTE * buf)
{
    STRPTR u[] = { "B", "KB", "MB", "GB", "TB" };
    ULONG r;
    ULONG i;

    r = 0;
    i = 0;

    while( s > 1024 && i < 4 ) {
        r = s % 1024;
        s >>= 10;
        i++;
    }
    if( r > 1000 ) {
        r = 0;
        s++;
    }
    r /= 100;

    sprintf(buf, "%lu.%lu %s", s, r, u[i]);

    return(buf);
}

/**********************************************************
 **
 ** PRIVATE: LIB_INIT
 **
 **********************************************************/

ULONG __saveds net_lib_init(struct NetLibData *nld)
{
    nld->socketLib = NULL;
    SocketBase = NULL;

    nld->graphicsLib = OpenLibrary("graphics.library", 37);
    if( nld->graphicsLib )
    {
        GfxBase = (struct GfxBase *)nld->graphicsLib;

        return 1;
    }

    return 0;
}

/**********************************************************
 **
 ** PRIVATE: LIB_EXPUNGE
 **
 **********************************************************/

ULONG __saveds net_lib_expunge(struct NetLibData *nld)
{
    if( nld->socketLib )
    {
        CloseLibrary(nld->socketLib);
        nld->socketLib = NULL;
        SocketBase = NULL;
    }

    if( nld->graphicsLib )
    {
        CloseLibrary(nld->graphicsLib);
        nld->graphicsLib = NULL;
    }

    return 1;
}

/**********************************************************
 **
 ** PRIVATE: NEW
 **
 **********************************************************/

DB_METHOD_D(NEW)

    data->counter = 0;
    data->clicked = FALSE;

    return 1;
}

/**********************************************************
 **
 ** PRIVATE: SET_TEXT_FONT
 **
 **********************************************************/

VOID __saveds set_text_font(struct IntuiText *text)
{
    struct Screen *screen;
    struct DrawInfo *drawInfo;
    struct TextAttr *ta;

    ta = (struct TextAttr *)text->ITextFont;

    screen = LockPubScreen(NULL);
    if( screen )
    {
        drawInfo = GetScreenDrawInfo(screen);
        if( drawInfo )
        {
            ta->ta_Name = drawInfo->dri_Font->tf_Message.mn_Node.ln_Name;
            ta->ta_YSize = drawInfo->dri_Font->tf_YSize;
            ta->ta_Style = drawInfo->dri_Font->tf_Style;
            ta->ta_Flags = drawInfo->dri_Font->tf_Flags;
            text->DrawMode = JAM1;
            text->NextText = NULL;

            FreeScreenDrawInfo(screen, drawInfo);
        }

        UnlockPubScreen(NULL, screen);
    }
}


/**********************************************************
 **
 ** PRIVATE: DRAW
 **
 **********************************************************/

VOID draw_net(struct RastPort *rp, struct DrawInfo *di, struct Rect *bounds, UWORD ix, STRPTR label, ULONG value, BOOL hasValue)
{
    struct IntuiText text;
    struct TextAttr ta;
    BYTE buf[32];
    BYTE svalue[32];
    struct Rect frame;

    frame.x = bounds->x;
    frame.h = bounds->h / 2;
    frame.y = bounds->y + (frame.h * ix);
    frame.w = bounds->w;

    SetAPen(rp, di->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, frame.x, frame.y, frame.x + frame.w - 1, frame.y + frame.h - 1);

    TD_DrawOutsetFrame(rp, &frame);

    frame.x += 1;
    frame.y += 1;
    frame.w -= 2;
    frame.h -= 2;

    TD_DrawInsetFrame(rp, &frame);

    if( hasValue ) {
        PrintSize(value, svalue);
        sprintf((STRPTR)&buf, label, svalue);
    } else {
        sprintf((STRPTR)&buf, label, "--");
    }

    text.ITextFont = &ta;
    ta.ta_Name = di->dri_Font->tf_Message.mn_Node.ln_Name;
    ta.ta_YSize = di->dri_Font->tf_YSize;
    ta.ta_Style = di->dri_Font->tf_Style;
    ta.ta_Flags = di->dri_Font->tf_Flags;
    text.DrawMode = JAM1;
    text.NextText = NULL;
    text.IText = (STRPTR)&buf;
    text.FrontPen = di->dri_Pens[TEXTPEN];
    text.LeftEdge = frame.x + 2;
    text.TopEdge = frame.y + (frame.h - di->dri_Font->tf_YSize) / 2;

    PrintIText(rp, &text, 0, 0);
}

/**********************************************************
 **
 ** METHOD: DRAW
 **
 **********************************************************/

DB_METHOD_DM(DRAW,DockMessageDraw)

    struct Screen *screen;
    struct DrawInfo *drawInfo;
    struct GadgetEnvironment env;
    SBQUAD_T byteRecv;
    SBQUAD_T byteSent;
    struct TagItem tags[3];
    BOOL hasValue;

    hasValue = FALSE;
    byteRecv.sbq_High = 0;
    byteRecv.sbq_Low = 0;
    byteSent.sbq_High = 0;
    byteSent.sbq_Low = 0;

    if( OpenNetworkLib() ) {

        tags[0].ti_Tag  = SBTM_GETREF(SBTC_GET_BYTES_RECEIVED);
        tags[0].ti_Data = (ULONG)&byteRecv;

        tags[1].ti_Tag  = SBTM_GETREF(SBTC_GET_BYTES_SENT);
        tags[1].ti_Data = (ULONG)&byteSent;

        tags[2].ti_Tag  = TAG_END;
        tags[2].ti_Data = 0;

        if( SocketBaseTagList(tags) == 0 )
        {
            hasValue = TRUE;
        }
        else
        {
            /* Stack is open but has no Roadshow byte counters. */
            CloseNetworkLib();

            data->counter = COUNTER_NOT_OK;
        }

    } else {

        data->counter = COUNTER_NOT_OK;
    }


    screen = LockPubScreen(NULL);
    if( screen )
    {
        drawInfo = GetScreenDrawInfo(screen);
        if( drawInfo )
        {
            TD_GetDockGadgetEnvironment(o, &env);

            draw_net(msg->rp, drawInfo, &env.gadgetBounds, 0, (STRPTR)MSG_LBL_RECV, byteRecv.sbq_Low, hasValue);
            draw_net(msg->rp, drawInfo, &env.gadgetBounds, 1, (STRPTR)MSG_LBL_SENT, byteSent.sbq_Low, hasValue);

            FreeScreenDrawInfo(screen, drawInfo);
        }

        UnlockPubScreen(NULL, screen);
    }
    return 1;
}

/**********************************************************
 **
 ** METHOD: CLICK
 **
 **********************************************************/

DB_METHOD_D(CLICK)

    TD_RequestDockGadgetDraw(o);

    DEBUG(TD_Printf(__METHOD__));

    data->clicked = TRUE;

    TD_RequestLaunch(o, NET_TOOL, NULL, NULL, TRUE);

    return 1;
}

/**********************************************************
 **
 ** METHOD: LAUNCHED
 **
 **********************************************************/

DB_METHOD_D(LAUNCHED)

    DEBUG(TD_Printf(__METHOD__));

    data->clicked = FALSE;

    TD_RequestDockGadgetDraw(o);

    return 1;
}

/**********************************************************
 **
 ** METHOD: TICK
 **
 **********************************************************/

DB_METHOD_D(TICK)

    DEBUG(TD_Printf(__METHOD__));

    if( data->counter > 0 )
    {
        data->counter--;
        return 1;
    }

    data->counter = COUNTER_LIB_OK;

    TD_RequestDockGadgetDraw(o);

    return 1;
}

/**********************************************************
 **
 ** METHOD: GETSIZE
 **
 **********************************************************/

DB_METHOD_M(GETSIZE,DockMessageGetSize)

    struct IntuiText text;
    struct TextAttr ta;
    UWORD rows;
    UWORD w;
    BYTE buf[32];
    struct Screen *screen;
    struct DrawInfo *di;

    w = 0;
    rows = 2;
    ta.ta_Name = (STRPTR)"topaz.font";
    ta.ta_YSize = 8;
    ta.ta_Style = 0;
    ta.ta_Flags = 0;
    text.FrontPen = 1;
    text.BackPen = 0;
    text.DrawMode = JAM1;
    text.ITextFont = &ta;
    text.IText = (STRPTR)buf;
    text.LeftEdge = 0;
    text.TopEdge = 0;
    text.NextText = NULL;

    sprintf((STRPTR)buf, "U: 999.9 MB");

    screen = LockPubScreen(NULL);
    if( screen )
    {
        di = GetScreenDrawInfo(screen);
        if( di )
        {
            ta.ta_Name = di->dri_Font->tf_Message.mn_Node.ln_Name;
            ta.ta_YSize = di->dri_Font->tf_YSize;
            ta.ta_Style = di->dri_Font->tf_Style;
            ta.ta_Flags = di->dri_Font->tf_Flags;

            w = IntuiTextLength(&text) + 10;

            FreeScreenDrawInfo(screen, di);
        }
        UnlockPubScreen(NULL, screen);
    }

    msg->w = w > DEFAULT_SIZE ? w : DEFAULT_SIZE;
    msg->h = rows * (2 + ta.ta_YSize) + 8;

    return 1;
}

/**********************************************************
 **
 ** END OF FILE
 **
 **********************************************************/
