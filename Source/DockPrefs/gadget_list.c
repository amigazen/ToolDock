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
#include "td_request.h"

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/listbrowser.h>
#include <gadgets/listbrowser.h>

VOID set_main_disabled(struct DockPrefs *prefs, ULONG id, BOOL disabled)
{
    if( prefs->gads[id] == NULL ) {
        return;
    }

    if( prefs->mainWin ) {
        if( SetGadgetAttrs(prefs->gads[id], prefs->mainWin, NULL,
                GA_Disabled, disabled, TAG_END) ) {
            RefreshGList(prefs->gads[id], prefs->mainWin, NULL, 1);
        }
    } else {
        SetAttrs(prefs->gads[id], GA_Disabled, disabled, TAG_END);
    }
}

LONG get_list_index(struct DockPrefs *prefs)
{
    ULONG index;

    index = (ULONG)-1;
    if( prefs->gads[GID_GADGETS] ) {
        GetAttr(LISTBROWSER_Selected, prefs->gads[GID_GADGETS], &index);
    }
    return (LONG)index;
}

VOID set_list_index(struct DockPrefs *prefs, LONG index)
{
    if( prefs->gads[GID_GADGETS] == NULL ) {
        return;
    }

    if( prefs->mainWin ) {
        SetGadgetAttrs(prefs->gads[GID_GADGETS], prefs->mainWin, NULL,
            LISTBROWSER_Selected, index,
            LISTBROWSER_MakeVisible, index,
            TAG_END);
    } else {
        SetAttrs(prefs->gads[GID_GADGETS],
            LISTBROWSER_Selected, index,
            TAG_END);
    }
}

STATIC VOID free_browser_list(struct List **list)
{
    struct Node *n;

    if( *list == NULL ) {
        return;
    }

    while( n = RemTail(*list) ) {
        FreeListBrowserNode(n);
    }
    TD_FreeMem(*list, sizeof(struct List));
    *list = NULL;
}

VOID gadget_selected(struct DockPrefs *prefs, LONG index)
{
    struct DgNode *dg;
    struct DgNode *curr;
    ULONG gadCount;

    dg = NULL;
    gadCount = 0;

    FOR_EACH_GADGET(&prefs->cfg.gadgets, curr) {
        if( (LONG)gadCount == index ) {
            dg = curr;
        }
        gadCount++;
    }

    if( gadCount == 0 || index < 0 ) {
        set_main_disabled(prefs, GID_BTN_UP, TRUE);
        set_main_disabled(prefs, GID_BTN_DOWN, TRUE);
        set_main_disabled(prefs, GID_BTN_EDIT, TRUE);
        set_main_disabled(prefs, GID_BTN_DELETE, TRUE);
    } else {
        set_main_disabled(prefs, GID_BTN_UP, index == 0);
        set_main_disabled(prefs, GID_BTN_DOWN, index == (LONG)(gadCount - 1));
        set_main_disabled(prefs, GID_BTN_EDIT, !dock_gadget_can_edit(dg->dg));
        set_main_disabled(prefs, GID_BTN_DELETE, FALSE);
    }
}

VOID update_gadget_list(struct DockPrefs *prefs)
{
    struct DgNode *curr;
    struct Node *ln;
    struct List *list;
    STRPTR label;
    LONG keep;

    keep = get_list_index(prefs);

    if( prefs->gads[GID_GADGETS] && prefs->mainWin ) {
        SetGadgetAttrs(prefs->gads[GID_GADGETS], prefs->mainWin, NULL,
            LISTBROWSER_Labels, (ULONG)~0, TAG_END);
    }

    free_browser_list(&prefs->gadNodes);

    if( list = (struct List *)TD_AllocMem(sizeof(struct List), MEMF_CLEAR) ) {
        NewList(list);

        FOR_EACH_GADGET(&prefs->cfg.gadgets, curr) {
            dock_gadget_get_label(curr->dg, &label);
            if( label == NULL ) {
                label = curr->n.ln_Name;
            }
            if( ln = AllocListBrowserNode(1,
                    LBNCA_CopyText, TRUE,
                    LBNCA_Text, label,
                    TAG_END) ) {
                AddTail(list, ln);
            }
        }
        prefs->gadNodes = list;
    }

    if( prefs->gads[GID_GADGETS] ) {
        if( prefs->mainWin ) {
            SetGadgetAttrs(prefs->gads[GID_GADGETS], prefs->mainWin, NULL,
                LISTBROWSER_Labels, prefs->gadNodes,
                LISTBROWSER_Selected, keep,
                TAG_END);
        } else {
            SetAttrs(prefs->gads[GID_GADGETS],
                LISTBROWSER_Labels, prefs->gadNodes,
                LISTBROWSER_Selected, keep,
                TAG_END);
        }
    }

    gadget_selected(prefs, keep);
}

struct DgNode *get_selected_gadget(struct DockPrefs *prefs)
{
    struct DgNode *curr;
    LONG index;
    LONG gadCount;

    index = get_list_index(prefs);
    if( index < 0 ) {
        return NULL;
    }

    gadCount = 0;
    FOR_EACH_GADGET(&prefs->cfg.gadgets, curr) {
        if( gadCount == index ) {
            return curr;
        }
        gadCount++;
    }
    return NULL;
}

VOID move_gadget_up(struct DockPrefs *prefs)
{
    struct DgNode *dg;
    struct Node *newPred;
    LONG index;

    if( (dg = get_selected_gadget(prefs)) && dg->n.ln_Pred ) {
        newPred = dg->n.ln_Pred->ln_Pred;
        Remove((struct Node *)dg);
        Insert(&(prefs->cfg.gadgets), (struct Node *)dg, newPred);
        update_gadget_list(prefs);
        index = get_list_index(prefs) - 1;
        set_list_index(prefs, index);
        gadget_selected(prefs, index);
    }
}

VOID move_gadget_down(struct DockPrefs *prefs)
{
    struct DgNode *dg;
    struct Node *newPred;
    LONG index;

    if( (dg = get_selected_gadget(prefs)) && dg->n.ln_Succ && dg->n.ln_Succ->ln_Succ ) {
        newPred = dg->n.ln_Succ;
        Remove((struct Node *)dg);
        Insert(&(prefs->cfg.gadgets), (struct Node *)dg, newPred);
        update_gadget_list(prefs);
        index = get_list_index(prefs) + 1;
        set_list_index(prefs, index);
        gadget_selected(prefs, index);
    }
}

VOID delete_gadget(struct DockPrefs *prefs)
{
    struct DgNode *dg;
    LONG result;

    if( dg = get_selected_gadget(prefs) ) {
        result = TD_OpenRequest(prefs->mainWin, (STRPTR)MSG_DEL_Title,
            (STRPTR)MSG_DEL_Text, (STRPTR)MSG_DEL_Buttons, TDREQIMAGE_QUESTION);
        if( result == 1 ) {
            remove_dock_gadget(dg);
            update_gadget_list(prefs);
            gadget_selected(prefs, get_list_index(prefs));
        }
    }
}

VOID free_gadget_nodes(struct DockPrefs *prefs)
{
    if( prefs->gads[GID_GADGETS] && prefs->mainWin ) {
        SetGadgetAttrs(prefs->gads[GID_GADGETS], prefs->mainWin, NULL,
            LISTBROWSER_Labels, (ULONG)~0, TAG_END);
    }
    free_browser_list(&prefs->gadNodes);
}
