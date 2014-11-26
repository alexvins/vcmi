#include "StdInc.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../Graphics.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"

#include "../windows/CAdvmapInterface.h"
#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"

#include "../gui/SDL_Pixels.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CCursorHandler.h"

#include "../battle/CBattleInterface.h"
#include "../battle/CBattleInterfaceClasses.h"

#include "../../CCallback.h"

#include "../../lib/CGameState.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"

/*
 * InfoWindows.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CSimpleWindow::show()
{
	if(bitmap)
		bitmap->blitTo(nullptr,&pos);
}
CSimpleWindow::~CSimpleWindow()
{
	if (bitmap)
	{
		delete bitmap;
		bitmap=nullptr;
	}
}

void CSelWindow::selectionChange(unsigned to)
{
	for (unsigned i=0;i<components.size();i++)
	{
		CSelectableComponent * pom = dynamic_cast<CSelectableComponent*>(components[i]);
		if (!pom)
			continue;
		pom->select(i==to);
	}
	redraw();
}

CSelWindow::CSelWindow(const std::string &Text, PlayerColor player, int charperline, const std::vector<CSelectableComponent*> &comps, const std::vector<std::pair<std::string, CFunctionList<void()> > > &Buttons, QueryID askID)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	ID = askID;
	for (int i = 0; i < Buttons.size(); i++)
	{
		buttons.push_back(new CButton(Point(0, 0), Buttons[i].first, CButton::tooltip(), Buttons[i].second));
		if (!i  &&  askID.getNum() >= 0)
			buttons.back()->addCallback(std::bind(&CSelWindow::madeChoice, this));
		buttons[i]->addCallback(std::bind(&CInfoWindow::close, this)); //each button will close the window apart from call-defined actions
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);

	buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
	buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape

	if (buttons.size() > 1 && askID.getNum() >= 0) //cancel button functionality
	{
		buttons.back()->addCallback([askID]() {
			LOCPLINT->cb.get()->selectionMade(0, askID);
		});
		//buttons.back()->addCallback(std::bind(&CCallback::selectionMade, LOCPLINT->cb.get(), 0, askID));
	}

	for(int i=0;i<comps.size();i++)
	{
		comps[i]->recActions = 255;
		addChild(comps[i]);
		components.push_back(comps[i]);
		comps[i]->onSelect = std::bind(&CSelWindow::selectionChange,this,i);
		if(i<9)
			comps[i]->assignedKeys.insert(SDLK_1+i);
	}
	CMessage::drawIWindow(this, Text, player);
}

void CSelWindow::madeChoice()
{
	if(ID.getNum() < 0)
		return;
	int ret = -1;
	for (int i=0;i<components.size();i++)
	{
		if(dynamic_cast<CSelectableComponent*>(components[i])->selected)
		{
			ret = i;
		}
	}
	LOCPLINT->cb->selectionMade(ret+1,ID);
}

CInfoWindow::CInfoWindow(std::string Text, PlayerColor player, const TCompsInfo &comps, const TButtonsInfo &Buttons, bool delComps)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	type |= BLOCK_ADV_HOTKEYS;
	ID = QueryID(-1);
	for(auto & Button : Buttons)
	{
		CButton *button = new CButton(Point(0,0), Button.first, CButton::tooltip(), std::bind(&CInfoWindow::close,this));
		button->borderColor = Colors::METALLIC_GOLD;
		button->addCallback(Button.second); //each button will close the window apart from call-defined actions
		buttons.push_back(button);
	}

	text = new CTextBox(Text, Rect(0, 0, 250, 100), 0, FONT_MEDIUM, CENTER, Colors::WHITE);
	if(!text->slider)
	{
		text->resize(text->label->textSize);
	}

	if(buttons.size())
	{
		buttons.front()->assignedKeys.insert(SDLK_RETURN); //first button - reacts on enter
		buttons.back()->assignedKeys.insert(SDLK_ESCAPE); //last button - reacts on escape
	}

	for(auto & comp : comps)
	{
		comp->recActions = 0xff;
		addChild(comp);
		comp->recActions &= ~(SHOWALL | UPDATE);
		components.push_back(comp);
	}
	setDelComps(delComps);
	CMessage::drawIWindow(this,Text,player);
}

CInfoWindow::CInfoWindow()
{
	ID = QueryID(-1);
	setDelComps(false);
	text = nullptr;
}

void CInfoWindow::close()
{
	GH.popIntTotally(this);
	if(LOCPLINT)
		LOCPLINT->showingDialog->setn(false);
}

void CInfoWindow::show()
{
	CIntObject::show();
}

CInfoWindow::~CInfoWindow()
{
	if(!delComps)
	{
		for (auto & elem : components)
			removeChild(elem);
	}
}

void CInfoWindow::showAll()
{
	CSimpleWindow::show();
	CIntObject::showAll();
}

void CInfoWindow::showInfoDialog(const std::string &text, const std::vector<CComponent *> *components, bool DelComps, PlayerColor player)
{
	CInfoWindow * window = CInfoWindow::create(text, player, components, DelComps);
	GH.pushInt(window);
}

void CInfoWindow::showYesNoDialog(const std::string & text, const std::vector<CComponent*> *components, const CFunctionList<void( ) > &onYes, const CFunctionList<void()> &onNo, bool DelComps, PlayerColor player)
{
	assert(!LOCPLINT || LOCPLINT->showingDialog->get());
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("ICANCEL.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, components ? *components : std::vector<CComponent*>(), pom, DelComps);

	temp->buttons[0]->addCallback( onYes );
	temp->buttons[1]->addCallback( onNo );

	GH.pushInt(temp);
}

void CInfoWindow::showOkDialog(const std::string & text, const std::vector<CComponent*> *components, const std::function<void()> & onOk, bool delComps, PlayerColor player)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * temp = new CInfoWindow(text, player, *components, pom, delComps);
	temp->buttons[0]->addCallback(onOk);

	GH.pushInt(temp);
}

CInfoWindow * CInfoWindow::create(const std::string &text, PlayerColor playerID /*= 1*/, const std::vector<CComponent*> *components /*= nullptr*/, bool DelComps)
{
	std::vector<std::pair<std::string,CFunctionList<void()> > > pom;
	pom.push_back(std::pair<std::string,CFunctionList<void()> >("IOKAY.DEF",0));
	CInfoWindow * ret = new CInfoWindow(text, playerID, components ? *components : std::vector<CComponent*>(), pom, DelComps);
	return ret;
}

