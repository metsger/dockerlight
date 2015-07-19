#include <stdio.h>
#include <string.h>
#include "dtray.h"

void get_atoms(Display *dpy, int scr, onatom_cb onatom)
{
	char buf[32];
	
	snprintf(buf, sizeof(buf), "_NET_SYSTEM_TRAY_S%d", scr);
	_atom_NET_SYSTEM_TRAY_S = XInternAtom(dpy, buf, False);
	_atom_MANAGER = XInternAtom(dpy, "MANAGER", False);
	_atom_NET_SYSTEM_TRAY_OPCODE = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	_atom_NET_SYSTEM_TRAY_MESSAGE_DATA = XInternAtom(dpy, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
	if(onatom) {
		onatom();
	}
}

int get_dock(Display *dpy, Window *dock)
{	
	XGrabServer(dpy);
	*dock = XGetSelectionOwner(dpy, _atom_NET_SYSTEM_TRAY_S);
	XUngrabServer(dpy);
	XFlush(dpy);
	
	return (*dock != None);
}

int tray_send_message(Display *dpy, Window win, long msg_op, long data1, long data2, long data3)
{
	XEvent ev;

	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = _atom_NET_SYSTEM_TRAY_OPCODE;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = msg_op;
	ev.xclient.data.l[2] = data1;
	ev.xclient.data.l[3] = data2;
	ev.xclient.data.l[4] = data3;

	XSendEvent(dpy, dock_win, False, NoEventMask, &ev);	
	XSync(dpy, False );

	return 0;
}

void tray_embed_request(Display *dpy, Window ico_win)
{
	tray_send_message(dpy, dock_win, SYSTEM_TRAY_REQUEST_DOCK, ico_win, 0, 0);
}

void tray_redraw(Display *dpy, Window ico_win, GC win_gc, redraw_cb redraw)
{
	XClearWindow(dpy, ico_win);
	redraw(dpy, ico_win,  win_gc);
	XFlush(dpy);
}

void set_root_event_mask(long mask)
{
	TRAYICON_ROOT_EVENT_MASK = mask;
}
