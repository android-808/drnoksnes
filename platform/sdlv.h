#ifndef _PLATFORM_SDLV_H_
#define _PLATFORM_SDLV_H_

#include <SDL.h>

extern SDL_Surface* screen;

#if CONF_HD
#	include <SDL_syswm.h>
#	include <X11/Xatom.h>
#	define HDATOM(X) hdAtomsValues[ ATOM ## X ]

enum hdAtoms {
	ATOM_HILDON_NON_COMPOSITED_WINDOW = 0,
	ATOM_HILDON_STACKABLE_WINDOW,
	ATOM_NET_WM_STATE,
	ATOM_NET_WM_STATE_FULLSCREEN,
	ATOM_NET_WM_WINDOW_TYPE,
	ATOM_NET_WM_WINDOW_TYPE_NORMAL,
	ATOM_NET_WM_WINDOW_TYPE_DIALOG,
	ATOM_HILDON_WM_WINDOW_TYPE_ANIMATION_ACTOR,
	ATOM_HILDON_ANIMATION_CLIENT_READY,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_SHOW,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_POSITION,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_ROTATION,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_SCALE,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_ANCHOR,
	ATOM_HILDON_ANIMATION_CLIENT_MESSAGE_PARENT,
	ATOM_HILDON_WM_WINDOW_TYPE_REMOTE_TEXTURE,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_SHM,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_DAMAGE,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_SHOW,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_POSITION,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_OFFSET,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_SCALE,
	ATOM_HILDON_TEXTURE_CLIENT_MESSAGE_PARENT,
	ATOM_HILDON_TEXTURE_CLIENT_READY,
	ATOM_COUNT
};

extern Atom hdAtomsValues[];

extern SDL_SysWMinfo WMinfo;

extern void hd_setup();
extern void hd_set_non_compositing(bool enable);

#endif

#if defined(MAEMO) && MAEMO_VERSION >= 5
extern void exitReset();
extern bool exitRequiresDraw();
extern void exitDraw(SDL_Surface* where);
#endif

#endif