/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scumm.h"
#include "mididrv.h"
#include "SDL_thread.h"
#include "gameDetector.h"

#include "scummvm.xpm"

#include <SDL.h>

#define MAX(a,b) (((a)<(b)) ? (b) : (a))
#define MIN(a,b) (((a)>(b)) ? (b) : (a))

class OSystem_SDL : public OSystem {
public:
	// Set colors of the palette
	void set_palette(const byte *colors, uint start, uint num);

	// Set the size of the video bitmap.
	// Typically, 320x200
	void init_size(uint w, uint h);

	// Draw a bitmap to screen.
	// The screen will not be updated to reflect the new bitmap
	void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h);

	// Update the dirty areas of the screen
	void update_screen();

	// Either show or hide the mouse cursor
	bool show_mouse(bool visible);
	
	// Set the position of the mouse cursor
	void set_mouse_pos(int x, int y);
	
	// Set the bitmap that's used when drawing the cursor.
	void set_mouse_cursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y);
	
	// Shaking is used in SCUMM. Set current shake position.
	void set_shake_pos(int shake_pos);
		
	// Get the number of milliseconds since the program was started.
	uint32 get_msecs();
	
	// Delay for a specified amount of milliseconds
	void delay_msecs(uint msecs);
	
	// Create a thread
	void *create_thread(ThreadProc *proc, void *param);
	
	// Get the next event.
	// Returns true if an event was retrieved.	
	bool poll_event(Event *event);
	
	// Set function that generates samples 
	bool set_sound_proc(void *param, SoundProc *proc, byte sound);
		
	// Poll cdrom status
	// Returns true if cd audio is playing
	bool poll_cdrom();

	// Play cdrom audio track
	void play_cdrom(int track, int num_loops, int start_frame, int end_frame);

	// Stop cdrom audio track
	void stop_cdrom();

	// Update cdrom audio status
	void update_cdrom();

	// Quit
	void quit();

	// Set a parameter
	uint32 property(int param, Property *value);

	// Add a callback timer
	void set_timer(int timer, int (*callback)(int));

	// Mutex handling
	void *create_mutex(void);
	void lock_mutex(void *mutex);
	void unlock_mutex(void *mutex);
	void delete_mutex(void *mutex);

	static OSystem *create(int gfx_mode, bool full_screen);

private:
	typedef void TwoXSaiProc(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);

	SDL_Surface *sdl_screen;      // unseen game screen
	SDL_Surface *sdl_hwscreen;    // hardware screen
	SDL_Surface *sdl_tmpscreen;   // temporary screen (for 2xsai)
	SDL_CD *cdrom;

	enum {
		DF_WANT_RECT_OPTIM			= 1 << 0,
		DF_REAL_8BIT				= 1 << 1,
		DF_SEPARATE_TEMPSCREEN		= 1 << 2,
		DF_UPDATE_EXPAND_1_PIXEL	= 1 << 3
	};

	int _mode;
	bool _full_screen;
	bool _mouse_visible;
	bool _mouse_drawn;
	uint32 _mode_flags;

	bool force_full; //Force full redraw on next update_screen
	bool cksum_valid;

	enum {
		NUM_DIRTY_RECT = 100,

		MAX_MOUSE_W = 40,
		MAX_MOUSE_H = 40,
		MAX_SCALING = 3
	};

	int SCREEN_WIDTH, SCREEN_HEIGHT, CKSUM_NUM;
	SDL_Rect *dirty_rect_list;
	int num_dirty_rects;
	uint32 *dirty_checksums;

	int scaling;

	/* CD Audio */
	int cd_track, cd_num_loops, cd_start_frame, cd_end_frame;
	Uint32 cd_end_time, cd_stop_time, cd_next_second;

	/* Keyboard mouse emulation */
	struct KbdMouse {	
		int16 x, y, xv, yv, xm, ym, xd, yd;
		uint32 last, delay, xdown, ydown;
	} km;

	struct MousePos {
		int16 x,y,w,h;
	};

	byte *_mouse_data;
	byte *_mouse_backup;
	MousePos _mouse_cur_state;
	MousePos _mouse_old_state;
	int16 _mouse_hotspot_x;
	int16 _mouse_hotspot_y;
	int _current_shake_pos;
	int _new_shake_pos;
	TwoXSaiProc *_sai_func;
	SDL_Color *_cur_pal;

	uint _palette_changed_first, _palette_changed_last;

	OSystem_SDL() : _current_shake_pos(0), _new_shake_pos(0) {}

	void add_dirty_rgn_auto(const byte *buf);
	void mk_checksums(const byte *buf);

	static void fill_sound(void *userdata, Uint8 * stream, int len);
	
	void add_dirty_rect(int x, int y, int w, int h);

	void draw_mouse();
	void undraw_mouse();

	void load_gfx_mode();
	void unload_gfx_mode();

	void hotswap_gfx_mode();

	void get_320x200_image(byte *buf);	

	void setup_icon();
	void kbd_mouse();
};

int Init_2xSaI (uint32 BitFormat);
void _2xSaI(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr, uint8 *dstPtr,
						uint32 dstPitch, int width, int height);
