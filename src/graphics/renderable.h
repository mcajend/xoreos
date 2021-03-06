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

/** @file graphics/renderable.h
 *  An object that can be displayed by the graphics manager.
 */

#ifndef GRAPHICS_RENDERABLE_H
#define GRAPHICS_RENDERABLE_H

#include "common/ustring.h"

#include "graphics/types.h"
#include "graphics/queueable.h"

namespace Graphics {

/** An object that can be displayed by the graphics manager. */
class Renderable : public Queueable {
public:
	Renderable(RenderableType type);
	~Renderable();

	bool operator<(const Queueable &q) const;

	/** Calculate the object's distance. */
	virtual void calculateDistance() = 0;

	/** Render the object. */
	virtual void render(RenderPass pass) = 0;

	/** Get the distance of the object from the viewer. */
	double getDistance() const;

	/** Get the object's unique ID. */
	uint32 getID() const;

	/** Is the object clickable? */
	bool isClickable() const;
	/** Set the object's clickable state. */
	void setClickable(bool clickable);

	/** Get the object's tag. */
	const Common::UString &getTag() const;
	/** Set the object's tag. */
	void setTag(const Common::UString &tag);

	bool isVisible() const; ///< Is the object visible?

	virtual void show(); ///< Show the object.
	virtual void hide(); ///< Hide the object.

	/** Is that point within the object? */
	virtual bool isIn(float x, float y) const;
	/** Is that point within the object? */
	virtual bool isIn(float x, float y, float z) const;
	/** Does the line from x1.y1.z1 to x2.y2.z2 intersect with the object? */
	virtual bool isIn(float x1, float y1, float z1, float x2, float y2, float z2) const;

protected:
	QueueType _queueExists;
	QueueType _queueVisible;

	uint32 _id;

	bool _clickable;
	Common::UString _tag;

	double _distance; ///< The distance of the object from the viewer.

	void resort();
};

} // End of namespace Graphics

#endif // GRAPHICS_RENDERABLE_H
