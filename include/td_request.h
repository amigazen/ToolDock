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
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>

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

    result = 0;
    screen = NULL;
    obj = NULL;
    reqClass = NULL;

    RequesterBase = OpenLibrary("requester.class", 44);
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

            obj = (Object *)NewObjectA(reqClass, NULL, tags);
            if( obj ) {
                if( win == NULL && RequesterBase->lib_Version < 47 ) {
                    screen = LockPubScreen(NULL);
                }
                msg.MethodID = TDRM_OPENREQ;
                msg.or_Attrs = NULL;
                msg.or_Window = win;
                msg.or_Screen = screen;
                result = (LONG)DoMethodA(obj, (Msg)&msg);
                if( screen ) {
                    UnlockPubScreen(NULL, screen);
                }
                DisposeObject(obj);
                CloseLibrary(RequesterBase);
                RequesterBase = NULL;
                return result;
            }
        }
        CloseLibrary(RequesterBase);
        RequesterBase = NULL;
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
