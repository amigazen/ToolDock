/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  Move logic matches WBTrash Source/trash/trash_ops.c:
**    1) Require Volume:Trashcan as a drawer (create if missing)
**    2) Prefer the original leaf when that name is free
**    3) On conflict, DateStamp unique leaf (OFS 30-char safe)
**    4) Rename the object and its paired .info together
**    5) Store WBT=<original> filenote, or WBTRASH_ORIG ToolType
**    6) Stamp trash time; UpdateWorkbench source and Trashcan
**
************************************/

#include <exec/memory.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <workbench/workbench.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/wb_protos.h>

#include "trash_move.h"

#ifndef ZERO
#define ZERO ((BPTR)0L)
#endif

#ifndef UPDATEWB_ObjectRemoved
#define UPDATEWB_ObjectRemoved 0
#endif
#ifndef UPDATEWB_ObjectAdded
#define UPDATEWB_ObjectAdded 1
#endif

#define MAX_PATH_LEN        512
#define DOS_SAFE_PATH_LEN   255
#define MAX_COMMENT_LENGTH  79
#define TRASH_SAFE_LEAF     30
#define TRASH_MAX_LEAF      107
#define TRASH_COMMENT_PREFIX "WBT="
#define TRASH_TOOLTYPE_ORIG  "WBTRASH_ORIG"

extern struct Library *IconBase;
extern struct Library *WorkbenchBase;

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

STATIC BOOL text_eq_i(STRPTR a, STRPTR b)
{
    ULONG i;

    if( a == NULL || b == NULL ) {
        return FALSE;
    }
    for( i = 0; a[i] && b[i]; i++ ) {
        if( ! chars_eq_i(a[i], b[i]) ) {
            return FALSE;
        }
    }
    return (a[i] == '\0' && b[i] == '\0') ? TRUE : FALSE;
}

STATIC BOOL text_prefix_i(STRPTR text, STRPTR prefix)
{
    ULONG i;

    if( text == NULL || prefix == NULL ) {
        return FALSE;
    }
    for( i = 0; prefix[i]; i++ ) {
        if( text[i] == '\0' || ! chars_eq_i(text[i], prefix[i]) ) {
            return FALSE;
        }
    }
    return TRUE;
}

STATIC BOOL is_info_file(STRPTR path)
{
    ULONG n;

    n = str_len(path);
    if( n < 5 ) {
        return FALSE;
    }
    return path[n - 5] == '.'
        && chars_eq_i(path[n - 4], 'i')
        && chars_eq_i(path[n - 3], 'n')
        && chars_eq_i(path[n - 2], 'f')
        && chars_eq_i(path[n - 1], 'o');
}

STATIC BOOL get_corresponding_file(STRPTR info, STRPTR out, ULONG outlen)
{
    ULONG n;

    if( ! is_info_file(info) ) {
        return FALSE;
    }
    n = str_len(info);
    if( n - 5 >= outlen ) {
        return FALSE;
    }
    str_copy(out, info, outlen);
    out[n - 5] = '\0';
    return TRUE;
}

STATIC BOOL get_corresponding_info(STRPTR file, STRPTR out, ULONG outlen)
{
    if( file == NULL || is_info_file(file) ) {
        return FALSE;
    }
    if( str_len(file) + 5 >= outlen ) {
        return FALSE;
    }
    str_copy(out, file, outlen);
    str_cat(out, ".info", outlen);
    return TRUE;
}

STATIC BOOL path_exists(STRPTR path)
{
    BPTR lock;

    if( path == NULL || path[0] == '\0' ) {
        return FALSE;
    }
    lock = Lock(path, SHARED_LOCK);
    if( lock == ZERO ) {
        return FALSE;
    }
    UnLock(lock);
    return TRUE;
}

STATIC BOOL path_is_drawer(STRPTR path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL ok;

    ok = FALSE;
    if( path == NULL || path[0] == '\0' ) {
        return FALSE;
    }
    lock = Lock(path, SHARED_LOCK);
    if( lock == ZERO ) {
        return FALSE;
    }
    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( fib ) {
        if( Examine(lock, fib) && fib->fib_DirEntryType > 0 ) {
            ok = TRUE;
        }
        FreeMem(fib, sizeof(struct FileInfoBlock));
    }
    UnLock(lock);
    return ok;
}