void Super2xSaI(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);
void SuperEagle(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);
void AdvMame2x(uint8 *srcPtr, uint32 srcPitch, uint8 *null,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);
void Normal1x(uint8 *srcPtr, uint32 srcPitch, uint8 *null,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);
void Normal2x(uint8 *srcPtr, uint32 srcPitch, uint8 *null,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);
void Normal3x(uint8 *srcPtr, uint32 srcPitch, uint8 *null,
								uint8 *dstPtr, uint32 dstPitch, int width, int height);

void atexit_proc() {
	SDL_ShowCursor(SDL_ENABLE);

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_QuitSubSystem(SDL_INIT_TIMER);
	SDL_QuitSubSystem(SDL_INIT_CDROM);
#ifndef WIN32	// FIXME: SDL bug - closing audio freezes w/ waveout
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
}

OSystem *OSystem_SDL::create(int gfx_mode, bool full_screen) {
	OSystem_SDL *syst = new OSystem_SDL();
	syst->_mode = gfx_mode;
	syst->_full_screen = full_screen;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) ==-1) {
		error("Could not initialize SDL: %s.\n", SDL_GetError());
	}

#ifdef WIN32				  /* Use waveout on win32, not */
	SDL_AudioInit("waveout"); /* dsound - unfortunatly dsound */
#endif						  /* doesn't do COOPERATIVE mode*/

	SDL_ShowCursor(SDL_DISABLE);

	/* Setup the icon */
	syst->setup_icon();

	/* Clean up on exit */
 	atexit(atexit_proc);

	return syst;
}

void OSystem_SDL::set_timer(int timer, int (*callback)(int)) {
	SDL_SetTimer(timer, (SDL_TimerCallback) callback);
}
OSystem *OSystem_SDL_create(int gfx_mode, bool full_screen) {
	return OSystem_SDL::create(gfx_mode, full_screen);
}

void OSystem_SDL::set_palette(const byte *colors, uint start, uint num) {
	const byte *b = colors;
	uint i;
	SDL_Color *base = _cur_pal + start;
	for(i=0;i!=num;i++) {
		base[i].r = b[0];
		base[i].g = b[1];
		base[i].b = b[2];
		b += 4;
	}

	if (start < _palette_changed_first)
		_palette_changed_first = start;

	if (start + num > _palette_changed_last)
		_palette_changed_last = start + num;
}

void OSystem_SDL::load_gfx_mode() {
	force_full = true;
	scaling = 1;
	_mode_flags = 0;

	_sai_func = NULL;
	sdl_tmpscreen = NULL;
	
	switch(_mode) {
	case GFX_2XSAI:
		scaling = 2;
		_sai_func = _2xSaI;
		break;
	case GFX_SUPER2XSAI:
		scaling = 2;
		_sai_func = Super2xSaI;
		break;
	case GFX_SUPEREAGLE:
		scaling = 2;
		_sai_func = SuperEagle;
		break;
	case GFX_ADVMAME2X:
		scaling = 2;
		_sai_func = AdvMame2x;
		break;

	case GFX_DOUBLESIZE:
		scaling = 2;
		break;

	case GFX_TRIPLESIZE:
		if (_full_screen) {
			warning("full screen in useless in triplesize mode, reverting to normal mode");
			goto normal_mode;
		}
		scaling = 3;
		break;

	case GFX_NORMAL:
normal_mode:;
		scaling = 1;
		break;
	}

	sdl_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0, 0, 0, 0);
	if (sdl_screen == NULL)
		error("sdl_screen failed failed");

	if (_sai_func) {
		uint16 *tmp_screen = (uint16*)calloc((SCREEN_WIDTH+3)*(SCREEN_HEIGHT+3),sizeof(uint16));
		_mode_flags = DF_WANT_RECT_OPTIM | DF_SEPARATE_TEMPSCREEN | DF_UPDATE_EXPAND_1_PIXEL;

		sdl_hwscreen = SDL_SetVideoMode(SCREEN_WIDTH * scaling, SCREEN_HEIGHT * scaling, 16, 
			_full_screen ? (SDL_FULLSCREEN|SDL_SWSURFACE) : SDL_SWSURFACE
		);
		if (sdl_hwscreen == NULL)
			error("sdl_hwscreen failed");
		
		/* Need some extra bytes around when using 2XSAI */
		if (sdl_hwscreen->format->Rmask == 0x7C00)	// HACK HACK HACK
			Init_2xSaI(555);
		else
			Init_2xSaI(565);
		sdl_tmpscreen = SDL_CreateRGBSurfaceFrom(tmp_screen,
							SCREEN_WIDTH + 3, SCREEN_HEIGHT + 3, 16, (SCREEN_WIDTH + 3)*2,
							sdl_hwscreen->format->Rmask,
							sdl_hwscreen->format->Gmask,
							sdl_hwscreen->format->Bmask,
							sdl_hwscreen->format->Amask);

		if (sdl_tmpscreen == NULL)
			error("sdl_tmpscreen failed");
		
	} else {
		switch(scaling) {
		case 3:
			_sai_func = Normal3x;
			break;
		case 2:
			_sai_func = Normal2x;
			break;
		case 1:
			_sai_func = Normal1x;
			break;
		}

		_mode_flags = DF_WANT_RECT_OPTIM | DF_REAL_8BIT;

		sdl_hwscreen = SDL_SetVideoMode(SCREEN_WIDTH * scaling, SCREEN_HEIGHT * scaling, 8, 
			_full_screen ? (SDL_FULLSCREEN|SDL_SWSURFACE) : SDL_SWSURFACE
		);
		if (sdl_hwscreen == NULL)
			error("sdl_hwscreen failed");
		
		sdl_tmpscreen = sdl_screen;
	}

	// keyboard cursor control, some other better place for it?
	km.xm = SCREEN_WIDTH * scaling;
	km.ym = SCREEN_HEIGHT * scaling;
	km.delay = 25;
	km.last = 0;

}

