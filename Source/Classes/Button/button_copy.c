/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
**  Drop onto a disk or drawer uses workbench.library's copy hook
**  (WBCTRLA_GetCopyHook). If no hook is installed, copy locally
**  and tell Workbench about the new objects.
**
************************************/

#include <exec/memory.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include <clib/intuition_protos.h>
#include <clib/wb_protos.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/utility_pragmas.h>
#include <pragmas/wb_pragmas.h>

#include "button_copy.h"

extern struct Library *DOSBase;

#ifndef UPDATEWB_ObjectAdded
#define UPDATEWB_ObjectAdded 1
#endif

#ifndef COPY_BUF
#define COPY_BUF 4096
#endif

#define MAX_LEAF 107

extern struct Library *UtilityBase;
extern struct IntuitionBase *IntuitionBase;

struct Library *WorkbenchBase;

VOID UpdateWorkbench(STRPTR name, BPTR lock, LONG added);
#pragma libcall WorkbenchBase UpdateWorkbench 1e 09803

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
        while( src[i] && i + 1 < dstlen ) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = 0;
}

STATIC VOID make_info_name(STRPTR dst, STRPTR src)
{
    ULONG n;

    str_copy(dst, src, MAX_LEAF - 4);
    n = str_len(dst);
    dst[n] = '.';
    dst[n + 1] = 'i';
    dst[n + 2] = 'n';
    dst[n + 3] = 'f';
    dst[n + 4] = 'o';
    dst[n + 5] = 0;
}

