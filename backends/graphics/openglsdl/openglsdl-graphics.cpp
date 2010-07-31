/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#if defined(USE_OPENGL)

#include "backends/graphics/openglsdl/openglsdl-graphics.h"
#include "backends/platform/sdl/sdl.h"

OpenGLSdlGraphicsManager::OpenGLSdlGraphicsManager()
	:
	_hwscreen(0),
	_screenResized(false) {

	// Initialize SDL video subsystem
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
		error("Could not initialize SDL: %s", SDL_GetError());
	}

	// Disable OS cursor
	SDL_ShowCursor(SDL_DISABLE);

	// Get desktop resolution
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	_desktopWidth = videoInfo->current_w; 
	_desktopHeight = videoInfo->current_h;
}

OpenGLSdlGraphicsManager::~OpenGLSdlGraphicsManager() {

}


bool OpenGLSdlGraphicsManager::hasFeature(OSystem::Feature f) {
	return
		(f == OSystem::kFeatureFullscreenMode) ||
		(f == OSystem::kFeatureIconifyWindow) ||
		OpenGLGraphicsManager::hasFeature(f);
}

void OpenGLSdlGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		setFullscreenMode(enable);
		break;
	case OSystem::kFeatureIconifyWindow:
		if (enable)
			SDL_WM_IconifyWindow();
		break;
	default:
		OpenGLGraphicsManager::setFeatureState(f, enable);
	}
}

#ifdef USE_RGB_COLOR

Common::List<Graphics::PixelFormat> OpenGLSdlGraphicsManager::getSupportedFormats() const {
	assert(!_supportedFormats.empty());
	return _supportedFormats;
}

void OpenGLSdlGraphicsManager::detectSupportedFormats() {

	// Clear old list
	_supportedFormats.clear();

	// Some tables with standard formats that we always list
	// as "supported". If frontend code tries to use one of
	// these, we will perform the necessary format
	// conversion in the background. Of course this incurs a
	// performance hit, but on desktop ports this should not
	// matter. We still push the currently active format to
	// the front, so if frontend code just uses the first
	// available format, it will get one that is "cheap" to
	// use.
	const Graphics::PixelFormat RGBList[] = {
#if defined(ENABLE_32BIT)
		Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0),	// RGBA8888
#ifndef USE_GLES
		Graphics::PixelFormat(4, 8, 8, 8, 8, 16, 8, 0, 24), // ARGB8888
#endif
		Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0),	// RGB888
#endif
		Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0),	// RGB565
		Graphics::PixelFormat(2, 5, 5, 5, 1, 11, 6, 1, 0),	// RGB5551
		Graphics::PixelFormat(2, 4, 4, 4, 4, 12, 8, 4, 0),	// RGBA4444
#ifndef USE_GLES
		Graphics::PixelFormat(2, 4, 4, 4, 4, 8, 4, 0, 12)   // ARGB4444
#endif
	};
#ifndef USE_GLES
	const Graphics::PixelFormat BGRList[] = {
#ifdef ENABLE_32BIT
		Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), // ABGR8888
		Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0), // BGRA8888
		Graphics::PixelFormat(3, 8, 8, 8, 0, 0, 8, 16, 0),  // BGR888
#endif
		Graphics::PixelFormat(2, 5, 6, 5, 0, 0, 5, 11, 0),  // BGR565
		Graphics::PixelFormat(2, 5, 5, 5, 1, 1, 6, 11, 0),  // BGRA5551
		Graphics::PixelFormat(2, 4, 4, 4, 4, 0, 4, 8, 12),  // ABGR4444
		Graphics::PixelFormat(2, 4, 4, 4, 4, 4, 8, 12, 0)   // BGRA4444
	};
