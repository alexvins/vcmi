/*
 * COpenGLRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
 
#include "CAcceleratedRenderer.h"

namespace OpenGLRenderer
{
	class Window;
	
	class RenderTarget : public IRenderTarget
	{
	public:
		RenderTarget(Window * owner, int width, int height);
		virtual ~RenderTarget();
	
	protected:		
		Window * getWindow(){return window;};
	private:
		Window * window;
	};	
	
	class Window : public IWindow
	{
		
	};


	class Renderer : public CAcceleratedRenderer, public CWindowHolder<Window>
	{
	public:
		Renderer();
		virtual ~Renderer();
		
		///IRenderer
		IWindow * createWindow(const std::string & name, int w, int h, int bpp, bool fullscreen) override;	
		void init() override;	
	protected:
	private:
	};

}
