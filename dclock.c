#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/epoll.h>

#include "dwin.h"
#include "version.h"

time_t rawtime;
char _stime[6] = "00:00";
char _sdate[16];

char *fontname = "-*-fixed-bold-r-*-*-18-*-*-*-*-*-*-*";
XFontStruct	*font_info;

char *bg = NULL;
char *fg = NULL;

XColor _bgcolor, _fgcolor;
Colormap colors;

int _onwindow(Display *dpy, Window win, GC win_gc, XWindowAttributes *attr)
{
	int ret;
	
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
	
	attr->width = XTextWidth(font_info, _stime, strlen(_stime));
	attr->height = font_info->ascent + font_info->descent;

	return 0;
}

void redraw(Display *dpy, Window win, GC win_gc)
{
	int x,y;

	x = (ico_attr.width - XTextWidth(font_info, _stime, strlen(_stime)))/2L;
	y = ico_attr.height - 2 - (ico_attr.height - font_info->ascent - font_info->descent)/2L;

	XDrawString(dpy, win, win_gc, (x < 0 ? 0 : x), (y < 0 ? 0 : y), _stime, strlen(_stime));
}

void update_timer(int *timer)
{
	struct tm *_tm = localtime(&rawtime);
	int _hour = _tm->tm_hour;
	int _minute = _tm->tm_min;

	time(&rawtime);
	_tm = localtime(&rawtime);
	if(timer)
		*timer = 60000 - _tm->tm_sec * 1000;

	if((_tm->tm_min != _minute) || (_tm->tm_hour != _hour)) {
		strftime(_stime, 6, "%H:%M0", _tm);
		tray_redraw(dpy, ico_win, _gc, redraw);
	}
}

int app_process_events()
{
	int xd_fd, ep_fd, nfds, timer, n;
	struct epoll_event ev, events[1];
	XEvent ev_x;
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

	update_timer(&timer);

	pID = fork();

	if (pID < 0) {
		printf("error:Fork failed\n");
		exit(-1);
	}
	else if (pID > 0) {
		exit(0);
	}

	while(1) {
		nfds = epoll_wait(ep_fd, events, 1, timer);

		if (nfds == -1) {
			perror("epoll_pwait");
			return -1;
		}

		if(nfds == 0) {
			update_timer(&timer);
			continue;
		}

		n = XPending (dpy);

		while (n--) {
				XNextEvent (dpy, &ev_x);

				switch (ev_x.type) {
					case Expose: {
						tray_redraw(dpy, ico_win, _gc, redraw);
						break;
					}
					case ConfigureNotify: {
						if (ev_x.xproperty.window == ico_win) {
							ico_attr.width = ((XConfigureEvent*) &ev_x)->width;
							ico_attr.height = ((XConfigureEvent*) &ev_x)->height;
						}
						break;
					}
					/*default: printf("Other event type: %i\n", ev_x.type);*/
				}

		}
		update_timer(&timer);
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
			printf("-bg requires a parameter\n");
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
	  printf("  -font <font>	  Customize font. Use Xfonts params string\n");
	  printf("					Default is fixed bold 18\n");
	  printf("  -fg <color>	   Font color from rgb.txt\n");
	  printf("					Default is white\n");
	  printf("  -bg <color> 	  Background color from rgb.txt.\n");
	  printf("					Default is parent\n");
	  exit(1);
	}
  }
}

int main(int argc, char **argv)
{
	parse_cmd_line(argc, argv);

	set_root_event_mask(StructureNotifyMask);
	set_win_event_mask(StructureNotifyMask | ExposureMask);

	if(create_trayicon(24, 24, argv[0], argc, argv, _onwindow, NULL) == -1) {
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