void OSystem_SDL::unload_gfx_mode() {
	SDL_FreeSurface(sdl_screen);
	sdl_screen = NULL; 

	SDL_FreeSurface(sdl_hwscreen); 
	sdl_hwscreen = NULL;

	if (_mode_flags & DF_SEPARATE_TEMPSCREEN) {
		free((uint16*)sdl_tmpscreen->pixels);
		SDL_FreeSurface(sdl_tmpscreen);
	}
	sdl_tmpscreen = NULL;
}

void OSystem_SDL::init_size(uint w, uint h) {
	//if (w != SCREEN_WIDTH && h != SCREEN_HEIGHT)
	//	error("320x200 is the only game resolution supported");

	SCREEN_WIDTH = w;
	SCREEN_HEIGHT = h;
	CKSUM_NUM = (SCREEN_WIDTH*SCREEN_HEIGHT/(8*8));
	/* allocate palette, it needs to be persistent across
	 * driver changes, so i'll alloc it here */
	_cur_pal = (SDL_Color*)calloc(sizeof(SDL_Color), 256);

	dirty_rect_list = (SDL_Rect*)calloc(NUM_DIRTY_RECT, sizeof(SDL_Rect));
	_mouse_backup = (byte*)malloc(MAX_MOUSE_W * MAX_MOUSE_H * MAX_SCALING);
	dirty_checksums = (uint32*)calloc(CKSUM_NUM*2, sizeof(uint32));

	load_gfx_mode();
}

