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

#include "dock.h"

#include <exec/memory.h>
#include <exec/libraries.h>

#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

#include <string.h>

const char __ver[40] = "$VER:" APP_NAME " " VERSION_STR;

struct Library *DosBase;
struct Library *LayersBase;
struct Library *GfxBase;
struct Library *IntuitionBase;
struct Library *WorkbenchBase;
struct Library *ToolDockBase;
struct Library *GadToolsBase;
struct Library *CxBase;
struct Library *LocaleBase;
struct Library *IconBase;

BOOL already_running(VOID)
{
    if( FindPort(APP_NAME) ) {
        return TRUE;
    }
    return FALSE;
}

/* RawDoFmt PutChProc: D0 = char, A3 = &cursor. Catalog %d is a WORD. */
STATIC VOID __asm put_fmt_char(register __d0 UBYTE c, register __a3 STRPTR *p)
{
    *(*p)++ = c;
}

struct Library *open_lib(STRPTR name, UWORD version, BOOL optional) 
{
    struct Library *lib;
    STRPTR msg;
    STRPTR cursor;
    UWORD l;
    UBYTE fmtArgs[6];

    DEBUG(DB_Printf("Opening %s V %d...", name, version));

    if( !(lib = OpenLibrary(name, version) ) ) {

        DEBUG(DB_Printf("Failed\n"));

        if( optional ) {
            return NULL;
        }

        l = strlen(name) + strlen(MSG_ERR_OpenLibrary) + 8;
        if( msg = TD_AllocMem(l, MEMF_CLEAR) ) {
            *(WORD *)fmtArgs = version;
            *(STRPTR *)(fmtArgs + 2) = name;
            cursor = msg;
            RawDoFmt(MSG_ERR_OpenLibrary, fmtArgs, (VOID (*)())put_fmt_char, &cursor);
            TD_ShowError(msg);
            TD_FreeMem(msg, l);
        }
    } else {
        DEBUG(DB_Printf("OK\n"));
    }

    return lib;
}

VOID close_lib(STRPTR name, struct Library *lib) 
{
    if( lib ) {
        DEBUG(DB_Printf("Closing %s\n", name));
        CloseLibrary(lib);
    }
}

int main(int argc, char** argv)
{
    struct DockWindow *dock;
 
    if( already_running() ) {
        return 0;
    }

#ifdef DEBUG_BUILD

    parse_args();

#endif

    if( ToolDockBase = OpenLibrary("PROGDIR:tooldock.library", 1) ) {

#ifdef DEBUG_BUILD

        TD_RegisterDebugStream(NULL);
#endif

        DEBUG(TD_RegisterDebugStream(Output()));

        if( DosBase = open_lib("dos.library", 39, FALSE) ) {

            if( GfxBase = open_lib("graphics.library", 39, FALSE) ) {

                if( LayersBase = open_lib("layers.library", 39, FALSE) ) {

                    if( IntuitionBase = open_lib("intuition.library", 39, FALSE) ) {
    
                        if( GadToolsBase = open_lib("gadtools.library", 39, FALSE) ) {
        
                            if( WorkbenchBase = open_lib("workbench.library", 39, FALSE) ) {

                                if( CxBase = open_lib("commodities.library", 39, FALSE) ) {

                                    if( LocaleBase = open_lib("locale.library", 38, FALSE) ) {

                                        if( IconBase = open_lib("icon.library", 37, FALSE) ) {

                                            open_catalog();

                                            if( dock = create_dock() ) {
                                                                   
                                                run_event_loop(dock);                     

                                                free_dock(dock);   
                                            }
    
                                            close_catalog();

                                            close_lib("icon.library", IconBase);
                                        }
                                        close_lib("locale.library", LocaleBase);
                                    }
                                    close_lib("commodities.library", CxBase);
                                }
                                close_lib("workbench.library", WorkbenchBase);
                            }
                            close_lib("gadtools.library", GadToolsBase);
                        }        
                        close_lib("intuition.library", IntuitionBase);
                    }
                    close_lib("layers.library", LayersBase);
                }
                close_lib("graphics.library", GfxBase);
            }
            close_lib("dos.library", DosBase);
        }  
        close_lib("tooldock.library", ToolDockBase);
    }
}
