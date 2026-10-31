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

#include <dos/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/button.h>
#include <proto/label.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <images/label.h>

#include <stdio.h>

STATIC VOID free_class_nodes(struct DockPrefs *prefs)
{
    struct Node *n;

    if( prefs->classNodes == NULL ) {
        return;
    }

    if( prefs->newList && prefs->newWin ) {
        SetGadgetAttrs(prefs->newList, prefs->newWin, NULL,
            LISTBROWSER_Labels, (ULONG)~0, TAG_END);
    }

    while( n = RemTail(prefs->classNodes) ) {
        FreeListBrowserNode(n);
    }
    TD_FreeMem(prefs->classNodes, sizeof(struct List));
    prefs->classNodes = NULL;
}

STATIC struct List *build_class_nodes(struct DockPrefs *prefs)
{
    struct List *list;
    struct Node *curr;
    struct Node *ln;

    if( list = (struct List *)TD_AllocMem(sizeof(struct List), MEMF_CLEAR) ) {
        NewList(list);
        for( curr = prefs->classes.lh_Head; curr->ln_Succ; curr = curr->ln_Succ ) {
            if( ln = AllocListBrowserNode(1,
                    LBNCA_CopyText, TRUE,
                    LBNCA_Text, curr->ln_Name,
                    TAG_END) ) {
                AddTail(list, ln);
            }
        }
    }
    return list;
}

STATIC LONG get_class_index(struct DockPrefs *prefs)
{
    ULONG index;

    index = 0;
    if( prefs->newList ) {
        GetAttr(LISTBROWSER_Selected, prefs->newList, &index);
    }
    return (LONG)index;
}

VOID class_selected(struct DockPrefs *prefs)
{
    STRPTR name;
    STRPTR desc;
    STRPTR copy;
    STRPTR vers;
    STRPTR text;
    Object *g;
    struct Node *curr;
    LONG gadCount;
    LONG index;
    ULONG len;

    if( prefs->newDesc == NULL ) {
        return;
    }

    index = get_class_index(prefs);
    gadCount = 0;

    for( curr = prefs->classes.lh_Head; curr->ln_Succ; curr = curr->ln_Succ ) {
        if( gadCount == index ) {
            if( g = TD_CreateDockGadget(curr->ln_Name) ) {
                dock_gadget_get_info(g, &name, &vers, &desc, &copy);
                len = 8;
                if( name ) len += strlen(name);
                if( vers ) len += strlen(vers);
                if( desc ) len += strlen(desc);
                if( copy ) len += strlen(copy);
                if( text = TD_AllocMem(len, MEMF_CLEAR) ) {
                    sprintf(text, "%s %s\n\n%s\n\n%s\n",
                        name ? name : "",
                        vers ? vers : "",
                        desc ? desc : "",
                        copy ? copy : "");
                    if( prefs->newWin ) {
                        SetGadgetAttrs(prefs->newDesc, prefs->newWin, NULL,
                            GA_Text, text, TAG_END);
                    } else {
                        SetAttrs(prefs->newDesc, GA_Text, text, TAG_END);
                    }
                    TD_FreeMem(text, len);
                }
                DisposeObject(g);
            }
            break;
        }
        gadCount++;
    }
}

VOID create_new_gadget(struct DockPrefs *prefs)
{
    struct Node *curr;
    LONG gadCount;
    LONG index;
    struct DgNode *dg;

    index = get_class_index(prefs);
    gadCount = 0;

    for( curr = prefs->classes.lh_Head; curr->ln_Succ; curr = curr->ln_Succ ) {
        if( gadCount == index ) {
            if( dg = add_dock_gadget(prefs, curr->ln_Name) ) {
                update_gadget_list(prefs);
                if( dock_gadget_can_edit(dg->dg) ) {
                    edit_gadget(prefs, dg);
                    prefs->editGadgetIsNew = TRUE;
                }
            }
            break;
        }
        gadCount++;
    }
}

VOID close_new_gadget_dialog(struct DockPrefs *prefs)
{
    free_class_nodes(prefs);
    if( prefs->newObj ) {
        DisposeObject(prefs->newObj);
        prefs->newObj = NULL;
        prefs->newWin = NULL;
        prefs->newList = NULL;
        prefs->newDesc = NULL;
    }
}

