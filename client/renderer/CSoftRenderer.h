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
	
	class SurfaceProxy : public virtual IRenderTarget
	{
	public:
		SurfaceProxy(Renderer * owner);
		virtual ~SurfaceProxy();
		
		int getWidth() override;
		int getHeight() override;		
		
		Renderer * getRenderer(){return owner;};
		
		void render(IShowable * object, bool total);	
	protected:
		Renderer * owner;
		SDL_Surface * surface;
		
		virtual void clear();	
	};
	
	class RenderTarget : public virtual SurfaceProxy
	{
	public:
		RenderTarget(Window * owner);
		virtual ~RenderTarget();
		
		///IRenderTarget

		void activate() override;
		void render(const std::function<void(void)> & cb) override;
	protected:
		Window * window;
	};
	
	
	class Window : public IWindow, public SurfaceProxy
	{
	public:
		Window(Renderer * owner, const std::string & name);
		virtual ~Window();
		
		///IRenderTarget
		void activate() override;
		void render(const std::function<void(void)> & cb) override;
		
		///IWindow
		bool setFullscreen(bool enabled) override;
		void warpMouse(int x, int y) override;
		
		IRenderTarget * createTarget() override;
		
		void blit(SDL_Surface * what, int x, int y) override;
		void render(IShowable * object, bool total) override;
		
		
				
		///internal interface
		#ifndef VCMI_SDL1		
		bool recreate(int w, int h, int bpp, bool fullscreen);
		#else
		void setScreenRes(int w, int h, int bpp, bool fullscreen);
		#endif
		
		void setActiveTarget(RenderTarget * target){activeTarget = target;};
	protected:
		void clear() override;	
	private:
		
		SurfaceProxy * activeTarget;
		
		#ifndef VCMI_SDL1		
		SDL_Texture * screenTexture;

		SDL_Window * sdlWindow;
		SDL_Renderer * sdlRenderer;
		#endif

		std::string name;		
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
		SDL_Renderer * createSDLRenderer(SDL_Window * window);
	protected:

	private:
		int preferredDriverIndex;	
	};

}
