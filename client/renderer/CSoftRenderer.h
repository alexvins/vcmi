/*
 * CSoftRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <SDL_version.h>

#include "CBaseRenderer.h"

struct SDL_Surface;


namespace SoftRenderer
{
	class Window;
	class Renderer;
	class RenderTarget;
	
	class SurfaceProxy : public virtual IRenderTarget
	{
	public:
		SurfaceProxy(Renderer * owner);
		virtual ~SurfaceProxy();
		
		void activate() override;		
		void blitTo(SDL_Rect * srcRect, SDL_Rect * dstRect) override;
		bool isActive() override;		
		int getWidth() override;
		int getHeight() override;
		SDL_PixelFormat * getFormat() override;		
		
		Renderer * getRenderer(){return owner;};
		
		void saveAsBitmap(const std::string & fileName) override;
		
		void runActivated(const std::function<void(void)> & cb) override;
		
		void update() override;
		
	protected:
		Renderer * owner;
		SDL_Surface * surface;
		
		virtual void clear();
		virtual Window * getWindow() = 0; 
		
	private:
		friend class RenderTarget;
		friend class Window;		
	};
	
	class RenderTarget : public virtual SurfaceProxy
	{
	public:
		RenderTarget(Window * owner, int width, int height);
		virtual ~RenderTarget();
	
	protected:
		Window * window;
		
		Window * getWindow(){return window;};
	};
	
	
	class Window : public IWindow, public SurfaceProxy
	{
	public:
		Window(Renderer * owner, const std::string & name);
		virtual ~Window();
		
		///IRenderTarget
		void renderFrame(const std::function<void(void)> & cb) override;
		
		///IWindow
		void blit(SDL_Surface * what, int x, int y) override;
		void blit(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) override;			
		
		IRenderTarget * createTarget(int width, int height) override;

		void fillWithColor(Uint32 color, SDL_Rect * dstRect) override;
		bool setFullscreen(bool enabled) override;
		void warpMouse(int x, int y) override;		
		
		void render(IShowable * object, bool total);
			
		///internal interface
		#ifndef VCMI_SDL1		
		bool recreate(int w, int h, int bpp, bool fullscreen);
		#else
		void setScreenRes(int w, int h, int bpp, bool fullscreen);
		#endif

		SurfaceProxy * getActiveTarget();		
		void setActiveTarget(SurfaceProxy * target);
		
		void pushActiveTarget();
		void popActiveTarget();		

	protected:
		void clear() override;	
		Window * getWindow(){return this;};
		
	private:		
		std::stack<SurfaceProxy *> targetStack;
		
		SurfaceProxy * activeTarget;
		
		#ifndef VCMI_SDL1		
		SDL_Texture * screenTexture;

		SDL_Window * sdlWindow;
		SDL_Renderer * sdlRenderer;
		#endif

		std::string name;
		
		friend class SurfaceProxy;		
	};

	class Renderer : public CBaseRenderer
	{
	public:
		Renderer();
		virtual ~Renderer();
		
		///IRenderer
		IWindow * createWindow(const std::string & name, int w, int h, int bpp, bool fullscreen) override;
		
		void init() override;
		
		///internal interface
		#ifndef VCMI_SDL1
		SDL_Renderer * createSDLRenderer(SDL_Window * window);
		#endif // VCMI_SDL1
		
	protected:

	private:
		int preferredDriverIndex;	
	};
}
