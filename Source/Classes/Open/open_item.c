/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  WBPROJECT -> OpenWorkbenchObjectA via icon/DefIcons/datatypes/default
**  anything else -> WBInfo()
**
************************************/

#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <workbench/workbench.h>
#include <datatypes/datatypes.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/wb_protos.h>
#include <clib/datatypes_protos.h>
#include <clib/intuition_protos.h>

#include "open_item.h"

#ifndef ZERO
#define ZERO ((BPTR)0L)
#endif

#ifndef WBOPENA_ArgLock
#define WBOPENA_ArgLock  (TAG_USER + 0x1B01)
#endif
#ifndef WBOPENA_ArgName
#define WBOPENA_ArgName  (TAG_USER + 0x1B02)
#endif
#ifndef ICONA_Dummy
#define ICONA_Dummy (TAG_USER + 0x9000)
#endif
#ifndef ICONA_ErrorCode
#define ICONA_ErrorCode (ICONA_Dummy + 1)
#endif
#ifndef ICONGETA_IdentifyBuffer
#define ICONGETA_IdentifyBuffer (ICONA_Dummy + 122)
#endif
#ifndef ICONGETA_IdentifyOnly
#define ICONGETA_IdentifyOnly (ICONA_Dummy + 123)
#endif
#ifndef ICONGETA_FailIfUnavailable
#define ICONGETA_FailIfUnavailable (ICONA_Dummy + 47)
#endif

#define MAX_PATH_LEN 512

extern struct Library *IconBase;
extern struct Library *WorkbenchBase;
extern struct Library *DataTypesBase;

BOOL OpenWorkbenchObjectA(STRPTR name, struct TagItem *tags);
#pragma libcall WorkbenchBase OpenWorkbenchObjectA 60 9802

ULONG WBInfo(BPTR lock, STRPTR name, struct Screen *screen);
#pragma libcall WorkbenchBase WBInfo 5a A9803

#pragma libcall IconBase GetIconTagList b4 9802

STATIC ULONG str_len(STRPTR s)
{
    ULONG n;

    n = 0;
    if( s == NULL ) {
        return 0;
    }
    while( s[n] ) {
        n++;
    }
    return n;
}

