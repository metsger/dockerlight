#ifndef __docker_h
#define __docker_h

#include <X11/Xlib.h>
#include "list.h"

extern Display *display;
extern Window root, win;
extern pList icons;
extern int width, height;
extern int border;
extern int horizontal;
extern int icon_size;

typedef struct
{
  Window id;
  int wmplug;
  int x, y, w, h;
} TrayWindow;

void reposition_icons();
void fix_geometry();
pNode get_firts_wmplug();

#endif /* __docker_h */