VOID open_new_gadget_dialog(struct DockPrefs *prefs)
{
    if( prefs->newObj ) {
        return;
    }

    prefs->classNodes = build_class_nodes(prefs);

    prefs->newObj = WindowObject,
        WA_Title, MSG_NGD_Title,
        WA_CloseGadget, TRUE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_SizeGadget, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_SharedPort, prefs->idcmpPort,
        WINDOW_ParentGroup, VLayoutObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_AddChild, prefs->newList = (struct Gadget *)ListBrowserObject,
                GA_ID, NID_LIST,
                GA_RelVerify, TRUE,
                LISTBROWSER_Labels, prefs->classNodes,
                LISTBROWSER_ShowSelected, TRUE,
                LISTBROWSER_Selected, 0,
            ListBrowserEnd,
            MemberLabel(MSG_NGD_ChooseGadget),
            LAYOUT_AddChild, prefs->newDesc = (struct Gadget *)ButtonObject,
                GA_ID, NID_DESC,
                GA_ReadOnly, TRUE,
                GA_Text, "",
            ButtonEnd,
            CHILD_MinHeight, 60,
            MemberLabel(MSG_NGD_About),
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_EvenSize, TRUE,
                LAYOUT_AddChild, ButtonObject,
                    GA_ID, NID_OK,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_OK,
                ButtonEnd,
                LAYOUT_AddChild, ButtonObject,
                    GA_ID, NID_CANCEL,
                    GA_RelVerify, TRUE,
                    GA_Text, MSG_Cancel,
                ButtonEnd,
            End,
            CHILD_WeightedHeight, 0,
        End,
    EndWindow;

    if( prefs->newObj == NULL ) {
        free_class_nodes(prefs);
        TD_ShowError((STRPTR)MSG_ERR_OpenWindow);
        return;
    }

    prefs->newWin = RA_OpenWindow(prefs->newObj);
    if( prefs->newWin == NULL ) {
        close_new_gadget_dialog(prefs);
        TD_ShowError((STRPTR)MSG_ERR_OpenWindow);
        return;
    }

    class_selected(prefs);
}

VOID handle_new_gadget_input(struct DockPrefs *prefs)
{
    ULONG result;
    UWORD code;

    if( prefs->newObj == NULL ) {
        return;
    }

    while( (result = RA_HandleInput(prefs->newObj, &code)) != WMHI_LASTMSG ) {
        switch( result & WMHI_CLASSMASK ) {
            case WMHI_CLOSEWINDOW:
                close_new_gadget_dialog(prefs);
                return;

            case WMHI_GADGETUP:
                switch( result & WMHI_GADGETMASK ) {
                    case NID_LIST:
                        class_selected(prefs);
                        break;
                    case NID_OK:
                        create_new_gadget(prefs);
                        close_new_gadget_dialog(prefs);
                        return;
                    case NID_CANCEL:
                        close_new_gadget_dialog(prefs);
                        return;
                }
                break;
        }
    }
}

VOID add_dropped_icon(struct DockPrefs *prefs, struct AppMessage *msg)
{
    STRPTR path;
    BPTR lock;
    struct FileInfoBlock *fib;
    struct DgNode *dg;

    if( msg == NULL || msg->am_NumArgs < 1 ) {
        return;
    }

    if( path = (STRPTR)TD_AllocMem(MAX_PATH_LENGTH, MEMF_ANY) ) {
        path[0] = 0;
        NameFromLock(msg->am_ArgList->wa_Lock, path, MAX_PATH_LENGTH);
        AddPart(path, msg->am_ArgList->wa_Name, MAX_PATH_LENGTH);

        if( lock = Lock(path, ACCESS_READ) ) {
            if( fib = AllocDosObjectTags(DOS_FIB, TAG_DONE) ) {
                if( Examine(lock, fib) ) {
                    if( fib->fib_DirEntryType < 0 ) {
                        if( dg = add_dock_gadget(prefs, DB_BUTTON_CLASS) ) {
                            dock_gadget_init_button(dg->dg, msg->am_ArgList->wa_Name, path);
                            update_gadget_list(prefs);
                            edit_gadget(prefs, dg);
                            prefs->editGadgetIsNew = TRUE;
                        }
                    } else {
                        TD_OpenRequest(prefs->mainWin, (STRPTR)MSG_NGD_Title,
                            (STRPTR)MSG_ERR_CantAddDirectory, (STRPTR)MSG_OK,
                            TDREQIMAGE_ERROR);
                    }
                }
                FreeDosObject(DOS_FIB, fib);
            }
            UnLock(lock);
        }
        TD_FreeMem(path, MAX_PATH_LENGTH);
    }
}
