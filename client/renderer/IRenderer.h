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

struct SDL_Surface;

class IShowable;

class IRenderer;
class IRenderTarget;
class IWindow;



/** @brief Single image, icon set or animation 
 *
 * In single texture atlas
 */
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
     *
     * Only one rendering target can be active in window
     */
	virtual void activate() = 0; 

	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
  
    /** @brief perform rendering of one frame
     * 
     * @param cb actual rendering implementation
     */                             	
	virtual void render(const std::function<void(void)> & cb) = 0;		
};

///OS window (with attached OGL context if applicable)
class IWindow: public virtual IRenderTarget
{
protected:	
	IWindow();
public:
	virtual ~IWindow();
    /** @brief Set window fullscreen mode
     *
     * @return true on success
     */                             
	
	virtual bool setFullscreen(bool enabled) = 0;	
	
	virtual void warpMouse(int x, int y) = 0;
    
    /** @brief Creates new rendering target
     * Target is created inactive
     *
     */                             	
	virtual IRenderTarget * createTarget() = 0;	

	///temporary, DEPRECATED. Blit surface to active target
	virtual void blit(SDL_Surface * what, int x, int y) = 0;	
	
	///temporary, DEPRECATED. Blit object to active target
	virtual void render(IShowable * object, bool total) = 0;		
};

///Class for managing shared data and global initialization
class IRenderer: public boost::noncopyable
{
protected:
	IRenderer();
public:
	
	virtual ~IRenderer();
	
	virtual IWindow * createWindow(const std::string & name, int w, int h, int bpp, bool fullscreen) = 0;	
	
	virtual void init() = 0;
	
};