std::string CInfoWindow::genText(std::string title, std::string description)
{
	return std::string("{") + title + "}" + "\n\n" + description;
}

void CInfoWindow::setDelComps(bool DelComps)
{
	delComps = DelComps;
	for(CComponent *comp : components)
	{
		if(delComps)
			comp->recActions |= DISPOSE;
		else
			comp->recActions &= ~DISPOSE;
	}
}

CInfoPopup::CInfoPopup(IRenderTarget * Bitmap, int x, int y, bool Free)
 :free(Free),bitmap(Bitmap)
{
	init(x, y);
}


CInfoPopup::CInfoPopup(IRenderTarget * Bitmap, const Point &p, EAlignment alignment, bool Free/*=false*/)
 : free(Free),bitmap(Bitmap)
{
	switch(alignment)
	{
	case BOTTOMRIGHT:
		init(p.x - Bitmap->getWidth(), p.y - Bitmap->getHeight());
		break;
	case CENTER:
		init(p.x - Bitmap->getWidth()/2, p.y - Bitmap->getHeight()/2);
		break;
	case TOPLEFT:
		init(p.x, p.y);
		break;
	default:
		assert(0); //not implemented
	}
}

CInfoPopup::CInfoPopup(IRenderTarget *Bitmap, bool Free)
{
	CCS->curh->hide();

	free=Free;
	bitmap=Bitmap;

	if(bitmap)
	{
		pos.x = mainScreen->getWidth()/2 - bitmap->getWidth()/2;
		pos.y = mainScreen->getHeight()/2 - bitmap->getHeight()/2;
		pos.h = bitmap->getHeight();
		pos.w = bitmap->getWidth();
	}
}

void CInfoPopup::close()
{
	if(free)
		delete bitmap;
	GH.popIntTotally(this);
}

void CInfoPopup::show()
{
	bitmap->blitTo(nullptr, &pos);
}

CInfoPopup::~CInfoPopup()
{
	CCS->curh->show();
}

