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

#include "../gui/SDL_Extensions.h"

struct SDL_Surface;
struct SDL_Overlay;
struct SDL_Texture;

namespace SoftRenderer
{
	class Window;
	class RenderTarget;
	class Renderer;
	class RenderTarget;
	
	class SurfaceProxy : public virtual IRenderTarget
	{
	public:
		SurfaceProxy(Renderer * owner);
		virtual ~SurfaceProxy();
		
		///IRenderTarget
		void activate() override;		
		void blitTo(SDL_Rect * srcRect, SDL_Rect * dstRect) override;
		bool isActive() override;		
		int getWidth() override;
		int getHeight() override;
		SDL_PixelFormat * getFormat() override;	
		
		void getClipRect(SDL_Rect * rect) override;
		void setClipRect(SDL_Rect * rect) override;	
		
		Renderer * getRenderer(){return owner;};
		
		void saveAsBitmap(const std::string & fileName) override;
		
		void runActivated(const std::function<void(void)> & cb) override;

		void update() override;
		
		///internal interface		
		void internalBlitRotation(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation);
		void internalBlitRotationAlpha(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation);		
		
	protected:
		Renderer * owner;
		SDL_Surface * surface;
		
		virtual void clear();
		virtual Window * getWindow() = 0; 
		void setSurface(SDL_Surface * newSurface);
		
	private:		
		BlitterWithRotationVal blitter, alphaBlitter;
		
		friend class RenderTarget;
		friend class Window;
		friend class EffectHandle;		
	};

	class EffectHandle: public IEffectHandle
	{
	public:
		EffectHandle(SurfaceProxy * target, const SDL_Rect * clipRect, EffectGuard::EffectType type);
		virtual ~EffectHandle();
			
	private:
		SurfaceProxy * target;
		SDL_Rect clipRect;
		EffectGuard::EffectType type;
	};
	
#ifndef DISABLE_VIDEO		
	
	class VideoOverlay: public IVideoOverlay
	{
	public:
		VideoOverlay(Window * owner, int width, int height);
		virtual ~VideoOverlay();
		
		void showFrame(AVFrame * frame, struct SwsContext * sws, AVCodecContext * codecContext) override;
		void presentFrame(SDL_Rect * pos) override;
		
	private:
		Window * owner;
		
	#ifdef VCMI_SDL1
		SDL_Overlay * overlay;
	#else
		SDL_Texture * overlay;
	#endif
		int width, height;
	};
	
#endif //DISABLE_VIDEO	
	
	class Window : public IWindow, public SurfaceProxy
	{
	public:
		Window(Renderer * owner, const std::string & name);
		virtual ~Window();
		
		///IRenderTarget
		void renderFrame(const std::function<void(void)> & cb) override;
		
		///IWindow		
		void accessActiveTarget(const std::function<void(SDL_Surface *)> & cb) override;
		
		IEffectHandle * applyEffect(const SDL_Rect * clipRect, EffectGuard::EffectType type) override;
				
		void blit(SDL_Surface * what, int x, int y) override;
		void blit(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) override;			
		void blitAlpha(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) override;
		
		void blitRotation(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation) override;
		void blitRotationAlpha(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation) override;
		
		IRenderTarget * createTarget(int width, int height) override;
		#ifndef DISABLE_VIDEO
		IVideoOverlay * createOverlay(int width, int height) override;
		#endif		
		
		void drawBorder(int x, int y, int w, int h, const SDL_Color &color) override;
		
		void getClipRect(SDL_Rect * rect, IRenderTarget *& currentActive) override;
		
		void fillRect(Uint32 color, SDL_Rect * dstRect) override;
		
		void fillRect(SDL_Color color, SDL_Rect * dstRect) override;
		
		bool setFullscreen(bool enabled) override;
		void warpMouse(int x, int y) override;		
		
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
		friend class VideoOverlay;				
	};
	
	
	class RenderTarget : public virtual SurfaceProxy, public virtual CRenderTargetBaseT<Window>
	{
	public:
		RenderTarget(Window * owner, int width, int height);
		virtual ~RenderTarget();
	protected:
		Window * getWindow() override{return CRenderTargetBaseT::getWindow();};
	};
		

	class Renderer : public CBaseRenderer, public CWindowHolder<Window>
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
