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

#ifndef RMLUI_CORE_GEOMETRYUTILITIES_H
#define RMLUI_CORE_GEOMETRYUTILITIES_H

#include "Header.h"
#include "Types.h"
#include "Vertex.h"
#include "ComputedValues.h"

namespace Rml {

class Geometry;

/**
	A class containing helper functions for rendering geometry.

	@author Robert Curry
 */

class RMLUICORE_API GeometryUtilities
{
public:
	/// Generates a quad from a position, size and colour.
	/// @param[out] vertices An array of at least four vertices that the generated vertex data will be written into.
	/// @param[out] indices An array of at least six indices that the generated index data will be written into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] colour The colour to be assigned to each of the quad's vertices.
	/// @param[in] index_offset The offset to be added to the generated indices; this should be the number of vertices already in the array.
	static void GenerateQuad(Vertex* vertices, int* indices, const Vector2f& origin, const Vector2f& dimensions, const Colourb& colour, int index_offset = 0);
	/// Generates a quad from a position, size, colour and texture coordinates.
	/// @param[out] vertices An array of at least four vertices that the generated vertex data will be written into.
	/// @param[out] indices An array of at least six indices that the generated index data will be written into.
	/// @param[in] origin The origin of the quad to generate.
	/// @param[in] dimensions The dimensions of the quad to generate.
	/// @param[in] colour The colour to be assigned to each of the quad's vertices.
	/// @param[in] top_left_texcoord The texture coordinates at the top-left of the quad.
	/// @param[in] bottom_right_texcoord The texture coordinates at the bottom-right of the quad.
	/// @param[in] index_offset The offset to be added to the generated indices; this should be the number of vertices already in the array.
	static void GenerateQuad(Vertex* vertices, int* indices, const Vector2f& origin, const Vector2f& dimensions, const Colourb& colour, const Vector2f& top_left_texcoord, const Vector2f& bottom_right_texcoord, int index_offset = 0);
	
	/// Generates the geometry required to render a line above, below or through a line of text.
	/// @param[out] geometry The geometry to append the newly created geometry into.
	/// @param[in] position The position of the baseline of the lined text.
	/// @param[in] width The width of the string to line.
	/// @param[in] decoration_type The type for vertical positioning of line.
	/// @param[in] colour The colour to draw the line in.
	static void GenerateLine(FontFaceHandle font_face_handle, Geometry* geometry, const Vector2f& position, int width, Style::TextDecoration decoration_type, const Colourb& colour);

private:
	GeometryUtilities();
	~GeometryUtilities();
};

} // namespace Rml
#endif
