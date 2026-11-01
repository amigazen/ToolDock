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

#include "dock.h"

#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/libraries.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <clib/layers_protos.h>
#include <clib/graphics_protos.h>
#include <devices/timer.h>
#include "dock_handle.h"
#include "rearrange.h"
#include "td_request.h"
#include "applife.h"

#ifndef WBOPENA_ArgLock
#define WBOPENA_ArgLock  (TAG_USER + 0x1B01)
#endif
#ifndef WBOPENA_ArgName
#define WBOPENA_ArgName  (TAG_USER + 0x1B02)
#endif

#define MAX_WB_ARGS 32
#define WB_ARG_NAME_LEN 256

extern struct Library *WorkbenchBase;

BOOL OpenWorkbenchObjectA(STRPTR name, struct TagItem *tags);
#pragma libcall WorkbenchBase OpenWorkbenchObjectA 60 9802

struct WBLaunchArg {
    BPTR lock;
    UBYTE name[WB_ARG_NAME_LEN];
};

STATIC VOID __asm put_fmt_char(register __d0 UBYTE c, register __a3 STRPTR *p)
{
    *(*p)++ = c;
}

STATIC BOOL next_arg(STRPTR *pp, STRPTR out, UWORD outLen)
{
    STRPTR p;
    UWORD i;
    BOOL quote;

    p = *pp;
    i = 0;
    quote = FALSE;

    if( p == NULL ) {
        return FALSE;
    }

    while( *p == ' ' ) {
        p++;
    }

    if( *p == '\0' ) {
        *pp = p;
        return FALSE;
    }

    if( *p == '"' ) {
        quote = TRUE;
        p++;
    }

    while( *p ) {
        if( quote ) {
            if( *p == '"' ) {
                p++;
                break;
            }
        } else if( *p == ' ' ) {
            break;
        }

        if( i + 1 < outLen ) {
            out[i] = *p;
            i++;
        }
        p++;
    }

    out[i] = '\0';
    *pp = p;
    return (BOOL)(i > 0);
}

STATIC BOOL path_to_wbarg(STRPTR path, struct WBLaunchArg *arg)
{
    BPTR lock;
    BPTR parent;
    STRPTR file;

    arg->lock = NULL;
    arg->name[0] = '\0';

    if( path == NULL || *path == '\0' ) {
        return FALSE;
    }

    lock = Lock(path, ACCESS_READ);
    if( lock == NULL ) {
        return FALSE;
    }

    file = FilePart(path);
    parent = ParentDir(lock);

    if( parent && file && *file ) {
        UnLock(lock);
        arg->lock = parent;
        {
            UWORD ni;
            ni = 0;
            while( ni < (WB_ARG_NAME_LEN - 1) && file[ni] ) {
                arg->name[ni] = file[ni];
                ni++;
            }
            arg->name[ni] = '\0';
        }
        return TRUE;
    }

    if( parent ) {
        UnLock(parent);
    }

    arg->lock = lock;
    return TRUE;
}

STATIC VOID free_wbargs(struct WBLaunchArg *args, UWORD count)
{
    UWORD i;

    for( i = 0; i < count; i++ ) {
        if( args[i].lock ) {
            UnLock(args[i].lock);
            args[i].lock = NULL;
        }
    }
}

