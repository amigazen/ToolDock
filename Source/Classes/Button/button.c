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
#include <dos/dos.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>

#include <clib/utility_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>

#include <stdio.h>

#include "class_def.h"

#include "button.h"
#include "dockbutton_cat.h"
#include "applife.h"

#define S_NAME      "name"
#define S_PATH      "path"
#define S_BRUSH     "brush"
#define S_START     "start"
#define S_ARGS      "args"
#define S_CON       "console"
#define S_HOTKEY    "key"

#define TT_CX       "TD_CX"
#define TT_RX       "TD_RX"
#define TT_RXSHOW   "TD_RXSHOW"
#define TT_RXHIDE   "TD_RXHIDE"
#define TT_RXQUIT   "TD_RXQUIT"
#define TT_RXOPEN   "TD_RXOPEN"
#define TT_LAUNCH   "TD_LAUNCH"
#define TT_OPEN     "TD_OPEN"

#define TT_LEN_CX   23
#define TT_LEN_RX   80
#define TT_LEN_CMD  128

#define ST_WB 0
#define ST_SH 1

struct Values StartValues[] = {
    { "wb", ST_WB },
    { "sh", ST_SH },
    { NULL, 0 }
};

STRPTR startTypes[] = { NULL /*"Workbench"*/, NULL /*"Shell"*/, NULL };

#define COPY_STRING(src, dst) \
    l = strlen(src);\
    CopyMem(src, dst, l);\
    dst += l;\
    *dst = ' ';\
    dst++; 

#define COPY_STRING_QUOTED(src, dst) \
    l = strlen(src);\
    *dst = '"';\
    dst++;\
    CopyMem(src, dst, l);\
    dst += l;\
    *dst = '"';\
    dst++;\
    *dst = ' ';\
    dst++;

enum {
    OBJ_STR_NAME = 1001,
    OBJ_STR_BRUSH,
    OBJ_STR_ARGS,
    OBJ_CYC_START,
    OBJ_STR_CON,
    OBJ_STR_HOTKEY,
    OBJ_ICON,
    OBJ_BTN_SELECT_TARGET,
    OBJ_BTN_SELECT_BRUSH,
    OBJ_TXT_PATH
};

struct Library *DOSBase;
struct Library *IconBase;
struct Library *GfxBase;
struct ButtonLibData *StaticData;

STATIC BOOL tt_eq(STRPTR val, STRPTR want)
{
    UBYTE a, b;

    if( val == NULL || want == NULL ) {
        return FALSE;
    }
    while( *val && *want ) {
        a = *val;
        b = *want;
        if( a >= 'A' && a <= 'Z' ) {
            a = (UBYTE)(a + 32);
        }
        if( b >= 'A' && b <= 'Z' ) {
            b = (UBYTE)(b + 32);
        }
        if( a != b ) {
            return FALSE;
        }
        val++;
        want++;
    }
    return (BOOL)(*val == *want);
}

STATIC VOID copy_td_string(STRPTR *dst, STRPTR src, UWORD maxLen)
{
    UWORD n;

    FREE_STRING(*dst);
    if( src == NULL || src[0] == 0 ) {
        return;
    }
    n = 0;
    while( src[n] && n < maxLen ) {
        n++;
    }
    if( n == 0 ) {
        return;
    }
    if( *dst = (STRPTR)TD_AllocMem(n + 1, MEMF_CLEAR) ) {
        CopyMem(src, *dst, n);
    }
}

STATIC VOID zero_applife(struct ButtonGadgetData *data)
{
    UWORD i;

    data->tdCx = NULL;
    data->tdRx = NULL;
    data->tdRxShow = NULL;
    data->tdRxHide = NULL;
    data->tdRxQuit = NULL;
    data->tdRxOpen = NULL;
    data->tdLaunch = TD_LAUNCH_MULTI;
    data->tdOpen = TD_OPEN_DEFAULT;
    data->appRunning = FALSE;
    data->hbCount = 0;
    data->pendingRxCount = 0;
    data->pendingRxTries = 0;
    data->tdCmdCount = 0;
    for( i = 0; i < 16; i++ ) {
        data->pendingRx[i] = NULL;
    }
    for( i = 0; i < AL_CMD_MAX; i++ ) {
        data->tdCmdLabel[i] = NULL;
        data->tdCmd[i] = NULL;
    }
}

STATIC VOID free_pending_rx(struct ButtonGadgetData *data)
{
    UWORD i;

    if( data->pendingRxCount > 16 ) {
        data->pendingRxCount = 0;
        data->pendingRxTries = 0;
        return;
    }
    for( i = 0; i < data->pendingRxCount; i++ ) {
        FREE_STRING(data->pendingRx[i]);
    }
    data->pendingRxCount = 0;
    data->pendingRxTries = 0;
}