STATIC BOOL trash_path_is_protected(STRPTR path)
{
    STRPTR colon;
    STRPTR after;
    STRPTR slash;
    ULONG i;

    if( path == NULL || path[0] == '\0' ) {
        return TRUE;
    }
    if( str_len(path) > DOS_SAFE_PATH_LEN ) {
        return TRUE;
    }
    if( text_prefix_i(path, "TRASH:") ) {
        return TRUE;
    }
    colon = NULL;
    for( i = 0; path[i]; i++ ) {
        if( path[i] == ':' ) {
            colon = path + i;
            break;
        }
    }
    if( colon == NULL ) {
        return TRUE;
    }
    after = colon + 1;
    if( *after == '\0' ) {
        return TRUE;
    }
    if( text_prefix_i(after, "Trashcan")
        && (after[8] == '\0' || after[8] == '/') ) {
        return TRUE;
    }
    slash = NULL;
    for( i = 0; after[i]; i++ ) {
        if( after[i] == '/' ) {
            slash = after + i;
            break;
        }
    }
    if( slash == NULL && text_eq_i(after, "Disk.info") ) {
        return TRUE;
    }
    return FALSE;
}

STATIC VOID trash_volume_of_path(STRPTR path, STRPTR vol, ULONG vollen)
{
    ULONG i;

    if( vol == NULL || vollen == 0 ) {
        return;
    }
    vol[0] = '\0';
    if( path == NULL ) {
        return;
    }
    for( i = 0; path[i] && path[i] != ':' && i < vollen - 1; i++ ) {
        vol[i] = path[i];
    }
    if( path[i] != ':' || i == 0 || i > 30 ) {
        vol[0] = '\0';
        return;
    }
    vol[i] = '\0';
}

STATIC VOID trash_file_part(STRPTR path, STRPTR out, ULONG outlen)
{
    STRPTR start;
    ULONG i;

    if( out == NULL || outlen == 0 ) {
        return;
    }
    out[0] = '\0';
    if( path == NULL ) {
        return;
    }
    start = path;
    for( i = 0; path[i]; i++ ) {
        if( path[i] == '/' || path[i] == ':' ) {
            start = path + i + 1;
        }
    }
    str_copy(out, start, outlen);
}

STATIC BOOL safe_join(STRPTR out, ULONG outlen, STRPTR dir, STRPTR leaf)
{
    if( out == NULL || outlen == 0 || dir == NULL || leaf == NULL ) {
        return FALSE;
    }
    str_copy(out, dir, outlen);
    if( ! AddPart(out, leaf, outlen) ) {
        out[0] = '\0';
        return FALSE;
    }
    if( str_len(out) > DOS_SAFE_PATH_LEN ) {
        out[0] = '\0';
        return FALSE;
    }
    return TRUE;
}

STATIC BOOL make_trashcan_path(STRPTR out, ULONG outlen, STRPTR volume)
{
    ULONG n;

    if( out == NULL || volume == NULL || volume[0] == '\0' ) {
        return FALSE;
    }
    n = str_len(volume);
    if( n + 10 >= outlen ) {
        return FALSE;
    }
    str_copy(out, volume, outlen);
    out[n] = ':';
    out[n + 1] = '\0';
    str_cat(out, "Trashcan", outlen);
    return TRUE;
}

STATIC BOOL trash_ensure_trashcan(STRPTR volume)
{
    UBYTE tpath[MAX_PATH_LEN];
    BPTR lock;

    if( ! make_trashcan_path(tpath, sizeof(tpath), volume) ) {
        return FALSE;
    }
    if( path_is_drawer(tpath) ) {
        return TRUE;
    }
    lock = CreateDir(tpath);
    if( lock == ZERO ) {
        return FALSE;
    }
    UnLock(lock);
    return path_is_drawer(tpath);
}

STATIC BOOL destination_family_free(STRPTR candidate)
{
    UBYTE info[MAX_PATH_LEN];

    if( candidate == NULL || candidate[0] == '\0' ) {
        return FALSE;
    }
    if( path_exists(candidate) ) {
        return FALSE;
    }
    if( str_len(candidate) + 5 >= sizeof(info) ) {
        return FALSE;
    }
    str_copy(info, candidate, sizeof(info));
    str_cat(info, ".info", sizeof(info));
    if( path_exists(info) ) {
        return FALSE;
    }
    return TRUE;
}