void CInfoPopup::init(int x, int y)
{
	CCS->curh->hide();

	pos.x = x;
	pos.y = y;
	pos.h = bitmap->getHeight();
	pos.w = bitmap->getWidth();

	// Put the window back on screen if necessary
	vstd::amax(pos.x, 0);
	vstd::amax(pos.y, 0);
	vstd::amin(pos.x, mainScreen->getWidth() - bitmap->getWidth());
	vstd::amin(pos.y, mainScreen->getHeight() - bitmap->getHeight());
}


void CRClickPopup::clickRight(tribool down, bool previousState)
{
	if(down)
		return;
	close();
}

void CRClickPopup::close()
{
	GH.popIntTotally(this);
}

void CRClickPopup::createAndPush(const std::string &txt, const CInfoWindow::TCompsInfo &comps)
{
	PlayerColor player = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1); //if no player, then use blue

	CSimpleWindow * temp = new CInfoWindow(txt, player, comps);
	temp->center(Point(GH.current->motion)); //center on mouse
	temp->fitToScreen(10);
	auto  rcpi = new CRClickPopupInt(temp,true);
	GH.pushInt(rcpi);
}

void CRClickPopup::createAndPush(const std::string &txt, CComponent * component)
{
	CInfoWindow::TCompsInfo intComps;
	intComps.push_back(component);

	createAndPush(txt, intComps);
}

void CRClickPopup::createAndPush(const CGObjectInstance *obj, const Point &p, EAlignment alignment /*= BOTTOMRIGHT*/)
{
	CIntObject *iWin = createInfoWin(p, obj); //try get custom infowindow for this obj
	if(iWin)
		GH.pushInt(iWin);
	else
	{
		if (adventureInt->curHero())
			CRClickPopup::createAndPush(obj->getHoverText(adventureInt->curHero()));
		else
			CRClickPopup::createAndPush(obj->getHoverText(LOCPLINT->playerID));
	}
}

CRClickPopup::CRClickPopup()
{
	addUsedEvents(RCLICK);
}

CRClickPopup::~CRClickPopup()
{
}

void CRClickPopupInt::show()
{
	inner->show();
}

CRClickPopupInt::CRClickPopupInt( IShowActivatable *our, bool deleteInt )
{
	CCS->curh->hide();
	inner = our;
	delInner = deleteInt;
}

CRClickPopupInt::~CRClickPopupInt()
{
	if(delInner)
		delete inner;

	CCS->curh->show();
}

void CRClickPopupInt::showAll()
{
	inner->showAll();
}

Point CInfoBoxPopup::toScreen(Point p)
{
	vstd::abetween(p.x, adventureInt->terrain.pos.x + 100, adventureInt->terrain.pos.x + adventureInt->terrain.pos.w - 100);
	vstd::abetween(p.y, adventureInt->terrain.pos.y + 100, adventureInt->terrain.pos.y + adventureInt->terrain.pos.h - 100);

	return p;
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGTownInstance * town):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(town, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CTownTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGHeroInstance * hero):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "HEROQVBK", toScreen(position))
{
	InfoAboutHero iah;
	LOCPLINT->cb->getHeroInfo(hero, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CHeroTooltip(Point(9, 10), iah);
}

CInfoBoxPopup::CInfoBoxPopup(Point position, const CGGarrison * garr):
	CWindowObject(RCLICK_POPUP | PLAYER_COLORED, "TOWNQVBK", toScreen(position))
{
	InfoAboutTown iah;
	LOCPLINT->cb->getTownInfo(garr, iah);

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CArmyTooltip(Point(9, 10), iah);
}

CIntObject * CRClickPopup::createInfoWin(Point position, const CGObjectInstance * specific) //specific=0 => draws info about selected town/hero
{
	if(!specific)
		specific = adventureInt->selection;

	assert(specific);

	switch(specific->ID)
	{
	case Obj::HERO:
		return new CInfoBoxPopup(position, dynamic_cast<const CGHeroInstance *>(specific));
	case Obj::TOWN:
		return new CInfoBoxPopup(position, dynamic_cast<const CGTownInstance *>(specific));
	case Obj::GARRISON:
	case Obj::GARRISON2:
		return new CInfoBoxPopup(position, dynamic_cast<const CGGarrison *>(specific));
	default:
		return nullptr;
	}
}