#endif

	Graphics::PixelFormat format = Graphics::PixelFormat::createFormatCLUT8();
	if (_hwscreen) {
		// Get our currently set hardware format
		format = Graphics::PixelFormat(_hwscreen->format->BytesPerPixel,
			8 - _hwscreen->format->Rloss, 8 - _hwscreen->format->Gloss,
			8 - _hwscreen->format->Bloss, 8 - _hwscreen->format->Aloss,
			_hwscreen->format->Rshift, _hwscreen->format->Gshift,
			_hwscreen->format->Bshift, _hwscreen->format->Ashift);

		// Workaround to MacOSX SDL not providing an accurate Aloss value.
		if (_hwscreen->format->Amask == 0)
			format.aLoss = 8;

		// Push it first, as the prefered format if available
		for (int i = 0; i < ARRAYSIZE(RGBList); i++) {
			if (RGBList[i] == format) {
				_supportedFormats.push_back(format);
				break;
			}
		}
#ifndef USE_GLES
		for (int i = 0; i < ARRAYSIZE(BGRList); i++) {
			if (BGRList[i] == format) {
				_supportedFormats.push_back(format);
				break;
			}
		}
#endif
	}

	// Push some RGB formats
	for (int i = 0; i < ARRAYSIZE(RGBList); i++) {
		if (_hwscreen && (RGBList[i].bytesPerPixel > format.bytesPerPixel))
			continue;
		if (RGBList[i] != format)
			_supportedFormats.push_back(RGBList[i]);
	}

	// Push some BGR formats
	for (int i = 0; i < ARRAYSIZE(BGRList); i++) {
		if (_hwscreen && (BGRList[i].bytesPerPixel > format.bytesPerPixel))
			continue;
		if (BGRList[i] != format)
			_supportedFormats.push_back(BGRList[i]);
	}

	_supportedFormats.push_back(Graphics::PixelFormat::createFormatCLUT8());
}

#endif

void OpenGLSdlGraphicsManager::warpMouse(int x, int y) {
	if (_cursorState.x != x || _cursorState.y != y) {
		if (!_overlayVisible)
			SDL_WarpMouse(x * _videoMode.scaleFactor, y * _videoMode.scaleFactor);
		else
			SDL_WarpMouse(x, y);

		setMousePos(x, y);
	}
}


//
// Intern
//

bool OpenGLSdlGraphicsManager::loadGFXMode() {
	_videoMode.overlayWidth = _videoMode.screenWidth * _videoMode.scaleFactor;
	_videoMode.overlayHeight = _videoMode.screenHeight * _videoMode.scaleFactor;

	// If the screen was resized, do not change its size
	if (!_screenResized) {
		_videoMode.hardwareWidth = _videoMode.overlayWidth;
		_videoMode.hardwareHeight = _videoMode.overlayHeight;

		float screenAspectRatio = (float)_videoMode.screenWidth / _videoMode.screenHeight;
		float desiredAspectRatio = getAspectRatio();
	
		// Do not downscale dimensions, only enlarge them if needed
		if (screenAspectRatio > desiredAspectRatio)
			_videoMode.hardwareHeight = (int)(_videoMode.overlayWidth / desiredAspectRatio + 0.5f);
		else if (screenAspectRatio < desiredAspectRatio)
			_videoMode.hardwareWidth = (int)(_videoMode.overlayHeight * desiredAspectRatio + 0.5f);

		// Only adjust the overlay height if it is bigger than original one. If
		// the width is modified it can break the overlay.
		if (_videoMode.hardwareHeight > _videoMode.overlayHeight)
			_videoMode.overlayHeight = _videoMode.hardwareHeight;
	}

	_screenResized = false;

	// Setup OpenGL attributes for SDL
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Find the best mode for fullscreen
	if (_videoMode.fullscreen) {
		SDL_Rect const* const*availableModes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_OPENGL);

		if (_videoMode.activeFullscreenMode == -1) {
			const SDL_Rect *bestMode = availableModes[0];
			int bestModeIndex = 0;
			uint bestMetric = (uint)-1;

			// Iterate over all available fullscreen modes
			for (int i = 0; const SDL_Rect *mode = availableModes[i]; i++) {
				// Prefer the native resolution over other modes
				if(mode->w == _desktopWidth && mode->h == _desktopHeight) {
					bestMode = mode;
					bestModeIndex = i;
					break;
				}

				if (mode->w < _videoMode.hardwareWidth)
					continue;
				if (mode->h < _videoMode.hardwareHeight)
					continue;

				uint metric = mode->w * mode->h - _videoMode.hardwareWidth * _videoMode.hardwareHeight;
				if (metric > bestMetric)
					continue;

				bestMode = mode;
				bestMetric = metric;
				bestModeIndex = i;
			}

			if (bestMode) {
				// If there is a suiting mode, use it
				_videoMode.hardwareWidth = bestMode->w;
				_videoMode.hardwareHeight = bestMode->h;

				_videoMode.activeFullscreenMode = bestModeIndex;
			}
		} else {
			if (!availableModes[_videoMode.activeFullscreenMode])
				_videoMode.activeFullscreenMode = 0;

			if (availableModes[_videoMode.activeFullscreenMode]) {
				_videoMode.hardwareWidth = availableModes[_videoMode.activeFullscreenMode]->w;
				_videoMode.hardwareHeight = availableModes[_videoMode.activeFullscreenMode]->h;
			} else {
				return false;
			}
		}
	}

	// If changing to any fullscreen mode or from fullscreen,
	// the OpenGL context is destroyed
	if (_oldVideoMode.fullscreen || _videoMode.fullscreen)
		_transactionDetails.newContext = true;

	// Create our window
	_hwscreen = SDL_SetVideoMode(_videoMode.hardwareWidth, _videoMode.hardwareHeight, 32,
		_videoMode.fullscreen ? (SDL_FULLSCREEN | SDL_OPENGL) : (SDL_OPENGL | SDL_RESIZABLE)
	);
