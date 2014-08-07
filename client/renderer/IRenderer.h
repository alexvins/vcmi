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
struct SDL_PixelFormat;
struct SDL_Rect;

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
		
    /** @brief Blit content of this target to active target
     *
     * Should not be called on active target
     * 
     * @param srcRect Define part of target to blit, nullptr for whole contents.
     */
	virtual void blitTo(SDL_Rect * srcRect, SDL_Rect * dstRect) = 0;	
	
	virtual bool isActive() = 0;

	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	
	virtual SDL_PixelFormat * getFormat() = 0;

	virtual void runActivated(const std::function<void(void)> & cb) = 0;
		
	virtual void update() = 0;
	
	virtual void saveAsBitmap(const std::string & fileName) = 0;
};

///OS window (with attached OGL context if applicable)
class IWindow: public virtual IRenderTarget
{
protected:	
	IWindow();
	
public:
	virtual ~IWindow();

	///temporary, DEPRECATED. Blit surface to active target
	virtual void blit(SDL_Surface * what, int x, int y) = 0;	

	///temporary, DEPRECATED. Blit surface to active target
	virtual void blit(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) = 0;    

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
	virtual IRenderTarget * createTarget(int width, int height) = 0;
	
    /** @brief Fill target with specified color
     * @param color Color to will with in Target`s pixel format
     * @param dstRect Rect to will with, whole target will be used if null
     */
	virtual void fillWithColor(Uint32 color, SDL_Rect * dstRect) = 0;	
    
    /** @brief perform rendering of one frame
     * 
     * @param cb actual rendering implementation
     */                             	
	virtual void renderFrame(const std::function<void(void)> & cb) = 0;

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
