#include <X11/Xlib.h>
#include "dtray.h"

typedef int (*onwindow_cb)(Display *dpy, Window win, GC win_gc, \
			  XWindowAttributes *attr);

long		 TRAYICON_WIN_EVENT_MASK;

Display		 *dpy;
Window		 root_win, ico_win;

int			 scr;
int			 scr_depth;
Visual		 *scr_visual;
GC			 _gc;

XWindowAttributes ico_attr;

int init_dpy(const char *Xdpy);
int create_icon_window( int ico_width, int ico_height, char *app_name, \
						int argc, char **argv, onwindow_cb cb_win);
int create_trayicon(int ico_width, int ico_height, char *app_name, \
					int argc, char **argv, onwindow_cb cb_win, onatom_cb onatom);

void set_win_event_mask(long mask);
