#include <stdio.h>
#include <X11/Xutil.h>
#include "dwin.h"

int init_dpy(const char *Xdpy)
{
	dpy = XOpenDisplay(Xdpy);
	
	if(!dpy) {
		perror("Can't open display:");
		return -1;
	}
	
	scr = DefaultScreen(dpy);
	root_win = RootWindow(dpy, scr);
	scr_depth = DefaultDepth(dpy, scr);
	scr_visual = DefaultVisual(dpy, scr);
	
	return 0;
}

/* Create window for tray icon
   cb_win - callback for window pre-initialization */
int create_icon_window( int ico_width, int ico_height, char *app_name, \
						int argc, char **argv, onwindow_cb cb_win)
{
	XSetWindowAttributes 	win_attr;
	XSizeHints 				*size_hints;
	XWMHints 				*wm_hints;
	XClassHint 				*class_hints;
	XTextProperty 			win_name, ico_name;
	ulong 					valuemask = 0;
	XGCValues 				values;
	int						ret;
	
	size_hints = XAllocSizeHints();
	if(!size_hints) {
		printf("error: can't allocate size_hints\n");
		return -1;
	}
	
	wm_hints = XAllocWMHints();
	if(!wm_hints) {
		printf("error: can't allocate wm_hints\n");
		XFree(size_hints);
		return -1;
	}
	
	class_hints = XAllocClassHint();
	if (!class_hints) {
		printf("error: can't allocate class_hints\n");
		XFree(size_hints);
		XFree(wm_hints);
		return -1;
	}
	
	ico_win = XCreateWindow(dpy, root_win, 0, 0, ico_width, \
							ico_height, 0, scr_depth, CopyFromParent, \
							scr_visual, 0, &win_attr);
	
	XSetWindowBackgroundPixmap(dpy, ico_win, ParentRelative);
	
	size_hints->flags = USSize | PSize | PMinSize | \
						PMaxSize | PBaseSize | PPosition;
	
	size_hints->width 	= size_hints->min_width \
						= size_hints->max_width \
						= size_hints->base_width \
						= ico_width;
						
	size_hints->height 	= size_hints->min_height \
						= size_hints->max_height \
						= size_hints->base_height \
						= ico_height;
	
	size_hints->x = 0;
	size_hints->y = 0;
		
	if (!XStringListToTextProperty(&app_name, 1, &win_name)) {
		printf("error: can't get win_name\n");
		XFree(size_hints);
		XFree(wm_hints);
		XFree(class_hints);
		XDestroyWindow(dpy, ico_win);
		return -1;
	}
	
	if (! XStringListToTextProperty(&app_name, 1, &ico_name)) {
		printf("error: can't get ico_name\n");
		XFree(size_hints);
		XFree(wm_hints);
		XFree(class_hints);
		XFree(win_name.value);
		XDestroyWindow(dpy, ico_win);
		return -1;
	}
	
	wm_hints->flags	= StateHint | InputHint;
	wm_hints->initial_state	= NormalState;
	wm_hints->input	= True;
	
	class_hints->res_name = app_name;
	/*Only for docker-ng plugin*/
	class_hints->res_class = "WM_DOCK_PLUG";
	
	XSetWMProperties(dpy, ico_win, &win_name, &ico_name, argv, argc, \
					 size_hints, wm_hints, class_hints);
	
	XSelectInput(dpy, ico_win, TRAYICON_WIN_EVENT_MASK);
	XGetWindowAttributes(dpy, ico_win, &ico_attr);
	
	_gc = XCreateGC(dpy, ico_win, valuemask, &values);	
	
	ret = 0;
	if(cb_win) {
		ret = cb_win(dpy, ico_win, _gc, &ico_attr);
		if(ret != -1) {
			if(ico_attr.width > 0 && ico_attr.width > ico_width && ico_attr.height > 0)
				XMoveResizeWindow(dpy, ico_win, 0, 0, ico_attr.width, ico_attr.height);
		}
	}
	
	XFree( win_name.value );
	XFree( ico_name.value );
	
	XFree( size_hints );
	XFree( wm_hints );
	XFree( class_hints );
	
	return ret;
}

/* Create tray icon and try embed to dock */
int create_trayicon(int ico_width, int ico_height, char *app_name, \
					int argc, char **argv, onwindow_cb cb_win, \
					onatom_cb onatom)
{
	if(init_dpy(NULL) == -1) {
		printf("error: init_dpy failed\n");
		return -1;
	}
	
	get_atoms(dpy, scr, onatom);
	
	if(create_icon_window(ico_width, ico_height, app_name, \
						  argc, argv, cb_win) == -1) {
		return -1;
	}

	if (get_dock(dpy, &dock_win)) {
		XSelectInput(dpy, root_win, TRAYICON_ROOT_EVENT_MASK);
		XSelectInput(dpy, dock_win, StructureNotifyMask | PropertyChangeMask);
		tray_embed_request(dpy, ico_win);
		return 0;
	}
	
	return -1;
}

void set_win_event_mask(long mask)
{
	TRAYICON_WIN_EVENT_MASK = mask;
}
