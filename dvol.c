#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <X11/xpm.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include "dwin.h"
#include "version.h"

Atom atom_XKB_RULES_NAMES;

unsigned long bgcolor;

char *bg = NULL;
char *ico_path = NULL;

XColor _bgcolor;
Colormap colors;

typedef struct _XpmIcon {
	XImage *img;
	XImage *mask;
	Pixmap pmask;
	XpmAttributes attributes;
} XpmIcon;

XpmIcon icons[8];

int ico_n, mute;

char *m_elem = "Master";
char	card_id[64] = "hw:0";

snd_mixer_t *mixer_handle;
snd_mixer_selem_id_t *sid;
snd_mixer_elem_t *velem;

struct {
	long max_vol;
	long min_vol;
	long vol;
} vol;

long mute_level = 0;

void check_volbar_state();

int load_icons()
{
	int i,ret;
	GC tgc;
	
	for(i=0;i<8;i++) {
		char fn[FILENAME_MAX];
		sprintf(fn, "%s%d.xpm", ico_path, i+1);
		printf("try %s\n", fn);
		ret = XpmReadFileToImage(dpy, fn, &icons[i].img, &icons[i].mask, NULL);
		if(!ret && icons[i].img) {
			icons[i].pmask = XCreatePixmap(dpy, root_win, \
										(icons[i].mask)->width, \
										(icons[i].mask)->height, \
										(icons[i].mask)->depth);
			
			tgc = XCreateGC (dpy, icons[i].pmask, 0, NULL);
			
			XPutImage(dpy, icons[i].pmask, tgc, icons[i].mask, 0, 0, 0, 0, \
					  (icons[i].mask)->width, (icons[i].mask)->height);
			
			XFree(tgc);
		}
		else {
			return -1;
		}
	}
	
	return 0;
}

int _onwindow(Display *dpy, Window win, GC win_gc, XWindowAttributes *attr)
{
	int ret;
		
	ret = 0;
	
	mute = 0;
	
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
	
	if(load_icons() == -1)
		return -1;
	
	ico_n = 0;
	
	check_volbar_state();
		
	return 0;
}

void _redraw(Display *dpy, Window win, GC win_gc)
{	
	XSetClipMask(dpy, win_gc, icons[ico_n].pmask);
	XSetClipOrigin(dpy, win_gc, 0, 0);
	XPutImage(dpy, win, win_gc, icons[ico_n].img, 0, 0, 0, 0, ico_attr.width, ico_attr.height);
}

void check_volbar_state()
{	
	if(vol.vol != vol.min_vol) {
		long cstage;
		
		cstage = ceil(vol.vol / ((vol.max_vol-vol.min_vol)/7.0));
		mute = 0;
		
		if(ico_n == cstage)
			return ;
		
		ico_n = cstage;
	}
	else
		ico_n = 0;
	
	/*printf("vol: %lu, ico: %i\n", vol.vol, ico_n);*/
	tray_redraw(dpy, ico_win, _gc, _redraw);
}

int mixer_init(char card[64])
{
	snd_mixer_elem_t *elem;
	int err;
	
	snd_mixer_selem_id_alloca(&sid);

	err = snd_mixer_open (&mixer_handle, 0);
	err = snd_mixer_attach (mixer_handle, card);
	err = snd_mixer_selem_register (mixer_handle, NULL, NULL);
	err = snd_mixer_load (mixer_handle);
	
	for (elem = snd_mixer_first_elem(mixer_handle); elem; elem = snd_mixer_elem_next(elem)) {
		snd_mixer_selem_get_id(elem, sid);

		if (!snd_mixer_selem_is_active(elem) || !snd_mixer_selem_has_playback_volume(elem))
			continue;
		
		if (strcmp(snd_mixer_selem_get_name(elem), m_elem) == 0) {
			velem = elem;
			return 0;
			break;
		} 
	}
	
	return -1;
}

static int mixer_callback(snd_mixer_elem_t *elem, unsigned int mask) {
		   
    if (mask == SND_CTL_EVENT_MASK_REMOVE)
        return 0;

    if (mask & SND_CTL_EVENT_MASK_VALUE) {
		snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol.vol);
		check_volbar_state();
    }
    
    if((mute == 0) && (mute_level != 0))
		mute_level = 0;

    return 0;
}

