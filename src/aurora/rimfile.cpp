/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file aurora/rimfile.cpp
 *  Handling BioWare's RIMs (encapsulated resource file).
 */

#include "common/stream.h"
#include "common/util.h"

#include "aurora/rimfile.h"

static const uint32 kRIMID     = MKID_BE('RIM ');
static const uint32 kVersion1  = MKID_BE('V1.0');

namespace Aurora {

RIMFile::RIMFile() {
}

RIMFile::~RIMFile() {
}

void RIMFile::clear() {
	AuroraBase::clear();

	_resources.clear();
}

bool RIMFile::load(Common::SeekableReadStream &rim) {
	clear();

	readHeader(rim);

	if (_id != kRIMID) {
		warning("RIMFile::load(): Not a RIM file");
		return false;
	}

	if (_version != kVersion1) {
		warning("RIMFile::load(): Unsupported file version");
		return false;
	}

	rim.skip(4);                            // Reserved
	uint32 resCount   = rim.readUint32LE(); // Number of resources in the RIM
	uint32 offResList = rim.readUint32LE(); // Offset to the resource list

	rim.skip(116); // Reserved

	_resources.resize(resCount);

	// Read the resource list
	if (!readResList(rim, offResList))
		return false;

	if (rim.err()) {
		warning("RIMFile::load(): Read error");
		return false;
	}

	return true;
}

bool RIMFile::readResList(Common::SeekableReadStream &rim, uint32 offset) {
	if (!rim.seek(offset))
		return false;

	for (ResourceList::iterator res = _resources.begin(); res != _resources.end(); ++res) {
		res->name   = AuroraFile::readRawString(rim, 16);
		res->type   = (FileType) rim.readUint16LE();
		rim.skip(4 + 2); // Resource ID + Reserved
		res->offset = rim.readUint32LE();
		res->size   = rim.readUint32LE();
	}

	return true;
}

const RIMFile::ResourceList &RIMFile::getResources() const {
	return _resources;
}

} // End of namespace Aurora