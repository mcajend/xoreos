/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file graphics/guifrontelement.cpp
 *  A GUI element that is to be drawn in front of the normal objects.
 */

#include "graphics/guifrontelement.h"
#include "graphics/graphics.h"

namespace Graphics {

GUIFrontElement::GUIFrontElement() : Renderable(GfxMan.getGUIFrontQueue()) {
}

GUIFrontElement::~GUIFrontElement() {
}

} // End of namespace Graphics