int app_process_events()
{
	int count;
	int xd_fd, ep_fd, nfds, n;
	struct epoll_event ev, alsa_ev, events[1];
	XEvent ev_x;
	pid_t pID;
	struct pollfd *alsa_fds;
	int vol_inc;
	
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
	
	if ((count = snd_mixer_poll_descriptors_count(mixer_handle)) < 0) {
		perror("snd_mixer_poll_descriptors_count");
		return -1;
	}
	else {
		alsa_fds = calloc (count, sizeof(struct pollfd));

		if (snd_mixer_poll_descriptors(mixer_handle, alsa_fds, count) < 0) {
			perror("snd_mixer_poll_descriptors");
			return -2;
		}
		
		alsa_ev.events = EPOLLIN;
		alsa_ev.data.fd = alsa_fds->fd;
		
		if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, alsa_fds->fd, &alsa_ev) == -1) {
			perror("epoll_ctl");
			return -1;
		}
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
		
		if(events[0].data.fd == alsa_fds->fd) {
			if (snd_mixer_handle_events(mixer_handle) < 0) {
				perror("snd_mixer_handle_events");
				return -3;
			}
			continue;
		}
						
        n = XPending (dpy);
		
        while (n--) {
                XNextEvent (dpy, &ev_x);
				
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
						if(ev_x.xbutton.button == 4) {
							if(vol.vol < vol.max_vol) {
								vol_inc = (vol.max_vol-vol.vol)>4 ? 5 : vol.max_vol-vol.vol;
								if(snd_mixer_selem_set_playback_volume(velem, SND_MIXER_SCHN_FRONT_LEFT, vol.vol += vol_inc) != 0) {
									perror("snd_mixer_selem_set_playback_volume");
									return -4;
								}
								mute = 0;
							}
						}
						else if(ev_x.xbutton.button == 5) {
							if(vol.vol > vol.min_vol) {
								vol_inc = (vol.vol-vol.max_vol)>4 ? 5 : vol.vol-vol.max_vol;
								if(snd_mixer_selem_set_playback_volume(velem, SND_MIXER_SCHN_FRONT_LEFT, vol.vol -= 5) != 0) {
									perror("snd_mixer_selem_set_playback_volume");
									return -4;
								}
								mute = 0;
							}
						}
						else if(ev_x.xbutton.button == 3) {
							system("lamixer&");
						}
						else if(ev_x.xbutton.button == 1) {
							if((mute == 0) && (vol.vol > vol.min_vol)) {
								mute_level = vol.vol;
								if(snd_mixer_selem_set_playback_volume(velem, SND_MIXER_SCHN_FRONT_LEFT, vol.min_vol) != 0) {
									perror("snd_mixer_selem_set_playback_volume");
									return -4;
								}
								vol.vol = vol.min_vol;
								mute = 1;
							}
							else {
								if(snd_mixer_selem_set_playback_volume(velem, SND_MIXER_SCHN_FRONT_LEFT, mute_level) != 0) {
									perror("snd_mixer_selem_set_playback_volume");
									return -4;
								}
								vol.vol = mute_level;
								mute = 0;
							}
						}
						else {
							break;
						}
							
						check_volbar_state();
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
		if (0 == strcasecmp(argv[i], "-bg")) {
			++i;
			if (i < argc) {
				bg = argv[i];
			} else { /* argument doesn't exist */
			printf("-bg requires a parameter\n");
			help = 1;
			}
		} else if (0 == strcasecmp(argv[i], "-icons")) {
			++i;
			if (i < argc) {
				ico_path = argv[i];
			} else { /* argument doesn't exist */
			printf("-icons requires a parameter\n");
			help = 1;
			}
		} else if (0 == strcasecmp(argv[i], "-card")) {
			++i;
			if (i < argc) {
				sprintf(card_id,"hw:%s", argv[i]);
			} else { /* argument doesn't exist */
			printf("-card requires a parameter\n");
			help = 1;
			}
		} else if (0 == strcasecmp(argv[i], "-master")) {
			++i;
			if (i < argc) {
				m_elem = argv[i];
			} else { /* argument doesn't exist */
			printf("-master requires a parameter\n");
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
	  printf("  -icons <path>	 XPM Icons dir path\n");
	  printf("					 Default is white\n");
	  printf("  -bg <color> 	 Background color from rgb.txt.\n");
	  printf("					 Default is parent\n");
	  printf("  -cardid <id> 	 Alsa card id, default 0\n");
	  printf("  -master <name> 	 In-tray volume element, default Master\n");
	  exit(1);
	}
  }
}

int main(int argc, char **argv)
{
	parse_cmd_line(argc, argv);
		
	set_root_event_mask(StructureNotifyMask);
	set_win_event_mask(PropertyNotify | StructureNotifyMask | ExposureMask | ButtonPressMask);
	
	if(mixer_init(card_id) == -1) {
		printf("error: mixer_init filed\n");
		return -4;
	}
	
	vol.vol = vol.max_vol = vol.min_vol = 0;
	
	snd_mixer_selem_get_playback_volume_range(velem, &vol.min_vol, &vol.max_vol);
	snd_mixer_selem_get_playback_volume(velem, SND_MIXER_SCHN_FRONT_LEFT, &vol.vol);
	snd_mixer_elem_set_callback(velem, mixer_callback);
	
	if(create_trayicon(24, 24, argv[0], argc, argv, _onwindow, NULL) == -1) {
		printf("error: create_trayicon filed\n");
		return -3;
	}
	
	if (app_process_events() == -1) {
		printf("error: app_process_events filed\n");
		XCloseDisplay(dpy);
		return -1;
	}
	
	snd_mixer_detach(mixer_handle, card_id);
	snd_mixer_close(mixer_handle);
	
	XCloseDisplay(dpy);
	
	return 0;
}
