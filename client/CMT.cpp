// CMT.cpp : Defines the entry point for the console application.
//
#include "StdInc.h"
#include <SDL_mixer.h>
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "mapHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "CPreGame.h"
#include "CCastleInterface.h"
#include "../lib/CConsoleHandler.h"
#include "gui/CCursorHandler.h"
#include "../lib/CGameState.h"
#include "../CCallback.h"
#include "CPlayerInterface.h"
#include "CAdvmapInterface.h"
#include "../lib/CBuildingHandler.h"
#include "CVideoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CSpellHandler.h"
#include "CMusicHandler.h"
#include "CVideoHandler.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "Client.h"
#include "../lib/CConfigHandler.h"
#include "../lib/Connection.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "../lib/NetPacks.h"
#include "CMessage.h"
#include "../lib/CModHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CScriptingModule.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "../lib/logging/CBasicLogConfigurator.h"

#include "renderer/CSoftRenderer.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#endif
#include "../lib/UnlockGuard.h"
#include "CMT.h"

#if __MINGW32__
#undef main
#endif

namespace po = boost::program_options;

/*
 * CMT.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

std::string NAME_AFFIX = "client";
std::string NAME = GameConstants::VCMI_VERSION + std::string(" (") + NAME_AFFIX + ')'; //application name
CGuiHandler GH;
static CClient *client=nullptr;

IWindow * mainScreen = nullptr; //Main game window
IRenderTarget * bufferScreen = nullptr; //buffer used to render to not-active interfaces layer
IRenderer * renderEngine = nullptr;

extern boost::thread_specific_ptr<bool> inGuiThread;

std::queue<SDL_Event> events;
boost::mutex eventsM;

bool gNoGUI = false;
static po::variables_map vm;

//static bool setResolution = false; //set by event handling thread after resolution is adjusted

static bool ermInteractiveMode = false; //structurize when time is right
void processCommand(const std::string &message);

void dispose();
void playIntro();
static void mainLoop();
//void requestChangingResolution();
void startGame(StartInfo * options, CConnection *serv = nullptr);
void endGame();

#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

void startGameFromFile(const std::string &fname)
{
	StartInfo si;
	try //attempt retrieving start info from given file
	{
		if(!fname.size() || !boost::filesystem::exists(fname))
			throw std::runtime_error("Startfile \"" + fname + "\" does not exist!");

		CLoadFile out(fname);
		if(!out.sfile || !*out.sfile)
		{
			throw std::runtime_error("Cannot read from startfile \"" + fname + "\"!");
		}
		out >> si;
	}
	catch(std::exception &e)
	{
		logGlobal->errorStream() << "Failed to start from the file: " + fname << ". Error: " << e.what()
			<< " Falling back to main menu.";
		GH.curInt = CGPreGame::create();
		return;
	}

	while(GH.topInt())
		GH.popIntTotally(GH.topInt());
	startGame(&si);
}

void init()
{
	CStopWatch tmh, pomtime;

	loadDLLClasses();
	const_cast<CGameInfo*>(CGI)->setFromLib();

    logGlobal->infoStream()<<"Initializing VCMI_Lib: "<<tmh.getDiff();


	if(!gNoGUI)
	{
		pomtime.getDiff();
		CCS->curh = new CCursorHandler;
		graphics = new Graphics(); // should be before curh->init()

		CCS->curh->initCursor();
		CCS->curh->show();
		logGlobal->infoStream()<<"Screen handler: "<<pomtime.getDiff();
		pomtime.getDiff();

		graphics->loadHeroAnims();
		logGlobal->infoStream()<<"\tMain graphics: "<<pomtime.getDiff();
		logGlobal->infoStream()<<"Initializing game graphics: "<<tmh.getDiff();

		CMessage::init();
		logGlobal->infoStream()<<"Message handler: "<<tmh.getDiff();
	}
}

static void prog_version(void)
{
	printf("%s\n", GameConstants::VCMI_VERSION.c_str());
	std::cout << VCMIDirs::get().genHelpString();
}

static void prog_help(const po::options_description &opts)
{
	printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
    printf("Copyright (C) 2007-2014 VCMI dev team - see AUTHORS file\n");
    printf("This is free software; see the source for copying conditions. There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("\n");
	printf("Usage:\n");
	std::cout << opts;
// 	printf("  -h, --help        display this help and exit\n");
// 	printf("  -v, --version     display version information and exit\n");
}

#ifdef __APPLE__
void OSX_checkForUpdates();
#endif

#if defined(_WIN32) && !defined (__GNUC__)
int wmain(int argc, wchar_t* argv[])
#elif defined(__APPLE__)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char** argv)
#endif
{
#ifdef __APPLE__
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
    std::string executablePath = argv[0];
    std::string workDir = executablePath.substr(0, executablePath.rfind('/'));
    chdir(workDir.c_str());

    // Check for updates
    OSX_checkForUpdates();

    // Check that game data is prepared. Otherwise run vcmibuilder helper application
    FILE* check = fopen((VCMIDirs::get().userDataPath() + "/game_data_prepared").c_str(), "r");
    if (check == nullptr) {
        system("open ./vcmibuilder.app");
        return 0;
    }
    fclose(check);
#endif
    std::cout << "Starting... " << std::endl;
	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("version,v", "display version information and exit")
		("battle,b", po::value<std::string>(), "runs game in duel mode (battle-only")
		("start", po::value<std::string>(), "starts game from saved StartInfo file")
		("onlyAI", "runs without human player, all players will be default AI")
		("noGUI", "runs without GUI, implies --onlyAI")
		("ai", po::value<std::vector<std::string>>(), "AI to be used for the player, can be specified several times for the consecutive players")
		("oneGoodAI", "puts one default AI and the rest will be EmptyAI")
		("autoSkip", "automatically skip turns in GUI")
		("disable-video", "disable video player")
		("nointro,i", "skips intro movies");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), vm);
		}
		catch(std::exception &e)
		{
            std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}

	po::notify(vm);
	if(vm.count("help"))
	{
		prog_help(opts);
		return 0;
	}
	if(vm.count("version"))
	{
		prog_version();
		return 0;
	}
	if(vm.count("noGUI"))
	{
		gNoGUI = true;
		vm.insert(std::pair<std::string, po::variable_value>("onlyAI", po::variable_value()));
	}
#ifdef VCMI_SDL1
	//Set environment vars to make window centered. Sometimes work, sometimes not. :/
	putenv((char*)"SDL_VIDEO_WINDOW_POS");
	putenv((char*)"SDL_VIDEO_CENTERED=1");
#endif

	// Have effect on X11 system only (Linux).
	// For whatever reason in fullscreen mode SDL takes "raw" mouse input from DGA X11 extension
	// (DGA = Direct graphics access). Because this is raw input (before any speed\acceleration proceesing)
	// it may result in very small \ very fast mouse when game in fullscreen mode
	putenv((char*)"SDL_VIDEO_X11_DGAMOUSE=0");

    // Init old logging system and new (temporary) logging system
	CStopWatch total, pomtime;
	std::cout.flags(std::ios::unitbuf);
	console = new CConsoleHandler;
	*console->cb = std::bind(&processCommand, _1);
	console->start();
	atexit(dispose);

	const auto logPath = VCMIDirs::get().userCachePath() + "/VCMI_Client_log.txt";
	CBasicLogConfigurator logConfig(logPath, console);
    logConfig.configureDefault();
	logGlobal->infoStream() << "Creating console and configuring logger: " << pomtime.getDiff();
	logGlobal->infoStream() << "The log file will be saved to " << logPath;

#ifdef __ANDROID__
	// boost will crash without this
	setenv("LANG", "C", 1);
#endif
    // Init filesystem and settings
	preinitDLL(::console);
    settings.init();

    // Initialize logging based on settings
    logConfig.configure();

	// Some basic data validation to produce better error messages in cases of incorrect install
	auto testFile = [](std::string filename, std::string message) -> bool
	{
		if (CResourceHandler::get()->existsResource(ResourceID(filename)))
			return true;

        logGlobal->errorStream() << message << " was not found!";
		return false;
	};

	if (!testFile("DATA/HELP.TXT", "Heroes III data") ||
	    !testFile("MODS/VCMI/MOD.JSON", "VCMI mod") ||
	    !testFile("DATA/StackQueueBgBig.PCX", "VCMI data"))
		exit(EXIT_FAILURE); // These are unrecoverable errors

	// these two are optional + some installs have them on CD and not in data directory
	testFile("VIDEO/GOOD1A.SMK", "campaign movies");
	testFile("SOUNDS/G1A.WAV", "campaign music"); //technically not a music but voiced intro sounds

	conf.init();
    logGlobal->infoStream() <<"Loading settings: "<<pomtime.getDiff();
    logGlobal->infoStream() << NAME;

	srand ( time(nullptr) );


	const JsonNode& video = settings["video"];
	const JsonNode& res = video["screenRes"];

	
	if (res["width"].Float() < 100 || res["height"].Float() < 100)	
	{
		//something is really wrong...
        logGlobal->errorStream() << "[FATAL] Settings load failed!";
        logGlobal->errorStream() << "Possible reasons:";
        logGlobal->errorStream() << "\tCorrupted local configuration file at " << VCMIDirs::get().userConfigPath() << "/settings.json";
        logGlobal->errorStream() << "\tMissing or corrupted global configuration file at " << VCMIDirs::get().userConfigPath() << "/schemas/settings.json";
        logGlobal->errorStream() << "VCMI will now exit...";
		exit(EXIT_FAILURE);
	}

	if(!gNoGUI)
	{
		if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO))
		{
			logGlobal->errorStream() << "[FATAL] SDL_Init failed: " << SDL_GetError();
			exit(EXIT_FAILURE);
		}
		GH.mainFPSmng->init(); //(!)init here AFTER SDL_Init() while using SDL for FPS management
		atexit(SDL_Quit);
		
		//todo: put Renderer selection and initialization here
		renderEngine = new SoftRenderer::Renderer();//todo: use other backends
		renderEngine->init();
		
		mainScreen = renderEngine->createWindow(NAME, res["width"].Float(), res["height"].Float(), video["bitsPerPixel"].Float(), video["fullscreen"].Bool());
		
		bufferScreen = mainScreen->createTarget(mainScreen->getWidth(), mainScreen->getHeight());

		logGlobal->infoStream() <<"\tInitializing screen: "<<pomtime.getDiff();
	}

	CCS = new CClientState;
	CGI = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler etc.)
	// Initialize video
#ifdef DISABLE_VIDEO
	CCS->videoh = new CEmptyVideoPlayer;
#else
	if (!gNoGUI && !vm.count("disable-video"))
		CCS->videoh = new CVideoPlayer;
	else
		CCS->videoh = new CEmptyVideoPlayer;
#endif

    logGlobal->infoStream()<<"\tInitializing video: "<<pomtime.getDiff();

#if defined(__ANDROID__)
	//on Android threaded init is broken
	#define VCMI_NO_THREADED_LOAD
#endif // defined

	//initializing audio
	// Note: because of interface button range, volume can only be a
	// multiple of 11, from 0 to 99.
	CCS->soundh = new CSoundHandler;
	CCS->soundh->init();
	CCS->soundh->setVolume(settings["general"]["sound"].Float());
	CCS->musich = new CMusicHandler;
	CCS->musich->init();
	CCS->musich->setVolume(settings["general"]["music"].Float());
    logGlobal->infoStream()<<"Initializing screen and sound handling: "<<pomtime.getDiff();

#ifndef VCMI_NO_THREADED_LOAD
	//we can properly play intro only in the main thread, so we have to move loading to the separate thread
	boost::thread loading(init);
#else	 
	init();
#endif

	if(!gNoGUI )
	{
		if(!vm.count("battle") && !vm.count("nointro") && settings["video"]["showIntro"].Bool())
			playIntro();
			
		mainScreen->fillRect(0, nullptr);
		mainScreen->update();
	}

#ifndef VCMI_NO_THREADED_LOAD
	loading.join();
#endif
    logGlobal->infoStream()<<"Initialization of VCMI (together): "<<total.getDiff();

	if(!vm.count("battle"))
	{
		Settings session = settings.write["session"];
		session["autoSkip"].Bool()  = vm.count("autoSkip");
		session["oneGoodAI"].Bool() = vm.count("oneGoodAI");

		std::string fileToStartFrom; //none by default
		if(vm.count("start"))
			fileToStartFrom = vm["start"].as<std::string>();

		if(fileToStartFrom.size() && boost::filesystem::exists(fileToStartFrom))
			startGameFromFile(fileToStartFrom); //ommit pregame and start the game using settings from file
		else
		{
			if(fileToStartFrom.size())
			{
                logGlobal->warnStream() << "Warning: cannot find given file to start from (" << fileToStartFrom
                    << "). Falling back to main menu.";
			}
			GH.curInt = CGPreGame::create(); //will set CGP pointer to itself
		}
	}
	else
	{
		auto  si = new StartInfo();
		si->mode = StartInfo::DUEL;
		si->mapname = vm["battle"].as<std::string>();
		si->playerInfos[PlayerColor(0)].color = PlayerColor(0);
		si->playerInfos[PlayerColor(1)].color = PlayerColor(1);
		startGame(si);
	}

	if(!gNoGUI)
	{
		mainLoop();
	}
	else
	{
		while(true)
			boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}

	return 0;
}

void printInfoAboutIntObject(const CIntObject *obj, int level)
{
    std::stringstream sbuffer;
    sbuffer << std::string(level, '\t');

    sbuffer << typeid(*obj).name() << " *** ";
	if (obj->active)
	{
#define PRINT(check, text) if (obj->active & CIntObject::check) sbuffer << text
		PRINT(LCLICK, 'L');
		PRINT(RCLICK, 'R');
		PRINT(HOVER, 'H');
		PRINT(MOVE, 'M');
		PRINT(KEYBOARD, 'K');
		PRINT(TIME, 'T');
		PRINT(GENERAL, 'A');
		PRINT(WHEEL, 'W');
		PRINT(DOUBLECLICK, 'D');
#undef  PRINT
	}
	else
        sbuffer << "inactive";
    sbuffer << " at " << obj->pos.x <<"x"<< obj->pos.y;
    sbuffer << " (" << obj->pos.w <<"x"<< obj->pos.h << ")";
    logGlobal->infoStream() << sbuffer.str();

	for(const CIntObject *child : obj->children)
		printInfoAboutIntObject(child, level+1);
}

void processCommand(const std::string &message)
{
	std::istringstream readed;
	readed.str(message);
	std::string cn; //command name
	readed >> cn;

	if(LOCPLINT && LOCPLINT->cingconsole)
		LOCPLINT->cingconsole->print(message);

	if(ermInteractiveMode)
	{
		if(cn == "exit")
		{
			ermInteractiveMode = false;
			return;
		}
		else
		{
			if(client && client->erm)
				client->erm->executeUserCommand(message);
            std::cout << "erm>";
		}
	}
	else if(message==std::string("die, fool"))
	{
		exit(EXIT_SUCCESS);
	}
	else if(cn == "erm")
	{
		ermInteractiveMode = true;
        std::cout << "erm>";
	}
	else if(cn==std::string("activate"))
	{
		int what;
		readed >> what;
		switch (what)
		{
		case 0:
			GH.topInt()->activate();
			break;
		case 1:
			adventureInt->activate();
			break;
		case 2:
			LOCPLINT->castleInt->activate();
			break;
		}
	}
	else if(cn=="redraw")
	{
		GH.totalRedraw();
	}
	else if(cn=="screen")
	{
        logGlobal->errorStream() << "mainScreen " << (mainScreen->isActive() ? "active" : "inactive");
        logGlobal->errorStream() << "bufferScreen " << (bufferScreen->isActive() ? "active" : "inactive");
        
        mainScreen->saveAsBitmap("Screen_c.bmp");
        bufferScreen->saveAsBitmap("Screen2_c.bmp");
	}
	else if(cn=="save")
	{
		std::string fname;
		readed >> fname;
		client->save(fname);
	}
	else if(cn=="load")
	{
		// TODO: this code should end the running game and manage to call startGame instead
		std::string fname;
		readed >> fname;
		client->loadGame(fname);
    }
	else if(message=="get txt")
	{
        std::cout<<"Command accepted.\t";

		std::string outPath = VCMIDirs::get().userCachePath() + "/extracted/";

		auto list = CResourceHandler::get()->getFilteredFiles([](const ResourceID & ident)
		{
			return ident.getType() == EResType::TEXT && boost::algorithm::starts_with(ident.getName(), "DATA/");
		});

		for (auto & filename : list)
		{
			std::string outName = outPath + filename.getName();

			boost::filesystem::create_directories(outName.substr(0, outName.find_last_of("/")));

			std::ofstream file(outName + ".TXT");
			auto text = CResourceHandler::get()->load(filename)->readAll();

			file.write((char*)text.first.get(), text.second);
		}

        std::cout << "\rExtracting done :)\n";
        std::cout << " Extracted files can be found in " << outPath << " directory\n";
	}
	else if(cn=="crash")
	{
		int *ptr = nullptr;
		*ptr = 666;
		//disaster!
	}
	else if(cn == "onlyai")
	{
		vm.insert(std::pair<std::string, po::variable_value>("onlyAI", po::variable_value()));
	}
	else if (cn == "ai")
	{
		VLC->IS_AI_ENABLED = !VLC->IS_AI_ENABLED;
        std::cout << "Current AI status: " << (VLC->IS_AI_ENABLED ? "enabled" : "disabled") << std::endl;
	}
	else if(cn == "mp" && adventureInt)
	{
		if(const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
            std::cout << h->movement << "; max: " << h->maxMovePoints(true) << "/" << h->maxMovePoints(false) << std::endl;
	}
	else if(cn == "bonuses")
	{
		std::cout << "Bonuses of " << adventureInt->selection->getObjectName() << std::endl
			<< adventureInt->selection->getBonusList() << std::endl;

        std::cout << "\nInherited bonuses:\n";
		TCNodes parents;
		adventureInt->selection->getParents(parents);
		for(const CBonusSystemNode *parent : parents)
		{
            std::cout << "\nBonuses from " << typeid(*parent).name() << std::endl << parent->getBonusList() << std::endl;
		}
	}
	else if(cn == "not dialog")
	{
		LOCPLINT->showingDialog->setn(false);
	}
	else if(cn == "gui")
	{
		for(const IShowActivatable *child : GH.listInt)
		{
			if(const CIntObject *obj = dynamic_cast<const CIntObject *>(child))
				printInfoAboutIntObject(obj, 0);
			else
                std::cout << typeid(*obj).name() << std::endl;
		}
	}
	else if(cn=="tell")
	{
		std::string what;
		int id1, id2;
		readed >> what >> id1 >> id2;
		if(what == "hs")
		{
			for(const CGHeroInstance *h : LOCPLINT->cb->getHeroesInfo())
				if(h->type->ID.getNum() == id1)
					if(const CArtifactInstance *a = h->getArt(ArtifactPosition(id2)))
                        std::cout << a->nodeName();
		}
	}
	else if (cn == "set")
	{
		std::string what, value;
		readed >> what;

		Settings conf = settings.write["session"][what];

		readed >> value;
		if (value == "on")
			conf->Bool() = true;
		else if (value == "off")
			conf->Bool() = false;
	}
	else if(cn == "sinfo")
	{
		std::string fname;
		readed >> fname;
		if(fname.size() && SEL)
		{
			CSaveFile out(fname);
			out << SEL->sInfo;
		}
	}
	else if(cn == "start")
	{
		std::string fname;
		readed >> fname;
		startGameFromFile(fname);
	}
	else if(cn == "unlock")
	{
		std::string mxname;
		readed >> mxname;
		if(mxname == "pim" && LOCPLINT)
			LOCPLINT->pim->unlock();
	}
	else if(cn == "def2bmp")
	{
		std::string URI;
		readed >> URI;
		if (CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + URI)))
		{
			CDefEssential * cde = CDefHandler::giveDefEss(URI);

			std::string outName = URI;
			std::string outPath = VCMIDirs::get().userCachePath() + "/extracted/";

			boost::filesystem::create_directories(outPath + outName);

			for (size_t i=0; i<cde->ourImages.size(); i++)
			{
				std::string filename = outPath + outName + '/' + boost::lexical_cast<std::string>(i) + ".bmp";
				SDL_SaveBMP(cde->ourImages[i].bitmap, filename.c_str());
			}
		}
		else
			logGlobal->errorStream() << "File not found!";
	}
	else if(cn == "extract")
	{
		std::string URI;
		readed >> URI;

		if (CResourceHandler::get()->existsResource(ResourceID(URI)))
		{
			std::string outName = URI;
			std::string outPath = VCMIDirs::get().userCachePath() + "/extracted/";
			std::string fullPath = outPath + outName;

			auto data = CResourceHandler::get()->load(ResourceID(URI))->readAll();

			boost::filesystem::create_directories(fullPath.substr(0, fullPath.find_last_of("/")));
			std::ofstream outFile(outPath + outName, std::ofstream::binary);
			outFile.write((char*)data.first.get(), data.second);
		}
		else
			logGlobal->errorStream() << "File not found!";
	}
	else if(cn == "setBattleAI")
	{
		std::string fname;
		readed >> fname;
        std::cout << "Will try loading that AI to see if it is correct name...\n";
		try
		{
			if(auto ai = CDynLibHandler::getNewBattleAI(fname)) //test that given AI is indeed available... heavy but it is easy to make a typo and break the game
			{
				Settings neutralAI = settings.write["server"]["neutralAI"];
				neutralAI->String() = fname;
                std::cout << "Setting changed, from now the battle ai will be " << fname << "!\n";
			}
		}
		catch(std::exception &e)
		{
            logGlobal->warnStream() << "Failed opening " << fname << ": " << e.what();
            logGlobal->warnStream() << "Setting not changes, AI not found or invalid!";
		}
	}
	else if(cn == "autoskip")
	{
		Settings session = settings.write["session"];
		session["autoSkip"].Bool() = !session["autoSkip"].Bool();
	}
	else if(client && client->serv && client->serv->connected && LOCPLINT) //send to server
	{
		boost::unique_lock<boost::recursive_mutex> un(*LOCPLINT->pim);
		LOCPLINT->cb->sendMessage(message);
	}
}

//plays intro, ends when intro is over or button has been pressed (handles events)
void playIntro()
{
//	if(CCS->videoh->openAndPlayVideo("3DOLOGO.SMK", 60, 40, screen, true))
//	{
//		CCS->videoh->openAndPlayVideo("AZVS.SMK", 60, 80, screen, true);
//	}
}

void dispose()
{
	if (console)
		delete console;

	// cleanup, mostly to remove false leaks from analyzer
	CResourceHandler::clear();
	if (CCS)
	{
		CCS->musich->release();
		CCS->soundh->release();
	}
	CMessage::dispose();
}

static void fullScreenChanged()
{
	boost::unique_lock<boost::recursive_mutex> lock(*LOCPLINT->pim);

	Settings full = settings.write["video"]["fullscreen"];
	const bool toFullscreen = full->Bool();
	
	if(!mainScreen->setFullscreen(toFullscreen))
	{
		//will return false and report error if video mode is not supported
		return;	
	}	
	GH.totalRedraw();
}

static void handleEvent(SDL_Event & ev)
{
	if((ev.type==SDL_QUIT) ||(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4 && (ev.key.keysym.mod & KMOD_ALT)))
	{
		handleQuit();	
		return;
	}

	#ifdef VCMI_SDL1
	//FIXME: this should work even in pregame
	else if(LOCPLINT && ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
	#else
	else if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F4)
	#endif // VCMI_SDL1		
	{
		Settings full = settings.write["video"]["fullscreen"];
		full->Bool() = !full->Bool();
		return;
	}
	else if(ev.type == SDL_USEREVENT)
	{
		switch(ev.user.code)
		{
		case RETURN_TO_MAIN_MENU:
			{
				endGame();
				GH.curInt = CGPreGame::create();;
				GH.defActionsDef = 63;
			}
			break;
		case STOP_CLIENT:
			client->endGame(false);
			break;
		case RESTART_GAME:
			{
				StartInfo si = *client->getStartInfo(true);
				endGame();
				startGame(&si);
			}
			break;
		case PREPARE_RESTART_CAMPAIGN:
			{
				auto si = reinterpret_cast<StartInfo *>(ev.user.data1);
				endGame();
				startGame(si);
			}
			break;
		case RETURN_TO_MENU_LOAD:
			endGame();
			CGPreGame::create();
			GH.defActionsDef = 63;
			CGP->update();
			CGP->menu->switchToTab(vstd::find_pos(CGP->menu->menuNameToEntry, "load"));
			GH.curInt = CGP;
			break;
		case FULLSCREEN_TOGGLED:
			fullScreenChanged();
			break;
		default:
			logGlobal->errorStream() << "Unknown user event. Code " << ev.user.code;		
			break;	
		}

		return;
	}
	{
		boost::unique_lock<boost::mutex> lock(eventsM);
		events.push(ev);
	}	
	
}


static void mainLoop()
{
	SettingsListener resChanged = settings.listen["video"]["fullscreen"];
	resChanged([](const JsonNode &newState){  CGuiHandler::pushSDLEvent(SDL_USEREVENT, FULLSCREEN_TOGGLED); });

	inGuiThread.reset(new bool(true));
	GH.mainFPSmng->init();

	while(true) //main SDL events loop
	{
		SDL_Event ev;
		
		while(1 == SDL_PollEvent(&ev))
		{
			handleEvent(ev);
		}
		
		GH.renderFrame();

	}
}

void startGame(StartInfo * options, CConnection *serv/* = nullptr*/)
{
	if(vm.count("onlyAI"))
	{
		auto ais = vm.count("ai") ? vm["ai"].as<std::vector<std::string>>() : std::vector<std::string>();

		int i = 0;


		for(auto & elem : options->playerInfos)
		{
			elem.second.playerID = PlayerSettings::PLAYER_AI;
			if(i < ais.size())
				elem.second.name = ais[i++];
		}
	}

    client = new CClient;
	CPlayerInterface::howManyPeople = 0;
	switch(options->mode) //new game
	{
	case StartInfo::NEW_GAME:
	case StartInfo::CAMPAIGN:
	case StartInfo::DUEL:
		client->newGame(serv, options);
		break;
	case StartInfo::LOAD_GAME:
		std::string fname = options->mapname;
		boost::algorithm::erase_last(fname,".vlgm1");
		client->loadGame(fname);
		break;
	}

		client->connectionHandler = new boost::thread(&CClient::run, client);
}

void endGame()
{
	client->endGame();
	vstd::clear_pointer(client);
}

void handleQuit()
{
	auto quitApplication = []()
	{
		if(client)
			endGame();

		delete console;
		console = nullptr;
		boost::this_thread::sleep(boost::posix_time::milliseconds(750));
		if(!gNoGUI)
			SDL_Quit();

		std::cout << "Ending...\n";
		exit(EXIT_SUCCESS);
	};

	if(client && LOCPLINT)
	{
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[69], quitApplication, 0);
	}
	else
	{
		quitApplication();
	}
}