STATIC VOID free_td_cmds(struct ButtonGadgetData *data)
{
    UWORD i;

    for( i = 0; i < AL_CMD_MAX; i++ ) {
        FREE_STRING(data->tdCmdLabel[i]);
        FREE_STRING(data->tdCmd[i]);
    }
    data->tdCmdCount = 0;
}

STATIC VOID free_td_strings(struct ButtonGadgetData *data)
{
    FREE_STRING(data->tdCx);
    FREE_STRING(data->tdRx);
    FREE_STRING(data->tdRxShow);
    FREE_STRING(data->tdRxHide);
    FREE_STRING(data->tdRxQuit);
    FREE_STRING(data->tdRxOpen);
    data->tdLaunch = TD_LAUNCH_MULTI;
    data->tdOpen = TD_OPEN_DEFAULT;
    free_pending_rx(data);
    free_td_cmds(data);
}

/* TD_CMD=Label|command  also TD_CMD2..TD_CMD8. Prefix WB / CLI / RX. */
STATIC VOID add_td_cmd(struct ButtonGadgetData *data, STRPTR spec)
{
    STRPTR bar;
    STRPTR label;
    STRPTR cmd;
    UBYTE tmp[80];
    UWORD n;
    UWORD i;

    if( data->tdCmdCount >= AL_CMD_MAX || spec == NULL || spec[0] == 0 ) {
        return;
    }

    bar = spec;
    while( *bar && *bar != '|' ) {
        bar++;
    }
    if( *bar == '|' ) {
        n = (UWORD)(bar - spec);
        if( n >= sizeof(tmp) ) {
            n = sizeof(tmp) - 1;
        }
        for( i = 0; i < n; i++ ) {
            tmp[i] = spec[i];
        }
        tmp[n] = 0;
        while( n > 0 && tmp[n - 1] == ' ' ) {
            n--;
            tmp[n] = 0;
        }
        label = tmp;
        cmd = bar + 1;
        while( *cmd == ' ' ) {
            cmd++;
        }
    } else {
        label = spec;
        cmd = spec;
    }
    if( label[0] == 0 || cmd[0] == 0 ) {
        return;
    }

    copy_td_string(&data->tdCmdLabel[data->tdCmdCount], label, 40);
    copy_td_string(&data->tdCmd[data->tdCmdCount], cmd, TT_LEN_CMD);
    if( data->tdCmdLabel[data->tdCmdCount] && data->tdCmd[data->tdCmdCount] ) {
        data->tdCmdCount++;
    } else {
        FREE_STRING(data->tdCmdLabel[data->tdCmdCount]);
        FREE_STRING(data->tdCmd[data->tdCmdCount]);
    }
}

STATIC VOID parse_td_cmds(struct ButtonGadgetData *data, STRPTR *tts)
{
    UWORD i;
    UBYTE key[8];
    STRPTR val;

    key[0] = 'T';
    key[1] = 'D';
    key[2] = '_';
    key[3] = 'C';
    key[4] = 'M';
    key[5] = 'D';
    key[6] = 0;
    key[7] = 0;

    for( i = 0; i < AL_CMD_MAX; i++ ) {
        if( i == 0 ) {
            key[6] = 0;
        } else {
            key[6] = (UBYTE)('1' + i);
            key[7] = 0;
        }
        val = FindToolType(tts, key);
        if( val && val[0] ) {
            add_td_cmd(data, val);
        }
    }
}

STATIC VOID parse_tooltypes(struct ButtonGadgetData *data)
{
    STRPTR *tts;
    STRPTR val;

    free_td_strings(data);

    if( data->diskObj == NULL || data->diskObj->do_ToolTypes == NULL ) {
        return;
    }

    tts = data->diskObj->do_ToolTypes;
    copy_td_string(&data->tdCx, FindToolType(tts, (STRPTR)TT_CX), TT_LEN_CX);
    copy_td_string(&data->tdRx, FindToolType(tts, (STRPTR)TT_RX), TT_LEN_RX);
    copy_td_string(&data->tdRxShow, FindToolType(tts, (STRPTR)TT_RXSHOW), TT_LEN_CMD);
    copy_td_string(&data->tdRxHide, FindToolType(tts, (STRPTR)TT_RXHIDE), TT_LEN_CMD);
    copy_td_string(&data->tdRxQuit, FindToolType(tts, (STRPTR)TT_RXQUIT), TT_LEN_CMD);
    copy_td_string(&data->tdRxOpen, FindToolType(tts, (STRPTR)TT_RXOPEN), TT_LEN_CMD);
    parse_td_cmds(data, tts);

    val = FindToolType(tts, (STRPTR)TT_LAUNCH);
    if( tt_eq(val, (STRPTR)"ONCE") ) {
        data->tdLaunch = TD_LAUNCH_ONCE;
    } else if( tt_eq(val, (STRPTR)"DOCS") ) {
        data->tdLaunch = TD_LAUNCH_DOCS;
    } else {
        data->tdLaunch = TD_LAUNCH_MULTI;
    }

    val = FindToolType(tts, (STRPTR)TT_OPEN);
    if( tt_eq(val, (STRPTR)"WB") ) {
        data->tdOpen = TD_OPEN_WB;
    } else if( tt_eq(val, (STRPTR)"CLI") ) {
        data->tdOpen = TD_OPEN_CLI;
    } else if( tt_eq(val, (STRPTR)"RX") ) {
        data->tdOpen = TD_OPEN_RX;
    } else {
        data->tdOpen = TD_OPEN_DEFAULT;
    }
}

