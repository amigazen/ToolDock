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
#include <intuition/screens.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/graphics_protos.h>
#include <clib/layers_protos.h>

UWORD get_max_window_size(struct Screen *screen, DockPosition pos)
{
    switch( pos ) {
        case DP_LEFT:
        case DP_RIGHT:
            return (UWORD)(screen->Height - screen->BarHeight - screen->BarVBorder);

        case DP_TOP:
        case DP_BOTTOM:
            return (UWORD)screen->Width;        
    }
    return 0;
}

VOID free_render_bitmap(struct DockWindow *dock)
{
    struct Region *r;

    dock->renderRP.BitMap = NULL;
    dock->renderRP.Layer = NULL;
    dock->renderW = 0;
    dock->renderH = 0;

    if( dock->renderBM ) {
        FreeBitMap(dock->renderBM);
        dock->renderBM = NULL;
        DEBUG(DB_Printf(__FUNC__ ": FreeBitMap\n"));
    }

    if( dock->renderL ) {
        if( r = InstallClipRegion(dock->renderL, NULL) ) {
            DisposeRegion(r);
            DEBUG(DB_Printf(__FUNC__ ": DisposeRegion\n"));
        }

        dock->renderL->rp->BitMap = NULL;

        DeleteLayer(0L, dock->renderL);
        dock->renderL = NULL;
        DEBUG(DB_Printf(__FUNC__ ": DeleteLayer\n"));
    }

    if( dock->renderLI ) {
        DisposeLayerInfo(dock->renderLI);
        dock->renderLI = NULL;
        DEBUG(DB_Printf(__FUNC__ ": DisposeLayerInfo\n"));
    }   
}

VOID update_render_bitmap(struct DockWindow *dock, UWORD w, UWORD h)
{
    struct Screen *screen;   
    struct Region *region;
    struct Rectangle r;
    ULONG d, il;

    if( w == dock->renderW && h == dock->renderH ) {
        return;
    }

    DEBUG(DB_Printf(__FUNC__ ": New window size: %ld, %ld\n", (LONG)w, (LONG)h));

    if( w < 1 || h < 1 ) {
        return;
    }

    free_render_bitmap(dock);

    if( screen = LockPubScreen(NULL) ) {

        d = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
        il = GetBitMapAttr(screen->RastPort.BitMap, BMA_FLAGS) & BMF_INTERLEAVED;

        if( dock->renderBM = AllocBitMap(
                                w, h, 
                                d, 
                                BMF_DISPLAYABLE | il, 
                                screen->RastPort.BitMap) ) {
  
            dock->renderW = w;
            dock->renderH = h;
  
            if( dock->renderLI = NewLayerInfo() ) {
                
                if( dock->renderL = CreateUpfrontLayer(
                                      dock->renderLI,
                                      dock->renderBM, 
                                      0, 0, 
                                      w - 1, h - 1,
                                      LAYERSIMPLE,
                                      NULL) ) {

                    dock->renderRP.BitMap = dock->renderBM;
                    dock->renderRP.Layer = dock->renderL;
    
                    if( region = NewRegion() ) {

                        r.MinX = 0;
                        r.MaxX = w - 1;
                        r.MinY = 0;
                        r.MaxY = h - 1;

                        OrRectRegion(region, &r);

                        InstallClipRegion(dock->renderL, region);
                    }
                }
            }
        }

        UnlockPubScreen(NULL, screen);
    }
}

VOID measure_dock_size(struct DockWindow *dock, UWORD *outW, UWORD *outH)
{
    UWORD w;
    UWORD h;
    UWORD max;
    UWORD size;
    UWORD maxSize;
    struct DgNode *curr;
    struct Screen *screen;

    w = DEFAULT_SIZE;
    h = DEFAULT_SIZE;
    max = 0;
    size = 0;
    maxSize = 1024;
    if( screen = LockPubScreen(NULL) ) {
        maxSize = get_max_window_size(screen, dock->cfg.pos);
        UnlockPubScreen(NULL, screen);
    }
    if( maxSize < 8 ) {
        maxSize = 1024;
    }

    FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {
        dock_gadget_get_size(curr->dg, dock->cfg.pos, dock->cfg.align, &w, &h);
        if( w < 1 || w > 128 ) {
            w = DEFAULT_SIZE;
        }
        if( h < 1 || h > 128 ) {
            h = DEFAULT_SIZE;
        }
        w += dock->cfg.gadgetPadding * 2;
        h += dock->cfg.gadgetPadding * 2;
        if( DOCK_HORIZONTAL(dock) ) {
            if( size + w > maxSize ) {
                break;
            }
            size += w;
            if( max < h ) {
                max = h;
            }
        } else {
            if( size + h > maxSize ) {
                break;
            }
            size += h;
            if( max < w ) {
                max = w;
            }
        }
    }
    if( size < 8 ) {
        size = DEFAULT_SIZE;
    }
    if( max < 8 ) {
        max = DEFAULT_SIZE;
    }
    if( ! dock->cfg.showGadgetBorders ) {
        max += 1;
    }
    if( DOCK_HORIZONTAL(dock) ) {
        *outW = size;
        *outH = max;
    } else {
        *outW = max;
        *outH = size;
    }
}

