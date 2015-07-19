#include <X11/Xlib.h>

#define SYSTEM_TRAY_REQUEST_DOCK 0

long TRAYICON_ROOT_EVENT_MASK;

typedef void (*redraw_cb)(Display *dpy, Window win, GC win_gc);
typedef void (*onatom_cb)(void);

Atom _atom_MANAGER;
Atom _atom_NET_SYSTEM_TRAY_S;
Atom _atom_NET_SYSTEM_TRAY_OPCODE;
Atom _atom_NET_SYSTEM_TRAY_MESSAGE_DATA;

Window dock_win;

int get_dock(Display *dpy, Window *dock);
int tray_send_message(Display *dpy, Window win, long msg_op, long data1, long data2, long data3);

void get_atoms(Display *dpy, int scr, onatom_cb onatom);
void tray_embed_request(Display *dpy, Window ico_win);
void tray_redraw(Display *dpy, Window ico_win, GC win_gc, redraw_cb redraw);

void set_root_event_mask(long mask);