STATIC UWORD effective_open(struct ButtonGadgetData *data)
{
    if( data->tdOpen == TD_OPEN_DEFAULT ) {
        if( data->startType == ST_WB ) {
            return TD_OPEN_WB;
        }
        return TD_OPEN_CLI;
    }
    return data->tdOpen;
}

STATIC BOOL has_rxopen(struct ButtonGadgetData *data)
{
    if( data->tdRx && data->tdRx[0] && data->tdRxOpen && data->tdRxOpen[0] ) {
        return TRUE;
    }
    return FALSE;
}

STATIC BOOL prefix_eq(STRPTR s, STRPTR pre)
{
    UBYTE a, b;

    if( s == NULL || pre == NULL ) {
        return FALSE;
    }
    while( *s && *pre ) {
        a = *s;
        b = *pre;
        if( a >= 'A' && a <= 'Z' ) {
            a = (UBYTE)(a + 32);
        }
        if( b >= 'A' && b <= 'Z' ) {
            b = (UBYTE)(b + 32);
        }
        if( a != b ) {
            return FALSE;
        }
        s++;
        pre++;
    }
    if( *pre ) {
        return FALSE;
    }
    if( *s == 0 || *s == ' ' ) {
        return TRUE;
    }
    return FALSE;
}

STATIC BOOL has_colon(STRPTR s)
{
    if( s == NULL ) {
        return FALSE;
    }
    while( *s ) {
        if( *s == ':' ) {
            return TRUE;
        }
        s++;
    }
    return FALSE;
}

STATIC VOID run_custom_cmd(Object *o, struct ButtonGadgetData *data, UWORD idx)
{
    STRPTR cmd;
    BOOL wb;

    if( idx >= data->tdCmdCount ) {
        return;
    }
    cmd = data->tdCmd[idx];
    if( cmd == NULL || cmd[0] == 0 ) {
        return;
    }

    if( prefix_eq(cmd, (STRPTR)"RX") ) {
        cmd += 2;
        while( *cmd == ' ' ) {
            cmd++;
        }
        if( data->tdRx && data->tdRx[0] && cmd[0] ) {
            applife_send_rexx(data->tdRx, cmd);
        }
        return;
    }

    wb = (data->startType == ST_WB) ? TRUE : FALSE;
    if( prefix_eq(cmd, (STRPTR)"WB") ) {
        cmd += 2;
        while( *cmd == ' ' ) {
            cmd++;
        }
        wb = TRUE;
    } else if( prefix_eq(cmd, (STRPTR)"CLI") ) {
        cmd += 3;
        while( *cmd == ' ' ) {
            cmd++;
        }
        wb = FALSE;
    } else if( data->tdRx && data->tdRx[0] && ! has_colon(cmd) ) {
        applife_send_rexx(data->tdRx, cmd);
        return;
    }

    if( cmd[0] ) {
        TD_RequestLaunch(o, cmd, NULL, NULL, wb);
    }
}

STATIC BOOL can_control(struct ButtonGadgetData *data)
{
    if( data->tdCx && data->tdCx[0] ) {
        return TRUE;
    }
    if( data->tdRx && data->tdRx[0] ) {
        return TRUE;
    }
    return FALSE;
}

STATIC VOID flash_button(Object *o, struct ButtonGadgetData *data)
{
    data->counter = 2;
    data->iconState = 1;
    TD_RequestDockGadgetDraw(o);
}

STATIC VOID queue_pending_rx(struct ButtonGadgetData *data, STRPTR *paths,
    UWORD count)
{
    UWORD i;
    UWORD n;
    UWORD len;

    free_pending_rx(data);
    if( paths == NULL || count == 0 ) {
        return;
    }
    n = count;
    if( n > 16 ) {
        n = 16;
    }
    for( i = 0; i < n; i++ ) {
        if( paths[i] == NULL ) {
            continue;
        }
        len = strlen(paths[i]);
        if( data->pendingRx[data->pendingRxCount] =
            (STRPTR)TD_AllocMem(len + 1, MEMF_CLEAR) ) {
            CopyMem(paths[i], data->pendingRx[data->pendingRxCount], len);
            data->pendingRxCount++;
        }
    }
    if( data->pendingRxCount ) {
        data->pendingRxTries = AL_RXOPEN_TRIES;
    }
}

