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

#include <SDL_rect.h>

#include <boost/noncopyable.hpp>



struct SDL_Surface;
struct SDL_PixelFormat;


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

class ClipRectQuard: public boost::noncopyable
{
public:
	ClipRectQuard(IWindow * window, const SDL_Rect * newClipRect);
	virtual ~ClipRectQuard();	
private:
	IRenderTarget * target;
	SDL_Rect oldRect;
};

class IEffectHandle: public boost::noncopyable
{
public:
	virtual ~IEffectHandle(){};
};

class EffectGuard: public boost::noncopyable
{
public:
	enum EffectType 
	{
		SEPIA = 0,
		GRAYSCALE = 1
	};
	
	EffectGuard(IWindow * window, const SDL_Rect * clipRect, EffectType type);
	virtual ~EffectGuard();
private:	
	IEffectHandle * handle;
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
	
	virtual void getClipRect(SDL_Rect * rect) = 0;
	
	virtual SDL_PixelFormat * getFormat() = 0;

	virtual void runActivated(const std::function<void(void)> & cb) = 0;

	virtual void saveAsBitmap(const std::string & fileName) = 0;
	
	virtual void setClipRect(SDL_Rect * rect) = 0;	
		
	virtual void update() = 0;	
};

///OS window (with attached OGL context if applicable)
class IWindow: public virtual IRenderTarget
{
protected:	
	IWindow();
	
public:
	virtual ~IWindow();
	
    /** @brief Apply specified effect in active target for a lifetime of EffectHandle
     *
     * @param clipRect rectangle to limit effect to
     * @param type Type of effevt
     * @return handle of effect
     *
     */                             	
	virtual IEffectHandle * applyEffect(const SDL_Rect * clipRect, EffectGuard::EffectType type) = 0;

	///temporary, DEPRECATED. Blit surface to active target
	virtual void blit(SDL_Surface * what, int x, int y) = 0;	

	///temporary, DEPRECATED. Blit surface to active target
	virtual void blit(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) = 0; 
	
	///temporary, DEPRECATED. Blit surface to active target with enabled alpha blending
	virtual void blitAlpha(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect) = 0;
	
	///temporary, DEPRECATED.
	virtual void blitRotation(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation) = 0;
	
	///temporary, DEPRECATED.
	virtual void blitRotationAlpha(SDL_Surface * what, SDL_Rect * srcrect, SDL_Rect * dstrect, ui8 rotation) = 0;
	
    /** @brief Creates new rendering target
     * Target is created inactive
     *
     */                             	
	virtual IRenderTarget * createTarget(int width, int height) = 0;

	void drawBorder(const SDL_Rect &r, const SDL_Color &color)
	{
		drawBorder(r.x, r.y, r.w, r.h, color);
	};
	
	virtual void drawBorder(int x, int y, int w, int h, const SDL_Color &color) = 0;	
	
    /** @brief Fill target with specified color
     * @param color Color to will with in Target`s pixel format
     * @param dstRect Rect to will with, whole target will be used if null
     */
	virtual void fillRect(Uint32 color, SDL_Rect * dstRect) = 0;
	

    /** @brief Set window fullscreen mode
     *
     * @return true on success
     */    	
	virtual bool setFullscreen(bool enabled) = 0;	
	
	///get clip rect for active target and itself
	virtual void getClipRect(SDL_Rect * rect, IRenderTarget *& currentActive) = 0;
    
    /** @brief perform rendering of one frame
     * 
     * @param cb actual rendering implementation
     */                             	
	virtual void renderFrame(const std::function<void(void)> & cb) = 0;	
	
	virtual void warpMouse(int x, int y) = 0;		
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
