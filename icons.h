#ifndef __icons_h
#define __icons_h

#include <X11/Xlib.h>
#include "docker.h"

extern int error;

int icon_add(Window id);
void icon_remove(pNode node);

#endif /* __icons_h */