STATIC VOID send_rxopen_paths(struct ButtonGadgetData *data, STRPTR *paths,
    UWORD count)
{
    UWORD i;
    STRPTR cmd;

    if( data->tdRx == NULL || data->tdRxOpen == NULL ) {
        return;
    }
    for( i = 0; i < count; i++ ) {
        if( paths[i] == NULL ) {
            continue;
        }
        cmd = applife_format_rxopen(data->tdRxOpen, paths[i]);
        if( cmd ) {
            applife_send_rexx(data->tdRx, cmd);
            FREE_STRING(cmd);
        }
    }
}

STATIC VOID try_pending_rx(struct ButtonGadgetData *data)
{
    if( data->pendingRxCount == 0 ) {
        return;
    }
    if( data->appRunning ) {
        send_rxopen_paths(data, data->pendingRx, data->pendingRxCount);
        free_pending_rx(data);
        return;
    }
    if( data->pendingRxTries > 0 ) {
        data->pendingRxTries--;
        if( data->pendingRxTries == 0 ) {
            free_pending_rx(data);
        }
    }
}

STATIC VOID update_running(Object *o, struct ButtonGadgetData *data)
{
    BOOL running;

    applife_reap();
    running = applife_is_running(data->tdCx, data->tdRx);
    if( running != data->appRunning ) {
        data->appRunning = running;
        TD_RequestDockGadgetDraw(o);
    }
    try_pending_rx(data);
}

VOID dock_button_launch(Object *o, struct ButtonGadgetData *dbd, Msg msg, STRPTR* dropNames, UWORD dropCount) 
{
    STRPTR args = NULL;
    STRPTR pos;
    UWORD i, len = 0, l;

    if( dbd->args ) {
        len += strlen(dbd->args) + 1;
    }
    for( i = 0; i < dropCount; i++ ) {
        len += strlen(dropNames[i]) + 3;
    }
    if( len > 0 ) {
        if( !( args = (STRPTR)TD_AllocMem(len, MEMF_CLEAR) ) ) {
            return;
        }

        pos = args;
        if( dbd->args ) {
            COPY_STRING(dbd->args, pos);
        }

        for( i = 0; i < dropCount; i++ ) {
            COPY_STRING_QUOTED(dropNames[i], pos);
        }
        pos--;
        *pos = '\0'; 
    }

    TD_RequestLaunch(o, dbd->path, args, dbd->con, dbd->startType == ST_WB ? TRUE : FALSE);
}

STATIC VOID launch_one_file(Object *o, struct ButtonGadgetData *data,
    STRPTR path)
{
    STRPTR names[1];

    names[0] = path;
    dock_button_launch(o, data, NULL, names, 1);
}

STATIC VOID handle_files(Object *o, struct ButtonGadgetData *data,
    STRPTR *paths, UWORD count)
{
    UWORD i;
    UWORD openHow;

    openHow = effective_open(data);

    /* Unique-instance open command takes dropped names, not a new launch. */
    if( has_rxopen(data) ) {
        if( data->appRunning ) {
            send_rxopen_paths(data, paths, count);
            return;
        }
        dock_button_launch(o, data, NULL, NULL, 0);
        queue_pending_rx(data, paths, count);
        data->hbCount = AL_HB_TICKS;
        return;
    }

    if( openHow == TD_OPEN_RX ) {
        return;
    }

    if( data->tdLaunch == TD_LAUNCH_DOCS ) {
        for( i = 0; i < count; i++ ) {
            if( paths[i] ) {
                launch_one_file(o, data, paths[i]);
            }
        }
        data->hbCount = AL_HB_TICKS;
        return;
    }

    dock_button_launch(o, data, NULL, paths, count);
    data->hbCount = AL_HB_TICKS;
}

STATIC VOID dock_button_activate(Object *o, struct ButtonGadgetData *data,
    STRPTR *paths, UWORD count)
{
    if( count > 0 ) {
        if( data->tdLaunch == TD_LAUNCH_ONCE && data->appRunning
            && ! has_rxopen(data) ) {
            applife_show(data->tdCx, data->tdRx, data->tdRxShow);
            return;
        }
        handle_files(o, data, paths, count);
        return;
    }

    if( data->appRunning ) {
        if( data->tdLaunch == TD_LAUNCH_ONCE
            || data->tdLaunch == TD_LAUNCH_DOCS ) {
            applife_show(data->tdCx, data->tdRx, data->tdRxShow);
            return;
        }
    }

    dock_button_launch(o, data, NULL, NULL, 0);
    data->hbCount = AL_HB_TICKS;
}