/* Official Workbench launch: OpenWorkbenchObjectA (workbench.library 44+). */
STATIC BOOL execute_workbench(STRPTR path, STRPTR args)
{
    struct WBLaunchArg wbArgs[MAX_WB_ARGS];
    struct TagItem *tags;
    STRPTR scan;
    UBYTE token[WB_ARG_NAME_LEN];
    UWORD count;
    UWORD i;
    BOOL ok;

    count = 0;
    tags = NULL;
    ok = FALSE;

    if( path == NULL || *path == '\0' ) {
        return FALSE;
    }

    if( WorkbenchBase == NULL || WorkbenchBase->lib_Version < 44 ) {
        return FALSE;
    }

    if( args ) {
        scan = args;
        while( count < MAX_WB_ARGS && next_arg(&scan, token, WB_ARG_NAME_LEN) ) {
            if( path_to_wbarg(token, &wbArgs[count]) ) {
                count++;
            }
        }
    }

    if( count > 0 ) {
        tags = (struct TagItem *)TD_AllocMem((count * 2 + 1) * sizeof(struct TagItem), MEMF_CLEAR);
        if( tags == NULL ) {
            free_wbargs(wbArgs, count);
            return FALSE;
        }
        for( i = 0; i < count; i++ ) {
            tags[i * 2].ti_Tag = WBOPENA_ArgLock;
            tags[i * 2].ti_Data = (ULONG)wbArgs[i].lock;
            tags[i * 2 + 1].ti_Tag = WBOPENA_ArgName;
            tags[i * 2 + 1].ti_Data = (ULONG)wbArgs[i].name;
        }
        tags[count * 2].ti_Tag = TAG_DONE;
    }

    ok = OpenWorkbenchObjectA(path, tags);

    if( tags ) {
        TD_FreeMem(tags, (count * 2 + 1) * sizeof(struct TagItem));
    }
    free_wbargs(wbArgs, count);

    return ok;
}

VOID show_about(struct DockWindow *dock)
{
    STRPTR msg;
    STRPTR body;
    STRPTR p;
    STRPTR name;
    STRPTR version;
    STRPTR desc;
    STRPTR copy;
    STRPTR cursor;
    ULONG msgLen;
    ULONG bodyLen;
    ULONG l;
    UBYTE fmtArgs[8];
    struct DgNode *curr;
    ClassID lastClass;
    ClassID currClass;

    lastClass = 0;
    msg = NULL;
    body = NULL;
    msgLen = 0;
    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

        if( ! dock_gadget_builtin(curr->dg) ) {

            currClass = OCLASS(curr->dg)->cl_ID;
            if( currClass == lastClass ) {
                continue;
            }
            lastClass = currClass;

            dock_gadget_get_info(curr->dg, &name, &version, &desc, &copy);

            msgLen += strlen(name) + strlen(version) + strlen(desc) + strlen(copy) + 8;
        }
    }

    if( msgLen > 0 ) {
        msg = (STRPTR)TD_AllocMem(msgLen, MEMF_CLEAR);
    }

    if( msg ) {
        p = msg;
        lastClass = 0;

        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            if( ! dock_gadget_builtin(curr->dg) ) {

                currClass = OCLASS(curr->dg)->cl_ID;
                if( currClass == lastClass ) {
                    continue;
                }
                lastClass = currClass;

                dock_gadget_get_info(curr->dg, &name, &version, &desc, &copy);

                l = strlen(name);
                CopyMem(name, p, l);
                p += l;
                *(p++) = ' ';
                l = strlen(version);
                CopyMem(version, p, l);
                p += l;
                *(p++) = '\n';
                l = strlen(desc);
                CopyMem(desc, p, l);
                p += l;
                *(p++) = '\n';
                l = strlen(copy);
                CopyMem(copy, p, l);
                p += l;
                *(p++) = '\n';
                *(p++) = '\n';
            }
        }
        *p = '\0';
    }

    bodyLen = strlen((STRPTR)MSG_ABOUT_Text) + strlen(VERSION_STR)
        + (msg ? msgLen : 0) + 1;
    if( body = (STRPTR)TD_AllocMem(bodyLen, MEMF_CLEAR) ) {
        *(STRPTR *)fmtArgs = VERSION_STR;
        *(STRPTR *)(fmtArgs + 4) = msg ? msg : (STRPTR)"";
        cursor = body;
        RawDoFmt((STRPTR)MSG_ABOUT_Text, fmtArgs, (VOID (*)())put_fmt_char, &cursor);
        TD_OpenRequest(dock->win, (STRPTR)MSG_ABOUT_Title, body,
            (STRPTR)MSG_OK, TDREQIMAGE_INFO);
        TD_FreeMem(body, bodyLen);
    }

    if( msg ) {
        TD_FreeMem(msg, msgLen);
    }
}

VOID delete_port(struct MsgPort *port) {
    struct Message *msg;

    if( port ) {
        while( msg = GetMsg(port) ) {
            ReplyMsg(msg);
        }

        if( port->mp_Node.ln_Name ) {
            DeletePort(port);
        } else {
            DeleteMsgPort(port);
        }
    }
}


