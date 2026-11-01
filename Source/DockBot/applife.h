/************************************
**
**  ToolDock - A Dock For AmigaOS 3
**
**  SPDX-License-Identifier: MIT
**
**  ToolDock changes Copyright 2026 amigazen project
**
************************************/

#ifndef __APPLIFE_H__
#define __APPLIFE_H__

#include <exec/types.h>
#include <intuition/intuition.h>

/*
 * Program-icon tooltypes (FindToolType on the DockButton target):
 *   TD_CX      Commodities broker name (CBD_NAMELEN)
 *   TD_RX      ARexx port name
 *   TD_RXSHOW  ARexx Show / front command
 *   TD_RXHIDE  ARexx Hide command
 *   TD_RXQUIT  ARexx Quit command
 *   TD_RXOPEN  ARexx open-file command (%s = path)
 *   TD_LAUNCH  ONCE | MULTI | DOCS
 *   TD_OPEN    WB | CLI | RX
 */

/* TD_LAUNCH on the program icon. MULTI is the default when omitted. */
#define TD_LAUNCH_MULTI     0
#define TD_LAUNCH_ONCE      1
#define TD_LAUNCH_DOCS      2

/* TD_OPEN on the program icon. DEFAULT follows the button WB/Shell type. */
#define TD_OPEN_DEFAULT     0
#define TD_OPEN_WB          1
#define TD_OPEN_CLI         2
#define TD_OPEN_RX          3

#define AL_POP_NONE         0
#define AL_POP_OPEN         1
#define AL_POP_SHOW         2
#define AL_POP_HIDE         3
#define AL_POP_QUIT         4

#define AL_CAN_SHOW         1
#define AL_CAN_HIDE         2
#define AL_CAN_QUIT         4
#define AL_CAN_OPEN         8

#define AL_RXOPEN_TRIES     8
#define AL_HB_TICKS         4

BOOL applife_init(VOID);
VOID applife_expunge(VOID);
VOID applife_reap(VOID);

BOOL applife_port_present(STRPTR name);
BOOL applife_broker_present(STRPTR name);
BOOL applife_is_running(STRPTR cxName, STRPTR rxName);

VOID applife_show(STRPTR cxName, STRPTR rxName, STRPTR rxShow);
VOID applife_hide(STRPTR cxName, STRPTR rxName, STRPTR rxHide);
VOID applife_quit(STRPTR cxName, STRPTR rxName, STRPTR rxQuit);

BOOL applife_send_rexx(STRPTR port, STRPTR cmd);
STRPTR applife_format_rxopen(STRPTR fmt, STRPTR path);

ULONG applife_open_app_popup(struct Window *win, STRPTR title,
    STRPTR openStr, STRPTR showStr, STRPTR hideStr, STRPTR quitStr,
    UWORD flags);

/* One selectable row. Returns 1 when the item is chosen. */
ULONG applife_open_choice_popup(struct Window *win, STRPTR title,
    STRPTR item, BOOL disabled);

/*
 * Exchange-style menu: each running broker is a row with a Show/Hide/Quit
 * submenu. BrokerCommand is issued here before return.
 */
VOID applife_run_exchange_popup(struct Window *win, STRPTR title,
    STRPTR showStr, STRPTR hideStr, STRPTR quitStr, STRPTR noneStr);

#endif