VOID load_icon(struct ButtonGadgetData *data, STRPTR path)
{
    struct Screen *screen;

    DEBUG(TD_Printf(__METHOD__ "path = %s\n", path));

    if( data->diskObj ) {
        FreeDiskObject(data->diskObj);
        data->diskObj = NULL;
    }

    if( path && (data->diskObj = GetDiskObjectNew(path)) ) {
        if( screen = LockPubScreen(NULL) ) {

            LayoutIconA(data->diskObj, screen, NULL);

            UnlockPubScreen(NULL, screen);
        }
    }

    parse_tooltypes(data);
}

STRPTR get_start_type(struct Values *values, UWORD val) {

    while( values->Name ) {
        if( values->Value == val ) {
            return values->Name;
        }
        values++;
    }
    return NULL;
}

VOID select_target(struct ButtonGadgetData *data, struct TR_Project *window)
{
    STRPTR oldPath, newPath, newName;
    UWORD l;

    oldPath = data->selectedPath == NULL ? data->path : data->selectedPath;

    if( newPath = TD_SelectFile((STRPTR)MSG_FR_ChooseApplication
                                , (STRPTR)MSG_FR_OkText
                                , (STRPTR)MSG_FR_CancelText
                                , oldPath) ) {

        FREE_STRING(data->selectedPath);

        l = strlen(newPath) + 1;        
        if( data->selectedPath = (STRPTR)TD_AllocMem(l, MEMF_CLEAR) ) {
    
            CopyMem(newPath, data->selectedPath, l);
            TR_SetAttribute(window, OBJ_TXT_PATH, TRAT_Text, (ULONG)data->selectedPath);

            newName = FilePart(data->selectedPath);
            TR_SetAttribute(window, OBJ_STR_NAME, 0L, (ULONG)newName);
                
            load_icon(data, data->selectedPath);
            TR_SetAttribute(window, OBJ_ICON, TRAT_Icon_DiskObj, (ULONG)data->diskObj); 
        }

        FREE_STRING(newPath);
    }     
}

BOOL set_brush(struct ButtonGadgetData *data, struct TR_Project *window, STRPTR newPath)
{
    DEBUG(TD_Printf(__METHOD__ "newPath = %s\n", newPath));
    
    if( newPath ) {

        if( data->brushImg ) {
            TD_FreeBrush(data->brushImg);   
        }

        data->brushImg = TD_LoadBrush(newPath, BF_CREATE_MASK);

        TR_SetAttribute(window, OBJ_ICON, TRAT_Icon_Brush, (ULONG)data->brushImg);

        return TRUE;
    }

    return FALSE;
}

VOID select_brush(struct ButtonGadgetData *data, struct TR_Project *window)
{
    STRPTR oldPath, newPath;
    
    DEBUG(TD_Printf(__METHOD__ "\n"));

    oldPath = (STRPTR)TR_GetAttribute(window, OBJ_STR_BRUSH, 0L);

    if( newPath = TD_SelectFile((STRPTR)MSG_FR_ChooseBrush
                                , (STRPTR)MSG_FR_OkText
                                , (STRPTR)MSG_FR_CancelText
                                , oldPath) ) {

        DEBUG(TD_Printf(__METHOD__ "newPath = %s\n", newPath));

        if( set_brush(data, window, newPath) ) {
    
            DEBUG(TD_Printf(__METHOD__ "Set UI brush path\n"));            

            TR_SetAttribute(window, OBJ_STR_BRUSH, 0L, (ULONG)newPath);
        }

        DEBUG(TD_Printf(__METHOD__, "Free path string\n"));

        FREE_STRING(newPath);
    }

    DEBUG(TD_Printf(__METHOD__ "Done\n"));    
}

ULONG __saveds button_lib_init(struct ButtonLibData* cld)
{
    if( cld->dosBase = OpenLibrary("dos.library", 37) ) {

        DOSBase = cld->dosBase;

        if( cld->gfxBase = OpenLibrary("graphics.library", 37) ) {

            GfxBase = cld->gfxBase;

            if( cld->iconBase = OpenLibrary("icon.library", 46) ) {

                IconBase = cld->iconBase;
                applife_init();

                return 1;
            }
            CloseLibrary(cld->gfxBase);
        }
        CloseLibrary(cld->dosBase);
    }
    cld->iconBase = NULL;
    cld->dosBase = NULL;

    return 0;
}

ULONG __saveds button_lib_expunge(struct ButtonLibData *cld)
{
    applife_expunge();

    if( cld->iconBase ) {
        CloseLibrary(cld->iconBase);
    }

    if( cld->gfxBase ) {
        CloseLibrary(cld->gfxBase);
    }

    if( cld->dosBase ) {
        CloseLibrary(cld->dosBase);
    }

    return 1;
}

