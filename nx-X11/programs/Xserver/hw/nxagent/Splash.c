/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#include "windowstr.h"
#include "scrnintstr.h"

#include "Agent.h"

#include "Xlib.h"
#include "Xutil.h"

#include "Display.h"
#include "Splash.h"
#include "Screen.h"
#include "Windows.h"
#include "Atoms.h"
#include "Trap.h"
#include "Init.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Colors used to paint the splash screen.
 */

#define nxagentLogoWhite       0xffffff
#define nxagentLogoBlack       0x000000
#define nxagentLogoDarkGray    0x222222
#define nxagentLogoLightGray   0xbbbbbb

Pixmap nxagentPixmapLogo;
Window nxagentSplashWindow = None;
Bool nxagentWMPassed = False;

static void nxagentPaintLogo(Window win, XlibGC gc, int scale, int width, int height);

void nxagentShowSplashWindow(Window parentWindow)
{
  XWindowAttributes getAttributes;
  XWindowChanges    values;
  XSetWindowAttributes attributes;

  /*
   * Show splash window only when running as X2Go Agent
   */
  if(!nxagentX2go)
    return;

  #ifdef TEST
  fprintf(stderr, "%s: Got called.\n", __func__);
  #endif

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;

    fprintf(stderr, "%s: Initializing splash start at [%d] milliseconds.\n", __func__,
            GetTimeInMillis() - startTime);
  }
  #endif

  #ifdef NXAGENT_ONSTART
  XSetSelectionOwner(nxagentDisplay, nxagentReadyAtom, None, CurrentTime);
  #endif

  nxagentWMPassed = False;

  /*
   * This would cause a GetWindowAttributes and a
   * GetGeometry (asynchronous) reply. We use instead
   * the geometry requested by the user for the agent
   * window.
   *
   * XGetWindowAttributes(nxagentDisplay, parentWindow, &getAttributes);
   */

  /*
   * During reconnection we draw the splash over
   * the default window and not over the root
   * window because it would be hidden  by other
   * windows.
   */

  if (nxagentReconnectTrap)
  {
    getAttributes.x = nxagentOption(RootX);
    getAttributes.y = nxagentOption(RootY);
  }
  else
  {
    getAttributes.x = 0;
    getAttributes.y = 0;
  }

  getAttributes.width  = nxagentOption(RootWidth);
  getAttributes.height = nxagentOption(RootHeight);

  #ifdef TEST
  fprintf(stderr, "%s: Going to create new splash window.\n", __func__);
  #endif

  nxagentSplashWindow =
      XCreateSimpleWindow(nxagentDisplay,
                          parentWindow,
                          getAttributes.x, getAttributes.y,
                          getAttributes.width, getAttributes.height,
                          0,
                          WhitePixel (nxagentDisplay, 0),
                          BlackPixel (nxagentDisplay, 0));

  #ifdef TEST
  fprintf(stderr, "%s: Created new splash window with id [%ld].\n", __func__,
              nxagentSplashWindow);
  #endif

  XlibGC gc = XCreateGC(nxagentDisplay, nxagentSplashWindow, 0, NULL);
  nxagentPaintLogo(nxagentSplashWindow, gc, 1, getAttributes.width, getAttributes.height);
  XMapRaised (nxagentDisplay, nxagentSplashWindow);
  values.stack_mode = Above;
  XConfigureWindow(nxagentDisplay, nxagentSplashWindow, CWStackMode, &values);
  attributes.override_redirect = True;
  XChangeWindowAttributes(nxagentDisplay, nxagentSplashWindow, CWOverrideRedirect, &attributes);
  XFreeGC(nxagentDisplay, gc);

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;
    fprintf(stderr, "%s: Splash ends [%d] milliseconds.\n", __func__,
            GetTimeInMillis() - startTime);
  }
  #endif
}

