#include "version.h"
#include "icons.h"
#include "docker.h"
#include "net.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xutil.h>

#include <sys/epoll.h>
#include <stdio.h>

int argc;
char **argv;

Window win = None, hint_win = None, root = None;
Display *display = NULL;
pList icons = NULL;
int width = 0, height = 0;
int border = 1; /* blank area around icons. must be > 0 */
int horizontal = 1; /* layout direction */
int icon_size = 24; /* width and height of systray icons */
XColor _bgcolor;
char *bg = NULL;

Colormap colors;

static char *display_string = NULL;
/* excluding the border. sum of all child apps */
static int exit_app = 0;

void signal_handler(int signal)
{
  switch (signal)
  {
  case SIGSEGV:
  case SIGFPE:
    printf("Segmentation Fault or Critical Error encountered.\n");
    abort();
    
  case SIGPIPE:
  case SIGTERM:
  case SIGINT:
  case SIGHUP:
    exit_app = 1;
  }
}


void parse_cmd_line()
{
  int i;
  int help = 0;
  
  for (i = 1; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-display")) {
      ++i;
      if (i < argc) {
        display_string = argv[i];
      } else { /* argument doesn't exist */
        printf("-display requires a parameter\n");
        help = 1;
      }
    } else if (0 == strcasecmp(argv[i], "-vertical")) {
      horizontal = 0;
    } else if (0 == strcasecmp(argv[i], "-bg")) {
		++i;
        if (i < argc) {
          bg = argv[i];
        } else { /* argument doesn't exist */
          printf("-bg requires a parameter\n");
          help = 1;
	    }
    } else if (0 == strcasecmp(argv[i], "-border")) {
      ++i;

      if (i < argc) {
        int b = atoi(argv[i]);
        if (b > 0) {
          border = b;
        } else {
          printf("-border must be a value greater than 0\n");
          help = 1;
        }
      } else { /* argument doesn't exist */
        printf("-border requires a parameter\n");
        help = 1;
      }
      } else if (0 == strcasecmp(argv[i], "-iconsize")) {
      ++i;
      if (i < argc) {
        int s = atoi(argv[i]);
        if (s > 0) {
          icon_size = s;
        } else {
          printf("-iconsize must be a value greater than 0\n");
          help = 1;
        }
      } else { /* argument doesn't exist */
        printf("-iconsize requires a parameter\n");
        help = 1;
      }
    } else {
      if (argv[i][0] == '-')
        help = 1;
    }


    if (help) {
      /* show usage help */
      printf("%s - version %s\n", argv[0], VERSION);
      printf("Copyright 2003 Ben Jansens <ben@orodu.net>\n\n");
      printf("Usage: %s [OPTIONS]\n\n", argv[0]);
      printf("Options:\n");
      printf("  -help             Show this help.\n");
      printf("  -display DISLPAY  The X display to connect to.\n");
      printf("  -border           The width of the border to put around the\n"
              "                    system tray icons. Defaults to 1.\n");
      printf("  -vertical         Line up the icons vertically. Defaults to\n"
              "                    horizontally.\n");
      printf("  -iconsize SIZE    The size (width and height) to display\n"
              "                    icons as in the system tray. Defaults to\n"
              "                    24.\n");
      printf("  -bg <color> 	  The background color for dock.\n"
			 "                       Default as for parent, custom example ");
      exit(1);
    }
  }
}


void create_hint_win()
{
  XWMHints hints;
  XClassHint classhints;

  hint_win = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
  assert(hint_win);

  hints.flags = StateHint | WindowGroupHint | IconWindowHint;
  hints.initial_state = WithdrawnState;
  hints.window_group = hint_win;
  hints.icon_window = win;

  classhints.res_name = "docker";
  classhints.res_class = "Docker";

  XSetWMProperties(display, hint_win, NULL, NULL, argv, argc,
                   NULL, &hints, &classhints);

  XMapWindow(display, hint_win);
}


void create_main_window()
{
  XWMHints hints;
  XTextProperty text;
  char *name = "Docker";
  int ret;

  assert(border > 0);

  win = XCreateSimpleWindow(display, root, 0, 0,
                              border * 2, border * 2+icon_size, 0, 0, 0);   
  assert(win);

  XStringListToTextProperty(&name, 1, &text);
  XSetWMName(display, win, &text);

  hints.flags = StateHint;
  hints.initial_state = WithdrawnState;
  XSetWMHints(display, win, &hints);
  
  create_hint_win();
  
  XSync(display, False);
  
  ret = 0;
  
  if(bg) {
	  colors = DefaultColormap(display, DefaultScreen(display));
	  if (XParseColor(display, colors, bg, &_bgcolor) == 0) 
		fprintf(stderr, "Bad Parse\n");
	  else
	    if ((ret = XAllocColor(display, colors, &_bgcolor)) == 0)
		  fprintf(stderr, "Bad Color\n");
	  
  }
  
  if(ret == 0)
	XSetWindowBackgroundPixmap(display, win, ParentRelative);
  else
    XSetWindowBackground(display, win, _bgcolor.pixel);

  XClearWindow(display, win);
}



/*если одновременно убиваются несколько аплетов 
* то крашится из-за последовательной обработки
* нужна переработка диспетчера событий*/
void reposition_icons() 
{
  int x = border, y = border + ((height % icon_size) / 2);
  
  pNode it;
  
  for (it = icons->first; it != NULL; it = it->next) {
    ((TrayWindow*)it->elem)->x = x;
    ((TrayWindow*)it->elem)->y = y;
    XMoveWindow(display, ((TrayWindow*)it->elem)->id, x, y);
    XSync(display, False);
    if (horizontal)
      x += ((TrayWindow*)it->elem)->w/* + border*/;
    else
      y += icon_size;
  }
}