DB_METHOD_D(DISPOSE)
    FREE_STRING(data->name);
    FREE_STRING(data->path);
    FREE_STRING(data->args);
    FREE_STRING(data->con);    
    FREE_STRING(data->hotKey);
    FREE_STRING(data->brushPath);
    free_td_strings(data);

    if( data->diskObj ) {
        FreeDiskObject(data->diskObj);
    }

    if( data->brushImg ) {
        DEBUG(TD_Printf(__METHOD__ "Free brush %lx\n", data->brushImg));
        TD_FreeBrush(data->brushImg);
    }

    return 1;
}

DB_METHOD_DM(DRAW, DockMessageDraw)

    struct GadgetEnvironment env;
    ULONG drawState;
    UWORD mx, my;
    
    TD_GetDockGadgetEnvironment(o, &env);

    if( env.gadgetBounds.w == 0 || env.gadgetBounds.h == 0 ) {
        return 1;
    }

    drawState = data->iconState;
    if( data->appRunning && data->counter == 0 ) {
        drawState = 1;
    }

    if( data->brushImg ) {

        TD_DrawBrush(data->brushImg,
            msg->rp,
            0, 0,
            env.gadgetBounds.x + (env.gadgetBounds.w - data->imageW) / 2, 
            env.gadgetBounds.y + (env.gadgetBounds.h - data->imageH) / 2,
            data->imageW, data->imageH);

    }
    else if( data->diskObj ) {

        DrawIconState(msg->rp, data->diskObj, 
                NULL, 
                env.gadgetBounds.x + (env.gadgetBounds.w - data->imageW) / 2, 
                env.gadgetBounds.y + (env.gadgetBounds.h - data->imageH) / 2,
                drawState,
                ICONDRAWA_Borderless, TRUE, 
                ICONDRAWA_EraseBackground, FALSE,
                TAG_END); 
    }

    if( data->appRunning ) {
        mx = env.gadgetBounds.x + env.gadgetBounds.w / 2;
        my = env.gadgetBounds.y + env.gadgetBounds.h - 4;
        SetAPen(msg->rp, 3);
        RectFill(msg->rp, mx - 2, my - 2, mx + 1, my + 1);
    }

    if( env.showBorders ) {
        if( data->iconState == 0 ) {
            TD_DrawOutsetFrame(msg->rp, &env.gadgetBounds);
        } else {
            TD_DrawInsetFrame(msg->rp, &env.gadgetBounds);
        }
    }
    return 1;
}

DB_METHOD_D(REMAP)

    struct Screen *screen;
    
    if( screen = LockPubScreen(NULL) ) {

        if( data->diskObj ) {
            LayoutIconA(data->diskObj, screen, NULL);
        }

        UnlockPubScreen(NULL, screen);
    }

    return 1;
}

DB_METHOD_D(TICK)

    if( data->counter > 0 ) {
        data->counter--;
        if( data->counter <= 0 ) {
            data->iconState = 1 - data->iconState;
            TD_RequestDockGadgetDraw(o);
        }
    }

    if( can_control(data) || data->pendingRxCount ) {
        data->hbCount++;
        if( data->hbCount >= AL_HB_TICKS ) {
            data->hbCount = 0;
            update_running(o, data);
        }
    }

    return 1;
}

DB_METHOD_D(CLICK)

    dock_button_activate(o, data, NULL, 0);
    flash_button(o, data);

    return 1;
}

DB_METHOD_DM(DROP,DockMessageDrop)

    dock_button_activate(o, data, msg->paths, msg->pathCount);
    flash_button(o, data);

    return 1;
}

DB_METHOD_DM(POPUP, DockMessagePopup)

    UWORD flags;
    ULONG choice;
    STRPTR title;

    if( ! applife_have_popup() ) {
        return 0;
    }

    flags = AL_CAN_OPEN;
    if( data->appRunning ) {
        flags = 0;
        if( data->tdCx || data->tdRxShow ) {
            flags |= AL_CAN_SHOW;
        }
        if( data->tdCx || data->tdRxHide ) {
            flags |= AL_CAN_HIDE;
        }
        if( data->tdCx || data->tdRxQuit ) {
            flags |= AL_CAN_QUIT;
        }
    }

    title = data->name;
    if( title == NULL && data->path ) {
        title = FilePart(data->path);
    }

    choice = applife_open_app_popup(msg->window, title,
        (STRPTR)MSG_POP_Open, (STRPTR)MSG_POP_Show,
        (STRPTR)MSG_POP_Hide, (STRPTR)MSG_POP_Quit,
        (STRPTR)MSG_POP_Remove, data->tdCmdLabel, data->tdCmdCount, flags);

    if( choice == AL_POP_REMOVE ) {
        DoMethod(o, DM_REQ_REMOVE);
        return 1;
    }
    if( choice >= AL_POP_CMD && choice < (ULONG)(AL_POP_CMD + AL_CMD_MAX) ) {
        run_custom_cmd(o, data, (UWORD)(choice - AL_POP_CMD));
        flash_button(o, data);
        return 1;
    }

    switch( choice ) {
        case AL_POP_OPEN:
            dock_button_activate(o, data, NULL, 0);
            flash_button(o, data);
            break;
        case AL_POP_SHOW:
            applife_show(data->tdCx, data->tdRx, data->tdRxShow);
            break;
        case AL_POP_HIDE:
            applife_hide(data->tdCx, data->tdRx, data->tdRxHide);
            break;
        case AL_POP_QUIT:
            applife_quit(data->tdCx, data->tdRx, data->tdRxQuit);
            data->appRunning = FALSE;
            TD_RequestDockGadgetDraw(o);
            break;
        default:
            break;
    }

    return 1;
}

