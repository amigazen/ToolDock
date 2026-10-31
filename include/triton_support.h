/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  Copyright 2026 amigazen project
**  Based on DockBot by Andrew Kennan
**
************************************/

#ifndef __TRITON_SUPPORT_H__
#define __TRITON_SUPPORT_H__

#ifndef LIBRARIES_TRITON_H
#include <libraries/triton.h>
#endif

/* triton.lib tag-varargs stubs, not triton.library LVOs. */

#ifndef TR_NOSUPPORT

#ifdef _DCC
#define TR_SUPPORT_STACK  __stkargs
#define TR_SUPPORT_REGS   __regargs
#else
#ifdef __GNUC__
#define TR_SUPPORT_STACK
#define TR_SUPPORT_REGS
#else
#define TR_SUPPORT_STACK  __stdargs
#define TR_SUPPORT_REGS   __regargs
#endif
#endif

TR_SUPPORT_STACK extern BOOL                 TR_OpenTriton(ULONG version, ULONG taglist,...);
TR_SUPPORT_REGS  extern VOID                 TR_CloseTriton(VOID);
TR_SUPPORT_STACK extern struct TR_App     *  TR_CreateAppTags(ULONG taglist,...);
TR_SUPPORT_STACK extern struct TR_Project *  TR_OpenProjectTags(struct TR_App *app, ULONG taglist,...);
TR_SUPPORT_STACK extern ULONG                TR_EasyRequestTags(struct TR_App *app, STRPTR bodymft, STRPTR gadfmt,
						     ULONG taglist,...);
TR_SUPPORT_STACK extern ULONG                TR_AutoRequestTags(struct TR_App *app, struct TR_Project *lockproject,
						     ULONG taglist,...);
TR_SUPPORT_STACK extern BOOL                 TR_AddClassTags(struct TR_App *app, ULONG tag, ULONG superTag,
						  TR_Method defaultMethod, ULONG datasize,
						  ULONG taglist,...);

#endif /* TR_NOSUPPORT */

#endif
