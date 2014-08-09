#include "StdInc.h"
#include "CCursorHandler.h"

#include <SDL.h>
#include "SDL_Extensions.h"
#include "../CAnimation.h"
#include "CGuiHandler.h"
#include "../CMT.h"

#include "../renderer/IRenderer.h"

/*
 * CCursorHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CCursorHandler::initCursor()
{
	xpos = ypos = 0;
	type = ECursor::DEFAULT;
	dndObject = nullptr;
	currentCursor = nullptr;

	help = mainScreen->createTarget(40,40);

	SDL_ShowCursor(SDL_DISABLE);

	changeGraphic(ECursor::ADVENTURE, 0);
}

void CCursorHandler::changeGraphic(ECursor::ECursorTypes type, int index)
{
	std::string cursorDefs[4] = { "CRADVNTR.DEF", "CRCOMBAT.DEF", "CRDEFLT.DEF", "CRSPELL.DEF" };

	if (type != this->type)
	{
		BLOCK_CAPTURING; // not used here

		this->type = type;
		this->frame = index;

		delete currentCursor;
		currentCursor = new CAnimImage(cursorDefs[int(type)], index);
	}

	if (frame != index)
	{
		frame = index;
		currentCursor->setFrame(index);
	}
}

void CCursorHandler::dragAndDropCursor(CAnimImage * object)
{
	if (dndObject)
		delete dndObject;

	dndObject = object;
}

void CCursorHandler::cursorMove(const int & x, const int & y)
{
	xpos = x;
	ypos = y;
}

void CCursorHandler::drawWithScreenRestore()
{
	if(!showing) return;
	int x = xpos, y = ypos;
	shiftPos(x, y);

	SDL_Rect temp_rect1 = genRect(40,40,x,y);
	SDL_Rect temp_rect2 = genRect(40,40,0,0);
	
	help->runActivated([&](){
		mainScreen->blitTo(&temp_rect1,&temp_rect2);
	});

	if (dndObject)
	{
		dndObject->moveTo(Point(x - dndObject->pos.w/2, y - dndObject->pos.h/2));
		dndObject->showAll();
	}
	else
	{
		currentCursor->moveTo(Point(x,y));
		currentCursor->showAll();
	}
}

void CCursorHandler::drawRestored()
{
	if(!showing)
		return;

	int x = xpos, y = ypos;
	shiftPos(x, y);

	SDL_Rect temp_rect = genRect(40, 40, x, y);
	
	help->blitTo(nullptr, &temp_rect);		
}

void CCursorHandler::shiftPos( int &x, int &y )
{
	if(( type == ECursor::COMBAT && frame != ECursor::COMBAT_POINTER) || type == ECursor::SPELLBOOK)
	{
		x-=16;
		y-=16;

		// Properly align the melee attack cursors.
		if (type == ECursor::COMBAT)
		{
			switch (frame)
			{
			case 7: // Bottom left
				x -= 6;
				y += 16;
				break;
			case 8: // Left
				x -= 16;
				y += 10;
				break;
			case 9: // Top left
				x -= 6;
				y -= 6;
				break;
			case 10: // Top right
				x += 16;
				y -= 6;
				break;
			case 11: // Right
				x += 16;
				y += 11;
				break;
			case 12: // Bottom right
				x += 16;
				y += 16;
				break;
			case 13: // Below
				x += 9;
				y += 16;
				break;
			case 14: // Above
				x += 9;
				y -= 15;
				break;
			}
		}
	}
	else if(type == ECursor::ADVENTURE)
	{
		if (frame == 0); //to exclude
		else if(frame == 2)
		{
			x -= 12;
			y -= 10;
		}
		else if(frame == 3)
		{
			x -= 12;
			y -= 12;
		}
		else if(frame < 27)
		{
			int hlpNum = (frame - 4)%6;
			if(hlpNum == 0)
			{
				x -= 15;
				y -= 13;
			}
			else if(hlpNum == 1)
			{
				x -= 13;
				y -= 13;
			}
			else if(hlpNum == 2)
			{
				x -= 20;
				y -= 20;
			}
			else if(hlpNum == 3)
			{
				x -= 13;
				y -= 16;
			}
			else if(hlpNum == 4)
			{
				x -= 8;
				y -= 9;
			}
			else if(hlpNum == 5)
			{
				x -= 14;
				y -= 16;
			}
		}
		else if(frame == 41)
		{
			x -= 14;
			y -= 16;
		}
		else if(frame < 31 || frame == 42)
		{
			x -= 20;
			y -= 20;
		}
	}
}

void CCursorHandler::centerCursor()
{
	this->xpos = (mainScreen->getWidth() / 2.) - (currentCursor->pos.w / 2.);
	this->ypos = (mainScreen->getHeight() / 2.) - (currentCursor->pos.h / 2.);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	
	mainScreen->warpMouse(this->xpos, this->ypos);
	
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

void CCursorHandler::render()
{
	mainScreen->runActivated([this](){
		drawWithScreenRestore();
		mainScreen->update();
		drawRestored();	
	});
}


CCursorHandler::~CCursorHandler()
{
	delete help;
	delete currentCursor;
	delete dndObject;
}