DB_METHOD_DM(GETSIZE,DockMessageGetSize)

    struct Rectangle r;

    msg->w = DEFAULT_SIZE;
    msg->h = DEFAULT_SIZE;

    if( data->brushImg ) {

        TD_GetBrushSize(data->brushImg, &data->imageW, &data->imageH);
        msg->w = data->imageW + 2;
        msg->h = data->imageH + 2;     

    } else if( data->diskObj ) {

        if( GetIconRectangle(NULL, data->diskObj, NULL, &r,
                ICONDRAWA_Borderless, TRUE, TAG_END) ) {

            msg->w = data->imageW = r.MaxX - r.MinX + 1;
            msg->h = data->imageH = r.MaxY - r.MinY + 1;
        }
    }

    return 1;
}

DB_METHOD_DM(READCONFIG,DockMessageConfig)
    
    struct DockSettingValue v;
    struct Values *vals;
    UWORD len;

    zero_applife(data);

    while( TD_ReadSetting(msg->settings, &v) ) {
        
        if( IS_KEY(S_NAME, v) ) {
            GET_STRING(v, data->name)     
        }
        else if( IS_KEY(S_PATH, v) ) {
            GET_STRING(v, data->path)
        }
        else if( IS_KEY(S_START, v) ) {
            GET_VALUE(v, StartValues, vals, len, data->startType)
        }
        else if( IS_KEY(S_ARGS, v) ) {
            GET_STRING(v, data->args)
        }
        else if( IS_KEY(S_CON, v) ) {
            GET_STRING(v, data->con)
        }
        else if( IS_KEY(S_HOTKEY, v) ) {
            GET_STRING(v, data->hotKey)
        }
        else if( IS_KEY(S_BRUSH, v) ) {
            GET_STRING(v, data->brushPath)

            data->brushImg = TD_LoadBrush(data->brushPath, BF_CREATE_MASK);
        }
    }    

    load_icon(data, data->path);

    return 1;
}

DB_METHOD_DM(WRITECONFIG,DockMessageConfig)

    struct DockSettings *s = msg->settings;
    if( data->name ) {
        TD_WriteSetting(s, S_NAME, data->name);
    }
    if( data->path ) {
        TD_WriteSetting(s, S_PATH, data->path);
    }
    if( data->brushPath ) {
        TD_WriteSetting(s, S_BRUSH, data->brushPath);
    }
    if( data->args ) {
        TD_WriteSetting(s, S_ARGS, data->args);
    }
    if( data->hotKey ) {
        TD_WriteSetting(s, S_HOTKEY, data->hotKey);
    }

    {
        STRPTR startName;
        startName = get_start_type(StartValues, data->startType);
        if( startName == NULL ) {
            startName = StartValues[0].Name;
        }
        if( startName ) {
            TD_WriteSetting(s, S_START, startName);
        }
    }

    if( data->con ) {
        TD_WriteSetting(s, S_CON, data->con);
    }

    return 1;
}

DB_METHOD_M(INITEDIT, DockMessageEditorInit)

    if( ! init_icon_class(msg->app) ) { 
        return 0;
    }

    return 1;
}

DB_METHOD_M(CANEDIT, DockMessageCanEdit)

    msg->canEdit = TRUE;

    return 1;
}

