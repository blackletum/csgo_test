﻿/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_CORE_ELEMENTBORDER_H
#define RMLUI_CORE_ELEMENTBORDER_H

#include "../../Include/RmlUi/Core/Geometry.h"

namespace Rml {

class Box;
class Element;

/**
	@author Peter Curry
 */

class ElementBorder
{
public:
	ElementBorder(Element* element);
	~ElementBorder();

	/// Renders the element's border, if it has one.
	void RenderBorder();

	/// Marks the border geometry as dirty.
	void DirtyBorder();

private:
	// Generates the border geometry for the element.
	void GenerateBorder();
	// Generates the border geometry for a single box.
	void GenerateBorder(Vertex*& vertices, int*& indices, int& index_offset, const Box& box, const Colourb* colours);

	Element* element;

	// The border geometry.
	Geometry geometry;

	bool border_dirty;
};

} // namespace Rml
#endif
