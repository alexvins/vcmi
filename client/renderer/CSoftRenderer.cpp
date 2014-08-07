/*
 * CSoftRenderer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <SDL_surface.h>
#include <SDL_video.h>
#include <SDL_mouse.h>

#ifndef VCMI_SDL1
#include <SDL_render.h>
#include <SDL_hints.h>

#endif // VCMI_SDL1

#include "../../lib/CConfigHandler.h"
#include "../gui/CIntObject.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/Geometries.h"


#include "CSoftRenderer.h"

namespace SoftRenderer
{
	///SurfaceProxy
	SurfaceProxy::SurfaceProxy(Renderer * owner): owner(owner), surface(nullptr)
	{

	}
	
	SurfaceProxy::~SurfaceProxy()
	{
		clear();
	}	
		
	void SurfaceProxy::activate()
	{
		getWindow()->setActiveTarget(this);
	}
	
	void SurfaceProxy::blitTo(SDL_Rect * srcRect, SDL_Rect * dstRect)
	{
		getWindow()->blit(surface, srcRect, dstRect);
	}	
	
	void SurfaceProxy::clear()
	{
		if(nullptr != surface)
		{
			SDL_FreeSurface(surface);
			surface = nullptr;
		}	
					
	}	
	
	bool SurfaceProxy::isActive()
	{
		return this == getWindow()->getActiveTarget();
	}	
	
	int SurfaceProxy::getWidth()
	{
		return nullptr == surface ? 0 : surface->w;
	}

	int SurfaceProxy::getHeight()
	{
		return nullptr == surface ? 0 : surface->h;
	}	
	
	SDL_PixelFormat * SurfaceProxy::getFormat()
	{
		return surface->format;
	}

	void SurfaceProxy::runActivated(const std::function<void(void)>& cb)
	{
		getWindow()->pushActiveTarget();
		activate();
		cb();
		getWindow()->popActiveTarget();		
	}	
	
	void SurfaceProxy::update()
	{
		#ifdef VCMI_SDL1
		SDL_UpdateRect(surface, 0, 0, surface->w, surface->h);	
		#else
		if(0 !=SDL_UpdateTexture(getWindow()->screenTexture, nullptr, surface->pixels, surface->pitch))
			logGlobal->errorStream() << __FUNCTION__ << "SDL_UpdateTexture " << SDL_GetError();		
		#endif // VCMI_SDL1		
	}	
	
	///RenderTarget
	RenderTarget::RenderTarget(Window * owner, int width, int height):
		SurfaceProxy(owner->getRenderer()), window(owner)
	{
		surface = CSDL_Ext::newSurface(width, height, owner->surface);
		#ifndef VCMI_SDL1
		//No blending for targets themselves 
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);	
		#endif // VCMI_SDL1		
	}

	RenderTarget::~RenderTarget()
	{
		
	}

	
	///Window
	Window::Window(Renderer * owner, const std::string & name):
		SurfaceProxy(owner),
		#ifndef VCMI_SDL1
		screenTexture(nullptr), sdlWindow(nullptr), sdlRenderer(nullptr), 
		#endif // VCMI_SDL1 
		name(name)
	{
		
	}

	Window::~Window()
	{
		
	}
	
	void Window::blit(SDL_Surface * what, int x, int y)
	{
		blitAt(what,x,y,activeTarget->surface);
	}
	
	void Window::blit(SDL_Surface* what, const SDL_Rect* srcrect, SDL_Rect* dstrect)
	{
		SDL_BlitSurface(what,srcrect, activeTarget->surface,dstrect);
	}
	
	
	void Window::clear()
	{
		SurfaceProxy::clear();
	
		if(nullptr != screenTexture)
		{
			SDL_DestroyTexture(screenTexture);
			screenTexture = nullptr;
		}
		
		if(nullptr != sdlRenderer)	
		{
			SDL_DestroyRenderer(sdlRenderer);
			sdlRenderer = nullptr;
		}
			
		if(nullptr != sdlWindow)
		{
			SDL_DestroyWindow(sdlWindow);
			sdlWindow = nullptr;
		}		
	}
	
	IRenderTarget * Window::createTarget(int width, int height)
	{
		return new RenderTarget(this, width, height);
	}
	
	void Window::fillWithColor(Uint32 color, SDL_Rect * dstRect)
	{
		SDL_Rect newRect;
		if (dstRect)
		{
			newRect = *dstRect;
		}
		else
		{
			newRect = Rect(0, 0, activeTarget->getWidth(), activeTarget->getHeight());
		}
		SDL_FillRect(activeTarget->surface, &newRect, color);
	}
	

	#ifndef VCMI_SDL1
	bool Window::recreate(int w, int h, int bpp, bool fullscreen)
	{
		auto checkVideoMode = [](int monitorIndex, int w, int h, int& bpp, bool fullscreen)-> bool
		{
			SDL_DisplayMode mode;
			const int modeCount = SDL_GetNumDisplayModes(monitorIndex);
			for (int i = 0; i < modeCount; i++) {
				SDL_GetDisplayMode(0, i, &mode);
				if (!mode.w || !mode.h || (w >= mode.w && h >= mode.h)) {
					return true;
				}
			}
			return false;
		};
		
		
		// VCMI will only work with 2 or 4 bytes per pixel	
		vstd::amax(bpp, 16);
		vstd::amin(bpp, 32);
		
		if(bpp > 16)
			bpp = 32;
		
		int suggestedBpp = bpp;

		if(!checkVideoMode(0,w,h,suggestedBpp,fullscreen))
		{
			logGlobal->errorStream() << "Error: SDL says that " << w << "x" << h << " resolution is not available!";
			return false;
		}	
			
		clear();
		
		
		if(fullscreen)
		{
			//in full-screen mode always use desktop resolution
			sdlWindow = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		}
		else
		{
			sdlWindow = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, w, h, 0);
		}
		
		
		
		if(nullptr == sdlWindow)
		{
			throw std::runtime_error("Unable to create window\n");
		}

		sdlRenderer = owner->createSDLRenderer(sdlWindow);

		SDL_RendererInfo info;
		SDL_GetRendererInfo(sdlRenderer,&info);
		logGlobal->infoStream() << "Created renderer " << info.name;	
		
		SDL_RenderSetLogicalSize(sdlRenderer, w, h);
		
		SDL_RenderSetViewport(sdlRenderer, nullptr);


		
		#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			int bmask = 0xff000000;
			int gmask = 0x00ff0000;
			int rmask = 0x0000ff00;
			int amask = 0x000000ff;
		#else
			int bmask = 0x000000ff;
			int gmask = 0x0000ff00;
			int rmask = 0x00ff0000;
			int amask = 0xFF000000;
		#endif

		surface = SDL_CreateRGBSurface(0,w,h,bpp,rmask,gmask,bmask,amask);
		if(nullptr == surface)
		{
			logGlobal->errorStream() << "Unable to create surface";
			logGlobal->errorStream() << w << " "<<  h << " "<< bpp;
			
			logGlobal->errorStream() << SDL_GetError();
			throw std::runtime_error("Unable to create surface");
		}	
		//No blending for screen itself. Required for proper cursor rendering.
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
		
		screenTexture = SDL_CreateTexture(sdlRenderer,
												SDL_PIXELFORMAT_ARGB8888,
												SDL_TEXTUREACCESS_STREAMING,
												w, h);

		if(nullptr == screenTexture)
		{
			logGlobal->errorStream() << "Unable to create screen texture";
			logGlobal->errorStream() << SDL_GetError();
			throw std::runtime_error("Unable to create screen texture");
		}	
			
		SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 0);
		SDL_RenderClear(sdlRenderer);
		SDL_RenderPresent(sdlRenderer);
			
		return true;		
	}
	#else
	void Window::setScreenRes(int w, int h, int bpp, bool fullscreen)
	{
		// VCMI will only work with 2, 3 or 4 bytes per pixel
		vstd::amax(bpp, 16);
		vstd::amin(bpp, 32);

		// Try to use the best screen depth for the display
		int suggestedBpp = SDL_VideoModeOK(w, h, bpp, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0));
		if(suggestedBpp == 0)
		{
			logGlobal->errorStream() << "Error: SDL says that " << w << "x" << h << " resolution is not available!";
			throw std::runtime_error("Requested screen resolution is not available\n");
		}


		if(suggestedBpp != bpp)
		{
			logGlobal->infoStream() << boost::format("Using %s bpp (bits per pixel) for the video mode. Default or overridden setting was %s bpp.") % suggestedBpp % bpp;
		}

		//For some reason changing fullscreen via config window checkbox result in SDL_Quit event

		if(surface) //screen has been already initialized
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		

		if((surface = SDL_SetVideoMode(w, h, suggestedBpp, SDL_SWSURFACE|(fullscreen?SDL_FULLSCREEN:0))) == nullptr)
		{
			logGlobal->errorStream() << "Requested screen resolution is not available (" << w << "x" << h << "x" << suggestedBpp << "bpp)";
			throw std::runtime_error("Requested screen resolution is not available\n");
		}

		logGlobal->infoStream() << "New screen flags: " << surface->flags;


		SDL_EnableUNICODE(1);
		SDL_WM_SetCaption(NAME.c_str(),""); //set window title
		SDL_ShowCursor(SDL_DISABLE);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	#ifdef _WIN32
		SDL_SysWMinfo wm;
		SDL_VERSION(&wm.version);
		int getwm = SDL_GetWMInfo(&wm);
		if(getwm == 1)
		{
			int sw = GetSystemMetrics(SM_CXSCREEN),
				sh = GetSystemMetrics(SM_CYSCREEN);
			RECT curpos;
			GetWindowRect(wm.window,&curpos);
			int ourw = curpos.right - curpos.left,
				ourh = curpos.bottom - curpos.top;
			SetWindowPos(wm.window, 0, (sw - ourw)/2, (sh - ourh)/2, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		}
		else
		{
			logGlobal->warnStream() << "Something went wrong, getwm=" << getwm;
			logGlobal->warnStream() << "SDL says: " << SDL_GetError();
			logGlobal->warnStream() << "Window won't be centered.";
		}
	#endif
		//TODO: centering game window on other platforms (or does the environment do their job correctly there?)
	}	
	#endif
	
	void Window::render(IShowable * object, bool total)
	{
		if(total)
			object->showAll(surface);
		else
			object->show(surface);
	}

	void Window::pushActiveTarget()
	{
		if(activeTarget != nullptr)
			targetStack.push(activeTarget);
		else
			logGlobal->debugStream() << "Window::pushActiveTarget() no active target";
	}

	void Window::popActiveTarget()
	{
		if(!targetStack.empty())
		{
			activeTarget = targetStack.top();
			targetStack.pop();
		}
		else
			logGlobal->debugStream() << "Window::popActiveTarget() no target to pop up";	
	}
	
		
	void Window::renderFrame(const std::function<void(void)>& cb)
	{
		cb();
		#ifndef	VCMI_SDL1
		if(0 != SDL_RenderCopy(sdlRenderer, screenTexture, nullptr, nullptr))
			logGlobal->errorStream() << __FUNCTION__ << " SDL_RenderCopy " << SDL_GetError();

		SDL_RenderPresent(sdlRenderer);				
		#endif			
	}

	bool Window::setFullscreen(bool enabled)
	{
		auto bitsPerPixel = surface->format->BitsPerPixel;	
			
		#ifdef VCMI_SDL1
		
		bitsPerPixel = SDL_VideoModeOK(surface->w, surface->h, bitsPerPixel, SDL_SWSURFACE|(enabled?SDL_FULLSCREEN:0));
		if(bitsPerPixel == 0)
		{
			logGlobal->errorStream() << "Error: SDL says that " << surface->w << "x" << surface->h << " resolution is not available!";
			return;
		}
		
		SDL_FreeSurface(surface);
		
		surface = SDL_SetVideoMode(surface->w, surface->h, bitsPerPixel, SDL_SWSURFACE|(enabled?SDL_FULLSCREEN:0));
		
		#else
		return recreate(surface->w,surface->h,bitsPerPixel,enabled);
		#endif		
	}

	void Window::warpMouse(int x, int y)
	{
		#ifdef VCMI_SDL1
		SDL_WarpMouse(int x, int y)
		#else		
		SDL_WarpMouseInWindow(sdlWindow,x,y);
		#endif
	}


	///Renderer
	Renderer::Renderer(): preferredDriverIndex(-1)
	{
		//ctor
	}

	Renderer::~Renderer()
	{
		//dtor
	}

	IWindow * Renderer::createWindow(const std::string & name, int w, int h, int bpp, bool fullscreen)
	{
		Window * window = new Window(this, name);
		
		#ifndef VCMI_SDL1
		if(!window->recreate(w, h, bpp, fullscreen))
			throw std::runtime_error("Requested screen resolution is not available\n");
		#else
		window->setScreenRes(w,h,bpp,fullscreen);
		#endif
		
		window->activate();
		
		return window;
	}
	
	void Renderer::init()
	{
		#ifndef VCMI_SDL1
		const JsonNode& video = settings["video"];
			
		int driversCount = SDL_GetNumRenderDrivers();
		std::string preferredDriverName = video["driver"].String();
		
		logGlobal->infoStream() << "Found " << driversCount << " render drivers";
		
		for(int it = 0; it < driversCount; it++)
		{
			SDL_RendererInfo info;
			SDL_GetRenderDriverInfo(it,&info);
			
			std::string driverName(info.name);
			
						
			logGlobal->infoStream() << "\t" << driverName;
			
			if(!preferredDriverName.empty() && driverName == preferredDriverName)
			{
				preferredDriverIndex = it;
				logGlobal->infoStream() << "\t\twill select this";
			}					
		}			
		#endif // VCMI_SDL1			
	}
	
	
	SDL_Renderer * Renderer::createSDLRenderer(SDL_Window * window)
	{
		
		//create first available renderer if preferred not set. Use no flags, so HW accelerated will be preferred but SW renderer also will possible
		SDL_Renderer * sdlRenderer = SDL_CreateRenderer(window, preferredDriverIndex,0);		
		if(nullptr == sdlRenderer)
		{
			throw std::runtime_error("Unable to create renderer");
		}
		
		return sdlRenderer;	
				
	}
	

}//namespace SoftRenderer