DB_METHOD_DM(GETEDITOR, DockMessageGetEditor)

    struct IconInit *ii = NULL;

    DEBUG(TD_Printf(__METHOD__ "name = %s, path = %s\n", data->name, data->path));

    if( ii = (struct IconInit *)TD_AllocMem(sizeof(struct IconInit), MEMF_ANY) ) {
        ii->DiskObj = data->diskObj;
        ii->Brush = data->brushImg;
    }

    startTypes[0] = (STRPTR)MSG_ST_Workbench;
    startTypes[1] = (STRPTR)MSG_ST_Shell;

    msg->uiTags = TD_MakeTagList(   
        VertGroupA,
            Space,
            HorizGroupC,
                Space,
                TROB_Icon, ii, ID(OBJ_ICON),
                Space,
            EndGroup,
            Space,
            HorizGroupSC,
                Space,
                TextT(data->path ? data->path : (STRPTR)""), TRAT_ID, OBJ_TXT_PATH,
                Space,
            EndGroup,
            Space,
            LineArray,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Name),
                    Space,
                    HorizGroup,
                        StringGadget(data->name, OBJ_STR_NAME),                
                        GetFileButton(OBJ_BTN_SELECT_TARGET),
                    EndGroup,
                    Space,  
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Brush),
                    Space,
                    HorizGroup,
                        StringGadget(data->brushPath, OBJ_STR_BRUSH),TRAT_Value,200L,
                        GetFileButton(OBJ_BTN_SELECT_BRUSH),
                    EndGroup,
                    Space,  
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Args),
                    Space,
                    StringGadget(data->args, OBJ_STR_ARGS),TRAT_Value,200L,
                    Space,
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Key),
                    Space,
                    StringGadget(data->hotKey, OBJ_STR_HOTKEY),
                    Space,
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_StartType),
                    Space,
                    CycleGadget(startTypes, data->startType, OBJ_CYC_START),
                    Space,  
                EndLine,
                Space,
                BeginLine,
                    Space,
                    TextN(MSG_UI_Console),
                    Space,
                    StringGadget(data->con, OBJ_STR_CON),
                        TRAT_Disabled, (data->startType == ST_WB),
                    Space,
                EndLine,
            EndArray,
        EndGroup,
        TAG_END);

    return 1;
}

DB_METHOD_DM(EDITOREVENT, DockMessageEditorEvent)

    switch( msg->message->trm_Class ) {
        case TRMS_NEWVALUE:
            switch( msg->message->trm_ID ) {
                case OBJ_CYC_START:
                    TR_SetAttribute(msg->window, OBJ_STR_CON, TRAT_Disabled, msg->message->trm_Data == ST_WB);
                    break;

                case OBJ_STR_BRUSH:
                    set_brush(data, msg->window, (STRPTR)msg->message->trm_Data);
                    break;

                default:
                    break;
            }
            break;

        case TRMS_ACTION:
            switch( msg->message->trm_ID ) {
                case OBJ_BTN_SELECT_TARGET:
                    select_target(data, msg->window);
                    break;
            
                case OBJ_BTN_SELECT_BRUSH:
                    select_brush(data, msg->window);
                    break;

                default:
                    break;
            }
            break; 

        default:
            break;
        
    }

    return 1;
}

DB_METHOD_DM(EDITORUPDATE, DockMessageEditorUpdate)

    STRPTR str;
    UWORD len;
    struct TR_Project *proj = msg->window;

    FREE_STRING(data->name)
    FREE_STRING(data->args)
    FREE_STRING(data->con)
    FREE_STRING(data->hotKey)
    FREE_STRING(data->brushPath)

    if( data->selectedPath ) {
        FREE_STRING(data->path)
        data->path = data->selectedPath;
        data->selectedPath = NULL;
    }

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_NAME, 0);
    if( str && (len = strlen(str)) ) {        
        data->name = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->name, len + 1);
    }

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_BRUSH, 0);
    if( str && (len = strlen(str)) ) {        
        data->brushPath = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->brushPath, len + 1);
    }

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_HOTKEY, 0);
    if( str && (len = strlen(str)) ) {        
        data->hotKey = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->hotKey, len + 1);
    }

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_ARGS, 0);
    if( str && (len = strlen(str)) ) {        
        data->args = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->args, len + 1);
    }

    str = (STRPTR)TR_GetAttribute(proj, OBJ_STR_CON, 0);
    if( str && (len = strlen(str)) ) {        
        data->con = (STRPTR)TD_AllocMem(len + 1, MEMF_ANY);
        CopyMem(str, data->con, len + 1);
    }   

    data->startType = (UWORD)TR_GetAttribute(proj, OBJ_CYC_START, TRAT_Value);

    return 1;
}

DB_METHOD_DM(GETHOTKEY,DockMessageGetHotKey)

    msg->hotKey = data->hotKey;

    return 1;
}

DB_METHOD_DM(GETLABEL,DockMessageGetLabel)

    msg->label = data->name;

    return 1;
}

DB_METHOD_DM(INITBUTTON,DockMessageInitButton)

    UWORD len;

    zero_applife(data);

    len = strlen(msg->name) + 1;
    if( ! (data->name = TD_AllocMem(len, MEMF_ANY) ) ) {
        return 0;
    }
    CopyMem(msg->name, data->name, len);

    len = strlen(msg->path) + 1;
    if( ! (data->path = TD_AllocMem(len, MEMF_ANY) ) ) {
        return 0;
    }
    CopyMem(msg->path, data->path, len);

    DEBUG(TD_Printf(__METHOD__ "name = %s, path = %s\n", data->name, data->path));

    data->args = NULL;
    data->hotKey = NULL;
    data->con = NULL;

    load_icon(data, data->path);

    return 1;
}

DB_METHOD_M(LAUNCHED,DockMessageLaunch)

    FREE_STRING(msg->args);

    return 1;
}

