/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**  DockBot Copyright 2019 Andrew Kennan
**
**  requester.class when CLASSES:requester.class (or LIBS:) is available,
**  otherwise EasyRequest().
**
************************************/

#ifndef __TD_REQUEST_H__
#define __TD_REQUEST_H__

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <utility/tagitem.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>

#ifndef REACTION_Dummy
#define REACTION_Dummy (TAG_USER + 0x5000000)
#endif

#define TDREQ_Dummy         (REACTION_Dummy + 0x45000)
#define TDREQ_Type          (TDREQ_Dummy + 1)
#define TDREQ_TitleText     (TDREQ_Dummy + 2)
#define TDREQ_BodyText      (TDREQ_Dummy + 3)
#define TDREQ_GadgetText    (TDREQ_Dummy + 4)
#define TDREQ_Image         (TDREQ_Dummy + 7)

#define TDREQTYPE_INFO      0

#define TDREQIMAGE_DEFAULT  0
#define TDREQIMAGE_INFO     1
#define TDREQIMAGE_WARNING  2
#define TDREQIMAGE_ERROR    3
#define TDREQIMAGE_QUESTION 4

#define TDRM_OPENREQ        (0x650001L)

struct TDOrRequest
{
    ULONG MethodID;
    struct TagItem *or_Attrs;
    struct Window *or_Window;
    struct Screen *or_Screen;
};

STATIC struct Library *RequesterBase;

Class *REQUESTER_GetClass(VOID);
#pragma libcall RequesterBase REQUESTER_GetClass 1e 00

STATIC struct Library *td_open_requester_class(VOID)
{
    struct Library *base;

    base = OpenLibrary("requester.class", 44);
    if( base == NULL ) {
        base = OpenLibrary("CLASSES:requester.class", 44);
    }
    return base;
}

/* Returns EasyRequest-compatible gadget numbers: 1, 2, ... , 0. */
STATIC LONG TD_OpenRequest(struct Window *win, STRPTR title, STRPTR body,
    STRPTR gadgets, ULONG image)
{
    Class *reqClass;
    Object *obj;
    struct TDOrRequest msg;
    struct TagItem tags[7];
    struct EasyStruct es;
    struct Screen *screen;
    LONG result;
    UWORD n;
    BOOL usedClass;

    result = 0;
    obj = NULL;
    reqClass = NULL;
    screen = NULL;
    usedClass = FALSE;

    RequesterBase = td_open_requester_class();
    if( RequesterBase ) {
        reqClass = REQUESTER_GetClass();
        if( reqClass ) {
            n = 0;
            tags[n].ti_Tag = TDREQ_Type;
            tags[n].ti_Data = TDREQTYPE_INFO;
            n++;
            tags[n].ti_Tag = TDREQ_TitleText;
            tags[n].ti_Data = (ULONG)title;
            n++;
            tags[n].ti_Tag = TDREQ_BodyText;
            tags[n].ti_Data = (ULONG)body;
            n++;
            tags[n].ti_Tag = TDREQ_GadgetText;
            tags[n].ti_Data = (ULONG)gadgets;
            n++;
            if( image && RequesterBase->lib_Version >= 47 ) {
                tags[n].ti_Tag = TDREQ_Image;
                tags[n].ti_Data = image;
                n++;
            }
            tags[n].ti_Tag = TAG_DONE;
            tags[n].ti_Data = 0;

            /* Both window and screen NULL makes a full-height deadlock. */
            if( win == NULL ) {
                screen = LockPubScreen(NULL);
            }

            obj = (Object *)NewObjectA(reqClass, NULL, tags);
            if( obj ) {
                msg.MethodID = TDRM_OPENREQ;
                msg.or_Attrs = NULL;
                msg.or_Window = win;
                msg.or_Screen = screen;
                result = (LONG)DoMethodA(obj, (Msg)&msg);
                DisposeObject(obj);
                usedClass = TRUE;
            }
            if( screen ) {
                UnlockPubScreen(NULL, screen);
            }
        }
        CloseLibrary(RequesterBase);
        RequesterBase = NULL;
        if( usedClass ) {
            return result;
        }
    }

    /* intuition.doc EasyRequest: es_Title NULL is "System Request." */
    if( title == NULL ) {
        title = "ToolDock";
    }
    if( gadgets == NULL ) {
        gadgets = "OK";
    }
    es.es_StructSize = sizeof(struct EasyStruct);
    es.es_Flags = 0;
    es.es_Title = title;
    es.es_TextFormat = "%s";
    es.es_GadgetFormat = gadgets;
    result = EasyRequest(win, &es, NULL, body);
    return result;
}

#endif
