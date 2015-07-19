#include "icons.h"
#include "net.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xutil.h>

int error;
int window_error_handler(Display *d, XErrorEvent *e)
{
  d=d;e=e;
  if (e->error_code == BadWindow) {
    error = 1;
  } else {
    perror("X ERROR NOT BAD WINDOW!\n");
    abort();
  }
  return 0;
}

int icon_swallow(TrayWindow *traywin)
{
  XErrorHandler old;

  error = 0;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(display, traywin->id, win, 0, 0);
  XSync(display, 0);
  XSetErrorHandler(old);

  return !error;
}


/*
  The traywin must have its id and type set.
*/
int icon_add(Window id)
{
  TrayWindow *traywin;
  XClassHint *class_hints;
  pNode pos = NULL;

  assert(id);

  traywin = malloc(sizeof(TrayWindow));
  traywin->id = id;

  if (!icon_swallow(traywin)) {
    free(traywin);
    return 0;
  }

  /* find the positon for the systray app window */
  traywin->x = border + (horizontal ? width : 0);
  traywin->y = border + (horizontal ? 0 : height);
  
  class_hints = XAllocClassHint();
  
  if(class_hints)
	  XGetClassHint(display, traywin->id, class_hints);
  
  /* Detect plugin app window*/
  /* FIXME: In other code need set correct position when dock not in right coner or add user option*/
  if((class_hints->res_class) && (strcmp(class_hints->res_class, "WM_DOCK_PLUG") == 0)) {
	XWindowAttributes traywin_attr;
	XGetWindowAttributes(display, traywin->id, &traywin_attr);
	
	/* only in horizontal mode wide-plugin allowed */
	if(horizontal)
		traywin->w = traywin_attr.width;
	else
		traywin->w = icon_size;
	
	traywin->wmplug = 1;
	pos = get_firts_wmplug();
  }
  else {
	  traywin->w = icon_size;
	  traywin->wmplug = 0;
	  pos = (void*)icons;
  }
  
  /* add the new icon to the list */
  if(add_list_node(icons, traywin, pos) == -1) {
	  printf("error: add_list_node filed");
	  free(traywin);
	  return 0;
  }
  
  /* watch for the icon trying to resize itself! BAD ICON! BAD! */
  XSelectInput(display, traywin->id, StructureNotifyMask);
    
  /* position and size the icon window */
  XMoveResizeWindow(display, traywin->id,
                    traywin->x, traywin->y, traywin->w, icon_size);
  
  /* if not appended */
  if(pos)
	  reposition_icons();
  
  /* resize our window so that the new window can fit in it */
  fix_geometry();

  /* flush before clearing, otherwise the clear isn't effective. */
  XFlush(display);
  /* make sure the new child will get the right stuff in its background
     for ParentRelative. */
  XClearWindow(display, win);
  
  /* show the window */
  XMapRaised(display, traywin->id);

  return 1;
}


void icon_remove(pNode node)
{
  XErrorHandler old;
  Window traywin_id = ((TrayWindow*)node->elem)->id;

  net_icon_remove(((TrayWindow*)node->elem));

  XSelectInput(display, ((TrayWindow*)node->elem)->id, NoEventMask);
  
  /* remove it from our list */
  if(delete_list_node(icons, node) == -1)
	  printf("error: delete_list_node filed");
  
  /* reparent it to root */
  error = 0;
  old = XSetErrorHandler(window_error_handler);
  XReparentWindow(display, traywin_id, root, 0, 0);
  XSync(display, 0);
  XSetErrorHandler(old);

  reposition_icons();
  fix_geometry();
}