STATIC VOID split_leaf(STRPTR leaf, STRPTR base, ULONG baselen,
    STRPTR ext, ULONG extlen)
{
    STRPTR dot;
    ULONG i;
    ULONG blen;

    base[0] = '\0';
    ext[0] = '\0';
    if( leaf == NULL || baselen == 0 || extlen == 0 ) {
        return;
    }
    dot = NULL;
    for( i = 0; leaf[i]; i++ ) {
        if( leaf[i] == '.' ) {
            dot = leaf + i;
        }
    }
    if( dot == NULL || dot == leaf ) {
        str_copy(base, leaf, baselen);
        return;
    }
    blen = (ULONG)(dot - leaf);
    if( blen >= baselen ) {
        blen = baselen - 1;
    }
    for( i = 0; i < blen; i++ ) {
        base[i] = leaf[i];
    }
    base[blen] = '\0';
    str_copy(ext, dot, extlen);
}

STATIC UWORD put_long(STRPTR dst, LONG n)
{
    UBYTE tmp[12];
    UWORD i;
    UWORD j;
    ULONG v;

    i = 0;
    if( n < 0 ) {
        dst[i++] = '-';
        v = (ULONG)(-n);
    } else {
        v = (ULONG)n;
    }
    j = 0;
    do {
        tmp[j++] = (UBYTE)('0' + (v % 10));
        v /= 10;
    } while( v && j < 11 );
    while( j > 0 ) {
        dst[i++] = tmp[--j];
    }
    dst[i] = '\0';
    return i;
}

STATIC VOID make_datestamp(STRPTR out, ULONG outlen)
{
    struct DateTime dt;
    UBYTE datebuf[16];
    UBYTE timebuf[16];
    UBYTE daybuf[16];
    ULONG i;
    ULONG j;

    if( out == NULL || outlen == 0 ) {
        return;
    }
    out[0] = '\0';
    DateStamp(&dt.dat_Stamp);
    dt.dat_Format = FORMAT_USA;
    dt.dat_Flags = 0;
    dt.dat_StrDay = daybuf;
    dt.dat_StrDate = datebuf;
    dt.dat_StrTime = timebuf;
    if( ! DateToStr(&dt) || str_len(datebuf) < 8 ) {
        i = put_long(out, dt.dat_Stamp.ds_Days);
        if( i + 1 < outlen ) {
            out[i++] = '-';
            i += put_long(out + i, dt.dat_Stamp.ds_Minute);
        }
        if( i + 1 < outlen ) {
            out[i++] = '-';
            put_long(out + i, dt.dat_Stamp.ds_Tick);
        }
        return;
    }
    /* MM-DD-YY + HH:MM:SS -> YYMMDD-HHMMSS */
    if( outlen < 14 ) {
        return;
    }
    out[0] = datebuf[6];
    out[1] = datebuf[7];
    out[2] = datebuf[0];
    out[3] = datebuf[1];
    out[4] = datebuf[3];
    out[5] = datebuf[4];
    out[6] = '-';
    j = 7;
    for( i = 0; timebuf[i] && j < outlen - 1; i++ ) {
        if( timebuf[i] >= '0' && timebuf[i] <= '9' ) {
            out[j++] = timebuf[i];
        }
    }
    out[j] = '\0';
}

