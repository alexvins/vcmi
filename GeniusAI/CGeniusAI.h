#ifndef __CGENIUSAI_H__
#define __CGENIUSAI_H__

#include "Common.h"
#include "BattleLogic.h"
#include "GeneralAI.h"
#include "../../lib/CondSh.h"
#include <set>
#include <list>
#include <queue>

class CBuilding;

namespace GeniusAI {

enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};

class CGeniusAI : public CGlobalAI
{
private:
	ICallback*							m_cb;
	GeniusAI::BattleAI::CBattleLogic*	m_battleLogic;
	GeniusAI::GeneralAI::CGeneralAI		m_generalAI;
	
	CondSh<BattleState> m_state; //are we engaged into battle?

	class AIObjectContainer
	{
	public:
		AIObjectContainer(const CGObjectInstance * o):o(o){}
		const CGObjectInstance * o;
		bool operator<(const AIObjectContainer& b)const
		{
			if (o->pos!=b.o->pos)
				return o->pos<b.o->pos;
			return o->id<b.o->id;
		}
	};

	class HypotheticalGameState
	{
	public:
		HypotheticalGameState(){}
		HypotheticalGameState(CGeniusAI & AI)
			:knownVisitableObjects(AI.knownVisitableObjects)
		{
			std::vector < const CGHeroInstance *> heroes = AI.m_cb->getHeroesInfo();	
			for(std::vector < const CGHeroInstance *>::iterator i = heroes.begin(); i != heroes.end(); i++)
				heroModels.push_back(HeroModel(*i));
			
			std::vector < const CGTownInstance *> towns = AI.m_cb->getTownsInfo();	
			for(std::vector < const CGTownInstance *>::iterator i = towns.begin(); i != towns.end(); i++)
				townModels.push_back(TownModel(*i));

			if(AI.m_cb->howManyTowns()!=0)
				AvailableHeroesToBuy = AI.m_cb->getAvailableHeroes(AI.m_cb->getTownInfo(0,0));

			for(int i = 0; i < 8;i++)resourceAmounts.push_back(AI.m_cb->getResourceAmount(i));
		}

		class HeroModel
		{
		public:
			HeroModel(){}
			HeroModel(const CGHeroInstance * h):h(h){
				pos = h->getPosition(false);remainingMovement = h->movement;
			}
			int3 pos;
			int3 interestingPos;
			int remainingMovement;
			const CGHeroInstance * h;
		};
		class TownModel
		{
		public:
			TownModel(const CGTownInstance *t):t(t){hasBuilt = t->builded;creaturesToRecruit = t->creatures;}
			const CGTownInstance *t;
			std::vector<std::pair<ui32, std::vector<ui32> > > creaturesToRecruit;
			bool hasBuilt;
		};
		std::vector<const CGHeroInstance *> AvailableHeroesToBuy;
		std::vector<int> resourceAmounts;
		std::vector<HeroModel> heroModels;
		std::vector<TownModel> townModels;
		std::set< AIObjectContainer > knownVisitableObjects;
	};
	
	class AIObjective
	{
	public: 
		enum Type
		{
			//hero objectives
			visit,
			attack,
			flee,
			dismissUnits,
			dismissYourself,
			finishTurn,			//uses up remaining motion to get somewhere nice.

			//town objectives
			recruitHero,
			buildBuilding,
			recruitCreatures,
			upgradeCreatures
		};
		
		Type type;
		//virtual bool operator < (const AIObjective &)const=0;
		//virtual bool stillPossible(const HypotheticalGameState &)const = 0;
		virtual void fulfill(CGeniusAI &,HypotheticalGameState & hgs)=0;
		virtual HypotheticalGameState pretend(const HypotheticalGameState &) =0;
		virtual float getValue() const=0;	//how much is it worth to the AI to achieve
	};

	class HeroObjective: public AIObjective
	{
	public:
		int3 pos;
		const CGObjectInstance * object;
		std::vector<HypotheticalGameState::HeroModel *> whoCanAchieve;
		
