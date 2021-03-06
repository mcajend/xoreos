/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file engines/thewitcher/thewitcher.h
 *  Engine class handling The Witcher
 */

#ifndef ENGINES_THEWITCHER_THEWITCHER_H
#define ENGINES_THEWITCHER_THEWITCHER_H

#include "common/ustring.h"

#include "aurora/types.h"

#include "engines/engine.h"
#include "engines/engineprobe.h"

namespace Common {
	class FileList;
}

namespace Engines {

namespace TheWitcher {

class TheWitcherEngineProbe : public Engines::EngineProbe {
public:
	TheWitcherEngineProbe();
	~TheWitcherEngineProbe();

	Aurora::GameID getGameID() const;

	const Common::UString &getGameName() const;

	bool probe(const Common::UString &directory, const Common::FileList &rootFiles) const;
	bool probe(Common::SeekableReadStream &stream) const;

	Engines::Engine *createEngine() const;

	Aurora::Platform getPlatform() const { return Aurora::kPlatformWindows; }

private:
	static const Common::UString kGameName;
};

extern const TheWitcherEngineProbe kTheWitcherEngineProbe;

class TheWitcherEngine : public Engines::Engine {
public:
	TheWitcherEngine();
	~TheWitcherEngine();

	void run(const Common::UString &target);

private:
	Common::UString _baseDirectory;

	void init();
	void initCursors();
};

} // End of namespace TheWitcher

} // End of namespace Engines

#endif // ENGINES_THEWITCHER_THEWITCHER_H
