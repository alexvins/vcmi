#pragma once

#include "renderer/IRenderer.h"

extern IWindow * mainScreen;

#if 0
extern IWindow * splashScreen; //TODO: implement splash screen with intro video and threaded background loading (important for OGL)
#endif // 0

extern IRenderTarget * bufferScreen;

extern IRenderer * renderEngine;

extern bool gNoGUI; //if true there is no client window and game is silently played between AIs

void handleQuit(bool ask = true);
