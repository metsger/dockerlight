#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/epoll.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "dwin.h"
#include "list.h"
#include "version.h"

Atom atom_XKB_RULES_NAMES;

int xkbOpcode;
int xkbEventType;
int xkbError;
int group, locked_group;
char *layout_name = NULL;
int layout_num = 0;
unsigned long bgcolor;

char *fontname = "-*-fixed-bold-r-*-*-18-*-*-*-*-*-*-*";
XFontStruct	*font_info;

char *bg = NULL;
char *fg = NULL;

XColor _bgcolor, _fgcolor;
Colormap colors;

pList layouts;

void clear_layouts_names()
{
	if (layout_num) {
		empty_list(layouts);
		layout_num = 0;
	}
}

char *get_layout_name(int group)
{
	pNode _node = NULL;
	
	if(group+1 <= layout_num) {
		_node = get_node_by_index(layouts, group);
	}
	
	return _node->elem;
}

void get_layouts()
{
	XTextProperty prop;
	char *lay, *lay_iter, c;
	int i, n, len;
	
	XGetTextProperty(dpy, root_win, &prop, atom_XKB_RULES_NAMES);
	lay = (char *)prop.value;
	
	for (i=0; i<2; i++)
		while(*lay++);
	
	clear_layouts_names();
	lay_iter = lay;
	
	if (*lay_iter )
		layout_num = 1;
	
	while((c = *lay_iter++))
		if(c == ',')
			layout_num++;
	
	if (layout_num) {
		for (i=0; i<layout_num; i++) {
			lay_iter = lay;
			len = 0;
			while((c = *lay_iter++))
				if ((c >= 'A') && (c <= 'z'))
					len++;
				else
					break;
			if ((c != ',') && (c != 0))
				while ((c = *lay_iter++))
					if (c == ',')
						break;
			if(len) {
				char *lay_name;
				lay_name = (char *)strndup(lay, len);
				for(n=0; n<len; n++)
					lay_name[n] = toupper(lay_name[n]);
				add_list_node(layouts, lay_name, NULL);
			}
					
			lay = lay_iter;
		}
	}
	XFree(prop.value);
}

int catch_layouts()
{
	int ret;

	ret = XkbQueryExtension(dpy, &xkbOpcode, &xkbEventType, \
							&xkbError, NULL, NULL);					  
	
	if(!ret) {
		printf("error: query xkb\n");
		return -1;
	}
	
	layouts = new_list();
	locked_group = 0;
	XkbLockGroup(dpy, XkbUseCoreKbd, locked_group);
	get_layouts();
	layout_name = get_layout_name(locked_group);
	
	XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify, \
						  XkbAllStateComponentsMask, XkbGroupStateMask);
    
	return 0;
}

void _onatom()
{
	atom_XKB_RULES_NAMES = XInternAtom(dpy, "_XKB_RULES_NAMES", True);
}

int _onwindow(Display *dpy, Window win, GC win_gc, XWindowAttributes *attr)
{
	int ret;
	
	if(catch_layouts() == -1)
		return -1;
	
	font_info = XLoadQueryFont(dpy, fontname);
	
	if (!font_info)	{
		fprintf(stderr, "cannot open \"%s\" font\n", fontname );
		return -1;
	}
	
	XSetFont(dpy, win_gc, font_info->fid);
	
	ret = 0;
	if(fg) {
		colors = DefaultColormap(dpy, scr);
		if (XParseColor(dpy, colors, fg, &_fgcolor) == 0) 
			fprintf(stderr, "Bad Parse\n");
		else
			if ((ret = XAllocColor(dpy, colors, &_fgcolor)) == 0)
			fprintf(stderr, "Bad Color\n");
	}
  
	if(ret == 0)
		XSetForeground(dpy, win_gc, WhitePixel(dpy, scr));
	else
		XSetForeground(dpy, win_gc, _fgcolor.pixel);	
	
	ret = 0;
	
	if(bg) {
		if(!colors)
			colors = DefaultColormap(dpy, scr);
			
		if (XParseColor(dpy, colors, bg, &_bgcolor) == 0) 
			fprintf(stderr, "Bad Parse\n");
		else
			if ((ret = XAllocColor(dpy, colors, &_bgcolor)) == 0)
			fprintf(stderr, "Bad Color\n");
	}
  
	if(ret != 0)
		XSetWindowBackground(dpy, win, _bgcolor.pixel);
	
	catch_layouts();
	attr->width = XTextWidth(font_info, layout_name, strlen(layout_name));
	attr->height = font_info->ascent + font_info->descent;
	printf("%i\n", attr->height);
	
	return 0;
}

void _redraw(Display *dpy, Window win, GC win_gc)
{
	int x,y;
	
	x = (ico_attr.width - XTextWidth(font_info, layout_name, strlen(layout_name)))/2L;
	y = ico_attr.height - 2 - (ico_attr.height - font_info->ascent - font_info->descent)/2L;
	
	XDrawString(dpy, win, win_gc, (x < 0 ? 0 : x), (y < 0 ? 0 : y), layout_name, strlen(layout_name));
}