STATIC BOOL unique_dest_name(STRPTR dir, STRPTR leaf, STRPTR out, ULONG outlen)
{
    UBYTE candidate[MAX_PATH_LEN];
    UBYTE base[108];
    UBYTE ext[32];
    UBYTE stamp[32];
    UBYTE unique_leaf[TRASH_MAX_LEAF + 1];
    UBYTE num[8];
    LONG max_base;
    LONG n;
    LONG elen;
    LONG slen;
    LONG nlen;
    LONG mb;

    if( ! safe_join(candidate, sizeof(candidate), dir, leaf) ) {
        return FALSE;
    }
    if( destination_family_free(candidate) ) {
        str_copy(out, candidate, outlen);
        return TRUE;
    }

    split_leaf(leaf, base, sizeof(base), ext, sizeof(ext));
    make_datestamp(stamp, sizeof(stamp));
    elen = (LONG)str_len(ext);
    slen = (LONG)str_len(stamp);
    max_base = (LONG)TRASH_SAFE_LEAF - 1 - slen - elen;
    if( max_base < 1 ) {
        max_base = 1;
    }
    if( (LONG)str_len(base) > max_base ) {
        base[max_base] = '\0';
    }

    str_copy(unique_leaf, base, sizeof(unique_leaf));
    str_cat(unique_leaf, "-", sizeof(unique_leaf));
    str_cat(unique_leaf, stamp, sizeof(unique_leaf));
    str_cat(unique_leaf, ext, sizeof(unique_leaf));
    unique_leaf[TRASH_SAFE_LEAF] = '\0';
    if( safe_join(candidate, sizeof(candidate), dir, unique_leaf)
        && destination_family_free(candidate) ) {
        str_copy(out, candidate, outlen);
        return TRUE;
    }

    for( n = 2; n < 1000; n++ ) {
        num[0] = '-';
        put_long(num + 1, n);
        nlen = (LONG)str_len(num);
        mb = (LONG)TRASH_SAFE_LEAF - 1 - slen - nlen - elen;
        if( mb < 1 ) {
            mb = 1;
        }
        split_leaf(leaf, base, sizeof(base), ext, sizeof(ext));
        if( (LONG)str_len(base) > mb ) {
            base[mb] = '\0';
        }
        str_copy(unique_leaf, base, sizeof(unique_leaf));
        str_cat(unique_leaf, "-", sizeof(unique_leaf));
        str_cat(unique_leaf, stamp, sizeof(unique_leaf));
        str_cat(unique_leaf, num, sizeof(unique_leaf));
        str_cat(unique_leaf, ext, sizeof(unique_leaf));
        unique_leaf[TRASH_SAFE_LEAF] = '\0';
        if( safe_join(candidate, sizeof(candidate), dir, unique_leaf)
            && destination_family_free(candidate) ) {
            str_copy(out, candidate, outlen);
            return TRUE;
        }
    }
    return FALSE;
}

STATIC BOOL clear_delete_protect(STRPTR path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    ULONG prot;
    BOOL changed;

    changed = FALSE;
    lock = Lock(path, SHARED_LOCK);
    if( lock == ZERO ) {
        return FALSE;
    }
    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),
        MEMF_PUBLIC | MEMF_CLEAR);
    if( fib == NULL ) {
        UnLock(lock);
        return FALSE;
    }
    if( Examine(lock, fib) ) {
        prot = fib->fib_Protection;
        if( prot & FIBF_DELETE ) {
            UnLock(lock);
            lock = ZERO;
            if( SetProtection(path, prot & ~FIBF_DELETE) ) {
                changed = TRUE;
            }
        }
    }
    FreeMem(fib, sizeof(struct FileInfoBlock));
    if( lock != ZERO ) {
        UnLock(lock);
    }
    return changed;
}

