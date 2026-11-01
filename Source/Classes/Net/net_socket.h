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

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* Local copy of the Roadshow SocketBaseTagList tags DockNet needs.
** proto/bsdsocket.h pulls the whole TCP include tree and needs the
** Roadshow SDK on the include path. The class still opens
** bsdsocket.library at runtime and shows "--" when it is missing.
*/

#ifndef SBTF_REF
#define SBTF_VAL 0x0000
#define SBTF_REF 0x8000
#define SBTB_CODE 1
#define SBTS_CODE 0x3FFF
#define SBTF_GET 0
#define SBTF_SET 1
#define SBTM_GETREF(code) \
    (TAG_USER | SBTF_REF | (((ULONG)(code) & SBTS_CODE) << SBTB_CODE) | SBTF_GET)
#endif

#ifndef SBTC_GET_BYTES_RECEIVED
#define SBTC_GET_BYTES_RECEIVED 64
#define SBTC_GET_BYTES_SENT 65
#endif

#ifndef SBQUAD_T
typedef struct {
    ULONG sbq_High;
    ULONG sbq_Low;
} SBQUAD_T;
#endif

extern struct Library *SocketBase;

#ifdef __SASC
#pragma libcall SocketBase SocketBaseTagList 126 801
#endif

LONG SocketBaseTagList(struct TagItem *tags);

#endif