BOOL get_prog_path(struct DockWindow *dock)
{
    BPTR lock;
    BOOL result = FALSE;
            
    if( lock = Lock("PROGDIR:", ACCESS_READ) ) {

        result = NameFromLock(lock, (STRPTR)&dock->progPath, 2048);
    
        UnLock(lock);

    }

    return result;
}

struct DockWindow* create_dock(VOID)
{
	struct DockWindow *dock;

    DEBUG(DB_Printf("create_dock\n"));

    if( dock = (struct DockWindow *)TD_AllocMem(sizeof(struct DockWindow), MEMF_CLEAR) ) {

        dock->runState = RS_STARTING;
        dock->disableLayout = TRUE;

        InitRastPort(&dock->renderRP);

        if( dock->pubPort = CreatePort(APP_NAME, 0L) ) {

            if( init_gadget_classes(dock) ) {

                if( init_config_notification(dock) ) {
                        
                    if( init_timer_notification(dock) ) {

                        if( get_prog_path(dock) ) {

                            rearrange_init(dock);
                            applife_init();
                            return dock;
                        }
                    }
                }
            }
        }    

        // Uh oh!    
        free_dock(dock);
    }

    return NULL;
}

VOID free_dock(struct DockWindow* dock)
{
    DEBUG(DB_Printf("free_dock\n"));

    free_cx_broker(dock);

    applife_expunge();

    rearrange_cleanup(dock);

    hide_dock_window(dock);

    remove_dock_gadgets(dock);

    free_config_notification(dock);

    free_gadget_classes(dock);

    free_timer_notification(dock);

    free_app_icon(dock);

    delete_port(dock->pubPort);

    free_icon_brushes(dock);

    free_config(dock);

    free_render_bitmap(dock);

	TD_FreeMem(dock, sizeof(struct DockWindow));

    DEBUG(DB_Printf("Done\n"));
}


VOID execute_external(struct DockWindow* dock, STRPTR path, STRPTR args, STRPTR console, BOOL wb)
{
    STRPTR cmd;
    STRPTR pos;
    STRPTR con;
    BPTR fhIn;
    BPTR fhOut;
    UWORD len = 0, l;

    struct TagItem shellTags[] = {
        { SYS_UserShell, TRUE },
        { SYS_Asynch, TRUE },
        { SYS_Input, NULL },
        { SYS_Output, NULL },
        { TAG_DONE, 0 }
    };

    if( wb && execute_workbench(path, args) ) {
        return;
    }
    
    len += strlen(path) + 1;
    if( args ) {
        len += strlen(args) + 1;
    }

    if( cmd = (STRPTR)TD_AllocMem(len + 1, MEMF_CLEAR) ) {
        
        pos = cmd;
        COPY_STRING(path, pos);
                
        if( args ) {
            COPY_STRING(args, pos);
        }

        pos--;
        *pos = '\0';

        con = console;
        if( con == NULL ) {
            con = DEFAULT_CONSOLE;
        }

        if( fhOut = Open(con, MODE_OLDFILE) ) {
            if( fhIn = Open(DEFAULT_CONSOLE, MODE_OLDFILE) ) {

                shellTags[2].ti_Data = fhIn;
                shellTags[3].ti_Data = fhOut;
                
                DEBUG(DB_Printf(__FUNC__ ": cmd = >%s<\n", cmd));             

                if( SystemTagList(cmd, (struct TagItem*)&shellTags) == -1 ) {
                    Close(fhIn);
                    Close(fhOut);
                }

            } else {
                Close(fhOut);
            }
        }
        TD_FreeMem(cmd, len + 1);
    }
}

VOID open_help(struct DockWindow *dock)
{
    UWORD l;
    UBYTE arg[256];
    STRPTR pos = (STRPTR)&arg;

    COPY_STRING((STRPTR)dock->progPath, pos);
    pos--;
    COPY_STRING("/doc/ToolDock.guide", pos);
    pos--;
    *pos = '\0';

    execute_external(dock, "SYS:Utilities/Multiview", (STRPTR)&arg, NULL, TRUE);
}