void nxagentPaintLogo(Window win, XlibGC gc, int scale, int width, int height)
{
  int depth = DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay));

  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);
  #endif

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "%s: begin\n", __func__);
  fprintf(stderr, "%s: gen params are: w=%d h=%d d=%d w=%x b=%x g1=%x g2=%x \n", __func__,
          width, height, depth,
          nxagentLogoWhite, nxagentLogoBlack, nxagentLogoDarkGray, nxagentLogoLightGray);
  #endif

  int w = width/scale;
  int h = height/scale;

  int w2 = w/2;
  int h2 = h/2;

  int c;
  if (height > width)
  {
    c = w/30;
  }
  else
  {
    c = w/48;
  }

  XSetFunction(nxagentDisplay, gc, GXcopy);
  XSetFillStyle(nxagentDisplay, gc, FillSolid);
  nxagentPixmapLogo = XCreatePixmap(nxagentDisplay, win, width, height, depth);

  if (!nxagentPixmapLogo)
  {
    return;
  }

  if (blackRoot)
  {
    XSetForeground(nxagentDisplay, gc, nxagentLogoBlack);
    XSetBackground(nxagentDisplay, gc, nxagentLogoWhite);
  }
  else
  {
    XSetForeground(nxagentDisplay, gc, nxagentLogoWhite);
    XSetBackground(nxagentDisplay, gc, nxagentLogoBlack);
  }

  XPoint rect[4];
  rect[0].x = 0;               rect[0].y = 0;
  rect[1].x = 0;               rect[1].y = h;
  rect[2].x = w;               rect[2].y = h;
  rect[3].x = w;               rect[3].y = 0;

  /* paint background */
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "%s: filled first poly\n", __func__);
  #endif

  /*
   * Draw X2GO Logo
   */

  if (blackRoot)
    XSetForeground(nxagentDisplay, gc, nxagentLogoDarkGray);
  else
    XSetForeground(nxagentDisplay, gc, nxagentLogoLightGray);

  /*
   * Start 'X'.
   */

  rect[0].x = w2-7*c;               rect[0].y = h2-5*c;
  rect[1].x = w2-8*c;               rect[1].y = h2-5*c;
  rect[2].x = w2-4*c;               rect[2].y = h2+3*c;
  rect[3].x = w2-3*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2-4*c;               rect[0].y = h2-5*c;
  rect[1].x = w2-3*c;               rect[1].y = h2-5*c;
  rect[2].x = w2-7*c;               rect[2].y = h2+3*c;
  rect[3].x = w2-8*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  /*
   * End 'X'.
   */

  /*
   * Start '2'.
   */

  rect[0].x = w2-2*c;               rect[0].y = h2-5*c;
  rect[1].x = w2-1*c;               rect[1].y = h2-5*c;
  rect[2].x = w2-1*c;               rect[2].y = h2-3*c;
  rect[3].x = w2-2*c;               rect[3].y = h2-3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2-2*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+2*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+2*c;               rect[2].y = h2-4*c;
  rect[3].x = w2-2*c;               rect[3].y = h2-4*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+1*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+2*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+2*c;               rect[2].y = h2-2*c;
  rect[3].x = w2+1*c;               rect[3].y = h2-2*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+2*c;               rect[0].y = h2-2*c;
  rect[1].x = w2+1*c;               rect[1].y = h2-2*c;
  rect[2].x = w2-2*c;               rect[2].y = h2+2*c;
  rect[3].x = w2-1*c;               rect[3].y = h2+2*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);


  rect[0].x = w2-2*c;               rect[0].y = h2+2*c;
  rect[1].x = w2+2*c;               rect[1].y = h2+2*c;
  rect[2].x = w2+2*c;               rect[2].y = h2+3*c;
  rect[3].x = w2-2*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);
  /*
   * End '2'.
   */

  /*
   * Start 'G'.
   */

  rect[0].x = w2+3*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+7*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+7*c;               rect[2].y = h2-4*c;
  rect[3].x = w2+3*c;               rect[3].y = h2-4*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+3*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+4*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+4*c;               rect[2].y = h2+3*c;
  rect[3].x = w2+3*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+3*c;               rect[0].y = h2+2*c;
  rect[1].x = w2+7*c;               rect[1].y = h2+2*c;
  rect[2].x = w2+7*c;               rect[2].y = h2+3*c;
  rect[3].x = w2+3*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+6*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+7*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+7*c;               rect[2].y = h2-3*c;
  rect[3].x = w2+6*c;               rect[3].y = h2-3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+6*c;               rect[0].y = h2-0*c;
  rect[1].x = w2+7*c;               rect[1].y = h2-0*c;
  rect[2].x = w2+7*c;               rect[2].y = h2+3*c;
  rect[3].x = w2+6*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+5*c;               rect[0].y = h2-1*c;
  rect[1].x = w2+7*c;               rect[1].y = h2-1*c;
  rect[2].x = w2+7*c;               rect[2].y = h2+0*c;
  rect[3].x = w2+5*c;               rect[3].y = h2+0*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);
  /*
   * End 'G'.
   */

  /*
   * Start 'O'.
   */

  rect[0].x = w2+8*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+12*c;              rect[1].y = h2-5*c;
  rect[2].x = w2+12*c;              rect[2].y = h2-4*c;
  rect[3].x = w2+8*c;               rect[3].y = h2-4*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+8*c;               rect[0].y = h2+3*c;
  rect[1].x = w2+12*c;              rect[1].y = h2+3*c;
  rect[2].x = w2+12*c;              rect[2].y = h2+2*c;
  rect[3].x = w2+8*c;               rect[3].y = h2+2*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+8*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+9*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+9*c;               rect[2].y = h2+3*c;
  rect[3].x = w2+8*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2+11*c;               rect[0].y = h2-5*c;
  rect[1].x = w2+12*c;               rect[1].y = h2-5*c;
  rect[2].x = w2+12*c;               rect[2].y = h2+3*c;
  rect[3].x = w2+11*c;               rect[3].y = h2+3*c;
  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  /*
   * End 'O'.
   */


  XSetWindowBackgroundPixmap(nxagentDisplay, win, nxagentPixmapLogo);

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "%s: end\n", __func__);
  #endif
}

void nxagentRemoveSplashWindow(void)
{
  if (nxagentWMPassed)
    return;

  if (nxagentReconnectTrap)
    return;

  #ifdef TEST
  fprintf(stderr, "%s: Destroying the splash window.\n", __func__);
  #endif

  if (!nxagentWMPassed)
  {
    #ifdef NXAGENT_ONSTART
    XSetSelectionOwner(nxagentDisplay, nxagentReadyAtom,
                          nxagentDefaultWindows[0], CurrentTime);
    #endif

    nxagentWMPassed = True;
  }

  if (nxagentSplashWindow != None)
  {
    XDestroyWindow(nxagentDisplay, nxagentSplashWindow);

    nxagentSplashWindow = None;
    nxagentRefreshWindows(screenInfo.screens[0]->root);

    #ifdef TEST
    fprintf(stderr, "%s: setting the ownership of %s (%d) on window 0x%lx\n", __func__
                "NX_CUT_BUFFER_SERVER", (int)serverTransToAgentProperty, nxagentWindow(screenInfo.screens[0]->root));
    #endif

    XSetSelectionOwner(nxagentDisplay, serverTransToAgentProperty,
                           nxagentWindow(screenInfo.screens[0]->root), CurrentTime);
  }

  if (nxagentPixmapLogo)
  {
    XFreePixmap(nxagentDisplay, nxagentPixmapLogo);
    nxagentPixmapLogo = (Pixmap) 0;
  }
}