STATIC BOOL rename_one(STRPTR src, STRPTR dst)
{
    LONG err;

    if( Rename(src, dst) ) {
        return TRUE;
    }
    err = IoErr();
    if( err == ERROR_DELETE_PROTECTED || err == ERROR_WRITE_PROTECTED ) {
        if( clear_delete_protect(src) ) {
            if( Rename(src, dst) ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

STATIC BOOL path_in_volume_trashcan(STRPTR path)
{
    ULONG i;
    ULONG j;
    UBYTE a;
    STATIC UBYTE tc[] = "Trashcan";

    if( path == NULL ) {
        return FALSE;
    }
    for( i = 0; path[i]; i++ ) {
        if( path[i] != ':' ) {
            continue;
        }
        for( j = 0; tc[j]; j++ ) {
            a = path[i + 1 + j];
            if( ! chars_eq_i(a, tc[j]) ) {
                break;
            }
        }
        if( tc[j] == '\0' && (path[i + 1 + j] == '\0' || path[i + 1 + j] == '/') ) {
            return TRUE;
        }
    }
    return FALSE;
}

STATIC VOID trash_update_wb(STRPTR path, LONG action)
{
    struct Library *wb;
    struct Library *saved;
    BPTR parent;
    BPTR troot;
    UBYTE name[108];
    UBYTE parent_path[MAX_PATH_LEN];
    STRPTR leaf;
    ULONG n;
    BOOL closeIt;

    if( path == NULL || path[0] == '\0' ) {
        return;
    }

    trash_file_part(path, name, sizeof(name));
    n = str_len(name);
    if( n >= 5 && is_info_file(name) ) {
        name[n - 5] = '\0';
    }
    if( name[0] == '\0' ) {
        return;
    }

    closeIt = FALSE;
    wb = WorkbenchBase;
    if( wb == NULL || wb->lib_Version < 37 ) {
        wb = OpenLibrary("workbench.library", 37);
        if( wb == NULL ) {
            return;
        }
        closeIt = TRUE;
    }
    saved = WorkbenchBase;
    WorkbenchBase = wb;

    str_copy(parent_path, path, sizeof(parent_path));
    leaf = PathPart(parent_path);
    if( leaf != NULL && leaf != parent_path ) {
        if( leaf > parent_path && leaf[-1] == '/' ) {
            leaf[-1] = '\0';
        } else {
            *leaf = '\0';
        }
        if( parent_path[0] ) {
            parent = Lock(parent_path, SHARED_LOCK);
            if( parent != ZERO ) {
                UpdateWorkbench(name, parent, action);
                UnLock(parent);
            }
        }
    }

    if( path_in_volume_trashcan(path) && ! text_prefix_i(path, "TRASH:") ) {
        troot = Lock("TRASH:", SHARED_LOCK);
        if( troot != ZERO ) {
            UpdateWorkbench(name, troot, action);
            UnLock(troot);
        }
    }

    WorkbenchBase = saved;
    if( closeIt ) {
        CloseLibrary(wb);
    }
}

STATIC BOOL info_exists(STRPTR object)
{
    UBYTE info[MAX_PATH_LEN];

    if( ! get_corresponding_info(object, info, sizeof(info)) ) {
        return FALSE;
    }
    return path_exists(info);
}

STATIC BOOL tooltype_is_ours(STRPTR tt)
{
    ULONG n;

    if( tt == NULL ) {
        return FALSE;
    }
    n = str_len(TRASH_TOOLTYPE_ORIG);
    if( ! text_prefix_i(tt, TRASH_TOOLTYPE_ORIG) ) {
        return FALSE;
    }
    return (tt[n] == '=' || tt[n] == '\0') ? TRUE : FALSE;
}

STATIC BOOL write_tooltype_orig(STRPTR object, STRPTR original)
{
    struct DiskObject *dobj;
    STRPTR *oldtt;
    STRPTR *newtt;
    STRPTR entry;
    BOOL ok;
    LONG count;
    LONG keep;
    LONG i;
    LONG j;
    LONG elen;

    ok = FALSE;
    if( object == NULL || original == NULL || ! info_exists(object) ) {
        return FALSE;
    }
    if( IconBase == NULL ) {
        return FALSE;
    }
    elen = (LONG)str_len(TRASH_TOOLTYPE_ORIG) + 1 + (LONG)str_len(original);
    dobj = GetDiskObject(object);
    if( dobj == NULL ) {
        return FALSE;
    }
    oldtt = dobj->do_ToolTypes;
    count = 0;
    keep = 0;
    if( oldtt ) {
        while( oldtt[count] ) {
            if( ! tooltype_is_ours(oldtt[count]) ) {
                keep++;
            }
            count++;
        }
    }
    newtt = (STRPTR *)AllocMem((ULONG)(keep + 2) * sizeof(STRPTR),
        MEMF_PUBLIC | MEMF_CLEAR);
    entry = (STRPTR)AllocMem((ULONG)elen + 1, MEMF_PUBLIC | MEMF_CLEAR);
    if( newtt && entry ) {
        j = 0;
        if( oldtt ) {
            for( i = 0; i < count; i++ ) {
                if( ! tooltype_is_ours(oldtt[i]) ) {
                    newtt[j++] = oldtt[i];
                }
            }
        }
        str_copy(entry, TRASH_TOOLTYPE_ORIG, (ULONG)elen + 1);
        str_cat(entry, "=", (ULONG)elen + 1);
        str_cat(entry, original, (ULONG)elen + 1);
        newtt[j++] = entry;
        newtt[j] = NULL;
        dobj->do_ToolTypes = newtt;
        if( PutDiskObject(object, dobj) ) {
            ok = TRUE;
        }
        dobj->do_ToolTypes = oldtt;
    }
    if( entry ) {
        FreeMem(entry, (ULONG)elen + 1);
    }
    if( newtt ) {
        FreeMem(newtt, (ULONG)(keep + 2) * sizeof(STRPTR));
    }
    FreeDiskObject(dobj);
    return ok;
}

STATIC VOID clear_tooltype_orig(STRPTR object)
{
    struct DiskObject *dobj;
    STRPTR *oldtt;
    STRPTR *newtt;
    LONG count;
    LONG keep;
    LONG i;
    LONG j;

    if( object == NULL || ! info_exists(object) || IconBase == NULL ) {
        return;
    }
    dobj = GetDiskObject(object);
    if( dobj == NULL ) {
        return;
    }
    oldtt = dobj->do_ToolTypes;
    count = 0;
    keep = 0;
    if( oldtt ) {
        while( oldtt[count] ) {
            if( ! tooltype_is_ours(oldtt[count]) ) {
                keep++;
            }
            count++;
        }
    }
    if( keep != count ) {
        newtt = (STRPTR *)AllocMem((ULONG)(keep + 1) * sizeof(STRPTR),
            MEMF_PUBLIC | MEMF_CLEAR);
        if( newtt ) {
            j = 0;
            for( i = 0; i < count; i++ ) {
                if( ! tooltype_is_ours(oldtt[i]) ) {
                    newtt[j++] = oldtt[i];
                }
            }
            newtt[j] = NULL;
            dobj->do_ToolTypes = newtt;
            PutDiskObject(object, dobj);
            dobj->do_ToolTypes = oldtt;
            FreeMem(newtt, (ULONG)(keep + 1) * sizeof(STRPTR));
        }
    }
    FreeDiskObject(dobj);
}

STATIC BOOL write_trash_metadata(STRPTR trashPath, STRPTR original)
{
    UBYTE comment[MAX_COMMENT_LENGTH + 1];

    if( trashPath == NULL || original == NULL
        || trashPath[0] == '\0' || original[0] == '\0' ) {
        return FALSE;
    }
    if( str_len(TRASH_COMMENT_PREFIX) + str_len(original) <= MAX_COMMENT_LENGTH ) {
        str_copy(comment, TRASH_COMMENT_PREFIX, sizeof(comment));
        str_cat(comment, original, sizeof(comment));
        if( ! SetComment(trashPath, comment) ) {
            return FALSE;
        }
        clear_tooltype_orig(trashPath);
        return TRUE;
    }
    if( info_exists(trashPath) ) {
        if( ! write_tooltype_orig(trashPath, original) ) {
            return FALSE;
        }
        SetComment(trashPath, "");
        return TRUE;
    }
    return FALSE;
}

STATIC VOID remove_trash_metadata(STRPTR trashPath)
{
    if( trashPath == NULL || trashPath[0] == '\0' ) {
        return;
    }
    SetComment(trashPath, "");
    clear_tooltype_orig(trashPath);
}

STATIC BOOL resolve_orphan_info(STRPTR src, STRPTR out, ULONG outlen)
{
    UBYTE info[MAX_PATH_LEN];

    if( src == NULL || out == NULL || outlen == 0 ) {
        return FALSE;
    }
    out[0] = '\0';
    if( path_exists(src) || is_info_file(src) ) {
        return FALSE;
    }
    if( ! get_corresponding_info(src, info, sizeof(info)) ) {
        return FALSE;
    }
    if( ! path_exists(info) ) {
        return FALSE;
    }
    str_copy(out, info, outlen);
    return TRUE;
}

STATIC BOOL move_to_trash_dir(STRPTR srcIn, STRPTR trashdir)
{
    UBYTE leaf[108];
    UBYTE dest[MAX_PATH_LEN];
    UBYTE infoSrc[MAX_PATH_LEN];
    UBYTE infoDest[MAX_PATH_LEN];
    UBYTE companion[MAX_PATH_LEN];
    UBYTE orphan[MAX_PATH_LEN];
    STRPTR src;
    STRPTR original;
    STRPTR notifySrc;
    BOOL haveInfo;
    BOOL srcIsInfo;
    BOOL rolled;
    struct DateStamp stamp;

    src = srcIn;
    orphan[0] = '\0';
    if( src != NULL && resolve_orphan_info(src, orphan, sizeof(orphan)) ) {
        src = orphan;
    }
    if( src == NULL || str_len(src) > DOS_SAFE_PATH_LEN
        || str_len(trashdir) > DOS_SAFE_PATH_LEN ) {
        return FALSE;
    }
    trash_file_part(src, leaf, sizeof(leaf));
    if( leaf[0] == '\0' || ! path_is_drawer(trashdir)
        || ! path_exists(src) || trash_path_is_protected(src) ) {
        return FALSE;
    }

    srcIsInfo = is_info_file(src);
    haveInfo = FALSE;
    companion[0] = '\0';

    if( srcIsInfo
        && get_corresponding_file(src, companion, sizeof(companion))
        && path_exists(companion) ) {
        if( trash_path_is_protected(companion) ) {
            return FALSE;
        }
        trash_file_part(companion, leaf, sizeof(leaf));
        if( ! unique_dest_name(trashdir, leaf, dest, sizeof(dest)) ) {
            return FALSE;
        }
        if( str_len(dest) + 5 >= sizeof(infoDest) ) {
            return FALSE;
        }
        str_copy(infoDest, dest, sizeof(infoDest));
        str_cat(infoDest, ".info", sizeof(infoDest));
        trash_update_wb(companion, UPDATEWB_ObjectRemoved);
        if( ! rename_one(companion, dest) ) {
            trash_update_wb(companion, UPDATEWB_ObjectAdded);
            return FALSE;
        }
        if( ! rename_one(src, infoDest) ) {
            rename_one(dest, companion);
            trash_update_wb(companion, UPDATEWB_ObjectAdded);
            return FALSE;
        }
        original = companion;
        if( ! write_trash_metadata(dest, original) ) {
            rolled = rename_one(infoDest, src);
            if( rolled ) {
                remove_trash_metadata(dest);
                rename_one(dest, companion);
            }
            trash_update_wb(companion, UPDATEWB_ObjectAdded);
            return FALSE;
        }
        DateStamp(&stamp);
        SetFileDate(dest, &stamp);
        trash_update_wb(dest, UPDATEWB_ObjectAdded);
        return TRUE;
    }

    if( ! unique_dest_name(trashdir, leaf, dest, sizeof(dest)) ) {
        return FALSE;
    }

    if( ! srcIsInfo && get_corresponding_info(src, infoSrc, sizeof(infoSrc))
        && path_exists(infoSrc) ) {
        if( trash_path_is_protected(infoSrc) ) {
            return FALSE;
        }
        haveInfo = TRUE;
    }

    notifySrc = src;
    trash_update_wb(notifySrc, UPDATEWB_ObjectRemoved);
    if( ! rename_one(src, dest) ) {
        trash_update_wb(notifySrc, UPDATEWB_ObjectAdded);
        return FALSE;
    }
    if( haveInfo ) {
        trash_file_part(dest, leaf, sizeof(leaf));
        str_copy(companion, leaf, sizeof(companion));
        str_cat(companion, ".info", sizeof(companion));
        if( ! safe_join(infoDest, sizeof(infoDest), trashdir, companion)
            || ! rename_one(infoSrc, infoDest) ) {
            rename_one(dest, src);
            trash_update_wb(notifySrc, UPDATEWB_ObjectAdded);
            return FALSE;
        }
    }

    if( ! write_trash_metadata(dest, src) ) {
        if( haveInfo ) {
            rename_one(infoDest, infoSrc);
        }
        remove_trash_metadata(dest);
        rename_one(dest, src);
        trash_update_wb(notifySrc, UPDATEWB_ObjectAdded);
        return FALSE;
    }
    DateStamp(&stamp);
    SetFileDate(dest, &stamp);
    trash_update_wb(dest, UPDATEWB_ObjectAdded);
    return TRUE;
}

BOOL trash_move_path(STRPTR src)
{
    UBYTE volume[32];
    UBYTE trashdir[MAX_PATH_LEN];

    if( src == NULL || src[0] == '\0' ) {
        return FALSE;
    }
    if( str_len(src) > DOS_SAFE_PATH_LEN || trash_path_is_protected(src) ) {
        return FALSE;
    }
    trash_volume_of_path(src, volume, sizeof(volume));
    if( volume[0] == '\0' ) {
        return FALSE;
    }
    if( ! trash_ensure_trashcan(volume) ) {
        return FALSE;
    }
    if( ! make_trashcan_path(trashdir, sizeof(trashdir), volume) ) {
        return FALSE;
    }
    return move_to_trash_dir(src, trashdir);
}