#ifdef USE_RGB_COLOR
	detectSupportedFormats();
#endif

	if (_hwscreen == NULL) {
		// DON'T use error(), as this tries to bring up the debug
		// console, which WON'T WORK now that _hwscreen is hosed.

		if (!_oldVideoMode.setup) {
			warning("SDL_SetVideoMode says we can't switch to that mode (%s)", SDL_GetError());
			g_system->quit();
		} else
			// Cancel GFX load, and go back to last mode
			return false;
	}

	// Check if the screen is BGR format
	_formatBGR = _hwscreen->format->Rshift != 0;

	// Call and return parent implementation of this method
	return OpenGLGraphicsManager::loadGFXMode();
}

void OpenGLSdlGraphicsManager::unloadGFXMode() {
	if (_hwscreen) {
		SDL_FreeSurface(_hwscreen);
		_hwscreen = NULL;
	}
}

void OpenGLSdlGraphicsManager::internUpdateScreen() {
	// Call to parent implementation of this method
	OpenGLGraphicsManager::internUpdateScreen();

	// Swap OpenGL buffers
	SDL_GL_SwapBuffers(); 
}

bool OpenGLSdlGraphicsManager::handleScalerHotkeys(Common::KeyCode key) {

	// Ctrl-Alt-a toggles aspect ratio correction
	if (key == 'a') {
		beginGFXTransaction();
			setAspectRatioCorrection(-1);
		endGFXTransaction();
#ifdef USE_OSD
		char buffer[128];
		sprintf(buffer, "Current aspect ratio mode: %s\n%d x %d -> %d x %d",
			getAspectRatioName().c_str(),
			_videoMode.screenWidth, _videoMode.screenHeight,
			_hwscreen->w, _hwscreen->h
			);
		displayMessageOnOSD(buffer);
#endif
		internUpdateScreen();
		return true;
	}

	// Ctrl-Alt-f toggles antialiasing
	if (key == 'f') {
		beginGFXTransaction();
			_videoMode.antialiasing = !_videoMode.antialiasing;
			_transactionDetails.filterChanged = true;
		endGFXTransaction();
#ifdef USE_OSD
		if (_videoMode.antialiasing)
			displayMessageOnOSD("Active filter mode: Linear");
		else
			displayMessageOnOSD("Active filter mode: Nearest");
#endif
		return true;
	}

	SDLKey sdlKey = (SDLKey)key;

	// Increase/decrease the scale factor
	if (sdlKey == SDLK_EQUALS || sdlKey == SDLK_PLUS || sdlKey == SDLK_MINUS ||
		sdlKey == SDLK_KP_PLUS || sdlKey == SDLK_KP_MINUS) {
		int factor = _videoMode.scaleFactor;
		factor += (sdlKey == SDLK_MINUS || sdlKey == SDLK_KP_MINUS) ? -1 : +1;
		if (0 < factor && factor < 4) {
			beginGFXTransaction();
				setScale(factor);
			endGFXTransaction();
#ifdef USE_OSD
			const char *newScalerName = 0;
			const OSystem::GraphicsMode *g = getSupportedGraphicsModes();
			while (g->name) {
				if (g->id == _videoMode.mode) {
					newScalerName = g->description;
					break;
				}
				g++;
			}
			if (newScalerName) {
				char buffer[128];
				sprintf(buffer, "Active graphics mode: %s\n%d x %d -> %d x %d",
					newScalerName,
					_videoMode.screenWidth, _videoMode.screenHeight,
					_hwscreen->w, _hwscreen->h
					);
				displayMessageOnOSD(buffer);
			}
#endif
			return true;
		}
	}
	return false;
}

