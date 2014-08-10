/*
 * IRenderer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "IRenderer.h"

ClipRectGuard::ClipRectGuard(IWindow * window, SDL_Rect * newClipRect)
{
	window->getClipRect(&oldRect, target);
	target->setClipRect(newClipRect);
}

ClipRectGuard::~ClipRectGuard()
{
	target->setClipRect(&oldRect);
}

EffectGuard::EffectGuard(IWindow * window, const SDL_Rect * clipRect, EffectType type)
{
	handle = nullptr;
	if(type!=NO_EFFECT)
		handle = window->applyEffect(clipRect, type); 
};
EffectGuard::~EffectGuard()
{	
	delete handle;
};

///
IRenderTarget::IRenderTarget()
{
	
}

IRenderTarget::~IRenderTarget()
{
	
}

IWindow::IWindow()
{
	
}

IWindow::~IWindow()
{
	
}

IRenderer::IRenderer()
{
	
}

IRenderer::~IRenderer()
{
	
}

