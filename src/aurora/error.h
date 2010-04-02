/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

#ifndef AURORA_ERROR_H
#define AURORA_ERROR_H

#include <exception>

namespace Aurora {

class GFFFieldTypeError : public std::exception {
	const char *what() const throw();
};

static const GFFFieldTypeError gffFieldTypeError;

} // End of namespace Aurora

#endif // AURORA_ERROR_H