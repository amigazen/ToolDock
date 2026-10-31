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

#include "prefs.h"

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>

struct DgNode *add_dock_gadget(struct DockPrefs *prefs, STRPTR name)
{
    struct DgNode *dg;

    DEBUG(printf("add_dock_gadget: %s\n", name));

    if( dg = TD_AllocGadget(name) ) {
        AddTail((struct List *)&prefs->cfg.gadgets, (struct Node *)dg);
        return dg;
    }
    return NULL;
}

VOID remove_dock_gadget(struct DgNode *dg)
{
    DEBUG(printf("remove_dock_gadget: %s\n", dg->n.ln_Name));

    Remove((struct Node *)dg);
    TD_FreeGadget(dg);
}

STATIC VOID free_plugins(struct DockPrefs *prefs)
{
    struct Node *node;
    UWORD len;

    while( ! IsListEmpty(&prefs->classes) ) {
        if( node = RemTail(&prefs->classes) ) {
            len = sizeof(struct Node) + strlen(node->ln_Name) + 1;
            TD_FreeMem(node, len);
        }
    }
}

VOID init_config(struct DockPrefs *prefs)
{
    prefs->cfg.align = DA_CENTER;
    prefs->cfg.pos = DP_RIGHT;
    prefs->cfg.showGadgetLabels = TRUE;
    prefs->cfg.showGadgetBorders = TRUE;
    prefs->cfg.bgBrushPath = NULL;
    prefs->cfg.gadgetPadding = MIN_PADDING;

    NewList(&prefs->cfg.gadgets);
    NewList(&prefs->classes);

    prefs->gadNodes = NULL;
    prefs->classNodes = NULL;
    prefs->posLabels = NULL;
    prefs->alignLabels = NULL;
    prefs->startLabels = NULL;
    prefs->swPosLabels = NULL;
    prefs->mainObj = NULL;
    prefs->mainWin = NULL;
    prefs->newObj = NULL;
    prefs->newWin = NULL;
    prefs->editObj = NULL;
    prefs->editWin = NULL;
    prefs->editGadget = NULL;
    prefs->editGadgetIsNew = FALSE;
    prefs->editKind = EK_NONE;
    prefs->editCount = 0;
}

VOID free_config(struct DockPrefs *prefs)
{
    TD_DisposeConfig(&prefs->cfg);
    free_plugins(prefs);
    FREE_STRING(prefs->cfg.bgBrushPath);
}

BOOL load_config(struct DockPrefs *prefs)
{
    struct DockSettings *s;
    BOOL r;

    r = FALSE;

    if( s = TD_OpenSettingsRead(CONFIG_FILE) ) {
        if( TD_ReadConfig(&prefs->cfg, s) ) {
            r = TRUE;
        }
        TD_CloseSettings(s);
    }
    return r;
}

BOOL save_config(struct DockPrefs *prefs, BOOL permanent)
{
    struct DockSettings *s;
    BOOL r;

    r = FALSE;

    if( s = TD_OpenSettingsWrite(permanent ? CONFIG_FILE_PERM : CONFIG_FILE) ) {
        if( TD_WriteConfig(&prefs->cfg, s) ) {
            r = TRUE;
        }
        TD_CloseSettings(s);
    }
    return r;
}

VOID revert_config(VOID)
{
    BPTR fh1;
    BPTR fh2;
    struct FileInfoBlock *fib;
    BYTE *buf;

    if( fib = AllocDosObjectTags(DOS_FIB, TAG_DONE) ) {
        if( fh1 = Open(CONFIG_FILE_PERM, MODE_OLDFILE) ) {
            if( ExamineFH(fh1, fib) ) {
                if( buf = TD_AllocMem(fib->fib_Size, MEMF_ANY) ) {
                    Read(fh1, buf, fib->fib_Size);
                    if( fh2 = Open(CONFIG_FILE, MODE_READWRITE) ) {
                        SetFileSize(fh2, 0, OFFSET_BEGINNING);
                        Write(fh2, buf, fib->fib_Size);
                        Close(fh2);
                    }
                    TD_FreeMem(buf, fib->fib_Size);
                }
            }
            Close(fh1);
        }
        FreeDosObject(DOS_FIB, fib);
    }
}