VOID layout_gadgets(struct DockWindow *dock)
{
    UWORD w, h, x = 0, y = 0, i, max, size = 0, maxSize, wx, wy;
    struct DgNode *curr;
    struct Screen *screen;   
    UWORD* sizes;
    UWORD gadgetCount;
    struct GadgetEnvironment env;

    DEBUG(DB_Printf(__FUNC__ "\n"));

    if( dock->disableLayout ) {
        return;
    }
    if( dock->hoverOverDock ) {
        return;
    }

    env.align = dock->cfg.align;
    env.pos = dock->cfg.pos;
    env.showBorders = dock->cfg.showGadgetBorders;
    env.padding = dock->cfg.gadgetPadding;

    if( screen = LockPubScreen(NULL)) {

/*
        if( ! dock->cfg.showGadgetBorders && dock->cfg.gadgetPadding == 0 ) {
            size = 1;   
            x = 1;
            y = 1;                    
        } else {
            size = 0;
            y = 0;
            x = 0;
        }      
*/
        maxSize = get_max_window_size(screen, dock->cfg.pos);

        gadgetCount = 0;    
        FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

            gadgetCount++;
        }

        if( sizes = (UWORD*)TD_AllocMem(sizeof(UWORD) * gadgetCount, MEMF_CLEAR) ) {

            max = 0;

            if( DOCK_HORIZONTAL(dock) ) {

                i = 0;
                FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

                    dock_gadget_get_size(curr->dg, dock->cfg.pos, dock->cfg.align, &w, &h);
                    if( w < 1 || w > 128 ) {
                        w = DEFAULT_SIZE;
                    }
                    if( h < 1 || h > 128 ) {
                        h = DEFAULT_SIZE;
                    }
                        
                    w += dock->cfg.gadgetPadding * 2;
                    h += dock->cfg.gadgetPadding * 2;

                    if( curr == dock->dragNode && dock->runState == RS_REARRANGING ) {
                        sizes[i] = 0;
                        i++;
                        continue;
                    }

                    if( size + w > maxSize ) {
                        break;
                    }
                                       
                    max = max < h ? h : max;
                    size += w;
                    sizes[i] = w;
                    i++;
                }

                if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                    && dock->gapIndex >= 0 ) {
                    size += dock->gapSize;
                }
             
                wx = get_window_left(screen, dock->cfg.pos, dock->cfg.align, size);
                wy = get_window_top(screen, dock->cfg.pos, dock->cfg.align, max);

                i = 0;

                y = max;
                env.gadgetBounds.y = 0;

                if( ! dock->cfg.showGadgetBorders ) {
                    y += 1;
                    env.gadgetBounds.y = 1;
                }
                
                env.windowBounds.x = wx;
                env.windowBounds.y = wy;
                env.windowBounds.w = x;
                env.windowBounds.h = y;

                FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

                    if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                        && i == dock->gapIndex ) {
                        x += dock->gapSize;
                    }

                    if( curr == dock->dragNode && dock->runState == RS_REARRANGING ) {
                        i++;
                        continue;
                    }
                
                    env.index = i;
                    env.isLast = i == gadgetCount - 1;
                    env.gadgetBounds.x = x;
                    env.gadgetBounds.w = sizes[i];
                    env.gadgetBounds.h = max;

                    dock_gadget_set_environment(curr->dg, &env);
                    
                    x += sizes[i];
                    i++;
                }

                if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                    && i == dock->gapIndex ) {
                    x += dock->gapSize;
                } 

            } else {
    
                i = 0;
                FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

                    dock_gadget_get_size(curr->dg, dock->cfg.pos, dock->cfg.align, &w, &h);
                    if( w < 1 || w > 128 ) {
                        w = DEFAULT_SIZE;
                    }
                    if( h < 1 || h > 128 ) {
                        h = DEFAULT_SIZE;
                    }

                    w += dock->cfg.gadgetPadding * 2;
                    h += dock->cfg.gadgetPadding * 2;

                    if( curr == dock->dragNode && dock->runState == RS_REARRANGING ) {
                        sizes[i] = 0;
                        i++;
                        continue;
                    }

                    if( size + h > maxSize ) {
                        break;
                    }                                       

                    max = max < w ? w : max;
                    size += h;
                    sizes[i] = h;
                    i++;
                }

                if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                    && dock->gapIndex >= 0 ) {
                    size += dock->gapSize;
                }

                wx = get_window_left(screen, dock->cfg.pos, dock->cfg.align, max);
                wy = get_window_top(screen, dock->cfg.pos, dock->cfg.align, size);
                
                i = 0;             

                x = max;
                env.gadgetBounds.x = 0;

                if( ! dock->cfg.showGadgetBorders ) {
                    x += 1;
                    env.gadgetBounds.x = 1;
                }
                
                env.windowBounds.x = wx;
                env.windowBounds.y = wy;
                env.windowBounds.w = x;
                env.windowBounds.h = y;

                FOR_EACH_GADGET(&dock->cfg.gadgets, curr) {

                    if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                        && i == dock->gapIndex ) {
                        y += dock->gapSize;
                    }

                    if( curr == dock->dragNode && dock->runState == RS_REARRANGING ) {
                        i++;
                        continue;
                    }
                
                    env.index = i;
                    env.isLast = i == gadgetCount - 1;
                    env.gadgetBounds.y = y;
                    env.gadgetBounds.w = max;
                    env.gadgetBounds.h = sizes[i];

                    dock_gadget_set_environment(curr->dg, &env);

                    y += sizes[i];
                    i++;
                }

                if( dock->runState == RS_REARRANGING && ! dock->rearrangeFromWB
                    && i == dock->gapIndex ) {
                    y += dock->gapSize;
                } 

            }

            if( x < 1 ) {
                x = 1;
            }
            if( y < 1 ) {
                y = 1;
            }
            /* Never grow to screen size. y=maxSize is a full-height empty
               window that looks like a requester. */
            if( DOCK_HORIZONTAL(dock) ) {
                if( x > size + 2 ) {
                    x = size;
                }
                if( y > 128 ) {
                    y = DEFAULT_SIZE;
                }
            } else {
                if( y > size + 2 ) {
                    y = size;
                }
                if( x > 128 ) {
                    x = DEFAULT_SIZE;
                }
            }

            DEBUG(DB_Printf("layout_gadgets: box %ld,%ld %ld x %ld\n",
                (LONG)wx, (LONG)wy, (LONG)x, (LONG)y));

            if( dock->win
                && (dock->win->LeftEdge != (WORD)wx
                    || dock->win->TopEdge != (WORD)wy
                    || dock->win->Width != x
                    || dock->win->Height != y) ) {
                ChangeWindowBox(dock->win, wx, wy, x, y);
            }

            update_render_bitmap(dock, x, y);
            
            TD_FreeMem(sizes, sizeof(UWORD) * gadgetCount);
        }

        UnlockPubScreen(NULL, screen);
    }
}

