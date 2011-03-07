/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file engines/nwn/module.cpp
 *  The context needed to run a NWN module.
 */

#include "common/util.h"
#include "common/error.h"
#include "common/configman.h"

#include "events/events.h"

#include "aurora/2dareg.h"

#include "graphics/camera.h"

#include "graphics/aurora/textureman.h"

#include "engines/aurora/util.h"
#include "engines/aurora/resources.h"

#include "engines/nwn/module.h"
#include "engines/nwn/types.h"
#include "engines/nwn/area.h"

#include "engines/nwn/menu/ingamemain.h"

namespace Engines {

namespace NWN {

Module::Module() : _hasModule(false), _hasPC(false), _currentTexturePack(-1),
	_exit(false), _area(0) {

}

Module::~Module() {
	clear();
}

void Module::clear() {
	unload();
}

bool Module::loadModule(const Common::UString &module) {
	unloadModule();

	if (module.empty())
		return false;

	try {
		indexMandatoryArchive(Aurora::kArchiveERF, module, 100, &_resModule);

		_ifo.load();

		if (_ifo.isSave())
			throw Common::Exception("This is a save");

		checkXPs();
		checkHAKs();

		_ifo.loadTLK();

	} catch (Common::Exception &e) {
		e.add("Can't load module \"%s\"", module.c_str());
		printException(e, "WARNING: ");
		return false;
	}

	_hasModule = true;
	return true;
}

void Module::checkXPs() {
	uint16 hasXP = 0;

	hasXP |= ConfigMan.getBool("NWN_hasXP1") ? 1 : 0;
	hasXP |= ConfigMan.getBool("NWN_hasXP2") ? 2 : 0;
	hasXP |= ConfigMan.getBool("NWN_hasXP3") ? 4 : 0;

	uint16 xp = _ifo.getExpansions();

	for (int i = 0; i < 16; i++, xp >>= 1, hasXP >>= 1)
		if ((xp & 1) && !(hasXP & 1))
			throw Common::Exception("Module requires expansion %d and we don't have it", i + 1);
}

void Module::checkHAKs() {
	const std::vector<Common::UString> &haks = _ifo.getHAKs();

	for (std::vector<Common::UString>::const_iterator h = haks.begin(); h != haks.end(); ++h)
		if (!ResMan.hasArchive(Aurora::kArchiveERF, *h + ".hak"))
			throw Common::Exception("Required hak \"%s\" does not exist", h->c_str());
}

bool Module::usePC(const CharacterID &c) {
	unloadPC();

	if (c.empty())
		return false;

	_pc = *c;

	_hasPC = true;
	return true;
}

void Module::run() {
	if (!_hasModule) {
		warning("Module::run(): Lacking a module?!?");
		return;
	}

	if (!_hasPC) {
		warning("Module::run(): Lacking a PC?!?");
		return;
	}

	loadTexturePack();

	status("Running module \"%s\" with character \"%s\"",
			_ifo.getName().getFirstString().c_str(), _pc.getFullName().c_str());

	try {

		loadHAKs();

	} catch (Common::Exception &e) {
		e.add("Can't initialize module \"%s\"", _ifo.getName().getFirstString().c_str());
		printException(e, "WARNING: ");
		return;
	}

	Common::UString startMovie = _ifo.getStartMovie();
	if (!startMovie.empty())
		playVideo(startMovie);

	_exit    = false;
	_newArea = _ifo.getEntryArea();

	// Roughly head position
	CameraMan.reset();
	CameraMan.setPosition(0.0, 2.0, 0.0);

	EventMan.enableKeyRepeat();

	try {

		while (!EventMan.quitRequested() && !_exit && !_newArea.empty()) {
			loadArea();

			Events::Event event;
			while (EventMan.pollEvent(event)) {
				if (event.type == Events::kEventKeyDown) {
					if      (event.key.keysym.sym == SDLK_ESCAPE)
						showMenu();
					else if (event.key.keysym.sym == SDLK_UP)
						CameraMan.move( 0.5);
					else if (event.key.keysym.sym == SDLK_DOWN)
						CameraMan.move(-0.5);
					else if (event.key.keysym.sym == SDLK_RIGHT)
						CameraMan.turn( 0.0,  5.0, 0.0);
					else if (event.key.keysym.sym == SDLK_LEFT)
						CameraMan.turn( 0.0, -5.0, 0.0);
					else if (event.key.keysym.sym == SDLK_w)
						CameraMan.move( 0.5);
					else if (event.key.keysym.sym == SDLK_s)
						CameraMan.move(-0.5);
					else if (event.key.keysym.sym == SDLK_d)
						CameraMan.turn( 0.0,  5.0, 0.0);
					else if (event.key.keysym.sym == SDLK_a)
						CameraMan.turn( 0.0, -5.0, 0.0);
					else if (event.key.keysym.sym == SDLK_e)
						CameraMan.strafe( 0.5);
					else if (event.key.keysym.sym == SDLK_q)
						CameraMan.strafe(-0.5);
					else if (event.key.keysym.sym == SDLK_INSERT)
						CameraMan.move(0.0,  0.5, 0.0);
					else if (event.key.keysym.sym == SDLK_DELETE)
						CameraMan.move(0.0, -0.5, 0.0);
					else if (event.key.keysym.sym == SDLK_PAGEUP)
						CameraMan.turn( 5.0,  0.0, 0.0);
					else if (event.key.keysym.sym == SDLK_PAGEDOWN)
						CameraMan.turn(-5.0,  0.0, 0.0);
					else if (event.key.keysym.sym == SDLK_END) {
						const float *orient = CameraMan.getOrientation();

						CameraMan.setOrientation(0.0, orient[1], orient[2]);
					}
				}
			}

			EventMan.delay(10);
		}

	} catch (Common::Exception &e) {
		e.add("Failed running module \"%s\"", _ifo.getName().getFirstString().c_str());
		printException(e, "WARNING: ");
	}

	EventMan.enableKeyRepeat(0);
}

void Module::unload() {
	unloadArea();
	unloadTexturePack();
	unloadHAKs();
	unloadPC();
	unloadModule();
}

void Module::unloadModule() {
	TwoDAReg.clear();

	_ifo.unload();

	ResMan.undo(_resModule);

	_hasModule = false;
}

void Module::unloadPC() {
	_pc.clear();

	_hasPC = false;
}

void Module::loadHAKs() {
	const std::vector<Common::UString> &haks = _ifo.getHAKs();

	_resHAKs.resize(haks.size());

	for (uint i = 0; i < haks.size(); i++)
		indexMandatoryArchive(Aurora::kArchiveERF, haks[i] + ".hak", 100, &_resHAKs[i]);
}

void Module::unloadHAKs() {
	std::vector<Aurora::ResourceManager::ChangeID>::iterator hak;
	for (hak = _resHAKs.begin(); hak != _resHAKs.end(); ++hak)
		ResMan.undo(*hak);

	_resHAKs.clear();
}

static const char *texturePacks[4][4] = {
	{ "textures_tpc.erf", "tiles_tpc.erf", "xp1_tex_tpc.erf", "xp2_tex_tpc.erf" }, // Worst
	{ "textures_tpa.erf", "tiles_tpc.erf", "xp1_tex_tpc.erf", "xp2_tex_tpc.erf" }, // Bad
	{ "textures_tpa.erf", "tiles_tpb.erf", "xp1_tex_tpb.erf", "xp2_tex_tpb.erf" }, // Okay
	{ "textures_tpa.erf", "tiles_tpa.erf", "xp1_tex_tpa.erf", "xp2_tex_tpa.erf" }  // Best
};

void Module::loadTexturePack() {
	int level = ConfigMan.getInt("texturepack", 1);
	if (_currentTexturePack == level)
		// Nothing to do
		return;

	unloadTexturePack();

	status("Loading texture pack %d", level);
	indexMandatoryArchive(Aurora::kArchiveERF, texturePacks[level][0], 13, &_resTP[0]);
	indexMandatoryArchive(Aurora::kArchiveERF, texturePacks[level][1], 14, &_resTP[1]);
	indexOptionalArchive (Aurora::kArchiveERF, texturePacks[level][2], 15, &_resTP[2]);
	indexOptionalArchive (Aurora::kArchiveERF, texturePacks[level][3], 16, &_resTP[3]);

	// If we already had a texture pack loaded, reload all textures
	if (_currentTexturePack != -1)
		TextureMan.reloadAll();

	_currentTexturePack = level;
}

void Module::unloadTexturePack() {
	for (int i = 0; i < 4; i++)
		ResMan.undo(_resTP[i]);
}

void Module::loadArea() {
	if (_area && (_area->getResRef() == _newArea))
		return;

	delete _area;

	_area = new Area(*this, _newArea);

	_area->show();

	status("Entered area \"%s\", (\"%s\")",
			_area->getName().c_str(), _area->getResRef().c_str());
}

void Module::unloadArea() {
	delete _area;
	_area = 0;
}

void Module::showMenu() {
	InGameMainMenu menu;

	menu.show();
	int code = menu.run();
	menu.hide();

	if (code == 2) {
		_exit = true;
		return;
	}

	// In case we changed the texture pack settings, reload it
	loadTexturePack();
}

} // End of namespace NWN

} // End of namespace Engines