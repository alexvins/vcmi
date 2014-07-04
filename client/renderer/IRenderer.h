/*
 * IRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/noncopyable.hpp>

class IRenderer;
class IRenderTarget;
class IWindow;

class ISprite
{
	
};

///"Screen" surface, FBO
class IRenderTarget: public boost::noncopyable
{
protected:
	IRenderTarget();
public:
	virtual ~IRenderTarget();	
	
    /** @brief direct all rendering to this target
     */
	virtual void activate() = 0;
};

///OS window (with attached OGL context if applicable)
class IWindow: public IRenderTarget
{
protected:
	IWindow();
public:
	
	virtual IRenderTarget * createTarget() = 0;	
};

///Class for managing shared data and global initialization
class IRenderer: public boost::noncopyable
{
protected:
	IRenderer();
public:
	virtual ~IRenderer();
	
	virtual IWindow * createWindow() = 0;	
	
	virtual void init() = 0;
	
};