void fix_geometry()
{
  pNode it;
 
  /* find the proper width and height */
  width = horizontal ? 0 : icon_size;
  height = horizontal ? icon_size : 0;
  for (it = icons->first; it != NULL; it = it->next) {
    if (horizontal)
      width += ((TrayWindow*)it->elem)->w;
    else
      height += icon_size;
  }

  XResizeWindow(display, win, width + border * 2, height + border * 2);
}


void event_loop()
{
  XEvent e;
  Window cover;
  pNode it;
  int xd_fd, ep_fd, nfds;
  struct epoll_event ev, events[1];
  
  xd_fd = ConnectionNumber(display);
  ep_fd = epoll_create1(0);
  if (ep_fd == -1) {
	  perror("epoll_create");
	  return;
  }
  
  ev.events = EPOLLIN;
  ev.data.fd = xd_fd;
  
  if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, xd_fd, &ev) == -1) {
	  perror("epoll_ctl");
	  return;
  }
  
  icons = new_list();
  
  pid_t pID = fork();
  
  if (pID < 0) {
        printf("error:Fork failed\n");
        exit(-1);
  }
  else if (pID > 0) {
      exit(0);
  }

  while (!exit_app) {
	  nfds = epoll_wait(ep_fd, events, 1, -1);
	  
	  if (nfds == -1) {
		  perror("epoll_pwait");
		  break;
	  }
	  
	  if(nfds == 0) {
		  continue;
	  }

	while (XPending(display)) {
      XNextEvent(display, &e);

      switch (e.type)
      {
      case PropertyNotify:
        /* systray window list has changed? */
/*        if (e.xproperty.atom == kde_systray_prop) {
          XSelectInput(display, win, NoEventMask);
          kde_update_icons();
          XSelectInput(display, win, StructureNotifyMask);

          while (XCheckTypedEvent(display, PropertyNotify, &e));
        }*/

        break;

      case ConfigureNotify:
        if (e.xany.window != win) {
          /* find the icon it pertains to and beat it into submission */
		  for (it = icons->first; it != NULL; it = it->next) {
            if (((TrayWindow*)it->elem)->id == e.xany.window) {
              XMoveResizeWindow(display, ((TrayWindow*)it->elem)->id, 
										 ((TrayWindow*)it->elem)->x, 
										 ((TrayWindow*)it->elem)->y,
										 ((TrayWindow*)it->elem)->w, 
										 icon_size);
              break;
            }
          }
          break;
        }
        
        /* briefly cover the entire containing window, which causes it and
           all of the icons to refresh their windows. finally, they update
           themselves when the background of the main window's parent changes.
        */
        cover = XCreateSimpleWindow(display, win, 0, 0,
                                    border * 2 + width, border * 2 + height,
                                    0, 0, 0);
        XMapWindow(display, cover);
        XDestroyWindow(display, cover);
        
        break;

      case ReparentNotify:
        if (e.xany.window == win) /* reparented to us */
          break;
      case UnmapNotify:
      case DestroyNotify: /*нужно переработать чтобы не крашилось*/
		it = icons->first;
		while (it) {
          if (((TrayWindow*)it->elem)->id == e.xany.window) {
            icon_remove(it);
            break;
          }
		  it = it->next;
        }
        break;

      case ClientMessage:
        if (e.xclient.message_type == net_opcode_atom &&
            e.xclient.format == 32 &&
            e.xclient.window == net_sel_win)
          net_message(&e.xclient);
        
      default:
        break;
      }
    }
  }
  
  /* remove/unparent all the icons */
  it = icons->first;
  while (it) {
    /* do the remove here explicitly, cuz the event handler isn't going to
       happen anymore. */
    pNode thisNode = it;
	it = it->next;
	icon_remove(thisNode);
  }
  destroy_list(icons);
}

void debug_list() {
	pNode it;
	
	it = icons->first;
	
	printf("DEBUG INFO\n");
	while(it) {
		printf("win: %d\n",(int)((TrayWindow*)it->elem)->id);
		it = it->next;
	}
	printf("DEBUG INFO END\n");
}

pNode get_firts_wmplug() {
	pNode it;
	
	it = icons->first;
	
	if(!it)
		return NULL;
	
	if(it->elem && (((TrayWindow*)it->elem)->wmplug == 1))
		return (void*)icons;
	
	while(it->next) {
		if(((TrayWindow*)it->next->elem)->wmplug == 1) {
			return it;
		}
		it = it->next;
	}
	return NULL;
}

int main(int c, char **v)
{
  struct sigaction act;
  
  argc = c; argv = v;
  
  act.sa_handler = signal_handler;
  act.sa_flags = 0;
  sigaction(SIGSEGV, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);
  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);

  parse_cmd_line(argc, argv);

  display = XOpenDisplay(display_string);
  if (!display) {
    printf("Unable to open Display %s. Exiting.\n",
               DisplayString(display_string));
  }

  root = RootWindow(display, DefaultScreen(display));
  assert(root);
  
  create_main_window();

  net_init();

  /* we want to get ConfigureNotify events, and assume our parent's background
     has changed when we do, so we need to refresh ourself to match */
  XSelectInput(display, win, StructureNotifyMask);
  
  event_loop();

  XCloseDisplay(display);

  return 0;
}