		HeroObjective(){}
		HeroObjective(Type t):object(NULL){type = t;}
		HeroObjective(Type t,const CGObjectInstance * object,HypotheticalGameState::HeroModel *h):object(object)
		{
			pos = object->pos;
			type = t;
			whoCanAchieve.push_back(h);
			_value = 100 + rand()%30;
		}
		bool operator < (const HeroObjective &other)const
		{
			if(type != other.type)
				return type<other.type;
			if(pos!=other.pos)
				return pos < other.pos;
			if(object->id!=other.object->id)
				return object->id < other.object->id;
			return false;
		}
		void fulfill(CGeniusAI &,HypotheticalGameState & hgs);
		HypotheticalGameState pretend(const HypotheticalGameState &hgs){return hgs;};
		float getValue() const{return _value;}
	private:
		float _value;
	};

	//town objectives
		//recruitHero
		//buildBuilding
		//recruitCreatures
		//upgradeCreatures

	class TownObjective: public AIObjective
	{
	public:
		HypotheticalGameState::TownModel * whichTown;
		int which;				//which hero, which building, which creature, 

		TownObjective(Type t,HypotheticalGameState::TownModel * tn,int Which):whichTown(tn),which(Which){type = t;_value = 100 + rand()%30;}
		
		bool operator < (const TownObjective &other)const
		{
			if(type != other.type)
				return type<other.type;
			if(which!=other.which)
				return which<other.which;
			if(whichTown->t->id!=other.whichTown->t->id)
				return whichTown->t->id < other.whichTown->t->id;
			return false;
		}
		void fulfill(CGeniusAI &,HypotheticalGameState & hgs);
		HypotheticalGameState pretend(const HypotheticalGameState &hgs){return hgs;};
		float getValue() const {return _value;}
	private:
		float _value;
	};

	class AIObjectivePtrCont
	{
	public:
		AIObjectivePtrCont():obj(NULL){}
		AIObjectivePtrCont(AIObjective * obj):obj(obj){};
		AIObjective * obj;
		bool operator < (const AIObjectivePtrCont & other) const
		{return obj->getValue()<other.obj->getValue();}

	};
	HypotheticalGameState trueGameState;
	AIObjective * getBestObjective();
	void addHeroObjectives(HypotheticalGameState::HeroModel &h, HypotheticalGameState & hgs);
	void addTownObjectives(HypotheticalGameState::TownModel &h, HypotheticalGameState & hgs);
	void fillObjectiveQueue(HypotheticalGameState & hgs);
	
	void reportResources();
	int turn;
	bool firstTurn;
	std::set< AIObjectContainer > knownVisitableObjects;
	std::set<HeroObjective> currentHeroObjectives;	//can be fulfilled right now
	std::set<TownObjective> currentTownObjectives;
	std::vector<AIObjectivePtrCont> objectiveQueue;

public:
	CGeniusAI();
	virtual ~CGeniusAI();

	virtual void init(ICallback * CB);
	virtual void yourTurn();
	virtual void heroKilled(const CGHeroInstance *);
	virtual void heroCreated(const CGHeroInstance *);
	virtual void heroMoved(const TryMoveHero &);
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val) {};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void tileRevealed(int3 pos);
	virtual void tileHidden(int3 pos);
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd);
	virtual void playerBlocked(int reason);

	virtual void objectRemoved(const CGObjectInstance *obj); //eg. collected resource, picked artifact, beaten hero
	virtual void newObject(const CGObjectInstance * obj); //eg. ship built in shipyard
	
	
	// battle
	virtual void actionFinished(const BattleAction *action);//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action);//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(BattleAttack *ba); //called when stack is performing attack
	virtual void battleStacksAttacked(std::set<BattleStackAttacked> & bsa); //called when stack receives damage (after battleAttack())
	virtual void battleEnd(BattleResult *br);
	virtual void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(int ID, int dest, int distance, bool end);
	virtual void battleSpellCast(SpellCast *sc);
	virtual void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	//
	virtual void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving);
	virtual void battleStackAttacking(int ID, int dest);
	virtual void battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting);
	virtual BattleAction activeStack(int stackID);
	void battleResultsApplied();
};
}

#endif // __CGENIUSAI_H__