void OSystem_SDL::copy_rect(const byte *buf, int pitch, int x, int y, int w, int h) {
	if (sdl_screen == NULL)
		return;

	if (pitch == SCREEN_WIDTH && x==0 && y==0 && w==SCREEN_WIDTH && h==SCREEN_HEIGHT && _mode_flags&DF_WANT_RECT_OPTIM) {
		/* Special, optimized case for full screen updates.
		 * It tries to determine what areas were actually changed,
		 * and just updates those, on the actual display. */
		add_dirty_rgn_auto(buf);
	} else {
		/* Clip the coordinates */
		if (x < 0) { w+=x; buf-=x; x = 0; }
		if (y < 0) { h+=y; buf-=y*pitch; y = 0; }
		if (w > SCREEN_WIDTH-x) { w = SCREEN_WIDTH - x; }
		if (h > SCREEN_HEIGHT-y) { h = SCREEN_HEIGHT - y; }
			
		if (w <= 0 || h <= 0)
			return;

		cksum_valid = false;
		add_dirty_rect(x, y, w, h);
	}

	/* FIXME: undraw mouse only if the draw rect intersects with the mouse rect */
	if (_mouse_drawn)
		undraw_mouse();

	if (SDL_LockSurface(sdl_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	byte *dst = (byte *)sdl_screen->pixels + y * SCREEN_WIDTH + x;
	do {
		memcpy(dst, buf, w);
		dst += SCREEN_WIDTH;
		buf += pitch;
	} while (--h);

	SDL_UnlockSurface(sdl_screen);
}


void OSystem_SDL::add_dirty_rect(int x, int y, int w, int h) {
	if (force_full)
		return;

	if (num_dirty_rects == NUM_DIRTY_RECT)
		force_full = true;
	else {
		SDL_Rect *r = &dirty_rect_list[num_dirty_rects++];
		
		/* Update the dirty region by 1 pixel for graphics drivers
		 * that "smear" the screen */
		if (_mode_flags & DF_UPDATE_EXPAND_1_PIXEL) {
			x--;
			y--;
			w+=2;
			h+=2;
		}

		/* clip */
		if (x < 0) { w+=x; x=0; }
		if (y < 0) { h+=y; y=0; }
		if (w > SCREEN_WIDTH-x) { w = SCREEN_WIDTH - x; }
		if (h > SCREEN_HEIGHT-y) { h = SCREEN_HEIGHT - y; }
	
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
	}
}

#define ROL(a,n) a = (a<<(n)) | (a>>(32-(n)))
#define DOLINE(x) a ^= ((uint32*)buf)[0+(x)*(SCREEN_WIDTH/4)]; b ^= ((uint32*)buf)[1+(x)*(SCREEN_WIDTH/4)]
void OSystem_SDL::mk_checksums(const byte *buf) {
	uint32 *sums = dirty_checksums;
	uint x,y;
	const uint last_x = (uint)SCREEN_WIDTH/8;
	const uint last_y = (uint)SCREEN_HEIGHT/8;

	/* the 8x8 blocks in buf are enumerated starting in the top left corner and
	 * reading each line at a time from left to right */
	for(y=0; y != last_y; y++, buf+=SCREEN_WIDTH*(8-1))
		for(x=0; x != last_x; x++, buf+=8) {
			uint32 a = x;
			uint32 b = y;

			DOLINE(0); ROL(a,13); ROL(b,11);
			DOLINE(2); ROL(a,13); ROL(b,11);
			DOLINE(4); ROL(a,13); ROL(b,11);
			DOLINE(6); ROL(a,13); ROL(b,11);

			a*=0xDEADBEEF;
			b*=0xBAADF00D;

			DOLINE(1); ROL(a,13); ROL(b,11);
			DOLINE(3); ROL(a,13); ROL(b,11);
			DOLINE(5); ROL(a,13); ROL(b,11);
			DOLINE(7); ROL(a,13); ROL(b,11);

			/* output the checksum for this block */
			*sums++=a+b;
		}
}
#undef DOLINE
#undef ROL


void OSystem_SDL::add_dirty_rgn_auto(const byte *buf) {
	assert( ((uint32)buf & 3) == 0);
	
	/* generate a table of the checksums */
	mk_checksums(buf);

	if (!cksum_valid) {
		force_full = true;
		cksum_valid = true;
	}

	/* go through the checksum list, compare it with the previous checksums,
		 and add all dirty rectangles to a list. try to combine small rectangles
		 into bigger ones in a simple way */
	if (!force_full) {
		int x,y,w;
		uint32 *ck = dirty_checksums;
		
		for(y=0; y!=SCREEN_HEIGHT/8; y++) {
			for(x=0; x!=SCREEN_WIDTH/8; x++,ck++) {
				if (ck[0] != ck[CKSUM_NUM]) {
					/* found a dirty 8x8 block, now go as far to the right as possible,
						 and at the same time, unmark the dirty status by setting old to new. */
					w=0;
					do {
						ck[w+CKSUM_NUM] = ck[w];
						w++;
					} while (x+w != SCREEN_WIDTH/8 && ck[w] != ck[w+CKSUM_NUM]);
					
					add_dirty_rect(x*8, y*8, w*8, 8);

					if (force_full)
						goto get_out;
				}
			}
		}
	} else {
		get_out:;
		/* Copy old checksums to new */
		memcpy(dirty_checksums + CKSUM_NUM, dirty_checksums, CKSUM_NUM * sizeof(uint32));
	}
}

void OSystem_SDL::update_screen() {
	
	if (sdl_hwscreen == NULL)
		return;	// Can this really happen?

	/* First make sure the mouse is drawn, if it should be drawn. */
	draw_mouse();
	
	/* Palette update in case we are NOT in "real" 8 bit color mode.
	 * Must take place before updating the screen, since the palette must
	 * be set up for conversion from 8bit to 16bit.
	 */ 
	if (((_mode_flags & DF_REAL_8BIT) == 0) && _palette_changed_last != 0) {
		SDL_SetColors(sdl_screen, _cur_pal + _palette_changed_first, 
			_palette_changed_first,
			_palette_changed_last - _palette_changed_first);
		
		_palette_changed_last = 0;

		force_full = true;
	}

	
	/* If the shake position changed, fill the dirty area with blackness */
	if (_current_shake_pos != _new_shake_pos) {
		SDL_Rect blackrect = {0, 0, SCREEN_WIDTH*scaling, _new_shake_pos*scaling};
		SDL_FillRect(sdl_hwscreen, &blackrect, 0);

		_current_shake_pos = _new_shake_pos;

		force_full = true;
	}

	/* force a full redraw if requested */
	if (force_full) {
		num_dirty_rects = 1;

		dirty_rect_list[0].x = 0;
		dirty_rect_list[0].y = 0;
		dirty_rect_list[0].w = SCREEN_WIDTH;
		dirty_rect_list[0].h = SCREEN_HEIGHT;
	}

	/* Only draw anything if necessary */
	if (num_dirty_rects > 0) {
	
		SDL_Rect *r; 
		uint32 srcPitch, dstPitch;
		SDL_Rect *last_rect = dirty_rect_list + num_dirty_rects;
	
		/* Convert appropriate parts of the image into 16bpp */
		if ((_mode_flags & DF_REAL_8BIT) == 0) {
			SDL_Rect dst;
			for(r=dirty_rect_list; r!=last_rect; ++r) {
				dst = *r;
				dst.x++;
				dst.y++;
				if (SDL_BlitSurface(sdl_screen, r, sdl_tmpscreen, &dst) != 0)
					error("SDL_BlitSurface failed: %s", SDL_GetError());
			}
		}
	
		SDL_LockSurface(sdl_tmpscreen);
		SDL_LockSurface(sdl_hwscreen);
	
		srcPitch = sdl_tmpscreen->pitch;
		dstPitch = sdl_hwscreen->pitch;
	
		if ((_mode_flags & DF_REAL_8BIT) == 0) {
			for(r=dirty_rect_list; r!=last_rect; ++r) {
				register int dst_y = r->y + _current_shake_pos;
				register int dst_h = 0;
				if (dst_y < SCREEN_HEIGHT) {
					dst_h = r->h;
					if (dst_h > SCREEN_HEIGHT - dst_y)
						dst_h = SCREEN_HEIGHT - dst_y;
					
					r->x <<= 1;
					dst_y <<= 1;
					
					_sai_func((byte*)sdl_tmpscreen->pixels + (r->x+2) + (r->y+1)*srcPitch, srcPitch, NULL, 
						(byte*)sdl_hwscreen->pixels + r->x*scaling + dst_y*dstPitch, dstPitch, r->w, dst_h);
				}
				
				r->y = dst_y;
				r->w <<= 1;
				r->h = dst_h << 1;
			}
		} else {
			for(r=dirty_rect_list; r!=last_rect; ++r) {
				register int dst_y = r->y + _current_shake_pos;
				register int dst_h = 0;
				if (dst_y < SCREEN_HEIGHT) {
					dst_h = r->h;
					if (dst_h > SCREEN_HEIGHT - dst_y)
						dst_h = SCREEN_HEIGHT - dst_y;
					
					dst_y *= scaling;
					
					_sai_func((byte*)sdl_tmpscreen->pixels + r->x + r->y*srcPitch, srcPitch, NULL, 
						(byte*)sdl_hwscreen->pixels + r->x*scaling + dst_y*dstPitch, dstPitch, r->w, dst_h);
				}
				
				r->x *= scaling;
				r->y = dst_y;
				r->w *= scaling;
				r->h = dst_h * scaling;
			}
		}
	
		if (force_full) {
			dirty_rect_list[0].y = 0;
			dirty_rect_list[0].h = SCREEN_HEIGHT * scaling;
		}
		
		SDL_UnlockSurface(sdl_tmpscreen);
		SDL_UnlockSurface(sdl_hwscreen);
	}
	
	/* Palette update in case we are in "real" 8 bit color mode.
	 * Must take place after the screen data was updated, since with
	 * "real" 8bit mode, palatte changes may be visible immediatly,
	 * and we want to avoid any ugly effects.
	 */
	if (_mode_flags & DF_REAL_8BIT && _palette_changed_last != 0) {
		SDL_SetColors(sdl_hwscreen, _cur_pal + _palette_changed_first, 
			_palette_changed_first,
			_palette_changed_last - _palette_changed_first);
		
		_palette_changed_last = 0;
	}
	
	if (num_dirty_rects > 0) {
		/* Finally, blit all our changes to the screen */
		SDL_UpdateRects(sdl_hwscreen, num_dirty_rects, dirty_rect_list);
	}

	num_dirty_rects = 0;
	force_full = false;
}

void OSystem_SDL::kbd_mouse() {
	uint32 time = get_msecs();
	if (time >= km.last + km.delay) {
		km.last = time;
		if (km.xd == 1) {
			km.xdown = time;
			km.xd = 2;
		}
		if (km.yd == 1) {
			km.ydown = time;      
			km.yd = 2;
		}

		if (km.xv || km.yv) {
			if (km.xd) {
				if (time > km.xdown + km.delay*12) {
					if (km.xv > 0)
						km.xv++;
					else
						km.xv--;
				} else if (time > km.xdown + km.delay*8) {
					if (km.xv > 0)
						km.xv = 5;
					else
						km.xv = -5;
				}
			}
			if (km.yd) {
				if (time > km.ydown + km.delay*12) {
					if (km.yv > 0)
						km.yv++;
					else
						km.yv--;
				} else if (time > km.ydown + km.delay*8) {
					if (km.yv > 0)
						km.yv = 5;
					else
						km.yv = -5;
				}
			}

			km.x += km.xv;
			km.y += km.yv;

			if (km.x < 0) {
				km.x = 0;
				km.xv = -1;
				km.xd = 1;
			} else if (km.x >= km.xm) {
				km.x = km.xm - 1;
				km.xv = 1;
				km.xd = 1;
			}

			if (km.y < 0) {
				km.y = 0;
				km.yv = -1;
				km.yd = 1;
			} else if (km.y >= km.ym) {
				km.y = km.ym - 1;
				km.yv = 1;
				km.yd = 1;
			}

			SDL_WarpMouse(km.x, km.y);
		}
	}
}

bool OSystem_SDL::show_mouse(bool visible) {
	if (_mouse_visible == visible)
		return visible;
	
	bool last = _mouse_visible;
	_mouse_visible = visible;

	if (visible)
		draw_mouse();
	else
		undraw_mouse();

	return last;
}
	
void OSystem_SDL::set_mouse_pos(int x, int y) {
	if (x != _mouse_cur_state.x || y != _mouse_cur_state.y) {
		_mouse_cur_state.x = x;
		_mouse_cur_state.y = y;
		undraw_mouse();
	}
}
	
void OSystem_SDL::set_mouse_cursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y) {
	_mouse_cur_state.w = w;
	_mouse_cur_state.h = h;

	_mouse_hotspot_x = hotspot_x;
	_mouse_hotspot_y = hotspot_y;

	_mouse_data = (byte*)buf;

	undraw_mouse();
}
	
void OSystem_SDL::set_shake_pos(int shake_pos) {
	_new_shake_pos = shake_pos;
}
		
uint32 OSystem_SDL::get_msecs() {
	return SDL_GetTicks();	
}
	
void OSystem_SDL::delay_msecs(uint msecs) {
	SDL_Delay(msecs);
}
	
void *OSystem_SDL::create_thread(ThreadProc *proc, void *param) {
	return SDL_CreateThread(proc, param);
}

int mapKey(int key, byte mod)
{
	if (key >= SDLK_F1 && key <= SDLK_F9) {
		return key - SDLK_F1 + 315;
	} else if (key >= 'a' && key <= 'z' && mod & KMOD_SHIFT) {
		key &= ~0x20;
	} else if (key >= SDLK_NUMLOCK && key <= SDLK_EURO)
		return 0;
	return key;
}
	
bool OSystem_SDL::poll_event(Event *event) {
	SDL_Event ev;
	kbd_mouse();

	for(;;) {
		if (!SDL_PollEvent(&ev))
			return false;

		switch(ev.type) {
		case SDL_KEYDOWN: {
				byte b = 0;
				if (ev.key.keysym.mod & KMOD_SHIFT) b |= KBD_SHIFT;
				if (ev.key.keysym.mod & KMOD_CTRL) b |= KBD_CTRL;
				if (ev.key.keysym.mod & KMOD_ALT) b |= KBD_ALT;
				event->kbd.flags = b;

				/* internal keypress? */				
				if (b == KBD_ALT && ev.key.keysym.sym==SDLK_RETURN) {
					property(PROP_TOGGLE_FULLSCREEN, NULL);
					break;
				}

				if ((b == KBD_CTRL && ev.key.keysym.sym=='z') || (b == KBD_ALT && ev.key.keysym.sym=='x')) {
					quit();
					break;
				}

				if (b == (KBD_CTRL|KBD_ALT) && 
				    (ev.key.keysym.sym>='1') && (ev.key.keysym.sym<='7')) {
					Property prop;
					prop.gfx_mode = ev.key.keysym.sym - '1';
					property(PROP_SET_GFX_MODE, &prop);
					break;
				}


				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = ev.key.keysym.sym;
				event->kbd.ascii = mapKey(ev.key.keysym.sym, ev.key.keysym.mod);
				switch(ev.key.keysym.sym) {
					case SDLK_LEFT:
						km.xv = -1;
						km.xd = 1;
					break;
					case SDLK_RIGHT:
						km.xv =  1;
						km.xd = 1;
					break;
					case SDLK_UP:
						km.yv = -1;
						km.yd = 1;
					break;
					case SDLK_DOWN:
						km.yv =  1;
						km.yd = 1;
					break;
					default:
					break;
				}

				return true;
			}

			case SDL_KEYUP: {
				switch(ev.key.keysym.sym){
					case SDLK_LEFT:                
						if (km.xv < 0) {
							km.xv = 0;
							km.xd = 0;
						}
					break;
					case SDLK_RIGHT:
						if (km.xv > 0) {
							km.xv = 0;
							km.xd = 0;
						}
					break;
					case SDLK_UP:
						if (km.yv < 0) {
							km.yv = 0;
							km.yd = 0;
						}
					break;
					case SDLK_DOWN:
						if (km.yv > 0) {
							km.yv = 0;
							km.yd = 0;
						}
					break;
					default:
					break;
				}
				return true;
			}

		case SDL_MOUSEMOTION:
			event->event_code = EVENT_MOUSEMOVE;
			km.x = event->mouse.x = ev.motion.x;
			km.y = event->mouse.y = ev.motion.y;

			event->mouse.x /= scaling;
			event->mouse.y /= scaling;

			return true;

		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event->event_code = EVENT_LBUTTONDOWN;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event->event_code = EVENT_RBUTTONDOWN;
			else
				break;
			km.x = event->mouse.x = ev.motion.x;
			km.y = event->mouse.y = ev.motion.y;
			event->mouse.x /= scaling;
			event->mouse.y /= scaling;

			return true;

		case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event->event_code = EVENT_LBUTTONUP;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event->event_code = EVENT_RBUTTONUP;
			else
				break;
			event->mouse.x = ev.button.x;
			event->mouse.y = ev.button.y;
			event->mouse.x /= scaling;
			event->mouse.y /= scaling;
			return true;

		case SDL_QUIT:
			quit();
		}
	}
}
	
bool OSystem_SDL::set_sound_proc(void *param, SoundProc *proc, byte format) {
	SDL_AudioSpec desired;

	/* only one format supported at the moment */

	desired.freq = SAMPLES_PER_SEC;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 2048;
	desired.callback = proc;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) != 0) {
		return false;
	}
	SDL_PauseAudio(0);
	return true;
}


