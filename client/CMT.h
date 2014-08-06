#pragma once

#include "renderer/IRenderer.h"

extern IWindow * mainScreen;

#if 0
extern IWindow * splashScreen; //TODO: implement splash screen with intro video and threaded background loading (important for OGL)
#endif // 0

extern IRenderTarget * bufferScreen;

extern IRenderer * renderEngine;

#ifndef VCMI_SDL1
#include <SDL_render.h>

extern SDL_Texture * screenTexture; //deprecated

extern SDL_Window * mainWindow; //deprecated
extern SDL_Renderer * mainRenderer; //deprecated

#endif // VCMI_SDL2

extern SDL_Surface *screen;  //deprecated     // main screen surface
extern SDL_Surface *screen2; //deprecated     // and hlp surface (used to store not-active interfaces layer)
extern SDL_Surface *screenBuf; //deprecated // points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed


extern bool gNoGUI; //if true there is no client window and game is silently played between AIs

void handleQuit();