int app_process_events()
{
	int xd_fd, ep_fd, nfds, n;
	struct epoll_event ev, events[1];
	XEvent ev_x;
	XkbEvent *ev_kb = NULL;
	pid_t pID;
	
	xd_fd = ConnectionNumber(dpy);
	ep_fd = epoll_create1(0);
	
	if (ep_fd == -1) {
		perror("epoll_create");
		return -1;
	}
	
	ev.events = EPOLLIN;
	ev.data.fd = xd_fd;
	
	if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, xd_fd, &ev) == -1) {
		perror("epoll_ctl");
		return -1;
	}
	
	pID = fork();
	
	if (pID < 0) {
		printf("error:Fork failed\n");
		exit(-1);
	}
	else if (pID > 0) {
		exit(0);
	}
	
	while(1) {
		nfds = epoll_wait(ep_fd, events, 1, -1);
		
		if (nfds == -1) {
			perror("epoll_pwait");
			return -1;
		}
		
		if(nfds == 0) {
			continue;
		}
						
        n = XPending (dpy);
		
        while (n--) {
                XNextEvent (dpy, &ev_x);
				if(ev_x.type == xkbEventType) {
					ev_kb = (XkbEvent*) &ev_x;
					if (ev_kb->type == xkbEventType &&  \
						ev_kb->any.xkb_type == XkbStateNotify) {
							locked_group = ev_kb->state.locked_group;
							group = ev_kb->state.group;
							layout_name = get_layout_name(locked_group);
							tray_redraw(dpy, ico_win, _gc, _redraw);
					}
					else if (ev_kb->type == PropertyNotify &&  \
							 ev_kb->core.xproperty.atom == atom_XKB_RULES_NAMES) {
							get_layouts();
							layout_name = get_layout_name(locked_group);
							tray_redraw(dpy, ico_win, _gc, _redraw);
					}
				}
				else
				switch (ev_x.type) {
					case Expose: {
						tray_redraw(dpy, ico_win, _gc, _redraw);
						break;
					}
					case ConfigureNotify: {
						if (ev_x.xproperty.window == ico_win) {
							ico_attr.width = ((XConfigureEvent*) &ev_x)->width;
							ico_attr.height = ((XConfigureEvent*) &ev_x)->height;
						}
						break;
					}
					case ButtonPress: {
						if (++locked_group == layout_num)
							locked_group = 0;
						XkbLockGroup(dpy, XkbUseCoreKbd, locked_group);
						break;
					}
					/*default: printf("Other event type: %i\n", ev_x.type);*/
				}
		}
	}
	
	return 0;
}

void parse_cmd_line(int argc, char **argv)
{
	int i;
	int help = 0;
	
	for (i = 1; i < argc; i++) {
		if (0 == strcasecmp(argv[i], "-font")) {
		++i;
			if (i < argc) {
				fontname = argv[i];
			} else { /* argument doesn't exist */
				printf("-font requires a parameter\n");
				help = 1;
			}
		} else if (0 == strcasecmp(argv[i], "-bg")) {
			++i;
			if (i < argc) {
				bg = argv[i];
			} else { /* argument doesn't exist */
			printf("-bg requires a parameter\n");
			help = 1;
			}
		} else if (0 == strcasecmp(argv[i], "-fg")) {
			++i;
			if (i < argc) {
				fg = argv[i];
			} else { /* argument doesn't exist */
			printf("-fg requires a parameter\n");
			help = 1;
			}
		} else {
			if (argv[i][0] == '-')
			help = 1;
		}

	  if (help) {
	  printf("%s - version %s\n", argv[0], VERSION);
	  printf("Usage: %s [OPTIONS]\n\n", argv[0]);
	  printf("Options:\n");
	  printf("  -help			 Show this help.\n");
	  printf("  -font <font>	 Customize font. Use Xfonts params string\n");
	  printf("					 Default is fixed bold 18\n");
	  printf("  -fg <color>	     Font color from rgb.txt\n");
	  printf("					 Default is white\n");
	  printf("  -bg <color> 	 Background color from rgb.txt.\n");
	  printf("					 Default is parent\n");
	  exit(1);
	}
  }
}

int main(int argc, char **argv)
{
	parse_cmd_line(argc, argv);
		
	set_root_event_mask(StructureNotifyMask);
	set_win_event_mask(PropertyNotify | StructureNotifyMask | ExposureMask | ButtonPressMask);
	
	if(create_trayicon(24, 24, argv[0], argc, argv, _onwindow, _onatom) == -1) {
		printf("error: create_trayicon filed\n");
		return -3;
	}
	
	if (app_process_events() == -1) {
		printf("error: app_process_events filed\n");
		XCloseDisplay(dpy);
		return -1;
	}
	
	XCloseDisplay(dpy);
	
	return 0;
}