/* retrieve the 320x200 bitmap currently being displayed */
void OSystem_SDL::get_320x200_image(byte *buf) {
	/* make sure the mouse is gone */
	undraw_mouse();
	
	if (SDL_LockSurface(sdl_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	memcpy(buf, sdl_screen->pixels, SCREEN_WIDTH*SCREEN_HEIGHT);

	SDL_UnlockSurface(sdl_screen);
}

void OSystem_SDL::hotswap_gfx_mode() {
	/* hmm, need to allocate a 320x200 bitmap
	 * which will contain the "backup" of the screen during the change.
	 * then draw that to the new screen right after it's setup.
	 */
	
	byte *bak_mem = (byte*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT);

	get_320x200_image(bak_mem);

	unload_gfx_mode();
	load_gfx_mode();

	force_full = true;

	/* reset palette */
	if (_mode_flags & DF_REAL_8BIT)
		SDL_SetColors(sdl_hwscreen, _cur_pal, 0, 256);
	else
		SDL_SetColors(sdl_screen, _cur_pal, 0, 256);

	/* blit image */
	OSystem_SDL::copy_rect(bak_mem, SCREEN_WIDTH, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	free(bak_mem);

	OSystem_SDL::update_screen();
}

uint32 OSystem_SDL::property(int param, Property *value) {
	switch(param) {

	case PROP_TOGGLE_FULLSCREEN:
		_full_screen ^= true;

		if (!SDL_WM_ToggleFullScreen(sdl_hwscreen)) {
			/* if ToggleFullScreen fails, achieve the same effect with hotswap gfx mode */
			hotswap_gfx_mode();
		}
		return 1;

	case PROP_GET_FULLSCREEN:
		return _full_screen;

	case PROP_SET_WINDOW_CAPTION:
		SDL_WM_SetCaption(value->caption, value->caption);
		return 1;

	case PROP_OPEN_CD:
		if (SDL_InitSubSystem(SDL_INIT_CDROM) == -1)
			cdrom = NULL;
		else {
			cdrom = SDL_CDOpen(value->cd_num);
			/* Did if open? Check if cdrom is NULL */
			if (!cdrom) {
				warning("Couldn't open drive: %s\n", SDL_GetError());
			}
		}
		break;

	case PROP_SET_GFX_MODE:
		if (value->gfx_mode >= 7)
			return 0;

		_mode = value->gfx_mode;
		hotswap_gfx_mode();

		return 1;

	case PROP_SHOW_DEFAULT_CURSOR:
		SDL_ShowCursor(value->show_cursor ? SDL_ENABLE : SDL_DISABLE);		
		break;

	case PROP_GET_SAMPLE_RATE:
		return SAMPLES_PER_SEC;
	}

	return 0;
}
		
void OSystem_SDL::quit() {
  if(cdrom) {
    SDL_CDStop(cdrom);
    SDL_CDClose(cdrom);
  }
	unload_gfx_mode();		
	exit(1);
}

void OSystem_SDL::draw_mouse() {
	if (_mouse_drawn || !_mouse_visible)
		return;

	int x = _mouse_cur_state.x - _mouse_hotspot_x;
	int y = _mouse_cur_state.y - _mouse_hotspot_y;
	int w = _mouse_cur_state.w;
	int h = _mouse_cur_state.h;
	byte color;
	byte *src = _mouse_data;		// Image representing the mouse
	byte *bak = _mouse_backup;		// Surface used to backup the area obscured by the mouse
	byte *dst;					// Surface we are drawing into

	// clip the mouse rect, and addjust the src pointer accordingly
	if (x < 0) {
		w += x;
		src -= x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		src -= y * _mouse_cur_state.w;
		y = 0;
	}
	if (w > SCREEN_WIDTH - x)
		w = SCREEN_WIDTH - x;
	if (h > SCREEN_HEIGHT - y)
		h = SCREEN_HEIGHT - y;

	// Store the bounding box so that undraw mouse can restore the area the
	// mouse currently covers to its original content.
	_mouse_old_state.x = x;
	_mouse_old_state.y = y;
	_mouse_old_state.w = w;
	_mouse_old_state.h = h;

	// Quick check to see if anything has to be drawn at all
	if (w <= 0 || h <= 0)
		return;

	// Draw the mouse cursor; backup the covered area in "bak"

	if (SDL_LockSurface(sdl_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	add_dirty_rect(x, y, w, h);

	dst = (byte *)sdl_screen->pixels + y * SCREEN_WIDTH + x;
	while (h > 0) {
		int width = w;
		while (width > 0) {
			*bak++ = *dst;
			color = *src++;
			if (color != 0xFF)	// 0xFF = transparent, don't draw
				*dst = color;
			dst++;
			width--;
		}
		src += _mouse_cur_state.w - w;
		bak += MAX_MOUSE_W - w;
		dst += SCREEN_WIDTH - w;
		h--;
	}

	SDL_UnlockSurface(sdl_screen);
	
	// Finally, set the flag to indicate the mouse has been drawn
	_mouse_drawn = true;
}

void OSystem_SDL::undraw_mouse() {
	if (!_mouse_drawn)
		return;
	_mouse_drawn = false;

	if (SDL_LockSurface(sdl_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	byte *dst, *bak = _mouse_backup;
	const int old_mouse_x = _mouse_old_state.x;
	const int old_mouse_y = _mouse_old_state.y;
	const int old_mouse_w = _mouse_old_state.w;
	const int old_mouse_h = _mouse_old_state.h;
	int x,y;

	// No need to do clipping here, since draw_mouse() did that already

	dst = (byte *)sdl_screen->pixels + old_mouse_y * SCREEN_WIDTH + old_mouse_x;
	for (y = 0; y < old_mouse_h; ++y, bak += MAX_MOUSE_W, dst += SCREEN_WIDTH) {
		for (x = 0; x < old_mouse_w; ++x) {
			dst[x] = bak[x];
		}
	}

	add_dirty_rect(old_mouse_x, old_mouse_y, old_mouse_w, old_mouse_h);

	SDL_UnlockSurface(sdl_screen);
}

void OSystem_SDL::stop_cdrom() {	/* Stop CD Audio in 1/10th of a second */
	cd_stop_time = SDL_GetTicks() + 100;
	cd_num_loops = 0;

}

void OSystem_SDL::play_cdrom(int track, int num_loops, int start_frame, int end_frame) {
	if (!num_loops && !start_frame)
		return;

	if (!cdrom)
		return;
	
	if (end_frame > 0)
		end_frame+=5;

	cd_track = track;
	cd_num_loops = num_loops;
	cd_start_frame = start_frame;

	SDL_CDStatus(cdrom);	
	SDL_CDPlayTracks(cdrom, track, start_frame, 0, end_frame);
	cd_end_frame = end_frame;
	cd_stop_time = 0;
	cd_end_time = SDL_GetTicks() + cdrom->track[track].length * 1000 / CD_FPS;
}

bool OSystem_SDL::poll_cdrom() {
	if (!cdrom)
		return false;

	return (cd_num_loops != 0 && (SDL_GetTicks() < cd_end_time || SDL_CDStatus(cdrom) != CD_STOPPED));
}

void OSystem_SDL::update_cdrom() {
	if (!cdrom)
		return;
		
	if (cd_stop_time != 0 && SDL_GetTicks() >= cd_stop_time) {
		SDL_CDStop(cdrom);
		cd_num_loops = 0;
		cd_stop_time = 0;
		return;
	}

	if (cd_num_loops == 0 || SDL_GetTicks() < cd_end_time)
		return;

	if (cd_num_loops != 1 && SDL_CDStatus(cdrom) != CD_STOPPED) {
		// Wait another second for it to be done
		cd_end_time += 1000;
		return;
	}

	if (cd_num_loops > 0)
		cd_num_loops--;

	if (cd_num_loops != 0) {
		SDL_CDPlayTracks(cdrom, cd_track, cd_start_frame, 0, cd_end_frame);
		cd_end_time = SDL_GetTicks() + cdrom->track[cd_track].length * 1000 / CD_FPS;
	}
}

void OSystem_SDL::setup_icon() {
	int w, h, ncols, nbytes, i;
	unsigned int rgba[256], icon[32 * 32];
	unsigned char mask[32][4];

	sscanf(scummvm_icon[0], "%d %d %d %d", &w, &h, &ncols, &nbytes);
	if ((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1)) {
		warning("Could not load the icon (%d %d %d %d)", w, h, ncols, nbytes);
		return;
	}
	for (i = 0; i < ncols; i++) {
		unsigned char code;
		char color[32];
		unsigned int col;
		sscanf(scummvm_icon[1 + i], "%c c %s", &code, color);
		if (!strcmp(color, "None"))
			col = 0x00000000;
		else if (!strcmp(color, "black"))
			col = 0xFF000000;
		else if (color[0] == '#') {
			sscanf(color + 1, "%06x", &col);
			col |= 0xFF000000;
		} else {
			warning("Could not load the icon (%d %s - %s) ", code, color, scummvm_icon[1 + i]);
			return;
		}
		
		rgba[code] = col;
	}
	memset(mask, 0, sizeof(mask));
	for (h = 0; h < 32; h++) {
		char *line = scummvm_icon[1 + ncols + h];
		for (w = 0; w < 32; w++) {
			icon[w + 32 * h] = rgba[line[w]];
			if (rgba[line[w]] & 0xFF000000) {
				mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
			}
		}
	}

	SDL_Surface *sdl_surf = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32, 32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
	SDL_WM_SetIcon(sdl_surf, (unsigned char *) mask);
}

void *OSystem_SDL::create_mutex(void) {
	return (void *) SDL_CreateMutex();
}

void OSystem_SDL::lock_mutex(void *mutex) {
	SDL_mutexP((SDL_mutex *) mutex);
}

void OSystem_SDL::unlock_mutex(void *mutex) {
	SDL_mutexV((SDL_mutex *) mutex);
}

void OSystem_SDL::delete_mutex(void *mutex) {
	SDL_DestroyMutex((SDL_mutex *) mutex);
}


#ifdef USE_NULL_DRIVER

/* NULL video driver */
class OSystem_NULL : public OSystem {
public:
	void set_palette(const byte *colors, uint start, uint num) {}
	void init_size(uint w, uint h);
	void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h) {}
	void update_screen() {}
	bool show_mouse(bool visible) { return false; }
	void set_mouse_pos(int x, int y) {}
	void set_mouse_cursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y) {}
	void set_shake_pos(int shake_pos) {}
	uint32 get_msecs();
	void delay_msecs(uint msecs);
	void *create_thread(ThreadProc *proc, void *param) { return NULL; }
	bool poll_event(Event *event) { return false; }
	bool set_sound_proc(void *param, SoundProc *proc, byte sound) {}
	void quit() { exit(1); }
	uint32 property(int param, Property *value) { return 0; }
	static OSystem *create(int gfx_mode, bool full_screen);
private:

	uint msec_start;

	uint32 get_ticks();
};

void OSystem_NULL::init_size(uint w, uint h, byte sound) {
	msec_start = get_ticks();
}

uint32 OSystem_NULL::get_ticks() {
	uint a = 0;
#ifdef WIN32
	a = GetTickCount();
#endif

#ifdef UNIX
	struct timeval tv;
	gettimeofday(&tv, NULL);
	a = tv.tv_sec * 1000 + tv.tv_usec/1000;
#endif

	return a;
}

void OSystem_NULL::delay_msecs(uint msecs) {
#ifdef WIN32
	Sleep(msecs);
#endif
#ifdef UNIX
	usleep(msecs*1000);
#endif
}

uint32 OSystem_NULL::get_msecs() {
	return get_ticks() - msec_start;
}

OSystem *OSystem_NULL_create() {
	return new OSystem_NULL();
}
#else /* USE_NULL_DRIVER */

OSystem *OSystem_NULL_create() {
	return NULL;
}

#endif