BOOL button_path_is_drawer(STRPTR path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL isDir;

    isDir = FALSE;
    if( path == NULL || path[0] == 0 ) {
        return FALSE;
    }
    lock = Lock(path, ACCESS_READ);
    if( lock == NULL ) {
        return FALSE;
    }
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if( fib ) {
        if( Examine(lock, fib) ) {
            if( fib->fib_DirEntryType >= 0 ) {
                isDir = TRUE;
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
    return isDir;
}

STATIC BOOL split_path(STRPTR path, BPTR *parent, STRPTR nameOut)
{
    BPTR lock;
    BPTR par;
    STRPTR leaf;
    ULONG n;

    *parent = NULL;
    nameOut[0] = 0;
    if( path == NULL || path[0] == 0 ) {
        return FALSE;
    }
    lock = Lock(path, ACCESS_READ);
    if( lock == NULL ) {
        return FALSE;
    }
    par = ParentDir(lock);
    UnLock(lock);
    leaf = FilePart(path);
    if( leaf == NULL || leaf[0] == 0 ) {
        n = 0;
        while( path[n] && path[n] != ':' && n < MAX_LEAF ) {
            nameOut[n] = path[n];
            n++;
        }
        nameOut[n] = 0;
        *parent = par;
        return TRUE;
    }
    n = 0;
    while( leaf[n] && n < MAX_LEAF ) {
        nameOut[n] = leaf[n];
        n++;
    }
    nameOut[n] = 0;
    *parent = par;
    return TRUE;
}

STATIC LONG copy_file_data(BPTR srcDir, STRPTR srcName, BPTR dstDir, STRPTR dstName)
{
    BPTR old;
    BPTR in;
    BPTR out;
    UBYTE *buf;
    LONG n;
    LONG err;

    err = 0;
    in = NULL;
    out = NULL;
    buf = (UBYTE *)AllocMem(COPY_BUF, MEMF_ANY);
    if( buf == NULL ) {
        return ERROR_NO_FREE_STORE;
    }
    old = CurrentDir(srcDir);
    in = Open(srcName, MODE_OLDFILE);
    CurrentDir(dstDir);
    if( in ) {
        out = Open(dstName, MODE_NEWFILE);
    }
    CurrentDir(old);
    if( in == NULL ) {
        err = IoErr();
        if( err == 0 ) {
            err = ERROR_OBJECT_NOT_FOUND;
        }
    } else if( out == NULL ) {
        err = IoErr();
        if( err == 0 ) {
            err = ERROR_DISK_WRITE_PROTECTED;
        }
    } else {
        for( ;; ) {
            n = Read(in, buf, COPY_BUF);
            if( n < 0 ) {
                err = IoErr();
                break;
            }
            if( n == 0 ) {
                break;
            }
            if( Write(out, buf, n) != n ) {
                err = IoErr();
                if( err == 0 ) {
                    err = ERROR_DISK_FULL;
                }
                break;
            }
        }
    }
    if( in ) {
        Close(in);
    }
    if( out ) {
        Close(out);
        if( err ) {
            old = CurrentDir(dstDir);
            DeleteFile(dstName);
            CurrentDir(old);
        }
    }
    FreeMem(buf, COPY_BUF);
    return err;
}

STATIC LONG copy_info_if_present(BPTR srcDir, STRPTR srcName, BPTR dstDir, STRPTR dstName)
{
    UBYTE srcInfo[MAX_LEAF + 8];
    UBYTE dstInfo[MAX_LEAF + 8];
    BPTR old;
    BPTR lock;

    make_info_name(srcInfo, srcName);
    make_info_name(dstInfo, dstName);
    old = CurrentDir(srcDir);
    lock = Lock(srcInfo, ACCESS_READ);
    CurrentDir(old);
    if( lock == NULL ) {
        return 0;
    }
    UnLock(lock);
    return copy_file_data(srcDir, srcInfo, dstDir, dstInfo);
}

STATIC LONG copy_tree(BPTR srcDir, STRPTR srcName, BPTR dstDir, STRPTR dstName)
{
    BPTR old;
    BPTR srcLock;
    BPTR newDir;
    struct FileInfoBlock *fib;
    LONG err;

    err = 0;
    if( srcName == NULL || srcName[0] == 0 || dstDir == NULL || dstName == NULL ) {
        return ERROR_OBJECT_WRONG_TYPE;
    }
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if( fib == NULL ) {
        return ERROR_NO_FREE_STORE;
    }

    old = CurrentDir(srcDir);
    srcLock = Lock(srcName, ACCESS_READ);
    CurrentDir(old);
    if( srcLock == NULL ) {
        FreeDosObject(DOS_FIB, fib);
        err = IoErr();
        if( err == 0 ) {
            err = ERROR_OBJECT_NOT_FOUND;
        }
        return err;
    }

    if( ! Examine(srcLock, fib) ) {
        err = IoErr();
        UnLock(srcLock);
        FreeDosObject(DOS_FIB, fib);
        return err;
    }

    if( fib->fib_DirEntryType < 0 ) {
        UnLock(srcLock);
        FreeDosObject(DOS_FIB, fib);
        err = copy_file_data(srcDir, srcName, dstDir, dstName);
        if( err == 0 ) {
            copy_info_if_present(srcDir, srcName, dstDir, dstName);
            if( WorkbenchBase ) {
                UpdateWorkbench(dstName, dstDir, UPDATEWB_ObjectAdded);
            }
        }
        return err;
    }

    old = CurrentDir(dstDir);
    newDir = CreateDir(dstName);
    CurrentDir(old);
    if( newDir == NULL ) {
        err = IoErr();
        UnLock(srcLock);
        FreeDosObject(DOS_FIB, fib);
        return err;
    }

    if( Examine(srcLock, fib) ) {
        while( err == 0 && ExNext(srcLock, fib) ) {
            if( fib->fib_FileName[0] ) {
                err = copy_tree(srcLock, fib->fib_FileName, newDir, fib->fib_FileName);
            }
        }
        if( err == 0 ) {
            err = IoErr();
            if( err == ERROR_NO_MORE_ENTRIES ) {
                err = 0;
            }
        }
    } else {
        err = IoErr();
    }

    copy_info_if_present(srcDir, srcName, dstDir, dstName);
    if( WorkbenchBase ) {
        UpdateWorkbench(dstName, dstDir, UPDATEWB_ObjectAdded);
    }
    UnLock(newDir);
    UnLock(srcLock);
    FreeDosObject(DOS_FIB, fib);
    return err;
}

STATIC struct Hook *get_wb_copy_hook(VOID)
{
    struct Hook *hook;
    struct TagItem tags[2];

    hook = NULL;
    if( WorkbenchBase == NULL || WorkbenchBase->lib_Version < 45 ) {
        return NULL;
    }
    tags[0].ti_Tag = WBCTRLA_GetCopyHook;
    tags[0].ti_Data = (ULONG)&hook;
    tags[1].ti_Tag = TAG_DONE;
    tags[1].ti_Data = 0;
    if( ! WorkbenchControlA(NULL, tags) ) {
        return NULL;
    }
    return hook;
}

STATIC LONG call_copy_hook(struct Hook *hook, APTR msg)
{
    if( hook == NULL || UtilityBase == NULL ) {
        return 0;
    }
    return (LONG)CallHookPkt(hook, NULL, msg);
}

BOOL button_copy_paths(STRPTR destPath, STRPTR *paths, UWORD count)
{
    struct Hook *hook;
    struct CopyBeginMsg cbm;
    struct CopyDataMsg cdm;
    struct CopyEndMsg cem;
    BPTR destLock;
    BPTR parents[16];
    UBYTE names[16][MAX_LEAF + 1];
    UWORD n;
    UWORD i;
    LONG err;
    BOOL opened;
    BOOL ok;

    ok = FALSE;
    opened = FALSE;
    destLock = NULL;
    hook = NULL;
    n = 0;
    err = 0;
    if( destPath == NULL || paths == NULL || count == 0 ) {
        DisplayBeep(NULL);
        return FALSE;
    }

    WorkbenchBase = OpenLibrary("workbench.library", 39);
    if( WorkbenchBase ) {
        opened = TRUE;
    }

    destLock = Lock(destPath, ACCESS_READ);
    if( destLock == NULL ) {
        DisplayBeep(NULL);
        if( opened ) {
            CloseLibrary(WorkbenchBase);
            WorkbenchBase = NULL;
        }
        return FALSE;
    }

    if( count > 16 ) {
        count = 16;
    }
    for( i = 0; i < count; i++ ) {
        parents[i] = NULL;
        names[i][0] = 0;
        if( paths[i] && split_path(paths[i], &parents[i], names[i]) ) {
            n++;
        }
    }
    if( n == 0 ) {
        UnLock(destLock);
        DisplayBeep(NULL);
        if( opened ) {
            CloseLibrary(WorkbenchBase);
            WorkbenchBase = NULL;
        }
        return FALSE;
    }

    hook = get_wb_copy_hook();
    if( hook ) {
        /* wb.doc: one Begin, one Copy per selected object, one End. */
        cbm.cbm_Length = sizeof(struct CopyBeginMsg);
        cbm.cbm_Action = CPACTION_Begin;
        cbm.cbm_SourceDrawer = parents[0];
        cbm.cbm_DestinationDrawer = destLock;
        err = call_copy_hook(hook, &cbm);
        if( err == 0 ) {
            for( i = 0; i < count && err == 0; i++ ) {
                if( names[i][0] == 0 ) {
                    continue;
                }
                cdm.cdm_Length = sizeof(struct CopyDataMsg);
                cdm.cdm_Action = CPACTION_Copy;
                cdm.cdm_SourceLock = parents[i];
                cdm.cdm_SourceName = names[i];
                cdm.cdm_DestinationLock = destLock;
                cdm.cdm_DestinationName = names[i];
                cdm.cdm_DestinationX = NO_ICON_POSITION;
                cdm.cdm_DestinationY = NO_ICON_POSITION;
                err = call_copy_hook(hook, &cdm);
            }
        }
        cem.cem_Length = sizeof(struct CopyEndMsg);
        cem.cem_Action = CPACTION_End;
        call_copy_hook(hook, &cem);
        ok = (BOOL)(err == 0);
    } else {
        err = 0;
        for( i = 0; i < count && err == 0; i++ ) {
            if( names[i][0] == 0 || parents[i] == NULL ) {
                if( names[i][0] && parents[i] == NULL ) {
                    err = ERROR_OBJECT_WRONG_TYPE;
                }
                continue;
            }
            err = copy_tree(parents[i], names[i], destLock, names[i]);
        }
        ok = (BOOL)(err == 0);
    }

    for( i = 0; i < count; i++ ) {
        if( parents[i] ) {
            UnLock(parents[i]);
        }
    }
    UnLock(destLock);
    if( opened ) {
        CloseLibrary(WorkbenchBase);
        WorkbenchBase = NULL;
    }
    if( ! ok ) {
        DisplayBeep(NULL);
    }
    return ok;
}