void OpenGLSdlGraphicsManager::setFullscreenMode(bool enable) {
	if (_oldVideoMode.setup && _oldVideoMode.fullscreen == enable &&
		_oldVideoMode.activeFullscreenMode == _videoMode.activeFullscreenMode)
		return;

	if (_transactionMode == kTransactionActive) {
		_videoMode.fullscreen = enable;
		_transactionDetails.needHotswap = true;
	}
}

bool OpenGLSdlGraphicsManager::isScalerHotkey(const Common::Event &event) {
	if ((event.kbd.flags & (Common::KBD_CTRL|Common::KBD_ALT)) == (Common::KBD_CTRL|Common::KBD_ALT)) {
		const bool isScaleKey = (event.kbd.keycode == Common::KEYCODE_EQUALS || event.kbd.keycode == Common::KEYCODE_PLUS || event.kbd.keycode == Common::KEYCODE_MINUS ||
			event.kbd.keycode == Common::KEYCODE_KP_PLUS || event.kbd.keycode == Common::KEYCODE_KP_MINUS);

		return (isScaleKey || event.kbd.keycode == 'a' || event.kbd.keycode == 'f');
	}
	return false;
}

void OpenGLSdlGraphicsManager::toggleFullScreen(bool loop) {
	beginGFXTransaction();
		if (_videoMode.fullscreen && loop) {
			_videoMode.activeFullscreenMode += 1;
			setFullscreenMode(true);
		} else {
			_videoMode.activeFullscreenMode = -1;
			setFullscreenMode(!_videoMode.fullscreen);
		}
	endGFXTransaction();
#ifdef USE_OSD
	char buffer[128];
	if (_videoMode.fullscreen)
		sprintf(buffer, "Fullscreen mode\n%d x %d",
			_hwscreen->w, _hwscreen->h
			);
	else
		sprintf(buffer, "Windowed mode\n%d x %d",
			_hwscreen->w, _hwscreen->h
			);
	displayMessageOnOSD(buffer);
#endif
}

bool OpenGLSdlGraphicsManager::notifyEvent(const Common::Event &event) {
	switch ((int)event.type) {
	case Common::EVENT_KEYDOWN:
		// Alt-Return and Alt-Enter toggle full screen mode
		if (event.kbd.hasFlags(Common::KBD_ALT) &&
			(event.kbd.keycode == Common::KEYCODE_RETURN ||
			event.kbd.keycode == (Common::KeyCode)SDLK_KP_ENTER)) {
			toggleFullScreen(false);
			return true;
		}

		// Ctrl-Alt-Return and Ctrl-Alt-Enter switches between full screen modes
		if (event.kbd.hasFlags(Common::KBD_CTRL|Common::KBD_ALT) &&
			(event.kbd.keycode == Common::KEYCODE_RETURN ||
			event.kbd.keycode == (Common::KeyCode)SDLK_KP_ENTER)) {
			toggleFullScreen(true);
			return true;
		}

		// Alt-S: Create a screenshot
		if (event.kbd.hasFlags(Common::KBD_ALT) && event.kbd.keycode == 's') {
			char filename[20];

			for (int n = 0;; n++) {
				SDL_RWops *file;

				sprintf(filename, "scummvm%05d.bmp", n);
				file = SDL_RWFromFile(filename, "r");
				if (!file)
					break;
				SDL_RWclose(file);
			}
			if (saveScreenshot(filename))
				printf("Saved '%s'\n", filename);
			else
				printf("Could not save screenshot!\n");
			return true;
		}

		// Ctrl-Alt-<key> will change the GFX mode
		if (event.kbd.hasFlags(Common::KBD_CTRL|Common::KBD_ALT)) {
			if (handleScalerHotkeys(event.kbd.keycode))
				return true;
		}
	case Common::EVENT_KEYUP:
		return isScalerHotkey(event);
	/*case OSystem_SDL::kSdlEventExpose:
		break;*/
	// HACK: Handle special SDL event
	case OSystem_SDL::kSdlEventResize:
		beginGFXTransaction();
			// Set the new screen size. It is saved on the mouse event as part of HACK,
			// there is no common resize event
			_videoMode.hardwareWidth = event.mouse.x;
			_videoMode.hardwareHeight = event.mouse.y;
			_screenResized = true;
			_transactionDetails.sizeChanged = true;
			// The OpenGL context is not always destroyed during resizing,
			// however it is better to waste some time recreating it than
			// getting a blank screen
			_transactionDetails.newContext = true;
		endGFXTransaction();
		return true;

	default:
		break;
	}

	return OpenGLGraphicsManager::notifyEvent(event);
}

#endif