VOID disable_layout(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));
    dock->disableLayout = TRUE;
}

VOID enable_layout(struct DockWindow *dock)
{
    DEBUG(DB_Printf(__FUNC__ "\n"));

    dock->disableLayout = FALSE;

    layout_gadgets(dock);
}

ULONG get_window_top(struct Screen* screen, DockPosition pos, DockAlign align, UWORD height)
{
	ULONG t = 0;
	switch( pos ) 
	{
		case DP_LEFT:
		case DP_RIGHT:
			switch( align ) 
			{
				case DA_LEFT:
					t = screen->BarHeight + screen->BarVBorder;
					break;
				case DA_CENTER:
					t = (screen->Height - height) / 2;
					break;
				case DA_RIGHT:
					t = screen->Height - height;
					break;
			}
			break;
		case DP_TOP:
			t = screen->BarHeight + screen->BarVBorder;
			break;
		case DP_BOTTOM:
			t = screen->Height - height;
			break;
		
	}
	return t;
}

ULONG get_window_left(struct Screen* screen, DockPosition pos, DockAlign align, UWORD width)
{
	ULONG l = 0;
	switch( pos )
	{
		case DP_TOP:
		case DP_BOTTOM:
			switch( align )
			{
				case DA_LEFT:
					l = 0;
					break;
				case DA_CENTER:
					l = (screen->Width - width) / 2;
					break;
				case DA_RIGHT:
					l = screen->Width - width;
					break;
			}
			break;
		case DP_LEFT:
			l = 0;
			break;
		case DP_RIGHT:
			l = screen->Width - width;
			break;
			
			
	}
	return l;
}