STATIC VOID str_copy(STRPTR dst, STRPTR src, ULONG dstlen)
{
    ULONG i;

    if( dst == NULL || dstlen == 0 ) {
        return;
    }
    i = 0;
    if( src ) {
        while( src[i] && i < dstlen - 1 ) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

STATIC VOID str_cat(STRPTR dst, STRPTR src, ULONG dstlen)
{
    ULONG i;
    ULONG n;

    if( dst == NULL || src == NULL || dstlen == 0 ) {
        return;
    }
    i = str_len(dst);
    n = 0;
    while( src[n] && i < dstlen - 1 ) {
        dst[i] = src[n];
        i++;
        n++;
    }
    dst[i] = '\0';
}

STATIC BOOL chars_eq_i(UBYTE a, UBYTE b)
{
    if( a >= 'A' && a <= 'Z' ) {
        a = (UBYTE)(a - 'A' + 'a');
    }
    if( b >= 'A' && b <= 'Z' ) {
        b = (UBYTE)(b - 'A' + 'a');
    }
    return (a == b) ? TRUE : FALSE;
}

STATIC BOOL ends_with_i(STRPTR path, STRPTR ext)
{
    ULONG n;
    ULONG e;
    ULONG i;

    n = str_len(path);
    e = str_len(ext);
    if( e == 0 || n < e ) {
        return FALSE;
    }
    for( i = 0; i < e; i++ ) {
        if( ! chars_eq_i(path[n - e + i], ext[i]) ) {
            return FALSE;
        }
    }
    return TRUE;
}

STATIC BOOL is_info_file(STRPTR path)
{
    return ends_with_i(path, ".info");
}

STATIC UBYTE icon_type(STRPTR path)
{
    struct DiskObject *icon;
    struct TagItem tags[3];
    LONG error;
    UBYTE type;

    type = 0;
    icon = NULL;
    error = 0;
    if( IconBase == NULL || path == NULL ) {
        return 0;
    }

    if( IconBase->lib_Version >= 44 ) {
        tags[0].ti_Tag = ICONGETA_FailIfUnavailable;
        tags[0].ti_Data = FALSE;
        tags[1].ti_Tag = ICONA_ErrorCode;
        tags[1].ti_Data = (ULONG)&error;
        tags[2].ti_Tag = TAG_DONE;
        icon = GetIconTagList(path, tags);
    }
    if( icon == NULL ) {
        icon = GetDiskObjectNew(path);
    }
    if( icon ) {
        type = icon->do_Type;
        FreeDiskObject(icon);
    }
    return type;
}

STATIC BOOL show_wbinfo(STRPTR path, BPTR lock)
{
    BPTR parent;
    struct Screen *screen;
    STRPTR leaf;
    BOOL ok;

    ok = FALSE;
    if( WorkbenchBase == NULL || WorkbenchBase->lib_Version < 39
        || path == NULL || lock == ZERO ) {
        return FALSE;
    }

    leaf = FilePart(path);
    parent = ParentDir(lock);
    screen = LockPubScreen(NULL);

    if( parent ) {
        if( WBInfo(parent, leaf, screen) ) {
            ok = TRUE;
        }
        UnLock(parent);
    } else if( WBInfo(ZERO, leaf[0] ? leaf : path, screen) ) {
        ok = TRUE;
    }

    if( screen ) {
        UnlockPubScreen(NULL, screen);
    }
    return ok;
}

STATIC BOOL open_wb(STRPTR name, struct TagItem *tags)
{
    struct TagItem st[3];

    if( name == NULL || name[0] == '\0' ) {
        return FALSE;
    }
    if( WorkbenchBase && WorkbenchBase->lib_Version >= 44 ) {
        SetIoErr(0);
        if( OpenWorkbenchObjectA(name, tags) ) {
            return TRUE;
        }
        return FALSE;
    }
    if( tags && tags[0].ti_Tag != TAG_DONE ) {
        return FALSE;
    }
    st[0].ti_Tag = SYS_Asynch;
    st[0].ti_Data = TRUE;
    st[1].ti_Tag = SYS_UserShell;
    st[1].ti_Data = TRUE;
    st[2].ti_Tag = TAG_DONE;
    return (SystemTagList(name, st) != -1) ? TRUE : FALSE;
}

STATIC BOOL launch_tool_on_file(STRPTR tool, STRPTR path)
{
    BPTR lock;
    BPTR parent;
    struct TagItem tags[3];
    UBYTE leaf[256];
    STRPTR fp;
    BOOL ok;

    ok = FALSE;
    if( tool == NULL || tool[0] == '\0' || path == NULL ) {
        return FALSE;
    }

    lock = Lock(path, ACCESS_READ);
    if( lock == ZERO ) {
        tags[0].ti_Tag = TAG_DONE;
        return open_wb(tool, tags);
    }

    fp = FilePart(path);
    str_copy(leaf, fp, sizeof(leaf));
    parent = ParentDir(lock);
    if( parent ) {
        tags[0].ti_Tag = WBOPENA_ArgLock;
        tags[0].ti_Data = (ULONG)parent;
        tags[1].ti_Tag = WBOPENA_ArgName;
        tags[1].ti_Data = (ULONG)leaf;
        tags[2].ti_Tag = TAG_DONE;
        ok = open_wb(tool, tags);
        UnLock(parent);
    }
    UnLock(lock);
    return ok;
}

STATIC BOOL copy_default_tool(struct DiskObject *icon, STRPTR out, ULONG outlen)
{
    if( icon == NULL || icon->do_DefaultTool == NULL
        || icon->do_DefaultTool[0] == '\0' ) {
        return FALSE;
    }
    str_copy(out, icon->do_DefaultTool, outlen);
    return (out[0] != '\0') ? TRUE : FALSE;
}

STATIC BOOL tool_from_icon(STRPTR path, STRPTR out, ULONG outlen)
{
    struct DiskObject *icon;
    BOOL ok;

    ok = FALSE;
    out[0] = '\0';
    if( IconBase == NULL ) {
        return FALSE;
    }
    icon = GetDiskObjectNew(path);
    if( icon ) {
        ok = copy_default_tool(icon, out, outlen);
        FreeDiskObject(icon);
    }
    return ok;
}

STATIC BOOL tool_from_deficons(STRPTR path, BPTR lock, STRPTR out, ULONG outlen)
{
    struct TagItem tags[4];
    struct DiskObject *icon;
    LONG error;
    UBYTE typeBuf[256];
    UBYTE defName[64];
    BPTR env;
    BPTR old;
    BOOL ok;

    ok = FALSE;
    out[0] = '\0';
    if( IconBase == NULL || IconBase->lib_Version < 44
        || FindPort("DEFICONS") == NULL ) {
        return FALSE;
    }

    typeBuf[0] = '\0';
    error = 0;
    old = CurrentDir(lock);
    tags[0].ti_Tag = ICONGETA_IdentifyBuffer;
    tags[0].ti_Data = (ULONG)typeBuf;
    tags[1].ti_Tag = ICONGETA_IdentifyOnly;
    tags[1].ti_Data = TRUE;
    tags[2].ti_Tag = ICONA_ErrorCode;
    tags[2].ti_Data = (ULONG)&error;
    tags[3].ti_Tag = TAG_DONE;
    icon = GetIconTagList(FilePart(path), tags);
    if( icon ) {
        FreeDiskObject(icon);
    }
    CurrentDir(old);
    if( error != 0 || typeBuf[0] == '\0' ) {
        return FALSE;
    }

    str_copy(defName, "def_", sizeof(defName));
    str_cat(defName, typeBuf, sizeof(defName));

    env = Lock("ENV:Sys", SHARED_LOCK);
    if( env == ZERO ) {
        env = Lock("ENVARC:Sys", SHARED_LOCK);
    }
    if( env ) {
        old = CurrentDir(env);
        icon = GetDiskObject(defName);
        CurrentDir(old);
        UnLock(env);
        if( icon ) {
            ok = copy_default_tool(icon, out, outlen);
            FreeDiskObject(icon);
        }
    }
    return ok;
}

STATIC BOOL tool_from_datatypes(BPTR lock, STRPTR out, ULONG outlen)
{
    struct DataType *dtn;
    struct ToolNode *tn;
    struct Node *node;
    BOOL ok;

    ok = FALSE;
    out[0] = '\0';
    if( DataTypesBase == NULL || lock == ZERO ) {
        return FALSE;
    }

    dtn = ObtainDataTypeA(DTST_FILE, (APTR)lock, NULL);
    if( dtn == NULL ) {
        return FALSE;
    }

    for( node = dtn->dtn_ToolList.lh_Head; node->ln_Succ; node = node->ln_Succ ) {
        tn = (struct ToolNode *)node;
        if( tn->tn_Tool.tn_Program && tn->tn_Tool.tn_Program[0] ) {
            str_copy(out, tn->tn_Tool.tn_Program, outlen);
            ok = TRUE;
            break;
        }
    }

    ReleaseDataType(dtn);
    return ok;
}

STATIC BOOL tool_from_env(STRPTR name, STRPTR out, ULONG outlen)
{
    if( GetVar(name, out, outlen, 0) > 0 && out[0] ) {
        return TRUE;
    }
    return FALSE;
}

STATIC BOOL open_data_file(STRPTR path, BPTR lock, STRPTR defaultTool)
{
    UBYTE tool[256];

    tool[0] = '\0';

    if( tool_from_icon(path, tool, sizeof(tool)) ) {
        return launch_tool_on_file(tool, path);
    }
    if( tool_from_deficons(path, lock, tool, sizeof(tool)) ) {
        return launch_tool_on_file(tool, path);
    }
    if( tool_from_datatypes(lock, tool, sizeof(tool)) ) {
        return launch_tool_on_file(tool, path);
    }
    if( defaultTool && defaultTool[0] ) {
        return launch_tool_on_file(defaultTool, path);
    }
    if( tool_from_env("Editor", tool, sizeof(tool)) ) {
        return launch_tool_on_file(tool, path);
    }
    if( tool_from_env("Viewer", tool, sizeof(tool)) ) {
        return launch_tool_on_file(tool, path);
    }
    return launch_tool_on_file("SYS:Utilities/MultiView", path);
}

BOOL open_dropped_path(STRPTR path, STRPTR defaultTool)
{
    BPTR lock;
    UBYTE stripped[MAX_PATH_LEN];
    BOOL ok;

    ok = FALSE;
    if( path == NULL || path[0] == '\0' ) {
        return FALSE;
    }

    if( is_info_file(path) ) {
        str_copy(stripped, path, sizeof(stripped));
        stripped[str_len(stripped) - 5] = '\0';
        path = stripped;
    }

    lock = Lock(path, ACCESS_READ);
    if( lock == ZERO ) {
        return FALSE;
    }

    if( icon_type(path) == WBPROJECT ) {
        ok = open_data_file(path, lock, defaultTool);
    } else {
        ok = show_wbinfo(path, lock);
    }

    UnLock(lock);
    return ok;
}
